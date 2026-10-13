#!/usr/bin/env python3
"""Predict how MWCC lays out a file's static data, and find declaration orders that give a wanted layout.

MWCC lists each data object of a section when it is declared, a local struct initializer when its function is, then
heapsorts the list by size, starting from the last object declared. Heapsort is not stable, so objects of the same size
come out in an order that depends on where every object in the list was declared. Objects of 64 bytes or more, local
initializers, and global objects that no code refers to get sections of their own; the others share one section. The
sections are created as the sorted list is walked, so the shared section sits where its smallest object comes, and an
initializer smaller than every shared object is laid out before them.

A global counts as referred to when some code takes its address, or reads it before its value is known, even if that
code is optimized away or never emitted. A read of a const global whose initializer has been seen is folded and does
not count.

This models only the size heapsort of aggregate data. String literals are a separate rule (laid out in the order they
first appear in the source, each 4-aligned, shared from the first use), so this tool cannot answer a string-order
question such as overlay 136's.

Objects are given in declaration order as NAME:SIZE. A local initializer is marked with a trailing `@`, as in BG1:32@,
and a global that no code refers to with a trailing `!`, as in UNK:4!.

The list must be complete, or the prediction is silently wrong: every function-local initializer counts, down to an
8-byte struct (btlv_input.c's layout was declared unreachable until BtlvInput_StartTileAnim's ClActorSetup was
listed), and a table referenced only from another table's initializer is referenced (the initializer relocates to
it), so it is a plain entry, never `!`. List `--from-object` prints every data object of a compiled object file
(compiler_probe.py leaves one next to its report) so nothing is forgotten, with the layout the compiler actually
produced; `--check` compares the prediction against that object and fails loudly when the model and the compiler
disagree.

    rodata_order.py VRAM:48 VRAMNamed:48 Pos:12 Up:12 BG1:32@ BG2:32@
    rodata_order.py VRAM:48 VRAMNamed:48 Pos:12 Up:12 BG1:32@ BG2:32@ --want Pos Up VRAM VRAMNamed BG2 BG1 \
        --permute VRAM VRAMNamed Pos Up
    rodata_order.py --from-object build/b2_us/src/ov168/btlv_input.o
    rodata_order.py VRAM:48 ... BG2:32@ --check build/b2_us/src/ov010/wifilogin_disp.o

With --want, --permute tries every order of the named objects in the slots they take, and prints the orders that give
the wanted layout.
"""
import argparse
import itertools
import re
import subprocess
from collections import Counter

SEPARATE_SIZE = 64


