#!/usr/bin/env python3
"""Convert a dependency file that mwccarm wrote under wibo to Unix paths, for ninja.

mwccarm's `-gccdep -MD` writes paths like `Z:\\home\\...\\include\\types.h`. This rewrites the file in place with the
drive letter removed and forward slashes, keeping the backslashes that continue lines. Paths inside the checkout,
where ninja runs, become relative to it, as build.ninja names them: ninja takes an absolute path for another file, so
an object wouldn't recompile in the run that regenerates a header it includes, such as
build/include/generated/constants/items.h, but only in the next.

    fix_depfile.py build/b2_us/src/gfl/vm.d
"""
import os
import re
import sys
from pathlib import Path


def unix_path(path: str, root: str) -> str:
    """Returns a path without its drive letter and with forward slashes, relative to root if it is inside it."""
    return re.sub(r"^[A-Za-z]:", "", path).replace("\\", "/").removeprefix(f"{root}/")


def fix_depfile(text: str, root: str) -> str:
    """Returns a dependency file's text with Unix paths, those inside root relative to it."""
    lines = []
    for line in text.splitlines():
        continued = line.endswith(" \\")
        body = line[:-2] if continued else line
        target, sep, rest = body.partition(": ")
        if sep and not body.startswith((" ", "\t")):
            body = unix_path(target, root) + ": " + " ".join(unix_path(p, root) for p in rest.split())
        else:
            body = "\t" + " ".join(unix_path(p, root) for p in body.split())
        lines.append(body + (" \\" if continued else ""))
    return "\n".join(lines) + "\n"


def main():
    path = Path(sys.argv[1])
    path.write_text(fix_depfile(path.read_text(), os.getcwd()))


if __name__ == "__main__":
    main()
