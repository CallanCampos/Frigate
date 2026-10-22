#!/usr/bin/env python3
"""Retain DS Protect's genuine MWLD ARM-to-Thumb interworking veneers.

The build supplies --objects, --lcf, --source-object, --output-object,
--output-response, --scratch, --linker, --runner and --link-flags (a JSON argv
list). An ARMv4T prelink uses the current real inputs. Its generated instructions
become local fragments in a derived compiler object; ABS32 relocations retain
the real SDK targets. The normal final link keeps its own processor setting.

This represents generated linker code, not Nintendo's recovered build commands.
No ROM bytes, fixed game addresses, instruction templates or substitute SDK
providers are used. Source instructions, function sizes and data stay intact.
Only the checked MWCC ELF32 ARM object layout is rewritten; other layouts fail
explicitly. The helper uses the standard library, like other build tools.
"""
import argparse
import json
import re
import shlex
import struct
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
SECTION_HEADER = struct.Struct("<10I")
SYMBOL = struct.Struct("<IIIBBH")
REL = struct.Struct("<II")
RELA = struct.Struct("<IIi")
SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB = 1, 2, 3
SHT_RELA, SHT_NOBITS, SHT_REL = 4, 8, 9
R_ARM_PC24, R_ARM_ABS32 = 1, 2


def require(condition, message):
    if not condition:
        raise ValueError(message)


def string_at(data, offset):
    require(0 <= offset < len(data), f"string offset {offset} outside string table")
    end = data.find(b"\0", offset)
    require(end >= 0, "unterminated ELF string")
    return data[offset:end].decode("ascii")


@dataclass
class Section:
    name_offset: int
    type: int
    flags: int
    address: int
    offset: int
    size: int
    link: int
    info: int
    alignment: int
    entry_size: int
    name: str = ""
    data: bytes = b""

    def pack(self):
        return SECTION_HEADER.pack(self.name_offset, self.type, self.flags,
                                   self.address, self.offset, self.size, self.link,
                                   self.info, self.alignment, self.entry_size)


