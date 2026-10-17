# Game changes

Every change the port makes outside `port/`, other than its own new files under `.github/` (`README.md` and
`workflows/port-*.yml`). Game code stays the decomp's ([PLAN.md §4.1](PLAN.md#41-repository),
[§4.8](PLAN.md#48-64-bit-and-dual-target-rules)), so this list should stay short, and every row is something an
upstream merge can conflict with. A change outside `port/` without a row here is a mistake.

## The three allowed kinds

1. **Neutral:** a portability fix that compiles to the same bytes under MWCC, such as `uptr` for a value that carries
   an address, or an SDK constant in place of a raw VRAM address. It stays in this repository: the port is a solo
   project and proposes nothing upstream ([ADR 0002](adr/0002-solo-project-no-upstream-posts.md)).
2. **Hook:** a hook macro (`PORT_EVENT`, `PORT_SHOULD`) that compiles to nothing, or to the bare expression, in the
   ROM build.
3. **Platform:** an `#ifdef PLATFORM_PC` block, used rarely and commented in the code. `include/platform.h` is the only
   place that decides between MWCC and a host. Memory-safety fixes for documented `// BUG:`s that 64-bit layouts make
   dangerous are this kind.

A change that fits none of these doesn't go in.

## Proving a change is MWCC-neutral

Every kind needs the same proof, since the code around a hook or a platform block, and its `#else`, still compiles
under MWCC.

- In a **complete** file, `python3 configure.py && ninja` with both SHA1s matching is the proof.
- In an **incomplete** file (its `delinks.txt` entry isn't marked `complete`) the ROM still links the original object,
  so the ROM build proves nothing about its C. Run
  `.venv/bin/python tools/decomp/compiler_probe.py FILE --compilers 1.1p1 --mismatches` for both versions (add
  `--version w2_us`) before and after, and show that no function's result changed.
- A **header** change (`include/platform.h` in a common header, a retyped API, a hook macro) changes every file that
  includes it: ninja proves the complete ones, and the probe runs on every incomplete file that includes the header,
  directly or through another header, in both versions. After a build, `ninja -t deps` lists each object's headers;
  when in doubt, probe every incomplete file.
- A **hook** must also expand to nothing or to its bare argument when `PLATFORM_PC` is undefined, and a **platform**
  block must leave the code MWCC compiles as it was; `PLATFORM_PC` is defined only by the host build.

## Columns

| Column | What to write |
| --- | --- |
| File | Path from the repository root; one row per file, or per header and its users when one macro change covers many |
| Kind | Neutral, Hook or Platform |
| Guard or macro | The macro, typedef or `#ifdef` that carries the change |
| Why | What breaks on a host without it, in one line |
| MWCC-neutral proof | For every kind: "ninja, both SHA1s" for complete files; the probe result for incomplete files, every including one for a header |
| Commit | The port commit that made the change, short SHA. A row is committed with its change, so it can't name that commit: write `PORT_SHA`, and the next commit to this file replaces it with the SHA from `git log -1 --format=%h -- port/docs/game-changes.md`, run before committing |

When a sync conflicts with a row's change, resolve it by taking upstream's version and reapplying the change, at the
code's new place if upstream moved it, then update the row's commit. When upstream makes the same change on its own,
git usually merges it without a conflict, so nothing flags it and no tool drops the row. So after each of its syncs, a
session deletes every row whose files all match upstream again (`git diff upstream/main -- FILE` prints nothing), in
its next commit; on a conflict, [SYNC.md](SYNC.md#resolving-conflicts), step 3, drops it. The git history of this
file keeps what the port needed.

## Changes

| File | Kind | Guard or macro | Why | MWCC-neutral proof | Commit |
| --- | --- | --- | --- | --- | --- |

None yet.
