"""Reads original bytes of a module address from extract/<version>/ (decompressed binaries written by dsd extract)."""
from pathlib import Path

import yaml


class RomBytes:
    def __init__(self, root, version):
        self.base = Path(root) / "extract" / version
        self.cache = {}

    def _load(self, module):
        if module in self.cache:
            return self.cache[module]
        b = self.base
        if module == "main":
            binp, yml = b / "arm9/arm9.bin", b / "arm9/arm9.yaml"
        elif module == "itcm":
            binp, yml = b / "arm9/itcm.bin", b / "arm9/itcm.yaml"
        elif module == "dtcm":
            binp, yml = b / "arm9/dtcm.bin", b / "arm9/dtcm.yaml"
        elif module.startswith("autoload_"):
            i = module.split("_")[1]
            binp, yml = b / f"arm9/unk_autoload_{i}.bin", b / f"arm9/unk_autoload_{i}.yaml"
        elif module == "ltd_autoload_0":
            binp, yml = b / "dsi/ltd_autoload_0.bin", b / "dsi/ltd_autoload_0.yaml"
        elif module.startswith("ov"):
            if "ovl" not in self.cache:
                self.cache["ovl"] = {o["id"]: o for o in yaml.safe_load(open(b / "arm9_overlays/overlays.yaml"))["overlays"]}
            o = self.cache["ovl"][int(module[2:])]
            data = (b / "arm9_overlays" / o["file_name"]).read_bytes()
            self.cache[module] = (o["base_address"], data)
            return self.cache[module]
        else:
            self.cache[module] = (0, b"")
            return self.cache[module]
        base = yaml.safe_load(open(yml))["base_address"]
        self.cache[module] = (base, binp.read_bytes())
        return self.cache[module]

    def read(self, module, addr, size):
        base, data = self._load(module)
        o = addr - base
        if o < 0 or o + size > len(data):
            return b""
        return data[o:o + size]


def is_veneer(rb, module, addr, size, mode):
    """Linker veneers (long-branch / interworking stubs) that dsd lists as functions."""
    if size > 0x10:
        return False
    d = rb.read(module, addr, size)
    if not d:
        return False
    if mode == "thumb" and d[:4] in (b"\x78\x47\xc0\x46",) and size <= 4:
        return True  # bx pc; nop  (thumb half of a thumb->arm veneer)
    if mode == "thumb" and d[:2] == b"\x78\x47" and size == 2:
        return True
    if mode == "arm" and size in (0xc, 0x8):
        w = [int.from_bytes(d[i:i + 4], "little") for i in range(0, size, 4)]
        if size == 0xc and w[0] == 0xe59fc000 and w[1] in (0xe12fff1c,):
            return True  # ldr ip, [pc]; bx ip; .word target
        if size == 0x8 and w[0] == 0xe51ff004:
            return True  # ldr pc, [pc, #-4]; .word target
    if mode == "thumb" and size in (0x8, 0xc, 0x10):
        # push {r?}; ldr r?, [pc]; mov ip, r?; pop {r?}; bx ip; .word  (thumb long branch veneers)
        h = [int.from_bytes(d[i:i + 2], "little") for i in range(0, min(size, 8), 2)]
        if h and (h[0] & 0xff00) == 0xb400 and len(h) > 1 and (h[1] & 0xf800) == 0x4800:
            return True
    return False
