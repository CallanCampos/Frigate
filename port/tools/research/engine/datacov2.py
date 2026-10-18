"""Data and .bss symbols of one module that an incomplete source file covers but doesn't define in its C yet.

    datacov2.py REPO MODULE        (MODULE: main or an overlay directory, such as ov036)

Reads Black 2's delinks.txt and symbols.txt for the module, and for each data symbol in an incomplete file's
ranges, looks for its definition in the file's C. Prints the 30 files with the most symbols still missing and the
symbols outside every file: the game data that becomes trap placeholders on the host (port/tools/gen_stubs.py).
Research of 2026-10-16 (port/docs/research/engine-boot.md).
"""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
W, mod = sys.argv[1], sys.argv[2]
CFG = os.path.join(W, "config/b2_us/arm9")
d = CFG if mod == "main" else os.path.join(CFG, "overlays", mod)
SEC_RE = re.compile(r"^\s+(\.\w+)\s+start:0x([0-9a-f]+)\s+end:0x([0-9a-f]+)")
files=[]; cur=None
for line in open(os.path.join(d,"delinks.txt")):
    if line.strip() and not line.startswith(" "):
        cur={"name":line.strip().rstrip(":"),"complete":False,"r":[]}; files.append(cur)
    elif line.strip()=="complete" and cur: cur["complete"]=True
    else:
        m=SEC_RE.match(line)
        if m and cur and m.group(1) in (".data",".rodata",".bss"):
            cur["r"].append((int(m.group(2),16),int(m.group(3),16)))
from collections import Counter
cnt=Counter(); nof=[]
cache={}
for line in open(os.path.join(d,"symbols.txt")):
    m=re.match(r"^(\S+) kind:(data|bss)\S*.*? addr:0x([0-9a-f]+)",line)
    if not m: continue
    name,a=m.group(1),int(m.group(3),16)
    own=None
    for f in files:
        if any(s<=a<e for s,e in f["r"]): own=f; break
    if own is None: nof.append(name); continue
    if own["complete"]: continue
    p=os.path.join(W,own["name"])
    if p not in cache: cache[p]=open(p,errors="replace").read() if os.path.exists(p) else ""
    ok=False
    for l in cache[p].splitlines():
        if re.search(r"\b"+re.escape(name)+r"\s*(\[[^\]]*\])*\s*(=|;|\[)",l) and not l.lstrip().startswith(("extern","//")): ok=True;break
    if not ok: cnt[own["name"]]+=1
for k,v in cnt.most_common(30): print(v,k)
print("nofile:", " ".join(nof[:30]))
