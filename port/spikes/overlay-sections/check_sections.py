#!/usr/bin/env python3
"""Counts the sections of a Mach-O link that has per-overlay sections, before and after the link.

A frozen phase-0 spike (port/spikes/README.md), outside Frigate's build: the live code is
port/tools/gen_overlay_sections.py; change that, not this copy.

    check_sections.py predict --inputs FILE --overlays FILE --out FILE [--limit 255] [--overlay-budget N]
                              [--nm PATH] [--bitcode-extra N]
    check_sections.py image EXE --map FILE --overlays FILE [--predicted FILE] [--expect-leak NAME@ID ...]
                            [--ignore-prefix PREFIX ...]

A Mach-O image holds at most 255 sections (nlist's n_sect is one byte), and past that ld64 fails with errors that
don't name the cause. "predict" runs as the program's PRE_LINK step, so it can stop the link with a message that
does. It reads every input of the link, including the compiler's runtime archives, loads archive members the way
ld64 does (a member is loaded when it defines a symbol that is still undefined), and counts the output sections that
the loaded objects bring, plus those the linker makes itself. The count is exact for native objects. For LLVM
bitcode (LTO), whose sections the back end picks only at link time, it is a range (see predict): the step fails
when the lower end is over the limit and warns when only the upper end is.

"image" runs after the link, as a test. It counts the image's sections, checks them against the prediction, and
reads the linker's map (-Wl,-map) to check that every writable object of an overlay's files lies in that overlay's
sections: a "leak" is one that a reset would miss. It works on the linked image, so it also sees what LTO did.
Under ThinLTO the program must be linked with -Wl,-object_path_lto,DIR, which names the per-module objects that
the map attributes local symbols to.

Both read the same overlays file, one line per overlay object: "ID LAYOUT SUFFIX", where LAYOUT is split or combined
and SUFFIX is the end of the object's path (the real source-list generator knows every overlay file's path).
Only the standard library is used.
"""
import argparse
import os
import platform
import re
import struct
import subprocess
import sys
import tempfile

MH_MAGIC_64 = 0xFEEDFACF
CPU_TYPE_ARM64, CPU_TYPE_X86_64 = 0x0100000C, 0x01000007
LC_SEGMENT_64 = 0x19
LC_SYMTAB = 0x2
S_ATTR_DEBUG = 0x02000000
N_STAB, N_TYPE, N_EXT, N_UNDF, N_SECT = 0xE0, 0x0E, 0x01, 0x0, 0xE
BITCODE_MAGICS = (b"\xde\xc0\x17\x0b", b"BC\xc0\xde")  # the wrapper Apple clang writes, and raw bitcode

# Sections ld64 makes itself in an executable that calls a dynamic library: the stubs, the GOT and the compact
# unwind table. They are counted as always present (all three exist in every program here and in the game).
SYNTHESIZED = {("__TEXT", "__stubs"), ("__DATA_CONST", "__got"), ("__TEXT", "__unwind_info")}

# Input sections that ld64 (ld-prime, Xcode 15 and later) merges or renames in the output, as measured: the literal
# pools and C++'s static initializers join other sections, __DATA,__const and the terminators move to __DATA_CONST
# (read-only after fixups), and the initializer pointers become __TEXT,__init_offsets. Only the merges change the
# count.
OUTPUT_NAME = {("__TEXT", "__literal4"): ("__TEXT", "__const"), ("__TEXT", "__literal8"): ("__TEXT", "__const"),
               ("__TEXT", "__literal16"): ("__TEXT", "__const"), ("__TEXT", "__StaticInit"): ("__TEXT", "__text"),
               ("__DATA", "__const"): ("__DATA_CONST", "__const"),
               ("__DATA", "__mod_init_func"): ("__TEXT", "__init_offsets"),
               ("__DATA", "__mod_term_func"): ("__DATA_CONST", "__mod_term_func")}

# What the back end may emit for a bitcode module that is not an overlay file, as output sections: code,
# constants, literals, and writable data. Sections that instrumentation names explicitly (ASan's __asan_globals,
# coverage's __llvm_prf_*) are not visible in bitcode either; --bitcode-extra adds room for them.
BITCODE_STANDARD = {("__TEXT", "__text"), ("__TEXT", "__const"), ("__TEXT", "__cstring"), ("__DATA_CONST", "__const"),
                    ("__DATA", "__data"), ("__DATA", "__bss"), ("__DATA", "__common")}

