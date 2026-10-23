# Repository permissions and merge policy

Frigate uses **fork → feature branch → pull request** for external contributions. Maintainers and the sync bot may
publish branches in the canonical repository, but every change to `main` still goes through a PR. The required
workflow is enforced by GitHub settings as well as documented in [the contribution guide](../CONTRIBUTING.md).

## Protected main

Two active rulesets apply to `refs/heads/main`:

| Rule | Enforcement |
| --- | --- |
| [PRs, validation and history](https://github.com/CallanCampos/Frigate/rules/25687796) | No bypass actors. PR required; no force pushes or deletion; conversations resolved; branch up to date; required checks; merge commits only. |
| [Maintainer review](https://github.com/CallanCampos/Frigate/rules/25687797) | One approval, CODEOWNER review, stale approvals dismissed; the owner can review after updating a bot/contributor branch. Only CallanCampos has a PR-only review bypass. |

The review-only exception is necessary while CallanCampos is the only maintainer: GitHub prohibits approving one's
own PR. Use it for owner-authored feature/fix PRs; contributors and upstream syncs require explicit human owner
review before merge, including locally prepared syncs created with owner credentials. GitHub
cannot limit that exception to owner-authored PRs, so this remains an owner responsibility. It cannot bypass the
other ruleset. Bots, Actions, deploy keys and other contributors have no bypass. Remove the exception when a second
trusted maintainer can review owner-authored work. CODEOWNERS names the owner for all paths, including itself.

`require_last_push_approval` is false in the reviewed settings payload. Requiring someone other than the last
pusher would prevent the sole owner from approving a bot-authored sync after merging a main update into its
branch. Stale approvals are still dismissed by new pushes, so the owner must inspect and approve the new head.
This setting has been applied and read back; changing the JSON alone does not change the live rule.

Required checks (names are a compatibility interface):

- `Compile src/ and lib/`: every decomp C file compiled for both game versions without ROMs.
- `Code standards`: inherited decomp style and consistency checks.
- `Frigate CI`: aggregate that succeeds only when every native build/test group and sync tooling tests succeed.
- `Frigate local verification`: both local ROM SHA1s and native host build/tests on the exact PR head.

The three Actions checks are bound to the GitHub Actions integration (15368). Local verification is a commit status
set by the authenticated maintainer, not a public runner; only trusted writers can submit statuses. GitHub cannot
prove an attestation's truth, so maintainers must use the build-running verification command. No self-hosted runner
with ROMs or write credentials processes untrusted PRs. Inspect code before executing it locally.

Merge commits are the only enabled merge method. In particular, never squash or rebase upstream sync PRs: the real
upstream parent and `Upstream-Commit` trailer make later imports preserve history and port modifications. Automatic
merge is disabled; merged topic branches are deleted automatically. GitHub's update-branch button is enabled; any
new head must be validated again.

## Actions settings

The default workflow token remains read-only. The live fork policy is
`first_time_contributors_new_to_github`, GitHub's least restrictive available approval option: approval is required
for first-time contributors with new GitHub accounts. Established contributors' PR CI can run automatically.
If GitHub does request a run approval, inspect the proposed code and workflow changes before granting it.
Public PR jobs use hosted runners without ROM or App secrets. Port checkouts do not persist credentials, and
port Actions are pinned to full commit SHAs. Automatic CI never substitutes for the owner's human code review:
contributor and sync PRs require explicit owner approval before merge.

The replacement scheduled publisher uses a dedicated GitHub App to start ordinary PR CI automatically, while
retaining owner review and local ROM verification. Its narrow installation/token scopes, secret names and setup
are documented once in [SYNC.md](SYNC.md#set-up-the-dedicated-sync-app). It cannot post the required local status,
approve PRs, merge, or bypass rules through this workflow. No PAT is used.

The dedicated **Frigate Upstream Sync** App (ID `5435198`, installation `174325863`) is installed on Frigate alone.
Its Contents/Pull requests write and automatic Metadata read scopes, empty webhook subscription list and selected
repository scope were verified through App authentication. The key is a repository Actions secret; a trial
installation token was revoked after verification. GitHub's combined **Allow GitHub Actions to create and approve
pull requests** setting is disabled (`can_approve_pull_request_reviews: false`). The App uses its own installation
token and does not need this default-token permission. Missing App setup fails with instructions instead of silently
falling back to the default token.

The inherited `build.yml` stays disabled because it requires upstream's private ROM credential. `compile.yml` and
`standards.yml` stay active and unchanged. Upstream workflow imports are handled locally and reviewed explicitly.
The repository installation endpoint returned HTTP 401 because it requires GitHub App authentication; the available
maintainer OAuth credential cannot enumerate all account-wide app/token grants. Recheck that access in account
settings; the rulesets give no integration a bypass.

## Reproduce or audit settings

The reviewed API payloads live in [main-ruleset.json](../tools/main-ruleset.json) and
[reviews-ruleset.json](../tools/reviews-ruleset.json). Changes to these files alone do not change GitHub. A maintainer
with Administration write access applies updates explicitly:

```sh
gh api --method PUT repos/CallanCampos/Frigate/rulesets/25687796 --input port/tools/main-ruleset.json
gh api --method PUT repos/CallanCampos/Frigate/rulesets/25687797 --input port/tools/reviews-ruleset.json
gh api repos/CallanCampos/Frigate/rules/branches/main
gh api repos/CallanCampos/Frigate/rulesets/25687796
gh api repos/CallanCampos/Frigate/rulesets/25687797
gh api repos/CallanCampos/Frigate/collaborators --paginate
gh api repos/CallanCampos/Frigate/keys
gh api repos/CallanCampos/Frigate/actions/permissions/workflow
gh api repos/CallanCampos/Frigate/actions/permissions/fork-pr-contributor-approval
gh workflow list --all --repo CallanCampos/Frigate
```

A personal public repository gives visitors read access and allows forks. At the 2026-10-22 audit, CallanCampos was
the sole collaborator (admin), and there were no invitations, deploy keys or repository webhooks. Main previously
had neither classic protection nor rulesets. Rulesets now enforce protection; a classic-protection endpoint can
still return 404 and must not be mistaken for absence of ruleset protection. Account owners can edit repository
settings themselves; protection is an enforced development policy, not a lock against the repository owner.

## Sources and tradeoffs

[ADR 0018](adr/0018-protected-main-and-sync-prs.md) records the port-project research and why Frigate keeps one main
branch. GitHub documents [rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets),
[CODEOWNERS](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)
and [workflow triggering](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/trigger-a-workflow).
