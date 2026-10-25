# Port rules

This repository, `CallanCampos/Frigate`, is the decomp `fuddlesworth/pokebw2` with its full history, merged
continuously, plus Frigate, a native port, in `port/`. It was renamed from `CallanCampos/pokebw2-native` on 2026-10-18
at the user's request; GitHub redirects the old name. The plan of record is [docs/PLAN.md](docs/PLAN.md); where the
port stands is [docs/STATUS.md](docs/STATUS.md). The root `CLAUDE.md` is upstream's and applies here as this file says;
for port work, this file wins where the two differ.

## The port is Frigate

The user named the port Frigate on 2026-10-18, in the line of community ports such as Ship of Harkinian.

- **What it is, in one line:** Frigate is a native PC port of Pokémon Black 2 and Pokémon White 2, built from the
  pokebw2 decompilation; you supply your own ROM. Wherever the name is presented (the landing page `.github/README.md`,
  [README.md](README.md), the program's `--help`, the macOS app's About window), it also says that Frigate is not
  affiliated with Nintendo, Game Freak, Creatures or The Pokémon Company.
- **What the player sees is named Frigate:** the programs `frigate-black2` and `frigate-white2`; the apps
  `Frigate Black 2.app` and `Frigate White 2.app` (`CFBundleName` Frigate, bundle identifiers
  `io.github.callancampos.frigate.black2` and `.white2`) and their disk images; the window's title, "Frigate — Pokémon
  Black 2"; the folders of the saves and logs (`~/Library/Application Support/Frigate`, `~/Library/Logs/Frigate`, and
  `Frigate` under the user's data folder on Linux and Windows); and the `FRIGATE_ROM` variable. Each is written once.
  The product's name, its one-line description and the not-affiliated line are `PORT_PRODUCT_NAME`,
  `PORT_PRODUCT_DESCRIPTION` and `PORT_PRODUCT_DISCLAIMER` in [hal/os/port_app.h](hal/os/port_app.h), which the build
  reads from there for the macOS app's names and About window (the test `app_about` checks that `--help` says what the
  build read). The programs' names, the games' and the icons are at the top of [CMakeLists.txt](CMakeLists.txt); the
  macOS app's names, made from the product's, are in [cmake/macos_app.cmake](cmake/macos_app.cmake), and its bundle
  identifier's prefix in [app/macos/bundle_id.cmake](app/macos/bundle_id.cmake); the ROM's variable is
  `PORT_ROM_ENVIRONMENT_VARIABLE` in [hal/rom/port_rom.h](hal/rom/port_rom.h). Comments and documents name them too,
  and change with them.
