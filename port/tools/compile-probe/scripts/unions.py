#!/usr/bin/env python3
"""The decomp's unions whose direct members mix pointers with integers: type punning that means one thing on the DS,
where both are 32 bits, and another on a 64-bit host.

    unions.py REPO

Scans REPO's include/, src/, lib/*/include/ and lib/*/src/ and prints how many unions there are and where each mixed
one is defined. run_all.sh writes it to unions.txt (port/docs/research/compile-probe.md, "Struct layout hazards").
"""
import re, sys, pathlib
repo = pathlib.Path(sys.argv[1]); n = 0; mixed = []
for p in list(repo.glob("include/**/*.h")) + list(repo.glob("src/**/*.[ch]")) + list(repo.glob("lib/*/include/**/*.h")) + list(repo.glob("lib/*/src/**/*.[ch]")):
    t = p.read_text(errors="replace")
    for m in re.finditer(r"\bunion\b[^{;()]*\{", t):
        i = m.end(); depth = 1; j = i
        while j < len(t) and depth:
            depth += {"{": 1, "}": -1}.get(t[j], 0); j += 1
        body = t[i:j - 1]
        # direct members only: drop nested braces
        flat = re.sub(r"\{[^{}]*\}", "{}", body)
        flat = re.sub(r"\{[^{}]*\}", "{}", flat)
        flat = re.sub(r"//.*", "", flat)
        mems = [s.strip() for s in flat.split(";") if s.strip()]
        has_ptr = any(re.search(r"\*\s*\w+(\[\w*\])?$", s) or re.search(r"\(\s*\*\s*\w+\s*\)", s) for s in mems)
        has_int = any(re.match(r"^(const\s+)?(u8|u16|u32|s8|s16|s32|int|unsigned|BOOL|fx32|long)\b[^*]*$", s) for s in mems)
        n += 1
        if has_ptr and has_int: mixed.append(f"{p.relative_to(repo)}:{t.count(chr(10), 0, m.start()) + 1}")
print(f"unions: {n}; mixing pointer and integer members: {len(mixed)}")
for x in mixed: print("  ", x)
