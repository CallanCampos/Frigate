#!/usr/bin/env python3
"""The pointer cast guard: fails when the game's C gains a cast that would cut an address to 32 bits on a 64-bit host,
or make a pointer of a 32-bit integer, that the committed baseline doesn't list. It touches no game functionality
itself: it reads the decomp's C (src/ and lib/), as the host build compiles it, for the casts that fault on a host the
first time the game runs them (PLAN.md §4.8, port/docs/game-changes.md's Neutral rows).

    check_ptrcasts.py --build-dir B [--baseline FILE] [--update] [--report] [--jobs N] [--diagnostics OUT] [FILE...]

It runs each src/ and lib/ command of B's compile_commands.json (the host build exports it) again with the build's own
compiler, syntax only (-fsyntax-only -ferror-limit=0), with what the host build turns off turned on: the four cast
warnings that port/CMakeLists.txt silences for the game's C come back, the decomp's include directories are -I rather
than -isystem so that warnings in its headers show (a host variant of a header, in the build's gen/hwinclude, is
reported as the header it was made from, ADR 0004), and -Wshorten-64-to-32 and -Wcast-function-type-strict are added.
What it counts, a site being one place in one file however many files include it:

  ptr-to-int  a pointer cast to an integer narrower than a pointer, such as (u32)work;
  int-to-ptr  such an integer cast to a pointer, such as (FieldTask *)a2 of a u32 a2;
  narrow      an integer that carries an address (uptr, sptr, uintptr_t, intptr_t) narrowed to 32 bits, as a uptr
              value stored in a u32 is on a host. A long or unsigned long isn't counted: the decomp's are numbers (the
              unsigned long locals the matching needed, atol's results, pointer differences within a buffer), and a
              pointer cast to long is against PLAN.md §4.8's rule 6. Nor is a call's result narrowed as it is stored:
              a getter that returns a uptr returns numbers and addresses alike (a field by its ID), its callers keep
              the numbers in 32 bits, and one that makes a pointer of the cut value again is flagged there, as
              int-to-ptr;
  fn-cast     a cast between function pointer types that pairs a pointer or a uptr with an integer of 32 bits or
              less, in the return or the same parameter, so a callback would get or give a cut address.

Each finding is keyed by its file, the function that holds it (the initialized variable at file scope, the macro in a
header's #define), its kind and its types, "to <- from" as the source spells them, with the number of sites. Line
numbers are printed but not keyed, so edits elsewhere in a file leave the baseline alone. The baseline, a TSV, gives
each key its count, its class and a note:

  real         a truncation the game reaches or will reach, not fixed yet: the note names its group or ledger row, and
               the row goes when the fix lands (a uptr retyping, with its row in port/docs/game-changes.md);
  harmless     it never cuts an address the game uses: an alignment or low-bit test, a difference within one block, an
               int carried in a void * and back, the number a uptr setter is given for a field that holds numbers;
  handled      the platform layer replaces or feeds the code so that the value is right (port/hal);
  unreachable  the host never runs it (TWL-only code, a mode nothing sets).

The check fails, status 1, when a key isn't in the baseline, when a key has more sites than the baseline gives it,
when a baseline row has another class (--update writes "new" for a key it adds, to be classified), or when a file
fails to compile. A failure lists each site with its file, line, function and source line, and says the two ways out.
Fewer sites, or a baseline row with none, pass with a note of what --update would change. A key that vanished and a
new key that agree on the kind and on two of the file, the function and the types, and pair with no other, are the
same finding renamed upstream (a function named, a file renamed, a struct named): it passes, with the note, when its
count is no higher.

--update rewrites the baseline to the findings, keeping each kept or renamed row's class and note. --report prints
one line, the sites by class, which is T1's 64-bit measurement (port/docs/STATUS.md's "64-bit warnings",
port/docs/adr/0019-pointer-cast-guard-against-a-classified-baseline.md).
--diagnostics OUT writes every finding as a TSV row. FILE... limits the run to those files (paths from the repository's
root), for a quick look; the comparison then covers only their own keys, not those of the headers they include.

It exits 77, which ctest reports as a skip, on Windows, when B has no compile_commands.json, when its compiler isn't
clang, or when the target isn't LP64 (Windows' LLP64, whose long is 32 bits, gives keys of its own and no narrowing;
the baseline is macOS's and Linux's). The ctest test game_ptrcasts runs it in each host build (port/cmake/game.cmake):
CI's macOS and Linux host and sanitized jobs on every pull request, the local verification's ctest
(port/tools/sync_upstream.sh --verify-pr) and the local sync's, so a merge that adds a cast fails before it reaches
main. Standard library only; it writes nothing but the baseline with --update and OUT with --diagnostics.
port/docs/SYNC.md says what to do when it fails, on a sync PR too.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import shlex
import subprocess
import sys
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BASELINE = ROOT / "port" / "tools" / "ptrcast_baseline.tsv"
CLASSES = ("real", "harmless", "handled", "unreachable")
COLUMNS = ("file", "function", "kind", "types", "count", "class", "note")

# What the host build silences for the game's C (port/CMakeLists.txt), and what the check turns on
SILENCED = {"-Wno-pointer-to-int-cast", "-Wno-int-to-pointer-cast", "-Wno-void-pointer-to-int-cast",
            "-Wno-int-to-void-pointer-cast"}
WARNINGS = ["-Wpointer-to-int-cast", "-Wvoid-pointer-to-int-cast", "-Wint-to-pointer-cast",
            "-Wint-to-void-pointer-cast", "-Wshorten-64-to-32", "-Wcast-function-type",
            "-Wcast-function-type-strict"]
KIND_OF_FLAG = {
    "-Wpointer-to-int-cast": "ptr-to-int", "-Wvoid-pointer-to-int-cast": "ptr-to-int",
    "-Wint-to-pointer-cast": "int-to-ptr", "-Wint-to-void-pointer-cast": "int-to-ptr",
    "-Wshorten-64-to-32": "narrow",
    "-Wcast-function-type": "fn-cast", "-Wcast-function-type-strict": "fn-cast",
}
# The integers that carry an address, as the source spells them: include/types.h's, and the C library's that the host
# variants of the SDK's headers give a DS address (ADR 0004)
POINTER_WIDE = {"uptr", "sptr", "uintptr_t", "intptr_t"}
# Integers of 32 bits or less on an LP64 host, canonical spellings
INT32 = {"int", "unsigned int", "signed int", "short", "unsigned short", "signed short", "char", "signed char",
         "unsigned char", "_Bool", "bool"}

DIAGNOSTIC = re.compile(r"^(.*?):(\d+):(\d+):((?:\{\d+:\d+-\d+:\d+\})*):? (?:warning|error): (.*) \[([^\]]+)\]$")
RANGE = re.compile(r"\{(\d+):(\d+)-(\d+):(\d+)\}")
# A call: a function's name, or a function pointer reached through members (work->funcs->get), then its arguments
CALL = re.compile(r"^[A-Za-z_]\w*(?:\s*(?:->|\.)\s*[A-Za-z_]\w*)*\s*\(")
QUOTED = re.compile(r"'([^']*)'(?: \(aka '([^']*)'\))?")
GENERATED_FROM = re.compile(r"^// Generated by port/tools/gen_host_tree\.py from (\S+);")
FUNCTION_START = re.compile(r"^[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\(")
INITIALIZER = re.compile(r"\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\]\s*)*=")
DEFINE = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)")
LABEL = re.compile(r"^[A-Za-z_]\w*:\s*(//.*)?$")


def skip(message: str):
    print(f"check_ptrcasts: skipped: {message}")
    sys.exit(77)


def split_command(entry: dict) -> list[str]:
    """A compile database entry's argument vector, with response files (@file) read in."""
    argv = list(entry["arguments"]) if "arguments" in entry else shlex.split(entry["command"])
    out = []
    for arg in argv:
        if arg.startswith("@"):
            path = Path(entry.get("directory", ".")) / arg[1:]
            out += shlex.split(path.read_text())
        else:
            out.append(arg)
    return out


