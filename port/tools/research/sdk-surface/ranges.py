import pickle,sys
from pathlib import Path
db=pickle.load(open(Path(__file__).parent/"db.pkl","rb"))
m=db["mods"][sys.argv[1]]
rs=sorted((e["sections"][".text"][0],e["sections"][".text"][1],e["path"],e["complete"]) for e in m["delinks"] if ".text" in e["sections"])
prev=None
for s,e,p,c in rs:
    d=p.rsplit("/",1)[0]
    if len(sys.argv)>2 or d!=prev:
        print(hex(s),hex(e),p,"C" if c else "")
    prev=d
