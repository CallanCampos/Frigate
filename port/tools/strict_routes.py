#!/usr/bin/env python3
"""Runs T5's set in a strict build: the build made with PORT_SANITIZE (ASan and UBSan, every report fatal) by upstream
LLVM runs its own route tests that need no oracle (`ctest -L route -LE oracle`: the replays, the port's checks of the
oracle's hashes, the smoke test), and Frigate's program alone on each comparison sequence of
port/tools/oracle/oracle.py that has no `_hashes` reference for the version and on both of the port's legs of each
round trip, each from the save it starts from. Both halves run with the criterion's runtime options and are judged by
the criterion of ADR 0033 (port/docs/adr/, the sanitizer-clean replays of phase 5); port/docs/sanitizers.md says how
to run it.

Game functionality: what those routes play. ctest's replays, from power-on to the Aspertia City lookout (the opening,
the title, New Game, the first save, and CONTINUE from it to the starter's scene and its save); and from the oracle's
saves: CONTINUE into the field; the field menu and its apps (the bag, the trainer card, the party, a summary, the
Pokédex); Hugh's trainer battle; Route 19 and Bianca's capture demo; a wild Purrloin caught; the PC Box's storage
system; Floccesy Town to Route 20; the saves these make (the round trips' make legs) and the games continued from them
(their continue legs).

Its inputs: the strict build's CMakeCache.txt and its ctest tests; oracle.py's tables (SEQUENCES, SAVES, ROUND_TRIPS),
read and never changed, each kind of entry by one adapter function (sequence_case, trip_cases, kept_save), so that a
change of a table's shape changes that function alone; the input files, port/tests/oracle/<name>_<version>.pbwinput;
and the save cache, the oracle's saves as `oracle.py save` makes them (NAME_VERSION.sav, each held to the SHA-1 that
SAVES keeps), by default the build's oracle-saves. It never runs melonDS, never makes a save, and never writes into the
repository: its runs go in the build directory's strict-routes/ (or --work).

    strict_routes.py list [--version b2_us|w2_us] [--only NAME...] [--all]
    strict_routes.py run --build-dir DIR [--save-cache DIR] [--rom PATH] [--only NAME...] [--all] [--jobs N]
                         [--watchdog S] [--timeout S] [--work DIR] [--keep] [--summary FILE] [--asan-extra OPTS]
                         [--no-stack-use-after-return] [--allow-apple-clang]

The cases, per version:
  - ctest:NAME, each of the build's ctest tests labelled route and not oracle, which ctest runs and judges (in this
    build it fails a test whose output has a report), with its own timeout and the ROM the build was configured with;
  - seq:NAME, each sequence without a reference for the version (one with a reference, ctest's oracle_NAME_hashes runs
    in the same tree; --all, or NAME in --only, adds it): from its save (a blank one when it names none), its input
    file, its port frames;
  - trip:NAME:make, from the start of the save the round trip makes, the input file the port plays there, the trip's
    port frames: it keeps the save it leaves, and fails when it leaves none or leaves the start save unchanged;
  - trip:NAME:continue, from that save, the continued sequence's input file and frames, after its make leg; skipped
    when the make leg didn't pass.
Cases with the same start and the same input file run once, at the largest frame count, the others named in its `also`
(the same input from the same save makes the shorter run the start of the longer; a make leg is the one that runs, for
its save). A case whose start save has no SHA-1 for the version is left out with why, as a skip; a missing input file
fails its case. A sequence that allows the port a stop on a trap stub (stop_from) runs as any other, and such a stop
fails it: a trap leaves without the leak check. --only NAME selects NAME, seq:NAME and trip:NAME:*, a continue leg
bringing its make leg, and ctest's route test NAME (or ctest:NAME); without --only, both halves run whole.

A run first checks the build (exit 2 when it refuses: no PORT_SANITIZE, PORT_SANITIZE_RECOVER on, a ROM language other
than us, a build configured from another checkout, no program, Apple clang without --allow-apple-clang), that the
program runs under ASan (ASAN_OPTIONS=help=1 PROGRAM --help prints ASan's flags, else 2) and has UBSan's function check
(its handler, function_type_mismatch, among the program's symbols, else 2, or not the criterion with
--allow-apple-clang), and that the ROM is the decomp's (else 77). ctest's half runs first: `ctest --test-dir DIR -L
route -LE oracle --parallel N`, its output and JUnit file in ctest-routes/. Each other case runs in a fresh directory,
the start save copied in as the game code's .sav: PROGRAM --headless --frames N --watchdog S --rom ROM --save-dir D
--input FILE, its output in output.txt; --jobs at a time (1 by default), the longest first. Both halves run with
ASAN_OPTIONS=detect_leaks=1:detect_stack_use_after_return=1 (the second dropped by --no-stack-use-after-return); the
caller's ASAN_OPTIONS, UBSAN_OPTIONS and LSAN_OPTIONS are dropped, and --asan-extra is appended (never log_path). A
program-alone case is clean when the program exits with status 0, a line says `port: stopped after N frames` for its
frames, and no line matches the expressions on which ctest fails a test of the sanitized build (port/CMakeLists.txt).
A clean case's directory is deleted (but a make leg's save, until its continue leg ends) unless --keep; a failure
keeps everything. A build that isn't RelWithDebInfo, a compiler that isn't upstream LLVM (Apple's with
--allow-apple-clang, which has no LeakSanitizer, so add --asan-extra detect_leaks=0), --asan-extra and
--no-stack-use-after-return mark the run as not the criterion.

It prints a summary (the build, its compiler, the options, whether the run is the criterion's, the checkout's HEAD and
whether port/, src/, lib/ or include/ have changes, then a line a case) and with --summary writes it as a table, a row a
case: SUMMARY_COLUMNS. Exit status: 0 when every case is clean, 1 when any failed, 77 when none failed and some were
skipped or left out, 2 when it refused. Standard library only.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import mmap
import os
import re
import shutil
import signal
import subprocess
import sys
import time
from dataclasses import dataclass, field
from xml.etree import ElementTree

sys.dont_write_bytecode = True
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "oracle"))

import oracle  # noqa: E402

VERSIONS = ("b2_us", "w2_us")
GAMES = {"b2_us": "Black 2", "w2_us": "White 2"}
# The program each version's build makes (PORT_PROGRAM, port/CMakeLists.txt)
PROGRAMS = {"b2_us": "frigate-black2", "w2_us": "frigate-white2"}
# The runtime's options of the criterion (ADR 0033): LeakSanitizer at exit, which upstream LLVM has on macOS only when
# asked, and ASan's fake stacks, which find a use of a stack frame after it returned
LEAKS = "detect_leaks=1"
STACK_USE_AFTER_RETURN = "detect_stack_use_after_return=1"
CALLER_VARIABLES = ("ASAN_OPTIONS", "UBSAN_OPTIONS", "LSAN_OPTIONS")
# The expressions on which ctest fails every test of the sanitized build (FAIL_REGULAR_EXPRESSION at the end of
# port/CMakeLists.txt), which the test holds this list to
MARKERS = (
    r"ERROR: (Address|Leak)Sanitizer",
    r"SUMMARY: [A-Za-z]+Sanitizer",
    r"runtime error: ",
    r"LeakSanitizer has encountered a fatal error",
)
# The program's last line on a run that ran its frames (port/app/main.c), after the time and process SDL_Log puts
# first on some hosts
FRAMES_LINE = re.compile(r"port: stopped after (\d+) frames")
# What ASan's runtime prints first with help=1, before the program runs
ASAN_FLAG_LIST = "Available flags for AddressSanitizer"
# The handler that UBSan's function check calls, which upstream clang emits for C with -fsanitize=function and Apple
# clang never does (port/docs/lessons.md, 2026-10-18): on macOS, where the sanitizers' runtime is a library, a program
# names it among its symbols only as an import that its own code calls
FUNCTION_CHECK = b"function_type_mismatch"
# ctest's half: the build's route tests that need no oracle, their output and JUnit file in the work's ctest-routes/
CTEST_ROUTES = ("-L", "route", "-LE", "oracle")
CTEST_PREFIX = "ctest:"
CTEST_DIRECTORY = "ctest-routes"
CLEAN, FAILED, SKIPPED = "clean", "failed", "skipped"
REFUSED = 2
DEFAULT_WATCHDOG = 120
DEFAULT_TIMEOUT = 3600
REASON_LIMIT = 200
SUMMARY_COLUMNS = ("version", "case", "also", "start", "start_sha1", "input", "frames", "reached", "status", "seconds",
                   "verdict", "reason", "produced_sha1")
# The program's exit statuses that a reason names (port/app/main.c's table, port/docs/ARCHITECTURE.md)
STATUS_MEANINGS = {oracle.TRAP_STATUS: "a trap stub", 71: "OS_Terminate", 72: "a deadlock", 73: "the watchdog",
                   134: "an abort"}


class Refusal(Exception):
    """A reason the runner won't run: exit 2"""


