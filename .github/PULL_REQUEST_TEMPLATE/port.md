<!-- Port review template for Frigate's native platform and tests; see port/CONTRIBUTING.md. -->
## What changes

Describe the problem, resulting behavior and affected part of the game or host platform.

## Verification

List commands and results, OS/architecture, and skipped or unrun checks with reasons:

- T0: both ROM SHA1s (maintainer verification required before merge)
- T1: host build
- T2/T6: unit and save tests
- T3: relevant replays
- Additional checks, if applicable:

For visual changes, describe the comparison and hashes; do not upload captured game frames or game assets.

## Review checklist

- [ ] The change follows port/CONTRIBUTING.md and documents any portability change to game files.
- [ ] No ROMs, extracted game assets or saves are included.
- [ ] Remaining work and verification gaps are described above.

- [ ] Required CI and `Frigate local verification` pass on the final head (maintainer completes verification).
- [ ] Sync PRs retain merge history and document any port-conflict resolution against `port/docs/game-changes.md`.
