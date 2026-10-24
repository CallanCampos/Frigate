#!/usr/bin/env python3
"""Drop the code that the original link dead-stripped from one compiled object.

    deadstrip.py --source src/system/wipe_sub.c build/b2_us/unstripped/src/system/wipe_sub.o \\
        build/b2_us/src/system/wipe_sub.o

The game was linked with dead-stripping, so a function nothing called is missing from the ROM while the data it
pooled with live functions' data stays. Our link keeps everything (`-nodead`, since dead-stripping would drop the
delinked objects nothing refers to), and `mwldarm -partial` can't strip, so the few source files whose original
lost code that way list the dead symbols in config/deadstrip.toml, and the build runs their objects through this
first. Each entry there is a flagged exception: every other file is built and linked as it is.

MWCC emits each function, and each data object it doesn't pool, in a section of its own. The strip drops each listed
symbol's section and every allocated section that only dropped sections refer to (a static function, a string or a
function-local static that only dead code uses), as the linker would. A section that defines a global symbol the list
doesn't name is kept, as is everything the kept sections refer to, so data pooled with live code stays. An exception
index (.exceptix) goes with the code it describes. Relocations into dropped sections are removed from the sections
that stay (the debug information), the relocation sections of dropped sections go with them, and so do the symbols
they defined and the undefined symbols that only dropped code referred to. The rest of the object, the string tables
included, is kept as it is, in its order. With nothing listed for the source, the object is copied unchanged.

It stops with an error, leaving no output, when a listed symbol isn't defined in the object, shares its section with
a global that isn't listed, or is reached from a section of the object that stays. It sees only that object: a call
from another file to a listed symbol, or to a function that only the dropped code called and nothing defines, would
link through a veneer to address 0 rather than fail, so check_references looks for those references in every other
object after the strip, in the build (tools/build/check_deadstrip.py) and in tools/decomp/compile_all.py.
"""
import argparse
import re
import struct
import sys
import tomllib
from collections.abc import Iterable
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "config" / "deadstrip.toml"

ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
SECTION_HEADER = struct.Struct("<10I")
SYMBOL = struct.Struct("<IIIBBH")
REL = struct.Struct("<II")
RELA = struct.Struct("<IIi")
ET_REL = 1
SHT_NULL, SHT_SYMTAB, SHT_RELA, SHT_NOBITS, SHT_REL, SHT_GROUP = 0, 2, 4, 8, 9, 17
SHF_ALLOC = 0x2
STB_LOCAL = 0
STT_SECTION, STT_FILE = 3, 4
SHN_UNDEF, SHN_LORESERVE, SHN_XINDEX = 0, 0xFF00, 0xFFFF
# MWCC starts the contents of each section at a multiple of 16 bytes in the file, and the section headers after them
FILE_ALIGN = 16
# ARM mapping symbols, which mark code and data inside a section rather than name anything
MAPPING_SYMBOL = re.compile(r"^\$[atd](\..*)?$")
IDENTIFIER = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")


class DeadStripError(ValueError):
    """An object or manifest the strip can't apply to, with the reason."""


@dataclass
class Section:
    name: str
    name_offset: int
    type: int
    flags: int
    addr: int
    size: int
    link: int
    info: int
    align: int
    entsize: int
    body: bytes


@dataclass
class Symbol:
    name: str
    name_offset: int
    value: int
    size: int
    info: int
    other: int
    shndx: int

    @property
    def bind(self) -> int:
        return self.info >> 4

    @property
    def kind(self) -> int:
        return self.info & 0xF


@dataclass
class Relocation:
    offset: int
    symbol: int
    type: int
    addend: int | None


@dataclass
class Stripped:
    """A stripped object, the names of the symbols defined in what it dropped (the listed ones and what only they
    used), the names of the undefined symbols only the dropped code referred to, and a line for each dropped allocated
    section and undefined symbol."""
    data: bytes
    symbols: set[str]
    undefined: set[str]
    report: list[str]


@dataclass
class ElfObject:
    header: bytes
    sections: list[Section]
    shstrndx: int
    symtab: int
    symbols: list[Symbol]
    # Relocations by the index of their relocation section
    relocations: dict[int, list[Relocation]] = field(default_factory=dict)


def c_string(table: bytes, offset: int) -> str:
    end = table.find(b"\0", offset)
    if offset >= len(table) or end < 0:
        raise DeadStripError(f"string table offset {offset:#x} out of range")
    return table[offset:end].decode("latin-1")


