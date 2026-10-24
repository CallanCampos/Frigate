#!/usr/bin/env python3
"""The test of the port-readiness measurement's compile step (port/tools/progress/compile_check.py, the test
progress_compile_check of port/cmake/progress.cmake). It touches no game functionality: it holds the step to the
repository's own tools, which upstream changes, so that a sync that changes them fails here rather than in
sync_upstream.sh --metrics. Its two checks:

- the settings: for a game file with the build's flags (src/system/printsys.c), one with config/compiler_flags.toml's
  override (src/ov257/ctvt_draw.c, -ipa function), and two libraries with their own compilers (lib/spl, 1.2/base;
  lib/nnsys, 2.0/sp2), the step compiles with the compiler configure.py's source_compiler gives the build, and with
  every one of the build's flags but its debug information (-sym on), as tools/decomp/compiler_probe.py's
  probe_defaults gives them, with -w off;
- the measure: compile_check.py, run as run_all.sh runs it, on those files and on src/system/wipe_sub.c, which
  config/deadstrip.toml lists, in the build's version. Each is complete, so its object must hold the functions that
  the version's symbols.txt has in the file's .text range, as many and as many bytes, and its .rodata the range's
  size; for wipe_sub.c that holds only once the 31 handlers the original link dropped are stripped.

    test_compile_check.py REPO VERSION

It skips (77) without the decomp's Python environment (pyelftools, capstone and PyYAML, which compiler_probe.py
imports), and after the settings' check without what compiling needs: wibo, sjiswrap and the compilers in tools/
(configure.py downloads wibo and sjiswrap; tools/mwccarm comes from the decomp checkout), or on Windows, which runs
no wibo. It writes only in a temporary directory.
"""

from __future__ import annotations

import csv
import importlib.util
import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

TOOL = Path(__file__).resolve().parents[2] / "tools" / "progress" / "compile_check.py"
SKIP = 77

# The files the step is checked on, and why each
SETTINGS_FILES = {
    "src/system/printsys.c": "a game file with the build's flags",
    "src/ov257/ctvt_draw.c": "a game file with config/compiler_flags.toml's -ipa function",
    "lib/spl/src/spl_emit.c": "a library with its own compiler, 1.2/base",
    "lib/nnsys/src/g2d/g2d_RendererCore.c": "a library with its own compiler, 2.0/sp2",
}
DEAD_STRIPPED = "src/system/wipe_sub.c"

failures: list[str] = []


def check(condition: bool, message: str) -> None:
    if not condition:
        failures.append(message)


def has_option(words: list[str], flag: str) -> bool:
    """Whether the flag's words (one option and its value, such as -ipa function) follow one another in words."""
    option = shlex.split(flag)
    return any(words[i:i + len(option)] == option for i in range(len(words)))


def rom_functions(repo: Path, version: str, path: str) -> tuple[int, int] | None:
    """The count and bytes of the functions that the version's symbols.txt has in the file's .text range."""
    from progress import parse_delinks, parse_symbols

    for delinks in (repo / "config" / version / "arm9").rglob("delinks.txt"):
        _, files = parse_delinks(delinks.read_text())
        for entry in files:
            if entry["path"] == path:
                start, end = entry["sections"][".text"]
                functions, _ = parse_symbols((delinks.parent / "symbols.txt").read_text())
                inside = [f for f in functions if start <= f["addr"] < end]
                return len(inside), sum(f["size"] for f in inside)
    return None


def check_settings(tools) -> None:
    """The compiler and flags the step uses against the build's own."""
    for path, what in SETTINGS_FILES.items():
        source = tools.root / path
        build = tools.configure.source_compiler(source)
        compiler, flags = tools.settings(source)
        check(compiler == build.compiler, f"{path} ({what}): compiled with {compiler}, the build uses {build.compiler}")
        words = shlex.split(flags)
        for flag in build.flags:
            if flag != "-sym on":
                check(has_option(words, flag),
                      f"{path} ({what}): the build's {flag!r} is missing from the step's flags: {flags}")
        check(has_option(words, "-w off"), f"{path} ({what}): the probe's -w off is missing: {flags}")
        command = tools.command(source, "b2_us", Path("out.o"))
        check(command[-1] == str(source) and "-o" in command,
              f"{path} ({what}): the command doesn't compile the file to the object: {command}")
    override = shlex.split(tools.settings(tools.root / "src/ov257/ctvt_draw.c")[1])
    check(override.count("-ipa") == 1 and has_option(override, "-ipa function"),
          f"src/ov257/ctvt_draw.c: config/compiler_flags.toml's -ipa function doesn't replace the default: {override}")


