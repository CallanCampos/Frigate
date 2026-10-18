# port/

**Frigate** is a native PC port of Pokémon Black 2 and Pokémon White 2, built from the pokebw2 decompilation; you
supply your own ROM. It compiles the decomp's C in this same tree with the platform layer in this directory:
everything the port adds lives here, and the rest of the tree is the decomp, merged from `fuddlesworth/pokebw2`. The
plan of record is [docs/PLAN.md](docs/PLAN.md), and the rules for working here are in [CLAUDE.md](CLAUDE.md). Frigate
is not affiliated with Nintendo, Game Freak, Creatures or The Pokémon Company.

## Building Frigate

The host build compiles every C file of `src/` and `lib/` with the host's clang into one program, except the files
[hal/replaced.txt](hal/replaced.txt) lists, which the platform layer replaces. Functions and data with no C yet are
trap stubs and inaccessible placeholders, generated at each build by [tools/gen_stubs.py](tools/gen_stubs.py): the
program stops with the name of the first one it reaches. It needs CMake 3.28 or later, Ninja, clang, Python 3.11 or
later and SDL3 3.4 (Homebrew's `sdl3`; without one, CMake builds the pinned SDL3 release).

```sh
cmake -S port -B build/port -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/port
ctest --test-dir build/port --output-on-failure
build/port/frigate-black2 --headless
```

`-DPORT_VERSION=w2_us` builds White 2 (`frigate-white2`). The ROM build (below) stays separate and must keep matching
after every port change; [tools/check_mwcc_neutral.py](tools/check_mwcc_neutral.py) proves a guard-only change to decomp
files leaves what MWCC compiles unchanged ([docs/game-changes.md](docs/game-changes.md)).

**Where it stands:** the game boots natively through its logos, the title screen and the start menu, with sound;
New Game's intro runs to the field's first script, and CONTINUE loads a save up to the field, which waits for the
decomp's overlay 36 (a trap stub stops the game there). [docs/STATUS.md](docs/STATUS.md) has the current state.

### Controls

The keyboard, by key position, and any gamepad at the same time. A gamepad's face buttons work by position, as on the
DS, which is a Nintendo pad's layout: the DS's A is the right face button whatever its label (B on an Xbox pad, Circle
on a PlayStation pad). The program names each pad's buttons, by the labels SDL reports, when the pad connects.

| DS | Keyboard | Gamepad |
| --- | --- | --- |
| A, B, X, Y | X, Z, S, A | right, bottom, top and left face buttons |
| L, R | Q, W | left and right shoulder buttons |
| Start, Select | Return; Right Shift or Backspace | start; back (View, Share or minus) |
| D-pad | arrow keys | D-pad, and the left stick past half its travel, diagonals included |
| Touch screen | the left mouse button on the bottom screen | |

The stick lets go only back inside seven eighths of half its travel, and keeps a direction until it leans about 4
degrees into the next one, so that a stick held near an edge doesn't flicker. Pads can come and go while the game runs.
With several, the first to press something drives the game until it lets go of everything; then the next to press
something takes over. Eight are open at once; a ninth waits until one of them is disconnected.
[app/present.c](app/present.c)'s header has the same table, and the gamepad's controls are one table in
[app/gamepad.c](app/gamepad.c) that a settings file will be able to replace.

### The macOS app

On macOS the host build also makes the program as an app, signed ad hoc and never notarized: there is no Apple
Developer account ([ADR 0003](docs/adr/0003-ad-hoc-signing-no-apple-developer-account.md)). The app is made locally
only; the port's compiled game code is never uploaded or distributed.

```sh
cmake --build build/port --target frigate-black2_app   # build/port/Frigate Black 2.app
port/tools/package_macos.sh                             # build/port/Frigate Black 2 0.0.N.dmg, with the app
```

- `-DPORT_VERSION=w2_us` makes `Frigate White 2.app` (target `frigate-white2_app`), and `-DPORT_MACOS_APP=ON` makes the
  app with every build. The menu bar and the About window call it Frigate; the Finder and the Dock, Frigate Black 2 or
  Frigate White 2.
  [app/macos/make_app.sh](app/macos/make_app.sh) builds it from the program the build linked: the program, the
  libraries it loads from outside macOS (Homebrew's SDL3, or an SDL3 installed with its `@rpath` name) in
  `Contents/Frameworks`, and an `Info.plist` from [app/macos/Info.plist.in](app/macos/Info.plist.in), which says where
  each of its values comes from. It checks that nothing in the app loads a library from outside macOS and the app,
  signs the whole app with `codesign --force --deep --sign -`, checks it with `codesign --verify --deep --strict`, and
  starts the program from the app once (`--help`) to see that it loads.
- **The bundle identifier** is `io.github.callancampos.frigate.black2` (`.white2` for White 2). macOS keeps an app's
  preferences and permissions under it. A build of your own that isn't Frigate's (a fork under another name) takes a
  reverse-DNS name of a domain of yours with `-DPORT_MACOS_BUNDLE_ID_PREFIX=`.
- The version comes from git: `0.0.N`, N being the number of commits built, and the commit in `PortSourceRevision`.
  The minimum macOS is the one the program and the libraries in the app were built for; with Homebrew's SDL3 that is
  the macOS Homebrew built SDL3 for.
- `package_macos.sh` puts the app and a link to `/Applications` in a compressed disk image, then checks the image, the
  app's signature in it, and that it holds no ROM or other game file. `--out PATH` puts it elsewhere, but never where
  git would take it into a repository, this one or another (only under an ignored directory such as `build/`, or
  outside every work tree). Its header lists its options.

**The ROM.** The app holds no ROM and no game data. It finds yours as the program does
([hal/rom](hal/rom/port_rom.h)): `--rom PATH`, then the `FRIGATE_ROM` environment variable, then the `orig/` of the
checkout the app was built in (`orig/baserom_b2_us.nds`, or `orig/baserom_w2_us.nds` for White 2). So an app built
from your checkout opens from the Finder with the ROM you placed for the ROM build, as long as the checkout stays
where it is (the program holds that checkout's path). Otherwise give the path when you open it:

```sh
open -n -a "build/port/Frigate Black 2.app" --args --rom ~/Games/black2.nds
open -n -a "build/port/Frigate Black 2.app" --env FRIGATE_ROM=$HOME/Games/black2.nds
"build/port/Frigate Black 2.app/Contents/MacOS/frigate-black2" --rom ~/Games/black2.nds
```

The app takes the program's options after `--args`. `-n` starts a new copy: without it, `open` only brings a copy that
is already running to the front, and the options are lost. Saves go to `~/Library/Application Support/Frigate/`
(`IREO.sav` for Black 2, `IRDO.sav` for White 2) unless `--save-dir` says otherwise.

**When the app stops.** Opened from the Finder or the Dock, the app has no terminal, so the program writes its messages
to `~/Library/Logs/Frigate/frigate-black2.log` (`frigate-white2.log`), emptied at each launch; Console shows it under
Log Reports. Without a ROM it can use, the app says why in an alert and quits. Any other stop closes the app with no
alert, and the log says why: a save file it can't use (one that isn't the 512 KiB the game's flash holds), the game
reaching code that has no C yet (a trap stub, the commonest today), the game stopping or resetting itself, or a crash.
Run from a terminal, or with `open --stderr PATH`, the messages go there instead.

**Gatekeeper.** An app built on your own Mac opens normally. One that came from elsewhere (a download, or a disk image
copied from another Mac) is quarantined, and macOS 15 and later block it until you allow it once:

1. Open the app. macOS says it can't verify it; click **Done**.
2. Open **System Settings → Privacy & Security** and scroll to **Security**, where it says the app "was blocked to
   protect your Mac". Click **Open Anyway** (it shows for about an hour after the blocked launch).
3. Confirm with **Open Anyway** and your password or Touch ID.

From then on the app opens normally. On macOS 14 and earlier, Control-clicking the app and choosing **Open** does the
same. Do this only for an app whose source you trust.

## Saves

The game's save is the cartridge's 4 Mbit flash chip, kept as a file of its 524,288 bytes in the save directory
(`--save-dir PATH`, else `~/Library/Application Support/Frigate` on macOS, `~/.local/share/Frigate` (or
`$XDG_DATA_HOME/Frigate`) on Linux, `%APPDATA%\Frigate` on Windows), named after the ROM's game code: `IREO.sav` for
Black 2 (US), `IRDO.sav` for White 2 (US). It is the same file as melonDS's `.sav` of the game and a cartridge dump's,
byte for byte:

- **From melonDS:** copy its `.sav` (named after the ROM's file, next to the ROM or in melonDS's save folder) into the
  save directory as `IREO.sav` or `IRDO.sav`. Copied under its own name, the port points it out and doesn't use it.
- **To melonDS:** copy `IREO.sav` or `IRDO.sav` next to the ROM under the ROM's name with `.sav` (`Pokemon Black 2.nds`
  takes `Pokemon Black 2.sav`), or into melonDS's save folder.
- **One save for both** (macOS and Linux): make `IREO.sav` or `IRDO.sav` a symbolic link to melonDS's `.sav`. The port
  reads that file and writes the game's saves into it, and the link stays a link.

A file of another size is refused, never overwritten, with what it seems to be: another chip's save by its size,
DeSmuME's `.dsv`, which has DeSmuME's own data after the image, this chip's image padded with 0xff (keep its first
524,288 bytes), or a directory. The port writes the file whole, through a temporary file renamed over it, a frame
after the game's writes end, and never writes a save the game only reads.

A save also holds the console it was last played on: its clock offset and MAC address. The port's console has offset 0,
as melonDS's does, so saves move between the two as they are. A save from a DS whose owner once set its clock holds
another offset: on CONTINUE the game then takes the clock for changed, as on that DS after a change of its clock, and
starts a day's wait for its daily events; the next save takes the port's console. Taking the save's console on import
instead is still to come (PLAN.md's D7 sidecar). [hal/card/port_card.h](hal/card/port_card.h) has the details and the
oracle's README the evidence ([tools/oracle/README.md](tools/oracle/README.md#saves)).

## Directory map

The layout from [PLAN.md §4.3](docs/PLAN.md#43-directory-structure). Entries marked **now** exist; the rest are
planned and appear as their phase starts.

```
port/
  README.md              now      this overview
  CLAUDE.md              now      rules for sessions working on the port
  CMakeLists.txt         now      host build: the decomp's C plus port/, linked into one program, and its tests
  app/                   now      main(), SDL3 window, gamepads, macos/ (the app's Info.plist and build script);
                                  later screen layouts, settings, paths, first-launch ROM setup
  include/               now      port_host.h, force-included into the decomp's C on a host (MWCC built-ins)
  hal/                   now      host side of the SDK, one directory per SDK module it replaces; trap.c for stubs
    os/ pxi/ hw/ gx/ dma/ mi/ fx/ math/ crypto/ rom/ card/ fs/ nvram/ rtc/ pm/ tp/ mic/ ctrdg/ reset/ input/
    snd7/ (the ARM7's sound driver) nnsys/ (NitroSystem: 2D files in host structs) gfl/ dwc/ stubs/
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
    package_macos.sh     now      the macOS app, signed ad hoc, in a disk image
    bootstrap_checkout.sh now     prepare a new clone or worktree for the ROM build and Claude Code
    hooks/port_guard.py  now      Claude Code hook that refuses writes to upstream and the decomp fork
    gen_host_tree.py     now      source list, include/ mirror without MSL's libc headers, generated constants
    gen_stubs.py         now      trap stubs and data placeholders for symbols with no C yet
    check_mwcc_neutral.py now     proves a guard-only change to decomp files neutral under MWCC (preprocessed text)
    oracle/              phase 1  melonDS test harness (a test tool, never linked into the port)
  docs/                  now      plan, research, sync guide (SYNC.md), status log, ADRs, game-change ledger
  third_party/           phase 0  pinned dependencies (SDL3, Dear ImGui, Lua, minicoro, ...)
```

PLAN.md also places sample mods in `mods/examples/` at the repository root (phase 6). Outside `port/`, the port's
own files are `.github/README.md`, `.github/workflows/port-sync.yml` (the hourly upstream merge) and
`.github/workflows/port-ci.yml` (the host build and tests); any other change outside `port/` is a game change listed in
[docs/game-changes.md](docs/game-changes.md).

## Preparing a checkout

Port work uses its own clone, not the decomp checkout. A clone or worktree needs the decomp's untracked inputs (the
base ROMs in `orig/`, `.venv` and the downloaded tools), which `port/tools/bootstrap_checkout.sh` links in from a
decomp checkout. The same script sets the checkout up for Claude Code and disables pushes to upstream (below). Its
header lists its options.

```sh
git clone https://github.com/CallanCampos/pokebw2-native.git
cd pokebw2-native
git remote add upstream https://github.com/fuddlesworth/pokebw2.git
gh repo set-default CallanCampos/pokebw2-native
port/tools/bootstrap_checkout.sh
PATH=/opt/homebrew/opt/llvm/bin:$PATH python3 configure.py && ninja
```

Without a decomp checkout, run `port/tools/bootstrap_checkout.sh --no-links`, which does everything but the links,
and follow the decomp's [README](../README.md) setup in this clone: place your dumps as `orig/baserom_b2_us.nds` and
`orig/baserom_w2_us.nds`, create `.venv`, and let `configure.py` download the tools.

Add the `upstream` remote before running the script: the script sets its push URL to
`DISABLED-port-never-pushes-upstream`, so a push there fails while fetching still works. `gh repo set-default` matters
because gh otherwise prefers the `upstream` remote, so a plain `gh issue create` would post on the decomp. Both are
kept in `.git/config`, which worktrees share, and the script says when either is missing.

`ninja` builds both ROMs and checks both SHA1s, as in the decomp's README. It takes about 5 minutes. ROM dumps are
your own and are never committed.

### For Claude Code sessions

Port work is done by Claude Code sessions under [CLAUDE.md](CLAUDE.md). Claude Code would load that file only once a
session reads something under `port/`, and the root `CLAUDE.md` alone sends a session to decomp work, whose skills
post claims and progress on upstream's issues. `bootstrap_checkout.sh` sets each checkout up so that the port's rules
load at startup and a session can't write upstream:

- `CLAUDE.local.md` holds `@port/CLAUDE.md`, so the port's rules load at startup.
- `.claude/settings.local.json` registers the write guard (below) as a `PreToolUse` hook for Bash, Monitor and the
  desktop app's terminal panel (`mcp__terminal__run_in_terminal`), which types commands into the user's own shell.
- `.worktreeinclude` lists both files, so Claude Code copies them into each worktree it creates: `claude --worktree`,
  subagent worktrees and desktop app sessions. It copies only files that git ignores. The tree's `.gitignore` ignores
  `.claude/settings.local.json`, and the script adds `/CLAUDE.local.md` and `/.worktreeinclude` to `info/exclude` in
  the git directory that worktrees share.

Run the script in the clone's root, where desktop app worktree sessions take their settings and hooks from, and in
each worktree made with `git worktree add`. It never replaces a file: it adds what is missing, replaces only the hook
an earlier version of it registered, and prints the hook to add by hand when `.claude/settings.local.json` isn't JSON
it can rewrite. Run it again after pulling a change to the hook. Also add one line to the clone's auto memory
(`/memory`), which all its worktrees share: "This clone is Frigate, the native port; read port/CLAUDE.md before
anything else." Codex isn't set up for port work: upstream's `AGENTS.md` gives it the decomp's rules only.

The hook runs `port_guard.py` from the session's own checkout (`$CLAUDE_PROJECT_DIR`), else from the clone's main
worktree, because a worktree checked out at `upstream/main` or at a port commit older than the guard doesn't have it.
When neither has it, the hook refuses every command on purpose, with a message that says so: a session whose commands
the guard can't check gets no shell. Checking out the port's `main` in the clone's root ends that.

By hand, the script's Claude Code and remote steps are these, from the checkout's root:

```sh
printf '@port/CLAUDE.md\n' >> CLAUDE.local.md
printf 'CLAUDE.local.md\n.claude/settings.local.json\n' >> .worktreeinclude
printf '/CLAUDE.local.md\n/.worktreeinclude\n' >> "$(git rev-parse --git-common-dir)/info/exclude"
git remote set-url --push upstream DISABLED-port-never-pushes-upstream
```

and this hook in `.claude/settings.local.json`, merged with what the file already has. Its command is one line, the
same as in the script:

```json
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "Bash|Monitor|mcp__terminal__run_in_terminal",
        "hooks": [
          {
            "type": "command",
            "command": "d=${CLAUDE_PROJECT_DIR:-.}; for r in \"$d\" \"$(git -C \"$d\" worktree list --porcelain 2>/dev/null | sed -n '1s/^worktree //p')\"; do [ -f \"$r/port/tools/hooks/port_guard.py\" ] && exec python3 \"$r/port/tools/hooks/port_guard.py\"; done; echo \"port_guard: port/tools/hooks/port_guard.py is neither in $d nor in the clone's main worktree, so no command can be checked. Check out the port's main branch in the clone's root, or start the session in a checkout that has port/.\" >&2; exit 2",
            "timeout": 10
          }
        ]
      }
    ]
  }
}
```

### The write guard

`port/tools/hooks/port_guard.py` refuses a Bash, Monitor or terminal panel command that would write to
`fuddlesworth/pokebw2` or to the decomp fork `CallanCampos/pokebw2`, link an upstream issue from the port, or
@-mention upstream's people ([ADR 0002](docs/adr/0002-solo-project-no-upstream-posts.md)):

- a gh subcommand that writes (create, comment, close, edit, merge, review, fork, transfer and the rest) when `-R`,
  `GH_REPO` or a repository or URL argument names either repository, or when it names none and gh's default here
  isn't the port; the text of a body, title or comment isn't taken for the repository written to;
- `gh api` with a method other than GET, or with fields or `--input`, on a path in either repository or on
  `{owner}/{repo}` that resolves to one; a `curl` that writes to either repository through the API;
- any GraphQL mutation, through `gh api graphql` or `curl`, because its target can't be told from the command, and a
  GraphQL query the guard can't see: one from a pipe, from a file it can't read, or built by the shell (`$(...)`, or a
  `$NAME` that isn't one of the query's own variables). Inline queries, quoted here-documents and readable files pass;
- `git push` to the `upstream` or `fork` remote, to a URL of either repository, or to a remote that pushes to one,
  including the branch's push remote when the command names none, and URLs that `git -c`, `git config` or
  `git remote` set earlier in the same command;
- a `git push` to GitHub that would add commits whose messages name an issue, pull request or discussion of any
  repository (`OWNER/REPO#N` or its URL), as `sync_upstream.sh` refuses them with exit status 5. The commits are those
  the pushed refs reach and the remote's tracking refs don't, with what a `git merge`, `rebase`, `reset`, `checkout`
  or `pull` earlier in the same command brings in. Each would record a reference to the port on that item's timeline;
- a gh write whose text links an upstream issue or pull request (`fuddlesworth/pokebw2#N` or its URL), and a
  `git commit`, `merge`, `cherry-pick` or `revert` whose message names an issue, pull request or discussion of any
  repository, including messages taken from other commits (`-C`, `--squash`, the picked commits);
- a gh write or commit message that @-mentions one of upstream's people: its human contributors as of 2026-10-17
  (`UPSTREAM_PEOPLE` in the script, fuddlesworth and mariano54 among them), since a mention can notify them.

Reads pass, and so does text that only mentions these names, such as a grep, an echo or a local file: the guard splits
the command as a shell would, here-documents, `$(...)`, `bash -c` and `find -exec` included, and checks a write's own
text, the files it reads and its here-document. It sees only the command, the files it names and the repository's
commits, so a script that runs gh or git itself isn't checked; it backs up CLAUDE.md's rules and doesn't replace them.
`python3 port/tools/hooks/port_guard.py --self-test` runs its test cases.

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
