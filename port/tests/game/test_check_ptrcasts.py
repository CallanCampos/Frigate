#!/usr/bin/env python3
"""The pointer cast guard's own test (port/tools/check_ptrcasts.py, the test game_ptrcasts_tool of
port/cmake/game.cmake). It touches no game functionality: it builds a small tree of its own under WORKDIR, a src/,
lib/ and port/ with a few casts and a compile database for CC whose commands carry what the host build's do (the four
-Wno- flags of the cast warnings, the decomp's headers as -isystem), and checks what the guard says about it: a new
finding fails with its site and the two ways out, --update writes rows to classify, a classified baseline passes, a
count above the baseline fails, a renamed function, type or file passes as a rename, a cast fixed in one function and
added in another of the same file doesn't, nor does a type changed to one that still exists, a finding gone passes with
a note, a narrowed uptr counts where a narrowed unsigned long or a getter's result as it is stored doesn't, a function
pointer cast that pairs a pointer or a uptr with an int counts where one between pointers doesn't, a header reached
through -isystem is checked, a host variant of a header (gen/hwinclude) is reported as the header it was made from, a
decomp file that a port/ wrapper compiles by #include is checked and the wrapper's own code isn't, a flag or a #pragma
that hides the warnings fails rather than passing with nothing found, a file that doesn't compile fails, and a build
without a compile database skips (77), or fails with --require.

    test_check_ptrcasts.py CC WORKDIR

It skips (77) when CC isn't clang, whose warnings the guard reads, and on Windows, after checking that the guard skips
there too (Windows' LLP64 gives other keys than its baseline's). Standard library only.
"""

from __future__ import annotations

import json
import os
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

# Two functions of one file, for the check that a cast fixed in one can't pass for one added in the other
PAIR = """
u32 First(void *work) {{
    {first}
}}

u32 Second(void *work) {{
    {second}
}}
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

    def database(*files: str, flags: str = ""):
        # The host build's commands for the game's C: the cast warnings silenced, the decomp's headers -isystem
        silenced = ("-Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-void-pointer-to-int-cast "
                    "-Wno-int-to-void-pointer-cast")
        entries = [{"directory": str(work), "file": f,
                    "command": f"{cc} -std=gnu99 {silenced} -I gen/hwinclude -isystem lib/x/include -I . {flags} "
                               f"-c {f} -o {Path(f).stem}.o"} for f in files]
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

    # No compile database: skip, or fail where the build says the check can run
    status, output = run()
    check(status == 77, "a build without compile_commands.json skips", output)
    status, output = run("--require")
    check(status == 1 and "doesn't skip" in output, "with --require, a build without compile_commands.json fails",
          output)

    # On Windows the guard skips whatever the tree holds: check that it does, and skip the rest
    if os.name == "nt" or sys.platform in ("win32", "cygwin", "msys"):
        write("src/a.c", FIRST)
        database("src/a.c")
        status, output = run()
        check(status == 77 and "Windows is LLP64" in output, "the guard skips on Windows", output)
        if failures:
            return 1
        print("skipped: the guard skips on Windows, whose LLP64 gives other keys than its baseline's")
        return 77

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

    # A cast fixed in one function and another added in a second function that still has its name is not a rename
    write("src/a.c", TYPES + PAIR.format(first="return (u32)work;", second="return 0;"))
    run("--update")
    classify_all("harmless")
    write("src/a.c", TYPES + PAIR.format(first="return 0;", second="return (u32)work;"))
    status, output = run()
    check(status == 1 and "new: src/a.c in Second: ptr-to-int (u32 <- void *)" in output and "renamed" not in output,
          "a cast fixed in one function can't pass for a new one in another", output)

    # A type renamed passes, with a note; a type changed to one that still exists doesn't
    write("src/a.c", TYPES + "typedef struct OldWork OldWork;\n\nu32 Keep(OldWork *work) {\n    return (u32)work;\n}\n")
    run("--update")
    classify_all("harmless")
    write("src/a.c", TYPES + "typedef struct NewWork NewWork;\n\nu32 Keep(NewWork *work) {\n    return (u32)work;\n}\n")
    status, output = run()
    check(status == 0 and "renamed" in output and "(u32 <- NewWork *)" in output, "a renamed type passes", output)
    write("src/a.c", TYPES + "typedef struct OldWork OldWork;\ntypedef struct NewWork NewWork;\n\n"
          "u32 Keep(NewWork *work) {\n    return (u32)work;\n}\n")
    status, output = run()
    check(status == 1 and "new: src/a.c in Keep: ptr-to-int (u32 <- NewWork *)" in output,
          "a type changed to one that still exists isn't a rename", output)

    # A file renamed passes, with a note; the same function in a new file while the old one still names it doesn't
    write("src/a.c", TYPES + "typedef struct NewWork NewWork;\n\nu32 Keep(NewWork *work) {\n    return (u32)work;\n}\n")
    run("--update")
    (work / "src/a.c").rename(work / "src/c.c")
    database("src/c.c")
    status, output = run()
    check(status == 0 and "renamed" in output and "src/c.c in Keep" in output, "a renamed file passes", output)
    write("src/a.c", TYPES + "u32 Keep(void *work);\n")
    database("src/a.c", "src/c.c")
    status, output = run()
    check(status == 1 and "new: src/c.c in Keep" in output,
          "a function in a new file while the old file still names it isn't a rename", output)
    (work / "src/c.c").unlink()
    database("src/a.c")

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

    # A header reached through -isystem, as the host build reaches the decomp's, is checked
    write("lib/x/include/y.h", "static inline unsigned int CutOf(void *p) {\n    return (unsigned int)p;\n}\n")
    write("src/a.c", "#include <y.h>\n")
    status, output = run()
    check(status == 1 and "lib/x/include/y.h in CutOf: ptr-to-int" in output,
          "a header reached through -isystem is checked", output)

    # A decomp file that a port/ wrapper compiles by #include is checked, as the decomp's; the wrapper's own code isn't
    (work / "lib/x/src").mkdir(parents=True)
    (work / "port/hal").mkdir(parents=True)
    write("lib/x/src/w.c", "unsigned int Wrapped(void *p) {\n    return (unsigned int)p;\n}\n")
    write("port/hal/w_host.c", '#include "lib/x/src/w.c"\n\nunsigned int HostOnly(void *p) {\n'
          "    return (unsigned int)p;\n}\n")
    write("src/a.c", TYPES)
    database("src/a.c", "port/hal/w_host.c")
    status, output = run()
    check(status == 1 and "new: lib/x/src/w.c in Wrapped: ptr-to-int" in output and "HostOnly" not in output,
          "a wrapped decomp file is checked, and the wrapper's own code isn't", output)

    # A flag or a pragma that hides the warnings fails, rather than passing with nothing found
    write("src/a.c", FIRST)
    database("src/a.c", flags="-w")
    status, output = run()
    check(status == 1 and "can't see what it checks" in output and "sites in" not in output,
          "-w in the build's commands fails", output)
    write("lib/x/include/quiet.h", '#pragma clang diagnostic ignored "-Wvoid-pointer-to-int-cast"\n')
    database("src/a.c", flags="-include lib/x/include/quiet.h")
    status, output = run()
    check(status == 1 and "can't see what it checks" in output and "no ptr-to-int" in output,
          "a forced include's pragma that hides a kind fails", output)
    database("src/a.c")

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
