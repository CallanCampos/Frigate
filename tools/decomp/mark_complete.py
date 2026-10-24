#!/usr/bin/env python3
"""Mark source files complete in every version's delinks.txt, once all their functions match.

Before marking, it checks that no function the file defines as `static` is called or referenced from another file.
Such a reference links to nothing once the file's object replaces the original code: the build then fails in the
referring module (the linker adds a veneer to address 0), which the per-function probe cannot see.

    mark_complete.py src/gfl/particle.c [src/...]

Use --incomplete after a natural C rewrite no longer matches. This removes the complete marker in every registered
version, restoring the original assembly fallback without checking static references in the unused C object.
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import config_lock, reloc_module_names, section_module  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SYMBOL_RE = re.compile(r"(\S+) kind:\S+ addr:(0x[0-9a-f]+)")
RELOC_RE = re.compile(r"from:(0x[0-9a-f]+) kind:\S+ to:(0x[0-9a-f]+)(?: add:\S+)? module:(\S+)")
SECTION_RANGE_RE = re.compile(r"\s*(\S+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)")


def entry_ranges(source: str, version_dir: Path) -> dict[Path, list[tuple[str, int, int]]]:
    """The section ranges of a source's entry in a version, each as (section, start, end), keyed by the config
    directory of the module whose addresses it is at: an ARM9 main file's migrated .ltdmain sections are the LTD
    autoload's, and link from the file's one object, so the file has code in both modules."""
    found: dict[Path, list[tuple[str, int, int]]] = {}
    for delinks in version_dir.rglob("delinks.txt"):
        lines = delinks.read_text().splitlines()
        if f"{source}:" not in lines:
            continue
        module = delinks.parent.relative_to(version_dir).as_posix()
        i = lines.index(f"{source}:") + 1
        while i < len(lines) and lines[i].startswith((" ", "\t")):
            if m := SECTION_RANGE_RE.match(lines[i]):
                placed = version_dir / section_module(module, m.group(1))
                found.setdefault(placed, []).append((m.group(1), int(m.group(2), 16), int(m.group(3), 16)))
            i += 1
    return found


def module_name(config_dir: Path, version_dir: Path) -> str:
    """The relocs' name for the module whose config is in config_dir."""
    if config_dir == version_dir:
        return "main"
    if config_dir.name.startswith("ov"):
        return f"overlay({int(config_dir.name[2:])})"
    if match := re.fullmatch(r"(ltd_)?autoload_(\d+)", config_dir.name):
        return f"{match.group(1) or ''}autoload({match.group(2)})"
    return config_dir.name


def externally_used_statics(source: str, version_dir: Path) -> list[str]:
    """Returns static functions referenced outside their original source file."""
    text = (ROOT / source).read_text()
    modules = entry_ranges(source, version_dir)
    # References from any of the file's ranges are its own, across modules too: a migrated section's code reaches the
    # main part's statics, as the main part's reaches its
    owned = {module: [(start, end) for _, start, end in ranges] for module, ranges in modules.items()}
    used = set()
    for module, ranges in modules.items():
        # Code: .text, and .ltdmain, which holds what was built in TwlSDK's LTDMAIN section
        code = [(start, end) for section, start, end in ranges if section in (".text", ".ltdmain")]
        own = module_name(module, version_dir)
        own_modules = reloc_module_names(own)
        # Labels may alias a function (including a local function). Keep every
        # name: symbols.txt order must not let an alias hide its static name.
        names: dict[int, set[str]] = {}
        for line in (module / "symbols.txt").read_text().splitlines():
            m = SYMBOL_RE.match(line)
            if m:
                names.setdefault(int(m.group(2), 16) & ~1, set()).add(m.group(1))
        for relocs in version_dir.rglob("relocs.txt"):
            for line in relocs.read_text().splitlines():
                m = RELOC_RE.match(line)
                if not m:
                    continue
                # References within this original file are allowed, including its
                # callback tables. Other files and unowned module gaps are external.
                origin = int(m.group(1), 16)
                if any(start <= origin < end for start, end in owned.get(relocs.parent, ())):
                    continue
                # dsd v0.12.1-dsi.4's delinker uses first_module() for symbol
                # relocations, so overlays(11,12) binds only overlay 11.
                targets = reloc_module_names(m.group(3))
                if m.group(3) == own or (own_modules and targets[:1] == own_modules):
                    target = int(m.group(2), 16) & ~1
                    if any(start <= target < end for start, end in code) and target in names:
                        used.update(names[target])
    return sorted(n for n in used if re.search(r"^static [^;{=]*\b%s\(" % re.escape(n), text, re.M))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("sources", nargs="+")
    parser.add_argument("--incomplete", action="store_true", help="restore original assembly fallback after a natural C nonmatch")
    args = parser.parse_args()

    # A source can legitimately exist in only one version. Require at least one owner, and reject duplicate or
    # malformed owners before writing any version or any of the other requested sources.
    owners = {source: set() for source in args.sources}
    planned = []
    messages = []
    for delinks in sorted(ROOT.glob("config/*/arm9/**/delinks.txt")):
        lines = delinks.read_text().splitlines()
        original = lines[:]
        version = delinks.relative_to(ROOT / "config").parts[0]
        for source in owners:
            header = f"{source}:"
            count = lines.count(header)
            if not count:
                continue
            if count != 1 or version in owners[source]:
                print(f"{source}: duplicate owner in {version}")
                return 1
            owners[source].add(version)
            i = lines.index(header)
            end = i + 1
            while end < len(lines) and lines[end].startswith((" ", "\t")):
                end += 1
            ranges = [SECTION_RANGE_RE.fullmatch(line) for line in lines[i + 1:end] if line.strip() != "complete"]
            if not ranges or any(m is None or int(m.group(2), 16) > int(m.group(3), 16) for m in ranges):
                print(f"{source}: invalid section ranges in {delinks.relative_to(ROOT)}")
                return 1
            markers = [j for j in range(i + 1, end) if lines[j].strip() == "complete"]
            if args.incomplete:
                for j in reversed(markers):
                    del lines[j]
                state = "incomplete" if markers else "already incomplete"
            elif markers:
                state = "already complete"
            else:
                lines.insert(i + 1, "    complete")
                state = "complete"
            messages.append(f"{source}: {state} in {delinks.relative_to(ROOT)}")
        if lines != original:
            planned.append((delinks, "\n".join(lines) + "\n"))
    for source, versions in owners.items():
        if not versions:
            print(f"{source}: no registered source in any version")
            return 1

    failed = False
    for version_dir in ([] if args.incomplete else sorted(ROOT.glob("config/*/arm9"))):
        for source in args.sources:
            statics = externally_used_statics(source, version_dir)
            if statics:
                print(f"{source}: static but referenced outside its source file in {version_dir.parent.name}: {', '.join(statics)}")
                failed = True
    if failed:
        return 1

    for delinks, text in planned:
        delinks.write_text(text)
    for message in messages:
        print(message)
    return 0


if __name__ == "__main__":
    with config_lock():
        sys.exit(main())
