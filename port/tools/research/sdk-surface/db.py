"""Build a database of every function, its owning source file/library, and the call/reference graph
for pokebw2 (b2_us). Reads the repository REPO and writes db.pkl to build/port-research/sdk-surface/ of the checkout
that holds this script, which git ignores; the other scripts here read and write there too. Run this first.
Usage: db.py [REPO], where REPO defaults to the checkout that holds this script."""
import bisect
import pickle
import re
import sys
from pathlib import Path

W = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[4]
DATA = Path(__file__).resolve().parents[4] / "build/port-research/sdk-surface"
OUT = DATA / "db.pkl"
CFG = W / "config/b2_us/arm9"
EXT = W / "extract/b2_us"

SYM_RE = re.compile(r"^(\S+) kind:(\w+)(?:\(([^)]*)\))? addr:0x([0-9a-f]+)")
REL_RE = re.compile(r"from:0x([0-9a-f]+) kind:(\w+)(?:\([^)]*\))? to:(0x[0-9a-f]+|\d+)(?: add:(-?0x[0-9a-f]+))?(?: module:(\S+))?")


def parse_yaml_overlays():
    text = (EXT / "arm9_overlays/overlays.yaml").read_text()
    res = {}
    for block in text.split("  - id: ")[1:]:
        oid = int(block.split("\n", 1)[0])
        base = int(re.search(r"base_address: (\d+)", block).group(1))
        res[oid] = base
    return res


def module_list():
    mods = {}
    mods["main"] = dict(dir=CFG, bin=EXT / "arm9/arm9.bin", base=0x02004000)
    mods["itcm"] = dict(dir=CFG / "itcm", bin=EXT / "arm9/itcm.bin", base=0x01FF8000)
    mods["dtcm"] = dict(dir=CFG / "dtcm", bin=EXT / "arm9/dtcm.bin", base=0x02FE0000)
    mods["autoload_2"] = dict(dir=CFG / "autoload_2", bin=EXT / "arm9/unk_autoload_2.bin", base=0x02400000)
    mods["autoload_3"] = dict(dir=CFG / "autoload_3", bin=EXT / "arm9/unk_autoload_3.bin", base=0x06898000)
    mods["ltd_autoload_0"] = dict(dir=CFG / "ltd_autoload_0", bin=EXT / "dsi/ltd_autoload_0.bin", base=0x02700000)
    for oid, base in parse_yaml_overlays().items():
        mods[f"ov{oid:03d}"] = dict(dir=CFG / f"overlays/ov{oid:03d}", bin=EXT / f"arm9_overlays/ov{oid:03d}.bin", base=base)
    return mods


def parse_symbols(path):
    funcs, data = [], []
    for line in path.read_text().splitlines():
        m = SYM_RE.match(line)
        if not m:
            continue
        name, kind, extra, addr = m.group(1), m.group(2), m.group(3) or "", int(m.group(4), 16)
        if kind == "function":
            mode = "thumb" if "thumb" in extra else "arm"
            sm = re.search(r"size=0x([0-9a-f]+)", extra)
            size = int(sm.group(1), 16) if sm else 0
            funcs.append(dict(name=name, addr=addr, size=size, mode=mode))
        else:
            data.append(dict(name=name, addr=addr, kind=kind))
    return funcs, data


def parse_delinks(path):
    entries = []
    cur = None
    for line in path.read_text().splitlines():
        if not line.strip():
            continue
        if not line.startswith(" ") and line.rstrip().endswith(":"):
            cur = dict(path=line.rstrip()[:-1], complete=False, sections={})
            entries.append(cur)
        elif cur is not None:
            s = line.strip()
            if s == "complete":
                cur["complete"] = True
            else:
                m = re.match(r"(\.\w+)\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+)", s)
                if m:
                    cur["sections"][m.group(1)] = (int(m.group(2), 16), int(m.group(3), 16))
    return entries


def parse_relocs(path):
    rel = []
    for line in path.read_text().splitlines():
        m = REL_RE.match(line)
        if not m:
            continue
        frm, kind, to, add, mod = m.groups()
        rel.append(dict(frm=int(frm, 16), kind=kind, to=int(to, 0), add=int(add, 0) if add else 0, mod=mod))
    return rel


def norm_module(mod, here):
    if mod is None:
        return None
    if mod == "main":
        return "main"
    if mod in ("itcm", "dtcm"):
        return mod
    m = re.match(r"overlays?\(([\d,]+)\)", mod)
    if m:
        return f"ov{int(m.group(1).split(',')[0]):03d}"
    m = re.match(r"ltd_autoload\((\d+)\)", mod)
    if m:
        return f"ltd_autoload_{m.group(1)}"
    m = re.match(r"autoload\((\d+)\)", mod)
    if m:
        return f"autoload_{m.group(1)}"
    return mod


