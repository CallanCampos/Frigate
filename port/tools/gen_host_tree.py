#!/usr/bin/env python3
"""Prepares the generated inputs of the host build, from the decomp's tree, without a base ROM.

    gen_host_tree.py --out build/port/gen

It writes:
  - the headers that ninja generates for the ROM build (data constants, message IDs, easy chat words), into
    build/include/generated as compile_all.py does, since the sources include them;
  - include/: a mirror of the decomp's include/ made of symbolic links (copies, on a Windows account that may not make
    links), without the C library headers the decomp keeps there for MWCC (stdio.h, string.h and the rest), so that the
    host's own C library headers are found instead; include_mirror.txt says which ("links" or "copies"), so that
    port/CMakeLists.txt configures again when a header changes while copies stand in for it;
  - hwinclude/: host variants of the headers that name DS hardware addresses, found ahead of the originals on the
    include path. Each DS address in them goes through PORT_HW_ADDR(), which port/include/port_host.h maps into the
    host memory that port/hal/hw provides; the registers whose reads or writes must act (the geometry command ports,
    the divider's results, VCOUNT, the card slot's SPI bus with its infrared chip, ...) go through the accessor that
    HW_REGISTER_POLICY names. Nothing else in a header changes, and the variants are made again from upstream's
    headers at every configure (ADR 0004);
  - sources.txt: every C file of src/ and lib/ that the host build compiles, one path per line, relative to the
    repository: all of them except the files port/hal/replaced.txt and the modules' port/hal/*/replaced.txt list.

Files are rewritten only when their contents change, so that CMake rebuilds only what depends on a change.
"""
import argparse
import fnmatch
import os
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.decomp.compile_all import generate_constants  # noqa: E402

# The decomp's own C library headers in include/, written for MWCC's MSL. The host build uses the host's C library
LIBC_HEADERS = {
    "ctype.h", "errno.h", "float.h", "limits.h", "locale.h", "math.h", "stdarg.h", "stddef.h", "stdio.h",
    "stdlib.h", "string.h", "wchar.h",
}


# DS hardware and memory map addresses: 0x01000000 (ITCM) up to the end of the GBA slot's space
HW_ADDRESS = re.compile(r"\b(0x0[1-9a-aA-A][0-9a-fA-F]{6})\b")
# A literal address right after a pointer cast, "(vu32 *)0x04000000", "(volatile u64 *)0x04000290", or opening an
# expression after one, "(vu16 *)(0x04004320 + (n) * 8)"
CAST_ADDRESS = re.compile(r"(\(\s*(?:(?:const|volatile)\s+)*[A-Za-z_][A-Za-z0-9_]*(?:\s+(?:const|volatile))*\s*\*\s*\)"
                          r"\s*\(?\s*)(0x0[1-9a-aA-A][0-9a-fA-F]{6})\b")
# A memory-map or register address constant, "#define HW_BG_VRAM 0x06000000", "#define REG_DMA0SAD_ADDR 0x040000b0"
CONSTANT_ADDRESS = re.compile(r"^(\s*#\s*define\s+(?:HW_[A-Z0-9_]+|REG_[A-Z0-9_]+_ADDR)\s+)(0x0[1-9a-aA-A][0-9a-fA-F]{6})\s*$")
# A register macro, "#define reg_GX_VCOUNT (*(vu16 *)0x04000006)", also with a plain or "volatile" type, as cp.h's
# "#define reg_CP_DIV_RESULT (*(u64 *)0x040002a0)" has
REGISTER = re.compile(r"^(\s*#\s*define\s+([A-Za-z0-9_]+)\s+\(\*\()(v[us](?:8|16|32|64)|(?:volatile\s+)?[us](?:8|16|32|64))"
                      r"(\s*\*\)\s*)(0x[0-9a-fA-F]{8})(\)\s*)$")

