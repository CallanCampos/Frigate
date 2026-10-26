"""Check the actual src/system/vm.c dispatch used by field, trainer AI, battle-effects and musical scripts.

Prove R13's exception leaves native/verifier calls checked, or explicitly skip without compiler support.
"""
import subprocess
import sys


def run(mode):
    return subprocess.run([sys.argv[1], mode], capture_output=True, text=True, timeout=15)


def function_error(result):
    return (result.returncode != 0 and 'runtime error: call to function' in result.stderr
            and 'incorrect function type' in result.stderr)


probe = run('capability')
if probe.returncode == 0:
    print('compiler emits no C function sanitizer check; negative dispatch test skipped')
    sys.exit(77)
if not function_error(probe):
    sys.exit(f'capability probe failed for another reason: {probe.returncode}\n{probe.stdout}{probe.stderr}')
mode = sys.argv[2]
result = run(mode)
if not function_error(result) or 'vm.c:' not in result.stderr:
    sys.exit(f'{mode}: expected VM function-type check, got {result.returncode}\n{result.stdout}{result.stderr}')
print(f'{mode}: VM function-type check remains active')
