#!/usr/bin/env python3
"""Find a declaration order whose MWCC data layout equals the ROM's, for a file whose
objects resist structured reordering (see docs/matching.md, "invert the model").

MWCC heapsorts a file's data objects by size, so the layout permutation depends only on
the sequence of sizes by declaration slot. For a given sequence, the ROM's layout forces
which object every slot must hold. This tool anneals that sequence until the forced
assignment defines every table before the initializers that reference it (its cost), and
optionally polishes a feasible sequence for naturalness (same-demo/kind adjacency by
name prefix) without raising the cost. The object list, sizes and reference edges come
from the compiled object file; the wanted order from the module's symbols.txt. A slot or
two may stay in conflict after long runs: a forward declaration of the referenced table
satisfies the reference without moving it, since MWCC lists an object where it is
defined (demo3d_data.c of overlay 293 keeps one).

Usage:
  rodata_solver.py src/ovNNN/file.c build/b2_us/src/ovNNN/file.o \\
      config/b2_us/arm9/overlays/ovNNN/symbols.txt --range 0xSTART-0xEND \\
      [--seconds 600] [--seed 7] [--start-sigma FILE] [--polish] [--out PREFIX]

Writes PREFIX_assign.txt (slot, object, size: the declaration order to realize in the
source) and PREFIX_sigma.txt (the size sequence, reusable with --start-sigma). Rerun
with --start-sigma and new seeds to continue from the best sequence found.
"""
import argparse
import random
import re
import time
from collections import defaultdict
from pathlib import Path

from elftools.elf.elffile import ELFFile

SEPARATE_SIZE = 64


def read_elf(obj_path):
    """Objects {name: size} and reference edges {(referencer, referenced)}."""
    with open(obj_path, "rb") as f:
        elf = ELFFile(f)
        syms = list(elf.get_section_by_name(".symtab").iter_symbols())
        objects = {}
        for s in syms:
            if s["st_info"]["type"] == "STT_OBJECT" and not s.name.startswith((".", "@")):
                objects[s.name] = (s["st_size"], s["st_shndx"], s["st_value"])
        by_sec = defaultdict(list)
        for n, (sz, sec, off) in objects.items():
            by_sec[sec].append((off, sz, n))
        for sec in by_sec:
            by_sec[sec].sort()

        def owner(sec, off):
            for o, sz, n in by_sec.get(sec, []):
                if o <= off < o + sz:
                    return n
            return None

        edges = set()
        for section in elf.iter_sections():
            if section["sh_type"] not in ("SHT_RELA", "SHT_REL"):
                continue
            tsec = section["sh_info"]
            if tsec not in by_sec:
                continue
            for rel in section.iter_relocations():
                src = owner(tsec, rel["r_offset"])
                tname = syms[rel["r_info_sym"]].name
                if not tname or tname.startswith("."):
                    xsec = syms[rel["r_info_sym"]]["st_shndx"]
                    tname = owner(xsec, rel["r_entry"].get("r_addend", 0))
                if src and tname and src != tname and tname in objects:
                    edges.add((src, tname))
    return {n: v[0] for n, v in objects.items()}, edges


def declaration_order(src_path, names):
    decl = [m.group(1) for m in re.finditer(
        r"^(?:static\s+)?const\s+\w+\s+\**(\w+)(?:\[[^\]]*\])?\s*=",
        Path(src_path).read_text(), re.M) if m.group(1) in names]
    missing = set(names) - set(decl)
    if missing:
        raise SystemExit(f"not found as const definitions in {src_path}: {sorted(missing)[:5]}")
    return decl


def rom_order(symbols_path, rng, names):
    rom = sorted((int(m.group(2), 16), m.group(1)) for m in (
        re.match(r"(\S+) kind:data\(\w+\) addr:(0x[0-9a-f]+)", l)
        for l in Path(symbols_path).read_text().splitlines())
        if m and m.group(1) in names and rng[0] <= int(m.group(2), 16) < rng[1])
    return [n for _, n in rom]


