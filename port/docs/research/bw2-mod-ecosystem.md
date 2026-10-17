# Pokémon Black 2 / White 2: the mod ecosystem, for a native port

Research snapshot of 2026-10-16. Scope: what exists around BW2 that a native PC/Mac port built from the pokebw2
decompilation (github.com/fuddlesworth/pokebw2) must interoperate with or replace, for three long-term mod goals —
**widescreen**, a **randomizer** and **multiplayer** — and for modding in general. Sources: web and GitHub
(read-only `gh`), and read-only inspection of a local pokebw2 checkout. Local paths are relative to the pokebw2
checkout. Addresses are Black 2 (US) unless marked W2. Claims resting on one weak source, a blocked page or
inference are marked **unverified** inline. Each section ends with its own sources.

## Executive summary

**Ports and decomps (section 1).** No PC port, static recompilation or native reimplementation of any Gen 5 game
exists publicly. The fan "remakes" are engine demos that don't run game logic. pokebw2 is far ahead of every other
Gen 5 code project: ByteMatch reports 82.89% of Black 2's code matched and 36,848 of 43,410 functions, with both
USA/Europe ROMs rebuilding byte for byte. The others are a Pokémon Black decomp at 0.03%, a private Black decomp at
3.23% and a two-week-old BW2 disassembly. pret has no Gen 5 repository. **swan**
(github.com/ds-pokemon-hacking/swan, GPL-3.0) is the Gen 5 hacking community's generated header set and symbol
database (`root.swandb`, ESDB YAML for White 2 with ~58k symbols, Black 2 with 984), maintained by HelloOO7 and
PlatinumMaster. It is what PMC code-injection mods compile against. pokebw2 imports its names at revision `4324f73`
(2025-07-03), swan's last commit.

**Randomizer (section 2).** UPR ZX's Gen 5 handler and its active successor UPR FVX read and write 22 BW2 archives (FVX adds the shop-size one)
(personal, learnsets, evolutions, babies, moves, items, scripts, text, trainers, trainer text boxes, PWT Driftveil,
egg moves, zone events, encounters, trades, Pokédex area data, starter art, Hidden Grottoes, shops). Each of those
works automatically if the port loads its assets from the user's ROM. Eight tables live in arm9 or overlay data,
where a compiled port would ignore them: TM/HM moves, TM palettes, tutors, starter cries, intro species ×2, Pickup,
type chart. There are 16 true code patches, several applied to every US ROM whatever the settings: an ITCM
extension, the Shedinja hook and the intro Pokémon. The decomp already models nearly all of the archives as
schema-validated JSON or assembly in `data/`. The exceptions are Hidden Grottoes `a/2/7/3`, shops `a/2/8/2-3`,
starter art and Pokédex area data. All eight in-binary tables are named C constants (`TM_MOVE_LIST`, the four
`MOVE_TUTOR_ITEMS_*` tables, `sPselSpecies`, the Pickup tables, the type chart at UPR's exact address
0x021D7500). A native randomizer has almost everything it needs.

**Widescreen (section 3).** Only one BW2 widescreen hack exists: TWiLight Menu++'s 16:10 code for 3DS TWPatch,
which changes the overworld camera's aspect literal in `FieldG3D_Init` and nothing else. melonDS only stretches the
picture; the WideMelon fork widens 3D but not 2D. In the decomp, 3D widescreen is tractable. Projections funnel
through `GFL_G3DSysMtxSetProjection`, culling uses the hardware box test, and map streaming is a per-map-type table.
The 2D side is the hard part, because every BG, OBJ, window, HBlank effect, capture and movie is authored for
256×192. The plan: Hor+ for 3D scenes, pillarbox for 2D-only screens, edge anchors for HUDs, and the bottom screen
kept as a 4:3 touch panel.

**Multiplayer (section 4).** Every connected mode except Download Play goes through one table, `GFLNetDevTable`.
The game picks the wireless (ov030), IR (ov031) or Wi-Fi (ov011) device in `src/system/net_define.c`. Many
signature Gen 5 features never connect at all: Tag Mode, surveys, Funfest Missions with up to 100 players, beacon
Mystery Gift, Union Room presence. Emulators can't sync these, but a port can relay them trivially. Revival today
(DNS 178.62.43.212: Kaeru WFC in front of Wiimmfi, plus the Poké Classic Network) covers the GTS, Battle Videos,
Musical photos, the Wi-Fi Battle Subway and some Mystery Gifts. Random Matchup needs a client patch, because the
RSA party signature can't be forged. Game Sync and Dream World work only through self-hosted Entralinked. Rating
battles, Global Link competitions and PWT downloads have no revival. Recommended order:
- a WH-level LAN/relay shim, keeping the game's protocol;
- IR as pairing over the same relay;
- save import in place of Download Play;
- DWC over host sockets with NitroWiFi's own SSLv3 compiled natively, for cross-play on revival servers.
That last step needs more decomp first: about two-thirds of overlay 11, 972 DWC/GameSpy functions, isn't in source
files yet.

**Hacks and tools (section 5).** The scene targets the same clean US ROMs (same SHA1) and ships xdelta patches.
- **Hacks:** Blaze Black 2/Volt White 2 is data-only. Redux adds Fairy by code patch. White 2 Upgrade (721, Fairy)
  is one PMC module.
- **CTRMap-CE + CTRMapV:** projects are a YAML `.cmproj` plus a sparse `vfs/` copy of the ROM tree, which a port
  can mount directly.
- **PMC:** hooks are declared by swan function names. The decomp uses the same names, so a source-compatible hook
  API is plausible. Binary DLLs are ARM code and can't run natively.
- **Other tools:** Frost's Gen 5 Editor (with its own per-NARC-member `.pokepatch`) and Pokeweb-Serverless, which
  bundles about 30 PMC QoL modules.
- **Gen 4 only:** PokEditor v2/v3, DSPRE, PPRE.
- **"Hotspot":** probably PlatinumMaster's Hotswap.

**Recommendations (section 6).** Build on the decomp's four seams:
- **Archives:** NARC paths in `gfl_use.c`, read through NitroSDK FS. Make this a layered VFS: base ROM, xdelta,
  CTRMap `vfs/`, per-member overrides.
- **Tables:** a symbol-keyed "patchable tables" registry, with a ROM extractor for data that lives in code.
- **Imports:** an import report that diffs code against the clean ROM.
- **Render policy:** hooks at the GFL layer.
- **Network:** a host `GFLNetDevTable` device.
- **Hooks:** a function API keyed by swan/decomp names, an event bus and script-command extension.

Decide early: version switching (B2/W2 in one binary), US-only at first, the 0x80000 save format (PKHeX/Entralinked
compatibility), and whether to lift the 17-type and 649-species limits before the save and data formats freeze.

## Contents

