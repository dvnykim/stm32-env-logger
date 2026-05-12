#!/usr/bin/env python3
"""Read SHT31 readings from the STM32L476 over USB serial and log to CSV.

Expects lines like:
    Temp: 23.47 C, Humidity: 55.21 %RH

Adds a UTC timestamp and appends to the output CSV. Non-matching lines
(boot banner, sensor errors, garbage) are printed to stderr and skipped.
"""

import argparse
import csv
import glob
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

import serial

# Matches the exact format your firmware emits. If you change the
# `uart_puts(...)` strings in main.c, update this regex too.
LINE_PATTERN = re.compile(
    r"^Temp:\s+([-\d.]+)\s+C,\s+Humidity:\s+([\d.]+)\s+%RH$"
)


def find_port():
    """Auto-detect the Nucleo's USB serial device."""
    candidates = sorted(glob.glob("/dev/cu.usbmodem*"))
    if not candidates:
        sys.exit("No /dev/cu.usbmodem* device found. Is the Nucleo plugged in?")
    if len(candidates) > 1:
        print(f"Multiple ports found, picking {candidates[0]}", file=sys.stderr)
    return candidates[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", help="serial port (auto-detect if omitted)")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--out", default="readings.csv",
                        help="output CSV (appended to if it exists)")
    args = parser.parse_args()

    port = args.port or find_port()
    out_path = Path(args.out)
    is_new_file = not out_path.exists()

    print(f"Opening {port} @ {args.baud} baud, logging to {out_path}")

    try:
        ser = serial.Serial(port, args.baud, timeout=5)
    except serial.SerialException as e:
        sys.exit(f"Failed to open {port}: {e}")

    # buffering=1 → line-buffered. Means every row hits disk immediately,
    # so a Ctrl-C or USB unplug doesn't lose the last few samples.
    with open(out_path, "a", newline="", buffering=1) as csv_file:
        writer = csv.writer(csv_file)
        if is_new_file:
            writer.writerow(["timestamp_utc", "temperature_c", "humidity_pct"])

        try:
            while True:
                raw = ser.readline()
                if not raw:
                    continue  # 5 s readline timeout, nothing came in
                line = raw.decode("utf-8", errors="replace").strip()
                if not line:
                    continue

                match = LINE_PATTERN.match(line)
                if not match:
                    # Boot banner, error message, etc. Log and skip.
                    print(f"[skip] {line}", file=sys.stderr)
                    continue

                temp = float(match.group(1))
                hum = float(match.group(2))
                ts = datetime.now(timezone.utc).isoformat(timespec="seconds")
                writer.writerow([ts, temp, hum])
                print(f"{ts}  T={temp:6.2f}°C  RH={hum:6.2f}%")
        except KeyboardInterrupt:
            print("\nStopping. CSV closed cleanly.")
        finally:
            ser.close()


if __name__ == "__main__":
    main()