OVERLAY_SECTION = re.compile(r"__ov(\d{3})(?:_([db]))?$")
# ld64 resolves section$start$SEG$SECT and section$end$SEG$SECT to a section's bounds, and creates the section,
# empty, when no input has it. The profile runtime refers to coverage sections this way.
SECTION_BOUND = re.compile(r"^_?section\$(?:start|end)\$([^$]+)\$(.+)$")


def fail(message):
    sys.exit(f"check_sections: {message}")


def cstr(field):
    return field.split(b"\0", 1)[0].decode("ascii", "replace")


def read_overlays(path):
    """[(suffix, id, layout)], longest suffix first so that a more specific line wins."""
    rows = []
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) == 3:
                rows.append((parts[2], int(parts[0]), parts[1]))
    return sorted(rows, key=lambda row: -len(row[0]))


def overlay_of(overlays, path):
    for suffix, ov, layout in overlays:
        if path.endswith(suffix):
            return ov, layout
    return None


def thin(data):
    """The slice of a universal (fat) file for this machine's architecture, or the data itself. The compiler's
    runtime archives (libclang_rt.*.a) are universal."""
    if data[:4] not in (b"\xca\xfe\xba\xbe", b"\xca\xfe\xba\xbf"):
        return data
    wide = data[3] == 0xBF  # fat_arch_64
    want = {"arm64": CPU_TYPE_ARM64, "x86_64": CPU_TYPE_X86_64}[platform.machine()]
    count = struct.unpack_from(">I", data, 4)[0]
    for i in range(count):
        if wide:
            cputype, subtype, offset, size = struct.unpack_from(">iiQQ", data, 8 + 32 * i)
        else:
            cputype, subtype, offset, size = struct.unpack_from(">iiII", data, 8 + 20 * i)
        if cputype == want and subtype & 0xFFFFFF in (0, 3):  # ALL (x86_64_ALL is 3), not arm64e or x86_64h
            return data[offset:offset + size]
    raise ValueError("no slice for this architecture")


def read_input(path):
    """Returns (is_archive, [(name, bytes)]): an object file, or each member of a static archive (BSD or GNU
    format), named path(member) as ld64's map names them."""
    with open(path, "rb") as f:
        data = thin(f.read())
    if not data.startswith(b"!<arch>\n"):
        return False, [(path, data)]
    out = []
    off = 8
    while off + 60 <= len(data):
        header = data[off:off + 60]
        name = header[:16].decode("ascii", "replace").strip()
        size = int(header[48:58].decode("ascii").strip())
        body = data[off + 60:off + 60 + size]
        if name.startswith("#1/"):  # BSD long name: the name precedes the member's contents
            length = int(name[3:])
            name = body[:length].split(b"\0", 1)[0].decode("ascii", "replace")
            body = body[length:]
        off += 60 + size + (size & 1)
        if name.startswith("__.SYMDEF") or name in ("/", "//", "/SYM64/"):
            continue  # the archive's symbol tables
        out.append((f"{path}({name.rstrip('/')})", body))
    return True, out


def macho(data):
    """Returns ([(segname, sectname, flags, addr, size)], symtab) of a 64-bit Mach-O file, where symtab is
    [(name, n_type, n_value)]."""
    magic, _, _, _, ncmds = struct.unpack_from("<5I", data, 0)
    if magic != MH_MAGIC_64:
        raise ValueError("not a 64-bit Mach-O file")
    sections, symtab = [], []
    off = 32
    for _ in range(ncmds):
        cmd, cmdsize = struct.unpack_from("<2I", data, off)
        if cmd == LC_SEGMENT_64:
            nsects = struct.unpack_from("<I", data, off + 64)[0]
            for i in range(nsects):
                s = off + 72 + 80 * i
                addr, size = struct.unpack_from("<2Q", data, s + 32)
                flags = struct.unpack_from("<I", data, s + 64)[0]
                sections.append((cstr(data[s + 16:s + 32]), cstr(data[s:s + 16]), flags, addr, size))
        elif cmd == LC_SYMTAB:
            symoff, nsyms, stroff = struct.unpack_from("<3I", data, off + 8)
            for i in range(nsyms):
                strx, ntype, _, _, value = struct.unpack_from("<IBBHQ", data, symoff + 16 * i)
                end = data.index(b"\0", stroff + strx)
                symtab.append((data[stroff + strx:end].decode("ascii", "replace"), ntype, value))
        off += cmdsize
    return sections, symtab