def read_sections(data: bytes) -> tuple[bytes, list[Section], int]:
    """Reads a 32-bit little-endian relocatable ELF object's header, its sections and the index of their names."""
    if len(data) < ELF_HEADER.size or data[:4] != b"\x7fELF":
        raise DeadStripError("not an ELF file")
    (ident, e_type, _, _, _, _, e_shoff, _, e_ehsize, _, e_phnum, e_shentsize, e_shnum,
     e_shstrndx) = ELF_HEADER.unpack_from(data)
    if ident[4] != 1 or ident[5] != 1 or e_type != ET_REL or e_ehsize != ELF_HEADER.size or e_phnum:
        raise DeadStripError("not a 32-bit little-endian relocatable object")
    if e_shentsize != SECTION_HEADER.size or e_shnum == 0 or e_shstrndx >= e_shnum:
        raise DeadStripError("unsupported section header table (extended numbering or wrong entry size)")
    if e_shoff + e_shnum * SECTION_HEADER.size > len(data):
        raise DeadStripError("section header table out of range")
    raw = []
    for i in range(e_shnum):
        name, kind, flags, addr, offset, size, link, info, align, entsize = SECTION_HEADER.unpack_from(
            data, e_shoff + i * SECTION_HEADER.size)
        if kind not in (SHT_NULL, SHT_NOBITS) and offset + size > len(data):
            raise DeadStripError(f"section {i} out of range")
        body = b"" if kind in (SHT_NULL, SHT_NOBITS) else bytes(data[offset:offset + size])
        raw.append((name, kind, flags, addr, size, link, info, align, entsize, body))
    shstrtab = raw[e_shstrndx][9]
    sections = [Section(c_string(shstrtab, name), name, kind, flags, addr, size, link, info, align, entsize, body)
                for name, kind, flags, addr, size, link, info, align, entsize, body in raw]
    return bytes(data[:ELF_HEADER.size]), sections, e_shstrndx


def read_symbols(sections: list[Section]) -> tuple[int, list[Symbol]]:
    """Returns the index of an object's symbol table and its symbols."""
    symtabs = [i for i, s in enumerate(sections) if s.type == SHT_SYMTAB]
    if len(symtabs) != 1:
        raise DeadStripError("expected exactly one symbol table")
    symtab = symtabs[0]
    if not 0 < sections[symtab].link < len(sections):
        raise DeadStripError("symbol table without a string table")
    strtab = sections[sections[symtab].link].body
    body = sections[symtab].body
    if len(body) % SYMBOL.size:
        raise DeadStripError("symbol table size is not a multiple of its entry size")
    symbols = []
    for offset in range(0, len(body), SYMBOL.size):
        name, value, size, info, other, shndx = SYMBOL.unpack_from(body, offset)
        if shndx == SHN_XINDEX or (SHN_UNDEF < shndx < SHN_LORESERVE and shndx >= len(sections)):
            raise DeadStripError(f"symbol {len(symbols)} has an unsupported section index {shndx:#x}")
        symbols.append(Symbol(c_string(strtab, name), name, value, size, info, other, shndx))
    return symtab, symbols


def parse(data: bytes) -> ElfObject:
    """Reads a 32-bit little-endian relocatable ELF object, as MWCC writes them."""
    header, sections, shstrndx = read_sections(data)
    if any(s.type == SHT_GROUP for s in sections):
        raise DeadStripError("section groups are not supported")
    symtab, symbols = read_symbols(sections)
    obj = ElfObject(header, sections, shstrndx, symtab, symbols)
    for i, section in enumerate(sections):
        if section.type not in (SHT_REL, SHT_RELA):
            continue
        entry = REL if section.type == SHT_REL else RELA
        if section.link != symtab or not 0 < section.info < len(sections) or len(section.body) % entry.size:
            raise DeadStripError(f"relocation section {section.name} ({i}) is malformed")
        relocations = []
        for offset in range(0, len(section.body), entry.size):
            fields = entry.unpack_from(section.body, offset)
            if fields[1] >> 8 >= len(symbols):
                raise DeadStripError(f"relocation in {section.name} ({i}) names symbol {fields[1] >> 8}, out of range")
            relocations.append(Relocation(fields[0], fields[1] >> 8, fields[1] & 0xFF,
                                          fields[2] if section.type == SHT_RELA else None))
        obj.relocations[i] = relocations
    return obj


