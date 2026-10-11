#!/usr/bin/env python3
"""Verify a DSi-enhanced ROM dump against the digests and SHA1-HMACs in its own header.

Nintendo hashes every 0x400-byte sector of the ROM, so a clean result means the dump is intact. The secure area is
hashed in its encrypted form, which dumps do not contain, so its sectors and the HMAC covering it are skipped.
"""
import argparse
import hashlib
import hmac
import struct
import sys
from pathlib import Path

from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.build.blz import blz_decompress  # noqa: E402

KEY_SCRAMBLER_CONSTANT = 0xFFFEFB4E295902582A680F5F1A4F3E79
MASK_128 = (1 << 128) - 1
SECURE_AREA_ENCRYPTED_SIZE = 0x800


def modcrypt(data: bytes, key: bytes, counter: bytes) -> bytes:
    aes = Cipher(algorithms.AES(key[::-1]), modes.ECB()).encryptor()
    ctr = int.from_bytes(counter, "little")
    out = bytearray()
    for i in range(0, len(data), 16):
        stream = aes.update(ctr.to_bytes(16, "big"))[::-1]
        out += bytes(a ^ b for a, b in zip(data[i : i + 16], stream))
        ctr = (ctr + 1) & MASK_128
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    args = parser.parse_args()

    rom = open(args.rom, "rb").read()
    u32 = lambda offset: struct.unpack_from("<I", rom, offset)[0]
    if not rom[0x12] & 2:
        sys.exit("not a DSi-enhanced ROM")

    # The HMAC key is stored in the decompressed ARM9 program, starting at the second nitrocode
    arm9 = blz_decompress(rom[u32(0x20) : u32(0x20) + u32(0x2C)])
    nitrocode = struct.pack("<I", 0xDEC00621)
    first = arm9.find(nitrocode)
    second = arm9.find(nitrocode, first + 4)
    if first < 0 or second < 0:
        sys.exit("HMAC key not found in the ARM9 program")
    key = arm9[second : second + 64]
    mac = lambda data: hmac.new(key, data, hashlib.sha1).digest()

    ok = True

    def check(name, good, total):
        nonlocal ok
        status = "ok" if good == total else "FAILED"
        ok &= good == total
        print(f"{name:32s} {good}/{total} {status}")

    sector_table, sector_table_size = u32(0x1F0), u32(0x1F4)
    block_table, block_table_size = u32(0x1F8), u32(0x1FC)
    sector_size, sector_count = u32(0x200), u32(0x204)
    ds_area, ds_area_size = u32(0x1E0), u32(0x1E4)
    dsi_area, dsi_area_size = u32(0x1E8), u32(0x1EC)
    digests = [rom[sector_table + i : sector_table + i + 20] for i in range(0, sector_table_size, 20)]

    # DS area, skipping the sectors of the encrypted secure area
    skipped = SECURE_AREA_ENCRYPTED_SIZE // sector_size
    num_ds = ds_area_size // sector_size
    good = sum(mac(rom[ds_area + i * sector_size : ds_area + (i + 1) * sector_size]) == digests[i]
               for i in range(skipped, num_ds))
    check("DS area sector digests", good, num_ds - skipped)

    # DSi area, with modcrypt area 1 decrypted
    gamecode = rom[0xC:0x10]
    key_x = int.from_bytes(b"Nintendo" + gamecode + gamecode[::-1], "little")
    key_y = int.from_bytes(rom[0x350:0x360], "little")
    scrambled = ((key_x ^ key_y) + KEY_SCRAMBLER_CONSTANT) & MASK_128
    modcrypt_key = (((scrambled << 42) | (scrambled >> 86)) & MASK_128).to_bytes(16, "little")
    dsi = bytearray(rom[dsi_area : dsi_area + dsi_area_size])
    for area, counter in ((0x220, rom[0x300:0x310]), (0x228, rom[0x314:0x324])):
        offset, size = u32(area), u32(area + 4)
        if size:
            start = offset - dsi_area
            dsi[start : start + size] = modcrypt(bytes(dsi[start : start + size]), modcrypt_key, counter)
    num_dsi = dsi_area_size // sector_size
    good = sum(mac(bytes(dsi[i * sector_size : (i + 1) * sector_size])) == digests[num_ds + i] for i in range(num_dsi))
    check("DSi area sector digests", good, num_dsi)

    blocks = [rom[block_table + i : block_table + i + 20] for i in range(0, block_table_size, 20)]
    good = sum(mac(b"".join(digests[j * sector_count : (j + 1) * sector_count])) == blocks[j] for j in range(len(blocks)))
    check("Block digests", good, len(blocks))

    arm9i, arm9i_size = u32(0x1C0), u32(0x1CC)
    arm7i, arm7i_size = u32(0x1D0), u32(0x1DC)
    hmacs = [
        ("ARM7 HMAC", 0x314, rom[u32(0x30) : u32(0x30) + u32(0x3C)]),
        ("Digest HMAC", 0x328, rom[block_table : block_table + block_table_size]),
        ("Banner HMAC", 0x33C, rom[u32(0x68) : u32(0x68) + u32(0x208)]),
        ("ARM9i HMAC", 0x350, bytes(dsi[arm9i - dsi_area : arm9i - dsi_area + arm9i_size])),
        ("ARM7i HMAC", 0x364, bytes(dsi[arm7i - dsi_area : arm7i - dsi_area + arm7i_size])),
        ("ARM9 (without secure area) HMAC", 0x3A0, rom[u32(0x20) + 0x4000 : u32(0x20) + u32(0x2C)]),
    ]
    for name, offset, data in hmacs:
        check(name, int(mac(data) == rom[offset : offset + 20]), 1)

    print("dump is intact" if ok else "dump is damaged")
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
