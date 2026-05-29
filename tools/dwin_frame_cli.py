#!/usr/bin/env python3

from __future__ import annotations

import argparse
import sys

HEADER = bytes([0x5A, 0xA5])


def frame(command: int, payload: bytes) -> bytes:
    return HEADER + bytes([len(payload) + 1, command]) + payload


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", type=lambda x: int(x, 0))
    parser.add_argument("payload", nargs="*", type=lambda x: int(x, 0))
    args = parser.parse_args(argv)
    print(frame(args.command, bytes(args.payload)).hex(" ").upper())
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
