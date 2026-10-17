"""Heuristic binary scan of every function: literal-pool/immediate hardware addresses, SWI, CP15, MSR/MRS.
Adds 'hw' info to each function in db.pkl -> binscan.pkl."""
import pickle
import struct
from collections import Counter, defaultdict
from pathlib import Path

DATA = Path(__file__).resolve().parents[4] / "build/port-research/sdk-surface"  # ignored by git; db.py fills it
db = pickle.load(open(DATA / "db.pkl", "rb"))


def io_block(v):
    if 0x04000000 <= v < 0x04000070:
        return "2D-A" if v != 0x04000000 else "IO-base"
    if 0x040000B0 <= v < 0x040000E0:
        return "DMA"
    if 0x040000E0 <= v < 0x040000F0:
        return "DMAFILL"
    if 0x04000100 <= v < 0x04000110:
        return "TIMER"
    if 0x04000130 <= v < 0x04000138:
        return "KEY"
    if 0x04000180 <= v < 0x04000190 or 0x04100000 <= v < 0x04100004:
        return "IPC/PXI"
    if 0x040001A0 <= v < 0x040001C0 or 0x04100010 <= v < 0x04100014:
        return "CARD"
    if 0x04000204 <= v < 0x04000206:
        return "EXMEMCNT"
    if 0x04000208 <= v < 0x04000220:
        return "IRQ"
    if 0x04000240 <= v < 0x04000250:
        return "VRAMCNT/WRAMCNT"
    if 0x04000280 <= v < 0x040002C0:
        return "DIV/SQRT"
    if 0x04000300 <= v < 0x04000308:
        return "POWCNT"
    if 0x04000320 <= v < 0x04000400:
        return "3D-ctrl"
    if 0x04000400 <= v < 0x04000600:
        return "G3-FIFO/cmd"
    if 0x04000600 <= v < 0x040006A4:
        return "3D-status"
    if 0x04001000 <= v < 0x04001070:
        return "2D-B"
    if 0x04004000 <= v < 0x04004100:
        return "TWL-SCFG"
    if 0x04004100 <= v < 0x04004200:
        return "TWL-NDMA"
    if 0x04004200 <= v < 0x04004300:
        return "TWL-CAMERA"
    if 0x04004300 <= v < 0x04004400:
        return "TWL-DSP"
    if 0x04004400 <= v < 0x04005000:
        return "TWL-other"
    return f"IO-other"


def region(v):
    if 0x04000000 <= v < 0x04800000:
        return io_block(v)
    if 0x05000000 <= v < 0x05000800:
        return "PLTT"
    if 0x06898000 <= v < 0x068A0000:
        return None  # overlay 169 and autoload 3 run from LCDC VRAM; these are code/data pointers
    if 0x06000000 <= v < 0x06900000:
        return "VRAM"
    if 0x07000000 <= v < 0x07000800:
        return "OAM"
    if 0x08000000 <= v < 0x0A010000 and (v & 0xFFFF) == 0 and False:
        return "GBASLOT"
    if 0x01FF8000 <= v < 0x02000000:
        return "ITCM"
    if 0x027E0000 <= v < 0x027E4000 or 0x02FE0000 <= v < 0x02FE4000:
        return "DTCM"
    if 0x027FF000 <= v < 0x02800000 or 0x02FFC000 <= v < 0x03000000:
        return "SYSSHARED"
    if 0x03000000 <= v < 0x03800000:
        return "WRAM"
    if 0xFFFF0000 <= v < 0xFFFF8000 and False:
        return "BIOS"
    return None


def ror(v, r):
    r &= 31
    return ((v >> r) | (v << (32 - r))) & 0xFFFFFFFF


def word(mod, addr):
    m = db["mods"][mod]
    off = addr - m["base"]
    if 0 <= off and off + 4 <= len(m["bin"]):
        return struct.unpack_from("<I", m["bin"], off)[0]
    return None


def half(mod, addr):
    m = db["mods"][mod]
    off = addr - m["base"]
    if 0 <= off and off + 2 <= len(m["bin"]):
        return struct.unpack_from("<H", m["bin"], off)[0]
    return None


