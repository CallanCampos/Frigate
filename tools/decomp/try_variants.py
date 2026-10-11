#!/usr/bin/env python3
"""Try variants of a function's source until one matches.

Each variant replaces the function's definition in the source file, and is compiled and compared with the original
code by compiler_probe.py. The first variant that matches is kept, or the one chosen with --keep; otherwise the file
is restored. Variants are separated by lines of =====, and one may begin with other definitions (static inlines,
say) to put before the function. A prototype of the function before its definition takes the variant's signature.

    try_variants.py src/gfl/particle.c func_0204ff54 variants.c
    try_variants.py src/gfl/particle.c func_0204ff54 --keep 2 variants.c
    try_variants.py src/gfl/particle.c func_0204ff54 --score variants.c
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from configure import MWCC_VERSION  # noqa: E402

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.compiler_probe import lib_compiler  # noqa: E402


def definition_span(source: str, name: str) -> tuple[int, int]:
    """Returns where a function's definition starts and ends."""
    match = re.search(r"^[A-Za-z][^\n;{}]*\b" + re.escape(name) + r"\([^;{]*\)\s*\{", source, re.M)
    if match is None:
        sys.exit(f"no definition of {name}")
    i, depth = match.end(), 1
    while depth:
        depth += {"{": 1, "}": -1}.get(source[i], 0)
        i += 1
    return match.start(), i


def with_variant(source: str, name: str, variant: str) -> str:
    start, end = definition_span(source, name)
    result = source[:start] + variant + source[end:]
    variant_start, _ = definition_span(variant, name)
    signature = variant[variant_start : variant.index("{", variant_start)].strip()
    prototype = re.search(r"^[A-Za-z][^\n;{}=]*\b" + re.escape(name) + r"\([^;{]*\);", result, re.M)
    if prototype and prototype.start() < start:
        result = result[: prototype.start()] + signature + ";" + result[prototype.end() :]
    return result


def probe(args) -> str:
    """Returns the function's status from compiler_probe.py, with the number of lines of its aligned diff that differ
    if scoring."""
    command = [sys.executable, str(Path(__file__).parent / "compiler_probe.py"), str(args.source),
               "--compilers", args.compiler, "--functions", args.function, "--version", args.version]
    if args.extra_flags:
        command += ["--extra-flags=" + args.extra_flags]
    if args.score:
        command += ["--show-diff", args.compiler, "--align"]
    output = subprocess.run(command, capture_output=True, text=True, cwd=ROOT).stdout
    differing = sum(1 for line in output.splitlines() if line.startswith("  "))
    for line in output.splitlines():
        if line.startswith(args.function + " "):
            status = line.split(None, 1)[1].strip()
            if args.score and status != "MATCH":
                status += f", {differing} differing lines"
            return status
    return "compile failed"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", type=Path)
    parser.add_argument("function")
    parser.add_argument("variants", type=Path, nargs="+", help="files of variants")
    parser.add_argument("--keep", type=int, help="the variant to keep, by its index, whether or not it matches")
    parser.add_argument("--version", default="b2_us", help="game version")
    parser.add_argument("--compiler", help="the compiler the file is built with by default")
    parser.add_argument("--extra-flags", default="", help="flags added to the compiler's")
    parser.add_argument("--score", action="store_true",
                        help="also count the differing lines of each variant's aligned diff, which tells how close "
                        "variants of the same size are")
    args = parser.parse_args()

    if args.compiler is None:
        lib = lib_compiler(args.source)
        args.compiler = lib[0] if lib else MWCC_VERSION.removeprefix("dsi/")
    original = args.source.read_text()
    variants = [v.strip() for path in args.variants for v in path.read_text().split("=====") if v.strip()]

    if args.keep is not None:
        args.source.write_text(with_variant(original, args.function, variants[args.keep]))
        print(f"variant {args.keep}: {probe(args)} (kept)")
        return
    for i, variant in enumerate(variants):
        args.source.write_text(with_variant(original, args.function, variant))
        status = probe(args)
        print(f"variant {i}: {status}", flush=True)
        if status == "MATCH":
            return
    args.source.write_text(original)


if __name__ == "__main__":
    main()
