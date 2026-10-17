# Game changes

Every change the port makes outside `port/`, other than its own new files under `.github/` (`README.md` and
`workflows/port-*.yml`). Game code stays the decomp's ([PLAN.md §4.1](PLAN.md#41-repository),
[§4.8](PLAN.md#48-64-bit-and-dual-target-rules)), so this list should stay short, and every row is something an
upstream merge can conflict with. A change outside `port/` without a row here is a mistake.

## The three allowed kinds

1. **Neutral:** a portability fix that compiles to the same bytes under MWCC, such as `uptr` for a value that carries
   an address, or an SDK constant in place of a raw VRAM address. Each is also offered upstream, with the user's OK
   for the post.
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
| Offered upstream | "no" (Hook and Platform rows by default), "to offer", or the upstream PR or issue number once the user OKs the post |
| Commit | The port commit that made the change, short SHA. A row is committed with its change, so it can't name that commit: write `PORT_SHA`, and the next commit to this file replaces it with the SHA from `git log -1 --format=%h -- port/docs/game-changes.md`, run before committing |

When upstream takes a change, the row stays, with "merged upstream in #NNN" under Offered upstream, so the history of
what the port needed is kept. When a sync conflicts with a row's change, resolve it by taking upstream's version and
reapplying the change, then update the row's commit.

## Changes

| File | Kind | Guard or macro | Why | MWCC-neutral proof | Offered upstream | Commit |
| --- | --- | --- | --- | --- | --- | --- |

None yet.