def target_flags(argv: list[str]) -> list[str]:
    """The flags that choose the target, the first -arch only: a universal macOS build's two slices are both LP64."""
    flags, arch = [], False
    i = 1
    while i < len(argv):
        arg = argv[i]
        if arg == "-arch" and i + 1 < len(argv):
            if not arch:
                flags += [arg, argv[i + 1]]
                arch = True
            i += 2
            continue
        if arg in ("-target", "-isysroot") and i + 1 < len(argv):
            flags += [arg, argv[i + 1]]
            i += 2
            continue
        if arg.startswith(("--target=", "-m32", "-m64", "-mmacosx-version-min", "--sysroot")):
            flags.append(arg)
        i += 1
    return flags


def syntax_command(argv: list[str], source: str) -> list[str]:
    """The host build's command for one file, as a syntax-only pass with the cast warnings on."""
    out, arch = [argv[0]], False
    i = 1
    while i < len(argv):
        arg = argv[i]
        if arg in ("-o", "-MT", "-MF", "-MQ") and i + 1 < len(argv):
            i += 2
            continue
        if arg in ("-c", "-MD", "-MMD") or arg in SILENCED or arg == source:
            i += 1
            continue
        if arg == "-arch" and i + 1 < len(argv):
            if not arch:
                out += [arg, argv[i + 1]]
                arch = True
            i += 2
            continue
        if arg == "-isystem" and i + 1 < len(argv):
            out += ["-I", argv[i + 1]]
            i += 2
            continue
        if arg.startswith("-isystem") and len(arg) > len("-isystem"):
            out.append("-I" + arg[len("-isystem"):])
            i += 1
            continue
        out.append(arg)
        i += 1
    return out + WARNINGS + ["-fsyntax-only", "-ferror-limit=0", "-fno-color-diagnostics",
                             "-fno-caret-diagnostics", "-fdiagnostics-show-option",
                             "-fdiagnostics-print-source-range-info", source]


