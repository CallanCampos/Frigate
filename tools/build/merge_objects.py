#!/usr/bin/env python3
"""Link the parts in config/merged_sources.toml from their whole's object.

    merge_objects.py build/b2_us/arm9.lcf build/b2_us/objects.txt build/b2_us/arm9_merged.lcf build/b2_us/objects_merged.txt

dsd lists each part of a merged file in its own module's delinks.txt, so its linker script places the part's sections
from an object of the part's name, and its object list has that object. For a part whose code is in its whole's source,
this places those sections from the whole's object instead, the part's .bss from the whole's .ltdmain.bss, where
TwlSDK's LTDMAIN section puts the part's statics, and lists the whole's object in the part's place. mwldarm makes the
long-branch veneers object by object, in the list's order, and a later object's call shares a veneer in range: the
original's LTD code takes its veneers in the order of the LTD autoload's objects, as when card_hash.c makes the veneer
of OS_DisableInterrupts that pm.c's DSi part shares, not the other way round.
"""
import argparse
import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / "config/merged_sources.toml"
# Each section that a part may have, and the whole's section that holds it: TwlSDK's LTDMAIN section puts the part's
# code and data in .ltdmain and its statics in .ltdmain.bss. Any other section of a part would be placed from the
# whole's main sections, which hold the main part's code and data, so it stops the build
PART_SECTIONS = {".ltdmain": ".ltdmain", ".bss": ".ltdmain.bss"}


def merged_sources(path: Path = CONFIG) -> dict[str, str]:
    """Each part's source path, mapped to its whole's."""
    if not path.exists():
        return {}
    return tomllib.loads(path.read_text()).get("merged", {})


def listed(lines: list[str], source: str) -> str:
    """The object list's line of a source's object."""
    suffix = "/" + Path(source).with_suffix(".o").as_posix() + '"'
    found = [line for line in lines if line.endswith(suffix)]
    if len(found) != 1:
        sys.exit(f"merge_objects.py: the object list has {len(found)} lines for {Path(suffix[1:-1]).name}")
    return found[0]


def merge(lcf: str, objects: str, merged: dict[str, str]) -> tuple[str, str]:
    for part, whole in merged.items():
        part_object = Path(part).with_suffix(".o").name
        whole_object = Path(whole).with_suffix(".o").name
        lines = objects.splitlines()
        part_line = listed(lines, part)
        whole_line = listed(lines, whole)
        # An incomplete file links its delinked object, from build/<version>/delinks. The parts are one object, so
        # both link the original code, apart, or both the whole's
        delinked = ("/delinks/" in part_line, "/delinks/" in whole_line)
        if all(delinked):
            continue
        if any(delinked):
            sys.exit(f"merge_objects.py: {part} and {whole} are one object; mark both complete or neither")
        placed = re.compile(r"^(\s*)" + re.escape(part_object) + r"\((\S+)\)$", re.M)
        sections = [match.group(2) for match in placed.finditer(lcf)]
        if not sections:
            sys.exit(f"merge_objects.py: the linker script places nothing from {part_object}")
        unknown = sorted(set(sections) - PART_SECTIONS.keys())
        if unknown:
            sys.exit(f"merge_objects.py: {part} has {', '.join(unknown)}, which its whole's object has no section for")

        def place(match: re.Match) -> str:
            return f"{match.group(1)}{whole_object}({PART_SECTIONS[match.group(2)]})"

        lcf = placed.sub(place, lcf)
        objects = "\n".join(whole_line if line == part_line else line for line in lines if line != whole_line) + "\n"
    return lcf, objects


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("lcf", type=Path, help="dsd's linker script")
    parser.add_argument("objects", type=Path, help="dsd's objects.txt")
    parser.add_argument("lcf_output", type=Path)
    parser.add_argument("objects_output", type=Path)
    args = parser.parse_args()
    lcf, objects = merge(args.lcf.read_text(), args.objects.read_text(), merged_sources())
    args.lcf_output.write_text(lcf)
    args.objects_output.write_text(objects)


if __name__ == "__main__":
    main()
