#!/usr/bin/env python3
"""Tests of port/tools/strict_routes.py, the runner of T5's set in a strict build (ADR 0033, port/docs/sanitizers.md):
ctest's route tests that need no oracle, and Frigate's sanitized program alone on oracle.py's comparison sequences and
on the port's legs of its round trips, each from the save it starts from, judged clean or not.

  - On oracle.py's real tables, the plan's invariants, with no counts and no names: every case resolves or is left out
    with why, no two share a start and an input file, a continue leg follows its make leg, every sequence without a
    reference is a case or merged into one, and every input file exists in both games.
  - On made-up tables: the sequences with a reference left to ctest and taken back by --all and --only, merges of an
    equal and of a shorter run but never across input files, a continue leg after a merged make leg, a save that a
    version doesn't keep, and a sequence that allows a trap stop planned as any other.
  - With a stand-in for the program (a script this test writes, run by this Python, which takes the program's options
    and does what its input file's "# standin:" lines say): the verdicts (a clean run; each of ctest's four report
    markers at status 0; statuses 134, 73 and 70, and a trap stop that the sequence allows; a run a frame short; the
    frames line after a time prefix; a hang past --timeout, killed; a crash by a signal), the saves (a missing one skips
    with the command that makes it, one changed byte fails with both SHA-1s, a make leg's save reaches its continue leg,
    a make leg that saves nothing fails and skips its continue leg), the build's checks (made-up CMake caches, Apple
    clang, a program without ASan, a program without the function check), the program's environment, the summary table
    and the directories' names. The markers are held to port/CMakeLists.txt's.
  - With a stand-in for ctest (another script, which lists the made-up build's route tests from its routes.txt and
    writes ctest's JUnit file for them): ctest's half runs first with the criterion's options, its failures, skips and
    a build without route tests count, and --only selects ctest's tests apart from the cases.
  - MUTANTS lists deliberate bugs in strict_routes.py, each with the tests that must catch it, and MutationTests applies
    each to the runner's source, loads the result in the runner's place and runs those tests, which must fail; the same
    tests pass on the runner as it is.

What the runner prints is kept, never printed: a stand-in prints sanitizer report markers, and ctest fails a test of the
sanitized build whose output has one (port/CMakeLists.txt), so the markers here are put together at run time. No ROM,
no game, no oracle; standard library only. Game functionality: none runs here; the runner plays ctest's replays and the
routes from the oracle's saves (CONTINUE, the field menu's apps, Hugh's battle, Route 19, the catch, the PC, Floccesy to
Route 20).

    test_strict_routes.py [unittest options]
"""

from __future__ import annotations

import contextlib
import hashlib
import io
import os
import shutil
import subprocess
import sys
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

sys.dont_write_bytecode = True
PORT = Path(__file__).resolve().parents[2]
RUNNER = PORT / "tools" / "strict_routes.py"
sys.path.insert(0, str(RUNNER.parent))

import strict_routes  # noqa: E402

oracle = strict_routes.oracle
Sequence = oracle.Sequence
RoundTrip = oracle.RoundTrip
POSIX = os.name != "nt"
VERSIONS = ("b2_us", "w2_us")
CRITERION = "detect_leaks=1:detect_stack_use_after_return=1"

# The program's stand-in. Its report lines are put together at run time, so that no line of this file is one
STANDIN = r'''
"""A stand-in for Frigate's program, for test_strict_routes.py: it takes the program's options and does what the
"# standin:" lines of its input file say; with --help it prints ASan's flag list when ASAN_OPTIONS asks for it"""
import hashlib
import os
import signal
import sys
import time

MARKERS = ("==1==ERROR: " + "AddressSanitizer: heap-use-after-free on address 0x1",
           "SUMMARY: " + "UndefinedBehaviorSanitizer: undefined-behavior x.c:1:1",
           "x.c:1:1: runtime " + "error: load of misaligned address",
           "==1==Leak" + "Sanitizer has encountered a fatal error")

arguments = sys.argv[1:]
if "--help" not in arguments:
    # First, so that a run killed at once has left it
    with open("pid.txt", "w") as f:
        f.write(str(os.getpid()))
if "--help" in arguments:
    if "help=1" in os.environ.get("ASAN_OPTIONS", "") and "--standin-no-asan" not in arguments:
        print("Available flags for " + "AddressSanitizer:", file=sys.stderr)
    print("usage: frigate-black2 [--help] [--headless] [--frames N]")
    sys.exit(0)


def option(name):
    return arguments[arguments.index(name) + 1]


frames = int(option("--frames"))
saves = option("--save-dir")
said = {}
with open(option("--input")) as f:
    for line in f:
        if line.startswith("# standin:"):
            words = line.split()[2:]
            said[words[0]] = words[1:]
print("port: entering the game's main()", file=sys.stderr)
if "print-env" in said:
    for name in ("ASAN_OPTIONS", "UBSAN_OPTIONS", "LSAN_OPTIONS"):
        print(f"env {name}={os.environ[name]}" if name in os.environ else f"env {name} unset")
if "print-save" in said:
    found = sorted(name for name in os.listdir(saves) if name.endswith(".sav"))
    for name in found:
        with open(os.path.join(saves, name), "rb") as f:
            print(f"save {name} {hashlib.sha1(f.read()).hexdigest()}")
    if not found:
        print("save none")
if "write-save" in said:
    with open(os.path.join(saves, "IREO.sav"), "wb") as f:
        f.write(hashlib.sha1(option("--input").encode()).digest() * 4)
if "hang" in said:
    time.sleep(600)
if "signal" in said:
    os.kill(os.getpid(), signal.SIGSEGV)
for number in said.get("marker", []):
    print(MARKERS[int(number)], file=sys.stderr)
ran = frames - 1 if "short" in said else frames
prefix = "2026-11-02 09:00:00.000 frigate-black2[1:2] " if "prefix" in said else ""
print(f"{prefix}port: stopped after {ran} frames", file=sys.stderr)
sys.exit(int(said["status"][0]) if "status" in said else 0)
'''

