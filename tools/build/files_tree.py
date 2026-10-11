#!/usr/bin/env python3
"""Make the file system that the ROM is built from, out of the extracted files and the files built from source.

The output mirrors the extracted directory with symbolic links, except for the built files, which the build writes
into the output directory itself. Only the directories that lead to a built file are real, so the tree stays small.

    files_tree.py extract/b2_us/files build/b2_us/files a/1/6/9 ...
"""
import argparse
import os
from pathlib import Path


def link(source: Path, output: Path):
    target = os.path.relpath(source, output.parent)
    if output.is_symlink():
        if os.readlink(output) == target:
            return
        output.unlink()
    output.symlink_to(target)


def mirror(source: Path, output: Path, built: set[Path], prefix: Path):
    output.mkdir(parents=True, exist_ok=True)
    for child in sorted(source.iterdir()):
        path = prefix / child.name
        if path in built:
            continue
        if any(path in b.parents for b in built):
            if (output / child.name).is_symlink():
                (output / child.name).unlink()
            mirror(child, output / child.name, built, path)
        else:
            link(child, output / child.name)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path, help="the extracted files directory")
    parser.add_argument("output", type=Path)
    parser.add_argument("built", type=Path, nargs="*", help="paths of the built files, relative to the output")
    parser.add_argument("--stamp", type=Path, help="file to touch when done")
    args = parser.parse_args()

    built = set(args.built)
    for path in built:
        if not (args.source / path).is_file():
            parser.error(f"{path} is not an extracted file")
    mirror(args.source, args.output, built, Path())
    if args.stamp:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.touch()


if __name__ == "__main__":
    main()
