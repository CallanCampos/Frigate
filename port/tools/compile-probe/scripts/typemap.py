"""Which struct or union each typedef of the decomp's C names, for the layout checks of the compile probe.

    import typemap; typemap.build(REPO, layout_keys)

Scans REPO's include/, lib/*/include/, src/ and lib/*/src/ for the typedef forms the decomp writes (a typedef of a
tagged record, a typedef'd record definition, a tagged definition) and maps each typedef's name to clang's record key,
"struct Tag", or for an unnamed one the "struct (unnamed at FILE:LINE:COL)" key of the layout table given. Used by
ast_file_types.py to find the layout of the types the game overlays on the data it loads from its archives
(port/docs/research/compile-probe.md, "Struct layout hazards").
"""
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