def scan(f):
    mod, a0, size = f["module"], f["addr"], f["size"]
    vals = []
    swi = []
    cp15 = 0
    psr = 0
    if size == 0:
        return dict(vals=vals, swi=swi, cp15=cp15, psr=psr)
    end = a0 + size
    pool = set()
    if f["mode"] == "arm":
        a = a0
        while a < end:
            if a in pool:
                a += 4
                continue
            w = word(mod, a)
            if w is None:
                break
            cond = w >> 28
            if (w & 0x0F7F0000) == 0x051F0000:
                imm = w & 0xFFF
                t = a + 8 + (imm if (w >> 23) & 1 else -imm)
                pool.add(t)
                v = word(mod, t)
                if v is not None:
                    vals.append(v)
            elif (w & 0x0FE00000) == 0x03A00000:  # MOV imm
                v = ror(w & 0xFF, 2 * ((w >> 8) & 0xF))
                vals.append(v)
            elif (w & 0x0F000000) == 0x0F000000 and cond == 0xE:
                swi.append((w & 0xFFFFFF) >> 16)
            elif (w & 0x0F000010) == 0x0E000010 and ((w >> 8) & 0xF) == 15 and cond == 0xE:
                cp15 += 1
            elif cond == 0xE and ((w & 0x0FBF0FFF) == 0x010F0000 or (w & 0x0FB0FFF0) == 0x0120F000 or (w & 0x0FB0F000) == 0x0320F000):
                psr += 1
            a += 4
    else:
        a = a0
        regs = {}
        while a < end:
            if a in pool:
                a += 4
                continue
            h = half(mod, a)
            if h is None:
                break
            if 0xF000 <= h < 0xF800:  # BL prefix
                a += 4
                regs = {}
                continue
            if (h & 0xF800) == 0x4800:
                t = ((a + 4) & ~3) + (h & 0xFF) * 4
                pool.add(t)
                pool.add(t + 2)
                v = word(mod, t)
                if v is not None:
                    vals.append(v)
            elif (h & 0xFF00) == 0xDF00:
                swi.append(h & 0xFF)
            elif (h & 0xF800) == 0x2000:
                regs[(h >> 8) & 7] = h & 0xFF
            elif (h & 0xF800) == 0x0000:
                rd, rm, sh = h & 7, (h >> 3) & 7, (h >> 6) & 0x1F
                if rm in regs:
                    v = (regs[rm] << sh) & 0xFFFFFFFF
                    vals.append(v)
                    regs[rd] = v
            a += 2
    return dict(vals=vals, swi=swi, cp15=cp15, psr=psr)


for f in db["funcs"]:
    r = scan(f)
    regs = Counter()
    addrs = set()
    for v in r["vals"]:
        g = region(v)
        if g:
            regs[g] += 1
            addrs.add(v)
    f["hw"] = dict(regions=dict(regs), addrs=sorted(addrs), swi=r["swi"], cp15=r["cp15"], psr=r["psr"])

pickle.dump(db, open(DATA / "binscan.pkl", "wb"))

# aggregate per lib
agg = defaultdict(lambda: dict(n=0, hw=0, regions=Counter(), swi=Counter(), cp15=0, psr=0, cp15f=0, psrf=0))
for f in db["funcs"]:
    key = f["lib"] if f["cat"] != "game" else "GAME:" + ("main" if f["module"] == "main" else "overlays")
    a = agg[key]
    a["n"] += 1
    h = f["hw"]
    if h["regions"] or h["swi"] or h["cp15"] or h["psr"]:
        a["hw"] += 1
    for k in h["regions"]:
        a["regions"][k] += 1
    for s in h["swi"]:
        a["swi"][s] += 1
    a["cp15"] += h["cp15"]
    a["cp15f"] += bool(h["cp15"])
    a["psr"] += h["psr"]
    a["psrf"] += bool(h["psr"])
for k in sorted(agg):
    a = agg[k]
    print(f"{k:28} n={a['n']:5} hwfuncs={a['hw']:4} cp15f={a['cp15f']:3} psrf={a['psrf']:3} swi={dict(a['swi'])} {dict(a['regions'].most_common(12))}")