class Elf32:
    """Read ELF32 little-endian ARM sections without interpreting opaque payloads."""

    def __init__(self, path, relocatable=False):
        self.raw = path.read_bytes()
        require(len(self.raw) >= ELF_HEADER.size, f"{path}: truncated ELF header")
        h = ELF_HEADER.unpack_from(self.raw)
        require(h[0][:7] == b"\x7fELF\x01\x01\x01" and h[2] == 40 and h[3] == 1,
                f"{path}: expected ELF32 little-endian ARM version 1")
        require(h[8] == 52 and h[11] == 40, f"{path}: unsupported ELF header sizes")
        require(0 < h[12] < 0xff00 and 0 < h[13] < h[12],
                f"{path}: extended or invalid section indices are unsupported")
        require(h[6] >= 52 and h[6] + h[12] * 40 <= len(self.raw),
                f"{path}: section table outside file")
        if relocatable:
            require(h[1] == 1 and h[4] == 0 and h[5] == 0 and h[9] == 0 and h[10] == 0,
                    f"{path}: rewrite requires ET_REL without program headers or entry point")
            require(h[7] == 0x02100000 and h[0][7:] == bytes(9),
                    f"{path}: expected MWCC interworking flags and System V ELF identification")
        else:
            require(h[1] == 2, f"{path}: prelink must be ET_EXEC")
        self.sections = [Section(*SECTION_HEADER.unpack_from(self.raw, h[6] + i * 40))
                         for i in range(h[12])]
        require(self.sections[0].pack() == bytes(40), f"{path}: nonempty null section")
        for sec in self.sections[1:]:
            require(sec.alignment == 0 or sec.alignment & (sec.alignment - 1) == 0,
                    f"{path}: invalid section alignment")
            require(sec.alignment <= 1 << 20, f"{path}: unsupported oversized alignment")
            if sec.type != SHT_NOBITS:
                require(sec.offset + sec.size <= len(self.raw), f"{path}: section outside file")
                sec.data = self.raw[sec.offset:sec.offset + sec.size]
        names = self.sections[h[13]]
        require(names.type == SHT_STRTAB, f"{path}: missing section-name string table")
        for sec in self.sections:
            sec.name = string_at(names.data, sec.name_offset)
        if relocatable:
            self.validate_rewrite(path, h)

    def validate_rewrite(self, path, header):
        allowed = {SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS, SHT_REL}
        ranges = [(0, 52), (header[6], header[6] + header[12] * 40)]
        for sec in self.sections[1:]:
            # In particular reject GROUP (sh_info is a symbol index), SYMTAB_SHNDX,
            # hashes, version tables and unknown/proprietary MWCC section types.
            require(sec.type in allowed,
                    f"{path}: unsupported section {sec.name!r} type {sec.type:#x}; "
                    "audit its symbol-index metadata before enabling it")
            if sec.type in (SHT_PROGBITS, SHT_NOBITS):
                require(sec.name in (".text", ".rodata", ".data", ".bss", ".ctor", ".comment") or
                        sec.name.startswith(".debug_"),
                        f"{path}: unsupported payload section {sec.name!r}; audit its metadata")
            require(sec.flags & ~7 == 0 and sec.address == 0,
                    f"{path}: unsupported flags/address in {sec.name!r}")
            require(sec.offset % max(1, sec.alignment) == 0,
                    f"{path}: misaligned section {sec.name!r}")
            if sec.type not in (SHT_SYMTAB, SHT_RELA, SHT_REL):
                require(sec.link == 0 and sec.info == 0,
                        f"{path}: unsupported metadata in {sec.name!r}")
            if sec.type != SHT_NOBITS and sec.size:
                ranges.append((sec.offset, sec.offset + sec.size))
        ranges.sort()
        require(all(a[1] <= b[0] for a, b in zip(ranges, ranges[1:])),
                f"{path}: overlapping ELF structures")
        tables = [i for i, s in enumerate(self.sections) if s.type == SHT_SYMTAB]
        require(len(tables) == 1, f"{path}: expected exactly one symbol table")
        self.sym_index = tables[0]
        table = self.sections[self.sym_index]
        require(table.entry_size == 16 and table.size % 16 == 0,
                f"{path}: invalid symbol-table entries")
        require(0 < table.link < len(self.sections) and
                self.sections[table.link].type == SHT_STRTAB and table.link != header[13],
                f"{path}: expected separate symbol and section string tables")
        self.symbols = list(SYMBOL.iter_unpack(table.data))
        require(self.symbols and self.symbols[0] == (0, 0, 0, 0, 0, 0),
                f"{path}: invalid null symbol")
        require(0 < table.info <= len(self.symbols), f"{path}: invalid first-global index")
        strings = self.sections[table.link].data
        require(strings[:1] == b"\0", f"{path}: invalid symbol string table")
        self.symbol_names = []
        for i, sym in enumerate(self.symbols):
            self.symbol_names.append(string_at(strings, sym[0]))
            require((sym[3] >> 4 == 0) == (i < table.info),
                    f"{path}: local/global symbol partition is invalid at {i}")
            require(sym[5] < len(self.sections) or sym[5] in (0xfff1, 0xfff2),
                    f"{path}: unsupported symbol section index {sym[5]:#x}")
        require(not any(n.startswith("__mw_generated_") for n in self.symbol_names),
                f"{path}: already contains generated veneers; use the compiler object")
        self.relocations = {}
        for i, sec in enumerate(self.sections):
            if sec.type not in (SHT_REL, SHT_RELA):
                continue
            entry = RELA if sec.type == SHT_RELA else REL
            require(sec.link == self.sym_index and 0 < sec.info < len(self.sections),
                    f"{path}: invalid relocation linkage in {sec.name!r}")
            require(sec.entry_size == entry.size and sec.size % entry.size == 0,
                    f"{path}: invalid relocation entries in {sec.name!r}")
            records = list(entry.iter_unpack(sec.data))
            for record in records:
                require(record[1] >> 8 < len(self.symbols), f"{path}: relocation symbol out of range")
                require(record[0] < self.sections[sec.info].size,
                        f"{path}: relocation offset outside target section")
            self.relocations[i] = records

    def serialize(self):
        """Keep section indices and raw types; change only file offsets and edited payloads."""
        out = bytearray(self.raw[:52])
        for sec in self.sections[1:]:
            out.extend(bytes(-len(out) % max(1, sec.alignment)))
            sec.offset = len(out)
            if sec.type != SHT_NOBITS:
                out.extend(sec.data)
        out.extend(bytes(-len(out) % 4))
        struct.pack_into("<I", out, 32, len(out))
        for sec in self.sections:
            out.extend(sec.pack())
        return bytes(out)


