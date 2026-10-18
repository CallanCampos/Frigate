"""Who calls the functions whose names match a pattern, from the binary scan's call graph.

    callers.py REGEX [TOP]

Reads build/port-research/sdk-surface/binscan.pkl (binscan.py) and prints each matching function with its
callers, their library and file, most frequent first (TOP, 30 by default; * marks an address load rather than a
call). A query used while choosing the platform layer's boundary (port/docs/research/sdk-surface.md).
"""
import pickle,sys,re
from collections import defaultdict, Counter
from pathlib import Path
db=pickle.load(open(Path(__file__).resolve().parents[4]/"build/port-research/sdk-surface/binscan.pkl","rb"))
F=db["funcs"]
pat=re.compile(sys.argv[1])
res=defaultdict(list)
for caller,callee,kind,mod,frm in db["edges"]:
    if caller is None: continue
    if pat.search(F[callee]["name"]):
        cf=F[caller]
        res[F[callee]["name"]].append(f'{cf["name"]}[{cf["lib"]}{"" if cf["path"] is None else ":"+cf["path"].split("/")[-1]}]{"*" if kind=="load" else ""}')
for k in sorted(res):
    c=Counter(res[k])
    print(f"{k} ({len(res[k])}): "+", ".join(f"{a}" + (f" x{n}" if n>1 else "") for a,n in c.most_common(int(sys.argv[2]) if len(sys.argv)>2 else 30)))
