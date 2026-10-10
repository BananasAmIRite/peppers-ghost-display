"""Send a local file to the ESP for writing to its SD card over SPI."""

import argparse
import struct
from pathlib import Path

from lib.comms import comms
from lib.comms.ESPSPI import ESPSPI


def build_payload(source_file: Path, destination_file: str) -> bytes:
    destination_path = destination_file.encode("utf-8")
    if len(destination_path) > 0xFFFF:
        raise ValueError("destination file name is too long for a uint16 length")

    return struct.pack("<H", len(destination_path)) + destination_path + source_file.read_bytes()


def main() -> None:
    parser = argparse.ArgumentParser(description="Write a file to the ESP SD card over SPI")
    parser.add_argument("source_file", type=Path, help="local file to transfer")
    parser.add_argument("destination_file", help="file name or path on the SD card")
    args = parser.parse_args()

    if not args.source_file.is_file():
        parser.error(f"source file does not exist: {args.source_file}")

    try:
        payload = build_payload(args.source_file, args.destination_file)
    except OSError as error:
        parser.error(f"could not read source file: {error}")
    except ValueError as error:
        parser.error(str(error))

    spi = ESPSPI()
    try:
        spi.send_packet(comms.SD_FILE_WRITE, payload)
    finally:
        spi.close()


if __name__ == "__main__":
    main()