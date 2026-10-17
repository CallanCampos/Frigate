# Port tools

Measurement scripts from the 2026-10-16 research. None of them write into the repository; give each an output
directory outside it (or under an ignored `out/`).

| Directory | Purpose | Run |
| --- | --- | --- |
| `progress/` | Port readiness of the decomp: per-function tiers (complete C / C not linked / no C), data and pointer coverage, both versions, daily history, GitHub claims, nonmatching rows by kind, ByteMatch data. Re-run at each sync and record the headline in `port/docs/STATUS.md`. | `sh port/tools/progress/run_all.sh <repo> <outdir>` (about 5 minutes; needs `extract/{b2_us,w2_us}` and `.venv`) |
| `compile-probe/` | Host-compiler probe: clang over every C file for the DS target and 64-bit hosts, warning counts by kind and module, struct layouts compared between MWCC and each host ABI. The 64-bit warning count is a port metric that must only go down. | see its README |
| `research/engine/` | Boot-path milestones, overlay base groups, data coverage per file (one-off) | scripts take the repository path |
| `research/sdk-surface/` | Library layering, game-side SDK surface, hardware access scan (one-off; `db.py` caches into a scratch directory) | scripts take the repository path |
| `research/assets/` | NitroFS survey, SDAT survey, version diff, and `nitrofs_manifest.json`, the per-file SHA1 manifest of both ROMs used to verify a player's dump | scripts take the extracted ROM path |

Requirements: a configured checkout with `extract/` (from `ninja extract/b2_us/config.yaml extract/w2_us/config.yaml`)
and `build/include/generated` (from `ninja constants_headers`), Homebrew LLVM for the compile probe, and read-only
`gh` for the claims report.
