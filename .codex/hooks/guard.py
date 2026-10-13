#!/usr/bin/env python3
"""PreToolUse guard for the hard rules of this project, each learned from a session that went wrong.

Claude Code passes the tool call as JSON on stdin. Exit code 2 blocks the call and shows stderr to Claude.
"""
import json
import os
import re
import sys

BASH_RULES = [
    (
        # A -j8 run filled 32 GB of RAM and swap, and systemd-oomd killed the whole terminal with Claude Code in it.
        lambda c: re.search(r"(python[\d.]*\s+|\./)\S*permuter\.py", c) is not None
        and not ("systemd-run" in c and "MemoryMax" in c),
        "Run decomp-permuter only under a memory cap, so that only it gets killed:\n"
        "  systemd-run --user --scope --unit=perm-FUNC -p MemoryMax=6G -p MemorySwapMax=0 "
        "python3 permuter.py -j2 build/permuter/FUNC\n"
        "Stop it with `systemctl --user stop perm-FUNC.scope`. See .claude/skills/match-function/permuter.md.",
    ),
    (
        # The pattern also matches the shell running pkill, which then kills itself (exit 144).
        # Only where it runs as a command, not where a message or a grep mentions it
        lambda c: re.search(r"(^|[;&|(]|\bsudo)\s*(pkill|killall)\b", c, re.M) is not None,
        "pkill/killall match the shell running them. Stop a capped permuter with "
        "`systemctl --user stop perm-FUNC.scope`, or kill a PID you looked up with `pgrep -a`.",
    ),
    (
        # clang-format releases disagree with .clang-format's era and rewrote code nobody touched.
        lambda c: re.search(r"\bninja\s+(\S+\s+)*format\b", c) is not None
        or re.search(r"clang-format\b.*\s-i\b.*\s(src|include)/?(\s|$)", c) is not None,
        "Don't format the whole tree: clang-format rewrites code nobody touched. Run `clang-format -i` on the "
        "files you wrote, and check `git diff` for changes outside your code.",
    ),
    (
        # Several sessions share a checkout and its worktrees; a broad add commits another session's work.
        # Only the flags on the command line count, not words of a message given in a heredoc or quotes
        lambda c: re.search(
            r"\bgit\s+(add\s+(-A|--all|\.(\s|$))|commit([ \t]+-[a-zA-Z]+)*[ \t]+-[a-zA-Z]*a[a-zA-Z]*\b)", c
        )
        is not None,
        "Stage files by name. Other sessions may have uncommitted work in this checkout.",
    ),
    (
        # The stash stack is shared by every worktree and session; a bare pop can take another session's changes.
        lambda c: re.search(r"\bgit\s+stash(\s*($|[;&|])|\s+pop\b|\s+push\b(?!.*\s-m\s))", c) is not None,
        "The stash is shared with other worktrees and sessions. Prefer a temporary WIP commit, or use "
        "`git stash push -u -m UNIQUE-TAG` and `git stash apply <sha>`, then drop that entry by its tag.",
    ),
]

PROTECTED_PATHS = re.compile(r"(^|/)(orig|extract|build)/|\.sha1$|(^|/)\.venv/")


def main():
    call = json.load(sys.stdin)
    tool = call.get("tool_name", "")
    params = call.get("tool_input", {})
    if tool == "Bash":
        command = params.get("command", "")
        for broken, message in BASH_RULES:
            if broken(command):
                print(message, file=sys.stderr)
                sys.exit(2)
    elif tool in ("Edit", "Write", "MultiEdit", "NotebookEdit"):
        path = params.get("file_path") or params.get("notebook_path") or ""
        project = os.environ.get("CLAUDE_PROJECT_DIR", os.getcwd()).rstrip("/") + "/"
        # Worktrees sit inside the project, so their own build/ and orig/ are covered too; the scratchpad is not
        if path.startswith(project) and PROTECTED_PATHS.search(path[len(project):]):
            print(f"{path} is a base ROM, extracted or generated file, or SHA1; change the source or config "
                  "that produces it instead.", file=sys.stderr)
            sys.exit(2)


if __name__ == "__main__":
    main()
