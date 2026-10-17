import sys, pathlib, collections, re
repo = pathlib.Path(sys.argv[1]); c = collections.Counter(); cache = {}
for loc in open(sys.argv[2]):
    f, l, _ = loc.strip().rsplit(":", 2)
    if f not in cache: cache[f] = (repo / f).read_text(errors="replace").splitlines()
    if re.search(r"OVERLAY_(ID|[A-Z])", cache[f][int(l) - 1]): continue
    c[f] += 1
print("game-code 64-bit pointer/int sites excluding overlay IDs:", sum(c.values()), "in", len(c), "files")
for f, n in c.most_common(int(sys.argv[3])): print(f"  {n:4d} {f}")