# ctest's stand-in, with the same markers put together at run time
STANDIN_CTEST = r'''
"""A stand-in for ctest, for test_strict_routes.py: it reads the build's route tests from routes.txt in --test-dir, a
line a test (its name, then what its run does: fail, skip, marker N); with -N it lists those -R selects, as ctest -N
does, and else writes ctest's JUnit file for them (--output-junit) and its arguments and environment in ctest-env.txt
beside routes.txt"""
import os
import re
import sys
from xml.sax.saxutils import escape, quoteattr

MARKERS = ("==1==ERROR: " + "AddressSanitizer: heap-use-after-free on address 0x1",
           "SUMMARY: " + "UndefinedBehaviorSanitizer: undefined-behavior x.c:1:1",
           "x.c:1:1: runtime " + "error: load of misaligned address",
           "==1==Leak" + "Sanitizer has encountered a fatal error")
arguments = sys.argv[1:]


def option(name):
    return arguments[arguments.index(name) + 1]


directory = option("--test-dir")
with open(os.path.join(directory, "routes.txt")) as f:
    tests = [line.split() for line in f if line.strip()]
if "-R" in arguments:
    tests = [test for test in tests if re.search(option("-R"), test[0])]
if "-N" in arguments:
    for number, test in enumerate(tests, 1):
        print(f"  Test  #{number}: {test[0]}")
    print(f"\nTotal Tests: {len(tests)}")
    sys.exit(0)
with open(os.path.join(directory, "ctest-env.txt"), "w") as f:
    for name in ("ASAN_OPTIONS", "UBSAN_OPTIONS", "LSAN_OPTIONS"):
        f.write(f"{name}={os.environ[name]}\n" if name in os.environ else f"{name} unset\n")
    f.write("arguments " + " ".join(arguments) + "\n")
cases = []
failed = 0
for name, *words in tests:
    lines = [MARKERS[int(words[words.index("marker") + 1])]] if "marker" in words else []
    lines.append("port: stopped after 40 frames")
    inner, status = "", "run"
    if "fail" in words or "marker" in words:
        failed += 1
        inner, status = '<failure message="Failed"/>', "fail"
    elif "skip" in words:
        inner, status = '<skipped message="SKIP_RETURN_CODE=77"/>', "notrun"
    cases.append(f'<testcase name={quoteattr(name)} classname={quoteattr(name)} time="0.5" status="{status}">'
                 f'{inner}<system-out>{escape(chr(10).join(lines))}</system-out></testcase>')
with open(option("--output-junit"), "w") as f:
    f.write(f'<?xml version="1.0" encoding="UTF-8"?>\n<testsuite tests="{len(tests)}" failures="{failed}">\n'
            + "\n".join(cases) + "\n</testsuite>\n")
print(f"{len(tests) - failed} of {len(tests)} tests passed")
sys.exit(8 if failed else 0)
'''


def fail_expressions(text: str) -> list[str]:
    """The quoted arguments of port/CMakeLists.txt's FAIL_REGULAR_EXPRESSION property, up to the call's end"""
    at = text.index("PROPERTY FAIL_REGULAR_EXPRESSION") + len("PROPERTY FAIL_REGULAR_EXPRESSION")
    found: list[str] = []
    current: list[str] = []
    quoted = False
    while at < len(text):
        character = text[at]
        if quoted and character == "\\":
            current.append(text[at + 1])
            at += 1
        elif quoted and character == '"':
            found.append("".join(current))
            current, quoted = [], False
        elif quoted:
            current.append(character)
        elif character == '"':
            quoted = True
        elif character == ")":
            break
        at += 1
    return found


def names(plan) -> list[str]:
    return [case.name for case in plan.cases]


class RealTableTests(unittest.TestCase):
    """oracle.py's own tables, as the runner plans them in each game"""

    def test_the_plan_of_oracle_pys_tables(self):
        for version in VERSIONS:
            with self.subTest(version=version):
                plan = strict_routes.plan_cases(version)
                every = strict_routes.plan_cases(version, include_all=True)
                for each in (plan, every):
                    cases = names(each)
                    for case, reason in each.left_out:
                        self.assertTrue(reason, f"{case.name} is left out without a reason")
                    for case in each.cases:
                        self.assertGreater(case.frames, 0, case.name)
                        self.assertTrue(not case.start or case.start in oracle.SAVES or case.after == case.start,
                                        f"{case.name} starts from {case.start}, which nothing makes")
                        if case.after:
                            self.assertLess(cases.index(case.after), cases.index(case.name),
                                            f"{case.name} comes before its make leg")
                    keys = [(case.start, case.input) for case in each.cases]
                    self.assertEqual(len(keys), len(set(keys)), "two cases share a start and an input file")
                    covered = set(cases) | {name for case in each.cases for name in case.also}
                    covered |= {case.name for case, _ in each.left_out}
                    for name in oracle.ROUND_TRIPS:
                        self.assertIn(f"trip:{name}:make", covered)
                        self.assertIn(f"trip:{name}:continue", covered)
                for name in oracle.SEQUENCES:
                    reference = os.path.join(oracle.REPO_ROOT,
                                             oracle.SEQUENCE_REFERENCE.format(name=name, version=version))
                    covered = set(names(plan)) | {other for case in plan.cases for other in case.also}
                    covered |= {case.name for case, _ in plan.left_out}
                    if os.path.exists(reference):
                        self.assertIn(name, plan.referenced)
                    else:
                        self.assertIn(f"seq:{name}", covered, f"{name} has no reference and no case")
        # Every input file exists in both games, as oracle_<sequence> and the round trips need too
        for version in VERSIONS:
            for name in oracle.SEQUENCES:
                case = strict_routes.sequence_case(name, version)[0]
                self.assertTrue(os.path.exists(strict_routes.input_path(case)), f"{name} in {version}")
            for name in oracle.ROUND_TRIPS:
                for case in strict_routes.trip_cases(name, version):
                    self.assertTrue(os.path.exists(strict_routes.input_path(case)), f"{case.name} in {version}")

    def test_list_runs_as_a_script(self):
        done = subprocess.run([sys.executable, str(RUNNER), "list", "--version", "b2_us"], capture_output=True,
                              text=True, timeout=120)
        self.assertEqual(done.returncode, 0, done.stderr[-2000:])
        self.assertIn("seq:", done.stdout)
        self.assertIn(":make", done.stdout)


