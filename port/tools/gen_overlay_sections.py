#!/usr/bin/env python3
"""Per-overlay sections for the writable statics of overlay code, and the check of a Mach-O link's section count.

    gen_overlay_sections.py generate --version b2_us --out build/port/gen/ovsec [--layout split|combined]
    gen_overlay_sections.py check --inputs FILE --out FILE [--limit 255] [--overlay-budget 150]
                                  [--overlay-objects FILE]
    gen_overlay_sections.py image PROGRAM --predicted FILE

On the DS, loading an overlay clears its .bss and copies its .data from the ROM, every time, and some game code relies
on it (port/docs/research/engine-boot.md, section 3.3). The host links every overlay into the program, so each
overlay's C files are compiled with a forced include that puts their writable statics in sections of that overlay
alone, and port/hal/fs/ovsec.c restores them where the DS would load the overlay. The design and its measurements are
the reviewed spike's (port/spikes/overlay-sections/README.md).

"generate" reads which source file belongs to which overlay from config/<version>/arm9/overlays/ovNNN/delinks.txt and
writes, into the output directory:
  - ovNNN_sections.h for every overlay with source files: the forced include, whose "#pragma clang section" names the
    overlay's sections for each object format (split layout: data and bss apart; combined: one section, which needs
    -fno-zero-initialized-in-bss on the overlay's files);
  - ovsec_list.h: OVSEC_OVERLAY(NNN, id) for every one of those overlays, which ovsec.c needs on ELF and COFF, where
    the sections' bounds are symbols;
  - overlay_sources.cmake: a call of port_ovsec_source(PATH NNN) for every overlay source file that exists, with the
    overlay's number in three digits, which port/cmake/overlay_sections.cmake defines to add the forced include to the
    file's compile options;
  - overlay_objects.txt: "ID LAYOUT PATH" for every one of those files, for the checks.
Files of ARM9 main, the ITCM, the DTCM and the autoloads get no forced include.

"check" runs before the program's link (a PRE_LINK step). A Mach-O image holds at most 255 sections, since nlist's
n_sect is one byte, and past that ld64 fails with errors that don't name the cause, or crashes. It reads every input of
the link (one path per line in the inputs file: objects, and static archives, whose members count only when ld64 would
load them), counts the output sections they bring, plus those the linker makes itself, and stops the link with a
message that names the cause past the limit, or past the budget of overlay sections, which keeps room for the
configurations with the most sections (Debug, sanitizers, coverage). LLVM bitcode (LTO) is refused: its sections are
chosen at link time, and the spike's range check isn't carried over.

With --overlay-objects (generate's overlay_objects.txt), "check" also looks for leaks among the link's objects, as the
spike's map check did after the link: a writable object of an overlay file outside its overlay's sections (in
__DATA,__data, __bss or __common, where the pragma didn't put it, such as a file-scope compound literal), which no
reset would set back; one in another overlay's sections; and an object of a file of no overlay in an overlay's
section. The objects are native (LTO is refused), so their symbol tables say which section holds each object, local
ones included. The data that the sanitizers' instrumentation adds to a file is not the file's own and is left out:
UBSan's check data (l___unnamed_N), ASan's ODR indicators, names and registration flag (___odr_asan_gen_*,
___asan_gen_*, ____asan_globals_registered), and the assembler's section markers (ltmpN).

"image" runs as a test after the link: the linked program must have the sections "check" counted, so that a new ld64
that merges, renames or makes sections differently shows up. Only the standard library is used.
"""
import argparse
import os
import platform
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# --- generate -----------------------------------------------------------------------------------------------------

# A file's entry in a delinks.txt: its path alone on a line, ending with a colon
SOURCE_ENTRY = re.compile(r"^(\S+\.c):$")


