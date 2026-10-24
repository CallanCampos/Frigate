#!/usr/bin/env python3
"""Check that nothing in a version's link refers to the code that tools/build/deadstrip.py removed.

    check_deadstrip.py --config config/b2_us --unstripped build/b2_us/unstripped build/b2_us/stamps/deadstrip.ok \\
        @objects.txt

The strip only sees its own object. A call from another file to a symbol config/deadstrip.toml lists, or to a
function that only the dropped code called and nothing defines (one the original link dropped from another file),
would link through a veneer to address 0 instead of failing, which only the ROM checks of a matching build notice.
This reruns each listed source's strip on its object under --unstripped, then reads every object given (the compiled
ones, the stripped ones included, and the delinked ones) and fails on each reference it finds to what a strip removed;
a name the version's symbols.txt files give, or that an object defines, is defined. It needs no ROM. On success it
writes the stamp file; on failure it removes it. An argument @FILE stands for the objects FILE lists, as ninja writes a
response file: one to a line, quoted as for the shell.
"""
import argparse
import shlex
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.build.deadstrip import (MANIFEST, DeadStripError, check_references, load_manifest,  # noqa: E402
                                   rom_symbol_names, strip)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--config", type=Path, required=True, help="the version's config directory, config/<version>")
    parser.add_argument("--unstripped", type=Path, required=True,
                        help="where the build compiles the listed sources, build/<version>/unstripped")
    parser.add_argument("--manifest", type=Path, default=MANIFEST, help="defaults to config/deadstrip.toml")
    parser.add_argument("stamp", type=Path)
    parser.add_argument("objects", nargs="*", help="every other object of the link, or @FILE listing them")
    args = parser.parse_args()
    args.stamp.unlink(missing_ok=True)
    try:
        objects = []
        for argument in args.objects:
            if argument.startswith("@"):
                objects += [Path(p) for line in Path(argument[1:]).read_text().splitlines() for p in shlex.split(line)]
            else:
                objects.append(Path(argument))
        strips = []
        for source, dead in load_manifest(args.manifest, ROOT).items():
            obj = args.unstripped / Path(source).with_suffix(".o")
            strips.append((source, dead, strip(obj.read_bytes(), dead)))
        errors = check_references(strips, ((str(obj), obj.read_bytes()) for obj in objects),
                                  rom_symbol_names(args.config))
    except (DeadStripError, OSError) as error:
        errors = [str(error)]
    if errors:
        for error in errors:
            print(f"check_deadstrip.py: error: {error}", file=sys.stderr)
        return 1
    args.stamp.parent.mkdir(parents=True, exist_ok=True)
    args.stamp.touch()
    return 0


if __name__ == "__main__":
    sys.exit(main())
