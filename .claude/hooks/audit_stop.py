#!/usr/bin/env python3
"""Stop hook of the code-audit skill: blocks a session from ending its turn while its audit is incomplete.

It looks for .claude/audit-state.json in the session's worktrees (the project directory and .claude/worktrees/*), and
only for a session whose transcript invoked the code-audit skill. It lets the turn end when the state says complete,
awaiting_agents (reviewers are running) or awaiting_user (a question is out), when the file is over 6 hours old, or
when nothing can be read. Otherwise it blocks with what is still open. Exit 0 always; the decision is in the JSON.
"""
import json
import os
import re
import sys
import time
from pathlib import Path


def states(root):
    yield root / ".claude/audit-state.json"
    for wt in sorted((root / ".claude/worktrees").glob("*")):
        yield wt / ".claude/audit-state.json"


def main():
    try:
        call = json.load(sys.stdin)
    except ValueError:
        return
    transcript = call.get("transcript_path")
    try:
        text = Path(transcript).read_text(errors="replace") if transcript else ""
    except OSError:
        text = ""
    if not re.search(r'"skill"\s*:\s*"code-audit"|<command-name>/code-audit<', text):
        return
    root = Path(os.environ.get("CLAUDE_PROJECT_DIR") or call.get("cwd") or ".")
    for path in states(root):
        if not path.exists() or time.time() - path.stat().st_mtime > 6 * 3600:
            continue
        try:
            state = json.loads(path.read_text())
        except ValueError:
            print(json.dumps({"decision": "block", "reason": f"{path} is not valid JSON: repair it (or delete it if no "
                                                            "audit is running), then continue the code-audit loop."}))
            return
        if state.get("complete") or state.get("awaiting_agents") or state.get("awaiting_user"):
            continue
        open_findings = [f for f in state.get("findings", []) if f.get("status") == "open"]
        phase, n = state.get("phase", "A"), state.get("pass", 1)
        step = ("Phase A is read-only: finish the reviewers' partitions, verify their findings and record them, then "
                "start Phase B." if phase == "A" else
                "Phase B: fix the open findings, most severe first, build both ROMs, then the confirmation round.")
        print(json.dumps({"decision": "block", "reason":
            f"The code-audit loop in {path.parent.parent} is active (phase {phase}, pass {n}, {len(open_findings)} "
            f"findings open) and its state says complete: false. Rule 0 of the skill: don't end the turn or ask whether "
            f"to continue. {step} Set awaiting_user only for a real decision, and complete only after the final report."}))
        return


if __name__ == "__main__":
    main()
