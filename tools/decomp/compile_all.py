#!/usr/bin/env python3
"""Compile every C file in src/ and lib/ for each version, with the compiler and flags configure.py gives it, and
report compiler diagnostics, including warnings on successful compilations. Only a nonzero compiler exit status
counts as failure. It needs no base ROM, so CI runs it on every push and pull request; matching is still checked
by `ninja` with the ROMs.

    compile_all.py                  # every file, both versions
    compile_all.py --version b2_us  # one version
"""
import argparse
import os
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from configure import (GENERATED_INCLUDE_DIR, INCLUDE_DIRS, ROM_BLOBS, TEXT_ARCHIVES,  # noqa: E402
                       VERSIONS, download_tools, overlay_digest_includes, source_compiler, mwcc_command)
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.build import overlay_digests, rom_blob  # noqa: E402
from tools.data.gen_constants import generate  # noqa: E402
from tools.text import pms_words, text_ids  # noqa: E402

TOOLS = ROOT / "tools"


@dataclass(frozen=True)
class CompileResult:
    source: Path
    version: str
    returncode: int
    stdout: str
    stderr: str


def compile_file(source: Path, version: str, out_dir: Path) -> CompileResult:
    """Compiles one file, retaining its exit status and both diagnostic streams."""
    settings = source_compiler(source)
    compiler, flags = settings.compiler, settings.flags
    env = os.environ.copy()
    if settings.is_library:
        # Match configure.py's library rules: older compilers require this system include path.
        env["MWCIncludes"] = "."
    obj = out_dir / version / source.with_suffix(".o")
    obj.parent.mkdir(parents=True, exist_ok=True)
    command = [
        *mwcc_command(TOOLS / "mwccarm" / compiler / "mwccarm.exe"),
        *(arg for flag in flags for arg in shlex.split(flag)),
        *(arg for define in VERSIONS[version]["defines"] for arg in ("-d", define)),
        *(arg for d in INCLUDE_DIRS for arg in ("-i", d)),
        "-o", str(obj),
        str(source),
    ]
    proc = subprocess.run(command, cwd=ROOT, env=env, capture_output=True, text=True)
    return CompileResult(source, version, proc.returncode, proc.stdout, proc.stderr)


def write_header(header: Path, text: str):
    header.parent.mkdir(parents=True, exist_ok=True)
    if not header.exists() or header.read_text() != text:
        header.write_text(text)


def generate_constants():
    """Writes the headers that ninja generates, since the sources include them: the constants of generated/,
    each message file's message IDs, the easy chat words, the embedded binaries and the overlay digests, which are
    zeros without an extracted ROM."""
    out_dir = ROOT / GENERATED_INCLUDE_DIR
    for source in sorted((ROOT / "generated").glob("*.txt")):
        write_header(out_dir / "constants" / source.with_suffix(".h").name, generate(source))
    for source_dir in TEXT_ARCHIVES.values():
        for source in sorted((ROOT / source_dir).glob("*.txt")):
            write_header(out_dir / "text" / source.parent.name / source.with_suffix(".h").name, text_ids.header(source))
    write_header(out_dir / "constants" / "pms_words.h", pms_words.header())
    for include, (module, config_dir, name) in ROM_BLOBS.items():
        # From any extracted version, as ninja takes it from the first one it builds. Without one, zeros, which never
        # replace the bytes that ninja wrote
        version = next((v for v in VERSIONS if (ROOT / "extract" / v / module).exists()), None)
        if version is None and (out_dir / include).exists():
            continue
        version = version or next(iter(VERSIONS))
        data = rom_blob.blob(ROOT / "extract" / version / module,
                             ROOT / "config" / version / "arm9" / config_dir / "symbols.txt", name, allow_missing=True)
        write_header(out_dir / include, rom_blob.initializer(data))
    for version in VERSIONS:
        # From the version's base ROM, as ninja computes them. Without one, zero digests, one for each overlay of the
        # config, which never replace those that ninja wrote
        table_include, overlays_include = (out_dir / include for include in overlay_digest_includes(version))
        rom = ROOT / "orig" / f"baserom_{version}.nds"
        key = ROOT / "extract" / version / "arm9" / "hmac_sha1_key.bin"
        if rom.exists() and key.exists():
            table, overlays = overlay_digests.digests(rom.read_bytes(), key.read_bytes())
        elif overlays_include.exists():
            continue
        else:
            count = sum(1 for _ in (ROOT / "config" / version / "arm9" / "overlays").iterdir())
            table, overlays = bytes(overlay_digests.DIGEST_SIZE), [bytes(overlay_digests.DIGEST_SIZE)] * count
        write_header(table_include, overlay_digests.initializer(table))
        write_header(overlays_include, "".join(overlay_digests.initializer(digest) for digest in overlays))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", choices=VERSIONS, action="append", help="version to compile, default every one")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count(), help="files to compile at once")
    args = parser.parse_args()

    download_tools(TOOLS)
    generate_constants()
    sources = sorted(p.relative_to(ROOT) for pattern in ("src/**/*.c", "lib/*/src/**/*.c") for p in ROOT.glob(pattern))
    versions = args.version or list(VERSIONS)
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(args.jobs) as pool:
        jobs = [pool.submit(compile_file, source, version, Path(tmp)) for version in versions for source in sources]
        results = [job.result() for job in jobs]
    failures = 0
    for result in results:
        for output, stream in ((result.stdout, sys.stdout), (result.stderr, sys.stderr)):
            if output:
                print(f"{result.source} ({result.version}):", file=stream)
                print(output, end="" if output.endswith("\n") else "\n", file=stream)
        if result.returncode != 0:
            failures += 1
            if not result.stdout and not result.stderr:
                print(f"{result.source} ({result.version}): compiler exited with status {result.returncode}",
                      file=sys.stderr)
    print(f"{len(sources) * len(versions) - failures} of {len(sources) * len(versions)} compiled "
          f"({len(sources)} files, {', '.join(versions)})")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