@dataclass
class Case:
    """One run of the program, or one of ctest's route tests (ctest). start: the SAVES name of the save it starts from,
    "" for a blank save; a continue leg's is its make leg's name (after), whose save it continues. kept: the SHA-1 that
    SAVES keeps for that save in the version, "" when it keeps none. input: the input file's name,
    port/tests/oracle/<input>_<version>.pbwinput. also: the cases merged into this one. produces: a make leg, whose
    save a continue leg starts from. stop_from: the frame from which its sequence allows the port a trap stop"""
    name: str
    version: str
    start: str
    input: str
    frames: int
    kept: str = ""
    also: list[str] = field(default_factory=list)
    after: str = ""
    produces: bool = False
    stop_from: int = 0
    ctest: bool = False


@dataclass
class Plan:
    """A version's cases in order; the cases left out of it, each with why; and the sequences left to ctest's
    oracle_NAME_hashes"""
    version: str
    cases: list[Case]
    left_out: list[tuple[Case, str]]
    referenced: list[str]


@dataclass
class Result:
    case: Case
    verdict: str
    reason: str = ""
    status: str = "-"
    reached: int | None = None
    seconds: float = 0.0
    start_sha1: str = ""
    produced: str = ""
    produced_sha1: str = ""


@dataclass
class Build:
    directory: str
    version: str
    program: str
    compiler: str
    build_type: str
    not_criterion: list[str]
    ctest: str = "ctest"


@dataclass
class Settings:
    """How each case runs: the program as a command, the ROM and its game code, the save cache, the work directory,
    the watchdog and timeout, the program's environment, whether to keep clean cases' directories, and ctest as a
    command"""
    command: list[str]
    rom: str
    game_code: str
    cache: str
    work: str
    watchdog: int
    timeout: float
    environment: dict[str, str]
    keep: bool
    oracle_binary: str
    ctest: list[str] = field(default_factory=lambda: ["ctest"])


# The adapters: the only functions that read the tables' entries

def kept_save(name: str, version: str) -> tuple[str, str]:
    """The save SAVES[name] starts from and the SHA-1 it keeps for the version ("" for none; a name SAVES lacks has
    neither, and the plan refuses it)"""
    maker = oracle.SAVES.get(name)
    if maker is None:
        return "", ""
    return maker.start, maker.sha1.get(version, "")


def sequence_case(name: str, version: str) -> tuple[Case, bool]:
    """The case of the sequence SEQUENCES[name] in a version, and whether the version has its reference"""
    sequence = oracle.SEQUENCES[name]
    kept = kept_save(sequence.save, version)[1] if sequence.save else ""
    case = Case(f"seq:{name}", version, sequence.save, sequence.input or name, sequence.port_frames, kept=kept,
                stop_from=sequence.stop_from)
    reference = os.path.join(oracle.REPO_ROOT, oracle.SEQUENCE_REFERENCE.format(name=name, version=version))
    return case, os.path.exists(reference)