class Module:
    """One object of the link: its external definitions and references, and what it brings to the image."""

    def __init__(self, name, data, nm):
        self.name = name
        self.bitcode = data[:4] in BITCODE_MAGICS
        self.defined, self.undefined = set(), set()
        self.sections = set()
        self.writable = False
        if self.bitcode:
            self.read_bitcode(data, nm)
        else:
            self.read_native(data)
        for symbol in self.undefined:
            m = SECTION_BOUND.match(symbol)
            if m:
                self.sections.add(OUTPUT_NAME.get((m.group(1), m.group(2)), (m.group(1), m.group(2))))

    def read_native(self, data):
        """Every section, except those ld64 consumes (__LD, compact unwind) or leaves in the object (debug).
        A common symbol (a tentative definition without -fno-common) gets __DATA,__common."""
        sections, symtab = macho(data)
        self.sections = {OUTPUT_NAME.get((seg, sect), (seg, sect)) for seg, sect, flags, _, _ in sections
                         if seg not in ("__LD", "__DWARF") and not flags & S_ATTR_DEBUG}
        for name, t, value in symtab:
            if t & N_STAB or not t & N_EXT:
                continue
            if t & N_TYPE == N_SECT:
                self.defined.add(name)
            elif t & N_TYPE == N_UNDF and value:
                self.sections.add(("__DATA", "__common"))
                self.defined.add(name)
            elif t & N_TYPE == N_UNDF:
                self.undefined.add(name)

    def read_bitcode(self, data, nm):
        """Symbols from nm, which reads bitcode through LLVM: d, D, b and B are writable data."""
        with tempfile.NamedTemporaryFile(suffix=".o", delete=False) as tmp:
            tmp.write(data)
        try:
            out = subprocess.run([nm, tmp.name], capture_output=True, text=True)
        finally:
            os.unlink(tmp.name)
        if out.returncode != 0:
            fail(f"{nm} cannot read {self.name}: {out.stderr.strip()}")
        for line in out.stdout.splitlines():
            parts = line.split()
            if len(parts) < 2:
                continue
            kind, name = parts[-2], parts[-1]
            if kind == "U":
                self.undefined.add(name)
            elif kind.isupper():
                self.defined.add(name)
            if kind in ("d", "D", "b", "B"):
                self.writable = True


def load(inputs, nm):
    """The modules ld64 will load: every object file, and each archive member that defines a symbol the loaded
    modules leave undefined, repeated until none does. Symbols that dynamic libraries define aren't known here, so
    a member that defines one of them as well would be loaded needlessly; ld64 searches in command-line order."""
    loaded, pending = [], []
    for path in inputs:
        if not os.path.exists(path):
            fail(f"missing input {path}")
        is_archive, members = read_input(path)
        modules = [Module(name, data, nm) for name, data in members]
        (pending if is_archive else loaded).extend(modules)
    defined = set().union(*(m.defined for m in loaded))
    undefined = set().union(*(m.undefined for m in loaded)) - defined
    changed = True
    while changed:
        changed = False
        for m in list(pending):
            if m.defined & undefined:
                pending.remove(m)
                loaded.append(m)
                defined |= m.defined
                undefined = (undefined | m.undefined) - defined
                changed = True
    return loaded


def overlay_names(ov, layout):
    if layout == "combined":
        return {("__DATA", f"__ov{ov:03d}")}
    return {("__DATA", f"__ov{ov:03d}_d"), ("__DATA", f"__ov{ov:03d}_b")}


def is_overlay_section(seg, sect):
    return seg == "__DATA" and OVERLAY_SECTION.match(sect) is not None


