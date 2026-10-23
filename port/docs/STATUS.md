# Port status

Where the port stands, newest entry first. A session adds an entry after each local sync with
`port/tools/sync_upstream.sh` and each finished task ([PLAN.md §8](PLAN.md#8-how-each-development-iteration-runs)), in
the same commit as the work it records; a proposed hourly `port-sync` PR gets no completed-sync entry
([SYNC.md](SYNC.md)). The plan of record is [PLAN.md](PLAN.md); every change outside `port/` is in
[game-changes.md](game-changes.md).

## How to add an entry

Put the new entry above the newest one, under a heading `## YYYY-MM-DD: title`. Fill every field. A measure or test
level that doesn't exist yet says "not yet"; no field is left out.

Evidence is what someone else can open or run again: a commit, a pull request, a CI run, a commit status, or the
command and the result it gave, in the entry. A log in `/tmp` or another temporary directory is not evidence, since
no one else can read it and it doesn't outlive the machine's next restart: write the command and its result instead.
Past entries are records, added to and never rewritten ([SYNC.md](SYNC.md#records-are-added-to)): the only edits an
old entry gets are its `PORT_SHA` filled in and a typo or a broken link fixed.

| Field | What to write |
| --- | --- |
| Date | The day of the sync or task, `YYYY-MM-DD`, in the heading |
| Upstream | The `fuddlesworth/pokebw2` commit last merged, short SHA, with its PR number when it is a merge |
| Port | The port commit the entry describes. An entry committed with its own work can't name that commit: write `PORT_SHA`, and the next entry's commit replaces it with the SHA from `git log -1 --format=%h -- port/docs/STATUS.md` |
| ROM | Result of `python3 configure.py && ninja`: both SHA1s match, or what failed (T0) |
| Host build | Result per platform and configuration (T1) |
| Tests | Result of each level T2–T7 that ran, with counts; "not yet" for levels that don't exist |
| Tiers | Functions and code bytes in tiers (a), (b) and (c) from `port/tools/progress/`, and the change since the last entry |
| Trap stubs | How many functions the host build still stubs, and which the game hit first |
| 64-bit warnings | The host build's pointer/int warning count, game and `lib/` apart; it must not rise |
| Work done | What changed, with ADRs and game changes by number |
| Next | The next task: the earliest unmet exit criterion of the current phase that the decomp allows |
| Blockers | What waits on upstream or on a decision, with issue numbers |

## 2026-10-23: the process after the PR 12 sync written down, the guard refusing main, asm_oracle green again

- **Upstream:** `b62c655bfa` (upstream merge #481), which main took in PR 16 (after PR 13's `0bed98c4c`) while this
  change was under way; this change imports nothing of its own.
- **Port:** `PORT_SHA`, the branch `port-process-docs` from main `8ca29313d1`, with main merged into it twice,
  `6a1ace981b` and then `a4648fce71`, and the review's fixes on top (`0123c22721` for the guard, then this entry's).
- **ROM:** T0 passes on the head: a full `python3 configure.py && ninja` (dsd `v0.12.1-dsi.6`) matches Black 2
  `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb` and White 2 `b5d7490be7b415b8f1e672a53e978a9cc667e56a`, on main
  `8ca29313d1` before the branch's commits, again after each merge of main into it, and on the review's fixes; `ninja`
  had no work to do after each of the branch's own commits, none of which changes a file the ROM build reads. No file
  outside `port/` and `.github/` changes, so there is no game change, no game-changes.md row and no compiler probe to
  run.
- **Host build:** T1 passes on macOS arm64 for both versions (`build/port`, `build/port-w2`, RelWithDebInfo), before and
  after the merges. The one header the game's code includes that changes is `port/include/port_wcm.h` (its include guard
  and its header comment), included only under `PLATFORM_PC`.
- **Tests:** T2, T3, T4 and T6 ran in full, serially, with the melonDS oracle (`pbw-oracle` in each build's `oracle/`),
  its cached saves and `build/port-venv`, on the head with the review's fixes: Black 2, 302 tests, 291 passed, 9 failed
  and 2 skipped (4,055 s); White 2, 299 tests, 288 passed, 9 failed and 2 skipped (3,985 s), with the load average above
  40 on 8 cores. The three earlier runs, on main `8ca29313d1`, after merging `6a1ace981b` and after merging `a4648fce71`
  (Black 2 291 passed and 2 skipped, White 2 289 passed and 1 skipped), failed the same nine in each version. The nine
  are the same in both versions and fail on main too, since this branch changes none of their code: `oracle_continue`
  and `oracle_continue_starter` wait for a stop on the trap of overlay 36's `FIELDMAP_PROC_FUNCS`
  (`port/tools/oracle/oracle.py`'s CONTINUE sequences), which has C now, and the port stops instead at frame 3436 in
  `FieldmapProc_Init` (`OS_Terminate`, status 71), on the field's external heap; `oracle_launch_field_menus`,
  `oracle_launch_party_menus`, `launch_summary_standalone`, `launch_touch_then_party`, `launch_unfinished`,
  `launch_not_offered` and `frame_times_party_menus` run `--launch`, which steps aside now that the field has C, so they
  stop at the same frame. `asm_oracle`, which fails on main, passes in both versions in every run. Skipped:
  `app_universal_slices`, which only a distribution build has, and a timing gate now and then (`frame_times_title` in
  both versions on the head, `frame_times_new_game` after the second merge, `frame_times_title` on main), over its
  budget while the load average was above the cores (`frame_times.py --max-load`); each passed in another run. The sync
  tooling's 14 tests pass, `port_guard.py --self-test` passes 183 of 183, and actionlint finds nothing in `port-ci.yml`
  but its unknown `ubuntu-26.04` label. T5 not run locally (no code a sanitizer checks changed; CI's sanitized jobs run
  on the PR); T7 not run (nothing visible changed).
- **Tiers:** unchanged from the entry below, on the head too: per version, a 37,546 functions, b 3,472, c zero; code
  bytes Black 2 a 3,435,848, White 2 a 3,435,936, and b 414,418 in each; 2,384 veneers left out and 178
  inline-assembly functions among a and b (`port/tools/progress/progress.py --root . --version b2_us --out DIR`, and
  `w2_us`, its `functions.tsv` summed by tier).
- **Trap stubs:** unchanged: 31 function stubs and 22 data placeholders in each version (`gen_stubs` in both builds,
  before and after the merges). The game hits none of them: CONTINUE stops before any, in `FieldmapProc_Init` at frame
  3436, as the nine failures above show, and so does New Game at frame 10,759 (Black 2's
  `frigate-black2 --headless --frames 11000 --input port/tests/replay/new_game.pbwinput --save-dir DIR` exits 71,
  `OS_Terminate` in `FieldmapProc_Init`). The first stub on the field's route after that heap is main's `_u32_div_f`,
  as the entry below says.
- **64-bit warnings:** not remeasured, so the last figure stands: 392 (game 161, libraries 231), from the 2026-10-22
  entry's all-TU audit, which isn't committed; the pointer-cast guard (ADR 0019, reserved) is to make it reproducible.
  No C that the host build compiles changed but `port_wcm.h`'s include guard and comment.
- **Work done:**
  - **STATUS.md:** the entry below, for the PR 12 sync that merged without one; the two `PORT_SHA`s of 2026-10-22 filled
    (`88237af41d`, the last STATUS commit of PR 8's branch, and `e6becbc07c`, PR 9's), as earlier fills took a merged
    branch's last STATUS commit; and two rules in "How to add an entry": a log in `/tmp` is not evidence, and past
    entries are added to, never rewritten. STATUS.md, SYNC.md and CONTRIBUTING say the same of what a past record may
    still get: its `PORT_SHA` filled in, and a typo or a broken link fixed.
  - **`asm_oracle`:** `test_fx.py` names libfx's functions as the SDK does since PR 10's sync (`MTX_Scale22_`,
    `MTX_Scale33_` and `MTX_Copy33To44_`, where it had swan's `MAT2_Scaling`, `MAT3_Scaling` and `MAT3_To4x4`, which the
    ROM's symbols no longer have), and `test_mi.py` gives `MI_CpuFill8` its fill value as the `u8` it now takes, with
    bytes whose high bit is set and clear.
  - **The write guard:** `port_guard.py` refuses a push that would update or delete Frigate's `main` (ADR 0018), before
    GitHub's rules refuse it on the server, and its docstring and message no longer call the port a solo project. Its
    first version read only a refspec's literal destination, `HEAD` and the checkout's own settings; the review of this
    change found pushes that git sends to `main` and it let through, each shown against a scratch bare repository with
    git 2.50.1. It now works out a push's branches as `set_refspecs` in git's `builtin/push.c` does: the matching
    refspec `:` and `+:`; `heads/main`, which git takes as `refs/heads/main`; `@` and, on macOS's file system, `head`
    for `HEAD`; a refspec without a colon that `remote.<name>.push` or `push.default=upstream` maps to `main` (`HEAD`
    and `@` it doesn't); settings from `git -c` and from `git config` earlier in the command, which it refuses when it
    can't read them (`--config-env`, `GIT_CONFIG_PARAMETERS`, `GIT_CONFIG_COUNT`); a `url.<base>.insteadOf` toward the
    port; and the branch and upstream that a `git switch`, `git checkout` or `git branch` earlier in the command
    leaves, so `git switch main && git push origin HEAD` is refused. It doesn't expand git aliases, which the
    docstring and README's guard section now say with the rest. The self-test has 58 more cases than main's 125
    (183 of 183 pass). A differential run of 38 commands, each given to the guard and then run against a scratch
    bare repository (git 2.50.1), found no push that updated `main` and that the guard let through. The guard refused
    five that left `main` alone: two whose `git branch -m main` or `git checkout --track origin/main` failed because
    `main` exists, a `--delete main` that the bare repository refuses for its current branch, and two after
    `git switch -`, whose branch the guard can't name.
  - **The sync:** the PR body that `upstream_sync.py` writes gives the verification's commands, `gh pr checkout N` and
    `port/tools/sync_upstream.sh --verify-pr N`; `port-ci.yml`'s header says how the ROMs are checked under ADR 0018 and
    names the sync-tooling job and the aggregate `Frigate CI`. SYNC.md and CONTRIBUTING: a sync's merge commit carries
    only the sync's own resolutions, never feature work (`90ad973b12` carried a contribution policy, an ADR, a review
    record and templates beside its native fixes), and records are added to, never rewritten.
  - **The ADR index:** 0018 in its place and "accepted" like the others; 0001's and 0017's rows say "amended by 0018",
    as the ADRs do; 0016 in docs/README.md; 0019, 0020 and 0021 reserved for the pointer-cast guard, one program for
    both versions and root heaps in host memory, the next free number 0022. ADR 0018 opens with a header comment, as
    0017 does. decomp-findings.md's introduction no longer calls the port a solo project.
  - **Guides:** GLOSSARY's rows for the ruleset, Frigate local verification, the sync PR and sync App, and the owner
    review exception, and its stale rows fixed (upstream "merged every hour" and "three files" under `.github/`, the
    test launcher's field "with no C yet"); README's directory map and the include guide name all three headers of
    `port/include`; `port_wcm.h` takes the `PORT_*_H` guard of its neighbours and says which game code uses it;
    ARCHITECTURE.md's over-long feature-map line rewrapped, its "Contribution and upstream validation" section, which
    followed the glossary, a rule at the boundaries now, and its plain-C rule saying that the test launcher steps aside
    now that the field has C. GLOSSARY's row for Frigate local verification says that the status is posted by the
    account gh is signed in with and that only CallanCampos's counts; the ADR index's row for D6 says 0020 is reserved
    for it.
  - **lessons.md:** never run lldb on this Mac (developer mode off, its password prompt hangs every `wibo` process,
    other sessions' ROM builds included); use the sanitizers, the crash reports and logging. And a green `--verify-pr`
    says nothing about T4.
- **Next:** the field entry, PR 18 (root heaps in host memory, ADR 0021; a native `_u32_div_f`; `field_transition.c`'s
  VRAM literal; the CONTINUE sequences into the field), which closes the oracle failures above; then the launcher's
  retirement. PR 13's and PR 16's syncs merged without STATUS entries; each gets one, with its verification, as the
  PR 12 sync's below does.
- **Blockers:** **the oracle and launch tests that expected the old field trap, the nine above, fail on main in both
  versions until the field entry PR lands**; the seven that run `--launch` then need the launcher's retirement too
  (ADR 0012's note). T4 is red on main until then, and `--verify-pr`, which builds no oracle, shows green. The field's
  external heap (`g_FieldHeapMemory`, a 64 KiB root on a fixed buffer that `port_heap_size` doesn't scale) stops
  CONTINUE at frame 3436 and New Game at 10,759. Battles also wait on overlay 167's `data_ov167_021d7490`, which has no
  C and is under another contributor's claim.

## 2026-10-23: upstream sync `0a12a2bd3` merged (PR 12): the save's city state initialized by the game's own C

This entry was added afterwards, by the entry above: the sync merged without one.

- **Upstream:** `0a12a2bd3e` (upstream merge #480), four commits after `d4a30c66b0`.
- **Port:** `8ca29313d1`, the merge of [PR 12](https://github.com/CallanCampos/Frigate/pull/12): the sync App's merge
  `0222276d69` and the owner's follow-up `2575a61410`, the head that was verified and approved.
- **ROM:** T0 passes on the exact head: `Frigate local verification` is `success` on `2575a61410`, posted by
  CallanCampos after both SHA1s matched. The merge `8ca29313d1` has no status of its own; on 2026-10-23 a local
  `python3 configure.py && ninja` in a fresh worktree of it matched both SHA1s again (Black 2
  `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb`, White 2 `b5d7490be7b415b8f1e672a53e978a9cc667e56a`). The sync needs dsd
  `v0.12.1-dsi.6`, since dsd now fills the overlay digests and defines NitroSDK's linker symbols; the older
  `v0.12.1-dsi.4` fails at `dsd lcf --symbols`. `configure.py` downloads the pinned version whenever `tools/dsd.rev`
  names another, but keeps a `tools/dsd` that has no `tools/dsd.rev`, whatever its version: prepare a worktree with
  `port/tools/bootstrap_checkout.sh`, which leaves `tools/dsd` to `configure.py`, never by linking it by hand.
- **Host build:** T1 passes on macOS arm64 for both versions in the exact-head verification, and every CI job on the
  head passes: the macOS, Linux and Windows host builds, both sanitized builds, the four macOS distribution builds and
  the sync tooling's tests (`Frigate CI`), with Compile and Code standards.
- **Tests:** the exact-head verification passes T2, T3 and T6 with no failure: Black 2, 284 passed and 18 skipped
  of 302; White 2, 281 passed and 18 skipped of 299; every timing gate that ran passes. That run builds no oracle and
  has no port virtual environment, and its skips hide tests that fail on this main when both are there, in both
  versions: the nine routes that the entry above lists, which stop on the field's heap, and `asm_oracle`, whose tests
  named libfx's functions by names the ROM's symbols no longer have. None comes from this sync: the field's C arrived
  with PR 5's sync (`83515e20ce`) and the SDK's names with PR 10's (`d4a30c66b`). T4 was not run; T5 runs only in CI
  (macOS and Linux, passing); T7 not run.
- **Tiers:** unchanged from the entry below: per version, a 37,546 functions, b 3,472, c zero; code bytes Black 2 a
  3,435,848, White 2 a 3,435,936, and b 414,418 in each; 2,384 veneers left out and 178 inline-assembly functions among
  a and b (`port/tools/progress/progress.py --root . --version b2_us --out DIR`, and `w2_us`, on `8ca29313d1`). The
  sync names a call and types a save block's member; it adds no C.
- **Trap stubs:** unchanged: 31 function stubs and 22 data placeholders per version, the same symbols in both. The two
  on a route the game takes soon are main's `_u32_div_f` (MSL's division, which the field's lens-flare data calls at
  each map change) and overlay 167's `data_ov167_021d7490` (a battle table). `ReshZekSave_UnresolvedInitTarget` was a
  platform-layer no-op, not a stub, so its removal changes neither count.
- **64-bit warnings:** not remeasured: the all-TU audit behind the entry below's 392 isn't committed. The sync's game
  changes take two `(u32 *)` casts between pointers away and add no cast between a pointer and an integer.
- **Work done:** the App's merge had no conflicts. Upstream #480 finds `ReshZekSave_Init`'s call
  (`src/system/resh_zek_savedata.c`): it is overlay 12's `CityState_Init`, not the placeholder
  `ReshZekSave_UnresolvedInitTarget`; it types the save block's city state as `CityState`, drops
  `tools/build/object_aliases.py` and its helpers, and lets dsd fill the overlay digests and define NitroSDK's linker
  symbols. The owner's follow-up `2575a61410` removes `port/hal/stubs/unresolved.c`, the platform layer's no-op for the
  placeholder, and corrects its row in decomp-findings.md (the call's target was read as overlay 11's
  `WCM_RssiToQuality`, which shares its address, so the "no effect" reading was wrong) and the HAL guide. A save the
  host creates now runs the game's `CityState_Init` (`initialized` 0, `city` 2); `CityState` stays four bytes in the
  save. The owner approved the exact head once every required check had passed, and merged it with a merge commit.
- **Next:** as the entry above says.
- **Blockers:** the field's external heap, unchanged; and the failures above, until the field opens on the host.

## 2026-10-22: reviewed upstream SDK sync completed

This is the completed result of [PR 10](https://github.com/CallanCampos/Frigate/pull/10). The prepared entry below
is retained as a historical snapshot from before final verification and owner review; its pending work is resolved
by the results here.

- **Upstream:** `d4a30c66b0` (upstream merge #479), the current imported tip: 110 commits after `fc478a9ee5`.
- **Port:** reviewed main merge `8c30c6fd70`, whose Git tree is identical to the final verified PR head
  `06e40a54efab0cb56f624834aff9f3907328d73f`.
- **ROM:** the official exact-head T0 gate passes; Black 2 SHA1 `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb` and
  White 2 SHA1 `b5d7490be7b415b8f1e672a53e978a9cc667e56a` both match. The actual local verification commit status
  on that head is `success`, posted by CallanCampos, not an Actions job with the same name.
- **Host build:** T1 passes for both canonical macOS arm64 native builds. All macOS, Linux and Windows native builds,
  both sanitized builds and all four macOS distribution builds pass in the final
  [Frigate CI run](https://github.com/CallanCampos/Frigate/actions/runs/39891758039).
- **Tests:** final complete serial T2/T3/T6 suites pass: Black 2, 284 passed and 18 expected skips of 302; White 2,
  281 passed and 18 expected skips of 299. All available timing gates pass. Optional prerequisites account for the
  skips; full optional T4 comparisons and T7 playtests were not run. Hosted T5 sanitizer coverage passes on macOS
  and Linux; full local sanitized gameplay routes remain unrun. The expheap regression fails on the old code and
  passes with the fix, including ASan/UBSan. Compile, Code standards, sync tooling and the aggregate CI gate pass.
  Main merge rule suites `4677197273` (PR 7) and `4677521638` (PR 10) report every rule `pass`.
- **Tiers:** unchanged from the prepared measurement: per version, a 37,546 functions, b 3,472 and c zero. Code
  bytes: Black 2 a 3,435,848, White 2 a 3,435,936, and b 414,418 in each. Compared with the preceding completed sync,
  a gains 396 functions and 66,714 bytes per version while b loses the same amounts. Excluded veneers remain 2,384;
  SDK assembly definitions total 225, including 47 now represented in MSL's assembly file. This representation
  change is not native C progress, and source coverage does not establish native gameplay readiness.
- **Trap stubs:** unchanged, 31 function traps and 22 data placeholders per version. The observed field-entry stop
  remains `OS_Terminate` in `FieldmapProc_Init`, not a missing-field-C trap.
- **64-bit warnings:** the normalized all-TU audit reproduces the documented baseline and passes without compilation
  failures: four pointer/int cast families fall from 469 to 392 instances (game 234 to 161, libraries 235 to 231).
  Optional narrowing diagnostics also fall, 388 to 383. The comparison covers 1,655 baseline and 1,657 final native
  translation units; individual upstream-file increases were reviewed. PR 10 records the detailed proof and limits.
- **Work done:** preserved the documented [port adaptations](game-changes.md) while resolving 27 conflicting files
  across 110 upstream commits. SDK names, layouts and signatures now agree with the native HAL and tests. Fixed
  `MI_CpuFill8`'s signed high-half shift, expheap capacity arithmetic across 4 GiB address boundaries and PXI callback
  signatures exposed by Linux UBSan. Upstream's MSL assembly conversion retains the native libc/libm replacement.
  The pointer-width contribution was reviewed and merged in [PR 7](https://github.com/CallanCampos/Frigate/pull/7).
  The dedicated sync App is installed, configured and verified; protected-main review and verification rules remain
  active ([ADR 0018](adr/0018-protected-main-and-sync-prs.md)). The owner approved PR 10 before merge, all required
  checks passed, and [Issue 6](https://github.com/CallanCampos/Frigate/issues/6) is closed.
- **Next:** size the external field heap for native allocations, then repeat field-entry, CONTINUE and real-menu
  checks before claiming a playable field.
- **Blockers:** no sync, review or App-setup blocker remains. The existing gameplay blocker is the fixed 64 KiB
  external field heap: weather and place-name child allocations grow to a combined `0x1b000` before other overhead,
  and the recorded field-entry trace stops at frame 10,759. This sync does not claim to resolve that heap-sizing work.

## 2026-10-22: prepared upstream SDK sync (owner review pending)

- **Upstream:** `d4a30c66b066b2d231c7e9fd5aa83b3daca157c2`, a frozen proposal of 110 commits after
  `fc478a9ee5`. This entry records preparation; the sync has not merged into Frigate main.
- **Port:** [PR 10](https://github.com/CallanCampos/Frigate/pull/10), topic `codex/upstream-sync-resolve`: resolved
  upstream merge `bde1702ad1`, native follow-ups `fef44d52dc` and `d307558232`, and reviewed-main integration
  `be0a08a14f` and `efe30f24c8` (main `297e28d37c`). The final two upstream commits are imported by
  real merge `8c314b0f4c`, preserving both upstream parents and trailers.
- **ROM:** exact-head T0 validation remains pending. Both ROM SHA1s are required by PR 10's
  `Frigate local verification` status; its checks and description record the final exact-head results. No checksum or
  decomp configuration was changed to accommodate the port.
- **MWCC-neutrality:** the historical `03674114e` proof covered 1,779 C files in both versions (3,558
  file/version comparisons). The sweep passed 3,554; its four stale samples of `friend.c` and `dwcutil_ac.c` were
  cleared by a final 8/8 targeted rerun that also covered `vm.c` and `os_thread.c`. The later expheap change passes
  2/2. The final two-commit extension replaces one excluded MSL C file with upstream's unchanged assembly source;
  the only other MSL header change is a comment. There are now 1,778 C files and one library assembly source.
  Main's reviewed PR 7 changes three pointer-cast-only files, rechecked against `d4a30c66b` in both versions (6/6), and a
  complete trade module whose added cast token relies on PR 7's object comparison and final ROM checks, rather than
  token identity. This is not a claim that all final C files have identical token streams.
  Evidence: `/tmp/frigate-sync-neutral.log`, `/tmp/frigate-sync-neutral-final-targets.log`,
  `/tmp/frigate-sync-neutral-expheap.log` and `/tmp/frigate-sync-neutral-pr7-final.log`.
- **Host build:** preliminary T1 passes on macOS arm64 for Black 2 (`build/port`) and White 2 (`build/port-w2`), in
  `/tmp/frigate-sync-host6.log` and `/tmp/frigate-sync-host-w2.log`. These precede the final follow-ups; both versions'
  final builds and T2/T3/T6 suites remain required by exact-head local verification. The previous `f2fb61a30a`
  verifier passed both ROM SHA1s and its Black 2 build; its suite completed with 284 passes and 18 skips. It stopped
  after Black 2 to integrate reviewed main and the final two upstream commits before repeating the exact-head gate.
- **Tests:** NNS serialized-layout checks pass. The expheap query regression fails with the old address locals and
  passes with native-width locals, including ASan/UBSan. Hosted Linux function-sanitizer reports identified three
  native PXI callbacks whose tag parameter did not match `PXIFifoTag`; all three now use the exact SDK signature and
  pass focused native type checks. Hosted CI is rerun on the updated proposal. The first local Black 2 non-timing
  suite was interrupted under heavy build contention; four routes expired their watchdog while hashing the ROM,
  before game startup. Final-head complete serial suites remain pending; no gameplay pass is claimed from that
  interrupted run. Full optional T4, local T5 routes and T7 were not run.
- **Tiers:** per version, a 37,546 functions (+396), b 3,472 (-396), c zero. Code bytes: Black 2 a 3,435,848,
  White 2 a 3,435,936 (+66,714 each), and b 414,418 (-66,714 each). Excluded veneers remain 2,384; included SDK
  assembly definitions total 225 (+1): 178 inline-assembly flags and 47 functions now owned by MSL's `.s` file.
  The representation change is not native C progress. No-build evidence:
  `/tmp/frigate-sync-progress-final/{b2_us,w2_us}/functions.tsv`.
  Source coverage does not establish native gameplay readiness.
- **Trap stubs:** unchanged: 31 function traps and 22 data placeholders per version in the generated native sources.
- **64-bit warnings:** final normalized all-TU measurement pending; native compiler warnings remain visible in the
  build logs.
- **Work done:** resolved 27 upstream merge conflicts and adopted SDK function, enum, member and prototype names in
  the HAL and its regression tests. Native `u32`/`s32` remain 32-bit while MWCC keeps upstream's SDK `long` types.
  Preserved pointer-wide callbacks, address arithmetic, sound handles, native heap extents and DS-layout checks.
  Retired the obsolete RTC/misc-save long overrides, opaque SOC-option array override and duplicate trainer-card
  parameter fix because upstream now supplies the correct shared types. Added a native implementation of the newly
  recovered assembly thread-availability getter; DWC auth callbacks retain the native 32-bit ABI. The host-header
  generator excludes upstream's new MSL `time.h`, leaving native libc in use. Byte-sized `MI_CpuFill8` follows
  upstream's prototype, widens before its high-half shift and gains a high-bit byte regression. The separate
  word-valued `MI_CpuFill` retains its high-bit regression cases. The expheap query retains native address ordering
  across a 4 GiB boundary; all byte counts stay 32-bit. MSL's hand-written runtime is assembled by upstream's
  versioned assembler for ROMs and remains replaced by native libc/libm. Required-check documentation now records
  GitHub's workflow-name limitation and requires the exact-head local status's maintainer creator to be inspected.
- **Next:** complete PR 10's exact-head ROM/native gate and normalized warning measurement, then obtain the owner's
  explicit review before any main merge. The App publisher is configured and verified; automatic CI is validation,
  not owner approval. Issue 6 remains open until the sync is verified and resolved through review.
- **Blockers:** final validation and human review pending. No upstream rewrite, cross-repository commit references
  or upstream workflow commits occur in this proposal. The upstream target is frozen at `d4a30c66b`. No upstream post
  was made, and the existing field-entry heap blocker is unchanged. The conflict-marker-looking line in upstream's `docs/nonmatching-functions.md:74` is byte-identical
  to the pinned upstream file; it is preserved and excluded only from the aggregate imported diff-marker check.

## 2026-10-22: automatic sync PR CI and dedicated App

- **Upstream:** `fc478a9ee5`; no new decomp sync or game change.
- **Port:** `e6becbc07c`, automatic-PR-CI implementation based on reviewed main `a3df47254b`.
- **ROM:** exact-head T0 validation remains required before merge; results belong on the required local status.
- **Host build:** both game versions remain required by local verification; no host code changed.
- **Tests:** 14 isolated Git/shell/config fixtures pass, including App scope and absence of status/approval calls.
  Sync workflow actionlint and diff checks pass. T2/T3/T6 belong to final local verification; T4/T7 not run for this
  infrastructure change; existing public T5 coverage is unchanged.
- **Tiers / trap stubs / 64-bit warnings:** unchanged source; no remeasurement.
- **Work done:** a repository-scoped, permission-reduced, revoked App token replaces default-token sync publishing
  so PR CI can start automatically. The live fork policy is `first_time_contributors_new_to_github`. The live
  review rule disables last-push approval to avoid the sole owner's update/review deadlock, retaining stale-review
  dismissal, owner code review, required checks and local verification. App setup stays in [SYNC.md](SYNC.md); App `5435198` is installed only on Frigate, with its two write scopes
  verified and a trial token revoked. Its variable/secret are configured and default-token PR creation/approval is off.
- **Next:** the App publisher merged through reviewed [PR 9](https://github.com/CallanCampos/Frigate/pull/9)
  as `e8c3d8eefa`, with all required checks passing. Its
  [publisher trial](https://github.com/CallanCampos/Frigate/actions/runs/39888464945) succeeded, found the
  existing PR 10 and left it unchanged, then revoked its token. Review the bounded sync proposal separately.
- **Blockers:** no App-setup blocker remains. Automatic CI does not remove explicit owner approval before
  contributor/sync PR merges. The solo owner-authored review exception remains.

## 2026-10-22: protected main and reviewed sync proposals

- **Upstream:** `fc478a9ee5`; this change imports no new decomp code.
- **Port:** `88237af41d`, the contribution/sync hardening PR based on `9453d79234`.
- **ROM:** T0 validation is required before this PR merges; the exact result is recorded by its required
  `Frigate local verification` status. No ROM/game source changed in this PR.
- **Host build:** both versions are required by the new exact-head verifier; the public CI gate also covers the
  existing nine native/distribution/sanitizer jobs. Results are recorded on this PR before merge.
- **Tests:** 11 isolated Git/shell sync fixtures pass, including preservation of port changes, conflicts, reference
  and rewrite stops, frozen proposals, failed verification and changed remote heads. Shell syntax and both workflows'
  actionlint/ShellCheck checks pass (actionlint's outdated Ubuntu 26.04 label inventory excluded). T2/T3/T6 run in
  local verification; public T5 coverage remains. Optional T4 and T7 were not run for infrastructure changes.
- **Tiers / trap stubs / 64-bit warnings:** unchanged game and native sources; no progress remeasurement.
- **Work done:** [ADR 0018](adr/0018-protected-main-and-sync-prs.md), two live main rulesets, PR-only sync publishers,
  explicit CI and ROM verification gates, CODEOWNERS and current contributor/maintainer procedures. The owner is the
  sole collaborator; the only bypass is owner PR review, never required validation or history protection.
- **Next:** merge the validated infrastructure PR and re-enable the replacement scheduled sync; future upstream
  proposals wait for owner review. Gameplay work resumes at the field-entry blocker below.
- **Blockers:** account-wide app/token grants could not be enumerated with the available maintainer OAuth credential.
  No gameplay blocker was changed by this work.

## 2026-10-21: contribution-policy source traceability

- **Upstream:** `fc478a9ee5`; no new sync.
- **Port:** this documentation follow-up, based on `6c214613e4`, whose tree matches native head `551cd9b1ea`.
- **ROM:** T0 passes, both SHA1s match (`/tmp/frigate-docfollowup-t0.log`).
- **Host build:** both macOS builds pass (`/tmp/frigate-docfollowup-{b2,w2}-build.log`).
- **Tests:** `hal_wstring` and `os_wcm_workspace` pass, 2/2 in each version
  (`/tmp/frigate-docfollowup-{b2,w2}-tests.log`). Native source is unchanged from `551cd9b1ea`, whose Compile, Standards
  and all nine target CI jobs pass ([manual run, attempt 2](https://github.com/CallanCampos/Frigate/actions/runs/39632971815)).
  Prior local T2/T3/T6 full suites pass (Black 2 281 pass/16 skip; White 2 279 pass/16 skip). Full optional T4 and
  local T5 routes were not rerun; CI's ROM-independent sanitized checks pass. T7 was not run.
- **Tiers:** unchanged: a 37,150 functions, b 3,868, c zero per version; Black 2 code bytes a 3,369,134/b 481,132,
  White 2 a 3,369,222/b 481,132, excluding 2,384 veneers and including 224 SDK assembly definitions.
- **Trap stubs:** unchanged, 31 functions and 22 data placeholders per version.
- **64-bit warnings:** unchanged native source; four pointer/int cast families 469 (game 234, libraries 235),
  optional integer shortening 388; no remeasurement for Markdown edits.
- **Work done:** corrected the review's policy wording and added primary-source observations to
  [ADR 0017](adr/0017-external-contributions.md), distinguishing other projects' branches from Frigate's `main` choice.
- **Next:** size the external field heap, then repeat field-entry and real menu checks.
- **Blockers:** the fixed 64 KiB field heap still terminates field entry at frame 10,759; this records no gameplay fix.

## 2026-10-21: latest upstream sync and renewed validation

- **Upstream:** `fc478a9ee5`, merged cleanly by `port/tools/sync_upstream.sh` in `c46687c006`.
- **Port:** `b418e7d15d`.
- **ROM:** T0 passes after the sync, both SHA1s match (`/tmp/frigate-latest-t0.log`).
- **Host build:** T1 passes on macOS for Black 2 (`build/port`) and White 2 (`build/port-w2`), with
  `/tmp/frigate-latest-build-{b2,w2}.log` recording the builds.
- **Tests:** T2/T3/T6 non-timing suites pass: Black 2, 281 pass and 16 skip of 297 (173.83 s); White 2,
  279 pass and 16 skip of 295 (166.14 s). Logs: `/tmp/frigate-latest-tests-{b2,w2}.log`. Each skips 15 optional
  oracle checks and `app_universal_slices`; full optional T4 coverage was skipped. T5 and T7 were not run this pass.
  The earlier 10,600-frame New Game replay and frame-10,759 field trace remain the gameplay evidence below.
- **Tiers:** unchanged from the preceding entry: per version, a 37,150 functions, b 3,868 and c zero. Code bytes:
  Black 2 a 3,369,134 and b 481,132; White 2 a 3,369,222 and b 481,132. Excluded veneers remain 2,384 and included
  SDK assembly definitions 224. Evidence: `/tmp/frigate-progress-latest-20261021/{b2_us,w2_us}/functions.tsv`.
  These source-coverage counts do not establish native readiness.
- **Trap stubs:** generated sources still contain 31 function traps and 22 data placeholders per version.
  The observed field-entry stop is `OS_Terminate` in `FieldmapProc_Init`, rather than a missing-field-C trap.
- **64-bit warnings:** the normalized all-TU native syntax probe passes all 1,538 baseline and 1,655 current units.
  The four pointer/int cast families fall from 492 to 469 instances: game 245 to 234, libraries 247 to 235.
  Separate optional `-Wshorten-64-to-32` diagnostics rise from 277 to 388 after the native string declarations; they are outside that four-family metric.
  Evidence: `/tmp/frigate-winfix-warning-full/{summary,results}.json`; the preceding entry explains the baseline
  and normalization.
- **Work done:** renewed the contribution and portability work's proof against the latest upstream. Targeted MWCC
  token streams are identical for 20/24 file/version pairs (`/tmp/frigate-latest-neutral.log`). The four exceptions
  are `friend.c` and `dwcutil_ac.c` in both versions. Exact object proof covers all ten DWC file/version pairs,
  754 function instances, all allocated sections and exact relocation targets
  (`/tmp/frigate-dwc-proof-fc478/{results,relocation-proof}.json`); DWC utility uses its configured `2.0/sp2` compiler.
  The portability rows in [game-changes.md](game-changes.md) identify their introduction commit `90ad973b12`.
  [The review](REVIEW-2026-10-21.md) now describes this sync and its validation limits. The publication check also
  replaced two stale function names in port-authored particle-work comments with their current names; executable C
  is unchanged. Windows CI also exposed a libc declaration collision for `strnicmp`: MSL comparison extensions
  now use private host names after including the host string header, preserving their signed bounds. GameStats, ND and SSL
  use the native string declarations on hosts while MWCC retains its original explicit declarations
  ([game changes](game-changes.md)); all 16 including file/version pairs preprocess identically to upstream
  (`/tmp/frigate-winfix-all-neutral.log`). MinGW intrinsic declarations are also read before the unsupported-wide-API
  poison, so later SDL headers can declare their intrinsics without exposing those APIs to game calls.
- **Next:** size the external field heap for host allocations, then repeat extended entry, CONTINUE, real menus
  and differential checks before claiming a playable field; follow up numeric-width and deferred hardware paths.
- **Blockers:** field C is present, but the fixed 64 KiB external field heap cannot hold weather and place-name
  child allocations doubled to a combined `0x1b000` before other overhead. The earlier trace terminates at frame
  10,759 in `FieldmapProc_Init`. Current full oracle, sanitizer and scripted visual coverage remains unverified.

## 2026-10-21: upstream sync, field entry diagnosis and contributor workflow

- **Upstream:** `83515e20ce`, merged on `codex/port-sync-contributions`; 13 conflicts resolved with the port's
  portability changes reapplied.
- **Port:** `90ad973b12`.
- **ROM:** T0 passes, both SHA1s match (`/tmp/frigate-final-t0.log`).
- **Host build:** T1 passes for Black 2 in `build/port` and White 2 in `build/port-w2` on macOS.
- **Tests:** T2/T3/T6: fresh non-timing suites pass in both versions: Black 2, 281 pass and 16 skip of 297
  (178.73 s); White 2, 279 pass and 16 skip of 295 (167.08 s). The registered assembly oracle uses the actual
  `SVC_UnpackBits` symbol and each build's version. The WCM workspace/configuration test covers native alignment,
  undersized and negative lengths, complete addresses and buffer canaries; restoring the DS-sized minimum fails the
  regression. Both suites skip 15 optional oracle tests without their binary and `app_universal_slices`. T4 full
  optional comparisons skipped; T5 and T7 not run this pass. The 10,600-frame New Game replay passes; an extended trace
  stops at frame 10,759 during field initialization. Logs: `/tmp/frigate-final-tests-{b2,w2}.log` and
  `/tmp/frigate-wcm-{red,green}-test.log`.
- **Tiers:** config/C scanner: 37,150 functions in complete files (tier a), 3,868 in partial files (b), zero without
  a detected C/assembly definition (c), per version. Code bytes: Black 2, a 3,369,134 and b 481,132; White 2,
  a 3,369,222 and b 481,132. Against `a14364b6ba`, a grows by 3,449 functions, b falls by 1,055 and c by 2,394.
  Counts exclude 2,384 linker veneers and include 224 SDK assembly definitions; they do not prove native readiness or
  partial-C semantics. Evidence: `/tmp/frigate-progress-20261021/` and its baseline counterpart.
- **Trap stubs:** 31 function traps and 22 data placeholders per version, down from 457 and 46 in the previous record.
  The field table has C; the observed field-entry stop is `OS_Terminate`, not its old placeholder.
- **64-bit warnings:** the four pointer/int cast families, explicitly enabled in configured native commands, fall
  from 245 to 234 game instances and from 247 to 235 library instances against `a14364b6ba` (492 to 469 total).
  All 1,538 baseline and 1,655 current translation units pass syntax checks with their own generated headers,
  `PLATFORM_PC`, Apple clang 21 and normalized `-O2 -DNDEBUG`. This establishes the comparable pointer-warning gate;
  it is not a zero-warning claim. The separate `-Wshorten-64-to-32` probe rises from 277 to 384 instances and remains
  numeric-width follow-up work. Earlier ordinary-build and non-native shim counts use different metrics.
  Evidence: `/tmp/frigate-native-warning-full/{summary,results}.json`.
- **Work done:** imported the field engine's C and removed obsolete `ov011_pending.c` (all four SDK definitions now
  have real C). Preserved SocConfig's hostname pointer with `uptr` ([game changes](game-changes.md)): targeted MWCC
  token streams 6/6 identical. The initial merge's global MWCC comparison has 3,550/3,554 identical pairs; its four
  exceptions are `event_field_proclink.c` (TrainerCardParam) and `vm.c` in both versions. Both files are complete in
  both delinks, so T0 proves those exceptions. ROM hashes do not prove the behavior of other incomplete C linked only
  on the host. Later SDK changes have 14/14 targeted identical token streams plus ten exact DWC object pairs
  (754 function instances, allocated data and relocation identities). They preserve native addresses, match callback
  contexts and the public callback ABI. WCM and both wrappers share a native DS workspace extent; signed lengths and
  AP-list alignment are checked before writing. Added external fork/topic-PR guidance under
  [ADR 0017](adr/0017-external-contributions.md), preserving
  earlier uncommitted contributor documentation. [Review](REVIEW-2026-10-21.md) records the architecture and remaining
  gaps.
- **Next:** size the external field heap for host allocations, then repeat extended field entry, current field/app
  replays and oracle comparisons. Investigate remaining numeric-width and deferred hardware boundaries.
- **Blockers:** `g_FieldHeapMemory` is an external 64 KiB buffer; host doubling of weather's `0x6400` and place-name's
  `0x7400` requires `0x1b000` combined before other overhead. The trace terminates in `FieldmapProc_Init` after
  `HEAPID_FIELD_PLACE_NAME` (`/tmp/frigate-host-field-entry.log`). This is a host allocation blocker, not missing
  field C. Sanitized and scripted visual coverage remains unverified this pass.

## 2026-10-19: the performance and hygiene merge, reviewed: ADR 0016's acceptance as it was, a timing failure's way out

- **Upstream:** `2825ea75f` (merge of PR #393), as merged into the port's `main` by `607ccd1a6`; no sync in this task.
- **Port:** `658f00e7e3`, on the branch `port-integrate` after the merge's last entry (`e653f3deb`); pushed to
  `port-perf` on origin for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, with nothing to
  rebuild). No file outside `port/` changed.
- **Host build:** the last entry's machine and builds. `build/port-n`, `build/port-w2` and `build/port-san` had
  nothing to compile, since only a Python tool, its test and documents changed, so each build's warnings stay 264, the
  decomp's. CI: run 39267615088 at `e653f3deb`, which the last entry dispatched, is green on all nine jobs with run
  39257475500's counts (pass and skip: 249 and 49 of 298 on macOS's host job and each of the four distribution jobs,
  231 and 50 of 281 on Linux, 221 and 60 of 281 on Windows, the sanitized builds 247 and 25 of 272 on macOS and 229
  and 26 of 255 on Linux, each then 3 and 8 of 11 routes). Dispatched again at this entry's commit; the next entry
  gives its result.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4 -LE timing` with the oracle and the cached saves, the two builds at once:
    Black 2, 294 pass and 1 skip of 295 (581 s); White 2, 292 pass and 1 skip of 293 (563 s). Each version's 15
    `oracle` tests pass, and `frame_times_tool` with its new checks; the skip is `app_universal_slices`, which a
    development build's one slice skips.
  - The timing routes alone, each build after the other, pass within the 8.36 ms budget (median and 95th percentile
    in ms): Black 2's 3, the title 2.83, 2.94, the start menu 0.67, 2.69, the party apps 0.74, 1.06, New Game's intro
    0.72, 1.04 and flight 2.68, 3.19, at a load average falling from 21 to 10; White 2's 2, the title 2.67, 2.79, the
    start menu 0.64, 2.38, the party apps 0.76, 1.16, at 10 to 7. In all, Black 2 297 pass and 1 skip of 298, White 2
    294 pass and 1 skip of 295.
  - `frame_times_tool`, which checks the new line, passes in `build/port-san` too. T5's routes and the distribution
    build didn't run again: nothing they build or run changed.
  - Linux on arm64, which no CI job builds: `port/gfx/workers.c`, which needs only clang's own headers, builds for
    `aarch64-unknown-linux-gnu` and `aarch64-unknown-linux-musl` (`-ffreestanding -std=c11 -Wall -Wextra -Werror`),
    its spin's `__builtin_arm_yield` a `yield` each time (3).
  - Every run passed `--save-dir`, with `HOME` a scratch directory, and nothing under `~/Library`'s
    `Application Support/Frigate` or `Logs/Frigate` is newer than 16:33 on 2026-10-18. T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** as the last entries: 457 function stubs and 46 data placeholders in each version.
- **64-bit warnings:** as the last entries.
- **Work done:** the review of the merge found no defect: the conflicts kept every side, the output is the same on
  `main`'s newest routes and with 1 and 8 render threads, ThreadSanitizer finds no race in the bands, the crash reports
  pass on every POSIX job, and the SHA-1 fallback hashes the known vectors on x86_64 under Rosetta and on Windows. Its
  three notes:
  - **ADR 0016's acceptance, as it happened.** Its note said the port's developer accepted it; the session that merged
    the track did, on the developer's standing instruction to carry the plan forward, and the developer's messages in
    that session never mention an ADR. The note now says so: the session accepted it, not the developer in person; the
    last entry's "by the port's developer" stays as written. ADRs 0013 to 0015, on `main`, have the same words, which
    their accepted text keeps until the developer says otherwise ([adr/README.md](adr/README.md)).
  - **A timing failure says what a slower machine does.** The quick start's `ctest -j4` runs the `frame_times_*` tests
    in an optimized build, and a quiet machine too slow for the 8.36 ms budget fails them rather than skipping them,
    by design. `frame_times.py check` now ends such a failure with the way out, `ctest -LE timing` or
    `-DPORT_FRAME_TIME_REPORT_ONLY=ON`, which `test_frame_times.py` checks, and port/README.md's tests line and the
    tests guide's timing paragraph say it. The x86_64 slice's 8.00 ms under Rosetta, the closest to the budget, was
    measured at a load of 16 to 26, where an over-budget scene skips; a quiet measurement of that slice is still to
    come (Next).
  - **Linux on arm64 is built by no job**, so `host_posix.c`'s branch that reads the program counter
    (`uc_mcontext.pc`) has only been read. It matches glibc's `mcontext_t` for aarch64, whose `fault_address`, `regs`,
    `sp`, `pc` and `pstate` have those names under `__USE_MISC`, which the `_GNU_SOURCE` of
    [port/CMakeLists.txt](../CMakeLists.txt) gives, as Linux x86-64's `REG_RIP` already needs; building it needs Linux's
    headers, which this machine doesn't have. The workers' spin builds for the host (Tests). A job on GitHub's arm64
    Linux runners would build the whole port there for the first time, a change to CI's jobs left for the developer
    (Next).
  - The last entry's `PORT_SHA` is `e653f3deb`. lessons.md has the review's two lessons: an ADR's note says who
    accepted it, and a file with only the compiler's own headers builds for a host no job builds.
- **Next:** as the last entry: the frames on a quiet machine, both slices, the x86_64 one under Rosetta measured since
  the bands; for T5, a nightly run of both versions' sanitized routes and of the x86_64 slice's; Linux on arm64 in CI,
  if the developer wants that host; then the apps no route reaches yet, and a save with an adventure made by the port,
  once overlay 36 lets it into the field.
- **Blockers:** as the last entries: overlay 36 (issue #83, another contributor's active claim) gates phase 3; macOS 11
  to 14 have no runner and no virtual machine here.

## 2026-10-19: the performance and hygiene tracks merged: frames with headroom (ADR 0016) and every crash reported

- **Upstream:** `2825ea75f` (merge of PR #393), as merged into the port's `main` by `607ccd1a6`; no sync in this task.
- **Port:** `e653f3deb`, on the branch `port-integrate` from `origin/main` at `607ccd1a6`: `8f28c3387` (the merge of
  `port-perf`), `ac06d887e` (the merge of `port-hygiene`), `6f50f90b4` (ADR 0016 accepted), then this entry; pushed to
  `port-perf` on origin for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, with nothing to
  rebuild) after both merges. No file outside `port/` changed.
- **Host build:** the last entries' machine (macOS 26.5.1 on an M2 MacBook Air, Xcode 26.6, Apple clang 21.0.0),
  RelWithDebInfo, Ninja, in the worktree `build/verify-tree`:
  - `build/port-n` (Black 2), `build/port-w2` (White 2) and `build/port-san` (Black 2, `-DPORT_SANITIZE=ON`) built
    again, with `build/oracle`'s `pbw-oracle` and the cached saves: only `port/`'s files compiled, with no warning, so
    each build's count stays 264, the decomp's.
  - `build/port-dist`, the universal distribution build, by `port/tools/package_macos.sh --distribution`, then every
    target of it (`cmake --build`): the game compiled again for both slices since its last build, 528 warnings, 264 a
    slice, the decomp's; none in `port/`. The oracle and Black 2's saves copied in for its x86_64 run.
  - CI: `port-ci.yml`'s run 39257475500 at `6f50f90b4` on `port-perf`, green on all nine jobs, the first build of
    either track on Linux and Windows (Tests, below). The runs the last entries left open were green too: 39215654518 at
    `fe88f07f2` on `port-sanitizers`, and 39218824080 at `607ccd1a6` on `main`. Dispatched again on `port-perf` at
    this entry's commit, which changes only documents; the next entry gives its result.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4 -LE timing` with the oracle and the cached saves, the two builds at once: Black
    2, 294 pass and 1 skip of 295 (567 s); White 2, 292 pass and 1 skip of 293 (552 s). Each version's 15 `oracle`
    tests pass, White 2's starter-save routes among them. Then the timing routes alone, each build after the other:
    Black 2's 3 and White 2's 2 pass within the 8.36 ms budget. In all, Black 2 297 pass and 1 skip of 298, White 2
    294 pass and 1 skip of 295; the skip is `app_universal_slices`, which a development build's one slice skips.
  - The frames after the merge, from those timing routes (headless, each frame the least of 3 runs, the frame without
    the pacing wait, median and 95th percentile in ms): Black 2's title 2.66, 2.85, the start menu 0.63, 2.65, the
    party apps 0.74, 1.03, New Game's intro 0.70, 1.01 and flight 2.59, 3.07; White 2's title 2.67, 3.09, start menu
    0.62, 2.68, party apps 0.74, 1.04. Other sessions' work on this machine kept the load average between 10 and 26 for
    the whole task, so these are loaded frames, about 0.6 ms over `port-perf`'s quiet ones (the title 1.97, 2.04), all
    of it in the 3D frame's bands (1.58 ms where the quiet frame took 0.98), which share the cores. The merge changed
    nothing in them: `port-perf`'s own Black 2 build (`build/wt-perf`) and this one, alternately, 3 runs each with
    `frame_times.py check`, gave the title 3.67, 3.87 and 3.68, 3.88 at a load of 13 to 16, then 3.33, 3.62 and 2.83,
    3.26 at 10 to 12.
  - T5, `build/port-san`, every test, `ctest -j4`, with the oracle and Black 2's saves: 295 pass and 1 skip (the same)
    of 296 (488 s), its 25 `route` and 15 `oracle` tests among them, the 3D renderer's workers drawing their bands
    under ASan and UBSan in every route with 3D; `g3d_ds3d_bands` and its sanitized twin pass. No output has a report
    of the runtime's.
  - The distribution build: the image (`Frigate Black 2 0.0.2917.dmg`, 13 MB) passes `package_macos.sh`'s checks, and
    the app passes `check_app.sh --archs "arm64 x86_64" --minimum 11.0 --run-every-slice`. Its arm64 slice holds the
    ARMv8 SHA instructions (72 in `otool`'s listing), the x86_64 one none, nor x86's own SHA extensions: its SHA-1 is
    the C rounds, which `sha1.c` compiles wherever the compiler doesn't target the ARMv8 ones. The `app_*` tests, run
    natively, pass (17, `app_slices` and `app_universal_slices` among them). The x86_64 slice under Rosetta
    (`ctest_one_slice.sh --arch x86_64`), every test but the timing ones, `-j4`: 284 pass and `app_universal_slices`
    skips of 285 (1,227 s at a load of about 22), `rom_sha1`, the 25 routes, the 15 `oracle` tests, the new host
    crash tests and `g3d_ds3d_bands` among them; then its timing routes alone pass, at a load of 16 to 26: the title
    5.72, 8.00, the start menu 1.45, 5.82, the party apps 1.71, 4.17, New Game's intro 1.76, 3.24 and flight 5.63,
    7.30, 1.6 to 2.2 times the arm64 slice's under a like load, and not yet a measurement on a quiet machine.
  - CI, run 39257475500, per job (pass and skip of the tests run; the skips need the ROM, or are another host's):

    | Job | Tests |
    | --- | --- |
    | Host build (macos-26) | 249 and 49 of 298 (147 s); the app built |
    | Host build (ubuntu-26.04) | 231 and 50 of 281 (50 s) |
    | Host build (windows-2025) | 221 and 60 of 281 (37 s) |
    | macOS distribution build (macos-26, macos-26-intel, macos-15, macos-15-intel) | 249 and 49 of 298 each (152, 199, 99 and 144 s); the app and its image built and checked, every slice started on Apple Silicon |
    | Sanitized build (macos-26) | 247 and 25 of 272 (169 s), then the routes without the oracle, 3 and 8 of 11 |
    | Sanitized build (ubuntu-26.04) | 229 and 26 of 255 (60 s), then 3 and 8 of 11 |

    Windows compiled `host_windows.c`'s clock (its frequency through `InitOnceExecuteOnce`), threads and signals and
    `EXCEPTION_BREAKPOINT` for the first time, and runs them: `g3d_ds3d_bands` and its sanitized twin, `g3d_hal`,
    `frame_times_tool`, and the hygiene track's seven crash tests (`host_divide_by_zero`, `host_illegal_instruction`,
    `host_breakpoint`, each on a fiber too, and `host_trap`, six new and `host_illegal_instruction_fiber` changed)
    pass there, as on Linux and macOS. `rom_sha1` passes on Linux, Windows and the Intel runners, where the C rounds
    run. The timing routes skip (77) on every job, which has no ROM: on CI they can't fail, however slow the runner,
    and nothing needed fixing.
  - Every run passed `--save-dir`, with `HOME` a scratch directory, and nothing under `~/Library`'s
    `Application Support/Frigate` or `Logs/Frigate` is newer than 16:33 on 2026-10-18. T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** as the last entries: 457 function stubs and 46 data placeholders in each version.
- **64-bit warnings:** as the last entries.
- **Work done:** the integration of the two tracks that branched from `main` at `607ccd1a6`.
  - **The merge of `port-perf`** (`8f28c3387`), 15 commits: `--frame-times` and `frame_times.py` with the timing
    routes in optimized builds, the still 3D picture kept, the 3D frame drawn in bands on worker threads, the game's
    thread at user-interactive QoS, the geometry engine's entries fed straight to its executor, capture a line at a
    time, the 2D sources kept and their colour effects only where a line can blend, SHA-1 through the ARMv8 SHA
    instructions, and the Windows clock read once. It merged clean.
  - **The merge of `port-hygiene`** (`ac06d887e`), 2 commits: SIGFPE, SIGILL and SIGTRAP reported on macOS and Linux
    with the crashing function named, `EXCEPTION_BREAKPOINT` fatal on Windows, and `package_macos.sh` refusing a
    sanitized build by any CMake flag. Both tracks had changed the host's seam and the documents; every conflict kept
    both sides: `host_posix.c`'s header names the crash reports and the clock and threads, `hal/README.md`'s `host/`
    row and its `host_posix.c` and `host_windows.c` rows name both, and STATUS.md and lessons.md have `port-perf`'s
    entries, the newer, above `port-hygiene`'s. `host.h`, `host_windows.c`, ARCHITECTURE.md, MODDING.md and the other
    guides merged clean, and the workers' threads already leave SIGFPE, SIGILL and SIGTRAP unblocked, so a crash on
    one reaches the same report.
  - **ADR 0016 accepted** (`6f50f90b4`) by the port's developer at the track's merge, as every earlier ADR: its status
    line, a note naming the merge, and the index's row.
  - **The merged entries' `PORT_SHA`:** `port-perf`'s last entry is `db64d5d76` and `port-hygiene`'s `0308587c6`, the
    commits that wrote them.
  - lessons.md has the merge's three lessons: a distribution build's tests after every target is built, not after
    `package_macos.sh` alone (a first x86_64 run, on the test programs of the tree's last full build, failed 11);
    no build of a tree that `ctest_one_slice.sh` has thinned; and a shared machine's load, which shows in the 3D
    frame first.
- **Next:** the frames on a quiet machine, both slices, the x86_64 one under Rosetta measured since the bands; for T5,
  a nightly run of both versions' sanitized routes and of the x86_64 slice's; then, as the last entries, the apps no
  route reaches yet, and a save with an adventure made by the port, once overlay 36 lets it into the field.
- **Blockers:** as the last entries: overlay 36 (issue #83, another contributor's active claim) gates phase 3; macOS 11
  to 14 have no runner and no virtual machine here.

## 2026-10-19: frames with headroom after review: the timing tests only where they can hold, ADR 0016 as the code shares

- **Upstream:** `2825ea75f`, as merged into the port's `main` by `607ccd1a6`; no sync in this task.
- **Port:** `db64d5d76`, on the branch `port-perf` after `b9eb5d4f1`: `11506d41d` (the timing tests only in an optimized
  build, and skipped over their budget under load), then this entry with ADR 0016's corrections and the Windows clock;
  not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at both commits. No
  file outside `port/` changed.
- **Host build:** the last entry's machine and builds, `build/port`, `build/port-w2` and `build/port-san` built again
  (78 of the port's files, which include `hal/host/host.h`; no game file); and `build/port-debug` (Debug) and
  `build/port-notype` (no build type), configured to check which tests they have, the Debug one built and timed, both
  deleted after. Warnings: none in `port/`; the game's 264 unchanged, no game file having been compiled again. Not
  pushed, so no CI run: `host_windows.c`'s clock, changed here, is still compiled by no one.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with the oracle and its saves, both builds at once: Black 2, 291 pass and 1 skip
    of 292 (531 s); White 2, 288 pass and 1 skip of 289 (496 s). The skip is `app_universal_slices`, which a
    development build's one slice skips. The final state's frames, quiet, each route 3 times alone (`frame_times.py
    check`), each frame's least, median and 95th percentile in ms, against `8cb006454` (the review's): Black 2's title
    1.97, 2.03 (4.23, 4.43), the start menu 0.56, 1.86 (2.53, 3.96), New Game's intro 0.63, 0.82 (0.82, 1.22) and
    flight 1.80, 2.06 (3.76, 4.67), the party apps 0.72, 0.95 (0.83, 3.29); White 2's title 1.93, 2.00, start menu
    0.56, 1.74, party apps 0.71, 0.95. As the last entry, to 0.03 ms.
  - The timing tests in each build: Black 2's three and White 2's two in `RelWithDebInfo`; none in the Debug build, in
    the build with no type or in the sanitized one, each of which keeps `frame_times_tool` (289, 289 and 290 tests).
  - Under load: 8 White 2 routes (the opening and the title, `--run-dir`, 2 to 8 render threads each) looping beside
    Black 2's `frame_times_title`, started 5 s before it: it passed at 7.39 ms (single runs 11.05 to 13.39), the load
    average 3.86 before the first run and 12.26 after the last; `check` with a 3 ms budget then skipped (77) at a load
    of 15.54. Quiet (a load of 5), a 1 ms budget failed.
  - T5: the sanitized Black 2 build's `frame_times_tool`, `host_*` and `g3d_*` pass (21 tests); the rest of its run is
    the last entry's, since nothing else it runs changed but comments. ThreadSanitizer, by hand (`-fsanitize=thread`,
    `-O1`): `g3d_ds3d` (1,556 checks) and `g3d_ds3d_bands` (76) pass with no report. T7 not yet.
  - Every run passed `--save-dir` with `HOME` a scratch directory; nothing under `~/Library`'s
    `Application Support/Frigate` or `Logs/Frigate` is newer than 16:31 on 2026-10-18.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** as the last entry: 457 function stubs and 46 data placeholders in each version.
- **64-bit warnings:** as the last entry.
- **Work done:** the review of `port-perf` (`8cb006454` to `b9eb5d4f1`) found the output unchanged on every route of
  both versions, at 1 to 8 render threads, the bands exact on 60 fuzzed scenes and under ThreadSanitizer, SHA-1 right
  on both paths and the gains as measured, and five problems:
  - **The timing tests failed in an unoptimized build.** `frame_times.cmake` registered them in every build but the
    sanitized one, and `port/CMakeLists.txt` sets no default build type, so a Debug build, or one configured with none
    (as `sync_upstream.sh` makes a new `build/port`, then runs every test), held -O0 frames to the budget. Measured here
    in Debug, median and 95th percentile: the title 9.97 and 10.44 ms, the start menu 4.35 and 9.62, over the 8.36 ms
    budget (the review's 95th percentiles: 10.48 and 9.71). Only `Release`, `RelWithDebInfo` and `MinSizeRel` have them
    now.
  - **The timing tests failed under heavy load.** Each frame's least of 3 runs leaves out load that comes and goes, not
    load that stays: with 8 other Frigate routes running, all at the same user-interactive QoS since `8aef262f2`, the
    review's title came to 8.45 ms. `frame_times.py check` now reads the 1-minute load average before each run and after
    the last, and skips (77) a scene over the budget when it was above `--max-load`, the cores by default; a pass stands
    at any load, and Windows, without a load average, never skips. It reads the load during the runs, not only at the
    start as the review proposed: the average lags by about a minute, and under the 8 routes above it read 3.86 when the
    test began. `frame_times_tool` covers the skip with the load made up. lessons.md's lesson, which took least-of-3 to
    leave out any load, now says which load it removes, and a second lesson says a timing budget holds only in its
    build.
  - **ADR 0016 said the parts share no memory** but the job's inputs and the signals: they share `ds3d.c`'s frame
    buffers and the output picture, by lines, and edge marking reads other bands' `sDepth` and `sAttr` after the sync.
    Its Context also had the stencil cleared per pixel, where the rear plane and the shadow masks clear the whole
    screen, which each band now does on its lines. Both are corrected, with ThreadSanitizer's clean runs (above and the
    review's).
  - **The last entry's `PORT_SHA`** (the sanitized build's merge after review) is `fe88f07f2`, as the rule gives; the
    entry before this one's is `b9eb5d4f1`.
  - **The Windows clock's frequency** was set on first use with no lock, and the renderer's workers read the clock
    too: it is now read once through `InitOnceExecuteOnce`, as the file's symbols are. `host.h` says any thread may
    read the clock. Not compiled here (no Windows toolchain); CI's Windows job will.
  - Also: `frame_times.py`'s docstring named `port/cmake/app.cmake` for the timing tests, which are
    `port/cmake/frame_times.cmake`'s.
- **Next:** as the last entry: the user's review of ADR 0016; CI's first run of the branch (Linux and Windows build the
  new host threads and clock); the x86_64 slice's frame times under Rosetta.
- **Blockers:** as the last entry.

## 2026-10-19: frames with headroom: 3D in bands on worker threads, still 3D kept, frame times measured and held

- **Upstream:** `2825ea75f`, as merged into the port's `main` by `607ccd1a6`; no sync in this task.
- **Port:** `b9eb5d4f1`, on the branch `port-perf` from `origin/main` at `607ccd1a6`: `8cb006454` (`--frame-times`,
  `frame_times.py` and the timing routes), `b9b46a0a9` (the still 3D picture kept), `f6f924be2` (the 3D frame in bands
  on worker threads, ADR 0016), `8aef262f2` (the game's thread at user-interactive QoS), `889fcc3de` (the geometry
  engine's entries fed straight to the executor), `f14b27045` (capture a line at a time), `a41ad3171` (the 2D sources
  kept until the VRAM layout changes), `795455dd7` (the 2D colour effects only where a line can blend), `bf43d595b`
  (SHA-1 through the ARMv8 SHA instructions), `8772bcc84` (this entry), `e0f4ce2d0` (the host layer's headers name
  the clock's and the threads' users), `4baae6d5b` (each worker waits on a signal of its own), then this entry's update
  with the tests of the last code; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at every commit. No
  file outside `port/` changed.
- **Host build:** macOS 26.5.1 on an M2 MacBook Air (4 performance and 4 efficiency cores, 16 GB), Xcode 26.6 (Apple
  clang 21.0.0), RelWithDebInfo, Ninja, in the worktree `build/wt-perf`: `build/port` (Black 2) and `build/port-w2`
  (White 2), with `build/verify-tree`'s `pbw-oracle` and cached saves copied in; `build/port-san` (Black 2,
  `-DPORT_SANITIZE=ON`). Warnings: 264 in each, the decomp's, none in `port/`. Not pushed, so no CI run: Linux and
  Windows weren't built here, and the Windows halves of the host's new clock, threads and signals (`host_windows.c`)
  have been compiled by no one yet.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with the oracle and its saves, both builds at once, at `4baae6d5b`: Black 2, 291
    pass and 1 skip of 292 (521 s); White 2, 288 pass and 1 skip of 289 (489 s); the same at `bf43d595b`. The skip is
    `app_universal_slices`, which a development build's one slice skips. The new tests pass: `frame_times_tool`,
    `g3d_ds3d_bands` and its sanitized twin, and the timing routes `frame_times_title`, `frame_times_party_menus` (both
    versions) and `frame_times_new_game` (Black 2) within the default budget, 8.36 ms.
  - Output unchanged, commit by commit: `frames.tsv` of the opening, title, CONTINUE, New Game and both launcher routes
    (Black 2, `--run-dir`, every frame's VBlank count, GFL count, game frame and both screens' SHA-1s) is byte for byte
    `607ccd1a6`'s after each commit, with `--frame-times` on too, and for the title, New Game and the party apps at
    `--render-threads` 1, 2 and 8 (the title and New Game at 8 again at `4baae6d5b`). `g3d_ds3d` runs every
    hand-worked 3D scene in one band and in three, and `g3d_ds3d_bands` draws random 2,000-triangle scenes in 2 to 8
    bands against one, pixel, depth, ID and fog flag.
  - T5, the sanitized Black 2 build (`build/port-san`, made from nothing at `bf43d595b`), with the oracle and its saves:
    at `4baae6d5b`, 289 pass and `app_universal_slices` skips of 290 (406 s), its 24 routes among them, the game's
    thread and the 3D renderer's workers under ASan and UBSan; at `bf43d595b` the same. No output has a sanitizer's
    report. White 2's sanitized build wasn't run. T7 not yet.
  - Every run passed `--save-dir` with `HOME` a scratch directory; `~/Library`'s `Application Support/Frigate` and
    `Logs/Frigate` hold nothing newer than 16:33 on 2026-10-18.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** 457 function stubs and 46 data placeholders in each version, as the last entry.
- **64-bit warnings:** as the last entry.
- **Work done:** making Frigate's frames with headroom, from the measurement of 2026-10-19 (every route under the
  16.7 ms budget, the worst scene's 95th percentile 4.7 ms headless, 3D frames about 6 ms paced; the CPU 3D renderer
  about three quarters of a 3D frame), in its order of gain, each change in a commit of its own with its measured gain
  and its proof of unchanged output.
  - **The measurement, repeatable** (`8cb006454`): `--frame-times FILE` (`port/hal/os/frame_time.c`) writes each
    frame's host time with and without the pacing wait and its parts (the 3D frame, the geometry engine, the 2D lines,
    capture, the ARM7, the window, the pacing, the rest), timed where their work is, and prints the median, 95th
    percentile and most; `port/tools/frame_times.py` reads runs of a route, each frame the least of the runs, by scene,
    and `check` holds them to a budget. The timing routes run 3 times each, alone, labelled `timing` (`ctest -LE
    timing` leaves them out; `PORT_FRAME_TIME_REPORT_ONLY` makes them report only); CI, without a ROM, skips them.
  - **The gains**, Black 2 on the M2, headless, each frame the least of 3 runs, the frame without the pacing wait,
    median and 95th percentile in ms, before and after:

    | Scene | Before | After |
    | --- | --- | --- |
    | Boot, logos, opening movies (2D) | 0.53, 0.71 | 0.41, 0.59 |
    | Title screen's 3D | 4.24, 4.45 | 1.97, 2.04 |
    | Start menu | 2.53, 3.95 | 0.56, 1.86 |
    | New Game's intro (Juniper's sprites) | 0.82, 1.23 | 0.63, 0.82 |
    | New Game's 3D flight | 3.78, 4.68 | 1.80, 2.08 |
    | CONTINUE (title, menu, CONTINUE) | 4.18, 4.41 | 1.94, 2.03 |
    | The bag and the trainer card | 2.54, 2.69 | 0.57, 0.70 |
    | The party list, summary, Pokédex, card and bag | 0.82, 3.29 | 0.71, 0.94 |

    White 2 after: the movies 0.41, 0.58; the title 1.94, 2.00; the start menu 0.56, 1.75; CONTINUE 1.90, 2.00; the bag
    and card 0.57, 0.70; the party apps 0.71, 0.95 (the earlier measurement had none). Paced, on SDL's dummy video
    driver (software presenting, so that no window opens), the title's frame went from 6.66, 7.01 to 5.23, 5.43, of
    which 1.4 present; a paced 2D scene's frame took as long as before (the movies 3.8 against 3.9), what it no longer
    does given back to a core that clocks down while the program sleeps. The program's start to its first frame: 0.65
    to 0.25 s.
  - **Each commit's share:** the still 3D picture kept (the start menu 2.53 to 0.62, the bag and card 2.54 to 0.66,
    the party apps' p95 3.29 to 1.04); the 3D frame in bands, four on the M2 (the title 4.25 to 2.24, its 3D 2.98 to
    0.98; the flight 3.79 to 1.93); the game's thread at user-interactive QoS (no change measured, quiet or under 4 and
    8 busy loops: kept so that the command line runs as the app does); the geometry engine (0.50 to 0.43 on the title,
    0.62 to 0.59 in the flight); capture (0.18 to 0.08 on the title); the kept 2D sources (the 2D lines 0.03 to 0.04
    faster); the 2D colour effects (0.02 to 0.06 faster); SHA-1 (the start 0.65 to 0.25 s, the run directory's routes
    18 to 40% shorter).
  - **Measured and not kept:** pacing with one sleep to 2 or 4 ms before the frame's time, then `SDL_DelayPrecise`:
    half the wake-ups and of the system CPU, but frames 7% slower after the longer sleep and woken up to 4 ms late (17.4
    to 21.4 ms at the 99th percentile against 16.74). Also measured: the game's thread's QoS (above), and fewer or more
    render threads paced (the title 6.61 ms on 1, 5.55 on 2, 5.18 on 4, 5.00 on 8), which keeps the default at the
    performance cores.
  - **Documents:** ARCHITECTURE.md's "Where a frame's time goes" (the hot paths, their files and functions, the
    numbers) and its threads, rules and feature map; ADR 0016 (proposed); README.md, MODDING.md ("How fast it runs"),
    GLOSSARY.md (frame times, bands and workers), the guides of `app/`, `gfx/`, `hal/`, `cmake/`, `tests/` and `tools/`,
    `--help`, and lessons.md.
- **Next:** the user's review of ADR 0016; CI's first run of the branch (Linux and Windows build the new host threads
  and clock); the x86_64 slice's frame times under Rosetta, unmeasured since the bands; then as the last entries: the
  apps no route reaches yet, and a save with an adventure made by the port, once overlay 36 lets it into the field.
  Further gains, if wanted: the geometry engine is now the 3D frame's largest part on the game's thread (0.43 to 0.59
  ms), then the 2D text backgrounds; the workers' wake-up each frame, and the paced core's clock.
- **Blockers:** as the last entries: overlay 36 (issue #83, another contributor's active claim) gates phase 3; macOS 11
  to 14 have no runner and no virtual machine here.

## 2026-10-19: the crash reports and the sanitized build's packaging, after review

- **Upstream:** `2825ea75f` (merge of PR #393), as merged into the port's `main` by `607ccd1a6`; no sync in this task.
- **Port:** `0308587c6`, on the branch `port-hygiene` after `503080025`; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, with nothing to
  rebuild). No file outside `port/` changed.
- **Host build:** the last entry's machine and builds. `build/port` (Black 2), `build/port-w2` (White 2) and
  `build/port-san` (Black 2, `-DPORT_SANITIZE=ON`) built again; only `port/`'s files compiled, with no warning, so each
  build's count stays the last entry's 264, the decomp's. Two more here:
  - `build/port-x86`, a development build of the x86_64 slice alone (`-DCMAKE_OSX_ARCHITECTURES=x86_64`), with the
    pinned SDL3 built along from its release's source, copied from the verify tree's distribution build; its
    `port_host_tests` only (1,872 steps), run under Rosetta.
  - `build/cfgsan`, configured only, with `-DCMAKE_C_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG -fsanitize=address"`: the
    review's case.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with the oracle and the cached saves, the two builds at once: Black 2, 291 pass
    and 1 skip of 292 (541 s); White 2, 289 pass and 1 skip of 290 (532 s). Each version's 15 `oracle` tests pass,
    White 2's starter-save routes among them. The skip is `app_universal_slices`, which a development build's one slice
    skips. After a last edit of comments in `host.h` and the guides, every build was built again and the 20 tests this
    round touches (the 18 `host_*`, `app_check_exit`, `app_package_macos_out`) ran again in each, and pass.
  - T5, `build/port-san`, with the same oracle and Black 2's saves copied in, every test, after that last build: 292
    pass and 1 skip (the same) of 293 (565 s), its 24 `route` and 15 `oracle` tests among them, with no report of the
    runtime's. White 2's sanitized build wasn't built in this round.
  - The x86_64 slice under Rosetta, `build/port-x86`: the 18 `host_*` pass; the division faults there (SIGFPE, no
    raise) and each report names its scenario's function (`DivideByZero`, `IllegalInstruction`, `Breakpoint`, `Trap`),
    as the backtrace, which shows `OnFault`, `_sigtramp` and a frame at 0, doesn't. The same POSIX half in a scratch
    driver of both slices named each function too.
  - `package_macos.sh --build-dir build/cfgsan` stops with status 2 and the refusal naming
    `CMAKE_C_FLAGS_RELWITHDEBINFO`, before building anything, where the review found `503080025`'s script building
    everything.
    `test_package_macos.sh` passes its 44 checks in about 16 s; run against `503080025`'s script (in a scratch
    repository), it fails 5: the three new kinds of flag let through, and the stand-in's image kept.
  - Every run passed `--save-dir` where it runs the program, with `HOME` a scratch directory, and nothing under
    `~/Library`'s `Application Support/Frigate` or `Logs/Frigate` is newer than 16:33 on 2026-10-18. macOS's crash
    reporter writes its reports of the crash tests' programs under `~/Library/Logs/DiagnosticReports`, as before. T7
    not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** as the last entry: 457 function stubs and 46 data placeholders in each version.
- **64-bit warnings:** as the last entry.
- **Work done:** the review of `503080025` found the crash handler sound on both macOS slices, on the host's stack and
  on fibers, and four problems, all fixed:
  - **The up-front refusal missed flags, and a failed image stayed** (`port/tools/package_macos.sh`). It read
    `CMAKE_C_FLAGS` and `CMAKE_EXE_LINKER_FLAGS` alone, so a build with `-fsanitize=` in
    `CMAKE_C_FLAGS_RELWITHDEBINFO`, `CMAKE_EXE_LINKER_FLAGS_<CONFIG>` or `CMAKE_SHARED_LINKER_FLAGS` was built in full;
    the image's own check then failed with status 1 and left the image in place (the cleanup removed only its work
    directory). The refusal now reads every `CMAKE_*FLAGS*` entry of the cache and names the one that has
    `-fsanitize=`; and an image is removed on the way out, with a line saying so, from its making until it has passed
    every check, so that one that fails a check, or that the script stops before checking, isn't left.
  - **The image's check had no test**, though sanitizers.md said `app_package_macos_out` covered it.
    `test_package_macos.sh` now also builds a small CMake project of its own with the build's CMake, generator and
    compiler: a stand-in app, signed ad hoc, with a library named `libclang_rt.stand_in_osx_dynamic.dylib` in its
    `Frameworks` and nothing of a sanitizer in its cache. The script packages it, fails with status 1 and the image's
    message, and leaves no image; the same app without the library is packaged, and its image stays. The refusal's
    cases cover each kind of flag (`CMAKE_C_FLAGS`, a configuration's, an executable's and a shared library's linker's)
    and a cache whose comment and `-ADVANCED` entry mention none. The test makes two real images now, so its timeout
    is 120 s (`port/cmake/macos_app.cmake`).
  - **The report didn't name the crashing function on x86_64** (`port/hal/host/host_posix.c`). A backtrace taken in
    the handler shows after `_sigtramp` a frame at 0 and the function's callers under Rosetta, or nothing more on a
    fiber; on arm64 macOS it does show the faulting frame. The seam's report of SIGFPE, SIGILL and SIGTRAP now names
    the function from the saved program counter, `  in NAME+0xOFFSET` under the first line, as `trap.c`'s stubs name
    their caller, when `port_host_function_at` names one: on macOS any of the program's functions, on Linux only an
    exported one (an ELF program exports none of its own). The host tests check that line on macOS
    (`port/cmake/host.cmake`): the scenario's function, or `__pthread_kill` for arm64's raised SIGFPE; the scenarios'
    functions are `noinline` now, so that the name is theirs (`port/tests/host/test_host.c`). Windows' backtrace
    already starts at the faulting instruction. SIGSEGV and SIGBUS reports, which `trap.c` prints with the address
    accessed and no instruction, are unchanged.
  - **The texts:** `tests/README.md`, `test_host.c`'s and `cmake/host.cmake`'s headers said `__builtin_trap` runs on
    a fiber too; it runs on the host's stack only. `hal/README.md`'s `host_windows.c` row said the vectored handler
    moves a division by zero, an illegal instruction or a breakpoint off the stack everywhere: off the fibers those go
    to the unhandled-exception filter, on the faulting stack, which the row and the `host/` row now say. The docstrings
    of `check_exit.py` and `test_check_exit.py` are wrapped again. host.h, MODDING.md's "When the game stops",
    sanitizers.md, `README.md`'s macOS app and `tools/README.md` say what changed.
  - lessons.md has the round's four lessons.
- **Next:** as the last entry, the user's review of ADR 0014's acceptance first; CI on a branch with these commits for
  the host tests on Linux and Windows. A SIGSEGV or SIGBUS crash could name its function too, once the fault handler
  is given the instruction's address as well as the address accessed.
- **Blockers:** as the last entries.

## 2026-10-19: SIGFPE, SIGILL and SIGTRAP reported on macOS and Linux, and no disk image of a sanitized build

- **Upstream:** `2825ea75f` (merge of PR #393), as merged into the port's `main` by `607ccd1a6`; no sync in this task.
- **Port:** `503080025`, on the branch `port-hygiene` from `main` at `607ccd1a6`; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, from nothing in
  this worktree). No file outside `port/` changed.
- **Host build:** macOS 26.5.1 on an M2, Xcode 26.6 (Apple clang 21.0.0), RelWithDebInfo, Ninja:
  - `build/port` (Black 2) and `build/port-w2` (White 2), plain, from nothing, with the oracle (`pbw-oracle`) and each
    version's `oracle-saves` copied in from the verify tree's builds (White 2's starter save, SHA-1 `3b9dac231d1f`).
  - `build/port-san` (Black 2), `-DPORT_SANITIZE=ON`, from nothing, its `port_host_tests` only.
  - The POSIX half alone, `host_posix.c` with a small driver in a scratch directory, built as each macOS slice, the
    x86_64 one run under Rosetta: a division by zero faults there (SIGFPE, reported at the division), `__builtin_trap`
    is SIGILL and `__builtin_debugtrap` SIGTRAP, each reported and ending the program by its signal; and the x86_64
    division again under ASan and UBSan, its check off as in the host test, reported by Frigate's handler (status 136).
  - Warnings: 264 in each, the decomp's, as the last entries; none in `port/`.
  - Not built here: Linux and Windows (no Linux or Windows machine here; CI's jobs build and run the new tests on both
    once a branch with this commit is pushed). The Windows change is one exception added to a list
    (`host_windows.c`), not compiled here.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with the oracle and the cached saves, the two builds at once for most of their
    runs: Black 2, 291 pass and 1 skip of 292 (621 s); White 2, 289 pass and 1 skip of 290 (598 s). Each version's
    15 `oracle` tests pass, White 2's starter-save routes among them. The 6 tests more than the last entry's are the
    new host tests; the skip is `app_universal_slices`, which a development build's one slice skips. After the last
    edit of a comment in `host_posix.c`, both builds built again and the 20 tests this change touches ran again, and
    pass.
  - T5, `build/port-san`, the tests this change touches: the 18 `host_*`, `app_check_exit` and
    `app_package_macos_out`, all 20 pass, before and after that edit; the runtime reports nothing, the division's
    scenario reaching Frigate's handler past UBSan's divide check, which the scenario turns off for its one division,
    and past ASan's own SIGFPE handler, which Frigate's replaces as it replaces ASan's SIGSEGV one.
  - Every run passed `--save-dir`, with `HOME` a scratch directory, and nothing under `~/Library`'s
    `Application Support/Frigate` or `Logs/Frigate` is newer than 16:33 on 2026-10-18. macOS's own crash reporter
    keeps writing a report of each crash test's program under `~/Library/Logs/DiagnosticReports`, as for the crash
    tests before (up to 25 a program a day); the new tests are seven more such crashes a run. T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** as the last entry: 457 function stubs and 46 data placeholders in each version.
- **64-bit warnings:** as the last entry.
- **Work done:** the two follow-ups of the sanitized build's integration (the last entries' Next):
  - **The crash handler on macOS and Linux reports SIGFPE, SIGILL and SIGTRAP** (`port/hal/host/host_posix.c`). A
    division by zero on x86-64 (as the Pokédex's scroll bar had, R5 in sanitizers.md) or an illegal instruction ended
    the program with no word, since only SIGSEGV and SIGBUS were taken; on Apple Silicon `__builtin_trap` is a
    breakpoint (BRK), SIGTRAP, so the compiler's own traps went unreported there too. The seam now takes all five
    signals on its alternate stack once the program sets its fault handler (`port_trap_protect_data`, at start-up).
    SIGSEGV and SIGBUS go to the handler as before (`port/hal/trap.c`: a placeholder's name, or a crash); the other
    three the seam reports itself, as Windows reports an exception that isn't a memory fault:
    `port: crash: signal N at address ...`, at the program counter from the handler's saved registers (Linux's
    `si_addr` is 0 for INT3 and the sender's pid for a sent signal), with the backtrace, then the signal's default
    action ends the program by that signal (status 128 + N from a shell), as the docs give.
  - **Windows** reported a division by zero and an illegal instruction everywhere, a fiber included, but a breakpoint
    (0x80000003) only off the fibers, through the unhandled-exception filter: on a fiber the process ended unreported.
    `EXCEPTION_BREAKPOINT` is now among `IsFatal`'s exceptions, so the vectored handler reports it on a fiber too.
  - **Host tests** (`port/tests/host/test_host.c`, `port/cmake/host.cmake`): `divide_by_zero`, `illegal_instruction`,
    `breakpoint` and `trap`, on the host's stack, and `divide_by_zero_fiber` and `breakpoint_fiber` on a fiber, with
    `illegal_instruction_fiber` now an illegal instruction on arm64 too (UDF; it had been `__builtin_trap`, a
    breakpoint there) and its report checked on every host. Each expects the report and the backtrace; the signal on
    macOS and Linux (SIGFPE 8, SIGILL 4, SIGTRAP 5, and 4 or 5 for `__builtin_trap` by the slice), the exception on
    Windows (0xc0000094, 0xc000001d, 0x80000003). arm64 divides by zero without a fault, so there the scenario raises
    SIGFPE itself; the division's operands are both volatile, since LLVM turns `1 / x` into a comparison.
    `port/tests/app/check_exit.py`'s `--crash` now also requires the program to end by the signal or exception its
    report names, which `test_check_exit.py` checks both ways.
  - **`port/tools/package_macos.sh` refuses a sanitized build** before it builds anything: one configured with
    `PORT_SANITIZE`, or with `-fsanitize=` in its C or linker flags, whose app carries the toolchain's ASan runtime
    (`make_app.sh` copies it into `Contents/Frameworks`), with a message that names the build and what to package
    instead; and its checks of the image fail on any `libclang_rt.*` in it. It reads a cache's options as CMake does,
    in any case (`PORT_MACOS_DISTRIBUTION` too). `make_app.sh` still makes a sanitized build's app, for testing on
    this Mac, as sanitizers.md has it, and its header says so. `test_package_macos.sh` checks the refusals
    (`PORT_SANITIZE`, with and without `--distribution`, the flags) and that a build with `PORT_SANITIZE` off goes on.
  - The guides: `hal/README.md`, `tools/README.md`, `tests/README.md`, `app/README.md`, `README.md`'s macOS app,
    ARCHITECTURE.md's exit statuses and feature map, MODDING.md's "When the game stops" (each signal and exception by
    number) and sanitizers.md (the app, the packaging refusal, and a crash in the sanitized build reported as in the
    plain one); the headers of the files changed and of `cmake/host.cmake`, `cmake/app.cmake` and
    `cmake/macos_app.cmake`. lessons.md has the round's three lessons. No ADR: the seam's design
    ([ADR 0013](adr/0013-windows-build-with-msys2-clang64.md)) is unchanged, and its POSIX half now reports what the
    Windows half did.
- **Next:** as the last entry, the user's review of ADR 0014's acceptance first; for T5, a nightly run of both
  versions' sanitized routes and of the x86_64 slice's; CI on a branch with this commit, for the new host tests on
  Linux and Windows. Then the apps no route reaches yet, and a save with an adventure made by the port, once overlay
  36 lets it into the field.
- **Blockers:** as the last entries: overlay 36 (issue #83, another contributor's active claim) gates phase 3; macOS 11
  to 14 have no runner and no virtual machine here.

## 2026-10-19: the sanitized build's merge after review: the read check and CI's smoke test described as they are

- **Upstream:** `56ee36977` (merge of PR #391), as merged into the port's `main` by `6b50636b1`; no sync in this task.
- **Port:** `fe88f07f2`, on the branch `port-integrate` after `5691f92a5`; pushed to `port-sanitizers` on origin for CI,
  not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, with nothing to
  rebuild). No file outside `port/` and `.github/` changed.
- **Host build:** the last entry's machine and builds. `build/port-n`, `build/port-w2`, `build/port-san` and
  `build/port-san-w2` built again, every decomp file among them, since `port/include/port_host.h` is force-included in
  each. Warnings: 264 in each, the decomp's, none in `port/`. CI: `port-ci.yml` dispatched once on `port-sanitizers`
  at this commit, whose result the next entry gives.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with `build/oracle`'s `pbw-oracle` and the cached saves, the two builds at once:
    Black 2, 285 pass and 1 skip of 286 (565 s); White 2, 283 pass and 1 skip of 284 (556 s). Each version's 15
    `oracle` tests pass, White 2's starter-save routes among them.
  - T5, the sanitized builds, every test, both at once: Black 2, 286 pass and 1 skip of 287 (666 s), its 25 `route`
    and 15 `oracle` tests among them; White 2, 284 pass and 1 skip of 285 (639 s), its 23 `route` and 15 `oracle`
    tests among them. Every test carries the sanitizers' report patterns, so no output had a report.
  - The skip is `app_universal_slices` in all four, which a development build's one slice skips. Every run passed
    `--save-dir`, with `HOME` a scratch directory, and nothing under `~/Library`'s `Application Support/Frigate` or
    `Logs/Frigate` is newer than 16:33 on 2026-10-18. T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** as the last entry: 457 function stubs and 46 data placeholders in each version, 456 and 46 in the
  sanitized builds.
- **64-bit warnings:** as the last entry.
- **Work done:** the review of the merge (`93cf59b99` to `5691f92a5`) found the merge, the game's changes, the
  ignorelist, the ADRs, the sanitized routes and CI sound, and three problems of text:
  - **The read check's descriptions.** Since `77326dde8`, `port_heap_ds_read_begin` checks in every build that its
    span lies in one of the game's heaps alive or the main arena, and stops the program otherwise, but its
    declaration's comment in `port/include/port_host.h` and game-changes.md's row of the copyright screen's read
    (`src/gfl/arc_util.c`, `9c5b9bfce`) still said both hooks do nothing outside the sanitized build. Both now say what
    sanitizers.md, `hal/README.md` and GLOSSARY.md already did.
  - **CI's smoke test.** The `sanitize` job's comment in `port-ci.yml` and sanitizers.md called the smoke test CI's
    one run of the program into the game's `main()`. Without a ROM the program logs the line the test passes on
    (`port/app/main.c`) and `port_rom_boot` then stops it, before `port_os_run(NitroMain)`; ctest's
    `PASS_REGULAR_EXPRESSION` ignores the status it exits with (a signal still fails the test, as a probe of ctest
    showed). Both now say that no CI run reaches the game's `main()`, so that LeakSanitizer's check at exit on Linux
    covers start-up as far as the ROM check; the smoke test's comment in `port/CMakeLists.txt` says what it passes on.
  - **The merge's message** (`93cf59b99`) says "three memory fixes": at `9c5b9bfce` the track had two, `gf_font.c`'s
    and `printsys.c`'s, since it kept the copyright screen's read rather than fixing it. The tree counts them right
    (game-changes.md, the feature map: those two and the touch bars' load in `arc_util.c`, with `scroll_bar.c`'s
    division, the last two at `3dfcec981`); the message stays, since history isn't rewritten.
  - lessons.md has the round's two lessons.
- **Next:** as the last entry, the user's review of ADR 0014's acceptance first.
- **Blockers:** as the last entry.

## 2026-10-19: the sanitized build merged (ADR 0014), clean on every route of both versions, the Pokédex's included

- **Upstream:** `56ee36977` (merge of PR #391), as merged into the port's `main` by `6b50636b1`; no sync in this task.
- **Port:** `5691f92a5`, on the branch `port-integrate` from `main` at `6b50636b1`: `93cf59b99` (the merge of
  `port-sanitizers`), `77326dde8` (the track's notes confirmed), `3dfcec981` (the Pokédex under the sanitizers), then
  this entry; pushed to `port-sanitizers` on origin (new there) for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `93cf59b99` and
  `3dfcec981`. `check_mwcc_neutral.py` finds 3,238 of 3,242 file and version pairs identical to `56ee36977` at the
  merge, the track's three game files among them; the other four are `vm.c`'s and `event_field_proclink.c`'s Neutral
  rows, as before. At `3dfcec981`, `src/gfl/arc_util.c` and `src/system/scroll_bar.c`, both complete, are identical
  in both versions (4 of 4).
- **Host build:** macOS 26.5.1 on an M2, Xcode 26.6 (SDK 26.5, Apple clang 21.0.0), RelWithDebInfo, Ninja:
  - `build/port-n` (Black 2) and `build/port-w2` (White 2), plain, built again at each commit; the oracle is
    `build/oracle`'s `pbw-oracle` and the saves their `oracle-saves` (White 2's starter save, SHA-1 `3b9dac231d1f`).
  - `build/port-san` (Black 2) and `build/port-san-w2` (White 2), `-DPORT_SANITIZE=ON`, made from nothing at the merge
    (1,808 steps each), with the same oracle and saves copied in.
  - `build/port-san-dist`, `-DPORT_SANITIZE=ON -DPORT_MACOS_DISTRIBUTION=ON` (Black 2): both slices for macOS 11.0, SDL3
    built along and linked statically, 2,060 steps; no availability error.
  - Warnings: 264 in each plain and sanitized build, the decomp's, as the last entries; 528 in the sanitized
    distribution build (two slices); none in `port/`.
  - CI, `port-ci.yml` dispatched on `port-sanitizers`: run
    [39201456935](https://github.com/CallanCampos/Frigate/actions/runs/39201456935) (`77326dde8`), the `sanitize`
    job's first run, green on eight of nine jobs and red on Windows (Work done); run
    [39204084628](https://github.com/CallanCampos/Frigate/actions/runs/39204084628) (`3dfcec981`) is green on all
    nine. macOS 240 pass and 46 skip of 286; Linux 222 pass and 47 skip of 269; Windows 212 pass and 57 skip of 269;
    the distribution job, 240 pass and 46 skip of 286 on each of `macos-26`, `macos-15`, `macos-26-intel` and
    `macos-15-intel`, with the app checked; the `sanitize` job on `macos-26`, 238 pass and 25 skip of the 263 tests
    that aren't routes, then the routes that need no oracle, 3 pass (the smoke test and the replays' fixtures) and 8
    skip of 11; on `ubuntu-26.04`, with LeakSanitizer at every program's exit and the function check, 220 pass and 26
    skip of 246, then the same 3 and 8 of 11. Warnings: 264 on macOS, 263 on Linux and Windows, 528 in the
    distribution job, none in `port/`. 457 function stubs and 46 data placeholders on every host, 456 and 46 in the
    sanitized builds.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with the oracle, the two builds at once at a load average of 20 to 30, at
    `3dfcec981`: Black 2, 285 pass and 1 skip of 286 (1,194 s); White 2, 283 pass and 1 skip of 284 (1,079 s). The
    skip is `app_universal_slices`, which a development build's one slice skips. White 2's starter-save routes
    (`oracle_continue_starter`, `oracle_launch_party_menus`, `launch_summary_standalone`, `launch_touch_then_party`)
    pass.
  - T5, the sanitized build, at the merge (`93cf59b99`, with `77326dde8`'s tests): Black 2, of the 260 tests that
    aren't routes 259 pass and `app_universal_slices` skips (264 s), and 24 of its 25 routes pass (977 s); White 2,
    280 pass, 1 skips and 1 fails of 282 (996 s). Both versions stopped on the test launcher's Pokédex in
    `oracle_launch_party_menus`, first on UBSan's division by zero in `scroll_bar.c` (R5), then, once fixed, on ASan's
    read past a heap block in the touch bar's load (H4): fixed at `3dfcec981` (Work done). Every other route ran
    clean: the opening, the title, the start menu, New Game (Black 2), CONTINUE from both saves, the field menu's apps
    from both saves, White 2's starter-save routes, the sound's sequences.
  - T5 at `3dfcec981`, `ctest -j4` with the oracle, both builds at once: Black 2, 286 pass and 1 skip of 287 (854 s),
    its 25 routes among them; White 2, 284 pass and 1 skip of 285 (820 s), its 23 routes among them; the skip is
    `app_universal_slices`. No test's output has a sanitizer's report. The host seam's tests (`host_*`) pass under the
    sanitizers in both, and so do the new `nnsys_ds_reads`, `nnsys_ds_read_outside` and `app_check_exit`.
  - The sanitized distribution build: all 263 of the arm64 slice's tests that aren't routes pass (280 s),
    `app_slices` and `app_universal_slices` (both slices' opening, the x86_64 one under Rosetta) among them; the x86_64
    slice, by `ctest_one_slice.sh` under Rosetta, 252 pass and 1 skip of its 253 (320 s), and `replay_start_menu`,
    `smoke_reaches_game_main` and `oracle_launch_party_menus`, the Pokédex pass for pass against melonDS, pass on it
    (302 s). Before the fix, the plain distribution app's x86_64 slice (`build/port-dist`, at `c4c273e33`) ended at
    the Pokédex with SIGFPE (status 136), without a report.
  - Every run passed `--save-dir`, with `HOME` a scratch directory, and none wrote under `~/Library`: its
    `Application Support/Frigate` and `Logs/Frigate` still hold files from 16:33 on 2026-10-18 at the latest. T7 not
    yet.
- **Tiers:** not measured: no new decomp C, two `PLATFORM_PC` blocks in complete files.
- **Trap stubs:** 457 function stubs and 46 data placeholders in each version, as the last entries; 456 in the
  sanitized builds, whose `nwm.c` doesn't reference `func_02769520`.
- **64-bit warnings:** none of the pointer and integer size kinds; the counts of other kinds are the last entries'.
- **Work done:**
  - **The merge of `port-sanitizers`** (`93cf59b99`), every conflict keeping both sides. Main's host seam owns the
    pages, faults and backtraces that `trap.c` and `fiber.c` used, so the track's changes sit on it: the placeholders'
    pages become readable at exit through a new `port_host_protect_read` (`port/hal/host`, POSIX's `mprotect`,
    Windows' `VirtualProtect`, which also forgets the region; `host_files` checks it), and `fiber.c` keeps main's
    dummy return slot and guard with the track's LeakSanitizer roots and `FreeFakeStack`. `CMakeLists.txt` has the
    availability check and the sanitized build's options; `port-ci.yml` the four jobs; `oracle.cmake` main's 1,800 s
    with the labels, and `launch_summary_standalone` and `launch_touch_then_party`, now in both versions, labelled
    `route` and `oracle`. The track's ADR is 0014 (file, index, every reference), accepted.
  - **The track's notes confirmed** (`77326dde8`). The claim that a sanitizer's report fails a test whatever else it
    expects was false for a test with `WILL_FAIL`, which turns ctest's failure on the report in its output into a
    pass: `integrate_unknown_option` runs through `check_exit.py --status 2`, configuring stops on any `WILL_FAIL`,
    and `check_exit.py` fails a run with a report whatever ending it wants (its `--crash` took a sanitized program's
    abort for a crash, and ctest saw only the tail it printed); it gains `--abort`, and `test_check_exit.py` checks
    it. `port_heap_ds_read_begin` checks, in every build, that its span (widened to ASan's granules where it unpoisons
    them) lies in one of the game's expanded or frame heaps alive or else in the main arena
    (`port_os_main_arena_holds`), and aborts with a message otherwise (`nnsys_ds_reads`, `nnsys_ds_read_outside`).
    The documents say which check holds where.
  - **The Pokédex under the sanitizers** (`3dfcec981`), the two reports on what main added since the track was cut,
    each fixed at its cause in the game's file under `PLATFORM_PC`, with rows in game-changes.md and decomp-findings.md:
    R5, `ScrollBar_GetPos` divides by a list's `max`, 0 when its rows all fit, where the DS's `_u32_div_f` returns the
    dividend and x86-64 faults; H4, the touch bars ask the dynamic character load for 2 KiB of a 128-byte file, which
    read past its heap block into characters the bar never shows: the host reserves the same characters and copies
    the file's alone. CI's Windows failures of run 39201456935, `app_check_exit` (Python's own abort ends with
    0xc0000409 there, so its stand-in turns the report off as the port's programs do) and `nnsys_ds_read_outside`
    (`%p` prints no `0x` on Windows), are fixed too.
  - **Documents:** sanitizers.md (which check makes a report fail a test, R5, H4, the merged tree's run, the sanitized
    distribution build), the feature map's row, the guides (`cmake/`, `tests/`, `hal/`, `tools/`), GLOSSARY.md, the
    ADR index and ADR 0014's note, game-changes.md (`arc_util.c`'s first row names `9c5b9bfce`, the two new rows
    `3dfcec981`), decomp-findings.md, and lessons.md.
- **Next:** the user's review of ADR 0014's acceptance; for T5, a nightly run of both versions' sanitized routes and of
  the x86_64 slice's; the host's fault handler on macOS and Linux reports SIGSEGV and SIGBUS only, so a SIGFPE or
  SIGILL ends the program without a report, where Windows reports every exception; `package_macos.sh
  --distribution` would package a sanitized build's app too. Then as the last entries: the apps no route reaches yet,
  and a save with an adventure made by the port, once overlay 36 lets it into the field.
- **Blockers:** as the last entries: overlay 36 (issue #83, another contributor's active claim) gates phase 3; macOS 11
  to 14 have no runner and no virtual machine here.

## 2026-10-18: the sanitized build after review: the copyright screen's read kept, every report fatal on Linux

- **Upstream:** `993e6c49b`, as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `9c5b9bfce`, on the branch `port-sanitizers` after `7773fd59c`: the two reviews' fixes and this entry; not
  pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) with
  `src/gfl/arc_util.c`'s new block. `check_mwcc_neutral.py` finds 3,176 of 3,180 file and version pairs identical to
  `993e6c49b`, the three game files among them; the other four are `vm.c`'s and `event_field_proclink.c`'s Neutral
  rows, as before.
- **Host build:** arm64 macOS, RelWithDebInfo, Ninja, Apple clang 21.0.0, in `build/`: `port` and `port-w2` plain,
  `port-san` and `port-san-w2` with `-DPORT_SANITIZE=ON`. Each gives the decomp's 263 warnings and none in `port/`.
  Not pushed, so no CI run; Linux not built.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4` with the oracle and its saves in each build: Black 2, all 267 pass (2,134 s);
    White 2, 261 of 263 pass and the two that need its starter save skip (632 s). The tests left `~/Library` alone
    (`HOME` a scratch directory).
  - T5, the strict sanitized build, Apple clang: Black 2, all 268 pass (3,062 s), its 24 routes among them; White 2,
    262 of 264 pass and `oracle_continue_starter` and `oracle_launch_party_menus` skip (2,480 s), 18 of its 20 routes
    run. No test's output has a sanitizer's report. Three suites ran at once on a machine loaded to 150: `asm_oracle`
    timed out at its 900 s in each and passed run again (Black 2's in 261 and 382 s; sanitized White 2's, run without
    ctest's limit, in 692 s, 181 s of CPU), and `oracle_continue` and `oracle_continue_starter` failed only on the
    frames this change first held close in them too (below), and passed once those were dropped.
    Upstream clang and LeakSanitizer were not run again: what changed in what runs is `fnd_hooks.c` and its hooks'
    calls in `arc_util.c`, which Apple clang's ASan covers, and the runtime's defaults, which are already macOS's.
  - The oracle's sequences with the copyright screen's read restored: the title's first frame of 3D differs from
    melonDS's in 8 pixels on the bottom screen in both versions (opening, port frame 3527), as at `2da26e048`; with
    `c8eaf91c9`'s change it differed in 14,330 (a band over lines 38 to 95 drawn as background). Over the 34 frames now
    held close in the opening and the title, the most that differ is 235 (Black 2) and 104 (White 2) of the 491
    allowed; the old change fails the first four in both. From a save (continue, continue_starter) the band's noise
    differs from the DS's, 10,764 pixels at port frame 1116 in Black 2 and 10,974 in White 2, since the heap holds
    other bytes there on the port: within the 3D's tolerance, as before, and those sequences don't hold the frames
    close. A probe of the hooks on 2,000 random heaps found the shadow given back exactly after each read.
    `compare.py`'s and `oracle.py`'s unit tests pass (51).
  - CI's `sanitize` job: not run (not pushed). T7 not yet.
- **Tiers:** not measured: no new C.
- **Trap stubs:** as in the last entry: 512 function stubs and 46 data placeholders in each version (`gen_stubs`, the
  plain builds), 511 in the sanitized builds.
- **64-bit warnings:** not measured by kind; each build gives the decomp's 263 warnings of every kind, as the last
  entry, and none in `port/`.
- **Work done:** the two reviews of `3271b7d43` to `7773fd59c`, each finding checked; all were real.
  - **H1 is not a bug to fix.** The copyright screen's tiles read past its file's block show on the title screen's
    bottom screen as it fades in, as melonDS shows them, and the host read the DS's bytes; `c8eaf91c9`'s change drew
    the background there, which the oracle comparison's 3D tolerance let through. The game reads them again, around
    `port_heap_ds_read_begin` and `port_heap_ds_read_end` (`port/include/port_host.h`), which in the sanitized build
    let that one read reach past the block and then poison again exactly what was poisoned (`fnd_hooks.c`). Its rows
    in game-changes.md and decomp-findings.md, sanitizers.md, ADR 0014, lessons.md and the feature map say so. The
    opening and the title now hold the title's first 34 frames of 3D close (`compare.py --close`, a sequence's
    `close`): there a screen with 3D may differ in at most 1% of its pixels at all. The continue sequences can't, since
    from a save the bytes after the file aren't the DS's on the port (the oracle README's "Known differences").
  - **Every report fatal on every host.** Off Apple, ASan's runtime ends a report (UBSan's and LeakSanitizer's too)
    with status 1, which the tests that pass on their output or on a failure accepted. `sanitizers.c` makes
    `abort_on_error=1` the default of every sanitized program, and every test of the sanitized build fails on a
    sanitizer's report in its output (`FAIL_REGULAR_EXPRESSION`, set at the end of the directory).
  - **A resized block** has the DS's size of what the game asked for addressable after every successful resize, not
    only when the host's size changed (`fnd_hooks.c`).
  - **Labels** are given where each test is defined (`port_test_labels`), the same tests as before; `timing` is listed.
    `PORT_SANITIZE_RECOVER` without `PORT_SANITIZE`, and a timing gate with it, are refused at configure time.
  - **Documents:** sanitizers.md (exit statuses per host, LeakSanitizer's reports and the policy for a third party's
    leak, the other options, what CI runs without a ROM, the survey's real coverage, the reports apart from the gaps),
    the CI job's comments, the `cmake/` and `tests/` guides, PLAN.md §4.4, `nnsys.cmake`'s header, and
    `port_mco_saved_stack_pointer` declared once, in `hal/os/mco_impl.h`. The last entry's coverage is corrected:
    White 2 ran 18 of its 20 routes, and the runs that end on a trap stub get no leak check.
- **Next:** as in the last entry: White 2's starter route, so that `party_menus` and `continue_starter` run in White 2;
  the apps no route reaches yet; a save with an adventure made by the port, once overlay 36 lets it into the field. For
  T5: the sanitize job's first CI run, Linux's LeakSanitizer and function check included; the `OS_Alloc` heaps under
  ASan as FND's are; a nightly run of the sanitized routes.
- **Blockers:** as in the last entry.

## 2026-10-18: the sanitized build, clean on every route of both versions, with the game's heaps under ASan

- **Upstream:** `993e6c49b`, as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `7773fd59c`, on the branch `port-sanitizers` from `main` at `2da26e048`: `3271b7d43` (the OS core's DMA and
  timer handlers called with their type), `c8eaf91c9` (three memory bugs of the game, fixed under `PLATFORM_PC`),
  `521d483b2` (the fibers and the trap under the sanitizers), then this commit (the sanitized build, its CI job, the
  documents and this entry); not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) with the three game
  changes, two of them in complete files (`arc_util.c`, `gf_font.c`). `check_mwcc_neutral.py` finds 3,176 of 3,180 file
  and version pairs identical to `993e6c49b`, the three changed files among them (`printsys.c` is incomplete, so this is
  its proof); the other four are `vm.c`'s and `event_field_proclink.c`'s Neutral rows, as before.
- **Host build:** arm64 macOS, RelWithDebInfo, Ninja, in `build/`: `port` (Black 2) and `port-w2` (White 2), plain;
  `port-san` (Black 2) and `port-san-w2` (White 2) with `-DPORT_SANITIZE=ON` under Apple clang 21.0.0; `port-san-llvm`
  (Black 2) under Homebrew clang 23.1.3; and a survey build with `-DPORT_SANITIZE_RECOVER=ON`, deleted after. The
  Apple builds give the decomp's 263 warnings and none in `port/` (White 2's plain build compiled every file); the
  Homebrew build 265, the decomp's and two of clang 23's `-Wunused-but-set-global` in older port code, which Apple
  clang 21 doesn't give (`hal/dwc/ov011_pending.c`'s `sGameSpyServer`, `tests/rom/test_backup.c`'s `sReplyData`),
  left as they are. Not pushed, so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, plain, `ctest -j4`, with the oracle and its saves in each build: Black 2, all 267 pass (1,399 s);
    White 2, 261 of 263 pass and the two that need its starter save skip (1,744 s). The tests left `~/Library` alone
    (`HOME` a scratch directory).
  - T5, the sanitized build (ASan and UBSan, every report fatal, the game's heaps poisoned): Black 2 under Apple clang,
    all 268 pass (the plain build's 267 and `os_all_fake_stacks`; 2,463 s), its 24 routes among them, the oracle's
    comparisons included; White 2, 262 of 264 pass and `oracle_continue_starter` and `oracle_launch_party_menus` skip,
    as in the plain build (2,771 s), 18 of its 20 routes among them (the two that skip are routes); Black 2 under
    upstream clang 23 with LeakSanitizer (`ASAN_OPTIONS=detect_leaks=1`), which adds the function check: the 244 tests
    that aren't routes pass (514 s), and so do its 24 routes, with no leak at the exit of any run that ended normally
    (1,938 s); the runs that end on a trap stub (`oracle_continue`, `oracle_continue_starter`, and `oracle_port_save`'s
    when it stops on one) leave through `_Exit`, which has no leak check. The start menu's route with LeakSanitizer
    ends in 67 s (it hung for 36 minutes in the survey). A survey build ran Black 2's 18 routes twice: the first round found
    the game's three memory bugs below, the second, after their fixes, no report.
  - The asm oracle passes in the sanitized build too (it left the sanitizers' options out of its libraries).
  - CI's new `sanitize` job: written, not run (not pushed). T7 not yet.
- **Tiers:** not measured: no new C.
- **Trap stubs:** 512 function stubs and 46 data placeholders in each version (`gen_stubs`, the plain builds); the
  sanitized builds have 511, their `lib/twl/src/nwm/nwm.c` no longer referencing `func_02769520`.
- **64-bit warnings:** not measured by kind; White 2's plain build compiled every file with 263 warnings of every kind,
  as the last entries, and the three changed game files compile with none.
- **Work done:** Frigate is sanitizer-clean on every route the tests run, in both versions, from the sanitizer survey
  of this day (four UBSan reports, R1 to R4, and seven gaps, G1 to G7; [sanitizers.md](sanitizers.md) has each with
  what was done), and the sanitized build is repeatable
  ([ADR 0014](adr/0014-sanitized-build-game-heaps-poisoned-ds-isms-ignorelisted.md)).
  - **`PORT_SANITIZE`** builds every target but SDL3 under `-fsanitize=address,undefined -fno-sanitize-recover=all`
    with an ignorelist, and `PORT_SANITIZE_RECOVER` lets a survey run go on. The tests carry labels by what they need,
    `route` and `oracle`, so `ctest -L route -LE oracle` runs the routes that need no oracle. CI's `sanitize` job builds
    the sanitized Black 2 on macOS and Linux and runs every test that isn't a route, then the routes that need no
    oracle, which skip without the ROM. `os.cmake`'s header no longer gives a recipe that left alignment fatal.
  - **R1, R2** (NitroSystem's 2-aligned reads of an animation's cell index through a 4-aligned type) and **R3** (the
    save data's calls to 56 typed initializers through `void (*)(void *)`): right on every host, left out by
    `port/cmake/sanitizer_ignorelist.txt`, by check and function, each with its reason. **R4** (`OS_SetIrqFunction`'s
    cast of a DMA or timer handler to a callback, in Frigate's own OS core): fixed in `os_interrupt.c` and `os_tick.c`.
  - **G1:** UBSan's reports print ASan's stack, which follows the fibers (`port/hal/os/sanitizers.c`). **G2:** Apple
    clang emits no function check for C and has no LeakSanitizer: upstream clang locally and CI's Linux job. **G3:** the
    trap's placeholders become readable at exit, so LeakSanitizer's check ends. **G4:** a suspended fiber's root region
    is its live frames only. **G5:** a fiber destroyed while suspended has its fake stack freed. **G6:** the game's
    expanded and frame heaps are poisoned block by block (`fnd_hooks.c`, FND built without ASan). **G7** (UBSan reports
    a place once) can't be changed and doesn't matter once a report is fatal.
  - **What G6 found**, three memory bugs of the game that the DS's heap tolerates, fixed under `PLATFORM_PC` with rows
    in game-changes.md and decomp-findings.md: the copyright screen's load of 0x8000 bytes from a smaller file (H1),
    the main font's cell a row and a byte too short for its glyph copy (H2), and a write after free when a print
    stream's task ends (H3).
  - The documents: [sanitizers.md](sanitizers.md) (running it, reading a report, what Frigate tells the sanitizers,
    what is left out and why, what it can't see, what it found), the guides (the README, `cmake/`, `tests/`, `hal/`),
    the feature map's row for the sanitized build, GLOSSARY.md, MODDING.md, PLAN.md §4.5 and §6, and lessons.md.
- **Next:** as in the last entry: White 2's starter route, so that `party_menus` and `continue_starter` run in White 2;
  the apps no route reaches yet; a save with an adventure made by the port, once overlay 36 lets it into the field. For
  T5: the sanitize job's first CI run, Linux's LeakSanitizer and function check included; the `OS_Alloc` heaps under
  ASan as FND's are; a nightly run of the sanitized routes.
- **Blockers:** as in the last entry: overlay 36 (issue #83, another contributor's active claim) gates phase 3, the
  overworld, and the launcher's retirement (ADR 0012); `NNSi_objSizeWTbl` (ARM9 main's NitroSystem G2D, no C yet)
  stops the Pokédex; overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: the macOS and White 2 merge after review: each VRAM view checked against the host's page again

- **Upstream:** `81eea95ee` (merge of PR #387), as merged into the port's `main` by `4d0ce54b7`; no sync in this task.
- **Port:** `c4c273e33`, on the branch `port-integrate` after `84b7d7f96` (the last entry): `6ce5ab330` (the review's
  fixes), then this entry; pushed to `port-macos-dist` on origin for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `6ce5ab330`. No
  file outside `port/` changed.
- **Host build:** the last entry's Mac (macOS 26.5.1 on an M2, Xcode 26.6, SDK 26.5, Apple clang 21.0.0),
  RelWithDebInfo, Ninja:
  - `build/port-n` (Black 2) and `build/port-w2` (White 2): `port_gx_tests` built again, no error, no warning. The
    oracle and the saves are the last entry's.
  - `build/port-dist` and `build/port-dist-w2`, the distribution builds: `package_macos.sh --distribution` and
    `--distribution --version w2_us` made the apps and their disk images, `Frigate Black 2 0.0.2867.dmg` and
    `Frigate White 2 0.0.2867.dmg`, 13 MB each; `cmake --build build/port-dist` built the tests' two slices; no warning
    in `port/`.
  - CI, `port-ci.yml` dispatched on `port-macos-dist` at `6ce5ab330`: run
    [39197055618](https://github.com/CallanCampos/Frigate/actions/runs/39197055618) is green on all seven jobs. macOS
    237 pass and 46 skip of 283; Linux 219 pass and 47 skip of 266; Windows 209 pass and 57 skip of 266; the
    distribution job, 237 pass and 46 skip of 283 on each of `macos-26`, `macos-15`, `macos-26-intel` and
    `macos-15-intel`, where `package_macos.sh --distribution` made the app and its image and `check_app.sh` passed it,
    both slices started on Apple Silicon and the x86_64 one on Intel. 457 function stubs and 46 data placeholders on
    every host. Warnings: 264 on macOS (the distribution job's 528, two slices), 263 on Linux and Windows, as the last
    entry, none in `port/`.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 with the oracle, `ctest -j4`, the two builds at once at a load average of 15 to 18, at `6ce5ab330`:
    Black 2, 282 pass and 1 skip of 283 (782 s); White 2, 280 pass and 1 skip of 281 (570 s). The skip is
    `app_universal_slices`, as in the last entry. The 25 `gx_` tests pass in each.
  - The distribution build's 37 `gx_` and `host_` tests pass as each slice: arm64 by `ctest`, x86_64 under Rosetta by
    `ctest_one_slice.sh`, the aliasing backends (`shm`, `mach-entry`, `mach-remap`) running on both. `check_app.sh
    --archs "arm64 x86_64" --minimum 11.0 --run-every-slice` passes on both apps: both slices for macOS 11.0, no
    `Contents/Frameworks`, signed ad hoc without the hardened runtime, and both slices start, x86_64 under Rosetta.
  - Every run passed `--save-dir`, and none wrote under `~/Library`: its `Application Support/Frigate` and
    `Logs/Frigate` still hold files from 16:33 at the latest. T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed in this task.
- **Trap stubs:** 457 function stubs and 46 data placeholders in each version, as the last entry.
- **64-bit warnings:** none of the pointer and integer size kinds; the counts of other kinds are the last entry's.
- **Work done** (`6ce5ab330`), the review of the merge's six commits:
  - **`test_gx.c` checks each view against the page the system reports again.** The merge had folded the mac track's
    `% port_vram_os_page_size()` into `% PORT_HOST_PAGE_ALIGNMENT`, which shows only that the linker kept `hw.c`'s
    alignment: on a host whose page is larger than the macro but fits in a slot, the aliasing backends would skip and
    the game copy the banks with every test passing. `test_views` checks both: the macro, and `port_host_page_size()`
    wherever a page is no larger than a slot. The review's example, Linux with 64 KiB pages, isn't one of those:
    there `port/hal/gx/vram_os.c` turns the aliasing backends off whatever the window's alignment (a page larger than
    a 16 KiB slot can't place F, G, H and I), so the copy backend is the design, and the check leaves it alone rather
    than fail `gx_backend_copy`. Rosetta reports 4096, as an Intel Mac does (probed with `sysconf` as each slice), so
    the x86_64 slice holds the check there too. A lesson says why the run-time check stays.
  - **ADR 0015's CI note** names the job's first run, 39190187078 (red on the Intel runners), and its first green,
    39191368934, where it named the second as the first.
  - **The README's distribution steps** say CI starts both slices on Apple Silicon and the x86_64 one on Intel, which
    can't run arm64, where they said every slice.
  - **The ADR index keeps 0014** for the sanitized build's ADR, 0013 on `port-sanitizers` until it merges: a reserved
    row, and a numbering rule saying that the next free number is 0016.
- **Next:** as the last entry.
- **Blockers:** as the last entry: overlay 36 (issue #83, another contributor's active claim) gates phase 3; macOS 11
  to 14 have no runner and no virtual machine here.

## 2026-10-18: the macOS distribution and White 2 starter tracks merged, ADR 0015 accepted, green on every host

- **Upstream:** `81eea95ee` (merge of PR #387), as merged into the port's `main` by `4d0ce54b7`; no sync in this task.
- **Port:** `84b7d7f96`, on the branch `port-integrate` from `main` at `62a4e5e2f`: `4122413bf` (the merge of
  `port-macos-dist`), `b23ccc24f` (ADR 0015 accepted), `b57e219d6` (the mac track's two notes corrected),
  `3dd85670c` (the merge of `port-w2-starter`), `cee1e635d` (`host_trap_placeholder_fiber` on an Intel Mac), then this
  entry; pushed to `port-macos-dist` on origin (new there) for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `3dd85670c`
  and `cee1e635d`. No game file changed: outside `port/`, only the landing page and `port-ci.yml`.
- **Host build:** macOS 26.5.1 on an M2, Xcode 26.6 (SDK 26.5, Apple clang 21.0.0), RelWithDebInfo, Ninja:
  - `build/port-n` (Black 2), built again at `4122413bf`, and `build/port-w2` (White 2), at `3dd85670c`: 1,338 of a
    fresh build's steps each (`81eea95ee`'s C merged since their last build, and `host.h`), then CMake alone at the
    later commits; no error, no warning in `port/`. The oracle is `build/oracle`'s `pbw-oracle` in both
    builds' `oracle/` (no oracle source changed), and the saves their `oracle-saves`: Black 2's two, White 2's
    `first_save` and its `starter_save`, copied from the White 2 track's build (SHA-1 `3b9dac231d1f`, the one
    `SAVES` keeps).
  - `build/port-dist` and `build/port-dist-w2`, the distribution builds, made from nothing by
    `package_macos.sh --distribution` and `--distribution --version w2_us`: 1,873 steps each, SDL3's included, both
    slices for macOS 11.0; 528 warnings each, the decomp's 264 a slice, none in `port/`, no availability error, no
    linker warning; 119 sections on arm64 and 117 on x86_64, as predicted, and no leaks.
  - CI, `port-ci.yml` dispatched on `port-macos-dist`: run
    [39190187078](https://github.com/CallanCampos/Frigate/actions/runs/39190187078) (`3dd85670c`) was green on macOS,
    Linux, Windows and the distribution job's `macos-26` and `macos-15`, and red on `macos-26-intel` and
    `macos-15-intel` (Work done); run [39191368934](https://github.com/CallanCampos/Frigate/actions/runs/39191368934)
    (`cee1e635d`) is green on all seven jobs. Warnings: 264 on macOS (the distribution job's 528, two slices), 263 on
    Linux and Windows, as the last entries, none in `port/`.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 with the oracle, `ctest -j4`, the two builds at once at a load average of 15 to 35, at
    `3dd85670c`: Black 2, 282 pass and 1 skips of 283 (683 s); White 2, 280 pass and 1 skips of 281 (672 s). The skip is
    `app_universal_slices`, which a development build's one slice skips. White 2's new tests pass:
    `oracle_continue_starter`, `oracle_launch_party_menus`, `launch_summary_standalone` and `launch_touch_then_party`,
    with `oracle_saves` taking both saves from the cache. At `cee1e635d`, the 12 `host_*` tests pass in each build,
    and the two trap tests in `build/port-dist` (arm64); its x86_64 slice, run by hand under Rosetta, reports `+0x0`
    too.
  - The distribution apps: `check_app.sh "build/port-dist/Frigate Black 2.app" --archs "arm64 x86_64" --minimum 11.0
    --run-every-slice`, and the same for `Frigate White 2.app`, pass: both slices macOS 11.0 (SDK 26.5),
    `LSMinimumSystemVersion` 11.0, no `Contents/Frameworks`, signed ad hoc without the hardened runtime, and every
    slice starts, the x86_64 one under Rosetta. Their disk images, `Frigate Black 2 0.0.2864.dmg` and
    `Frigate White 2 0.0.2864.dmg`, 13 MB each, were checked by `package_macos.sh`.
  - CI, run 39191368934: macOS 237 pass and 46 skip of 283; Linux 219 pass and 47 skip of 266; Windows 209 pass and 57
    skip of 266; the distribution job, 237 pass and 46 skip of 283 on each of `macos-26` (macOS 26.6.2), `macos-15`
    (15.7.9, Xcode 26.3), `macos-26-intel` (26.6.1) and `macos-15-intel` (15.7.9), the Intel ones running the x86_64
    slices natively; there `package_macos.sh --distribution` made the app and its image, and `check_app.sh` passed
    it, every slice started on Apple Silicon (the x86_64 one under Rosetta) and the x86_64 one on Intel.
  - Every run passed `--save-dir`, and none wrote under `~/Library`: its `Application Support/Frigate` and
    `Logs/Frigate` hold files from 16:33 at the latest, before this task. T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed in this task.
- **Trap stubs:** 457 function stubs and 46 data placeholders in each version, locally and on every CI host, from 474
  at the last entries: upstream's C merged by `4d0ce54b7`. A continued game without `--launch` still stops on
  `FIELDMAP_PROC_FUNCS`.
- **64-bit warnings:** none of the pointer and integer size kinds, which the game's target leaves off; the counts of
  other kinds are the last entries' (Host build above).
- **Work done:**
  - **The merge of `port-macos-dist`** (`4122413bf`), every conflict keeping both sides. The x86_64 Mach-O alignment
    folds into the Windows track's seam: `PORT_HOST_PAGE_ALIGNMENT` in `port/hal/host/host.h` is 4096 on Windows and
    for Apple x86_64 (`#if defined(_WIN32) || (defined(__APPLE__) && defined(__x86_64__))`), 16384 elsewhere, and
    `hw.c`, `gen_stubs.py` and `test_gx.c` take it, without the branch's own `#if`s or `PORT_TRAP_PAGE_ALIGNMENT`;
    `test_gx.c` keeps the branch's check that each view starts a slot of region 0x06's window. `overlay_sections.cmake`
    has the branch's JSON predictions with `main`'s `checked_objects.txt`; `port-ci.yml` has the `host`,
    `host-windows` and `macos-distribution` jobs; the guides, the feature map and the indexes have both sides' rows.
    The entries and lessons of the merged branches went above `main`'s, each top `PORT_SHA` filled (`6489c0795`,
    `6c0aed21f`, `807116946`).
  - **ADR 0015 accepted** (`b23ccc24f`) by the port's developer, as every earlier ADR: macOS 11 and the two
    architectures are the developer's call. It keeps 0015, the Windows track holding 0013 (the branch's first commits
    say 0013: history); PLAN.md §7 phase 5 and ADR 0010's note name it.
  - **The mac track's notes, corrected** (`b57e219d6`): its entry after review and lessons.md said that
    `-Wno-everything` silences `-Werror=unguarded-availability` in either order. Probed again with `/usr/bin/cc`,
    `-Wno-everything -Werror=unguarded-availability` holds in a file of its own; `-w`, or a `-Wno-everything` after
    the `-Werror=`, cancels it; and `minicoro.h` is reached through `-isystem`, which hides the diagnostic whatever the
    flags, so the fix, `--no-system-header-prefix=minicoro.h` without `-w`, stands. The README's distribution steps
    build the tests (`cmake --build build/port-dist`) before `ctest`, since `package_macos.sh` builds only the app.
  - **The merge of `port-w2-starter`** (`3dd85670c`): only STATUS.md and lessons.md conflicted.
  - **An Intel Mac's `memset`** (`cee1e635d`): the distribution job ran the Windows track's host tests on Intel Macs
    first, and `host_trap_placeholder_fiber` alone failed there: the C library's `memset`, clearing a placeholder's
    first 4 bytes on a fiber, touches `+0x3` first, where the test wanted `+0x0`. The test takes any of the four; a
    lesson says why Rosetta didn't show it.
  - **The documents** say what CI ran: the README's quick start and distribution build, the landing page, the app
    guide's and the glossary's rows and the feature map's macOS app row (tested on macOS 15 and 26 by CI, with the ROM
    on 26 only), and a note on ADR 0015.
- **Next:** as the last entries: the apps no route reaches yet (the town map, the Xtransceiver, the medal box, the
  options); White 2's Route 19 save; Windows' stage 4; a save with an adventure made by the port, once overlay 36 lets
  it into the field; a sound comparison sequence on the game's count; the distribution build on a macOS older than 15.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7. macOS 11 to
  14 have no runner and no virtual machine here.

## 2026-10-18: White 2's starter save, and its party list and Pokédex against melonDS pass for pass

- **Upstream:** `9b486e486`, merged by `334077ba7`; no sync in this task.
- **Port:** `807116946`, on the branch `port-w2-starter` from `77263e609`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, a full build in the
  worktree). This task changes only `port/` and `.github/README.md`.
- **Host build:** arm64 macOS, RelWithDebInfo, `build/port` (Black 2) and `build/port-w2` (White 2), both Ninja, from
  scratch: no errors, 264 warnings in each (263 at the last entry, built before `334077ba7` merged upstream's
  `9b486e486`; this task changes no C), none of the pointer and integer size kinds. The oracle is `build/verify-tree`'s
  `pbw-oracle`, copied into both builds' `oracle/` (no oracle source changed), and the cached saves its `oracle-saves`
  (White 2's `first_save`, Black 2's two). Not pushed, so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, White 2, `ctest -j4` with the oracle: all 265 pass, none skipped (2,402 s, at a load average of 35
    to 170). New in White 2: `oracle_continue_starter`, 3,407 of 3,407 compared frames match (1,312 exact, 2,095 with
    3D within the tolerance), with continue's known differences, the start menu's saved game with its Pokédex line
    among them, then the stop on `FIELDMAP_PROC_FUNCS` after frame 3,436; `oracle_launch_party_menus`, 8 of 8 app
    screens at the oracle's counts (the summary left out, `SUMMARY_SCROLL`), 2,639 of 2,639 counts of the apps' spans
    the oracle's pictures, the 19 rows of the table held to the oracle's run, event type 0 after each of the four
    apps, the save unchanged; `launch_summary_standalone` and `launch_touch_then_party`, which ran in Black 2 only
    (the party list after a touch shows the White 2 table's party list at count 4250). `oracle_saves` takes both saves
    from the cache, and `oracle_compare` checks the White 2 entries of `SAVES` and `LAUNCHES` (their input files, the
    table, the known rows among its labels).
  - Black 2, the oracle's and the launcher's tests, `ctest -j2 -R 'oracle|launch'`: all 31 pass (3,513 s), with the
    same results as the last entry (`oracle_continue_starter` 3,415 of 3,415, `oracle_launch_party_menus` 8 of 8 and
    2,639 of 2,639, `oracle_launch_field_menus` 2 of 2 and 621 of 621). The rest of Black 2's ctest wasn't run: this
    task changes no code the other tests build.
  - The starter save on White 2's oracle: SHA-1 `3b9dac231d1f` in three runs (one with traces and pictures, two of
    `oracle.py save starter_save --version w2_us`, from the cached `first_save`). Every run passed `--save-dir` or
    `--save`, and none wrote under `~/Library`: the files in its `Application Support/Frigate` and `Logs/Frigate` are
    from 16:33 at the latest, before this branch. T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** 474 function stubs and 46 data placeholders in each version (`gen_stubs`, each build above), from 512
  and 46 at the last entry: upstream's code merged by `334077ba7`. A continued game without `--launch` still stops on
  `FIELDMAP_PROC_FUNCS`.
- **64-bit warnings:** none of the pointer and integer size kinds among the 264 warnings, in each version.
- **Work done:**
  - **Why Black 2's route went astray in White 2.** Traced on White 2's oracle every 5 frames (the player's
    `PlayerState`, the event variables and flags, the party, the event slot; White 2's heap addresses are Black 2's
    plus 0x40): Mom's and Hugh's scenes run as in Black 2 within a few counts, then, going up x = 37, the townsperson
    who wanders around (36, 747) (`aspertia_city.json`'s npc 11, movement 3) steps in front of the player, who stands
    from count 8124 to 8169; every timed walk after it ends short, and the player ends in front of the Trainers'
    School.
  - **White 2's starter route** (`port/tests/oracle/starter_save_w2_us.pbwinput`): Black 2's to (41, 751), then one
    tile west and up x = 40, out of the walker's reach and eight tiles shorter, along z = 741 to x = 54 and on as
    Black 2's, its presses from the stairs on 124 frames sooner. The oracle's screens show the story as Black 2's:
    Hugh's comment, Bianca's talks, Tepig, the Pokédex, and the save's question with Aspertia City, badges 0, Pokédex
    1, time 0:04. The save, decoded: Tepig, level 5, 135 experience, 22/22 HP, its checksum valid; event flags 0x961
    and 0x962; all 431 event variables and 3,064 saved flags as in Black 2's save (the route sets 0x4030 = 1,
    0x4043 = 255, 0x40a0 = 1, 0x40a1 = 4 and 0x40a4 = 1; 0x4165 = 1 is in `first_save` already); both copies the same;
    2,080 bytes from White 2's `first_save`. `SAVES` keeps its SHA-1.
  - **White 2's `party_menus`**: Black 2's input file and counts, measured again on White 2's oracle from the new save
    (`launch_party_menus_w2_us.*`): each app's first frame and the black after it come at Black 2's counts, and the
    White 2 port takes Black 2's passes to each app's first frame (`--launch APP@4044`: 4046 for the party list and
    the bag, 4048 for the Pokédex and the card), so `LAUNCHES` gives White 2 Black 2's `--launch` and spans. The
    table's rows are Black 2's labels at Black 2's counts; the field menu's, the party list's, the summary's and the
    card's bottom screens are White 2's own. The summary is left out for Black 2's reason, checked in White 2: the DS
    spends 38 VBlanks more than passes loading it (30 on count 4530), and the port's summary at count 4700 is the
    oracle's but for its background, which a shift of 9 pixels lines up best. No port bug turned up.
  - **The launcher's two smoke tests** from the starter save now run in both versions (`oracle.cmake`, with White 2's
    copies of their input files).
  - **Documents:** the oracle README's Saves (White 2's starter save, why its route differs), the comparison
    sequences (continue_starter in both versions, its White 2 row) and the launch checks (the table's White 2 row);
    MODDING.md's save commands; the tests guide; ARCHITECTURE.md's feature map row, the landing page and the README
    (the party list and the Pokédex match melonDS in both versions); a lesson on routes and wandering townspeople.
- **Next:** the apps no route reaches yet (the town map, the Xtransceiver, the medal box, the options); White 2's
  Route 19 save, for the battles; a save with an adventure made by the port, once overlay 36 lets it into the field;
  for the sound, a comparison sequence where a player plays on the game's count.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: the macOS distribution build after review: macOS 15 on CI, SDL3 and minicoro under the availability check

- **Upstream:** `993e6c49b`, as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `6c0aed21f`, on the branch `port-macos-dist` from `main` at `2da26e048`: `97547e7c7`, `5c7ade588`,
  `c29bdb182` and `27544db31` (the build as the entry below has it, its second to fourth commits rewritten before
  publishing, so that the glossary rows and the feature map's `check_app.sh` come with the commits that bring them),
  then this commit, the reviews' fixes and this entry; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) on this commit.
  Outside `port/`, only the landing page and the CI workflow changed; no game change.
- **Host build:** macOS 26.5.1 on an M2, Xcode 26.6 (SDK 26.5, Apple clang 21.0.0), Black 2, RelWithDebInfo, Ninja,
  the entry below's two build directories built again:
  - `build/port`, the development build: `hw.c`, `mco_impl.c` (now without `-w`) and the links, with no warning; the
    decomp's 263 stand, as in the entry below.
  - `build/port-dist`, the distribution build: SDL3's 251 compiles, C and Objective-C, built again for both
    slices under `-Werror=unguarded-availability`, and `mco_impl.c` with `minicoro.h` as a header of its own: no
    warning, no error, no linker warning; 119 sections on arm64 and 117 on x86_64, as predicted.
  - Not pushed, so no CI run: the `macos-distribution` job, now on `macos-26`, `macos-26-intel`, `macos-15` and
    `macos-15-intel`, hasn't run yet.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 with the oracle and its saves in each build, `ctest -j4`, the two builds at once, the machine at a
    load of 60 to 260 from the other tracks:
    - `build/port`: 267 of 269 pass (2,732 s); `app_universal_slices` skips (one slice), and `asm_oracle` hit ctest's
      900 s limit in its first file (`test_fx.py`, 746 s alone); run again by itself, as ctest runs it, all 4 of its
      files pass (424 s).
    - `build/port-dist` as its arm64 slices: 268 of 269 pass (2,638 s), `app_slices`, `app_universal_slices` and the 8
      `overlay_sections_image_*` among them; `asm_oracle` hit the same limit, and alone passes its 4 files (428 s),
      its libraries and stubs now universal like the build's objects.
    - `build/port-dist` as its x86_64 slices under Rosetta, with the new
      `port/tools/ctest_one_slice.sh --build-dir build/port-dist --arch x86_64 --exclude '^asm_oracle$' -- -j4`
      (43 executables thinned, then put back with their times; nothing relinked): all 259 tests it runs pass
      (1,298 s), among them the 21 `oracle_*` against melonDS, the 4 `launch_*` and 5 `replay_*`, the four
      `gx_backend_*`, `os_guard_huge_frame` and the 12 `*_sanitized`; `app_universal_slices` skips (one slice). Left
      out: `app_slices` and the 8 `overlay_sections_image_*`, which a thinned tree fails by design, and `asm_oracle`,
      which checks the arm64 slice its Python runs and ran above.
  - The scripts: `test_make_app.sh` passes 78 checks (69 before, and `check_app.sh` on a universal app: each slice's
    own ad hoc signature, `--minimum 11` and `11.0` alike, `--minimum 12.0` failing three ways, an app made with
    `--minimum 11`); `test_package_macos.sh` passes as before.
  - The apps: `package_macos.sh --distribution`, with a stand-in `hdiutil` on `PATH` that fails the first
    `create` ("Resource busy"), tried again and made `Frigate Black 2 0.0.2728.dmg` (13 MB) in 21 s, then checked it;
    `check_app.sh --run-every-slice` on the app prints each slice's CodeDirectory (x86_64 size 117,918, arm64 27,998,
    both `flags=0x2(adhoc)`, `Signature=adhoc`), and both slices start; `--minimum 11` passes the app and the program
    alike, where the review saw two failures.
  - The checks by hand: a build directory whose cache has the distribution mark stops when configured without the
    option, with the message; `run_all.py` skips (77) a build for x86_64 alone and goes on for a universal one; the
    two SDL3 compiles and the `minicoro.h` copy that call macOS 26's `posix_spawn_file_actions_addchdir` fail, as
    above.
  - A trial merge of this branch (`git merge-tree`) with `port-windows` conflicts in 14 files, as the review found,
    and with `port-sanitizers`, which has moved since, in 8: the documents' indexes and rows, STATUS.md, lessons.md,
    the CI workflow and `port/CMakeLists.txt`, each a matter of keeping both sides. `asm_oracle/build.py`'s change is
    placed so that it merges cleanly with the sanitizer track's.
  - Every relative link of the Markdown files changed resolves, anchors included (checked by script); clang-format
    23.1.3 is clean on `hw.c` and `test_gx.c`; the shell scripts pass `sh -n` and `bash -n`; `port-ci.yml` parses.
  - The tests and the checks left `~/Library` alone: `~/Library/Logs/Frigate/frigate-black2.log` kept its 14:40 time.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** as the entry below: 512 function stubs and 46 data placeholders in Black 2.
- **64-bit warnings:** not measured: no game or `lib/` code changed.
- **Work done:** the two reviews of the branch, each finding checked; all but one fixed here, and that one (the
  Windows track's) settled as far as this branch can.
  - **macOS 15 on CI** (review A, major). The entry below said no older runner but the deprecated `macos-14` existed;
    runner-images' README of 2026-10-18 lists `macos-15` (Apple Silicon) and `macos-15-intel`, neither deprecated,
    whose macOS 15.7.9 images have Xcode 26.0.1 to 26.3 beside their default 16.4, and whose arm64 template installs
    Rosetta as the macOS 26 one does. The job is now set up for all four runners, the macOS 15 ones with their newest
    Xcode 26 (`DEVELOPER_DIR`, found by the job), so that the app built for 11.0 runs on a macOS older than its SDK's.
    `macos-14` stays out: deprecated, and Xcode 26 needs macOS 15.6. The entry below, ADR 0015 and the README say so.
  - **SDL3 under the availability check** (A): `SDL3-static` gets `-Werror=unguarded-availability` too when it is
    built along with a deployment target, so a pinned release that calls a newer API unguarded fails the build. Its
    251 compiles, C and Objective-C, both slices each, pass; one C and one Objective-C compile command of SDL3 with a
    header calling macOS 26's `posix_spawn_file_actions_addchdir` added fail. This replaces the hand check the entry
    below describes (review B's request for a script for it).
  - **minicoro under the availability check** (A). Its `-w` did cancel the check, but so does more than that:
    minicoro's code is all in `minicoro.h`, a system header through `port_hal`'s `SYSTEM` include (`-isystem`), where
    clang reports no warning, `-Werror=` ones included, whatever the flags. So the review's fix,
    `-Wno-everything -Werror=unguarded-availability`, can't hold for `mco_impl.c`. Corrected at the merge into
    `port-integrate`, probed again with Apple clang 21.0.0 (`/usr/bin/cc`, SDK 26.5): in a file of its own, that order
    does stop a macOS 26 call at a target of 11.0, and `-w` anywhere or a `-Wno-everything` after the `-Werror=`
    cancels it; through `-isystem`, every order lets it through. (This entry first said that `-Wno-everything`
    silences the check in either order, which only the system header made it look.) `mco_impl.c` now drops `-w` and,
    with a deployment target, takes `--no-system-header-prefix=minicoro.h`: it gives no warning under
    `-Wall -Wextra`, and a copy of `minicoro.h` with that call appended fails to compile, where both the old options
    and the system header alone let it through. That fix stands. lessons.md has it.
  - **`check_app.sh`** (A): it reads each slice's signature (`codesign -dv --arch`) for the ad hoc and hardened runtime
    checks, and compares the program's minos and `LSMinimumSystemVersion` with `--minimum` part by part, as
    `make_app.sh` does, so `--minimum 11` passes a build for 11.0; `package_macos.sh` compares the same way.
    `test_make_app.sh` checks both (78 checks).
  - **Rosetta on CI** (A): an Apple Silicon job checks `arch -x86_64 /usr/bin/true` before it builds and stops with a
    message naming the image when Rosetta is missing; installing it would accept Apple's licence, so the job doesn't.
  - **`asm_oracle` and the slices** (A): `build.py` gives the stubs and the link the build's `-arch`,
    `-mmacosx-version-min` and `-isysroot`, so a library has the objects' slices, and `run_all.py` skips (77) a build
    with no slice for its Python's architecture, such as an x86_64 build on Apple Silicon, saying so, where it failed
    with a missing symbol. The tests check the slice their Python runs, as its README now says.
  - **ADR 0015** (review B, major): the Windows track's branch, `port-windows`, already pushed, took 0013 for the
    Windows build, and so did the sanitizer track's, `port-sanitizers`, for the sanitized build (accepted there), so
    one of the two will be 0014: this ADR is 0015 everywhere, a number neither takes, in whatever order the three
    merge. The Windows track also folds the 4 KiB alignment into one macro, `PORT_HOST_PAGE_ALIGNMENT` in
    `port/hal/host/host.h`, a file this branch doesn't have, so the two can't share it until one merges: whichever
    lands second takes that macro in `hw.c`, `gen_stubs.py` and `test_gx.c`, with the x86_64 Mach-O case in its
    condition (`#if defined(_WIN32) || (defined(__APPLE__) && defined(__x86_64__))` for 4096), and drops this
    branch's own `#if`s.
  - **What the documents claim** (B): the README, the landing page, the glossary, the feature map, the workflow's
    comments and ADR 0015 say that CI's distribution job is set up and hasn't run, that it runs the tests that don't
    need the ROM, and that nothing below macOS 26 has run the app.
  - **A distribution directory configured without its option** (B) stops with a message: `PORT_MACOS_DISTRIBUTION`
    leaves an internal mark in the cache, since its architectures, deployment target and SDL3 stay there.
  - **`hdiutil create`** (B) gets three tries, 5 s apart; a run with a stand-in `hdiutil` that fails the first create
    made the image on the second.
  - **The builder's paths** (B): ADR 0015's consequences and the README's "Installing on another Mac" say that the app
    holds the checkout's path (the ROM's default) and, in each slice's debug map, the objects', which name the home
    folder, and that ADR 0010's neutral path and stripping come before any public build. Stripping wasn't done: it
    removes neither the ROM's default path nor the decision, which stays the user's.
  - **STATUS** (B): the launcher entry's `PORT_SHA` is `bbfab4c72`, filled in `27544db31`; this branch's first entry
    corrects its runner claims and names the rewritten commits.
  - **Commit order** (B): the feature map's `check_app.sh` and the new tests move into `c29bdb182` with the glossary's
    Rosetta, and the universal program, slice, deployment target and minos rows into `5c7ade588`, the commits that
    bring them; the branch's tree at the fourth commit is unchanged but for STATUS.md.
  - **Repeatable checks** (B): `port/tools/ctest_one_slice.sh --build-dir DIR --arch x86_64` runs a universal build's
    suite as one slice, thinning its executables for the run and putting them back with their times, leaving out the
    tests that check the slices; the SDL3 check is now the build's own (above).
  - **The README's small defects** (B): the 206-column line rewrapped, `port-ci.yml`'s line names the distribution
    build, and the first configure downloads SDL3's pinned release (the network) and configures it, the build compiles
    it, in the README and `package_macos.sh`.
- **Next:** CI's first run of the distribution job, on four runners; then the user's decisions as in the entry below
  (downloads, a ROM picker), and ADR 0015's acceptance. Otherwise as the entry below.
- **Blockers:** as the entry below. The order in which this branch, `port-windows` and `port-sanitizers` merge is the
  user's or the integrator's call; the two others both number their ADR 0013, so one of them renumbers.

## 2026-10-18: the macOS distribution build, one universal app for macOS 11 (ADR 0015, proposed)

- **Upstream:** `993e6c49b`, as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `27544db31`, on the branch `port-macos-dist` from `main` at `2da26e048`: `97547e7c7` (ADR 0015, proposed),
  `5c7ade588` (the tools read every slice; x86_64's 4 KiB alignment), `c29bdb182` (`make_app.sh` reads every slice and
  takes the build's architectures and minimum; `check_app.sh`), then this commit, the distribution build itself and
  this entry; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) on the final tree.
  Outside `port/` only the landing page, `.github/README.md`, and the CI workflow, `.github/workflows/port-ci.yml`,
  changed; no game change.
- **Host build:** macOS 26.5.1 on an M2, Xcode 26.6 (SDK 26.5, Apple clang 21.0.0), CMake 4.4.4, Black 2,
  RelWithDebInfo, Ninja, each from scratch in this worktree:
  - `build/port`, the development build as before: arm64, built for the SDK's macOS (26.0), Homebrew's SDL3 3.4.18.
    263 warnings, all the decomp's, as in the last entries; none in `port/`.
  - `build/port-dist`, `-DPORT_MACOS_DISTRIBUTION=ON`: universal, macOS 11.0, SDL3 3.4.18 built along, statically and
    without libusb. The same 263 warnings in each slice (526), none of them an availability one under
    `-Werror=unguarded-availability`, and no linker warning: the 20 "reducing alignment" ones of the research are gone.
    700 MB, against `build/port`'s 230 MB.
  - `build/port-dist-w2`, White 2's, made from nothing by `port/tools/package_macos.sh --distribution --version w2_us`
    in 4 min 25 s (configure, SDL3, both slices, the app and its 13 MB image), checked as below, then deleted.
  - **What the load commands say** (`port/app/macos/check_app.sh` prints them, slice by slice):
    - the development app, `build/port/Frigate Black 2.app`: the program arm64 only, platform MACOS, minos 26.0, SDK
      26.5; `Contents/Frameworks/libSDL3.0.dylib`, Homebrew's, arm64 only, minos 26.0; `LSMinimumSystemVersion` 26.0;
      signed ad hoc. That app opens only on Apple Silicon with macOS 26.
    - the distribution apps, `Frigate Black 2.app` and `Frigate White 2.app`: one binary each, the program, universal,
      x86_64 and arm64, each slice platform MACOS, minos 11.0, SDK 26.5 (`LC_BUILD_VERSION`, tool LD 1267.0); no
      `Contents/Frameworks`; it loads only libSystem, libobjc and the frameworks SDL3 needs, all macOS's own
      (UniformTypeIdentifiers and CoreHaptics weakly), and SDL3 holds no `dlopen` of libusb any more; an Info.plist
      `LSMinimumSystemVersion` of 11.0; signed ad hoc (`flags=0x2(adhoc)`), no hardened runtime, and
      `codesign --verify --deep --strict` passes; `--help` starts as both slices, x86_64 under Rosetta. The program is
      29 MB (14 MB arm64, 15 MB x86_64), and Black 2's disk image 13 MB.
  - Not pushed, so no CI run: the new `macos-distribution` job, on `macos-26` and `macos-26-intel`, hasn't run yet.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, with the oracle (`pbw-oracle` and the oracle-made saves copied from the verify tree into each build)
    and the ROM build's link map, the machine at a load of 50 to 85 from two other tracks:
    - `build/port`: 268 of 269 pass (2,816 s); `app_universal_slices` skips, the program having one slice.
    - `build/port-dist` as its arm64 slices (ctest runs each program as this Mac's): all 267 tests of the time passed
      (1,923 s) before SDL3's libusb change and the two new tests; on the final build all 269 pass (1,343 s),
      `app_slices` and `app_universal_slices` among them.
    - `build/port-dist` with its 44 executables thinned to x86_64 (`lipo -thin x86_64`, the universal ones kept aside
      and put back with their times after, so that nothing relinked), so that ctest's children run under Rosetta: 260 of
      269 pass (2,930 s), among them the 21 `oracle_*` tests against melonDS, the 4 `launch_*` and 5 `replay_*`, the
      four `gx_backend_*`, `os_guard_huge_frame` (the stack probes on x86-64) and the 12 `*_sanitized` tests. The 9
      others fail as they must on a thinned tree, which is not the build's: `app_slices` (one slice, the build is for
      two) and the 8 `overlay_sections_image_*` (one slice where the check predicted two); `app_universal_slices`
      skips. `asm_oracle` loads its library into the arm64 Python, so it checks arm64 code only.
    - Both slices of Black 2's program, 900 headless frames from power-on with `--fixed-time`, with each of the four
      VRAM backends: the eight `frames.tsv` are the same byte for byte, 153 distinct top screens. `app_universal_slices`
      does it with the default backend in 14 s, and passes for White 2's too (153 distinct top screens).
  - The scripts: `test_make_app.sh`, 69 checks (a universal program and library made for 11.0: the app of both, started
    as both; a library for 13.0 refused with `--minimum 11.0` and taken as the app's minimum without it; a library and a
    program without a slice refused; `--archs` as a CMake list); `test_package_macos.sh`, 22.
  - `package_macos.sh --distribution` on the configured `build/port-dist`: 18 s, `Frigate Black 2 0.0.2727.dmg`,
    `hdiutil verify`, the signature, the slices and the minimum checked in the mounted image.
  - SDL3's own sources, which the build's `-Werror=unguarded-availability` doesn't reach: each of SDL3's 249 compile
    commands of `build/port-dist` run again per architecture with `-fsyntax-only -Wno-everything
    -Wunguarded-availability -Wunguarded-availability-new` (its precompiled headers kept): 498 compiles, none failing,
    no availability warning. The same flags warn on a test file that calls macOS 12's `isLowPowerModeEnabled` at 11.0.
  - Every relative link of the Markdown files changed resolves, anchors included (checked by script); clang-format
    23.1.3 is clean on the two C files changed; `port-ci.yml` parses as YAML.
  - The tests left `~/Library` alone, but a first version of `check_app.sh` didn't: run on an app, it sent `--help`'s
    output to `/dev/null`, so the program, finding itself in an app with nowhere to write, wrote its log to
    `~/Library/Logs/Frigate/frigate-black2.log` (`port/app/message_log.c`), overwriting the one there. It now keeps the
    output, and a run leaves that file's time unchanged.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 512 function stubs and 46 data placeholders in Black 2 (`gen_stubs`, both builds; the slices define
  the same symbols), fewer than the 624 and 48 of the last entry's count, since upstream's merge `1f0c184f6` brought C.
- **64-bit warnings:** not measured: no game or `lib/` code changed (the count of every kind is 263, as above).
- **Work done:** the app for other Macs, from the research's plan: why it needed macOS 26 (the program built for the
  SDK's macOS, Homebrew's SDL3 arm64 only and built for 26), the choice of one universal app for macOS 11, its tools
  and its checks. ADR 0015, proposed.
  - **The distribution build** (`port/CMakeLists.txt`): `-DPORT_MACOS_DISTRIBUTION=ON` sets `CMAKE_OSX_ARCHITECTURES`
    `arm64;x86_64` and `CMAKE_OSX_DEPLOYMENT_TARGET` 11.0 before `project()` when the cache has neither, skips the
    installed SDL3 and builds the pinned release statically without libusb (SDL3's configure had found Homebrew's
    libusb and built in a `dlopen` of it), and every build with a deployment target fails on an API newer than it.
    `port/tools/package_macos.sh --distribution [--version w2_us]` configures `build/port-dist` (`-w2`), builds and
    packages in one command, refuses a directory configured otherwise or for another game, and checks the image's
    slices and minimum.
  - **The tools follow the slices** (`5c7ade588`): `gen_overlay_sections.py` predicts and checks each slice, with the
    link rules of its architecture and deployment target, each measured with a small program for both architectures
    from macOS 10.13 to 26 (lazy binding below 12 on arm64 and 13 on x86_64, `__DATA_CONST` from 10.15, SDL3's
    Objective-C, and on arm64 `__objc_stubs` and `__objc_methlist`), and its image test compares by name: arm64
    predicts and has 119 sections, x86_64 117, 87 of them the overlays' (the development build's program, 100).
    `check_struct_layout.py` reads a universal object slice by slice. `hw.c` and `gen_stubs.py` align to 4 KiB in an
    x86_64 Mach-O program; `test_gx.c` checks the views against the host's page.
  - **The app's scripts** (`c29bdb182`): `make_app.sh` reads every slice, takes `--archs` and `--minimum` from the
    build and refuses a binary short of a slice or built for a newer macOS, and starts the program as each slice the Mac
    runs; `check_app.sh` reports and checks an app or a program; `app_slices` and `app_universal_slices` test them.
  - **CI**: a `macos-distribution` job on `macos-26` and `macos-26-intel` (`brew install ninja` only): configure,
    build, ctest (natively x86_64 on Intel), `package_macos.sh --distribution`, and `check_app.sh` on the app, every
    slice started on Apple Silicon. The runner labels are runner-images' README's of 2026-10-18; `macos-14`, the oldest,
    is deprecated, so it isn't used (and `macos-15` and `macos-15-intel`, which aren't, came only after review: the
    entry above).
  - **Documents**: port/README.md's macOS app section (who each app is for, the distribution build in one command,
    what has been tested, installing on another Mac, Gatekeeper on macOS 15 and later and on 11 to 14),
    ARCHITECTURE.md's feature map, the app, cmake, tests and tools guides, `Info.plist.in`'s comment, the glossary
    (universal program and slice, Rosetta, deployment target and minos, distribution build, ad-hoc signature and
    Gatekeeper), the landing page, ADR 0010's note, and lessons.md.
- **Next:** for the app, CI's first run of the distribution job (on Intel natively), then the user's decisions: whether
  Frigate offers downloads at all (port/README.md and `package_macos.sh` say the compiled game code is never uploaded),
  and, before any, a first-launch ROM picker, since a downloaded app opened from the Finder finds no ROM. Otherwise as
  the last entry: White 2's starter route, the apps no route reaches yet, a save with an adventure made by the port once
  overlay 36 lets it into the field.
- **Blockers:** as the last entry: overlay 36 (issue #83, another contributor's active claim) gates phase 3 and the
  launcher's retirement; `NNSi_objSizeWTbl` stops the Pokédex; overlays 11 and 182 only matter from phase 7. ADR 0015
  waits for the user's acceptance; macOS 11 to 15 can't be tried on this Mac (no virtual machine), and CI's macOS 15
  runners, which aren't deprecated, came only after review (the entry above).

## 2026-10-18: the Windows track's merge after review: its warnings traced and its steps counted

- **Upstream:** `9b486e486`, as merged into the port's `main` by `334077ba7`; no sync in this task.
- **Port:** `6489c0795`, on the branch `port-integrate` after `d02026b0b`: this entry, which also corrects the last;
  pushed to `port-windows` (a fast-forward from `d02026b0b`) for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `d02026b0b`
  with this entry's documents. No game file changed.
- **Host build:**
  - arm64 macOS, RelWithDebInfo, `build/port-n` (Black 2) and `build/port-w2` (White 2), both Ninja, built again at
    `d02026b0b` with this entry's documents: CMake ran again and nothing else had to (no source changed since the last
    entry's builds), so their 264 warnings stand, none in `port/`. The review's fresh builds of `d02026b0b` (since
    deleted) had 1,797 steps and 264 warnings each, none in `port/`. The oracle is `build/oracle`'s `pbw-oracle` in
    both builds' `oracle/`, and the saves their `oracle-saves`.
  - CI: run [39175404511](https://github.com/CallanCampos/Frigate/actions/runs/39175404511), at `d02026b0b`, green on
    all three hosts (263 warnings on Linux and Windows, 264 on macOS, none in `port/`); this entry changes only
    documents, and its own run on `port-windows` is the next entry's.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 at `d02026b0b` with this entry's documents, the two builds at once, `ctest -j6` (Black 2) and
    `ctest -j4` (White 2), with the oracle in both, under a load average of 20 to 85 from the other tracks' runs:
    Black 2, 280 of 281 pass (2,075 s), none skipped, and `asm_oracle` timed out at its 900 s and passes on its own
    (138 s), so all 281 pass; `oracle_launch_party_menus` (the Pokédex among its apps) passes in 1,175 s and
    `oracle_continue_starter` in 1,434 s. White 2, 275 of 277 pass (2,182 s), `asm_oracle` among them (459 s), and
    `oracle_continue_starter` and `oracle_launch_party_menus` skip as before (no SHA-1 of White 2's `starter_save`).
  - CI, run 39175404511: macOS 236 pass and 45 skip of 281; Linux 219 pass and 47 skip of 266; Windows 209 pass and
    57 skip of 266, the same tests and skips as the Windows track's last run (39162189835), as the review found.
  - Every run passed `--save-dir`: the tests left `~/Library` alone (nothing in its `Frigate` folders is newer than
    this task's start, and the folders were last written at 16:31). This entry changes no C, and the relative links
    of STATUS.md and lessons.md resolve, anchors included (checked by script). T5's nightly runs and T7 not yet; on
    Windows, nothing with a ROM has run yet (not yet).
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 474 function stubs and 46 data placeholders in each version (each build's `gen/stubs.c`), as in the
  last entry.
- **64-bit warnings:** not measured: no game or `lib/` code changed. The last entry's field gave none of those kinds
  among the 264 warnings, which the game's target leaves off; it now says not measured too, with why.
- **Work done:** the review of the Windows track's merge (`d294567e3` to `d02026b0b`): clean, with three nits, each
  checked against the logs and fixed in this entry, but one.
  - **The 264th warning, traced:** the last entry called it "not traced". It is `lib/dwc/src/gp/gpiInfo.c:138:16`'s
    `-Wtautological-pointer-compare`, in upstream's GameSpy `gpiInfo.c` (`1ec257379`, merged by `4fee9b918`): the
    Pokédex entry's 263 came from its branch, from `2da26e048`, which predates it, and `main`'s own macOS job at
    `77263e609` (run 39171576539) has the same 264, file and line for file and line, as the merge's local builds and
    its macOS job have. On Linux the merge took one away: `main`'s job had 264 with `port/app/run_dir.c:8`'s
    `_POSIX_C_SOURCE` redefinition, which the host seam's `run_dir.c` no longer has, and Windows' 263 are Linux's.
    macOS's one more than Linux is `src/ov011/dwc_rap.c:320`'s `sprintf`, which only macOS's SDK marks deprecated.
    The last entry's Host build field says so.
  - **The steps, counted:** the last entry said its builds ran "every step again (1,701 each)". A fresh build has
    1,797 steps, as the review's builds and CI's macOS job had; 1,701 of them ran in the merge's. The field says so.
  - **The last entry's 64-bit warnings** said none of the pointer and integer size kinds were among the 264, which
    measures nothing while the game's target leaves them off (`port/CMakeLists.txt`; `port/tools/compile-probe` counts
    them); it now says not measured, as the launcher's review had its entry say. Its Port field names `d02026b0b`.
  - **Not changed: the merge's message.** `d294567e3` names neither of its two conflicted files, as `859e4c1cf`'s and
    `77263e609`'s messages did. They were `port/docs/STATUS.md` and `port/docs/lessons.md`; the last entry's Work done
    names them, and so does this entry's commit message. Rewording the merge would rewrite `port-windows`, which holds
    it, and so need a force-push.
  - **ADR 0013's acceptance:** the review could confirm that the integration task asked for it, not that the user
    approved. It was accepted by the developer, as ADR 0012 was at publishing (`bbfab4c72`), and no record here holds
    the user's own word on the toolchain, so the user is told; reverting `f86bca826` puts the ADR back to proposed.
  - **lessons.md:** a moved warning count is traced against the base's own CI run, file and line, and Ninja's
    `[n/total]` counts what runs, not the build.
- **Next:** as in the last entry: Windows' stage 4 (a run with a ROM on a Windows machine, sanitizers on CLANG64, a
  build without a console, a package, the memory-mapped ROM, ARM64 and VRAM aliasing); White 2's starter route; the
  apps no route reaches yet; a save with an adventure made by the port; a sound comparison sequence.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: the Windows track merged under today's names, and ADR 0013 accepted

- **Upstream:** `9b486e486`, as merged into the port's `main` by `334077ba7`; no sync in this task.
- **Port:** `d02026b0b`, on the branch `port-integrate` from `main` at `77263e609`: `d294567e3` (the merge of
  `port-windows` at `a49a40626`), `f86bca826` (ADR 0013 accepted), `d6fe067d8` (the Windows track under today's
  names), then this entry; pushed to `port-windows` (a fast-forward from `a49a40626`) for CI, not to `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `d294567e3` and
  `d6fe067d8`. No game file changed: the Windows track changes only `port/` and Frigate's own `port-ci.yml`.
- **Host build:**
  - arm64 macOS, RelWithDebInfo, `build/port-n` (Black 2) and `build/port-w2` (White 2), both Ninja, rebuilt from
    `d294567e3`: 1,701 of a fresh build's 1,797 steps ran again, since the merge brought the Windows track's flags; no
    errors, 264 warnings in each, none in `port/`. That is one more than the Pokédex entry's 263:
    `lib/dwc/src/gp/gpiInfo.c:138:16`'s `-Wtautological-pointer-compare`, in upstream's GameSpy `gpiInfo.c`
    (`1ec257379`, merged by `4fee9b918`), which the Pokédex's branch, from `2da26e048`, predates. `main`'s own macOS
    job at `77263e609` (run [39171576539](https://github.com/CallanCampos/Frigate/actions/runs/39171576539)) has the
    same 264, file and line for file and line, so the merge added none. The game's target turns the pointer and
    integer casts' warnings off and leaves `-Wshorten-64-to-32` off (`port/CMakeLists.txt`; `port/tools/compile-probe`
    counts them), so the 264 are other kinds. The oracle is `build/oracle`'s `pbw-oracle` in both builds' `oracle/`,
    and the saves their `oracle-saves`.
  - CI, `port-ci.yml` dispatched on `port-windows` at `d6fe067d8`: run
    [39173987503](https://github.com/CallanCampos/Frigate/actions/runs/39173987503), green on all three hosts. Windows
    built with MSYS2's clang 22.1.8, CMake 4.4.3, Python 3.14.7 and SDL3 3.4.16; 263 warnings there and on Linux, 264
    on macOS, none in `port/` on any. Windows' 263 are Linux's; macOS's one more is `src/ov011/dwc_rap.c:320`'s
    `sprintf`, which only macOS's SDK marks deprecated. `main`'s Linux job at `77263e609` had 264: the 264th was
    `port/app/run_dir.c:8`'s `_POSIX_C_SOURCE` redefinition, a `port/` warning that the host seam's `run_dir.c` no
    longer has.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 on `d294567e3`, the two builds at once, `ctest -j6` (Black 2) and `ctest -j4` (White 2), with the
    oracle in both, under a load average of 40 to 180 from the other tracks' runs: Black 2, all 281 pass (2,173 s),
    none skipped, `oracle_launch_party_menus` (the Pokédex among its apps) in 868 s; White 2, 274 of 277 pass (1,937
    s), `oracle_continue_starter` and `oracle_launch_party_menus` skip as before (no SHA-1 of White 2's
    `starter_save`), and `asm_oracle` timed out at its 900 s and passes on its own (270 s), so all that run pass. The
    host seam's tests (`host_*`, the fiber guards, `app_symbols`, `integrate_overlay_sections`) pass in both, and
    `fx_libfx`, which checks `VEC_Normalize` through the translated cp.h, too.
  - CI, run 39173987503: macOS 236 pass and 45 skip of 281; Linux 219 pass and 47 skip of 266; Windows 209 pass and
    57 skip of 266, the same counts as the Windows track's last run, so main's work since `2da26e048` added no test
    and lost none on any host; `fx_libfx` passes on all three.
  - Every run passed `--save-dir`: the tests left `~/Library` alone (its `Frigate` folders were last written at
    16:33, by White 2's app opened from the Finder, before this task). Every relative link of the Markdown files under
    `port/` and `.github/README.md` resolves, anchors included (checked by script). clang-format 23.1.3 is clean on
    the merge's C files; this task's own commits change no C. On Windows nothing with a ROM has run yet (not yet);
    T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 474 function stubs and 46 data placeholders in each version and on all three CI hosts, from the
  Pokédex entry's 512 and 46: upstream's C merged by `4fee9b918` and `334077ba7` (among it libfx and libmath, NitroWiFi
  and DWC), which the last entries didn't count. The host's C library provides 34 of the ROM's symbols on macOS, 35 on
  Linux and 36 on Windows.
- **64-bit warnings:** not measured: no game or `lib/` code changed in this task. The host build's 264 warnings (Host
  build above) are of other kinds, since the game's target leaves those off.
- **Work done:**
  - **The merge** (`d294567e3`, `git merge --no-ff origin/port-windows`): two conflicts, both documents, both kept
    whole. STATUS.md and lessons.md take the Windows track's entries above `main`'s, newest first, and the Windows
    track's last entry names its commit, `a49a40626`, where it had `PORT_SHA`. Everything else merged clean and keeps
    both sides: `gen_host_tree.py` routes cp.h's address constants of the registers that act to their functions
    (`main`) and links the include mirror with the system's separator, or copies it where Windows refuses links
    (`port-windows`); `CMakeLists.txt` has the repository's new `HOMEPAGE_URL` and Windows' manifest and stack;
    `main.c` has the seam's watchdog and `--launch`'s text without the Pokédex's trap; the README, ARCHITECTURE.md,
    PLAN.md and the tests guide have the rename's and the Pokédex's text and the Windows track's.
  - **ADR 0013 accepted** (`f86bca826`): by the port's developer, as every earlier ADR was, since the toolchain, MSYS2's
    CLANG64 over the research's clang-cl, is the developer's call. Its status is accepted, with a note saying where;
    PLAN.md §4.4, the glossary's MSYS2 row, the README's Windows build, the feature map's platform row and ADR 0005's
    note no longer call it proposed; the ADR index says accepted.
  - **Today's names** (`d6fe067d8`): the README's Windows build clones `CallanCampos/Frigate` into `Frigate`, the one
    place the track named the old repository (no workflow or script did). MODDING.md says an experiment builds on
    macOS, Linux or Windows, an abort's and a crash's status and message on Windows, that the sanitizer build is
    macOS's and Linux's, which tests skip on Windows, and that the oracle runs where the ROM build does;
    ARCHITECTURE.md's layers end the platform layer in `hal/host`; the tools guide says what `gen_host_tree.py` and
    `gen_stubs.py` do on each host. The track's new files already opened with what they do and touch, and had their
    rows in the hal, app, cmake and tests guides, the directory map, the feature map and the glossary.
  - **gen_host_tree.py's routing on Windows:** nothing to do. It is header text: the translated cp.h names
    `((void *)port_hw_div_result())`, a C call that `port_host.h` declares the same on every host, and CI's Windows
    job passes `fx_libfx`.
- **Next:** Windows' stage 4, as in the Windows track's entries: a run of the game with a ROM on a Windows machine,
  sanitizer builds on CLANG64, a build without a console with its log under `%LOCALAPPDATA%\Frigate`, a package, the
  memory-mapped ROM, ARM64 and VRAM aliasing on Windows; a backtrace on a fiber on Linux arm64. As in the Pokédex
  entry: White 2's starter route, so that `party_menus` and `continue_starter` run in White 2; the apps no route
  reaches yet (the town map, the Xtransceiver, the medal box, the options); a save with an adventure made by the port,
  once overlay 36 lets it into the field; a sound comparison sequence played on the game's count.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: Windows in CI after review: crash reports on fibers and through the C library, UTF-8 names, atexit

- **Upstream:** `993e6c49b` (merge of PR #377), as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `a49a40626`, on the branch `port-windows` (origin's `main` still at `2da26e048`): `d173080f3` (the seam and
  its tests), `f6689ec6b` (the build: the stubs' lookup, `wcslen`, the manifest, the include copies), `176c67e70` (the
  Windows save's commit), `d19c6dcd5` (the Windows job and the documents), `29858a3a9` (a fiber's backtrace on Linux,
  the include links under MSYS2's Python), `29bb942da` (a test of a 0-second watchdog), then this entry; pushed to
  `port-windows` only, not merged into `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `4785b3ad8`
  before the work and at `29858a3a9` and `29bb942da`. No game file changed: outside `port/`, only Frigate's own
  `port-ci.yml`.
- **Host build:**
  - arm64 macOS, RelWithDebInfo, Black 2 (the worktree's `build/port`, Ninja): every decomp file again, since
    `-fno-builtin-wcslen` joined the decomp's flags; no warning in `port/`.
  - CI, `port-ci.yml` dispatched on `port-windows`: run 39158430166 (`d19c6dcd5`) had macOS green, Windows red (every
    decomp file's "types.h not found": the include mirror's links, made by MSYS2's Python with "/" in their targets) and
    Linux red (the three new fiber tests: glibc's backtrace faulted on minicoro's dummy return address); run 39159078044
    (`29858a3a9`) and 39160792215 (`29bb942da`) are green on all three. The Windows job now builds as the README says,
    with MSYS2's Python 3.14.7 and SDL3 3.4.16 (unpinned), clang 22.1.8; 262 warnings in the decomp, none in `port/`.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 on macOS, Black 2, `ctest -j4` with the oracle in `build/port/oracle`, at `29858a3a9`: all 280 pass
    (968 s, none skipped; `asm_oracle` in 448 s, `oracle_saves` reusing the saves of an earlier run). After each step
    before it, the host, OS guard, backup, launcher, run directory and symbol tests passed on their own, and at
    `29bb942da` those 46 tests with the GX backends', the overlay sections' and the PXI bus's, `host_watchdog_off` among
    them.
  - CI, run 39160792215: Windows 209 pass and 57 skip of 266 (the same skips as before); Linux 219 pass and 47 skip of
    266; macOS 236 pass and 45 skip of 281. Each host has six new tests: `host_trap_placeholder_fiber`,
    `host_crash_report_fiber`, `host_guard_fiber`, `host_illegal_instruction_fiber`, `host_watchdog_off` and
    `host_utf8_argument`, and `host_files` checks a folder whose name isn't ASCII.
  - On Windows nothing with a ROM has run yet (not yet); T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 512 function stubs and 46 data placeholders on all three hosts: Linux's 513th, `atexit`, which
  `backup.c` calls at every run's card initialization, is gone (glibc's `libc_nonshared.a`, read through the compiler
  driver). The host's C library provides 30 of the ROM's symbols on macOS, 31 on Linux, 32 on Windows: `wcslen` is no
  longer among them, since nothing calls it now.
- **64-bit warnings:** not measured: no game or `lib/` code changed (`-fno-builtin-wcslen` changes code, not warnings).
- **Work done:** the two reviews of the Windows work, finding by finding.
  - **Crash reports on Windows**: the vectored handler, installed before `main()`, takes a memory fault at any address
    the seam made inaccessible, whatever code touched it (the C library's `memset` on a trap placeholder ended the
    process unreported), and every fatal exception on a fiber, where the system never reaches the unhandled-exception
    filter; non-memory exceptions are reported as crashes, and a second fault ends the program with its own status.
    `check_exit.py --crash` counts every status from 0x80000000 (a guard page's 0x80000001).
  - **The seam's contract**: `port_host_unmap_file_copy` (Windows' views were never released, and their files never
    removed); a 0-second watchdog is off on Windows as on POSIX (`host_watchdog_off`); `port_host_write_error`, so that
    `test_os.c`'s fault handler writes as before the seam, async-signal-safe; host.h says what each half does. The
    one-line forwarders are gone, `port/app/symbols.c` with them.
  - **Linux**: `gen_stubs.py` reads what the compiler driver links, linker scripts' archives included, besides `ctypes`,
    and never stubs a function of standard C, so `atexit` is glibc's again; a fiber's first frame has a return address
    of 0, so a backtrace on a fiber (every trap's report from the game) no longer faults.
  - **All hosts**: `-fno-builtin-wcslen` keeps two u16 loops (`MBGameInfo_CountChars`, `Gt2_WStrLen`) from becoming
    calls of a 32-bit `wcslen` on macOS and Linux, and `gen_stubs.py` warns of any wide-character call; the stubs take
    their page alignment from `host.h`; `_GNU_SOURCE` comes from one place per build path.
  - **Windows**: every program carries a manifest that makes UTF-8 its code page (SDL's arguments are UTF-8, the C
    library's names were the ANSI code page's); the save is committed to the disk before its rename; the include
    mirror's copies are brought up to date by a configure that any header's change starts; CMake prefers the Python on
    PATH; the CI job uses MSYS2's own Python and shows the SDL3 package it got.
  - **Documents**: the README's Windows build (git, `git clone -c core.autocrlf=false`, `CC=clang`, the skips, no
    Preparing a checkout); the seam's real boundary in hal/README.md, host.h, the glossary and ADR 0013, with each
    module that keeps a host branch and why; ADR 0013 corrected (SDL3 3.4.16, unpinned) and citing ADR 0005, which gets
    a note; PLAN.md calls CLANG64 proposed; the feature map and the directory map; lessons.md. The last entry's facts
    are corrected (SDL3 3.4.16; the POSIX code moved with the same calls, not unchanged).
- **Next:** the user's acceptance of ADR 0013. Windows' stage 4: a run of the game with a ROM on a Windows machine,
  sanitizer builds on CLANG64, a build without a console with its log under `%LOCALAPPDATA%\Frigate`, a package, the
  memory-mapped ROM, ARM64 and VRAM aliasing on Windows. A backtrace on a fiber on Linux arm64, where minicoro keeps the
  dummy return address in its context rather than on the stack, is untried. actionlint wasn't run on the workflow (not
  installed here). As in the last entry: White 2's starter route; the apps no route reaches yet; a save with an
  adventure made by the port, once overlay 36 lets it into the field; a sound comparison sequence played on the game's
  count.
- **Blockers:** as in the last entry: overlay 36 (issue #83, another contributor's active claim) gates phase 3 and the
  launcher's retirement; `NNSi_objSizeWTbl` stops the Pokédex; overlays 11 and 182 only matter from phase 7.

## 2026-10-18: Windows in CI: Frigate builds with MSYS2's CLANG64 and passes every test that needs no ROM

- **Upstream:** `993e6c49b` (merge of PR #377), as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `4785b3ad8`, on the branch `port-windows` from `2da26e048` (origin's `main` had not moved at the end):
  `797c55628` and `f3fde9b43` (the Windows job), `6d913cb93` (the host seam and Windows' half), `14ffc553d` and
  `cd3932d32` (the tests' POSIX expectations), `24bd4a18e` (the host tests), `d5b9aa990` (the documents and ADR 0013),
  then this entry; pushed to `port-windows` only, not merged into `main`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `2da26e048`
  before the work and at `d5b9aa990`. No game file changed: outside `port/`, only Frigate's own
  `.github/workflows/port-ci.yml` and `.github/README.md`.
- **Host build:**
  - arm64 macOS, RelWithDebInfo, Black 2 (the worktree's `build/port`, Ninja): from scratch at `797c55628`, then at
    `d5b9aa990` (the port's changed files, with no warning; the decomp's C didn't change, so its warnings stand).
  - CI, `port-ci.yml` dispatched on `port-windows`: macOS 26 and Ubuntu 26.04 as before, and the new Windows job on
    `windows-2025` with MSYS2's CLANG64 (clang 22.1.8, lld, the UCRT, CMake 4.4.3, Ninja, SDL3 3.4.16, Python 3.12 from
    `actions/setup-python`): all three green on runs 39148942447 (`cd3932d32`), 39149612934 (`24bd4a18e`) and
    39150122118 (`d5b9aa990`). The Windows build compiles the decomp's 1,473 C files with 262 warnings, of the kinds
    macOS's has (`-Wswitch`, `-Wpointer-sign`, library redeclarations), none in `port/`; a run takes 3 to 5 minutes.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 on macOS, Black 2, `ctest -j4` with the oracle in `build/port/oracle`: at `797c55628` (the configure
    changes), 266 of 267 pass and `asm_oracle` timed out at its 900 s while the machine's load passed 100 (other tracks'
    builds; `oracle_saves` took 1,917 s); at `d5b9aa990`, all 275 pass (2,149 s, none skipped; `asm_oracle` in 600 s,
    `oracle_saves` reusing the saves of the first run). Between those, each stage's tests ran on their own (the OS, PXI,
    sound, backup, ROM, GX, run directory, symbols, launcher, integration and host tests, and `oracle_compare`'s 50
    cases), all passing.
  - CI, run 39149612934: Windows 203 pass and 57 skip of 260 (the 47 that Linux skips without a ROM but
    `layout_trainer_card_param`, which CLANG64's `llvm-dwarfdump` lets run, plus the 10 `gx_*_shm`, no aliasing backend
    there, and `card_backup_linked_save`); Linux 213 pass and 47 skip of 260; macOS 230 pass and 45 skip of 275. Linux's
    206 passing tests of the last run are 213 with the 7 new ones.
  - New on every host: `integrate_overlay_sections` (a probe file compiled as overlay 162's: its statics are in the
    overlay's sections, and a load sets them back) and `host_trap_placeholder`, `host_crash_report`, `host_watchdog`,
    `host_watchdog_kicked`, `host_static_data`, `host_files` (port/tests/host).
  - On Windows nothing with a ROM has run yet: the replays, the oracle's comparisons and the game past `main()` need a
    Windows machine with the player's dump (not yet). T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 512 function stubs and 46 data placeholders on macOS and Windows, 513 and 46 on Linux. `gen_stubs.py`
  now lists the ROM's symbols each host's C library provides: macOS 31, Windows 33 (`sin` and `cos` besides), Linux 31
  (with `sin` and `cos`, but without `atexit` and `atol`, which Python's `ctypes` doesn't find in glibc's process; if
  the game's objects reference them on Linux as on macOS, they are trap stubs there: a finding, under Next).
- **64-bit warnings:** not measured: no game or `lib/` code changed.
- **Work done:** PLAN.md phase 5's "Windows and Linux builds in CI", stages 0 to 3 of the Windows plan
  ([ADR 0013](adr/0013-windows-build-with-msys2-clang64.md), proposed).
  - **The Windows job** in `port-ci.yml`: MSYS2's CLANG64 through `msys2/setup-msys2`, a checkout without `res/`'s
    PNG, WAV and glTF files (the text's headers read the JSON data; `res/text` alone wasn't enough), git's line ends
    kept, `PYTHONUTF8`, and a build with `-k 0`.
  - **The host seam, `port/hal/host`**: what differs between the hosts, one file a host. `host_posix.c` holds the POSIX
    code that `trap.c`, `fiber.c`, `pointer.c`, `os_terminate.c`, `main.c`'s watchdog, `run_dir.c` and `symbols.c` had,
    moved with the same calls (a backtrace shows one more frame, one message is worded differently, the run directory's
    files open in binary mode, and `test_os.c`'s fault handler used stdio, which the next entry undoes);
    `host_windows.c` is Windows': `VirtualAlloc` and `VirtualProtect`; a vectored handler that moves a fault's report to
    a stack of its own (with the TEB's bounds), and the unhandled-exception filter for the rest; a fiber's guard topped
    by one guard page over 12 KiB left writable for the system's exception frame; backtraces by `RtlVirtualUnwind`;
    names and globals from the COFF symbol table lld keeps; a watchdog thread; the PE image's writable sections; binary
    stdio and no error dialogs before `main()`. The tests' harnesses (OS, PXI, sound, backup, ROM, integration) use it
    for their hang reports, faults, temporary files, links and environment.
  - **Windows' fixes**: the memory windows align to 4 KiB there (`PORT_HOST_PAGE_ALIGNMENT`; COFF stops at 8 KiB);
    `gen_stubs.py` reads its 1,582 objects from a list file on every host and, on Windows, asks the compiler driver's
    libraries what the C library provides, read with the toolchain's `nm`; the overlays' COFF bss markers are placed by
    the pragma, as uninitialized data like the statics they bracket (lld had grouped them into another `.bss`); the main
    thread has 8 MiB of stack; the include mirror's links to directories say so, and become copies where links are
    refused; `PYTHONUTF8` for the configure's generators.
  - **Portable fixes**: the run directory's files and the artwork generator's SVGs are written with `"\n"` on every
    host; the layout test finds `.obj` objects; the oracle's script tests run their stand-ins through a `.cmd` on
    Windows; `test_pxi.c` uses `localtime`; the backup tests expect the host's own error text and `%APPDATA%`;
    `card_backup_linked_save` skips on Windows, where the saves follow no links; the GX view check holds the views to
    the host's alignment; `check_exit.py --crash` for a test whose program must crash, since ctest fails a crashed test
    whatever its output.
  - **Documents**: port/README.md's Building on Windows and the landing page; ARCHITECTURE.md's feature map row
    "macOS, Linux and Windows", Windows' crash statuses and watchdog thread; the guides of `hal/`, `tests/`, `cmake/`
    and `app/`; the glossary (the host seam, MSYS2's CLANG64); PLAN.md §4.4; lessons.md; ADR 0013 and the indexes.
- **Next:** the user's acceptance of ADR 0013 (the research had recommended clang-cl). Windows' stage 4: a run of the
  game with a ROM on a Windows machine (the replays, `oracle_*`), sanitizer builds on CLANG64, a build without a
  console with its log under `%LOCALAPPDATA%\Frigate`, a UTF-8 manifest for user folders that aren't ASCII, a
  package, the memory-mapped ROM, ARM64 (minicoro's fiber fallback) and VRAM aliasing on Windows (placeholders,
  which a run-time region 0x06 would need); and Linux's trap stubs for `atexit` and `atol` (gen_stubs's `ctypes`
  lookup), to check. As in the last entry: White 2's starter route; the apps no route reaches yet; a save with an
  adventure made by the port, once overlay 36 lets it into the field; a sound comparison sequence played on the game's
  count.
- **Blockers:** as in the last entry: overlay 36 (issue #83, another contributor's active claim) gates phase 3 and the
  launcher's retirement; `NNSi_objSizeWTbl` stops the Pokédex; overlays 11 and 182 only matter from phase 7.

## 2026-10-18: the Pokédex runs and matches melonDS, thanks to the decomp's NitroSystem tables

- **Upstream:** `993e6c49b` (merge of PR #377), as merged into the port's `main` by `1f0c184f6`, which brought
  upstream's C of NitroSystem G2D's `NNSi_objSizeHTbl` and `NNSi_objSizeWTbl` (`e6dcf9b89`, PR #371,
  `lib/nnsys/src/g2d/g2d_OAM.c`); no sync in this task.
- **Port:** `92b9778ac`, on the branch `port-pokedex` from `2da26e048`, merged into `main` after `334077ba7`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`, a full build in the
  worktree). This task changes only `port/` and `.github/README.md`.
- **Host build:** arm64 macOS, RelWithDebInfo, `build/port` (Black 2) and `build/port-w2` (White 2), both Ninja, from
  scratch, then again after `main.c`'s `--help` text changed: no errors, 263 warnings in each, as before, none of them
  of the pointer and integer size kinds. The oracle is `build/verify-tree`'s `pbw-oracle`, copied into both builds'
  `oracle/` (no oracle source changed), and the saves its `oracle-saves`. Not pushed, so no CI run.
- **Tests:** T0 as above.
  - T4, `oracle.py launch party_menus` (Black 2) with the Pokédex launched: 8 of 8 app screens at the oracle's counts
    (the party list, Tepig's menu, the party list after the summary, the Pokédex's Unova list, Tepig's entry, the list
    after it, the card, the bag), 2,639 of 2,639 counts of the apps' spans the oracle's pictures (1,148 of them the
    Pokédex's, 5477-6624: its opening, the list, Tepig's entry, the list again and its close to black), the 19 rows of
    the table held to the oracle's run; the launcher reports event type 0 after each of the four apps (the Pokédex:
    opened at count 5473, closed at 6630, result 0), and the save is unchanged. Only the summary's row and span stay
    left out (`SUMMARY_SCROLL`).
  - T2, T3, T4, T6, Black 2, `ctest -j4` with the oracle: 266 of 267 pass (2,961 s), none skipped, under a load
    average of 60 to 110 from the other tracks' runs; `asm_oracle` timed out at its 900 s there (520 s in
    `build/verify-tree`'s last full run) and passes on its own through `ctest` (221 s), so all 267 pass.
    `oracle_launch_party_menus` passes as above, and `oracle_launch_field_menus`, `launch_summary_standalone`,
    `launch_touch_then_party`, `launch_unfinished`, `launch_not_offered`, `oracle_compare` (the route's declarations:
    four apps, four event types, the known rows among the table's) and `app_about` (`--help`'s `--launch` text) pass
    with it. The saves came from the build's cache, copied from `build/verify-tree`, whose SHA-1s are the ones
    `oracle.py` keeps. After the review's fixes, which touch only documents, `ctest -j4` again: 266 of 267 (2,290 s,
    none skipped; `oracle_launch_party_menus` 832 s), `asm_oracle` again past its 900 s under a load average of 70 to
    94, and passing on its own (433 s).
  - White 2, the tests this change touches, `ctest -j3`: `app_about`, `integrate_launch_bad_option`,
    `integrate_launch_never_asked`, `oracle_compare`, `oracle_saves`, `oracle_launch_field_menus` (2 of 2, 621 of 621),
    `launch_unfinished` and `launch_not_offered` pass; `oracle_launch_party_menus` skips as before (`SAVES` keeps no
    SHA-1 of `starter_save` for White 2).
  - Every run passed `--save-dir`; `~/Library/Application Support/Frigate` doesn't exist. clang-format 23.1.3 is clean
    on `main.c`. T5's nightly runs and T7 not yet.
- **Tiers** (`port/tools/progress`, upstream `993e6c49b`): (a) 32,723 functions, 2,962,822 code bytes (77.0%); (b)
  5,036, 561,554 bytes; (c) 3,259, 325,868 bytes (8.5%). Since the last entry's upstream, `cf8ded4c2`, which that
  entry didn't measure (`progress.py --rev` measures it here, and gives `aae2a03f6`'s recorded numbers back exactly):
  (a) +439 functions and 66,018 bytes; (b) 106 functions and 43,550 bytes fewer; (c) 333 functions and 22,468 bytes
  fewer, down from 9.0%. All of it is upstream's C merged by `1f0c184f6`; this task changes no decomp C.
- **Trap stubs:** 512 function stubs and 46 data placeholders in each version (`gen_stubs`, each build above), from 624
  and 48 at the last entry: upstream's code merged by `1f0c184f6`, the two data placeholders gone being
  `NNSi_objSizeHTbl` and `NNSi_objSizeWTbl`, on which the Pokédex stopped. A continued game without `--launch` still
  stops on `FIELDMAP_PROC_FUNCS`.
- **64-bit warnings:** none of the pointer and integer size kinds among the host build's 263 warnings, in each version;
  no game or `lib/` code changed in this task.
- **Work done:**
  - **The Pokédex in `party_menus`.** The route's `--launch` is now `party@4044,pokedex@5473,card@6974,bag@7673`, the
    apps in the oracle's order, with four event types and the Pokédex's span. The count is the oracle's: on its run
    (`launch_party_menus_b2_us.pbwinput` from the starter save), the field fades to black by count 5468 and the
    Pokédex's first picture shows at count 5477 (oracle frame 7682); the port takes 4 passes from opening the Pokédex
    to its first frame (`--launch pokedex@4044` shows it at count 4048, the same picture, `66252ee3`/`aa6f48fc`), so
    it opens at 5473. Its span ends at count 6624, the first black after it closes (oracle frame 8920; the field menu
    shows again at 6650). The DS spends 9 frames on count 5476 and 17 on count 5868, loading, yet every count of the
    span shows the oracle's picture on the port: unlike the summary's background, nothing the Pokédex shows moves by
    VBlanks, so no known difference was needed and no port bug turned up. Nothing was regenerated: the table's hashes
    are the oracle's, as committed.
  - **`POKEDEX_TRAP`** is gone from `oracle.py`, with its three known rows, and the trap from the documents that
    described it: the README's `--launch` section, `--help`'s `--launch` text in `main.c`, ARCHITECTURE.md's feature
    map row (the Pokédex with Tepig's entry is melonDS's pass for pass), MODDING.md's "What it can't do", the oracle
    README's launch checks (the table, the measured passes, the first frames aligned by construction, and what is
    left out), the route's input file and table comments, and `oracle.cmake`'s. ADR 0012 gets a dated note in its
    header, as the ADR rules allow, and its text is left as accepted; the older STATUS entries keep their words. The
    landing page and the README say that the Pokédex matches melonDS in Black 2 with the party list, and the tests
    guide names it among the oracle's apps.
  - **Equivalence notes** ([decomp-findings.md](decomp-findings.md), PLAN.md §6) for the unlinked C the Pokédex now
    runs: `lib/nnsys/src/g2d/g2d_OAM.c` and `g2d_RendererCore.c`, both incomplete. The size tables hold the ROM's 48
    bytes in both versions. A trace of `party_menus` under lldb shows the game reading them only in the Pokédex, through
    `DrawCellToSurface2D_`'s horizontal flip, from count 5869 in Tepig's entry. The nonmatching
    `NNS_G2dApplyOamManagerToHW`, `NNS_G2dGetNewOamManagerInstanceAsFastTransferMode` and `DrawCellToSurface2D_` run in
    every app of the route, so they are equivalent as observed; `SetAffinedOamPosition_` never runs there and stays
    unknown.
- **Next:** White 2's starter route, so that `party_menus` and `continue_starter` run in White 2; the apps no route
  reaches yet (the town map, the Xtransceiver, the medal box, the options); a save with an adventure made by the port,
  once overlay 36 lets it into the field; for the sound, a comparison sequence where a player plays on the game's
  count.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: upstream's matched libfx merged, and the divider's results read through the HAL however cp.h names them

- **Upstream:** `026351ef7` (40 commits after `6ca74421c`: overlay 11's GameSpy GP, overlay 36's script commands, the
  last libfx and libmath functions matched, NitroSystem G3D/G2D readers), merged by `4fee9b918` after `6ca74421c`'s
  `4e9685c0e`.
- **Port:** `2a06b6b82`, on `port-integrate` after the rename's commits (`c1ae95c3e`, `7a7bedf8f`), the hash checks' longer
  limit (`e67b54f28`), the merges and `3338cab12` (libfx's ledger).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `check_mwcc_neutral.py --base 026351ef7` on the four
  libfx files: 8 of 8 preprocess identically.
- **Host build:** arm64 macOS, `build/port` (Ninja, no build type, as `sync_upstream.sh` makes it), rebuilt.
- **Tests:** `fx_libfx` with four new checks of `VEC_Normalize` and `VEC_Fx16Normalize` through the translated headers,
  worked out by hand from GBATEK's divider and square root (all four fail without the fix: zeros); `replay_new_game`,
  `oracle_title_hashes` and `fx_libfx` pass; the asm oracle's `test_fx_decomp.py` passes on upstream's new libfx
  (88,500 cases). The full ctest runs at publishing (`sync_upstream.sh`).
- **Work done:**
  - **The merge's conflict:** upstream's `MTX_Concat33`, `MTX_Concat43` and `MTX_Concat44` now load each row of `a`
    before storing it, as the ROM does, so the port's blocks that made a product into `a` in `tmp` are gone;
    `MTX_MultVec43`'s copy of an in-place vector stays (game-changes.md, decomp-findings.md).
  - **The regression it exposed:** after the merge the intro drew no MCSS sprite (Professor Juniper, Minccino;
    `replay_new_game` showed 4 of 9 screens). Bisected by file against a pre-merge binary to upstream's `VEC_Normalize`
    and cp.h's new inlines, which read the divider's and square root unit's results through `REG_DIV_RESULT_ADDR` and
    `REG_SQRT_RESULT_ADDR`. `port/tools/gen_host_tree.py` translated those constants as plain memory; it now routes an
    address constant of a register that acts to the register's function, as it does the register macros, keeping write
    streams (`REG_GXFIFO_ADDR`) plain addresses. The DWC voice chat's `CP_GetDivResult32` read the same constant.
  - **Timeouts under load:** the sequences' hash checks get 1800 s (`e67b54f28`); lessons.md has the three lessons.
- **Next:** as the last entries: phase 5's tracks (sanitizers, the macOS distribution build, Windows) and the
  Pokédex's launch check (branch `port-pokedex`) merge next.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld, and the launcher's retirement (ADR 0012).

## 2026-10-18: the repository renamed CallanCampos/Frigate: the sync's guards, the scripts and the documents

- **Upstream:** `993e6c49b` (merge of PR #377), as merged into the port's `main` by `1f0c184f6`; no sync in this task.
- **Port:** `7a7bedf8f`, on the branch `port-integrate` from `main` at `2da26e048`: `c1ae95c3e` (the rename), then the
  review's fixes with this entry; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`), at `c1ae95c3e` and
  again with the review's fixes. No file that the ROM build reads changed, so ninja had only the two ROM checks to run.
- **Host build:** arm64 macOS, RelWithDebInfo, `build/port-n` (Black 2), Ninja, rebuilt from this tree: CMake re-ran for
  the project's new `HOMEPAGE_URL` and 1,277 steps rebuilt, exit 0, with `gen_overlay_sections`' checks passing. The
  rebuild also compiled the C of upstream's `993e6c49b`, merged by `1f0c184f6`, which the last entry's builds predate.
  White 2's `build/port-w2` not rebuilt: the change is the same for both versions and touches no C. With the review's
  fixes, which touch no C or CMake file, `cmake --build build/port-n` exits 0 with no work to do. Not pushed, so no CI
  run.
- **Tests:** T0 as above.
  - `port_guard.py --self-test`: 125 of 125 cases pass (77 blocked, 48 allowed), its cases now writing to
    `CallanCampos/Frigate`; `python3 -m py_compile` is clean, and fed as a hook in this clone, a write with
    `-R CallanCampos/Frigate` or to `repos/CallanCampos/Frigate` passes and one to upstream is refused.
  - `sync_upstream.sh`'s origin check, dry, in a throwaway clone (`build/rename-check`, deleted after) whose tracked
    change stops the script right after the check, before anything is fetched: 30 of 30 cases. The new name passes over
    https (with and without `.git`, with a trailing slash, in lower case), `git@github.com:` and `ssh://`, with and
    without `.git`. The old name is refused with exit 2 in all six forms tried, each with the `git remote set-url`
    command for its own transport; running the printed command, then the script, gets past the check every time. An old
    push URL gets the `--push` form, and an old URL beside a new one gets the form that names the URL it replaces. The
    decomp and the fork still get their message, and `CallanCampos/Frigate2`, `someone/Frigate`, `pokebw2-native2` and a
    non-GitHub host the general one. `--help` prints the header with the new step 1, exit 0.
  - After the review, the same check in 32 throwaway repositories (in the session's scratchpad, deleted after), 85 of
    85 checks: the 7 new forms pass; 7 old forms are refused with exit 2 and the command for their own transport, which
    gets past the check; the decomp, the fork, `Frigate2`, `someone/Frigate`, `pokebw2-native2`, gitlab, `http://` and
    no origin get their usual refusal; and 10 cases with several URLs of a kind (the review's two old https URLs, two
    old `git@` URLs, an old URL before or after a new one, a mixed-case one beside its lower-case twin, two old push
    URLs, one old push URL, and an old URL configured twice, alone, beside a new one and as push URLs) each get the
    command whose run leaves only the port's URLs. Run against `c1ae95c3e`'s script, 6 of those 10 printed a command
    that git refuses ("has multiple values"), the review's case among them.
  - `bootstrap_checkout.sh`'s gh default check, run alone against throwaway repositories: 15 of 15 cases after the
    review (the port by URL over https and ssh or by name; origin's old URL over https, mixed case with a trailing
    slash, `git@` and `ssh://`, each of which gets the `git remote set-url` command in its own transport; origin on
    `someone/Frigate`, `CallanCampos/Frigate2`, `pokebw2-native2` or the old name on another host, a default on the old
    name by name, another remote on the old URL, `upstream`, or none, which get
    `gh repo set-default CallanCampos/Frigate`). `c1ae95c3e`'s version fails 6 of them: it told origin on any other
    repository to point at the port, and gave `git@` and `ssh://` clones an https URL.
  - The write guard, fed the commands the scripts now print (`git remote set-url`, `--push`, both
    `git config --replace-all` forms, `gh repo set-default CallanCampos/Frigate`), lets them all through, and still
    refuses a `git config --replace-all` of origin's push URL to upstream followed by a push.
  - `bash -n` and `sh -n` on both scripts. actionlint isn't installed on this machine, so not run; both port workflows
    parse as YAML, and `port-sync.yml`'s two jobs' conditions name `CallanCampos/Frigate`.
  - Every relative link of the Markdown files under `port/` and `.github/README.md` resolves, anchors as GitHub makes
    them included: 1,096 Markdown links and 5 HTML `href` and `src` attributes, 1,101 in all, in 58 files (checked by
    script, before and after the review's fixes, which add none).
  - `ctest` not run: this change touches no C, test or target, and nothing reads the project's `HOMEPAGE_URL`. T5's
    nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 512 function stubs and 46 data placeholders in Black 2 (`gen_stubs`, the rebuild above), 112 and 2
  fewer than the last entry's: the C that upstream's `993e6c49b` brought, not this change. White 2 not measured.
- **64-bit warnings:** none of the pointer/int kinds (`-Wint-to-pointer-cast`, `-Wpointer-to-int-cast`,
  `-Wshorten-64-to-32`, `-Wint-conversion`) in the rebuild above, game and `lib/` alike; its 204 warnings are of other
  kinds (`-Wswitch` 132, `-Wpointer-sign` 34), and a partial rebuild's total isn't comparable with a full build's.
- **Work done:** the user had the repository renamed from `CallanCampos/pokebw2-native` to `CallanCampos/Frigate`;
  GitHub redirects the old URL. Every living reference now names the new one.
  - **The hourly sync**: `port-sync.yml`'s two jobs run only when `github.repository` is `CallanCampos/Frigate`. Until
    this is on `main`, the renamed repository's runs skip both jobs, since they compare with the old name.
  - **`sync_upstream.sh`** requires origin to be `CallanCampos/Frigate`; its matcher takes the repository as an
    argument. A fetch or push URL with the old name, in any of the forms the check accepts, is refused with exit 2 and
    the exact command that updates it, in the URL's own transport
    (`git remote set-url origin https://github.com/CallanCampos/Frigate.git`, `--push` for a push URL), rather than
    accepted through the redirect. Where origin has several URLs of that kind, the command is
    `git config --replace-all` with the old URL as an escaped, anchored pattern: git reads that argument as an extended
    regular expression matching anywhere in a URL, so the old https URL without `.git` matched the one with it, and
    `git remote set-url` refuses a pattern that matches more than one URL (the review's fix). Its header and messages
    name the new repository.
  - **`bootstrap_checkout.sh`** reports the port as gh's default only for `CallanCampos/Frigate`, matched as a whole
    name; when the default is origin and origin's URL still has the old name, as in a clone made before the rename, it
    gives the `git remote set-url` command in that URL's transport, and for any other default that isn't the port,
    origin on another repository included, `gh repo set-default CallanCampos/Frigate` (the review's fix).
  - **`port_guard.py`**: `PORT`, the self-test's `PORT_URL`, its messages and its cases name `CallanCampos/Frigate`; the
    pattern it uses for a command it can't parse no longer leaves out the old name.
  - **CMake**: the project's `HOMEPAGE_URL`. **The landing page** links the new repository's issues.
  - **port/README.md**: the clone steps (`git clone https://github.com/CallanCampos/Frigate.git`, `cd Frigate`,
    `gh repo set-default CallanCampos/Frigate`), and what a clone made before the rename runs.
  - **port/CLAUDE.md** says the repository is `CallanCampos/Frigate`, renamed from `pokebw2-native` on 2026-10-18 at the
    user's request, and drops the sentence that it stays `pokebw2-native`; its remotes, gh commands and the sync's
    origin rule name the new repository. The port clone on this machine keeps its folder, `~/dev/decomp/pokebw2-native`,
    for now, as CLAUDE.md, PLAN.md §9 and ADR 0001's note say; a fresh clone's folder is `Frigate`.
  - **SYNC.md**'s commands and the Action's and the local sync's repository checks, with the old URL's refusal.
  - **The switch-over** (the review's transition finding): worktrees share the clone's `origin`, so once it names
    Frigate, a worktree whose branch predates this commit runs the old `sync_upstream.sh`, which refuses the new URL
    and says origin must be `CallanCampos/pokebw2-native`. SYNC.md and CLAUDE.md now say to merge `origin/main` there
    and run it again, never to set origin back; the worktrees' own copies say so only once they have merged.
  - **Records**: ADR 0001 and PLAN.md's D1 and §4.1 get a dated one-line note of the rename, their decisions as written;
    PLAN.md's opening and §9 name the new repository. Older STATUS entries, the research notes, ADR 0010's log folder
    and the `com.example.pokebw2-native` bundle placeholder that `bundle_id.cmake` migrates (and its tests) are history
    and stay. `git grep -i pokebw2-native` now finds only those, the clone's folder, CLAUDE.md's rename line, and the
    old name in `sync_upstream.sh` and `bootstrap_checkout.sh`, which recognize a clone made before the rename by it.
  - **The last entry**: its Port field names `bbfab4c72`.
- **Next:** the clone's `origin` moves to `https://github.com/CallanCampos/Frigate.git` and gh's default to
  `CallanCampos/Frigate` before this is published with `sync_upstream.sh`, which refuses the old URL from this commit
  on; then each worktree whose branch predates it (the clone's root, `wt-dex`, `wt-mac`, `wt-san` and `wt-win` when this
  was written) merges `origin/main` before its next sync. Renaming the clone's folder is the user's call. Then as in the
  last entry: White 2's starter route, so that `party_menus` and `continue_starter` run in White 2; the apps no route
  reaches yet (the Pokédex past its trap); a save with an adventure made by the port, once overlay 36 lets it into the
  field; for the sound, a comparison sequence where a player plays on the game's count.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); `NNSi_objSizeWTbl` (ARM9 main's NitroSystem G2D, no C yet) stops the Pokédex;
  overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: the launcher track's merge after review: --help's options and statuses, and the documents as built

- **Upstream:** `cf8ded4c2` (merge of PR #367), as merged into the port's `main` by `65ea798fb`; no sync in this task.
- **Port:** `bbfab4c72`, on the branch `port-integrate` after `bfe72cd2b`: `b1ca0c257` (the reviews' fixes), then this
  entry; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`) at `b1ca0c257`. No
  game file changed: game-changes.md's one edit marks `event_mapchange.c`'s second row "(again)".
- **Host build:** arm64 macOS, RelWithDebInfo, `build/port-n` (Black 2) and `build/port-w2` (White 2), both Ninja,
  built again from `b1ca0c257`: 10 C files each (`main.c`, the launcher, the OS core and its test, which changed), with
  no warning; the decomp's C didn't change, so its 263 warnings (the last entry) stand. The oracle, `build/oracle`, had
  nothing to rebuild (no oracle source changed), and its `pbw-oracle` is the one in both builds' `oracle/`. Not pushed,
  so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 on `b1ca0c257`, the two builds at once, `ctest -j4` each, with the oracle in both: Black 2, all 267
    `ctest` tests pass (980 s), none skipped; White 2, 261 of 263 pass (884 s), and `oracle_continue_starter` and
    `oracle_launch_party_menus` skip as before, since `SAVES` keeps no SHA-1 of `starter_save` for White 2. The saves
    came from each build's cache, whose SHA-1s are still the ones `oracle.py` keeps, so `oracle_saves` reused them
    (1.3 s). `oracle_launch_field_menus` passes in both versions (2 of 2 app screens at the oracle's counts, 621 of 621
    counts of the apps' spans the oracle's), `oracle_launch_party_menus` in Black 2 (5 of 5, 1,491 of 1,491).
    `app_about` now also checks that `--help` describes each of the 19 options of its usage and that its `--launch`
    names the apps the launcher names (party, pokedex, bag, card, summary); fed a `--help` without `--rom`'s line, or
    without one of the apps, it fails. The tests left `~/Library` alone.
  - The apps: `package_macos.sh` makes `Frigate Black 2.app` and `Frigate White 2.app` and their disk images, `Frigate
    Black 2 0.0.2662.dmg` and `Frigate White 2 0.0.2662.dmg` (5.5 MB each), from `b1ca0c257` (revision
    `b1ca0c257163`, clean). `plutil -lint` passes and reads the names, identifiers, programs and icon as in the last
    entry; `hdiutil verify` and `codesign --verify --deep --strict` on each app and on the app in its mounted image
    pass. Each app's program runs 300 frames headless (`--save-dir` a scratch directory), status 0, and its `--help`
    opens with the name, the description and the disclaimer, then the usage, 19 options and the exit statuses.
  - Every relative link of the Markdown files under `port/` and `.github/README.md` resolves, anchors as GitHub makes
    them included (checked by script). clang-format 23.1.3 is clean on the five C files changed.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp change.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version (`gen_stubs`, each build above), as in
  the last entry.
- **64-bit warnings:** not measured: no game or `lib/` code changed. The last entry's field gave the host build's 263
  warnings, which are of other kinds; it now says not measured too, with why.
- **Work done:** the two reviews of the launcher track's merge (`859e4c1cf` to `bfe72cd2b`), each finding checked
  against the code and fixed in `b1ca0c257` or this entry, but one.
  - **`--help`** printed only the usage line, though the README sends readers to it for the options, and its comment
    in `main.c` said `--frames` always exits with status 0. The options and exit statuses are now tables in `main.c`,
    `sOptions` (name, value, whether it repeats, what it does) and `sExitStatuses`, which the usage, `--help` and the
    option check all read, and `main.c`'s header points to them; `--frames` says the status can be another, such as an
    unfinished launch's 1. `app_about` tests it, and MODDING.md's steps for a new option say where it goes.
  - **The exit statuses** read the same in `main.c`, `launcher.h`, the README and MODDING.md: a status of the run's
    own comes before the launcher's, such as a trap's 70, a deadlock's 72, the watchdog's 73 or a 1 of its own;
    MODDING.md's "When the game stops" has a status 2 bullet (bad options, or a `--launch` app the save's menu doesn't
    offer); `launcher.h`'s status comment ends before `PORT_LAUNCH_STATUS_DONE`'s.
  - **ARCHITECTURE.md**: the plain-C rule names the test launcher as `port/app`'s one exception, and the layer table
    the decomp's seam into it; the touch screen's row has `launch_touch_then_party`, whose input file touches the
    card's return button; the field's row says a continued game stops on `FIELDMAP_PROC_FUNCS`, data with no C, with
    `oracle_continue_starter`; the saves' row says which saves the tests start from.
  - **MODDING.md**: the commands that make a save on the oracle run as written (`python3`, since `oracle.py` isn't
    executable, as no Python tool of the port is), link to the oracle's Building section, and give White 2's first
    save; the game's stop at the field names `FIELDMAP_PROC_FUNCS`.
  - **Facts**: the menu without a BAG is the Black Tower's or the White Treehollow's (README, MODDING, the app guide,
    `launcher_spec.h`); the README and the landing page say that the party list matches melonDS in Black 2 so far;
    GLOSSARY.md, ARCHITECTURE.md and the tests guide say that `oracle_saves` makes the first and the starter save, and
    that the Route 19 save is made on request, for battles, with no test yet; the party_menus table and
    `oracle.cmake` say that the summary's row is left out of the check.
  - **Words**: GLOSSARY.md folds "H-blank, DISPSTAT's flags" into its VBlank and HBlank row, and HBlank is spelled one
    way in the living documents and the port's comments (lessons.md's dated entry keeps its words); a row for game
    events and event types; the documents' index names the test launcher and the oracle's sound and launch checks;
    game-changes.md marks `event_mapchange.c`'s second row "(again)"; `check_struct_layout.py`'s usage prints its line
    continuation (a raw docstring; no other Python file of the port has the problem).
  - **ADR 0012** gets a one-line note: accepted at the merge, so "On acceptance" is done and its "Until then" no longer
    holds, and what retiring the launcher also removes from the documents and tools.
  - **The last entry**: its Port field names `bfe72cd2b`, its 64-bit warnings field says not measured instead of the
    host build's count of every kind, and it says who accepted ADR 0012 and that the user's acceptance is wanted.
  - **Not changed**: the merge's subject, "Merge port-launch", though its second parent is `port-launch-adr0012`
    (`b4c0fdc68`), which holds `port-launch`: the message was the integration task's, the last entry names the branch
    rightly, and rewording it would rewrite every later commit, whose SHAs the records cite.
  - **ADR 0012, accepted by the developer at publishing.** The ADR doesn't loosen port/CLAUDE.md's rule on
    stand-ins: it uses the rule's own exception, a documented stand-in for a subsystem under another contributor's
    claim, written for `ov011_pending.c`. The launcher is test-only and off by default, runs only the game's own C,
    and steps aside once `FIELDMAP_PROC_FUNCS` has C, so it was accepted as every earlier ADR was, and the user is
    told; reverting `d7849a158` and the launcher's commits takes it out. This closes what the review left to the
    user. Also at publishing: `--help`'s exit statuses use the named constants where the program can include their
    headers (65 and 67 stay numbers, with their constants named, since their headers need the decomp's) and 71 names
    the card DMA call too; ARCHITECTURE.md's Saves row says the first save starts from a blank one; MODDING.md and
    the tests guide say the oracle's runs need the ROM build's link map.
- **Next:** as in the last entry: White 2's starter route, so that `party_menus` and `continue_starter` run in White 2;
  the apps no route reaches yet (the Pokédex past its trap); a save with an adventure made by the port, once overlay 36
  lets it into the field; for the sound, a comparison sequence where a player plays on the game's count.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the launcher's
  retirement (ADR 0012); `NNSi_objSizeWTbl` (ARM9 main's NitroSystem G2D, no C yet) stops the Pokédex; overlays 11 and
  182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: the launcher track merged under Frigate's rules, and ADR 0012 accepted

- **Upstream:** `cf8ded4c2` (merge of PR #367), as merged into the port's `main` by `65ea798fb`; no sync in this task.
- **Port:** `bfe72cd2b`, on the branch `port-integrate` from `main` at `e7187b168`: `859e4c1cf` (the merge of
  `port-launch-adr0012`, which holds `port-launch`), then `d7849a158` (ADR 0012 accepted), `1249408e1` (the launcher
  track under Frigate's rules) and `0b51e6acf` (exit status 1's every cause), then this entry; not pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`; `python3 configure.py && ninja`), at the merge and at
  `0b51e6acf`. The merge brings the launcher track's two game changes, rows of game-changes.md: the Neutral row of
  `src/ov012/event_field_proclink.c` (`014fe956c`, a complete file, so the ROM build is its proof) and the Platform row
  of `src/ov035/event_mapchange.c` (`b8b52f420`, the launcher's seam, which no longer calls ADR 0012 proposed).
  `check_mwcc_neutral.py` on the merged tree finds 3,142 of 3,146 file and version pairs identical to `cf8ded4c2`,
  `event_mapchange.c` among them, the other four being `vm.c`'s and `event_field_proclink.c`'s Neutral rows. The later
  commits change nothing outside `port/` but the landing page, `.github/README.md`.
- **Host build:** arm64 macOS, RelWithDebInfo: Black 2 in a new Ninja directory, `build/port-n` (the old `build/port`,
  made with Unix Makefiles, deleted after), White 2 in `build/port-w2` (Ninja), configured again. The merge, and
  `port_host.h`'s new header comment after it, rebuilt all of the decomp's C: 263 warnings in each version, all in its
  `src/` (193) and `lib/` (70), none in `port/`, as in Frigate's entries. The oracle, `build/oracle` (Release, on
  melonDS 1.1's source in the clone's `build/port-oracle`), rebuilt from the merged sources and copied into each build's
  `oracle/`. Not pushed, so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6 on `0b51e6acf`, the two builds at once, `ctest -j4` each, with the oracle in both and both
    directories Ninja's, so no test skips for want of either: Black 2, all 267 `ctest` tests pass (1,392 s), none
    skipped, `asm_oracle` among them; White 2, 261 of 263 pass (1,150 s), and `oracle_continue_starter` and
    `oracle_launch_party_menus` skip, since `SAVES` keeps no SHA-1 of `starter_save` for White 2 (the Black 2 route's
    walk goes astray there: the launcher track's entries). The saves: Black 2's `first_save` and `starter_save` came
    from the launcher worktree's cache, whose SHA-1s are the ones `oracle.py` keeps, so `oracle_saves` reused them;
    White 2's `first_save` was made by `oracle_saves` (188 s) and reused after. `oracle_launch_field_menus` passes in
    both versions (Black 2: 2 of 2 app screens at the oracle's counts, 621 of 621 counts of the apps' spans the
    oracle's), `oracle_launch_party_menus` in Black 2 (5 of 5, 1,491 of 1,491); `launch_unfinished` (status 1),
    `launch_not_offered` (2), `integrate_launch_never_asked` (1), `integrate_launch_bad_option` (2),
    `integrate_launcher_off`, `app_launcher_spec_*`, `hal_card_spi_*`, `rom_game_ir_genuine_card`,
    `os_hblank_flag_in_handler` and `layout_trainer_card_param` pass in both, `launch_summary_standalone` and
    `launch_touch_then_party` in Black 2, and Frigate's and the sound's tests as before (`app_about`, `app_make_app`,
    `assets_branding`, `oracle_sound_*`). A quick run at the merge, before the later commits, passed all 234 tests but
    the oracle's, the replays', the asm oracle's and the launch tests.
  - The apps: `package_macos.sh` makes `Frigate Black 2.app` and `Frigate White 2.app` and their disk images, `Frigate
    Black 2 0.0.2660.dmg` and `Frigate White 2 0.0.2660.dmg` (5.5 MB each), from `0b51e6acf`'s tree with this entry's
    edits (revision `0b51e6acfec6-dirty`). `plutil` reads `CFBundleName` Frigate, `CFBundleDisplayName` "Frigate Black
    2" and "Frigate White 2", `io.github.callancampos.frigate.black2` and `.white2`, `CFBundleExecutable`
    `frigate-black2` and `frigate-white2` and `CFBundleIconFile` AppIcon; `plutil -lint` passes, and so do `hdiutil
    verify` and `codesign --verify --deep --strict` on each app and on the app in its mounted image, which holds the
    app and a link to `/Applications`. Each app's program runs 300 frames headless (`--save-dir` a scratch directory),
    status 0, and its `--help` opens with the name, the description and the disclaimer and lists `--launch`. Black 2's,
    from a copy of the oracle's first save with `field_menus`' input and `--launch bag@4044,card@4584`, opens and
    closes both apps (event type 0), stops after 4,942 frames with status 0, and leaves the save as it was.
  - Every relative link of the Markdown files under `port/` and `.github/README.md` resolves, anchors as GitHub makes
    them included (58 files, 1,099 links, checked by script). clang-format 23.1.3 is clean on every C file changed since
    `e7187b168`: 19 in `port/` and the two decomp files.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: the only decomp changes are the launcher track's two rows, measured on its branch.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version (`gen_stubs`, each build above), as in
  the launcher track's and Frigate's entries.
- **64-bit warnings:** not measured: the compile probe didn't run. The merge's game changes are the launcher's seam, a
  `PLATFORM_PC` block MWCC never sees, and `event_field_proclink.c`'s typedef removal, in a complete file. The host
  build's 263 warnings (Host build above) are other kinds, `-Wswitch` and `-Wpointer-sign` among them: the game's
  target turns the pointer/int warnings off, which `port/tools/compile-probe` counts.
- **Work done:** the launcher track, `port-launch` (the cartridge's infrared chip, saves made from saves on the oracle,
  the H-blank interrupt with DISPSTAT's flag) and `port-launch-adr0012` (the test launcher and its routes), merged into
  `main` after Frigate's identity and the sound, which it never saw.
  - **The merge** (`859e4c1cf`). Ten conflicts, each kept with both sides: `main.c` (Frigate's usage, program name and
    statuses with the sound's options and `--launch`), `integrate.cmake` (the launcher's objects and option checks
    beside Frigate's boot test), `os.cmake` (the H-blank's scenario and the sound's timed work), `trap.c`,
    `test_run_dir.c` (`frigate-black2` with the run directory's launch field), `oracle.py` (the sound and launch
    commands; `run_port`'s sound trace, WAV dump, `--launch` and log together), the oracle README and lessons.md (both
    sides' sections, the launcher's lessons first), port/README.md (Frigate's directory map) and this file: the
    launcher track's eight entries above Frigate's, the last naming `b4c0fdc68`, Frigate's last naming `e7187b168`;
    game-changes.md's `event_mapchange.c` row names `b8b52f420`.
  - **ADR 0012 accepted** (`d7849a158`), in this integration, on its task's instructions. The ADR's status had waited
    for the user's acceptance, since it reads port/CLAUDE.md's rule on stand-ins, and no record in this repository says
    that the user gave it: the user confirms it before `port-integrate` is published, or `d7849a158` is reverted (the
    next entry's Blockers). Its status is accepted, and its "On acceptance" section is done: port/CLAUDE.md's "Code that
    isn't C yet" names the launcher, after `ov011_pending.c`, as the other documented stand-in (test-only, off by
    default, while overlay 36 is claimed), and PLAN.md's phase 4 says that until overlay 36 lands the field menu's apps
    are tested through it against the oracle's route through the field menu. The ADR index says accepted.
  - **The launcher track under Frigate's rules** (`1249408e1`). Names: the README's `--launch` examples and the run
    directory's test run `frigate-black2`; nothing else the track wrote had an old name. The README's test launcher
    section moves from "Playing" to the quick start, whose options list `--launch`; the README's and the landing page's
    status say that the bag, the trainer card and the party list match melonDS pass for pass through it. Headers:
    `launcher.h`, `launcher_spec.h` and `card_spi.h` open with what they do before their guards, and the files the
    track changed name what it added (`CMakeLists.txt`, `run_dir.c` and `.h`, `app.cmake`, `oracle.cmake`,
    `rom.cmake`, `core.c`, `trap.c` and `.h`, `port_host.h`, `gen_host_tree.py`, `oracle.py`, `test_os.c`,
    `test_run_dir.c`, `test_compare.py`). Guides: port/app's rows for `launcher.c` and `launcher_spec.c`; port/hal's
    card module (the SPI bus and the infrared chip, the genuine-card check), `card_spi.c`'s row, the OS core's H-blank
    and the screen wipes, and the trap's placeholder check; port/include's new declarations; port/tools' and the oracle
    README's saves and launch checks; port/tests' rows for the launcher's, the card's, the layout and the H-blank
    tests; port/cmake's fifth object library, `layout_tests.cmake` and the launch tests. ARCHITECTURE.md's feature map
    has rows for the field menu's apps through the test launcher and for the H-blank and screen wipes, and the saves'
    and the wireless rows have the saves made on the oracle and the infrared chip with the genuine-card check.
    GLOSSARY.md has the H-blank and DISPSTAT's flags, the infrared chip and the card's SPI bus, the field menu and app
    call, the genuine-card check, the test launcher and the saves made on the oracle. MODDING.md says how to reach the
    field menu's apps with `--launch` while overlay 36 is missing; the documents' index lists ADRs 0011 and 0012.
  - **Exit status 1** (`0b51e6acf`). The confirmation found more causes of status 1 than `main.c`'s usage and the README
    gave (a run directory that fails to write during the run, the launcher stepping aside). `main.c`'s usage,
    `launcher.h`, `launcher.c`'s header, the README's launcher section, ARCHITECTURE.md's run-end table and
    MODDING.md's "When the game stops" now give the same list: the window, a write to the run directory during the run
    (which stops it at once) or to the sound trace, a module out of host memory or finding a build that can't work,
    and a launch that didn't finish (the run ended first, the game never asked for the field, the launcher stepped
    aside, an app wanted the field); a status of the run's own comes before the launcher's.
  - **This entry's commit** also names the test launcher in ARCHITECTURE.md's layer diagram, its layers table and its
    list of object libraries, which the guides' pass missed.
- **Next:** White 2's starter route, measured on its own oracle, so that `party_menus` and `continue_starter` run in
  White 2; the apps no route reaches yet (the Pokédex past its trap); a save with an adventure made by the port, once
  overlay 36 lets it into the field; for the sound, a comparison sequence where a player plays on the game's count.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) gates phase 3, the overworld, and the
  launcher's retirement (ADR 0012); `NNSi_objSizeWTbl` (ARM9 main's NitroSystem G2D, no C yet) stops the Pokédex;
  overlays 11 and 182 (Wi-Fi, Download Play) only matter from phase 7.

## 2026-10-18: the test launcher after review: an unfinished launch fails, the menu's kinds, the touch flag per app

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `b4c0fdc68`, on the branch `port-launch-adr0012`, after `265a6bbe9`; the launcher track (`port-launch`
  and `port-launch-adr0012`) merged into `main` after Frigate's identity and the sound (`e7187b168`).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 252 `ctest` tests pass, the oracle's included (461 s), 4 new:
  `integrate_launch_never_asked` (no save, so no CONTINUE: status 1, and the run says so), `launch_unfinished`
  (`field_menus`' presses with `--frames 4400`, so the card never opens: status 1), `launch_not_offered` (the party
  list from the first save, whose menu is type 3: status 2) and `launch_touch_then_party` (the card closed by a touch
  on its return button, then the party list, which shows the `party_menus` route's oracle screen at count 4250);
  `integrate_launch_bad_option` checks its status, 2, through `tests/app/check_exit.py`; `app_launcher_spec_*` check
  the kinds of menu against `field_menu.c` read by hand; `integrate_launcher_off` checks that the launcher leaves the
  status alone. Against the program before this commit, `launch_unfinished` fails (status 0, no line about the card)
  and `launch_touch_then_party` fails (the party list's bottom screen `70ab6892` at count 4250, not the oracle's
  `47ea70c7`). `oracle_launch_field_menus`, `oracle_launch_party_menus` and `launch_summary_standalone` pass as
  before. White 2: all 248 pass but `oracle_continue_starter` and `oracle_launch_party_menus`, which skip (no starter
  save); `oracle_launch_field_menus`, `launch_unfinished`, `launch_not_offered` and `integrate_launch_never_asked`
  among the passes. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version.
- **64-bit warnings:** unchanged: no game file changed.
- **Work done:**
  - **The exit status** (`port/app/launcher.h`'s `PORT_LAUNCH_STATUS_*`): a run that ended before the last app closed,
    a game that never asked for the field and a launch cut short by an event type that needs the field all ended with
    status 0. Now 0 means every app opened and closed (event type 0 or 1), 1 that the launch didn't finish, with a
    line saying what never opened or never closed, and 2 that the save's menu doesn't offer an app.
  - **The menu's kinds** (`launcher_spec.c`): the launcher asked event flags 0x961 and 0x962 only. It now gives the
    kind of menu `FieldMenu_GetMenuType` gives, from the same zone functions (ARM9 main's), `GameData` and flags in the
    same order, and the items its table gives each kind: the Black Tower's and White Treehollow's menu has no BAG.
  - **The touch flag** is set FALSE before every app, as the menu's choice by a key does, not only before the first.
  - **Documentation:** ADR 0012's decision names the three things the launcher stands in for, and says its last
    subscreen is a constant the oracle shows; the README and `main.c` give the statuses and the Pokédex's trap, some
    395 passes after it opens; the oracle README and the launch entry say that each launched app's first frame is
    aligned by its `--launch` count, measured on the port, so its timing isn't a check.
  - `oracle_compare`'s launch tests left the runs of each failed check they make on purpose in the system's temporary
    directory, some 25 MB a run; they go in the test's own directory now.
- **Next:** the user's decision on ADR 0012; White 2's starter route; the apps no route reaches yet.
- **Blockers:** as in the last entry.

## 2026-10-18: White 2's field menu route on the launcher; its starter save left for a route of its own

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `265a6bbe9`, on the branch `port-launch-adr0012`, after `b0fd1f745`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. White 2: `oracle_launch_field_menus` passes, 2 of 2 app screens and 621 of 621 counts of the
  bag's and the card's spans the oracle's, with Black 2's counts and spans, measured again on White 2's oracle (the
  rows at Black 2's counts show the same screens, White 2's menu and card in its own colours on the bottom screen);
  `oracle_compare` (50) and `oracle_saves` pass; `oracle_launch_party_menus` skips (no starter save). Black 2: all 248
  `ctest` tests pass, as in the last entry. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version.
- **64-bit warnings:** unchanged: no game file changed.
- **Work done:**
  - White 2's `field_menus` route: Black 2's input file with `version w2_us`, its table of the oracle's hashes at the
    same labelled counts (`port/tests/oracle/launch_field_menus_w2_us.*`), and the route's `--launch` and spans for
    White 2 in `LAUNCHES`.
  - White 2's starter save, from Black 2's starter route on White 2's first save, went astray: at frame 17,000 the
    player stands in front of the Trainers' School with no Pokémon, the save holding 0x40a1 = 1 and neither flag
    0x961 nor 0x962. White 2 stays out of `starter_save` and `party_menus` until it has a route of its own, and its
    tests that need them skip with the reason.
  - `decomp-findings.md`: `src/ov165/plist_sys.c` (incomplete, two nonmatching rows) runs on the port, equivalent as
    observed on the paths the party route takes.
- **Next:** the user's decision on ADR 0012; White 2's starter route; the apps no route reaches yet.
- **Blockers:** as in the last entry.

## 2026-10-18: the field menu's apps natively, through a test launcher, checked against melonDS pass by pass

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `b0fd1f745`, on the branch `port-launch-adr0012`, after `b8b52f420` (the launcher, ADR 0012, proposed).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `b8b52f420` adds a guard-only block to
  `src/ov035/event_mapchange.c` (a Platform row): `check_mwcc_neutral.py` finds 3,142 of 3,146 file and version pairs
  identical to `045e97b90`, this file among them, the other four being `vm.c`'s and `event_field_proclink.c`'s Neutral
  rows. This commit changes no game file.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 248 `ctest` tests pass, the oracle's included, 3 new over the last entry:
  `oracle_launch_field_menus`, `oracle_launch_party_menus` and `launch_summary_standalone`;
  `oracle_compare` has 50 tests (7 new: the launch check with stand-ins for both programs, and the routes'
  declarations). With the option off nothing changed: `oracle_continue` still stops on `FIELDMAP_PROC_FUNCS` after the
  same frame with the same screens, and the replays pass. T4, the apps against melonDS 1.1 (`oracle.py launch`):
  - `field_menus`, from the first save: the bag and the trainer card, 2 of 2 app screens at the oracle's counts, and
    621 of 621 counts of their spans (the bag 4046-4323, the card 4588-4930) the oracle's pictures, pass by pass (the
    count of each launched app's first frame is aligned by its `--launch` count, measured on the port: for 2 of the
    621, and 3 of the 1,491 below, the picture is checked but not when it shows);
  - `party_menus`, from the starter save: the party list, Tepig's menu, back from the summary, the trainer card with
    its Pokédex line and the bag, 5 of 5 app screens, 1,491 of 1,491 counts; left out, the summary (its background,
    scrolled by VBlanks, is about 9 pixels on, from the DS's long loading passes) and the Pokédex (a trap, below);
  - the launcher reports event type 0 after every app, as the oracle's menu shows again, and the saves are unchanged.
  `launch_summary_standalone`: the summary as the game opens it without the party list, closed with B, no trap.
  White 2: all its tests pass or skip; the launch checks and `oracle_continue_starter` skip (no starter save and no
  route measured for White 2). T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed beyond the seam's guard.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version. With `--launch`, the Pokédex
  stops on `NNSi_objSizeWTbl` (ARM9 main's NitroSystem G2D, `0x020942b0`, with `NNSi_objSizeHTbl` at `0x02094298`: the
  48 bytes of `.rodata` no delinks entry covers); without it, a continued game stops on `FIELDMAP_PROC_FUNCS` as before.
- **64-bit warnings:** unchanged: the game change is a `PLATFORM_PC` branch.
- **Work done:**
  - **The launcher** (`b8b52f420`, the last entry; [ADR 0012](adr/0012-test-launcher-for-the-field-menus-apps.md),
    proposed: it waits on this branch for the user's decision). `--launch APP[@COUNT],...` opens the field menu's apps
    where CONTINUE asks for the field, through overlay 12's app call as the game has it, and never touches overlay
    36's code.
  - **The launch checks** (`oracle.py launch`, the oracle README's "The launch checks"): the oracle walks the field
    menu with an input file from a save; the port plays the same file with `--launch` at the counts the oracle opens
    each app (its first frame on the oracle less the passes the port itself takes to it, so that frame's timing is
    aligned by construction, not checked), and must show every app screen of the route's table at its count, and the
    oracle's picture at every count of each app. Two routes for Black 2, their input files and tables (hashes only)
    in `port/tests/oracle`.
  - **What the field leaves**, measured on the oracle: the UI's touch flag 0 and the last subscreen 1 at an app's first
    frame; the launcher sets both through the game's setters. Nothing else was needed for the apps' pictures.
  - Found on the way: the H-blank interrupt's timing (`0eb7214e2`), which hid the card's opening wipe.
  - White 2's starter save: the Black 2 route's input, run on White 2's first save, goes astray (the player ends in
    front of the Trainers' School with no Pokémon; the save has 0x40a1 = 1), so White 2 is left out of the starter
    save and the launch routes until it has a route of its own.
- **Next:** the user's decision on ADR 0012; White 2's routes; the apps the routes don't reach yet (options, the town
  map, the Xtransceiver, the medal box); battles, which wait on overlay 36 and `data_ov167_021d7490`.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim): the field, the field menu, SAVE and Retire
  from it, and every battle event. `NNSi_objSizeWTbl` and `NNSi_objSizeHTbl` (ARM9 main, NitroSystem G2D, no overlay
  issue and so no claim): the Pokédex, and any flipped or affine cell. `data_ov167_021d7490` (overlay 167, issue #120):
  the first move of any battle. Overlay 12's script data, as before.
## 2026-10-18: Frigate's test launcher for the field menu's apps (ADR 0012, proposed), on a branch of its own

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `b8b52f420`, on the branch `port-launch-adr0012`, after `cc281af40` (`port-launch`). The branch waits for
  the user's decision on ADR 0012 and isn't pushed.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `src/ov035/event_mapchange.c` gets a guard-only
  block (a Platform row of `game-changes.md`): `check_mwcc_neutral.py` finds 3,142 of 3,146 file and version pairs
  identical to `045e97b90`, this file among them, the other four being `src/system/vm.c`'s and
  `src/ov012/event_field_proclink.c`'s Neutral rows.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** measured on this code as first committed (`9be56a819`, the same code with port/CLAUDE.md's and PLAN.md's
  lines about the launcher): T0 as above. Black 2: all 245 `ctest` tests pass, the oracle's included, 4 new:
  `app_launcher_spec_plain` and `_sanitized` (the option's parsing, and the gate on a made-up table of placeholders),
  `integrate_launcher_off` (without `--launch` the seam returns 0 before touching its event) and
  `integrate_launch_bad_option`; `app_run_dir` checks run.txt's launch line. White 2: all 243 pass but
  `oracle_continue_starter`, which skips (no starter save for White 2). Without `--launch` nothing changes:
  `oracle_continue` stops on `FIELDMAP_PROC_FUNCS` at the same frame with the same screens, and the replays pass. By
  hand, `--launch bag` from the first save shows the oracle's empty bag. T3's long runs, T5's nightly runs and T7 not
  yet.
- **Tiers:** not measured: no decomp C changed but the guard-only block.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version. A continued game without
  `--launch` still stops on `FIELDMAP_PROC_FUNCS` (overlay 36).
- **64-bit warnings:** unchanged: the game change is a block MWCC never sees.
- **Work done:** Frigate's test launcher ([ADR 0012](adr/0012-test-launcher-for-the-field-menus-apps.md), proposed;
  `port/app/launcher.c`, `launcher_spec.c`): `--launch APP[@COUNT],...` opens the field menu's apps (the party list,
  the Pokédex, the bag, the trainer card, the summary) after CONTINUE, in place of the field, whose process table
  (overlay 36) has no C and is under another contributor's claim. It runs the app call's field-free states through
  the game's own table and functions, and reports each app's result; it steps aside once `FIELDMAP_PROC_FUNCS` has C.
  The seam is a guard-only block in `EventFieldContinue_Callback`'s state 2. Neither port/CLAUDE.md nor PLAN.md names
  the launcher until the user accepts the ADR (its "On acceptance").
- **Next:** the apps against melonDS through the launcher (the next entry).
- **Blockers:** the user's decision on ADR 0012; overlay 36 (issue #83, another contributor's active claim) and
  overlay 12's script data, as in the last entry.

## 2026-10-18: after review: the commits that stand alone, without the test launcher; the H-blank cycle's source

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `cc281af40`, on the branch `port-launch`, after `0eb7214e2`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 241 `ctest` tests pass, the oracle's included (`ctest -j4`, 977 s on a loaded
  machine): `oracle_opening`, `oracle_title`, `oracle_continue`, `oracle_continue_starter` and `oracle_port_save` among
  them, a continued game still stopping on `FIELDMAP_PROC_FUNCS`. White 2: rebuilt (`core.c`'s comment is shared code);
  its 60 OS, card, infrared, layout and `replay_same_save` tests pass, `os_hblank_flag_in_handler` among them. T3's long
  runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version.
- **64-bit warnings:** unchanged: no game file changed.
- **Work done:**
  - **The test launcher waits for its ADR.** The design asked for the user's acceptance of ADR 0012, which reads
    port/CLAUDE.md's exception for a deferred subsystem's documented stand-in, before the launcher landed; its three
    commits had landed here anyway, with a port/CLAUDE.md sentence naming it as a documented stand-in. They now wait on
    the branch `port-launch-adr0012`, which isn't pushed, and this branch ends with the commits that stand alone: the
    infrared chip, the trainer card's param, the saves made from saves and the H-blank fix.
  - The entries of those commits no longer lean on the launcher: the H-blank entry says the bug was found by comparing
    every pass of the card's opening with the oracle's, and their "Next" lines name the apps, not the launcher.
  - **The H-blank cycle's source** (`port/hal/os/core.c`, `lessons.md`): GBATEK's 1606 cycles into the line, for the
    interrupt and the flag; melonDS 1.1 takes both at 1584 (`HBLANK_CYCLES`), 22 cycles earlier. What the game sees,
    the two together, holds at either, so the port keeps GBATEK's.
  - `oracle.py`'s docstring had two lines over 120 columns; `game-changes.md`'s trainer card row names its commit,
    `014fe956c`.
- **Next:** the field menu's apps on the port while overlay 36 is claimed, by ADR 0012's launcher if the user accepts
  it; White 2's starter route.
- **Blockers:** the user's decision on ADR 0012 (a test launcher for the field menu's apps, on the branch
  `port-launch-adr0012`); overlay 36 (issue #83, another contributor's active claim) and overlay 12's script data, as
  in the last entry.

## 2026-10-18: the H-blank interrupt when DISPSTAT shows the H-blank: the screen wipes that cover lines one by one

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `0eb7214e2`, on the branch `port-launch`, after `f7aa43045`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 241 `ctest` tests pass, the oracle's included (424 s), 1 new:
  `os_hblank_flag_in_handler` (an H-blank handler reads DISPSTAT's H-blank flag set: 192 of 192 lines; 0 of 192 with
  the interrupt at its old cycle, 1536). White 2: all 239 pass, the oracle's included (382 s), but
  `oracle_continue_starter`, which skips (no starter save for White 2 yet). T4 unchanged: `oracle_opening`,
  `oracle_title`, `oracle_continue`, `oracle_continue_starter` and `oracle_port_save` pass in Black 2, and the first
  four's White 2 counterparts; the replays pass. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no decomp C changed.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version.
- **64-bit warnings:** unchanged: no game file changed.
- **Work done:** the port's OS core took the H-blank interrupt 1536 cycles into a line and set DISPSTAT's H-blank flag
  at 1606 (`port/hal/os/core.c`); its handlers take no time, so a handler read the flag clear. melonDS raises the
  interrupt as it sets the flag (`GPU::StartHBlank`), and the screen wipes' H-blank function writes the window planes
  only while `GX_IsHBlank()` (`src/system/wipe_sub.c`'s `WipeLineWnd_HBlank`), so the wipes that cover or uncover the
  screens line by line were never drawn: the trainer card's opening went from black to the card at once, where the DS
  reveals its top screen and then its bottom one 32 lines a pass. The interrupt is now taken at 1606, with the flag.
  Found by comparing every pass of the trainer card's opening with the oracle's: with the change, the card's opening
  on the port is the oracle's pass for pass (14 passes of the two wipes, by their hashes). The cycle is GBATEK's;
  melonDS 1.1 takes both at 1584, and what the game sees, the two together, holds at either.
- **Next:** as in the last entry.
- **Blockers:** as in the last entry.

## 2026-10-18: saves made from saves (the first Pokémon), and the start menu's Pokédex line against the oracle

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `f7aa43045`, on the branch `port-launch`, after `014fe956c` (the trainer card's param from its header).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`), after each of the two commits. `014fe956c` changed
  `src/ov012/event_field_proclink.c`, complete in both versions, so the ROM build is its proof (a Neutral row in
  `game-changes.md`); this commit changes no game file.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 240 `ctest` tests pass, the oracle's included (`ctest -j4`, 465 s), 3 new:
  `layout_trainer_card_param` (`014fe956c`: the trainer card's param has the same layout in the app call's object as
  in the card's, by their debug information; it fails on the old objects, 0x14 and 0x18 against 0x20 and 0x24),
  `oracle_saves` (makes `first_save` and `starter_save` into `<build>/oracle-saves`, or reuses them while their SHA-1s
  are the ones kept: 0.5 s with both cached) and `oracle_continue_starter`; `oracle_compare` has 43 tests (2 new: saves
  made from saves and the cache, with a stand-in oracle; a sequence whose save a version doesn't keep is skipped; the
  declarations extended to chains). T4: `oracle_continue_starter` 3,415 of 3,415 compared frames match (1,310 exact,
  2,105 with 3D within the tolerance), with continue's known differences: the start menu with the starter save's
  saved game, its Pokédex line (Pokédex: 1, time 0:04) among it, exact at the DS's 6 bits; the port then stops on
  `FIELDMAP_PROC_FUNCS` after frame 3,436, as from the first save. White 2: `oracle_compare`, `oracle_saves` (which
  made White 2's `first_save` from a blank backup, 97 s, and left `starter_save` out: no SHA-1 for White 2 yet),
  `oracle_continue` pass, `oracle_continue_starter` skips with that reason; `layout_trainer_card_param` and the card
  and backup tests pass. The saves on the oracle: `first_save` `a7a58889e239`, `starter_save` `ce71fe0b8c1d` (three
  runs, the research's two and this one; Tepig, level 5, 135 experience, by a decoder of Gen 5's encoding, its checksum
  valid; event flags 0x961 and 0x962 set, variables 0x40a1 = 4 and 0x4030 = 1; 2,086 bytes from `first_save`) and
  `route19_save` `826bc561a328` (two runs; Tepig, level 6, 229 experience). T3's long runs, T5's nightly runs and T7
  not yet.
- **Tiers:** not measured: no decomp C changed beyond one file's include.
- **Trap stubs:** unchanged, 624 function stubs and 48 data placeholders in each version.
- **64-bit warnings:** unchanged: the game change removes a typedef and adds an include.
- **Work done:**
  - **The trainer card's param** (`014fe956c`, Neutral): `src/ov012/event_field_proclink.c` declared its own
    `TrainerCardParam`, the same as the header's on the DS but not on a host, where the card's result was read from
    `gameData`'s high half and `appParam` written over `canEdit`. It includes `include/field/trcard_sys.h` now.
  - **Saves made from saves** (`oracle.py`): a save in `SAVES` names the save it starts from; each step of a chain is
    made in order and held to its SHA-1. `--save-cache DIR` keeps the saves as `NAME_VERSION.sav` and reuses one only
    while its SHA-1 is the one kept. Two new saves, Black 2: `starter_save` (from `first_save`, 17,000 frames: the
    story to the Aspertia City lookout, Tepig and the Pokédex, then SAVE) and `route19_save` (from `starter_save`,
    28,600 frames: Hugh's battle and the scenes to Route 19's grass, for the battles, run by no test). Their input
    files are in `port/tests/oracle`; the saves are made at test time and never committed.
  - **continue_starter**: continue's presses from the starter save (`Sequence.input` names the sequence whose input
    file a sequence plays), a check of the start menu's Pokédex line, which the last entries named as next.
- **Next:** the field menu's apps on the port while overlay 36 is claimed, which needs a way to open them without the
  field and the user's decision on it, and White 2's starter save.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) and overlay 12's script data, as in the last
  entry.

## 2026-10-18: the card's infrared chip on the SPI registers: the genuine-card check passes as on melonDS

- **Upstream:** `045e97b90` (PR #364), merged by `5ad9a1e1d`; no sync, this task doesn't push.
- **Port:** `1ba3ab56f`, on the branch `port-launch` from `5ad9a1e1d`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed, so no
  `check_mwcc_neutral.py` run and no row in `game-changes.md`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2. Not pushed, so no CI run.
- **Tests:** T0 as above. Black 2: all 237 `ctest` tests pass, the oracle's included (`ctest -j4`, 419 s), 3 of them
  new: `hal_card_spi_plain` and `hal_card_spi_sanitized` (the model against melonDS 1.1's `NDSCartSlot` and
  `CartRetailIR` functions and GBATEK: the genuine check's two exchanges, receive's length 0, send, an unknown command,
  the slot or its SPI bus off, the deselect when bit 13 is cleared while held, the busy bit, a byte sent equal to the
  last answer, the low byte), and `rom_game_ir_genuine_card` (the game's own `IrCheck_IsGenuineCard` on the whole
  program's objects returns TRUE twice, as the check passes on melonDS). Mutation: without the two lines of
  `HW_REGISTER_POLICY`, `rom_game_ir_genuine_card` fails. T4 unchanged: `oracle_continue`, `oracle_opening`,
  `oracle_title` and `oracle_port_save` pass, as do `replay_new_game`, `replay_start_menu` and `replay_same_save`.
  By hand, Black 2 under lldb from the oracle's first save through CONTINUE to the field's trap: `func_0202d6a8` and
  `IrCheck_IsGenuineCard` run once, `GameBeaconSys_HideNewHBlank` never (it ran from the HBlank task list before), and
  the save is unchanged. White 2: the 16 `card_backup` tests, `hal_card_spi_*`, `rom_game_ir_genuine_card`,
  `smoke_reaches_game_main` and `replay_same_save` pass; `oracle.py sequence continue` 3,407 of 3,407 (1,312 exact,
  2,095 with 3D within the tolerance), as in the last entry. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: this task changed no decomp C.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version, against the last entry's 574 and 63:
  the change is upstream's, merged by `5ad9a1e1d`, which had no entry. A continued game still stops on
  `FIELDMAP_PROC_FUNCS` (overlay 36).
- **64-bit warnings:** unchanged: no game file changed.
- **Work done:**
  - **The card's infrared chip** ([ADR 0011](adr/0011-cartridge-infrared-chip-as-a-register-model.md),
    `port/hal/card/card_spi.c`). AUXSPICNT and AUXSPIDATA were plain memory, so AUXSPIDATA read back the last byte
    written, 0, and the game's genuine-card check (`src/ov338/ir_check.c`) failed at every CONTINUE and NEW GAME:
    the game then left `GameBeaconSys_HideNewHBlank` in the HBlank task list for good, and would skip the experience
    of a faint in battle. The two registers are now accessors (`port_hw_auxspicnt`, `port_hw_auxspidata`) that model
    the bus and the chip as melonDS 1.1 does, so the game's own C runs and passes, and the infrared library
    (`lib/irc`, overlay 31) has a chip to talk to; PLAN.md §4.5's stub of the check is dropped. An accessor can't tell
    a load from a store, so each call resolves the store made through the memory the previous call returned:
    AUXSPIDATA's cell keeps a marker in its upper byte that a store clears, a documented deviation (a 16-bit read sees
    it where the hardware gives 0; every reader keeps the low byte).
  - `decomp-findings.md`: `ir_check.c` runs on the port, equivalent as observed; `buildSeed` hashes its digest buffer
    before writing it (stack bytes on the DS, zero on a host), so the boot's and each battle's random numbers can't
    match the oracle's.
- **Next:** the field menu's apps on the port while overlay 36 is claimed: the trainer card's param layout, saves with
  a party made by the oracle, and a way to open the apps without the field, which needs the user's decision.
- **Blockers:** overlay 36 (issue #83, another contributor's active claim) and overlay 12's script data, as in the last
  entry.

## 2026-10-18: Frigate and the sound, merged and reviewed: the sound track under Frigate's names and guides

- **Upstream:** `cf8ded4c2` (merge of PR #367), as merged into the port's `main` by `65ea798fb`; no sync in this task.
- **Port:** `e7187b168`, on the branch `port-integrate` from `main` at `65ea798fb`: `a389e6d9b` (the merge of
  `frigate-identity` after the sound's branch, `a8beb72c5`, and the upstream sync `65ea798fb`), then `bd915af7b` (the
  sound track under Frigate's rules), then the fixes from the review of those two; not pushed yet.
- **ROM:** both SHA1s match (`python3 configure.py && ninja`), at the merge, at `bd915af7b` and at this commit. Nothing
  outside `port/` changed but the port's own `.github/README.md` and `.github/workflows/port-*.yml` (the workflows named
  "Frigate CI" and "Frigate upstream sync", and CI's app target `frigate-black2_app`), so no game-changes row and no
  `check_mwcc_neutral.py` run.
- **Host build:** arm64 macOS, Black 2 and White 2 RelWithDebInfo, in the tree's build directories, configured again
  (their cached placeholder bundle-identifier prefix moved to Frigate's by `bundle_id.cmake`). The merge rebuilt all of
  the decomp's C: 263 warnings in each version, all in its `src/` (193) and `lib/` (70), none in `port/`, as in
  Frigate's entries; `bd915af7b`'s changes and the review's, comments only in C and CMake, rebuilt with none. Not
  pushed, so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6, with the oracle built from the sound branch's sources in each build's `oracle/`, so none of the
    melonDS oracle's tests skips: at the merge, Black 2, 248 of 249 `ctest` tests pass and `asm_oracle` skips (462 s);
    White 2, all 247 pass (472 s), none skipped. At `bd915af7b`, the same: Black 2, 248 of 249 pass and `asm_oracle`
    skips (600 s); White 2, all 247 pass (606 s), none skipped. After the review's fixes, the same again: Black 2, 248
    of 249 pass and `asm_oracle` skips (551 s); White 2, all 247 pass (558 s), none skipped. Each time the two builds
    ran at once, `ctest -j4` each. No test's checks changed, only their files' comments. `asm_oracle` skips in Black 2's
    build because that directory was made with Unix Makefiles and the test reads its flags from a Ninja build (the tests
    guide says so now); run on its own with the flags of a Black 2 directory configured with Ninja for it (and deleted
    after), its four test files pass, at the merge and after the review; in White 2's build it runs and passes.
  - The apps: `package_macos.sh` makes `Frigate Black 2.app` and `Frigate White 2.app` and their disk images, `Frigate
    Black 2 0.0.2644.dmg` and `Frigate White 2 0.0.2644.dmg` (5.5 MB each), from the merge's tree (revision
    `a389e6d9b2df-dirty`), and after the review `Frigate Black 2 0.0.2645.dmg` and `Frigate White 2 0.0.2645.dmg` (5.5
    MB each), from this commit's tree before this entry (revision `bd915af7bde0-dirty`), which the checks below passed
    the same way both times. `plutil` reads `CFBundleName` Frigate, `CFBundleDisplayName` "Frigate Black 2" and "Frigate
    White 2", `io.github.callancampos.frigate.black2` and `.white2`, `CFBundleExecutable` `frigate-black2` and
    `frigate-white2` and `CFBundleIconFile` AppIcon; `plutil -lint` passes, and so does `codesign --verify --deep
    --strict` on each app and on the app in its mounted image, which holds the app and a link to `/Applications`. Each
    app's program runs 300 frames headless (`--save-dir` a scratch directory), status 0, and its `--help` opens with the
    name, the description and the disclaimer; Black 2's, with `--sound-trace`, writes a `pbwsnd 1` trace of 935 records.
  - Every relative link of the Markdown files under `port/` and `.github/README.md` resolves, anchors as GitHub makes
    them included (56 files, 1033 links after the review, checked by script). clang-format 23.1.3 is clean on every C
    file changed since `65ea798fb` but four frozen phase-0 spikes whose header comments Frigate's comment pass rewrote:
    `port/spikes/os-fibers`'s `src/hal_fiber.c`, `src/os_host.c` and `tests/test_os.c` and
    `port/spikes/overlay-sections/ovsec/ovsec.c` keep the formatting they had at `65ea798fb` (1, 8, 46 and 39
    differences, the same counts), since the spikes are frozen records.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no game code changed in this task.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version (`gen_stubs`, each build above), as in
  Frigate's entries; the sound's entry counted 56 placeholders at its older upstream base (`aae2a03f6`).
- **64-bit warnings:** not measured: no game or `lib/` code changed in this task.
- **Work done:** Frigate's branch, which never saw the sound track, merged into `main` after it (`a389e6d9b`), the
  sound track brought under Frigate's rules (`bd915af7b`), and the review's findings fixed in this commit.
  - **The merge.** Four conflicts, each kept with both sides' meaning: `main.c`'s usage comment (Frigate's program and
    `--help` with the sound's `--sound-trace`), `app.cmake`'s header (the window's icon and the sound driver's trace),
    the oracle README's commands (the sound's on `frigate-black2`), and this file (Frigate's four entries above the
    sound's, the last naming its commit and the merge).
  - **Names.** After the merge the sound track's text had none of the old names but its two `oracle.py sound` commands
    on `build/port/pokeblack2`, which the merge wrote on `frigate-black2`. Where its headers meant the product, they say
    Frigate: `sound.py` ("Frigate's sound against the melonDS oracle"), `sndtrace.h` (Frigate writes the trace too and
    includes the header) and `oracle.cmake` (the oracle is never part of Frigate); `oracle.py`'s summary names
    `sound.py` beside `compare.py`. The layer's own names stay, as port/CLAUDE.md says (`port_`, `PORT_*`,
    `pbw-oracle`), and so does the trace format's `pbwsnd`, named as `pbwinput` is.
  - **Headers.** Every file the sound track added or changed opens with what it does and which part of the game it
    touches: `port/app/sound_trace.c` (one line before) and `.h`, `sndtrace.h`, `sound.py`, `oracle.cpp` (its
    `--audio-dump` and `--sound-trace`), `port/hal/snd7`'s changed files (the music, sound effects and cries they play,
    and the trace), `port_os.h` (timed work), `test_snd.c`, `test_os.c`, `test_sound.py`, `sound.cmake` and
    `oracle.cmake`. `snd7.c`'s `Execute`, the command switch that Frigate's comment pass left to the sound track, has
    its comment.
  - **Guides.** `port/app/README.md` has `sound_trace.c`'s row; `port/hal/README.md` the OS core's timed work (the
    module's row, `port_os.h`, `core.c`) and the sound driver's alarms, start delays, FIFO and trace (the module's row,
    `snd7.c`, `snd7_hw.c` and `snd7_output.h`, with the `oracle_sound_*` tests); `port/cmake/README.md`,
    `port/tools/README.md`, the oracle README's opening and `port/tests/README.md` (the oracle's, the OS's and the
    sound's rows) the sound comparison. ARCHITECTURE.md: the frame's clock runs timed work (the sound driver's next
    alarm), the sound driver's row in the threads table, the sound trace among what a run closes and in status 1, and
    the feature map's sound row: the driver held to melonDS run by run, with `sound_trace.c`, `sound.py` and
    `sndtrace.h` and the tests that cover it. GLOSSARY.md has "Timed work", "Periodic run, play" and "Sound trace";
    MODDING.md's oracle paragraph, port/README.md's and the landing page's status say the sound is held to melonDS too.
    No document was added, so the documents' index is unchanged.
  - **The review: what the sound is held to.** port/README.md's and the landing page's status, MODDING.md's oracle
    paragraph, `oracle.cmake`, `snd7_hw.c` and the feature map's sound row said that the sound's output is held to
    melonDS's sample by sample. Only the title's stream and Kyurem's cry are, the segments that `sound.py` compares
    exactly; the music is held by its envelope and levels, since its notes start 1 to 5 samples apart. They say so
    now, and the feature map's "at each frame on the game's count" names the title's frames the comparison leaves out
    (the DS's long load passes and the cry).
  - **The review: records.** The merge had written into Frigate's last entry that the developer settled the platform
    rule, and taken the decision out of its Blockers, though nothing records such a decision; that entry is as its
    branch wrote it again, and the decision is the user's (Blockers). "The port is Frigate"'s Next said "as the last
    entry", which the merge made the sound's: it names the saves' entries below; and the sound's own Next, written on
    its branch against the gamepads entry, names that entry. This entry's ROM line names the port workflows, and its
    clang-format line says which files it covers.
  - **The review: guides.** The oracle README names `sndtrace.h` beside `pbwinput.c` as the plain C11 of the oracle's
    directory that Frigate includes, with who includes it (`snd7.c`'s `port_snd7_trace_record`, `sound_trace.c`,
    `test_snd.c`), as the tools guide does, and its file table has `oracle.h`. port/audio's guide, `audio.c` and
    `audio.h` say that the samples are made as the virtual clock passes them (at each VBlank, at the ARM9's words and at
    the driver's alarms) and handed over at each VBlank. `main.c`'s header lists status 1. The tests guide,
    `hal_tests.cmake` and the cmake guide say that `asm_oracle` needs a Ninja build directory. The sound trace is in
    port/app's line of the directory map, of the layer diagram and of the layers table, and in the cmake guide's
    `app.cmake` row. GLOSSARY.md and `snd7.c` give the periodic runs as about 192 a second (33,514,000 / (2728 x 64) =
    191.96), not 191.9.
  - **The review: every script's header.** `port/tools/progress/` and `port/tools/research/` have guides with a row per
    script, the compile probe's guide a row per script, the asm oracle's its test files, the vram-alias spike's its four
    Windows stand-ins and its CMakeLists, and os-fibers' its CMakeLists; port/README.md's directory map and the tools
    guide point to them. Every script of `progress/`, `research/` and `compile-probe/` opens with what it measures,
    which part of the game or the DS it serves, what it reads and which research report it fed, as `datasyms.py` did;
    `os_internal.h` says what the OS module's files share (the DTCM, the contexts' side table, the interrupt controller
    and timers on the virtual clock); the compile probe's `port_shim.h` points to `port/include/port_host.h` in place of
    "a real port would"; and `check_sections.py`'s frozen note is in its docstring.
  - **The platform rule, settled at publishing.** It stays as built, which closes the decision the entries below and
    this entry's review left open: the host's types are defined in `include/types.h`'s `PLATFORM_PC` block, a port
    change with its game-changes row since phase 1, and other headers branch on `PLATFORM_PC` themselves. A separate
    `include/platform.h` would add a second file outside `port/` for what that block already holds, so PLAN.md §4.8's
    header is not made; the rules already say so. The VRAM backends' headers (`vram_os.h`, `vram_os.c`) say which
    graphics they serve, the one header the confirmation found short.
- **Next:** as the last entries: a save with an adventure made by the port, once overlay 36 lets it into the field; for
  the sound, a comparison sequence where a player plays on the game's count. The launcher's modules get their rows in
  the guides and the feature map when they merge.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.

## 2026-10-18: Frigate's identity and guides, reviewed and fixed

- **Upstream:** `045e97b90` (merge of PR #364), as in the last entry; no sync in this task.
- **Port:** `094422f4b`, on the branch `frigate-identity`, after `aad2ce8df`, merged into `main` after the sound's
  branch (`a8beb72c5`) and the upstream sync `65ea798fb`: `de76726d2` (the review's fixes), then `483392fa4` (the
  logo's ship redrawn as the Plasma Frigate), then `7b9db5d77` (the last confirmation's doc notes), then the fixes from
  the review of those two in `094422f4b`.
- **ROM:** both SHA1s match (`python3 configure.py && ninja`). Nothing outside `port/` changed but the port's own
  `.github/README.md`, so no game-changes row.
- **Host build:** arm64 macOS, Black 2 and White 2 RelWithDebInfo, from clean build directories: 263 warnings in each
  version, all in the decomp's `src/` (193) and `lib/` (70), none in `port/`, unchanged from the last entry. Not
  pushed, so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6: Black 2, all 239 `ctest` tests pass (`ctest -j4`, the oracle's included), none skipped; White 2,
    all 237, none skipped. New: `app_about` (`--help` says the product's name, description and disclaimer
    exactly as the build read them from `port_app.h`) and `app_bundle_id` (the bundle identifier's prefix, configured
    alone: Frigate's by default, an old build directory's placeholder moved to it, a builder's own kept). Extended:
    `app_make_app` (the About text with XML's and sed's special characters, a `com.example` identifier refused, the
    `.icns`'s 16 and 32 px images the drawn ones), `app_window_icon_*` (the 32 px bitmap the drawn window icon pixel
    for pixel, the 64 px one its alternate) and `assets_branding` (the twelve SVGs, and each small PNG's ship pixel for
    pixel). `app_bundle_id` fails without the move (3 of its 12 checks), `app_about` on a wrong expectation, and
    `assets_branding` on a 16 px image scaled from the master (2 checks).
  - Every relative link of the Markdown files under `port/` and `.github/README.md` resolves (checked by script).
    clang-format 23.1.3 is clean on every changed C file. Black 2's app, built again from this tree, reads back
    `CFBundleName` Frigate, `CFBundleDisplayName` "Frigate Black 2", `io.github.callancampos.frigate.black2` and the
    About text from `port_app.h`, passes `plutil -lint` and `codesign --verify --deep --strict`, and its `.icns` holds
    the ten images.
  - After the ship's redraw, from clean build directories: Black 2, all 239 `ctest` tests pass again, White 2, all
    237, none skipped; in each, `assets_branding` (469 checks), `app_make_app` (47) and `app_window_icon_plain` and
    `_sanitized` (32 each). Both apps, built again with `package_macos.sh`, carry the new ship: their `.icns`, unpacked
    with `iconutil`, holds the drawn 16 and 32 px images and the master scaled above them. Every new PNG was looked at:
    the 16 and 32 px images enlarged, the 1024 px masters, and the logos on light and dark.
  - After the redrawn ship's review fixes, in the same build directories: Black 2, all 239 `ctest` tests pass (402 s),
    White 2, all 237 (336 s), none skipped; in each, `assets_branding` (475 checks: the new clearance check, 12, fails
    8 of them on the art as it was), `app_make_app` (47) and `app_window_icon_plain` and `_sanitized` (32 each). Every
    small PNG was looked at enlarged and at its size on light and dark, beside the art as it was; the masters and the
    logos render byte for byte as before. Both SHA1s match again, and every relative link of the changed Markdown
    files resolves.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no game code changed.
- **Trap stubs:** 624 function stubs in each version, as in the last entry.
- **64-bit warnings:** unchanged: no game or `lib/` code changed.
- **Work done:** two reviews of the rename, the artwork and the guides, and a fix for each finding they confirmed.
  - **Names written once.** The product's name, description and disclaimer are `PORT_PRODUCT_NAME`,
    `PORT_PRODUCT_DESCRIPTION` and `PORT_PRODUCT_DISCLAIMER` in `port/hal/os/port_app.h`, which `port/CMakeLists.txt`
    now reads (`port_product_string`) for `project()`, the macOS app's names and its About window (Info.plist's
    `@ABOUT@`, filled by `make_app.sh --about`, which now escapes its text fields for XML and sed). `--help` prints the
    same macros. port/CLAUDE.md's rule says where each name is set.
  - **Old build directories.** A build directory configured before the rename cached the placeholder bundle-identifier
    prefix `com.example.pokebw2-native`, which the new default never replaced: configuring it again now moves it to
    `io.github.callancampos.frigate` with a message (`port/app/macos/bundle_id.cmake`), and `make_app.sh` refuses any
    `com.example` identifier. A prefix of one's own is kept.
  - **Small icons.** The icons' 16 and 32 px images are drawn pixel by pixel in `frigate_logo.py` (`SHIP_16`,
    `SHIP_32`), with a 35% ink edge on White 2's plate. The `.icns` takes them for its 16 and 32 px images
    (`make_icns.sh MASTER SMALL16 SMALL32 OUT`); the window on Linux and Windows gets the 32 px one drawn with its plate
    filling the square, and the master at 64 px as its alternate for 200% (`gen_window_icon WINDOW MASTER OUT`,
    `SDL_AddSurfaceAlternateImage`). Twelve new files in `port/assets/branding`: six SVGs and their PNGs.
  - **The ship is the Plasma Frigate.** The user asked for the logo's ship to be redrawn as the games' own Plasma
    Frigate, the ship Frigate is named after, keeping the style, and accepted the design: one main mast with its square
    sails as a grid of three tiers of four panels, two swept jibs over a long ram, a tall sail at the stern, a domed
    tower on the poop, a fin under the bow, and an outrigger pontoon on struts, in the same palette, plates and
    wordmark (`frigate_logo.py`, and every SVG and PNG rendered again by `render.py`; the lockup keeps its 1200 x 333).
    The 16 and 32 px images are drawn again for it, pixel by pixel: at 32 px the sails as twelve panels two pixels
    wide, the mast and pennant, the stern sail and the tower, the jibs over the ram, the hull with five ports and the
    pontoon on three struts; at 16 px the sails as one tall block of two columns under the truck, the stern sail, the
    jib, the ram, the hull and the pontoon on two struts. The branding README says what the ship depicts and that its
    design belongs to Nintendo, Game Freak, Creatures and The Pokémon Company, in place of the claim that nothing in it
    came from the game; the landing page's license line, port/README.md's icon, the assets guide and port/CLAUDE.md's
    artwork rule say the same, and `make_icns.sh`'s comment no longer speaks of masts.
  - **The guides, corrected:** the frame's first step (the game puts its sound loader to sleep for the VBlank wait),
    the boot's `port_hw_init`, SDL's audio thread, the exit statuses (1, signals, 71 from a DMA misuse), the boundary
    rules (where `replaced.txt` lists what, what the asm oracle covers), the feature map (the touch tests, the font
    path, the file chain, the window and its icon, and every module in a row: `hal/crypto`, `hal/ctrdg`, `hal/trap.c`,
    `gen_stubs.py`, `gen_host_tree.py`), the hal guide's file counts, the "settings file" wording, the glossary's
    "upstream", and the "Plain C types only" marker on the three headers `port/app` includes that lacked it.
  - **The guides, extended:** MODDING.md gains "When the game stops" (each way a run ends, the trap messages, and what
    to do about a trap), the bag as a worked example of finding code, and the files a widescreen mod touches.
    port/hal/README.md lists every file of every module. The compile probe's `overlay.h` shim has its header, and its
    README lists its files. README says the window can be resized.
  - **The platform rule, as built.** port/CLAUDE.md's rule 3, PLAN.md §4.8's first rule and game-changes.md (the third
    kind and header changes) named `include/platform.h`, which was never made, as the one header that decides between
    MWCC and a host. They now say what was built: the host's types are defined in one place, `include/types.h`'s
    `PLATFORM_PC` block (`uptr`, `sptr`, `ulong32` and `PORT_DS_ALIGN_64`, with its game-changes rows); any other
    definition that must differ (`OVERLAY_ID` in `include/gfl/overlay.h`, `HeapBlockHeader`'s padding, NitroSDK's
    `math.h` and `snd.h`, DS Protect's checked calls) branches on `PLATFORM_PC` in its own header, with its row; and
    `PLATFORM_PC` itself is defined only by the host build (`port_decomp_env` in `port/CMakeLists.txt`), never by a
    header, with `port/include/port_host.h` force-included. PLAN.md's phase 0 list and the compile-probe research,
    which proposed the header, note that it was built as that block (on 2026-10-17); MODDING.md says the same in place
    of its note that the header was missing; and PLAN.md's libc rule (§4.4, §4.8 rule 8) says what replaced the
    "platform switch" for the C library headers: the host build's generated `include/` mirror leaves them out. This
    task reworded the rule to match the code; the user has not chosen between it and creating the header (Blockers).
  - **Doc notes:** ARCHITECTURE.md's build paragraph and boot step 2, MODDING.md's trap paragraph, a paragraph of
    port/CLAUDE.md and two module lines of port/hal/README.md wrapped at 120 columns, and README's directory map says
    what the guides list: every file in most, and in `tests/` and `tools/` groups of files whose own headers say the
    rest.
  - **The redrawn ship's review.** The 16 px ship filled the plate's whole interior, painting over White 2's edge on
    all four sides and touching Black 2's shaded rim, and its sails, stern sail and jib, all 2 px blocks at a 3 px
    pitch split by the gaps between the tiers, read as a grid of squares. It is drawn again inside the edge (10 x 10,
    rows and columns 3 to 12): the sails one tall block of two columns under the truck, with no rows between the tiers,
    a stern sail a pixel wide, the jib a wedge over the ram, two rows of hull, and the pontoon on two struts. The 32 px
    ship, 24 pixels wide, touched White 2's edge with its stern sail and its ram: it is 22 wide now (columns 5 to 26),
    its stern sail a column narrower and its ram a pixel shorter. White 2's small edge is drawn as a filled ring in
    place of a one-pixel stroke, which AppKit drew with a notch on the window icon's right side (row 10). The branding
    README's strake sentence says which hull cuts what (White 2's black hull its six runs, Black 2's white hull its
    five ports), as the generator does. `assets_branding` checks that every small image's ship clears the plate's
    outermost pixel.
- **Next:** as the last entry: a save with an adventure made by the port, once overlay 36 lets it into the field. The
  sound driver's and the launcher's modules get their rows in the guides and the feature map when they merge.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7. A decision for the user: port/CLAUDE.md's rule 3, PLAN.md §4.8 and game-changes.md now describe
  the platform rule as built (the host's types in `include/types.h`'s `PLATFORM_PC` block, other headers branching on
  `PLATFORM_PC` themselves) in place of naming `include/platform.h`, which was never made. This task made the change
  so that the rules say what is so; keeping that wording or creating the header instead is the user's call.

## 2026-10-18: Frigate's code mapped for modders

- **Upstream:** `045e97b90` (merge of PR #364), as in the last entry; no sync in this task.
- **Port:** `260640953` (the files' comments) and `aad2ce8df` (the guides, with this entry), on the branch
  `frigate-identity`, after `687134df4`.
- **ROM:** both SHA1s match (`python3 configure.py && ninja`). Nothing outside `port/` changed but the port's own
  `.github/README.md`, so no game-changes row and no `check_mwcc_neutral.py` run.
- **Host build:** arm64 macOS, Black 2 and White 2 RelWithDebInfo, in the last entry's build directories. The comment
  of `port_host.h`, which every decomp file includes, recompiled all of the decomp's C: no warning in port code; the
  decomp's own files give 263, the same in each version (`-Wswitch` and the like), which only a full rebuild of them
  shows. Not pushed, so no CI run.
- **Tests:** T0 as above.
  - T2, T3, T4, T6: Black 2, all 237 `ctest` tests pass (`ctest -j4`, 369 s, the oracle's included), none skipped;
    White 2, all 235 (360 s), none skipped. No test changed.
  - The documents: every relative link of the Markdown files this task touched resolves, anchors as GitHub makes
    them included, and every test name, repository path and file name the new documents cite exists (checked by
    script). The code changes are comments only (every changed line of C and CMake is a comment; the Python files
    only gained docstrings and still parse). clang-format 23.1.3 is clean on every changed C file but four spike
    files, which were not formatted before and have the same count of differences after.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no game code changed.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version, as in the last entry.
- **64-bit warnings:** unchanged: no game or `lib/` code changed.
- **Work done:** the user asked that Frigate's code be easy for modders and new developers to find their way in; this
  task maps it, from a survey of the tree.
  - port/README.md is the front door: what Frigate is, where it stands, a quick start (build, run, test, White 2, the
    options, the ROM build), a directory map with one line and one guide per directory, and where to go next. The
    controls, saves, the macOS app, preparing a checkout and syncing follow as they were; the planned layout of
    PLAN.md §4.3 is one line.
  - New in port/docs: ARCHITECTURE.md (the layers, the build, the boot, one frame with the threads and the frame hooks
    in their order, the exit statuses, the boundary rules with their ADRs, and the feature map from each part of the
    game to the decomp's files, the port's modules and the tests), MODDING.md (where the game lives, that the checked
    ROM keeps data mods for the file layers to come, finding a feature's code, the three kinds of game change,
    adding an option, testing with replays and the oracle, and what phase 6 will add, marked as not built),
    GLOSSARY.md and README.md, the index of every document.
  - Directory guides for `hal/` (a row per module with the SDK module it replaces, the four ways a decomp file is
    adapted, the names, and the headers named apart from their files), `gfx/`, `audio/`, `include/`, `cmake/` (every
    build option), `tests/` (which tests need the ROM) and `spikes/`; `tools/` is rewritten to cover every tool, and
    `app/` has a tests column. Each spike's README and each spike copy of live code say they are frozen.
  - Every C, header, Python, shell and CMake file under `port/` opens with what it does and the part of the game it
    touches: the short or missing headers are written, the research scripts' docstrings among them (`manifest.py` wrote
    the NitroFS manifest every build reads), stale comments corrected (`arm7.c`'s null sound driver, `hw.c`'s timers and
    VRAM windows, `os_init.c`, `main.c`'s usage, `present.c`, `tp_host.h`, `integrate.cmake`, `CMakeLists.txt`'s
    options, `port_os.h`'s frame hooks, now in their real order), and the long functions the survey found uncommented
    commented, except the sound driver's `Execute`, left to the sound track working in `hal/snd7`.
  - port/CLAUDE.md: the architecture map of the readability rule is ARCHITECTURE.md's feature map; a new directory,
    document or term also gets its line in the directory map, the documents' index or the glossary. The landing page
    links the new documents and its legal note names upstream's `res/`, which replaced `data/`.
  - Found: port/CLAUDE.md, PLAN.md §4.8 and game-changes.md name `include/platform.h` as the one header that decides
    between MWCC and a host, but there is no such file: the host build defines `PLATFORM_PC` on the compiler's command
    line (`port_decomp_env`, port/CMakeLists.txt). The new documents say so; the rule's text is unchanged.
- **Next:** as the last entry: a save with an adventure made by the port, once overlay 36 lets it into the field. The
  sound driver's and the launcher's modules get their rows in the guides and the feature map when they merge.
- **Blockers:** unchanged: overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download
  Play) only matter from phase 7.

## 2026-10-18: Frigate's logo and icons

- **Upstream:** `045e97b90` (merge of PR #364), as in the last entry; no sync in this task.
- **Port:** `687134df4`, on the branch `frigate-identity`, after `a92bb98d9`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). Nothing outside `port/` changed but the port's own
  `.github/README.md`, so no game-changes row and no `check_mwcc_neutral.py` run.
- **Host build:** arm64 macOS, Black 2 and White 2 RelWithDebInfo, reconfigured in the last entry's build directories,
  and both apps; not pushed, so no CI run (Linux unbuilt; the window icon's code is compiled on macOS too, and only its
  `SDL_SetWindowIcon` call is skipped there).
- **Tests:** T0 as above.
  - T2, T3, T4, T6: Black 2, all 237 `ctest` tests pass (`ctest -j4`, 270 s, with the oracle's), none skipped; White 2,
    all 235 (314 s), none skipped. Neither build has a compiler warning. New: `assets_branding` (the six SVGs are what
    their generator writes, draw with paths alone, and the PNG masters' sizes; 202 checks), `app_window_icon_plain`
    and `_sanitized` (the window's bitmap is the version's and its plate fills the square, and giving it to a window
    of SDL's dummy driver goes through; 17 checks), and
    `app_make_app` now also makes an `.icns` from the version's master with `make_icns.sh`, checks its ten images'
    sizes, that the app holds it as `Contents/Resources/AppIcon.icns` named by `CFBundleIconFile`, and both scripts'
    refusals (a 512 px master, a PNG given as the icon): 36 checks.
  - The apps: `Frigate Black 2.app` and `Frigate White 2.app` hold their version's `AppIcon.icns`, `plutil` reads
    `CFBundleIconFile` AppIcon, and `codesign --verify --deep --strict` passes. macOS 26.5 shows each app's icon as
    drawn from 64 px up (`NSWorkspace`'s icon of the app, rendered to PNG and looked at); at 32 and 16 px it puts it
    inside a dark plate of its own, as it does other apps' plain `.icns` icons (Discord's, OpenMTP's). Each app's
    program runs 300 frames headless from the app (`--save-dir` a scratch directory), status 0. `package_macos.sh`
    makes `Frigate Black 2 0.0.2619.dmg` and `Frigate White 2 0.0.2619.dmg` (5.4 MB each, 0.3 MB more for the icon),
    whose apps hold their icon and pass `codesign --verify --deep --strict` as mounted. The app and image results are
    from the tree that became this commit (version `0.0.2619`, revision `a92bb98d91c8-dirty`).
  - The window's 64 px icons, made by `gen_window_icon`, were looked at enlarged: the plate fills the square with clean
    edges and the three masts read. The landing page's `<picture>` was tried in a browser with the emulated light and
    dark schemes: each shows its logo.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no game code changed.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version, as in the last entry.
- **64-bit warnings:** unchanged: no game or `lib/` code changed.
- **Work done:** Frigate has its logo and app icons, in `port/assets/branding` with the script that draws them
  (`tools/frigate_logo.py`), its renderer (`tools/render.py`, AppKit, macOS) and a README: the files, the design, the
  colours, how the build uses them, how to change them, and their license, the repository's GPL v3.0.
  - The landing page (`.github/README.md`) and port/README.md open with the logo, `logo.svg` or `logo-dark.svg` by
    colour scheme in a `<picture>`, above the one-line description and the disclaimer; the landing page's documents
    table links the artwork.
  - The macOS app's icon: `PORT_APP_ICON` (port/CMakeLists.txt) is the version's 1024 px master;
    `port/app/macos/make_icns.sh` makes an `.icns` of it with `sips` and `iconutil` when it changes, never committed,
    and `make_app.sh --icon` puts it in the app as `Contents/Resources/AppIcon.icns`, which the Info.plist's new
    `CFBundleIconFile` names. Black 2's app has the dark plate with the white-hulled ship, White 2's the light plate
    with the black-hulled one.
  - The window's icon on Linux and Windows: `port/tools/gen_window_icon.c`, built for the machine that builds, trims
    the master's macOS margin and averages it down to a 64 px bitmap in a generated C file, and
    `port/app/window_icon.c` gives it to the window (`SDL_SetWindowIcon`). On macOS the window sets none, so the Dock
    keeps the app's icon.
  - Directory guides: `port/assets/README.md` and the branding README are new, and so is `port/app/README.md`, the
    guide of `app/` with a row for each of its files; port/README.md's directory map, PLAN.md §4.3 and
    `port/tools/README.md` (a new table of the build's generators) have the new files. port/CLAUDE.md says where the
    artwork lives and that a change to it is a change to its script.
- **Next:** as the last entry: a save with an adventure made by the port, once overlay 36 lets it into the field.
  Possible follow-ups for the artwork: hand-drawn 16 and 32 px images in the `.icns`, an Icon Composer icon for
  macOS 26, and a volume icon for the disk image.
- **Blockers:** unchanged: overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download
  Play) only matter from phase 7.

## 2026-10-18: the port is Frigate

- **Upstream:** `045e97b90` (merge of PR #364), which `main` merged before this task (`5ad9a1e1d`); no sync in this
  task.
- **Port:** `61a528143`, on the branch `frigate-identity` from `5ad9a1e1d`; this entry is the commit after it.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`), from a clean `build/`. Nothing outside `port/`
  changed but the port's own `.github/README.md` and `.github/workflows/port-*.yml`, so no game-changes row and no
  `check_mwcc_neutral.py` run.
- **Host build:** arm64 macOS, Black 2 and White 2 RelWithDebInfo from clean build directories, and the macOS app and
  disk image of each version; not pushed, so no CI run (Linux unbuilt).
- **Tests:** T0 as above. The test results are from the tree that became `61a528143`, the apps' from that commit
  with nothing uncommitted (version `0.0.2618`, revision `61a528143172`).
  - T2, T3, T4, T6: Black 2, all 234 `ctest` tests pass (`ctest -j4`, with the clone's built `pbw-oracle` copied into
    `build/port/oracle`), none skipped; `oracle_opening` and `oracle_title` failed at first for want of the ROM build's
    link map (`build/b2_us/arm9.o.xMAP`, which the ROM build was still writing) and passed on a rerun once it was there.
    White 2, all 232 pass, none skipped, the oracle's included (`oracle_opening`, `oracle_title`, `oracle_continue`,
    `oracle_port_save`). On macOS `app_make_app`, `app_message_log` and `app_package_macos_out` pass in both, with the
    new checks of the Info.plist's names and of `--help`.
  - The apps: `plutil` reads `CFBundleName` Frigate, `CFBundleDisplayName` Frigate Black 2 and Frigate White 2,
    `CFBundleIdentifier` `io.github.callancampos.frigate.black2` and `.white2`, `CFBundleExecutable`
    `frigate-black2` and `frigate-white2`, and the copyright line; `plutil -lint` and
    `codesign --verify --deep --strict` pass. Each app's program, run from inside the app with `--headless
    --frames 300 --rom orig/baserom_<version>.nds --save-dir` a scratch directory, loads the app's own SDL3
    (`DYLD_PRINT_LIBRARIES`), finds the decomp's dump and writes its screens after frame 300, status 0.
    `package_macos.sh` makes `Frigate Black 2 0.0.2618.dmg` and `Frigate White 2 0.0.2618.dmg` (5.1 MB each), whose
    volumes are named after the apps and hold the app and a link to `/Applications`.
  - `--help` prints "Frigate — Pokémon Black 2", the description and the disclaimer before the usage.
  - T5's nightly runs and T7 not yet.
- **Tiers:** not measured: no game code changed.
- **Trap stubs:** 624 function stubs and 48 data placeholders in each version (`gen_stubs`), against the 574 and 63
  the saves' entry below measured: the difference came with the upstream code `main` merged before this task
  (`045e97b90`); this task changes no game code.
- **64-bit warnings:** unchanged: no game or `lib/` code changed.
- **Work done:** the user named the port **Frigate** and asked that the code be easy for modders to read; both are
  standing rules in port/CLAUDE.md ("The port is Frigate", "Code that modders can read").
  - What the player sees: the programs are `frigate-black2` and `frigate-white2` (were `pokeblack2`, `pokewhite2`);
    the window's title is "Frigate — Pokémon Black 2" ("Pokémon White 2"), and so is a no-ROM alert's; `--help` opens
    with the name, the one-line description and that Frigate is not affiliated with Nintendo, Game Freak, Creatures
    or The Pokémon Company. Saves go to `~/Library/Application Support/Frigate` (`$XDG_DATA_HOME/Frigate` or
    `~/.local/share/Frigate` on Linux, `%APPDATA%\Frigate` on Windows) and an app's log to
    `~/Library/Logs/Frigate/<program>.log` (were `pokebw2-native`; nothing was released, so nothing migrates). The
    ROM's variable is `FRIGATE_ROM` (was `POKEBW2_ROM`, no fallback), `PORT_ROM_ENVIRONMENT_VARIABLE` in
    `port/hal/rom/port_rom.h`.
  - The macOS app is `Frigate Black 2.app` (`Frigate White 2.app`), built by the target `frigate-black2_app`
    (`frigate-white2_app`): `CFBundleName` Frigate, `CFBundleDisplayName` Frigate Black 2, the bundle identifier
    `io.github.callancampos.frigate.black2` (`.white2`; was the placeholder `com.example.pokebw2-native.<program>`),
    and an `NSHumanReadableCopyright` with the description and the disclaimer, which the About window shows.
    `make_app.sh` takes the product's name as `--name`; `package_macos.sh` reads the app's name and target from the
    build's cache (`PORT_MACOS_APP_NAME`, `PORT_MACOS_APP_TARGET`) and names the disk image and its volume after the
    app: `Frigate Black 2 0.0.N.dmg`.
  - The names live in one place each: `PORT_PRODUCT_NAME` in `port/hal/os/port_app.h` for the C, `PORT_PRODUCT_NAME`,
    `PORT_PROGRAM` and `PORT_GAME_NAME` at the top of `port/CMakeLists.txt` (`project(frigate)`), the app's names in
    `port/cmake/macos_app.cmake`. The ROM manifest gained `port_rom_manifest_game` ("Pokémon Black 2"), which the
    window's title and `--help` use.
  - CI's workflows show as "Frigate CI" and "Frigate upstream sync"; the landing page (`.github/README.md`),
    port/README.md, PLAN.md's title and the oracle's tools name Frigate, and say where the port stands today. ADR
    0010 has a note with the new names; past entries here, the research notes and the ADRs' decisions keep theirs.
  - Kept, as port/CLAUDE.md says: the layer's names (`port/`, `port_`, `PORT_*`, `port_*` targets, `pokebw2_game`,
    `pbw-oracle`, `pbwinput`, the `port-*.yml` files, the "port:" prefix of messages), the repository
    `CallanCampos/pokebw2-native` and its clone's directory, and upstream's own files (its README's "Native port
    notes", the ROM build's `pokeblack2_us.nds`).
  - New checks: `app_make_app` reads the app's `CFBundleName`, `CFBundleDisplayName`, `CFBundleIdentifier` and
    `CFBundleExecutable` back; `app_message_log` checks that `--help` starts with the name and the disclaimer;
    `card_backup_save_path` checks the save directory's `Frigate`.
- **Next:** as the saves' entries below: a save with an adventure made by the port, once overlay 36 lets it into the
  field. The landing page's logo and artwork are a separate task, under way.
- **Blockers:** unchanged: overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download
  Play) only matter from phase 7.

## 2026-10-17: the sound against the oracle

- **Upstream:** `aae2a03f6` (merge of PR #361), as merged into the port's `main` by `f5f498a89`.
- **Port:** `6d9b4b41f`, on the branch `port-sound-oracle` from `4a0fa7d89`, merged into `main` after the upstream
  sync `42ddf3e79`: the work in `4e0509a8c` and `8bc14a893`, the fixes from two reviews of it in `d66e67b82`, and the
  confirmation's three notes in `6d9b4b41f` (2026-10-18).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `python3 configure.py && ninja` after the fixes
  only checks the two ROMs. Nothing outside `port/` changed, so no game-changes row and no `check_mwcc_neutral.py`
  run.
- **Host build:** arm64 macOS, RelWithDebInfo, Black 2 and White 2; the oracle (`port/tools/oracle`, melonDS 1.1)
  Release, built from this branch and put in each build's `oracle/`. Not pushed, so no CI run (Linux unbuilt).
- **Tests:** T0 as above. T2, full runs after the last change, with the oracle built from this branch in each build's
  `oracle/`, so no oracle test skips: Black 2, 233 of 233 `ctest` tests pass; White 2, 231 of 231. New:
  `oracle_sound_tool` (38 tests of `sound.py` on made-up traces and dumps), `os_timed_work`, `snd_fifo`,
  `snd_pxi_alarm_timing` (each alarm's handler at its cycle, and an alarm for no ticks at once; without the fixes they
  come at the VBlank), `snd_timer_write` (a write to SOUNDxTMR mid-note, worked out by hand from GBATEK's and
  melonDS's timer; it fails on the timer as first built), `snd_trace_layout`; `oracle_boot` now also holds the
  oracle's WAV dump and sound trace to determinism and their timing, each record in the frame whose VBlank comes
  next. `snd_rom_sequences` is a change detector: its hashes are the port's own, made again for the new sound unit,
  and say nothing about fidelity. T4: `oracle_sound_opening` and `oracle_sound_title` in both versions (below), and
  their port-only checks `oracle_sound_<name>_hashes` (9,639 and 1,957 runs with each play's runs, 75 and 355 frames
  of status, White 2 354, 3 segments each, 1 and 2 of them to the bit); the screen sequences `oracle_opening` and
  `oracle_title` still pass in both versions. T3: `replay_start_menu` and, in Black 2, `replay_new_game` pass (among
  the T2 tests). T5, T6, T7 not yet: the fidelity review's ASan and UBSan run of the `snd_` scenarios and both
  sequences, at `2ecc85a85` (the same tree as `8bc14a893`), was clean but for the known `g2d_CellAnimation.c:33`
  misalignment; the fixes after it haven't run sanitized.
- **Tiers** (`port/tools/progress`, upstream `aae2a03f6`): (a) 32,254 functions, 2,892,792 code bytes (75.1%); (b)
  5,172, 609,116 bytes; (c) 3,592, 348,336 bytes (9.0%). Since the last entry (`615939e25`): (a) +23 functions and
  10,838 bytes, from (b); (c) unchanged. Upstream's matching, merged by `f5f498a89`; this branch changes no decomp C.
- **Trap stubs:** 624 function stubs and 56 data placeholders in both versions (`gen_stubs`, each build above), from
  574 and 63 at `615939e25`: upstream's C since then (overlay 12's data among it) references more functions it doesn't
  define yet. Neither sequence reaches one.
- **64-bit warnings:** game / `lib/` on arm64 (`port/tools/compile-probe`, its arm64 host pass, at this branch's base
  `4a0fa7d89`, whose `src/`, `lib/` and `include/` the branch doesn't change): `-Wpointer-to-int-cast` 865 / 184,
  `-Wvoid-pointer-to-int-cast` 52 / 117, `-Wint-to-pointer-cast` 76 / 144, `-Wint-to-void-pointer-cast` 69 / 132, the
  same as at `615939e25`. That pass doesn't define `PLATFORM_PC`, where `uptr` is `u32`, so it still counts the three
  `btlv_input.c` casts that `4a0fa7d89` made `uptr`; upstream's merge added none.
- **Work done:** T4 for the sound (ADR 0006's "Revisit the output's 16-bit scaling against the oracle"),
  [port/tools/oracle/README.md, "Sound"](../tools/oracle/README.md#sound).
  - The oracle writes melonDS's mixed output sample by sample (`--audio-dump`, before melonDS resamples it) and the
    ARM7 sound driver's state after each run of its thread (`--sound-trace`, `sndtrace.h`: the driver's work in the
    DS's layout, read from the ARM7's WRAM once its thread waits again); the port writes the same trace
    (`--sound-trace`, `port/app/sound_trace.c`, `port_snd7_trace_record`). `sound.py` compares them: each play of
    each player by the sequence's own clock (the periodic runs since its start), field by field, with every run on
    both programs (a play that ends earlier or later fails); what the ARM9 sees of the driver as each frame's VBlank
    comes, where the game counts passes; and the sound by segment (envelope correlation, RMS, peak, samples at full
    scale and in which seconds, the share of samples equal at the best offset; streams and cries to the bit).
    `oracle.py sound` and `sound-check`, ctest `oracle_sound_<name>` and `oracle_sound_<name>_hashes`, with
    references for both sequences in both versions.
  - Results: every run paired is equal, field by field, and no run is on one program only (opening: 9,639 runs and
    6,279,801 fields in each version; title: 1,957 and 636,043 up to START's press, after which 4,940 of the DS's
    runs of the opening's music and 26 of the port's are left out, White 2 4,706 and 26); every frame on the count
    but the declared ones is equal or a run apart (opening: 579 of 579 equal; title: 2,791 equal and 1 a run apart of
    2,792 in Black 2, 2,790 of 2,790 in White 2, the DS's long loading passes and Kyurem's cry left out). No frame on
    the count has a player active, so the frames hold the locks, alarms, shared work and the ARM9's own channels, and
    the run-by-run comparison holds the players and tracks. The title's stream and the cry are melonDS's to the bit
    (2,170,736 and 117,070 samples); the sequences' music matches in its envelope (correlation 0.989, RMS within
    0.05%) but not to the sample: the DS's notes start 1 to 5 mixer samples after its periodic alarm (Black 2's
    opening music: 43, 232, 147, 8 and 3 notes), the port's in the first, so 15.6% of the music's samples are the
    DS's at the best offset (White 2 10.3%) and 89.8% of the logo's (36.2%).
  - Fixes from the comparison: the sound alarms are the OS core's timed work (`port_os_set_timed_work`, new), so
    their words reach the ARM9 when they go off; lazily at the VBlank, two or three came together and NitroSystem's
    stream player dropped a block every 7 frames or so, playing silence and falling 27 frames behind the DS's music
    in 420. The sound unit has melonDS's start delays (3 timer steps for PCM, 11 for IMA-ADPCM, 1 for a square wave
    or noise), its 8-word FIFO, the channel's step before the mixer's sample, GBATEK's mixer widths and 127 counting
    as 128; and the lock words' addresses in `snd7_exchannel.c`'s comment are the right way round.
  - Fixes from the reviews (2026-10-18): a channel's timer counts on through a write to SOUNDxTMR and reloads at its
    next overflow, as the hardware and melonDS do, where it had counted each step from its start against the current
    period, which moved a note's phase whenever an LFO, a sweep or a bend changed its pitch (most of the opening's
    notes; it changed 0.8% of the music's samples and, of the six ROM sequences that `snd_rom_sequences` plays, only
    the Pokemon Center's music); an alarm for no ticks goes off at once, not at the next VBlank; `sound.py` fails a
    run or a play on one program only, inside a window up to the press, and the references keep each play's runs; the
    oracle stamps a record with the frame whose VBlank comes next (melonDS's frame runs on to line 262), which
    `oracle_boot` checks on the oracle's traces and `sound-check` on the port's, and which turned up White 2's port
    frame 3214 (the cry still playing on the DS, now declared) and made Black 2's 3218 a run apart; the trace has the
    sample by which a periodic run began, so note starts are measured from each program's alarm, not from the
    earliest; and the docs say the output is 16 bits on the DS's 10-bit scale, not 10 bits.
  - The clipping in the opening's music is the game's: melonDS clips it in the same seconds, as often (Black 2:
    2,277 samples at full scale on the oracle, 2,271 on the port, in seconds 24 to 57, at most 440 and 421 a second;
    White 2: 2,301 and 2,271, at most 434 and 421), on the same scale: 16 bits on the scale of the DS's 10 bits times
    64, neither program cutting the low 6 bits. The port had 2,007 before 127 counted as 128.
- **Next:** as the gamepads entry below, the last on this track's branch (overlay 12's field script data, upstream).
  For the sound: a comparison sequence where a player plays on the game's count (sound effects and a cry through the
  field once phase 3 reaches it), so that the frame-by-frame status holds a player's state too.
- **Blockers:** unchanged: overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download
  Play) only matter from phase 7.

## 2026-10-17: saves, after review: the same save at every run, and the port's save checked on melonDS

- **Upstream:** `615939e25`, as in the last entry; no sync in this task.
- **Port:** `ca2b56b2e`, on the branch `port-saves`, after `3d86232c7`, merged into `main` with the gamepads' branch. The branch's two earlier commits were rebuilt
  before any of it was published, so that the first carries the oracle README's format evidence its links point to
  (`01b182f4b`, `3d86232c7`; their final tree is unchanged).
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `check_mwcc_neutral.py`: 3,142 of 3,144 file and
  version pairs preprocess as upstream's `615939e25` does, `util_wifiidtool.c` among them; the other two are
  `src/system/vm.c`'s Neutral change already in `game-changes.md`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2, and Black 2 under ASan and UBSan. Not pushed, so
  no CI run.
- **Tests:** T0 as above. Black 2: all 229 `ctest` tests pass, the oracle's included (`ctest -j4`, 345 s), 6 of them
  new: `replay_same_save`, `card_backup_linked_save`, `card_backup_refuse_padded`, `card_backup_refuse_larger`,
  `card_backup_refuse_directory` and `oracle_port_save`; `oracle_compare` has 41 tests (2 new: `port-save` with
  stand-ins for both programs, and its skips; extended: the named stop, the blank `--save-out`, the declarations'
  `stop_trap`). T4: `oracle_continue` 3,415 of 3,415 frames (1,310 exact, 2,105 with 3D within the tolerance), stopping
  on `FIELDMAP_PROC_FUNCS` as `stop_trap` requires; `oracle_opening` 3,800 of 3,800 and `oracle_title` 3,579 of 3,579,
  as before. T6: `oracle_port_save`, 5,400 of 5,400 frames the same, neither save written, the Wi-Fi ID the oracle's,
  the saves 10,712 bytes apart; `replay_same_save`; the card tests. White 2: the 16 `card_backup` tests and
  `replay_same_save` pass; `oracle.py sequence continue` 3,407 of 3,407 (1,312 and 2,095) and `oracle.py port-save`
  5,400 of 5,400, the saves 10,720 bytes apart, run directly. Sanitized (ASan and UBSan, Black 2): the 16 `card_backup`
  tests with no report, `replay_same_save`, and the program on the first save through CONTINUE to its trap: no ASan
  error, from UBSan only the known misaligned read (`g2d_CellAnimation.c:33`), its 3,436 frames' counts and screens
  those of the plain build, the save untouched. Mutations: `replay_same_save` and `oracle.py port-save` fail on the old
  DWC line (another ID at each launch, `e70c0000f3a0b385` and `e7080000f2a0b385`), `card_backup_linked_save` without the
  link's resolution, `card_backup_other_saves` with the old name rule. T3's long runs, T5's nightly runs and T7 not yet.
- **Tiers:** not measured: the one game change is a branch for hosts in an incomplete file.
- **Trap stubs:** unchanged, 574 function stubs and 63 data placeholders in each version; a continued game still stops
  on `FIELDMAP_PROC_FUNCS` (overlay 36), which `oracle_continue` now requires by name.
- **64-bit warnings:** unchanged: the game change is a `PLATFORM_PC` branch, which the compile probe doesn't define.
- **Work done** (the two reviews of the last entry's work):
  - **The port's saves differed at every run.** DWC seeds the console's pending Wi-Fi ID with the clock plus the
    address of `OS_GetTick` (`util_wifiidtool.c`'s `BUG`), a constant on the DS and on a host wherever ASLR loaded the
    program, and New Game copies the ID into both copies of the save. The host build now adds the DS's address
    (`PORT_DS_OS_GETTICK_ADDRESS`, `0x0207bb0d` and `0x0207bb39` with the Thumb bit, from the ROM's literal pools):
    a game change of the Platform kind, row in `game-changes.md`. The port's New Game now writes the same save at every
    run, with the oracle's Wi-Fi ID (`280c0000f2a0b315` in Black 2, `69080000f2a0b3ff` in White 2): ctest
    `replay_same_save` runs it twice and holds the ID to the oracle's, and fails on the old code.
  - **The port to melonDS is a test now**, where the last entry's T6 claim rested on a run by hand: `oracle.py
    port-save`, ctest `oracle_port_save`, boots the port's New Game save on the oracle against the oracle's own save of
    that moment, frame for frame, checks that neither is written and that the Wi-Fi ID is the oracle's.
  - **The two formatted saves' difference, traced** (`port/tools/oracle/README.md`, "Saves"): 10,712 bytes in Black 2
    (10,720 in White 2), the same at every run, all from when each program set the blocks up: three blocks encrypted
    with a key seeded by the VBlank count, DWC's player ID from `OS_GetLowEntropyData`, the day care's egg seed, five
    small fields, and the CRCs over them. The last entry's 10,718 was one run's.
  - **The card layer** (`port/hal/card/backup.c`): a save path that is a symbolic link stays one (the image goes to the
    file it links to, so a link to melonDS's `.sav` keeps one save for both programs); a padded image of this chip
    and a directory are refused with what they are; a size is said to be another chip's save's size, not that save;
    the hint for a melonDS save under another name leaves out only this game's saves as the port names them
    (`IRE?.sav`, `IRD?.sav`), so a `6149.sav` is named. The `.dsv` check is DeSmuME's own (`src/mc.cpp`), and its test
    builds the footer as DeSmuME writes it.
  - **`oracle.py`:** a sequence that allows a stop names its trap (`stop_trap`), so `oracle_continue` passes only on
    overlay 36's `FIELDMAP_PROC_FUNCS`; `port --save-out` never leaves an earlier run's file (the blank chip when the
    game wrote nothing). White 2's known press frame in the continue sequence is described from the oracle's own
    frames: the cursor's pulse step the port shows there is one the DS shows at counts 3274, 3296, 3338 and 3360.
  - **The tests' save directories:** `smoke_reaches_game_main` and `integrate_boot_logos` ran the program without
    `--save-dir`, so with the player's own save directory; they have one in the build tree now.
  - **D7 corrected:** the save keeps the console's clock offset, MAC and birthday, which CONTINUE checks
    (`EventFieldContinue_Create`, which the port reaches before its trap) and overwrites with the console's. Saves from
    the port, the oracle and melonDS's generated firmware move as they are (offset 0), but one from a DS whose clock
    was set takes the changed clock's path on the port, and back. Taking a save's console on import, with the
    console's Wi-Fi ID kept between launches, is the sidecar's first job; not built.
- **Next:** as in the last entry: a save with an adventure once overlay 36 lets the port into the field; then the
  import of a save's console (D7), DeSmuME's `.dsv`, and a save with the Pokédex for the start menu's last line.
- **Blockers:** overlay 36 (issue #83) and overlay 12's script data, as in the last entry.

## 2026-10-17: saves with melonDS, and CONTINUE from the oracle's first save

- **Upstream:** `615939e25`, which `main` merged before this task (`e1b68f578`); no sync in this task.
- **Port:** `3d86232c7`, on the branch `port-saves` from `f52dad0b2`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). No game file changed, so no
  `check_mwcc_neutral.py` run and no row in `game-changes.md`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 (everything) and White 2 (the program and the backup tests),
  and Black 2 under ASan and UBSan (the program and the backup tests). Not pushed, so no CI run.
- **Tests:** T0 as above. T2: Black 2, 219 of 219 `ctest` tests pass with the four long oracle tests left out
  (`oracle_boot`, `oracle_opening`, `oracle_title`, `oracle_continue`, run apart), 223 in all; new or extended:
  `card_backup_round_trip`, `card_backup_other_saves`, `card_backup_refuse_eeprom`, `card_backup_refuse_dsv` and
  `card_backup_bad_file`'s message, and `oracle_compare`'s 39 tests (4 new: the port's save directory with `--save`
  and `--save-out`, runs a sequence lets stop on a trap, saves held to their SHA-1, the save command's skips; and the
  sequences' declarations, saves included). White 2: the
  12 `card_backup` tests pass. Sanitized: the 12 `card_backup` tests, and the program on the first save through
  CONTINUE to its trap: no ASan error, from UBSan only the known misaligned read (`g2d_CellAnimation.c:33`), every
  frame's count and screens those of the plain build, the save file untouched. T6, for the first time: a save of the
  4 Mbit flash round-trips through the card layer byte for byte (unit), melonDS's save continues in the port and the
  port's own file boots in melonDS (below). T4: `oracle.py sequence continue`, the port against melonDS 1.1 on the
  game's count, from power-on with the first save to the port's trap after CONTINUE:
  - Black 2: 3,415 of 3,415 compared frames match (1,310 exact, 2,105 with 3D within the tolerance), the title's 21
    known frames and oracle frame 4920 left out; frames 3,244-3,436, the start menu with the saved game, CONTINUE and
    the wipe out to the field's black, all exact;
  - White 2: 3,407 of 3,407 (1,312 and 2,095), the title's 28 known frames, port frame 3402 (the cursor's blink at
    the press, a step apart since the cry's lag) and oracle frames 1260 and 4852 left out.

  T3, T5's nightly runs, T7 not yet.
- **Tiers:** not measured: this task changed no decomp C.
- **Trap stubs:** 574 function stubs and 63 data placeholders in each version. A continued game stops after port
  frame 3,436, in the black after the start menu, on `FIELDMAP_PROC_FUNCS` (overlay 36's .data, `0x021c9eb8` in Black
  2, `0x021c9eec` in White 2): overlay 36 gates CONTINUE, before overlay 12's script tables, which still stop New Game.
- **64-bit warnings:** unchanged (no game C changed).
- **Work done:**
  - The save format, settled with evidence (`port/tools/oracle/README.md`, "Saves"): the port's file and melonDS's
    `.sav` of BW2 are both the 4 Mbit flash's 524,288 bytes with nothing before or after; melonDS 1.1 gives `IREO`
    and `IRDO` save type 6, a 512 KiB flash, and reads and writes the `.sav` whole, padding or cutting another size
    without a word. Only the names differ (the ROM's file for melonDS, the game code for the port). D7's sidecar isn't
    needed yet: the port keeps nothing of its own, and the one thing a save would need from it, the console's clock
    offset, is 0 on the port, the oracle and melonDS's generated firmware alike.
  - The card layer (`port/hal/card/backup.c`): a file of another size is refused with what it seems to be (another
    chip's save by melonDS's sizes, DeSmuME's `.dsv` by its footer), and a `.sav` of the chip's size under another
    name in the save directory, such as a melonDS save copied in as it was, is named with how to play it. The save's
    names and both directions in `port/README.md`, "Saves".
  - The oracle makes saves: `oracle.py save first_save` plays `port/tests/oracle/first_save_<version>.pbwinput`, New
    Game through the intro into the field, the scripts to the player's room and the field menu's SAVE (18,800
    frames), and checks the save against the SHA-1 `SAVES` keeps; nothing the game wrote is committed. `oracle.py run`
    and `port` take `--save` and `--save-out`; a sequence can start from a save and allow a trap stub from a frame on.
  - The continue sequence (`continue_<version>.pbwinput`, ctest `oracle_continue`): the title's presses from the first
    save and A on CONTINUE.
  - The port's own save to melonDS: the file the game wrote when New Game formatted the blank backup on the port boots
    in melonDS to the same 5,400 frames as melonDS's own file of that moment. Lessons in `port/docs/lessons.md`.
- **Next:** a save with an adventure made by the port, once overlay 36 lets it into the field (phase 4's exit: catch,
  save, quit, reload; a melonDS save continues in the port and back); import of DeSmuME's `.dsv` and the clock offset
  a save brings, with D7's sidecar, when a save from another console needs it.
- **Blockers:** overlay 36 (issue #83) stops CONTINUE at the field's process table and gates the overworld; overlay
  12's script data stops New Game at the field's first script.
## 2026-10-17: gamepads, and the macOS app in a disk image

- **Upstream:** `615939e25`, as merged into the port's `main` by `e1b68f578`.
- **Port:** `3ad982e8c`, on the branch `port-controller-pkg` from `f52dad0b2` (merged into `main`, with fixes in `df3a77ccd`, `cfd1cf911` and `4a0fa7d89`): the work in
  `78758202d` and `c424bf45c`, and the fixes from two reviews of it in `01240f928` and `31814aaf2`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `ninja` has no work after the commits. Nothing
  outside `port/` changed but `.github/workflows/port-ci.yml`, a port file, so no game-changes row and no
  `check_mwcc_neutral.py` run.
- **Host build:** arm64 macOS, at `31814aaf2`: Black 2 RelWithDebInfo and its app, Black 2 under ASan and UBSan (the
  alignment check recoverable), White 2 RelWithDebInfo and its app; not pushed, so no CI run (Linux unbuilt).
- **Tests:** T0 as above. Every result below is from `31814aaf2` with nothing uncommitted (the apps say version
  `0.0.2544`, revision `31814aaf2f46`); the proofs this entry first gave for the launch through LaunchServices, the disk
  image and all of White 2 were from apps built before the work was committed (`0.0.2540` and `0.0.2541`, `-dirty`), and
  are replaced.
  - T2: Black 2, 220 of 223 `ctest` tests pass and 3 are skipped without the oracle (`oracle_boot`, `oracle_opening`,
    `oracle_title`); White 2, 218 of 221 pass and the same 3 are skipped, `replay_start_menu` and `rom_game_overlays`
    among the passes (the last entry had them failing in White 2). New: `app_gamepad_plain` and `app_gamepad_sanitized`
    (424 checks each: the stick's dead zone, sectors and hysteresis, the map by position and by label, the pad that
    drives, the description, and SDL's virtual joysticks as pads that come and go, a noisy desk pad and nine pads at
    once), and on macOS `app_make_app` (19 checks), `app_message_log` (11) and `app_package_macos_out` (13).
  - Sanitized (`build/port-asan`): the app, integration, smoke and `replay_start_menu` tests, 20 of 20, and a windowed
    run of 120 frames with SDL's dummy video and sound drivers, with no report.
  - T3: `replay_start_menu` and `replay_new_game` pass (among the 220).
  - The app, each version: `codesign --verify --deep --strict` passes (ad hoc, no team, no hardened runtime); its
    program, run from inside the app with `--headless --frames 300 --rom orig/baserom_<version>.nds`, loads the app's
    own SDL3 (`DYLD_PRINT_LIBRARIES`) and stops after 300 frames, with the plain program's picture at frame 300
    (`d66b7bf8`, the same picture in both versions), also from the mounted disk image. Black 2's, opened through
    LaunchServices (`open -n -W --env HOME=<scratch>`, headless, no `--rom`), finds the checkout's ROM, shows the same
    picture and writes its messages to `<scratch>/Library/Logs/pokebw2-native/pokeblack2.log`; `spctl --assess`
    rejects it, as Gatekeeper does any app that isn't notarized. The disk images verify (`hdiutil verify`) and hold
    the app and a link to `/Applications` only. `package_macos.sh` refuses an image path in another repository and
    takes one under an ignored directory or outside every work tree.
  - T4, T5's nightly runs, T6, T7 not yet.
- **Tiers** (`port/tools/progress`, upstream `615939e25`): (a) 32,231 functions, 2,881,954 code bytes (74.9%); (b)
  5,195, 619,954 bytes; (c) 3,592, 348,336 bytes (9.0%). Since the last entry that measured them ("the title screen and
  the start menu", `e484d5011`): (a) +138 functions and 22,912 bytes, from (b); (c) unchanged. The change is upstream's
  matching merged by `e1b68f578`; the entries since then called the tiers unchanged without measuring them.
- **Trap stubs:** 574 function stubs and 63 data placeholders (`gen_stubs`, each build above), down from the 581 and 67
  the last entry carried over unmeasured, by the same upstream C. Overlay 12's script data, which New Game stops on,
  are still placeholders; the two replays reach none.
- **64-bit warnings:** game / `lib/` on arm64 (`port/tools/compile-probe`, its arm64 host pass at `615939e25`):
  `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast` 52 / 117, `-Wint-to-pointer-cast` 76 / 144,
  `-Wint-to-void-pointer-cast` 69 / 132. The game's `-Wvoid-pointer-to-int-cast` rose by 3 since the last measurement
  (49): `src/ov168/btlv_input.c` lines 3021, 3025 and 3029, `(u32)vram` in upstream's newly matched battle code, merged
  by `e1b68f578` before this branch. This branch changes no game or `lib/` code.
- **Work done:** phase 5's controller support and macOS packaging.
  - Gamepads through SDL3's gamepad API (`port/app/gamepad.c`,
    [ADR 0009](adr/0009-gamepads-by-position-one-pad-at-a-time.md)): every pad connected, hot-plugged and remapped
    (eight open at once, a ninth opened when a place frees); the DS's A, B, X and Y by position (a Nintendo pad's
    layout), L and R on the shoulders, Start and Select, the D-pad and the left stick as the D-pad with a circular dead
    zone of half its travel and hysteresis at the dead zone's edge and at the sectors' edges; one pad drives at a time;
    one table, `PortGamepadMap`, for a settings file to replace. Each pad names its DS buttons by SDL's labels when it
    connects. The keyboard works at the same time. The controls are in `port/app/present.c`'s header and
    port/README.md.
  - The macOS app ([ADR 0010](adr/0010-macos-app-as-a-signed-copy-of-the-program.md)): the target
    `pokeblack2_app` (`pokewhite2_app` for White 2; `-DPORT_MACOS_APP=ON` builds it every time) copies the program into
    `<program>.app` with the libraries it loads from outside macOS in `Contents/Frameworks`, however they are named
    (absolute, `@rpath`, `@loader_path`, `@executable_path`), and an Info.plist (a placeholder bundle identifier,
    `com.example.pokebw2-native.<program>`, the version from git, the minimum macOS from the binaries, high DPI, a game
    category with Game Mode); checks that the app loads nothing from outside macOS and itself, signs it ad hoc, and
    starts its program once. Opened from the Finder, the program writes its messages to
    `~/Library/Logs/pokebw2-native/<program>.log` and shows a missing or refused ROM in an alert
    (`port/app/message_log.c`, `port_app_fatal_notice`). `port/tools/package_macos.sh` makes the disk image, checks it,
    and refuses a path that git would track in any work tree. port/README.md says where the ROM goes, where the log is,
    and how to get past Gatekeeper ("Open Anyway"). CI builds the app on its macOS runner and uploads nothing.
  - The reviews' fixes, each with a test: make_app.sh had left a program that loads SDL3 by `@rpath` with no library and
    no search path, signed and verified but unable to start; the stick flickered at the dead zone's and the sectors'
    edges (30 presses in 60 frames of noise; now one); a ninth pad was dropped for good; a Finder launch lost every
    reason it stopped; `package_macos.sh` accepted an image path in another work tree.
- **Next:** as the last entry (overlay 12's field script data, upstream). For the app: a first-launch ROM picker; an app
  for older macOS (the pinned SDL3 built statically with a deployment target, not tried yet). The 3 new 64-bit
  warnings in `btlv_input.c` want a portability fix (`uptr`) once phase 4 reaches the battle screens.
- **Blockers:** unchanged: overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download
  Play) only matter from phase 7.

## 2026-10-17: oracle comparisons from power-on to the start menu

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry.
- **Port:** `f3f285b1c`, merged with the intro's branch into `main` (below), which supplies the libfx fix.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: all 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`.
- **Host build:** arm64 macOS, RelWithDebInfo for Black 2 and White 2, and Black 2 under ASan and UBSan; not pushed,
  so no CI run (Linux unbuilt: the program now links with `ENABLE_EXPORTS` and `CMAKE_DL_LIBS` there).
- **Tests:** T0 as above. T2: Black 2, 211 of 214 `ctest` tests pass and 3 are skipped without the oracle
  (`oracle_boot`, `oracle_opening`, `oracle_title`); White 2, 209 pass, the same 3 are skipped, and 2 fail as they did
  before this work (`replay_start_menu`, whose input file is Black 2's, and `rom_game_overlays`, on overlay 12's
  address). Among them:
  - the run directory's 93 checks, plain and sanitized (memory dumps added), and `oracle_compare`'s 35 tests;
  - the 3D renderer's 777 checks, 13 of them new on edge interpolation (they fail on the old interpolation);
  - `oracle_opening_hashes` (337 frames and 638 screens without 3D in each version) and `oracle_title_hashes` (400
    and 393 frames, 544 and 541 screens, 321 and 314 of the frames on the count).

  Sanitized: `app_run_dir`, `oracle_compare`, `replay_start_menu`, `g3d_ds3d`, `g3d_hal` and the smoke test
  pass; both sequences under ASan and UBSan (3,600 and 3,800 frames) give the plain build's hashes and
  counts at every frame and its `NNS_G3dGlb` where dumped, with the one misaligned read already known
  (`g2d_CellAnimation.c:33`, NitroSystem reading packed file data). T4: `oracle.py sequence`, the port against
  melonDS 1.1, every port frame on the game's count (the movies by their pictures in order, no oracle picture
  skipped) but the known differences, and NNS G3D's state on the count through the title:
  - opening (power-on to the title's first 274 frames, nothing pressed): Black 2, 3,800 of 3,800 frames match (3,526
    exact at 6 bits, 274 with 3D within the tolerance); White 2, the same, with the DS's torn first frame of the
    opening declared; `NNS_G3dGlb` equal at all 274 frames compared in each;
  - title (the opening skipped with START, the title's 3D on both screens, START to the start menu): Black 2, 3,579
    of 3,579 compared match (1,474 exact, 2,105 with 3D within the tolerance), 21 known differences left out; White 2,
    3,572 of 3,572 (1,477 and 2,095), 28 left out; `NNS_G3dGlb` equal at 2,470 of 2,470 frames in each, 15 left out;
  - 3D beyond the pixel tolerance: a median of 17 pixels a screen in Black 2's title and 7 in White 2's, at most 80 of
    the 245 allowed (before the rasterizer fix, a median of 79 and at most 172).

  T3, T5's nightly runs, T6, T7 not yet.
- **Tiers:** unchanged since the last entry (no decomp C changed).
- **Trap stubs:** unchanged, 581 function stubs and 67 data placeholders; neither sequence reaches one in either
  version.
- **64-bit warnings:** unchanged by this branch.
- **Work done:**
  - Phase 2's last exit criterion: oracle comparisons pass within tolerance, for both versions, in two sequences
    (`port/tests/oracle/<name>_<version>.pbwinput`: opening and title; `oracle.py sequence` and `sequence-check`;
    ctest `oracle_<name>` and `oracle_<name>_hashes`).
  - compare.py pairs on the game's count (`--align counter`), the movies by their pictures (`--clock`), with declared
    count lags (`--lag`), and fails any oracle picture no port frame shows (`--known-oracle` for the DS's torn
    frames); `--align sequence` alone, which only holds pictures and their order, passed a port that showed pictures
    late or held a still. The references keep the count each row expects; every known difference is declared, with
    its evidence in `port/tools/oracle/README.md`.
  - The program writes its own run directory for compare.py (`--run-dir`, `--run-screens`, `--run-dump`,
    `port/app/run_dir.c`): `frames.tsv` with GFL's frame count and the game frame at every frame hook, PNG files at
    the frames of a list, `memory.txt` with globals by name (`dlsym`), through a pointer with `*`. `oracle.py port`
    runs the port once.
  - Two 3D fixes. The decomp's `MTX_Concat33`, `MTX_Concat43`, `MTX_Concat44` and `MTX_MultVec43` read an argument
    after storing over it when the result replaces it, which the original never does, so on a host the title's camera
    looked elsewhere (`MAT3_RotationEulerZYX`). This branch had host versions in `port/hal/fx`; the intro's branch
    fixed the same functions in place, with `#ifdef PLATFORM_PC` blocks and an asm oracle test of the decomp's C, and
    the merge keeps that fix alone (a decomp finding). And the rasterizer takes values along an edge by line, as
    melonDS does, not along an X-major edge's run, which had put whole spans' colours a step apart.
  - Settled with evidence (README, "Game frames and video frames" and "Known differences"): START held doesn't skip
    the opening on melonDS or on the port (a press does, on the same pass on both); the oracle's start-up gives
    oracle frame = port frame + 242 from count 5 on through the boot screens, and game frames part where the game
    waits on a clock (from the black after the copyright screen) and line up again at a press; the DS's camera leads
    after START (NNS G3D's state a pass ahead), and White 2's cry ends a pass later against the DS's passes because
    the DS starts it later (traced by the sound driver's shared work on both programs).
- **Next:** overlay 12's field script data (`g_ActiveFieldScriptSubEvents` and the script tables), which New Game
  stops on after the intro, as decomp work upstream; the White 2 failures of `replay_start_menu` and
  `rom_game_overlays`; `--screen-hashes` folded into `--run-dir`.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.
## 2026-10-17: the intro as the DS draws it: libfx's products in place, and a replay checked against the oracle

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry; no sync in this task.
- **Port:** `e3d170b17`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`). `check_mwcc_neutral.py`: this task's three game
  files preprocess identically to the last entry's commit in both versions; over every file, 3,138 of 3,140 file and
  version pairs give upstream `e484d5011`'s token stream, the other two being `src/system/vm.c`'s cast from the last
  entry, which is complete.
- **Host build:** arm64 macOS, RelWithDebInfo, and the sanitized build (ASan and UBSan, alignment recoverable). CI not
  run: the work is on a branch, not pushed.
- **Tests:** T0 as above. T2: 210 `ctest` tests, 209 pass and `oracle_boot` is skipped (no oracle build in this
  worktree). New: `fx_libfx`, libfx's products and transforms into their own inputs and GFL's Euler rotation, by hand;
  the asm oracle, which now runs here, with `test_fx_decomp.py`: the decomp's libfx C against the ROM's code, 88,500
  cases in each version, apart and in place; `integrate_screens` checks the screens' hashes. T3: `replay_new_game` is
  the first replay with goldens: it must show nine screens, from the start menu through the intro to the opening's
  season banner, exactly as the DS does by the oracle's hashes, and run 10,600 frames; it fails on a run that never
  takes NEW GAME (1 of 9) and on the code before this entry's fix (5 of 9). T4, by hand (pbw-oracle, melonDS 1.1,
  `new_game.pbwinput` on both): the DS plays the same file through the intro and the opening into the field's first
  script. From START to the port's trap, 3,378 of the port's 6,856 frames show exactly a DS frame's two screens, in
  order, MCSS's Juniper and Minccino included. In the others a sprite's animation meets a line of text at another moment
  than on the DS (the two reach each scene at another point between presses), or the 3D flight shows. That follows the
  DS's camera frame for frame, 346 frames later, and differs from it in 0.1 to 0.7 % of the top screen's pixels by more
  than 2 steps, at polygon edges (the 3D renderer, not the game). The title screen's 3D Kyurem, about 35,000 of its
  49,152 pixels off at 5 bits a channel before the fix, differs in 500 to 1,000 now. T5: `new_game.pbwinput` in the
  sanitized build to its trap: no ASan error; from UBSan only the two misaligned NitroSystem reads already known
  (`g2d_CellAnimation.c:33`, `g2d_MultiCellAnimation.c:119`); every frame's screens identical to the release build's. T6
  and T7 not yet.
- **Tiers:** unchanged (no upstream merge, no decomp C added).
- **Trap stubs:** unchanged, 574 function stubs and 65 data placeholders. `new_game.pbwinput` now reaches the trap
  on `g_ActiveFieldScriptSubEvents` (overlay 12) at video frame 10,756.
- **64-bit warnings:** unchanged: this task's game changes are inside `#ifdef PLATFORM_PC`, which the compile probe
  doesn't define.
- **Work done:**
  - The decomp's `MTX_Concat33`, `MTX_Concat43`, `MTX_Concat44` and `MTX_MultVec43` read their first input after storing
    part of the result, where the ROM loads each row (the vector) first, so the game's products into their own input
    were wrong on hosts: the intro drew Juniper and Minccino mirrored and in the wrong places (`MCSSSys_Draw`), the
    title's Kyurem from another angle, and every other product into its own input (GFL's Euler rotations, SPL's
    particles, NitroSystem's SBC) came out wrong. Platform blocks route those products through `tmp` and read an
    in-place vector from a copy (`port/docs/game-changes.md`, `port/docs/decomp-findings.md`). The rest of libfx's C is
    safe in place: the scalings, `MTX_MultVec33` and the vector arithmetic by the asm oracle, the inverses (through
    `tmp` in both) and `VEC_Normalize` by reading.
  - `--screen-hashes PATH` writes every frame's screens and GFL's frame count in the oracle's `frames.tsv` format,
    with the platform layer's SHA-1; `compare.py align` reads it. `port/tests/replay/check_screens.py` checks a run
    against a table of the oracle's screen hashes (`new_game.screens.tsv`).
  - `new_game.pbwinput` rewritten so that its presses land the same way on the DS: START at video frame 3,900, then A
    twice a second from 4,400 to 11,000. The 200-pass gap between START and the start menu that the review saw was
    the input, not the game: keyed to game frames, START reached the DS after its title screen had restarted its
    animation, since the DS's game count runs about 2,500 passes behind through the logo movie, where it drops frames.
    With START on a video frame both take 155 passes to the start menu. The DS then falls behind at each load, 70 to
    346 frames by the opening, all of it frames it drops and presses it waits for.
  - [ADR 0008](adr/0008-particle-work-from-the-platform-layer.md) records the last entry's particle work design. The
    last entry's four intermediate commits were not built one by one when made; the review built the host and ran
    `replay_start_menu` and the integration tests at each, and their game changes are token-identical under MWCC but
    for `vm.c`, which is the same at each as at the end.
  - Lessons in `port/docs/lessons.md`.
- **Next:** as in the last entry: overlay 12's field script data, which is decomp work (issue #73 has no active
  claim), then overlay 36. The 3D renderer's edge differences in the opening's flight are worth a look when phase 3's
  oracle comparison closes.
- **Blockers:** as in the last entry: overlay 12's script data stops New Game at the field's first script; overlay 36
  (issue #83, claimed by another contributor) gates the overworld after it.

## 2026-10-17: New Game's intro natively, to the field's first script

- **Upstream:** `e484d5011` (merge of PR #353), as in the last entry; no sync in this task.
- **Port:** `cbec17cb7`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: 3,138 of 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`. The other two are `src/system/vm.c` in each
  version, which differs by one cast and is complete, so the SHA1s prove it.
- **Host build:** arm64 macOS, RelWithDebInfo, and a sanitized build (ASan and UBSan, alignment recoverable). CI not
  run: the work is on a branch, not pushed.
- **Tests:** T0 as above. T2: 209 `ctest` tests, 207 pass and 2 are skipped (`asm_oracle` and `oracle_boot`, which need
  the oracle build), among them the new `replay_new_game`, which plays an input file from power-on through NEW GAME and
  the intro to video frame 12,000. T5, by hand: the same replay in the sanitized build, to frame 12,100: no ASan error,
  and from UBSan only the two misaligned reads already known, NitroSystem reading a 2-byte element of a file through a
  larger struct (`g2d_CellAnimation.c:33`, `g2d_MultiCellAnimation.c:119`). T3 (frame hashes), T4, T6 and T7 not yet.
- **Tiers:** unchanged since the last entry (no upstream merge, no decomp C added).
- **Trap stubs:** 574 function stubs (355 in overlay 36, 113 in overlay 11) and 65 data placeholders: DS Protect's seven
  functions and two data objects are `port/hal/stubs/dsprot.c` now. New Game stops just after video frame 12,060 of
  `new_game.pbwinput`, on `g_ActiveFieldScriptSubEvents` (overlay 12's .bss), which `ScriptWork_Create` reads as the
  opening starts the field's first script. Overlay 12's script tables have no C either: `GLOBAL_SCRIPT_TABLE`,
  `EVCMD_TABLE`, `EVCMD_PERM_TABLE`, `EVCMD_MAX`, `VM_CMP_LUT`, `FIELD_SCRIPT_SUB_EVENT_FINISH_FUNCS`. Until this entry
  no placeholder was protected on macOS: ld64 placed them all before the paddings that were to bracket them, so the
  first run here read `GLOBAL_SCRIPT_TABLE` as zeros and hung in an assert's error screen.
- **64-bit warnings:** game / `lib/` on arm64 unchanged: `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast`
  49 / 117, `-Wint-to-pointer-cast` 76 / 144, `-Wint-to-void-pointer-cast` 69 / 132. The probe preprocesses without
  `PLATFORM_PC`, where `uptr` is `u32`, so this task's `uptr` fixes don't show in it.
- **Work done:**
  - New Game plays natively from the start menu through the whole intro (overlay 294): Professor Juniper and Minccino
    with the intro's particle effects, the player's sex, the name entries for the player and the rival, then the
    opening's season banner and its 3D flight over Aspertia City, which fades out at the player's house.
  - The particle systems' work comes from the platform layer, twice the size the 13 creating files give for the DS's
    structs (`port/hal/gfl/particle_work.c`).
  - Pointers carried in 32-bit values on the way, with each API's siblings: the Pokémon parameter setters (47 call sites
    in 23 files), NitroSystem's animation callback parameter with ClAct's and MCSS's (14 callbacks), the sound preload
    handle, the script VM's stack and loaded file; the opening's raw LCDC VRAM address as the SDK's constant.
  - DS Protect as a genuine cartridge: `port/hal/stubs/dsprot.c`, and the game's own checks of DS Protect's code skipped
    on hosts. `EventData_Reset`'s placeholder pointer, past an array, worked out without undefined behaviour.
  - Trap stubs: the data placeholders' range is worked out from where the linker put each one, so every one faults.
    `--screenshot-every N` writes the screens every N frames, for playtests.
  - Game changes, all proven neutral, in `port/docs/game-changes.md`; the lessons in a new `port/docs/lessons.md`.
- **Next:** overlay 12's field script data (the tables above and the .bss), which is decomp work (issue #73 has no
  active claim) and lets the opening run the field's first script; then overlay 36. Meanwhile, phase 2's oracle
  comparison, and the intro's. The same 32-bit pointer shape waits further on: `JoinAvenue_GetParam` and
  `JoinAvenuePerson_SetParam`, `BmpMenuList_GetParam`, and `NameEntryParam`'s `unkC` and `unk10` (`pokemonNameEntry`).
- **Blockers:** overlay 12's script data stops New Game at the field's first script; overlay 36 (issue #83, claimed by
  another contributor) gates the overworld after it.

## 2026-10-17: the title screen and the start menu, natively

- **Upstream:** `e484d5011` (merge of PR #353), which brings the ITCM's ClAct renderers (PR #352, decomp work this
  session did to unblock the title screen).
- **Port:** `570db24f4`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); `check_mwcc_neutral.py`: all 3,140 file and
  version pairs give the same MWCC token stream as upstream's `e484d5011`.
- **Host build:** arm64 macOS, RelWithDebInfo and unoptimized (`-fgnu89-inline` made the unoptimized build link); CI
  green on macOS and Linux for every commit pushed.
- **Tests:** T0 as above. T2: 207 of 207 `ctest` tests pass, among them the 3D engine's (1,400 hand-derived checks,
  plain and sanitized), the sound driver's 31 and its two ROM scenarios (all 1,390 sequences), the G2D loaders' ROM
  equivalence (every one of the 30,078 G2D files of each ROM against a simulation of the DS's unpack), display capture,
  and `replay_start_menu`, which plays an input file from power-on to the start menu. T4: the melonDS oracle builds on
  request (`-DPORT_BUILD_ORACLE=ON`); a comparison of the boot sequence before this entry's renderer fix matched 33 of 33
  sampled frames at 5 bits per channel, and 11 of 11 at 6 bits in a scratch build with this entry's fix. T3, T5-T7 not yet.
- **Tiers:** (a) 32,093 functions, 2,859,042 code bytes (74.3%); (b) 5,333, 642,866 bytes; (c) 3,592, 348,336 bytes
  (9.0%). Since the research measurement: (a) +209 functions and 26,318 bytes, (c) -3,024 bytes, the ITCM renderers.
- **Trap stubs:** 581 function stubs (355 in overlay 36, 113 in overlay 11) and 67 data placeholders. The game runs
  4,400 frames to the title screen, and through START to the start menu, without reaching one.
- **64-bit warnings:** game / `lib/` on arm64: `-Wpointer-to-int-cast` 865 / 184, `-Wvoid-pointer-to-int-cast` 49 /
  117, `-Wint-to-pointer-cast` 76 / 144, `-Wint-to-void-pointer-cast` 69 / 132 (`port/tools/compile-probe`); as in the
  research measurement but for three void-pointer casts in new upstream C.
- **Work done:**
  - Phase 1's exit, the boot logos in the repository build, and most of phase 2: the boot screens, the Game Freak logo,
    the opening movie and the title screen with its 3D Black Kyurem play as on the DS, and the start menu responds.
  - NitroSystem's 2D files in host structs with a registry tied to heap frees
    ([ADR 0007](adr/0007-nitrosystem-files-in-host-structs.md)); the DS's geometry engine and a CPU 3D renderer;
    display capture; the ARM7's sound driver in host C with SDL3 audio output and `--audio-dump`
    ([ADR 0006](adr/0006-sound-driver-as-built.md)); the melonDS test oracle (`port/tools/oracle`); `--input` replays
    in the oracle's input format, counting game frames by GFL's own frame count.
  - Game changes, all proven neutral (`port/docs/game-changes.md`): the sound archive's file table, wave archive links
    and the sound heap's data as 32-bit handles or `uptr`; GFL's allocation head padded to 32 bytes on hosts. NNS FND's
    heaps are aligned for 64-bit structs by the platform layer: a strict ASan and UBSan build runs the 4,400 frames with
    two misaligned reads left, both NitroSystem reading packed file data.
  - The 2D renderer follows melonDS 1.1 in ignoring a colour's bit 15; the overlay-sections check accepts a program
    whose `__eh_frame` ld64 dropped.
- **Next:** phase 2's oracle comparison through the start menu with one input file for both programs, then New Game's
  intro (overlay 294): 64-bit fixes for the particle system's work buffers and for the Pokémon parameter setters,
  which take pointers as `u32`.
- **Blockers:** overlay 36 (issue #83) gates phase 3, the overworld; overlays 11 and 182 (Wi-Fi, Download Play) only
  matter from phase 7.

## 2026-10-17: phase 1 platform layer, and the game changes it needs

- **Upstream:** `4aa041311` (merge of PR #348); unchanged.
- **Port:** `e49c96554`.
- **ROM:** both SHA1s match; `check_mwcc_neutral.py`: all 3,138 file and version pairs give the same MWCC token stream
  as upstream's `4aa041311`.
- **Host build:** arm64 macOS; CI checks macOS and Linux.
- **Tests:** T0 as above. T2: 144 of 144 `ctest` tests pass (OS scheduler and interrupt scenarios, the 2D renderer's
  16,318 pixel checks, PXI services, ROM and card, overlay data sections, VRAM banks, DMA, integration, the asm
  oracle, the smoke test, which now runs the first frame). T3–T7 not yet.
- **Tiers:** not re-measured (upstream unchanged).
- **Trap stubs:** 587 function stubs and 69 data placeholders.
- **64-bit warnings:** not yet counted by the host build.
- **Work done:**
  - The phase 1 platform layer ([ADR 0005](adr/0005-platform-boundary-as-built.md)): the OS on fibers with the
    decomp's own scheduler and a virtual clock (`port/hal/os`); the PXI bus and the ARM7's services (power, touch,
    firmware settings, reset, cartridge slot, mic, a sound driver); the ROM from the player's dump, the backup chip as an
    atomic save file, and resident overlays whose statics reset on load (`port/hal/rom`, `card`, `fs`); VRAM banks
    aliased by page mapping (`port/hal/gx`); DMA done at once (`port/hal/dma`); the CPU 2D renderer (`port/gfx/ref`),
    screens, input and an SDL window (`port/gfx/screens.c`, `port/app`, `port/hal/input`), `--screenshot`.
  - Game changes, all proven neutral (`port/docs/game-changes.md`): `uptr`, `sptr` and `ulong32`, and the sites that
    needed them (NNS FND heaps, `HW_DTCM`, GX register-address parameters, the sound heap, two alignment computations,
    the RTC date and the misc save block); the heap size hook.
  - `port/hal/gfl/heap_policy.c` (root heaps x4, child heaps x2, `--heap-scale-root`/`--heap-scale-child`);
    `port/hal/dwc/ov011_pending.c`, the four overlay 11 symbols DWC's boot-time init reaches before upstream's claim
    writes them; `port/docs/decomp-findings.md`.
  - The generator routes `reg_CP_DIV_RESULT` (a plain `u64` register) through its accessor; before, every `FX_Div`
    returned 0. Crashes outside the trap pages now print their address and a backtrace, on their own signal stack.
  - In an exploration tree with the remaining fix (below) the game played the boot screens and the whole Game Freak
    logo movie for 1,100 frames, deterministically, and reached the title screen.
- **Next:** NitroSystem G2D's in-place relocation (PLAN.md §4.8 rule 5), in progress as a workflow: a relocation
  registry tied to heap frees and host unpackers for every G2D format, proven on every such file in the ROM. Then the
  boot logos in the repository build (phase 1's exit), then phase 2.
- **Blockers:** the repository build crashes in the boot screens (`src/ov162/boot_screens.c`, the first character
  data), on the G2D relocation above. Overlay 36 (issue #83) still gates phase 3.

## 2026-10-17: phase 0 complete; DS addresses, asm replacements and 64-bit layouts

- **Upstream:** `4aa041311` (merge of PR #348); upstream hasn't moved since this session's sync.
- **Port:** `d63819fc5`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`) after a full `ninja` with this commit's
  `include/types.h` change; `check_mwcc_neutral.py`, now comparing token streams: all 3,138 file and version pairs give the same MWCC token stream as upstream's `4aa041311`.
- **Host build:** arm64 macOS and x86-64 Linux build in CI (`port-ci.yml` green), which closes phase 0's exit criteria
  (links, starts, opens a window, reaches the game's `main()`, both ROMs match, CI green).
- **Tests:** T0 as above. T2: 4 of 4 `ctest` tests pass: the smoke test, C unit tests of the host MI and FX/MATH/RC4
  functions, and `asm_oracle`, which runs the ROM's own machine code for each replaced asm function in a test-only CPU
  emulator and compares it with the host C on thousands of seeded inputs, both versions (it skips without the ROM
  extract, as in CI). T3–T7 not yet.
- **Tiers** (`port/tools/progress`, `b1369e48f`): (a) 31,884 functions (77.7%), 2,832,724 bytes (73.6%); (b) 5,538
  (13.5%), 666,160 bytes (17.3%); (c) 3,596 (8.8%), 351,360 bytes (9.1%), unchanged since 2026-10-16 (overlay 36's C
  hasn't landed upstream yet).
- **Trap stubs:** 664 function stubs and 72 data placeholders; the game stops first at `OS_Init`.
- **64-bit warnings:** not yet counted by the host build.
- **Work done:**
  - DS addresses through build-time header translation ([ADR 0004](adr/0004-ds-addresses-by-header-translation.md)):
    static 16 MiB windows per memory-map region in `port/hal/hw`, accessors for the registers whose accesses act
    (geometry command log, bit-exact divider and square root, VCOUNT, timers, GXSTAT, matrix readbacks), and
    `port_hw_g3_send` for SDK copies to the geometry FIFO.
  - Host C for 38 SDK asm functions (MI copies, fills, compare, swap and LZ decompression; FX matrices; MATH CLZ,
    QSort, SHA-1 block; RC4), each proven against the ROM's code by `port/tests/asm_oracle`.
  - `include/types.h`: `u64`, `s64` and `f64` keep the DS's 4-byte alignment on hosts, so structs keep the DS's
    layout (a bag sort entry stays 12 bytes, so `MATH_QSort` orders equal keys as on the DS; save blocks).
  - The three phase 0 spikes kept in `port/spikes/` (overlay data sections, VRAM aliasing, fibers); the host build
    compiles `port/hal` against the decomp's headers with the game's ABI flags, `port/gfx` as its own target, and
    `port/cmake/*.cmake` fragments.
- **Next:** phase 1, running as a workflow: the OS core on fibers, the PXI bus and host ARM7 services, the ROM, file
  system, saves and overlay data resets, VRAM banks, and the CPU 2D renderer, then integration toward the boot logos.
  Later: fast paths for the byte-wise MI copies (the game's most-called functions), under the same differential tests.
- **Blockers:** none for phase 1. Overlay 36 (issue #83) still gates phase 3.

## 2026-10-17: phase 0, the native build links and enters the game's main()

- **Upstream:** `4aa041311` (merge of PR #348), merged in `0f65946be` by this session's sync, which built both ROMs and
  checked both SHA1s.
- **Port:** `577554b0b`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`) after a full `ninja` with this commit's guards;
  `port/tools/check_mwcc_neutral.py`: all 3,138 file and version pairs preprocess under MWCC exactly as upstream's
  `4aa041311` does.
- **Host build:** arm64 macOS, Apple clang, RelWithDebInfo: 1,480 of 1,518 C files of `src/` and `lib/` compile
  natively (38 are replaced by the platform layer, `port/hal/replaced.txt`) and link into `pokeblack2` (10 MB) with
  SDL3 3.4.18. Linux and Windows not yet; CI (`port-ci.yml`) builds macOS and Linux from this commit on.
- **Tests:** T0 as above; T1: builds; T2: the smoke test (`ctest`) passes: the program reaches the game's `main()`.
  T3–T7 not yet.
- **Tiers:** not re-measured since 2026-10-16.
- **Trap stubs:** 692 function stubs and 72 data placeholders. The game stops first at `OS_Init` (called by
  `SystemInit`), the first SDK module the platform layer replaces.
- **64-bit warnings:** not yet counted by the host build (silenced there); the compile probe baseline of the
  2026-10-16 entry stands.
- **Work done:**
  - `port/CMakeLists.txt`: the host build, with MWCC's language model (signed char, 16-bit `wchar_t`, wrapping
    arithmetic, no strict aliasing, zeroed locals) and SDL3 (Homebrew's, or the pinned release).
  - `port/tools/gen_host_tree.py`: the source list, an `include/` mirror without MSL's C library headers, the generated
    constants. `port/tools/gen_stubs.py`: a trap stub for each DS function the C references and nothing defines, and a
    placeholder on a page with no access for each such data object, so the first use is reported by name.
  - `port/hal/trap.c`, `port/hal/os/memory_map.c`, `port/hal/stubs/unresolved.c`, `port/include/port_host.h`,
    `port/app/main.c` (SDL3 window, the game on its own thread, `--headless`).
  - Game changes (`port/docs/game-changes.md`): `OVERLAY_ID(n)` is `n` on a host; `MATH_CountLeadingZeros` uses the
    compiler's builtin; the 18 MWCC `asm` functions of seven mostly-C NitroSDK files are behind `#ifndef PLATFORM_PC`.
  - `port/tools/check_mwcc_neutral.py`, which proves a guard-only change neutral for every file at once.
  - `.github/workflows/port-ci.yml`: the host build and tests on macOS and Linux.
- **Next:** the phase 0 spikes (overlay data sections, VRAM aliasing, fibers), then phase 1: the platform layer in the
  order the traps ask for it, starting with `OS_Init`, threads, IRQs, arenas, MI and FX.
- **Blockers:** none for phase 0 or 1.

## 2026-10-17: solo project, ad-hoc signing and upstream guards

- **Upstream:** `462e90193` (merge of PR #345), still the last merged. Upstream `main` is now `69e82e80c` (merge of
  PR #347), four commits ahead, and not yet merged by this commit; the next sync merges it.
- **Port:** `5b1102e5d`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`); this commit changes nothing outside `port/` and
  `.github/`.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Tiers:** not re-measured; no upstream commit has been merged since the 2026-10-16 entry.
- **Trap stubs:** not yet.
- **64-bit warnings:** not yet from a host build; the compile probe baseline in the 2026-10-16 entry stands.
- **Work done:**
  - Two decisions by the user: the port is a solo project, so nothing about it is posted on `fuddlesworth/pokebw2`
    and its portability changes stay here (D10, [ADR 0002](adr/0002-solo-project-no-upstream-posts.md), which amends
    [ADR 0001](adr/0001-repository-and-upstream-sync.md)); and there is no Apple Developer account, so macOS builds
    are ad-hoc signed and never notarized (D11, [ADR 0003](adr/0003-ad-hoc-signing-no-apple-developer-account.md)).
  - The reference guard: whatever the repository's visibility, `port/tools/sync_upstream.sh` stops with exit code 5
    and the `port-sync` Action with the `references` outcome before pushing commits whose messages reference issues;
    `--allow-references` pushes them once the user agrees. The sync also checks strictly that `origin` is
    `CallanCampos/pokebw2-native`.
  - The write guard: `port/tools/hooks/port_guard.py`, a PreToolUse hook that `port/tools/bootstrap_checkout.sh`
    installs in the clone's `.claude/settings.local.json`, which Claude Code also reads in the clone's worktrees. The
    same script sets the `upstream` remote's push URL to `DISABLED-port-never-pushes-upstream`.
  - `port/CLAUDE.md`: `gh` writes only to the port, no @-mentions of upstream's people, port commit messages name
    only the port's own issues, and the root `CLAUDE.md`'s overlay claim and progress
    rules and the `decomp-file`, `finish-file` and `match-function` skills never run in the port clone. The plan,
    ADRs, landing page, game-change ledger and research index say the same. The scripts in
    `port/tools/research/sdk-surface/` take the repository as an argument, or find it from their own location,
    instead of a hardcoded worktree path, and write their outputs to `build/port-research/sdk-surface/`, which git
    ignores.
  - No branch of the decomp checkout holds the port commit `0835ef06c` any more, so no decomp session there can push
    it upstream. The `port-sync` Action was paused while these guards were written and is re-enabled after this push.
- **Next:** the session sync with `port/tools/sync_upstream.sh`, merging `69e82e80c` (none of its four new commits
  references an issue), then phase 0: CMake skeleton with SDL3, generated source lists and trap stubs,
  `include/platform.h`, the overlay `.data`/`.bss` spike, then the first arm64 macOS link.
- **Blockers:** none for phase 0. Overlay 36 (issue #83, claimed by mariano54) still gates phase 3. The four ITCM
  ClAct renderers (phase 2) have no owner; writing them is ordinary decomp work for a decomp session, which the user
  starts. D10, which the 2026-10-16 entry listed as waiting, is decided.

## 2026-10-17: local clone and first syncs

- **Upstream:** `462e90193` (merge of PR #345); unchanged since the repository was created.
- **Port:** `ecb9e5f65`.
- **ROM:** both SHA1s match (b2_us `e51e6dfb8`, w2_us `b5d7490be`), from the first run of
  `port/tools/sync_upstream.sh` in the new clone, which configured from scratch (downloading wibo, sjiswrap and dsd
  v0.12.1-dsi.4) and built both ROMs. 22 compiler warnings, the same upstream ov036 ones as before.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Tiers:** not re-measured; upstream hasn't moved since the 2026-10-16 entry.
- **Trap stubs:** not yet.
- **64-bit warnings:** not yet from a host build; the compile probe baseline in the 2026-10-16 entry stands.
- **Work done:**
  - The public repository `CallanCampos/pokebw2-native` exists with upstream's full history and the port commit
    `0835ef06c`. Upstream's `build.yml` is disabled here (it needs upstream's ROM deploy key); `compile.yml` and
    `standards.yml` pass on `main`.
  - The `port-sync` Action ran once by hand and reported upstream already merged. Its merge path runs at the next
    upstream push.
  - The local clone `~/dev/decomp/pokebw2-native` is set up: `origin` is the port, `upstream` is the decomp, gh's
    default repository is the port, `port/tools/bootstrap_checkout.sh` linked the inputs from the decomp checkout,
    and `CLAUDE.local.md` (`@port/CLAUDE.md`) with `.worktreeinclude` loads the port rules in every session.
  - `port/tools/sync_upstream.sh` ran end to end on the real build: fetch, already-merged check, configure, ninja,
    both SHA1s, nothing to push.
- **Next:** phase 0 — CMake skeleton with SDL3, generated source lists and trap stubs, `include/platform.h`, the
  overlay `.data`/`.bss` spike, then the first arm64 macOS link.
- **Blockers:** none for phase 0. Overlay 36 (issue #83, claimed by mariano54) still gates phase 3.

## 2026-10-16: repository created, research and plan complete

- **Upstream:** `462e90193` (merge of PR #345). The repository was created from upstream's full history at this
  commit.
- **Port:** `0835ef06c`.
- **ROM:** both SHA1s match (Black 2 `e51e6dfb…`, White 2 `b5d7490b…`), full `ninja` on `462e90193` with `port/`
  present. 22 MWCC "function has no prototype" warnings, all in upstream's new `src/ov036/field_g3dobj.c` and
  `src/ov036/fld_wipe_3dobj.c`, none from the port.
- **Host build:** not yet (phase 0).
- **Tests:** T0 as above; T1–T7 not yet.
- **Trap stubs:** not yet (no generator). The functions they will cover, those with no C, numbered 3,596 on
  `0c2bbee9d`.
- **64-bit warnings:** not yet from a host build. The compile probe's baseline on arm64 macOS (clang 23.1.3,
  `0c2bbee9d`): 1,164 unique pointer/int truncation sites in game code (`src/` and `include/`), 700 of them the
  `OVERLAY_ID(n)` macro, leaving 464 sites in 154 of 1,170 game files; 678 sites in 117 `lib/` files.

### Tiers

Measured on `0c2bbee9d`, three upstream merges (PRs #343–#345) before the seed commit, Black 2; White 2 has the same
tiers function for function ([decomp-progress.md](research/decomp-progress.md)). The next sync re-measures.

| Tier | Functions | Code bytes |
| --- | ---: | ---: |
| (a) complete file, matching C, linked | 31,068 (75.7%) | 2,751,648 (71.5%) |
| (b) C written, file incomplete, ROM code still linked | 6,354 (15.5%) | 747,236 (19.4%) |
| (c) no C at all | 3,596 (8.8%) | 351,360 (9.1%) |
| C exists, (a) + (b) | 37,422 (91.2%) | 3,498,884 (90.9%) |

Totals: 41,018 ARM9 functions and 3,850,244 code bytes, without 2,384 linker veneers. Of tier (b), 432 functions
(145 KB) have a row in `docs/nonmatching-functions.md`. About 84% of initialized data is C; 3,707 of 13,063 pointer
words in data are not (2,789 in overlay 36, 674 in overlay 12's event-script command tables).

When tier (c) reaches zero, at each area's own recent rate:

| Area | Left (bytes) | Rate, last 7 days | ETA | Rate, last 3 days | ETA | Who |
| --- | ---: | ---: | --- | ---: | --- | --- |
| ov036 field | 110,808 | 12.2 KB/day | ~Oct 25 | 27.1 KB/day | ~Oct 20 | mariano54 (claim; re-landing a fork branch) |
| ov011 Wi-Fi stack | 143,146 | 10.3 KB/day | ~Oct 30 | 16.4 KB/day | ~Oct 25 | mariano54 (claim; same fork) |
| ov182 Wi-Fi utility | 79,342 | 3.0 KB/day | ~Nov 11 | 2.9 KB/day | ~Nov 12 | mariano54 (claim) |
| LTD (TwlSDK, DSi) | 11,164 | lumpy | days | | | no issue tracks it |
| ITCM (ClAct renderers, IRQ) | 4,788 | 200 B in 20 days | none | | | no issue tracks it |
| main (crt0, SVC stubs, `exit`) | 2,112 | flat | none | | | no issue tracks it |

Overlays 36 and 11 could be C within one to two weeks at their recent rates. The rest of tier (c) only by
mid-November, if overlay 182 keeps its pace and someone takes the untracked ITCM, crt0 and DSi code. These are
extrapolations from a 20-day-old project whose progress comes in large merges.

### Compile probe

From [RESEARCH.md §9](RESEARCH.md#9-the-native-c-compile-probe): clang accepts all 1,170 `src/` files for 32-bit
targets, and for arm64 macOS once `OVERLAY_ID(n)` becomes `n` on hosts. Every remaining failure is in `lib/`: 43 files
with MWCC `asm` and 6 with dialect issues. i386 SysV matches MWCC's layouts exactly; save blocks are pointer-free and
keep the DS layout on 64-bit hosts with `aligned(4)` typedefs for `s64`/`u64`/`f64` and a typedef for two
`unsigned long` fields. The 64-bit work is estimated at 500–700 edited lines in about 160 game files.

### Work done

- Repository created from upstream's history; upstream's `build.yml`, which needs a ROM deploy key this repository
  doesn't have, disabled here with `gh workflow disable build.yml -R CallanCampos/pokebw2-native`.
- Research ([RESEARCH.md](RESEARCH.md) and 11 reports) and the plan ([PLAN.md](PLAN.md)).
- Measurement tools in `port/tools/` (progress tracker, compile probe, research scripts).
- Port rules (`port/CLAUDE.md`), the landing page (`.github/README.md`), `port/README.md`, the hourly sync Action
  (`.github/workflows/port-sync.yml`), the local sync and bootstrap scripts, the sync guide ([SYNC.md](SYNC.md)),
  [ADR 0001](adr/0001-repository-and-upstream-sync.md), this log, and the game-change ledger (empty).

### Current phase: 0, foundations

Exit criteria ([PLAN.md §7](PLAN.md#phase-0-foundations)): the executable links on macOS, starts, opens a window and
reaches `main()`; both ROMs still match; CI green.

### Next

1. CI for the host build, and the progress tracker and compile probe wired in as metrics for this log.
2. CMake skeleton with SDL3 in `port/third_party/`.
3. `port/tools/gen_sources.py`: source lists generated from `config/`, minus `hal/replaced.txt`; trap stubs; and
   placeholder definitions for data symbols with no C (zero-filled or loaded from the ROM at startup), so the host
   build links.
4. `include/platform.h` with `PLATFORM_PC`, `uptr`/`sptr` and the host typedefs; `OVERLAY_ID(n)` as `n` on hosts.
   These are game changes: each gets a row in [game-changes.md](game-changes.md).
5. Compile every game and library C file for arm64 macOS and fix 64-bit sites far enough to link: the 9 incompatible
   function-pointer casts, the 5 missing returns and the save-struct typedefs first.
6. Spikes: overlay `.data`/`.bss` sections on Mach-O, VRAM page aliasing on macOS, fibers with DS priority
   scheduling. Each ends in an ADR.

### Blockers

- None for phase 0: code and data with no C get trap stubs and placeholders (Next, item 3).
- Phase 3 (the overworld) needs overlay 36's C and data: issue #83, claimed by mariano54 on 2026-10-16 and being
  re-landed from a fork. Overlay 12's event-script command tables (674 pointer words) also need to become C.
- Phase 2 needs the four ITCM ClAct renderers as C; no issue tracks them and nobody owns them. Opening an issue for
  them is a public post that needs the user's OK, and writing them is decomp work for a decomp session in the decomp
  checkout, never for a port session.
- Decision D10 (proposing the portability conventions upstream) waits on the user's OK.
