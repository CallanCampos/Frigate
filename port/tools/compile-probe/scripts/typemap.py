"""typedef name -> clang record key ("struct Tag" or "struct (unnamed at f:l:c)") for the repo's headers and sources."""
import re, pathlib
def build(repo, diff_keys):
    repo = pathlib.Path(repo)
    by_loc = {}
    for k in diff_keys:
        m = re.search(r"\((?:unnamed|anonymous) (?:struct |union )?at ([^:]+):(\d+):\d+\)$", k)
        if m and "::" not in k: by_loc[(m[1], int(m[2]))] = k
    tmap = {}
    files = list(repo.glob("include/**/*.h")) + list(repo.glob("lib/*/include/**/*.h")) + list(repo.glob("src/**/*.[ch]")) + list(repo.glob("lib/*/src/**/*.[ch]"))
    for p in files:
        f = str(p.relative_to(repo)); L = p.read_text(errors="replace").splitlines()
        for i, l in enumerate(L):
            m = re.match(r"^\s*typedef\s+(struct|union)\s+(\w+)\s+(\w+)\s*;", l)
            if m: tmap.setdefault(m[3], f"{m[1]} {m[2]}"); continue
            m = re.match(r"^\}\s*(\w+)\s*;", l)
            if m:
                depth = 0
                for j in range(i, -1, -1):
                    depth += L[j].count("}") - L[j].count("{")
                    if depth == 0:
                        mm = re.search(r"typedef\s+(struct|union)\s*(\w+)?\s*\{", L[j])
                        if mm: tmap.setdefault(m[1], f"{mm[1]} {mm[2]}" if mm[2] else by_loc.get((f, j + 1), "?"))
                        break
            m = re.match(r"^\s*(?:typedef\s+)?(struct|union)\s+(\w+)\s*\{", l)
            if m: tmap.setdefault(m[2], f"{m[1]} {m[2]}")
    return tmap
