#!/usr/bin/env python3
"""Decompress backwards LZ77 (BLZ) data, as used for the ARM9, ARM9i and overlays."""
import argparse
import struct


def blz_decompress(data: bytes) -> bytes:
    """Decompress a BLZ buffer. Data before the compressed region is kept as-is."""
    end_info, extra_size = struct.unpack_from('<II', data, len(data) - 8)
    compressed_size = end_info & 0xFFFFFF
    header_size = end_info >> 24
    out = bytearray(data) + bytearray(extra_size)
    src = len(data) - header_size
    dst = len(out)
    limit = len(data) - compressed_size
    while src > limit:
        src -= 1
        flags = data[src]
        for _ in range(8):
            if src <= limit:
                break
            if flags & 0x80:
                src -= 2
                value = data[src] | (data[src + 1] << 8)
                length = (value >> 12) + 3
                distance = (value & 0xFFF) + 3
                for _ in range(length):
                    dst -= 1
                    out[dst] = out[dst + distance]
            else:
                src -= 1
                dst -= 1
                out[dst] = data[src]
            flags = (flags << 1) & 0xFF
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input')
    parser.add_argument('output')
    args = parser.parse_args()
    with open(args.input, 'rb') as f:
        data = f.read()
    with open(args.output, 'wb') as f:
        f.write(blz_decompress(data))


if __name__ == '__main__':
    main()