def trip_cases(name: str, version: str) -> tuple[Case, Case]:
    """The port's legs of the round trip ROUND_TRIPS[name] in a version: the make leg, from the start of the save the
    trip makes, with the input file the port plays (oracle.py's command_round_trip), and the continue leg, from the
    save the make leg leaves, with the continued sequence's input file and frames"""
    trip = oracle.ROUND_TRIPS[name]
    start = kept_save(trip.maker, version)[0]
    kept = kept_save(start, version)[1] if start else ""
    make = Case(f"trip:{name}:make", version, start, trip.port_input.get(version, trip.maker), trip.port_frames,
                kept=kept, produces=True)
    continued = oracle.SEQUENCES[trip.continued]
    resume = Case(f"trip:{name}:continue", version, make.name, trip.continued, continued.port_frames,
                  after=make.name, stop_from=continued.stop_from)
    return make, resume


# The plan

def selects(name: str, wanted: str) -> bool:
    """Whether --only's NAME selects the case named name"""
    return name in (wanted, f"seq:{wanted}") or name.startswith(f"trip:{wanted}:")


def merge_key(case: Case) -> tuple[str, ...]:
    """Cases with the same key run once: the same start and the same input file"""
    return (case.start, case.input)


def merge_pass(cases: list[Case], into: dict[str, str]) -> list[Case]:
    """One pass of merged(): the cases that run, each merged one's name mapped in into to the case that runs it"""
    groups: dict[tuple[str, ...], list[Case]] = {}
    for case in cases:
        groups.setdefault(merge_key(case), []).append(case)
    kept: list[Case] = []
    for case in cases:
        group = groups[merge_key(case)]
        makers = [member for member in group if member.produces]
        runner = makers[0] if makers else max(group, key=lambda member: member.frames)
        if case is not runner:
            into[case.name] = runner.name
            continue
        for member in group:
            if member is not runner:
                runner.also.extend([member.name] + member.also)
        runner.frames = max(member.frames for member in group)
        kept.append(case)
    return kept


def merged(cases: list[Case]) -> list[Case]:
    """The cases with the same start and input file run once, at the largest frame count: a make leg runs when one is
    among them, else the longest (the first of the longest); the others go in its also. A continue leg after a make
    leg merged into another follows that one, and then merges with a continue leg like it"""
    into: dict[str, str] = {}
    kept = merge_pass(cases, into)
    for case in kept:
        if case.after in into:
            case.after = into[case.after]
            case.start = case.after
    return merge_pass(kept, into)


def plan_cases(version: str, only: list[str] | None = None, include_all: bool = False) -> Plan:
    """The version's cases (those only selects, when given): refuses (Refusal) a name in only that selects nothing"""
    cases: list[Case] = []
    referenced: list[str] = []
    for name in oracle.SEQUENCES:
        case, has_reference = sequence_case(name, version)
        named = bool(only) and any(selects(case.name, wanted) for wanted in only)
        if has_reference and not include_all and not named:
            referenced.append(name)
            continue
        cases.append(case)
    for name in oracle.ROUND_TRIPS:
        cases.extend(trip_cases(name, version))
    if only:
        unknown = [wanted for wanted in only if not any(selects(case.name, wanted) for case in cases)]
        if unknown:
            raise Refusal(f"no case is named {', '.join(unknown)} (strict_routes.py list shows them, and ctest -N -L "
                          "route -LE oracle in the build ctest's)")
        chosen = {case.name for case in cases if any(selects(case.name, wanted) for wanted in only)}
        chosen |= {case.after for case in cases if case.name in chosen and case.after}
        cases = [case for case in cases if case.name in chosen]
        referenced = []
    left_out: list[tuple[Case, str]] = []
    runnable: list[Case] = []
    gone: dict[str, str] = {}
    for case in cases:
        reason = None
        if case.after:
            if case.after in gone:
                reason = f"its make leg is left out: {gone[case.after]}"
        elif case.start:
            try:
                reason = oracle.save_kept(case.start, version)
            except ValueError as error:
                raise Refusal(f"oracle.py's SAVES: {error}") from error
        if reason:
            gone[case.name] = reason
            left_out.append((case, reason))
        else:
            runnable.append(case)
    return Plan(version, merged(runnable), left_out, referenced)


def case_directory(name: str) -> str:
    """A case's directory, named after it with "-" for ":", which Windows refuses in a name"""
    return name.replace(":", "-")


def input_path(case: Case) -> str:
    return os.path.join(oracle.REPO_ROOT, oracle.SEQUENCE_INPUT.format(name=case.input, version=case.version))


# The build

def cmake_true(value: str | None) -> bool:
    """CMake's truth of a cache value: 1, ON, YES, TRUE, Y or a number other than 0"""
    if value is None:
        return False
    text = value.strip().upper()
    if text in ("1", "ON", "YES", "TRUE", "Y"):
        return True
    try:
        return float(text) != 0
    except ValueError:
        return False


def read_cache(directory: str) -> dict[str, str]:
    """A CMake build directory's CMakeCache.txt, NAME to value; a Refusal when it has none"""
    path = os.path.join(directory, "CMakeCache.txt")
    if not os.path.isfile(path):
        raise Refusal(f"{directory} has no CMakeCache.txt: configure the strict build there first")
    entries = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\r\n")
            if not line or line.startswith(("#", "//")):
                continue
            match = re.match(r"([^:=]+)(?::[^=]*)?=(.*)$", line)
            if match:
                entries[match.group(1)] = match.group(2)
    return entries


def same_path(a: str, b: str) -> bool:
    return os.path.normcase(os.path.realpath(a)) == os.path.normcase(os.path.realpath(b))


