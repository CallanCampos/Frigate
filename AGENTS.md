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
`file-writer`, `function-matcher`, `struct-recovery`, the reviewers `pbw-game-reviewer`, `pbw-library-reviewer`,
`pbw-config-reviewer`, `pbw-data-reviewer`, `pbw-tools-reviewer`, and the experts `pbw-mwcc-expert`,
`pbw-arm-expert`, `pbw-sdk-expert`. `docs/code-standards.md` is the C rule set; `tools/decomp/check_style.py` checks
it, and CI's Standards workflow runs it on every pull request.

The `.codex/hooks.json` hooks run `.codex/hooks/guard.py` before a tool (the repository's process, staging and
protected-path rules), `.codex/hooks/post_edit.py` after an edit (the code standards and clang-format), and
`.codex/hooks/audit_stop.py` at the end of a turn (the code-audit loop). Keep `.claude/` and these Codex copies
synchronized when either configuration changes; CI checks the skills and the hooks.
