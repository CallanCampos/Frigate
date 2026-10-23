# Reviewing upstream decomp syncs

Frigate imports the decomp through **topic branch → validated PR → owner review → merge commit**. Both the hourly
Action and the local sync leave `main` to GitHub's protected PR merge flow. The owner controls when imported code
lands. [ADR 0018](adr/0018-protected-main-and-sync-prs.md) supersedes the former direct-push design;
[repository policy](REPOSITORY-POLICY.md) describes the live rules and required checks.

## Daily contribution work

Start a feature from Frigate's reviewed `main`. External contributors use a personal fork and the commands in
[CONTRIBUTING.md](../CONTRIBUTING.md). Maintainers can use canonical feature branches. Do not run an upstream merge
as an automatic first step of unrelated feature work. Review the pending sync independently.

## Scheduled proposals

`.github/workflows/port-sync.yml` runs hourly and manually, only in `CallanCampos/Frigate`. Its trusted helper
[upstream_sync.py](../tools/upstream_sync.py):

1. Leaves an existing canonical `codex/upstream-sync-*` PR unchanged. This bounds review work to one proposal and
   avoids dismissing approvals whenever upstream advances.
2. Fetches upstream `main`, verifies continuity against the last recorded upstream parent, and checks incoming
   messages for cross-repository issue references. It stops on upstream workflow changes, including add/revert
   pairs, because the short-lived default token cannot publish them.
3. Creates a fresh branch from reviewed Frigate `main` and performs a real `--no-ff` merge. A conflict stops the run;
   it never chooses upstream's version automatically or discards port changes.
4. Pushes only the topic branch, records local verification as pending, opens a PR and requests CallanCampos review.

The publishing job runs trusted git/gh logic from Frigate's base, disables Git hooks, and never executes incoming
build scripts with its write token. The usual PR workflows perform compilation and host tests on separate hosted
runners with read-only permissions. Approve the bot-created PR's workflow runs after inspecting it. No ROMs or game
assets are uploaded. A failed proposal is visible as a failed Actions run; inspect its error and use the local path
below. No issue or comment is posted upstream. New proposals wait until the current sync PR is merged or closed.

## Validate and approve a proposed PR

Use a separate worktree prepared with `port/tools/bootstrap_checkout.sh`; it needs your own two ROM dumps and the
decomp toolchain. Inspect the proposed code and workflows before running them on your computer.

```sh
gh pr checkout NUMBER --repo CallanCampos/Frigate
port/tools/sync_upstream.sh --verify-pr NUMBER
```

This verifies the exact checked-out PR head without merging or pushing. It requires clean tracked source and no
nonignored untracked files, posts a pending status, runs both ROM builds and verifies both SHA1s, then configures,
builds and tests the native host. It checks the local and remote PR head again before recording
`Frigate local verification` success. Failed or interrupted validation cannot create a green status. A new push
needs a new verification. Keep build logs locally and describe skips or optional unrun tests in the PR.

Also review [game-changes.md](game-changes.md): confirm each port adaptation still exists or was deliberately
superseded. For changed incomplete C or shared headers, supply the matching-neutral evidence described there;
rebuilding the ROM alone cannot prove C that the ROM does not link. Run relevant replay, sanitizer and oracle checks
when those systems change. Public CI does not replace local ROM or gameplay validation.

When all four required checks pass on an up-to-date branch, approve contributor/bot PRs and merge using **Create a
merge commit**. No auto-merge is enabled. The owner-authored exception is review-only; it still requires the PR,
checks and resolved conversations. Do not bypass review on a bot sync merely to speed it up.

## Prepare or repair a sync locally

The local tool is for maintainers in a clone whose `origin` is `CallanCampos/Frigate`; `upstream` must be the decomp
and its push URL is disabled. Start from a clean reviewed `origin/main` in your own worktree:

```sh
git fetch origin
git switch -c codex/upstream-sync-manual origin/main
port/tools/sync_upstream.sh --branch codex/upstream-sync-manual
```

The tool fetches Frigate and upstream, merges them without rewriting history, runs local validation, then publishes
only the topic branch and opens a PR. It refuses `--branch main`. When you are repairing an existing pending sync,
check out that branch and pass its exact name. Do not start a second sync while one is pending. A local PR created
by the owner uses the review-only exception; use the bot's proposed PR when available so the owner can approve it.

Useful options:

| Option | Effect |
| --- | --- |
| `--verify-pr NUMBER` | Validate a checked-out PR head, including contributor PRs, and submit its local status; no merge or push. |
| `--branch codex/upstream-sync-NAME` | Explicit topic destination, never main. |
| `--no-build` | Prepare an unpublished merge only; implies no push. Cannot certify a PR. |
| `--no-push` | Validate but leave the topic local. |
| `--abort-on-conflict` | Abort a conflicting merge instead of leaving it for resolution. |
| `--metrics` | Also gather the progress metrics for STATUS.md. |
| `--allow-rewrite` | Explicit user decision required to accept rewritten upstream ancestry. |
| `--allow-references` | Explicit user decision required to publish commits with cross-repository issue references. |

Never force-push. If `main` advances, merge it into the topic, rerun validation and obtain fresh review. If a proposal
is superseded, close it and let a new bounded proposal be prepared. Preserve the real upstream parents; do not
squash, rebase, copy files wholesale or reset Frigate to upstream.

## Conflicts and stopped syncs

- **Merge conflict (local exit 3):** inspect every conflict. Use upstream's new structure, then reapply the port's
  documented adaptation from game-changes.md at its new location, or document why upstream makes it unnecessary.
  Stage only resolved files, commit the merge with `git commit --no-edit --cleanup=strip`, then rerun the local
  script on the same sync branch. Do not replace an entire game file with upstream's version and lose port changes.
- **Upstream rewrite (exit 4):** stop and ask the owner before `--allow-rewrite`. Never infer permission from a
  scheduled task. Preserve the published Frigate history and examine the new upstream ancestry.
- **Cross-repository reference (exit 5):** GitHub can notify another project's issue timeline when commits name it.
  Stop before publishing. The user alone authorizes `--allow-references`; port PR text must not add upstream issue,
  PR or discussion links or @-mentions. A commit SHA or commit link is fine.
- **Upstream workflow changes:** use local maintainer credentials after reviewing the workflow diff. Keep
  `build.yml` disabled and `compile.yml`, `standards.yml`, `port-ci.yml`, `port-sync.yml` active. Check for newly
  introduced workflows and permissions before merge. No workflow-scoped PAT is stored in Actions.
- **Build/test failure:** fix only the port's change or hold the PR. Do not edit decomp configs or expected SHA1s to
  force a pass. A failure reproduced on upstream itself is a blocker recorded here, never a port report upstream.

Every merge message retains `Upstream-Commit: FULL_SHA`. Record verified syncs and validation limits in STATUS.md.
