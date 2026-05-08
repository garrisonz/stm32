#include <stdint.h>

#include "oled_display.h"

#define RCC_BDCR             (*(volatile uint32_t *)0x40021020u)
#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define PWR_CR               (*(volatile uint32_t *)0x40007000u)
#define BKP_DR3              (*(volatile uint32_t *)0x40006C0Cu)

#define RTC_CRH              (*(volatile uint32_t *)0x40002800u)
#define RTC_CRL              (*(volatile uint32_t *)0x40002804u)
#define RTC_PRLH             (*(volatile uint32_t *)0x40002808u)
#define RTC_PRLL             (*(volatile uint32_t *)0x4000280Cu)
#define RTC_DIVH             (*(volatile uint32_t *)0x40002810u)
#define RTC_DIVL             (*(volatile uint32_t *)0x40002814u)
#define RTC_CNTH             (*(volatile uint32_t *)0x40002818u)
#define RTC_CNTL             (*(volatile uint32_t *)0x4000281Cu)

#define RCC_APB1ENR_BKPEN    (1u << 27)
#define RCC_APB1ENR_PWREN    (1u << 28)
#define PWR_CR_DBP           (1u << 8)

#define RCC_BDCR_LSEON       (1u << 0)
#define RCC_BDCR_LSERDY      (1u << 1)
#define RCC_BDCR_RTCSEL_LSE  (1u << 8)
#define RCC_BDCR_RTCEN       (1u << 15)

#define RTC_CRL_RSF          (1u << 3)
#define RTC_CRL_CNF          (1u << 4)
#define RTC_CRL_RTOFF        (1u << 5)

#define RTC_INIT_SIGNATURE   0x5256u
#define RTC_EPOCH_YEAR       2000u
#define RTC_INIT_YEAR        2026u
#define RTC_INIT_MONTH       5u
#define RTC_INIT_DAY         8u
#define RTC_INIT_HOUR        9u
#define RTC_INIT_MINUTE      38u
#define RTC_INIT_SECOND      5u
#define LSE_START_TIMEOUT    8000000u
#define RTC_PRESCALER        32767u

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
} RtcDateTime;

static void delay(volatile uint32_t cycles)
{
    while (cycles-- > 0u) {
    }
}

static void backup_domain_enable_write(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR_CR |= PWR_CR_DBP;
}

static int wait_rtc_write_done(void)
{
    uint32_t timeout = 1000000u;

    while ((RTC_CRL & RTC_CRL_RTOFF) == 0u) {
        if (timeout-- == 0u) {
            return 0;
        }
    }

    return 1;
}

static int wait_rtc_sync(void)
{
    uint32_t timeout = 1000000u;

    RTC_CRL &= ~RTC_CRL_RSF;
    while ((RTC_CRL & RTC_CRL_RSF) == 0u) {
        if (timeout-- == 0u) {
            return 0;
        }
    }

    return 1;
}

static int rtc_enter_config(void)
{
    if (!wait_rtc_write_done()) {
        return 0;
    }

    RTC_CRL |= RTC_CRL_CNF;
    return 1;
}

static int rtc_exit_config(void)
{
    RTC_CRL &= ~RTC_CRL_CNF;
    return wait_rtc_write_done();
}

static void rtc_write_counter(uint32_t value)
{
    RTC_CNTH = value >> 16u;
    RTC_CNTL = value & 0xFFFFu;
}

static int is_leap_year(uint32_t year)
{
    return ((year % 4u) == 0u && (year % 100u) != 0u) ||
           ((year % 400u) == 0u);
}

static uint32_t days_in_month(uint32_t year, uint32_t month)
{
    static const uint8_t days_by_month[] = {
        31u, 28u, 31u, 30u, 31u, 30u,
        31u, 31u, 30u, 31u, 30u, 31u,
    };

    if (month == 2u && is_leap_year(year)) {
        return 29u;
    }

    return days_by_month[month - 1u];
}

static uint32_t datetime_to_counter(uint32_t year, uint32_t month, uint32_t day,
                                    uint32_t hour, uint32_t minute,
                                    uint32_t second)
{
    uint32_t days = 0u;

    for (uint32_t y = RTC_EPOCH_YEAR; y < year; ++y) {
        days += is_leap_year(y) ? 366u : 365u;
    }

    for (uint32_t m = 1u; m < month; ++m) {
        days += days_in_month(year, m);
    }

    days += day - 1u;

    return (((days * 24u) + hour) * 60u + minute) * 60u + second;
}

