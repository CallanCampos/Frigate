<!-- Records how Frigate is to hold itself to the melonDS oracle at every game frame without running melonDS for every
change (verification v2): a digest of the DS's display state and, later, the pointer-free game state at each game
frame, 3D as its clipped polygon list, compared with oracle runs kept in a local cache keyed by their inputs and
stopped at the first frame that differs; and where its hash, xxHash, comes from. -->
# 0031: Per-game-frame digests against a content-addressed oracle cache

- **Status:** proposed
- **Date:** 2026-11-02
- **Plan:** §6 (verification: T3 routes, T4 the oracle, T5 sanitized routes), §7 phase 5 (the whole game, hardened)
- **Upstream:** `9496bc869b` (upstream's of 2026-10-28), as Frigate's `main` (`d19894b1f7`) has it
- **Note (2026-11-02):** drafted from a design of 2026-11-01 and a prototype outside the repository, which added a
  digest writer to both programs and ran the title sequence and the aspertia route, Black 2. Proposed in the pull
  request of its first part (the canonical encoding, its format and its tests, [below](#consequences)), which wires
  the digest into neither program. It stays proposed until the owner confirms the defaults that
  [Consequences](#consequences) marks as the owner's to confirm; the change that records that sets the status line.

## Context

`oracle_<seq>` runs the player's ROM on melonDS beside Frigate and compares every port frame: the 2D screens exactly,
the 3D screens within a pixel tolerance, `NNS_G3dGlb` and GFL's random state
([oracle README](../../tools/oracle/README.md#comparing)). melonDS is about 95% of its cost: `oracle_aspertia` takes
about 1,900 s, the port alone about 100 s ([STATUS.md](../STATUS.md)). The port-alone checks that exist,
`oracle_<seq>_hashes`, keep the oracle's hashes of every 12th frame and can't judge a screen with 3D; the sequences
that start from saves only melonDS can make have none. So most changes need half-hour oracle runs, a 3D tolerance can
hide a wrong vertex, and a failing route runs to its end before anyone sees where it went wrong.

Three facts, measured by the prototype, make an exact, port-alone check possible:

- Frigate's geometry engine ([port/gfx/ge/ge.c](../../gfx/ge/ge.c)) follows melonDS's measured arithmetic, and its
  render list after clipping and the viewport is the same as melonDS's polygon RAM: identical at 14,674 of the aspertia
  route's 14,689 game frames (6.2 million polygons, clipped ones included) and 2,476 of the title's 2,491. The frames
  that differ are the `CAMERA` difference the sequences already declare with its cause.
- The hardware state the screens are drawn from (VRAM, palettes, OAM, the 2D registers decoded as melonDS keeps them)
  is equal at every game frame of both runs but for a few differences, each with a traced cause, two of them invisible
  on screen (a palette fade a step apart, stale opening data in VRAM).
- melonDS runs the same inputs to the same outputs every time, and its savestates restore to the same digests: 8,600
  frames after a restore were byte-identical, but for the first frame's 3D picture, which melonDS draws before the
  frame starts and doesn't save.

Comparing the two programs' state needs one canonical form of it. melonDS 1.1 keeps some registers decoded (the
windows' coordinates split, BLDY clamped, BG2X and BG3X sign-extended) and drops the bits its writes mask; Frigate keeps
the bytes the game wrote. Strips share vertices in both, by different rules, and the two programs' depth differs where
the list W-buffers (and differed where W is 0, until Frigate took melonDS's `0x7ffe00` there, below). The repository
keeps no third-party code in its tree: its dependencies are fetched at configure time, pinned, into the build directory.
minicoro, a single header like xxHash, is fetched by every host build ([port/cmake/os.cmake](../../cmake/os.cmake),
pinned to a commit) and put on a system include path; SDL3 is fetched when no installed SDL3 serves
([port/CMakeLists.txt](../../CMakeLists.txt), by the SHA-256 of its release archive); melonDS, for the oracle alone, the
same way ([melonds.cmake](../../tools/oracle/melonds.cmake)).

## Decision

1. **A digest per game frame.** Both programs write a record for each value of GFL's frame count, taken at the last
   VBlank with that count (where its pass has finished), at line 192 before the VBlank interrupt is taken. A record
   holds one 64-bit hash per group:
   - hardware: each VRAM bank, the VRAM mapping, the palettes, OAM, each 2D engine's registers decoded and masked
     (master brightness and display capture's control among them), POWCNT1 and WRAMCNT, the frame's display capture;
   - 3D: the render list in submission order, every polygon's attributes and every vertex's screen position, depth, W,
     colour and texture coordinates, dereferenced; the rendering registers; the list's size and RAM overflow;
   - the 2D screens;
   - later, with their probes: the sound driver as the ARM9 sees it at the VBlank, and pointer-free game state (GFL's
     random state, `NNS_G3dGlb`, the main save's 73 blocks with the play time and CRCs masked, the player's place, the
     procs' and overlays' ids).

   One plain C11 file pair, [port/tools/oracle/digest.c](../../tools/oracle/digest.c) and `digest.h`, compiled into
   both programs as `pbwinput.c` and `display3d.c` are, decodes, canonicalizes, encodes and hashes every group; each
   program fills its structures from its own model. Its canonical rules: registers masked to the bits the hardware
   keeps and decoded where melonDS keeps a decoded form; vertices dereferenced; the depth the Z-buffer's even where the
   list W-buffers (melonDS's computed from clip Z and W as its `SubmitPolygon` does, Frigate's as its `ToScreen` gives
   it), and 0 where W is 0; the VRAM a capture of 3D holds zeroed. The stream is `digest.tsv`, format `#pbwdigest 1`
   ([oracle README](../../tools/oracle/README.md#digests)). Full canonical bytes are written only for frames asked
   for.
2. **Exactness.** Every group is compared exactly at every game frame that the sequence pairs on its count. A VRAM
   region that holds a capture of 3D is left out of the bytes on both sides and held through the 3D groups. The 3D
   picture itself is held by `oracle_<seq>`'s tolerance check and by a renderer corpus made from the full dumps.
   Differences are declared per group and frame in `SEQUENCES`, each with its traced cause, as today; `MID_PASS`
   declarations are no longer needed.
3. **A content-addressed oracle cache.** An oracle run is stored under `sha256` of its inputs: the melonDS pin, the
   oracle's sources, the ROM, the input file, the start save, the options and the resolved dump table. Entries hold the
   run directory, the digest stream, the 3D frames' pictures, the saves and melonDS savestates every 1,000 oracle
   frames. The cache is per user and machine, outside every checkout (`$FRIGATE_ORACLE_CACHE`, else the platform's user
   cache directory), never committed or uploaded, because its contents derive from the ROM. Each sequence's stream has
   a committed SHA-256 fingerprint that a regenerated entry must reproduce.
4. **Tests.** `digest_<seq>` runs the port alone against the cached stream, compares each record as it is made and
   stops at the first divergent frame; it then reruns a window before that frame on both programs, melonDS from its
   nearest checkpoint, and reports the first differing field with both values. `oracle_<seq>` keeps the full
   comparison and runs melonDS only when its key isn't cached. A refresh, run on demand, regenerates every entry and
   compares it with the cache. CMake test presets name the tiers for what they run. `digest_format_plain` and
   `digest_format_sanitized` test the canonical encoding without a ROM, in CI.
5. **Native state only.** Digests are taken from the DS-native path. Enhancements that only change presentation
   (widescreen among them, [ADR 0030](0030-widescreen-by-an-engine-side-hor-plus-scale-with-native-readbacks.md)) leave
   every digest equal; those that change gameplay are excluded from oracle comparisons.
6. **The hash is XXH3-64 from xxHash, fetched at configure time as minicoro is.** The digest's hash is xxHash's
   XXH3-64 with seed 0, whose output xxHash fixed at its 0.8.0 release, so that `xxhsum -H3` gives a group's hash from
   its canonical bytes. Its single header comes from xxHash's 0.8.3 release, which
   [port/tools/oracle/xxhash.cmake](../../tools/oracle/xxhash.cmake) pins by tag and by the SHA-256 of GitHub's archive
   of it, as `melonds.cmake` pins melonDS; the build fetches the archive at configure time into the build directory and
   puts its top directory on a system include path, as `os.cmake` does minicoro's header, so the repository keeps no
   third-party code in its tree and xxHash's own code is held to its own warnings. That release's `xxhash.h` is byte for
   byte the one melonDS 1.1 bundles, so the oracle compiles the same hash whichever copy it includes. A test pins what
   the code computes: `digest_format_*` checks XXH3-64 against xxHash's own known answers. `digest.c` includes the
   header with `XXH_INLINE_ALL`, which keeps xxHash's functions private to it, so that they can't meet the copy of
   xxHash that melonDS's core links into the oracle.

## Alternatives

- **Keep comparing pictures, with the cache alone (research R1 without R2):** cheaper to build, but a 3D screen still
  needs the oracle's PNG files and a tolerance, which forgives a vertex a pixel off, and state that isn't on screen
  (the prototype's latent palette fade and stale VRAM) is never seen.
- **Compare 3D pictures exactly by porting melonDS's rasterizer into Frigate:** Frigate's renderer would become
  melonDS's code, and the renderer's own correctness would still not be separated from the game's.
- **Compare all of guest memory, as zelda3 does:** Frigate's game memory holds host pointers and host layouts by design
  ([ADR 0004](0004-ds-addresses-by-header-translation.md),
  [ADR 0021](0021-root-heaps-in-the-games-own-memory-from-host-memory.md)); only pointer-free state compares.
- **Commit the oracle's full streams to git so that no cache is needed:** megabytes per route and version, rewritten at
  every oracle input change, and still useless for the window rerun, which needs melonDS.
- **Pair on the first VBlank of a count, as `compare.py` does:** it puts the DS's mid-pass state against Frigate's
  finished one and needs `MID_PASS` declarations; the prototype found more of these in the new groups than the current
  declarations list.
- **A GitHub self-hosted runner for the oracle:** a ROM-holding runner that fork PRs can reach; the cache and a local
  runner keep ROM-derived data on the owner's machine ([REPOSITORY-POLICY.md](../REPOSITORY-POLICY.md)).
- **The hash:** the prototype's own multiply-xorshift mix needs no dependency, but has no published known answers or
  tools, and took 0.9-1.0 ms for VRAM's 656 KiB where XXH3-64 took 0.17-0.23 ms (arm64, `-O2`, a loaded machine,
  2026-11-02). SHA-256, which the cache uses for its keys, is slower still for about a mebibyte a frame, and its
  strength buys nothing here: the digest guards against accidents, not adversaries, and two different states hash
  alike at one group and frame with odds of 2^-64, about 5 × 10^-14 over a route's million group hashes.
- **xxHash's header vendored in `port/third_party/`:** it would be the first third-party code in the tree, a second
  convention beside the one minicoro, SDL3 and melonDS follow, for a single header like minicoro's; its files couldn't
  open with the description every port file opens with ([port/CLAUDE.md](../../CLAUDE.md#code-that-modders-can-read)),
  so that rule would need an exception; and it would save nothing, since every host build already fetches minicoro at
  configure time, and the pinned SHA-256 holds the archive's bytes. **An installed xxHash** (`find_package`,
  pkg-config): one more dependency for players and CI to install, at a version the build doesn't choose, for one
  header. **xxHash taken from melonDS's archive at build time:** Frigate's own program must never need melonDS
  ([PLAN.md §1](../PLAN.md#1-goal-and-what-native-means)).

## Consequences

- A change that leaves the oracle's inputs alone is to be verified on every game frame, 3D included, in about the time
  of a port run, and a failure names its first frame and field. The sequences that start from oracle-made saves get a
  port-alone check for the first time.
- The check is stricter than the screens: it finds differences that never reach the screen, each of which needs a
  traced cause before it can be declared. The prototype found two in aspertia.
- Frigate is held to melonDS's register semantics (writes ignored while an engine is off, decoded storage). A
  difference found on hardware is declared with its evidence.
- Frigate's `ToScreen` ([port/gfx/ge/ge.c](../../gfx/ge/ge.c)) computes a vertex's depth as melonDS's `SubmitPolygon`
  does, cutting the product to 32 bits before its clamp and giving `0x7ffe00` where W is 0
  ([digest.h](../../tools/oracle/digest.h), `pbw_digest_zbuffer_depth`; `g3d_ge`'s depth cut cases). The cut matters
  only for a vertex whose clip W is 2^24 or more, where W's truncation to 24 bits can leave a small divisor; the
  prototype computed melonDS's side without the cut, so its equal lists don't cover that case. `ToScreen` clamped the
  whole 64-bit product, and gave 0 where W is 0, until a change of its own, with its replays, made it cut before the
  second part compares lists.
- A digest costs about 0.4 ms a frame on Frigate in the prototype, with a slower hash; checkpoints take about 20 MB
  each before compression.
- A machine without the oracle can't fill the cache: until it has one, the committed `_hashes` references are its
  check.
- **xxHash at configure time.** Every host build fetches xxHash's pinned archive (1.1 MB) at configure time, beside
  minicoro, into its build directory. Updating xxHash means changing the tag and SHA-256 in `xxhash.cmake` and running
  `digest_format_*`, whose known answers hold XXH3's output. xxHash is BSD-2-Clause, which GPL-3.0 code may include; a
  binary that links it must carry its notice in the documentation it ships with, so the change that compiles the
  digest into Frigate's program adds the notice, from the archive's `LICENSE`, to what the program and the macOS app
  carry. Until then only the tests link it.
- **Built in six parts**, each landable alone and each verified with the full suite on its head: (1) the canonical
  encoding, its format and its ROM-free tests, wired into neither program; (2) the digest stream in both programs, the
  comparison with last-VBlank pairing, lags and declarations, `oracle_<seq>` checking it, and the declarations of the
  opening, the title and aspertia in both versions; (3) the oracle cache and the port-alone `digest_<seq>` tests; (4)
  checkpoints, the comparison as Frigate runs, the window rerun and the first-divergence report; (5) the game-state
  groups and the sequences that start from saves; (6) the test presets and the determinism test.
- **The owner's to confirm**, written here as defaults that the parts follow until the owner says otherwise:
  1. *The merge gate* stays "the full suite on the exact head" ([REPOSITORY-POLICY.md](../REPOSITORY-POLICY.md)).
     Once part 3 lands, a `pr` test preset of the port-alone digest tests replaces the oracle runs in the per-PR
     suite. (The owner's to confirm.)
  2. *No scheduled runs.* The owner declined scheduled nightly and weekly runs on 2026-11-01 and asked that long checks
     be optional where they would block pull requests. The long tiers (the oracle where its inputs changed, the
     cache's refresh, the sanitized build's digests, the other macOS slice) are on-demand presets, named for what they
     run rather than "nightly" or "weekly". (The owner's to confirm.)
  3. *The cache* lives in the per-user cache directory above (`~/Library/Caches/Frigate/oracle` on macOS,
     `$XDG_CACHE_HOME/frigate/oracle` on Linux, `%LOCALAPPDATA%\Frigate\oracle` on Windows), with a default cap of 20
     GiB that `oracle.py cache prune` keeps it under, the least recently used entries going first. (The owner's to
     confirm.)
  4. *The committed `_hashes` references* stay as the cold tier, for a machine with the ROM and no oracle, until the
     owner decides their fate. (The owner's to confirm.)
  5. *Every latent difference* the digest finds is declared with its traced cause, as the prototype's are; none counts
     as a warning instead. (The owner's to confirm.)
  6. *The fingerprint files* (`port/tests/oracle/digest/<seq>_<version>.fingerprint`, hashes of the oracle's output, as
     the `_hashes` references are) are committed. (The owner's to confirm.)
- Revisit if melonDS's pin changes how it stores the registers or the polygon RAM the digest reads, if Frigate gains a
  timing model that changes how passes fall on VBlanks, if an enhancement can't keep the native state computable beside
  its own, or if xxHash changes XXH3's output (it has promised not to since 0.8.0).
