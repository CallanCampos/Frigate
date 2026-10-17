# 0001: Repository and upstream sync

- **Status:** accepted; amended by [0002](0002-solo-project-no-upstream-posts.md)
- **Date:** 2026-10-16
- **Plan:** D1 and §4.1; §8, step 1
- **Upstream:** `462e90193` (merge of PR #345)

## Context

The user's requirement: "I want this repo, and the native port development, to stay up-to-date with the decomp as it
continues to get more decompiled. The port should be developed alongside the decomp."

- **Upstream moves fast.** `fuddlesworth/pokebw2` is a matching decompilation built with MWCC. Its maintainer merges
  pull requests from forks with merge commits, and `main` gains 150–250 commits a day. The port needs each new piece
  of C as it lands: the overworld waits for overlay 36, multiplayer for overlays 11 and 182 (PLAN.md §9).
- **Access.** The `CallanCampos` account can only pull from upstream. Decomp contributions already go through the
  fork `CallanCampos/pokebw2` and pull requests, and GitHub allows one fork of a repository per account.
- **Matching is checked only with ROMs.** `python3 configure.py && ninja` rebuilds both ROMs from the user's own dumps
  and checks both SHA1s. Upstream's CI fetches the ROMs from a private repository with a deploy key; no other
  repository has them, so outside upstream only a local build can check matching.
- **The port changes a few game files.** PLAN.md §4.1 allows three kinds of change outside `port/`: portability fixes
  that compile to the same bytes, hook macros that compile away, and rare `PLATFORM_PC` blocks.
- **Upstream's setup files change often.** The root `CLAUDE.md`, `AGENTS.md`, `README.md`, `CONTRIBUTING.md`,
  `.claude/` and the workflows are rewritten regularly; 14 commits changed `.github/workflows/` between 2026-10-04 and
  2026-10-15.
- **The port needs its own home**: issues, releases, CI on macOS (free on public repositories), and a landing page.
- **CI limits** ([SYNC.md](../SYNC.md#references)): `GITHUB_TOKEN` can't push changes to workflow files, and its pushes
  start no other workflow.

## Decision

1. **A new public repository, `CallanCampos/pokebw2-native`, created from upstream's full history.** It is upstream's
   tree plus `port/`. Its only other files are ones upstream doesn't have: `.github/README.md` (the landing page,
   which GitHub shows before the root `README.md`) and `.github/workflows/port-*.yml`. The port never edits an
   upstream file for its own setup; an upstream workflow that can't run here (`build.yml`, which needs the ROM deploy
   key) is turned off with `gh workflow disable`, not edited.
2. **Two checkouts on the user's machine.** The decomp checkout `~/dev/decomp/pokebw2` (`origin` upstream, `fork` the
   fork) is where decomp work happens. The port clone `~/dev/decomp/pokebw2-native` (`origin` this repository,
   `upstream` the decomp, fetch only) is where port work happens. `port/tools/bootstrap_checkout.sh` links the ROMs
   and tools into a port clone or worktree.
3. **Decomp work goes upstream.** Matching, new decomp files, config changes and overlay claims are made in the decomp
   checkout and sent upstream as fork pull requests. They reach this repository only by sync, never as commits here.
4. **Sync by merge.** Upstream `main` is merged into `main` with `--no-ff`, never rebased, squashed or copied, so
   history and authorship stay intact and the next merge starts from the last one. Each sync merge has the message
   `Merge upstream fuddlesworth/pokebw2@<short>` and an `Upstream-Commit: <sha>` trailer. Nobody force-pushes.
5. **Two sync paths.**
   - The hourly Action `port-sync.yml` merges upstream `main`, runs upstream's no-ROM compile check, and pushes to
     `main`. It pushes without a ROM build only while the tree outside the port's own files is exactly upstream's
     tree at the merged commit, which it checks on every merge: `main` is then exactly as good as that upstream
     commit. It records upstream's own CI results for the commit but doesn't wait for them or require them to pass.
     Upstream's code runs in a job with a read-only token; a second job, which runs none of it, repeats the merge
     and pushes it. A conflict, a compile failure, a change to upstream's workflows, game files that differ from
     upstream or a rewrite of upstream's history opens one `upstream-sync` issue instead, and the next clean sync
     closes it.
   - The local sync `port/tools/sync_upstream.sh` runs first in every port session. It does the same merge, builds
     both ROMs, checks both SHA1s, runs the host build and its tests, and pushes only when all of that passed. It is
     the fallback for everything the Action can't do, and every session's STATUS.md entry records its result.