class World(unittest.TestCase):
    """A made-up checkout, strict build, ROM, save cache and oracle.py tables in a temporary directory, and the runner
    run on them in this process, with the stand-ins as the program and as ctest"""

    def setUp(self):
        self.root = Path(tempfile.mkdtemp(prefix="strict-routes-"))
        self.addCleanup(shutil.rmtree, self.root, True)
        self.checkout = self.root / "checkout"
        self.inputs = self.checkout / "port" / "tests" / "oracle"
        self.inputs.mkdir(parents=True)
        self.build = self.root / "build"
        self.build.mkdir()
        self.program = self.build / ("frigate-black2" + ("" if POSIX else ".exe"))
        self.write_program()
        self.cache = self.root / "cache"
        self.cache.mkdir()
        self.work = self.root / "work"
        self.rom = self.root / "baserom.nds"
        self.rom.write_bytes(bytes(12) + b"IREO" + bytes(16))
        self.standin = self.root / "standin.py"
        self.standin.write_text(STANDIN, encoding="utf-8")
        self.ctest = self.root / "ctest"
        self.standin_ctest = self.root / "standin_ctest.py"
        self.standin_ctest.write_text(STANDIN_CTEST, encoding="utf-8")
        self.routes(["replay_clean"])
        self.compiler = "Homebrew clang version 23.1.3\nTarget: arm64-apple-darwin25.5.0\n"
        self.write_cache({})
        self.patch(oracle, "REPO_ROOT", str(self.checkout))
        self.patch(oracle, "rom_matches", lambda version, path: None)
        self.use_tables({}, {}, {})

    def patch(self, target, name, value):
        patcher = mock.patch.object(target, name, value)
        patcher.start()
        self.addCleanup(patcher.stop)

    def write_program(self, function_check: bool = True) -> None:
        """The program's file, naming the function check's handler as a program built with the check does among its
        symbols, unless not function_check"""
        handler = b"___ubsan_handle_" + strict_routes.FUNCTION_CHECK + b"_abort" if function_check else b"_main"
        self.program.write_bytes(b"\0" + handler + b"\0")

    def routes(self, lines: list[str]) -> None:
        """The made-up build's ctest tests labelled route and not oracle, for the stand-in ctest: a line a test, its
        name and what its run does"""
        (self.build / "routes.txt").write_text("".join(f"{line}\n" for line in lines), encoding="utf-8")

    def write_cache(self, changes: dict[str, str | None]) -> None:
        """The build's CMakeCache.txt, a strict build of this checkout but for changes (None leaves an entry out)"""
        entries = {"CMAKE_HOME_DIRECTORY": ("INTERNAL", str(self.checkout / "port")),
                   "CMAKE_C_COMPILER": ("FILEPATH", str(self.root / "clang")),
                   "CMAKE_CTEST_COMMAND": ("INTERNAL", str(self.ctest)),
                   "CMAKE_BUILD_TYPE": ("STRING", "RelWithDebInfo"),
                   "PORT_SANITIZE": ("BOOL", "ON"), "PORT_SANITIZE_RECOVER": ("BOOL", "OFF"),
                   "PORT_ROM_LANGUAGE": ("STRING", "us"), "PORT_VERSION": ("STRING", "b2_us")}
        lines = ["# This is the CMakeCache file.", "//The program's version"]
        for name, (kind, value) in entries.items():
            value = changes.get(name, value) if name in changes else value
            if value is not None:
                lines.append(f"{name}:{kind}={value}")
        (self.build / "CMakeCache.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")

    def save(self, name: str, start: str = "", versions=VERSIONS, cached: bool = True):
        """A save of the made-up SAVES, its file in the cache in each version unless not cached"""
        sha1 = {}
        for version in versions:
            data = hashlib.sha1(f"{name} {version}".encode()).digest() * 8
            if cached:
                (self.cache / f"{name}_{version}.sav").write_bytes(data)
            sha1[version] = hashlib.sha1(data).hexdigest()
        return oracle.SaveMaker(100, sha1, start)

    def use_tables(self, sequences, saves, trips, references=(), said=None):
        """oracle.py's tables made up for the test, with every input file they name in both games, its stand-in lines
        those of said[input name], and a reference for each (name, version) of references"""
        self.patch(oracle, "SEQUENCES", sequences)
        self.patch(oracle, "SAVES", saves)
        self.patch(oracle, "ROUND_TRIPS", trips)
        inputs = {sequence.input or name for name, sequence in sequences.items()}
        for trip in trips.values():
            inputs |= {trip.maker, trip.continued, *trip.port_input.values()}
        for name in inputs:
            lines = ["pbwinput 1", *(f"# standin: {line}" for line in (said or {}).get(name, []))]
            for version in VERSIONS:
                (self.inputs / f"{name}_{version}.pbwinput").write_text("\n".join(lines) + "\n", encoding="utf-8")
        for name, version in references:
            (self.inputs / f"{name}_{version}.tsv").write_text("port\toracle\n", encoding="utf-8")

    def run_runner(self, *arguments: str, program=None, environment=None, real_compiler=False) -> tuple[int, str]:
        """strict_routes.py run on the made-up build, in this process: its status and what it printed. program gives
        the command that starts the program (the stand-in by default); ctest is always its stand-in"""
        argv = ["run", "--build-dir", str(self.build), "--rom", str(self.rom), "--work", str(self.work),
                "--save-cache", str(self.cache), "--summary", str(self.root / "summary.tsv"), *arguments]
        start = program or (lambda path: [sys.executable, str(self.standin)])

        def launcher(path: str) -> list[str]:
            return [sys.executable, str(self.standin_ctest)] if path == str(self.ctest) else start(path)

        printed = io.StringIO()
        with contextlib.ExitStack() as stack:
            if not real_compiler:
                stack.enter_context(mock.patch.object(strict_routes, "compiler_version",
                                                      lambda compiler: self.compiler))
            stack.enter_context(mock.patch.dict(os.environ, environment or {}))
            stack.enter_context(contextlib.redirect_stdout(printed))
            stack.enter_context(contextlib.redirect_stderr(printed))
            status = strict_routes.main(argv, launcher=launcher)
        return status, printed.getvalue()

    def summary(self) -> dict[str, dict[str, str]]:
        lines = (self.root / "summary.tsv").read_text(encoding="utf-8").splitlines()
        header = lines[0].split("\t")
        return {row["case"]: row for row in (dict(zip(header, line.split("\t"))) for line in lines[1:])}

    def output(self, case: str) -> str:
        return (self.work / case / "output.txt").read_text(encoding="utf-8", errors="replace")


