#!/usr/bin/env python3
"""The pointer cast guard's own test (port/tools/check_ptrcasts.py, the test game_ptrcasts_tool of
port/cmake/game.cmake). It touches no game functionality: it builds a small tree of its own under WORKDIR, a src/ and
lib/ with a few casts and a compile database for CC, and checks what the guard says about it: a new finding fails with
its site and the two ways out, --update writes rows to classify, a classified baseline passes, a count above the
baseline fails, a renamed function passes as a rename, a finding gone passes with a note, a narrowed uptr counts where a
narrowed unsigned long or a getter's result as it is stored doesn't, a function pointer cast that pairs a pointer or a
uptr with an int counts where one between pointers doesn't, a host variant of a header (gen/hwinclude) is reported as
the header it was made from, a file that doesn't compile fails, and a build without a compile database skips (77).

    test_check_ptrcasts.py CC WORKDIR

It skips (77) when CC isn't clang, whose warnings the guard reads. Standard library only.
"""

from __future__ import annotations

import json
import shutil
import subprocess
import sys
from pathlib import Path

TOOL = Path(__file__).resolve().parents[2] / "tools" / "check_ptrcasts.py"

TYPES = """\
typedef unsigned int u32;
typedef __UINTPTR_TYPE__ uptr;
typedef struct Task Task;
"""

FIRST = TYPES + """
u32 TakeAddress(void *work) {
    return (u32)work;
}

Task *GiveTask(u32 value) {
    return (Task *)value;
}
"""

failures = []