def compilers_missing(tools) -> list[str]:
    """What compiling the files needs and this checkout lacks."""
    needed = [tools.probe.TOOLS / "wibo", tools.probe.TOOLS / "sjiswrap.exe"]
    for path in [*SETTINGS_FILES, DEAD_STRIPPED]:
        compiler, _ = tools.settings(tools.root / path)
        needed.append(tools.probe.TOOLS / "mwccarm" / compiler / "mwccarm.exe")
    return [str(path.relative_to(tools.root)) for path in dict.fromkeys(needed) if not path.exists()]


def check_measure(repo: Path, version: str) -> None:
    """The step's measure of each file against the ROM's functions in the file's range."""
    files = [*SETTINGS_FILES, DEAD_STRIPPED]
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "compile.tsv"
        proc = subprocess.run([sys.executable, str(TOOL), "--root", str(repo), "--version", version, "--out", str(out),
                               *files], capture_output=True, text=True)
        if proc.returncode != 0:
            failures.append(f"compile_check.py failed (status {proc.returncode}):\n{proc.stdout}{proc.stderr}")
            return
        rows = {row["path"]: row for row in csv.DictReader(out.open(), delimiter="\t")}
    for path in files:
        row = rows.get(path)
        expected = rom_functions(repo, version, path)
        if row is None or expected is None:
            failures.append(f"{path}: not in config/{version}'s delinks.txt any more; give the test another complete "
                            "file of its kind")
            continue
        check(row["complete"] == "True", f"{path}: no longer complete in {version}; give the test another file")
        check(row["status"] == "ok", f"{path}: {row['status']}")
        if row["status"] != "ok":
            continue
        count, size = expected
        check(int(row["c_funcs"]) == count, f"{path}: {row['c_funcs']} functions, the ROM has {count} in its range")
        check(int(row["c.text"]) == size, f"{path}: {row['c.text']} bytes of code, the ROM's functions have {size}")
        check(row["c.rodata"] == row["rom.rodata"], f"{path}: .rodata {row['c.rodata']}, the ROM {row['rom.rodata']}")


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    repo, version = Path(sys.argv[1]).resolve(), sys.argv[2]
    missing = [module for module in ("elftools", "capstone", "yaml") if importlib.util.find_spec(module) is None]
    if missing:
        print(f"skipped: this Python ({sys.executable}) lacks {', '.join(missing)}: the decomp's .venv has them")
        return SKIP
    sys.path.insert(0, str(TOOL.parent))
    try:
        import compile_check

        tools = compile_check.Tools(repo)
        check(DEAD_STRIPPED in tools.deadstrip.load_manifest(),
              f"{DEAD_STRIPPED} is no longer in config/deadstrip.toml; give the test another file it lists")
        check_settings(tools)
    except (AttributeError, ImportError, TypeError) as error:
        # What the step calls is gone or changed in the repository's tools: compile_check.py follows their API
        print(f"compile_check.py no longer fits the repository's tools (tools/decomp/compiler_probe.py, configure.py, "
              f"tools/build/deadstrip.py): {type(error).__name__}: {error}")
        return 1
    if failures:
        print("\n".join(failures))
        return 1
    print(f"the settings of {len(SETTINGS_FILES)} files are the build's")
    lacking = compilers_missing(tools)
    if os.name == "nt" or lacking:
        print("skipped the measure: " + ("Windows runs no wibo" if os.name == "nt" else "no " + ", ".join(lacking)))
        return SKIP
    check_measure(repo, version)
    if failures:
        print("\n".join(failures))
        return 1
    print(f"the measure of {len(SETTINGS_FILES) + 1} files in {version} is the ROM's")
    return 0


if __name__ == "__main__":
    sys.exit(main())
