# pokebw2 Codex project guidance

The repository's project skills are in `.agents/skills/`. Use the relevant
skill when carrying out decompilation, matching, build-fix, finish-file, or
lesson-recording work.

The `.codex/hooks.json` pre-tool hook runs `.codex/hooks/guard.py`, which
enforces the repository's memory, process, staging, and protected-path rules.
Keep `.claude/` and these Codex copies synchronized when either configuration
changes.