1. Existing ports, recomps and Gen 5 decomps
2. Randomizer: UPR ZX Gen 5 handler (2.10: cross-check against the decomp's `data/`)
3. Widescreen
4. Multiplayer (4.0: the decomp's network stack)
5. ROM hacks and tools
6. Recommendations

## 1. Existing ports, recomps and Gen 5 decomps

Snapshot taken 2026-10-16/17. Repository dates, star counts and licenses come from the GitHub API (`gh repo view`
and `gh api`). Progress figures come from each project's own tracker. Anything taken only from a search-engine
snippet or a secondary repost, or from a page that refused the fetch, is marked **unverified**.

### 1.1 Bottom line

- **No native PC port, static recompilation or code-level reimplementation of any Gen 5 game (Black/White or
  Black 2/White 2) exists publicly.** The search covered GitHub (repository search under many names), GitLab and
  Codeberg (API search), press coverage, PokeCommunity, Project Pokémon, GBAtemp and YouTube. The Gen 5 fan
  "remakes" are engine showcases in Unreal, Godot or Unity. None of them runs the original game logic.
- **fuddlesworth/pokebw2 is by far the most advanced Gen 5 code project.** It reports 82.89% of Black 2's code
  matched and both USA/Europe ROMs rebuilding byte for byte. The next Gen 5 efforts are a public Pokémon Black
  decomp at 0.03%, a private Black decomp at 3.23% and a two-week-old Black 2/White 2 disassembly.
- **pret has no Gen 5 repository.** Its org list on 2026-10-16 had pokediamond, pokeplatinum and pokeheartgold,
  and nothing for Gen 5.
- **DS precedents for a decomp-to-native port now exist:**
  - The Super Mario 64 DS decomp feeds a Windows PC port.
  - Two DS static recompilers have appeared: ndsrecomp, which runs Metroid Prime Hunters and Mario Kart DS, and
    ndskit.
  - Gen 4 Pokémon has a dormant SDL3 Platinum fork, a disputed "surprise" Platinum PC release, an early 3DS
    Platinum port and a HeartGold "native engine" package.
- **swan** is the Gen 5 code-injection community's header and symbol database, from ds-pokemon-hacking (formerly
  Kingdom of DS Hacking). It is GPL-3.0, has been dormant since 2025-07-03, and is the source of pokebw2's
  imported names.

### 1.2 Gen 5 decompilations and disassemblies

| Project | Game | Kind | Created / last push | License | Stars / forks | Stated progress | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| [fuddlesworth/pokebw2](https://github.com/fuddlesworth/pokebw2) | Black 2 + White 2 (USA/EUR, NDSi Enhanced) | Matching decomp (dsd + MWCC) | Git history starts 2026-09-27; GitHub repo created 2026-10-04; last push 2026-10-17 | GPL-3.0 | 30 / 14 | 82.89% code matched (b2_us) | Very active |
| [mariano54/pokebw2](https://github.com/mariano54/pokebw2) (`parallel-decomp` and topic branches) | same | Fork of the above | last push 2026-10-17 | GPL-3.0 (inherited) | 0 | `parallel-decomp` is 31 commits ahead and 213 behind upstream `main` | Active; its work is upstreamed through PRs |
| [nds197/pokeblack2](https://github.com/nds197/pokeblack2) | Black 2 + White 2 (same SHA1s as pokebw2) | pret-style disassembly (Makefile, `asm/`) | 2026-10-08 / 2026-10-16 | none | 7 / 2 | README says "WIP disassembly"; `src/` holds a single `.c` file | Active, very early |
| [squiddonaut/pokeblack](https://github.com/squiddonaut/pokeblack) | Black (and White via `make white`), USA/EUR v1.0 | pret-style decomp (ndsdisasm, metroskrew MWCC, TWL-SDK) | 2025-11-04 / 2026-10-16 | none | 60 / 23 | [decomp.dev](https://decomp.dev/squiddonaut/pokeblack): **0.03%** decompiled; CONTRIBUTING says 28,721 functions are dumped | Active, early |
| Private Black decomp, shown by [reflxay/pokeblack-progress](https://github.com/reflxay/pokeblack-progress) | Black (US) | Private matching decomp; only `data/progress.json` is public | site repo 2026-10-03 / 2026-10-16 | site: none | 0 | Snapshot of 2026-10-16T21:18Z: 117,970 of 3,657,168 code bytes matched (**3.23%**); 1,980 of 26,091 functions; ARM9 main 21.4% | Active; source not public, so unverifiable beyond the JSON |
| [FieryMewtwo/pokeblack2](https://github.com/FieryMewtwo/pokeblack2) | Black 2 | Stub; its README is pokediamond's ("This is a decompilation of Pokémon Diamond") | 2021-09-02 / 2021-09-02 | none | 0 / 6 | none | Abandoned |
| [FieryMewtwo/pokeblack](https://github.com/FieryMewtwo/pokeblack) | Black | Empty "disassembly" | 2021-06-03 / 2021-06-03 | none | 0 | none | Abandoned |
| [SakuraiTsubaki/PocketMonsters-Black2-Decompilation](https://github.com/SakuraiTsubaki/PocketMonsters-Black2-Decompilation) | "Pocket Monsters Black 2" | Research scaffold; README: "Exact build identity: Not selected" | 2026-09-14 / 2026-09-18 | none | 0 | none | Dormant scaffold |
| kleenxfeu/pokewhite2, GinjaNinja5197/PokeBlack2, Kingkording/pokeblack2forgor | – | Empty or personal repositories with no description | 2022–2026 | none | 0 | – | Not projects |

**pokebw2 details** (target of this report; local README, `docs/` and git, plus
[ByteMatch](https://bytematch.dev/fuddlesworth/pokebw2)):

- **Progress (ByteMatch, b2_us, latest commit shown 4fc235c, "Merge pull request #327", Oct 16):**
  - Code: 82.89% matched (3,205,732 of 3,867,402 bytes), 68.40% fully linked, 87.33% fuzzy.
  - Functions: 36,848 of 43,410 matched (84.9%).
  - Units: 1,336 of 1,625 complete (README shield).
  - Data: 17.86% matched, 60.91% linked.
  - By category: ARM9 main 86.89%, overlays 82.13%, game code 85.01%, libraries 72.88%, DS Protect 23.78%.
  - By library: SPL, irc and DPW 100%; NitroSDK 81.72%; TwlSDK 81.70%; NitroSystem 64.74%; nitrowifi 43.68%;
    dwcutil 25.42%.
  - The page showed no White 2 figures. The build checks both SHA1s on every change.
- **Toolchain:**
  - [dsd / ds-decomp](https://github.com/AetiasHax/ds-decomp), used through fuddlesworth's `dsi-hybrid` fork, for
    delinking.
  - MWCC `dsi/1.1p1` run through wibo, with ninja and objdiff.
  - The README requires Linux, WSL or macOS.
  - nds197/pokeblack2's INSTALL names MWCC `dsi/1.2p2` instead. pokebw2's second commit is "Decompile overlay 4
    and pin down the compiler", which settled on 1.1p1. The two projects disagree.
- **Process:**
  - 2,438 commits; the top committers are Nathan/fuddlesworth (about 1,432–1,587), CallanCampos (about 300) and
    TeenageMutantNinjaTurtle (231). The repository ships Claude Code and Codex agent configurations (`CLAUDE.md`,
    `.claude/`, `.agents/`), and "claude" and "codex" appear as GitHub contributors. **The decomp is heavily
    LLM-agent-assisted.**
  - Of 123 `[Decomp] Overlay NNN` issues, 22 are still open.
- **Port-related content:**
  - The README states "a long-term goal of a native PC port".
  - `docs/porting-notes.md` exists but so far covers only NitroWiFi's SSL, SOC and WCM pointer-width, IRQ and
    lifetime questions.
  - The upstream has no port branch; `main` is its only branch.
- **Relationship to the other Gen 5 projects:** pokebw2 is independent of nds197/pokeblack2 (pret-style asm,
    different toolchain) and of both Black decomps. They share no code. pokebw2 credits only swan for names, and
    pret's pokeplatinum and pokeheartgold as references (`CONTRIBUTING.md`: write the C from the asm, never copy).
- **mariano54's fork:** It works on overlays 011 (GameSpy), 036 (field) and 182 (DWC utilities) and merges back
  upstream. Merged PRs are #323, #326, #327, #332, #334, #337 and #339; #331 and #342 were open on 2026-10-16/17.

### 1.3 Ports, recompilations and remakes of Gen 5

- **Native PC port or static recomp: none found.** GitHub searches for "pokemon black 2 port", "bw2 port",
  "pokemon black 2 pc", "nds recomp" and similar turned up no Gen 5 target. Web, press and YouTube searches found
  none either. Searches of Codeberg and GitLab by API found nothing relevant.
- **Fan remakes (engine showcases, not ports):**
  - **ArcSin Jesse, Unreal Engine 5, original Black/White.** A Windows demo of N's Castle with a battle against N
    and a Zekrom catch ([GGRecon, 2023-12-18](https://ggrecon.com/articles/you-can-play-the-pokemon-black-and-white-remake-now-in-unreal-engine)).
    A second UE5 demo by Shemne was reportedly not planned for release (search snippet; **unverified**).
  - **[one-om-jha/PokemonBlack](https://github.com/one-om-jha/PokemonBlack).** A small Godot demo of a possible
    Black/White remake (2024-01-08 to 2024-02-03, 9 stars, no license). Abandoned.
  - **[advent259out/pokeblack2](https://github.com/advent259out/pokeblack2).** A Unity (C#) project, created and
    last pushed 2026-04-19, with no README content. Inactive.
  - **A Black 2/White 2 3D remake shown as Reddit screenshots.** Reported by
    [GamePro.de](https://www.gamepro.de/artikel/pokemon-schwarz-weiss-2-remake-community-projekt,3434301.html).
    The fetch returned 403, so the engine, author and status are **unverified**.
  - **A Gumroad "Castelia City" Black/White fan game** ([cuate2](https://cuate2.gumroad.com/l/PokemonBlackWhiteCasteliaCity)).
    It is a single-area reimagining with no engine named.
  - **Ax461/pokeblack** is a creepypasta recreation: a name collision, not Gen 5.
- **None of these remakes reuses or reimplements the original game code.** They are irrelevant as a port base.

### 1.4 DS static recompilers and decomp-based native ports (general precedent)

| Project | What it is | Created / last push | License | Status |
| --- | --- | --- | --- | --- |
| [RetroPortingToolKit/ndsrecomp](https://github.com/RetroPortingToolKit/ndsrecomp) (originally `mstan/ndsrecomp`, which redirects here) | Ahead-of-time ARM-to-C static recompiler for both DS CPUs, with a runtime that provides the memory bus, two-CPU scheduler, hardware models and SDL frontend; melonDS serves as accuracy oracle | 2026-07-15 / 2026-10-07 | Original code under PolyForm Noncommercial 1.0.0 (commits from before the change stay MIT); runner and oracle parts are MIT; melonDS-derived parts are GPL-3.0-or-later, so distributed `nds_runner` binaries are GPL-3.0 | "Very early pre-alpha (v0.6.10)". Demonstrated: DS firmware menu, Super Mario 64 DS, Mario Kart DS (Wiimmfi online, local wireless) and Metroid Prime Hunters (`mstan/MetroidPrimeHuntersRecomp`, public alpha). The README mentions no Pokémon or DSi-mode titles. 58 stars |
| [vs-sr-dev/ndskit](https://github.com/vs-sr-dev/ndskit) | Game-agnostic DS reverse-engineering and porting kit: ARM/Thumb discovery, static recompiler to C++ (programs and overlays), and a runtime replacing both CPUs, the 2D engines (in software), the 3D engine (OpenGL at any scale) and sound | 2026-10-09 / 2026-10-10 | none | One consumer port, [pc-moon](https://github.com/vs-sr-dev/pc-moon) (Moon, 2009). 1 star; brand new |
| [tangosdev/sm64ds-decomp](https://github.com/tangosdev/sm64ds-decomp) | Matching decomp of Super Mario 64 DS that feeds a **native Windows PC port** (download at tangos.dev) | 2026-06-14 / 2026-10-17 | none | README: 100% matched (11,389 functions), 96.2% "linked" into the PC port (matched units replace the port's stand-ins), 39.8% "converted" to readable C++. Adds higher resolution, AA and texture filtering. 202 stars. **The closest structural precedent for pokebw2's port goal** |
| [ImLunaHey/ds-recomp](https://github.com/ImLunaHey/ds-recomp) | Misnamed: a TypeScript in-browser DS **emulator**, not a recompiler | 2026-06-09 / 2026-06-11 | none | Pokémon Platinum, HeartGold, Diamond and Pearl stall early. DSi-Enhanced titles are not supported |
| [Kuldeep-bhadouriya/NDS-recomp](https://github.com/Kuldeep-bhadouriya/NDS-recomp) | C++ repository with no README | 2026-10-16 / 2026-10-17 | none | Too new to assess |

Other DS decomps exist (zeldaret/ph and st, NSMB-Decomp/nsmb, rhgold, ooe, fe11-us; listed on the
[Recompendium](https://nio03.github.io/unricopie/en/)). That index has no Gen 4 or Gen 5 Pokémon port entry.

### 1.5 Gen 4 (and other Pokémon) native ports, for context

- **[arcanite24/pokeplatinum-portable](https://github.com/arcanite24/pokeplatinum-portable).** An SDL3 fork of
  pret/pokeplatinum.
  - Dates and size: created 2025-11-14, **last push 2025-12-09** (dormant about 10 months), 13 stars, no license
    file.
  - Approach: "stub-first", with every DS function preserved as an SDL3 stub behind a platform abstraction layer.
    It drops the DS build.
  - Status: phases 1–3 (build, display, input) are complete; graphics loading is "in progress"; audio and the file
    system are "planned". Only macOS has been tested.
- **"Pokémon Platinum PC port gets a surprise release."** GBAtemp thread
  [682001](https://gbatemp.net/threads/pokemon-platinum-pc-port-gets-a-surprise-release.682001/).
  - **Unverified:** GBAtemp returned 403, so everything here comes from search snippets and the
    [pocg.net repost](https://forum.pocg.net/t/pokemon-platinum-pc-port-surfaces-with-source-code-and-no-rom-required/619)
    (dated 2026-06-30; the snippets give the thread's start as 2026-05-26).
  - Features: a Windows build, Linux in one report, with source code. One source tree builds the NDS ROM and the
    PC port. 3D internal resolution defaults to 10x, a 60 fps patch is on, LAN and online multiplayer work without
    voice chat, and saves are not emulator-compatible.
  - Source and legality: the author is anonymous and builds were hosted on Archive.org. Commenters allege that it
    derives from the 2020 Nintendo leak rather than the pret decomp. That claim is unconfirmed.
- **[GabeSud0/pokeplatinum-3ds](https://github.com/GabeSud0/pokeplatinum-3ds).** A native 3DS port built from the
  pret decomp (2026-10-09 to 2026-10-17). Its README says "Phase 1… Nothing playable yet". The plan is a platform
  layer separating the game from DS hardware, then a 3DS backend, with SoulSilver later.
- **[CVGisBored/PokeHeartGold_PC](https://github.com/CVGisBored/PokeHeartGold_PC).**
  - What it is: "Pokémon HeartGold Native Port v0.49.3 — source-only". A native C/C++ engine with NARC, NSBMD,
    Nitro2D and SDAT loaders that extracts assets from the user's ROM.
  - Dates and size: 2026-08-29 to 2026-09-06, 1 star, no license.
  - Its description says it was built "with the help of AI". How much of it derives from the decomp is
    **unverified**.
- **[ctrl-cheeb-del/pokeheartgold](https://github.com/ctrl-cheeb-del/pokeheartgold).** An LLM-agent fork of
  pret/pokeheartgold.
  - Claim: "Fully decompiled: everything's C/C++ and both ROMs build byte-for-byte."
  - Dates and size: created 2026-09-17, 722 stars.
  - Press and X posts report about one month of work and about US$56k in API spend. That figure is **unverified**:
    it comes from secondary coverage such as
    [ixbt.games, 2026-10-16](https://ixbt.games/en/news/2026/10/16/proshhaite-ekskliuzivy-dlia-konsolei-ii-perevernul-mir-igr-s-nog-na-golovu.html).
  - It has no PC port. [antonsynd/pokeheartgold-slop](https://github.com/antonsynd/pokeheartgold-slop) is a similar
    LLM fork at about 95.7%.
  - pret refuses AI-generated contributions, so these stay separate forks.
- **Other Pokémon "recomps" are not decomp-based:**
  - [bryanthaboi/gen1recomp](https://github.com/bryanthaboi/gen1recomp) (3,946 stars) is a hand-written Lua/LÖVE
    recreation of Gen 1–3 games that reads data from the user's ROM. Gen2Recomped is forked from it.
  - Team Luminescent's BDSP work targets the Switch remakes
    ([DSOGaming, 2024-07-04](https://dsogaming.com/?p=181510)).
- **So for Gen 4:** no decomp-derived native port is finished and verifiably clean. The only one billed as
  playable, the Platinum surprise release, has disputed provenance.

### 1.6 swan: what it is and how pokebw2 uses it

- **Repository:** [ds-pokemon-hacking/swan](https://github.com/ds-pokemon-hacking/swan). The old
  `kingdom-of-ds-hacking/swan` URL redirects to it.
  - Description: "C/C++ include headers for Pokémon White 2."
  - License: **GPL-3.0**.
  - Created 2022-09-28. The last commit is `4324f73` on **2025-07-03** by HelloOO7, the exact revision pokebw2
    imports.
  - 15 commits, all by HelloOO7 (11) and PlatinumMaster (4). There has been one PR ever (#1, ESDBs, 2023). The four
    forks (mariano54, MCMi460, Tdavide04, totallyanon) are not ahead.
  - **Dormant for about 15 months.**
- **Contents:**
  - **130 generated headers** in `battle/` (1), `data/` (2), `field/` (58), `gfl/` (24), `math/` (5), `nds/` (12),
    `ns/` (11), `pml/` (4), `save/` (6) and `system/` (5), plus `swantypes.h` and `swan_cpp_enum_ops.h`.
  - The headers declare structs, enums, typedefs, globals, macros and function prototypes, with C++ namespaces and
    operators behind `SWAN_CPPONLY`.
  - They are generated from `root.swandb`, a 244 KB YAML database. It is edited with the **SwanComposer** GUI in
    CTRMap-CE plus the CTRMapV plugin (Extras tab; Ctrl+S saves, Ctrl+G regenerates).
- **External symbol databases (ESDBs), per game:**
  - **`IRDO.yml`, White 2 USA:** 3.37 MB, 57,952 symbols. About 10,100 are real names; the rest are IDA
    auto-labels (`loc_`, `sub_`, `off_`, `dword_`…).
  - **`IREO.yml`, Black 2 USA:** 62 KB, 984 symbols, 837 of them real names.
  - Both segment tables list ARM9, static RAM, ITCM, shared work and only **21 overlays plus 2 BSS segments**
    (OVL_10, 12, 27, 33, 35, 36, 66, 76, 104, 146, 147, 165, 167–170, 207, 255, 279, 299, 302). The game has
    hundreds of overlays.
  - The README warns the two ESDBs "are not cross compatible" because the memory layouts differ.
  - Updates go through `ESDBMerger.py` from
    [ESDB-Tools](https://github.com/ds-pokemon-hacking/ESDB-Tools), which also ships Ghidra `ESDBExport.py` and
    `ESDBImport.py`.
- **How the code-injection toolchain uses it**
  ([ds-pokemon-hacking guide](https://ds-pokemon-hacking.github.io/docs/generation-v/guides/bw_b2w2-code_injection/)):
  1. Patches are written in C, C++ or ARM asm against swan's headers and compiled with `arm-none-eabi-gcc` (`-r`,
     `-march=armv5t`, optionally `-mthumb`, `-Os`).
  2. CTRMap's "Convert ELF to DLL" links the ELF against the ESDB into an RPM module. Hooks are encoded in symbol
     names (`THUMB_BRANCH_LINK_FunctionName` and similar).
  3. **PMC** hooks the game's overlay loader from code placed in the secure area and swaps RPM modules in and out at
     runtime. **NitroKernel** loads shared libraries from `/lib`.
  4. A failed ESDB link produces **no error**, so the guide recommends checking with RPMDump.
- **How pokebw2 uses it** (local `docs/code-organization.md` §Names, `tools/decomp/import_swan.py`):
  - `import_swan.py` reads `IREO.yml` (Black 2) as primary and `IRDO.yml` (White 2) as the other version. Names
    carry across versions through the version map.
  - IDA-generated names are skipped. A name is applied only where a symbol starts exactly at its address, and only
    default `func_`/`data_` names are replaced.
  - The headers are a reference that is never copied verbatim. swan-derived names and layouts are marked as swan's
    in each file's header.
  - In `lib/`, the SDK's own names override swan's; the header notes swan's name where they differ.
  - Names swan lacks go in `config/names.txt`, which now has **25,418** entries. Most of the decomp's naming is
    therefore its own, beyond swan's roughly 10k real names.
  - The README states the GPL-3.0 license covers the imported names. This is consistent with swan's license.
- **Port implication (inference, not a stated fact):** swan's ESDB binds to absolute ARM9 and overlay addresses.
  PMC, RPM and swan mods therefore cannot run on a native build as they are. They would need recompiling against
  pokebw2's headers or a compatibility layer. swan's names, which pokebw2 shares, make a source-level migration
  path plausible.

### 1.7 ds-pokemon-hacking and related repos relevant to code-patch mods

| Repo | One line | License | Last push | Stars |
| --- | --- | --- | --- | --- |
| [CTRMap-CE](https://github.com/ds-pokemon-hacking/CTRMap-CE) | Extensible Java level-editor framework for Gen 5 (B/W/B2/W2); plugin host (latest release `2026/04/28/1`) | GPL-3.0, plus a non-binding "CFRU" no-profit request in the README | 2026-04-28 | 30 |
| [CTRMapV](https://github.com/ds-pokemon-hacking/CTRMapV) | Gen V plugin for CTRMap: game adapters, PMC install, ELF-to-DLL conversion, SwanComposer | none detected | 2026-04-28 | 12 |
| [PMC](https://github.com/ds-pokemon-hacking/PMC) | "Pokémon Black/White 2 code injection system": hooks overlay loading, loads RPM patch modules (C++, CMake; release 13.2.4, 2026-03-01) | GPL-3.0 | 2026-03-01 | 6 |
| [swan](https://github.com/ds-pokemon-hacking/swan) | Gen 5 headers and ESDBs (§1.6) | GPL-3.0 | 2025-07-03 | 1 |
| [ESDB-Tools](https://github.com/ds-pokemon-hacking/ESDB-Tools) | ESDB merger and Ghidra ESDB import/export scripts | none | 2025-09-29 | 1 |
| [White2Upgrade](https://github.com/ds-pokemon-hacking/White2Upgrade) | Code-injection-heavy B2W2 hack and template: Dex to 721, Fairy type; team PlatinumMaster, Dararo/Paideieitor, BluRose and others | none | 2026-10-04 | 15 |
| [PokeScript](https://github.com/ds-pokemon-hacking/PokeScript) / [PokeScriptSDK5](https://github.com/ds-pokemon-hacking/PokeScriptSDK5) | High-level scripting language and IDE, with its Gen 5 SDK | GPL-3.0 / none | 2026-04-16 | 2 / 2 |
| [Sparkplug](https://github.com/ds-pokemon-hacking/Sparkplug) | Minimal CTRMap plugin example (RTLDR) | none | 2023-07-01 | 0 |
| [Nightfire](https://github.com/ds-pokemon-hacking/Nightfire) | Java NetBeans project whose README is Sparkplug's copy; purpose **unverified** | none | 2024-08-17 | 1 |
| [ds-pokemon-hacking.github.io](https://github.com/ds-pokemon-hacking/ds-pokemon-hacking.github.io) | Community guides site (Gen 4/5 hacking, code injection) | none | 2026-09-13 | 23 |
| [HelloOO7/libRPM](https://github.com/HelloOO7/libRPM) | Relocatable Program Module runtime (<10 KB, ARMv5T and Win32 targets) that PMC is built on | none detected | 2026-03-01 | 5 |
| [HelloOO7/RPMAuthoringTools](https://github.com/HelloOO7/RPMAuthoringTools) | CLI tools for the RPM format (RPMDump and others) | none | 2026-04-09 | 1 |
| [HelloOO7/NitroKernel](https://github.com/HelloOO7/NitroKernel) | "Small kernel DLL for the Nintendo DS" that loads libraries from `/lib` (release 1.2.4, 2025-06-03) | none detected | 2025-06-03 | 4 |
| [HelloOO7/ExtLib](https://github.com/HelloOO7/ExtLib) | Pure C++ embedded library (new and delete over the GFL heap, and so on) that PMC needs | GPL-3.0 | 2025-07-03 | 4 |
| [PlatinumMaster/BeaterScript](https://github.com/PlatinumMaster/BeaterScript) | Script editor and decompiler for the DS generations (B2W2 scripts are NARC a/0/5/6) | none | 2021-02-02 | 7 |

Maintainers across these are **HelloOO7** (CTRMap, PMC, libRPM, NitroKernel, swan) and **PlatinumMaster** (PMC,
ESDBs, White2Upgrade). Paideieitor (Dararo), from White2Upgrade's core team, also commits to squiddonaut/pokeblack.
Community Discords are linked from White2Upgrade's README ("DS Modding Community") and from squiddonaut/pokeblack's
README.

### Sources

- https://github.com/fuddlesworth/pokebw2 ; https://bytematch.dev/fuddlesworth/pokebw2 ; local checkout: `README.md`, `docs/code-organization.md`, `docs/building.md`, `docs/porting-notes.md`, `tools/decomp/import_swan.py`, `config/names.txt`, git log
- https://github.com/mariano54/pokebw2 (branches; compare `main...mariano54:parallel-decomp`)
- https://github.com/AetiasHax/ds-decomp ; https://github.com/fuddlesworth/ds-decomp
- https://github.com/nds197/pokeblack2
- https://github.com/squiddonaut/pokeblack ; https://decomp.dev/squiddonaut/pokeblack
- https://github.com/reflxay/pokeblack-progress ; https://reflxay.github.io/pokeblack-progress/ (`data/progress.json`)
- https://github.com/FieryMewtwo/pokeblack2 ; https://github.com/FieryMewtwo/pokeblack ; https://github.com/SakuraiTsubaki/PocketMonsters-Black2-Decompilation
- https://github.com/pret (org repository list)
- https://github.com/one-om-jha/PokemonBlack ; https://github.com/advent259out/pokeblack2
- https://ggrecon.com/articles/you-can-play-the-pokemon-black-and-white-remake-now-in-unreal-engine
- https://www.gamepro.de/artikel/pokemon-schwarz-weiss-2-remake-community-projekt,3434301.html (403, not read)
- https://cuate2.gumroad.com/l/PokemonBlackWhiteCasteliaCity
- https://github.com/RetroPortingToolKit/ndsrecomp ; https://1379.tech/
- https://github.com/vs-sr-dev/ndskit ; https://github.com/vs-sr-dev/pc-moon
- https://github.com/tangosdev/sm64ds-decomp ; https://capitalandcompute.net/blog/which-games-has-ai-decompiled/
- https://github.com/ImLunaHey/ds-recomp ; https://github.com/Kuldeep-bhadouriya/NDS-recomp
- https://nio03.github.io/unricopie/en/ (Recompendium)
- https://github.com/arcanite24/pokeplatinum-portable
- https://gbatemp.net/threads/pokemon-platinum-pc-port-gets-a-surprise-release.682001/ (403, via search snippets) ; https://forum.pocg.net/t/pokemon-platinum-pc-port-surfaces-with-source-code-and-no-rom-required/619 ; https://www.antonretro.com/news/pokemon-platinum-gets-unofficial-native-pc-port (timed out)
- https://github.com/GabeSud0/pokeplatinum-3ds ; https://github.com/CVGisBored/PokeHeartGold_PC
- https://github.com/ctrl-cheeb-del/pokeheartgold ; https://github.com/antonsynd/pokeheartgold-slop ; https://ixbt.games/en/news/2026/10/16/proshhaite-ekskliuzivy-dlia-konsolei-ii-perevernul-mir-igr-s-nog-na-golovu.html ; https://x.com/Pirat_Nation/status/2110989368454189268
- https://github.com/bryanthaboi/gen1recomp ; https://github.com/UNDERdecoded/Gen2Recomped ; https://dsogaming.com/?p=181510
- https://github.com/ds-pokemon-hacking/swan (README, `IRDO.yml`, `IREO.yml`, `root.swandb`, commits, forks, PRs)
- https://ds-pokemon-hacking.github.io/docs/generation-v/guides/bw_b2w2-code_injection/
- https://github.com/ds-pokemon-hacking (CTRMap-CE, CTRMapV, PMC, ESDB-Tools, White2Upgrade, PokeScript, PokeScriptSDK5, Sparkplug, Nightfire, ds-pokemon-hacking.github.io)
- https://github.com/HelloOO7 (libRPM, RPMAuthoringTools, NitroKernel, ExtLib) ; https://github.com/PlatinumMaster/BeaterScript

## 2. Randomizer: UPR ZX Gen 5 handler

**What was examined.** Universal Pokemon Randomizer ZX at `master` 7f00eb8 (2024-11-23, the commit of its last
release v4.6.1; the repo has had no push since) and its active successor, Universal Pokemon Randomizer FVX, at
`master` 300ad60 (2026-10-11, latest release vFVX1.6.2 on 2026-10-03). FVX reorganised the code into
`romio/` (ROM I/O) and `random/` (logic), but its Gen 5 handler is a direct descendant of ZX's: nearly every BW2
read and write below is the same in both. Section 2.8 lists what FVX adds.

**Method.** I read `Gen5RomHandler.java`, `Gen5Constants.java`, `gen5_offsets.ini`, `AbstractDSRomHandler.java`,
`NDSRom`/`NDSY9Entry`/`NARCArchive`, `PPTxtHandler` and every BW2 `.ips` patch, decoding each patch record by record.
Many of UPR's code and table edits are found by searching for a byte pattern, not by a fixed offset. I resolved
every such pattern against the pokebw2 checkout's extracted binaries (`extract/b2_us`, `extract/w2_us`; arm9 file
offset + 0x02004000, overlay file offset + the load address in `config/*/arm9/overlays/ovNNN/delinks.txt`).
**Every pattern matched exactly once in both US ROMs.** The RAM addresses below come from that work. I did not
cross-check against the decomp's `data/` directory.

Classification legend, from the point of view of a native port built from the decomp's C that reads its assets
from a user-supplied (possibly randomized) ROM:

| Class | Meaning for the port |
| --- | --- |
| **DATA** | Lives in a NARC in the ROM filesystem. Works automatically if the port loads that NARC from the user's ROM. |
| **SCRIPT** | A byte edit inside the field-script NARC `a/0/5/6`. Data-only as long as the port runs the ROM's scripts through the decompiled script VM (the `scrcmd_*` files are in `src/ov012/` in this repo). |
| **TEXT** | A message bank in `a/0/0/2` or `a/0/0/3`. Data-only. UPR re-encodes the whole bank in the game's own Gen 5 format through `PPTxtHandler`. |
| **ARM9-DATA / OVL-DATA** | A table in arm9 or overlay `.rodata`/`.data`, or a literal-pool constant. A port compiled from C has its own copy and **ignores the ROM's**. It needs an extractor that reads the table from the ROM binary, or an external data file. |
| **CODE** | An instruction-level patch, or new code injected into arm9/ITCM. It cannot carry over. The port needs an equivalent option or hook, and can detect the patch by comparing the bytes at the patched site. |

---

### 2.1 How the handler works (BW2 specifics that matter for a port)

- **ROM entries.** `gen5_offsets.ini` has one section per ROM: game code, CRC32s, `File<Key>=<path, crc>`, overlay
  numbers, offsets and tweak IPS names. Black 2 (U) `IREO` does `CopyFrom=IRBO` (Black 1 US) and **inherits keys it
  does not override**: `File<MoveData>=a/0/2/1`, `ShopItemOvlNumber=21`, `RoamerOvlNumber=10`, `ShopItemOffsets`,
  `DoublesTrainerClasses`. Most of those are unused for BW2; MoveData is used.
- **ZX does not copy `*Tweak` keys through `CopyFrom`.** It copies arrays, numbers, strings, offset arrays and
  files, but not `tweakFiles`. So **only Black 2/White 2 US** get Fastest Text, the static-music fix and the
  Shedinja fix, and only they get the ARM9 extension. F/G/I/S/K get only `NationalDexAtStartTweak`, because they
  list it themselves. Japanese (`IREJ`/`IRDJ`, Version=1) gets **no** IPS tweak at all. FVX does copy tweak files
  (`RomEntry.java` L79), so in FVX non-US entries inherit the US IPS names.
- **ARM9 is always extended and always rewritten (US B2/W2).** `loadedROM` calls
  `extendARM9(arm9, 496, TCMCopyingPrefix, 0x02004000)` whenever the entry has `ShedinjaEvolutionTweak` or
  `NewIndexToMusicTweak`, which every US BW2 entry has. The function:
  - grows the **ITCM autoload** from 0x13A0 to 0x1590 bytes (new ITCM space 0x01FF93A0–0x01FF958F, 448 B for
    music plus 48 B for Shedinja);
  - shifts everything after the ITCM image (DTCM and the other autoloads) by 496 bytes;
  - rewrites the crt0 module-params autoload-list pointer and the ARM9 static-end pointer at **0x02004FB0**
    (arm9.yaml `build_info` = 0xFB0), plus the ITCM size in the autoload list.

  `savingROM` always writes arm9. Main-module offsets below 0x99740 (B2) / 0x99780 (W2) do not move.
- **Rewritten on every save, changed or not:** `a/0/0/2`, `a/0/0/3`, `a/0/5/6`, arm9, `a/0/1/6` (personal),
  `a/0/2/1` (moves) and `a/0/1/9` (evolutions, through `savePokemonStats` → `writeEvolutions`).
- **Applied on every US BW2 save, whatever the settings:**
  - The **Shedinja hook.** `writeEvolutions` calls `writeShedinjaEvolution` for Nincada whenever the tweak key
    exists and Nincada still has two or more evolutions, which is the vanilla case. That patches ov284 and the
    ITCM extension.
  - The **intro Pokémon.** `Randomizer.java` L671 calls `randomizeIntroPokemon()` unconditionally, which writes
    ov294 and ov162.
- **Overlays are never resized.** `writeOverlay` replaces the bytes in place and re-BLZ-compresses the overlay if
  it was compressed. Fixed overlay offsets therefore stay valid in randomized ROMs. ARM9 is re-BLZ-compressed too.
- **NARCs** are rebuilt by `NARCArchive.getBytes()`, which keeps file names when they are present. Files can change
  size: learnsets, trainer Pokémon, script 854 and, in FVX, shops.
- **Validation.** `isRomValid()` compares CRC32s of arm9, overlays 36/162/166/167/168/284/294/316 and every
  `File<>` against the ini, and warns on a non-vanilla input.
- **There is no "check value" for Gen 5.** `writeCheckValueToROM` is a no-op except in Gen 2.

### 2.2 Filesystem files read or written (Black 2 US; White 2 US uses the same paths)

W2 US overrides only the CRCs of `a/1/2/7`, `a/1/7/6` and `a/2/9/6`; the contents differ but the paths are the
same. Other regions keep the same paths with their own CRCs.

| ini key | ROM path | Contents (as UPR uses it) | R/W | UPR features | Class |
| --- | --- | --- | --- | --- | --- |
| `TextStrings` | `a/0/0/2` | System message banks (indices in 2.3) | R + W every save | names, descriptions, trades, trainer names | TEXT |
| `TextStory` | `a/0/0/3` | Story/event message banks | R + W every save | starter text, trade NPC dialogue | TEXT |
| `PokemonGraphics` | `a/0/0/4` | Battle sprites: 20 files per species; file N·20 = LZ front sprite, N·20+18/19 = normal/shiny palette | ZX: R only. FVX: **W** palettes | starter sprite source, mascot; FVX palette randomizer | DATA |
| `MapTableFile` | `a/0/1/2` | Zone header table: 48-byte entries, byte 20 = wild set (u8 in BW2), byte 26 = map-name index | R | wild-area names in the log | DATA |
| `PokemonStats` | `a/0/1/6` | personal: bytes 0–5 stats, 6–7 types, 8 catch rate, 12/14/16 held items (common/rare/dark grass), 21 growth, 24–26 abilities (incl. hidden), 28/30/32 forme data, 40–52 TM/HM bits, 60–75 BW2 tutor bits (4×u32) | R + W every save | base stats, types, abilities, wild items, catch rate, EXP curve, TM/tutor compat; FVX also EV yield, egg cycles | DATA |
| `PokemonMovesets` | `a/0/1/8` | Level-up learnsets: (move u16, level u16) pairs ending FFFF FFFF; files resized | R/W | movesets | DATA |
| `PokemonEvolutions` | `a/0/1/9` | 7 × (method u16, param u16, target u16) | R + W every save | random/impossible/easier/time-based evolutions | DATA |
| `BabyPokemon` | `a/0/2/0` | Per-species base (baby) species u16 | W | "limit Pokémon" pool | DATA |
| `MoveData` (inherited from Black 1 entry) | `a/0/2/1` | Move data; UPR writes byte 0 type, 2 category, 3 power, 4 accuracy, 5 PP (effects never) | R + W every save | move updates/randomization | DATA |
| `ItemData` | `a/0/2/4` | Item data; u16 at 0 = price/10 | W | balanced shop prices | DATA |
| `Scripts` | `a/0/5/6` | Field scripts (files in 2.3) | R + W every save | statics, starters, field/hidden items, gifts, national dex | SCRIPT |
| `TrainerTextBoxes` | `a/0/8/9` | File 0: (trainer id u16, text-box type u16) pairs | W | double-battle mode | DATA |
| `TrainerData` | `a/0/9/1` | TRData, 20 B: flags, class, battle type, count, 4 items, AI flags, healer, money, prize | R/W | trainers; doubles sets byte 2 = 1 and byte 12 \|= 0x80 | DATA |
| `TrainerPokemon` | `a/0/9/2` | TRPoke: IV byte, ability/gender, level, species, form (+item)(+4 moves) | R/W (rebuilt) | trainer Pokémon | DATA |
| `EggMoves` | `a/1/2/4` | count u16 + moves (rewritten in place, same count) | R/W | egg moves | DATA |
| `MapFiles` | `a/1/2/6` | Per-zone overworld/entity data ("Overworlds") | R/W | **level** bytes of the Foongus/Amoonguss fake item balls | DATA |
| `WildPokemon` | `a/1/2/7` | Encounter tables: 232 B per season (×4 if seasonal); 7 rates then slots 12/12/12/5/5/5/5 × (species\|forme<<11 u16, min, max) | R/W | wild Pokémon | DATA |
| `InGameTrades` | `a/1/6/3` | Trades: given species @4, forme @8, IVs @0x10, nature @0x2C (set 0xFF), OT ID @0x34, item @0x4C, requested @0x5C | R/W | trades | DATA |
| `PokedexAreaData` | `a/1/7/6` | Pokédex habitat map: per species 345 B (85 areas × 4 seasons) | **regenerated** on every wild write | wild Pokémon | DATA |
| `StarterGraphics` | `a/2/0/2` | Starter-selection art: files 0/2/4 = palettes, 12/13/14 = uncompressed sprites (copied from `a/0/0/4`) | W | starters | DATA |
| `DriftveilPokemon` | `a/2/5/0` | PWT Driftveil tournament teams, 17 trainers (2 × 3 + 15 × 6 mons): species @0, 4 moves @2, item @12 | R/W | trainers (PWT) | DATA |
| `HiddenHollows` | `a/2/7/3` | Hidden Grottoes: per file 2 versions × 78 B = 3 rarity × 26 B (4 species u16, max lvl ×4 @8, min lvl ×4 @12, gender ×4 @16, forme ×4 @20). UPR edits only its version (`HiddenHollowIndex` B2=1, W2=0) and randomizes gender | R/W | statics (grottoes) | DATA |
| `ShopItems` | `a/2/8/2` | One file per shop (32): u16 item list. ZX keeps sizes | R/W | shops | DATA |
| `ShopSizes` (FVX only) | `a/2/8/3` | Shop size table at file offset 0x3C | W (FVX) | shop size changes | DATA |
| `HabitatList` | `a/2/9/6` | Habitat list | CRC only; **write disabled** (crashed) | none | n/a |

### 2.3 Message-bank and script-file indices (Black 2 US; W2 identical unless noted)

| Bank | Index | What | UPR use |
| --- | --- | --- | --- |
| `a/0/0/2` | 37 | In-game trade nicknames/OTs (2 per trade) | W (trades) |
| `a/0/0/2` | 63 / 64 | Item descriptions / item names | 63 W (TM descriptions ← move descriptions; TM01–92 = items 328–419, TM93–95 = 618–620) / 64 R |
| `a/0/0/2` | 90 | Pokémon names | R + W every save (unchanged content) |
| `a/0/0/2` | 109 | Map names | R |
| `a/0/0/2` | 368 | Trainer mugshot names | W (trainer names) |
| `a/0/0/2` | 374 | Ability names | R |
| `a/0/0/2` | 382 / 383 | Trainer names / classes | W |
| `a/0/0/2` | 402 / 403 | Move descriptions / move names | R; FVX W 403 ("random move names") |
| `a/0/0/2` | 409 / 410 | PWT trainer names / classes | W |
| `a/0/0/2` | 16 (FVX `BattleMoveNamesTextOffset`) | Pre-composed battle move messages (3 per move) | FVX W |
| `a/0/0/3` | 169 | Aspertia starter scene: entries 37/36/35 (starter descriptions), 60 (rival line) | W (starters) |
| `a/0/0/3` | 529 (W2: 537), 555, 193, 594, 628 | Trade NPC dialogue; old species names replaced with new | W (trades) |

| `a/0/5/6` file | What UPR edits | Class |
| --- | --- | --- |
| 854 | Starter species (`StarterOffsets1–3` = 0x58B/590/595, 0x5C0/5C5/5CA, 0x5E2/5E7/5EC); starter-flow splice (2.4); National Dex IPS (2.6) | SCRIPT |
| 676 | "No free Lucky Egg": the two Lucky Egg gifts (cmd 0x2A, var 0x8000) become a random Mulch (FVX: Gooey Mulch) | SCRIPT |
| 1240 / 1241 | Item balls (setvar 0x28, var 0x800C) / hidden items (0x2A, 0x8000): item u16 | SCRIPT |
| 32, 54, 208, 304, 364, 426, 480, 526, 556, 652, 662, 664, 684, 730, 740, 880, 948, 950, 988, 1018, 1030, 1034, 1068, 1080, 1102, 1112, 1122, 1128, 1168, 1208, 1222, 1224, 1226, 1253 | Static encounter species/level/forme words (`StaticPokemon{}`: Cobalion, Virizion, Terrakion, Reshiram, Zekrom, Kyurem and its B/W formes, Latias, Latios, lake trio, Regis, Regigigas, Cresselia, Heatran, Mandibuzz, Braviary, Volcarona, Crustle, Jellicent, shiny Haxorus, Eevee, Deerling (forme), shiny Gible/Dratini, Happiny egg (54), Magikarp, 9 fossils (1253, shared level 0x3D6), Zorua) | SCRIPT |
| 1273 | Foongus (0x45) / Amoonguss (0xC7) fake-ball species; their levels are in `a/1/2/6` files 500/506/534/562/563 | SCRIPT + DATA |

BW2 has **no roamers**: `RoamingPokemon{}` is Black/White 1 only, and `CopyRoamingPokemon` is unset. BW2 trades
also have **no trade-script edits**, because `TradeScript[]` is BW1 only.

### 2.4 Per-feature breakdown (BW2)

| Feature | Where | How | Class |
| --- | --- | --- | --- |
| Base stats, types, abilities, wild held items, catch rate, EXP curve, TM/HM and tutor compatibility | `a/0/1/6` | field writes | DATA |
| Level-up moves / egg moves / evolutions | `a/0/1/8` / `a/1/2/4` / `a/0/1/9` | | DATA |
| "Change impossible evolutions" (trade → level 37, trade+item → held item day/night, Karrablast/Shelmet → level with other species) and "remove time-based evolutions" | `a/0/1/9` | uses existing Gen 5 methods; **no** arm9 change | DATA |
| "Make evolutions easier" | `a/0/1/9` (level-with-other → level 35) **and arm9 code** | friendship threshold `cmp r0,#220` → 160 at 3 sites | DATA + **CODE** |
| Move data updates / randomization | `a/0/2/1` (type, category, power, accuracy, PP only) | | DATA |
| Trainers (incl. Challenge Mode teams, e.g. E4 772–776, and rematches) | `a/0/9/1`, `a/0/9/2`, `a/2/5/0` | | DATA |
| Trainer names / classes | `a/0/0/2` 368/382/383/409/410 | | TEXT |
| Double-battle mode | `a/0/9/1` + `a/0/8/9` **plus 3 ov036 code edits and 1 arm9 code edit** (2.5) | | DATA + **CODE** |
| Wild Pokémon | `a/1/2/7`; `a/1/7/6` regenerated | | DATA |
| Hidden Grottoes | `a/2/7/3`; pool restricted to Gen 1–4 plus a list of Unova species | treated as statics | DATA |
| Static encounters | script words in `a/0/5/6`; fake-ball levels in `a/1/2/6` | species, level and forme bytes | SCRIPT/DATA |
| "Correct static music" | arm9 ITCM extension (new code plus species→music literal pool) + ov036 hook | `NewIndexToMusic` IPS, then the pool words after `NewIndexToMusicPrefix` are rewritten to the new statics; for Kyurem-B/W, 4 halfwords of the patch code are zeroed | **CODE** (the mapping is data embedded in injected code) |
| Starters | `a/0/5/6` 854 (3 offsets each); `a/2/0/2`; **ov316 starter cry table**; `a/0/0/3` 169 | see the splice note below | SCRIPT + DATA + **OVL-DATA** |
| Starter-flow splice ("GIVE ME BACK MY PURRLOIN") | `a/0/5/6` 854 | replaces the first `28 00 A1 40 04 00` (cmd 0x28, var 0x40A1, value 4) with `1E 00 <rel32>` (jump) to 14 appended bytes `28 00 A1 40 04 00 DE 00 00 00 FD 01 05 00`. J ROMs adjust one command byte by −4. The purpose is undocumented beyond the comment | SCRIPT |
| TMs/HMs | **arm9 `.rodata` TM/HM move table** (101 u16: TM01–92, HM01–06, TM93–95); **arm9 `.rodata` item icon/palette table** (palette u16 at item·4+2 recoloured by move type); `a/0/0/2` 63 | | **ARM9-DATA** + TEXT |
| Field TMs and items, hidden items | `a/0/5/6` 1240/1241 | | SCRIPT |
| Move tutors (BW2 only) | **ov036 `.rodata` table**, 60 × 12 B, word 0 = move; compat bits in personal @60 (personal order 15/17/13/15 vs table order 13/15/15/17) | | **OVL-DATA** + DATA |
| Shops | `a/2/8/2` (ZX keeps sizes; shops 0–5 and TM shops 7/10/12/17/24 skipped); FVX also `a/2/8/3` | in BW1 shops are ov021 tables, in BW2 they are a NARC | DATA |
| Shop prices | `a/0/2/4` | | DATA |
| Pickup items | **ov166 `.rodata`** 29 u16; probabilities stay in code | | **OVL-DATA** |
| In-game trades | `a/1/6/3` + `a/0/0/2` 37 + `a/0/0/3` NPC texts | | DATA/TEXT |
| Intro Pokémon (always) | **ov294 `.rodata`** species u16 (sprite); **ov162 literal pool** species (cry, vanilla 0x23D) | | **OVL-DATA** |
| Type chart | **ov167 `.rodata`** 17×17 bytes (8/4/2/0). ZX tweak: Ghost and Dark → neutral vs Steel. FVX: full type-table randomization | | **OVL-DATA** |
| Guaranteed catching (minimum catch rate level 5) | ov167 code | `blt` → `0000` | **CODE** |
| Shedinja (always if Nincada has 2 evolutions) | ov284 hook + ITCM code (species literal) | | **CODE** |
| Pokémon palettes (FVX) | `a/0/0/4` files N·20+18/19 | | DATA |
| Not touched by ZX or FVX | Join Avenue, Dream Radar, Pokéstar Studios, Medals, Funfest missions, Key System (only Challenge Mode can be forced), Entralink/Hidden abilities beyond personal ability slot 3, Memory Link, Pokéwalker (n/a) | | — |

### 2.5 Every code and in-binary edit for BW2 US

RAM = arm9 file offset + 0x02004000, or overlay file offset + overlay load address. B2 bases: ov036 0x0217F600,
ov162/166/316 0x0219CE80, ov167 0x021998C0, ov168 0x021DDA60, ov284 0x021E30E0, ov294 0x021A1B20. W2 overlays load
0x40 higher, and the W2 main module is 0x40 bytes longer.

| # | Module | B2 US RAM (W2 US RAM) | Bytes | Trigger | Purpose | Class |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | arm9 crt0 params | 0x02004FB0 (same) + ITCM autoload size; all data after the ITCM image shifted by 496 | pointers/size | **always** (US) | ITCM extension for injected code | CODE |
| 2 | arm9 → ITCM | 0x01FF93A0–0x01FF955F (arm9.bin 0x9AAE0 / W2 0x9AB20) | 448 B new code + species→music pool | "correct static music" | new species→BGM/encounter-intro subroutine | CODE |
| 3 | ov036 | 0x021C5F16 (0x021C5F4E) | 28 B | same as #2 | hook into #2 (`ldr r2,=0x01FF93A1; bx r2`) | CODE |
| 4 | arm9 → ITCM | 0x01FF9560–0x01FF958F (arm9.bin 0x9ACA0 / 0x9ACE0); species literal 0x01FF958C (`ShedinjaSpeciesOffset` 0x9ACCC / 0x9AD0C) | 48 B | **always** (US, vanilla Nincada) | builds the "extra" Nincada evolution from a literal species | CODE (+ embedded data) |
| 5 | ov284 | 0x021E488E (0x021E48CE) | 18 B | same as #4 | hook to `0x01FF9561` | CODE |
| 6 | arm9 | 0x02022544 (0x02022570): `00 D0 81 E0` → `C0 46 C0 46`; 0x020225FC (0x02022628): `02 D1` → `C5 D1` | 6 B | Fastest Text | instant text printing | CODE |
| 7 | arm9 | 0x02010528 (same): → `02 20 70 47` (`mov r0,#2; bx lr`) | 4 B | Force Challenge Mode | difficulty getter returns Challenge | CODE |
| 8 | arm9 | 0x020208E8, 0x020208FC, 0x0202090E (W2 0x02020914/928/93A): imm 220 → 160 | 3 B | make evolutions easier | friendship evolution threshold | CODE |
| 9 | arm9 | 0x02030656 (0x02030682): `0A` → `02` | 1 B | double-battle mode | text-box lookup checks only the relevant trainer | CODE |
| 10 | ov036 | 0x021A6924/26/28 (+0x40) | 3 B | double-battle mode | overworld text-box ids for the first trainer of a pair | CODE |
| 11 | ov036 | 0x021A5DDA–DD (+0x40): 2 × `C0 46` | 4 B | double-battle mode | doubles trainers spot you even with 1 Pokémon | CODE |
| 12 | ov036 | 0x021A5DEE–F7 (+0x40): shift 3 halfwords, insert `cmp r0,#0` + `beq` (0xD0C7) | 10 B | double-battle mode | fall back to the single-trainer path | CODE |
| 13 | ov036 | 0x0219AC3A (0x0219AC7A): `01 D0` → `00 00` | 2 B | Run without Running Shoes | ignore flag 2403 | CODE |
| 14 | ov167 | 0x021AF7FE (0x021AF83E): `08 DB` → `00 00` | 2 B | guaranteed catching | always catch | CODE |
| 15 | ov168 | 0x021F1969 (0x021F19A9): `D1` → `E0` | 1 B | Disable low-HP music | `bne` → `b` | CODE |
| 16 | arm9 (FVX) | 0x02026B18 (0x02026B44), 40 B → `movs r0,#0; bx lr` and zeroes | 40 B | FVX Forgettable HMs | "is HM move" returns 0 | CODE |
| 17 | arm9 `.rodata` | 0x02090C84 (0x02090CB0) | 202 B | TM randomization | TM/HM move table | ARM9-DATA |
| 18 | arm9 `.rodata` | 0x02090D50 (0x02090D7C), palette u16 at item·4+2 | ~2 B × 95 | TM randomization | TM disc colour | ARM9-DATA |
| 19 | ov036 `.rodata` | 0x021D0B38 (0x021D0B6C), `MoveTutorDataOffset` 0x51538 (W2 0x5152C) | 60 × u16 at stride 12 | tutor randomization | tutor move table | OVL-DATA |
| 20 | ov316 `.rodata` | 0x0219FA94 (0x0219FAD4) | 3 × u16 (vanilla 495/498/501) | starters | starter cry table | OVL-DATA |
| 21 | ov294 `.rodata` | 0x021A458C (0x021A460C) | u16 (vanilla 0x23D) | **always** | intro sprite species | OVL-DATA |
| 22 | ov162 literal | 0x021A0170 (0x021A01E8) | u16 | **always** | intro cry species | OVL-DATA |
| 23 | ov166 `.rodata` | 0x0219D664 (0x0219D6A4) | 29 × u16 | pickup randomization | Pickup item table | OVL-DATA |
| 24 | ov167 `.rodata` | 0x021D7500 (0x021D7540), FVX `TypeEffectivenessOffset` 0x3DC40 | 289 B | type tweak / FVX type table | type chart | OVL-DATA |

Other regions (ZX): the move tutor offset varies per region. B2 F 0x5152C, G 0x5155C, I 0x51554, S 0x5153C,
K 0x5160C, J 0x512DC; W2 F 0x51520, G 0x51550, I 0x51548, S 0x51530, K 0x51600, J 0x512D0. Pattern-found edits are
located per ROM, so they work for any region where the pattern is unique. Edits #1–#6 are US-only in ZX.

### 2.6 IPS patches used for BW2

Records decoded from the ZX files; FVX ships identical files and adds asm sources under `patches/asm/`.

| File | Applied to | Records | What it does | Class |
| --- | --- | --- | --- | --- |
| `instant_text/b2_instant_text.ips` | arm9 | 0x1E544 len 4 `C046C046`; 0x1E5FC len 2 `C5D1` | Fastest Text (#6) | CODE |
| `instant_text/w2_instant_text.ips` | arm9 | 0x1E570 len 4; 0x1E628 len 2 (same bytes) | same for W2 | CODE |
| `musicfix/black2_musicfix.ips` / `white2_musicfix.ips` | extended arm9 | 0x9AAE1 / 0x9AB21 len 447 | species→music subroutine in ITCM (#2) | CODE |
| `musicfix/black2_ovl36_musicfix.ips` / `white2_ovl36_musicfix.ips` | ov036 | 0x46916 / 0x4690E len 28 | hook (#3) | CODE |
| `shedinja/black2_shedinja.ips` / `white2_shedinja.ips` | extended arm9 | 0x9ACA0 / 0x9ACE0 len 46 | Shedinja subroutine (#4): SetSpecies(0x124), ball 4, no item, no mark | CODE |
| `shedinja/black2_ovl284_shedinja.ips` / `white2_ovl284_shedinja.ips` | ov284 | 0x17AE len 18 (identical bytes) | hook (#5) | CODE |
| `national_dex/bw2_national_dex.ips` | **script file `a/0/5/6`[854]**, expanded by 4 zero bytes first | 13 records incl. a 6343-byte body at 0x676 (pointer table + script) | adds the "enable National Dex" command to the Pokédex-given script | SCRIPT |

There are no BW2 IPS patches for running indoors, level scaling, reusable TMs (already native in Gen 5), crit or
X Accuracy fixes (Gen 1 only), EXP (Gen 1–2 only) or followers.

### 2.7 Misc tweaks offered for Gen 5 (`miscTweaksAvailable`)

| Tweak | BW2? | Implementation | Class |
| --- | --- | --- | --- |
| Fastest Text | US only (ZX) | IPS on arm9 | CODE |
| Ban Lucky Egg / Ban Big Maniac items | yes | item-pool filter only; no ROM write | n/a |
| No free Lucky Egg | yes | script 676 | SCRIPT |
| Balance static levels | BW1 only | — | — |
| National Dex at start | all except J (ZX) | IPS on script 854 | SCRIPT |
| Run without Running Shoes | yes | ov036 #13 | CODE |
| Update type effectiveness (ZX; FVX replaced it with type-table randomization) | yes | ov167 table #24 | OVL-DATA |
| Force Challenge Mode | BW2 only | arm9 #7 | CODE |
| Disable low-HP music | yes | ov168 #15 | CODE |
| FVX: Forgettable HMs | yes (`HMMovesForgettableFunctionOffset`) | arm9 #16 | CODE |
| FVX: Fast egg hatching / No EV yields | yes | personal | DATA |

**Settings outside the misc tab that need code:**
- double-battle mode (#9–#12);
- "correct static music" (#2–#3);
- guaranteed catching (#14);
- "make evolutions easier" (#8);
- Shedinja handling (#4–#5, always);
- TM randomization (#17–#18);
- tutor randomization (#19);
- starters (#20);
- pickup (#23);
- intro Pokémon (#21–#22, always).

### 2.8 FVX vs ZX for BW2

FVX adds:
- the `a/2/8/3` shop-size writes (variable-size shops);
- palette randomization (`a/0/0/4`);
- random move names (`a/0/0/2` 403 and 16);
- type-table randomization (ov167);
- Forgettable HMs (arm9);
- EV-yield and egg-cycle tweaks (personal).

It also copies tweak files through `CopyFrom`, and adds `Arm9RepointExtensionSize=600` for BW1 shops, which it
moves into ITCM (BW1 only). It drops nothing BW2-relevant; the musicfix and Shedinja patches are kept.

### 2.9 What this means for a native port

1. **Works by loading NARCs from the user's ROM:** everything in 2.2, every script edit (provided the port runs
   `a/0/5/6` through the decompiled VM) and every text edit. That covers the bulk of randomization: species data,
   learnsets, evolutions, moves, trainers, wild Pokémon, grottoes, statics, field items, shops, prices, trades,
   starters' scripts, art and text.
2. **Tables a C build would not read from the ROM (#17–#24):**
   - the TM/HM table and TM palettes in arm9 `.rodata`;
   - the tutor table (ov036);
   - the starter cry table (ov316);
   - the intro species (ov294 and the ov162 literal);
   - the Pickup table (ov166);
   - the type chart (ov167).

   Because UPR never resizes overlays and inserts arm9 bytes only after the main module, a port can extract these at
   fixed per-version file offsets from the ROM's decompressed arm9/overlays, after BLZ decode, and fall back to its
   compiled defaults.
3. **Code patches (#1–#16):** each needs a port option or hook. A port can detect each one deterministically by
   comparing the bytes at the listed sites. Two patches carry data inside code that a port must parse if it wants
   fidelity:
   - **static music:** read the species→music literal pool after `NewIndexToMusicPrefix` in the extended arm9;
   - **Shedinja's species:** the u32 at `ShedinjaSpeciesOffset`.

   Every ZX-randomized US ROM has #1, #4–#5 and #21–#22, whatever the settings.

### 2.10 Cross-check: what the decomp already models (what a built-in randomizer could operate on)

Source: `docs/data.md` (its archive table and the sections *Species*, *Moves*, *Items*, *Trainers*, *Wild
encounters*, *Zone headers*, *In-game trades*, *Battle facilities*, *Map model sources*, *Map matrices, areas and
lights*, *Town map*, *Zone events*, *Text*), `docs/scripts.md`, the `data/` tree, and the C sources that hold the
in-binary tables. Every `data/` archive is rebuilt by the build and checked byte for byte against the ROM, so the
JSON is proven to say what the ROM holds.

`data/` today: `abilities/`, `areas/`, `constants/` (26 lists: species, moves, items, trainers, encounters, zones,
flags, vars, ...), `encounters/` (136 tables), `events/` (617), `facilities/` (Battle Subway, Trial House, Black
Tower/White Treehollow, PWT), `field_prop_models/`, `field_prop_textures/`, `field_scripts/` (1,289 `.s` files),
`items/` (640), `lights/`, `map_matrices/`, `map_models/` (glTF), `map_texture_sets/` (PNG), `moves/` (561),
`natures/`, `places/`, `pokemon/` (per species JSON + PNG sprites + palettes + WAV cries), `text/` (system and
script banks), `town_map/`, `tr_ai/`, `trades/` (32), `trainer_classes/`, `trainers/` (815), `types/`, `zones/` (616).

| UPR target (ROM location) | In the decomp | Form | Randomizer-ready? |
| --- | --- | --- | --- |
| Personal `a/0/1/6` (stats, types, abilities, held items, catch rate, growth, TM/HM and tutor bits) | `data/pokemon/<species>/data.json`, forms in `form_<n>.json` | JSON + schema; TMs and tutor moves named, not bits | Yes |
| Learnsets `a/0/1/8`, evolutions `a/0/1/9`, babies `a/0/2/0`, egg moves `a/1/2/4`, growth `a/0/1/7` | `data/pokemon/` | JSON | Yes |
| Moves `a/0/2/1` | `data/moves/` | JSON (all fields, incl. effects UPR never touches) | Yes |
| Items `a/0/2/4` (prices) | `data/items/` | JSON | Yes |
| Trainers `a/0/9/1`, parties `a/0/9/2`, text-box table `a/0/8/9`, messages `a/0/9/0` | `data/trainers/` | JSON | Yes |
| PWT Driftveil `a/2/5/0` (and every PWT, Subway, Trial House, Black Tower archive) | `data/facilities/pwt_*/`, `battle_subway/`, `black_tower/` | JSON | Yes (wider than UPR) |
| Wild encounters `a/1/2/7` | `data/encounters/` (B2/W2 differences inline, seasons) | JSON | Yes |
| In-game trades `a/1/6/3` + names in `a/0/0/2` 37 | `data/trades/` (names carried in the JSON) | JSON | Yes |
| Zone headers `a/0/1/2`; zone events/NPCs `a/1/2/6` (fake-ball levels) | `data/zones/`, `data/events/` | JSON | Yes |
| Field scripts `a/0/5/6` (statics, starters, gift Pokémon, item balls, hidden items, Lucky Egg, National Dex) | `data/field_scripts/*.s`, assembled with `include/asm/field_script.inc` (e.g. `CallWildBattle 377, 65, 1` in `rock_peak_chamber.s`) | Assembly source | Yes, but statics are raw numbers in places; a randomizer edits source or patches the assembled archive |
| Text `a/0/0/2`, `a/0/0/3` | `data/text/system/`, `data/text/script/` (`tools/text/text_data.py`) | Text files | Yes |
| Battle sprites/palettes `a/0/0/4` (FVX palettes), icons, cries | `data/pokemon/<species>/` PNG + JASC palettes + WAV | Images/audio | Yes |
| Hidden Grottoes `a/2/7/3` | Not in `data/`; entrances only in C (`src/ov012/hidden_hollow.c`, `HIDDEN_HOLLOW_ENTRANCES`) | Binary NARC | **No** (needs a converter) |
| Shops `a/2/8/2` items, `a/2/8/3` counts | Not in `data/`; read by `src/ov036/scrcmd_shop.c` (`ARCID_SHOP_ITEMS` 282, `ARCID_SHOP_ITEM_COUNTS` 283); BP shop counts and badge-mart mapping are C tables there | Binary NARC + C | **No** (needs a converter) |
| Starter art `a/2/0/2` | Not in `data/` | Binary | No |
| Pokédex area data `a/1/7/6` (UPR regenerates it) | Not in `data/` | Binary | No (a port can regenerate it from `data/encounters/` at build or load time) |
| TM/HM move table (arm9 `.rodata`) | `TM_MOVE_LIST[]` in `src/system/pml_item.c` (named `MOVE_*`) | C const | In C, not data: move to a data file for a runtime randomizer |
| TM disc palettes (arm9) | `ITEM_GRAPHICS_CHARS_AND_PALETTES` / item graphics tables in `src/system/pml_item.c` | C const | Same |
| Move tutor tables (ov036) | `MOVE_TUTOR_ITEMS_{HUMILAU_CITY,DRIFTVEIL_CITY,NACRENE_CITY,LENTIMAS_TOWN}` in `src/ov036/scrcmd_shop.c`; species compat in `data/pokemon` (`by_tutor`) | C const + JSON | Same |
| Starter species / cries (ov316) | `sPselSpecies[] = { SPECIES_SNIVY, SPECIES_TEPIG, SPECIES_OSHAWOTT }` in `src/ov316/psel.c`; the given species are in script 854's source | C const + script | Same |
| Intro Pokémon (ov294, ov162) | `SPECIES_CINCCINO` in `src/ov294/intro_script.c` and `src/ov162/game_start.c` | C const | Same |
| Pickup table (ov166) | `sPickupItems[18]`, `sPickupRareItems[11]` in `src/ov166/event_battle_return.c` | C const | Same |
| Type chart (ov167) | `data_ov167_021d7500[17][17]` in `src/ov167/btl_calc.c` (UPR's address 0x021D7500 exactly) | C const | Same |
| Friendship evolution threshold 220 (UPR code patch #8) | `#define EVO_FRIENDSHIP 220` in `src/system/shinka_check.c` | C constant | Becomes a config value |
| Fast text, Force Challenge Mode, run indoors, guaranteed catch, low-HP music, forgettable HMs, doubles, static music, Shedinja | Ordinary C in the decomp (text speed `src/system/text_speed.c` area, key system, `src/ov036` field movement, ov167 battle, ov168, `shinka_check.c`) | C code | Become port options/hooks |

**Bottom line.** Of UPR's BW2 data set, everything except Hidden Grottoes, shops, starter art and Pokédex area
data already has an editable, schema-validated source form in `data/`, plus the field scripts as assembly. The eight
in-binary tables that a compiled port would otherwise ignore are all in named C (`TM_MOVE_LIST`, the four tutor
tables, `sPselSpecies`, the intro species, the Pickup tables, the type chart). A built-in randomizer therefore needs:
(1) the port to load these archives at runtime from either the `data/` sources or a ROM (rather than baking them in),
(2) the eight C tables lifted into data files (or into a "patchable tables" registry the randomizer can overwrite at
boot), (3) converters for `a/2/7/3`, `a/2/8/2`/`a/2/8/3` and `a/2/0/2`, and (4) the UPR code patches as named options.

### Sources

- UPR ZX repository: https://github.com/Ajarmar/universal-pokemon-randomizer-zx (commit 7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c; release v4.6.1)
- ZX `Gen5RomHandler.java`, file: https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java
  - ini parsing and CopyFrom (no tweak copy): [#L129-L283](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L129-L283)
  - loadedROM and ARM9 extension: [#L429-L488](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L429-L488)
  - savingROM, saveMoves, savePokemonStats: [#L705-L808](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L705-L808)
  - starters: [#L847-L1013](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L847-L1013)
  - encounters and dex area data: [#L1020-L1364](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L1020-L1364)
  - trainers, doubles, PWT: [#L1366-L1710](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L1366-L1710)
  - learnsets and egg moves: [#L1712-L1818](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L1712-L1818)
  - static music: [#L1976-L2047](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L1976-L2047)
  - statics and grottoes: [#L2064-L2292](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L2064-L2292)
  - misc tweaks and code patches: [#L2389-L2680](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L2389-L2680)
  - TMs: [#L2682-L2776](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L2682-L2776)
  - tutors: [#L2837-L2952](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L2837-L2952)
  - text I/O: [#L2972-L2983](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L2972-L2983)
  - evolutions, Shedinja, easier evolutions: [#L3010-L3228](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3010-L3228)
  - trainer names: [#L3306-L3408](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3306-L3408)
  - intro Pokémon: [#L3450-L3509](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3450-L3509)
  - field items: [#L3582-L3803](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3582-L3803)
  - trades: [#L3805-L3917](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3805-L3917)
  - baby table: [#L3929-L3965](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3929-L3965)
  - shops, prices, pickup: [#L3990-L4162](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L3990-L4162)
  - CRC validation: [#L4164-L4203](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/Gen5RomHandler.java#L4164-L4203)
- ZX `gen5_offsets.ini`: [Black (U) #L1-L137](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/config/gen5_offsets.ini#L1-L137), [Black 2 (U) #L173-L314](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/config/gen5_offsets.ini#L173-L314), [White 2 (U) #L316-L345](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/config/gen5_offsets.ini#L316-L345), [other BW2 regions #L639-L957](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/config/gen5_offsets.ini#L639-L957)
- ZX `Gen5Constants.java`:
  - [starter scripts and encounters #L58-L116](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/constants/Gen5Constants.java#L58-L116)
  - [script command constants #L137-L142](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/constants/Gen5Constants.java#L137-L142)
  - [BW2 shop names #L301-L337](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/constants/Gen5Constants.java#L301-L337)
  - [byte-pattern locators #L362-L379](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/constants/Gen5Constants.java#L362-L379)
- ZX `AbstractDSRomHandler.extendARM9`: [#L326-L386](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/romhandlers/AbstractDSRomHandler.java#L326-L386)
- ZX `Randomizer.java` (misc tweaks first, intro Pokémon always): [#L149](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/Randomizer.java#L149), [#L671](https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/Randomizer.java#L671)
- ZX `PPTxtHandler.java`: https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/pptxt/PPTxtHandler.java
- ZX `NDSY9Entry.java` (overlay re-BLZ): https://github.com/Ajarmar/universal-pokemon-randomizer-zx/blob/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/newnds/NDSY9Entry.java
- ZX patches folder: https://github.com/Ajarmar/universal-pokemon-randomizer-zx/tree/7f00eb866ed35c8fe3963f078b6a2e0979dc2b8c/src/com/dabomstew/pkrandom/patches
- UPR FVX repository: https://github.com/upr-fvx/universal-pokemon-randomizer-fvx (commit 300ad60e905ce911b628054edd9d75320a010919; release vFVX1.6.2)
- FVX `gen5_offsets.ini`, Black 2 (U): [#L185-L336](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/resources/com/uprfvx/romio/romentries/gen5_offsets.ini#L185-L336)
- FVX `Gen5RomHandler.java`, file: https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java
  - [loadedROM #L135-L188](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L135-L188)
  - [saveMoves #L495-L535](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L495-L535)
  - [misc tweaks #L2256-L2322](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L2256-L2322)
  - [type table #L2399-L2464](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L2399-L2464)
  - [forgettable HMs #L2502-L2517](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L2502-L2517)
  - [BW2 shops #L3737-L3785](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L3737-L3785)
  - [palettes #L4031-L4063](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/Gen5RomHandler.java#L4031-L4063)
- FVX `Gen5Constants.java` (forgettable-HM bytes): [#L134-L135](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/constants/Gen5Constants.java#L134-L135)
- FVX `RomEntry.java` (tweak files copied): [#L79](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/java/com/uprfvx/romio/romhandlers/romentries/RomEntry.java#L79)
- FVX patch asm sources: [`black2_musicfix.asm`](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/resources/com/uprfvx/romio/patches/asm/musicfix/black2_musicfix.asm), [`black2_shedinja.asm`](https://github.com/upr-fvx/universal-pokemon-randomizer-fvx/blob/300ad60e905ce911b628054edd9d75320a010919/romio/src/main/resources/com/uprfvx/romio/patches/asm/shedinja/black2_shedinja.asm)
- ProjectPokemon, "B2W2 General ROM Info" (NARC descriptions): https://projectpokemon.org/home/forums/topic/22629-b2w2-general-rom-info/
- ProjectPokemon raw DB, Black 2 NARC list: https://projectpokemon.org/rawdb/black2/narc.php
- ProjectPokemon, "Editing hidden grottos via hex": https://projectpokemon.org/home/forums/topic/24239-editing-hidden-grottos-via-hex/
- Local verification (read-only): pokebw2 `extract/b2_us`, `extract/w2_us` binaries and `config/{b2_us,w2_us}/arm9/overlays/*/delinks.txt` load addresses.

## 3. Widescreen

Local paths are relative to the pokebw2 checkout. Addresses are Black 2 (US) unless marked W2. "Unverified" marks
claims that rest on secondary sources, inference or something not checked in code.

### 3.1 Summary

- **One BW2 widescreen hack exists, and it only covers the overworld.** TWiLight Menu++ (DS-Homebrew) ships a 16:10
  cheat for BW2 that works with the 3DS `TWPatch` widescreen TwlBg. It is the only one found. It changes one value:
  the overworld camera's 4:3 aspect literal (`0x1555`, 4096ths) becomes 16:10 (`0x199A`). The literal is in
  `FieldG3D_Init`'s pool in `src/ov036/fieldmap.c`. The output is anamorphic: the game still renders 256x192, the 3DS
  stretches it to 384x240, and every 2D layer and every non-field 3D scene (battles included) is simply stretched.
- **Emulators:** upstream melonDS only stretches each screen (4:3, 5:3, 16:9 or 21:9). The **WideMelon** fork
  (September 2026) renders genuinely wider 3D on its OpenGL renderer: it scales clip-space X of every vertex and of
  every box test. Its 2D layers stay 4:3 in the centre. It has no compatibility list and says nothing about Pokémon.
- **No widescreen work turned up in the Gen 4 decomp ports.** AKCore/pokeplatinum-port (melonDS-derived hardware
  models) and arcanite24/pokeplatinum-portable (SDL3) have none. The "Platinum PC port" covered on GBAtemp, whose
  provenance is disputed, advertises internal resolution and screen layout, but its coverage never says
  "widescreen" (unverified).
- **The decomp makes 3D widescreen tractable.** Almost every projection goes through one choke point,
  `GFL_G3DSysMtxSetProjection` (`src/gfl/g3d_system.c`). The aspect is an explicit parameter, the viewport is set at
  23 `G3_ViewPort(0, 0, 255, 191)` sites, chunk culling uses the hardware box test, and map streaming is a per-map-type
  table.
- **The 2D side is the hard part.** Every 2D screen (BG tilemaps, OBJ cell actors, HBlank effects, windows, display
  capture, pre-rendered movies) is authored for 256x192. A credible plan is Hor+ widescreen for the 3D scenes,
  pillarbox for 2D-only screens, and HUD anchoring for the 2D overlays drawn over 3D.

### 3.2 Existing widescreen hacks and tools

#### TWiLight Menu++ / nds-bootstrap + TWPatch (3DS, 16:10)

The DS-Homebrew guide ("Widescreen + RTCom") has the user build a widescreen `TwlBg.cxi` with TWPatch ("Widescreen
patch (384x240 16:10)"). TWiLight Menu++ then turns it on per game and applies a per-game cheat from
`resources/widescreen/<TID>-<CRC>.bin`. The guide warns that "most widescreen patches will only adjust 3D elements;
2D elements (such as menus) will only be stretched". TWiLight's list of supported games includes every Gen 4 and Gen 5
title in USA/Europe and the European languages: Diamond/Pearl, Platinum, HeartGold/SoulSilver, Black/White and
Black 2/White 2 (`7zfile/3DS - CFW users/Games supported with widescreen.txt`).

The BW2 cheats, decoded from the repository's binary files (u32 pairs, Action Replay format):

| File | Code | Meaning |
| --- | --- | --- |
| `IREO-7680.bin` (B2 USA/EUR) | `9226FE50 00001555` / `1226FE50 0000199A` / `D2000000` | if u16 [0x0226FE50] == 0x1555 (4:3), write 0x199A (1.6 = 16:10) |
| | `9218083C 00001555` / `1218083C 0000199A` / `D2000000` | same at 0x0218083C |
| `IRDO-D75C.bin` (W2) | `0x0226FE90` and `0x0218087C` | the same two patches, 0x40 later |
| `IRBO-8485.bin` (Black 1) | `0x02282308` and `0x02188FFC` | same pattern |
| `CPUE-FFFF.bin` (Platinum) | `0x0202022C` | one patch, in ARM9 main |
| `IPKE-FFFF.bin` (HeartGold) | `921F02FC 0000D008` + `92022F80 0000199A` → `12022F80 00001555`; `921F02FC 00001C20` + `92022F80 00001555` → `12022F80 0000199A` | switches between 4:3 and 16:10 by a game-state word, so widescreen is on only in some scenes |

Checked against the decomp's configs:

- `0x0218083C` is in `src/ov036/fieldmap.c`'s `.text` (`config/b2_us/arm9/overlays/ov036/delinks.txt`), in the
  literal pool of `FieldG3D_Init` (0x02180724, size 0x120, `config/b2_us/arm9/overlays/ov036/symbols.txt`).
  That function calls `GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
  FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), ...)` (`src/ov036/fieldmap.c`). W2's `0x0218087C` is the same
  literal in W2's `fieldmap.c` range.
- No module in the B2 config covers `0x0226FE50`: overlays end near `0x021FD080`, so the address is heap. It is
  probably the live field camera's copy of the aspect (`G3DCamera.projection.param3`, `src/gfl/g3d_camera.c`)
  (unverified).
- The repository history fits this reading. The first version patched only the heap address. The commit "Update
  Pokemon BW 1&2 (USA/Europe) widescreen codes for DSi mode" (RocketRobz, 2021-11-10) added the literal-pool patch,
  presumably because heap addresses differ in DSi mode.
- **Inference (unverified in play):** the codes match only `0x1555`. The battle camera uses `FX32_CONST(1.35)`
  (`0x1599`, `src/ov168/btlv_camera.c`), so battles and every other 3D scene keep their 4:3 projection and look
  horizontally stretched on a 16:10 screen.

#### Emulators

- **melonDS (upstream):** per-screen aspect ratios of "4:3 (native)", "5:3 (3DS)", "16:9", "21:9" and "window"
  (`src/frontend/qt_sdl/Screen.h`, `aspectRatios[]`). These only stretch. They are meant to pair with projection
  cheats like TWiLight's (anamorphic widescreen). The OpenGL renderer upscales internally, but there is no true
  widescreen.
- **WideMelon** (`pruefsumme/widemelon`, GPL-3.0, first release 11 September 2026): "expands the 3D render target and
  adjusts the projection so compatible games reveal additional geometry on both sides". The original view stays
  centred at the same scale, and "native 2D layers are composited over the middle without stretching". The view width
  runs from 256 to 768 (4:3 to 32:9), and only the OpenGL renderer supports it.
  - Its `src/GPU3D.cpp` multiplies clip-space X by `256 / Width()` (`WideMelon::ProjectX`, `src/WideMelon.h`) in
    `SubmitVertex` and in the box-test cube. Games that cull with the hardware box test therefore get wider culling for
    free.
  - The README admits that "some games ... may cull objects outside the original view", and that "menus, battles,
    videos, and special effects can remain 4:3".
  - There is no compatibility list and no issue mentions Pokémon. BW2 results are unknown. The only open issue about a
    specific game is flickering vertical bars in Mario Kart DS.
- **DraStic, DeSmuME:** no widescreen mode found. DraStic has a high-resolution 3D option (unverified; no primary
  source checked).
- **Dual-screen 3D and capture limits:** a Notebookcheck article quotes melonDS's developer: captured frames still
  need to be at the native 256x192 and fit within emulated VRAM. That limit applies to BW2's capture-based effects
  (3.3.5) under emulator-style widening (secondary source).
- **Demos:** no YouTube or Twitter demo of BW2 in widescreen was found. The searches returned only generic guides.

#### Gen 4 decomps and ports

- **pret/pokeplatinum and pret/pokeheartgold:** no widescreen experiment found in either repository's README or in
  the searches.
- **AKCore/pokeplatinum-port:** "compiles the same C into a native program". It maps guest memory at 0x02000000 in a
  32-bit build and models the DS's 2D, 3D and sound in `pc/hw/`, derived from melonDS's `GPU2D_Soft`, `GPU3D` and
  `GPU3D_Soft`. A differential runner checks it against melonDS. It has 3DS, Android and ARM targets but no
  widescreen. Its README's clone URL, `wize-logic/pokeplatinum-port`, no longer resolves.
- **The GBAtemp "Pokemon Platinum PC port gets a surprise release":** its readme, as quoted in the GBAtemp posts,
  has 3D internal resolution (default 10x), a 60 fps patch and a screen layout set in the config or ImGui. Commenters
  dispute its provenance (leaked source versus decomp). Whether it is the same project as AKCore's is unverified.
- **arcanite24/pokeplatinum-portable:** an SDL3 port, still loading graphics. No widescreen.
- These ports show the architectural fork in 3.5: modelling the DS GPU, which keeps 256x192 assumptions, versus
  reimplementing at the API level.

### 3.3 How BW2 renders, from the decomp

#### 3.3.1 Screen and engine use

- Only the DS main engine has 3D, on BG0: `GX_BG0_AS_3D` appears in 41 files. The sub engine is 2D only.
- Normally the main engine (and so 3D) is on the top screen. 19 files call `GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN)`
  to put it on the bottom. Examples are the bag (`src/ov142/itemmenu_disp.c`), the party list
  (`src/ov165/plist_sys.c`), the summary (`src/ov207/p_sta_sys.c`) and the Musical's dress-up
  (`src/ov208/dup_fitting.c`). There, 3D draws Pokémon or props in a pixel-space orthographic camera on the touch
  screen: the summary's is `GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(192), 0, 0, FX32_CONST(256),
  ...)`. **3D is not top-screen-only, and much of the "3D" in menus is really 2D.**
- **3D on both screens** is in `src/gfl/double3Ddisp.c` (`GFL_G3DDual3D*`), modelled on NitroSDK's dual-3D demo. The
  main engine renders each screen on alternate frames (30 Hz each). Display capture copies the frame to VRAM D, and
  the sub engine shows it as 12 bitmap OBJs of 64x64. The Hall of Fame (`src/ov265/ddemo_main.c`, scene 1) and the
  3D demos (`src/ov293/demo3d_engine.c`) use it.

#### 3.3.2 The 3D pipeline: GFL over NitroSystem G3D

- **The camera object:** `src/gfl/g3d_camera.c` (`GFL_G3DCameraCreate`, `GFL_G3DCameraFlush`, FOV, near and far
  setters).
- **The projection struct:** `include/gfl/g3d.h`, `G3DCameraProjection`: type PERSPECTIVE, FRUSTUM or ORTHO; for a
  perspective, the sine and cosine of half the FOV plus the aspect; `near`, `far`.
- **The choke point:** `GFL_G3DSysMtxSetProjection` (`src/gfl/g3d_system.c`) dispatches to `NNS_G3dGlbPerspective(W)`,
  `NNS_G3dGlbFrustum(W)` or `NNS_G3dGlbOrtho(W)`. The default projection there is perspective with
  `FX32_CONST(4.0 / 3.0)`.
- **Camera sites:** about 40 `GFL_G3DCameraCreate` calls. Roughly 26 are perspective, 12 orthographic and 2 frustum.
  22 files spell the aspect as `FX32_CONST(4.0 / 3.0)`.
- **Projections that bypass the choke point:**

  | File | Call |
  | --- | --- |
  | `src/system/screentex.c` | `NNS_G3dGlbPerspective(..., FX32_ONE * 4 / 3, ...)` |
  | `src/ov148/encounter_effect_capture.c` | `G3i_PerspectiveW_(..., FX32_ONE * 4 / 3, ...)` |
  | `src/system/mcss.c` | `G3i_OrthoW_(96, -96, -128, 128, ...)`, a pixel-space ortho for MCSS sprites |
  | `src/ov209/musical_mcss.c` | `G3i_OrthoW_(96, -95, -127, 128, ...)` |
  | `src/ov310/circle_graph.c` | `NNS_G3dGlbOrtho(..., ±1.333, ...)` |
  | `src/ov036/field_3dbg.c` | reloads `NNS_G3dGlb.projMtx` itself |
  | `src/ov293/demo3d_engine.c`, `src/ov162/title.c` | build frustums from a 4:3 aspect |

- **Viewport:** `G3_ViewPort(0, 0, 255, 191)` at 23 sites, among them `FieldG3D_InitCallback` in `fieldmap.c`.
- **World to screen:** `NNS_G3dWorldPosToScrPos` (`lib/nnsys/src/g3d/util.c`) projects with the global projection
  and **reads the viewport**. It anchors 2D to 3D in battle effects (`src/ov168/btlv_effvm.c`), the field's message
  balloons (`src/ov036/scrcmd_msg.c`), the save animation (`src/ov036/field_saveanime.c`), Xtransceiver
  (`src/ov257/ctvt_game.c`) and Feeling Check (`src/ov177/irc_sync_check.c`, the reverse, `ScrPosToWorldLine`).

#### 3.3.3 The overworld

- **Camera:** created in `FieldG3D_Init` with half a vertical FOV of 20° (40° fovy, about 52° horizontal at 4:3 and
  about 66° at 16:9, Hor+), aspect 4:3, near 1 and far 1024. `src/ov036/field_camera.c` follows the player with
  delays and changes FOV (`FieldCamera_SetFOV`), near and far.
- **Camera boundaries:** per-zone boundaries from archive 0x6D (`FieldCameraBoundary_RestrictRect`, `_RestrictCircle`
  and `_RepealRect`).
- **Camera areas:** `src/ov036/fld_scenearea.c` switches the camera inside parts of a map. Scripts also drive it
  (`scrcmd_camera.c`, `field_camera_anime.c`, `entrance_camera.c`). All of this is tuned to the 4:3 frustum.
- **Map streaming:** `src/ov036/field_g3d_mapper.c` loads a `chunkLoadDiameterX × chunkLoadDiameterZ` window of
  chunks around the player. `MAP_CONFIGS[]` in `src/ov036/fieldmap.c` sets it per map type.
  - Each chunk is `0x200000` units (512, or 32 tiles of 16), and the grid diameters vary: 2×2 is the most common,
    with 1×1, 2×3, 3×2, 1×6 and 5×1 elsewhere.
  - The config also sets the chunk buffer size (0x15004 or 0x1B008, among others) and the field heap (usually
    0x136800).
  - With an even diameter, `func_ov036_02184fb8` picks the neighbouring chunk on the side of the half the player
    stands in, so at least half a chunk (16 tiles) of geometry is loaded on every side.
- **Chunk data:** chunks are `WB`, `GC`, `NG` or `RD` (`field_map_chunk_*.c`), each with a BMD0 model and terrain
  (`docs/data.md`, "Map model sources"). Map matrices are grids of chunk models, and null cells draw nothing
  (`docs/data.md`, "Map matrices").
- **Culling** uses the hardware box test only:
  - `FieldChunk_CheckModelBBox` and `FieldChunk_CheckBBox` (`src/ov012/field_g3d_map.c`) test each chunk's model box.
  - `GFL_G3DSysDrawObjBBoxCull` (`src/gfl/g3d_system.c`) does the same for buildings (`field_buildmodel.c`),
    `field_g3dobj.c` objects and field effects (`fldeff_*.c`).
  - Both test against the current clip matrix, so they widen automatically once the projection is wider.
- **NPCs and the player** are camera-facing billboards (`src/gfl/billboard.c`, `BlAct`; `src/ov036/fldmmdl_blact.c`)
  drawn straight to the geometry engine, with no frustum test of their own. Some field objects are full 3D models
  (`fldmmdl_g3dobj.c`).
  - Field actors are created from the zone's entities (`src/ov012/fldmmdl.c`). The decompiled code shows no
    view-dependent NPC culling or activation radius.
  - Trainer sight is a tile range (`src/ov036/event_trainer_eye.c`), so a wider view changes nothing in gameplay.
  - Whether actors of a neighbouring zone in a multi-zone matrix are loaded before the player crosses into it is
    unverified. In Gen 4 they are not, which shows as pop-in.
- **Other field 3D:**
  - `field_3dbg.c` draws flat textured quads for weather backgrounds, with explicit width and height.
  - The fog, lights, lens flare and toon or edge-marking setup are in `FieldG3D_InitCallback`.
- **Field 2D on the top screen:** message windows, the place name, the weather layers and screen effects are 2D BG
  and OBJ.
  - Weather is OBJ particles and scrolling BGs hard-coded to 256x192: `src/ov076/weather_rain.c` tests
    `pos.x >= 256 || pos.y >= 192`, scrolls `% 256`, and spawns from a table of x 0xC0 to 0x130; sandstorm is in
    `src/ov077/weather_sandstorm.c`.

#### 3.3.4 Battles

- **The 3D stage:** `src/ov168/btlv_effect.c` builds the view.
  - `btlv_stage.c`: the two grounds the Pokémon stand on.
  - `btlv_field.c`: the ground and weather model around them, archive 0x97.
  - `btlv_camera.c`: the camera, at sin and cos `0x399`/`0xF97` (about 13° half FOV, 26° fovy) with aspect
    **`FX32_CONST(1.35)`**, not 4:3, plus moves and shakes.
- **Pokémon sprites** are MCSS (`src/system/mcss.c`; battle side in `src/ov168/btlv_mcss.c`).
  - Each sprite is a multi-cell animation whose cells are textured quads in 3D, with a shadow. Sprites go through
    the perspective camera, or optionally a private pixel-space ortho (`G3i_OrthoW_` ±128 by ±96).
  - MCSS is also used by trade, evolution, egg hatching, the intro and the Musical (`*_mcss.c`).
- **The HUD** is 2D cell actors at hard-coded positions. The gauges are in `src/ov168/btlv_gauge.c`:
  `basePositions[6] = { {216,120}, {44,40}, ... }`, anchored implicitly to the right edge for the player and the left
  for the opponent. The message window is a BG.
- **Move effects** (`btlv_effvm.c`) mix 3D particles (`src/gfl/particle.c`, SPL), MCSS moves, 2D OBJ, and BG 3
  scrolls and waves (`src/ov168/btlv_bg.c`, per-line offsets in an HBlank task). They anchor 2D to 3D with
  `NNS_G3dWorldPosToScrPos`. Full-screen effect backgrounds are 256-wide BG art.
- **The bottom screen** (`btlv_input.c`) is a 2D touch UI.
- **Unverified:** whether the battle field model has geometry beyond the 4:3 frustum at the sides. Camera moves
  during attacks may already show more of it.

#### 3.3.5 2D, raster effects and capture

- **BG:** `src/gfl/bg_sys.c` (`GFL_BGSys*`), included by about 330 files.
  - It handles text and affine BGs (`BGMODE_AFFINE`) at 256x256 to 512x512 (`BGRES_*`) and **keeps screen buffers in
    RAM** before copying them to VRAM, a natural interception point.
  - The C-Gear (`src/ov079/c_gear_sw.c`, five panels plus a background) uses 256x256 BGs.
- **OBJ:** `src/gfl/clact.c` (ClAct over `NNS_G2d` renderers), in about 260 files.
  - Its render surfaces are `{0, 0, 256, 192}` for each engine (`sSurfaceSetups`), with the sub screen at y = 192 in
    one virtual space (`data_02093f24`).
  - A culling callback drops OAM outside a surface. On hardware, OBJ X is 9-bit (wrapping at 512) and Y is 8-bit.
- **HBlank and raster:** 18 files register HBlank work.
  - The screen wipes (`src/system/wipe.c` and `wipe_sub.c`) draw a circle "line by line in the H-blank" with the
    window registers, and also cover lines from an edge or grow a window rectangle.
  - Others: battle BG waves (`btlv_bg.c`), Pokédex lists and search (`ov299`, `ov300`, `ov304`), `gsync_disp.c`,
    `un_select.c`, `pmsiv_edit.c`, `wifilogin_disp.c`, and the error screens.
  - 96 files use the window registers. DS window X coordinates are 8-bit (0 to 255).
- **Display capture:**
  - `src/system/screentex.c`: the screen captured to a 256x192 direct-colour texture, used by
    `src/ov149/encounter_effect_model.c`.
  - `src/ov148/encounter_effect_capture.c`: the 3D screen to VRAM D, then distorted as a model (battle-intro
    transitions).
  - `src/ov293/demo3d_capture.c`: motion blur by blending each frame with the last.
  - `src/ov162/title.c`: capture blend on the title screen.
  - `double3Ddisp.c` (above).
  - The hardware caps every capture at 256 wide.
- **Pre-rendered movies:** `src/ov264/cdemo_*.c` plays scripted sequences (`CDemoCmd_StartMovie`), streaming
  LZ-compressed per-frame BG screens into BG2 and BG3 with double buffering (`CDemo_LoadMovieFrame`,
  `cdemo_control.c`). One script is `CDEMO_SCRIPT_LOGO`. These are fixed 256x192 art.

### 3.4 What widescreen needs, by screen type

Difficulty is the effort in a native port, given the hooks in 3.6.

| Screen type | How it is drawn now | Widescreen approach | Difficulty | Hooks it needs |
| --- | --- | --- | --- | --- |
| **Overworld 3D** (top) | Perspective camera, 40° fovy, 4:3 (`fieldmap.c`); box-test culling; billboard NPCs; chunk window per map type (`MAP_CONFIGS`) | Hor+: aspect = W/H; widen the viewport. Raise `chunkLoadDiameterX` (2 to 3 for 16:9, more for 21:9) and grow the heap and chunk buffers. Review camera boundaries (arc 0x6D) and map-matrix edges for exposed voids. TWiLight's 16:10 code shows the projection alone is enough for a first pass. | Medium | Projection override in `GFL_G3DSysMtxSetProjection`; viewport; box test against the widened clip matrix; overrides for `MAP_CONFIGS`, heap and the boundary data; a polygon budget if the 3D model keeps the 2048-polygon and 6144-vertex limits |
| **Overworld 2D overlays** (message windows, place name, weather, field effects, balloons) | BG and OBJ on main, 256x192 | Centre the message windows (or anchor them bottom-centre). Widen weather spawn and despawn ranges and BG scroll wraps, and make the balloons follow `WorldPosToScrPos` in wide coordinates. | Low to medium | Per-layer BG policy; ClAct surface rects; wider OBJ X than 9 bits; per-file patches (`weather_*.c`) |
| **Battle top screen** | 3D stage, field model, MCSS; camera aspect 1.35; HUD OBJ at fixed positions; BG 3 effects | Hor+ on the battle camera; anchor the gauges (player right, opponent left); extend or pillarbox the full-screen effect BGs; check that the field model covers the sides (unverified); check camera moves | Medium to high | Projection; HUD anchor tables (`btlv_gauge.c` `basePositions`); BG effect policy; MCSS in wide coordinates |
| **Battle bottom screen** (`btlv_input.c`) | 2D touch UI | Keep 4:3; place it beside, overlaid, or in its own window or device | Low | Screen layout; touch mapping |
| **Menus and 2D apps** (bag, party, summary, PC box `box2_*`, Pokédex `zukan*`, Xtransceiver `ctvt_*`, mail, trainer card, PWT/`wbt_*` and Join Avenue UIs) | BG and OBJ on both screens, often with 3D ortho Pokémon (summary, party list) and HBlank lists | **Pillarbox** (a coloured or blurred border, or a wallpaper). True widescreen would mean re-authoring every layout. Keep the 3D ortho layers centred with the 2D. | Low for pillarbox, very high for native wide | Per-scene "4:3 island" mode; renderer composites the centred 256x192 |
| **C-Gear and the field's bottom screen** (`c_gear_sw.c`, `field_menu.c` "on the touch screen") | Sub-engine BGs and skins (256x192 art), touch | Keep 4:3; it is a touch surface. Optionally a side panel on wide displays | Low | Screen layout; touch mapping |
| **3D cutscenes** (`demo3d` cruise, ferris wheel, ships and flights; Hall of Fame `ddemo`; evolution `shinka_demo`; egg `egg_demo`; trade `pokemontrade_3d`; intro; title) | Perspective cameras at 4:3, some dual-screen via capture, motion blur via capture | Hor+ where the set is modelled wide enough. Dual 3D and capture effects need wide capture targets, or render both screens directly in the port. Else pillarbox per demo. | Medium (per scene) | Projection; wide capture buffers; dual-3D reimplementation (two render targets, no alternate frames) |
| **Pre-rendered movies** (`cdemo`) | BG frame streams, 256x192 | Pillarbox only, unless replacement assets are made | Low (pillarbox) | Per-scene 4:3 mode |
| **Pokémon Musical** (`sta_acting.c`, `musical_mcss.c` ortho ±128/±96; dress-up `dup_fitting.c` ortho on the bottom screen) | 2D stage with pixel-space ortho 3D | Pillarbox, unless the stage art is extended | Low (pillarbox), high (wide) | Ortho kept centred; per-scene mode |
| **Fades and transitions** | Master brightness; `wipe.c` window and HBlank circles; encounter effects that capture the screen to a texture (`ov148`, `ov149`, `screentex.c`) | Brightness covers the whole window. Wipes need window coordinates wider than 255 (a circle centred at virtual x + offset). Encounter effects need a wide capture and a wider effect quad, or a pillarboxed capture. | Medium | Window registers in wide coordinates; wide capture; per-line effects in the new renderer |
| **Pokéstar Studios, PWT, Join Avenue** | Mostly field, battle and menus (`wbt_*.c`, `scrcmd_join_avenue_store.c`); Pokéstar's own rendering not checked (unverified) | Inherit the field, battle and menu policies | (as parts) | (as parts) |

### 3.5 What the port's renderer architecture means

1. **A hardware-model renderer (melonDS-style, like pokeplatinum-port's `pc/hw/`).** The game writes registers, OAM
   and VRAM as on the DS, and the models rasterize 256x192.
   - Widescreen means changing the models, as WideMelon did: a wider 3D target, clip-X scaling (box test included),
     and wider 2D line buffers with a policy for each BG and OBJ.
   - The hardware formats don't fit: OAM X is 9-bit, BG scroll is 9-bit, window X is 8-bit and the viewport is 8-bit.
     That forces side channels: the port records where things are meant to go outside the registers, or patches the
     game.
   - Capture, dual 3D and HBlank effects stay accurate, but stay 256 wide unless reworked.
   - Upside: the best fidelity and a differential oracle. Downside: every widescreen feature fights the hardware
     model.
2. **An API-level reimplementation (HLE, RT64-like).** Take over at GFL and NitroSystem: `GFL_G3DSys*` and the
   `NNS_G3d*`/`G3_*` geometry commands go to a modern GPU backend, `GFL_BGSys*` (screen buffers in RAM) and
   ClAct/`NNS_G2d` cells are drawn as textured quads with float positions, and capture becomes render-to-texture.
   - Widescreen, HUD anchoring and high resolution become parameters, not hacks.
   - The cost is reproducing DS quirks: toon and edge marking, fog tables, 1-dot polygon rules, translucency sort
     (`GX_SORTMODE_MANUAL`), the polygon and vertex limits, and box-test semantics.
3. **A hybrid fits the decomp's state best.** Use a hardware model for correctness and as an oracle, and add
   HLE-level hooks for widescreen at the GFL layer: projection, viewport, ClAct surfaces, BG policy and capture size.
   The 2D engine can keep a "4:3 island" mode for pillarboxed scenes.

### 3.6 Hooks the port must expose

1. **Projection override** at `GFL_G3DSysMtxSetProjection`, plus the 6 or 7 direct calls in 3.3.2. Policies:
   - Perspective: Hor+, so aspect × (W/256) at constant fovy.
   - Pixel-space ortho (the summary, party list, MCSS and Musical): keep centred, as 2D.
   - Frustum: widen left and right.
   - Each scene can opt out.
2. **Viewport and world-to-screen** consistency: the 23 `G3_ViewPort` sites and `NNS_G3DGetViewport` (used by
   `NNS_G3dWorldPosToScrPos`) must report the same virtual width as the 2D layers.
3. **Culling:** the box test (`G3X_GetBoxTestResult`) must test against the widened clip volume. WideMelon applies
   `ProjectX` to the box-test cube for this.
4. **Streaming:** per-map-type overrides of `chunkLoadDiameterX` and `Z`, the chunk buffers and the field heap (the
   `MAP_CONFIGS` fields). A view-width-aware load window would be cleaner.
5. **Camera limits:** a widescreen-aware relaxation of the camera boundaries (arc 0x6D), or edge geometry (skirts) on
   maps whose edges become visible (unverified how many).
6. **2D layout:** for each scene and layer, choose centre (pillarbox), stretch, wrap or extend. Anchor OBJs by
   explicit tables (battle gauges) or a heuristic (x < 64 left, x > 192 right), with OBJ coordinates wider than 9 bits.
   ClAct surface rects must widen with them.
7. **Capture and dual 3D:** render targets at the virtual width; dual 3D drawn on both screens each frame.
8. **Raster:** per-line scroll and window effects applied in wide coordinates (wipes, battle waves).
9. **Screen layout and touch:** top widescreen with the bottom screen side by side, overlaid, swapped or on a second
   window or device (WideMelon streams it to a phone), and touch mapped back to 256x192.

### 3.7 Lessons from other ports

- **sm64 (n64decomp):** `include/gfx_dimensions.h` defines `GFX_DIMENSIONS_FROM_LEFT_EDGE(v)` and `_RIGHT_EDGE(v)`.
  Coordinates stay in a 320x240 space with x = 0 at the 4:3 edge (−53.33 at 16:9), so HUD code needs negative
  coordinates. Lesson: **keep the native coordinate space and add edge-anchor macros**. BW2's ClAct and BG positions
  could take the same, around 256x192.
- **Ship of Harkinian (OoT):** `WidescreenActorCulling` in `soh/src/code/z_actor.c` divides the projected-X culling
  test by `currentAspect / (4/3)`. Its options exclude certain actors to keep glitches intact. 2 Ship 2 Harkinian
  (MM) ships a HUD editor with 4:3 and widescreen presets. Lesson: **CPU-side culling and draw distance must be
  widened separately, and they can affect gameplay.** BW2 appears to use hardware box tests and tile-based logic, so
  it is less exposed. This rests on the decompiled field files only, and the decomp is incomplete.
- **Zelda64Recomp / RT64:** "any aspect ratio is supported, with most effects modded to work correctly in
  widescreen". The HUD can stay at 16:9 on ultrawide, and some cutscenes show quirks at the edges. Lesson: **effects
  and cutscenes need per-case fixes**, and a "HUD at 16:9" option is a cheap compromise for ultrawide.
- **WideMelon, the TWiLight codes and Dolphin's widescreen-hack history:** projection-only hacks give fast results
  in 3D-heavy scenes, but 2D and HUD stretch or stay 4:3, and objects pop at the edges.
- **DS decomps:** an sm64ds decomp reports about 90% of functions matched and plans a "recomp" (Patreon,
  self-reported; widescreen not stated). No Mario Kart DS decomp port was found. TWiLight lists a Mario Kart DS
  widescreen `xdelta`, and WideMelon has an open issue about flickering bars in Mario Kart DS.

### 3.8 Open questions (unverified)

- What the heap address `0x0226FE50` (B2) is. A live camera aspect is likely; confirm with a debugger.
- Whether the 2×2 chunk window shows pop-in at the far corners at 16:9 and 21:9 with the default field camera.
- Whether the battle field model covers a Hor+ 16:9 frustum, and whether any battle camera move breaks.
- Whether NPCs of neighbouring zones in multi-zone matrices are loaded before the player enters.
- How Pokéstar Studios renders.
- How DSi mode changes the heap layout. It matters for cheat addresses only, not for a port.

### Sources

- DS-Homebrew, TWiLight Menu++ widescreen list:
  https://github.com/DS-Homebrew/TWiLightMenu/blob/master/7zfile/3DS%20-%20CFW%20users/Games%20supported%20with%20widescreen.txt
- TWiLight Menu++ widescreen codes (`IREO-7680.bin`, `IRDO-D75C.bin`, `IRBO-8485.bin`, `CPUE-FFFF.bin`,
  `IPKE-FFFF.bin`): https://github.com/DS-Homebrew/TWiLightMenu/tree/master/resources/widescreen
- Commit "Update Pokemon BW 1&2 (USA/Europe) widescreen codes for DSi mode":
  https://github.com/DS-Homebrew/TWiLightMenu/commit/5a504578bc61bbb5d32cefb85b068c73bbf918a1
- DS-Homebrew wiki, Widescreen + RTCom (TWPatch 384x240 16:10): https://wiki.ds-homebrew.com/ds-index/rtcom
  (source: https://github.com/DS-Homebrew/wiki/blob/main/pages/_en-US/ds-index/rtcom.md); older "Playing in
  Widescreen" page (the 3D-only caveat): https://wiki.ds-homebrew.com/twilightmenu/playing-in-widescreen
- melonDS aspect-ratio options:
  https://github.com/melonDS-emu/melonDS/blob/master/src/frontend/qt_sdl/Screen.h
- WideMelon repository (README; `src/GPU3D.cpp`, `src/WideMelon.h`): https://github.com/pruefsumme/widemelon
- Held Games, WideMelon article: https://heldgames.com/guides/widemelon-ds-widescreen
- Notebookcheck, melonDS dual-screen 3D and capture constraints (secondary):
  https://www.notebookcheck.net/High-accuracy-dual-screen-3D-texture-filtering-and-other-features-coming-to-DS-emulation.1165674.0.html
- Emulation General Wiki, Widescreen hacks (general limitations; 403 when fetched, cited from search snippets):
  https://emulation.gametechwiki.com/index.php/Widescreen_hacks
- AKCore/pokeplatinum-port (README, `pc/README.md`, `pc/hw/README.md`): https://github.com/AKCore/pokeplatinum-port
- arcanite24/pokeplatinum-portable: https://github.com/arcanite24/pokeplatinum-portable
- GBAtemp, "Pokemon Platinum PC port gets a surprise release" (403 when fetched; cited from search snippets):
  https://gbatemp.net/posts/10863352/
- pret/pokeplatinum: https://github.com/pret/pokeplatinum ; pret/pokeheartgold: https://github.com/pret/pokeheartgold
- sm64 `gfx_dimensions.h`: https://github.com/n64decomp/sm64/blob/master/include/gfx_dimensions.h
- Ship of Harkinian `z_actor.c` (`WidescreenActorCulling`):
  https://github.com/HarbourMasters/Shipwright/blob/develop/soh/src/code/z_actor.c
- 2 Ship 2 Harkinian 1.0.0 release notes (HUD editor presets):
  https://github.com/HarbourMasters/2ship2harkinian/releases/tag/1.0.0
- Zelda64Recomp README (Widescreen and Ultrawide Support): https://github.com/Zelda64Recomp/Zelda64Recomp
- sm64ds decomp progress (self-reported): https://patreon.com/the_tango
- GBATEK, DS hardware reference (OAM, BG, window, capture and 3D viewport limits): https://problemkaputt.de/gbatek.htm
- Local evidence: `src/gfl/g3d_system.c`, `src/gfl/g3d_camera.c`, `include/gfl/g3d.h`, `src/gfl/double3Ddisp.c`,
  `src/gfl/billboard.c`, `src/gfl/bg_sys.c`, `src/gfl/clact.c`, `src/ov036/fieldmap.c`,
  `src/ov036/field_camera.c`, `src/ov036/field_g3d_mapper.c`, `include/field/field_g3d_mapper.h`,
  `src/ov012/field_g3d_map.c`, `src/ov036/field_3dbg.c`, `src/ov036/event_trainer_eye.c`, `src/ov076/weather_rain.c`,
  `src/ov168/btlv_camera.c`, `src/ov168/btlv_gauge.c`, `src/ov168/btlv_bg.c`, `src/ov168/btlv_effvm.c`,
  `src/system/mcss.c`, `src/ov209/musical_mcss.c`, `src/ov207/p_sta_sys.c`, `src/ov165/plist_sys.c`,
  `src/system/wipe.c`, `src/system/wipe_sub.c`, `src/system/screentex.c`, `src/ov148/encounter_effect_capture.c`,
  `src/ov293/demo3d_capture.c`, `src/ov264/cdemo_control.c`, `src/ov079/c_gear_sw.c`, `lib/nnsys/src/g3d/util.c`,
  `config/b2_us/arm9/overlays/ov036/{delinks,symbols}.txt`, `config/w2_us/arm9/overlays/ov036/delinks.txt`,
  `docs/data.md`.

## 4. Multiplayer

Researched 2026-10-16. Where a claim rests on one community source, a forum post or my own inference, it is marked
**(unverified)**. "ROM strings" means `strings` run over the decompressed modules in `extract/b2_us/` (US Black 2). The
code map of the network stack (GFL net, the `GFLNetDevTable` seam, which overlays hold which device) is in 4.0. This
section covers what each feature uses, what still works through revival servers, and how a native port could do
multiplayer.

**Key findings**

- BW2 has five transports. The first is **local wireless**: NitroSDK WM, used both as MP connections and as
  beacons-only broadcasts. The others are **DS Download Play** (NitroSDK MB), **infrared** (Game Freak/Nintendo `irc`
  over the cartridge's IR chip), **Nintendo WFC** (TWL-DWC with the GameSpy SDK, NitroWiFi SOC/CPS/SSL and NHTTP) and
  the 3DS-side **"Nintendo 3DS Link"**, which goes through the cartridge rather than a network. Every connected mode
  except Download Play goes through one function table, `GFLNetDevTable`, which gives a native port a single place to
  plug in a host-socket device.
- Many signature Gen 5 features are **beacons-only**: the C-Gear Tag Log and surveys, Pass Power sharing, Funfest
  Missions with up to 100 players, the beacon Mystery Gift, and the Xtransceiver and Union Room presence. They never
  set up a connection. melonDS's lead says these are nearly impossible for an emulator to sync. For a native port
  they are the easy case: send each beacon to a relay and fan it out to the other players.
- In October 2026 the revival ecosystem covers about half of BW2's WFC features. Through Wiimmfi, Kaeru WFC and the
  Poké Classic Network (PKC), DNS 178.62.43.212, these work: the GTS, Battle Videos, Musical photos, the Wi-Fi Battle
  Subway, some Wi-Fi Mystery Gifts and friend battles. Random Matchup needs a client patch, because Nintendo's
  RSA-1024 party signature can't be forged. Game Sync, Memory Link over Wi-Fi and Join Avenue visitors work only
  through the self-hosted **Entralinked**. Rating battles, Wi-Fi Competitions and PWT downloads have no revival.
- A port that keeps DWC and GameSpy byte-compatible and swaps only the socket, DNS and access-point layers can play
  with real DS players and emulator users on the same revival servers. Local wireless, IR and Download Play can't
  realistically cross-play with real hardware. For those, connect port to port and replace Download Play with
  save-file import.

### 4.0 What the decomp already shows about the network stack (local evidence)

All paths are relative to the pokebw2 checkout. This is what the port actually has to replace or re-host.

**The seam: one device table.** The game never calls WM, IRC or DWC directly from feature code. Feature code talks to
the GFL net (`src/gfl/net.c`, `net_system.c`, `net_command.c`, `net_handle.c`, `net_state.c`, `net_queue.c`,
`net_ring_buff.c`, `net_lower_data.c`, `net_irc_wireless.c`), which frames packets (header with CRC, size, connected
machines bitmap, flags, sequence number; commands from a send queue; a ring buffer per machine into command handlers;
server-relayed negotiation and timing in `net_handle.c`). Below it sits a **`GFLNetDevTable`** (`include/gfl/net.h`,
~50 function pointers: `init`, `update(connectBits)`, beacon/scan accessors by index (data + MAC), `send`,
`getRecvData(netId)`, `setRecvCallback`, `isConnected`, `getConnectBits`, `getNetId`, `getSignalLevel`, `isError`,
disconnect callback, parent/child start calls...). `src/system/net_define.c` (`func_020116c0`) picks the device by
net type and loads its overlays:

| Net type | Device | Overlays and files | Library underneath |
| --- | --- | --- | --- |
| 0, 5 (local wireless) | `NetDevWireless_GetTable` | ov030: `net_devwireless.c`, `net_whpipe.c` (beacons, parent/child start), `wih.c` (Game Freak's fork of NitroSDK's `wh.c` demo) | NitroSDK WM (`lib/nitro/src/wm`: `wm_mp.c`, `wm_ds.c`, `wm_sync.c`...); DSi `lib/twl/src/nwm` |
| 3, 4 (infrared) | `NetDevIrc_GetTable` | ov031: `net_devirc.c`, `net_irc.c` (turn-taking packets, timeout, peer filter) | `lib/irc`: `irc_link.c` (frames), `irc_spi.c` (the cart's IR chip on the backup SPI bus) |
| `GFL_NET_TYPE_WIFI` (1), `_WIFI_LOBBY` (2) | `NetDevWifi_GetTable` | ov011: `net_devwifi.c`, `dwc_rap.c` (matchmaking incl. `randommatch_query`), `dwc_rapcommon.c`; ov029: `dwc_rap_vchat.c` + libVCT voice chat | `lib/dwc` (ac, auth, base, bm, crypt, gamespy, gamestats, gdb, ghttp, gt2, nd, nhttp, sake, sc, vct, wds), `lib/nitrowifi` (cps, soc, ssl, wcm), `lib/dwcutil` (AOSS/WCM/AC setup utility) |
| `GFL_NET_TYPE_WIFI_GTS` (6) | `NetDevWifi_GetTable` + ov189 | ov189: `nhttp_rap.c` (Game Freak's HTTP wrapper to the game servers), `nhttp_rap_evilcheck.c` (signs data for the server's Pokémon legality check), `dpw_profile.c` | `lib/dpw` (`dpw_tr.c` = GTS, `dpw_battle_tower.c` = Wi-Fi Battle Subway) |
| (separate) | GDS | ov201: `gds_rap.c`, `gds_rap_response.c`, `battle_rec_ov.c` | `lib/poke_net` (`poke_net.c`, `poke_net_gds.c`, `poke_net_ssl.c`): Global Data Server client (battle videos, musical shots) |
| (separate) | DS Download Play parent | ov181: `mbp.c` (NitroSDK demo parent), `mb_parent_sys.c`, `mb_comm_sys.c`, `mb_parent_dataconv_sys.c`; ov182: `download_play.c`, `download_play_sys.c`, `download_play_wih.c`, `dwc_raputil.c` | NitroSDK MB (`lib/nitro/src/mb`) |

Feature code found in `docs/source-files.md` that rides on this: the Union Room (`union_comm.c`, `union_main.c`,
`union_receive.c`, `union_subproc.c`, `union_actor.c`, `union_oneself.c`, `union_msg.c`, `union_eff.c`,
`union_app.c`, `union_subdisp.c`; ov028 per `include/system/game_comm.h`), trades (`pokemontrade_*.c`, incl.
`pokemontrade_ircdemo.c`), IR battles (`ircbattlematch.c`, `ircbattlemenu.c`, `event_ircbattle.c`), Feeling Check /
compatibility over IR (`compatible_irc_sys.c`, `irc_compatible.c`, `irc_sync_check.c`, `irc_rhythm_check.c`,
`irc_compat_*.c`, `irc_ranking.c`), the musical's sharing (`mus_comm_func.c`, ov211), beacons / Tag Mode / survey
(`src/system/game_beacon.c`, `beacon_status.c`, `game_beacon_search.c`, `game_beacon_set.c`, `beacon_view*.c`,
`beacon_detail*.c`, `medal_info_beacon.c`), the Xtransceiver (`ctvt_call.c`, `ctvt_camera.c`, `ctvt_mic.c`,
`ctvt_talk.c`, `ctvt_comm.c`, `ctvt_game*.c`, `src/system/ctvt_beacon.c`), Mystery Gift (`mystery*.c`,
`mystery_net.c`, `delivery_beacon.c` for local wireless, `delivery_irc.c` for IR), Entralink (`field_palace_sys.c`,
`entralink_warp.c`), Funfest Missions (`event_funfest_mission.c`, `scrcmd_funfest.c`, `src/system/fest_mission*.c`),
the GTS and GTS Negotiations (`worldtrade*.c`, `event_worldtrade.c`, `gtsnego_*.c`, `event_gtsnego.c`), Wi-Fi Club
(`event_wificlub.c`), Wi-Fi Battle Subway (`event_wbt_wifi.c`), Random Matchup (`br_rndmatch_proc.c`,
`rndmatch_savedata.c`), Dream World / Game Sync (`pdw_postman.c`, `pdwacc_disp.c`, `pdwacc_state.c`,
`dream_world_savedata.c`, `game_sync_settings_proc.c`), the Poké Transfer Lab (`src/ov053/scrcmd_palpark.c`, which
starts ov181's Download Play parent), and Unova Link's key/memory transfers (`src/ov332/key_system_net.c`: local
wireless, the ov181 link, and Wi-Fi). `src/system/game_comm.c` runs one "communication" at a time
(`GAME_COMM_NO_BEACON_SEARCH`, `_UNION`, `_MUSICAL`, ...) and logs players met.

**How much of this is source today (2026-10-16).** Overlay 30 (wireless), overlay 31 and `lib/irc` (infrared) are
complete files. The Wi-Fi side is not: of overlay 11's `.text` (0x34c08 bytes), only 33.6% is assigned to source
files (`dwc_rap.c`, `dwc_rapcommon.c`, `net_devwifi.c`, the NitroWiFi SOC/SSL/CPS/WCM files and GameSpy GT2); the
four unassigned ranges (0x0215dd74–0x0216eea8, 0x0216fbf8–0x021714cc, 0x0217241c–0x0217c944, 0x0217ee90–0x02184fc8)
hold 972 still-unnamed `func_ov011_*` functions, which by elimination are DWC's login/match/friend code and the
GameSpy GP, QR2, SB and NATNEG SDKs (computed from `config/b2_us/arm9/overlays/ov011/delinks.txt` and
`symbols.txt`; the identification is by elimination, not by name). ByteMatch reports nitrowifi at 43.68% and dwcutil
at 25.42% matched. Option (c) below therefore depends on more decomp work in ov011 than options (a) and (b) do.

`docs/porting-notes.md` already records host-port hazards in NitroWiFi's SSL/SOC/WCM (pointer-carrying handles,
network-order address fields, big-integer capacity, POKE_NET's Nintendo CA key), i.e. the decomp team expects to
substitute a host socket API under SOC rather than emulate the DS Wi-Fi chip.

### 4.1 The transports and what the ROM talks to

```
feature code (union_*, pokemontrade_*, btl_net, mus_comm_func, ctvt_*, key_system_net, worldtrade*, gtsnego_*, ...)
   └─ GFL net (net_system/net_command/net_handle/net_queue; ≤ GFL_NET_MACHINE_MAX = 8 machines; parent relays in MP mode)
        └─ GFLNetDevTable ── wireless: net_devwireless → net_whpipe (beacons/scan/connect) → wih.c (fork of SDK wh.c) → WM
                          ├─ infrared: net_devirc → net_irc → lib/irc irc_link (frames) → irc_spi (cart IR chip)
                          └─ Wi-Fi:    net_devwifi → dwc_rap → DWC (GameSpy GP/QR2/SB/NATNEG/GT2) → NitroWiFi SOC/CPS/SSL
side paths: MB Download Play (ov181 mbp.c → MB → WM); GTS/Subway HTTP (ov189 nhttp_rap → dpw → DWC gamestats/ghttp);
            GDS (ov201 gds_rap → lib/poke_net, own SSL thread); PGL (ov189 NHTTP → en-ds.pokemon-gl.com)
```

Facts from the SDK headers and the decomp that matter for a reimplementation:

- **WM:** one MP packet holds up to 0x200 bytes (`WM_SIZE_MP_DATA_MAX`). A beacon carries 0x70 bytes of user
  GameInfo (`WM_SIZE_USER_GAMEINFO`). Up to 15 children can join at the radio level, but GFL caps a session at 8
  machines.
- **WH:** `wih.c` supports WH's three connect modes: MP, key sharing and data sharing.
- **Funfest and deliveries:** `net_whpipe.h` names a Funfest beacon and "the beacons that deliveries come by", so the
  beacon Mystery Gift is broadcast-only as well.
- **IR handover:** `net_irc_wireless.c` pairs over IR, swaps MAC addresses, then finds the peer's wireless beacon by
  that MAC.
- **IR link:** `lib/irc` is point-to-point. Whoever pings becomes the parent, a ping times out after 100 ms, and a
  frame is a 4-byte header (command, link ID, 16-bit checksum) plus up to 0xb4 bytes.
- **Download Play:** the parent (GGID 0x1380) sends one of two child programs. `/dl_rom/child_r_eng.srl` is the Poké
  Transfer child, for a DS with a Gen 4 cartridge. `/dl_rom/child2_r_eng.srl` is the Memory Link child: it reads the
  Black/White cartridge and sends back 0x500 bytes of its save (`mb_comm_sys.c`, `mb_parent_dataconv_sys.c`).
- **DSi mode:** BW2 is DSi-enhanced. In DSi mode the WFC connection goes through TwlSDK NWM (`NWM_Connect` and
  others), which is what gives DSi and 3DS players WPA/WPA2. ov189 also carries "GameSpy SSL (TWL engine)" strings.

**Hostnames and endpoints in the ROM strings.** Each module lists the endpoints it holds and what they are for.

- ov011 (DWC and NitroWiFi):
  - Login: `https://nas.nintendowifi.net/ac`. The `nas.test` and `nas.dev` variants are also present.
  - Connection test: `http://conntest.nintendowifi.net/`.
  - GameSpy Presence: `gpcm.gs.nintendowifi.net` and `gpsp.gs.nintendowifi.net`.
  - Matchmaking: `%s.master.gs.nintendowifi.net`, `%s.available.gs.nintendowifi.net` and
    `%s.ms%d.gs.nintendowifi.net`, with the game name `syachi2ds`.
  - NAT negotiation: `natneg1`, `natneg2` and `natneg3.gs.nintendowifi.net`.
  - Stats: `gamestats.gs.nintendowifi.net`.
  - Trusted CA certificates: Nintendo CA, Nintendo Class 2 CA and Thawte.
- ov189:
  - GTS (`dpw_tr`): `http://gamestats2.gs.nintendowifi.net/` + `/web/worldexchange/{info,post,post_finish,get,
    result,delete,return,search,exchange,exchange_finish}.asp`.
  - Wi-Fi Battle Subway (`dpw_battle_tower`): `/web/battletower/{info,roomnum,download,upload}.asp`.
  - Profile: `/web/common/setProfile.asp`.
  - Party validation: `https://pkvldtprod.nintendo.co.jp/pokemon/validate`.
  - Pokémon Global Link: `https://en-ds.pokemon-gl.com/dsio/gw?p=…`. The calls are `account.create.upload`,
    `account.createdata`, `account.playstatus`, `savedata.upload`, `savedata.download`, `savedata.download.finish`,
    `savedata.getbw` (Memory Link over Wi-Fi), `sleepily.bitlist`, `worldbattle.upload` and `worldbattle.download`
    (Wi-Fi Competitions).
  - DWC ND downloads: `https://%s/download`, i.e. DLS1, used by the Wi-Fi Mystery Gift **(unverified)**.
  - The GameSpy gamestats salt and game name, `…HZEdGCzcGGLvguqUEKQN…syachi2ds`.
  - Developer leftovers: `sdkdev.gamespy.com` and `ishikawa.servebeer.com`.
- ov201 (GDS, `lib/poke_net`): `pkgdsprod.nintendo.co.jp`, with the Nintendo Class 2 CA. GDS carries the Battle
  Videos and Musical photos.
- ov260 (Random Matchup rating and Wi-Fi cup records): the GameSpy Sake file server,
  `https://%s.secure.sake.gs.nintendowifi.net/SakeFileServer/{upload,download}.aspx`. The record names include
  `ARENA_ELO_RATING_1V1_WIFICUP`.
- `WiiLink24/wfc-server`'s `game_list.tsv` lists `syachi 2 (DS)` with the game name `syachi2ds`, game ID 2911 and the
  same gamestats key `HZEdGCzcGGLvguqUEKQN`, which matches the ROM.

The DS's SSL is narrow. NitroWiFi's `ssl.c` speaks SSLv3 only, offers only `SSL_RSA_WITH_RC4_128_MD5` and
`SSL_RSA_WITH_RC4_128_SHA`, and uses RSA key exchange; the decomp documents this. No modern TLS server will agree to
that, which is why revival servers run custom TLS front ends. The decomp also says `poke_net_ssl.c`'s
certificate-check callback returns 0 for every result, so the GDS client in effect accepts any server certificate.
That explains how PKC can serve `pkgdsprod` **(inference from the decomp comment)**.

### 4.2 Feature table

Abbreviations:

- **WM-MP**: a connected WM session (parent and children)
- **WM-beacon**: beacons and scanning only, with no connection
- **MB**: DS Download Play
- **IR**: the cartridge infrared link
- **WFC**: the internet through DWC
- **KC**: Kaeru WFC, the DNS-plus-TLS gateway
- **W**: Wiimmfi
- **PKC**: Poké Classic Network
- **EL**: Entralinked

| Feature | Transport | SDK libs (game layer) | Server / protocol | Revival status (Oct 2026) |
| --- | --- | --- | --- | --- |
| Union Room: trades, 2- and 4-player battles including Multi, chat | WM-MP (presence by beacon) | WM ← WH (`wih.c`) ← GFL net (ov028/034/069/082 `union_*`) | none | local only. Reported to work across melonDS instances (community guides) **(unverified for BW2)** |
| C-Gear IR trade, battle, Multi battle, Pal Pad friend codes | IR pairing, then WM-MP (`net_irc_wireless.c`) | IRC + WM | none | local only. No emulator emulates the cartridge IR chip **(unverified)** |
| Feeling Check and its ranking | IR | IRC (ov175 `irc_compatible`, ov180 `irc_ranking`) | none | local only |
| C-Gear Tag Mode: Tag Log, Passerby Analytics surveys, thanks/messages, Pass Power sharing | WM-beacon (passive) | WM ← `game_beacon.c`, `beacon_status.c`, ov310 `research_*`, ov083/084/308 `beacon_view/detail` | none | local only. melonDS only partly handles passive beacons (issue #1672) **(unverified)** |
| Funfest Missions, up to 100 players (Entralink hub) | WM-beacon (`fest_mission_beacon.c`) | WM | none | local only. melonDS caps at 16 and its MP sync doesn't cover beacon-only traffic (issue #2510) |
| Join Avenue visitors | side effect of any connection (Tag Log, trades, battles), plus Game Sync | `game_comm.c` logs players met | PGL (Game Sync) | local, and EL can push visitors |
| Xtransceiver: up to 4-way recorded voice, DSi/3DS camera video as JPEG, drawing, balloon minigames | WM-MP (`ctvt_comm.c`); over WFC from the Global Terminal (BW2) | WM + TWL camera/mic/JPEG (`lib/twl`); WFC path via DWC **(details unverified)** | WFC P2P for the Global Terminal path | local works in melonDS after registering friends (issue #1672); WFC path **unverified** |
| Pokémon Musical with friends | WM-MP (ov211 `mus_comm_func`, `comm_entry_menu`) | WM | none | local only |
| Musical photo upload and browse | WFC | `lib/poke_net` (GDS) | `pkgdsprod.nintendo.co.jp` | **works** (PKC) |
| Musical show, C-Gear and Pokédex skin downloads | WFC (Game Sync) | NHTTP | PGL `dsio/gw` | EL (self-hosted) |
| Mystery Gift: Wonder Cards by local wireless | WM-beacon (`delivery_beacon.c`) | WM | distribution beacon | local only. A port can load card files |
| Mystery Gift by IR | IR (`delivery_irc.c`) | IRC | none | local only |
| Mystery Gift by Wi-Fi, serial codes | WFC | DWC ND (DLS1) **(unverified)** | NAS svcloc → DLS1 `/download` | **partial**: W's third-party DLS1 carries some Gen 5 events (Poképédia) |
| Unova Link Key System key transfer | WM-MP (`key_system_net.c`) | WM | none | local only |
| Memory Link (BW1 → BW2) | MB (child2 program to a DS with BW1), or WFC via PGL `savedata.getbw` | MB/WM; NHTTP | PGL | MB local only. EL does Memory Link from a BW save file |
| Poké Transfer from Gen 4 | MB (`child_r_eng.srl`; the minigame runs on the Gen 4 DS) | MB/WM (ov181) | none | local only. A port should import a Gen 4 save instead |
| Pokémon Dream Radar (3DS) | "Nintendo 3DS Link": the 3DS app writes research data to the inserted BW2 cartridge, which Unova Link then reads | none (save data). The decomp's ov181 notes infer a second MB mode tied to Dream Radar **(unverified)** | none | app delisted with the 3DS eShop, 27 Mar 2023. A port can inject the data |
| Battle Subway Multi with a friend | WM-MP or IR | WM/IRC | none | local only |
| Wi-Fi Battle Subway (Wi-Fi Train) | WFC | DWC gamestats/ghttp ← `lib/dpw` (`dpw_battle_tower.c`) | `gamestats2…/syachi2ds/web/battletower/*.asp` | **works** (PKC) |
| GTS | WFC | DWC + `lib/dpw` (`dpw_tr.c`) + NHTTP validation | `gamestats2…/web/worldexchange/*.asp`; `pkvldtprod` (the GTS accepts an all-zero signature, per PKC's code) | **works** (PKC, filterable by Gen V version) |
| GTS Negotiations | WFC P2P | DWC matchmaking (GameSpy QR2/SB/NATNEG) + GT2, through GFL net Wi-Fi | GameSpy master + NATNEG | probably works through W's general matchmaking **(unverified)** |
| Wi-Fi Club: friend battles and trades, libVCT voice chat | WFC P2P | DWC GP (presence) + GT2; libVCT (ov029) | GameSpy GP + NATNEG | friend battles reported working DS↔melonDS on PKC/W (melonDS #2164, Oct 2024) |
| Random Matchup, Free mode (Single/Double/Triple/Rotation/Launcher) | WFC P2P | DWC matchmaking + `nhttp_rap_evilcheck` | GameSpy + `pkvldtprod` RSA signature | **blocked unless patched**. W fixed matchmaking, but the signature can't be forged. An AR code or ROM patch (2025) makes it work |
| Random Matchup, Rating mode | as Free, plus records | + Sake/SC (ov260) and PGL registration | Sake, PGL | **not working** (no PGL ranking; signature, as above) |
| Global Link Wi-Fi Competitions | WFC | NHTTP, Sake/SC | PGL `worldbattle.*` | **not working** (PKC: unsupported) |
| Live Competitions with a Digital Player ID | IR registration (`livebattlematch_irc.c`) | IRC | none (the ID came from PGL) | offline parts only |
| Battle Videos: upload, browse, rankings | WFC | `lib/poke_net` (GDS) | `pkgdsprod.nintendo.co.jp` | **works** (PKC) |
| Game Sync and Dream World (tuck in, wake up, Entree Forest, Dream Remnants) | WFC from the C-Gear "Online" menu | NHTTP + GameSpy auth | `en-ds.pokemon-gl.com/dsio/gw` | **EL only** (self-hosted Game Sync emulator). Dream World browser game: "Dream World Reawakened" (Flash) **(unverified maturity)**. PKC: open since 2014 (issue #8) |
| PWT Download Tournaments | WFC download (ov327 `wbt_download_*` **(identification unverified)**) | DWC | unknown (DLS1 or PGL) **(unverified)** | none known |

Shutdown dates: the Gen 5 Dream World closed on 14 Jan 2014, after new tuck-ins stopped on 10 Dec 2013. Nintendo WFC
for DS ended on 20 May 2014. The Dream Radar became unavailable when the 3DS eShop closed on 27 Mar 2023.

### 4.3 Revival services today

| Service | What it does for BW2 | How to connect | Notes and status |
| --- | --- | --- | --- |
| **Wiimmfi** (wiimmfi.de) | General Nintendo WFC emulation: NAS login, GameSpy GP/QR2/SB/NATNEG, DLS1 | Through Kaeru's DNS (below). Fallback: AdmiralCurtiss's WfcPatcher rewrites the domain to `wiimmfi.de` and HTTPS to HTTP | Active; its site sits behind a Cloudflare challenge (not fetched). On 2026-10-16 its GameSpy hosts resolved to 95.217.77.151, the same IP Poképédia gives as Wiimmfi's Wii DNS |
| **Kaeru WFC** (kaeru.world) | DNS plus an SSL gateway in front of Wiimmfi. It forges server certificates the DS accepts, using the "nds-constraint" flaw found by shutterbug2000 in Oct 2018 | Primary and secondary DNS `178.62.43.212`. DS: 2.4 GHz WEP or open. DSi-enhanced games on DSi/3DS (BW2 included) can use WPA/WPA2 through the advanced connection slots | No ROM patching needed; that is its point |
| **Poké Classic Network** (pkmnclassic.net; code `mm201/pkmn-classic-framework`, C#) | Gen 4/5 game servers: GTS, Battle Videos, Musical photos (BW/BW2), Wi-Fi Battle Subway, plus Gen 4 extras | Same DNS, 178.62.43.212 | Live, with about 21k battle videos site-wide. Maintenance Thursdays 03:00 UTC. README lists Game Sync and Rating/Competitions as not working. Last push July 2026 |
| **WiiLink WFC** (wfc.wiilink.ca; `WiiLink24/wfc-server`, Go) | Open-source NWFC replacement with NAS, GPCM/GPSP, QR2, server browser, NATNEG, Sake, gamestats and DLC. Has a DS TLS 1.0/RC4 server path (`enableHttpsExploitDS`, nds-constraint) and lists `syachi2ds` | The public site documents only Wii setup. A rvtr wiki commit of July 2024 calls WiiLink's DS DNS a downstream of Kaeru's. Poképédia gives the DS DNS as `167.235.229.36` | On 2026-10-16, `167.235.229.36` answered exactly like Kaeru for every BW2 host (`conntest` aside). Useful as a **self-hostable** reference server |
| **AltWFC** (`barronwaffles/dwc_network_server_emulator`, Python 2.7; WFCZwei instance) | The original open NWFC emulator: NAS, GP, QR2, SB, NATNEG, gamestats, storage, DLS1 | DNS `172.104.88.237` (ds-homebrew wiki). Some games need a NoSSL patch | Last push Aug 2024. Its DNS still answered on 2026-10-16. PKC says it can't guarantee compatibility with AltWFC servers |
| **Entralinked** (`kuroppoi/entralinked`, Java 17; v1.4.3, May 2026) | Standalone Game Sync / PGL emulator. Downloads Pokémon, items, C-Gear and Pokédex skins and Musicals, and in BW2 Join Avenue visitors. Also does Memory Link from a BW save file | Run it locally. It has its own DNS server and generates DS-trusted certificates. Point the DS's DNS at it | Self-hosted, not a public service. The PGL protocol research came out of PKC issue #8. Related: `alexmstern/dream-mist` (native macOS) and `kuroppoi/dreamworld-reawakened` (Flash Dream World) |
| Random Matchup patches | Party validation is SHA1 with RSA-1024 (exponent 65537), checked against a modulus hard-coded in the game. The private key is not public, and a 2026 search of the leaked repos found nothing | Isaac0-dev's AR code makes the check return 1 (White US `0x021b9a3c`); a Python patcher handles any Gen 5 ROM. `kuroppoi/WfcPatcher` swaps in a custom modulus | Both peers must be patched, since the signature is sent to the other player (PKC #116) |

**How a DS connects today.** The player edits a WFC connection slot: auto DNS off, primary DNS 178.62.43.212, and a
secondary of the same address or 1.1.1.1. The game then resolves Nintendo's hostnames through that DNS. A
dig against 178.62.43.212 on 2026-10-16 returned:

| Hosts | Resolved to |
| --- | --- |
| `nas.nintendowifi.net`, `en-ds.pokemon-gl.com`, `pkvldtprod.nintendo.co.jp` | 178.62.43.212 (the Kaeru TLS gateway) |
| `gpcm`, `natneg1`, `syachi2ds.master` and `syachi2ds.available` under `.gs.nintendowifi.net` | 95.217.77.151 (Wiimmfi) |
| `gamestats2.gs.nintendowifi.net` | 195.201.236.139 (operator not identified **(unverified)**) |
| `pkgdsprod.nintendo.co.jp` | 20.104.25.104, the same IP as `pkmnclassic.net` |

TLS works through nds-constraint. The DS SDK doesn't check `basicConstraints`, so a server certificate signed by the
Wii's non-unique "NWC" client certificate chains to a Nintendo CA, as long as the server speaks SSLv3. Because of
this, DNS alone is enough for an unpatched cartridge. The fallback when an ISP blocks custom DNS is WfcPatcher, which
rewrites the domains and HTTPS to HTTP. Emulators connect the same way. melonDS emulates an access point, through
libslirp (indirect mode) or libpcap (direct mode), and the player enters the same DNS in the game. Delta (iOS)
offers a list of community servers, and warns that players on one LAN may fail to connect to each other.

**BW2 status in summary.** Poképédia's B2W2 notes, PKC's README and the issues agree on what works and what doesn't.

- Working: the GTS, Battle Videos, Musical photos, the Battle Subway and some Mystery Gifts.
- Not working: Random Matchup without a patch, Rating, Competitions, Game Sync (except through Entralinked) and the
  online statistics.
- Not covered by any source: GTS Negotiations and the Wi-Fi Club. They use only general DWC matchmaking, so Wiimmfi
  probably handles them **(unverified)**. A melonDS issue from Oct 2024 reports friend battles working between a DS
  and an emulator.

### 4.4 Emulators and tunnelling (context)

- **melonDS** emulates the wireless chip at the 802.11 level.
  - 0.9.5 (Nov 2022) added shared-memory sync for running several instances as local multiplayer.
  - 1.0 RC (Nov 2024) and 1.0 (Jul 2025) run all instances in one process and add LAN play. Discovery is a UDP
    broadcast on port 7063 (magic `LAND`), the session runs over ENet on UDP 7064, and packets carry the magic `NIFI`.
  - The developers warn that LAN play needs a good wired network, and that VPNs, Hamachi and other tunnels are
    unsupported.
  - 1.1 (Nov 2025) fixed a multiplayer crash. Internet netplay has a scaffold in the tree (`src/net/Netplay.cpp`) but
    has not shipped.
  - Timing is the hard part. The host sends a CMD frame about every 4 ms, the hardware sends each client's REPLY in a
    contention-free slot, and an ACK follows. melonDS's sync is driven by the host.
  - The MP protocol caps a session at 16 consoles. Arisotura (Dec 2025): BW2's Funfest broadcasts beacons "into the
    void at different intervals", which her sync design can't handle.
  - On WFC, two melonDS users of BW/BW2 could only connect to each other once they used proper firmware identities
    (issue #2164).
- **DeSmuME** has no local wireless. Its WFC support has always been partial, per the GameTechWiki (seen only as a
  search snippet).
- **Tunnels.** XLink Kai lists no DS support. "Project Nitro" (2004) was abandoned. LoDS ("Locally online DS",
  2024–25 alphas) claims to relay DS local wireless over the internet; I didn't read its README **(unverified)**.
  melonDS rejects tunnel bug reports. **BW2 local wireless across the internet has no reliable solution today.**

### 4.5 How a native port could implement multiplayer

#### (a) A local-wireless shim

| Shim layer | Surface | Pros | Cons |
| --- | --- | --- | --- |
| Raw **WM** API (`WM_StartParent`/`Scan`/`Connect`/`StartMP`/`SetMPDataToPort`, `WM_SetGameInfo`, callbacks) | Large, asynchronous callback state machine | Covers everything: GFL net, MB and DWC's WDS beacon scanner (game ID 0x857) unchanged | Must fake WM's states, indications, port and sequence semantics and link levels. The most work and the most fragile |
| **WH** (`wih.c`, Game Freak's fork of the SDK's `wh.c` demo) | About 20 calls: init, parent/child connect, scan with a beacon callback, send, receivers, beacon data with GGID/TGID, measure channel | Best ratio of work to coverage. The game's beacon, Funfest and delivery paths go through `net_whpipe.c` → WH, and `mbp.c` uses WH for channel and shutdown | MB itself still needs a stub or a replacement |
| **`GFLNetDevTable`** (`net_devwireless.c`, about 50 entries) | Game-specific, highest level | Easiest delivery semantics. A new `net_devhost.c` could also take over the IR device | About 25 entries are still `unkXX`. Beacon-only features, MB and anything that calls `net_whpipe` directly sit beside it |

**Recommendation:** replace `wih.c` and the bottom of `net_whpipe.c` with a host-socket implementation, keep GFL net
and every feature unchanged, and stub MB.

Semantics to preserve:

- **Roles.** The parent (host) starts a session with a GGID/TGID, a channel and a maximum number of children; children
  scan, pick a beacon and connect by MAC. GFL net runs a star with the parent as relay (`bMPMode`), which maps
  straight onto a relay server where the parent's client owns the session. Give each install a stable fake 6-byte
  MAC: beacons and the IR handover identify peers by MAC.
- **Beacons and scanning.** A beacon is 112 bytes of user data plus GGID/TGID, sent at the beacon interval, and a
  scan returns a list of them. Model it as publish/subscribe: each client sends its current beacon to the
  relay, or to a LAN multicast group, several times a second. The relay fans it out to everyone in the same "area".
  An area can be a room code, a friend list or a public plaza; this is a design choice. The scan returns what arrived
  since the last poll. No cross-instance timing is needed. Funfest's 100 players and Tag Mode simply work, without
  the problem that limits melonDS.
- **MP timing.** On hardware the parent sends once per MP cycle, within the V-blank frame (CMD about every 4 ms), and
  children's replies are queued ahead of time and sent by the hardware. Above WM, GFL net already tolerates lost or
  late packets: queues, ring buffers, retries through WM ports, and timing negotiation in `net_handle.c`. The shim
  can deliver reliably over ENet or QUIC streams with a frame counter, and the game simply waits. Turn-based battles
  and trades should tolerate 100–200 ms RTT. Real-time bits (Union Room avatars, Xtransceiver balloon games, the
  Musical) may stutter **(unverified until tested)**. Raise the error (`net_err.c`) only after a generous timeout
  rather than on the first missed MP cycle. The data-sharing mode, if used, is lockstep and just adds a frame of
  latency.
- **Cross-play.**
  - **Real DS: no.** It needs a radio speaking Nintendo's MP 802.11 frames (monitor mode, injection, CFP timing), and
    no maintained tool exists.
  - **melonDS: theoretically.** The port would have to implement melonDS's LAN protocol (discovery, ENet, `NIFI`
    packets with emulated timestamps), synthesise raw 802.11 frames from WH-level messages, and follow melonDS's
    host-driven timing. That is very hard and would break whenever melonDS changes its protocol. Not worth it early.
- **Download Play (MB).** Replace Poké Transfer with importing a Gen 4 `.sav`, and Memory Link with importing a BW1
  `.sav`, which is what Entralinked does. Both child programs read only a cartridge save. Real-DS MB parenting is
  impossible without the radio.

Effort **(estimates)**:

- LAN-only WH shim, Union Room and trades working: **4–8 person-weeks**, given that ov030 is already decompiled.
- Relay server and internet support, presence areas and NAT-free relaying: **+3–6 weeks**.
- Save-file import replacing MB: **1–2 weeks**.

#### (b) Infrared over the network

IRC is a small point-to-point frame protocol, so either reimplement `net_devirc.c`'s table, or replace
`irc_spi.c`/`irc_link.c` with a 2-peer channel over the same relay. "Pointing the DS at each other" becomes a pairing
step: a short code, a QR or deep link, or a "nearby on LAN" list. The IR-then-wireless handover in
`net_irc_wireless.c` keeps working if pairing hands over the peer's fake MAC. Pure-IR features then work port to
port: Feeling Check, IR Mystery Gift and Live Competition registration.

Effort: **1–2 person-weeks** once (a) exists. No cross-play.

#### (c) DWC and NitroWiFi over host sockets, pointed at revival servers

Keep DWC, the GameSpy SDK code (GP, QR2, SB, NATNEG, GT2, Sake, gamestats), `dpw`, `poke_net`, NHTTP and the game's
`*_rap.c` byte-compatible, and replace only the bottom layer:

- **SOC/CPS → BSD sockets.** `docs/porting-notes.md` already lists the hazards: handles that carry pointers,
  network-order fields, the semantics of peek and SSL reads.
- **WCM/AC/NWM → "always connected".** Hide the DWC utility's access-point setup (ov182) or turn it into a server
  picker.
- **DNS.** A remap table at `SOC_GetHostByName`, or simply the host resolver plus a user-set DNS such as Kaeru's.
  Prefer an explicit table: `*.nintendowifi.net`, `pkvldtprod`, `pkgdsprod` and `en-ds.pokemon-gl.com` mapped to the
  chosen services, with Entralinked as an optional PGL target.
- **SSL.** The simplest compatible choice is to port NitroWiFi's own SSL (SSLv3, RC4, MD5/SHA1, the BigInt code),
  whose C is portable. With it the port talks to Kaeru's gateway exactly as a DS does. Modern TLS libraries refuse
  SSLv3/RC4 and would reject the nds-constraint chain. Alternatives:
  - Plain HTTP where the server allows it (Wiimmfi's patched-ROM mode).
  - Pinning the gateway's certificate.
  - Real TLS 1.3, only for new port-specific endpoints.

  Don't embed Nintendo's or the Wii's client key: the port controls its own trust store and doesn't need it.
- **Identity.** DWC keeps the WFC user ID and friend-code material in the DS firmware's NVRAM (`lib/dwc/src/bm`,
  `dwc_nvram.c`) and logs in with the MAC and console fields. The port must create and persist a stable per-install
  identity outside the game save. melonDS #2164 shows that a bad identity breaks P2P.
- **Party validation.** For Random Matchup the port can disable or redirect the check in `nhttp_rap_evilcheck.c`
  (ov189). A real-DS opponent still rejects the port's unsigned party unless that DS runs the AR code.
- **NAT.** GT2/NATNEG P2P behaves like a DS behind a home router, including the same-LAN hairpin failures that Delta
  warns about. A port-only relay fallback is possible, but only between ports.

Pros:

- Real cross-play with DS hardware, melonDS and Delta players on Wiimmfi, Kaeru and PKC, for the GTS, Battle Videos,
  Musical photos, the Subway, Wi-Fi Club, GTS Negotiations and patched Random Matchup.
- Reuses the decompiled code as it is.

Cons:

- Depends on volunteer servers, and must not abuse them.
- Inherits the unreachable features: Rating, Competitions and PWT downloads. The port could load those from files.
- Much of DWC's matchmaking and GameSpy GP/QR2/SB/NATNEG is not yet decompiled or named in `lib/dwc`. See 4.0: only 33.6% of
  overlay 11's code is in source files, and 972 functions are still unnamed.

Effort **(estimates)**:

- Sockets, DNS and SSL bring-up: **3–6 weeks**.
- Every WFC path, end to end against PKC/Wiimmfi: **+4–8 weeks**.
- Self-hosting for testing, with WiiLink `wfc-server`, PKC and Entralinked: **about 1 week**.

#### (d) New online features (sm64coopdx-style)

The game already has the right hooks:

- `field_comm_actor.c` (ov036) draws other players as field actors. It is left over from BW1's Entralink visiting.
- `game_comm.c` runs one communication at a time and logs the players met, which feeds Join Avenue.
- `comm_player_support.c` applies a remote player's support to the next battle.
- Trades (`pokemontrade_proc`, ov194) and network battles (`btl_net.c`, ov167) are self-contained processes that GFL
  net drives.

Possible features:

- **Overworld presence.** Send zone ID, position, facing, sprite and outfit at about 10 Hz through a relay, and
  spawn or despawn remote actors per zone. It needs zone and position accessors, an actor-spawn API safe to call
  outside cutscenes, and rules for event or script zones.
- **Trade or battle anywhere.** A menu entry that starts the existing trade or battle process with a peer over the
  shim, skipping the Union Room. It needs a script or event entry point and an atomic save commit; the trade code's
  save step already exists.
- **Lobby and relay server.** Rooms, friends without friend codes, a global plaza for Tag Mode and Funfest
  beacons, optional voice. QUIC, ENet or WebSocket all work.
- **Co-op or a shared world.** Hard. The save, flags and scripts are single-player. A realistic scope is visiting,
  presence, emotes and joint battles (the existing Multi battles), not shared progress.
- **Anti-cheat.** Optional server-side legality checks (PKHeX-style), since the Gen 5 validation key is gone.

Precedent: PokeMMO runs a server-side MMO on the Black/White ROM assets (per a third-party guide)
**(unverified details)**. sm64coopdx syncs entities and events, with Lua mods. Effort: presence alone is **2–3
person-months**. "Trade or battle anywhere" is **3–6 weeks** on top of (a). Never break the original local and WFC
protocols: keep new features behind a port-only capability flag in the relay.

**Suggested order:**

1. (a) WH-level LAN shim, plus save-file import in place of MB.
2. (b) IR over the shim.
3. A relay server for internet play and the beacon plaza.
4. (c) DWC over sockets, defaulting to the Kaeru/PKC DNS, with Entralinked as an optional local target.
5. (d) New features.

### 4.6 Legal and terms of service (brief)

- **The servers are volunteer-run.** Wiimmfi, Kaeru and PKC are unofficial and unaffiliated with Nintendo/Game Freak.
  Their own rules apply: Wiimmfi has auto-bans and denies some clients. Ask before pointing a non-DS client at them,
  identify the port honestly (no spoofed console IDs) and rate-limit.
- **Don't ship the Wii key.** nds-constraint depends on the Wii's NWC client certificate and its private key, which
  is Nintendo's. A port controls its own trust and doesn't need it.
- **Anti-cheat.** Random Matchup patches deliberately bypass Nintendo's validation, so on shared servers they raise
  fairness and hacked-Pokémon issues. Make them opt-in.
- **Protocols and IP.** Reimplementing GameSpy/NWFC protocols for interoperability is what every revival does.
  Nintendo/TPC trademarks and the user-supplied ROM requirement are as for the rest of the port.
- **Privacy.** A relay or lobby server collects player names, friend lists and possibly voice or camera (the
  Xtransceiver): treat it as personal data (GDPR and similar).

### Sources

**Local evidence**

- Decompressed ROM modules: `extract/b2_us/arm9/arm9.bin` and `extract/b2_us/arm9_overlays/ov011.bin`,
  `ov189.bin`, `ov201.bin` and `ov260.bin`.
- Download Play children: `extract/b2_us/files/dl_rom/*.srl`.
- Decomp files:
  - `include/gfl/net.h`, `wih.h` and `net_whpipe.h`
  - `src/ov181/mb_*.c`
  - `lib/irc/src/irc_link.c`
  - `lib/nitrowifi/src/ssl/ssl.c`
  - `lib/poke_net/src/poke_net_ssl.c`
  - `lib/nitro/include/nitro/wm.h`
  - `src/ov332/key_system_net.c`
  - `src/ov257/ctvt_comm.c`
  - `src/system/fest_mission_beacon.c`
  - `src/ov036/field_comm_actor.c`
- DNS queries (`dig`) against 178.62.43.212, 167.235.229.36 and 172.104.88.237 on 2026-10-16.

**Feature behaviour (Bulbapedia)**

- [C-Gear](https://bulbapedia.bulbagarden.net/wiki/C-Gear)
- [Xtransceiver](https://bulbapedia.bulbagarden.net/wiki/Xtransceiver)
- [Union Room](https://bulbapedia.bulbagarden.net/wiki/Union_Room)
- [Entralink](https://bulbapedia.bulbagarden.net/wiki/Entralink)
- [Funfest Mission](https://bulbapedia.bulbagarden.net/wiki/Funfest_Mission)
- [Join Avenue](https://bulbapedia.bulbagarden.net/wiki/Join_Avenue)
- [Passerby Analytics HQ](https://bulbapedia.bulbagarden.net/wiki/Passerby_Analytics_HQ)
- [Memory Link](https://bulbapedia.bulbagarden.net/wiki/Memory_Link)
- [Unova Link](https://bulbapedia.bulbagarden.net/wiki/Unova_Link)
- [Key System](https://bulbapedia.bulbagarden.net/wiki/Key_System)
- [Poké Transfer](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9_Transfer)
- [Pokémon Dream Radar](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_Dream_Radar)
- [Game Sync](https://bulbapedia.bulbagarden.net/wiki/Game_Sync)
- [Pokémon Dream World](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_Dream_World)
- [Pokémon Global Link](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_Global_Link)
- [Mystery Gift](https://bulbapedia.bulbagarden.net/wiki/Mystery_Gift)
- [Global Trade System](https://bulbapedia.bulbagarden.net/wiki/Global_Trade_System)
- [Battle Video](https://bulbapedia.bulbagarden.net/wiki/Battle_Video)
- [Battle Subway](https://bulbapedia.bulbagarden.net/wiki/Battle_Subway)
- [Pokémon World Tournament](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_World_Tournament)
- [Pokémon Musical](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_Musical)
- Bulbapedia pages were read through the MediaWiki API.

**Memory Link**

- [PokéBase: Memory Link with Download Play](https://pokemondb.net/pokebase/99747/how-to-use-the-memory-link-to-make-ns-pokemon-appear)

**Revival services**

- Poké Classic Network: [pkmnclassic.net](https://pkmnclassic.net/), [its GTS page](https://pkmnclassic.net/gts/)
- PKC source: [mm201/pkmn-classic-framework](https://github.com/mm201/pkmn-classic-framework)
  - [its `pkvldtprod.ashx.cs`](https://github.com/mm201/pkmn-classic-framework/blob/master/gts/pkvldtprod.ashx.cs)
  - [issue #116, validation signatures](https://github.com/mm201/pkmn-classic-framework/issues/116)
  - [issue #61, Random Matchup](https://github.com/mm201/pkmn-classic-framework/issues/61)
  - [issue #8, Game Sync](https://github.com/mm201/pkmn-classic-framework/issues/8)
- Kaeru WFC: [kaeru.world/projects/wfc](https://kaeru.world/projects/wfc),
  [KaeruTeam/nds-constraint](https://github.com/KaeruTeam/nds-constraint)
- WiiLink WFC: [wfc.wiilink.ca](https://wfc.wiilink.ca/), [WiiLink24/wfc-server](https://github.com/WiiLink24/wfc-server)
  (`nas/tls.go`, `game_list.tsv`, `config_example.xml`)
- AltWFC: [barronwaffles/dwc_network_server_emulator](https://github.com/barronwaffles/dwc_network_server_emulator)
  (`altwfc.cfg`), [AdmiralCurtiss/WfcPatcher](https://github.com/AdmiralCurtiss/WfcPatcher)
- Game Sync and Dream World:
  - [kuroppoi/entralinked](https://github.com/kuroppoi/entralinked)
  - [kuroppoi/WfcPatcher](https://github.com/kuroppoi/WfcPatcher)
  - [kuroppoi/dreamworld-reawakened](https://github.com/kuroppoi/dreamworld-reawakened)
  - [alexmstern/dream-mist](https://github.com/alexmstern/dream-mist)
  - [NDSLink/pkmn-dreamland](https://github.com/NDSLink/pkmn-dreamland)
- Wiki pages:
  - [Poképédia: Poké Classic Network](https://www.pokepedia.fr/Pok%C3%A9_Classic_Network)
  - [Poképédia: Wiimmfi](https://www.pokepedia.fr/Wiimmfi)
  - [MKWiiki: Wiimmfi](https://mkwiiki.org/wiki/Wiimmfi)
  - [DS-Homebrew wiki: Wi-Fi](https://wiki.ds-homebrew.com/ds-index/wifi)
  - [rvtr wiki commit on WiiLink's DS DNS](https://git.randommeaninglesscharacters.com/rvtr/wiki/commit/75ef5c0bb36ebcb20dee2ba2cf2677fcc51b8dee)
- Wiimmfi's own site, [wiimmfi.de](https://wiimmfi.de/), was behind a Cloudflare challenge and not read.
- [Delta FAQ: online](https://faq.deltaemulator.com/using-delta/online)
- [PokéBase: Wiimmfi and Poké Classic](https://pokemondb.net/pokebase/333935/what-are-wiimmfi-and-poke-classic)

**Emulators, local wireless and tunnels**

- melonDS:
  - [1.0 RC post](https://melonds.kuribo64.net/comments.php?id=216)
  - [FAQ](https://melonds.kuribo64.net/faq.php)
  - [releases](https://github.com/melonDS-emu/melonDS/releases)
  - source [`src/net/LAN.cpp`](https://github.com/melonDS-emu/melonDS/blob/master/src/net/LAN.cpp) and
    [`MPInterface.h`](https://github.com/melonDS-emu/melonDS/blob/master/src/net/MPInterface.h)
  - issues [#2510](https://github.com/melonDS-emu/melonDS/issues/2510),
    [#1672](https://github.com/melonDS-emu/melonDS/issues/1672) and
    [#2164](https://github.com/melonDS-emu/melonDS/issues/2164)
- [Arisotura on Dolphin forums: netplay ideas](https://forums.dolphin-emu.org/Thread-asking-around-for-ideas-about-netplay)
- [libretro melonDS DS docs](https://docs.libretro.com/library/melonds_ds/)
- [BlocksDS dswifi local multiplayer docs](https://blocksds.skylyrac.net/docs/dswifi/md_documentation_2local__multiplayer__mode.html)
- [GameTechWiki: Nintendo WFC](https://emulation.gametechwiki.com/index.php/Nintendo_Wi-Fi_Connection) (search snippet
  only; the page returned 403)
- [XLink Kai (Wikipedia)](https://en.wikipedia.org/wiki/XLink_Kai)
- [LoDS](https://git.randommeaninglesscharacters.com/JSSchumacher/LoDS)

**Modern online features**

- [sm64coopdx](https://github.com/coop-deluxe/sm64coopdx)
- [Tech-Insider PokeMMO guide](https://tech-insider.org/?p=20316) (low-reliability)

## 5. ROM hacks and tools

This section covers what the Black 2/White 2 (BW2) hacking scene produces and what it edits, so that the port's mod
format can take a hacked ROM, a CTRMap project or a code-injection mod as input. Evidence levels: **[repo]** read in
the tool's source or README; **[decomp]** checked against the pokebw2 checkout; **[forum]** a first-party release post
or wiki; **[2nd]** download sites, reviews or search-result snippets only; **[inferred]** our reasoning;
**[unverified]** could not be confirmed. PokéCommunity, Reddit, romhacking.net and RetroAchievements served bot
challenges or 403s to every fetch, so claims that only those sites hold are marked [2nd] or [unverified].

### 5.0 Key findings

1. **The scene targets the same base ROMs as the decomp.** The ds-pokemon-hacking wiki's "known good" Black 2 (USA,
   Europe) SHA1 is `e51e6dfb8678a3d19dcd2a10691b96a569ca0abb`, the decomp's `b2_us` SHA1 ([decomp] `configure.py`). CTRMap,
   PMC and SwissArmyKnife only support the USA codes `IREO` (Black 2) and `IRDO` (White 2). Hacks ship as xdelta patches
   against a clean US ROM. An importer that first verifies the clean ROM against the decomp's SHA1 and then applies the
   user's xdelta reproduces exactly what players run.
2. **Most hacks are mostly data, but the data is not only in NARCs.** Encounters, trainers, species, learnsets,
   evolutions, items, moves, marts, grottos, scripts and text are all NARC members under `data/a/`. Several tables that
   hacks edit sit in overlays: the type chart (overlay 167), move tutors (overlay 36), pickup (overlay 166), evolution
   special cases (overlay 284). A NARC-only loader misses those edits.
3. **The code-injection standard is PMC, and it is White 2-first.** Code mods are relocatable ARMv5T modules (`.dll`,
   libRPM's "DLXF" RPM) that PMC links into ARM9 and overlays at runtime. Hooks are named by **swan symbol names**
   (`THUMB_BRANCH_BagSave_AddItem`). The decomp imports swan's names, and its Black 2 configs name 29,538 of 43,402
   functions ([decomp]), so a source-level hook API keyed by swan names is plausible. Binary DLLs are ARM machine
   code, so a native port can only rebuild them from source or reimplement them.
4. **The biggest engine-changing projects are code mods.** White 2 Upgrade (Pokédex to 721, Fairy type) is a single PMC
   DLL. The Pokeweb editor bundles about 30 PMC DLLs (level caps, following Pokémon, instant text, IV/EV viewer, battle
   type HUD, animated sprites). Blaze Black 2/Volt White 2 Redux adds the Fairy type by patching code. These features
   would have to be native port features or source-ported mods.

### 5.1 Major BW2 hacks

| Hack | Author(s) | Latest known version | Base | What it changes | Data vs code | Distribution | Tooling | Evidence |
|---|---|---|---|---|---|---|---|---|
| **Blaze Black 2 / Volt White 2** | Drayano60 | 1.1 (11 Nov 2012). A community v1.3 by DeadSkullzJr (25 Sep 2025, Slowking fix) is reported [2nd] | Clean B2 / W2, "WITHOUT anti-piracy patches" | All 649 Pokémon obtainable; every area's encounters; trainer rosters (Gym Leaders 3–5 Pokémon, 6 in Challenge Mode); learnsets, abilities, TM/tutor compatibility, base stats (a "Vanilla" patch leaves species data alone); Pledge moves 100 BP, Cut becomes Grass; trade evolutions by item; new Ore items; TMs moved earlier, Sky Drop/Quash/Struggle Bug replaced by Hurricane/Sucker Punch/Bug Buzz; marts, trades (better IVs), Hidden Grottos, new events and legendary encounters, text | Data: NARCs (personal, learnsets, evolutions, moves, items, trainers, encounters, scripts, text). Moving TMs and changing item effects may touch ARM9 tables [inferred]. The 1.1 changelog bundles RetroGameFan's anti-piracy patch, which is a code patch | Patches (a commenter names `Complete.xdelta`), "Complete" and "Vanilla" variants, PDF docs | NitroExplorer, Lua scripts and RAM research by Kaphotics, pichu2001's scripting [forum] | [forum] Project Pokémon thread |
| **Blaze Black 2 Redux / Volt White 2 Redux** | AphexCubed, with Drayano60 | 1.4.1 (26 Mar 2022). A 2.0.0 is "announced" per secondary pages [unverified] | Clean B2 / W2 US. Must not be applied to the original BB2/VW2 | **Fairy type**, Gen VI Steel; Gen VIII base stats; type and ability changes (Dream World abilities moved into normal slots); learnsets rebuilt up to Gen VIII; later-gen moves replace old ones (Power-Up Punch for Comet Punch, Dual Wingbeat for Vice Grip, Lunge for Barrage); "Link Cable" item and held items usable like stones; level curve; shiny rate 1/512; fossils earlier; Easy/Normal/Challenge modes; trainer levels scale with badges, "up to +7"; wild held items as in SwSh; a "Classic" variant | **Mixed.** Fairy is a code patch (the readme warns it "can fail to run on original hardware"). The shiny rate is a code constant [inferred]. The Steel change edits the type chart in overlay 167. New move behaviours need battle code or effect reuse [inferred]. The rest is NARC data. How it implements Fairy, badge scaling and modes is [unverified]. Frost's editor says Redux ROMs need "Fairy" added to type-name text file 398 | xdelta, applied with the bundled xdeltaUI [2nd: Scribd guide] | Not documented. The FAQ says AphexCubed did most of the work and Drayano mostly text [2nd] | [forum] pastebin readme and changelog by AphexCubed, Notion docs (did not render) |
| **White 2 Upgrade** (`ds-pokemon-hacking/White2Upgrade`) | PlatinumMaster, Dararo, BluRose, SpagoAsparago and others | WIP, active (pushed 2026-10-04) | White 2 US (`IRDO`) only | National Dex to 721, Fairy type; goals are Gen 6+ moves, abilities and modern battle mechanics | **Code-heavy.** Everything is one PMC DLL, `romfs/patches/White2Upgrade.dll`, plus NARCs rebuilt from YAML (`data/encounters/a127_*.yml`, built with knarc) | Built from source with a Makefile (arm-none-eabi-gcc, CTRMap.jar, knarc, nitrogfx, blz, ndstool) | PMC, swan, CTRMap; based on KazoWAR's Project 721; Fairy from MeroMero's research | [repo] |
| **Project 721** | KazoWAR | Beta 1 (Nov 2014). Thread active to Aug 2023 | Clean Black 2 | Species expanded to 721 (sprites from Showdown, data from ORAS) as a proof of concept. Gen 6 abilities do nothing, no Fairy in the type chart, new species only via forced encounters | **Code by Action Replay**: a required AR code patches opcodes at runtime, plus expanded NARCs (text, sprites, icons, personal, learnsets, evolutions, moves, footprints) | `721_beta1.zip` with a patcher (format [unverified]) | enc.lua, BWSE, DS-Scene Rom Tool | [forum] |
| **Black 2 Kaizo** | Insipid Axiom [2nd] | 1.6 (31 Oct 2022) [2nd] | Black 2 | Every battle is a Double Battle; about 450 Pokémon available; all 250+ trainer parties documented; damage calculator | Data (trainers, encounters, species) [inferred]. Forcing every trainer battle to doubles may be a trainer-data flag (a/0/9/1 battle type) [inferred] | Patch (format [unverified]) | [unverified] | [2nd] PokéCommunity snippet, PokeHarbor |
| **Moon Black 2** | JrFort, with Aster [2nd] | Beta 4.2.3 (22 Jun 2023) [2nd] | Black 2 (U) | Gen 7 Pokémon, Alolan forms, Sun/Moon characters, Fairy type, Megas "outside of battle" | Necessarily mixed (Fairy type, species expansion) [inferred]. Implementation [unverified] | Patch on a clean B2 [2nd] | [unverified] | [2nd] |
| **Faster Black 2/White 2** | SilverstarStream | ~2022 | B2/W2 (US = EU ROM) | Cuts cutscenes through the first two gyms, for Nuzlocke/IronMON runs | Scripts and events (a/0/5/6, a/1/2/6) [inferred] | Patch | CTRMap (credited to Hello007) | [2nd] PokéCommunity snippet |
| **Pokémon PWT** | RobbieRaymond69420 | "Complete" (~2024) | Black 2 (USA, Europe) (NDSi Enhanced), CRC32 `F8485310` | Pokémon World Tournament-centred hack | [unverified] | Patch | [unverified] | [2nd] |
| **Complete Unova Pokédex Edition** | [unverified] | 1.12 (2015) | B2/W2 | Completes the Unova Dex without trading | Data (encounters, trades) [inferred] | Patch (romhacking.net #8981) | [unverified] | [2nd] |
| Small hacks and mods | various | 2012–2024 | B2/W2 | English and Spanish translations of the Japanese release (2012, DeltaPatcherLite/xdeltaUI); Cynthia hero-sprite replacer; Curtis/Yancy swap; Humilau Resort escape-room hack; Moémon B2/W2 "Vanilla+" (Oct 2024, [2nd]) | Text, graphics, scripts | xdelta | Tinke, PPTXT, KazoWAR's tools | [forum] Project Pokémon tag list; [2nd] |

**Names from the brief that could not be verified as Gen 5 hacks:** "Ancient Black 2", "Vortex Black / Zeta White",
"Pokémon Tempo", "Black 2/White 2 Unlimited", a standalone "Unova Redux", and "White 2 Redux" apart from the Volt White
2 Redux above. Searches found nothing reliable. "Sterling Silver" is not a Gen 5 project [unverified, likely a
HeartGold/SoulSilver hack]. A "Black2Upgrade.dll" monolith is referenced (and excluded) by Pokeweb's patch manifest, so
a Black 2 counterpart of White 2 Upgrade exists somewhere [repo reference only; source and status unverified].

**Reusable TMs** need no patch in BW2: Gen V made TMs reusable. The vanilla game also has the Exp. Share, Challenge Mode
and the physical/special split. No verified BW2 hack adds a Gen 6-style party Exp. Share. Pokeweb ships "Hard level
caps" and "Infinite Rare Candy" as PMC modules (5.3).

### 5.2 Tools

| Tool | Link | Status (checked 2026-10-16) | Formats / NARCs edited | Code patches it applies |
|---|---|---|---|---|
| **CTRMap-CE** (core) | github.com/ds-pokemon-hacking/CTRMap-CE | Active. Java, GPL-3.0 with a non-commercial "CFRU clause" request. Release `2026/04/28/1`. Windows/Linux; the wiki says **macOS is not supported** | Project VFS over an extracted ROM (5.5); CreativeStudio model editor (DAE/SMD/OBJ to NSBMD, NSBTA/NSBCA) | Hosts the code-injection suite (via CTRMapV) |
| **CTRMapV** (Gen V plug-in) | github.com/ds-pokemon-hacking/CTRMapV | Active, 1.4.1 (2026-04-28). Supports IRAO/IRBO/IRDO/IREO (USA only). Features that use tables in the executables are White 2-only (Project Pokémon listing, 2023) | Zone headers (a/0/1/2: area, matrix, scripts, text, encounters, entities, camera bounds, NPC cache, battle BG, BGM, fly point); zone entities (a/1/2/6: proxies/BG events, NPCs, warps, triggers, init scripts); buildings and prop bundles (likely a/2/2/5, a/2/2/6, textures a/1/7/4–5 [inferred]); dynamic cameras; scripts (a/0/5/6) via PokéScript, raw script replace; text (a/0/0/2, a/0/0/3); sound and script dumpers; SwanComposer (edits swan's `root.swandb`) | "Install/Update PMC" (patches ARM9 and adds overlay 344); "Convert ELF to DLL" (links against an ESDB, sets priority); bulk upgrade of DLLs in `patches/` and `lib/`; a legacy "OvlPatchSystem" with an `OvlLoader.rpm` and an "ARM9-NoDecompress" RPM (from source strings, workflow [unverified]) |
| **PMC** | github.com/ds-pokemon-hacking/PMC | Active, 13.2.4 (2026-03-01). C++, GPL-3.0. Builds `PMC_B2.rpm` / `PMC_W2.rpm` | `/patches/*.dll` and `/lib/*.dll` in the NDS filesystem | The loader itself (5.6) |
| **libRPM / RPMAuthoringTools / NitroKernel / ExtLib** | github.com/HelloOO7/{libRPM, RPMAuthoringTools, NitroKernel, ExtLib} | Active (libRPM 2026-03, RPMTool 2026-04, NitroKernel 2025-06, ExtLib 2025-07) | RPM/DLL module format; ESDB YAML (MAP2ESDB, RPMTool, RPMDump) | Runtime linker; NitroKernel adds `k::dll::LoadLibrary`, a heap, `kThread` threads and DeSmuME debug prints |
| **swan** | github.com/ds-pokemon-hacking/swan | Last push 2025-07-03 (the decomp imports revision `4324f73` of that date). GPL-3.0 | C/C++ headers generated from `root.swandb`; ESDBs `IRDO.yml` (White 2: **57,920 symbols**, ~9.3k not IDA auto-names) and `IREO.yml` (Black 2: **984 symbols**, ~260 named); counts by prefix heuristic | None (headers and symbols for mods) |
| **ESDB-Tools** | github.com/ds-pokemon-hacking/ESDB-Tools | 2025-09 | ESDBMerge for ESDB YAML | None |
| **PokéScript** + **PokeScriptSDK5** | github.com/ds-pokemon-hacking/PokeScript, …/PokeScriptSDK5 | Active (2026-04) | High-level script language, compiled by CTRMap to vanilla field-script bytecode (a/0/5/6). The SDK holds command DBs: `yml/B2W2/Base.yml` plus `Overlay 50.yml` to `Overlay 68.yml`, matching the decomp's script-plugin overlays 50–68 (commands ≥1000) | CTRMap can bind custom command DBs ("script plugin" overlays) for injected commands [repo log strings] |
| **SwissArmyKnife** (SAK) | github.com/PlatinumMaster/SwissArmyKnife | Stale (last push 2023-11). C# and Avalonia, cross-platform | Map containers, text, event and init scripts, zone headers, zone entities, wild and trainer encounters. Uses **Hotswap**, a project-based engine that keeps a base ROM plus swapped files | None |
| **Beater suite** (BeaterScript, BeaterText, BeaterFurniture, BeaterLibrary; older CheapScript/CheapFurniture) | github.com/PlatinumMaster/* | Archival (2020–2023) | Script compile/decompile (Gen V and HGSS), text, overworld furniture/NPC/warp/trigger | None. **BeaterScript is PlatinumMaster's, not Kaphotics'** |
| **Hotswap** | github.com/PlatinumMaster/Hotswap | 2023-05 | "A faster way to modify Nintendo DS(i) ROMs": a base ROM plus serialized changed files | None. ("Hotspot" in the brief is probably this; nothing named Hotspot was found) |
| **Frost's Gen 5 Editor** | github.com/FrostFalcon/FrostsGen5Editor (also on Project Pokémon) | Active, 5.0.1 (2026-03-05). C#/WinForms. Full BW2, most of BW1 | Text (a/0/0/2, a/0/0/3), sprites and icons (a/0/0/4, a/0/0/7), map files and matrices (a/0/0/8, a/0/0/9), zone data (a/0/1/2), personal (a/0/1/6), EXP curves (a/0/1/7), learnsets (a/0/1/8), evolutions (a/0/1/9), babies (a/0/2/0), moves (a/0/2/1), items (a/0/2/4), scripts (a/0/5/6), move animations (a/0/6/5–6), trainer text (a/0/8/9, a/0/9/0), trainers (a/0/9/1, a/0/9/2), egg moves (a/1/2/4), overworlds (a/1/2/6), encounters (a/1/2/7), AI (a/1/6/9), grottos (a/2/7/3), marts (a/2/8/2–3), habitat list (a/2/9/6) | An overlay editor; "custom game modes" (rogue/test modes); its own patch format: `.pokepatch` stores ARM9, y9 and overlay diffs plus **per-NARC-member** add/change/remove entries and can merge patches; there is also a `.Vpatch`. Says it is compatible with other hacks **except White 2 Upgrade** |
| **Pokeweb** / **Pokeweb-Serverless** | github.com/hzla/Pokeweb (deprecated) and github.com/hzla/Pokeweb-Serverless | Serverless (TypeScript, browser) is very active (pushed 2026-10-16). US English ROMs | Headers, overworlds, 3D map viewer and permissions, personal, learnsets, evolutions, TM/tutor, egg moves, sprites and animations (PWAN, White2Upgrade's w2anim), trainers, battle facilities, encounters, statics, grottos, moves, move animations and SPA particles, items, TMs, the **type chart (with Fairy)**, text; NARC diff changelog generator; Dynamic Calc export | **Bundles about 30 PMC DLLs**: level caps, infinite Rare Candy, following Pokémon (W2, B2, W2U, Italian W2), instant text, IV/EV summary, battle type icons and effectiveness preview, party quick actions, portable PC, BGM toggle and streamed BGM, trainer natures, PWAN animated sprites, battle log, map autosave, double-battle fix, form evolution, custom weather, Black2Upgrade companions, debug helpers. Per-patch source provenance is in `patch-sources/manifest.json` |
| **Universal Pokémon Randomizer ZX** | github.com/Ajarmar/universal-pokemon-randomizer-zx | Last push 2024-11 | NARCs listed per game in `gen5_offsets.ini` (B2: a/0/0/2, a/0/0/3, a/0/1/2, a/0/1/6, a/0/1/8, a/0/1/9, a/0/2/0, a/0/2/4, a/0/5/6, a/0/8/9, a/0/9/1, a/0/9/2, a/1/2/4, a/1/2/6, a/1/2/7, a/1/6/3, a/1/7/6, a/2/0/2, a/2/7/3, a/2/8/2); starters and statics by script offsets; move tutors at overlay 36 offset 0x51538 | **IPS tweaks to ARM9 and overlays**: instant text, a music index fix (ARM9 and overlay 36), a Shedinja evolution fix (overlay 284), National Dex at start (`bw2_national_dex.ips`) |
| **PokEditor** | github.com/turtleisaac/PokEditor-v1 / -v2 / PokEditor-Core | v1 (CLI, archived) covered **Gen 4 and 5**. v2/v3 are **Gen 4 only** | Spreadsheet-driven species, learnset and trainer data (v1) | None |
| **DSPRE Reloaded** | github.com/DS-Pokemon-Rom-Editor/DSPRE | Active, but **Gen 4 only** | — | — |
| **PPRE** | github.com/projectpokemon/ppre | Dead (2017). D/P/Pt; **no Gen 5** | — | — |
| **Pokémon DS Map Studio** (Trifindo) | github.com/Trifindo/Pokemon-DS-Map-Studio | Active (2026-10). Java 8; Windows/macOS/Linux | Builds **new** Gen 4/5 maps (BW/B2W2 export) from a tile interface; does not import or edit original maps; NSBMD export needs Nintendo's `g3dcvtr.exe` | None |
| **SDSME** | (forks only) | Abandoned. Gen 5 map headers never worked | — | — |
| **NitroPaint** | github.com/Garhoogin/NitroPaint | Active (2026-10) | NCGR/NCLR/NSCR/NCER/NANR, textures | None |
| **Tinke / TinkeDSi** | github.com/R-YaTian/TinkeDSi | Active (2026-07). The wiki recommends TinkeDSi 0.9.3 to **preserve the DSi-enhanced header** | Generic NDS filesystem, NARC repack, file replace | None (used to insert others' patched overlays) |
| **Nitro Studio 2 Deluxe** | github.com/PlatinumMaster/NitroStudio2Deluxe (fork by Lonk12) | 2022 / 2026 | SDAT (`swan_sound_data.sdat`) | None |
| **KazoWAR's tools**: BWTE (trainers; Kaphotics added B2W2 [2nd]), BWSE (sprites), BWOE (overworlds), PPTXT (text) | Project Pokémon file #2090 (BWTE); others on old threads | Legacy, Windows-only, ~2011–2014. PPTXT's authorship and B2W2 support [unverified] | Trainers a/0/9/1–2, sprites a/0/0/4, overworld graphics, text a/0/0/2–3 | None |
| **Kaphotics / PKHeX** | github.com/kwsch/PKHeX | Very active | Saves, not ROMs: B2W2 saves are 0x80000 bytes (`SAV5B2W2`: Pokéstar at 0x77000, PWT at 0x7A000, key data at 0x7DC00, CGear at 0x52800). Project Pokémon has B2W2 save-structure research (2024) | None |
| **Smaller Gen 5 tools** | Project Pokémon ROM-editing category | Various, 2023–2026 | Raven Dex Editor (BW/BW2), FrostsBW2Editor, LuStarterEditBW (starters; github.com/xhydrochaeris/LuStarterEditBW), text-NARC export/import, Data.bin/wc5 injector, pypokescript (github.com/vgmoose/pypokescript, B2W2 script conversion), BluRose's `b2w2-fairy` (below) | b2w2-fairy patches code |
| **BluRose's b2w2-fairy** | github.com/BluRosie/b2w2-fairy, wiki guide | 2022–2024 | Graphics: a/0/8/2, a/1/2/5 (type icons), a/0/1/1 (adds member 572, a move-selection palette), a/2/1/3 (Hall of Fame palette) | **Assembly (`asm/fairy.s`, applied by a Windows batch script)** patching ARM9, `y9.bin` and overlays 167 (type chart), 168, 207, 255, 265, 296 and 298, then reinserted with Tinke. Built on MeroMero's research |

Not found or not Gen 5: "BW2 Script Editor", "BWScriptEditor", "Pokémon Gen 5 script compiler", "VS Maker",
"EnhancedPkmnEditor", "BWHack", and any Mikelan98 Gen 5 tool [unverified]. The Gen 5 script compiler by HelloOO7
(Hello007) is PokéScript inside CTRMap.

### 5.3 File formats and NARC paths

All of the paths below are confirmed by at least two of: the decomp's `docs/data.md` (packs each archive and checks it
against both ROMs) [decomp], UPR ZX's `gen5_offsets.ini` [repo], and Frost's `VersionConstants.cs` [repo]. **They are
for BW2.** BW1 is shifted for some of them (BW1 scripts a/0/5/7, trainers a/0/9/2–3, encounters a/1/2/6, egg moves
a/1/2/3), so a loader must key archive roles off the game code.

| Data | BW2 path | Notes |
|---|---|---|
| System text / story text | a/0/0/2 / a/0/0/3 | Encrypted per-entry message files. Fixed bank numbers per role in a/0/0/2: species names 90, ability names 374, trainer names 382, trainer classes 383, type names **398** (Fairy hacks append to it), move names 403, move descriptions 402, item names 64, zone names 109 |
| Battle sprites / icons | a/0/0/4 / a/0/0/7 | Nitro 2D (NCGR/NCLR/NCER/NANR) |
| Map models / map matrices | a/0/0/8 / a/0/0/9 | NSBMD plus collision ("map containers") / matrix grids |
| Type-icon palettes etc. | a/0/1/1, a/0/8/2, a/1/2/5, a/2/1/3 | Touched by Fairy patches |
| Zone headers | a/0/1/2 | One record per zone (decomp: `data/zones/`) |
| Area records / map textures | a/0/1/3 / a/0/1/4 | |
| Personal (species) / EXP tables | a/0/1/6 / a/0/1/7 | 0x4C bytes per species (`PML_PersonalGetParam`) |
| Learnsets / evolutions / babies | a/0/1/8 / a/0/1/9 / a/0/2/0 | |
| Moves / items | a/0/2/1 / a/0/2/4 | 0x24 bytes per move; 36 bytes `ItemData` |
| Field scripts | a/0/5/6 | VM bytecode. Commands ≥1000 come from script-plugin overlays 50–68 |
| Move animations | a/0/6/5, a/0/6/6 | |
| Trainer messages / text table | a/0/8/9 / a/0/9/0 | |
| Trainers / parties | a/0/9/1 / a/0/9/2 | |
| Egg moves | a/1/2/4 | |
| Zone entities (events) | a/1/2/6 | BG events, NPCs, warps, triggers, init scripts (UPR calls it "MapFiles") |
| Wild encounters | a/1/2/7 | Version-specific between B2 and W2 |
| In-game trades / Dex area data | a/1/6/3 / a/1/7/6 | |
| Trainer AI | a/1/6/9 | 14 scripts, one per AI flag |
| Field props / prop textures | a/2/2/5–6 / a/1/7/4–5 | |
| Hidden Grottos / marts / mart counts / habitat list | a/2/7/3 / a/2/8/2 / a/2/8/3 / a/2/9/6 | |
| Sound | `swan_sound_data.sdat` | |
| **Tables inside code** (no NARC) | Overlay 167 (type chart, `gTypeEffectiveness[atk][def]`, values 0/2/4/8), overlay 36 (move tutors), overlay 166 (pickup), overlay 168 (move-type to palette), overlay 284 (evolution code), ARM9 | Hacks that edit these change overlay binaries. CTRMap edits them only for White 2 |

**Distribution.** Hacks ship as **xdelta** patches (xdeltaUI, DeltaPatcher, RomPatcher.js) against a clean, unmodified
US ROM. Some older ones require that it has no anti-piracy patch, and some patch sets apply in stages. Frost's editor adds
`.pokepatch`, which is NARC-member-granular and mergeable. Project 721 used an AR code. Code mods ship as PMC
`.dll` files dropped into a CTRMap project's `vfs/data/patches`, or as a ROM pre-built by Pokeweb or White 2 Upgrade's
Makefile. Full pre-patched `.nds` files circulate only on download sites. Hack authors don't distribute ROMs.

### 5.4 Which hack features are code (a port must reimplement them)

| Feature | Seen in | Implementation | Port implication |
|---|---|---|---|
| Fairy type (19th type) | BB2/VW2 Redux, Moon Black 2, White 2 Upgrade | b2w2-fairy asm (ARM9 and overlays 167, 168, 207, 255, 265, 296, 298) or PMC (W2U); graphics in a/0/1/1, a/0/8/2, a/1/2/5, a/2/1/3; text 398 | Make `NUM_TYPES` and the type chart data-driven in the port (type chart from the mod, type icons from NARCs). Then data-only import of Fairy hacks works |
| National Dex past 649 / new species | White 2 Upgrade (721), Project 721, Moon Black 2 | PMC DLL / AR code plus expanded NARCs | Remove the 649 limits in the port (save format, Dex, sprite and cry tables); the NARC data then imports. Save layout changes need care |
| New move effects / abilities | Redux (later-gen moves), W2U goals | Code (battle overlays) [inferred for Redux] | Needs a native mechanism for new effects and abilities (scriptable hooks or C plugins); not portable from ARM |
| Shiny rate, level scaling, game modes | Redux (1/512, badge scaling, Easy/Challenge) | Code constants or patches [inferred/unverified] | Expose as port options or mod-config keys |
| Level caps, infinite Rare Candy, instant text, IV/EV viewer, type-effectiveness HUD, following Pokémon, autosave, portable PC, BGM toggle | Pokeweb's PMC bundle, UPR ZX instant text | PMC DLLs / IPS | Natural built-in port features (QoL settings) rather than mods |
| Trade evolutions by item, new evolution items | BB2/VW2, Redux | Evolution data (a/0/1/9) plus item data; Shedinja-style special cases are code in overlay 284 | Mostly data; special cases are port code |
| Anti-piracy removal | BB2/VW2 1.1 | Code patch | Irrelevant: the port has no anti-piracy (the decomp's `lib/dsprot` can be stubbed) |
| Custom script commands | CTRMap script plugins, W2U | PMC plus command DB | The port's script VM needs an extension point (command IDs past the vanilla range) |

### 5.5 CTRMap project format

From `CTRMapProject.java` and `WSFS.java` [repo]:

- `<project>/<project>.cmproj` is **YAML**, with keys `UserDataPath` (`user`), `VFSBase` (absolute or relative path to
  the **extracted base game**, the NDS file tree with `data/`, `overlay/`, `arm9.bin`, `y9.bin`), `VFSOverlay`
  (`vfs`), `Playtime` and `BackupAndRestore`.
- `<project>/vfs/` is an **overlay filesystem** that mirrors the NDS tree and holds only changed or added files. When an
  editor touches a NARC member, CTRMap copies it into the OvFS under the archive's path (`<arc path>/<member>`, with
  index-based member names [inferred, exact naming unverified]). Code mods live in `vfs/data/patches/*.dll` and
  `vfs/data/lib/*.dll`.
- `<project>/user/` holds editor state (logs, a VFS change blacklist); `<project>/backup/` holds backups. PokéScript
  sources for zones opened "in IDE" are kept in the project [unverified location].
- "Export ROM" merges base and OvFS into an `.nds`.

For a port, a CTRMap project is the cleanest mod input there is: a sparse overlay of the ROM filesystem. Mount `vfs/`
over the base ROM's filesystem in the port's file layer, and read NARC members either from repacked NARCs or from
CTRMap's member files.

### 5.6 How Gen 5 code injection works (PMC / RPM)

From the PMC, libRPM and CTRMapV sources and the ds-pokemon-hacking guides [repo]:

- **Install.** CTRMap writes `PMC_B2.rpm`/`PMC_W2.rpm` into the ROM. Boot code goes into unused secure-area space at
  `0x0200400C` (`FULL_COPY_ARM9_0x0200400C_PMCBootCode`). `main+0x6` is redirected there instead of `GFLAppInit`; the
  boot code calls `GFLAppInit`, loads the new **overlay 344** (PMC itself; retail ends at 343 [decomp]) and runs
  `_PMCSystemInit`. PMC also moves the user heap start (`GetUserMemRegionDefaultStart+0x7C` becomes `0x022050C0`) and
  uncaps the overlay limit in `sys_read_overlay_header`. A successful install leaves `00` at RAM `0x02005050`.
- **Runtime.** `THUMB_BRANCH_LINK_GFL_OvlLoad_0x76` and `..._GFL_OvlEntryUnload_0xA` hook overlay load and unload. PMC
  scans `/patches`, and whenever an overlay that a module targets is loaded it relocates that module's hooks into it.
  Modules are linked into ARM9 or into overlays on demand, so a patch for overlay 167 is applied every time the battle
  overlay loads. Priority defaults to 4. `NitroKernel.dll` (also in `/patches`) provides the DLL loader for `/lib`, a
  heap, threads and debug print. `malloc` must not be used; use `GFL_HeapAllocate` or ExtLib allocators.
- **Module format.** RPM ("Relocatable Program Module", libRPM, <10 KB runtime, ARMv5T and Win32 targets). A PMC
  `.dll` is an RPM container with magic `DLXF` and metadata (`PMCModulePriority`). Relocation types: `R_ARM_ABS32`,
  Thumb BL/BLX, ARM BL/BLX, Thumb B (or PUSH/BL/POP), ARM B, `FULL_COPY`, `THUMB_BRANCH_SAFESTACK`. Imports between
  modules are matched by **name hash**.
- **Authoring.** Write C/C++ with `extern "C"` against swan headers, build with `arm-none-eabi-gcc -r -march=armv5t
  [-mthumb] -Os`, and convert ELF to DLL in CTRMap against an ESDB. **Hooks are declared by symbol name:**
  `<TYPE>_<FunctionName>` (`THUMB_BRANCH_BagSave_AddItem`), `<TYPE>_<FunctionName>_0x<offset>`
  (`THUMB_BRANCH_LINK_BagSave_AddItem_0x2`), or `<TYPE>_<SEGMENT>_0x<address>` (`FULL_COPY_ARM9_0x02008268`). An
  unresolved name fails silently (it is taken as a runtime import).
- **ESDB.** YAML with `Segments:` (ID, Name: `ARM9`, `ITCM`, `OVL_<n>`…, Type) and `Symbols:` (Address with the Thumb
  bit set, Name, Segment). swan provides one for each game; Black 2's is sparse (984 entries against White 2's 57,920),
  which is why the guides say "White 2 recommended" and much research lives in White 2 IDBs on Discord.
- **Other methods.** Raw asm patches into overlays (b2w2-fairy), IPS patches to ARM9 and overlays (UPR ZX), AR codes
  (Project 721), and CTRMap's legacy overlay-344 loader.

**Name compatibility with the decomp [decomp].** The decomp imports swan's names (`tools/decomp/import_swan.py`,
revision `4324f73`) and carries them to the other version through its version map. The names the hooks above use exist
in the Black 2 configs: `GFLAppInit` at 0x020054B8, `BagSave_AddItem` at 0x02008268, `GFL_OvlLoad` at 0x0203CE0C.
Names swan lacks are ours (`config/names.txt`, 25,418 lines), so the decomp's name space is a superset of swan's for
both versions. A port built from the decomp can therefore offer a **symbol-level hook API with swan-compatible names**
(replace or wrap the C function `BagSave_AddItem`), and a PMC mod's *source* could in principle be recompiled
natively against it. Offsets into functions (`_0x76`) and absolute addresses have no native equivalent.

### 5.7 Interop recommendations for the port's mod format

Ranked by value against effort.

**(d) Accept xdelta-patched ROMs as input (do this first; cheap, covers nearly every released hack).**
Ask for the clean ROM, verify its SHA1 against the decomp's (`e51e6dfb…` B2, `b5d7490b…` W2), apply the user's xdelta
(a small vcdiff decoder or bundled `xdelta3`), then treat the result as case (a). Many players already have a pre-patched
`.nds`, so also accept that directly, with a game-code check (`IREO`/`IRDO`; refuse BW1 and non-US ROMs at first).

**(a) Load data from a hacked ROM's filesystem (data-only hacks).**
- Unpack the NDS filesystem and serve every file the port reads from `data/` (NARCs, SDAT) out of the user's ROM, not
  from the port's built-in copy. Because the port's asset loaders are the decomp's own NARC readers, unchanged formats
  just work. This covers BB2/VW2, most of Redux's data, Kaizo, Faster B2W2 and translations.
- **Diff ARM9 and every overlay against the clean ROM** (after BLZ decompression). If only bytes inside known data
  symbols changed (type chart in ov167, tutor table in ov36, pickup in ov166, and every other `const` table the decomp
  defines with an address per version), extract those tables by symbol and override the port's compiled-in copies.
  The decomp's symbol configs give the exact address and size for each. If code bytes changed, report "this hack
  contains code patches: features X/Y may not work" and list the changed functions by name. That report is cheap to
  build from the decomp's symbol map and doubles as a hack-compatibility checker.
- Grow beyond vanilla limits (19 types, more species, larger NARCs) by making the corresponding constants data-driven,
  so Fairy and Dex-expansion hacks load as data once their code side is a port feature (5.4).

**(b) CTRMap project import.**
Mount `<project>/vfs/` (from `.cmproj`'s `VFSOverlay`) over the base filesystem named by `VFSBase`, or over the user's
clean ROM. Every non-code CTRMap edit then loads unchanged. Detect `vfs/data/patches/*.dll` and route them to (c).
Scripts compiled by PokéScript are vanilla bytecode for the decomp's VM; custom commands ≥1000 beyond the plugin
overlays need a port-side extension table. Use PokeScriptSDK5's YAML command DBs to cross-check the port's command
table names (the decomp names commands after swan's handlers too).

**(c) PMC/RPM code-injection mods.**
- **Binary compatibility is out of reach** without an ARM interpreter and a faithful DS memory map: DLLs are ARMv5T or
  Thumb code with relocations against DS addresses (ESDB) and NitroSDK/GFL calling conventions. Embedding an ARM
  interpreter only for mods would have to emulate the GFL heap, overlays and structure layouts too. Not recommended.
- **Source compatibility is feasible and attractive.** Offer a native hook API whose names are the swan/decomp symbol
  names: `HOOK_REPLACE(BagSave_AddItem, my_fn)`, `HOOK_WRAP`, plus "call original". Ship swan-compatible headers
  generated from the decomp's own headers. A mod written as `THUMB_BRANCH_<Func>` could be machine-translated to
  `HOOK_REPLACE(<Func>)`; `THUMB_BRANCH_LINK_<Func>_0x<off>` (one call site) needs a manual port to a named callee.
  Mods compile as host shared libraries (`.dylib`/`.so`/`.dll`) or, for safety and portability, as WASM or a
  scripting layer. Struct layouts on 64-bit hosts differ from swan's 32-bit ones (see `docs/porting-notes.md`), so mods
  must use the port's headers, not swan's.
- **Reimplement the popular ones natively** instead of supporting their binaries: the Pokeweb QoL set (level caps,
  instant text, IV/EV view, effectiveness HUD, following Pokémon, autosave, portable PC, BGM toggle) and the W2U and
  Redux engine features (Fairy, Dex expansion, new effects).
- The White 2 skew of the ecosystem (ESDB, CTRMap executable-table features, many W2-only DLLs) does not matter to a
  port built from shared B2/W2 source: a name-based API serves both versions.

**Suggested mod package shape** [inferred]: a directory or zip with `mod.toml` (name, version, base game
`IREO`/`IRDO`, dependencies, load order); `fs/` as a sparse overlay of the NDS tree (CTRMap's `vfs/` drops in as-is);
`tables/` as JSON overrides keyed by decomp symbol name (type chart, tutor lists, the decomp's own `data/*.json` schemas
for species, moves, trainers and encounters); `code/` as native hook libraries against the port's API; and an optional
`source.xdelta` that the installer turns into `fs/` and `tables/` with the overlay diff above. Merging per NARC member
(as Frost's `.pokepatch` does) lets independent mods stack where whole-ROM xdelta patches cannot.

### Sources

- Drayano, *Pokémon Blaze Black 2 & Volt White 2* thread: https://projectpokemon.org/home/forums/topic/24126-pokémon-blaze-black-2-pokémon-volt-white-2/
- AphexCubed, Redux readme (v1.4.1): https://pastebin.com/BSR4TWYk; Redux "Pokémon Changes (v1.4.1)": https://pastebin.com/ZZtDTfmU
- Redux docs (Notion, did not render): https://pokeromcodex.notion.site/Blaze-Black-2-Redux-Volt-White-2-Redux-aab47c9b319940fd8eae7651511340e5
- Redux patching guide (secondary): https://www.scribd.com/document/639142204/Patching-Guide; Redux FAQ (secondary): https://www.scribd.com/document/601228606/Redux-FAQ
- KazoWAR, Project 721: https://projectpokemon.org/home/forums/topic/33493-project-721/
- Black 2 Kaizo (thread, 403 to fetch): https://www.pokecommunity.com/threads/official-black-2-kaizo-public-release.485328/; listing: https://www.pokeharbor.com/2022/11/pokemon-black-2-kaizo/
- Moon Black 2 (secondary): https://www.pokemoncoders.com/pokemon-moon-black-2/
- Faster Black 2/White 2: https://www.pokecommunity.com/threads/faster-black-2-white-2.484001/
- Pokémon PWT: https://www.pokecommunity.com/threads/pokemon-pwt-black-2-rom-hack-complete.531823/
- Complete Unova Pokédex Edition: https://www.romhacking.net/hacks/8981/
- Project Pokémon B2W2 tag list: https://projectpokemon.org/home/tags/black%202%20/%20white%202/; ROM-editing category: https://projectpokemon.org/home/files/category/54-rom-editing
- White 2 Upgrade: https://github.com/ds-pokemon-hacking/White2Upgrade
- CTRMap-CE: https://github.com/ds-pokemon-hacking/CTRMap-CE (`src/ctrmap/editor/system/workspace/CTRMapProject.java`, `WSFS.java`); Project Pokémon listing: https://projectpokemon.org/home/files/file/4831-ctrmap-community-edition/
- CTRMapV: https://github.com/ds-pokemon-hacking/CTRMapV (`VExtrasPanel.java`, `CodeInjectionSystemGUI.java`)
- PMC: https://github.com/ds-pokemon-hacking/PMC (`Interface/PMC_AsmInterface.s`, `Interface/PMC_Interface.cpp`)
- swan: https://github.com/ds-pokemon-hacking/swan (`IRDO.yml`, `IREO.yml`, `root.swandb`); ESDB-Tools: https://github.com/ds-pokemon-hacking/ESDB-Tools
- libRPM: https://github.com/HelloOO7/libRPM (`include/RPM_Control.h`); RPMAuthoringTools: https://github.com/HelloOO7/RPMAuthoringTools; NitroKernel: https://github.com/HelloOO7/NitroKernel; ExtLib: https://github.com/HelloOO7/ExtLib
- PokéScript: https://github.com/ds-pokemon-hacking/PokeScript; PokeScriptSDK5: https://github.com/ds-pokemon-hacking/PokeScriptSDK5
- DS Pokémon Hacking wiki, Gen V guides (sources at https://github.com/ds-pokemon-hacking/ds-pokemon-hacking.github.io): Getting Started (tools, known-good hashes), Using CTRMap, Code Injection (Generation V), Code Injection set up, Fairy Type in B2W2, Hands-on with PokéScript: https://ds-pokemon-hacking.github.io/
- BluRose, b2w2-fairy: https://github.com/BluRosie/b2w2-fairy
- Frost's Gen 5 Editor: https://github.com/FrostFalcon/FrostsGen5Editor (`Data/VersionConstants.cs`, `Data/PatchingSystem.cs`, `Forms/Pokepatcher.cs`); https://projectpokemon.org/home/files/file/5689-frosts-gen-5-editor/
- Pokeweb: https://github.com/hzla/Pokeweb; Pokeweb-Serverless: https://github.com/hzla/Pokeweb-Serverless (`patch-sources/manifest.json`)
- SwissArmyKnife: https://github.com/PlatinumMaster/SwissArmyKnife; Hotswap: https://github.com/PlatinumMaster/Hotswap; BeaterScript and others: https://github.com/PlatinumMaster
- UPR ZX: https://github.com/Ajarmar/universal-pokemon-randomizer-zx (`config/gen5_offsets.ini`, `patches/`)
- PokEditor v1: https://github.com/turtleisaac/PokEditor-v1; v2: https://github.com/turtleisaac/PokEditor-v2
- DSPRE Reloaded: https://github.com/DS-Pokemon-Rom-Editor/DSPRE; PPRE: https://github.com/projectpokemon/ppre and https://projectpokemon.org/editing/ppre.html
- Pokémon DS Map Studio: https://github.com/Trifindo/Pokemon-DS-Map-Studio
- NitroPaint: https://github.com/Garhoogin/NitroPaint; TinkeDSi: https://github.com/R-YaTian/TinkeDSi; Nitro Studio 2 Deluxe: https://github.com/PlatinumMaster/NitroStudio2Deluxe
- BWTE: https://projectpokemon.org/home/files/file/2090-bwb2w2-trainer-editor-bwte/; KazoWAR tool list (PokéCommunity): https://www.pokecommunity.com/showthread.php?p=9546556
- PKHeX `SAV5B2W2.cs`: https://github.com/kwsch/PKHeX
- pypokescript: https://github.com/vgmoose/pypokescript; LuStarterEditBW: https://github.com/xhydrochaeris/LuStarterEditBW
- Local: pokebw2 `configure.py` (SHA1s), `docs/data.md` (NARC table), `docs/scripts.md` (script plugins, swan command names), `docs/code-organization.md` (swan import), `config/b2_us/arm9/symbols.txt`, `config/names.txt`

## 6. Recommendations: what the port must expose, and what to scope out early

These follow from sections 1–5. Decomp paths are relative to the pokebw2 checkout.

### 6.1 Four seams that already exist in the decomp

The decomp gives the port four narrow places to stand. Each mod goal leans on one or two of them.

| Seam | Where | What goes through it | Serves |
| --- | --- | --- | --- |
| **Archive and filesystem** | `src/system/gfl_use.c` (`NARC_PATH_LIST`, 308 archive paths, `GFL_ArcSysInit`), `src/gfl/arc_tool.c` / `include/gfl/arc.h` (`GFL_ArcSysReadHeapNew` and friends), all over NitroSDK FS (`FS_OpenFile`, `lib/nitro/src/fs`) | Every NARC and SDAT read the game makes | Randomizer, hacked-ROM loading, CTRMap import, all data mods |
| **Projection and 2D layout** | `GFL_G3DSysMtxSetProjection` (`src/gfl/g3d_system.c`), 23 `G3_ViewPort` sites, `FieldG3D_Init` in `src/ov036/fieldmap.c`, `MAP_CONFIGS` chunk table, GFL BG system and ClAct | The 3D camera, culling and streaming window, 2D layer placement | Widescreen |
| **Network device table** | `GFLNetDevTable` (`include/gfl/net.h`), chosen in `src/system/net_define.c`; wireless helper `src/ov030/wih.c` + `net_whpipe.c`; IR `src/ov031` + `lib/irc`; Wi-Fi `src/ov011` + `lib/dwc` + `lib/nitrowifi` | Every connected mode except Download Play | Multiplayer |
| **Named functions and script commands** | swan-compatible names across the decomp (`config/*/symbols.txt`, `config/names.txt`); script commands named after swan's handlers, plugin overlays 50–68 for commands ≥1000 (`docs/scripts.md`) | Behaviour | Hook API, PMC source compatibility, custom script commands |

### 6.2 The data layer the port must expose

1. **A layered virtual filesystem under NitroSDK FS.** Order, lowest first:
   - the user's base ROM, verified against the decomp's SHA1s (`e51e6dfb…` B2, `b5d7490b…` W2), which every hack and
     tool also targets (section 5);
   - an optional xdelta applied on import, or an already-patched `.nds` with a game-code check (`IREO`/`IRDO`);
   - mod overlays mounted as sparse NDS trees: CTRMap's `vfs/` drops in as-is;
   - per-NARC-member overrides, so independent mods stack (Frost's `.pokepatch` already works this way).
   Never bake the archives into the executable. The decomp's own NARC readers then make every data-only hack and every
   data-only randomizer change work unchanged.
2. **Packers at runtime, not only at build time.** `data/` already holds schema-validated JSON, assembly scripts,
   glTF and PNG for most archives, and `tools/data/*.py` packs them (section 2.10). Either ship these packers (or a
   C/Rust reimplementation) inside the port, so a mod can be JSON, or keep the packers as a "mod build" step. Four
   UPR targets still lack a source form and need converters first: Hidden Grottoes `a/2/7/3`, shops `a/2/8/2` and
   `a/2/8/3`, starter art `a/2/0/2`, and the Pokédex area data `a/1/7/6`, which is best regenerated from
   `data/encounters/`.
3. **A "patchable tables" registry for data that lives in code.** Hacks and UPR edit tables that a compiled port
   would otherwise ignore:
   - `TM_MOVE_LIST` and the item graphics table in `src/system/pml_item.c`;
   - the four `MOVE_TUTOR_ITEMS_*` tables in `src/ov036/scrcmd_shop.c`;
   - `sPselSpecies` in `src/ov316/psel.c`;
   - the intro species in `src/ov294/intro_script.c` and `src/ov162/game_start.c`;
   - `sPickupItems` and `sPickupRareItems` in `src/ov166/event_battle_return.c`;
   - the type chart `data_ov167_021d7500[17][17]` in `src/ov167/btl_calc.c`;
   - `EVO_FRIENDSHIP` in `src/system/shinka_check.c`;
   - Hidden Grotto entrances in `src/ov012/hidden_hollow.c`, and the overlay-284 evolution special cases.
   Key each by its decomp symbol. Give it a JSON override and an **extractor** that reads it from a user ROM's
   BLZ-decoded arm9 or overlay at the decomp's per-version address. UPR never resizes overlays (section 2.1), so this
   recovers randomized TMs, tutors, starters' cries, intro, Pickup and type chart from a UPR ROM.
4. **A code-diff report on import.** Diff arm9 and every overlay against the clean ROM. Changes confined to registry
   tables are extracted (point 3). Changes at the 16 known UPR sites (section 2.5) map to port options. Anything else
   is reported as "code patch in function X", named from the decomp's symbols, so the user knows which hack features
   need a native equivalent (Redux's Fairy type, PMC modules).
5. **Lift limits early, or decide not to.** Fairy (19 types) and Dex expansion (721+) are the most-requested engine
   changes in BW2 hacking (section 5.4). Both touch `NUM_TYPES`-sized tables, sprite/cry/personal indexing and the
   save format. That is cheap to plan before the port's save and data formats freeze, and expensive afterwards.

### 6.3 Built-in randomizer

- **Operate on the decoded model, not on bytes.** A native randomizer should read the same records `data/` describes
  (species, learnsets, evolutions, moves, items, trainers, facilities, encounters, trades, text) and emit a mod
  folder (JSON overrides plus a log, seeded and reproducible) that the VFS of 6.2 mounts. That keeps randomizer output
  inspectable, shareable and stackable with other mods.
- **Prefer runtime hooks for script-embedded content.** Statics, gifts, starters and field items are bytecode in
  `a/0/5/6` (`CallWildBattle 377, 65, 1`; item-ball scripts 1240/1241; starter script 854). UPR rewrites bytes at
  offsets. A port can instead substitute at the call: hook the script commands that create Pokémon and give items
  (`CallWildBattle`, `PokePartyAdd*`, `ItemAdd`, the item-ball and hidden-item handlers), the wild encounter
  generator, trainer party creation, the shop list load (`scrcmd_shop.c`), the starter selection (`psel.c`) and the
  TM/tutor lookups. One substitution table per seed then covers what UPR needs 30+ script offsets for, and it survives
  script edits by other mods.
- **Built-in equivalents of UPR's code tweaks.** Fast text, Force Challenge Mode, easier friendship evolutions, run
  without shoes, guaranteed catch, no low-HP music, forgettable HMs, doubles mode, static-encounter music, and the
  Shedinja case become option flags (section 2.5). Several also make sense as plain QoL options.
- **Also accept UPR-randomized ROMs.** With 6.2 points 1, 3 and 4 they load correctly, including the ITCM-injected
  music and Shedinja patches, which the import report recognises by their bytes.
- **Gaps to decide.** No randomizer touches Join Avenue, Dream Radar, Pokéstar Studios, Medals, Funfest Missions or
  the Key System (section 2.4). A built-in randomizer could, because the decomp has their code and save layouts.
  Those are good stretch features, not launch features.

### 6.4 Widescreen

- **Pick the renderer architecture first.** The widescreen research (section 3.5) argues for a hybrid: a hardware
  model of the DS 2D/3D engines for fidelity and as a regression oracle, plus HLE hooks at the GFL layer
  (projection, viewport, ClAct surfaces, BG policy, capture size). A pure hardware model fights widescreen at every
  9-bit OAM X and 8-bit window register.
- **Expose these hooks** (section 3.6):
  - projection override with Hor+ for perspective cameras;
  - viewport and world-to-screen consistency;
  - box-test culling against the widened clip volume;
  - per-map-type `MAP_CONFIGS` overrides and a larger field heap;
  - camera-limit relaxation;
  - per-scene and per-layer 2D policy (centre, stretch, wrap, extend) with edge anchors in the style of sm64's
    `GFX_DIMENSIONS_FROM_*_EDGE`;
  - capture targets at the virtual width;
  - raster effects in wide coordinates;
  - screen layout and touch remapping.
- **Phase it.**
  1. Overworld Hor+, with everything else pillarboxed. The TWiLight 16:10 code shows the projection change alone
     already gives a usable overworld.
  2. Battles, with gauge anchoring.
  3. Overworld 2D overlays and transitions.
  4. 3D cutscenes, one by one.
- **Scope out early:** native-wide menus, the Musical stage, pre-rendered movies, and the C-Gear. Keep the bottom
  screen a 4:3 touch panel that can be placed beside, over, or on a second window.

### 6.5 Multiplayer

- **Keep the game's protocols; replace transports.** Implement a host network device under `GFLNetDevTable`, at the
  WH level (`wih.c` and the bottom of `net_whpipe.c`). Back it with UDP LAN discovery plus a relay server for the
  internet. Model beacons as publish/subscribe, so Tag Mode, surveys, Funfest (100 players), beacon Mystery Gift and
  Union Room presence work. These are exactly what emulators can't sync. IR becomes a pairing code over the same relay.
- **Replace Download Play with save import.** Poké Transfer reads a Gen 4 save; Memory Link reads a BW1 save, as
  Entralinked already does. Dream Radar data can be injected the same way.
- **Nintendo WFC: keep DWC/GameSpy byte-compatible and swap only the bottom.** That means SOC/CPS over BSD sockets, an
  always-connected WCM/AC, a DNS remap table, and NitroWiFi's own SSLv3/RC4 code compiled natively, so the port talks
  to Kaeru/Wiimmfi/PKC exactly as a DS does. That gives cross-play with real DS and emulator players for the GTS,
  Battle Videos, Musical photos, the Wi-Fi Battle Subway, Wi-Fi Club and GTS Negotiations. **Prerequisite:** about
  two-thirds of overlay 11 (972 functions of DWC and the GameSpy SDKs) is not yet in source files (section 4.0).
  Persist a per-install WFC identity outside the save. Never ship Nintendo's or the Wii's keys.
- **New online features later, behind a port-only capability flag.** Overworld presence through
  `field_comm_actor.c`, and "trade or battle anywhere" by starting the existing trade/battle processes over the shim.
- **Scope out early:** cross-play over local wireless or IR with real hardware (needs an 802.11 MP radio);
  melonDS LAN interop; Rating battles, Global Link competitions and PWT downloads (no servers; load from files);
  Xtransceiver camera video (DSi camera; map to a webcam or drop); libVCT voice chat.

### 6.6 The mod API the three goals share

| Port surface | Minimum contents | Used by |
| --- | --- | --- |
| VFS and data packers | 6.2 points 1–2 | Randomizer, hacks, CTRMap import |
| Patchable tables | 6.2 point 3, symbol-keyed JSON plus ROM extractor | Randomizer, hacks |
| Import report | 6.2 point 4 | Hacked/UPR ROMs |
| Function hooks | `HOOK_REPLACE`/`HOOK_WRAP` by decomp (= swan) name, with "call original" | PMC-mod source ports, QoL features, randomizer substitution |
| Event bus | zone change, encounter generated, trainer party built, Pokémon created, item given, shop opened, battle start/end, save | Randomizer, multiplayer presence, trackers (Nuzlocke/IronMON) |
| Script VM extension | Command IDs past the vanilla and plugin ranges; PokeScriptSDK5 command DBs as a cross-check | CTRMap/PokeScript content, custom events |
| Render policy | Projection, viewport, culling, streaming, 2D layer and anchor policy per scene | Widescreen, high resolution |
| Net device | Host `GFLNetDevTable`, relay protocol, DNS/SSL configuration | Multiplayer |
| Settings | UPR tweak equivalents, QoL toggles (level caps, instant text, IV/EV view, effectiveness HUD, autosave, portable PC) | Everyone |

### 6.7 BW2 features to decide early

1. **Version model.** B2 and W2 share source (`BLACK2`/`WHITE2`). Decide whether one port binary switches version at
   runtime: Black City vs White Forest, encounter differences, Black Tower vs White Treehollow, exclusive trades. A
   runtime switch affects every `#ifdef` and the data packers.
2. **Region and language.** The decomp builds the USA/Europe ROMs, the scene's tools support only `IREO`/`IRDO`, and
   UPR's IPS tweaks are US-only in ZX. Start US-only. Other languages are mostly text banks, but JP has different
   offsets.
3. **Save format.** Keep the 0x80000 B2W2 save format, readable and writable. That buys PKHeX, Entralinked,
   real-cartridge import/export and the transfer features above. Any species or type expansion (6.2 point 5) must
   version the save explicitly.
4. **DSi-enhanced features.** The Xtransceiver camera and photo signing (`lib/twl` camera, JPEG, `photo_sign*.c`),
   DSi-mode Wi-Fi (NWM), and the 3DS-side Dream Radar link. Treat them as optional.
5. **Unova Link and the Key System.** Key exchange between versions uses local wireless (`src/ov332/key_system_net.c`).
   A single-player port may want an "unlock keys" menu. That needs a decision, since it changes Challenge/Easy Mode
   availability.
6. **Anti-piracy.** `lib/dsprot` and the DS Protect checks are irrelevant on PC and can be stubbed. Hacks that bundle
   anti-piracy removal (BB2/VW2 1.1) then need no special handling.
7. **Licensing.** The decomp and swan are GPL-3.0. CTRMap asks for non-commercial use. A port with a plug-in API
   should decide how GPL applies to mods (exceptions or a data-only mod format) before the ecosystem forms.

### 6.8 Discrepancies and open questions found while assembling this report

- **Dream Radar vs Memory Link in overlay 181.** The decomp's header comment of `src/ov181/mb_parent_sys.c` infers
  that the second Download Play child is the Pokémon Dream Radar transfer. The multiplayer research (section 4.1)
  instead identifies `child2_r_eng.srl` as the Memory Link child, which reads a Black/White save. Public sources say
  the Dream Radar transfer is done by the 3DS app writing to the inserted BW2 cartridge. Worth checking in the
  decomp: if confirmed, the comment there is wrong.
- **"Hotspot".** Nothing named Hotspot was found. It is most likely PlatinumMaster's **Hotswap**, the project-based
  ROM file-swapping engine behind SwissArmyKnife (section 5.2).
- **Black 2 symbol coverage in swan.** swan's Black 2 ESDB has 984 symbols against White 2's ~58k. This is why the
  hacking scene is White 2-first. The decomp, which names both versions from one source, covers the gap: a
  name-based hook API serves both.
- **Unverified items** are marked inline in each section. The largest are the battle field models' coverage at 16:9,
  whether GTS Negotiations and Wi-Fi Club work on Wiimmfi today, and how Redux implements Fairy.