def check(condition: bool, what: str, output: str = ""):
    if not condition:
        failures.append(what)
        print(f"FAIL: {what}")
        if output:
            print("    " + output.replace("\n", "\n    "))


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    cc, work = sys.argv[1], Path(sys.argv[2]).resolve() / "ptrcasts"
    version = subprocess.run([cc, "--version"], capture_output=True, text=True)
    if "clang" not in version.stdout.lower():
        print(f"skipped: {cc} isn't clang")
        return 77
    if work.exists():
        shutil.rmtree(work)
    (work / "src").mkdir(parents=True)
    (work / "lib" / "x" / "include").mkdir(parents=True)
    (work / "gen" / "hwinclude").mkdir(parents=True)
    baseline = work / "baseline.tsv"

    def write(name: str, text: str):
        (work / name).write_text(text)

    def database(*files: str):
        entries = [{"directory": str(work), "file": f,
                    "command": f"{cc} -std=gnu99 -I gen/hwinclude -c {f} -o {Path(f).stem}.o"} for f in files]
        (work / "compile_commands.json").write_text(json.dumps(entries))

    def run(*extra: str) -> tuple[int, str]:
        proc = subprocess.run([sys.executable, str(TOOL), "--root", str(work), "--build-dir", str(work), "--baseline",
                               str(baseline), "--jobs", "2", *extra], capture_output=True, text=True)
        return proc.returncode, proc.stdout + proc.stderr

    def classify_all(cls: str):
        lines = baseline.read_text().splitlines()
        out = []
        for line in lines:
            fields = line.split("\t")
            if len(fields) == 7 and fields[5] == "new":
                fields[5], fields[6] = cls, "the test's classification"
            out.append("\t".join(fields))
        baseline.write_text("\n".join(out) + "\n")

    # No compile database: skip
    status, output = run()
    check(status == 77, "a build without compile_commands.json skips", output)

    # Two new findings, no baseline: fail, naming each site and the ways out
    write("src/a.c", FIRST)
    database("src/a.c")
    status, output = run()
    check(status == 1, "new findings fail", output)
    check("new: src/a.c in TakeAddress: ptr-to-int (u32 <- void *)" in output, "the pointer cut is named", output)
    check("new: src/a.c in GiveTask: int-to-ptr (Task * <- u32)" in output, "the pointer made is named", output)
    check("src/a.c:6  return (u32)work;" in output, "the site's line and source are shown", output)
    check("uptr or sptr" in output and "game-changes.md" in output and "--update" in output,
          "the failure says the two ways out", output)

    # --update writes the rows as new, which still fails until they are classified
    status, output = run("--update")
    check(status == 1 and "2 new to classify" in output, "--update writes unclassified rows, which fail", output)
    classify_all("harmless")
    status, output = run()
    check(status == 0 and "2 sites in 2 keys (real 0, harmless 2" in output, "a classified baseline passes", output)

    # A second cast of the same kind in the same function: a count above the baseline fails
    write("src/a.c", FIRST.replace("return (u32)work;", "return (u32)work + (u32)(void *)((char *)work + 1);"))
    status, output = run()
    check(status == 1 and "2 sites, the baseline allows 1" in output, "a count above the baseline fails", output)

    # The function renamed upstream: a rename, which passes with a note
    write("src/a.c", FIRST.replace("TakeAddress", "TakeWork"))
    status, output = run()
    check(status == 0 and "renamed" in output and "in TakeWork" in output, "a renamed function passes", output)
    status, output = run("--update")
    check(status == 0 and "TakeWork" in baseline.read_text() and "TakeAddress" not in baseline.read_text(),
          "--update renames the row and keeps its class", output)

    # A finding gone: passes, with what --update would drop
    write("src/a.c", FIRST.replace("TakeAddress", "TakeWork").split("Task *GiveTask")[0])
    status, output = run()
    check(status == 0 and "no longer found" in output and "GiveTask" in output, "a finding gone passes", output)
    run("--update")

    # A narrowed uptr counts, a narrowed unsigned long doesn't, nor does a getter's result narrowed as it is stored
    write("src/a.c", TYPES + """
uptr GetField(int id);

u32 KeepAddress(uptr address) {
    return address;
}

u32 KeepNumber(unsigned long number) {
    return number;
}

typedef struct Fields {
    uptr (*get)(int id);
} Fields;

u32 KeepField(const Fields *fields) {
    u32 field = (GetField(1));

    return field ? fields->get(2) : 0;
}

u32 KeepFieldSum(void) {
    return GetField(1) + 1;
}

static const uptr sAddresses[2];

u32 KeepConstAddress(void) {
    return sAddresses[1];
}
""")
    status, output = run()
    check(status == 1 and "KeepAddress: narrow (u32 <- uptr)" in output, "a narrowed uptr counts", output)
    check("KeepNumber" not in output, "a narrowed unsigned long doesn't count", output)
    check("KeepField:" not in output, "a getter's result narrowed as it is stored doesn't count", output)
    check("KeepFieldSum: narrow (u32 <- uptr)" in output, "arithmetic on a getter's result counts", output)
    check("KeepConstAddress: narrow (u32 <- const uptr)" in output, "a narrowed const uptr counts", output)

    # A function pointer cast that pairs a pointer with an int counts; one between pointer types doesn't
    write("src/a.c", TYPES + """
typedef void (*IntCallback)(u32 value);
typedef void (*WideCallback)(uptr value);
typedef void (*WorkCallback)(void *work);
typedef void (*TaskCallback)(Task *task);

void CallAll(WorkCallback callback, IntCallback numbers) {
    IntCallback cut = (IntCallback)callback;
    WideCallback widened = (WideCallback)numbers;
    TaskCallback kept = (TaskCallback)callback;

    cut(0);
    widened(0);
    kept(0);
}
""")
    status, output = run()
    check(status == 1 and "CallAll: fn-cast (IntCallback <- WorkCallback)" in output,
          "a callback cast that cuts a pointer counts", output)
    check("CallAll: fn-cast (WideCallback <- IntCallback)" in output,
          "a callback cast between a uptr and a u32 counts", output)
    check("TaskCallback <-" not in output, "a callback cast between pointer types doesn't count", output)

    # A host variant of a header, as port/tools/gen_host_tree.py writes it, is reported as its original
    header = "static inline unsigned int AddressOf(void *p) {\n    return (unsigned int)p;\n}\n"
    write("lib/x/include/x.h", header)
    write("gen/hwinclude/x.h",
          "// Generated by port/tools/gen_host_tree.py from lib/x/include/x.h; don't edit.\n" + header)
    write("src/a.c", '#include "x.h"\n')
    status, output = run()
    check(status == 1 and "lib/x/include/x.h in AddressOf: ptr-to-int" in output
          and "lib/x/include/x.h:2  return (unsigned int)p;" in output,
          "a host variant of a header is reported as the header it was made from", output)

    # A file that doesn't compile fails
    write("src/a.c", "int Broken(void) { return }\n")
    status, output = run()
    check(status == 1 and "failed to compile" in output, "a file that doesn't compile fails", output)

    if failures:
        print(f"{len(failures)} check(s) failed")
        return 1
    print("check_ptrcasts.py: every check passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
