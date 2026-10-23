<!-- Records how external contributors submit Frigate platform changes while preserving decomp sync boundaries. -->
# 0017: External contributions through forks and pull requests

- **Status:** accepted; amended by [0018](0018-protected-main-and-sync-prs.md) (PR-only publication and protection)
- **Date:** 2026-10-21
- **Plan:** D1, D10, §9; amends ADR 0002's solo-project scope
- **Upstream:** `83515e20ce`

## Context

Frigate is public and the user wants outside contributions. The inherited root contribution guide and templates
serve the decomp, while the port's checkout and sync tools assume a maintainer who publishes directly to main.
ADR 0002's solo-project description no longer describes contribution policy; its no-upstream-posts boundary remains.

## Primary-source basis

Observed on 2026-10-21: Shipwright's default branch is `develop`; its official
[development section](https://github.com/HarbourMasters/Shipwright/blob/develop/README.md#development) describes
CI builds from that branch. No `CONTRIBUTING` file was found at its root, `.github` or `docs`, so this is evidence
of its development branch, not a claimed contribution policy. Platinum's PC fork defaults to `pc_port`; its
[forking guidelines](https://github.com/cybervisi0n/pokeplatinum/blob/pc_port/README.md#forking-guidelines) welcome
forks. Its [pull-request guide](https://github.com/cybervisi0n/pokeplatinum/blob/pc_port/CONTRIBUTING.md#how-do-i-submit-a-pull-request)
describes a fork and topic-branch workflow, but still identifies pret and addresses the decompilation project.
These observations informed Frigate's separate contributor guide; Frigate chooses its own `main` target below.

## Decision

External contributors fork Frigate, work on topic branches and submit pull requests to CallanCampos/Frigate:main.
[port/CONTRIBUTING.md](../../CONTRIBUTING.md) documents this workflow and the existing build/test commands.
Frigate owns its new .github contribution entry point and port-specific issue/PR templates. Upstream's root
README, CONTRIBUTING and existing templates remain intact. No develop branch is introduced.

Maintainers own upstream sync and final merge verification. Contributors update from Frigate's main and do not run
sync_upstream.sh, which expects the canonical origin and publishes there. Contributors without ROMs can report the
checks they could not run in a draft PR; both ROM SHA1s and applicable host tests must pass before merge.

Human contributors, Claude Code and explicitly authorized Codex sessions can work on the port under port/CLAUDE.md.
Port work still makes no posts, references or notifications on upstream. Every other part of ADR 0002 remains in force.

## Alternatives

- Keep only the inherited decomp guide: it sends port contributors to the wrong workflow.
- Introduce develop: it adds another integration branch without an established release need.
- Require outsiders to use maintainer sync tools: those tools publish canonical main and deliberately reject forks.

## Consequences

New contributors have a dedicated guide and templates without changing synced upstream files. Existing PR CI checks
host builds without ROMs; maintainers remain responsible for full ROM verification. This decision changes no workflow
enablement, repository permissions or branch rules. The maintainer direct-publication and hourly-sync paths remain.
