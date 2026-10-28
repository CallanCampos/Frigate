"""Require AuthService's absent gsCryptRSAEncryptBuffer to fail closed through its actual header ABI.

The test_gscrypt.c child uses NULL or normal buffers; a returned error/no-op, success, signal or unrelated exit fails.
"""
import subprocess
import sys

result = subprocess.run([sys.argv[1], sys.argv[2]], capture_output=True, text=True, timeout=10)
if (result.returncode != 70 or 'gsCryptRSAEncryptBuffer' not in result.stderr
        or 'not in ROM' not in result.stderr or 'function returned' in result.stderr):
    sys.exit(f'{sys.argv[2]}: expected exit 70 and named not-in-ROM diagnostic; '
             f'got {result.returncode}, stdout={result.stdout!r}, stderr={result.stderr!r}')