def compiler_version(compiler: str) -> str:
    """What the compiler says to --version; a Refusal when it doesn't run"""
    try:
        done = subprocess.run([compiler, "--version"], capture_output=True, text=True, errors="replace", timeout=120)
    except (OSError, subprocess.SubprocessError) as error:
        raise Refusal(f"the build's compiler {compiler} doesn't run: {error}") from error
    if done.returncode != 0:
        raise Refusal(f"the build's compiler {compiler} --version exits with status {done.returncode}")
    return done.stdout + done.stderr


def classify_compiler(text: str) -> str:
    """A compiler by its --version text's first line: "apple" for Apple clang, which emits no function check for C and
    has no LeakSanitizer (port/docs/sanitizers.md), "llvm" for upstream clang (Homebrew's, a distribution's, LLVM's
    own), "other" for anything else"""
    lines = text.strip().splitlines()
    first = lines[0] if lines else ""
    if first.startswith("Apple clang"):
        return "apple"
    if re.search(r"\bclang version \d+", first):
        return "llvm"
    return "other"


def inspect_build(directory: str, allow_apple_clang: bool) -> Build:
    """The strict build in directory, checked (a Refusal when it isn't one this runner runs), with what makes a run of
    it not the criterion's. Its directory and program are whole paths, since each case runs in a directory of its own"""
    directory = os.path.abspath(directory)
    cache = read_cache(directory)
    home = cache.get("CMAKE_HOME_DIRECTORY", "")
    port = os.path.join(oracle.REPO_ROOT, "port")
    if not home or not same_path(home, port):
        raise Refusal(f"{directory} was configured from {home or 'an unknown source'}, not this checkout's {port}")
    if not cmake_true(cache.get("PORT_SANITIZE")):
        raise Refusal(f"{directory} isn't a sanitized build (PORT_SANITIZE isn't ON)")
    if cmake_true(cache.get("PORT_SANITIZE_RECOVER")):
        raise Refusal(f"{directory} is a survey build (PORT_SANITIZE_RECOVER is ON), which goes on after a report")
    language = cache.get("PORT_ROM_LANGUAGE", "")
    if language and language != "us":
        raise Refusal(f"{directory} plays the {language} ROM; the oracle's saves and input files are the us ROM's")
    version = cache.get("PORT_VERSION", "")
    if version not in PROGRAMS:
        raise Refusal(f"{directory} has no PORT_VERSION this runner knows ({version or 'none'})")
    program = os.path.join(directory, PROGRAMS[version] + (".exe" if os.name == "nt" else ""))
    if not os.path.isfile(program):
        raise Refusal(f"{directory} has no {os.path.basename(program)}: build it first")
    ctest = cache.get("CMAKE_CTEST_COMMAND", "") or shutil.which("ctest") or ""
    if not ctest:
        raise Refusal(f"{directory}'s cache names no ctest (CMAKE_CTEST_COMMAND), and none is on PATH")
    compiler = cache.get("CMAKE_C_COMPILER", "")
    if not compiler:
        raise Refusal(f"{directory}'s cache names no C compiler")
    text = compiler_version(compiler)
    first = (text.strip().splitlines() or [""])[0]
    not_criterion = []
    kind = classify_compiler(text)
    if kind == "apple" and not allow_apple_clang:
        raise Refusal(f"{directory} was built by Apple clang ({first}), which has no function check for C and no "
                      "LeakSanitizer: build with -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang, or run with "
                      "--allow-apple-clang, which isn't the criterion")
    if kind != "llvm":
        not_criterion.append(f"the compiler isn't upstream LLVM ({first})")
    build_type = cache.get("CMAKE_BUILD_TYPE", "")
    if build_type != "RelWithDebInfo":
        not_criterion.append(f"CMAKE_BUILD_TYPE is {build_type or 'empty'}, not RelWithDebInfo")
    return Build(directory, version, program, first, build_type, not_criterion, ctest)


# The program's environment and the preflight

def asan_options(stack_use_after_return: bool, extra: str) -> str:
    options = [LEAKS] + ([STACK_USE_AFTER_RETURN] if stack_use_after_return else [])
    return ":".join(options + ([extra] if extra else []))


def program_environment(base: dict[str, str], options: str) -> tuple[dict[str, str], dict[str, str]]:
    """The program's environment: base without the caller's sanitizer options, and ASAN_OPTIONS options; with the
    dropped variables"""
    environment = dict(base)
    dropped = {name: environment.pop(name) for name in CALLER_VARIABLES if name in environment}
    environment["ASAN_OPTIONS"] = options
    return environment, dropped


def preflight(command: list[str], environment: dict[str, str]) -> str | None:
    """None when the program runs under ASan's runtime: with ASAN_OPTIONS=help=1 it prints ASan's flags and its --help
    exits 0; else why not"""
    asked = dict(environment)
    asked["ASAN_OPTIONS"] = "help=1"
    try:
        done = subprocess.run(command + ["--help"], capture_output=True, text=True, errors="replace", env=asked,
                              timeout=120)
    except (OSError, subprocess.SubprocessError) as error:
        return f"the program doesn't run: {error}"
    if done.returncode != 0:
        return f"ASAN_OPTIONS=help=1 {command[-1]} --help exits with status {done.returncode}"
    if ASAN_FLAG_LIST not in done.stdout + done.stderr:
        return f"ASAN_OPTIONS=help=1 {command[-1]} --help prints no ASan flags: the program isn't built with ASan"
    return None


def has_function_check(program: str) -> bool:
    """Whether the program's file names the function check's handler (FUNCTION_CHECK), as its symbols do when its
    compiler emitted the check"""
    try:
        with open(program, "rb") as f:
            if os.fstat(f.fileno()).st_size == 0:
                return False
            with mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
                return data.find(FUNCTION_CHECK) >= 0
    except (OSError, ValueError):
        return False