def heap_out(sigma):
    """Output position -> declaration slot, under MWCC's size heapsort and sections."""
    n = len(sigma)
    a = list(range(n - 1, -1, -1))
    key = sigma.__getitem__

    def sift(root, end):
        while True:
            child = 2 * root + 1
            if child > end:
                return
            if child + 1 <= end and key(a[child]) < key(a[child + 1]):
                child += 1
            if key(a[root]) < key(a[child]):
                a[root], a[child] = a[child], a[root]
                root = child
            else:
                return

    for start in range(n // 2 - 1, -1, -1):
        sift(start, n - 1)
    for end in range(n - 1, 0, -1):
        a[0], a[end] = a[end], a[0]
        sift(0, end - 1)
    shared, out = [], []
    for slot in a:
        if sigma[slot] >= SEPARATE_SIZE:
            out.append(("own", slot))
        else:
            if not shared:
                out.append(("shared", None))
            shared.append(slot)
    flat = []
    for kind, slot in out:
        flat.extend(shared if kind == "shared" else [slot])
    return flat


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("source")
    ap.add_argument("object")
    ap.add_argument("symbols")
    ap.add_argument("--range", required=True, help="module data range START-END (hex)")
    ap.add_argument("--seconds", type=float, default=600)
    ap.add_argument("--seed", type=int, default=7)
    ap.add_argument("--start-sigma", help="continue from this sigma file")
    ap.add_argument("--polish", action="store_true",
                    help="maximize naturalness instead, keeping the cost at its start value")
    ap.add_argument("--out", default="rodata_solver", help="output file prefix")
    args = ap.parse_args()

    rng = tuple(int(x, 16) for x in args.range.split("-"))
    sizes, edges = read_elf(args.object)
    decl = declaration_order(args.source, set(sizes))
    rom = rom_order(args.symbols, rng, set(sizes))
    if set(rom) != set(decl):
        raise SystemExit("symbols.txt does not cover the object's data symbols in the range")
    rom_idx = {n: k for k, n in enumerate(rom)}
    edge_k = [(rom_idx[a], rom_idx[b]) for a, b in edges]
    n = len(decl)
    prefix = {nm: nm.split("_")[0] for nm in decl}

    def evaluate(sigma):
        flat = heap_out(sigma)
        q = [0] * n
        assign = [None] * n
        for k, slot in enumerate(flat):
            q[k] = slot
            assign[slot] = rom[k]
        viol = [e for e in edge_k if q[e[1]] > q[e[0]]]
        nat = sum(1 for x, y in zip(assign, assign[1:]) if prefix[x] == prefix[y])
        return viol, nat, assign, q

    if args.start_sigma:
        sigma = [int(x) for x in Path(args.start_sigma).read_text().split()]
    else:
        sigma = [sizes[nm] for nm in decl]
    v, nat, assign, q = evaluate(sigma)
    print(f"objects {n}, edges {len(edge_k)}; start: cost {len(v)}, naturalness {nat}")

    random.seed(args.seed)
    cap = len(v)
    best = (len(v), -nat)
    best_sigma = list(sigma)
    cur = best
    t0 = time.time()
    T0 = 0.6 if args.start_sigma or args.polish else 6.0
    it = 0
    while time.time() - t0 < args.seconds:
        it += 1
        T = max(0.03, T0 * pow(0.5, (time.time() - t0) / (args.seconds / 6)))
        if v and not args.polish and random.random() < 0.8:
            ka, kb = random.choice(v)
            i = q[kb]
            j = q[ka] if random.random() < 0.5 else (random.randrange(i) if i else q[ka])
        else:
            i, j = random.randrange(n), random.randrange(n)
        if sigma[i] == sigma[j]:
            continue
        sigma[i], sigma[j] = sigma[j], sigma[i]
        nv, nnat, _, nq = evaluate(sigma)
        if args.polish:
            ok = len(nv) <= cap and (nnat >= cur[1] * -1
                                     or random.random() < pow(2.718, (nnat + cur[1]) / (T * 50)))
            score = (len(nv), -nnat)
        else:
            d = len(nv) - cur[0]
            ok = d <= 0 or random.random() < pow(2.718, -d / T)
            score = (len(nv), -nnat)
        if ok:
            cur, v, q = score, nv, nq
            if score < best:
                best, best_sigma = score, list(sigma)
                if best[0] == 0 and not args.polish:
                    break
        else:
            sigma[i], sigma[j] = sigma[j], sigma[i]

    v, nat, assign, _ = evaluate(best_sigma)
    print(f"done: {it} iterations, cost {len(v)}, naturalness {nat}")
    for a, b in ((rom[x], rom[y]) for x, y in v):
        print(f"  unsatisfied: {a} references {b} (forward-declare {b}, or rerun)")
    Path(args.out + "_assign.txt").write_text(
        "".join(f"{i} {nm} {sizes[nm]}\n" for i, nm in enumerate(assign)))
    Path(args.out + "_sigma.txt").write_text(" ".join(map(str, best_sigma)))
    print(f"written {args.out}_assign.txt (declaration order) and {args.out}_sigma.txt")


if __name__ == "__main__":
    main()