def heapsort(items: list, key) -> list:
    a = list(items)

    def sift(root: int, end: int):
        while True:
            child = 2 * root + 1
            if child > end:
                return
            if child + 1 <= end and key(a[child]) < key(a[child + 1]):
                child += 1
            if key(a[root]) < key(a[child]):
                a[root], a[child] = a[child], a[root]
                root = child
            else:
                return

    for start in range(len(a) // 2 - 1, -1, -1):
        sift(start, len(a) - 1)
    for end in range(len(a) - 1, 0, -1):
        a[0], a[end] = a[end], a[0]
        sift(0, end - 1)
    return a


def layout(objects: list[tuple[str, int, bool]]) -> list[str]:
    """Returns the names of the objects in the order they are laid out, section by section."""
    ordered = heapsort(list(reversed(objects)), key=lambda o: o[1])
    sections = []
    shared = None
    for name, size, separate in ordered:
        if separate or size >= SEPARATE_SIZE:
            sections.append([name])
        elif shared is None:
            shared = [name]
            sections.append(shared)
        else:
            shared.append(name)
    return [name for section in sections for name in section]


def parse(spec: str) -> tuple[str, int, bool]:
    separate = spec.endswith(("@", "!"))
    name, size = spec.rstrip("@!").split(":")
    return name, int(size, 0), separate


DATA_SECTIONS = (".rodata", ".data", ".bss")


def read_object(path: str) -> list[tuple[str, int, int, int, str]]:
    """Returns (name, size, section index, offset, section name) for each data object symbol of an object file."""
    sections = {}
    out = subprocess.run(["readelf", "-SW", path], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        m = re.match(r"\s*\[\s*(\d+)\]\s+(\S+)\s", line)
        if m and m.group(2) in DATA_SECTIONS:
            sections[int(m.group(1))] = m.group(2)
    symbols = []
    out = subprocess.run(["readelf", "-sW", path], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) >= 8 and parts[3] == "OBJECT" and parts[6].isdigit() and int(parts[6]) in sections:
            index = int(parts[6])
            symbols.append((parts[7], int(parts[2], 0), index, int(parts[1], 16), sections[index]))
    return symbols


def object_layout(symbols: list) -> dict[str, list]:
    """Splits an object's data symbols into each kind's shared section, in layout order, and the separate ones."""
    counts = Counter(s[2] for s in symbols)
    layouts = {}
    for kind in DATA_SECTIONS:
        shared = sorted((s for s in symbols if s[4] == kind and counts[s[2]] > 1), key=lambda s: (s[2], s[3]))
        if shared:
            layouts[kind] = shared
    layouts["separate"] = sorted((s for s in symbols if counts[s[2]] == 1), key=lambda s: s[2])
    return layouts


def from_object(path: str):
    symbols = read_object(path)
    layouts = object_layout(symbols)
    print("objects (declaration order is yours to supply; @NNNN names are function-local initializers):")
    for name, size, _, _, kind in sorted(symbols, key=lambda s: (s[4], s[1], s[0])):
        marker = "@" if name.startswith("@") else ""
        print(f"  {name}:{size}{marker}  ({kind})")
    for kind, entries in layouts.items():
        if kind == "separate":
            continue
        print(f"{kind} shared section, as compiled:", " ".join(s[0] for s in entries))
    if layouts["separate"]:
        print("own sections, in order:", " ".join(f"{s[0]}:{s[1]}" for s in layouts["separate"]))


def check(objects: list[tuple[str, int, bool]], markers: dict[str, str], path: str) -> bool:
    """Compares the predicted layout with a compiled object's; returns True when they agree."""
    symbols = read_object(path)
    layouts = object_layout(symbols)
    predicted = layout(objects)
    ok = True
    for kind, entries in layouts.items():
        if kind == "separate":
            continue
        actual = [s[0] for s in entries]
        names = set(actual)
        want = [name for name in predicted if name in names]
        if want != actual:
            ok = False
        print(f"{kind} shared section:", "ok" if want == actual else "MISMATCH")
        if want != actual:
            print("  predicted:", " ".join(want))
            print("  compiled: ", " ".join(actual))
    spec_local = Counter(size for name, size, _ in objects if markers.get(name) == "@")
    object_local = Counter(size for name, size, _, _, _ in symbols if name.startswith("@"))
    if spec_local != object_local:
        ok = False
        print("MISMATCH: local initializers by size: spec has", dict(spec_local), "object has", dict(object_local))
    named = {name for name, _, _ in objects}
    missing = [f"{s[0]}:{s[1]}" for s in symbols if s[0] not in named and not s[0].startswith("@")]
    if missing:
        ok = False
        print("MISMATCH: not in the spec:", " ".join(missing))
    return ok


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "objects",
        nargs="*",
        help="NAME:SIZE in declaration order, with @ for local initializers and ! for globals no code refers to",
    )
    parser.add_argument("--want", nargs="+", help="the wanted layout, by name")
    parser.add_argument("--permute", nargs="+", default=[], help="objects whose declaration order may change")
    parser.add_argument("--from-object", metavar="FILE.o", help="list a compiled object's data objects and layout")
    parser.add_argument("--check", metavar="FILE.o", help="compare the prediction against a compiled object")
    args = parser.parse_args()

    if args.from_object:
        from_object(args.from_object)
        return
    if not args.objects:
        parser.error("objects are required unless --from-object is given")

    objects = [parse(spec) for spec in args.objects]
    if args.check:
        markers = {parse(spec)[0]: spec[-1] if spec.endswith(("@", "!")) else "" for spec in args.objects}
        raise SystemExit(0 if check(objects, markers, args.check) else 1)
    print(" ".join(layout(objects)))
    if not args.want:
        return
    slots = [i for i, o in enumerate(objects) if o[0] in args.permute]
    movable = [objects[i] for i in slots]
    found = 0
    for order in itertools.permutations(movable):
        trial = list(objects)
        for slot, obj in zip(slots, order):
            trial[slot] = obj
        if layout(trial) == args.want:
            found += 1
            print("match:", " ".join(o[0] for o in trial))
    print(f"{found} matching orders")


if __name__ == "__main__":
    main()