# ctest's half

def ctest_routes(command: list[str], directory: str, environment: dict[str, str]) -> list[str]:
    """The build's route tests that need no oracle, by name in ctest's order (ctest -N); a Refusal when ctest can't
    list them"""
    try:
        done = subprocess.run(command + ["--test-dir", directory, "-N", *CTEST_ROUTES], capture_output=True,
                              text=True, errors="replace", env=environment, timeout=300)
    except (OSError, subprocess.SubprocessError) as error:
        raise Refusal(f"ctest doesn't run: {error}") from error
    if done.returncode != 0:
        raise Refusal(f"ctest -N in {directory} exits with status {done.returncode}")
    return re.findall(r"^\s*Test\s+#\d+: (\S+)", done.stdout, re.MULTILINE)


def split_only(only: list[str], routes: list[str]) -> tuple[list[str], list[str]]:
    """--only's names: those of ctest's route tests (NAME or ctest:NAME), and the others, the program-alone cases'"""
    tests: list[str] = []
    others: list[str] = []
    for name in only:
        bare = name[len(CTEST_PREFIX):] if name.startswith(CTEST_PREFIX) else name
        if bare in routes:
            tests.append(bare)
        else:
            others.append(name)
    return tests, others


def ctest_case(name: str, version: str) -> Case:
    return Case(f"{CTEST_PREFIX}{name}", version, "", "", 0, ctest=True)


def ctest_results(path: str, version: str, tests: list[str], status: int) -> list[Result]:
    """ctest's verdicts from its JUnit file: a test it passed is clean, one it failed fails (with its first report's
    line, else its last `port:` line), and one it didn't run is skipped; a test it listed and has no result for fails,
    and so does ctest itself when it exits with a status other than 0 and no test failed"""
    results: list[Result] = []
    try:
        root = ElementTree.parse(path).getroot()
    except (OSError, ElementTree.ParseError) as error:
        return [Result(ctest_case("routes", version), FAILED,
                       short(f"ctest exited with {status_text(status)} and wrote no results: {error}"))]
    for test in root.iter("testcase"):
        case = ctest_case(test.get("name", ""), version)
        lines = (test.findtext("system-out") or "").splitlines()
        seconds = float(test.get("time") or 0)
        failure = test.find("failure")
        skipped = test.find("skipped")
        if failure is not None or test.get("status") == "fail":
            message = failure.get("message", "Failed") if failure is not None else "Failed"
            said = [line[line.index("port: "):] for line in lines if "port: " in line]
            detail = first_marker(lines) or (said[-1] if said else "")
            results.append(Result(case, FAILED, short(f"ctest: {message}" + (f": {detail}" if detail else "")),
                                  message.lower(), frames_reached(lines), seconds))
        elif skipped is not None or test.get("status") in ("notrun", "disabled"):
            message = skipped.get("message", "") if skipped is not None else test.get("status", "")
            results.append(Result(case, SKIPPED, short(f"ctest didn't run it ({message})"), "skipped", None, seconds))
        else:
            results.append(Result(case, CLEAN, "", "passed", frames_reached(lines), seconds))
    found = {result.case.name for result in results}
    results += [Result(ctest_case(test, version), FAILED, "ctest has no result for it") for test in tests
                if f"{CTEST_PREFIX}{test}" not in found]
    if status != 0 and all(result.verdict != FAILED for result in results):
        results.append(Result(ctest_case("routes", version), FAILED, f"ctest exited with {status_text(status)}"))
    return results


def run_ctest(tests: list[str], whole: bool, settings: Settings, directory: str, version: str,
              jobs: int) -> list[Result]:
    """ctest's half in the build in directory: every route test that needs no oracle (whole), or the tests named,
    jobs at a time, with the program's environment; a Result a test. Its output and JUnit file go in the work's
    ctest-routes/, deleted when every test passed unless --keep"""
    if not tests:
        return [Result(ctest_case("routes", version), FAILED,
                       f"ctest lists no test labelled route and not oracle in {directory}")]
    work = os.path.join(settings.work, CTEST_DIRECTORY)
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    junit = os.path.join(work, "junit.xml")
    command = settings.ctest + ["--test-dir", directory, *CTEST_ROUTES, "--parallel", str(jobs), "--output-on-failure",
                                "--output-junit", junit]
    if not whole:
        command += ["-R", "^(" + "|".join(re.escape(test) for test in tests) + ")$"]
    with open(os.path.join(work, "output.txt"), "wb") as output:
        status = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, env=settings.environment,
                                cwd=work).returncode
    results = ctest_results(junit, version, tests, status)
    if all(result.verdict == CLEAN for result in results) and not settings.keep:
        remove(work)
    return results


# A case's run and verdict

def status_text(status: int) -> str:
    if status < 0:
        try:
            return f"signal {-status} ({signal.Signals(-status).name})"
        except ValueError:
            return f"signal {-status}"
    meaning = STATUS_MEANINGS.get(status)
    return f"status {status}" + (f" ({meaning})" if meaning else "")


def first_marker(lines: list[str]) -> str | None:
    for line in lines:
        if any(re.search(marker, line) for marker in MARKERS):
            return line.strip()
    return None


def frames_reached(lines: list[str]) -> int | None:
    """The largest N of the program's `port: stopped after N frames` lines"""
    found = [int(match.group(1)) for line in lines for match in [FRAMES_LINE.search(line)] if match]
    return max(found) if found else None


def short(text: str) -> str:
    text = " ".join(text.split())
    return text if len(text) <= REASON_LIMIT else text[:REASON_LIMIT - 3] + "..."


