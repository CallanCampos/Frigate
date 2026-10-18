#!/usr/bin/env python3
r"""Checks that every game file of the host build that declares a struct gives its members the same offsets: a file that
declares its own copy of a struct whose header has another layout on a host reads the wrong bytes there, though both
agree on the DS (port/docs/game-changes.md has the trainer card's case).

    check_struct_layout.py --objects-dir build/port/CMakeFiles/pokebw2_game.dir --struct TrainerCardParam \
        --members result,appParam src/ov012/event_field_proclink.c src/ov012/trcard_sys.c

It reads each source file's object's debug information with llvm-dwarfdump (the RelWithDebInfo and Debug builds have
it) and compares the offsets of the named members of the typedef STRUCT. No offset is written here: the objects must
agree with each other, and the header's object (the last file named) is the reference. Exits 77 (skipped) without
llvm-dwarfdump or without debug information. Standard library only.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys

SKIP = 77
# Homebrew's LLVM, where macOS has no llvm-dwarfdump of its own on PATH
EXTRA_PATHS = ("/opt/homebrew/opt/llvm/bin", "/usr/local/opt/llvm/bin")


def find_dwarfdump() -> str | None:
    found = shutil.which("llvm-dwarfdump")
    if found:
        return found
    for directory in EXTRA_PATHS:
        path = os.path.join(directory, "llvm-dwarfdump")
        if os.access(path, os.X_OK):
            return path
    return None


def find_object(objects_dir: str, source: str) -> str | None:
    """CMake names an object after its source's absolute path below the target's directory, with .o, or .obj on
    Windows"""
    wanted = "/" + source.replace(os.sep, "/").lstrip("/")
    for root, _, files in os.walk(objects_dir):
        for name in files:
            path = os.path.join(root, name)
            if path.replace(os.sep, "/").endswith((wanted + ".o", wanted + ".obj")):
                return path
    return None


ENTRY = re.compile(r"^(0x[0-9a-f]+):( +)(DW_TAG_\w+)")
ATTRIBUTE = re.compile(r"^\s+(DW_AT_\w+)\s+\((.*)\)$")


def read_entries(dump: str) -> dict[int, tuple[str, int, dict[str, str]]]:
    """Each debug entry by offset: its tag, its depth and its attributes"""
    entries = {}
    current = None
    for line in dump.splitlines():
        match = ENTRY.match(line)
        if match:
            # llvm-dwarfdump indents an entry by two spaces a level
            depth = len(match.group(2)) // 2
            current = (match.group(3), depth, {})
            entries[int(match.group(1), 16)] = current
            continue
        match = ATTRIBUTE.match(line)
        if match and current is not None:
            current[2][match.group(1)] = match.group(2)
    return entries


def member_offsets(dwarfdump: str, path: str, struct: str) -> dict[str, int] | None:
    """The offsets of the members of the struct the typedef struct names, or None if the object doesn't have it"""
    dump = subprocess.run([dwarfdump, "--debug-info", path], capture_output=True, text=True, check=True).stdout
    entries = read_entries(dump)
    offsets = sorted(entries)
    for offset in offsets:
        tag, _, attributes = entries[offset]
        if tag != "DW_TAG_typedef" or attributes.get("DW_AT_name") != f'"{struct}"':
            continue
        target = int(attributes["DW_AT_type"].split()[0], 16)
        struct_tag, depth, _ = entries[target]
        if struct_tag != "DW_TAG_structure_type":
            continue
        members = {}
        for child in offsets[offsets.index(target) + 1:]:
            child_tag, child_depth, child_attributes = entries[child]
            if child_depth <= depth:
                break
            if child_tag == "DW_TAG_member" and child_depth == depth + 1:
                name = child_attributes.get("DW_AT_name", "").strip('"')
                location = child_attributes.get("DW_AT_data_member_location")
                if location is not None:
                    members[name] = int(location, 0)
        return members
    return None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--objects-dir", required=True, help="the game target's object directory")
    parser.add_argument("--struct", required=True, help="the typedef's name")
    parser.add_argument("--members", required=True, help="the members to compare, comma-separated")
    parser.add_argument("sources", nargs="+", help="the source files whose objects declare it; the header's last")
    args = parser.parse_args(argv)

    dwarfdump = find_dwarfdump()
    if dwarfdump is None:
        print("check_struct_layout.py: skipped: no llvm-dwarfdump")
        return SKIP
    members = args.members.split(",")
    layouts = []
    for source in args.sources:
        path = find_object(args.objects_dir, source)
        if path is None:
            print(f"check_struct_layout.py: no object of {source} in {args.objects_dir}")
            return 1
        offsets = member_offsets(dwarfdump, path, args.struct)
        if offsets is None:
            print(f"check_struct_layout.py: skipped: {source}'s object has no debug information for {args.struct} "
                  "(a build without -g)")
            return SKIP
        missing = [name for name in members if name not in offsets]
        if missing:
            print(f"FAIL: {source}'s {args.struct} has no member {', '.join(missing)}")
            return 1
        layouts.append((source, {name: offsets[name] for name in members}))
    reference_source, reference = layouts[-1]
    failed = False
    for source, layout in layouts:
        described = ", ".join(f"{name} at {offset:#x}" for name, offset in layout.items())
        print(f"{source}: {args.struct}: {described}")
        if layout != reference:
            failed = True
            print(f"FAIL: {source} puts {args.struct}'s members elsewhere than {reference_source}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
