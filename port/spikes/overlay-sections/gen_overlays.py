#!/usr/bin/env python3
"""Generate synthetic overlays for the overlay-sections spike.

    gen_overlays.py overlays OUTDIR --mode split|combined (--real CONFIG_OVERLAYS_DIR | --count N)
    gen_overlays.py header OUTFILE ID --mode split|combined

"overlays" writes, into OUTDIR:
- ovNNN.c, one file per overlay, with writable statics of every kind sized like the overlay's .data and .bss;
- ovNNN_sections.h, the forced include that the real build would generate for each overlay (see header below);
- ov_table.c, the list of overlays with their test functions and, on Mach-O, their linker-provided bounds;
- ovsec_list.h, the X-macro list of overlay IDs that ovsec.c needs on ELF and COFF;
- sources.txt, the overlay files, read by CMakeLists.txt;
- overlays.txt, the overlay of each overlay object, read by check_sections.py.

--real reads the .data and .bss sizes of every overlay from the decomp's delinks.txt files (only the 168 overlays
that have either); --count makes N overlays with a small .data and .bss each. Only the standard library is used.

"header" writes one forced include. The section names per object format:
- Mach-O: __DATA,__ovNNN_d and __DATA,__ovNNN_b (zerofill); combined: __DATA,__ovNNN (with -fno-zero-initialized-in-bss).
- ELF: ovNNN_d and ovNNN_b, C identifiers so the linker defines __start_ and __stop_ symbols for them.
- COFF: .data$ovNNNm and .bss$ovNNNm, grouped sections that ovsec.c brackets with markers in $a and $z.
"""
import argparse
import pathlib
import re
import sys


def header_text(ov, mode):
    n = f"{ov:03d}"
    if mode == "combined":
        mach = f'#pragma clang section data="__DATA,__ov{n}"'
        elf = f'#pragma clang section data="ov{n}"'
        coff = f'#pragma clang section data=".data$ov{n}m"'
        note = " Zero-initialized objects join the data section because the file is also compiled with\n" \
               " * -fno-zero-initialized-in-bss."
    else:
        mach = f'#pragma clang section data="__DATA,__ov{n}_d" bss="__DATA,__ov{n}_b,zerofill"'
        elf = f'#pragma clang section data="ov{n}_d" bss="ov{n}_b"'
        coff = f'#pragma clang section data=".data$ov{n}m" bss=".bss$ov{n}m"'
        note = ""
    return f"""/* Generated: places this file's writable statics in overlay {ov}'s sections.{note}
 * Constants, string literals and compound literals are not affected. GCC ignores "#pragma clang section", which
 * would leave the overlay's statics where a reset can't find them, so other compilers must say they build
 * without overlay resets. */
#if !defined(__clang__)
#if !defined(OVSEC_DISABLED)
#error "overlay sections need clang; define OVSEC_DISABLED to build without overlay resets"
#endif
#elif defined(__APPLE__)
{mach}
#elif defined(_WIN32)
{coff}
#else
{elf}
#endif
"""


# Sections whose ld64 bounds the test compares with the scan (two boundary symbols each; see table_c).
BOUND_SECTIONS = 64

SEC = re.compile(r"^\s+\.(data|bss)\s+start:0x([0-9a-f]+)\s+end:0x([0-9a-f]+)")


def real_sizes(config):
    """Returns [(id, data_bytes, bss_bytes)] for every overlay with a non-empty .data or .bss."""
    rows = []
    for d in sorted(pathlib.Path(config).iterdir()):
        m = re.fullmatch(r"ov(\d+)", d.name)
        if not m:
            continue
        sizes = {"data": 0, "bss": 0}
        for ln in (d / "delinks.txt").read_text().splitlines():
            if not ln.strip():
                break  # the module's own sections come before the first blank line
            s = SEC.match(ln)
            if s:
                sizes[s.group(1)] = int(s.group(3), 16) - int(s.group(2), 16)
        if sizes["data"] or sizes["bss"]:
            rows.append((int(m.group(1)), sizes["data"], sizes["bss"]))
    return rows


def overlay_c(ov, data, bss):
    """One synthetic overlay: data bytes of initialized statics and bss bytes of zeroed ones, at least one of each
    kind that the overlay has. Dirty() writes every object; Pristine() checks they all hold their initial values.
    The arrays are indexed through the volatile gOvTestZero: with constant indexes GlobalOpt splits an array into
    the elements used, moves a zero one into the bss section, and drops the rest, which would make the sizes and
    the section counts wrong."""
    n = f"{ov:03d}"
    lines = [f"/* Generated: synthetic overlay {ov}, {data} bytes of .data and {bss} bytes of .bss on the DS. */",
             '#include "ovtest.h"', ""]
    dirty, check = [], []
    if data:
        # An int, a pointer with a relocation, and a byte array for the rest of the size.
        rest = max(data - 16, 4)
        lines += [f"static int sSeed = {ov + 1};",
                  "static int *sSeedPtr = &sSeed;",
                  f"static unsigned char sData[{rest}] = {{ {ov & 0xff}, 1, 2, 3 }};",
                  f"int Ov{n}_Calls(int add)",
                  "{",
                  f"    static int sCalls = {ov};  /* function-local, initialized */",
                  "    sCalls += add;",
                  "    return sCalls;",
                  "}"]
        dirty += ["sSeed = -1;", "sSeedPtr = 0;", f"sData[gOvTestZero] ^= 0x5a;",
                  f"sData[gOvTestZero + {rest - 1}] = 0xa5;", f"Ov{n}_Calls(1);"]
        check += [f"sSeed == {ov + 1}", "sSeedPtr == &sSeed", f"sData[gOvTestZero] == {ov & 0xff}",
                  f"sData[gOvTestZero + {rest - 1}] == 0", f"Ov{n}_Calls(0) == {ov}"]
    if bss:
        rest = max(bss - 8, 4)
        lines += ["static int sCounter;",
                  f"static unsigned char sBss[{rest}];",
                  f"int *Ov{n}_Lazy(int create)",
                  "{",
                  "    static int *sLazy;  /* function-local, zero: the 'created if NULL' pattern */",
                  "    if (sLazy == 0 && create) {",
                  "        sLazy = &sCounter;",
                  "    }",
                  "    return sLazy;",
                  "}"]
        dirty += ["sCounter += 7;", "sBss[gOvTestZero] = 1;", f"sBss[gOvTestZero + {rest - 1}] = 0xa5;",
                  f"Ov{n}_Lazy(1);"]
        check += ["sCounter == 0", "sBss[gOvTestZero] == 0", f"sBss[gOvTestZero + {rest - 1}] == 0",
                  f"Ov{n}_Lazy(0) == 0"]
    lines += ["", f"void Ov{n}_Dirty(void)", "{"] + [f"    {s}" for s in dirty] + ["}", "",
              f"int Ov{n}_Pristine(void)", "{", "    return " + "\n        && ".join(check) + ";", "}", ""]
    return "\n".join(lines)


