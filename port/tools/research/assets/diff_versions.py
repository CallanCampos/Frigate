"""Which NitroFS files differ between Black 2 and White 2.

    diff_versions.py EXTRACT_B2_FILES EXTRACT_W2_FILES

Walks both versions' extracted files (extract/<version>/files), lists the files only one version has, and prints
each common file whose bytes differ, with how many members differ for a NARC. Research of 2026-10-16
(port/docs/research/assets-arm7-sound.md), behind the plan of one program for both versions (PLAN.md D6).
"""
import sys, os, hashlib, struct
a, b = sys.argv[1], sys.argv[2]
def walk(r):
    s = {}
    for dp, dn, fn in os.walk(r):
        for f in fn:
            p = os.path.join(dp, f)
            s[os.path.relpath(p, r)] = p
    return s
A, B = walk(a), walk(b)
print('only b2', sorted(set(A) - set(B)))
print('only w2', sorted(set(B) - set(A)))
diff = []
same_bytes = 0
for k in sorted(set(A) & set(B)):
    da = open(A[k], 'rb').read(); db = open(B[k], 'rb').read()
    if da != db:
        extra = ''
        if da[:4] == b'NARC' and db[:4] == b'NARC':
            def mem(d):
                off = struct.unpack_from('<H', d, 12)[0]
                sz, n = struct.unpack_from('<IH', d, off + 4)
                fat = [struct.unpack_from('<II', d, off + 12 + i * 8) for i in range(n)]
                o2 = off + sz; o3 = o2 + struct.unpack_from('<I', d, o2 + 4)[0]; base = o3 + 8
                return [d[base + s:base + e] for s, e in fat]
            ma, mb = mem(da), mem(db)
            nd = sum(1 for x, y in zip(ma, mb) if x != y)
            extra = f'members {len(ma)}/{len(mb)} differing {nd}'
        diff.append((k, len(da), len(db), extra))
    else:
        same_bytes += len(da)
print('common', len(set(A) & set(B)), 'differing', len(diff), 'identical bytes', same_bytes)
for d in diff: print(*d)