def predict(a):
    """Counts the sections the link will have. Native objects give them exactly. A bitcode module's sections are
    chosen by the LTO back end at link time, so it gives a range: an overlay file with writable objects brings at
    least one of its overlay's sections and at most all of them (in the split layout the back end decides between
    data and bss, and GlobalOpt can move the zero part of an initialized array into bss); any other module brings
    up to every standard section, plus --bitcode-extra for instrumentation's own sections."""
    overlays = read_overlays(a.overlays)
    with open(a.inputs) as f:
        inputs = [line.strip() for line in f if line.strip()]
    certain = set(SYNTHESIZED)
    possible = set()
    unsure = {}  # overlay -> the sections its bitcode may use, at least one of them
    for m in load(inputs, a.nm):
        certain |= m.sections
        if not m.bitcode:
            continue
        certain.add(("__TEXT", "__text"))
        owner = overlay_of(overlays, m.name)
        if owner is None:
            possible |= BITCODE_STANDARD
        elif m.writable:
            unsure[owner[0]] = overlay_names(*owner)
            possible |= unsure[owner[0]]
    extra = a.bitcode_extra if possible else 0
    low = len(certain) + sum(1 for names in unsure.values() if not names & certain)
    high = len(certain | possible) + extra
    overlay_low = sum(1 for s in certain if is_overlay_section(*s)) \
        + sum(1 for names in unsure.values() if not names & certain)
    overlay_high = sum(1 for s in certain | possible if is_overlay_section(*s))

    def show(lo, hi):
        return str(lo) if lo == hi else f"{lo} to {hi}"
    print(f"check_sections: {os.path.basename(a.out)}: {show(low, high)} sections ({show(overlay_low, overlay_high)} "
          f"overlay) of the {a.limit} a Mach-O image allows" + ("" if low == high else "; a range, for LTO bitcode"))
    with open(a.out, "w") as f:
        f.write(f"low {low}\nhigh {high}\noverlay_low {overlay_low}\noverlay_high {overlay_high}\n")
    cause = ("ld64 doesn't say so: past the limit it fails with unrelated errors (\"ADRP out of range\", \"pointer "
             "not aligned\", an assertion in Layout.cpp).")
    if low > a.limit:
        fail(f"{low} sections, more than the {a.limit} a Mach-O image can hold (nlist's n_sect is one byte). {cause} "
             "Use the combined overlay layout, one section per overlay.")
    if high > a.limit:
        print(f"check_sections: warning: up to {high} sections, more than the {a.limit} a Mach-O image can hold. "
              f"The LTO back end decides the bitcode's sections, so the link may still fit. If it fails, {cause} "
              "The combined overlay layout is predictable under LTO: one section per overlay.")
    if a.overlay_budget and overlay_low > a.overlay_budget:
        fail(f"{overlay_low} overlay sections, over the budget of {a.overlay_budget}, which keeps room for the "
             "configuration with the most sections (Debug, -O0, ASan, coverage, SDL3 and C++ linked statically). "
             "Switch to the combined overlay layout before the limit is reached.")


