#!/usr/bin/env python3
"""Fix relocations and symbols that dsd's analysis gets wrong, in every version, and keep the fixes across config
regeneration.

Each fix is recorded in config/fixes.txt by module and address in the primary version. The address in the other
versions is found through the function containing it, which must be paired in the version map, or else by its offset
from the start of its section, which must have the same size in every version. regenerate_configs.py runs `apply`
after importing names from swan.

    config_fixes.py reloc-module overlays/ov035 'overlay(12)' 0x0217c9e8    # an ambiguous destination module
    config_fixes.py overlay-id overlays/ov035 279 0x0217cbe4                 # a literal that is an overlay ID
    config_fixes.py remove-reloc overlays/ov035 0x0217f540                   # a relocation that is not one
    config_fixes.py reloc-addend overlays/ov036 2 0x021841e8                 # a relocation to inside an object
    config_fixes.py remove-symbol overlays/ov005 0x0214f5fd                  # a symbol that is not one
    config_fixes.py add-label . _ll_mul 0x0208d60c                           # a second name for a function
    config_fixes.py add-data overlays/ov307 EGG_DEMO_VIEW_UNK_FX32 0x021df70c  # an object that nothing references
    config_fixes.py add-function . ampOffFreeBlocks thumb 0x20 0x02006dec     # a function that dsd took for a label
    config_fixes.py add-reloc overlays/ov036 load 'overlay(259)' 0x021a039c 0x021ae188  # a pointer dsd missed
    config_fixes.py section-end overlays/ov310 src/ov310/research_list.c .rodata 0x021a7028  # move a file's boundary
    config_fixes.py section-start overlays/ov310 src/ov310/research_common.c .rodata 0x021a773c
    config_fixes.py add-section . lib/nnsys/src/g2d/g2di_Mtx32.c .bss 0x408 0x021435d8  # give a file a section
    config_fixes.py apply
"""
import argparse
import csv
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT, config_lock, load_modules, parse_sections, reloc_module_names  # noqa: E402

PRIMARY = "b2_us"
OTHERS = ["w2_us"]
FIXES = ROOT / "config" / "fixes.txt"
MAP = ROOT / "build" / "version_map.tsv"
HEADER = (
    "# Fixes to dsd's analysis, applied by tools/decomp/config_fixes.py\n"
    "# module address(b2_us) action [argument]\n"
)
RELOC_RE = re.compile(r"^from:(0x[0-9a-f]+) ")
SYMBOL_ADDR_RE = re.compile(r" addr:(0x[0-9a-f]+)")


class AddressMapper:
    """Maps an address in the primary version to the other versions."""

    def __init__(self):
        self.pairs: dict[str, dict[tuple[str, int], int]] = {other: {} for other in OTHERS}
        with MAP.open() as f:
            for row in csv.DictReader(f, delimiter="\t"):
                for other in OTHERS:
                    key = (row["module"], int(row[f"{PRIMARY}_addr"], 16))
                    self.pairs[other][key] = int(row[f"{other}_addr"], 16)
        self.functions = {name: module.functions() for name, module in load_modules(PRIMARY).items()}

    def map(self, module: str, addr: int, other: str) -> int:
        pairs = self.pairs[other]
        function = next((f for f in self.functions[module] if f.addr <= addr < f.addr + (f.size or 0)), None)
        if function is not None and (module, function.addr) in pairs:
            return pairs[(module, function.addr)] + addr - function.addr
        # Between functions, such as in padding, or in a function the version map doesn't pair, such as one a fix added:
        # through the nearest paired functions on either side, if they are the same distance apart in both versions
        paired = [f.addr for f in self.functions[module] if (module, f.addr) in pairs]
        before = max((a for a in paired if a <= addr), default=None)
        after = min((a for a in paired if a > addr), default=None)
        if before is not None and after is not None:
            if pairs[(module, after)] - pairs[(module, before)] == after - before:
                return pairs[(module, before)] + addr - before
        if function is not None:
            sys.exit(f"{module} {addr:#010x}: function {function.name} is not paired with {other}")
        primary_sections = parse_sections(config_dir(PRIMARY, module) / "delinks.txt")
        other_sections = parse_sections(config_dir(other, module) / "delinks.txt")
        for name, (start, end) in primary_sections.items():
            if start <= addr < end:
                other_start, other_end = other_sections[name]
                if end - start != other_end - other_start:
                    sys.exit(f"{module} {addr:#010x}: {name} has different sizes in {PRIMARY} and {other}")
                return other_start + addr - start
        sys.exit(f"{module} {addr:#010x} is in no section")


def config_dir(version: str, module: str) -> Path:
    return ROOT / "config" / version / "arm9" / module


