<!-- Records why Frigate's main changes only through reviewed pull requests, upstream syncs included. -->
# 0018: Protected main and reviewed upstream syncs

- **Status:** accepted; amends 0001 and 0017; amended by [0024](0024-a-conflicting-sync-proposes-a-draft-of-upstreams-head.md)
- **Date:** 2026-10-22
- **Plan:** D1, §4.1 and §8
- **Upstream:** `fc478a9ee5`

## Context

The owner requested enforced fork → feature branch → PR contributions and reviewable upstream syncs. The live
GitHub audit found no branch protection or rulesets, one collaborator (the owner), no deploy keys or pending
invitations, and read-only default Actions permissions. Contribution prose required forks, but maintainer guidance
and both sync publishers still pushed `main`. The scheduled sync had extensive safeguards and a compile-only
validation path; port game changes already caused it to defer to local sync. Local sync checked ROMs and host tests,
then pushed the result directly. Neither path provided a review point before main changed.

## Decision

Keep one integration branch, `main`, and the existing same-tree history-preserving upstream merges. All changes go
through PRs. Two rulesets separate unbypassable PR/CI/history requirements from human review: one approval and
CODEOWNER review normally apply, with an owner-only PR review exception because there is no second maintainer.
The exception cannot skip checks, force-push, delete main, or directly publish it. Remove it when another trusted
maintainer can review owner-authored PRs. CODEOWNERS requests the owner for imported source and infrastructure too.

The hourly sync proposes one frozen PR on a dedicated branch. It never auto-merges or rewrites that proposal while
review is pending. Incoming code runs only in read-only PR CI; publishing uses trusted git/gh logic. Both ROM SHA1s
and native tests are a separately required local status on the tested commit. Local sync publishes topic PRs too.
Conflicts, rewritten history, reference notifications and workflow imports remain explicit inspection points.
Merge commits are the sole merge method, preserving upstream ancestry. A dedicated GitHub App now authenticates
only the scheduled topic push and PR creation so PR CI starts automatically. Its installation is limited to Frigate
and Contents/Pull requests write; it has no bypass, workflow-write or local-verification status authority. The App
private key is an Actions secret; each job mints and revokes its own short-lived token. Missing setup fails visibly.

[REPOSITORY-POLICY.md](../REPOSITORY-POLICY.md) records settings and readback commands; [SYNC.md](../SYNC.md) is the
operational procedure. Imported root instructions and workflows remain unchanged to reduce future merge conflicts;
port-owned instructions explicitly override their direct-publishing guidance.

## Research adapted to Frigate

- Ship of Harkinian's [modding guide](https://github.com/HarbourMasters/Shipwright/blob/develop/docs/MODDING.md)
  shows forks and descriptive feature branches. Its own-fork mod distribution PR is deliberately unmerged; Frigate
  instead targets its canonical main for contributions. Its
  [build workflow](https://github.com/HarbourMasters/Shipwright/blob/develop/.github/workflows/generate-builds.yml)
  supports multi-platform PR testing and cancellation of superseded runs, which Frigate retains with a stable gate.
- Shipwright's [formatting guide](https://github.com/HarbourMasters/Shipwright/blob/develop/docs/FORMATTING.md)
  separates imported/generated code from formatting scope. Frigate keeps upstream-owned files intact and makes
  port infrastructure changes in port-owned paths. Its
  [README](https://github.com/HarbourMasters/Shipwright/blob/develop/README.md) illustrates separating the platform
  library from decompiled game logic; Frigate retains port/ and its existing architecture and game-change maps.
- Shipwright's [public default-branch ruleset](https://github.com/HarbourMasters/Shipwright/rules/24855177) exposed
  deletion and non-fast-forward bans at audit time, not required reviews/checks. We adopt the history protection
  but do not claim its public rules prove a review policy.
- sm64ex's [README](https://github.com/sm64pc/sm64ex/blob/nightly/README.md) makes its contribution target explicit
  (`nightly`). Frigate uses main instead of adding a branch with no current release-management need. Its
  [Jenkinsfile](https://github.com/sm64pc/sm64ex/blob/nightly/Jenkinsfile) separates private ROM provisioning from
  build validation. Frigate keeps public CI ROM-free and local ROM verification explicit. Checked-in Jenkins
  configuration is evidence of design, not proof of a currently operating Jenkins service.

## Alternatives and consequences

Direct automated main updates would bypass the requested review point and leave port compatibility dependent on
later validation. The initial default-token design required approval before bot-created PR checks ran. The owner
subsequently requested automatic CI; a dedicated, narrowly scoped GitHub App provides that without a personal
access token or review/check bypass. A second integration branch,
submodule conversion, or copied decomp snapshot would add synchronization work without solving review enforcement.

Main advances more slowly but every update has recorded CI, local validation and review. Frozen proposals avoid
hourly approval churn. Owner-authored work still has an auditable PR, even where the owner must use the narrowly
scoped review exception. GitHub cannot prove a maintainer's local attestation or prevent the owner editing the rules;
keep the scripts, payloads and procedure reviewable and audit settings after access changes.

## Amendment: automatic sync PR CI (2026-10-22)

The owner requested automatic CI for scheduled sync proposals. Keep the frozen proposal, workflow-change guard,
reference/history checks, owner code review and separate local ROM verification. Replace only the publisher's
credential. The App requests Contents and Pull requests write explicitly; it never posts even a pending local
verification status, and it never approves or merges. GitHub's Pull requests write permission is broader than PR
creation, so the trusted publisher's small command surface and protected-main rules remain part of the boundary.
[SYNC.md](../SYNC.md#set-up-the-dedicated-sync-app) records exact setup and failure behavior.
