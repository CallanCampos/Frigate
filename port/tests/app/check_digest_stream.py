"""Runs Frigate's program twice with --digest and checks the two digest streams it writes: the test app_digest_stream
(port/cmake/app.cmake), which keeps the program's writer of the per-game-frame digest (port/app/digest_stream.c,
ADR 0031) run in the merge gate's verification, which builds no oracle. It touches every screen of the game: the
digest is a hash of each part of the display state at the last VBlank of each game frame.

    check_digest_stream.py --work DIR --saves DIR -- PROGRAM ARGUMENT...

runs PROGRAM with its arguments, --save-dir with the given directory emptied first, and --digest DIR/a/digest.tsv, then
again into DIR/b, and reads both streams with compare.py's reader (load_digest, which holds a stream to digest.c's
reader's rules). It checks that the two are byte for byte the same (the program is deterministic, and so is its
writer); that the first line is "#pbwdigest 1"; that the counts run from 1, each one more than the last, with no gap
(the writer's start and last-VBlank rules: a stream starts at count 1 and has one row per count); and that every row
has every group digest.c knows (DIGEST_GROUPS), none of them "-". A run that fails, or stops before its last frame,
fails the test; one without the player's ROM is skipped as the program says (status 66). Exits 0 when every check
passes, 1 otherwise, 2 for bad arguments. Standard library only.
"""

import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "tools", "oracle"))

import compare  # noqa: E402 - port/tools/oracle's reader of digest streams

NO_ROM = 66


def run(command, saves, out):
    """Runs the program once into out/digest.tsv, from an empty save directory: its exit status and its messages"""
    shutil.rmtree(saves, ignore_errors=True)
    os.makedirs(saves)
    os.makedirs(out, exist_ok=True)
    stream = os.path.join(out, "digest.tsv")
    if os.path.exists(stream):
        os.remove(stream)
    done = subprocess.run([*command, "--save-dir", saves, "--digest", stream], capture_output=True, text=True)
    return done.returncode, done.stderr + done.stdout


def check(stream):
    """The failures of one stream's own checks, as lines"""
    failures = []
    with open(stream, "rb") as f:
        first = f.readline()
    if first != b"#pbwdigest 1\n":
        failures.append(f"{stream}: the first line is {first!r}, not \"#pbwdigest 1\"")
    try:
        digest = compare.load_digest(stream)
    except compare.RunError as error:
        return failures + [str(error)]
    counts = sorted(digest.records)
    if not counts:
        return failures + [f"{stream}: no records"]
    if counts != list(range(1, len(counts) + 1)):
        gaps = [c for c in range(1, counts[-1] + 1) if c not in digest.records]
        failures.append(f"{stream}: the counts run {counts[0]}-{counts[-1]}, not from 1 by one; missing "
                        f"{compare.describe_ranges(gaps)}")
    if set(digest.groups) != set(compare.DIGEST_GROUPS):
        failures.append(f"{stream}: the header names {len(digest.groups)} groups, not digest.c's "
                        f"{len(compare.DIGEST_GROUPS)}")
    lacking = [record.count for record in digest.records.values() if len(record.hashes) != len(compare.DIGEST_GROUPS)]
    if lacking:
        failures.append(f"{stream}: counts {compare.describe_ranges(lacking)} lack a group")
    print(f"{stream}: counts {counts[0]}-{counts[-1]}, {len(counts)} records of {len(digest.groups)} groups")
    return failures


def main(argv):
    if "--" not in argv:
        print(__doc__)
        return 2
    split = argv.index("--")
    options, command = argv[1:split], argv[split + 1:]
    if len(options) != 4 or options[0] != "--work" or options[2] != "--saves" or not command:
        print(__doc__)
        return 2
    work, saves = options[1], options[3]
    streams = []
    for name in ("a", "b"):
        out = os.path.join(work, name)
        status, messages = run(command, saves, out)
        if status == NO_ROM:
            print(messages.strip())
            print("check_digest_stream.py: skipped: the program has no ROM (status 66)")
            return NO_ROM
        if status != 0:
            print(messages.strip()[-2000:])
            print(f"check_digest_stream.py: FAIL: run {name} ended with status {status}")
            return 1
        streams.append(os.path.join(out, "digest.tsv"))
    failures = []
    with open(streams[0], "rb") as a, open(streams[1], "rb") as b:
        if a.read() != b.read():
            failures.append("the two runs' streams differ")
    for stream in streams:
        failures += check(stream)
    for failure in failures:
        print(f"FAIL: {failure}")
    if failures:
        return 1
    print("check_digest_stream.py: two runs gave the same stream, from count 1 by one, every group in every row")
    shutil.rmtree(work, ignore_errors=True)
    shutil.rmtree(saves, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