- **Its artwork is in [assets/branding](assets/branding/README.md)**: the logo of the landing page and the READMEs,
  and the app icons that the macOS app and the window on Linux and Windows are built from. Its ship is Frigate's own
  drawing of the Plasma Frigate, the games' flying ship that the port is named after: the drawing is Frigate's and the
  design the games' owners', as the directory's README says, so no text calls the art free of the games' designs.
  Every file there is drawn by the directory's `tools/frigate_logo.py`, so a change to the artwork is a change to that
  script, rendered again with `tools/render.py` on a Mac and committed with the SVGs and PNGs it writes (the test
  `assets_branding` checks that the SVGs match the script, and that the small images' PNGs hold their pixels). The
  icons' 16 and 32 px images are drawn pixel by pixel in that script, not scaled. Nothing the build makes from the
  icons (the `.icns`, the window's bitmaps) is committed.
- **The layer keeps its own names:** the `port/` directory, the `port_` C prefix, the `PORT_*` CMake variables and
  `port_*` targets, `pokebw2_game` (the decomp's code), `pbw-oracle` and the `pbwinput` format, the `port-*.yml`
  workflows and the "port:" prefix of the program's messages. The repository itself is named Frigate,
  `CallanCampos/Frigate` ([above](#port-rules)); the port clone on this machine keeps its folder for now
  ([Where to work](#where-to-work)).
- **Living documents say Frigate** where they mean the port as a product; "the port" stays for the code and the layer.
  Records keep the names of their time: past STATUS.md entries, the research notes in `docs/research/` and RESEARCH.md,
  and ADR decisions as written (an ADR may get a one-line note).

## Code that modders can read

The user asked that the code be easy for people to read and find their way in: what everything does, and which part of
the game each piece touches, for the modders who come later.

- **Every port file opens with what it does and which game functionality it touches**: a header comment (a docstring
  in Python, a comment block in CMake, shell and YAML) that says what the file is for, which part of the game or the DS
  it serves (the saves, the title screen's 3D, the touch screen, the ARM7's sound driver), and where the code it works
  with lives (the decomp's files, the SDK module it replaces, its header). A reader who opens any one file knows what
  it does without opening another.
- **A new module gets its row in its directory's guide and in the architecture map**, in the same commit as the
  module: the guide is the directory's `README.md` (created with the directory's first module where there is none
  yet), and the architecture map is the feature map of [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md#feature-map), in
  the row of the game functionality the module serves. A new directory also gets its line in [README.md](README.md)'s
  directory map, a new document its line in [docs/README.md](docs/README.md), and a new term its row in
  [docs/GLOSSARY.md](docs/GLOSSARY.md).
- **Where a reader starts:** [README.md](README.md) (the front door and the directory map),
  [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) (the layers, a frame, the feature map) and
  [docs/MODDING.md](docs/MODDING.md) (changing the game today, and the mod platform to come). Keep them true when the
  code they describe changes.

## Where to work

- **Port work happens in the port clone**, `~/dev/decomp/pokebw2-native`, and its `.claude/worktrees/*`. The clone
  keeps that folder for now; a fresh clone's folder is `Frigate`. Its remotes: `origin` is `CallanCampos/Frigate` (the
  port), `upstream` is `fuddlesworth/pokebw2` (fetch only).
- **Decomp work happens in the decomp checkout**, `~/dev/decomp/pokebw2` (`origin` is upstream, `fork` is
  `CallanCampos/pokebw2`), under the root rules. Decomp PRs go from its `fork` remote, never from this repository.
  Decomp changes reach the port only by sync. A session in the wrong checkout for its task stops and says so.
- **This file loads at startup only through `CLAUDE.local.md`.** Claude Code loads a subdirectory's `CLAUDE.md` once
  a session reads a file there, and a worktree doesn't see the clone root's untracked files. So the clone root and
  every worktree have a `CLAUDE.local.md` containing `@port/CLAUDE.md`, kept out of git by `.git/info/exclude`, and the
  clone's auto memory says to read this file first. [README.md](README.md#preparing-a-checkout) has the steps. A
  session that finds its worktree without `CLAUDE.local.md` creates it.
- **gh targets the port.** gh prefers a remote named `upstream` to `origin` and can't ask which one in a Claude
  session, so each clone runs `gh repo set-default CallanCampos/Frigate` (kept in `.git/config`, which worktrees
  share). Every gh command that writes still names its repository, `-R CallanCampos/Frigate`.
  `-R fuddlesworth/pokebw2` is only for reads, never for writes (D10,
  [ADR 0002](docs/adr/0002-solo-project-no-upstream-posts.md)).
- **Port work is open to contributors.** Human contributors, Claude Code and explicitly authorized Codex sessions
  follow this file and [CONTRIBUTING.md](CONTRIBUTING.md). Root agent instructions describe the decomp; for port work,
  these rules take precedence. External contributors use personal forks and topic PRs to Frigate's `main`. Maintainers and automation also
  publish only topic branches and PRs ([ADR 0018](docs/adr/0018-protected-main-and-sync-prs.md)).
- **Port files live in `port/`**, plus files upstream doesn't have: `.github/workflows/port-*.yml` and
  `.github/README.md`, `.github/CONTRIBUTING.md`, `.github/CODEOWNERS`, `.github/PULL_REQUEST_TEMPLATE/port.md` and
  `.github/ISSUE_TEMPLATE/port_*.yml`. Never edit an upstream file for the port's own setup (root `CLAUDE.md`, `AGENTS.md`,
  `README.md`, `CONTRIBUTING.md`, `.gitignore`, `.claude/`, `.agents/`, `.codex/`, upstream's `.github/` templates and
  workflows, `docs/`, `tools/`, `config/`, `configure.py`): every sync would conflict.
- **Only upstream's `build.yml` is off here**, because it needs the ROM deploy key
  (`gh workflow disable build.yml -R CallanCampos/Frigate`). Disabling or enabling any other workflow needs the
  user's OK. Never disable `compile.yml` or `standards.yml` to make CI pass: fix the change instead.
- **Upstream's issue and pull request templates** stay unchanged. Port contributors use Frigate's added templates
  linked from [CONTRIBUTING.md](CONTRIBUTING.md) and report commands, results and unrun checks.

External contributors update from Frigate's `main` and push topic branches to their own forks. They do not run
`sync_upstream.sh`, whose canonical-origin check and publishing are for maintainers. Contributors without ROMs may
submit draft PRs with unrun checks stated; maintainers verify both ROM SHA1s and applicable host tests before merge.
The no-upstream-posts and reference rules below still apply to every contributor.

## How the root rules apply here

- **Both ROMs stay byte for byte.** Every maintainer commit ends with `python3 configure.py && ninja` and both SHA1s, even one
  that touches only `port/`: it proves the last sync and every change outside `port/` left the DS build alone (T0).
- **The code rules apply to any edit in `src/`, `lib/` or `include/`**: natural C, names, `docs/code-standards.md`,
  clang-format 23.1.1. `post_edit.py` and CI's Standards workflow check those paths; they don't look at `port/`.
- **Publish every commit through a PR.** Fetch `origin/main`, create a feature branch, and push that branch to
  Frigate (maintainers) or a personal fork (contributors). Never push `HEAD:main`. Open a PR targeting Frigate `main`;
  merge updates from `origin/main` and repeat verification when the base changes. Required CI and local verification
  must pass before merge. The sole owner may bypass only the separate review rule for owner-authored PRs; there is
  no bypass for the PR, CI, conversation-resolution, deletion or force-push rules. Contributor and bot sync PRs get
  explicit owner approval before merge. Automatic CI does not approve code. The least restrictive fork CI policy
  only requires run approval for first-time contributors with new GitHub accounts; inspect code/workflows if GitHub
  requests that run approval. See [repository policy](docs/REPOSITORY-POLICY.md).
- **Never here:** overlay claims, matching, new decomp source files, config changes (`add_source_file.py`,
  `mark_complete.py`, `rename_symbol.py`, `config_fixes.py`), and edits to `docs/nonmatching-functions.md` or
  `docs/matching.md`. The `decomp-file`, `match-function`, `finish-file` and `record-lesson` skills and the root
  "Claim an overlay", "Record overlay progress" and "Learn" rules belong to the decomp checkout
  ([below](#upstream-doesnt-know-about-the-port)). **Keep going** means the next task of the loop.
- **`fix-build`** helps diagnose a ROM build failure here, but the fix is always to the port's own change (rework or
  revert it), never a config, `delinks.txt`, symbol or rename change. A failure that upstream's `main` shows on its
  own is recorded as a blocker in STATUS.md and told to the user, never fixed here or reported upstream.

## Staying in sync ([docs/SYNC.md](docs/SYNC.md))

- **Syncs are reviewed PRs.** The hourly `port-sync` Action proposes one bounded upstream merge on a topic branch.
  It never pushes `main`, auto-merges, or updates a pending PR. Its App-authenticated proposals start PR CI
  automatically. Review the proposed changes and verify its exact head locally with
  `port/tools/sync_upstream.sh --verify-pr NUMBER` before approving and merging.
- **App setup is separate from code review.** [SYNC.md](docs/SYNC.md#set-up-the-dedicated-sync-app) owns the App setup
  procedure. Missing setup must fail visibly. Keep the default Actions token's combined create/approve-PR permission
  disabled. Never give the App a review/check bypass or local verification authority.
- **Start work from Frigate's reviewed `origin/main`.** Check the pending sync PR separately; do not automatically
  import unreviewed upstream changes into feature work. The local `port/tools/sync_upstream.sh` is a dedicated sync
  operation that validates and publishes a topic PR, never a pre-task direct publisher.
- **Preserve port changes.** Use real upstream merges, keep the `Upstream-Commit` trailer, and check
  [game-changes.md](docs/game-changes.md). Resolve conflicts by taking upstream's updated structure and reapplying
  the documented port adaptation. Both ROMs and host tests must pass after resolution.
- **Stops stay visible.** Conflicts, history rewrites, cross-repository references and upstream workflow changes
  require local inspection. The hourly sync shows a conflict as a draft PR of upstream's head, merging nothing,
  which a maintainer resolves locally ([SYNC.md](docs/SYNC.md#conflicts-and-stopped-syncs)); the other three stop the
  run before it pushes anything. Only the user authorizes `--allow-rewrite` or `--allow-references`. Never silently
  resolve conflicts or modify decomp configs to get a sync through. Check that upstream's ROM-dependent `build.yml`
  stays disabled after workflow imports; Compile and Standards remain enabled.

## The iteration loop (PLAN.md §8)

1. **Update** from Frigate's reviewed `main`, then regenerate source lists and stubs. Handle upstream sync PRs
   separately as above.
2. **Pick** the earliest unmet exit criterion of the current phase (STATUS.md names it) that the decomp allows.
3. **Build it in small commits**, each passing the pre-push tests. Large subsystems (renderer, sound driver,
   scheduler) go to subagents in their own worktrees, one writer per set of files.
4. **Verify** with the replay and oracle tests, and run the app and look at screenshots for anything visual.
5. **Record**, in the same commit as the work: a [STATUS.md](docs/STATUS.md) entry after each session sync and each
   finished task (a proposed PR gets no completed-sync entry), an [ADR](docs/adr/README.md) for each non-obvious choice, a row in
   [game-changes.md](docs/game-changes.md) for every change outside `port/`, and lessons in `port/docs/lessons.md`
   (created with the first one), never in `docs/matching.md` or the skills.
6. **Publish** a topic branch and PR as above; merge only after required validation and review.

## Changes outside port/ (PLAN.md §4.1, §4.8)

Game code stays the decomp's. Only three kinds of change are allowed outside `port/`:

1. **Matching-neutral portability fixes**, such as `uptr` for a value that carries an address. They stay in this
   repository; none is proposed upstream.
2. **Hook macros** (`PORT_EVENT`, `PORT_SHOULD`) that compile to nothing, or to the bare expression, in the ROM build.
3. **`#ifdef PLATFORM_PC` blocks**, rare, each with a comment saying why. The host's types are defined in one place,
   `include/types.h`'s `PLATFORM_PC` block (`uptr`, `sptr`, `ulong32` and `PORT_DS_ALIGN_64`, a port change with its
   rows in [game-changes.md](docs/game-changes.md)). Any other definition that must differ between MWCC and a host
   branches on `PLATFORM_PC` in its own header, such as `OVERLAY_ID` in `include/gfl/overlay.h`, `HeapBlockHeader`'s
   padding, NitroSDK's `math.h` and `snd.h` and DS Protect's checked calls, each with its row in game-changes.md.
   `PLATFORM_PC` itself is defined only by the host build (`port_decomp_env` in [CMakeLists.txt](CMakeLists.txt)),
   never by a header, and the host build force-includes [port/include/port_host.h](include/port_host.h) into every
   decomp file.

All three need the same proof that MWCC's output didn't change. The ROM build proves a complete file. An incomplete
file still links the original object, so the ROM build proves nothing about its C: show with
`compiler_probe.py --mismatches` in both versions that no function got worse, and for a header change do it for every
incomplete file that includes the header. [game-changes.md](docs/game-changes.md) has the steps.

Each gets a row in game-changes.md in the same commit. Never copy a decomp file into `port/` to change it; source
lists are generated from `config/`, never kept by hand. The one sanctioned transformation is the build-time host
variant of a header that names DS addresses ([ADR 0004](docs/adr/0004-ds-addresses-by-header-translation.md)): it is
generated into `build/` at every configure, never committed or edited; change the generator instead. `BUGFIX` stays
off, so gameplay matches the reference, but memory-safety bugs (out-of-bounds writes, use after free) are fixed under
`PLATFORM_PC`, each with a comment, because 64-bit layouts move what they hit. No new `long`; structs on disk, on the
wire or in the save stay pointer-free and get host `_Static_assert`s.

## Code that isn't C yet

- **Trap stubs show it**: a function with no C stops the game with its name, address and caller. Never run,
  interpret, recompile or translate the ROM's ARM code, and never write a stand-in in `port/` to get past a missing
  game function. The HAL's SDK replacements and the ARM7 sound driver are port code by design (PLAN.md §4.5, §4.7).
- **Writing the missing C is decomp work.** For an overlay, read its `[Decomp] Overlay NNN` issue and comments
  (`gh issue view N -R fuddlesworth/pokebw2 --comments`). If someone holds a claim, the task waits: record the
  blocker in STATUS.md and pick another. Code with no overlay issue (the ITCM, ARM9 main, LTD) has no claim yet. The
  four ITCM ClAct renderers that phase 2 needs are such code.
- **When the port is blocked on missing C, a port session may do the decomp work itself** (the user allowed it on
  2026-10-17): in its own worktree of the decomp checkout (`~/dev/decomp/pokebw2/.claude/worktrees/...`), never in
  this clone, on a topic branch from upstream `main`, under the root rules and skills there (claim an overlay's issue
  first, C from the asm, both SHA1s, `finish-file`, a fork PR), as ordinary decomp work: nothing it commits, comments
  or posts mentions the port. Work under someone else's active claim stays theirs; the port waits or uses a documented
  stand-in for a deferred subsystem where the plan documents one. No stand-in is left: the `ov011_pending.c` stand-in
  was removed once all four SDK definitions had C, and Frigate's test launcher
  ([ADR 0012](docs/adr/0012-test-launcher-for-the-field-menus-apps.md)), which opened the field menu's apps while
  overlay 36 had no C, was retired once the field opened. The C reaches the port by sync once upstream merges it.

## What native means (PLAN.md §1)

- All code runs as host machine code compiled from the decomp's C. There is no CPU emulator.
- The ROM is a data source the player supplies. Nothing from it is committed: no ROMs, extracted files, VRAM or memory
  dumps, or captured frames. The golden images and captured display state that PLAN.md §4.3 and §6 (T2) call for are
  committed only as hashes, or built locally from the player's dump.
- melonDS is a test oracle (`port/tools/oracle/`) only, never linked into or shipped with the port.
- libntr, the Platinum port and other ports are references for patterns, never code to copy.

## Testing (PLAN.md §6)

| Level | What | When |
| --- | --- | --- |
| T0 | ROM build, both SHA1s | before every maintainer push and PR merge |
| T1 | Host build; the pointer cast guard (`game_ptrcasts`) passes, its real sites not above STATUS.md's last entry | before every push |
| T2, T6 | Unit tests; save round trips and ROM dump verification | before every push |
| T3 | Replay tests: frame hashes and state checkpoints | short ones before every push, long ones nightly |
| T4 | Oracle comparison with melonDS | nightly, and to close a milestone |
| T5 | Replays under ASan and UBSan | nightly |
| T7 | Scripted playtests with screenshots against the milestone checklist | at each milestone |

A level that doesn't exist yet is marked "not yet" in the STATUS.md entry, never skipped silently. A phase closes
only when its exit criteria pass on macOS.

## Environment and context

- Python is `.venv/bin/python`. `configure.py` needs Homebrew LLVM first on `PATH`
  (`PATH=/opt/homebrew/opt/llvm/bin:$PATH`). A full `ninja` takes about 5 minutes and 2.2 GB; exit code 139 is an
  intermittent wibo crash, so run it once more.
- A new clone or worktree is prepared with `port/tools/bootstrap_checkout.sh`, which links `orig/`, `.venv` and the
  tools from the decomp checkout and installs the write guard ([below](#upstream-doesnt-know-about-the-port)), and the
  Claude Code steps in [README.md](README.md#preparing-a-checkout).
- Long jobs (`ninja`, the sync, the progress tracker, the compile probe, replays) run with `run_in_background` or
  Monitor, never `sleep`. Never print a whole `.s` file, research report or log; use `grep -n` and `tail`.
- Write only in your own worktree. If a hook refuses a write, stop and report it; never route around it with the
  shell. `guard.py` refuses Write and Edit to any path with an `orig/`, `extract/`, `build/` or `.venv/` component and
  to any `*.sha1` file, so port sources avoid those names. The host build goes in `build/port` (ignored), as
  `sync_upstream.sh` does.
- Stage files by name, never with `git add -A` or `git add .`, and never use a bare `git stash`.

## Upstream doesn't know about the port

The port accepts external contributions ([ADR 0017](docs/adr/0017-external-contributions.md)).
The independent-upstream boundary of PLAN.md D10 and [ADR 0002](docs/adr/0002-solo-project-no-upstream-posts.md) remains.

- **Never post anything about the port on `fuddlesworth/pokebw2`**: no issues, comments, reactions, pull requests,
  discussions or proposals, and no mention of the port in decomp PRs or claims made from the decomp checkout. Read-only
  `gh` against upstream is fine.
- **Never link upstream issues or pull requests** (`fuddlesworth/pokebw2#NNN` or their URLs) from port issues, pull
  requests, comments or commit messages: GitHub shows such links on upstream's timelines. Plain `#NNN` text inside
  committed files is fine. Commit links (`fuddlesworth/pokebw2@<sha>`) are fine.
- **Port commit messages name only the port's own issues**, as a bare `#N`, and port issues, pull requests and comments
  should too. They carry no `OWNER/REPO#N` and no issue, pull request or discussion URL of any repository, a third
  party's included (ds-decomp, SDL): name a third-party issue by its title. GitHub records such a reference on the other
  repository's issue, and the sync's reference check, which doesn't tell repositories apart, also stops on an
  unpublished port commit that has one.
- **Never @-mention upstream's maintainers or contributors** (for example fuddlesworth or mariano54) in port issues,
  pull requests, comments or commit messages: a mention can notify them.
- **Upstream's own commit messages can reference issues** (`OWNER/REPO#N`, or an issue, pull request or discussion
  URL), and pushing such a commit can record a "referenced" event, naming this repository, on that issue. So the sync
  stops before pushing commits whose messages reference issues, whatever the repository's visibility (a private
  repository's references still show, without their details): `sync_upstream.sh` exits 5 and the Action fails with a reference error. Stop and ask the user; only the user decides to run `--allow-references`
  ([SYNC.md](docs/SYNC.md#commit-messages-that-name-other-repositories)).
- **The root `CLAUDE.md`'s overlay rules and the decomp skills never run in this clone.** Its "Claim an overlay" and
  "Record overlay progress" rules and the `decomp-file`, `finish-file` and `match-function` skills post claims and
  progress comments, with commit links, on upstream's issues; run from this clone, they would post about port commits
  on upstream. They belong to the decomp checkout.
- **The write guard is never bypassed.** `port/tools/hooks/port_guard.py` is a PreToolUse hook that
  `bootstrap_checkout.sh` installs in the clone's `.claude/settings.local.json`. In a worktree, Claude Code 2.1.211 and
  later read that file from the clone's root (not on Windows), so the hook covers every worktree of the clone, and only
  a new clone needs bootstrap for it; `.worktreeinclude` also copies the file into new worktrees, as a fallback. If the
  hook refuses a command, stop and report it; never route around it with another tool or the shell.
- **Port findings stay here.** Unlinked C that the port shows behaving differently from the ROM is recorded in
  `port/docs/decomp-findings.md` (created with the first one), and so are PLAN.md §6's equivalence notes; the user
  decides whether a decomp session fixes a finding as ordinary decomp work.
- **Never push to `upstream`; never force-push.** `bootstrap_checkout.sh` disables the `upstream` remote's push URL
  by setting it to `DISABLED-port-never-pushes-upstream`. Run it again whenever `git remote -v` shows another push URL
  for `upstream`, as it does after the remote is added again. `sync_upstream.sh` runs only in a checkout whose `origin`
  is `CallanCampos/Frigate`; for a clone whose `origin` still has the old name, it prints the command that updates it.
  A worktree whose branch predates the rename runs the old script, which asks for the old name: merge `origin/main`
  there and run it again, never set `origin` back.
- **macOS builds are ad-hoc signed** (`codesign --force --sign -`) and never notarized: there is no Apple Developer
  account (PLAN.md D11, [ADR 0003](docs/adr/0003-ad-hoc-signing-no-apple-developer-account.md)). Nothing in the build
  may need a paid certificate, entitlements that require one, or notarization.
