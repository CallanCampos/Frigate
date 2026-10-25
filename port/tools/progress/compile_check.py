#!/usr/bin/env python3
"""Compiles every incomplete source file (or the files given) to a temporary object and compares what the C emits with
the original file's ranges: functions, code bytes, data bytes per section, and pointer relocations in data.

Usage: <repo>/.venv/bin/python compile_check.py --root <repo> [--version b2_us] --out out/compile_b2.tsv [files...]
Writes nothing under the repository.

How much of the game's unlinked C (tier b) emits what the ROM has, for the port's readiness tiers; run_all.sh writes
out/compile_b2_us.tsv, which summarize.py reads (port/docs/research/decomp-progress.md, "Code tiers"). It touches no
game functionality: it measures every module's C, ARM9 main's, the autoloads' and each overlay's, in one version.

Each file is compiled as the repository's own compile probe compiles it (<repo>/tools/decomp/compiler_probe.py): its
probe_defaults gives the build's compiler and flags for the file (configure.py's source_compiler: a library's from its
library.toml, a game file's with config/compiler_flags.toml's overrides) with the probe's warning policy, and for a game
file, where the probe tries every compiler, this takes the build's own. MWCC runs under wibo through sjiswrap
(configure.py's mwcc_command), with the version's defines and the include directories. A library's hand-written
assembly (MSL's fp_fastI.s, which --all reaches) is assembled as the build and tools/decomp/compile_all.py assemble
it, by the mwasmarm of its compiler's version with library.toml's asm_flags (configure.py's source_assembler). A file
that config/deadstrip.toml lists goes through the build's dead-strip (tools/build/deadstrip.py) before it is
measured, as the probe and the link do, since the ROM has none of the code the original link dropped. A failed strip
is reported as a failed compile, which summarize.py counts with them. port/tests/progress/test_compile_check.py
(ctest progress_compile_check) checks this against the repository's tools.
"""
import argparse
import bisect
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

sys.path.insert(0, str(Path(__file__).resolve().parent))
from progress import parse_delinks, parse_relocs  # noqa: E402

# Sections whose sizes are compared with the original file's ranges, and those whose relocations are data pointers
SIZED_SECTIONS = (".text", ".rodata", ".data", ".bss", ".sdata", ".sbss", ".init", ".ctor", ".sinit", ".exception",
                  ".exceptix")
POINTER_SECTIONS = (".data", ".rodata", ".sdata", ".init", ".ctor", ".sinit")


class Tools:
    """The repository's compile probe and the modules it builds on, imported from <repo>: compiler_probe (the probe's
    settings, the version defines, the include directories, the compiler command), configure (the build's settings)
    and tools.build.deadstrip (the dead-strip of the original link)."""

    def __init__(self, root: Path):
        self.root = root.resolve()
        for path in (self.root / "tools" / "decomp", self.root):
            if str(path) not in sys.path:
                sys.path.insert(0, str(path))
        import compiler_probe
        import configure
        from tools.build import deadstrip
        self.probe = compiler_probe
        self.configure = configure
        self.deadstrip = deadstrip

    def settings(self, source: Path) -> tuple[str, str]:
        """The compiler, as tools/mwccarm names its directory (dsi/1.1p1, 2.0/sp2), and the flags the probe compiles
        the file with: the build's, with the probe's warning policy."""
        compiler, flags = self.probe.probe_defaults(source)
        # A game file gets every compiler in the probe; the measure uses the one the build compiles it with
        return compiler or self.configure.source_compiler(source).compiler, flags

    def command(self, source: Path, version: str, obj: Path) -> list[str]:
        """The command that compiles the file for the version into obj, or assembles it for an assembly source."""
        if source.suffix == ".s":
            # The build's assembler for the library: tools/mwccarm/<compiler>/mwasmarm.exe, under wibo and sjiswrap as
            # tools/decomp/compile_all.py runs it. Assembly has no version defines or include directories
            compiler, asm_flags = self.configure.source_assembler(source)
            mwasm = self.probe.TOOLS / "mwccarm" / compiler / "mwasmarm.exe"
            return [*self.probe.mwcc_command(mwasm), *(arg for flag in asm_flags for arg in shlex.split(flag)), "-o",
                    str(obj), str(source)]
        compiler, flags = self.settings(source)
        # tools/mwccarm/<compiler>, where a bare version (1.1p1) is one of the dsi builds, as in the probe
        compilers = self.probe.TOOLS / "mwccarm"
        mwcc = (compilers / compiler if "/" in compiler else compilers / "dsi" / compiler) / "mwccarm.exe"
        return [*self.probe.mwcc_command(mwcc), *shlex.split(flags),
                *(arg for define in self.probe.VERSION_DEFINES.get(version, []) for arg in ("-d", define)),
                *(arg for d in self.probe.INCLUDE_DIRS for arg in ("-i", str(self.root / d))), "-o", str(obj),
                str(source)]

    def compile(self, source: Path, version: str, tmp: Path) -> tuple[Path | None, str | None]:
        """Compiles the file to an object in tmp, dead-stripped as the link takes it; the object, or None and why."""
        obj = tmp / (source.stem + ".o")
        proc = subprocess.run(self.command(source, version, obj), capture_output=True, text=True, cwd=tmp)
        if proc.returncode != 0:
            return None, (proc.stdout + proc.stderr)[-300:]
        dead = self.deadstrip.dead_symbols(source)
        if dead:
            try:
                stripped = self.deadstrip.strip(obj.read_bytes(), dead)
            except self.deadstrip.DeadStripError as error:
                return None, f"dead-strip failed: {error}"
            obj.write_bytes(stripped.data)
        return obj, None


