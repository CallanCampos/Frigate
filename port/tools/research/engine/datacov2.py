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
