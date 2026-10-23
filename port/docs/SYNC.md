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
default token's workflow-approval restriction. Human code review and local ROM verification remain required. No ROMs or game
assets are uploaded. A failed proposal is visible as a failed Actions run; inspect its error and use the local path
below. No issue or comment is posted upstream. New proposals wait until the current sync PR is merged or closed.

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
when those systems change. Public CI does not replace local ROM or gameplay validation. A sync PR whose host jobs
fail the pointer cast guard (`game_ptrcasts`) waits for a follow-up feature PR
([below](#when-the-pointer-cast-guard-fails)).

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
check out that branch and pass its exact name. Do not start a second sync while one is pending. A locally prepared sync still requires the owner's explicit human review before merge, even when owner credentials
created its PR. Automation must not treat it as ordinary owner-authored feature work and merge it under the solo
exception without that review. Prefer an App-authored proposal so GitHub can record the owner's approval.

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

## When the pointer cast guard fails

Why the guard works this way is [ADR 0019](adr/0019-pointer-cast-guard-against-a-classified-baseline.md).

An upstream merge can bring C that cuts an address to 32 bits on a 64-bit host: a pointer passed as `(u32)work` to an
API whose parameter is a `u32`, an integer made a pointer with `(FieldTask *)a2`, a `uptr` stored in a `u32`. Under
MWCC that is the DS's code as it was, so both ROMs still match and nothing in the ROM build notices; on a host the game
faults the first time it runs it. The test `game_ptrcasts` ([../cmake/game.cmake](../cmake/game.cmake)) catches it in
every host build's `ctest`: CI's macOS and Linux jobs (host, sanitized and the macOS distribution builds) on every PR,
sync PRs included, the local verification's `ctest` in both versions (`--verify-pr`), and the local sync's before it
publishes.

It runs [../tools/check_ptrcasts.py](../tools/check_ptrcasts.py) on the build: each `src/` and `lib/` command of the
build's `compile_commands.json` again with the build's clang, syntax only, with the cast warnings the host build
silences for the game's C turned back on and the decomp's headers searched as ordinary headers, so that their warnings
show too. It counts four kinds, `ptr-to-int`, `int-to-ptr`, `narrow` (a `uptr`, `sptr`, `uintptr_t` or `intptr_t`
narrowed to 32 bits, other than a call's result as it is stored: a getter that returns a `uptr` gives numbers and
addresses alike, and a caller that makes a pointer of the cut value again is flagged there) and `fn-cast` (a callback
cast that pairs a pointer or a `uptr` with a 32-bit integer), and holds them to
[../tools/ptrcast_baseline.tsv](../tools/ptrcast_baseline.tsv). The baseline has a row per finding, its file, the
function that holds it, the kind and the types as the source spells them, with its number of sites, its class and a
one-line reason: `real` (a truncation not fixed yet, its group in the note), `harmless` (it never cuts an address the
game uses), `handled` (the platform layer makes the value right) or `unreachable` (the host never runs it). Line
numbers aren't part of a row, so upstream's edits elsewhere in a file leave the baseline alone, and a row whose
function, file or types upstream renamed passes as the same row, with a note. Both versions give the same findings,
and so do Apple clang, Homebrew's clang and Ubuntu's.

When it fails, it prints each new site with its file, line, function and source line. For each:

1. **Read the code**: does the value carry an address that the game uses, through an integer parameter, field, return
   or local?
2. **If it does, fix it the Neutral way** ([PLAN.md §4.8](PLAN.md#48-64-bit-and-dual-target-rules), rule 3): make the
   parameter, field or return `uptr` (`sptr` for a signed one), and the casts at the callers `(uptr)`, retyping the API
   rather than each cast; a getter that returns pointers and numbers alike returns `uptr`, and its callers' numbers
   then show as harmless `narrow` rows. `uptr` is `u32` for MWCC, so the proof is
   `port/tools/check_mwcc_neutral.py FILE...` on the changed files and every file that includes a changed header, in
   both versions, which finds their token streams unchanged, and ninja with both SHA1s; the change gets its row in
   [game-changes.md](game-changes.md). An `int` made `sptr` is `s32` for MWCC, the same type but another token: ninja
   proves the complete files that differ, and `tools/decomp/compiler_probe.py FILE --compilers 1.1p1` in both versions,
   before and after, the incomplete ones ([game-changes.md](game-changes.md#proving-a-change-is-mwcc-neutral)).
3. **If it can't** (an alignment or low-bit test, a difference of two addresses in one block, an int carried in a
   `void *` and back, code the host never runs), run
   `.venv/bin/python port/tools/check_ptrcasts.py --build-dir build/port --update`, which writes the new rows with the
   class `new`, and give each its class and reason: the check fails on any other class than the four.
4. **A truncation that waits for later work**, such as Wi-Fi code before phase 7, can stay a `real` row whose note
   names its group, until its fix lands.

Then run the check again in both versions (`build/port` and `build/port-w2`) and commit the baseline with the change.
A count below the baseline's, or a row no longer found, passes with a note of what `--update` would change; run it
when a fix lands, so that the baseline shrinks with the fixes. A file that no longer compiles fails the check too.

**On a sync PR, the App's or a local one, the maintainer fixes it in a follow-up feature PR, never by editing the sync
PR.** The sync's merge commit carries only the sync's own resolutions, and a push to the sync branch would change the
head the owner reviews. So:

1. Check out the sync PR's head in a verification worktree (`gh pr checkout NUMBER --repo CallanCampos/Frigate`),
   build it, and run the check there to read each new site.
2. On a feature branch from `origin/main`, give the baseline the rows the sync needs: the rows `--update` writes on the
   sync's head, each with its class and reason, a cast that cuts an address the game uses as `real` with the fix to
   come in its note. On `main`, which doesn't have the new code yet, the rows pass as "no longer found"; don't run
   `--update` there before the sync merges. Open, verify and merge that PR as any other.
3. Bring the sync PR up to date with `main` (the owner closes it and the bot proposes it again, or `main` is merged
   into it), so that CI and `--verify-pr` run again on the new head, and the owner reviews it.
4. After the sync merges, fix each `real` row the sync brought in its own feature PR, with its game-changes row.

A local sync (`sync_upstream.sh`) stops on the failure in its `ctest`, before it publishes anything; the same feature
PR comes first, and the sync runs again after it.

With `--report` it prints the sites by class in one line, when it was added:

```text
check_ptrcasts: 389 sites in 250 keys (real 159, harmless 99, handled 96, unreachable 35), 1667 files, 0 not compiled
```

That line is T1's 64-bit measurement: [STATUS.md](STATUS.md) gives it in each entry's "64-bit warnings", the real
sites not above the last entry's, and `sync_upstream.sh` prints it in its summary.

It skips (77) on Windows, whose LLP64 (a 32-bit `long`) gives other findings than macOS's and Linux's, without clang,
and without the compile database. What it doesn't see: a pointer written into a union's integer member, which has no
cast (`port/tools/compile-probe/scripts/unions.py` lists the unions that mix pointers and integers); a compound
assignment such as `u32 dst += (uptr)p`, whose read back is flagged instead; a pointer cast to `long` or
`unsigned long`, which [PLAN.md §4.8](PLAN.md#48-64-bit-and-dual-target-rules)'s rule 6 forbids; and the platform
layer's own code, whose files are in `port/`.