class Sources:
    """The repository's files as the findings name them, read once: a host variant of a header (gen/hwinclude) is
    named after the header it was made from, one line further up, for the line the generator puts first."""

    def __init__(self):
        self.lines: dict[Path, list[str]] = {}

    def read(self, path: Path) -> list[str]:
        if path not in self.lines:
            try:
                self.lines[path] = path.read_text(errors="replace").splitlines()
            except OSError:
                self.lines[path] = []
        return self.lines[path]

    def locate(self, path: Path, line: int) -> tuple[str, int, str]:
        """The repository-relative name, the line there and the source line of a reported location."""
        text = self.read(path)
        source = text[line - 1].strip() if 0 < line <= len(text) else ""
        generated = GENERATED_FROM.match(text[0]) if text else None
        if generated:
            return generated.group(1), line - 1, source
        try:
            name = path.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            name = path.as_posix()
        return name, line, source

    def is_call(self, path: Path, span) -> bool:
        """Whether the source range of a diagnostic's expression is one function call, in parentheses or not."""
        if not span:
            return False
        text = self.read(path)
        first_line, first_column, last_line, last_column = span
        if not 0 < first_line <= last_line <= len(text):
            return False
        if first_line == last_line:
            code = text[first_line - 1][first_column - 1:last_column - 1]
        else:
            code = "\n".join([text[first_line - 1][first_column - 1:], *text[first_line:last_line - 1],
                              text[last_line - 1][:last_column - 1]])
        code = code.strip()
        while code.startswith("(") and code.endswith(")") and closing(code, 0) == len(code) - 1:
            code = code[1:-1].strip()
        call = CALL.match(code)
        return bool(call) and closing(code, call.end() - 1) == len(code) - 1

    def function_at(self, name: str, line: int) -> str:
        """The function that holds a line of a repository file, the variable a file-scope initializer sets, or the
        macro a #define line belongs to, from the layout clang-format gives the decomp: definitions start in column 0,
        bodies are indented."""
        text = self.read(ROOT / name)
        if not 0 < line <= len(text):
            return "?"
        index = line - 1
        start = index
        while start > 0 and text[start - 1].rstrip().endswith("\\"):
            start -= 1
        define = DEFINE.match(text[start])
        if define:
            return define.group(1)
        for i in range(index, -1, -1):
            row = text[i]
            if not row or not (row[0].isalpha() or row[0] == "_") or LABEL.match(row):
                if row.startswith("}") and i != index:
                    return "<file scope>"
                continue
            if row.startswith(("typedef", "struct ", "union ", "enum ")) and "(" not in row:
                return "<file scope>"
            initializer = INITIALIZER.search(row)
            function = FUNCTION_START.match(row)
            if function and (not initializer or row.index("(") < initializer.start()):
                if row.rstrip().endswith(";"):
                    return "<file scope>"
                return function.group(1)
            if initializer:
                return initializer.group(1)
            return "<file scope>"
        return "<file scope>"


