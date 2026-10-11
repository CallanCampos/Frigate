#!/usr/bin/env python3
"""Print where MWCC put each parameter and local variable of a function: its register or stack slot.

The build compiles with `-sym on`, so every object has DWARF debug info that names the register or the offset from
`sp` of each variable. A register swap or a stack slot mismatch then shows which variable to move, instead of trying
declaration orders blindly. Variables of inner blocks are indented under their block. MWCC records one location per
variable, that of its first live range. It reads the objects with LLVM's `llvm-dwarfdump`, since pyelftools does not
apply the RELA relocations of MWCC's debug sections.

    locals.py src/ov033/trial_house.c func_ov033_0217acd4    # builds the object with ninja first
    locals.py build/b2_us/src/ov033/trial_house.o CreateTrialHouseWk func_ov033_0217acd4
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TAG_RE = re.compile(r"^0x[0-9a-f]+:( +)(DW_TAG_\w+|NULL)")
ATTR_RE = re.compile(r"^ +(DW_AT_\w+)\s+\((.*)\)$")
REG_RE = re.compile(r"DW_OP_reg\d+ (\w+)")
FBREG_RE = re.compile(r"DW_OP_fbreg ([+-]\d+)")


def parse_dies(text: str) -> list[tuple[int, str, dict[str, str]]]:
    """Returns each DIE of llvm-dwarfdump's output as (depth, tag, attributes)."""
    dies = []
    for line in text.splitlines():
        if m := TAG_RE.match(line):
            dies.append((len(m.group(1)), m.group(2), {}))
        elif (m := ATTR_RE.match(line)) and dies:
            dies[-1][2][m.group(1)] = m.group(2)
    return dies


def location(attrs: dict[str, str]) -> str:
    loc = attrs.get("DW_AT_location")
    if loc is None:
        return "(none)"
    if m := REG_RE.fullmatch(loc):
        return m.group(1).lower()
    if m := FBREG_RE.fullmatch(loc):
        return f"sp+{int(m.group(1)):#x}"
    return loc


def type_name(attrs: dict[str, str]) -> str:
    m = re.search(r'"(.*)"', attrs.get("DW_AT_type", ""))
    return m.group(1) if m else "?"


def print_function(dies: list[tuple[int, str, dict[str, str]]], start: int):
    """Prints the variables of the subprogram at dies[start], down to where it ends."""
    base = dies[start][0]
    blocks = []
    for depth, tag, attrs in dies[start + 1:]:
        while blocks and depth <= blocks[-1]:
            blocks.pop()
            print("    " * len(blocks) + "  }")
        if depth <= base:
            break
        indent = "    " * len(blocks)
        if tag in ("DW_TAG_formal_parameter", "DW_TAG_variable"):
            kind = "param" if tag == "DW_TAG_formal_parameter" else "local"
            line = attrs.get("DW_AT_decl_line", "?")
            name = attrs.get("DW_AT_name", "?").strip('"')
            print(f"{indent}  {location(attrs):<10} {kind:<5} line {line:<5} {type_name(attrs)} {name}")
        elif tag == "DW_TAG_lexical_block":
            print(f"{indent}  {{")
            blocks.append(depth)
    while blocks:
        blocks.pop()
        print("    " * len(blocks) + "  }")


def object_for(path: Path, version: str) -> Path:
    """The object of a source file, built with ninja; an object path is returned as it is."""
    if path.suffix == ".o":
        return path
    obj = Path("build") / version / path.with_suffix(".o")
    subprocess.run(["ninja", str(obj)], cwd=ROOT, check=True, stdout=subprocess.DEVNULL)
    return ROOT / obj


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path, help="a source file under src/, or a built object")
    parser.add_argument("functions", nargs="+")
    parser.add_argument("--version", default="b2_us")
    args = parser.parse_args()

    text = subprocess.run(["llvm-dwarfdump", "--debug-info", str(object_for(args.source, args.version))],
                          check=True, capture_output=True, text=True).stdout
    dies = parse_dies(text)
    wanted = set(args.functions)
    for i, (_, tag, attrs) in enumerate(dies):
        name = attrs.get("DW_AT_name", "").strip('"')
        if tag == "DW_TAG_subprogram" and name in wanted and "DW_AT_low_pc" in attrs:
            print(name)
            print_function(dies, i)
            wanted.discard(name)
    for name in sorted(wanted):
        print(f"{name}: not found", file=sys.stderr)


if __name__ == "__main__":
    main()