def judge(case: Case, status: int | None, output: str, timeout: float) -> tuple[str, str, int | None]:
    """A finished run's verdict, its reason (at most REASON_LIMIT characters), and the frames it says it ran; status
    None for a run killed at the timeout"""
    lines = output.splitlines()
    reached = frames_reached(lines)
    marker = first_marker(lines)
    ending = f"timeout after {timeout:g} s" if status is None else status_text(status)
    if marker is not None:
        return FAILED, short(f"{ending}: {marker}"), reached
    if status is None:
        return FAILED, short(f"{ending}, killed"), reached
    if status != 0:
        said = [line[line.index("port: "):] for line in lines if "port: " in line]
        allowed = f", a stop its sequence allows from frame {case.stop_from}" if status == oracle.TRAP_STATUS and \
            case.stop_from > 0 else ""
        return FAILED, short(f"{ending}{allowed}" + (f": {said[-1]}" if said else "")), reached
    if reached != case.frames:
        what = f"stopped after {reached} of {case.frames} frames" if reached is not None else \
            f"no line says it stopped after its {case.frames} frames"
        return FAILED, short(what), reached
    return CLEAN, "", reached


def make_command(settings: Settings, name: str, version: str) -> str:
    """The command that makes a missing save into the cache, on the oracle"""
    script = os.path.join(oracle.REPO_ROOT, "port", "tools", "oracle", "oracle.py")
    return (f"{script} save {name} --version {version} --save-cache {settings.cache} "
            f"--oracle {settings.oracle_binary}")


def run_case(case: Case, settings: Settings, finished: dict[str, Result]) -> Result:
    """Runs one case in a fresh directory of settings.work and judges it; a make leg's save stays there as produced.sav
    for its continue leg"""
    directory = os.path.join(settings.work, case_directory(case.name))
    shutil.rmtree(directory, ignore_errors=True)
    path = input_path(case)
    if not os.path.exists(path):
        return Result(case, FAILED, short(f"no input file {os.path.relpath(path, oracle.REPO_ROOT)}"))
    start_path = None
    start_sha1 = ""
    if case.after:
        make = finished.get(case.after)
        if make is None or make.verdict != CLEAN or not make.produced:
            return Result(case, SKIPPED, "its make leg failed" if make and make.verdict == FAILED else
                          "its make leg didn't run")
        start_path = make.produced
        start_sha1 = make.produced_sha1
    elif case.start:
        start_path = oracle.cached_save(settings.cache, case.start, case.version)
        if not os.path.exists(start_path):
            command = make_command(settings, case.start, case.version)
            return Result(case, SKIPPED, f"no save {case.start} in {settings.cache}; make it with: {command}")
        start_sha1 = oracle.file_sha1(start_path)
        if start_sha1 != case.kept:
            return Result(case, FAILED, f"the cache's {case.start} has SHA-1 {start_sha1}, not {case.kept}, which "
                          "SAVES keeps", start_sha1=start_sha1)
    saves = os.path.join(directory, "saves")
    os.makedirs(saves)
    save_file = os.path.join(saves, f"{settings.game_code}.sav")
    if start_path:
        shutil.copyfile(start_path, save_file)
    command = settings.command + ["--headless", "--frames", str(case.frames), "--watchdog", str(settings.watchdog),
                                  "--rom", settings.rom, "--save-dir", saves, "--input", path]
    output_path = os.path.join(directory, "output.txt")
    began = time.monotonic()
    with open(output_path, "wb") as output:
        process = subprocess.Popen(command, stdout=output, stderr=subprocess.STDOUT, env=settings.environment,
                                   cwd=directory, start_new_session=os.name != "nt")
        try:
            status = process.wait(timeout=settings.timeout)
        except subprocess.TimeoutExpired:
            status = None
        finally:
            if process.poll() is None:
                stop(process)
    seconds = time.monotonic() - began
    with open(output_path, encoding="utf-8", errors="replace") as f:
        verdict, reason, reached = judge(case, status, f.read(), settings.timeout)
    result = Result(case, verdict, reason, "timeout" if status is None else str(status) if status >= 0 else
                    f"signal {-status}", reached, seconds, start_sha1)
    if case.produces and verdict == CLEAN:
        if not os.path.exists(save_file):
            result.verdict, result.reason = FAILED, "the run left no save for its continue leg"
        else:
            produced_sha1 = oracle.file_sha1(save_file)
            if produced_sha1 == start_sha1:
                result.verdict, result.reason = FAILED, "the run left the start save unchanged: it saved nothing"
            else:
                result.produced = os.path.join(directory, "produced.sav")
                shutil.move(save_file, result.produced)
                result.produced_sha1 = produced_sha1
    if result.verdict == CLEAN and not settings.keep:
        for name in os.listdir(directory):
            if os.path.join(directory, name) != result.produced:
                remove(os.path.join(directory, name))
        if not result.produced:
            remove(directory)
    return result


def stop(process: subprocess.Popen) -> None:
    """Kills a run and everything it started (ASan's symbolizer), on POSIX its whole session"""
    try:
        if os.name != "nt":
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
    except OSError:
        pass
    process.wait()


def remove(path: str) -> None:
    if os.path.isdir(path):
        shutil.rmtree(path, ignore_errors=True)
    elif os.path.exists(path):
        os.remove(path)