def table_c(rows, mode):
    out = ["/* Generated: the synthetic overlays and, on Mach-O, the bounds ld64 gives for their sections. */",
           '#include "ovtest.h"', ""]
    for ov, data, bss in rows:
        out.append(f"void Ov{ov:03d}_Dirty(void);")
        out.append(f"int Ov{ov:03d}_Pristine(void);")
    out += ["", "#if defined(__APPLE__)",
            "/* section$start$SEG$SECT and section$end$SEG$SECT are resolved by ld64 to a section's bounds. A",
            " * reference to a section that does not exist creates an empty one, so only existing ones are named.",
            f" * ld64 makes one internal section per such symbol and asserts past 255 of them (\"custom section index",
            f" * overflow\"), so only the first {BOUND_SECTIONS} sections are cross-checked this way. */"]
    bounded = set()
    for ov, data, bss in rows:
        n = f"{ov:03d}"
        if mode == "combined":
            secs = [("d", f"__ov{n}")]
        else:
            secs = ([("d", f"__ov{n}_d")] if data else []) + ([("b", f"__ov{n}_b")] if bss else [])
        for k, s in secs:
            if len(bounded) == BOUND_SECTIONS:
                break
            bounded.add((ov, k))
            out.append(f'extern unsigned char gStart{k}{n} __asm("section$start$__DATA${s}");')
            out.append(f'extern unsigned char gEnd{k}{n} __asm("section$end$__DATA${s}");')
    out += ["#endif", "", "volatile int gOvTestZero;", "", "const OvTestEntry gOvTests[] = {"]
    for ov, data, bss in rows:
        n = f"{ov:03d}"
        has_d = (mode == "combined" or data) and (ov, "d") in bounded
        has_b = (mode != "combined" and bss) and (ov, "b") in bounded
        d = f"&gStartd{n}, &gEndd{n}" if has_d else "0, 0"
        b = f"&gStartb{n}, &gEndb{n}" if has_b else "0, 0"
        out.append(f"    {{ {ov}, {data}, {bss}, Ov{n}_Dirty, Ov{n}_Pristine, OVTEST_BOUNDS({d}), OVTEST_BOUNDS({b}) }},")
    out += ["};", "", f"const int gOvTestCount = {len(rows)};", f"const int gOvTestSections = "
            f"{sum((1 if mode == 'combined' else (bool(d) + bool(b))) for _, d, b in rows)};", ""]
    return "\n".join(out)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    o = sub.add_parser("overlays")
    o.add_argument("outdir")
    o.add_argument("--mode", choices=("split", "combined"), required=True)
    g = o.add_mutually_exclusive_group(required=True)
    g.add_argument("--real")
    g.add_argument("--count", type=int)
    h = sub.add_parser("header")
    h.add_argument("outfile")
    h.add_argument("id", type=int)
    h.add_argument("--mode", choices=("split", "combined"), required=True)
    a = ap.parse_args()

    if a.cmd == "header":
        pathlib.Path(a.outfile).parent.mkdir(parents=True, exist_ok=True)
        write(pathlib.Path(a.outfile), header_text(a.id, a.mode))
        return
    out = pathlib.Path(a.outdir)
    out.mkdir(parents=True, exist_ok=True)
    rows = real_sizes(a.real) if a.real else [(i, 64, 64) for i in range(a.count)]
    if any(ov > 999 for ov, _, _ in rows):
        sys.exit("overlay IDs are three digits in section names")
    sources = []
    for ov, data, bss in rows:
        write(out / f"ov{ov:03d}_sections.h", header_text(ov, a.mode))
        write(out / f"ov{ov:03d}.c", overlay_c(ov, data, bss))
        sources.append(f"{ov:03d}")
    write(out / "ov_table.c", table_c(rows, a.mode))
    write(out / "ovsec_list.h", "/* Generated: OVSEC_OVERLAY(three-digit name, id) for every overlay. */\n"
          + "".join(f"OVSEC_OVERLAY({ov:03d}, {ov})\n" for ov, _, _ in rows))
    write(out / "sources.txt", ";".join(sources))
    # For check_sections.py: the overlay and layout of each overlay object, by the end of its path.
    write(out / "overlays.txt", "".join(f"{ov} {a.mode} /ov{ov:03d}.c.o\n" for ov, _, _ in rows))


def write(path, text):
    """Writes only when the content changes, so that re-running CMake does not rebuild everything."""
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


main()
