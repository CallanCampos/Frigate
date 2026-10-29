# Sync tooling tests

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