def load_fixes() -> list[tuple[str, int, str, str]]:
    fixes = []
    if FIXES.exists():
        for line in FIXES.read_text().splitlines():
            if line.strip() and not line.startswith("#"):
                module, addr, action, *argument = line.split()
                fixes.append((module, int(addr, 16), action, argument[0] if argument else ""))
    return fixes


def save_fixes(fixes: list[tuple[str, int, str, str]]):
    def key(fix):
        module, addr, _, _ = fix
        # Main first, then the modules in numeric order
        number = re.search(r"(\d+)$", module)
        return (module != ".", int(number.group(1)) if number else 0, module, addr)

    lines = [f"{m} {a:#010x} {act} {arg}".rstrip() for m, a, act, arg in sorted(set(fixes), key=key)]
    FIXES.write_text(HEADER + "\n".join(lines) + "\n")


def apply_fix(version: str, module: str, addr: int, action: str, argument: str):
    """Applies a fix to one version's config. Applying a fix again changes nothing."""
    if action == "remove_symbol":
        path = config_dir(version, module) / "symbols.txt"
        lines = path.read_text().splitlines()
        kept = [line for line in lines if not (m := SYMBOL_ADDR_RE.search(line)) or int(m.group(1), 16) != addr]
        path.write_text("\n".join(kept) + "\n")
        return
    if action == "add_data":
        # A data object that dsd found no reference to, such as a global constant that the compiler folds into code but
        # still emits. Without a symbol, the object before it seems to run on over it
        path = config_dir(version, module) / "symbols.txt"
        lines = path.read_text().splitlines()
        # An object in .bss has no contents, so dsd gives it the bss kind
        sections = parse_sections(config_dir(version, module) / "delinks.txt")
        in_bss = any(start <= addr < end for name, (start, end) in sections.items() if name.endswith("bss"))
        kind = "kind:bss" if in_bss else "kind:data"
        # Already there under any name: rename_symbol.py may have renamed it since the fix was recorded
        if any(kind in line and (m := SYMBOL_ADDR_RE.search(line)) and int(m.group(1), 16) == addr for line in lines):
            return
        data = [i for i, line in enumerate(lines) if kind in line and (m := SYMBOL_ADDR_RE.search(line))
                and int(m.group(1), 16) < addr]
        symbol = f"{argument} kind:bss addr:{addr:#010x}" if in_bss else f"{argument} kind:data(any) addr:{addr:#010x}"
        lines.insert(data[-1] + 1 if data else len(lines), symbol)
        path.write_text("\n".join(lines) + "\n")
        return
    if action == "add_function":
        # A function that dsd's analysis missed, such as one after a literal pool that it took for code. Replaces the
        # symbols at the address
        name, mode, size = argument.split(":")
        path = config_dir(version, module) / "symbols.txt"
        lines = path.read_text().splitlines()
        line = f"{name} kind:function({mode},size={size}) addr:{addr:#010x}"
        # Already there under any name: rename_symbol.py may have renamed it since the fix was recorded. Under the name
        # dsd generates, dsd found it itself, so it takes the fix's name
        existing = next((i for i, l in enumerate(lines)
                         if l.endswith(f" kind:function({mode},size={size}) addr:{addr:#010x}")), None)
        if existing is not None:
            if re.fullmatch(r"func_(?:ov\d{3}_)?[0-9a-f]{8}", lines[existing].split()[0]):
                lines[existing] = line
                path.write_text("\n".join(lines) + "\n")
            return
        lines = [l for l in lines if not (m := SYMBOL_ADDR_RE.search(l)) or int(m.group(1), 16) != addr]
        before = [i for i, l in enumerate(lines) if "kind:function" in l and (m := SYMBOL_ADDR_RE.search(l))
                  and int(m.group(1), 16) < addr]
        lines.insert(before[-1] + 1 if before else 0, line)
        path.write_text("\n".join(lines) + "\n")
        return
    if action == "add_label":
        # A label with the instruction mode of the function at the address, for code that the compiler calls by
        # two names, such as the runtime's _ll_mul and _ull_mul, or of the label dsd found there, for a second entry
        # point inside a function, such as _ll_sdiv inside _ll_mod
        path = config_dir(version, module) / "symbols.txt"
        lines = path.read_text().splitlines()
        at = [i for i, line in enumerate(lines) if (m := SYMBOL_ADDR_RE.search(line)) and int(m.group(1), 16) == addr]
        if any(lines[i].split()[0] == argument for i in at):
            # Added before local labels were replaced: drop the local label left beside it
            kept = [line for i, line in enumerate(lines)
                    if not (i in at and line.startswith(".L_") and "kind:label" in line)]
            if len(kept) != len(lines):
                path.write_text("\n".join(kept) + "\n")
            return
        mode = next((m.group(1) for i in at
                     if (m := re.search(r"kind:(?:function|label)\((arm|thumb)", lines[i]))), None)
        if mode is None:
            sys.exit(f"{version}: no function or label at {module} {addr:#010x}")
        # A local label dsd made there takes the name: dsd dis refuses a .L_ label that shares its address
        local = next((i for i in at if lines[i].startswith(".L_") and "kind:label" in lines[i]), None)
        if local is not None:
            lines[local] = f"{argument} kind:label({mode}) addr:{addr:#010x}"
        else:
            lines.insert(at[-1] + 1, f"{argument} kind:label({mode}) addr:{addr:#010x}")
        path.write_text("\n".join(lines) + "\n")
        return

    if action in ("section_start", "section_end"):
        # A registered source file's section range, moved after add_source_file.py, such as when an object at a boundary
        # turns out to belong to the neighbouring file. The argument is source:section
        path = config_dir(version, module) / "delinks.txt"
        lines = path.read_text().splitlines()
        source, section = argument.rsplit(":", 1)
        try:
            start = lines.index(f"{source}:")
        except ValueError:
            sys.exit(f"{version} {module}: {source} is not registered")
        bound = "start" if action == "section_start" else "end"
        for i in range(start + 1, len(lines)):
            if not lines[i].startswith(" "):
                break
            if lines[i].split()[0] == section:
                lines[i] = re.sub(rf"{bound}:0x[0-9a-f]+", f"{bound}:{addr:#010x}", lines[i])
                break
        else:
            sys.exit(f"{version} {module}: {source} has no {section}")
        path.write_text("\n".join(lines) + "\n")
        return

    if action == "add_section":
        # A section that a registered source file turns out to have, such as an object it defines that its neighbour
        # reaches through a symbol. The argument is source:section:size, and the address is the section's start
        path = config_dir(version, module) / "delinks.txt"
        lines = path.read_text().splitlines()
        source, section, size = argument.rsplit(":", 2)
        try:
            start = lines.index(f"{source}:")
        except ValueError:
            sys.exit(f"{version} {module}: {source} is not registered")
        order = [".text", ".init", ".rodata", ".ctor", ".data", ".bss"]
        at = start + 1
        while at < len(lines) and lines[at].startswith(" "):
            name = lines[at].split()[0]
            if name == section:
                return
            if name in order and section in order and order.index(name) > order.index(section):
                break
            at += 1
        end = addr + int(size, 0)
        lines.insert(at, f"    {section:<11} start:{addr:#010x} end:{end:#010x}")
        path.write_text("\n".join(lines) + "\n")
        return

    path = config_dir(version, module) / "relocs.txt"
    lines = path.read_text().splitlines()
    index = next((i for i, line in enumerate(lines) if int(RELOC_RE.match(line).group(1), 16) == addr), None)
    if action == "reloc_module":
        if index is None:
            sys.exit(f"{version}: no relocation at {module} {addr:#010x}")
        lines[index] = re.sub(r"module:\S+", f"module:{argument}", lines[index])
    elif action == "overlay_id":
        line = f"from:{addr:#010x} kind:overlay_id to:{argument}"
        if index is not None:
            if lines[index] != line:
                sys.exit(f"{version}: {module} {addr:#010x} already has another relocation: {lines[index]}")
        else:
            position = next((i for i, l in enumerate(lines) if int(RELOC_RE.match(l).group(1), 16) > addr), len(lines))
            lines.insert(position, line)
    elif action == "add_reloc":
        # A pointer that dsd left as a plain word, such as one into an overlay that is not loaded with this module, or a
        # code address with no symbol. Unrelocated, it keeps its address when the code before its target grows
        kind, destination, target, *addend = argument.split(",")
        line = f"from:{addr:#010x} kind:{kind} to:{int(target, 16):#010x}"
        line += f" add:{int(addend[0], 16):#x}" if addend else ""
        line += f" module:{destination}"
        if index is not None:
            if lines[index] != line:
                sys.exit(f"{version}: {module} {addr:#010x} already has another relocation: {lines[index]}")
        else:
            position = next((i for i, l in enumerate(lines) if int(RELOC_RE.match(l).group(1), 16) > addr), len(lines))
            lines.insert(position, line)
    elif action == "remove_reloc":
        if index is not None:
            del lines[index]
    elif action == "reloc_addend":
        # A relocation to an address inside an object, such as a field of a struct in a table, which dsd gave a symbol
        # of its own: point it at the object, with the offset as the addend
        if index is None:
            sys.exit(f"{version}: no relocation at {module} {addr:#010x}")
        addend = int(argument, 0)
        if f" add:{addend:#x} " in lines[index]:
            return
        to = int(re.search(r"to:(0x[0-9a-f]+)", lines[index]).group(1), 16)
        lines[index] = re.sub(r"to:0x[0-9a-f]+", f"to:{to - addend:#010x} add:{addend:#x}", lines[index])
    else:
        sys.exit(f"unknown action {action!r}")
    path.write_text("\n".join(lines) + "\n")


