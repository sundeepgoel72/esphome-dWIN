#!/usr/bin/env python3
"""Small CLI to generate DWIN/DGUS test frames as hex strings."""

from __future__ import annotations

import argparse
import sys

HEADER = bytes([0x5A, 0xA5])


def frame(command: int, payload: bytes) -> bytes:
    return HEADER + bytes([len(payload) + 1, command]) + payload


def vp_write_word(vp: int, value: int) -> bytes:
    return frame(0x82, bytes([vp >> 8, vp & 0xFF, value >> 8, value & 0xFF]))


def vp_read(vp: int, words: int) -> bytes:
    return frame(0x83, bytes([vp >> 8, vp & 0xFF, words & 0xFF]))


def vp_write_text(vp: int, text: str, length: int) -> bytes:
    data = text.encode("ascii", errors="replace")[:length]
    data += b"\x00" * (length - len(data))
    return frame(0x82, bytes([vp >> 8, vp & 0xFF]) + data)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)

    p = sub.add_parser("write-word")
    p.add_argument("vp", type=lambda x: int(x, 0))
    p.add_argument("value", type=lambda x: int(x, 0))

    p = sub.add_parser("read-vp")
    p.add_argument("vp", type=lambda x: int(x, 0))
    p.add_argument("words", type=lambda x: int(x, 0))

    p = sub.add_parser("write-text")
    p.add_argument("vp", type=lambda x: int(x, 0))
    p.add_argument("text")
    p.add_argument("length", type=int)

    args = parser.parse_args(argv)
    if args.cmd == "write-word":
        out = vp_write_word(args.vp, args.value)
    elif args.cmd == "read-vp":
        out = vp_read(args.vp, args.words)
    elif args.cmd == "write-text":
        out = vp_write_text(args.vp, args.text, args.length)
    else:
        raise AssertionError(args.cmd)

    print(out.hex(" ").upper())
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
