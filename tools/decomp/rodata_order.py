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

Objects are given in declaration order as NAME:SIZE. A local initializer is marked with a trailing `@`, as in BG1:32@,
and a global that no code refers to with a trailing `!`, as in UNK:4!.

    rodata_order.py VRAM:48 VRAMNamed:48 Pos:12 Up:12 BG1:32@ BG2:32@
    rodata_order.py VRAM:48 VRAMNamed:48 Pos:12 Up:12 BG1:32@ BG2:32@ --want Pos Up VRAM VRAMNamed BG2 BG1 \
        --permute VRAM VRAMNamed Pos Up

With --want, --permute tries every order of the named objects in the slots they take, and prints the orders that give
the wanted layout.
"""
import argparse
import itertools

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


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument(
        "objects",
        nargs="+",
        help="NAME:SIZE in declaration order, with @ for local initializers and ! for globals no code refers to",
    )
    parser.add_argument("--want", nargs="+", help="the wanted layout, by name")
    parser.add_argument("--permute", nargs="+", default=[], help="objects whose declaration order may change")
    args = parser.parse_args()

    objects = [parse(spec) for spec in args.objects]
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
