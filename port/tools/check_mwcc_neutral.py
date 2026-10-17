#!/usr/bin/env python3
"""Proves that the port's changes outside port/ leave what MWCC compiles unchanged, without base ROMs.

    check_mwcc_neutral.py [--base REV] [--jobs N] [FILE...]

For every C file of src/ and lib/ (or the given ones), and for both versions, it runs MWCC's preprocessor (-EP, the
same compiler, flags, defines and include paths as the ROM build) on the working tree and on a checkout of BASE
(default: the upstream commit last merged, `git merge-base HEAD upstream/main`), and compares the outputs with
only the spacing between tokens ignored. MWCC never defines PLATFORM_PC, so a change that only adds `#ifdef PLATFORM_PC` branches leaves
every file's preprocessed text identical: the compiler gets the same input, so it produces the same object, in complete
and incomplete files alike. That proves a guard-only change neutral for the whole tree at once
(port/docs/game-changes.md). Any difference is printed with its first differing lines, and the exit status is 1.

The checkout of BASE is a temporary git worktree, removed at the end. Both trees use this tree's generated headers
(build/include/generated), which the change doesn't touch.
"""
import argparse
import os
import re
import shlex
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from configure import CC_FLAGS, MWCC_VERSION, VERSIONS, library_of, mwcc_command  # noqa: E402
from tools.decomp.compile_all import generate_constants  # noqa: E402


# C tokens: string and character literals whole (their spaces matter), identifiers and numbers, and single punctuation
# characters. Comparing token streams ignores only the spacing between tokens, which the compiler ignores too
TOKEN = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_][A-Za-z0-9_]*|[0-9][A-Za-z0-9_.]*|\S')


def preprocess(root: Path, source: str, version: str, generated: Path) -> tuple[int, str]:
    """MWCC's preprocessed text of one file in one tree as its tokens, one per line, or its error output."""
    library = library_of(ROOT / source)
    compiler, flags = library if library else (MWCC_VERSION, CC_FLAGS)
    include_dirs = ["include", *(p.relative_to(root).as_posix() for p in sorted((root / "lib").glob("*/include")))]
    env = os.environ.copy()
    if library:
        env["MWCIncludes"] = "."
    command = [
        *mwcc_command(ROOT / "tools" / "mwccarm" / compiler / "mwccarm.exe", ROOT / "tools"),
        *(arg for flag in flags if flag not in ("-nolink", "-sym on") for arg in shlex.split(flag)),
        *(arg for define in VERSIONS[version]["defines"] for arg in ("-d", define)),
        *(arg for d in include_dirs for arg in ("-i", d)),
        "-i", str(generated),
        "-EP", source,
    ]
    proc = subprocess.run(command, cwd=root, env=env, capture_output=True, text=True, errors="replace")
    if proc.returncode != 0:
        return proc.returncode, proc.stdout + proc.stderr
    # Each tree's own path appears in #line directives and in __FILE__ strings, as wibo's Windows path
    text = proc.stdout.replace("Z:" + str(root).replace("/", "\\\\"), "<root>")
    tokens = (token for line in text.splitlines() if not line.startswith("#line") for token in TOKEN.findall(line))
    return 0, "\n".join(tokens)


def compare(base: Path, source: str, version: str, generated: Path) -> str | None:
    """None when both trees preprocess to the same text; else a short description of the difference."""
    if not (base / source).exists():
        return f"{source} ({version}): not in the base tree"
    head_status, head = preprocess(ROOT, source, version, generated)
    base_status, old = preprocess(base, source, version, generated)
    if head_status or base_status:
        return f"{source} ({version}): the preprocessor failed: {(head if head_status else old)[:400]}"
    if head == old:
        return None
    old_tokens, head_tokens = old.splitlines(), head.splitlines()
    first = next((i for i, (a, b) in enumerate(zip(old_tokens, head_tokens)) if a != b),
                 min(len(old_tokens), len(head_tokens)))
    context = slice(max(first - 8, 0), first + 8)
    return (f"{source} ({version}): the token streams differ at token {first}:\n"
            f"  base:         {' '.join(old_tokens[context])}\n  working tree: {' '.join(head_tokens[context])}")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--base", help="the revision to compare with, default the last merged upstream commit")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    parser.add_argument("files", nargs="*", help="C files to check, default every one of src/ and lib/")
    args = parser.parse_args()

    if args.base is None:
        args.base = subprocess.run(["git", "merge-base", "HEAD", "upstream/main"], cwd=ROOT, capture_output=True,
                                   text=True, check=True).stdout.strip()
    generate_constants()
    generated = ROOT / "build" / "include" / "generated"
    sources = args.files or sorted(p.relative_to(ROOT).as_posix() for pattern in ("src/**/*.c", "lib/*/src/**/*.c")
                                   for p in ROOT.glob(pattern))
    with tempfile.TemporaryDirectory(prefix="mwcc-neutral-") as tmp:
        base = Path(tmp) / "base"
        subprocess.run(["git", "worktree", "add", "--detach", "--quiet", str(base), args.base], cwd=ROOT, check=True)
        try:
            with ThreadPoolExecutor(args.jobs) as pool:
                jobs = [pool.submit(compare, base, source, version, generated)
                        for source in sources for version in VERSIONS]
                problems = [problem for job in jobs if (problem := job.result())]
        finally:
            subprocess.run(["git", "worktree", "remove", "--force", str(base)], cwd=ROOT, check=False)
    for problem in problems:
        print(problem)
    print(f"check_mwcc_neutral: {len(sources) * len(VERSIONS) - len(problems)} of {len(sources) * len(VERSIONS)} "
          f"preprocess identically to {args.base}")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
