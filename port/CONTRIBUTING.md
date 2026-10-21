<!-- Contributor workflow for Frigate's host platform, tests and documentation; game decompilation stays upstream. -->
# Contributing to Frigate

Contributions to Frigate's native platform, builds, tests and documentation are welcome. Start with
[the current status](docs/STATUS.md), [the architecture](docs/ARCHITECTURE.md) and [the build guide](README.md).
For a substantial feature, open an issue here first to discuss its user benefit and scope.

## Fork, branch and submit

Fork [CallanCampos/Frigate](https://github.com/CallanCampos/Frigate), then clone your fork and create a topic branch:

```sh
git clone https://github.com/YOUR-USERNAME/Frigate.git
cd Frigate
git remote add frigate https://github.com/CallanCampos/Frigate.git
git switch -c your-topic
```

`origin` is your personal fork; `frigate` is the canonical port. Update from `frigate/main` before starting another
change. You do not need a remote for the decomp to contribute to the port. If you add one, keep it fetch-only.

Keep each change focused. Push your topic branch to your fork:

```sh
git push -u origin your-topic
```

Open [a port pull request](https://github.com/CallanCampos/Frigate/compare/main?expand=1&template=port.md), choose
**compare across forks**, select your fork and topic branch, and keep **base repository CallanCampos/Frigate,
base main**. If GitHub shows the inherited decomp checklist, use the
[port template](https://github.com/CallanCampos/Frigate/blob/main/.github/PULL_REQUEST_TEMPLATE/port.md) instead.
Describe the problem, the resulting behavior and your validation. A maintainer reviews the change; address feedback
on the same branch. Draft PRs are useful when implementation or verification remains unfinished.

## Build and verify

Install the dependencies in [Quick start](README.md#quick-start), or follow
[Building on Windows](README.md#building-on-windows). The host build needs no ROM:

```sh
cmake -S port -B build/port -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build/port
ctest --test-dir build/port --output-on-failure
```

Tests that need your ROM skip without it. Report skips and checks you could not run, including your OS and architecture.
See [the test guide](tests/README.md) for coverage and [the sanitized build](docs/sanitizers.md) for memory checks.
White 2 uses a separate build directory with `-DPORT_VERSION=w2_us`, as the quick start describes.

With both personal ROM dumps and the decomp toolchain prepared as in the repository's root README, run
`python3 configure.py && ninja`: both Black 2 and White 2 must match their SHA1s (T0). Report the host build (T1),
unit/save tests (T2/T6), relevant replay tests (T3) and any additional checks you ran, with their commands and results.
A contributor without ROMs can submit a draft PR and state that T0 and ROM-dependent tests were not run. Before
merging, a maintainer must verify both ROM SHA1s and the applicable host tests on the final change.

`port/tools/sync_upstream.sh` and the hourly sync are maintainer tools. Do not run the sync script in a personal fork:
it expects `origin` to be the canonical repository and publishes to its `main`. Maintainers handle decomp sync and
verification; contributors take updates from Frigate's `main`. The maintainer checkout preparation and agent hooks in
[Preparing a checkout](README.md#preparing-a-checkout) are optional local setup, not prerequisites for a host build.

## Scope and evidence

Port code lives in `port/`; the contribution entry points and `port-*.yml` workflows are also owned by Frigate.
Keep upstream's root README, CONTRIBUTING, decomp configuration and templates intact. A necessary game portability
fix must preserve both ROMs and be recorded in [game-changes.md](docs/game-changes.md). Missing game C and matching
work belong to the separate decomp project, following its contribution rules; they reach Frigate through sync.

Open a [port bug report](https://github.com/CallanCampos/Frigate/issues/new?template=port_bug.yml) with the commit,
build command, OS/architecture, reproduction steps and a short relevant log. Feature requests should explain the
player or contributor benefit and proposed scope. Never upload ROMs, extracted game assets, captured game frames or
saves; describe visual changes and test results in text or hashes. Frigate distributes no ROM or game assets.

Keep port discussion here. Do not post about Frigate on the decomp repository, link other repositories' issues,
pull requests or discussions in port posts or commit messages, or mention upstream contributors with an @-mention.
Use bare issue numbers for this repository's issues. These boundaries preserve the independent projects' workflows
([ADR 0017](docs/adr/0017-external-contributions.md)).
