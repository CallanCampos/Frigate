#!/usr/bin/env python3
"""Convert a dependency file that mwccarm wrote under wibo to Unix paths, for ninja.

mwccarm's `-gccdep -MD` writes paths like `Z:\\home\\...\\include\\types.h`. This rewrites the file in place with the
drive letter removed and forward slashes, keeping the backslashes that continue lines.

    fix_depfile.py build/b2_us/src/gfl/vm.d
"""
import re
import sys
from pathlib import Path


def unix_path(path: str) -> str:
    return re.sub(r"^[A-Za-z]:", "", path).replace("\\", "/")


def main():
    path = Path(sys.argv[1])
    lines = []
    for line in path.read_text().splitlines():
        continued = line.endswith(" \\")
        body = line[:-2] if continued else line
        target, sep, rest = body.partition(": ")
        if sep and not body.startswith((" ", "\t")):
            body = unix_path(target) + ": " + " ".join(unix_path(p) for p in rest.split())
        else:
            body = "\t" + " ".join(unix_path(p) for p in body.split())
        lines.append(body + (" \\" if continued else ""))
    path.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