6. **Conflict resolution.** Upstream's side wins: take upstream's version, then apply the port's change again as its
   [game-changes.md](../game-changes.md) row describes, at the code's new place if upstream moved it, or drop the
   change if upstream made it unnecessary. Never drop or rewrite upstream's matching work to keep a port change. The
   resolved merge must build both ROMs before it is pushed. A rewrite of upstream's history is the user's decision.
   [SYNC.md](../SYNC.md#when-the-upstream-sync-issue-opens) has the procedure.
7. **When the port first changes a game file**, the Action stops pushing to `main` by itself: its check of the tree
   outside `port/` fails, it pushes nothing, and its issue says that the switch to pull requests is due. After the
   switch, it proposes each merge as a pull request that a ROM-verified local sync supersedes
   ([SYNC.md](../SYNC.md#when-the-port-changes-game-files)). The switch gets its own ADR, superseding the push part
   of this one.

## Alternatives

- **A branch of the existing fork `CallanCampos/pokebw2`.** No new repository, but the fork already carries the decomp
  pull request branches, and GitHub allows only that one fork per account. Port issues and releases would mix with
  decomp work, the fork's `main` would stop being a mirror of upstream that decomp branches start from, and scheduled
  workflows are off by default in forks. Rejected.
- **The decomp as a git submodule of a port repository.** It keeps the trees apart, but the port's changes to game
  files would then need a branch of the fork for the submodule to point at: two repositories to keep in step, a
  submodule bump commit for every upstream advance (as often as the merges it replaces), and a ROM build that has to
  run inside the submodule. The decomp's tools and rules would not apply to the combined tree. Same-tree ports such as
  sm64-port and the Sonic Advance 2 port merge upstream instead. Rejected.
- **Putting `port/` in upstream now.** Upstream's rules and reviews are about matching, port work would slow them, and
  with pull-only access every port commit would be a pull request on someone else's project. It also needs the
  maintainer's agreement, which is a public post the user hasn't approved. Not now; `port/` can move over unchanged
  later if the maintainers want it.
- **Sync only by pull request.** The Action would open a pull request for each upstream advance and a person would
  merge it after a ROM build. That keeps `main` ROM-verified, but at upstream's pace it means a pull request needing
  attention every hour, and checks on pull requests opened by `GITHUB_TOKEN` wait for approval. While the tree outside
  the port's files is upstream's own, a direct push publishes exactly an upstream commit, which the next local sync
  checks anyway. Deferred: it becomes the mode once the port changes a game file (decision 7).
- **Rebasing the port onto upstream.** It would rewrite published history and need force-pushes. Rejected.
- **Copying or squash-merging upstream's files.** It loses history and authorship, and every later merge conflicts
  with the copies; Shipwright's edited snapshot of its decomp shows how such a tree drifts. Rejected.
- **A personal access token with the `workflow` scope for the Action.** It would let the Action push upstream's
  workflow changes, and its pushes would start `compile.yml` and `standards.yml`. But it puts a long-lived token that
  can rewrite workflows into a public repository's secrets, for a case the local sync already handles. Rejected for
  now.
- **A GitHub App for the Action's pushes.** An App installed only on this repository, with Contents and Workflows
  write, whose installation tokens the job makes per run with `actions/create-github-app-token`. The tokens last an
  hour and reach one repository, and their pushes start `compile.yml` and `standards.yml` (SYNC.md references 1 and
  13). The App's private key would sit in a repository secret, and the Action would then merge upstream's workflow
  changes automatically, where now a session's local sync pushes them and checks which workflows are enabled. Not
  now: it is the better option if upstream's workflow changes (5 of 12 days before 2026-10-16) delay the port too
  much.

## Consequences

- **Easier:** upstream's C reaches the port within about an hour of landing, with history, blame and authorship
  intact, except after an upstream workflow change or once the port changes a game file, when it waits for the next
  local sync. One tree builds both ROMs and the port, so the ROM build keeps proving that port changes leave the DS
  build alone, and the decomp's tools (`compiler_probe.py`, `show_func.py`, the progress tracker) work unchanged.
- **Harder:** the clone carries the decomp's full history. Upstream's root `CLAUDE.md` and skills are written for
  decomp work, so `port/CLAUDE.md` says how they apply here. Port changes to game files must stay few and documented,
  or every merge becomes a conflict.
- **Watch:** `main` can hold Action merges that no ROM build here has checked yet, of upstream commits whose ROM jobs
  upstream may not have finished or passed; the next session's sync checks them. Upstream workflow changes need a
  local sync, and a workflow that upstream adds is enabled here once pushed. GitHub disables the schedule after 60
  days without activity in the repository. In the port clone, the root `CLAUDE.md`'s "publish every commit" step
  (`git push origin HEAD:main`) publishes to this repository, as intended; `sync_upstream.sh` refuses to run in a
  checkout whose `origin` is upstream.
- **Revisit:** when the port first changes a game file (decision 7); if upstream rewrites its history; if upstream's
  maintainers want the port in-tree; or if the Action's minutes or notifications become a burden.
