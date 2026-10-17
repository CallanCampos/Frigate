#!/usr/bin/env python3
"""Runs every test_*.py of port/tests/asm_oracle, each in its own process, and summarizes.

    build/port-venv/bin/python port/tests/asm_oracle/run_all.py [--version b2_us|w2_us] [-k TEXT]

Exits 0 when every test file passes, 1 when one fails (or crashes), and 77 (skipped) when the oracle can't run here:
the ROM isn't extracted in extract/<version> (CI has no ROM), or Unicorn isn't installed, which means this isn't the
oracle's virtual environment (README.md, "Setup"). A test file exits 77 itself to skip.
"""

import argparse
import os
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
