#!/usr/bin/env python3
"""Simple DWIN/DGUS panel simulator for Windows/Linux serial testing.

Typical Windows setup:
  1. Install a virtual null-modem pair such as com0com.
  2. Create a pair, for example COM10 <-> COM11.
  3. Point the test client/device UART bridge at COM10.
  4. Run this simulator on COM11:

     python tools/dwin_simulator.py --port COM11 --baud 115200

The simulator accepts common DGUS frames:
  5A A5 LEN 82 VP_H VP_L DATA...         write VP
  5A A5 04 83 VP_H VP_L WORD_COUNT       read VP

It stores VP words in memory and replies to read requests with:
  5A A5 LEN 83 VP_H VP_L WORD_COUNT DATA_H DATA_L...
"""

from __future__ import annotations

import argparse
import logging
import sys
import time
from dataclasses import dataclass, field
from typing import Dict, List

try:
    import serial
except ImportError as exc:  # pragma: no cover
    raise SystemExit("pyserial is required: python -m pip install pyserial") from exc

HEADER = bytes([0x5A, 0xA5])
CMD_WRITE_REGISTER = 0x80
CMD_READ_REGISTER = 0x81
CMD_WRITE_VP = 0x82
CMD_READ_VP = 0x83


@dataclass
class DwinState:
    vp_words: Dict[int, int] = field(default_factory=dict)
    registers: Dict[int, int] = field(default_factory=dict)

    def write_vp_bytes(self, vp: int, data: bytes) -> None:
        if len(data) % 2:
            data += b"\x00"
        for offset in range(0, len(data), 2):
            word = (data[offset] << 8) | data[offset + 1]
            self.vp_words[vp + offset // 2] = word

    def read_vp_words(self, vp: int, count: int) -> List[int]:
        return [self.vp_words.get(vp + i, 0) for i in range(count)]


def make_frame(command: int, payload: bytes) -> bytes:
    if len(payload) + 1 > 255:
        raise ValueError("DGUS frame too large")
    return HEADER + bytes([len(payload) + 1, command]) + payload


def make_vp_response(vp: int, words: List[int]) -> bytes:
    payload = bytes([vp >> 8, vp & 0xFF, len(words)])
    for word in words:
        payload += bytes([(word >> 8) & 0xFF, word & 0xFF])
    return make_frame(CMD_READ_VP, payload)


def parse_stream(buffer: bytearray) -> List[tuple[int, bytes]]:
    frames: List[tuple[int, bytes]] = []
    while True:
        start = buffer.find(HEADER)
        if start < 0:
            buffer.clear()
            return frames
        if start:
            del buffer[:start]
        if len(buffer) < 4:
            return frames
        length = buffer[2]
        total = 3 + length
        if length == 0:
            del buffer[0]
            continue
        if len(buffer) < total:
            return frames
        command = buffer[3]
        payload = bytes(buffer[4:total])
        del buffer[:total]
        frames.append((command, payload))


def printable_text(data: bytes) -> str:
    clean = data.split(b"\x00", 1)[0]
    return clean.decode("ascii", errors="replace")


def run(port: str, baud: int, inject_touch_vp: int | None, inject_interval: float) -> None:
    logging.info("Opening %s at %s baud", port, baud)
    state = DwinState()
    rx = bytearray()
    last_inject = time.monotonic()

    with serial.Serial(port=port, baudrate=baud, timeout=0.05) as ser:
        logging.info("DWIN simulator ready")
        while True:
            chunk = ser.read(256)
            if chunk:
                rx.extend(chunk)
                for command, payload in parse_stream(rx):
                    logging.info("RX cmd=0x%02X payload=%s", command, payload.hex(" "))

                    if command == CMD_WRITE_VP and len(payload) >= 2:
                        vp = (payload[0] << 8) | payload[1]
                        data = payload[2:]
                        state.write_vp_bytes(vp, data)
                        logging.info("WRITE_VP vp=0x%04X bytes=%s text=%r", vp, data.hex(" "), printable_text(data))

                    elif command == CMD_READ_VP and len(payload) >= 3:
                        vp = (payload[0] << 8) | payload[1]
                        count = payload[2]
                        words = state.read_vp_words(vp, count)
                        frame = make_vp_response(vp, words)
                        ser.write(frame)
                        logging.info("TX VP_RESPONSE vp=0x%04X words=%s frame=%s", vp, words, frame.hex(" "))

                    elif command == CMD_WRITE_REGISTER and len(payload) >= 1:
                        address = payload[0]
                        for i, value in enumerate(payload[1:]):
                            state.registers[address + i] = value
                        logging.info("WRITE_REGISTER address=0x%02X bytes=%s", address, payload[1:].hex(" "))

                    elif command == CMD_READ_REGISTER and len(payload) >= 2:
                        address = payload[0]
                        count = payload[1]
                        data = bytes(state.registers.get(address + i, 0) for i in range(count))
                        frame = make_frame(CMD_READ_REGISTER, bytes([address]) + data)
                        ser.write(frame)
                        logging.info("TX REGISTER_RESPONSE address=0x%02X frame=%s", address, frame.hex(" "))

            if inject_touch_vp is not None and time.monotonic() - last_inject >= inject_interval:
                last_inject = time.monotonic()
                value = (state.vp_words.get(inject_touch_vp, 0) + 1) & 0xFFFF
                state.vp_words[inject_touch_vp] = value
                frame = make_vp_response(inject_touch_vp, [value])
                ser.write(frame)
                logging.info("INJECT VP=0x%04X value=%u frame=%s", inject_touch_vp, value, frame.hex(" "))


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="DWIN/DGUS serial panel simulator")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM11 or /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--inject-touch-vp", type=lambda x: int(x, 0), default=None)
    parser.add_argument("--inject-interval", type=float, default=5.0)
    parser.add_argument("--log-level", default="INFO")
    args = parser.parse_args(argv)

    logging.basicConfig(level=getattr(logging, args.log_level.upper()), format="%(asctime)s %(levelname)s %(message)s")
    run(args.port, args.baud, args.inject_touch_vp, args.inject_interval)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