def closing(code: str, start: int) -> int:
    """The index of the parenthesis that closes the one at start, or -1."""
    depth = 0
    for i in range(start, len(code)):
        depth += code[i] == "("
        depth -= code[i] == ")"
        if depth == 0:
            return i
    return -1


def quoted_types(message: str) -> list[tuple[str, str]]:
    """Each type a diagnostic quotes, as spelled and canonical."""
    return [(m.group(1), m.group(2) or m.group(1)) for m in QUOTED.finditer(message)]


def function_type(text: str):
    """A function pointer type's return and parameter types, or None."""
    m = re.match(r"^(.*?)\s*\(\*\)\((.*)\)$", text.strip())
    if not m:
        return None
    params, depth, current = [], 0, ""
    for ch in m.group(2):
        depth += ch == "("
        depth -= ch == ")"
        if ch == "," and depth == 0:
            params.append(current.strip())
            current = ""
        else:
            current += ch
    if current.strip():
        params.append(current.strip())
    if params == ["void"]:
        params = []
    return m.group(1).strip(), params


def unqualified(text: str) -> str:
    return re.sub(r"\b(const|volatile|restrict)\b", "", text).strip()


def width_class(text: str) -> str:
    """wide for what carries an address on an LP64 host (a pointer, a uptr's unsigned long), int32 for an integer of
    32 bits or less, other for the rest (structs, 64-bit integers that are numbers on the DS too)."""
    text = unqualified(text)
    if "*" in text or "(" in text or text in ("long", "unsigned long", "long int", "unsigned long int"):
        return "wide"
    if text in INT32 or text.startswith("enum "):
        return "int32"
    return "other"


def fn_cast_pairs_pointer_with_int(source: str, target: str) -> bool:
    """Whether a cast between function pointer types gives a callback a pointer or a uptr where it takes a 32-bit
    integer, or the reverse, in its return or in the same parameter: the value is cut, or its high half undefined."""
    a, b = function_type(source), function_type(target)
    if not a or not b:
        return False
    pairs = [(a[0], b[0]), *zip(a[1], b[1])]
    return any({width_class(x), width_class(y)} == {"wide", "int32"} for x, y in pairs)


def classify(flag: str, message: str):
    """A diagnostic's kind and its "to <- from" types as spelled, or None when the check doesn't count it."""
    kind = KIND_OF_FLAG.get(flag)
    types = quoted_types(message)
    if not kind or len(types) < 2:
        return None
    if kind in ("ptr-to-int", "int-to-ptr"):
        return kind, f"{types[0][0]} <- {types[1][0]}"
    if kind == "narrow":
        (source, source_canonical), (target, _) = types[0], types[1]
        if unqualified(source) not in POINTER_WIDE or unqualified(source_canonical) not in ("unsigned long", "long"):
            return None
        return kind, f"{target} <- {source}"
    (source, source_canonical), (target, target_canonical) = types[0], types[1]
    if not fn_cast_pairs_pointer_with_int(source_canonical, target_canonical):
        return None
    return kind, f"{target} <- {source}"


