"""The SDK functions at the boundary ("edge" category) of one SDK module, from the SDK surface database.

    q_edge.py MODULE

Reads build/port-research/sdk-surface/db.pkl (db.py) and prints each edge function of MODULE with its address,
mode, size and library: the functions the platform layer had to consider replacing for that module
(port/docs/research/sdk-surface.md).
"""
import pickle,sys
from pathlib import Path
db=pickle.load(open(Path(__file__).resolve().parents[4]/"build/port-research/sdk-surface/db.pkl","rb"))
F=db["funcs"]
want=sys.argv[1]
for f in F:
    if f["cat"]=="edge" and f["module"]==want:
        print(hex(f["addr"]),f["mode"],hex(f["size"]),f["name"],f["lib"])
