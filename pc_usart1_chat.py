#!/usr/bin/env python3
import argparse
import os
import select
import sys
import termios
import time


BAUD_RATES = {
    9600: termios.B9600,
    19200: termios.B19200,
    38400: termios.B38400,
    57600: termios.B57600,
    115200: termios.B115200,
}


def configure_serial(fd, baud):
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
    attrs[3] = 0
    attrs[4] = BAUD_RATES[baud]
    attrs[5] = BAUD_RATES[baud]
    attrs[6][termios.VMIN] = 0
    attrs[6][termios.VTIME] = 1
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    termios.tcflush(fd, termios.TCIOFLUSH)


def read_available(fd, timeout):
    deadline = time.monotonic() + timeout
    data = bytearray()

    while time.monotonic() < deadline:
        ready, _, _ = select.select([fd], [], [], 0.05)
        if not ready:
            continue

        chunk = os.read(fd, 256)
        if not chunk:
            break

        data.extend(chunk)
        if b"\n" in chunk:
            break

    return bytes(data)


def main():
    parser = argparse.ArgumentParser(
        description="Send command-line text to STM32 USART1 and print the marked response."
    )
    parser.add_argument("message", nargs="*", help="text to send; omit for interactive mode")
    parser.add_argument("-p", "--port", default="/dev/ttyUSB0", help="serial port")
    parser.add_argument("-b", "--baud", type=int, default=9600, choices=sorted(BAUD_RATES))
    parser.add_argument("-t", "--timeout", type=float, default=2.0, help="read timeout in seconds")
    args = parser.parse_args()

    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        configure_serial(fd, args.baud)

        messages = [" ".join(args.message)] if args.message else None
        if messages is not None:
            for message in messages:
                os.write(fd, (message + "\n").encode("utf-8"))
                response = read_available(fd, args.timeout)
                sys.stdout.write(response.decode("utf-8", errors="replace"))
            return

        print(f"Connected to {args.port} at {args.baud}. Press Ctrl-D to exit.")
        for line in sys.stdin:
            os.write(fd, line.rstrip("\r\n").encode("utf-8") + b"\n")
            response = read_available(fd, args.timeout)
            sys.stdout.write(response.decode("utf-8", errors="replace"))
            sys.stdout.flush()
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
