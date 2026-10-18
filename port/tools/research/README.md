# port/tools/research/

Scripts of the 2026-10-16 research that the plan was made from, kept so that its numbers can be measured again. Each
reads the decomp's tree (Black 2's configs, its C, the extracted ROM) and prints or writes what a report in
[../../docs/research/](../../docs/research/README.md) quotes; none is part of Frigate's build, but one file here is
its input: `assets/nitrofs_manifest.json`. They write only to a work directory given to them (`engine/`), to
`build/port-research/sdk-surface/` of the checkout, which git ignores (`sdk-surface/`), or to a file named on the
command line (`assets/`). Each script's docstring has its usage and what it reads.

## engine/: the game's engine, its boot path and the port's milestones

The report is [engine-boot.md](../../docs/research/engine-boot.md). `coverage.py` runs first and writes the tables the
others read from the work directory.

| Script | What it measures | Run |
| --- | --- | --- |
| [coverage.py](engine/coverage.py) | C coverage of Black 2 per function, file and module, from `config/b2_us` and the files' C | `coverage.py REPO WORK` |
| [overlays.py](engine/overlays.py) | The overlay table (addresses, sizes, `.data` and `.bss`, static initializers, coverage) and the groups of overlays that share addresses | `overlays.py REPO WORK` |
| [milestones.py](engine/milestones.py) | What blocks each of the port's milestones, from the boot and logos (M1) to the menus (M7): functions with no C and incomplete files, by the overlays each needs | `milestones.py WORK REPO` |
| [callers.py](engine/callers.py) | Which C files call game functions that have no C yet | `callers.py REPO WORK` |
| [datacov.py](engine/datacov.py) | The data and `.bss` symbols that aren't C yet, per module: what the host build's data placeholders stand in for | `datacov.py REPO WORK` |
| [datacov2.py](engine/datacov2.py) | One module's data symbols that an incomplete file covers but doesn't define yet | `datacov2.py REPO MODULE` |
| [datasyms.py](engine/datasyms.py) | How many overlays have `.data` and `.bss` symbols, for the host's per-overlay sections ([../../hal/fs/ovsec.h](../../hal/fs/ovsec.h)) | `datasyms.py REPO` |

## sdk-surface/: where the platform layer's boundary sits

The report is [sdk-surface.md](../../docs/research/sdk-surface.md). `db.py` runs first, then `binscan.py`; the rest
read their files in `build/port-research/sdk-surface/`.

| Script | What it measures | Run |
| --- | --- | --- |
| [db.py](sdk-surface/db.py) | Every function of Black 2 with its library and file, and the call and reference graph, from `config/b2_us` and `extract/b2_us` | `db.py [REPO]`, writes `db.pkl` |
| [binscan.py](sdk-surface/binscan.py) | The DS hardware each function's machine code touches: register regions, SWIs, CP15, interrupt masking | `binscan.py`, writes `binscan.pkl` |
| [hwscan.py](sdk-surface/hwscan.py) | The hardware access in the C: register macros, raw addresses, `asm`, SWIs, CP15 | `hwscan.py [REPO]`, writes `hwscan.json` |
| [layout.py](sdk-surface/layout.py) | Function counts, bytes and C coverage per library | `layout.py [game]` |
| [classify.py](sdk-surface/classify.py) | NitroSDK's and TwlSDK's functions classed A (kept), B (replaced), B-lite (incidental hardware), C (stubbed) | `classify.py`, writes `b_used.json` |
| [boundary.py](sdk-surface/boundary.py) | The SDK API that the code kept native calls, by the caller's layer | `boundary.py`, writes `sdk_used.json` |
| [surface.py](sdk-surface/surface.py) | The library functions the game's code calls or takes the address of, by family | `surface.py [REPO]`, writes `surface.json` |
| [gfl.py](sdk-surface/gfl.py) | GFL as a boundary, and the graphics calls that go around it | `gfl.py` |
| [dossier.py](sdk-surface/dossier.py) | Each library sub-module's size, coverage, hardware and calls into PXI, interrupts, DMA and the caches | `dossier.py`, writes `dossier.json` |
| [callers.py](sdk-surface/callers.py) | Who calls the functions whose names match a pattern | `callers.py REGEX [TOP]` |
| [q_edge.py](sdk-surface/q_edge.py) | The functions at the boundary of one SDK module | `q_edge.py MODULE` |
| [ranges.py](sdk-surface/ranges.py) | Where a module's source files sit in its `.text` | `ranges.py MODULE [ALL]` |

## assets/: the ROM's files and its sound archive

The report is [assets-arm7-sound.md](../../docs/research/assets-arm7-sound.md).

| File | What it measures | Run |
| --- | --- | --- |
| [fs_survey.py](assets/fs_survey.py) | Every NitroFS file's type and every NARC's members | `fs_survey.py EXTRACT_FILES OUT.json` |
| [coverage.py](assets/coverage.py) | How much of NitroFS the decomp builds from source | `coverage.py REPO SURVEY.json` |
| [diff_versions.py](assets/diff_versions.py) | Which files differ between Black 2 and White 2 | `diff_versions.py EXTRACT_B2_FILES EXTRACT_W2_FILES` |
| [sdat_survey.py](assets/sdat_survey.py) | The sound archive's blocks, files and records, and its sequences split into music, sound effects and cries | `sdat_survey.py SOUND_DATA.sdat` |
| [sdat_players.py](assets/sdat_players.py) | The sound archive's players and streams, and how many sequences each player plays | `sdat_players.py SOUND_DATA.sdat` |
| [manifest.py](assets/manifest.py) | Writes `nitrofs_manifest.json` | `manifest.py REPO OUT.json` |
| [nitrofs_manifest.json](assets/nitrofs_manifest.json) | The size and SHA-1 of every NitroFS file of both versions' ROMs, hashes only: [../../cmake/rom.cmake](../../cmake/rom.cmake) makes it the ROM manifest that lets Frigate take a trimmed or re-padded dump ([../../hal/rom/port_rom.h](../../hal/rom/port_rom.h)) | build input |