def entries(root: Path, version: str) -> list[dict]:
    """Every file of the version's delinks.txt files, with its ranges and its module's relocation sources."""
    out = []
    for d in sorted((root / "config" / version / "arm9").rglob("delinks.txt")):
        _, files = parse_delinks(d.read_text())
        relocs = parse_relocs((d.parent / "relocs.txt").read_text()) if (d.parent / "relocs.txt").exists() else []
        srcs = sorted(r[0] for r in relocs)
        for f in files:
            f["module_dir"] = d.parent
            f["relocs"] = srcs
            out.append(f)
    return out


def rom_ptrs(f: dict, sec: str) -> int:
    """The relocations the ROM has from inside the file's range of a section."""
    if sec not in f["sections"]:
        return 0
    a, b = f["sections"][sec]
    return bisect.bisect_left(f["relocs"], b) - bisect.bisect_left(f["relocs"], a)


def analyze(obj: Path) -> dict:
    """The object's functions, their bytes, its data objects, its sections' sizes and its data pointers."""
    res = {"funcs": 0, "func_bytes": 0, "objects": 0, "sec": {}, "data_relocs": {}}
    with obj.open("rb") as fh:
        elf = ELFFile(fh)
        names = {}
        for i, s in enumerate(elf.iter_sections()):
            names[i] = s.name
            base = s.name.split("$")[0] if s.name.startswith(".") else s.name
            if s["sh_type"] in ("SHT_PROGBITS", "SHT_NOBITS") and base in SIZED_SECTIONS:
                res["sec"][base] = res["sec"].get(base, 0) + s["sh_size"]
        for s in elf.iter_sections():
            if isinstance(s, RelocationSection):
                tgt = names.get(s["sh_info"], "")
                if tgt.startswith(POINTER_SECTIONS):
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


COLUMNS = ["path", "complete", "has_src", "status", "rom.text", "c.text", "c_funcs", "rom.rodata", "c.rodata",
           "rom.data", "c.data", "rom.bss", "c.bss", "c.sdata", "c.sbss", "c.init", "rom_ptrs_rodata", "c_ptrs_rodata",
           "rom_ptrs_data", "c_ptrs_data"]


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", required=True)
    ap.add_argument("--version", default="b2_us")
    ap.add_argument("--out", required=True)
    ap.add_argument("--all", action="store_true", help="also compile complete files")
    ap.add_argument("files", nargs="*")
    args = ap.parse_args(argv)
    root = Path(args.root).resolve()
    tools = Tools(root)

    want = set(args.files)
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        for f in entries(root, args.version):
            if want and f["path"] not in want:
                continue
            if not want and f["complete"] and not args.all:
                continue
            src = root / f["path"]
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
            obj, err = tools.compile(src, args.version, Path(tmp))
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
    with open(args.out, "w") as fh:
        fh.write("\t".join(COLUMNS) + "\n")
        for r in rows:
            fh.write("\t".join(str(r.get(c, "")) for c in COLUMNS) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