class PlanTests(World):
    """The plan of made-up tables"""

    def made_up(self, stop_from: int = 0) -> None:
        saves = {"first": self.save("first"), "second": self.save("second", "first", versions=("b2_us",)),
                 "made": self.save("made", "first")}
        sequences = {
            "opening": Sequence(50, 60, "", "", stop_from=stop_from),
            "resume": Sequence(40, 50, "", "", save="first"),
            "resume_long": Sequence(60, 70, "", "", save="first", input="resume"),
            "menus": Sequence(40, 50, "", "", save="first"),
            "twin": Sequence(25, 30, "", ""),
            "twin_too": Sequence(25, 30, "", "", input="twin"),
            "piece": Sequence(30, 40, "", "", save="first", input="made"),
            "later": Sequence(20, 30, "", "", save="second"),
            "after": Sequence(20, 30, "", "", save="made"),
        }
        trips = {"loop": RoundTrip("made", 30, "after"), "again": RoundTrip("made", 30, "after")}
        self.use_tables(sequences, saves, trips, references=[("opening", "b2_us"), ("opening", "w2_us")])

    def test_references(self):
        self.made_up()
        plan = strict_routes.plan_cases("b2_us")
        self.assertNotIn("seq:opening", names(plan))
        self.assertEqual(plan.referenced, ["opening"])
        self.assertIn("seq:opening", names(strict_routes.plan_cases("b2_us", include_all=True)))
        self.assertEqual(names(strict_routes.plan_cases("b2_us", only=["opening"])), ["seq:opening"])

    def test_merges(self):
        self.made_up()
        plan = strict_routes.plan_cases("b2_us")
        # A shorter run from the same save and input: the longer runs, at its frames. An equal one: the first. An equal
        # one beside a make leg: the make leg, for its save. The same save with another input file: no merge
        self.assertEqual({case.name: (case.also, case.frames) for case in plan.cases},
                         {"seq:resume_long": (["seq:resume"], 60), "seq:menus": ([], 40),
                          "seq:twin": (["seq:twin_too"], 25), "seq:later": ([], 20), "seq:after": ([], 20),
                          "trip:loop:make": (["seq:piece", "trip:again:make"], 30),
                          "trip:loop:continue": (["trip:again:continue"], 20)})
        keys = [(case.start, case.input) for case in plan.cases]
        self.assertEqual(len(keys), len(set(keys)))

    def test_a_continue_leg_follows_its_merged_make_leg(self):
        self.made_up()
        plan = strict_routes.plan_cases("b2_us")
        cases = {case.name: case for case in plan.cases}
        self.assertNotIn("trip:again:make", cases)
        self.assertNotIn("trip:again:continue", cases)
        resume = cases["trip:loop:continue"]
        self.assertEqual((resume.after, resume.start, resume.input, resume.frames),
                         ("trip:loop:make", "trip:loop:make", "after", 20))
        self.assertEqual(resume.also, ["trip:again:continue"])
        order = names(plan)
        self.assertLess(order.index("trip:loop:make"), order.index("trip:loop:continue"))
        # The sequence from the save the oracle made is a case of its own
        self.assertEqual(cases["seq:after"].start, "made")

    def test_a_save_a_version_doesnt_keep_is_left_out(self):
        self.made_up()
        self.assertIn("seq:later", names(strict_routes.plan_cases("b2_us")))
        plan = strict_routes.plan_cases("w2_us")
        self.assertNotIn("seq:later", names(plan))
        reasons = {case.name: reason for case, reason in plan.left_out}
        self.assertIn("second", reasons["seq:later"])
        self.assertIn("w2_us", reasons["seq:later"])

    def test_a_sequence_that_allows_a_trap_stop_is_a_case(self):
        self.made_up(stop_from=10)
        self.assertNotIn("seq:opening", names(strict_routes.plan_cases("b2_us")))
        for selection in ({"include_all": True}, {"only": ["opening"]}):
            with self.subTest(**selection):
                cases = {case.name: case for case in strict_routes.plan_cases("b2_us", **selection).cases}
                self.assertEqual(cases["seq:opening"].stop_from, 10)

    def test_only(self):
        self.made_up()
        self.assertEqual(names(strict_routes.plan_cases("b2_us", only=["loop"])),
                         ["trip:loop:make", "trip:loop:continue"])
        self.assertEqual(names(strict_routes.plan_cases("b2_us", only=["trip:loop:continue"])),
                         ["trip:loop:make", "trip:loop:continue"])
        self.assertEqual(names(strict_routes.plan_cases("b2_us", only=["piece", "menus"])),
                         ["seq:menus", "seq:piece"])
        with self.assertRaises(strict_routes.Refusal):
            strict_routes.plan_cases("b2_us", only=["nothing"])