def run_plan(plan: Plan, settings: Settings, jobs: int, report=None) -> list[Result]:
    """Every case of the plan, jobs at a time, the longest ready first, a continue leg once its make leg has ended;
    report(result) as each ends. The results in the plan's order"""
    finished: dict[str, Result] = {}
    pending = list(plan.cases)
    # A make leg's save stays until every continue leg after it has ended clean
    waiting = {case.name: sum(1 for later in plan.cases if later.after == case.name) for case in plan.cases}
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        running: dict[concurrent.futures.Future, Case] = {}
        while pending or running:
            ready = sorted((case for case in pending if not case.after or case.after in finished),
                           key=lambda case: -case.frames)
            if not ready and not running:
                for case in pending:
                    finished[case.name] = Result(case, SKIPPED, "its make leg isn't in the plan")
                break
            for case in ready[:jobs - len(running)]:
                pending.remove(case)
                running[pool.submit(run_case, case, settings, dict(finished))] = case
            done, _ = concurrent.futures.wait(running, return_when=concurrent.futures.FIRST_COMPLETED)
            for future in done:
                case = running.pop(future)
                try:
                    result = future.result()
                except Exception as error:  # noqa: BLE001: a case that can't run is a failure, not the runner's end
                    result = Result(case, FAILED, short(f"the runner couldn't run it: {error}"))
                finished[case.name] = result
                make = finished.get(case.after) if case.after else None
                if make and make.produced and result.verdict == CLEAN and not settings.keep:
                    waiting[make.case.name] -= 1
                    if waiting[make.case.name] == 0:
                        remove(os.path.dirname(make.produced))
                if report:
                    report(result)
    return [finished[case.name] for case in plan.cases]


# The summary

def start_label(case: Case) -> str:
    if case.ctest:
        return "-"
    if case.after:
        return f"{case.after}'s save"
    return case.start or "blank"


def input_label(case: Case) -> str:
    if case.ctest:
        return "-"
    return os.path.basename(oracle.SEQUENCE_INPUT.format(name=case.input, version=case.version))


def case_line(result: Result) -> str:
    case = result.case
    also = f" (also {', '.join(case.also)})" if case.also else ""
    if result.verdict == SKIPPED or result.status == "-":
        return f"  {result.verdict:<8} {case.name}{also}: {result.reason}"
    if case.ctest:
        line = f"  {result.verdict:<8} {case.name}: {result.status}, {result.seconds:.1f} s"
        return line + (f": {result.reason}" if result.reason else "")
    reached = "-" if result.reached is None else f"{result.reached:,}"
    line = f"  {result.verdict:<8} {case.name}{also}: {reached} of {case.frames:,} frames, {result.seconds:.1f} s"
    return line + (f": {result.reason}" if result.reason else "")


def summary_row(result: Result) -> list[str]:
    case = result.case
    return [case.version, case.name, ",".join(case.also) or "-", start_label(case), result.start_sha1[:12] or "-",
            input_label(case), "-" if case.ctest else str(case.frames),
            "-" if result.reached is None else str(result.reached), result.status, f"{result.seconds:.1f}",
            result.verdict, short(result.reason) or "-", result.produced_sha1[:12] or "-"]


def write_summary(path: str, results: list[Result]) -> None:
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\t".join(SUMMARY_COLUMNS) + "\n")
        for result in results:
            f.write("\t".join(summary_row(result)) + "\n")


def checkout_state() -> str:
    """The checkout's HEAD, and which of port/, src/, lib/ and include/ have changes"""
    root = oracle.REPO_ROOT
    try:
        head = subprocess.run(["git", "-C", root, "rev-parse", "HEAD"], capture_output=True, text=True, timeout=120)
        status = subprocess.run(["git", "-C", root, "status", "--porcelain", "--", "port", "src", "lib", "include"],
                                capture_output=True, text=True, timeout=300)
    except (OSError, subprocess.SubprocessError):
        return "unknown (git doesn't run)"
    if head.returncode != 0 or status.returncode != 0:
        return "unknown (not a git checkout)"
    dirty = sorted({line[3:].split("/")[0].strip('"') + "/" for line in status.stdout.splitlines() if line[3:]})
    return f"{head.stdout.strip()}, " + (f"changes in {', '.join(dirty)}" if dirty else
                                          "port/, src/, lib/ and include/ clean")


def load_text() -> str:
    return f"load {os.getloadavg()[0]:.1f}" if hasattr(os, "getloadavg") else "load unknown"


def plan_frames(plan: Plan) -> int:
    return sum(case.frames for case in plan.cases)


# The commands

def command_list(args) -> int:
    for version in [args.version] if args.version else VERSIONS:
        try:
            plan = plan_cases(version, args.only, args.all)
        except Refusal as refusal:
            print(f"strict_routes: refused: {refusal}", file=sys.stderr)
            return REFUSED
        print(f"{GAMES[version]} ({version}): {len(plan.cases)} cases, {plan_frames(plan):,} frames, after ctest's "
              "route tests that need no oracle (ctest -N -L route -LE oracle in the build lists them)")
        for case in plan.cases:
            also = f", also {', '.join(case.also)}" if case.also else ""
            print(f"  {case.name}: from {start_label(case)}, {input_label(case)}, {case.frames:,} frames{also}")
        for case, reason in plan.left_out:
            print(f"  left out: {case.name}: {reason}")
        if plan.referenced:
            print(f"  left to ctest's oracle_<name>_hashes: {', '.join(plan.referenced)}")
    return 0


