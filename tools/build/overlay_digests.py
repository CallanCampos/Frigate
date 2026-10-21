#!/usr/bin/env python3
"""Write the digests of a version's ARM9 overlays, which ARM9 main keeps at the end of its .data, as the bodies of
the C arrays' initializers that lib/nitro/src/fs/fs_overlay_digest.c includes.

NitroSDK's build fills this table after the link: each overlay's digest is the HMAC-SHA1, under the key of
fs_overlay.c, of its image as the ROM stores it (compressed, if it is), which FS_StartOverlay checks when the game
was not booted from the ROM. Ahead of them is the digest of the overlay table, an HMAC-SHA1 with words 6 and 14 of
each 64-byte block cleared (sha1_set_masked), which are the file IDs of its 32-byte entries. This computes them from
the base ROM's overlays, as ROM_BLOBS in configure.py takes the bytes no compiler of ours makes from it.

    overlay_digests.py orig/baserom_b2_us.nds extract/b2_us/arm9/hmac_sha1_key.bin \\
        build/include/generated/rom/b2_us/overlay_table_digest.inc build/include/generated/rom/b2_us/overlay_digests.inc
"""
import argparse
import hashlib
import hmac
import struct
from pathlib import Path

DIGEST_SIZE = 20
SHA1_BLOCK_SIZE = 64
OVERLAY_HEADER_SIZE = 32
OVERLAY_FLAG_COMPRESS = 0x01


def rotate(x: int, n: int) -> int:
    return ((x << n) | (x >> (32 - n))) & 0xFFFFFFFF


def sha1_masked(message: bytes) -> bytes:
    """SHA-1 with words 6 and 14 of every block cleared before it is compressed, as sha1_block_masked does."""
    state = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0]
    padded = message + b"\x80" + bytes((55 - len(message)) % SHA1_BLOCK_SIZE) + struct.pack(">Q", len(message) * 8)
    for offset in range(0, len(padded), SHA1_BLOCK_SIZE):
        w = list(struct.unpack(">16I", padded[offset : offset + SHA1_BLOCK_SIZE]))
        w[6] = w[14] = 0
        for i in range(16, 80):
            w.append(rotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1))
        a, b, c, d, e = state
        for i in range(80):
            if i < 20:
                f, k = (b & c) | (~b & d), 0x5A827999
            elif i < 40:
                f, k = b ^ c ^ d, 0x6ED9EBA1
            elif i < 60:
                f, k = (b & c) | (b & d) | (c & d), 0x8F1BBCDC
            else:
                f, k = b ^ c ^ d, 0xCA62C1D6
            a, b, c, d, e = (rotate(a, 5) + f + e + k + w[i]) & 0xFFFFFFFF, a, rotate(b, 30), c, d
        state = [(x + y) & 0xFFFFFFFF for x, y in zip(state, (a, b, c, d, e))]
    return struct.pack(">5I", *state)


def hmac_sha1_masked(key: bytes, message: bytes) -> bytes:
    """HMAC-SHA1 over sha1_masked, as MATH_CalcHMACSHA1 gives it while the masked block function is chosen."""
    key = key.ljust(SHA1_BLOCK_SIZE, b"\0")
    inner = sha1_masked(bytes(k ^ 0x36 for k in key) + message)
    return sha1_masked(bytes(k ^ 0x5C for k in key) + inner)


def digests(rom: bytes, key: bytes) -> tuple[bytes, list[bytes]]:
    """Returns the digest of the ROM's ARM9 overlay table and those of its overlays, in the order of their IDs."""
    fat_offset = struct.unpack_from("<I", rom, 0x48)[0]
    table_offset, table_size = struct.unpack_from("<II", rom, 0x50)
    table = rom[table_offset : table_offset + table_size]
    overlays = []
    for offset in range(0, table_size, OVERLAY_HEADER_SIZE):
        _, _, ram_size, _, _, _, file_id, word = struct.unpack_from("<8I", table, offset)
        start = struct.unpack_from("<I", rom, fat_offset + 8 * file_id)[0]
        # FSi_GetOverlayBinarySize: the stored size of a compressed image, else the size in memory
        size = word & 0xFFFFFF if word >> 24 & OVERLAY_FLAG_COMPRESS else ram_size
        overlays.append(hmac.new(key, rom[start : start + size], hashlib.sha1).digest())
    return hmac_sha1_masked(key, table), overlays


def initializer(digest: bytes) -> str:
    """Returns a digest as the initializer of an FSOverlayDigest."""
    return "{ { " + ", ".join(f"0x{b:02X}" for b in digest) + " } },\n"


def write(path: Path, text: str):
    """Writes the file only if it changes, so that ninja's restat leaves the file that includes it alone."""
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("rom", type=Path)
    parser.add_argument("key", type=Path, help="the digests' HMAC key, as dsd extracts it")
    parser.add_argument("table_output", type=Path)
    parser.add_argument("overlays_output", type=Path)
    args = parser.parse_args()
    table, overlays = digests(args.rom.read_bytes(), args.key.read_bytes())
    write(args.table_output, initializer(table))
    write(args.overlays_output, "".join(initializer(digest) for digest in overlays))


if __name__ == "__main__":
    main()