class VerdictTests(World):
    """Each way a run ends, as the stand-in ends it"""

    def test_verdicts(self):
        scenarios = {"clean": [], "prefix": ["prefix"], "short": ["short"],
                     "status_134": ["status 134"], "status_73": ["status 73"], "status_70": ["status 70"]}
        scenarios |= {f"marker_{number}": [f"marker {number}"] for number in range(4)}
        self.use_tables({name: Sequence(40, 50, "", "") for name in scenarios}, {}, {}, said=scenarios)
        status, _ = self.run_runner("--jobs", "4")
        self.assertEqual(status, 1)
        rows = self.summary()
        verdicts = {name: rows[f"seq:{name}"]["verdict"] for name in scenarios}
        expected = {name: "clean" if name in ("clean", "prefix") else "failed" for name in scenarios}
        self.assertEqual(verdicts, expected)
        self.assertEqual(rows["seq:status_134"]["status"], "134")
        self.assertIn("39 of 40", rows["seq:short"]["reason"])
        self.assertEqual(rows["seq:prefix"]["reached"], "40")

    def test_a_trap_stop_its_sequence_allows_fails(self):
        allowed = {"stop_from": 10, "stop_trap": "func_ov099_0", "stop": "no C yet"}
        self.use_tables({"gap": Sequence(40, 50, "", "", **allowed), "whole": Sequence(40, 50, "", "", **allowed)},
                        {}, {}, said={"gap": ["status 70"]})
        status, _ = self.run_runner()
        self.assertEqual(status, 1)
        rows = self.summary()
        self.assertEqual((rows["seq:gap"]["verdict"], rows["seq:gap"]["status"]), ("failed", "70"))
        self.assertIn("allows from frame 10", rows["seq:gap"]["reason"])
        self.assertEqual(rows["seq:whole"]["verdict"], "clean")

    def test_a_hang_past_the_timeout_fails_and_is_killed(self):
        self.use_tables({"hang": Sequence(40, 50, "", "")}, {}, {}, said={"hang": ["hang"]})
        status, _ = self.run_runner("--timeout", "2")
        self.assertEqual(status, 1)
        row = self.summary()["seq:hang"]
        self.assertEqual((row["verdict"], row["status"]), ("failed", "timeout"))
        self.assertIn("timeout", row["reason"])
        # Killed: its process is gone (a stand-in that a loaded machine kept from starting in time has no pid to check)
        pid = self.work / "seq-hang" / "pid.txt"
        if POSIX and pid.exists() and pid.read_text():
            with self.assertRaises(ProcessLookupError):
                os.kill(int(pid.read_text()), 0)

    @unittest.skipIf(not POSIX, "a crash by a signal is POSIX's")
    def test_a_crash_by_a_signal_fails(self):
        self.use_tables({"crash": Sequence(40, 50, "", "")}, {}, {}, said={"crash": ["signal"]})
        status, _ = self.run_runner()
        self.assertEqual(status, 1)
        row = self.summary()["seq:crash"]
        self.assertEqual((row["verdict"], row["status"]), ("failed", "signal 11"))


class SaveTests(World):
    """The saves the cases start from, and the ones the make legs leave"""

    def test_a_missing_save_skips_and_prints_the_command(self):
        self.use_tables({"resume": Sequence(40, 50, "", "", save="first")}, {"first": self.save("first", cached=False)},
                        {})
        status, printed = self.run_runner()
        self.assertEqual(status, 77)
        self.assertIn(f"oracle.py save first --version b2_us --save-cache {self.cache} --oracle ", printed)
        self.assertEqual(self.summary()["seq:resume"]["verdict"], "skipped")
        self.assertFalse((self.work / "seq-resume").exists(), "a program ran")

    def test_a_changed_byte_fails_naming_both_sha1s(self):
        first = self.save("first")
        path = self.cache / "first_b2_us.sav"
        data = bytearray(path.read_bytes())
        data[5] ^= 1
        path.write_bytes(bytes(data))
        self.use_tables({"resume": Sequence(40, 50, "", "", save="first")}, {"first": first}, {})
        status, _ = self.run_runner()
        self.assertEqual(status, 1)
        row = self.summary()["seq:resume"]
        self.assertEqual(row["verdict"], "failed")
        self.assertIn(hashlib.sha1(data).hexdigest(), row["reason"])
        self.assertIn(first.sha1["b2_us"], row["reason"])

    def test_the_produced_save_reaches_the_continue_leg(self):
        saves = {"first": self.save("first"), "made": self.save("made", "first")}
        sequences = {"piece": Sequence(30, 40, "", "", save="first", input="made"),
                     "after": Sequence(20, 30, "", "", save="made")}
        trips = {"loop": RoundTrip("made", 30, "after"), "again": RoundTrip("made", 30, "after")}
        self.use_tables(sequences, saves, trips, said={"made": ["write-save"], "after": ["print-save"]})
        status, _ = self.run_runner("--keep", "--jobs", "2")
        self.assertEqual(status, 0)
        rows = self.summary()
        produced = hashlib.sha1((self.work / "trip-loop-make" / "produced.sav").read_bytes()).hexdigest()
        self.assertEqual((rows["trip:loop:make"]["also"], rows["trip:loop:make"]["produced_sha1"]),
                         ("seq:piece,trip:again:make", produced[:12]))
        self.assertEqual((rows["trip:loop:continue"]["start"], rows["trip:loop:continue"]["start_sha1"]),
                         ("trip:loop:make's save", produced[:12]))
        self.assertIn(f"save IREO.sav {produced}", self.output("trip-loop-continue"))
        self.assertIn(f"save IREO.sav {saves['made'].sha1['b2_us']}", self.output("seq-after"))

    def test_a_make_leg_that_saves_nothing_fails(self):
        saves = {"first": self.save("first"), "made": self.save("made", "first"), "fresh": self.save("fresh")}
        trips = {"same": RoundTrip("made", 30, "after"), "none": RoundTrip("fresh", 30, "after")}
        self.use_tables({"after": Sequence(20, 30, "", "", save="made")}, saves, trips)
        status, _ = self.run_runner()
        self.assertEqual(status, 1)
        rows = self.summary()
        self.assertEqual(rows["trip:same:make"]["verdict"], "failed")
        self.assertIn("unchanged", rows["trip:same:make"]["reason"])
        self.assertEqual(rows["trip:none:make"]["verdict"], "failed")
        self.assertIn("no save", rows["trip:none:make"]["reason"])
        for trip in trips:
            row = rows[f"trip:{trip}:continue"]
            self.assertEqual((row["verdict"], row["reason"]), ("skipped", "its make leg failed"))