def lib_of_path(path):
    p = path.split("/")
    if p[0] == "lib":
        # lib/<name>/src/<sub>/file.c or lib/<name>/src/file.c
        name = p[1]
        sub = p[3] if len(p) > 4 else "-"
        return ("lib", f"{name}/{sub}")
    if p[0] == "src":
        return ("game", p[1])
    return ("?", path)


def main():
    mods = module_list()
    db = dict(mods={}, funcs=[], edges=[])
    for mname, m in mods.items():
        d = m["dir"]
        if not d.exists():
            continue
        funcs, data = parse_symbols(d / "symbols.txt")
        delinks = parse_delinks(d / "delinks.txt") if (d / "delinks.txt").exists() else []
        relocs = parse_relocs(d / "relocs.txt") if (d / "relocs.txt").exists() else []
        binb = m["bin"].read_bytes() if m["bin"].exists() else b""
        sect = delinks[0]["sections"] if delinks and delinks[0]["path"] == "" else {}
        # first block in delinks has no path line: module sections. Re-parse header lines
        header = {}
        for line in (d / "delinks.txt").read_text().splitlines():
            if not line.strip():
                break
            mm = re.match(r"\s*(\.\w+)\s+start:0x([0-9a-f]+) end:0x([0-9a-f]+)", line)
            if mm:
                header[mm.group(1)] = (int(mm.group(2), 16), int(mm.group(3), 16))
        db["mods"][mname] = dict(base=m["base"], bin=binb, data=data, delinks=delinks, relocs=relocs, header=header)
        # text ranges
        ranges = sorted((e["sections"][".text"][0], e["sections"][".text"][1], e["path"], e["complete"])
                        for e in delinks if ".text" in e["sections"])
        starts = [r[0] for r in ranges]
        funcs.sort(key=lambda f: f["addr"])
        for f in funcs:
            f["module"] = mname
            i = bisect.bisect_right(starts, f["addr"]) - 1
            f["path"] = None
            f["complete"] = False
            if i >= 0 and ranges[i][0] <= f["addr"] < ranges[i][1]:
                f["path"] = ranges[i][2]
                f["complete"] = ranges[i][3]
            # neighbours
            prev = ranges[i] if i >= 0 else None
            nxt = ranges[i + 1] if i + 1 < len(ranges) else None
            f["prev_path"] = prev[2] if prev else None
            f["next_path"] = nxt[2] if nxt else None
        db["funcs"].extend(funcs)
    # owner classification
    for f in db["funcs"]:
        if f["path"]:
            cat, lib = lib_of_path(f["path"])
            f["cat"], f["lib"], f["inferred"] = cat, lib, False
        else:
            a = lib_of_path(f["prev_path"]) if f["prev_path"] else None
            b = lib_of_path(f["next_path"]) if f["next_path"] else None
            f["inferred"] = True
            if a and b and a[0] == b[0] == "lib" and a[1].split("/")[0] == b[1].split("/")[0]:
                f["cat"] = "lib"
                f["lib"] = a[1] if a[1] == b[1] else a[1].split("/")[0] + "/" + a[1].split("/")[1] + "|" + b[1].split("/")[1]
            elif a and b and a[0] == b[0] == "game":
                f["cat"], f["lib"] = "game", a[1] if a[1] == b[1] else a[1]
            else:
                f["cat"], f["lib"] = "edge", f"{a[1] if a else None}..{b[1] if b else None}"
    # index for lookup
    bymod = {}
    for idx, f in enumerate(db["funcs"]):
        bymod.setdefault(f["module"], []).append(idx)
    for mname in bymod:
        bymod[mname].sort(key=lambda i: db["funcs"][i]["addr"])
    db["bymod"] = bymod
    db["bymod_addrs"] = {m: [db["funcs"][i]["addr"] for i in idxs] for m, idxs in bymod.items()}

    def find(mname, addr):
        idxs = bymod.get(mname)
        if not idxs:
            return None
        addrs = db["bymod_addrs"][mname]
        i = bisect.bisect_right(addrs, addr) - 1
        if i < 0:
            return None
        f = db["funcs"][idxs[i]]
        if f["addr"] <= addr < f["addr"] + max(f["size"], 2):
            return idxs[i]
        return None

    # edges
    for mname, m in db["mods"].items():
        for r in m["relocs"]:
            if r["kind"] in ("overlay_id", "link_time_const"):
                continue
            caller = find(mname, r["frm"])
            tmod = norm_module(r["mod"], mname)
            if tmod is None:
                continue
            target = r["to"] & ~1
            callee = find(tmod, target)
            if callee is None or db["funcs"][callee]["addr"] != target:
                continue  # data reference
            db["edges"].append((caller, callee, r["kind"], mname, r["frm"]))
    DATA.mkdir(parents=True, exist_ok=True)
    with open(OUT, "wb") as fh:
        pickle.dump(db, fh)
    print("funcs", len(db["funcs"]), "edges", len(db["edges"]))


if __name__ == "__main__":
    main()
