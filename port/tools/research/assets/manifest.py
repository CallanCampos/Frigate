"""Writes nitrofs_manifest.json: the size and SHA-1 of every NitroFS file of both versions' ROMs.

    manifest.py REPO OUT.json

For each version it reads the extracted files (extract/<version>/files), the ROM's game code and header CRC from
orig/baserom_<version>.nds, and the ROM's SHA-1 from <version>.sha1. Every host build reads the manifest it wrote
(port/cmake/rom.cmake turns it into the ROM manifest), so that Frigate accepts a trimmed or re-padded dump of the
player's ROM when every file the game reads matches (port/hal/rom/port_rom.h). Hashes only: nothing of the ROM.
"""
import sys, os, hashlib, json, struct
W = sys.argv[1]; out = sys.argv[2]
man = {}
for v, rom in (('b2_us', 'baserom_b2_us.nds'), ('w2_us', 'baserom_w2_us.nds')):
    root = os.path.join(W, 'extract', v, 'files')
    files = {}
    for dp, dn, fn in os.walk(root):
        for f in fn:
            p = os.path.join(dp, f)
            b = open(p, 'rb').read()
            files[os.path.relpath(p, root)] = {'size': len(b), 'sha1': hashlib.sha1(b).hexdigest()}
    h = open(os.path.join(W, 'orig', rom), 'rb').read(0x200)
    man[v] = {'gamecode': h[0xC:0x10].decode(), 'header_crc16': '%04x' % struct.unpack_from('<H', h, 0x15E)[0],
              'rom_sha1': open(os.path.join(W, v + '.sha1')).read().split()[0], 'files': dict(sorted(files.items()))}
json.dump(man, open(out, 'w'), indent=1)
print({v: len(m['files']) for v, m in man.items()})