def run_file(item):
    source, argv, directory = item
    proc = subprocess.run(argv, cwd=directory, capture_output=True, text=True, errors="replace")
    found, errors = [], []
    for line in proc.stderr.splitlines():
        m = DIAGNOSTIC.match(line)
        if m:
            flags = [f for f in m.group(6).split(",") if f.startswith("-W") and f != "-Werror"]
            path = Path(m.group(1))
            if not path.is_absolute():
                path = Path(directory) / path
            ranges = RANGE.findall(m.group(4))
            span = tuple(int(n) for n in ranges[0]) if ranges else None
            found.append((path, int(m.group(2)), int(m.group(3)), flags[0] if flags else "", m.group(5), span))
        if ": error:" in line or ": fatal error:" in line:
            errors.append(line)
    if proc.returncode != 0 and not errors:
        errors.append(proc.stderr.strip().splitlines()[-1] if proc.stderr.strip() else f"status {proc.returncode}")
    return source, proc.returncode, found, errors


def read_baseline(path: Path) -> dict:
    rows = {}
    if not path.exists():
        return rows
    for number, line in enumerate(path.read_text().splitlines(), 1):
        fields = line.split("\t")
        if not line.strip() or line.startswith("#") or tuple(fields) == COLUMNS:
            continue
        if len(fields) != len(COLUMNS):
            sys.exit(f"check_ptrcasts: {path}:{number}: {len(fields)} columns, not {len(COLUMNS)}")
        file, function, kind, types, count, cls, note = fields
        rows[(file, function, kind, types)] = {"count": int(count), "class": cls, "note": note}
    return rows


BASELINE_HEADER = """\
# The pointer cast guard's baseline (port/tools/check_ptrcasts.py, the ctest test game_ptrcasts, port/docs/SYNC.md).
# A row per finding of the game's C on a 64-bit host: the file, the function that holds it, the kind (ptr-to-int,
# int-to-ptr, narrow, fn-cast), the types "to <- from" as spelled, the number of sites, the class and a one-line reason.
# real: a truncation not fixed yet, its group in the note (the row goes when its uptr fix lands, with its row in
# port/docs/game-changes.md); harmless: it never cuts an address the game uses; handled: the platform layer makes the
# value right; unreachable: the host never runs it. check_ptrcasts.py --update rewrites the counts, keeping the classes.
"""


def write_baseline(path: Path, rows: dict):
    lines = [BASELINE_HEADER.rstrip("\n"), "\t".join(COLUMNS)]
    for key in sorted(rows):
        row = rows[key]
        lines.append("\t".join([*key, str(row["count"]), row["class"], row["note"]]))
    path.write_text("\n".join(lines) + "\n")