def read_map(linked):
    rows = []
    region = None
    pattern = re.compile(r"  ([0-9a-fA-F]{8}) ([0-9a-fA-F]{8}) \.text\s+(\S+)\s+\(([^()]+)\)")
    for line in linked.with_suffix(".o.xMAP").read_text().splitlines():
        boundary = re.fullmatch(r"#>[0-9A-Fa-f]{8}\s+(\w+)_TEXT_(START|END) \(linker command file\)", line)
        if boundary:
            region = boundary[1] if boundary[2] == "START" else None
        match = pattern.fullmatch(line)
        if match:
            rows.append((int(match[1], 16), int(match[2], 16), match[3], match[4], region))
    require(rows, f"{linked}: no text entries in prelink map")
    return rows


def generate(original, linked, section_name, expected_count):
    obj = Elf32(original, relocatable=True)
    executable = Elf32(linked)
    regions = [s for s in executable.sections if s.name == section_name]
    require(len(regions) == 1, f"{linked}: expected one {section_name} section")
    payload = (linked.parent / "build" / f"arm9_ov{section_name[2:]}.bin").read_bytes()
    rows = read_map(linked)

    def read_at(address, size):
        offset = address - regions[0].address
        require(0 <= offset and offset + size <= len(payload),
                f"prelink address {address:#x} outside {section_name} payload")
        return payload[offset:offset + size]

    def entry(name, provider=None):
        entries = [r for r in rows if r[2] == name and (provider is None or r[3] == provider)]
        require(len(entries) == 1, f"ambiguous or missing prelink map entry: {name}")
        return entries[0]

    table = obj.sections[obj.sym_index]
    first_global = table.info
    strings = bytearray(obj.sections[table.link].data)
    locals_added = []
    changes = {}
    evidence = []

    def add_local(name, section, offset, size=0, kind=0):
        name_offset = len(strings)
        strings.extend(name.encode("ascii") + b"\0")
        index = first_global + len(locals_added)
        locals_added.append(SYMBOL.pack(name_offset, offset, size, kind, 0, section))
        return index

    for ri, relocs in obj.relocations.items():
        rs = obj.sections[ri]
        si = rs.info
        sec = obj.sections[si]
        if sec.name != ".text":
            continue
        owners = [(i, s) for i, s in enumerate(obj.symbols)
                  if s[5] == si and s[3] & 15 == 2 and s[2]]
        require(len(owners) == 1, f"section {si}: expected one source function")
        owner_index, owner = owners[0]
        owner_name = obj.symbol_names[owner_index]
        require(owner[1] == 0 and owner[2] == sec.size and sec.alignment == 4,
                f"{owner_name}: expected whole-section word-aligned ARM function")
        owner_row = entry(owner_name, original.name)
        require(owner_row[4] == section_name, f"{owner_name}: prelink map region changed")
        require(owner_row[1] == owner[2], f"{owner_name}: prelink function size changed")
        mappings = [obj.symbol_names[i] for i, s in enumerate(obj.symbols)
                    if s[5] == si and s[1] == 0 and obj.symbol_names[i] in ("$a", "$t")]
        require(mappings == ["$a"], f"{owner_name}: expected ARM entry mapping")
        body = bytearray(sec.data)
        redirects, extra = {}, []
        for index, relocation in enumerate(relocs):
            offset, info = relocation[:2]
            if info & 255 != R_ARM_PC24:
                continue
            # PC24 is S + A - P; -8 accounts for ARM's PC pipeline without
            # introducing an offset into the target function.
            require(rs.type == SHT_RELA and relocation[2] == -8,
                    f"{owner_name}: veneer calls require RELA with ARM PC bias -8")
            require(offset % 4 == 0 and offset + 4 <= sec.size,
                    f"{owner_name}: invalid ARM call offset")
            target_index = info >> 8
            target = obj.symbol_names[target_index]
            target_row = entry(target)
            caller = owner_row[0] + offset
            word = int.from_bytes(read_at(caller, 4), "little")
            require((word >> 25) & 7 == 5 and word >> 28 != 15,
                    f"{owner_name}+{offset:#x}: expected ARM B/BL relocation")
            immediate = word & 0xffffff
            if immediate & 0x800000:
                immediate -= 1 << 24
            stub_address = caller + 8 + immediate * 4
            if stub_address == target_row[0]:
                continue
            require(word >> 24 == 0xeb, f"{owner_name}+{offset:#x}: expected ARM BL veneer call")
            require(stub_address == owner_row[0] + len(body),
                    f"{owner_name}: veneer is not contiguous after the source body; audit layout")
            require(not any(r[4] == section_name and r[1] and
                            r[0] < stub_address + 12 and r[0] + r[1] > stub_address
                            for r in rows), f"{owner_name}: veneer overlaps a source function")
            # The real provider's map supplies its instruction mode. Do not infer
            # Thumb merely from the generated pointer's low bit.
            modes = [r[2] for r in rows if r[0] == target_row[0] and r[3] == target_row[3]
                     and r[2] in ("$a", "$t")]
            require(modes == ["$t"], f"{target}: expected actual Thumb provider mapping")
            fragment = read_at(stub_address, 12)
            load, branch, pointer = struct.unpack("<III", fragment)
            # Decode/validate LDR ip,[pc,#0]; BX ip. These predicates never
            # construct instruction bytes: only MWLD's bytes enter the object.
            require(load >> 28 == 14 and (load >> 20) & 255 == 0x59 and
                    (load >> 16) & 15 == 15 and (load >> 12) & 15 == 12 and load & 0xfff == 0,
                    f"{owner_name}: unsupported generated literal load")
            require(branch >> 28 == 14 and (branch >> 4) & 0xffffff == 0x12fff1 and
                    branch & 15 == 12, f"{owner_name}: unsupported generated exchange branch")
            require(pointer == target_row[0] + 1, f"{target}: generated pointer/addend mismatch")
            new_offset = len(body)
            name = f"__mw_generated_{owner_name}_{len(extra)}"
            redirects[index] = add_local(name, si, new_offset, 12, 2)
            add_local("$a", si, new_offset)
            add_local("$d", si, new_offset + 8)
            body.extend(fragment[:8] + bytes(4))
            extra.append((new_offset + 8, target_index, R_ARM_ABS32, 0))
            evidence.append(dict(caller=owner_name, call_offset=hex(offset), target=target,
                                 provider=target_row[3], generated_link_address=hex(stub_address),
                                 generated_pointer=hex(pointer), generated_bytes=fragment.hex(),
                                 new_symbol=name, new_section_offset=hex(new_offset)))
        if extra:
            sec.data, sec.size = bytes(body), len(body)
            changes[ri] = redirects, extra
    require(len(evidence) == expected_count,
            f"expected {expected_count} veneers in {section_name}, found {len(evidence)}; "
            "audit source/SDK modes and layout before changing the count")
    added = len(locals_added)

    def shifted(index):
        return index + added if index >= first_global else index

    for ri, relocs in obj.relocations.items():
        sec = obj.sections[ri]
        redirects, extra = changes.get(ri, ({}, []))
        rewritten = bytearray()
        entry_format = RELA if sec.type == SHT_RELA else REL
        # Includes every debug relocation, not just the text relocations.
        for index, relocation in enumerate(relocs):
            offset, info = relocation[:2]
            symbol_index = redirects.get(index, shifted(info >> 8))
            require(symbol_index < 1 << 24, "ELF32 relocation symbol index overflow")
            rewritten.extend(entry_format.pack(offset, symbol_index << 8 | (info & 255),
                                               *relocation[2:]))
        for offset, symbol_index, kind, addend in extra:
            rewritten.extend(RELA.pack(offset, shifted(symbol_index) << 8 | kind, addend))
        sec.data, sec.size = bytes(rewritten), len(rewritten)
    table.data = table.data[:first_global * 16] + b"".join(locals_added) + table.data[first_global * 16:]
    table.size, table.info = len(table.data), first_global + added
    strtab = obj.sections[table.link]
    strtab.data, strtab.size = bytes(strings), len(strings)
    return obj.serialize(), evidence


