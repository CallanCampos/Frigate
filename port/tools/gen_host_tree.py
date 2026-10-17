#!/usr/bin/env python3
"""Prepares the generated inputs of the host build, from the decomp's tree, without a base ROM.

    gen_host_tree.py --out build/port/gen

It writes:
  - the headers that ninja generates for the ROM build (data constants, message IDs, easy chat words), into
    build/include/generated as compile_all.py does, since the sources include them;
  - include/: a mirror of the decomp's include/ made of symbolic links, without the C library headers the decomp
    keeps there for MWCC (stdio.h, string.h and the rest), so that the host's own C library headers are found instead;
  - sources.txt: every C file of src/ and lib/ that the host build compiles, one path per line, relative to the
    repository: all of them except the files port/hal/replaced.txt lists.

Files are rewritten only when their contents change, so that CMake rebuilds only what depends on a change.
"""
import argparse
import fnmatch
import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.decomp.compile_all import generate_constants  # noqa: E402

# The decomp's own C library headers in include/, written for MWCC's MSL. The host build uses the host's C library
LIBC_HEADERS = {
    "ctype.h", "errno.h", "float.h", "limits.h", "locale.h", "math.h", "stdarg.h", "stddef.h", "stdio.h",
    "stdlib.h", "string.h", "wchar.h",
}


def write_if_changed(path: Path, text: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def mirror_include(out: Path):
    """Links every entry of include/ except the C library headers into out, and removes stale links."""
    source = ROOT / "include"
    out.mkdir(parents=True, exist_ok=True)
    wanted = {entry.name for entry in source.iterdir() if entry.name not in LIBC_HEADERS}
    for link in out.iterdir():
        if link.name not in wanted:
            link.unlink()
    for name in sorted(wanted):
        link = out / name
        target = os.path.relpath(source / name, out)
        if link.is_symlink() and os.readlink(link) == target:
            continue
        if link.is_symlink() or link.exists():
            link.unlink()
        link.symlink_to(target)


def read_replaced() -> list[tuple[str, str]]:
    """Returns the patterns of port/hal/replaced.txt with their reasons: a pattern, then " # " and the reason."""
    patterns = []
    for number, line in enumerate((ROOT / "port" / "hal" / "replaced.txt").read_text().splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        pattern, _, reason = line.partition(" # ")
        if not reason:
            sys.exit(f"port/hal/replaced.txt:{number}: a pattern needs a reason after ' # '")
        patterns.append((pattern.strip(), reason.strip()))
    return patterns


def list_sources() -> list[str]:
    """Every C file the host build compiles, sorted: the ones compile_all.py compiles, minus replaced.txt."""
    sources = sorted(p.relative_to(ROOT).as_posix() for pattern in ("src/**/*.c", "lib/*/src/**/*.c")
                     for p in ROOT.glob(pattern))
    patterns = read_replaced()
    unused = {pattern for pattern, _ in patterns}
    kept = []
    for source in sources:
        matched = [pattern for pattern, _ in patterns if fnmatch.fnmatchcase(source, pattern)]
        unused.difference_update(matched)
        if not matched:
            kept.append(source)
    for pattern in sorted(unused):
        print(f"gen_host_tree: replaced.txt pattern matches no source file: {pattern}", file=sys.stderr)
    return kept


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True, help="directory for the generated files")
    args = parser.parse_args()
    out = args.out if args.out.is_absolute() else ROOT / args.out

    generate_constants()
    mirror_include(out / "include")
    write_if_changed(out / "sources.txt", "".join(f"{source}\n" for source in list_sources()))


if __name__ == "__main__":
    main()