def overlay_sources(version: str) -> dict[int, list[str]]:
    """The C files of each overlay, from the overlay's delinks.txt, in the order it lists them."""
    overlays = {}
    owner = {}
    for directory in sorted((ROOT / "config" / version / "arm9" / "overlays").iterdir()):
        match = re.fullmatch(r"ov(\d{3})", directory.name)
        if not match:
            continue
        number = int(match.group(1))
        files = []
        for line in (directory / "delinks.txt").read_text().splitlines():
            entry = SOURCE_ENTRY.match(line)
            if entry:
                path = entry.group(1)
                if path in owner:
                    sys.exit(f"gen_overlay_sections: {path} is in overlays {owner[path]} and {number}")
                owner[path] = number
                files.append(path)
        if files:
            overlays[number] = files
    return overlays


def header_text(number: int, layout: str) -> str:
    name = f"{number:03d}"
    if layout == "combined":
        pragmas = {
            "macho": f'#pragma clang section data="__DATA,__ov{name}"',
            "coff": f'#pragma clang section data=".data$ov{name}m"',
            "elf": f'#pragma clang section data="ov{name}"',
        }
        layout_note = ("// Zero-initialized statics join the data section: the file is also compiled with\n"
                       "// -fno-zero-initialized-in-bss.\n")
    else:
        pragmas = {
            "macho": f'#pragma clang section data="__DATA,__ov{name}_d" bss="__DATA,__ov{name}_b,zerofill"',
            "coff": f'#pragma clang section data=".data$ov{name}m" bss=".bss$ov{name}m"',
            "elf": f'#pragma clang section data="ov{name}_d" bss="ov{name}_b"',
        }
        layout_note = ""
    return (
        "// Generated by port/tools/gen_overlay_sections.py; don't edit.\n"
        f"// Forced into every C file of overlay {number}: puts the file's writable statics (globals, file and\n"
        "// function-local statics, zeroed or not) in the overlay's own sections, which port/hal/fs/ovsec.c sets back\n"
        "// when the game loads the overlay, as the DS does. Constants, string literals and code stay where they\n"
        "// were.\n"
        f"{layout_note}"
        "// GCC ignores \"#pragma clang section\", which would leave the statics where no reset finds them, so\n"
        "// another compiler has to say that it builds without overlay resets.\n"
        "#if !defined(__clang__)\n"
        "#if !defined(PORT_OVSEC_DISABLED)\n"
        "#error \"overlay sections need clang; define PORT_OVSEC_DISABLED to build without overlay resets\"\n"
        "#endif\n"
        "#elif defined(__APPLE__)\n"
        f"{pragmas['macho']}\n"
        "#elif defined(_WIN32)\n"
        f"{pragmas['coff']}\n"
        "#else\n"
        f"{pragmas['elf']}\n"
        "#endif\n")


def write_if_changed(path: Path, text: str) -> None:
    """Writes only a change, so that configuring again rebuilds nothing that didn't change."""
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def generate(args: argparse.Namespace) -> None:
    out = args.out if args.out.is_absolute() else ROOT / args.out
    out.mkdir(parents=True, exist_ok=True)
    overlays = overlay_sources(args.version)
    if any(number > 999 for number in overlays):
        sys.exit("gen_overlay_sections: overlay numbers have three digits in section names")
    cmake_lines = ["# Generated by port/tools/gen_overlay_sections.py; don't edit. port_ovsec_source(PATH NNN) for\n"
                   f"# every C file of an overlay of {args.version}, with the overlay's number in three digits\n"]
    objects = []
    for number, files in overlays.items():
        write_if_changed(out / f"ov{number:03d}_sections.h", header_text(number, args.layout))
        for path in files:
            if not (ROOT / path).exists():
                continue
            cmake_lines.append(f'port_ovsec_source("${{REPO_ROOT}}/{path}" {number:03d})\n')
            objects.append(f"{number} {args.layout} {path}\n")
    write_if_changed(out / "ovsec_list.h",
                     "// Generated by port/tools/gen_overlay_sections.py; don't edit. OVSEC_OVERLAY(NNN, id) for\n"
                     "// every overlay with C files, for port/hal/fs/ovsec.c\n"
                     + "".join(f"OVSEC_OVERLAY({number:03d}, {number})\n" for number in overlays))
    write_if_changed(out / "overlay_sources.cmake", "".join(cmake_lines))
    write_if_changed(out / "overlay_objects.txt", "".join(objects))