class BuildTests(World):
    """The checks of the build before anything runs"""

    def setUp(self):
        super().setUp()
        self.use_tables({"clean": Sequence(40, 50, "", "")}, {}, {})

    def test_refusals(self):
        for name, changes in (("no PORT_SANITIZE", {"PORT_SANITIZE": None}), ("PORT_SANITIZE off",
                              {"PORT_SANITIZE": "OFF"}), ("PORT_SANITIZE_RECOVER on", {"PORT_SANITIZE_RECOVER": "ON"}),
                              ("the Spanish ROM", {"PORT_ROM_LANGUAGE": "es"}),
                              ("another checkout", {"CMAKE_HOME_DIRECTORY": str(self.root / "other" / "port")})):
            with self.subTest(name):
                self.write_cache(changes)
                status, _ = self.run_runner()
                self.assertEqual(status, 2)
                self.assertFalse(self.work.exists(), "a case ran")
        self.write_cache({})
        self.program.unlink()
        self.assertEqual(self.run_runner()[0], 2)
        self.write_program()
        status, printed = self.run_runner()
        self.assertEqual(status, 0)
        self.assertIn("the criterion: yes", printed)

    def test_compiler_classification(self):
        apple = "Apple clang version 17.0.0 (clang-1700.0.13.3)\nTarget: arm64-apple-darwin25.5.0"
        for text, kind in ((apple, "apple"),
                           ("Homebrew clang version 23.1.3\nTarget: arm64-apple-darwin25.5.0", "llvm"),
                           ("Ubuntu clang version 18.1.3 (1ubuntu1)\nTarget: x86_64-pc-linux-gnu", "llvm"),
                           ("clang version 21.1.0\nTarget: x86_64-w64-windows-gnu", "llvm"),
                           ("gcc (GCC) 14.2.0", "other"), ("", "other")):
            with self.subTest(text=text.split("\n")[0]):
                self.assertEqual(strict_routes.classify_compiler(text), kind)

    def test_apple_clang_is_refused(self):
        self.compiler = "Apple clang version 17.0.0 (clang-1700.0.13.3)\nTarget: arm64-apple-darwin25.5.0\n"
        self.assertEqual(self.run_runner()[0], 2)
        self.assertFalse(self.work.exists(), "a case ran")
        status, printed = self.run_runner("--allow-apple-clang")
        self.assertEqual(status, 0)
        self.assertIn("the criterion: no", printed)

    @unittest.skipIf(not POSIX, "the stand-in compiler is a shell script")
    def test_apple_clang_by_the_compiler_itself(self):
        compiler = self.root / "clang"
        compiler.write_text("#!/bin/sh\necho 'Apple clang version 17.0.0 (clang-1700.0.13.3)'\n", encoding="utf-8")
        compiler.chmod(0o755)
        self.assertEqual(self.run_runner(real_compiler=True)[0], 2)
        compiler.write_text("#!/bin/sh\necho 'Homebrew clang version 23.1.3'\n", encoding="utf-8")
        self.assertEqual(self.run_runner(real_compiler=True)[0], 0)

    def test_a_program_without_asan_is_refused(self):
        status, _ = self.run_runner(program=lambda path: [sys.executable, str(self.standin), "--standin-no-asan"])
        self.assertEqual(status, 2)
        self.assertFalse(self.work.exists(), "a case ran")

    def test_a_program_without_the_function_check_is_refused(self):
        self.write_program(function_check=False)
        status, printed = self.run_runner()
        self.assertEqual(status, 2)
        self.assertIn("has no function check", printed)
        self.assertFalse(self.work.exists(), "a case ran")
        status, printed = self.run_runner("--allow-apple-clang")
        self.assertEqual(status, 0)
        self.assertIn("the criterion: no: the program has no function check", printed)

    def test_another_build_type_isnt_the_criterion(self):
        self.write_cache({"CMAKE_BUILD_TYPE": "Debug"})
        status, printed = self.run_runner()
        self.assertEqual(status, 0)
        self.assertIn("the criterion: no: CMAKE_BUILD_TYPE is Debug", printed)


class CtestTests(World):
    """ctest's half: the build's route tests that need no oracle, as ctest's stand-in runs them"""

    def setUp(self):
        super().setUp()
        self.use_tables({"alone": Sequence(40, 50, "", "")}, {}, {})

    def ctest_ran(self) -> str:
        """What the stand-in ctest recorded of its last run: its environment's sanitizer options and its arguments"""
        path = self.build / "ctest-env.txt"
        self.assertTrue(path.exists(), "ctest didn't run")
        return path.read_text(encoding="utf-8")

    def test_the_routes_run_first_with_the_criterions_options(self):
        self.routes(["replay_one", "replay_two"])
        caller = {"ASAN_OPTIONS": "detect_leaks=0", "UBSAN_OPTIONS": "print_stacktrace=0",
                  "LSAN_OPTIONS": "verbosity=1"}
        status, printed = self.run_runner("--jobs", "2", environment=caller)
        self.assertEqual(status, 0)
        ran = self.ctest_ran()
        self.assertIn(f"ASAN_OPTIONS={CRITERION}\n", ran)
        self.assertIn("UBSAN_OPTIONS unset", ran)
        self.assertIn("LSAN_OPTIONS unset", ran)
        self.assertIn(" -L route -LE oracle --parallel 2 ", ran)
        self.assertNotIn(" -R ", ran)
        self.assertIn("ctest's route tests that need no oracle: 2,", printed)
        lines = (self.root / "summary.tsv").read_text(encoding="utf-8").splitlines()
        self.assertEqual([line.split("\t")[1] for line in lines[1:]], ["ctest:replay_one", "ctest:replay_two",
                                                                      "seq:alone"])
        row = self.summary()["ctest:replay_one"]
        self.assertEqual((row["verdict"], row["status"], row["frames"], row["reached"]), ("clean", "passed", "-", "40"))
        self.assertFalse((self.work / strict_routes.CTEST_DIRECTORY).exists(), "a clean ctest half's files are kept")

    def test_a_failed_route_fails(self):
        self.routes(["replay_clean", "replay_report marker 2", "replay_plain fail"])
        status, _ = self.run_runner()
        self.assertEqual(status, 1)
        rows = self.summary()
        self.assertEqual({name: rows[f"ctest:{name}"]["verdict"] for name in
                          ("replay_clean", "replay_report", "replay_plain")},
                         {"replay_clean": "clean", "replay_report": "failed", "replay_plain": "failed"})
        self.assertIn("misaligned address", rows["ctest:replay_report"]["reason"])
        self.assertIn("stopped after 40 frames", rows["ctest:replay_plain"]["reason"])
        self.assertTrue((self.work / strict_routes.CTEST_DIRECTORY / "junit.xml").exists())

    def test_a_skipped_route_is_a_gap(self):
        self.routes(["replay_clean", "replay_without_rom skip"])
        status, _ = self.run_runner()
        self.assertEqual(status, 77)
        row = self.summary()["ctest:replay_without_rom"]
        self.assertEqual((row["verdict"], row["status"]), ("skipped", "skipped"))

    def test_a_build_without_routes_fails(self):
        self.routes([])
        status, _ = self.run_runner()
        self.assertEqual(status, 1)
        self.assertIn("lists no test", self.summary()["ctest:routes"]["reason"])

    def test_only_selects_routes_apart_from_the_cases(self):
        self.routes(["replay_one", "replay_two"])
        status, _ = self.run_runner("--only", "replay_two")
        self.assertEqual(status, 0)
        self.assertIn(" -R ^(replay_two)$", self.ctest_ran())
        self.assertEqual(list(self.summary()), ["ctest:replay_two"])
        status, _ = self.run_runner("--only", "ctest:replay_one", "alone")
        self.assertEqual(status, 0)
        self.assertEqual(list(self.summary()), ["ctest:replay_one", "seq:alone"])
        (self.build / "ctest-env.txt").unlink()
        status, _ = self.run_runner("--only", "alone")
        self.assertEqual(status, 0)
        self.assertEqual(list(self.summary()), ["seq:alone"])
        self.assertFalse((self.build / "ctest-env.txt").exists(), "ctest ran for a case of the program's alone")
        self.assertEqual(self.run_runner("--only", "ctest:nothing")[0], 2)


