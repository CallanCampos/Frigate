# port/tools/tests/

The tests of two tools that CI's `Sync tooling tests` job runs ([port-ci.yml](../../../.github/workflows/port-ci.yml)),
feeding the required `Frigate CI` gate: the upstream sync's, `test_upstream_sync.py`, and the documents check's,
`test_check_docs.py` ([below](#the-documents-check)). Neither needs a ROM or touches GitHub.

## The upstream sync

`test_upstream_sync.py` creates isolated Git repositories to check upstream merge preservation, the draft proposal of a
conflicting upstream head and its resolution's fast-forward, the history/reference/workflow stops, frozen pending
proposals and publication behavior. It makes no real GitHub writes and needs no ROM.

Run from the repository root:

```sh
python3 -m unittest discover -s port/tools/tests -p test_upstream_sync.py
```

The `Sync tooling tests` job feeds the required `Frigate CI` gate. The publishing helper lives in
[../upstream_sync.py](../upstream_sync.py); the maintainer procedure is [SYNC.md](../../docs/SYNC.md).

The sync fixtures also cover recovery after a lost PR-creation response, a genuine creation failure, separately
retrying failed review requests without a push, existing/completed reviews and the owner's self-review exception,
and conflict filenames containing blank lines, backticks and terminal controls. No fixture contacts GitHub.

Mixed resource/C diff fixtures also verify that non-UTF-8 resource bytes cannot crash the inherited style checker,
while newly added C violations still fail strict checking.

Shell-verifier controls cover build/test failures; a caught hangup, interrupt, quit or termination, also when the
failure-status upload fails, or when the signal goes to the whole process group and stops the reader of the
verifier's output too, its whole output or its standard output alone; a changed PR head, and one that can't be read,
before the pending status or after the builds; a local HEAD that another process moves before the build;
unsuccessful pending, success and failure-status uploads; status requests that discard GitHub's response; builds and
tests that keep SIGPIPE at its default; and successful completion. They require failed runs to stay failed, with the
verifier's own exit status, while reporting the result on the pinned commit; only completed validation may leave a
successful local-verification status. The hourly sync workflow's App-token step is checked too: its repository and
permission scope, its revocation, and its `client-id` input.

`sync_upstream.sh`'s messages send the reader to [SYNC.md](../../docs/SYNC.md) by a section's name, as
`'Upstream rewrite (exit 4)' under 'Conflicts and stopped syncs' in port/docs/SYNC.md`, or by an anchor
(`SYNC.md#conflicts-and-stopped-syncs`). A test reads each such name as the script prints it and fails when SYNC.md
has no such heading, or no list item of that bold label under that heading, so that renaming a section means updating
the messages too. A line of the script's code (a message; a comment may name the document as it likes) that names
SYNC.md in neither form fails as well, since nothing could check it. Its controls rename a label, change a message's
name, move one under another heading, add an anchor SYNC.md lacks and add a message that names its section another
way or names no section, and each must fail.

The script's summary, printed when a run stops, names what stopped it. Two fixtures run `sync_upstream.sh --no-build`
offline, through a `git` wrapper that sends the fetches and `ls-remote` of the GitHub remotes to local bare
repositories: one whose topic branch on origin can't take the push as a fast-forward, one whose upstream no longer
contains the last upstream commit merged. Both stop with exit 4, and the summary's last line must name the check that
stopped each.

## The documents check

`test_check_docs.py` runs [../check_docs.py](../check_docs.py) on small repositories of Markdown documents that it
writes into a scratch directory. A clean tree passes: it uses every form the check reads, links to files, directories
and headings (repeated and punctuated ones, of the same document and of another), HTML ids, line anchors and a Markdown
file's source lines, links whose text runs over two lines, reference definitions, HTML sources, a table's escaped pipes
and one without outer pipes, and the links GitHub doesn't render, in code spans, fenced blocks and HTML comments. Each
negative control seeds one problem, which must be reported with its kind and line: a missing file (in an inline link, a
link over two lines, a reference definition, an HTML source), a link out of the repository, a missing anchor (in
another document, in the same one, a heading in a fenced block, a fragment on a file that isn't Markdown), a line
anchor past the end of a file or of a Markdown file's source, a row that a code span's pipes split, a short row, a line
straight after a table, a header over a delimiter row of another width, a row cut off by a blank line and a stray row.
The allowlist's controls: an allowed problem passes and is printed with its reason, wherever it moves in its document
and at each place it repeats, and a row that names no problem, one without a reason, an unknown kind or a row of the
wrong width fails. The anchors' rule is held to headings whose anchors GitHub's renderer gave.

```sh
python3 -m unittest discover -s port/tools/tests -p test_check_docs.py
python3 port/tools/check_docs.py
```

The second command is the check itself, on the whole tree, as CI runs it after the tests.
