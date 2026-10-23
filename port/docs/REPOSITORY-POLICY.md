# Repository permissions and merge policy

Frigate uses **fork → feature branch → pull request** for external contributions. Maintainers and the sync bot may
publish branches in the canonical repository, but every change to `main` still goes through a PR. The required
workflow is enforced by GitHub settings as well as documented in [the contribution guide](../CONTRIBUTING.md).

## Protected main

Two active rulesets apply to `refs/heads/main`:

| Rule | Enforcement |
| --- | --- |
| [PRs, validation and history](https://github.com/CallanCampos/Frigate/rules/25687796) | No bypass actors. PR required; no force pushes or deletion; conversations resolved; branch up to date; required checks; merge commits only. |
| [Maintainer review](https://github.com/CallanCampos/Frigate/rules/25687797) | One approval, CODEOWNER review, stale approvals dismissed, approval after the last push. Only CallanCampos has a PR-only review bypass. |

The review-only exception is necessary while CallanCampos is the only maintainer: GitHub prohibits approving one's
own PR. Use it for owner-authored PRs; contributor and bot sync PRs should receive an explicit owner approval. GitHub
cannot limit that exception to owner-authored PRs, so this remains an owner responsibility. It cannot bypass the
other ruleset. Bots, Actions, deploy keys and other contributors have no bypass. Remove the exception when a second
trusted maintainer can review owner-authored work. CODEOWNERS names the owner for all paths, including itself.

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

The default workflow token remains read-only. External contributors' workflows all require maintainer approval;
inspect workflow and code changes before allowing a run. Public PR jobs use hosted runners without ROM secrets.
Port checkouts do not persist credentials, and port Actions are pinned to full commit SHAs.

GitHub's combined **Allow GitHub Actions to create and approve pull requests** setting is enabled because the sync
bot needs to create PRs. The bot has no approval step and cannot approve its own PR; CODEOWNER review and the required
checks still apply. No PAT or GitHub App private key was added. With the current GitHub behavior, PR events created
with `GITHUB_TOKEN` produce approval-required workflow runs; approve those before expecting required checks to pass.

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
