#!/usr/bin/env python3
"""PostToolUse check of an edited C file against docs/code-standards.md, for Claude Code and Codex.

Reads the tool call as JSON on stdin, finds the src/, lib/ or include/ .c or .h files it wrote, and runs
tools/decomp/check_style.py on the lines that differ from HEAD, and clang-format 23.1.1 on the whole file. Findings go
back to the agent as feedback (decision "block" on PostToolUse doesn't undo the edit; it tells the agent to fix it).
Nothing found, or anything it can't run: silent, exit 0.
"""
import json
import os
import re
import subprocess
import sys
from pathlib import Path


def edited_files(call):
    tool_input = call.get("tool_input") or {}
    paths = []
    for key in ("file_path", "path", "notebook_path"):
        if isinstance(tool_input.get(key), str):
            paths.append(tool_input[key])
    for key in ("patch", "input", "command"):
        text = tool_input.get(key)
        if isinstance(text, str):
            paths += re.findall(r"^\*\*\* (?:Update|Add) File: (.+)$", text, re.M)
    return paths


def main():
    try:
        call = json.load(sys.stdin)
    except ValueError:
        return
    root = Path(os.environ.get("CLAUDE_PROJECT_DIR") or subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True).stdout.strip() or ".")
    # a worktree's own root, when the edited file is in one
    files = []
    for f in edited_files(call):
        path = Path(f)
        if not path.is_absolute():
            path = Path.cwd() / path
        top = subprocess.run(["git", "-C", str(path.parent), "rev-parse", "--show-toplevel"],
                             capture_output=True, text=True).stdout.strip()
        if not top:
            continue
        rel = path.resolve().relative_to(Path(top).resolve()).as_posix()
        if rel.endswith((".c", ".h")) and rel.startswith(("src/", "lib/", "include/")) and path.exists():
            files.append((Path(top), rel))
    messages = []
    for top, rel in files:
        checker = top / "tools/decomp/check_style.py"
        if not checker.exists():
            continue
        tracked = subprocess.run(["git", "-C", str(top), "ls-files", "--error-unmatch", rel],
                                 capture_output=True).returncode == 0
        args = [sys.executable, str(checker)] + (["--diff", "HEAD", rel] if tracked else [rel])
        out = subprocess.run(args, cwd=top, capture_output=True, text=True).stdout.strip()
        if out:
            messages.append(out)
        fmt = subprocess.run(["clang-format", "--dry-run", "--Werror", rel], cwd=top, capture_output=True, text=True)
        if fmt.returncode not in (0,) and "error" in fmt.stderr:
            messages.append(f"{rel}: not clang-formatted: run `clang-format -i {rel}` (23.1.1), guarding asm functions, "
                            "hand-laid-out tables and GFL_ASSERTs with operators with // clang-format off/on")
    if messages:
        print(json.dumps({"decision": "block", "reason": "docs/code-standards.md findings in what you just wrote "
                          "(fix them, or add `// style-ok: RULE reason` where the code has one):\n" + "\n".join(messages)}))


if __name__ == "__main__":
    main()