def serialize(obj: ElfObject) -> bytes:
    """Writes an object laid out as MWCC lays them out: the header, each section's contents at the next multiple of
    16 bytes in section order, then the section headers."""
    out = bytearray(obj.header)
    offsets = []
    for section in obj.sections:
        if section.type == SHT_NULL:
            offsets.append(0)
            continue
        out += bytes(-len(out) % FILE_ALIGN)
        offsets.append(len(out))
        if section.type != SHT_NOBITS:
            out += section.body
    out += bytes(-len(out) % FILE_ALIGN)
    shoff = len(out)
    for section, offset in zip(obj.sections, offsets):
        size = section.size if section.type in (SHT_NULL, SHT_NOBITS) else len(section.body)
        out += SECTION_HEADER.pack(section.name_offset, section.type, section.flags, section.addr, offset, size,
                                   section.link, section.info, section.align, section.entsize)
    struct.pack_into("<I", out, 0x20, shoff)
    struct.pack_into("<HH", out, 0x30, len(obj.sections), obj.shstrndx)
    return bytes(out)


def named_symbols(obj: ElfObject, index: int) -> list[Symbol]:
    """Returns the symbols that name something in a section: not its section symbol or ARM mapping symbols."""
    return [s for s in obj.symbols if s.shndx == index and s.kind not in (STT_SECTION, STT_FILE)
            and s.name and not MAPPING_SYMBOL.match(s.name)]


def describe(obj: ElfObject, index: int) -> str:
    """Names a section by its type and the symbols it defines, for messages."""
    defined = named_symbols(obj, index)
    names = [s.name for s in defined if s.bind != STB_LOCAL] + [s.name for s in defined if s.bind == STB_LOCAL]
    return f"{obj.sections[index].name} ({', '.join(names) if names else f'section {index}'})"


def references(obj: ElfObject) -> dict[int, set[int]]:
    """Returns, for each section, the allocated sections that its relocations' symbols are defined in."""
    refs: dict[int, set[int]] = {i: set() for i in range(len(obj.sections))}
    for index, relocations in obj.relocations.items():
        source = obj.sections[index].info
        for relocation in relocations:
            target = obj.symbols[relocation.symbol].shndx
            if SHN_UNDEF < target < SHN_LORESERVE and obj.sections[target].flags & SHF_ALLOC:
                refs[source].add(target)
    return refs


def reach(starts: set[int], refs: dict[int, set[int]]) -> dict[int, int | None]:
    """Returns every section reachable from the starting ones, each with the section it was first reached from."""
    parents: dict[int, int | None] = {i: None for i in starts}
    pending = sorted(starts)
    while pending:
        index = pending.pop()
        for target in sorted(refs[index]):
            if target not in parents:
                parents[target] = index
                pending.append(target)
    return parents


