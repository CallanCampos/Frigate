#!/usr/bin/env python3
"""Compiles every incomplete source file (or the files given) to a temporary object and compares what the C emits with
the original file's ranges: functions, code bytes, data bytes per section, and pointer relocations in data.

Usage: <repo>/.venv/bin/python compile_check.py --root <repo> [--version b2_us] --out out/compile_b2.tsv [files...]
Uses the repository's compiler setup (tools/decomp/compiler_probe.py): mwcc via wibo, the library.toml compilers.
Writes nothing under the repository.
"""
import argparse
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("--root", required=True)
ap.add_argument("--version", default="b2_us")
ap.add_argument("--out", required=True)
ap.add_argument("--all", action="store_true", help="also compile complete files")
ap.add_argument("files", nargs="*")
args = ap.parse_args()
ROOT = Path(args.root).resolve()
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "tools" / "decomp"))
import compiler_probe as cp  # noqa: E402
from elftools.elf.elffile import ELFFile  # noqa: E402
from elftools.elf.relocation import RelocationSection  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parent))
from progress import parse_delinks, parse_relocs  # noqa: E402


def entries():
    out = []
    for d in sorted((ROOT / "config" / args.version / "arm9").rglob("delinks.txt")):
        secs, files = parse_delinks(d.read_text())
        relocs = parse_relocs((d.parent / "relocs.txt").read_text()) if (d.parent / "relocs.txt").exists() else []
        srcs = sorted(r[0] for r in relocs)
        for f in files:
            f["module_dir"] = d.parent
            f["relocs"] = srcs
            out.append(f)
    return out


def rom_ptrs(f, sec):
    if sec not in f["sections"]:
        return 0
    a, b = f["sections"][sec]
    import bisect
    return bisect.bisect_left(f["relocs"], b) - bisect.bisect_left(f["relocs"], a)


def compile_one(src, tmp):
    lib = cp.lib_compiler(src)
    compiler = lib[0] if lib else "1.1p1"
    flags = lib[1] if lib else cp.DEFAULT_FLAGS
    compilers_dir = cp.TOOLS / "mwccarm" / "dsi"
    mwcc = (cp.TOOLS / "mwccarm" / compiler if "/" in compiler else compilers_dir / compiler) / "mwccarm.exe"
    obj = Path(tmp) / (src.stem + ".o")
    command = [*cp.mwcc_command(mwcc), *shlex.split(flags),
               *(a for define in cp.VERSION_DEFINES.get(args.version, []) for a in ("-d", define)),
               *(a for d in cp.INCLUDE_DIRS for a in ("-i", str(ROOT / d))), "-o", str(obj), str(src)]
    proc = subprocess.run(command, capture_output=True, text=True, cwd=tmp)
    if proc.returncode != 0:
        return None, (proc.stdout + proc.stderr)[-300:]
    return obj, None


def analyze(obj):
    res = {"funcs": 0, "func_bytes": 0, "objects": 0, "sec": {}, "data_relocs": {}}
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        names = {}
        for i, s in enumerate(elf.iter_sections()):
            names[i] = s.name
            base = s.name.split("$")[0] if s.name.startswith(".") else s.name
            if s["sh_type"] in ("SHT_PROGBITS", "SHT_NOBITS") and base in (".text", ".rodata", ".data", ".bss",
                                                                             ".sdata", ".sbss", ".init", ".ctor",
                                                                             ".sinit", ".exception", ".exceptix"):
                res["sec"][base] = res["sec"].get(base, 0) + s["sh_size"]
        for s in elf.iter_sections():
            if isinstance(s, RelocationSection):
                tgt = names.get(s["sh_info"], "")
                if tgt.startswith((".data", ".rodata", ".sdata", ".init", ".ctor", ".sinit")):
                    k = tgt.split("$")[0]
                    res["data_relocs"][k] = res["data_relocs"].get(k, 0) + s.num_relocations()
        st = elf.get_section_by_name(".symtab")
        for sym in st.iter_symbols():
            if sym["st_shndx"] in ("SHN_UNDEF", "SHN_ABS") or sym.name.startswith("$"):
                continue
            if sym["st_info"]["type"] == "STT_FUNC":
                res["funcs"] += 1
                res["func_bytes"] += sym["st_size"]
            elif sym["st_info"]["type"] == "STT_OBJECT":
                res["objects"] += 1
    return res


def main():
    want = set(args.files)
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        for f in entries():
            if want and f["path"] not in want:
                continue
            if not want and f["complete"] and not args.all:
                continue
            src = ROOT / f["path"]
            rng = {s: b - a for s, (a, b) in f["sections"].items()}
            row = {"path": f["path"], "complete": f["complete"], "has_src": src.exists()}
            for s in (".text", ".rodata", ".data", ".bss"):
                row["rom" + s] = rng.get(s, 0)
            row["rom_ptrs_rodata"] = rom_ptrs(f, ".rodata")
            row["rom_ptrs_data"] = rom_ptrs(f, ".data")
            if not src.exists():
                row["status"] = "no source"
                rows.append(row)
                continue
            obj, err = compile_one(src, tmp)
            if obj is None:
                row["status"] = "compile failed: " + err.replace("\n", " ")[:200]
                rows.append(row)
                continue
            r = analyze(obj)
            row["status"] = "ok"
            row["c_funcs"] = r["funcs"]
            for s in (".text", ".rodata", ".data", ".bss", ".sdata", ".sbss", ".init"):
                row["c" + s] = r["sec"].get(s, 0)
            row["c_ptrs_rodata"] = r["data_relocs"].get(".rodata", 0)
            row["c_ptrs_data"] = r["data_relocs"].get(".data", 0) + r["data_relocs"].get(".sdata", 0)
            rows.append(row)
            print(f["path"], row["status"], row.get("c.text"), row["rom.text"], row.get("c.rodata"), row["rom.rodata"],
                  row.get("c.data"), row["rom.data"], flush=True)
    cols = ["path", "complete", "has_src", "status", "rom.text", "c.text", "c_funcs", "rom.rodata", "c.rodata",
            "rom.data", "c.data", "rom.bss", "c.bss", "c.sdata", "c.sbss", "c.init", "rom_ptrs_rodata",
            "c_ptrs_rodata", "rom_ptrs_data", "c_ptrs_data"]
    with open(args.out, "w") as fh:
        fh.write("\t".join(cols) + "\n")
        for r in rows:
            fh.write("\t".join(str(r.get(c, "")) for c in cols) + "\n")


if __name__ == "__main__":
    main()
