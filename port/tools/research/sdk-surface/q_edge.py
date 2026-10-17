import pickle,sys
from pathlib import Path
db=pickle.load(open(Path(__file__).resolve().parents[4]/"build/port-research/sdk-surface/db.pkl","rb"))
F=db["funcs"]
want=sys.argv[1]
for f in F:
    if f["cat"]=="edge" and f["module"]==want:
        print(hex(f["addr"]),f["mode"],hex(f["size"]),f["name"],f["lib"])
