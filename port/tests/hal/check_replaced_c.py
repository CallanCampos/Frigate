#!/usr/bin/env python3
"""The SDK functions written in C that port/hal replaces because their decomp C isn't the ROM's code (ADR 0008), each
still a row of docs/nonmatching-functions.md in a file that isn't complete in either version's delinks.txt.

A function whose row goes away, or whose file becomes complete, matches the ROM now: its C is the original's, so the
host version, the SDK file's #ifndef PLATFORM_PC guard and their port/docs/game-changes.md row go, in one commit, after
the asm oracle's test of the products (port/tests/asm_oracle/test_fx.py) has passed on that C. This check fails as
soon as either happens, naming the function. It reads the repository only, so it runs in every build. Standard library
only.
"""

from __future__ import annotations

import os
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
# (the SDK file, its function, the host version's file)
REPLACED = [
    ("lib/nitro/src/fx/fx_mtx33.c", "MTX_Concat33", "port/hal/fx/fx_mtx33.c"),
    ("lib/nitro/src/fx/fx_mtx43.c", "MTX_Concat43", "port/hal/fx/fx_mtx43.c"),
    ("lib/nitro/src/fx/fx_mtx43.c", "MTX_MultVec43", "port/hal/fx/fx_mtx43.c"),
    ("lib/nitro/src/fx/fx_mtx44.c", "MTX_Concat44", "port/hal/fx/fx_mtx44.c"),
]


def complete_files(version: str) -> set[str]:
    """The files that delinks.txt marks complete"""
    complete = set()
    current = None
    with open(os.path.join(REPO_ROOT, "config", version, "arm9", "delinks.txt")) as f:
        for line in f:
            if line and not line[0].isspace() and line.rstrip().endswith(":"):
                current = line.rstrip()[:-1]
            elif current is not None and line.strip() == "complete":
                complete.add(current)
    return complete


def main() -> int:
    with open(os.path.join(REPO_ROOT, "docs", "nonmatching-functions.md")) as f:
        rows = f.read()
    complete = complete_files("b2_us") | complete_files("w2_us")
    failures = []
    matches = "upstream's C is the ROM's now: remove the host version (port/docs/adr/0008)"
    for path, function, host in REPLACED:
        if f"| `{path}` | `{function}` |" not in rows:
            failures.append(f"{function} ({path}) has no row in docs/nonmatching-functions.md: {matches}")
        if path in complete:
            failures.append(f"{path}, which has {function}, is complete: {matches}")
        with open(os.path.join(REPO_ROOT, host)) as f:
            if f"{function}(" not in f.read():
                failures.append(f"{host} has no {function}: remove it from this list too")
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print(f"{len(REPLACED)} functions replaced in port/hal, each still a nonmatching row of an incomplete file")
    return 0


if __name__ == "__main__":
    sys.exit(main())
