#!/usr/bin/env python3
"""Compile a C file with several CodeWarrior versions and compare each function against the original game code.

Relocated bytes (calls and pointers) are ignored, since the probe objects are not linked.
"""
import argparse
import difflib
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

import capstone
import yaml
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(ROOT))
from configure import INCLUDE_DIRS, library_of  # noqa: E402

DEFAULT_FLAGS = (
    "-O4,p -proc arm946e -thumb -interworking -enum int -char signed -fp soft -lang=c99 -Cpp_exceptions off -gccext,on -gccinc "
    "-inline on,noauto -ipa file -requireprotos -nolink -msgstyle gcc -w off"
)
# Preprocessor defines of each game version, as in configure.py
VERSION_DEFINES = {"b2_us": ["BLACK2"], "w2_us": ["WHITE2"]}
SYMBOL_RE = re.compile(r"^(\S+) kind:function\((\w+),size=(0x[0-9a-f]+)\S*\) addr:(0x[0-9a-f]+)")


def lib_compiler(source: Path) -> tuple[str, str] | None:
    """Returns the compiler and flags of a library built with its own, from its library.toml as in configure.py."""
    try:
        library = library_of(source)
    except ValueError:
        return None
    return (library[0], " ".join(library[1]) + " -w off") if library else None


def load_modules(version: str) -> dict[str, tuple[Path, int]]:
    """Returns the binary and base address of every module, keyed by its config directory."""
    extract = ROOT / "extract" / version
    config = ROOT / "config" / version / "arm9"
    arm9 = yaml.safe_load((extract / "arm9" / "arm9.yaml").read_text())
    modules = {str(config): (extract / "arm9" / "arm9.bin", arm9["base_address"])}
    for name in ("itcm", "dtcm"):
        info = yaml.safe_load((extract / "arm9" / f"{name}.yaml").read_text())
        modules[str(config / name)] = (extract / "arm9" / f"{name}.bin", info["base_address"])
    overlays = yaml.safe_load((extract / "arm9_overlays" / "overlays.yaml").read_text())["overlays"]
    for overlay in overlays:
        path = extract / "arm9_overlays" / overlay["file_name"]
        modules[str(config / "overlays" / f"ov{overlay['id']:03d}")] = (path, overlay["base_address"])
    return modules


def find_function(version: str, name: str, modules) -> tuple[bytes, int, bool] | None:
    """Returns the original bytes, address and Thumb flag of a function."""
    config = ROOT / "config" / version / "arm9"
    for symbols in config.rglob("symbols.txt"):
        for line in symbols.read_text().splitlines():
            if not line.startswith(name + " "):
                continue
            match = SYMBOL_RE.match(line)
            if not match:
                continue
            mode, size, addr = match.group(2), int(match.group(3), 16), int(match.group(4), 16)
            if "dsprot=" in line:
                # DS Protect's functions are encrypted in the ROM; dsd decrypts them into the delinked objects
                data = delinked_function(version, name)
                if data is None:
                    print(f"{name}: encrypted, and not in build/{version}/delinks; run ninja first", file=sys.stderr)
                    return None
                return data[:size], addr, mode == "thumb"
            binary, base = modules[str(symbols.parent)]
            data = binary.read_bytes()[addr - base : addr - base + size]
            return data, addr, mode == "thumb"
    return None


_delinked: dict[str, dict[str, bytes]] = {}


def delinked_function(version: str, name: str) -> bytes | None:
    """Returns a function's bytes from the delinked objects of build/<version>/delinks, which hold DS Protect's code
    decrypted."""
    if version not in _delinked:
        found: dict[str, bytes] = {}
        for path in (ROOT / "build" / version / "delinks").rglob("*.o"):
            with path.open("rb") as f:
                elf = ELFFile(f)
                symtab = elf.get_section_by_name(".symtab")
                if symtab is None:
                    continue
                for symbol in symtab.iter_symbols():
                    if symbol["st_info"]["type"] != "STT_FUNC" or symbol["st_shndx"] in ("SHN_UNDEF", "SHN_ABS"):
                        continue
                    start = symbol["st_value"] & ~1
                    data = elf.get_section(symbol["st_shndx"]).data()
                    found[symbol.name] = data[start : start + symbol["st_size"]] if symbol["st_size"] else data[start:]
        _delinked[version] = found
    return _delinked[version].get(name)


