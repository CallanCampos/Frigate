#!/usr/bin/env python3
"""Link the LTD autoload's NitroWiFi objects as an overlay group of their own.

    ltd_objects.py build/b2_us/objects_merged.txt build/b2_us/arm9_merged.lcf build/b2_us/objects_link.txt

mwldarm gives every out-of-range call in a file listed after its -overlaygroup and -overlay options a long-branch
veneer of its own, and shares one veneer per target among the other files (docs/configs.md, "Long-branch veneers").
The original gives each call of NitroWiFi's Thumb code in the LTD autoload (.ltdmain) its own, as for an overlay,
while dsd's objects.txt lists the module's objects with ARM9 main's. This moves the objects that the linker script
places in each LTD autoload to the end, in a group named after the autoload's memory region. TwlSDK's objects stay
with main's: their .ltdmain code shares one veneer per target across files, as main's does (card_hash.c's veneer of
OS_DisableInterrupts serves mi_ndma.c, mi_ndma_gxcommand.c, mi_sharedWram_twl.c and pm.c's DSi part, and
os_interrupt_twl.c calls OS_EnableIrqMask through mi_ndma_gxcommand.c's), which the group would give each call apart. So
does the object of a merged file (config/merged_sources.toml) that holds a TwlSDK part: card_rom.c's TWL code calls
CARD_GetOwnRomHeaderTWL twice through one veneer, and card_hash.c and mi_sharedWram_twl.c call others through its
veneers.
"""
import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from merge_objects import merged_sources  # noqa: E402

# A section of dsd's linker script that an LTD autoload's objects are placed in, and the memory region it goes to
LTD_SECTION = re.compile(r"^\s*\.ltd_autoload_\d+ : \{\n(.*?)^\s*\} > (LTD_AUTOLOAD_\d+)$", re.M | re.S)
PLACED_OBJECT = re.compile(r"^\s*(\S+\.o)\(", re.M)
# The objects that keep main's shared veneers, by their source directory
SHARED_VENEERS = "/lib/twl/"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("objects", type=Path, help="dsd's objects.txt, as merge_objects.py rewrites it")
    parser.add_argument("lcf", type=Path, help="dsd's linker script, as merge_objects.py rewrites it")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    lines = [line for line in args.objects.read_text().splitlines() if line]
    wholes = tuple("/" + Path(whole).with_suffix(".o").as_posix() for whole in merged_sources().values())

    def shares_veneers(line: str) -> bool:
        path = line.strip('"')
        return SHARED_VENEERS in path or path.endswith(wholes)

    groups = []
    for body, region in LTD_SECTION.findall(args.lcf.read_text()):
        names = set(PLACED_OBJECT.findall(body))
        objects = [line for line in lines if Path(line.strip('"')).name in names and not shares_veneers(line)]
        if objects:
            groups.append((region, objects))
    moved = {line for _, objects in groups for line in objects}
    out = [line for line in lines if line not in moved]
    for region, objects in groups:
        out += [f"-overlaygroup {region},0", f"-overlay {region}", *objects]
    args.output.write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
