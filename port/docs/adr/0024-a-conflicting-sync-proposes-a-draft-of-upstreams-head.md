<!-- Records why the hourly upstream sync opens a draft pull request of upstream's head when the merge conflicts,
instead of only failing its run, and how a maintainer's resolved merge turns that draft into the proposal. -->
# 0024: A conflicting sync proposes a draft of upstream's head

- **Status:** accepted
- **Date:** 2026-10-25
- **Plan:** D1, §4.1 and §8 (syncing with the decomp)
- **Upstream:** `629a53cccb` (upstream merge #504), as Frigate's `main` has it
- **Note (2026-10-25):** the owner asked for it and chose this design in the session that made the change. It amends
  [0018](0018-protected-main-and-sync-prs.md), whose hourly sync stopped its run on any conflict.

## Context

Under [0018](0018-protected-main-and-sync-prs.md), the hourly `port-sync` Action merges upstream into a branch made
from `main` and opens one frozen proposal. Git can't make a merge commit while files conflict, so a conflict ended the
run before anything was pushed, and its only trace was a failed Actions run. On 2026-10-23 and 24 one went unseen
for about 17 hours, from the first failed run (40042123242, 23:53Z) to the hand-made sync that resolved it (PR 27).
The owner asked that a conflicting sync still open a pull request, which a maintainer resolves by pushing to it.

Constraints from 0018 and [SYNC.md](../SYNC.md) still hold: the bot never resolves a conflict, chooses no side and
discards no port change; a sync merge is a real merge from `main` with its `Upstream-Commit` trailer, carrying only
the sync; the App has Contents and Pull requests write and nothing else, no Workflows permission among them; and a
pending proposal is never rewritten by the bot.

## Decision

After the three stops that already came first (a history rewrite, a cross-repository reference in an incoming
message, upstream workflow changes, now including a merge commit's own changes), the helper merges upstream into
`main` in Git's object store (`git merge-tree`, which changes no file or branch). If that conflicts, it merges nothing:
it pushes upstream's head, unchanged, to a fresh `codex/upstream-sync-*` branch and opens a **draft** PR listing the
conflicting files and the commands that resolve it. The draft is frozen like any proposal.

A maintainer resolves it with `sync_upstream.sh` on a branch of the draft's name made from `main`, as any local sync:
the merge of upstream's newest head, its conflicts resolved, both ROMs and the host checked. The draft's head is among
that merge's ancestors, so its push is a fast-forward of the draft's branch, and the publishing helper retitles the
PR, gives it the ordinary proposal's text and marks it ready for review. The script checks before merging that origin's
branch of that name is in what the push carries, and stops with exit 4 when upstream rewrote the draft's commits.

## Alternatives

- **Commit the merge with its conflict markers and open a PR of it.** The proposal would hold a merge commit that
  never builds and only looks resolved, and the resolution would come as a second commit, against SYNC.md's rule that
  a sync merge carries its own resolutions.
- **A placeholder commit on `main` to open a PR from.** GitHub needs a commit to open a PR, so the placeholder would
  stay in history once the resolution is pushed on top of it, and the PR wouldn't show the conflict.
- **Merge `main` into the draft's branch to resolve it**, as GitHub's Update branch does. The merge would have
  upstream as its first parent and no `Upstream-Commit` trailer, and the next sync's continuity check reads that
  trailer from merges whose second parent is upstream.
- **Move the draft to each newer upstream head.** It keeps the list current, but the bot would then rewrite a pending
  proposal, which 0018 rules out; the resolution merges upstream's newest head anyway.
- **An issue as the notice.** It needs the App's Issues permission, which it doesn't have, and a draft PR is also the
  place where the resolution lands. An issue would still suit the three stops, which push nothing.

## Consequences

- A conflict is a visible, frozen draft that can't be merged (a draft, with no PR checks, which GitHub doesn't run
  while a PR conflicts) and that the resolved merge fast-forwards. The PR keeps the App as its author, so the owner
  approves it as any App proposal.
- The draft's branch holds upstream's tree, without Frigate's own workflows and `port/`. Nothing in it runs: upstream's
  workflows run on pushes to `main` only, and the workflow stop, which now also compares upstream's tree and its
  merges' own changes, keeps each of them as it was in the last synced upstream commit. GitHub may still refuse the
  App's push of that branch, because GitHub doesn't document how it checks a new branch's workflow files against the
  App's missing Workflows permission. The run then fails with the conflicting files in its error, as before but naming
  them; the first real conflict shows which it is.
- The three stops are still visible only as a failed run. A notice for them needs the App's Issues permission and a
  `permission-issues: write` line in `port-sync.yml`, both the owner's call.
- Revisit if GitHub refuses the draft's push, or if the frozen draft's list goes stale often enough to matter.