def apply_everywhere(mapper: AddressMapper, module: str, addr: int, action: str, argument: str):
    apply_fix(PRIMARY, module, addr, action, argument)
    for other in OTHERS:
        other_argument = argument
        if action == "add_reloc":
            # The target moves between versions too: map it through its own module
            kind, destination, target, *addend = argument.split(",")
            target = int(target, 16)
            mapped = mapper.map(reloc_module_names(destination)[0], target & ~1, other) | (target & 1)
            other_argument = ",".join([kind, destination, f"{mapped:#010x}", *addend])
        apply_fix(other, module, mapper.map(module, addr, other), action, other_argument)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("reloc-module", help="set the destination module of relocations")
    command.add_argument("module", help="module path in the config, e.g. overlays/ov035")
    command.add_argument("destination", help="destination module, e.g. overlay(12)")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("overlay-id", help="mark literals as overlay IDs")
    command.add_argument("module")
    command.add_argument("overlay", type=int)
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("remove-reloc", help="remove relocations")
    command.add_argument("module")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("reloc-addend", help="point relocations at an object with an addend")
    command.add_argument("module")
    command.add_argument("addend")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("remove-symbol", help="remove symbols")
    command.add_argument("module")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("add-label", help="give a function a second name")
    command.add_argument("module")
    command.add_argument("name")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("add-data", help="add a data object that nothing references")
    command.add_argument("module")
    command.add_argument("name")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("add-function", help="add a function that dsd missed")
    command.add_argument("module")
    command.add_argument("name")
    command.add_argument("mode", choices=["arm", "thumb"])
    command.add_argument("size")
    command.add_argument("addresses", nargs="+")
    command = commands.add_parser("add-reloc", help="add a relocation that dsd missed")
    command.add_argument("module")
    command.add_argument("kind", help="relocation kind, e.g. load")
    command.add_argument("destination", help="destination module, e.g. main or overlay(259)")
    command.add_argument("target", help="target address, odd for a Thumb function")
    command.add_argument("--addend", help="offset from the target, for an address inside a function or object")
    command.add_argument("addresses", nargs="+")
    for bound in ("start", "end"):
        command = commands.add_parser(f"section-{bound}", help=f"move the {bound} of a registered file's section")
        command.add_argument("module")
        command.add_argument("source", help="the source file as delinks.txt names it, e.g. src/ov310/research_list.c")
        command.add_argument("section", help="e.g. .rodata")
        command.add_argument("addresses", nargs="+", help=f"the new {bound}")
    command = commands.add_parser("add-section", help="give a registered file a section it lacks")
    command.add_argument("module")
    command.add_argument("source", help="the source file as delinks.txt names it")
    command.add_argument("section", help="e.g. .bss")
    command.add_argument("size", help="the section's size, e.g. 0x408")
    command.add_argument("addresses", nargs="+", help="the section's start")
    commands.add_parser("apply", help="apply every fix in config/fixes.txt")
    args = parser.parse_args()

    mapper = AddressMapper()
    fixes = load_fixes()
    if args.command == "apply":
        for fix in fixes:
            apply_everywhere(mapper, *fix)
        print(f"applied {len(fixes)} fixes")
        return

    action = args.command.replace("-", "_")
    argument = {"reloc_module": getattr(args, "destination", ""), "overlay_id": str(getattr(args, "overlay", "")),
                "reloc_addend": getattr(args, "addend", ""), "add_label": getattr(args, "name", ""), "add_data": getattr(args, "name", ""),
                "add_function": f"{getattr(args, 'name', '')}:{getattr(args, 'mode', '')}:{getattr(args, 'size', '')}",
                "add_reloc": ",".join([getattr(args, "kind", ""), getattr(args, "destination", ""),
                                       getattr(args, "target", ""), *([args.addend] if getattr(args, "addend", None)
                                                                      else [])]),
                "section_start": f"{getattr(args, 'source', '')}:{getattr(args, 'section', '')}",
                "section_end": f"{getattr(args, 'source', '')}:{getattr(args, 'section', '')}",
                "add_section": f"{getattr(args, 'source', '')}:{getattr(args, 'section', '')}:{getattr(args, 'size', '')}"}
    argument = argument.get(action, "")
    for address in args.addresses:
        addr = int(address, 16)
        apply_everywhere(mapper, args.module, addr, action, argument)
        # One fix of each kind per address: a word can need its symbol removed and a relocation added
        fixes = [f for f in fixes if (f[0], f[1], f[2]) != (args.module, addr, action)] + [
            (args.module, addr, action, argument)]
        print(f"{action} {args.module} {addr:#010x} {argument}".rstrip())
    save_fixes(fixes)


if __name__ == "__main__":
    with config_lock():
        main()
