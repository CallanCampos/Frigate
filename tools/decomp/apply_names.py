#!/usr/bin/env python3
"""Replace default symbol names in the sources with the names config/names.txt records.

After a merge, code from another branch may still call a function by its `func_XXXXXXXX` name (or use a `data_` name)
that this branch has renamed. The primary version's symbols.txt has the current names; every source identifier
`func_<addr>`, `data_<addr>` or `func_ovNNN_<addr>` whose address now has another name is rewritten.

    apply_names.py            # rewrite src/, include/ and lib/
    apply_names.py --dry-run  # only list what would change
"""
import argparse
import re
import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, SYMBOL_RE  # noqa: E402

PRIMARY = "b2_us"
DEFAULT_RE = re.compile(r"\b(?:func|data)_(?:ov(\d{3})_)?([0-9a-f]{8})\b")
IDENTIFIER_RE = re.compile(r"[A-Za-z_]\w*")
SOURCE_DIRS = ["src", "include", "lib"]


def current_names() -> dict[tuple[str, int], str]:
    """Returns (module, address) -> name from every symbols.txt of the primary version."""
    arm9 = ROOT / "config" / PRIMARY / "arm9"
    names = {}
    for path in arm9.rglob("symbols.txt"):
        module = "." if path.parent == arm9 else path.parent.name
        for line in path.read_text().splitlines():
            m = SYMBOL_RE.match(line)
            if m:
                names[(module, int(m.group(4), 16) & ~1)] = m.group(1)
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    names = current_names()

    def replace(m: re.Match) -> str:
        module = f"ov{m.group(1)}" if m.group(1) else "."
        name = names.get((module, int(m.group(2), 16)))
        # Labels (`.L_...`) and default names are not names to give
        if name is None or DEFAULT_RE.fullmatch(name) or not IDENTIFIER_RE.fullmatch(name):
            return m.group(0)
        return name

    for directory in SOURCE_DIRS:
        for path in sorted((ROOT / directory).rglob("*")):
            if path.suffix not in (".c", ".h", ".inc"):
                continue
            text = path.read_text()
            new = DEFAULT_RE.sub(replace, text)
            if new != text:
                changed = sorted({m.group(0) for m in DEFAULT_RE.finditer(text) if replace(m) != m.group(0)})
                print(f"{path.relative_to(ROOT)}: {', '.join(changed)}")
                if not args.dry_run:
                    path.write_text(new)


if __name__ == "__main__":
    main()
