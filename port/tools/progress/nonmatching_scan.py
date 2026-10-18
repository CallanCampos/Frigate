"""Classifies docs/nonmatching-functions.md rows by the kind of difference their text describes.

Usage: python3 nonmatching_scan.py <repo> [--list CLASS]
Classes (first match wins, in this order):
  semantic   the row says or implies the C may not do what the ROM does (uncertain/unknown/behaviour/differs in effect)
  missing    code is not emitted / not written / still in assembly / not linked
  size       a size difference with instruction-level reasons (scheduling, extra or fewer instructions)
  regalloc   register allocation / order only, same size
  other

Reads REPO/docs/nonmatching-functions.md. The semantic rows are the game code whose C the port compiles but that may
not behave as the ROM does (port/docs/decomp-findings.md records what the port shows); run_all.sh writes the counts to
nonmatching_classes.txt (port/docs/research/decomp-progress.md, "Nonmatching functions").
"""
import re
import sys

root = sys.argv[1]
text = open(f"{root}/docs/nonmatching-functions.md").read()
rows = [l for l in text.splitlines() if l.startswith("| `")]
SEM = re.compile(r"semantic|behav|functional|not (yet )?(known|understood)|unknown (what|whether|meaning)|uncertain|guess|"
                 r"does not do what|doesn't do what|is wrong|wrong (value|result|offset|field)|different (result|value|effect)|"
                 r"differs? in (effect|behaviou?r)|not equivalent|isn't equivalent|placeholder|stub|unverified|"
                 r"may not|might not|untested|approximat|not the same (code|logic)|logic differs|"
                 r"does what the ROM does|same effect|same behaviou?r|equivalent", re.I)
MISSING = re.compile(r"not emitted|still in assembly|not written|in assembly|unwritten|asm function|not linked", re.I)
SIZE = re.compile(r"bytes? (smaller|larger|longer|shorter|short)|0x[0-9a-f]+ (bytes )?(against|versus|vs\.?) 0x|size 0x", re.I)
REG = re.compile(r"register|regalloc|r\d+ |swap|order|schedul|same size", re.I)
cls = {}
for r in rows:
    cells = [c.strip() for c in r.split("|")]
    f, fn, addr, diff = cells[1], cells[2], cells[3], "|".join(cells[4:])
    m = SEM.search(diff)
    if m:
        k = "semantic?"
    elif MISSING.search(diff):
        k = "missing"
    elif SIZE.search(diff):
        k = "size"
    elif REG.search(diff):
        k = "regalloc/order"
    else:
        k = "other"
    cls.setdefault(k, []).append((f, fn, addr, diff, m.group(0) if m else ""))
for k, v in cls.items():
    print(k, len(v))
if "--list" in sys.argv:
    want = sys.argv[sys.argv.index("--list") + 1]
    for f, fn, addr, diff, kw in cls.get(want, []):
        i = diff.lower().find(kw.lower()) if kw else 0
        print(f"{f} {fn} [{kw}] ...{diff[max(0, i - 160):i + 200]}...")