def compiled_functions(obj: Path) -> dict[str, tuple[bytes, set[int]]]:
    """Returns the bytes and relocated offsets of every function in a compiled object."""
    functions = {}
    with obj.open("rb") as f:
        elf = ELFFile(f)
        relocated: dict[int, set[int]] = {}
        for section in elf.iter_sections():
            if isinstance(section, RelocationSection):
                target = section["sh_info"]
                offsets = relocated.setdefault(target, set())
                for reloc in section.iter_relocations():
                    offsets.update(range(reloc["r_offset"], reloc["r_offset"] + 4))
        symtab = elf.get_section_by_name(".symtab")
        for symbol in symtab.iter_symbols():
            if symbol["st_info"]["type"] != "STT_FUNC" or symbol["st_shndx"] in ("SHN_UNDEF", "SHN_ABS"):
                continue
            if symbol.name.startswith("$"):
                continue
            section = elf.get_section(symbol["st_shndx"])
            start = symbol["st_value"] & ~1
            size = symbol["st_size"]
            data = section.data()[start : start + size]
            masked = {o - start for o in relocated.get(symbol["st_shndx"], set()) if start <= o < start + size}
            functions[symbol.name] = (data, masked)
    return functions


def defined_functions(source: Path) -> list[str]:
    """The functions a C file defines at the top level, other than inline ones, which aren't emitted on their own."""
    text = source.read_text(errors="replace")
    pattern = r"^(?![ \t#/])(?!.*\binline\b)[^;{}()=\n]*?\b(\w+)\([^;{}]*\)\s*\{"
    return [m.group(1) for m in re.finditer(pattern, text, re.M)]


def disassemble(data: bytes, address: int, thumb: bool) -> list[str]:
    md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB if thumb else capstone.CS_MODE_ARM)
    # Literal pools sit between instructions, and capstone stops at the first word it can't decode, which would cut
    # the listing (and the diff) short there. Skipping such words as data keeps going to the end of the function.
    md.skipdata = True
    return [f"{i.address:08x}: {i.mnemonic} {i.op_str}" for i in md.disasm(data, address)]