static void counter_to_datetime(uint32_t counter, RtcDateTime *date_time)
{
    uint32_t days = counter / 86400u;
    uint32_t day_seconds = counter % 86400u;
    uint32_t year = RTC_EPOCH_YEAR;
    uint32_t month = 1u;
    uint32_t year_days;
    uint32_t month_days;

    while (1) {
        year_days = is_leap_year(year) ? 366u : 365u;
        if (days < year_days) {
            break;
        }
        days -= year_days;
        ++year;
    }

    while (1) {
        month_days = days_in_month(year, month);
        if (days < month_days) {
            break;
        }
        days -= month_days;
        ++month;
    }

    date_time->year = year;
    date_time->month = month;
    date_time->day = days + 1u;
    date_time->hour = day_seconds / 3600u;
    date_time->minute = (day_seconds % 3600u) / 60u;
    date_time->second = day_seconds % 60u;
}

static uint32_t rtc_read_counter(void)
{
    uint32_t high1;
    uint32_t low;
    uint32_t high2;

    do {
        high1 = RTC_CNTH & 0xFFFFu;
        low = RTC_CNTL & 0xFFFFu;
        high2 = RTC_CNTH & 0xFFFFu;
    } while (high1 != high2);

    return (high1 << 16u) | low;
}

static uint32_t rtc_read_divider(void)
{
    uint32_t high1;
    uint32_t low;
    uint32_t high2;

    do {
        high1 = RTC_DIVH & 0x000Fu;
        low = RTC_DIVL & 0xFFFFu;
        high2 = RTC_DIVH & 0x000Fu;
    } while (high1 != high2);

    return (high1 << 16u) | low;
}

static uint32_t rtc_elapsed_ms_in_second(void)
{
    uint32_t divider = rtc_read_divider();

    if (divider > RTC_PRESCALER) {
        divider = RTC_PRESCALER;
    }

    return ((RTC_PRESCALER - divider) * 1000u) / (RTC_PRESCALER + 1u);
}

static int rtc_lse_init_once(void)
{
    uint32_t timeout;

    backup_domain_enable_write();

    if ((BKP_DR3 & 0xFFFFu) == RTC_INIT_SIGNATURE) {
        return wait_rtc_sync();
    }

    RCC_BDCR |= RCC_BDCR_LSEON;
    timeout = LSE_START_TIMEOUT;
    while ((RCC_BDCR & RCC_BDCR_LSERDY) == 0u) {
        if (timeout-- == 0u) {
            return 0;
        }
    }

    RCC_BDCR = (RCC_BDCR & ~(3u << 8u)) | RCC_BDCR_RTCSEL_LSE |
               RCC_BDCR_RTCEN | RCC_BDCR_LSEON;

    if (!wait_rtc_sync() || !rtc_enter_config()) {
        return 0;
    }

    RTC_CRH = 0u;
    RTC_PRLH = 0u;
    RTC_PRLL = RTC_PRESCALER;
    rtc_write_counter(datetime_to_counter(RTC_INIT_YEAR, RTC_INIT_MONTH,
                                          RTC_INIT_DAY, RTC_INIT_HOUR,
                                          RTC_INIT_MINUTE, RTC_INIT_SECOND));

    if (!rtc_exit_config()) {
        return 0;
    }

    BKP_DR3 = RTC_INIT_SIGNATURE;
    return 1;
}

int main(void)
{
    uint32_t last_counter = 0xFFFFFFFFu;
    RtcDateTime date_time;

    OLED_DisplayInit();
    OLED_DisplayTextStatus("RTC", "INIT LSE");

    if (!rtc_lse_init_once()) {
        OLED_DisplayTextStatus("RTC", "LSE FAIL");
        while (1) {
            delay(800000u);
        }
    }

    last_counter = rtc_read_counter();
    counter_to_datetime(last_counter, &date_time);

    while (1) {
        uint32_t counter = rtc_read_counter();
        uint32_t elapsed_ms = rtc_elapsed_ms_in_second();

        if (counter != last_counter) {
            counter_to_datetime(counter, &date_time);
            last_counter = counter;
        }

        OLED_DisplayRtcTime(date_time.year, date_time.month, date_time.day,
                            date_time.hour, date_time.minute,
                            date_time.second, counter, elapsed_ms);
    }
}
