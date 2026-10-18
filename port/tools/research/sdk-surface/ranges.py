"""The .text ranges of a module's source files, from the SDK surface database.

    ranges.py MODULE [ALL]

Reads build/port-research/sdk-surface/db.pkl (db.py) and prints each file's .text range, its path and C when it is
complete, the first of each run of files in one directory unless a second argument asks for every file: where a
library's code sits in a module (port/docs/research/sdk-surface.md).
"""
import pickle,sys
from pathlib import Path
db=pickle.load(open(Path(__file__).resolve().parents[4]/"build/port-research/sdk-surface/db.pkl","rb"))
m=db["mods"][sys.argv[1]]
rs=sorted((e["sections"][".text"][0],e["sections"][".text"][1],e["path"],e["complete"]) for e in m["delinks"] if ".text" in e["sections"])
prev=None
for s,e,p,c in rs:
    d=p.rsplit("/",1)[0]
    if len(sys.argv)>2 or d!=prev:
        print(hex(s),hex(e),p,"C" if c else "")
    prev=d