# --- check --------------------------------------------------------------------------------------------------------

MH_MAGIC_64 = 0xFEEDFACF
CPU_TYPES = {"arm64": 0x0100000C, "x86_64": 0x01000007}
LC_SEGMENT_64 = 0x19
LC_SYMTAB = 0x2
S_ATTR_DEBUG = 0x02000000
N_STAB, N_TYPE, N_EXT, N_UNDF, N_SECT = 0xE0, 0x0E, 0x01, 0x0, 0xE
BITCODE_MAGICS = (b"\xde\xc0\x17\x0b", b"BC\xc0\xde")

# Sections ld64 makes itself in a program that calls a dynamic library: the stubs, the GOT and the compact unwind
# table. Every program here has all three
SYNTHESIZED = {("__TEXT", "__stubs"), ("__DATA_CONST", "__got"), ("__TEXT", "__unwind_info")}

# Input sections that ld64 (ld-prime, Xcode 15 and later) merges into others or renames, as the spike measured them
OUTPUT_NAME = {
    ("__TEXT", "__literal4"): ("__TEXT", "__const"),
    ("__TEXT", "__literal8"): ("__TEXT", "__const"),
    ("__TEXT", "__literal16"): ("__TEXT", "__const"),
    ("__TEXT", "__StaticInit"): ("__TEXT", "__text"),
    ("__DATA", "__const"): ("__DATA_CONST", "__const"),
    ("__DATA", "__mod_init_func"): ("__TEXT", "__init_offsets"),
    ("__DATA", "__mod_term_func"): ("__DATA_CONST", "__mod_term_func"),
}

OVERLAY_SECTION = re.compile(r"^__ov\d{3}(?:_[db])?$")
# ld64 resolves section$start$SEG$SECT and section$end$SEG$SECT to a section's bounds, and makes the section, empty,
# when no input has it (the profile runtime refers to coverage sections this way)
SECTION_BOUND = re.compile(r"^_?section\$(?:start|end)\$([^$]+)\$(.+)$")


def fail(message: str) -> None:
    sys.exit(f"gen_overlay_sections check: {message}")


def c_string(field: bytes) -> str:
    return field.split(b"\0", 1)[0].decode("ascii", "replace")


