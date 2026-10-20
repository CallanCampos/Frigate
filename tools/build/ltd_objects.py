#!/usr/bin/env python3
"""Link the LTD autoload's objects as an overlay group of their own.

    ltd_objects.py build/b2_us/objects.txt build/b2_us/arm9.lcf build/b2_us/objects_link.txt

mwldarm gives every out-of-range call in a file listed after its -overlaygroup and -overlay options a long-branch
veneer of its own, and shares one veneer per target among the other files (docs/configs.md, "Long-branch veneers").
The original gives each call of the LTD autoload's Thumb code (.ltdmain) its own, as for an overlay, while dsd's
objects.txt lists the module's objects with ARM9 main's. This moves the objects that the linker script places in each
LTD autoload to the end, in a group named after the autoload's memory region.
"""
import argparse
import re
from pathlib import Path

# A section of dsd's linker script that an LTD autoload's objects are placed in, and the memory region it goes to
LTD_SECTION = re.compile(r"^\s*\.ltd_autoload_\d+ : \{\n(.*?)^\s*\} > (LTD_AUTOLOAD_\d+)$", re.M | re.S)
PLACED_OBJECT = re.compile(r"^\s*(\S+\.o)\(", re.M)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("objects", type=Path, help="dsd's objects.txt")
    parser.add_argument("lcf", type=Path, help="dsd's linker script")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    lines = [line for line in args.objects.read_text().splitlines() if line]
    groups = []
    for body, region in LTD_SECTION.findall(args.lcf.read_text()):
        names = set(PLACED_OBJECT.findall(body))
        objects = [line for line in lines if Path(line.strip('"')).name in names]
        if objects:
            groups.append((region, objects))
    moved = {line for _, objects in groups for line in objects}
    out = [line for line in lines if line not in moved]
    for region, objects in groups:
        out += [f"-overlaygroup {region},0", f"-overlay {region}", *objects]
    args.output.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
