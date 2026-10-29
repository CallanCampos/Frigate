#!/usr/bin/env python3
"""Checks C files against the rules of docs/code-standards.md that a script can see.

    check_style.py FILE...                     every line of the files (src/ and lib/ .c and .h)
    check_style.py --diff BASE [--strict]      only the lines the working tree adds over BASE (a commit or branch)
    check_style.py --diff BASE FILE...         the same, for those files only

It prints one line per finding, `file:line: rule: what`, the rule being the section of docs/code-standards.md. A
finding the code has a reason for is allowed by a comment `// style-ok: RULE reason` on its line or the line before.
Exit status: 0, or 1 with --strict when there are findings; 2 when a path can't be read or BASE is unknown.

Rules checked (docs/code-standards.md):
  goto-return   1   a goto whose label is followed only by a return or the function's end
  byte-offset   4   a field reached through a cast to another pointer type and a byte offset
  extern        5   extern in a .c file
  prototype     5   a non-static prototype in a .c file of a function the file doesn't define
  volatile      6   volatile with no comment on its line or the line before (register macros excepted)
  asm           7   asm in src/, or an asm function in lib/ with no comment above it
  todo          8   TODO, FIXME or XXX in a comment
  narration     8   a comment that narrates the matching rather than the code
  bug           10  a // BUG: with no #ifdef BUGFIX in the next lines
  split-decl    11  a comment between a declaration's specifiers and the rest (`static // ...`)
  row           12  a row of docs/nonmatching-functions.md that narrates its history
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

NONMATCHING = "docs/nonmatching-functions.md"
COMMENT = re.compile(r"//(.*)$|/\*(.*?)(\*/|$)")
NARRATION = re.compile(
    r"\b(added (?:only )?to match|(?:declared|written|kept|split) [^.]{0,40}\bto match\b|so (?:that )?it matches|"
    r"to get the match|for the match|matches only (?:with|if|when)|only matches (?:with|if|when)|permuter|"
    r"this pass|previous pass|earlier pass|down from \d)",
    re.I)
ROW_NARRATION = re.compile(
    r"\b(down from|this pass|previous pass|earlier pass|last pass|re-?confirmed|theory|disproven|record-lesson|"
    r"iterations|untried|stopped per|flagged for)\b",
    re.I)
OK = re.compile(r"style-ok:\s*([\w-]+)")
FUNC_DEF = re.compile(r"^(?!static\b)[A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\s*\([^;{]*\)\s*\{\s*$")
PROTO = re.compile(r"^(?!static\b|extern\b|typedef\b|return\b)[A-Za-z_][\w \*]*?\b([A-Za-z_]\w*)\s*\([^;{)]*\)\s*;\s*$")
BYTE_OFFSET = re.compile(r"\*\s*\(\s*\w+\s*\*\s*\)\s*\(\s*\(\s*(?:u8|s8|char|unsigned char)\s*\*\s*\)[^;]*?\+")


def code_part(line):
    """The line without its comment and string literals."""
    line = re.sub(r'"(\\.|[^"\\])*"', '""', line)
    line = re.sub(r"'(\\.|[^'\\])*'", "''", line)
    return re.sub(r"//.*$|/\*.*?(\*/|$)", "", line)


def comments(line):
    return [m.group(1) or m.group(2) or "" for m in COMMENT.finditer(line)]


def allowed(lines, i, rule):
    for j in (i, i - 1):
        if 0 <= j < len(lines):
            m = OK.search(lines[j])
            if m and m.group(1) in (rule, "all"):
                return True
    return False


def check_c(path, lines):
    found = []
    is_c = path.endswith(".c")
    in_src = path.startswith("src/")
    in_lib = path.startswith("lib/")
    defined = set()
    protos = []
    labels = {}
    in_macro = False
    for i, line in enumerate(lines):
        m = re.match(r"^\s*([A-Za-z_]\w*)\s*:\s*(;\s*)?$", line)
        if m and m.group(1) not in ("default",):
            labels[m.group(1)] = i
        md = FUNC_DEF.match(line)
        if md:
            defined.add(md.group(1))
    for i, line in enumerate(lines):
        code = code_part(line)
        stripped = line.strip()
        macro_line = in_macro or re.match(r"\s*#\s*define\b", line)
        in_macro = bool(macro_line) and line.rstrip().endswith("\\")

        def add(rule, what):
            if not allowed(lines, i, rule):
                found.append((i + 1, rule, what))

        # goto-return
        mg = re.search(r"\bgoto\s+([A-Za-z_]\w*)\s*;", code)
        if mg and mg.group(1) in labels:
            rest = [l.strip() for l in lines[labels[mg.group(1)] + 1:labels[mg.group(1)] + 4] if l.strip()]
            if rest and (rest[0].startswith("return") or rest[0] == "}" or (rest[0] == ";" and rest[1:2] == ["}"])):
                add("goto-return", f"`goto {mg.group(1)}` only jumps to a return: return there instead")
        if BYTE_OFFSET.search(code):
            add("byte-offset", "a field reached by a cast and a byte offset: use the struct's field")
        if is_c and re.match(r"\s*extern\b", code) and not macro_line:
            add("extern", "extern in a .c file: declare it in the owner's header")
        if is_c and not line.startswith((" ", "\t")):
            mp = PROTO.match(line)
            # A global defined with an alignment (`T x ATTRIBUTE_ALIGN(32);`) is not a prototype
            if mp and mp.group(1) != "ATTRIBUTE_ALIGN":
                protos.append((i, mp.group(1)))
        if re.search(r"\bvolatile\b", code) and not macro_line and not re.search(r"\(\s*\*\s*\(\s*volatile", code) \
                and not re.match(r"\s*typedef\b", code):
            above = [l.strip() for l in lines[max(0, i - 3):i]]
            if "//" not in line and "/*" not in line and not any(l.startswith(("//", "/*", "*")) for l in above):
                add("volatile", "volatile without a comment saying what changes it")
        if re.search(r"\basm\b|__asm", code):
            if in_src:
                add("asm", "asm in game code")
            elif in_lib and re.match(r"^(static\s+)?(inline\s+)?asm\b.*\{\s*$", line):
                above = [l.strip() for l in lines[max(0, i - 4):i] if l.strip() and "clang-format" not in l]
                if not above or not above[-1].startswith(("//", "/*", "*")):
                    add("asm", "asm function with no comment saying why it is assembly")
        for c in comments(line):
            if re.search(r"\b(TODO|FIXME|XXX)\b", c):
                add("todo", "TODO/FIXME/XXX left in")
            if NARRATION.search(c):
                add("narration", "comment narrates the matching: say what the code does")
            if re.match(r"\s*BUG\b", c) and not any(re.search(r"#\s*(if|else).*BUGFIX|#\s*else", l)
                                                     for l in lines[max(0, i - 10):i + 15]):
                add("bug", "// BUG: with no #ifdef BUGFIX fix after it")
        if re.match(r"\s*(?:(?:static|inline|extern|const)\s+)*(?:static|inline|extern|const)\s*//", line):
            add("split-decl", "a comment splits the declaration's specifiers from it: put the comment above")
    for i, name in protos:
        if name not in defined and not allowed(lines, i, "prototype"):
            found.append((i + 1, "prototype", f"`{name}` is another file's: declare it in its owner's header"))
    return found


def check_rows(lines, only=None):
    found = []
    for i, line in enumerate(lines):
        if only is not None and i + 1 not in only:
            continue
        if line.startswith("| `") and ROW_NARRATION.search(line):
            found.append((i + 1, "row", f"narrates history ({ROW_NARRATION.search(line).group(0)!r}): say what differs now"))
    return found


def check_path(path, only=None):
    try:
        text = Path(path).read_text(encoding="utf-8", errors="replace")
    except OSError:
        print(f"{path}: cannot read", file=sys.stderr)
        return None
    lines = text.split("\n")
    if path.replace("\\", "/").endswith(NONMATCHING):
        return check_rows(lines, only)
    found = check_c(path, lines)
    return found if only is None else [f for f in found if f[0] in only]


def added_lines(base):
    """The lines the working tree adds over base, by file: {path: set of line numbers}."""
    # Non-C resources in a textual diff need not be UTF-8. Preserve their bytes while finding the checked files' hunks.
    diff = subprocess.run(["git", "diff", "-U0", "--no-color", base, "--"], capture_output=True,
                          encoding="utf-8", errors="surrogateescape")
    if diff.returncode != 0:
        print(diff.stderr.strip(), file=sys.stderr)
        return None
    out = {}
    path = None
    for line in diff.stdout.splitlines():
        if line.startswith("+++ "):
            path = line[6:] if line.startswith("+++ b/") else None
        elif line.startswith("@@") and path:
            m = re.search(r"\+(\d+)(?:,(\d+))?", line)
            start, count = int(m.group(1)), int(m.group(2) or 1)
            out.setdefault(path, set()).update(range(start, start + count))
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("paths", nargs="*")
    parser.add_argument("--diff", metavar="BASE", help="check only the lines added over BASE")
    parser.add_argument("--strict", action="store_true", help="exit 1 when there are findings")
    args = parser.parse_args()
    targets = {}
    if args.diff:
        added = added_lines(args.diff)
        if added is None:
            sys.exit(2)
        for path, nums in added.items():
            if (path.endswith((".c", ".h")) and path.startswith(("src/", "lib/", "include/"))) or path == NONMATCHING:
                targets[path] = nums
    if args.diff and args.paths:
        # only the given files' added lines
        wanted = {str(Path(p)) for p in args.paths}
        targets = {k: v for k, v in targets.items() if str(Path(k)) in wanted}
    else:
        for p in args.paths:
            targets[p] = None
    total = 0
    unreadable = False
    for path in sorted(targets):
        found = check_path(path, targets[path])
        if found is None:
            unreadable = True
            continue
        for line, rule, what in found:
            print(f"{path}:{line}: {rule}: {what}")
        total += len(found)
    if unreadable:
        sys.exit(2)
    if total and args.strict:
        print(f"{total} finding(s): see docs/code-standards.md; a justified exception takes `// style-ok: RULE reason`",
              file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
