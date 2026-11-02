#!/usr/bin/env python3
"""gen_stubs.py's check that the game's objects call MSL's exit, not the host C library's (port/tools/gen_stubs.py,
MSL_OWN), on three objects this test compiles with the build's compiler: no ROM and none of the build's objects.

MSL's exit, the DS's, only notes a failure and returns, and the game's heap check (GFL_HeapDumpOnFailure,
src/gfl/heap.c) relies on it to go on to OS_Terminate. The host's exit ends the program at once, with status 1 and no
"port:" line, as it did when the map sweep's CONTINUE into Castelia City's café ran the field heap out
(port/docs/STATUS.md, 2026-11-02). port/cmake/msl.cmake forces port/hal/msl/exit_calls.h into heap.c, which sends its
calls to MSL's, and gen_stubs.py, which runs at every build, fails it when another game object calls the host's:
  - a game file that calls exit as heap.c does, declared as nitro/os.h declares it, fails the generation, which names
    the object, the function and port/cmake/msl.cmake, and writes no stubs;
  - the same file with exit_calls.h first, as msl.cmake compiles heap.c, calls port_msl_exit and passes, and the
    summary of what the host's C library provides for the game doesn't list exit;
  - a file of the platform layer that calls the host's exit, as the program does to end, passes.

    test_gen_stubs.py --cc COMPILER --version VERSION --work DIRECTORY [-- GEN_STUBS_OPTION...]

The options after -- go to gen_stubs.py as port/CMakeLists.txt gives them (--nm and --compiler, off macOS).
"""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GEN_STUBS = ROOT / "port" / "tools" / "gen_stubs.py"
EXIT_CALLS = ROOT / "port" / "hal" / "msl" / "exit_calls.h"

# A game file's heap check: exit declared as lib/nitro/include/nitro/os.h declares it, then called twice
GAME = """void exit(int status);

void TestHeapCheck(void) {
    exit(1);
    exit(0);
}
"""
# The platform layer's end of the program, through the host's <stdlib.h>
PORT = """#include <stdlib.h>

void TestPortEnd(void) {
    exit(0);
}
"""


def compile_object(cc: str, work: Path, name: str, text: str) -> str:
    source = work / f"{name}.c"
    source.write_text(text)
    obj = work / f"{name}.o"
    result = subprocess.run([cc, "-c", str(source), "-o", str(obj)], capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"test_gen_stubs: {cc} didn't compile {source}:\n{result.stderr}")
    return str(obj)


def gen_stubs(work: Path, version: str, options: list[str], objects: list[str], game: list[str], name: str):
    game_file = work / f"{name}_game_objects.txt"
    game_file.write_text("".join(f"{path}\n" for path in game))
    out = work / f"{name}_stubs.c"
    if out.exists():
        out.unlink()
    result = subprocess.run([sys.executable, str(GEN_STUBS), "--version", version, "--out", str(out), "--objects",
                             *objects, "--game-objects-file", str(game_file), *options], capture_output=True,
                            text=True, cwd=ROOT)
    return result, out.exists()


def main():
    split = sys.argv.index("--") if "--" in sys.argv else len(sys.argv)
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--version", required=True, choices=["b2_us", "w2_us"])
    parser.add_argument("--work", type=Path, required=True)
    args = parser.parse_args(sys.argv[1:split])
    options = sys.argv[split + 1:]
    args.work.mkdir(parents=True, exist_ok=True)

    game = compile_object(args.cc, args.work, "game_heap_check", GAME)
    renamed = compile_object(args.cc, args.work, "game_heap_check_msl", f'#include "{EXIT_CALLS.as_posix()}"\n{GAME}')
    port = compile_object(args.cc, args.work, "port_end", PORT)
    failures = []

    result, wrote = gen_stubs(args.work, args.version, options, [game, port], [game], "host_exit")
    message = result.stderr.strip().splitlines()[-1] if result.stderr.strip() else ""
    if (result.returncode == 0 or wrote or os.path.basename(game) not in message or
            "the host C library's exit" not in message or "port/cmake/msl.cmake" not in message):
        failures.append(f"a game object that calls exit: status {result.returncode}, stubs written: {wrote}, "
                        f"last line: {message or '(none)'}")
    else:
        print(f"a game object that calls exit fails the generation: {message}")

    result, wrote = gen_stubs(args.work, args.version, options, [renamed, port], [renamed], "msl_exit")
    provided = [line for line in result.stderr.splitlines() if "the host's C library provides" in line]
    if result.returncode != 0 or not wrote or not provided or "exit" in provided[0].split(": ", 2)[-1].split(", "):
        failures.append(f"the same file with exit_calls.h first: status {result.returncode}, stubs written: {wrote}, "
                        f"output: {result.stderr.strip()}")
    else:
        print("the same file with exit_calls.h first passes, and the host's exit isn't among what it provides the game")

    result, wrote = gen_stubs(args.work, args.version, options, [port], [], "port_exit")
    if result.returncode != 0 or not wrote:
        failures.append(f"a port object that calls exit: status {result.returncode}, output: {result.stderr.strip()}")
    else:
        print("a port object that calls the host's exit passes")

    for failure in failures:
        print(f"FAIL: {failure}")
    print("test_gen_stubs: " + ("FAIL" if failures else "ok"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