class MarkerTests(unittest.TestCase):
    def test_the_markers_are_ctests(self):
        cmakelists = (PORT / "CMakeLists.txt").read_text(encoding="utf-8")
        self.assertEqual(list(strict_routes.MARKERS), fail_expressions(cmakelists))


class EnvironmentTests(World):
    """What reaches the program of the caller's sanitizer options and of the runner's"""

    def setUp(self):
        super().setUp()
        self.use_tables({"env": Sequence(40, 50, "", "")}, {}, {}, said={"env": ["print-env"]})

    def test_the_criterion_reaches_the_program(self):
        caller = {"ASAN_OPTIONS": "detect_leaks=0", "UBSAN_OPTIONS": "print_stacktrace=0",
                  "LSAN_OPTIONS": "verbosity=1"}
        status, printed = self.run_runner("--keep", environment=caller)
        self.assertEqual(status, 0)
        output = self.output("seq-env")
        self.assertIn(f"env ASAN_OPTIONS={CRITERION}\n", output)
        self.assertIn("env UBSAN_OPTIONS unset", output)
        self.assertIn("env LSAN_OPTIONS unset", output)
        self.assertIn(f"ASAN_OPTIONS: {CRITERION}\n", printed)
        self.assertIn("the criterion: yes", printed)
        for name, value in caller.items():
            self.assertIn(f"the caller's {name}={value} is dropped", printed)
        status, printed = self.run_runner("--keep", "--asan-extra", "halt_on_error=1")
        self.assertEqual(status, 0)
        self.assertIn(f"env ASAN_OPTIONS={CRITERION}:halt_on_error=1\n", self.output("seq-env"))
        self.assertIn("the criterion: no: --asan-extra halt_on_error=1", printed)
        status, printed = self.run_runner("--keep", "--no-stack-use-after-return")
        self.assertEqual(status, 0)
        self.assertIn("env ASAN_OPTIONS=detect_leaks=1\n", self.output("seq-env"))
        self.assertIn("the criterion: no: --no-stack-use-after-return", printed)

    def test_log_path_is_refused(self):
        status, _ = self.run_runner("--asan-extra", "log_path=reports/asan")
        self.assertEqual(status, 2)
        self.assertFalse(self.work.exists(), "a case ran")


class SummaryTests(World):
    def test_a_row_a_case_in_the_columns_order(self):
        self.assertEqual(strict_routes.SUMMARY_COLUMNS,
                         ("version", "case", "also", "start", "start_sha1", "input", "frames", "reached", "status",
                          "seconds", "verdict", "reason", "produced_sha1"))
        self.use_tables({"one": Sequence(40, 50, "", ""), "two": Sequence(30, 50, "", "", input="other")}, {}, {})
        status, _ = self.run_runner()
        self.assertEqual(status, 0)
        lines = (self.root / "summary.tsv").read_text(encoding="utf-8").splitlines()
        self.assertEqual(lines[0].split("\t"), list(strict_routes.SUMMARY_COLUMNS))
        self.assertEqual(len(lines), 4)
        rows = [line.split("\t") for line in lines[1:]]
        self.assertTrue(all(len(row) == len(strict_routes.SUMMARY_COLUMNS) for row in rows))
        route, one = (dict(zip(strict_routes.SUMMARY_COLUMNS, row)) for row in rows[:2])
        self.assertEqual({column: route[column] for column in route if column != "seconds"},
                         {"version": "b2_us", "case": "ctest:replay_clean", "also": "-", "start": "-",
                          "start_sha1": "-", "input": "-", "frames": "-", "reached": "40", "status": "passed",
                          "verdict": "clean", "reason": "-", "produced_sha1": "-"})
        self.assertEqual({column: one[column] for column in one if column != "seconds"},
                         {"version": "b2_us", "case": "seq:one", "also": "-", "start": "blank", "start_sha1": "-",
                          "input": "one_b2_us.pbwinput", "frames": "40", "reached": "40", "status": "0",
                          "verdict": "clean", "reason": "-", "produced_sha1": "-"})
        self.assertEqual(rows[2][1], "seq:two")


class DirectoryTests(unittest.TestCase):
    def test_directory_names_are_windows_safe(self):
        self.assertEqual(strict_routes.case_directory("trip:catch:make"), "trip-catch-make")
        plans = [strict_routes.plan_cases(version, include_all=True) for version in VERSIONS]
        cases = [case.name for plan in plans for case in plan.cases]
        for name in cases:
            with self.subTest(case=name):
                self.assertFalse(set(strict_routes.case_directory(name)) & set('<>:"/\\|?*'))


