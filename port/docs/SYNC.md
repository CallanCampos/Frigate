# Keeping the port in sync with the decomp

This repository is `fuddlesworth/pokebw2` (upstream) with its full history, plus `port/`. The port is developed
alongside the decomp, so upstream's `main` is merged into ours continuously: each function upstream turns into C
reaches the port at the next sync. Two things do the merging:

- **The hourly Action** ([`.github/workflows/port-sync.yml`](../../.github/workflows/port-sync.yml)) merges upstream
  `main`, checks that every C file still compiles, and pushes while the tree outside the port's own files is
  upstream's. It has no ROMs.
- **The local sync** ([`port/tools/sync_upstream.sh`](../tools/sync_upstream.sh)) does the same merge on a machine
  with the base ROMs, builds both ROMs, checks both SHA1s, runs the host build and its tests, and pushes. Every port
  session runs it first, and it is the fallback for everything the Action can't do.

Why the repository is set up this way, and what else was considered, is
[ADR 0001](adr/0001-repository-and-upstream-sync.md).

## The rule: decomp work goes upstream

Decompilation and matching, new decomp source files, config changes and overlay claims happen in the decomp checkout
(`~/dev/decomp/pokebw2`, whose `origin` is upstream and `fork` is `CallanCampos/pokebw2`). They go to upstream as pull
requests from the fork, and they reach this repository only by sync. Never commit decomp work here directly: it would
bypass upstream's review and conflict with upstream's own version of the same change at the next sync.

The port changes files outside `port/` only in the three ways of PLAN.md §4.1, each listed in
[game-changes.md](game-changes.md). It never edits upstream's own setup files (root `CLAUDE.md`, `README.md`,
`.gitignore`, `.claude/`, upstream's workflows); its own files outside `port/` are `.github/README.md` and
`.github/workflows/port-*.yml`, which upstream doesn't have.

## The hourly Action

Every hour at minute 23, and on demand with `gh workflow run port-sync.yml -R CallanCampos/Frigate`, in that
repository only. Its first job, `merge`, has a read-only token and keeps no credentials in its checkout:

1. Checks out `main` with full history and fetches upstream `main`.
2. Finds the upstream commit merged last, from the newest `Upstream-Commit:` trailer (see below), and stops if
   upstream `main` no longer contains it: upstream rewrote its history. This comes before step 3, so that upstream
   going back to an older commit is caught too.
3. Stops if `main` already contains upstream `main`.
4. Merges upstream `main` as `github-actions[bot]`, with the message `Merge upstream fuddlesworth/pokebw2@<short>` and
   the trailer. On a conflict it aborts the merge.