def command_run(args, launcher=None) -> int:
    if args.asan_extra and "log_path" in args.asan_extra:
        print("strict_routes: refused: --asan-extra takes no log_path, which moves the reports out of the output "
              "the verdict reads", file=sys.stderr)
        return REFUSED
    try:
        build = inspect_build(args.build_dir, args.allow_apple_clang)
    except Refusal as refusal:
        print(f"strict_routes: refused: {refusal}", file=sys.stderr)
        return REFUSED
    not_criterion = list(build.not_criterion)
    if args.asan_extra:
        not_criterion.append(f"--asan-extra {args.asan_extra}")
    if args.no_stack_use_after_return:
        not_criterion.append("--no-stack-use-after-return")
    options = asan_options(not args.no_stack_use_after_return, args.asan_extra or "")
    environment, dropped = program_environment(dict(os.environ), options)
    for name, value in dropped.items():
        print(f"strict_routes: the caller's {name}={value} is dropped: the runs have the criterion's options",
              file=sys.stderr)
    command = launcher(build.program) if launcher else [build.program]
    ctest = launcher(build.ctest) if launcher else [build.ctest]
    reason = preflight(command, environment)
    if reason:
        print(f"strict_routes: refused: {reason}", file=sys.stderr)
        return REFUSED
    if not has_function_check(build.program):
        reason = (f"{build.program} has no function check: no {FUNCTION_CHECK.decode()} among its symbols, so its "
                  "compiler emitted none (Apple clang emits none for C)")
        if not args.allow_apple_clang:
            print(f"strict_routes: refused: {reason}: rebuild it with upstream clang", file=sys.stderr)
            return REFUSED
        not_criterion.append("the program has no function check")
    try:
        routes = ctest_routes(ctest, build.directory, environment)
        whole = not args.only
        tests, wanted = split_only(args.only, routes) if args.only else (routes, [])
        plan = plan_cases(build.version, wanted or None, args.all) if whole or wanted else \
            Plan(build.version, [], [], [])
    except Refusal as refusal:
        print(f"strict_routes: refused: {refusal}", file=sys.stderr)
        return REFUSED
    rom = oracle.rom_path(build.version, args.rom)
    reason = oracle.rom_matches(build.version, rom)
    if reason:
        print(f"strict_routes: skipped: {reason}")
        return oracle.SKIP
    work = os.path.abspath(args.work or os.path.join(build.directory, "strict-routes"))
    binary = os.path.join(build.directory, "oracle", "pbw-oracle")
    settings = Settings(command, os.path.abspath(rom), oracle.rom_game_code(rom),
                        os.path.abspath(args.save_cache or os.path.join(build.directory, "oracle-saves")), work,
                        args.watchdog, args.timeout, environment, args.keep,
                        binary if os.path.exists(binary) else "PBW_ORACLE", ctest)
    os.makedirs(work, exist_ok=True)
    began = time.monotonic()
    print(f"strict_routes: {GAMES[build.version]} ({build.version}), the build {build.directory}")
    print(f"  program: {build.program}")
    print(f"  compiler: {build.compiler}")
    print(f"  build type: {build.build_type or 'none'}")
    print(f"  ASAN_OPTIONS: {options}")
    print("  the criterion: " + ("yes" if not not_criterion else f"no: {'; '.join(not_criterion)}"))
    print(f"  checkout: {checkout_state()}")
    print(f"  ctest's route tests that need no oracle: {len(tests)}" + (f" of {len(routes)}" if not whole else "")
          + f", {args.jobs} at a time ({build.ctest})")
    print(f"  cases: {len(plan.cases)}, {plan_frames(plan):,} frames, {len(plan.left_out)} left out; "
          f"{args.jobs} at a time; save cache {settings.cache}; work {work}")
    print(f"  started {time.strftime('%Y-%m-%d %H:%M:%S')}, {load_text()}", flush=True)

    def report(result: Result) -> None:
        print(f"  [{time.strftime('%H:%M:%S')}]" + case_line(result)[1:], flush=True)

    results: list[Result] = []
    if tests or whole:
        for result in run_ctest(tests, whole, settings, build.directory, build.version, args.jobs):
            report(result)
            results.append(result)
    results += [Result(case, SKIPPED, reason) for case, reason in plan.left_out]
    results += run_plan(plan, settings, args.jobs, report)
    print(f"  ended {time.strftime('%Y-%m-%d %H:%M:%S')}, {load_text()}, after {time.monotonic() - began:.0f} s")
    print("summary:")
    for result in results:
        print(case_line(result))
    counts = {verdict: sum(1 for result in results if result.verdict == verdict) for verdict in (CLEAN, FAILED,
                                                                                                    SKIPPED)}
    print(f"{len(results)} cases: {counts[CLEAN]} clean, {counts[FAILED]} failed, {counts[SKIPPED]} skipped"
          + ("" if not not_criterion else "; not the criterion"), flush=True)
    if args.summary:
        write_summary(args.summary, results)
    if counts[FAILED]:
        return 1
    return oracle.SKIP if counts[SKIPPED] else 0


def positive(text: str) -> float:
    value = float(text)
    if value <= 0:
        raise argparse.ArgumentTypeError(f"{text} isn't above 0")
    return value


def job_count(text: str) -> int:
    value = int(text)
    if not 1 <= value <= 64:
        raise argparse.ArgumentTypeError(f"{text} isn't from 1 to 64")
    return value


def main(argv: list[str] | None = None, launcher=None) -> int:
    """The command line; launcher, for the tests, gives the command that starts a program found at a path (the
    build's program, or its ctest)"""
    parser = argparse.ArgumentParser(description="T5's set in a strict build: ctest's routes and the program alone "
                                                 "from the oracle's saves.")
    commands = parser.add_subparsers(dest="command", required=True)
    listing = commands.add_parser("list", help="the program-alone cases of each version, without running them")
    listing.add_argument("--version", choices=VERSIONS)
    running = commands.add_parser("run", help="run ctest's routes and the cases on a strict build")
    running.add_argument("--build-dir", required=True)
    running.add_argument("--save-cache")
    running.add_argument("--rom")
    running.add_argument("--jobs", type=job_count, default=1, metavar="N")
    running.add_argument("--watchdog", type=int, default=DEFAULT_WATCHDOG)
    running.add_argument("--timeout", type=positive, default=DEFAULT_TIMEOUT)
    running.add_argument("--work")
    running.add_argument("--keep", action="store_true")
    running.add_argument("--summary")
    running.add_argument("--asan-extra")
    running.add_argument("--no-stack-use-after-return", action="store_true")
    running.add_argument("--allow-apple-clang", action="store_true")
    for sub in (listing, running):
        sub.add_argument("--only", nargs="+", metavar="NAME")
        sub.add_argument("--all", action="store_true")
    args = parser.parse_args(argv)
    if args.command == "list":
        return command_list(args)
    return command_run(args, launcher)


if __name__ == "__main__":
    sys.exit(main())
