#!/usr/bin/env python3
"""Runs every test_*.py of port/tests/asm_oracle, each in its own process, and summarizes.

    build/port-venv/bin/python port/tests/asm_oracle/run_all.py [--version b2_us|w2_us] [-k TEXT]

Exits 0 when every test file passes, 1 when one fails (or crashes), and 77 (skipped) when the oracle can't run here:
the ROM isn't extracted in extract/<version> (CI has no ROM), Unicorn isn't installed, which means this isn't the
oracle's virtual environment (README.md, "Setup"), the host build it takes its flags from (ASM_ORACLE_BUILD_DIR,
else build/port) wasn't made with the Ninja generator, or, on macOS, that build has no slice for this Python's
architecture (an x86_64 build on Apple Silicon): the tests load the host library into this process, so they check the
slice this Python runs. A test file exits 77 itself to skip.
"""

import argparse
import os
import platform
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from harness import VERSIONS, missing_reason  # noqa: E402

SKIP = 77
TIMEOUT = 1800


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", choices=VERSIONS, default=os.environ.get("ASM_ORACLE_VERSION", "b2_us"))
    parser.add_argument("-k", dest="pattern", default="", help="run only the test files whose name contains TEXT")
    args = parser.parse_args()

    try:
        import unicorn  # noqa: F401
    except ImportError:
        print("asm_oracle: skipped: Unicorn isn't installed for this Python; run with build/port-venv/bin/python "
              "after python3 -m venv build/port-venv && build/port-venv/bin/pip install -r "
              "port/tests/asm_oracle/requirements.txt")
        sys.exit(SKIP)
    reason = missing_reason(args.version)
    if reason:
        print(f"asm_oracle: skipped: {reason}")
        sys.exit(SKIP)
    # The tests compile with the host build's flags, which build.py reads from its build.ninja
    build_dir = Path(os.environ.get("ASM_ORACLE_BUILD_DIR", HERE.parents[2] / "build" / "port"))
    if not (build_dir / "build.ninja").exists():
        print(f"asm_oracle: skipped: {build_dir} isn't a host build made with the Ninja generator")
        sys.exit(SKIP)
    # On macOS, the architectures the build is for (CMAKE_OSX_ARCHITECTURES; none set, this Mac's)
    cache = build_dir / "CMakeCache.txt"
    text = cache.read_text() if cache.exists() else ""
    found = re.search(r"^CMAKE_OSX_ARCHITECTURES:\w+=(.*)$", text, re.MULTILINE)
    archs = found.group(1).split(";") if found and found.group(1).strip() else []
    if sys.platform == "darwin" and archs and platform.machine() not in archs:
        print(f"asm_oracle: skipped: {build_dir} is built for {' '.join(archs)}, and this Python runs "
              f"{platform.machine()}: the tests load the host library into this process")
        sys.exit(SKIP)

    tests = sorted(p for p in HERE.glob("test_*.py") if args.pattern in p.name)
    if not tests:
        print(f"asm_oracle: no test file matches {args.pattern!r}")
        sys.exit(1)
    environment = dict(os.environ, ASM_ORACLE_VERSION=args.version, PYTHONDONTWRITEBYTECODE="1")
    passed, failed, skipped = [], [], []
    for test in tests:
        print(f"== {test.name} ({args.version})", flush=True)
        started = time.monotonic()
        try:
            result = subprocess.run([sys.executable, str(test)], cwd=HERE, env=environment, timeout=TIMEOUT)
            code = result.returncode
        except subprocess.TimeoutExpired:
            print(f"{test.name}: timed out after {TIMEOUT} s")
            code = 1
        elapsed = time.monotonic() - started
        if code == 0:
            passed.append(test.name)
        elif code == SKIP:
            skipped.append(test.name)
        else:
            if code < 0:
                print(f"{test.name}: killed by signal {-code} (a crash in the host library?)")
            failed.append(test.name)
        print(f"== {test.name}: {'passed' if code == 0 else 'skipped' if code == SKIP else 'FAILED'} "
              f"in {elapsed:.1f} s", flush=True)

    print(f"asm_oracle: {len(passed)} passed, {len(failed)} failed, {len(skipped)} skipped"
          + (f" (failed: {', '.join(failed)})" if failed else ""))
    if failed:
        sys.exit(1)
    sys.exit(SKIP if skipped and not passed else 0)


if __name__ == "__main__":
    main()
