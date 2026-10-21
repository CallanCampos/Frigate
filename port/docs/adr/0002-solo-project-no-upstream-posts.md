# 0002: A solo project with no upstream posts

- **Status:** accepted; contribution scope amended by [ADR 0017](0017-external-contributions.md)
- **Date:** 2026-10-17
- **Plan:** D10, §9
- **Upstream:** `462e90193` (merge of PR #345)

## Context

The plan proposed offering the port's matching-neutral portability conventions (`include/platform.h`, the `uptr`
typedef, size asserts, SDK constants for raw addresses, an "equivalent" column for nonmatching rows, no-op hook macros)
to `fuddlesworth/pokebw2`, and reporting decomp bugs the port finds. The user decided against it: "Do not make a post
or anything about this pc port on the pokebw2 repo, this port is a solo project for us and that repo shouldn't need to
know about it."

## Decision

- Nothing about the port is posted on `fuddlesworth/pokebw2`: no issues, comments, reactions, pull requests,
  discussions or proposals. Read-only access (fetching, `gh` reads, the sync Action's check-run query) continues.
- Decomp work that the port needs is done by decomp sessions in the decomp checkout as ordinary decomp work, under the
  root `CLAUDE.md`, and what they post never mentions the port.
- The port's portability fixes and hook macros stay in this repository, each with a row in
  [game-changes.md](../game-changes.md).
- Decomp bugs that the port finds (unlinked C that behaves differently from the ROM) are recorded in
  `port/docs/decomp-findings.md`, and so are the equivalence notes of PLAN.md §6 (the "equivalent: yes/no/unknown"
  note for each nonmatching row the port checks). The user decides whether a decomp session fixes a finding, as
  ordinary decomp work.
- Port issues, pull requests, comments and commit messages never link upstream issues or pull requests
  (`fuddlesworth/pokebw2#NNN` or their URLs), because GitHub shows such links on the target's timeline. Plain `#NNN`
  text inside committed files and links to upstream commits are fine.
- Port commit messages name only the port's own issues, as a bare `#N`, and port issues, pull requests and comments
  should too. They carry no `OWNER/REPO#N` and no issue, pull request or discussion URL of any repository, a third
  party's included; a third-party issue is named by its title. A pushed commit that names one records the "referenced"
  event described below on that issue, and the syncs' check doesn't tell repositories apart, so an unpublished port
  commit that has one also stops the local sync. If the user wants third-party references allowed, the check must be
  narrowed to match, so that the rules and the check agree.
- They never @-mention upstream's maintainers or contributors either (for example fuddlesworth or mariano54), because
  a mention can notify the person mentioned.
- Neither sync pushes commits whose messages reference an issue or pull request
  (`OWNER/REPO#N`, or an issue, pull request or discussion URL). Upstream's own commit messages sometimes do
  (`26cd037c8` names an upstream issue, `aedcd641d` and `9b6e4334b` a pull request of another repository), and a push
  of such a commit can record a "referenced" event, naming the repository pushed to, on that issue. The initial push
  of upstream's history here created no such event on either timeline, which fits GitHub recording a reference once
  per commit (upstream had pushed the same commits first), but that isn't documented. `port/tools/sync_upstream.sh`
  stops with exit code 5 and the `port-sync` Action with the `references` outcome; `--allow-references` pushes
  anyway, and only the user decides to use it. The check runs whatever the repository's visibility: GitHub still
  shows a reference from a private repository to people who can't read it, with a minimum of information
  ([SYNC.md](../SYNC.md#commit-messages-that-name-other-repositories)).
- Port sessions never run the upstream root `CLAUDE.md`'s overlay claim and progress comment rules or the
  `decomp-file`, `finish-file` and `match-function` skills, which post on upstream's issues; `port/CLAUDE.md` says so.
  Rule text isn't the only guard: `port/tools/hooks/port_guard.py`, a PreToolUse hook that
  `port/tools/bootstrap_checkout.sh` installs in the clone's `.claude/settings.local.json`, refuses writes to upstream.
  Claude Code 2.1.211 and later read that file from the clone's root in each of its worktrees (not on Windows), and
  `.worktreeinclude` also copies it into new worktrees, as a fallback. The same script sets the `upstream` remote's
  push URL to `DISABLED-port-never-pushes-upstream`; it is run again whenever that remote is added again.

## Alternatives

- **Offer matching-neutral conventions upstream** (the earlier D10 proposal). It would shrink the port's changes to
  game files, but tells upstream about the port. Rejected by the user.
- **Report decomp bugs upstream without mentioning the port.** A finding's evidence comes from the port's oracle tests,
  so the report would invite questions about it. The user's choice covers it: decomp sessions fix findings as ordinary
  decomp work when the user asks.

## Consequences

- The port carries every portability change itself. More files differ from upstream sooner, which moves the
  `port-sync` Action to its pull request mode earlier ([SYNC.md](../SYNC.md)) and makes conflicts on synced files more
  likely. Keeping each change small and logged matters more.
- [ADR 0001](0001-repository-and-upstream-sync.md)'s rejected alternative "putting `port/` in upstream" stays rejected
  for good, and its revisit trigger "if upstream's maintainers want the port in-tree" no longer applies.
- The repository stays public. That was the user's choice in ADR 0001, and it involves no post on upstream: it isn't a
  GitHub fork, so it doesn't appear in upstream's fork network.
- An upstream commit whose message references an issue holds up the sync until the user decides, so some upstream
  advances wait for a session. History so far has 3 such commits in about 2,450.
- Making the repository private wouldn't lift the reference check, since it hides only what a reference says, not that
  it exists. It would also end the free GitHub Actions minutes that ADR 0001 and [PLAN.md
  §4.1](../PLAN.md#41-repository) chose a public repository for. GitHub Free includes 2,000 minutes a month for private
  repositories ([GitHub Actions billing](https://docs.github.com/en/billing/concepts/product-billing/github-actions)),
  and the hourly sync alone, whose compile check takes about 3 minutes whenever upstream has moved, could use them up
  partway through a month. GitHub then blocks the sync, Compile and Standards until the next month, unless billing is
  set up or the schedule is slowed. The switch needs its own ADR, superseding ADR 0001's choice of visibility.
- Each new clone needs `bootstrap_checkout.sh` for the write guard and the disabled push URL. Both live in the
  clone's local configuration (`.claude/settings.local.json` and `.git/config`), not in git, so a clone has neither
  until it is prepared, and a session there relies on rule text alone. The clone's worktrees share both.
- Revisit only if the user changes this decision.
