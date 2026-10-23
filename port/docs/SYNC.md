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
   pairs, because the sync App deliberately has no Workflows permission.
3. Creates a fresh branch from reviewed Frigate `main` and performs a real `--no-ff` merge. A conflict stops the run;
   it never chooses upstream's version automatically or discards port changes.
4. Pushes only the topic branch, opens a PR and requests CallanCampos review. Local verification stays missing
   until a maintainer runs the verifier; a missing required status blocks merging.

The publishing job runs trusted git/gh logic from Frigate's base, disables Git hooks, and never executes incoming
build scripts with its write token. The usual PR workflows perform compilation and host tests on separate hosted
runners with read-only permissions. The dedicated App token allows PR CI to start automatically, without the
default token's workflow-approval restriction. Human code review and local ROM verification remain required. No ROMs
or game assets are uploaded. A failed proposal is visible as a failed Actions run; inspect its error and use the local
path below. No issue or comment is posted upstream. New proposals wait until the current sync PR is merged or closed.

### Set up the dedicated sync App

The dedicated **Frigate Upstream Sync** App (ID `5435198`) is registered and installed on Frigate alone. Its
repository variable and secret are configured; the default token's combined create/approve-PR setting is disabled.
The steps below document recreation and rotation; never create a duplicate installation just to repair a secret.

The owner registers a dedicated GitHub App and installs it on **CallanCampos/Frigate only**, using selected
repositories. Grant repository **Contents: read and write** and **Pull requests: read and write**; Metadata read is
automatic. Grant no other repository or organization permissions, subscribe to no webhook events, and leave the
webhook inactive. The App needs neither Actions nor Workflows nor Commit statuses write access. Never add it to a
ruleset bypass list or configure it to approve or merge PRs.

Set repository variable `FRIGATE_SYNC_APP_ID` to its numeric App ID and repository Actions secret
`FRIGATE_SYNC_APP_PRIVATE_KEY` to its generated PEM private key. Store the key only as a secret, never in Git,
PRs or logs. Keep this installation dedicated to sync: installation tokens cannot reduce access to individual
branches. Contents write enables ordinary repository pushes, so protected-main rules are still essential.
Pull requests write is GitHub's coarse scope for PR creation; the publisher uses it only to inspect/create PRs and
request owner review, never to submit reviews or approvals.

The pinned official [create-github-app-token action](https://github.com/actions/create-github-app-token) mints a
short-lived installation token explicitly limited to this owner/repository and those two permissions. Its post-job
cleanup revokes the token (`skip-token-revoke: false`). The ordinary workflow token has only Contents read for
checkout, with persisted credentials disabled. Only the trusted publishing step receives the App token; incoming
code never runs in that job, and separate PR CI receives no App key or token. The App cannot certify
`Frigate local verification`; the maintainer's local command still does that after real builds.

Missing variables/secrets or an invalid installation make the scheduled run visibly fail. There is no
`GITHUB_TOKEN` or PAT fallback. After setup, dispatch `port-sync.yml` manually and confirm the bounded PR has the
App as author, PR checks start automatically, CallanCampos review is requested, and local verification remains
required. If the run stops on workflow imports or another safeguard, inspect that blocker and use the documented
local procedure. Do not expand App permissions to get past it.

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
check out that branch and pass its exact name. Do not start a second sync while one is pending. A locally prepared
sync still requires the owner's explicit human review before merge, even when owner credentials created its PR.
Automation must not treat it as ordinary owner-authored feature work and merge it under the solo exception without
that review. Prefer an App-authored proposal so GitHub can record the owner's approval.

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

### A sync merge carries only the sync

The merge commit holds upstream's changes and the resolutions they force: its conflicts resolved, the port's
adaptations reapplied where upstream moved them ([game-changes.md](game-changes.md)), and the native fixes without
which the merged tree doesn't build or pass its tests. Nothing else goes into it: no documents, tools, features or
review follow-ups of Frigate's own, which a reviewer could then find only by diffing the merge against both parents
(`90ad973b12` changed 40 port files: its native fixes, and beside them a contribution policy, ADR 0017, a review
record, templates and rewritten guides). Such work goes in commits of its own after the merge, or in a separate PR,
with messages that say what each does. The sync's PR and its STATUS.md entry say which upstream change forced each
native fix in the merge.

### Records are added to

STATUS.md's entries, the review records (`REVIEW-*.md`) and accepted ADRs keep what they said when they were written.
A change that finds one wrong, or finishes what one left pending, adds a new entry (or an ADR's status line) that
says so; it never edits the old text. The only edit to a past STATUS.md entry is filling its `PORT_SHA`, as
[STATUS.md](STATUS.md#how-to-add-an-entry) says; typos and broken links may be fixed.

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

Every merge message retains `Upstream-Commit: FULL_SHA`. Record each merged sync, with its verification and its
limits, in a new STATUS.md entry.