def strip(data: bytes, dead: list[str]) -> Stripped:
    """Returns the object without the listed symbols' code and what only it used."""
    if not dead:
        return Stripped(bytes(data), set(), set(), [])
    obj = parse(data)
    sections, symbols = obj.sections, obj.symbols
    alloc = {i for i, s in enumerate(sections) if s.flags & SHF_ALLOC}
    exception_indices = {i for i in alloc if sections[i].name == ".exceptix"}
    listed = set(dead)

    dead_sections = set()
    for name in dead:
        defined = [s for s in symbols if s.name == name and s.kind not in (STT_SECTION, STT_FILE)
                   and SHN_UNDEF < s.shndx < SHN_LORESERVE]
        if not defined:
            raise DeadStripError(f"{name} is not defined in this object")
        if len(defined) > 1:
            raise DeadStripError(f"{name} is defined more than once in this object")
        if defined[0].shndx not in alloc:
            raise DeadStripError(f"{name} is not in an allocated section")
        dead_sections.add(defined[0].shndx)
    for symbol in symbols:
        if symbol.shndx in dead_sections and symbol.bind != STB_LOCAL and symbol.name not in listed:
            others = sorted(s.name for s in symbols if s.shndx == symbol.shndx and s.name in listed)
            raise DeadStripError(f"{', '.join(others)} shares its section with {symbol.name}, which isn't listed; "
                                 "a section is dropped whole")

    refs = references(obj)
    # Everything the dead code reaches may go. What it doesn't reach stays as it is, and so does a reached section
    # that defines a global the list doesn't name, which another file may use
    doomed = set(reach(dead_sections, refs))
    roots = {i for i in alloc - exception_indices
             if i not in doomed
             or (i not in dead_sections and any(s.shndx == i and s.bind != STB_LOCAL for s in symbols))}
    parents = reach(roots, refs)
    kept = set(parents)
    reached_dead = sorted(kept & dead_sections)
    if reached_dead:
        index = reached_dead[0]
        chain = [index]
        while parents[chain[-1]] is not None:
            chain.append(parents[chain[-1]])
        raise DeadStripError("a section that stays refers to a listed symbol: "
                             + " -> ".join(describe(obj, i) for i in reversed(chain)))
    # An exception index goes with the code it describes
    for index in sorted(exception_indices):
        targets = refs[index]
        if targets <= kept:
            kept.add(index)
        elif targets & kept:
            raise DeadStripError(f"{describe(obj, index)} describes both kept and dropped code")
    dropped = alloc - kept
    report = [f"dropped {describe(obj, i)}" for i in sorted(dropped)]
    gone = {s.name for i in dropped for s in named_symbols(obj, i)}

    removed = dropped | {i for i in obj.relocations if sections[i].info in dropped}
    section_map: dict[int, int] = {}
    for i in range(len(sections)):
        if i not in removed:
            section_map[i] = len(section_map)

    def survives(symbol: Symbol) -> bool:
        return symbol.shndx not in removed

    surviving = {index: [r for r in relocations if survives(symbols[r.symbol])]
                 for index, relocations in obj.relocations.items() if index not in removed}
    referenced = {r.symbol for relocations in obj.relocations.values() for r in relocations}
    still_referenced = {r.symbol for relocations in surviving.values() for r in relocations}
    symbol_map: dict[int, int] = {}
    new_symbols = []
    undefined = set()
    for n, symbol in enumerate(symbols):
        if not survives(symbol):
            continue
        if n and symbol.shndx == SHN_UNDEF and n in referenced and n not in still_referenced:
            report.append(f"dropped undefined {symbol.name}")
            undefined.add(symbol.name)
            continue
        symbol_map[n] = len(new_symbols)
        new_symbols.append(symbol)
    first_global = next((k for k, s in enumerate(new_symbols) if s.bind != STB_LOCAL), len(new_symbols))
    if any(s.bind == STB_LOCAL for s in new_symbols[first_global:]):
        raise DeadStripError("the symbol table doesn't list its local symbols first")

    table = bytearray()
    for symbol in new_symbols:
        shndx = section_map[symbol.shndx] if SHN_UNDEF < symbol.shndx < SHN_LORESERVE else symbol.shndx
        table += SYMBOL.pack(symbol.name_offset, symbol.value, symbol.size, symbol.info, symbol.other, shndx)
    sections[obj.symtab].body = bytes(table)
    sections[obj.symtab].info = first_global
    for index, relocations in surviving.items():
        entry = REL if sections[index].type == SHT_REL else RELA
        body = bytearray()
        for r in relocations:
            info = symbol_map[r.symbol] << 8 | r.type
            body += entry.pack(r.offset, info) if r.addend is None else entry.pack(r.offset, info, r.addend)
        sections[index].body = bytes(body)
        sections[index].info = section_map[sections[index].info]
    new_sections = []
    for i, section in enumerate(sections):
        if i in removed:
            continue
        if section.link:
            if section.link in removed:
                raise DeadStripError(f"{section.name} ({i}) links to a dropped section")
            section.link = section_map[section.link]
        new_sections.append(section)
    obj.sections = new_sections
    obj.shstrndx = section_map[obj.shstrndx]
    return Stripped(serialize(obj), gone, undefined, report)


def symbol_names(data: bytes) -> tuple[set[str], set[str]]:
    """Returns the names of the global symbols an object defines and of the symbols it refers to without defining."""
    _, symbols = read_symbols(read_sections(data)[1])
    defined = {s.name for s in symbols if s.name and s.bind != STB_LOCAL and s.shndx != SHN_UNDEF}
    undefined = {s.name for s in symbols if s.name and s.shndx == SHN_UNDEF}
    return defined, undefined