def aligned_diff(ours: list[str], theirs: list[str]) -> list[str]:
    """Returns the hunks that differ between two disassemblies, aligned by difflib so that an instruction added or
    left out shifts nothing after it. Addresses, branch targets and literal pool offsets are ignored."""

    def key(line: str) -> str:
        ins = line.split(": ", 1)[-1]
        ins = re.sub(r"#0x[0-9a-f]{7}\b", "#ADDR", ins)
        return re.sub(r"\[pc, #0x[0-9a-f]+\]", "[pc]", ins)

    a, b = [key(line) for line in ours], [key(line) for line in theirs]
    lines = []
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes():
        if tag == "equal":
            continue
        lines.append(f"@@ ours {ours[i1][:8] if i1 < len(ours) else '-'} / original {theirs[j1][:8] if j1 < len(theirs) else '-'}")
        for k in range(max(i2 - i1, j2 - j1)):
            lines.append(f"  {a[i1 + k] if i1 + k < i2 else '':40s} | {b[j1 + k] if j1 + k < j2 else ''}")
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--version", default="b2_us", help="game version")
    parser.add_argument(
        "--compilers",
        help="comma-separated dsi compiler versions, or 'all' (the default); "
        "other builds by their directory under tools/mwccarm, as in 2.0/sp2p2. "
        "A library with its own compiler in configure.py defaults to that one and its flags",
    )
    parser.add_argument("--flags")
    parser.add_argument("--extra-flags", default="", help="flags appended to --flags")
    parser.add_argument("--opt", help="optimization flags replacing -O4,p, e.g. -O4,s")
    parser.add_argument("--show-diff", help="compiler version to show a disassembly diff for")
    parser.add_argument("--functions", help="comma-separated functions to report, and to diff; all by default")
    parser.add_argument("--mismatches", action="store_true", help="leave out the functions every compiler matches")
    parser.add_argument("--align", action="store_true",
                        help="show only the differing hunks of the diff, aligned so that one instruction more or less "
                        "does not shift the rest")
    args = parser.parse_args()

    lib = lib_compiler(args.source)
    if args.compilers is None:
        args.compilers = lib[0] if lib else "all"
    if args.flags is None:
        args.flags = lib[1] if lib else DEFAULT_FLAGS
    functions = set(args.functions.split(",")) if args.functions else None

    if args.opt:
        args.flags = args.flags.replace("-O4,p", args.opt)
    compilers_dir = TOOLS / "mwccarm" / "dsi"
    compilers = sorted(p.name for p in compilers_dir.iterdir()) if args.compilers == "all" else args.compilers.split(",")
    modules = load_modules(args.version)

    results: dict[str, dict[str, str]] = {}
    with tempfile.TemporaryDirectory() as tmp:
        for compiler in compilers:
            obj = Path(tmp) / f"{compiler.replace('/', '_')}.o"
            command = [
                str(TOOLS / "wibo"),
                str((TOOLS / "mwccarm" / compiler if "/" in compiler else compilers_dir / compiler) / "mwccarm.exe"),
                *shlex.split(args.flags),
                *shlex.split(args.extra_flags),
                *(arg for define in VERSION_DEFINES.get(args.version, []) for arg in ("-d", define)),
                *(arg for d in INCLUDE_DIRS for arg in ("-i", str(ROOT / d))),
                "-o", str(obj),
                str(args.source),
            ]
            proc = subprocess.run(command, capture_output=True, text=True)
            if proc.returncode != 0:
                print(f"{compiler}: compile failed\n{proc.stdout}{proc.stderr}")
                continue
            compiled = compiled_functions(obj)
            # MWCC drops a static function that nothing references, which would otherwise pass unnoticed
            for name in defined_functions(args.source):
                if name not in compiled and (functions is None or name in functions):
                    results.setdefault(name, {})[compiler] = "not emitted"
            for name, (data, masked) in compiled.items():
                if functions is not None and name not in functions:
                    continue
                original = find_function(args.version, name, modules)
                if original is None:
                    results.setdefault(name, {})[compiler] = "unknown"
                    continue
                orig_data, addr, thumb = original
                if len(orig_data) != len(data):
                    status = f"size {len(data):#x}/{len(orig_data):#x}"
                else:
                    diffs = sum(1 for i in range(len(data)) if i not in masked and data[i] != orig_data[i])
                    status = "MATCH" if diffs == 0 else f"{diffs} bytes"
                results.setdefault(name, {})[compiler] = status

                if args.show_diff == compiler and status != "MATCH":
                    ours = disassemble(data, addr, thumb)
                    theirs = disassemble(orig_data, addr, thumb)
                    print(f"--- {name} ({compiler}): ours | original")
                    if args.align:
                        print("\n".join(aligned_diff(ours, theirs)))
                        continue
                    for i in range(max(len(ours), len(theirs))):
                        a = ours[i] if i < len(ours) else ""
                        b = theirs[i] if i < len(theirs) else ""
                        mark = " " if a.split(":", 1)[-1] == b.split(":", 1)[-1] else "*"
                        print(f"{mark} {a:45s} | {b}")

    width = max((len(n) for n in results), default=8)
    print(f"{'function':{width}s}  " + "  ".join(f"{c:>12s}" for c in compilers))
    for name, row in results.items():
        if args.mismatches and all(status == "MATCH" for status in row.values()):
            continue
        print(f"{name:{width}s}  " + "  ".join(f"{row.get(c, '-'):>12s}" for c in compilers))


if __name__ == "__main__":
    sys.exit(main())