def host_slice(data: bytes) -> bytes:
    """The slice of a universal file for this machine; the compiler's runtime archives are universal."""
    if data[:4] not in (b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf"):
        return data
    wide = data[3] == 0xBF
    wanted = CPU_TYPES.get(platform.machine())
    count = struct.unpack_from(">I", data, 4)[0]
    for index in range(count):
        if wide:
            cpu, subtype, offset, size = struct.unpack_from(">iiQQ", data, 8 + 32 * index)
        else:
            cpu, subtype, offset, size = struct.unpack_from(">iiII", data, 8 + 20 * index)
        # The plain slice (subtype ALL, or x86_64's 3), not arm64e or x86_64h
        if cpu == wanted and subtype & 0xFFFFFF in (0, 3):
            return data[offset:offset + size]
    raise ValueError("no slice for this machine")


def read_input(path: str) -> tuple[bool, list[tuple[str, bytes]]]:
    """(whether it is an archive, [(name, contents)]): an object, or every member of a static archive."""
    with open(path, "rb") as f:
        data = host_slice(f.read())
    if not data.startswith(b"!<arch>\n"):
        return False, [(path, data)]
    members = []
    offset = 8
    while offset + 60 <= len(data):
        header = data[offset:offset + 60]
        name = header[:16].decode("ascii", "replace").strip()
        size = int(header[48:58].decode("ascii").strip())
        body = data[offset + 60:offset + 60 + size]
        if name.startswith("#1/"):  # a BSD long name, which comes first in the member's contents
            length = int(name[3:])
            name = body[:length].split(b"\0", 1)[0].decode("ascii", "replace")
            body = body[length:]
        offset += 60 + size + (size & 1)
        if name.startswith("__.SYMDEF") or name in ("/", "//", "/SYM64/"):
            continue
        members.append((f"{path}({name.rstrip('/')})", body))
    return True, members


class Module:
    """One object of the link: its external definitions and references, the output sections it brings, and the symbols
    it defines in the __DATA segment, local ones included, with their sections."""

    def __init__(self, name: str, data: bytes):
        self.name = name
        self.defined: set[str] = set()
        self.undefined: set[str] = set()
        self.sections: set[tuple[str, str]] = set()
        self.data_symbols: list[tuple[str, str]] = []
        if data[:4] in BITCODE_MAGICS:
            fail(f"{name} is LLVM bitcode (LTO), whose sections are chosen at link time; this check counts native "
                 "objects only. Build without LTO, or bring back the spike's range check "
                 "(port/spikes/overlay-sections/check_sections.py)")
        self.read(data)
        for symbol in self.undefined:
            bound = SECTION_BOUND.match(symbol)
            if bound:
                key = (bound.group(1), bound.group(2))
                self.sections.add(OUTPUT_NAME.get(key, key))

    def read(self, data: bytes) -> None:
        """Every section except what ld64 consumes (__LD, compact unwind) or leaves out (debug); a common symbol
        brings __DATA,__common."""
        magic, _, _, _, ncmds = struct.unpack_from("<5I", data, 0)
        if magic != MH_MAGIC_64:
            fail(f"{self.name} is not a 64-bit Mach-O object")
        # Every section in order, which a symbol's n_sect counts from 1
        numbered: list[tuple[str, str]] = []
        symtab = None
        offset = 32
        for _ in range(ncmds):
            cmd, cmdsize = struct.unpack_from("<2I", data, offset)
            if cmd == LC_SEGMENT_64:
                nsects = struct.unpack_from("<I", data, offset + 64)[0]
                for index in range(nsects):
                    section = offset + 72 + 80 * index
                    segment = c_string(data[section + 16:section + 32])
                    key = (segment, c_string(data[section:section + 16]))
                    numbered.append(key)
                    flags = struct.unpack_from("<I", data, section + 64)[0]
                    if segment in ("__LD", "__DWARF") or flags & S_ATTR_DEBUG:
                        continue
                    self.sections.add(OUTPUT_NAME.get(key, key))
            elif cmd == LC_SYMTAB:
                symtab = struct.unpack_from("<3I", data, offset + 8)
            offset += cmdsize
        if symtab is None:
            return
        symoff, nsyms, stroff = symtab
        for index in range(nsyms):
            strx, ntype, nsect, _, value = struct.unpack_from("<IBBHQ", data, symoff + 16 * index)
            if ntype & N_STAB:
                continue
            kind = ntype & N_TYPE
            in_data = kind == N_SECT and 0 < nsect <= len(numbered) and numbered[nsect - 1][0] == "__DATA"
            if not ntype & N_EXT and not in_data:
                continue
            end = data.index(b"\0", stroff + strx)
            name = data[stroff + strx:end].decode("ascii", "replace")
            if in_data:
                self.data_symbols.append((name, numbered[nsect - 1][1]))
            if not ntype & N_EXT:
                continue
            if kind == N_SECT:
                self.defined.add(name)
            elif kind == N_UNDF and value:
                self.sections.add(("__DATA", "__common"))
                self.data_symbols.append((name, "__common"))
                self.defined.add(name)
            elif kind == N_UNDF:
                self.undefined.add(name)


def load(inputs: list[str]) -> list[Module]:
    """The modules ld64 loads: every object, and each archive member that defines a symbol that the loaded modules
    leave undefined, until none does. Dynamic libraries aren't read, so a member that defines a symbol a dylib also
    has may be counted needlessly (the spike saw it once, with a member that brought no new section)."""
    loaded, pending = [], []
    for path in inputs:
        if not os.path.exists(path):
            fail(f"missing input {path}")
        is_archive, members = read_input(path)
        (pending if is_archive else loaded).extend(Module(name, data) for name, data in members)
    defined = set().union(*(module.defined for module in loaded)) if loaded else set()
    undefined = (set().union(*(module.undefined for module in loaded)) if loaded else set()) - defined
    changed = True
    while changed:
        changed = False
        for module in list(pending):
            if module.defined & undefined:
                pending.remove(module)
                loaded.append(module)
                defined |= module.defined
                undefined = (undefined | module.undefined) - defined
                changed = True
    return loaded


# The sections an overlay's writable statics go to (the overlay's number in the first group), and those a leak lies in
OVERLAY_NUMBER = re.compile(r"^__ov(\d{3})(?:_[db])?$")
WRITABLE = {"__data", "__bss", "__common"}
# What the sanitizers and the assembler add to an object, which isn't the source file's own data
INSTRUMENTATION = re.compile(r"^(?:l___unnamed_\d+|_+(?:odr_)?asan_.*|ltmp\d+)$")


def overlay_file(name: str, by_basename: dict[str, list[str]]) -> str | None:
    """The overlay file (its path in the repository) that an object of the link was compiled from, if any. CMake names
    an object for its source's path: .../<path of the source>.o, the absolute path for a source outside the build's
    source directory."""
    if not name.endswith(".c.o"):
        return None
    source = name[:-2]
    for path in by_basename.get(source.rsplit("/", 1)[-1], []):
        if source.endswith("/" + path):
            return path
    return None


def leaks(modules: list[Module], overlay_objects: str) -> None:
    """Stops the link when a writable object is outside its overlay's sections, or in an overlay's sections it
    doesn't belong to."""
    owners = {}
    by_basename: dict[str, list[str]] = {}
    with open(overlay_objects) as f:
        for line in f:
            number, _, path = line.split(maxsplit=2)
            path = path.strip()
            owners[path] = int(number)
            by_basename.setdefault(path.rsplit("/", 1)[-1], []).append(path)
    problems = []
    matched = 0
    for module in modules:
        source = overlay_file(module.name, by_basename)
        overlay = owners.get(source) if source else None
        if overlay is not None:
            matched += 1
        for name, section in module.data_symbols:
            number = OVERLAY_NUMBER.match(section)
            if number and overlay is None:
                problems.append(f"{name} of {module.name}, a file of no overlay, in __DATA,{section}")
            elif number and int(number.group(1)) != overlay:
                problems.append(f"{name} of overlay {overlay}'s {source} in __DATA,{section}")
            elif not number and overlay is not None and section in WRITABLE and not INSTRUMENTATION.match(name):
                problems.append(f"{name} of overlay {overlay}'s {source} in __DATA,{section}")
    if owners and not matched:
        fail(f"none of the {len(owners)} overlay files of {overlay_objects} is among the link's objects, whose "
             "names the leak check reads as CMake makes them (<source path>.o)")
    if problems:
        listed = "\n  ".join(problems[:20]) + (f"\n  and {len(problems) - 20} more" if len(problems) > 20 else "")
        fail(f"{len(problems)} writable statics outside their overlay's sections, which loading the overlay wouldn't "
             f"set back (port/hal/fs/ovsec.c):\n  {listed}\nA file-scope compound literal, or a section the forced "
             "include's pragma doesn't reach, puts one there")
    print(f"gen_overlay_sections check: no leaks among the objects of {matched} overlay files")


def check(args: argparse.Namespace) -> None:
    with open(args.inputs) as f:
        inputs = [line.strip() for line in f if line.strip()]
    sections = set(SYNTHESIZED)
    modules = load(inputs)
    for module in modules:
        sections |= module.sections
    overlay = sum(1 for segment, section in sections if segment == "__DATA" and OVERLAY_SECTION.match(section))
    print(f"gen_overlay_sections check: {os.path.basename(args.out)}: {len(sections)} sections ({overlay} for "
          f"overlays) of the {args.limit} a Mach-O image allows; the overlay budget is {args.overlay_budget}")
    with open(args.out, "w") as f:
        f.write(f"sections {len(sections)}\noverlay_sections {overlay}\n")
    if len(sections) > args.limit:
        fail(f"the link would have {len(sections)} sections, more than the {args.limit} a Mach-O image can hold "
             "(nlist's n_sect is one byte). ld64 wouldn't say so: past the limit it fails with unrelated errors "
             "(\"ADRP out of range\", \"pointer not aligned\", an assertion in Layout.cpp) or crashes. Build the "
             "overlays with the combined layout (-DPORT_OVSEC_LAYOUT=combined), one section per overlay")
    if args.overlay_budget and overlay > args.overlay_budget:
        fail(f"{overlay} overlay sections, over the budget of {args.overlay_budget}, which keeps room for the "
             "configuration with the most sections (Debug, -O0, sanitizers, coverage, libraries linked statically). "
             "Switch to the combined layout (-DPORT_OVSEC_LAYOUT=combined) before the limit is reached")
    if args.overlay_objects:
        leaks(modules, args.overlay_objects)


def image(args: argparse.Namespace) -> None:
    """Compares the linked program's sections with what "check" predicted before the link, so that a change in ld64's
    merging or naming of sections shows as a failing test rather than as a wrong count."""
    with open(args.executable, "rb") as f:
        data = host_slice(f.read())
    magic, _, _, _, ncmds = struct.unpack_from("<5I", data, 0)
    if magic != MH_MAGIC_64:
        fail(f"{args.executable} is not a 64-bit Mach-O program")
    sections = []
    offset = 32
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<2I", data, offset)
        if cmd == LC_SEGMENT_64:
            nsects = struct.unpack_from("<I", data, offset + 64)[0]
            for index in range(nsects):
                section = offset + 72 + 80 * index
                sections.append((c_string(data[section + 16:section + 32]), c_string(data[section:section + 16])))
        offset += cmdsize
    overlay = sum(1 for segment, section in sections if segment == "__DATA" and OVERLAY_SECTION.match(section))
    predicted = {}
    with open(args.predicted) as f:
        for line in f:
            key, _, value = line.partition(" ")
            predicted[key] = int(value)
    print(f"gen_overlay_sections image: {len(sections)} sections ({overlay} for overlays); predicted "
          f"{predicted.get('sections')} ({predicted.get('overlay_sections')})")
    if len(sections) != predicted.get("sections") or overlay != predicted.get("overlay_sections"):
        fail("the program's sections differ from the count before the link: ld64 merges, renames or makes sections "
             "differently than the check knows (OUTPUT_NAME, SYNTHESIZED)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    gen = commands.add_parser("generate", help="the forced includes and the build's list of overlay files")
    gen.add_argument("--version", required=True, choices=("b2_us", "w2_us"))
    gen.add_argument("--out", type=Path, required=True)
    gen.add_argument("--layout", choices=("split", "combined"), default="split")
    chk = commands.add_parser("check", help="count a Mach-O link's sections before it runs")
    chk.add_argument("--inputs", required=True, help="a file with the link's inputs, one path per line")
    chk.add_argument("--out", required=True, help="where to write the counts")
    chk.add_argument("--limit", type=int, default=255)
    chk.add_argument("--overlay-budget", type=int, default=150)
    chk.add_argument("--overlay-objects", help="generate's overlay_objects.txt, to look for leaks")
    img = commands.add_parser("image", help="compare a linked Mach-O program's sections with check's count")
    img.add_argument("executable")
    img.add_argument("--predicted", required=True, help="the counts check wrote")
    args = parser.parse_args()
    if args.command == "generate":
        generate(args)
    elif args.command == "check":
        check(args)
    else:
        image(args)


if __name__ == "__main__":
    main()