# The registers whose accesses must act, by address: the host function that returns the memory a read or write of the
# register uses. A write-stream function returns a fresh slot for each write, so that each store is seen; a read
# function computes the value the hardware would hold now (port/hal/hw).
HW_REGISTER_POLICY = {
    0x04000006: "port_hw_vcount",  # VCOUNT: the scanline being drawn
    0x04000100: "port_hw_timer_counter0",  # TM0CNT_L: the counter as read
    0x04000104: "port_hw_timer_counter1",  # TM1CNT_L
    0x040001a0: "port_hw_auxspicnt",  # AUXSPICNT: the card slot's SPI bus, with its infrared chip (port/hal/card)
    0x040001a2: "port_hw_auxspidata",  # AUXSPIDATA: a byte sent to the chip, or its answer
    0x040002a0: "port_hw_div_result",  # DIV_RESULT: the quotient of DIV_NUMER by DIV_DENOM in DIVCNT's mode
    0x040002a8: "port_hw_divrem_result",  # DIVREM_RESULT: the remainder
    0x040002b4: "port_hw_sqrt_result",  # SQRT_RESULT: the square root of SQRT_PARAM in SQRTCNT's mode
    0x04000400: "port_hw_gxfifo",  # GXFIFO: packed geometry commands, one word per write
    0x04000600: "port_hw_gxstat",  # GXSTAT: the geometry engine's status
    0x04000604: "port_hw_ram_count",  # RAM_COUNT: polygons and vertices in the current frame
    0x04000606: "port_hw_vtxram_count",  # VTXRAM_COUNT
    0x04000640: "port_hw_clipmtx_result",  # CLIPMTX_RESULT: the current clip matrix, 16 words
    0x04000680: "port_hw_vecmtx_result",  # VECMTX_RESULT: the current directional vector matrix, 9 words
}
# The geometry command ports: every write is one command parameter for the geometry engine
G3_PORTS = range(0x04000440, 0x04000600)


def translate_header(text: str) -> tuple[str, int]:
    """The host variant of a header's text, and how many addresses it translated."""
    count = 0
    lines = []
    for line in text.split("\n"):
        register = REGISTER.match(line)
        if register:
            head, name, width, cast, address, tail = register.groups()
            value = int(address, 16)
            accessor = HW_REGISTER_POLICY.get(value)
            if accessor is None and value in G3_PORTS:
                accessor = "port_hw_g3_port"
            if accessor == "port_hw_g3_port":
                line = f"{head}{width}{cast}port_hw_g3_port({address}){tail}"
            elif accessor:
                line = f"{head}{width}{cast}{accessor}(){tail}"
            elif HW_ADDRESS.fullmatch(address):
                line = f"{head}{width}{cast}PORT_HW_ADDR({address}){tail}"
            count += 1
            lines.append(line)
            continue
        constant = CONSTANT_ADDRESS.match(line)
        if constant:
            line = f"{constant.group(1)}PORT_HW_ADDR({constant.group(2)})"
            count += 1
        elif not line.lstrip().startswith("//"):
            line, n = CAST_ADDRESS.subn(lambda m: f"{m.group(1)}PORT_HW_ADDR({m.group(2)})", line)
            count += n
        lines.append(line)
    return "\n".join(lines), count


def translate_hw_headers(out: Path) -> list[str]:
    """Writes the host variants of every header that names a DS address into out, keeping each header's path below
    its include directory, removes stale variants, and returns a line per translated header for the report."""
    roots = [ROOT / "include", *sorted(p for p in (ROOT / "lib").glob("*/include") if p.parent.name != "msl")]
    wanted, report = set(), []
    for root in roots:
        for header in sorted(root.rglob("*.h")):
            if header.name in LIBC_HEADERS and root == ROOT / "include":
                continue
            text = header.read_text()
            if not HW_ADDRESS.search(text):
                continue
            translated, count = translate_header(text)
            if count == 0:
                continue
            relative = header.relative_to(root)
            note = f"// Generated by port/tools/gen_host_tree.py from {header.relative_to(ROOT).as_posix()}; don't edit.\n"
            write_if_changed(out / relative, note + translated)
            wanted.add(relative)
            report.append(f"{header.relative_to(ROOT).as_posix()}: {count} addresses")
    if out.exists():
        for stale in out.rglob("*.h"):
            if stale.relative_to(out) not in wanted:
                stale.unlink()
    return report


def write_if_changed(path: Path, text: str):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def remove_entry(path: Path):
    """Removes a link, a file or a copied directory of the mirror."""
    if path.is_dir() and not path.is_symlink():
        shutil.rmtree(path)
    else:
        path.unlink()


def copy_file(source, copy):
    """Copies a file with its time, unless the copy already has the source's size and time: a configure copies only
    the headers that changed, and Ninja rebuilds only what includes them."""
    if os.path.isfile(copy) and not os.path.islink(copy):
        given, made = os.stat(source), os.stat(copy)
        if given.st_size == made.st_size and abs(given.st_mtime_ns - made.st_mtime_ns) < 2_000_000:
            return copy
    return shutil.copy2(source, copy)