def find_renames(new_keys, vanished_keys, current, baseline):
    """Pairs of a new key and a vanished baseline key that are one finding renamed: the same kind, two of the file,
    the function and the types the same, and no more sites than before; each side unique."""
    candidates = defaultdict(list)
    for new in new_keys:
        for old in vanished_keys:
            if new[2] != old[2]:
                continue
            same = sum(new[i] == old[i] for i in (0, 1, 3))
            if same >= 2 and current[new] <= baseline[old]["count"]:
                candidates[new].append(old)
    used = Counter(old for olds in candidates.values() for old in olds)
    return {new: olds[0] for new, olds in candidates.items() if len(olds) == 1 and used[olds[0]] == 1}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--build-dir", required=True, type=Path, help="a host build directory (build/port)")
    parser.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    parser.add_argument("--update", action="store_true", help="rewrite the baseline to the findings")
    parser.add_argument("--report", action="store_true", help="print the count by class, the port's 64-bit metric")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    parser.add_argument("--diagnostics", type=Path, help="write every finding to this TSV")
    parser.add_argument("--root", type=Path, help=argparse.SUPPRESS)  # another tree than this one, for the tool's test
    parser.add_argument("files", nargs="*", help="only these files of src/ and lib/, from the repository's root")
    args = parser.parse_args()
    global ROOT
    if args.root:
        ROOT = args.root.resolve()

    if os.name == "nt" or sys.platform in ("win32", "cygwin", "msys"):
        skip("Windows is LLP64: the baseline is macOS's and Linux's, and LLP64's 32-bit long gives other keys and no "
             "narrowing (port/docs/SYNC.md)")
    database = args.build_dir / "compile_commands.json"
    if not database.exists():
        skip(f"{database} doesn't exist: the host build exports it (CMAKE_EXPORT_COMPILE_COMMANDS, "
             "port/CMakeLists.txt)")
    entries = json.loads(database.read_text())
    wanted = {f.replace("\\", "/") for f in args.files}
    work = []
    for entry in entries:
        path = Path(entry["file"])
        if not path.is_absolute():
            path = Path(entry.get("directory", ".")) / path
        try:
            name = path.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            continue
        if not name.startswith(("src/", "lib/")) or (wanted and name not in wanted):
            continue
        argv = split_command(entry)
        work.append((name, syntax_command(argv, entry["file"]), entry.get("directory", str(args.build_dir))))
    if not work:
        skip("no file of src/ or lib/ in the compile database" + (" matches" if wanted else ""))

    compiler = work[0][1][0]
    version = subprocess.run([compiler, "--version"], capture_output=True, text=True, errors="replace")
    if "clang" not in version.stdout.lower():
        skip(f"{compiler} isn't clang, whose cast warnings the baseline is made of")
    macros = subprocess.run([compiler, *target_flags(work[0][1]), "-dM", "-E", "-x", "c", os.devnull],
                            capture_output=True, text=True, errors="replace").stdout
    if not re.search(r"#define __LP64__ 1", macros):
        skip("the target isn't LP64: the baseline is macOS's and Linux's, and LLP64's 32-bit long gives other keys "
             "and no narrowing (port/docs/SYNC.md)")

    sources = Sources()
    sites = {}
    failures = []
    with ThreadPoolExecutor(max(1, args.jobs)) as pool:
        for name, status, found, errors in pool.map(run_file, work):
            if status != 0:
                failures.append((name, errors))
            for path, line, column, flag, message, span in found:
                counted = classify(flag, message)
                if not counted or (counted[0] == "narrow" and sources.is_call(path, span)):
                    continue
                file, line, text = sources.locate(path, line)
                site = (file, line, column, counted[0], counted[1])
                if site not in sites:
                    sites[site] = text
    findings = defaultdict(list)
    for (file, line, column, kind, types), text in sorted(sites.items()):
        findings[(file, sources.function_at(file, line), kind, types)].append((line, text))
    if wanted:
        findings = defaultdict(list, {key: rows for key, rows in findings.items() if key[0] in wanted})
    current = {key: len(rows) for key, rows in findings.items()}

    baseline = read_baseline(args.baseline)
    if wanted:
        baseline = {key: row for key, row in baseline.items() if key[0] in wanted}
    new_keys = [key for key in current if key not in baseline]
    vanished = [key for key in baseline if key not in current]
    renames = find_renames(new_keys, vanished, current, baseline)
    new_keys = [key for key in new_keys if key not in renames]
    vanished = [key for key in vanished if key not in renames.values()]
    higher = [key for key in current if key in baseline and current[key] > baseline[key]["count"]]
    lower = [key for key in current if key in baseline and current[key] < baseline[key]["count"]]
    unclassified = [key for key, row in baseline.items() if row["class"] not in CLASSES]

    if args.diagnostics:
        with args.diagnostics.open("w") as out:
            out.write("file\tline\tfunction\tkind\ttypes\tsource\n")
            for key, rows in sorted(findings.items()):
                for line, text in rows:
                    out.write("\t".join([key[0], str(line), key[1], key[2], key[3], text]) + "\n")

    if args.update:
        rows = {}
        for key, count in current.items():
            old = baseline.get(key) or baseline.get(renames.get(key))
            if old:
                rows[key] = {"count": count, "class": old["class"], "note": old["note"]}
            else:
                rows[key] = {"count": count, "class": "new",
                             "note": "classify: real, harmless, handled or unreachable, and say why"}
        if wanted:
            full = read_baseline(args.baseline)
            full = {key: row for key, row in full.items() if key[0] not in wanted}
            full.update(rows)
            rows = full
        write_baseline(args.baseline, rows)
        added = [key for key in rows if rows[key]["class"] == "new"]
        print(f"check_ptrcasts: wrote {args.baseline}: {len(rows)} rows, {len(vanished)} dropped, {len(renames)} "
              f"renamed, {len(added)} new to classify")
        baseline = read_baseline(args.baseline)
        if wanted:
            baseline = {key: row for key, row in baseline.items() if key[0] in wanted}
        new_keys, higher, vanished, lower, renames = [], [], [], [], {}
        unclassified = [key for key, row in baseline.items() if row["class"] not in CLASSES]

    def describe(key):
        return f"{key[0]} in {key[1]}: {key[2]} ({key[3]})"

    def show_sites(key, limit=None):
        for line, text in findings[key][:limit]:
            print(f"    {key[0]}:{line}  {text}")

    for old_key in vanished:
        print(f"note: no longer found, --update drops it: {describe(old_key)} [{baseline[old_key]['class']}]")
    for key in lower:
        print(f"note: {current[key]} sites where the baseline has {baseline[key]['count']}, --update lowers it: "
              f"{describe(key)} [{baseline[key]['class']}]")
    for new, old in renames.items():
        print(f"note: renamed, --update renames the row: {describe(old)} -> {describe(new)}")

    failed = False
    if failures:
        failed = True
        print(f"\ncheck_ptrcasts: {len(failures)} file(s) failed to compile, so their casts weren't checked:")
        for name, errors in failures:
            print(f"  {name}")
            for error in errors[:3]:
                print(f"    {error}")
    if new_keys or higher:
        failed = True
        print(f"\ncheck_ptrcasts: {len(new_keys) + len(higher)} finding(s) that the baseline ({args.baseline.name}) "
              f"doesn't allow:")
        for key in new_keys:
            print(f"  new: {describe(key)}")
            show_sites(key)
        for key in higher:
            print(f"  {current[key]} sites, the baseline allows {baseline[key]['count']}: {describe(key)} "
                  f"[{baseline[key]['class']}]")
            show_sites(key)
        print("""
Each would cut an address to 32 bits on a 64-bit host, or make a pointer of a 32-bit integer, the first time the game
runs it. Either:
  - carry the address in a uptr or sptr (include/types.h: u32 and s32 under MWCC, pointer-wide on a host), retyping the
    API's parameter, return or field rather than each cast, with its Neutral row in port/docs/game-changes.md and the
    proofs (port/tools/check_mwcc_neutral.py on the files, both versions; ninja with both SHA1s); or
  - when it can never cut an address the game uses (an alignment test, an int carried in a void * and back, a
    difference within one block, code the host never runs), run port/tools/check_ptrcasts.py --build-dir BUILD
    --update, and give each new row its class (harmless, handled, unreachable, or real while the fix waits) and a
    one-line reason. port/docs/SYNC.md has the details.""")
    if unclassified:
        failed = True
        print(f"\ncheck_ptrcasts: {len(unclassified)} baseline row(s) without a class ({', '.join(CLASSES)}):")
        for key in unclassified:
            print(f"  {describe(key)}: {baseline[key]['class']}")
            show_sites(key)

    if args.report or not failed:
        by_class = Counter()
        for key, count in current.items():
            row = baseline.get(key) or baseline.get(renames.get(key)) or {"class": "new"}
            by_class[row["class"]] += count
        parts = ", ".join(f"{cls} {by_class[cls]}" for cls in (*CLASSES, "new") if by_class[cls] or cls in CLASSES)
        print(f"check_ptrcasts: {sum(current.values())} sites in {len(current)} keys ({parts}), "
              f"{len(work)} files, {len(failures)} not compiled")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