5. Stops if the merge brings changes to `.github/workflows/`, which its token can't push.
6. Stops if any file outside the port's own files differs from upstream `main` at the merged commit (see
   [When the port changes game files](#when-the-port-changes-game-files)).
7. Runs upstream's no-ROM compile check, `python tools/decomp/compile_all.py`, with the same Python and tools cache
   as `compile.yml`. It compiles every C file for both versions.

The second job, `publish`, holds the write token and runs none of upstream's code:

8. Makes the same merge on the current `main` and pushes it if the tree outside `port/` is the one that compiled and
   no commit that the push adds names an issue in another repository (see
   [Commit messages that name other repositories](#commit-messages-that-name-other-repositories)). If `main` moved
   meanwhile only in `port/`, it merges onto the new `main` and tries again, up to three attempts. If `main` changed
   outside `port/`, it pushes nothing and leaves the merge to the next run, which compiles again.
9. Writes a job summary: the upstream commits before and after, the conclusions of upstream's own CI for the merged
   commit, the number merged, the port commits before and after, whether the tree outside `port/` is upstream's, and
   the short SHAs of commits that name issues in other repositories or carry a closing keyword.
10. Opens or updates one issue labeled `upstream-sync` when it is blocked, and closes it at the next sync that gets
    through, naming the merged upstream commit.

| Outcome | Pushed | Issue | Job |
| --- | --- | --- | --- |
| Nothing new upstream | no | closed if open, unless files outside `port/` still differ | success |
| Merged | yes | closed if open | success |
| `main` or upstream changed during the run | no | unchanged | success with a notice; the next run merges again |
| Conflict | no | opened or updated | fails when the blocker is new, then succeeds with a warning |
| Compile failure | no | opened or updated | fails when the blocker is new, then succeeds with a warning |
| Upstream history rewritten | no | opened or updated | fails when the blocker is new, then succeeds with a warning |
| Workflow files changed upstream | no | opened or updated | succeeds with a warning |
| Files outside `port/` differ from upstream | no | opened or updated, kept open | succeeds with a warning |
| Commits name issues in other repositories | no | opened or updated | fails when the blocker is new, then succeeds with a warning |
| Push refused although `main` didn't move | no | opened or updated | fails |

A failed scheduled run notifies the person who added the schedule (or last changed its cron line), so a run fails
only when a blocker is new or changes; while the same blocker stays, the run updates the issue quietly. A compile
failure counts as new when the set of failing files changes. A workflow change upstream is an expected limit of the
token, and differing game files are the port's own doing, so neither fails the job: the issue, whose creation
notifies, is the signal.

### Its limits

- **No ROMs in CI.** The ROMs are the user's own dumps and are never committed or uploaded, so the Action can't build
  them. It checks that the code compiles; matching is checked by the local sync.
- **It can't push workflow changes.** `GITHUB_TOKEN` is a GitHub App installation token [1]. Changing files in
  `.github/workflows/` needs the GitHub App "Workflows" permission [2], and `GITHUB_TOKEN`'s `permissions` block has
  no such key [3], so GitHub refuses those pushes. Upstream changes its workflows often (14 commits, on 5 of the 12
  days before 2026-10-16). Each time, the issue asks for a local sync, whose push uses your own credentials, and
  upstream's C waits for it.
- **Its pushes start no other workflow.** Events caused by `GITHUB_TOKEN` don't create workflow runs, except
  `workflow_dispatch` and `repository_dispatch` [1], so `compile.yml` and `standards.yml` don't run on its pushes.
  That is why the compile check runs inside the sync workflow. Pushes from the local sync do start them.
- **Upstream's code runs without the write token.** The compile check runs upstream's `compile_all.py`, and the
  compilers that `configure.py` downloads, from a host outside GitHub, without a hash check. Code in a job can read
  that job's token, so the `merge` job gets a read-only token and `persist-credentials: false` [9][12]. The `publish`
  job, which can push and edit issues, runs only `git` and `gh`, and treats the `merge` job's outputs as untrusted:
  it checks their form, makes the merge itself, and pushes only a tree whose fingerprint outside `port/` matches the
  one that compiled.
- **The schedule is approximate.** Scheduled runs use the latest commit on the default branch, can be delayed under
  load, especially at the start of each hour [4], hence minute 23. Runs never overlap: the workflow has one
  concurrency group, and a newer pending run replaces an older pending one [3].
- **It can be switched off by inactivity.** In a public repository, GitHub disables scheduled workflows after 60 days
  without repository activity [4][5]. The Action's own pushes are commits, so this should only happen if upstream
  stops for two months, or while the Action has stopped pushing and no session syncs. Turn it back on with
  `gh workflow enable port-sync.yml -R CallanCampos/Frigate`.
- **Forks don't run it.** The jobs run only when the repository is `CallanCampos/Frigate`, and GitHub
  disables scheduled workflows in forks of public repositories by default anyway [5].
- **Upstream's `build.yml` is disabled here** (`gh workflow disable build.yml -R CallanCampos/Frigate`),
  since it fetches the ROMs with a deploy key that only upstream has. Disabling, not editing, keeps the file identical
  to upstream's. The disabled state belongs to that one file: a workflow that upstream adds, or `build.yml` under a
  new name, is enabled here as soon as a sync pushes it, and if it needs upstream's secrets it fails on every push.
  The Action's issue lists the workflow files a merge adds or renames and those that use secrets other than
  `GITHUB_TOKEN`.
- **gh commands name the repository.** gh prefers a remote named `upstream` to `origin`, so every `gh` command here
  passes `-R CallanCampos/Frigate` (the workflow passes `--repo` and sets `GH_REPO`).

## The local sync

Every port session starts with it (PLAN.md §8, step 1):

```sh
port/tools/sync_upstream.sh            # from anywhere in the port clone or one of its worktrees
```

It refuses to run when a URL of `origin` (fetch or push) isn't `CallanCampos/Frigate` as `https://github.com/...` or
`git@github.com:...`, with or without `.git` (so not the decomp or its fork `CallanCampos/pokebw2`), when tracked files
have changes, or when a merge is in progress; it never stashes. A clone made before the repository's rename on
2026-10-18 still has the old name in `origin`'s URL, which GitHub redirects: the script refuses it too and prints the
command that updates it, such as `git remote set-url origin https://github.com/CallanCampos/Frigate.git`. It adds the
`upstream` remote when it is missing, and sets that remote's push URL to `DISABLED-port-never-pushes-upstream` whenever
it is anything else, as `bootstrap_checkout.sh` does, so that a push there fails while fetching still works. It
fast-forwards to `origin/main`, checks for a rewrite of upstream's history, checks commit messages for issues in other
repositories (see [below](#commit-messages-that-name-other-repositories)), merges upstream `main` with the same message
and trailer as the Action, runs `python3 configure.py b2_us w2_us` and `ninja` (once more if ninja exits 139, wibo's
intermittent crash), checks both ROM files against `b2_us.sha1` and `w2_us.sha1`, runs the host build and `ctest` once
`port/CMakeLists.txt` exists, and pushes to `origin main` only when all of that passed. A refused push is fetched,
merged again, rebuilt only where the new commits change something, and retried, three attempts in all; it never
force-pushes. At the end it prints the fields of a [STATUS.md](STATUS.md) entry that it knows (upstream and port
commits, ROM, host build, tests, and any upstream workflow files the push changes); the session adds the rest and
commits the entry.

| Option | Effect |
| --- | --- |
| `--branch NAME` | Sync and publish `NAME` instead of `main`. |
| `--no-build` | Merge only: no ROM build, no host build, and nothing pushed. |
| `--no-push` | Build and check, but don't publish. |
| `--metrics` | Also measure the decomp with `port/tools/progress/run_all.sh`. |
| `--abort-on-conflict` | Abort a conflicting merge instead of leaving it in progress. |
| `--allow-rewrite` | Merge although upstream rewrote its history (see below). |
| `--allow-references` | Merge and push although commit messages name issues in other repositories (see below). |

With `--branch`, a worktree on a branch of its own is brought up to `origin/NAME` and pushed there. `--metrics` writes
to `build/port-metrics`, takes about 5 minutes and needs `extract/`. The script publishes nothing it didn't build:
`--no-build` never pushes, and no option pushes an unbuilt merge. Like `--allow-rewrite`, `--allow-references` is the
user's decision, never a session's.

Exit status: 0 done, 1 failed, 2 bad usage or an `origin` that isn't the port repository, 3 merge conflict, 4 upstream
history rewritten, 5 commit messages name issues in other repositories.

A new clone or worktree needs the ROMs and tools first. `port/tools/bootstrap_checkout.sh [DECOMP_CHECKOUT]` works
from the decomp checkout (default `~/dev/decomp/pokebw2`) and never replaces an existing file. It links `orig/`,
`.venv`, `tools/wibo`, `tools/sjiswrap.exe` and `tools/objdiff-cli`. It copies `tools/mwccarm`, because
`configure.py` extracts every compiler again when one is missing, which through a link would rewrite the decomp
checkout's compilers. It leaves `tools/dsd` to `configure.py`, which keeps a `tools/dsd` without `tools/dsd.rev`
whatever its version, so a link would never follow upstream's pinned version. It also adds `/orig` and
`/CLAUDE.local.md` to the repository's `info/exclude` and prints the next commands. The push of a merge that carries
workflow changes needs a credential with the `workflow` scope: `gh auth status` shows the token's scopes, and
`gh auth setup-git` makes git use it.

## When the upstream-sync issue opens

The issue says what blocks the sync and what to run. The fix is a local sync, or waiting for upstream; never an edit
to an upstream file made only so that the Action's merge passes.

- **Workflow files changed upstream.** Run `port/tools/sync_upstream.sh`. Then run
  `gh workflow list --all -R CallanCampos/Frigate`: `build.yml` must still be disabled. For a workflow that the
  issue lists as added or renamed, or as using secrets other than `GITHUB_TOKEN`, ask the user before disabling it:
  `port/CLAUDE.md` allows only `build.yml` to be disabled without asking.
- **Compile failure.** If upstream `main` itself doesn't compile, wait: the next run after upstream's fix gets
  through. If a port change outside `port/` broke it, fix that change in a port commit; the local sync shows the
  errors.
- **Conflict.** Run `port/tools/sync_upstream.sh`. It stops with the merge in progress. Resolve the files as below,
  then `git add` them, `git commit --no-edit --cleanup=strip` and run the script again; it builds, checks and pushes.
  Without `--cleanup=strip`, git keeps the `# Conflicts:` lines of its prepared message in the commit.
- **Files outside `port/` differ from upstream.** The Action has stopped pushing; see
  [When the port changes game files](#when-the-port-changes-game-files). Each session's local sync publishes
  upstream's changes meanwhile, and the switch described there is due.
- **Upstream history rewritten.** See below.
- **Commits name issues in other repositories.** Ask the user; see
  [Commit messages that name other repositories](#commit-messages-that-name-other-repositories).
- **Push refused.** The issue quotes git's message. Usually branch protection or a token problem; fix that, then run
  the workflow again with `gh workflow run port-sync.yml -R CallanCampos/Frigate`.

The next hourly run closes the issue once `main` contains upstream `main`, or for differing files, once none differs.

### Resolving conflicts

Conflicts can only come from the port's changes outside `port/`, which upstream doesn't know about, or from upstream
adding a file the port also has (`.github/README.md`, a `port-*.yml` workflow), which the port then renames.

1. Upstream's side wins. Take upstream's version of the file, then apply the port's change to it again, in the same
   form, as the file's [game-changes.md](game-changes.md) row describes. Never drop or rewrite upstream's matching
   work to keep a port change.
2. If upstream moved or renamed the code, apply the change at its new place and update the row.
3. If upstream made the change unnecessary (for example by adopting the same portability fix), drop the port's
   change and its row.
4. The resolved merge must still build both ROMs with both SHA1s, which the script checks before it pushes.

### Upstream rewrote its history

Upstream force-pushing `main` is unlikely (it merges pull requests from forks). If it happens, upstream `main` no
longer contains the last upstream commit recorded here, and a merge would bring in the rewritten commits beside the
old ones. Both tools stop, and so does the session: this is the user's decision. Compare the two histories
(`git range-diff OLD...upstream/main`, with `OLD` from the issue or the script's message) and report what changed.

- If the rewrite only removed or reworded commits, the usual answer is to merge with
  `port/tools/sync_upstream.sh --allow-rewrite`, resolving any conflict in favor of upstream's new content. The new
  trailer records the new tip, and later syncs are normal again. The old commits stay in this repository's history;
  nobody force-pushes to remove them.
- If the rewrite removed something on purpose that must not be published (a ROM file, a secret), say so first: the
  old commits are still public here, and only the user can decide what to do about that.

Both tools check for a rewrite before anything else, so they also catch upstream going back to an older commit, the
usual way to undo an accidental push, although `main` then already contains upstream `main`. There is nothing to
merge until upstream moves on, so both tools keep reporting the rewrite until then; once upstream has new commits,
`--allow-rewrite` makes the merge that records the new trailer.

## Commit messages that name other repositories

The port is a solo project, and nothing about it may appear on `fuddlesworth/pokebw2`
([ADR 0002](adr/0002-solo-project-no-upstream-posts.md)). Upstream's commit messages sometimes name an issue in
another repository, as `OWNER/REPO#N` or its URL: three in the history up to 2026-10-17, two naming the same pull
request of `AetiasHax/ds-decomp` and one an issue of upstream itself. When a commit whose message names an issue is
pushed, GitHub records a `referenced` event on that issue, naming the repository the commit was pushed to [14]. A sync
that pushes such a commit here could then show this repository on that issue's timeline. The first push of the history
created no event from this repository (checked on both timelines on 2026-10-17), but GitHub doesn't document why, so
the tools don't rely on it.

The repository's visibility doesn't change that, only what the reference shows. GitHub's docs say that people who
can't read a private repository "will see references that contain a minimum of information" [15]: the reference still
appears, without its details. Since the details depend on the reader's access to this repository, references recorded
while it is private would show in full to everyone if it were made public again.

So both tools, whatever this repository's visibility, check the messages of the commits that a push would add:

- They look for `[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+#[0-9]+` and
  `github\.com/[^/[:space:]]+/[^/[:space:]]+/(issues|pull|discussions)/[0-9]+`, ignoring case, the forms GitHub
  links [17]. They don't tell repositories apart, so a port commit message names the port's own issues as `#N`.
- The Action checks exactly the commits its push adds to `main`. The local sync checks the new upstream commits and
  the clone's own unpublished ones, before it merges and again before each retry.
- On a match, nothing is merged or pushed. Both record only the commits' short SHAs, oldest first, never the text
  that matched, because everyone who can read this repository can read the Action's logs, summary and issue. Read a
  message locally with `git show -s SHA`.
- The Action reports `references`. Its issue lists the SHAs and is keyed by the oldest one, so the run fails once and
  then only warns while upstream adds commits on top. The local sync exits 5.

The user decides what happens next; a session never does. A merge can't leave out one upstream commit, so until the
user decides, upstream's later commits wait too. To accept that the reference may appear on the item's timeline, run
`port/tools/sync_upstream.sh --allow-references`: it merges, builds, checks and pushes as usual, and the next Action
run closes the issue.

Making the repository private is not a way around the check: the reference still appears, without its details, and
the check still runs. It would also reverse the choice of a public repository in
[PLAN.md §4.1](PLAN.md#41-repository) and [ADR 0001](adr/0001-repository-and-upstream-sync.md), made because GitHub
Actions minutes are free only for public repositories [18]. A private repository uses the account's minutes: 2,000 a
month on GitHub Free. GitHub rounds each job up to a whole minute [19], and each hourly run is two jobs, so the sync
alone uses about 1,500 minutes a month before any compile check, and upstream's Compile and Standards workflows also
run here on pull requests and on every push to `main` that isn't the Action's. Without a payment method, GitHub blocks
Actions once the minutes are used up [18], which would stop the hourly sync partway through a month. Such a switch
needs its own ADR, which would also have to slow the schedule or set up billing.

Text that the Action copies from git, the compiler or upstream's CI into its issue and summary (file names, the end
of the compile log, git's push message, the names of upstream's checks) is in code blocks, but GitHub's docs don't say
whether references in code blocks link [17]. So the Action also breaks them up there: `OWNER/REPO#N` gets a
fullwidth number sign (`＃`), `github.com/OWNER/REPO` becomes `github[.]com/OWNER/REPO`, and `@name` gets a fullwidth
at sign (`＠`), so that no one upstream is mentioned either.

### Closing keywords

A commit message with a closing keyword and an issue number of its own repository ("fixes #3", "Closes GH-3") closes
that issue once the commit is on the default branch [16]. Pushed here, an upstream message closes the port's own
issue with that number instead, which is harmless upstream. No commit in the history up to 2026-10-17 has one:
upstream's closing keywords are in pull request descriptions, which its merge commits don't copy. So the tools only
warn, with the short SHAs: the local sync prints a warning, and the Action adds a row to its summary. Reopen a port
issue that one closed; the `upstream-sync` issue needs nothing, since the next blocked run opens a new one. A bare
`#N` without a keyword, as in every "Merge pull request #N" commit, can add a reference to the port's own issue or
pull request `#N`, which stays inside this repository.

## The Upstream-Commit trailer

Every sync merge, from either tool, has the message

```text
Merge upstream fuddlesworth/pokebw2@<9-character SHA>

Upstream-Commit: <40-character SHA>
```

The trailer records exactly which upstream commit was merged, so `git log --grep=Upstream-Commit` lists the syncs,
and the tools can tell a normal upstream advance from a rewrite. When no merge has a trailer yet, the tools use the
merge base. A merge of upstream made by hand should keep the convention:

```sh
git merge --no-ff -m "Merge upstream fuddlesworth/pokebw2@$(git rev-parse --short=9 upstream/main)" \
    -m "Upstream-Commit: $(git rev-parse upstream/main)" upstream/main
```

## What the Action's pushes are checked against

The Action pushes merges that no ROM build here has checked. It does so only while the tree outside the port's own
files is exactly upstream's `main` at the merged commit, which it checks on every merge, and the ROM build reads
nothing in `port/`. The port's `main` is then exactly as good as that upstream commit, and no better.

Upstream's CI builds each push to its `main` with both ROMs, but the Action neither waits for those jobs nor requires
them to pass; it only records their conclusions in its summary. A run's overall result is not a clean signal: of
the last 60 `build.yml` runs on upstream `main` before 2026-10-17, 30 failed, and in the eight newest, all failures,
only the job "Build both ROMs on macOS" failed (the newest on compiler warnings), while "Build b2_us" and
"Build w2_us" passed. An upstream commit that breaks the ROM build reaches `main` here just as it reaches a local
sync, and the next session's sync finds it; `port/CLAUDE.md` says how to handle an upstream `main` that fails
`ninja`. Matching is verified by every session's local sync and recorded in [STATUS.md](STATUS.md).

## When the port changes game files

Once the port changes files outside `port/` (the first row in [game-changes.md](game-changes.md)), a clean textual
merge can still break matching, for example when upstream rewrites a function that carries a port portability fix.
From then on, `main` should only receive ROM-verified merges. The Action enforces the first half by itself: its
`merge` job finds files outside `port/` that differ from upstream, reports `needs-rom-check`, and nothing is pushed;
the issue stays open while the files differ. Until the switch below, `main` gets upstream's changes only from the
local syncs, so the port follows upstream only as often as sessions run. The switch is the next port task, in which
the Action proposes merges instead of pushing them:

1. Add `pull-requests: write` to the `publish` job's permissions.
2. Instead of stopping at differing files, merge upstream into the branch `sync/upstream` (created from `main` when
   missing) and push that branch, fast-forward only, never forced. The reference check covers that push too, with the
   range it adds to `sync/upstream`.
3. Open or update one pull request from `sync/upstream` to `main`, with the same summary as the job. Runs of
   `compile.yml` on a pull request that `GITHUB_TOKEN` opened wait for approval [1], so the compile check stays inside
   the sync workflow.
4. A port session's local sync merges upstream into `main` with the ROMs and pushes it. The next Action run finds
   upstream already merged, closes the pull request with a comment naming the merged commit, and deletes the branch.

The issue handling stays as it is. A self-hosted runner with the ROMs could verify merges in CI instead, but GitHub
recommends self-hosted runners only for private repositories, since pull requests from forks of a public one could
run code on them [11]. Record the switch in a new ADR that supersedes the push part of
[ADR 0001](adr/0001-repository-and-upstream-sync.md).

## References

Checked on 2026-10-16; 14 to 19 on 2026-10-17.

1. GitHub Docs, [GITHUB_TOKEN](https://docs.github.com/en/actions/concepts/security/github_token): an installation
   access token; events it triggers create no workflow runs, except `workflow_dispatch` and `repository_dispatch`;
   runs for pull requests it creates or updates need approval, which a GitHub App installation token or a personal
   access token avoids.
2. GitHub Docs, [Choosing permissions for a GitHub App](https://docs.github.com/en/apps/creating-github-apps/registering-a-github-app/choosing-permissions-for-a-github-app)
   and [Permissions required for GitHub Apps](https://docs.github.com/en/rest/authentication/permissions-required-for-github-apps):
   editing `.github/workflows` needs the "Workflows" permission; creating labels and issues needs "Issues" write.
3. GitHub Docs, [Workflow syntax](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax):
   the `permissions` keys (no `workflows`), unlisted permissions set to `none`, and `concurrency`.
4. GitHub Docs, [Events that trigger workflows: schedule](https://docs.github.com/en/actions/reference/workflows-and-actions/events-that-trigger-workflows#schedule):
   default branch only, delays at high load including the start of every hour, a 5-minute minimum interval, and the
   60-day inactivity rule for public repositories.
5. GitHub Docs, [Disabling and enabling a workflow](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/disable-and-enable-workflows):
   the 60-day rule, scheduled workflows disabled in forks of public repositories, `gh workflow disable` and `enable`.
6. GitHub Docs, [Using GitHub CLI in workflows](https://docs.github.com/en/actions/how-tos/write-workflows/choose-what-workflows-do/use-github-cli):
   `gh` is preinstalled on GitHub-hosted runners and reads `GH_TOKEN`.
7. GitHub Docs, [Notifications for workflow runs](https://docs.github.com/en/actions/concepts/workflows-and-actions/notifications-for-workflow-runs):
   scheduled runs notify the user who created the workflow or last changed its cron line.
8. GitHub Docs, [Adding a job summary](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-commands#adding-a-job-summary).
9. [actions/checkout](https://github.com/actions/checkout): `fetch-depth: 0` fetches all history; since v6 the token
   is kept in a separate file and `git push` in later steps still works, unless `persist-credentials: false`; the
   `github-actions[bot]` identity.
10. GitHub CLI manual: [gh label create](https://cli.github.com/manual/gh_label_create) (`--force` updates an existing
    label), [gh issue create](https://cli.github.com/manual/gh_issue_create),
    [gh issue close](https://cli.github.com/manual/gh_issue_close) (`--comment`); checked against `gh` 2.96.0.
11. GitHub Docs, [Adding self-hosted runners](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners):
    the warning against self-hosted runners on public repositories.
12. GitHub Docs, [Secure use reference](https://docs.github.com/en/actions/reference/security/secure-use): a read-only
    default for `GITHUB_TOKEN`, raised only for the jobs that need more.
13. [actions/create-github-app-token](https://github.com/actions/create-github-app-token): makes a GitHub App
    installation token for the current repository; it expires after one hour.
14. GitHub Docs, [Issue event types](https://docs.github.com/en/rest/using-the-rest-api/issue-event-types): the
    `referenced` event, "The issue was referenced from a commit message", whose `commit_repository` "is where that
    commit was pushed".
15. GitHub Docs, [Creating an issues-only repository](https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-an-issues-only-repository):
    references from a private repository to issues in a public one, which people without access see with "a minimum
    of information" (the reference still appears, without its details); a commit with a closing keyword pushed to the
    private repository's default branch by someone with write access to both closes the public issue.
16. GitHub Docs, [Linking a pull request to an issue](https://docs.github.com/en/issues/tracking-your-work-with-issues/using-issues/linking-a-pull-request-to-an-issue):
    the closing keywords (close, closes, closed, fix, fixes, fixed, resolve, resolves, resolved), in pull request
    descriptions and commit messages; the issue closes when the commit is merged into the default branch.
17. GitHub Docs, [Autolinked references and URLs](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/autolinked-references-and-urls):
    the forms `#26`, `GH-26`, `OWNER/REPO#26` and issue URLs; "references generate a backlink"; autolinked
    references are not created in files in a repository. It says nothing about code blocks.
18. GitHub Docs, [GitHub Actions billing](https://docs.github.com/en/billing/concepts/product-billing/github-actions):
    free for public repositories on standard GitHub-hosted runners; 2,000 minutes a month on GitHub Free and 3,000 on
    GitHub Pro; without a valid payment method, usage is blocked once the quota is used up.
19. GitHub Docs, [Actions runner pricing](https://docs.github.com/en/billing/reference/actions-runner-pricing): each
    job's minutes and partial minutes are rounded up to the nearest whole minute.
