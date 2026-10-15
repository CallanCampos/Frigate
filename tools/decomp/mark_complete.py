#!/usr/bin/env python3
"""Mark source files complete in every version's delinks.txt, once all their functions match.

Before marking, it checks that no function the file defines as `static` is called or referenced from another file.
Such a reference links to nothing once the file's object replaces the original code: the build then fails in the
referring module (the linker adds a veneer to address 0), which the per-function probe cannot see.

    mark_complete.py src/gfl/particle.c [src/...]
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import config_lock, reloc_module_names  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SYMBOL_RE = re.compile(r"(\S+) kind:\S+ addr:(0x[0-9a-f]+)")
RELOC_RE = re.compile(r"from:(0x[0-9a-f]+) kind:\S+ to:(0x[0-9a-f]+)(?: add:\S+)? module:(\S+)")
SECTION_RANGE_RE = re.compile(r"\s*(\S+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)")


def module_name(config_dir: Path, version_dir: Path) -> str:
    """The relocs' name for the module whose config is in config_dir."""
    if config_dir == version_dir:
        return "main"
    if config_dir.name.startswith("ov"):
        return f"overlay({int(config_dir.name[2:])})"
    return config_dir.name


def externally_used_statics(source: str, version_dir: Path) -> list[str]:
    """Returns static functions referenced outside their original source file."""
    text = (ROOT / source).read_text()
    for delinks in version_dir.rglob("delinks.txt"):
        lines = delinks.read_text().splitlines()
        if f"{source}:" not in lines:
            continue
        i = lines.index(f"{source}:") + 1
        ranges = []
        owner_ranges = []
        while i < len(lines) and lines[i].startswith((" ", "\t")):
            m = SECTION_RANGE_RE.match(lines[i])
            if m:
                bounds = (int(m.group(2), 16), int(m.group(3), 16))
                owner_ranges.append(bounds)
                if m.group(1) == ".text":
                    ranges.append(bounds)
            i += 1
        own = module_name(delinks.parent, version_dir)
        own_modules = reloc_module_names(own)
        # Labels may alias a function (including a local function). Keep every
        # name: symbols.txt order must not let an alias hide its static name.
        names: dict[int, set[str]] = {}
        for line in (delinks.parent / "symbols.txt").read_text().splitlines():
            m = SYMBOL_RE.match(line)
            if m:
                names.setdefault(int(m.group(2), 16) & ~1, set()).add(m.group(1))
        used = set()
        for relocs in version_dir.rglob("relocs.txt"):
            for line in relocs.read_text().splitlines():
                m = RELOC_RE.match(line)
                # References within this original file are allowed, including its
                # callback tables. Other files and unowned module gaps are external.
                if m and relocs.parent == delinks.parent:
                    origin = int(m.group(1), 16)
                    if any(start <= origin < end for start, end in owner_ranges):
                        continue
                # dsd v0.12.1-dsi.4's delinker uses first_module() for symbol
                # relocations, so overlays(11,12) binds only overlay 11.
                targets = reloc_module_names(m.group(3)) if m else []
                if m and (m.group(3) == own or (own_modules and targets[:1] == own_modules)):
                    target = int(m.group(2), 16) & ~1
                    if any(start <= target < end for start, end in ranges) and target in names:
                        used.update(names[target])
        return sorted(n for n in used if re.search(r"^static [^;{=]*\b%s\(" % re.escape(n), text, re.M))
    return []


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("sources", nargs="+")
    args = parser.parse_args()

    failed = False
    for version_dir in sorted(ROOT.glob("config/*/arm9")):
        for source in args.sources:
            statics = externally_used_statics(source, version_dir)
            if statics:
                print(f"{source}: static but referenced outside its source file in {version_dir.parent.name}: {', '.join(statics)}")
                failed = True
    if failed:
        return 1

    for delinks in sorted(ROOT.glob("config/*/arm9/**/delinks.txt")):
        lines = delinks.read_text().splitlines()
        changed = False
        for source in args.sources:
            header = f"{source}:"
            if header not in lines:
                continue
            i = lines.index(header)
            if i + 1 < len(lines) and lines[i + 1].strip() == "complete":
                print(f"{source} is already complete in {delinks.relative_to(ROOT)}")
                continue
            lines.insert(i + 1, "    complete")
            changed = True
            print(f"{source}: complete in {delinks.relative_to(ROOT)}")
        if changed:
            delinks.write_text("\n".join(lines) + "\n")
    return 0


if __name__ == "__main__":
    with config_lock():
        sys.exit(main())
