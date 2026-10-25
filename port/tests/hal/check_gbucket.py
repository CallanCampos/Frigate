"""Require GameSpy's absent bucket-set functions to stop, not implement substitute behavior.

Runs test_gbucket.c in a subprocess, including FreeBucketSet(NULL), so every host's
CTest can check the exact trap status and diagnostic without WILL_FAIL inversion.
"""
import subprocess
import sys


result = subprocess.run([sys.argv[1], sys.argv[2]], capture_output=True, text=True, timeout=10)
if result.returncode != 70 or sys.argv[2] not in result.stderr or 'not in ROM' not in result.stderr:
    sys.exit(f'{sys.argv[2]}: expected exit 70 and named not-in-ROM diagnostic; '
             f'got {result.returncode}, stdout={result.stdout!r}, stderr={result.stderr!r}')