def response_objects(text):
    """Accept dsd's quoted object lines and overlay directives without shell parsing."""
    objects = []
    for line in text.splitlines():
        match = re.fullmatch(r'\s*"([^"\r\n]+)"\s*', line)
        if match:
            objects.append(match[1])
        else:
            require(not line.strip() or re.fullmatch(r"-overlay(?:group)? [A-Za-z0-9_,]+", line),
                    f"unsupported response line: {line!r}")
    require(objects, "empty object response")
    return objects


def memory_outputs(text, scratch):
    """Reject unfamiliar LCF output syntax before MWLD can write outside scratch."""
    match = re.match(r"\s*MEMORY\s*\{([^{}]*)\}", text)
    require(match is not None, "LCF must start with one MEMORY block")
    regions = set()
    outputs = []
    for line in match[1].splitlines():
        if not line.strip():
            continue
        row = re.fullmatch(r"\s*([A-Za-z0-9_]+)\s*:\s*ORIGIN\s*=\s*[^>\r\n]+?"
                           r"(?:>\s*(build/[A-Za-z0-9_]+\.bin))?\s*", line)
        require(row is not None, f"unsupported MEMORY line: {line!r}")
        require(row[1] not in regions, f"duplicate MEMORY region: {row[1]}")
        regions.add(row[1])
        if row[2] is None:
            continue
        path = scratch / row[2]
        require(path.resolve().is_relative_to(scratch.resolve()) and not path.is_symlink(),
                f"LCF output escapes private scratch: {path}")
        outputs.append(path)
    for line in text[match.end():].splitlines():
        if ">" in line:
            placement = re.fullmatch(r"\s*}\s*>\s*([A-Za-z0-9_]+)\s*", line)
            require(placement is not None and placement[1] in regions,
                    f"unsupported output/region selection outside MEMORY: {line!r}")
    require(outputs and len(set(outputs)) == len(outputs), "missing or duplicate MEMORY outputs")
    return outputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("objects", "lcf", "source-object", "output-object", "output-response", "scratch", "linker"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--runner", default="", help="one executable path, not a shell command")
    parser.add_argument("--link-flags", required=True, help="normal link argv as a JSON list")
    parser.add_argument("--section", default="OV165")
    parser.add_argument("--expected-count", type=int, default=8)
    args = parser.parse_args()
    try:
        require(re.fullmatch(r"OV[0-9]{3}", args.section) and args.expected_count > 0,
                "expected an OVnnn section and a positive veneer count")
        flags = json.loads(args.link_flags)
        require(isinstance(flags, list) and all(isinstance(f, str) for f in flags),
                "--link-flags must be a JSON list of argv strings")
        require(flags.count("-proc") == 1 and flags.index("-proc") + 1 < len(flags),
                "link flags must contain exactly one -proc setting")
        flags[flags.index("-proc") + 1] = "v4t"
        # Paths and output flags come solely from the checked inputs below.
        require(not any(f in ("-o", "-lcf") or f.startswith("@") for f in flags),
                "link flags must not supply input/output paths")
        source_text = args.objects.read_text()
        objects = response_objects(source_text)
        matches = [p for p in objects if Path(p).resolve() == args.source_object.resolve()]
        require(len(matches) == 1, "source object must occur exactly once in the response")
        require(sum(Path(p).name == args.source_object.name for p in objects) == 1,
                "LCF object basename is ambiguous")
        require(args.output_object.name == args.source_object.name,
                "derived object must keep the basename selected by the LCF")
        require(args.output_object.parent.resolve() == args.output_response.parent.resolve(),
                "derived object and response must share an output directory")
        require(args.scratch.resolve().parent == args.output_object.parent.resolve(),
                "private prelink directory must be a child of the output directory")
        inputs = [args.objects, args.lcf, args.linker, *map(Path, objects)]
        require(all(not p.resolve().is_relative_to(args.scratch.resolve()) for p in inputs),
                "private prelink directory contains an input")
        outputs = [args.output_object, args.output_response,
                   args.output_object.parent / "generation-evidence.json"]
        require(len({p.resolve() for p in outputs}) == 3 and
                all(p.resolve() not in {i.resolve() for i in inputs} for p in outputs),
                "an output aliases an input or another output")
        require(all(not p.is_symlink() for p in outputs), "output symlinks are unsupported")
        Elf32(args.source_object, relocatable=True)
        text = args.lcf.read_text()
        payloads = memory_outputs(text, args.scratch)
        require(any(p.name == f"arm9_ov{args.section[2:]}.bin" for p in payloads),
                f"LCF has no payload for {args.section}")
        args.scratch.mkdir(parents=True, exist_ok=True)
        (args.scratch / "build").mkdir(exist_ok=True)
        linked = args.scratch / "arm9.o"
        private_lcf = args.scratch / "arm9.lcf"
        # Stale successful outputs cannot satisfy a failed/incomplete prelink.
        for path in [linked, linked.with_suffix(".o.xMAP"), private_lcf, *payloads]:
            require(not path.is_symlink(), f"private output is a symlink: {path}")
            path.unlink(missing_ok=True)
        private_lcf.write_text(text)
        command = ([args.runner] if args.runner else []) + [str(args.linker), *flags,
                   "@" + str(args.objects), str(private_lcf), "-o", str(linked)]
        run = subprocess.run(command, capture_output=True, text=True)
        log = args.scratch / "prelink.log"
        require(not log.is_symlink(), f"private log is a symlink: {log}")
        log.write_text(shlex.join(command) + "\n" + run.stdout + run.stderr)
        require(run.returncode == 0, f"MWLD prelink exited {run.returncode}; see {log}\n{run.stdout}{run.stderr}")
        require(not run.stdout and not run.stderr,
                f"MWLD prelink emitted diagnostics; see {log}\n{run.stdout}{run.stderr}")
        data, evidence = generate(args.source_object, linked, args.section, args.expected_count)
        old_line = re.compile(r'(?m)^(\s*)"' + re.escape(matches[0]) + r'"([ \t]*)$')
        replacement = args.output_object.as_posix()
        require('"' not in replacement and "\n" not in replacement and "\r" not in replacement,
                "output path cannot be represented in an MWLD response")
        response, count = old_line.subn(lambda m: f'{m[1]}"{replacement}"{m[2]}', source_text)
        require(count == 1, "expected one complete object line to replace")
        args.output_object.parent.mkdir(parents=True, exist_ok=True)
        args.output_object.write_bytes(data)
        args.output_response.write_text(response)
        (args.output_object.parent / "generation-evidence.json").write_text(json.dumps(evidence, indent=2) + "\n")
        print(f"Retained {len(evidence)} genuine MWLD veneers in {args.output_object}")
    except (ValueError, OSError, struct.error) as error:
        sys.exit(f"DS Protect veneer generation failed: {error}")


if __name__ == "__main__":
    main()
