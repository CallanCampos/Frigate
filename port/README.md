# port/

The native port of Pokémon Black 2 and White 2, built from the decomp's C in this same tree. Everything the port adds
lives here; the rest of the tree is the decomp, merged from `fuddlesworth/pokebw2`. The plan of record is
[docs/PLAN.md](docs/PLAN.md), and the rules for working here are in [CLAUDE.md](CLAUDE.md).

## Building the port

Not yet: the host build arrives in phase 0. Until then the only build is the decomp's ROM build (below), which must
keep matching after every port change.

## Directory map

The layout from [PLAN.md §4.3](docs/PLAN.md#43-directory-structure). Entries marked **now** exist; the rest are
planned and appear as their phase starts.

```
port/
  README.md              now      this overview
  CLAUDE.md              now      rules for sessions working on the port
  CMakeLists.txt         phase 0  host build; reads the decomp's file lists from config/
  app/                   phase 0  main(), SDL3 window, screen layouts, settings file, paths, first-launch ROM setup
  hal/                   phase 0  host side of the SDK, one directory per SDK module it replaces
    os/ fs/ card/ input/ rtc/ pm/ mic/ mi/ fx/ gx/ snd/ stubs/
    replaced.txt                  SDK source files the host build compiles from hal/ instead of lib/
  gfx/                   phase 1  ref/ (CPU reference renderers), ge/ (geometry engine), gpu/ (SDL_GPU), shaders/
  audio/                 phase 2  SDL3 audio output, resampling, mic input
  vfs/                   phase 1  ROM reader, SHA1 manifest, layered mounts, NARC member overrides
  save/                  phase 4  save image plus sidecar JSON for port and mod data
  mod/                   phase 6  mod loader, manifest, hook bus, events, data registries, Lua runtime
  features/              phase 6  built-in enhancements: widescreen/ randomizer/ fastforward/ hd/ qol/
  net/                   phase 7  GFL network device over UDP, LAN discovery, relay client
  ui/                    phase 6  ImGui menus and debug views
  tests/                 phase 1  unit tests, replay tests, golden hashes, oracle comparisons
  tools/                 now      measurement scripts, sync and bootstrap scripts (see tools/README.md)
    progress/            now      port readiness of the decomp: tiers per function, data, pace, claims
    compile-probe/       now      host-compiler probe: 64-bit warnings and struct layouts
    research/            now      one-off research scripts and the NitroFS SHA1 manifest
    sync_upstream.sh     now      merge upstream and verify both ROMs and the host build
    bootstrap_checkout.sh now     prepare a new clone or worktree
    gen_sources.py       phase 0  generated source lists and trap stubs
    oracle/              phase 1  melonDS test harness (a test tool, never linked into the port)
  docs/                  now      plan, research, sync guide (SYNC.md), status log, ADRs, game-change ledger
  third_party/           phase 0  pinned dependencies (SDL3, Dear ImGui, Lua, minicoro, ...)
```

PLAN.md also places sample mods in `mods/examples/` at the repository root (phase 6). Outside `port/`, the port's
own files are `.github/README.md` and `.github/workflows/port-*.yml` (now `port-sync.yml`, the hourly upstream merge);
any other change outside `port/`, such as the planned `include/platform.h`, is a game change listed in
[docs/game-changes.md](docs/game-changes.md).

## Preparing a checkout

Port work uses its own clone, not the decomp checkout. A clone or worktree needs the decomp's untracked inputs (the
base ROMs in `orig/`, `.venv` and the downloaded tools), which `port/tools/bootstrap_checkout.sh` links in from a
decomp checkout. Its header lists its options.

```sh
git clone https://github.com/CallanCampos/pokebw2-native.git
cd pokebw2-native
git remote add upstream https://github.com/fuddlesworth/pokebw2.git
gh repo set-default CallanCampos/pokebw2-native
port/tools/bootstrap_checkout.sh
PATH=/opt/homebrew/opt/llvm/bin:$PATH python3 configure.py && ninja
```

Without a decomp checkout, skip the bootstrap script and follow the decomp's [README](../README.md) setup in this
clone: place your dumps as `orig/baserom_b2_us.nds` and `orig/baserom_w2_us.nds`, create `.venv`, and let
`configure.py` download the tools.

`gh repo set-default` matters because gh otherwise prefers the `upstream` remote, so a plain `gh issue create` would
post on the decomp. The setting is kept in `.git/config`, which worktrees share.

`ninja` builds both ROMs and checks both SHA1s, as in the decomp's README. It takes about 5 minutes. ROM dumps are
your own and are never committed.

### For Claude Code sessions

Port work is done by Claude Code sessions under [CLAUDE.md](CLAUDE.md). Claude Code would load that file only once a
session reads something under `port/`, and the root `CLAUDE.md` alone sends a session to decomp work. Make the port's
rules load at startup, once per clone, from its root:

```sh
printf '@port/CLAUDE.md\n' > CLAUDE.local.md
printf 'CLAUDE.local.md\n' > .worktreeinclude
printf '/.worktreeinclude\n' >> .git/info/exclude
```

`bootstrap_checkout.sh` already keeps `CLAUDE.local.md` out of git through `.git/info/exclude`. `.worktreeinclude`
makes Claude Code copy that file into each worktree it creates; in a worktree made another way, run the first line
there too. Also add one line to the clone's auto memory (`/memory`), which all its worktrees share: "This clone is
the native port; read port/CLAUDE.md before anything else." Codex isn't set up for port work: upstream's `AGENTS.md`
gives it the decomp's rules only.

## Syncing with the decomp

Two mechanisms keep this repository on top of upstream; [docs/SYNC.md](docs/SYNC.md) describes both and what to do
when one stops.

- Every hour, the `port-sync` Action (`.github/workflows/port-sync.yml`) merges upstream's `main` into the port's
  `main` when the merge is clean and every C file still compiles for both versions. CI has no ROMs, so it can't check
  the SHA1s. A conflict, a compile failure, a change to upstream's workflows or a rewrite of upstream's history leaves
  `main` as it was and opens an `upstream-sync` issue.
- Locally, `port/tools/sync_upstream.sh` merges upstream on top of that, builds both ROMs and checks both SHA1s,
  builds and tests the host side once `port/CMakeLists.txt` exists, and pushes only when all of it passes. Run it
  before starting a task and at least daily: upstream lands 150–250 commits a day. Exit status 3 is a merge conflict
  and 4 an upstream history rewrite.

Why the repository works this way is [ADR 0001](docs/adr/0001-repository-and-upstream-sync.md). Record each local sync
in [docs/STATUS.md](docs/STATUS.md).

## Documents

| Document | Covers |
| --- | --- |
| [docs/PLAN.md](docs/PLAN.md) | Goals, architecture, mod design, testing, roadmap: the plan of record |
| [docs/RESEARCH.md](docs/RESEARCH.md) | Findings behind the plan; full reports in [docs/research/](docs/research/README.md) |
| [docs/SYNC.md](docs/SYNC.md) | How the port follows upstream: the hourly Action, the local sync, conflicts |
| [docs/STATUS.md](docs/STATUS.md) | Status log, newest first, one entry per local sync or finished task |
| [docs/game-changes.md](docs/game-changes.md) | Every change the port makes outside `port/` |
| [docs/adr/](docs/adr/README.md) | Architecture decision records |
| [tools/README.md](tools/README.md) | The measurement scripts and how to run them |
| [CLAUDE.md](CLAUDE.md) | Rules for Claude sessions working on the port |
