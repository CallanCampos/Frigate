# pokebw2 Codex project guidance

Read `CLAUDE.md` first: its Rules (both ROMs byte for byte, natural C only, claiming an overlay, publishing every
commit) apply to Codex sessions too, and it points to the README, `CONTRIBUTING.md` and `docs/`, which are the source
of truth.

The repository's project skills are in `.agents/skills/`. Use the relevant skill when carrying out decompilation,
matching, build-fix, finish-file, or lesson-recording work. They are copies of `.claude/skills/`: where a skill names a
file under `.claude/skills/`, the same file under `.agents/skills/` is meant too, and a change to a skill goes into both
copies in the same commit.

The named specialist roles from `.claude/agents/` are ported as Codex roles in `.codex/config.toml` and
`.codex/agents/`. Skills that mention those roles can therefore use `boundary-scout`, `decomp-reviewer`,
`file-writer`, `function-matcher`, and `struct-recovery`.

The `.codex/hooks.json` pre-tool hook runs `.codex/hooks/guard.py`, which enforces the repository's memory, process,
staging, and protected-path rules. Keep `.claude/` and these Codex copies synchronized when either configuration
changes.