def rom_symbol_names(config_dir: Path) -> set[str]:
    """Returns the names that a version's symbols.txt files give the ROM's symbols, under config/<version>."""
    names = set()
    for path in sorted(config_dir.rglob("symbols.txt")):
        for line in path.read_text().splitlines():
            if line.split():
                names.add(line.split()[0])
    return names


def check_references(strips: Iterable[tuple[str, list[str], Stripped]], objects: Iterable[tuple[str, bytes]],
                     known: set[str]) -> list[str]:
    """Returns an error for each reference that an object makes to what a strip removed from a source file: a symbol
    config/deadstrip.toml lists, or one that only the dropped code referred to and that neither the ROM (known, the
    names of the version's symbols.txt files) nor any of the objects defines. mwldarm would link a call to either
    through a veneer to address 0 rather than fail. Each strip is the source, its dead symbols and the strip's result;
    the objects are the rest of the link, by a name for messages, the stripped objects included."""
    strips = list(strips)
    if not strips:
        return []
    defined = set(known)
    references = []
    for name, data in objects:
        try:
            defines, undefined = symbol_names(data)
        except DeadStripError as error:
            return [f"{name}: {error}"]
        defined |= defines
        references.append((name, undefined))
    guarded = {}
    for source, dead, stripped in strips:
        for symbol in dead:
            guarded.setdefault(symbol, f"config/deadstrip.toml lists as dead code of {source}")
        for symbol in sorted(stripped.undefined - defined):
            guarded.setdefault(symbol, f"nothing defines: only the code dead-stripped from {source} called it")
    return [f"{name}: refers to {symbol}, which {guarded[symbol]}"
            for name, undefined in references for symbol in sorted(undefined & guarded.keys())]


def manifest_key(source: Path, root: Path = ROOT) -> str | None:
    """Returns a source file's path as config/deadstrip.toml lists it, or None for a file outside the project."""
    try:
        return (root / source).resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return None


def load_manifest(path: Path = MANIFEST, root: Path = ROOT) -> dict[str, list[str]]:
    """Reads config/deadstrip.toml: each source file's dead symbols, under [deadstrip]."""
    with path.open("rb") as stream:
        manifest = tomllib.load(stream)
    if set(manifest) - {"deadstrip"}:
        raise DeadStripError(f"{path}: unknown tables {sorted(set(manifest) - {'deadstrip'})}")
    entries = manifest.get("deadstrip", {})
    if not isinstance(entries, dict):
        raise DeadStripError(f"{path}: [deadstrip] must be a table")
    for source, names in entries.items():
        relative = PurePosixPath(source)
        if (relative.is_absolute() or ".." in relative.parts or relative.as_posix() != source
                or relative.suffix not in (".c", ".cpp") or not (root / relative).is_file()
                or manifest_key(Path(source), root) != source):
            raise DeadStripError(f"{path}: invalid source path {source!r}")
        if not isinstance(names, list) or any(not isinstance(name, str) or not IDENTIFIER.fullmatch(name)
                                              for name in names):
            raise DeadStripError(f"{path}: {source}: the dead symbols must be a list of C names")
        if len(set(names)) != len(names):
            raise DeadStripError(f"{path}: {source}: a symbol is listed twice")
    return {source: list(names) for source, names in entries.items()}


def dead_symbols(source: Path, path: Path = MANIFEST, root: Path = ROOT) -> list[str]:
    """Returns the dead symbols config/deadstrip.toml lists for a source file, none for a file it doesn't list."""
    return load_manifest(path, root).get(manifest_key(source, root) or "", [])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", type=Path, required=True, help="the source file the object was compiled from")
    parser.add_argument("--manifest", type=Path, default=MANIFEST, help="defaults to config/deadstrip.toml")
    parser.add_argument("--verbose", action="store_true", help="list what was dropped")
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    try:
        manifest = load_manifest(args.manifest, ROOT)
        key = manifest_key(args.source, ROOT)
        if key not in manifest:
            raise DeadStripError(f"{args.source} is not listed in {args.manifest}")
        stripped = strip(args.input.read_bytes(), manifest[key])
    except DeadStripError as error:
        print(f"deadstrip.py: {args.input}: error: {error}", file=sys.stderr)
        args.output.unlink(missing_ok=True)
        return 1
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(stripped.data)
    if args.verbose:
        print("\n".join(stripped.report))
    return 0


if __name__ == "__main__":
    sys.exit(main())