def copy_entry(source: Path, copy: Path):
    """Makes copy the same as source, a file or a directory, for a system that refuses symbolic links: Windows without
    the right to make them (an account that isn't an administrator, without Developer Mode)."""
    if copy.is_symlink() or (copy.exists() and copy.is_dir() != source.is_dir()):
        remove_entry(copy)
    if source.is_dir():
        shutil.copytree(source, copy, dirs_exist_ok=True, copy_function=copy_file)
        stale = {p.relative_to(copy) for p in copy.rglob("*")} - {p.relative_to(source) for p in source.rglob("*")}
        for path in sorted(stale, key=lambda p: len(p.parts), reverse=True):
            if (copy / path).exists() or (copy / path).is_symlink():
                remove_entry(copy / path)
    else:
        copy_file(source, copy)


def links_allowed(out: Path) -> bool:
    """Whether the system lets this account make symbolic links in out, tried once with a link to nothing."""
    probe = out / ".link_probe"
    try:
        if probe.is_symlink() or probe.exists():
            probe.unlink()
        probe.symlink_to("nothing")
        probe.unlink()
        return True
    except OSError as error:
        print(f"gen_host_tree: can't make symbolic links ({error}); copying include/ into {out} instead, again "
              "whenever a header changes", file=sys.stderr)
        return False


def mirror_include(out: Path) -> bool:
    """Links every entry of include/ except the C library headers into out, and removes stale links. Where the system
    refuses symbolic links, the entries are copies instead, brought up to date at every configure. True for copies."""
    source = ROOT / "include"
    out.mkdir(parents=True, exist_ok=True)
    wanted = {entry.name for entry in source.iterdir() if entry.name not in LIBC_HEADERS}
    for link in out.iterdir():
        if link.name not in wanted:
            remove_entry(link)
    copying = not links_allowed(out)
    for name in sorted(wanted):
        link = out / name
        if copying:
            copy_entry(source / name, link)
            continue
        target = os.path.relpath(source / name, out)
        if link.is_symlink() and os.readlink(link) == target:
            continue
        if link.is_symlink() or link.exists():
            remove_entry(link)
        # A link to a directory has to say so on Windows; elsewhere the flag is ignored
        link.symlink_to(target, target_is_directory=(source / name).is_dir())
    return copying


def read_replaced() -> list[tuple[str, str]]:
    """Returns the patterns of port/hal/replaced.txt and of each module's port/hal/<module>/replaced.txt, with their
    reasons: a pattern, then " # " and the reason."""
    patterns = []
    hal = ROOT / "port" / "hal"
    for path in [hal / "replaced.txt", *sorted(hal.glob("*/replaced.txt"))]:
        for number, line in enumerate(path.read_text().splitlines(), 1):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            pattern, _, reason = line.partition(" # ")
            if not reason:
                sys.exit(f"{path.relative_to(ROOT)}:{number}: a pattern needs a reason after ' # '")
            patterns.append((pattern.strip(), reason.strip()))
    return patterns


def list_sources() -> list[str]:
    """Every C file the host build compiles, sorted: the ones compile_all.py compiles, minus replaced.txt."""
    sources = sorted(p.relative_to(ROOT).as_posix() for pattern in ("src/**/*.c", "lib/*/src/**/*.c")
                     for p in ROOT.glob(pattern))
    patterns = read_replaced()
    unused = {pattern for pattern, _ in patterns}
    kept = []
    for source in sources:
        matched = [pattern for pattern, _ in patterns if fnmatch.fnmatchcase(source, pattern)]
        unused.difference_update(matched)
        if not matched:
            kept.append(source)
    for pattern in sorted(unused):
        print(f"gen_host_tree: replaced.txt pattern matches no source file: {pattern}", file=sys.stderr)
    return kept


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=Path, required=True, help="directory for the generated files")
    args = parser.parse_args()
    out = args.out if args.out.is_absolute() else ROOT / args.out

    generate_constants()
    copying = mirror_include(out / "include")
    write_if_changed(out / "include_mirror.txt", "copies\n" if copying else "links\n")
    write_if_changed(out / "hwinclude.txt", "".join(f"{line}\n" for line in translate_hw_headers(out / "hwinclude")))
    write_if_changed(out / "sources.txt", "".join(f"{source}\n" for source in list_sources()))


if __name__ == "__main__":
    main()