# Deliberate bugs in strict_routes.py, each the text it replaces (found exactly once there), the text that replaces it,
# and the tests, ClassName or ClassName.test_name, at least one of which must fail on it
MUTANTS = (
    ("the status not checked", "    if status != 0:\n        said =", "    if False:\n        said =",
     ("VerdictTests.test_verdicts",)),
    ("the markers not checked", "    marker = first_marker(lines)\n", "    marker = None\n",
     ("VerdictTests.test_verdicts",)),
    ("the frames reached not checked", "    if reached != case.frames:\n", "    if False:\n",
     ("VerdictTests.test_verdicts",)),
    ("the start save's SHA-1 not checked", "        if start_sha1 != case.kept:\n", "        if False:\n",
     ("SaveTests.test_a_changed_byte_fails_naming_both_sha1s",)),
    ("a missing save treated as clean", '            return Result(case, SKIPPED, f"no save {case.start} in',
     '            return Result(case, CLEAN, f"no save {case.start} in',
     ("SaveTests.test_a_missing_save_skips_and_prints_the_command",)),
    ("the produced save not passed on", "        start_path = make.produced\n", "        start_path = None\n",
     ("SaveTests.test_the_produced_save_reaches_the_continue_leg",)),
    ("PORT_SANITIZE_RECOVER accepted", '    if cmake_true(cache.get("PORT_SANITIZE_RECOVER")):\n', "    if False:\n",
     ("BuildTests.test_refusals",)),
    ("Apple clang accepted", '    if kind == "apple" and not allow_apple_clang:\n', "    if False:\n",
     ("BuildTests.test_apple_clang_is_refused",)),
    ("equal cases not merged", "    return (case.start, case.input)\n",
     "    return (case.start, case.input, case.name)\n",
     ("PlanTests.test_merges",)),
    ("the caller's ASAN_OPTIONS kept", '    environment["ASAN_OPTIONS"] = options\n',
     '    environment["ASAN_OPTIONS"] = dropped.get("ASAN_OPTIONS", options)\n',
     ("EnvironmentTests.test_the_criterion_reaches_the_program",)),
    ("the marker list one short", '    r"runtime error: ",\n', "",
     ("MarkerTests.test_the_markers_are_ctests", "VerdictTests.test_verdicts")),
    ("a merge across different inputs", "    return (case.start, case.input)\n", '    return (case.start, "")\n',
     ("PlanTests.test_merges",)),
    ("sequences with a reference not left out", "        if has_reference and not include_all and not named:\n",
     "        if False:\n", ("PlanTests.test_references",)),
    ("an unchanged produced save accepted", "            if produced_sha1 == start_sha1:\n", "            if False:\n",
     ("SaveTests.test_a_make_leg_that_saves_nothing_fails",)),
    ("the preflight skipped", "    reason = preflight(command, environment)\n", "    reason = None\n",
     ("BuildTests.test_a_program_without_asan_is_refused",)),
    ("':' kept in the directories' names", '    return name.replace(":", "-")\n', "    return name\n",
     ("DirectoryTests.test_directory_names_are_windows_safe",)),
    ("the function check not looked for", "    if not has_function_check(build.program):\n", "    if False:\n",
     ("BuildTests.test_a_program_without_the_function_check_is_refused",)),
    ("a trap stop its sequence allows accepted", "    if status != 0:\n        said =",
     '    if status == oracle.TRAP_STATUS and case.stop_from > 0:\n        return CLEAN, "", reached\n'
     "    if status != 0:\n        said =",
     ("VerdictTests.test_a_trap_stop_its_sequence_allows_fails",)),
    ("ctest's failures not counted", '        if failure is not None or test.get("status") == "fail":\n',
     "        if False:\n", ("CtestTests.test_a_failed_route_fails",)),
    ("ctest run without the criterion's options",
     "stderr=subprocess.STDOUT, env=settings.environment,\n                                cwd=work)",
     "stderr=subprocess.STDOUT, env=None,\n                                cwd=work)",
     ("CtestTests.test_the_routes_run_first_with_the_criterions_options",)),
    ("ctest's skips counted clean",
     '        elif skipped is not None or test.get("status") in ("notrun", "disabled"):\n', "        elif False:\n",
     ("CtestTests.test_a_skipped_route_is_a_gap",)),
    ("ctest's half not run", "    if tests or whole:\n", "    if False:\n",
     ("CtestTests.test_the_routes_run_first_with_the_criterions_options",)),
)


class MutationTests(unittest.TestCase):
    """Every row of MUTANTS: strict_routes.py with that one bug, loaded in its place, fails at least one of its tests;
    and the runner as it is passes all of them through the same loader, so that what fails is the bug"""

    @classmethod
    def setUpClass(cls):
        cls.source = RUNNER.read_text(encoding="utf-8")

    def load(self, source: str) -> types.ModuleType:
        """A module made from that source as strict_routes.py's, its path the runner's own"""
        module = types.ModuleType("strict_routes_mutant")
        module.__file__ = str(RUNNER)
        with mock.patch.object(sys, "path", list(sys.path)), \
                mock.patch.dict(sys.modules, {"strict_routes_mutant": module}):
            exec(compile(source, str(RUNNER), "exec"), module.__dict__)
        return module

    def run_tests(self, module: types.ModuleType, tests: tuple[str, ...], first_failure: bool) -> unittest.TestResult:
        """The tests named, with module as strict_routes, each class a subclass of its own; with first_failure, they
        stop at the first test or subtest that fails"""
        suite = unittest.TestSuite()
        for name in tests:
            class_name, _, method = name.partition(".")
            isolated = type(class_name, (globals()[class_name],), {"__module__": __name__})
            suite.addTests([isolated(method)] if method else unittest.defaultTestLoader.loadTestsFromTestCase(isolated))
        result = unittest.TestResult()
        result.failfast = first_failure
        with mock.patch.dict(globals(), {"strict_routes": module}):
            suite.run(result)
        return result

    def test_the_runner_as_it_is_passes_the_mutants_tests(self):
        tests = tuple(sorted({name for *_, names_ in MUTANTS for name in names_}))
        result = self.run_tests(self.load(self.source), tests, first_failure=False)
        self.assertEqual(result.testsRun, len(tests))
        self.assertTrue(result.wasSuccessful(), [str(test) for test, _ in result.failures + result.errors])

    def test_each_mutant_fails_its_tests(self):
        self.assertEqual(len({name for name, *_ in MUTANTS}), len(MUTANTS))
        for name, old, new, tests in MUTANTS:
            with self.subTest(mutant=name):
                self.assertEqual(self.source.count(old), 1, "the text a mutant replaces is in strict_routes.py once")
                self.assertNotEqual(old, new)
                result = self.run_tests(self.load(self.source.replace(old, new)), tests, first_failure=True)
                self.assertFalse(result.wasSuccessful(), f"strict_routes.py with this bug passes {', '.join(tests)}")


if __name__ == "__main__":
    unittest.main()