def parse_map(path):
    """Reads an ld64 map: {index: path}, [(start, end, segment, section)], [(address, index, name)]."""
    objects, sections, symbols = {}, [], []
    state = None
    with open(path, errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("# Object files:"):
                state = "objects"
            elif line.startswith("# Sections:"):
                state = "sections"
            elif line.startswith("# Symbols:"):
                state = "symbols"
            elif line.startswith("# Dead Stripped Symbols:"):
                state = None
            elif line.startswith("#") or not line:
                continue
            elif state == "objects":
                m = re.match(r"\[\s*(\d+)\] ?(.*)$", line)
                if m:
                    objects[int(m.group(1))] = m.group(2)
            elif state == "sections":
                parts = line.split("\t")
                start, size = int(parts[0], 16), int(parts[1], 16)
                sections.append((start, start + size, parts[2], parts[3]))
            elif state == "symbols":
                m = re.match(r"(0x[0-9A-Fa-f]+)\t(0x[0-9A-Fa-f]+)\t\[\s*(\d+)\] (.*)$", line)
                if m:
                    symbols.append((int(m.group(1), 16), int(m.group(3)), m.group(4)))
    return objects, sections, symbols


def read_object(path):
    """The bytes of an object, or of an archive member written path(member)."""
    m = re.match(r"(.*)\((.*)\)$", path)
    if m and os.path.exists(m.group(1)):
        return next((d for n, d in read_input(m.group(1))[1] if n == path), b"")
    with open(path, "rb") as f:
        return f.read()


def lto_owners(objects, symbols):
    """Maps each ThinLTO object of the map (N.arch.thinlto.o, from -object_path_lto) to the bitcode input it was
    compiled from. The map attributes a module's external symbols to the bitcode file and its local ones to the
    ThinLTO object, so the object's own external definitions name its module. A module without any falls back to
    its number, which is the module's position among the bitcode inputs."""
    owner_of_global = {}
    for _, index, name in symbols:
        if not re.search(r"\.thinlto\.o$", objects.get(index, "")):
            owner_of_global.setdefault(name, index)
    bitcode_inputs = []
    for index in sorted(objects):
        try:
            if read_object(objects[index])[:4] in BITCODE_MAGICS:
                bitcode_inputs.append(index)
        except OSError:
            continue
    owners = {}
    for index, path in objects.items():
        m = re.search(r"(?:^|/)(\d+)\.[^/]*\.thinlto\.o$", path)
        if not m:
            continue
        _, symtab = macho(read_object(path))
        votes = {}
        for name, t, _ in symtab:
            if not t & N_STAB and t & N_TYPE == N_SECT and t & N_EXT and name in owner_of_global:
                votes[owner_of_global[name]] = votes.get(owner_of_global[name], 0) + 1
        if votes:
            owners[index] = max(votes, key=votes.get)
        elif int(m.group(1)) < len(bitcode_inputs):
            owners[index] = bitcode_inputs[int(m.group(1))]
    return owners


def find_leaks(a):
    """Returns (problems, leaks): every overlay object outside its overlay's sections in the image's writable data,
    and every other object inside an overlay section."""
    overlays = read_overlays(a.overlays)
    objects, sections, symbols = parse_map(a.map)
    if any(path == "" for path in objects.values()):
        fail(f"{a.map} has unnamed LTO objects: link with -Wl,-object_path_lto,DIR so that their symbols can be "
             "attributed")
    owners = lto_owners(objects, symbols)
    sections.sort()
    starts = [s[0] for s in sections]
    ignore = tuple(a.ignore_prefix or ())
    problems, leaks = [], []
    for address, index, name in symbols:
        path = objects.get(index, "")
        if index == 0:
            continue  # linker synthesized: stubs, GOT entries, section$start and section$end
        if re.search(r"(?:^|/)lto\.o$", path):
            problems.append(f"{name}: in {path}, a full-LTO object; its module is unknown (use ThinLTO)")
            continue
        owner = overlay_of(overlays, objects.get(owners.get(index, index), ""))
        lo, hi = 0, len(starts)
        while lo < hi:  # the last section that starts at or below the address
            mid = (lo + hi) // 2
            if starts[mid] <= address:
                lo = mid + 1
            else:
                hi = mid
        if lo == 0 or address >= sections[lo - 1][1]:
            continue
        _, _, seg, sect = sections[lo - 1]
        in_overlay = is_overlay_section(seg, sect)
        generic = seg == "__DATA" and sect in ("__data", "__bss", "__common")
        if owner is None:
            if in_overlay:
                problems.append(f"{name}: from {path}, which is not an overlay file, in {seg},{sect}")
            continue
        ov = owner[0]
        if in_overlay and int(OVERLAY_SECTION.match(sect).group(1)) != ov:
            problems.append(f"{name}: from overlay {ov}, in another overlay's section {seg},{sect}")
        elif generic and not name.startswith(ignore):
            leaks.append((f"{re.sub(r'[.]llvm[.][0-9]+$', '', name)}@{ov}", f"{seg},{sect}", path))
    return problems, leaks


def image(a):
    with open(a.exe, "rb") as f:
        sections, _ = macho(f.read())
    total = len(sections)
    overlay = sum(1 for seg, sect, _, _, _ in sections if is_overlay_section(seg, sect))
    failures = 0
    line = f"image: {total} sections ({overlay} overlay, {total - overlay} other)"
    if a.predicted:
        with open(a.predicted) as f:
            predicted = {k: int(v) for k, v in (line.split() for line in f if line.strip())}
        low, high = predicted["low"], predicted["high"]
        line += f"; predicted before the link: {low}" + ("" if low == high else f" to {high}")
        if not low <= total <= high:
            line += " WRONG"
            failures += 1
    print(line)
    problems, leaks = find_leaks(a)
    for problem in problems[:20]:
        print(f"FAIL {problem}")
    failures += len(problems)
    expected = sorted(a.expect_leak or [])
    found = sorted(name for name, _, _ in leaks)
    unexpected = [row for row in leaks if row[0] not in expected]
    for name, where, path in [row for row in leaks if row[0] in expected] + unexpected[:20]:
        print(f"leak: {name} in {where}" + ("" if name in expected else " FAIL") + f" ({path})")
    if found != expected:
        missing = sorted(set(expected) - set(found))
        print(f"FAIL {len(unexpected)} unexpected leaks" + (f", expected but not found: {missing}" if missing else ""))
        failures += 1
    print("ok" if failures == 0 else "FAILED")
    return 1 if failures else 0


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("predict")
    p.add_argument("--inputs", required=True, help="the link's objects and archives, one path per line")
    p.add_argument("--overlays", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--limit", type=int, default=255)
    p.add_argument("--overlay-budget", type=int, default=0, help="0: none")
    p.add_argument("--nm", default="nm")
    p.add_argument("--bitcode-extra", type=int, default=0)
    i = sub.add_parser("image")
    i.add_argument("exe")
    i.add_argument("--map", required=True)
    i.add_argument("--overlays", required=True)
    i.add_argument("--predicted")
    i.add_argument("--expect-leak", action="append", help="NAME@ID, a leak that is known and allowed")
    i.add_argument("--ignore-prefix", action="append", help="writable objects that instrumentation makes")
    a = ap.parse_args()
    if a.cmd == "predict":
        predict(a)
        return 0
    return image(a)


sys.exit(main())
