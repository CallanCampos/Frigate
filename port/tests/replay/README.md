# Whole-program replay checks

These scripts run Frigate's real game and platform code. Their input and reference files contain text, hashes and
examined save metadata; ROMs, screenshots and save images are generated locally from the player's dump.
The parent [test map](../README.md) describes the established screen, same-save and oracle-save routes.

| Module | Purpose | Tests |
| --- | --- | --- |
| [check_recording.py](check_recording.py) | Run `--record` with isolated saves, replay the resulting video-frame input in a new process, and compare every frame/counter/screen hash, NNS/random-state payload, effective clock and used seed. Compare save bytes exactly and validate both main-save copies, all 73 blocks, tables and footers. | `replay_recording`; returns 66 without the matching ROM. |
| [test_check_recording.py](test_check_recording.py) | Feed actual run-directory parsers synthetic mismatches; prove missing frames/state, bad seeds/clocks and equally corrupt saves fail. Different native addresses do not hide different payloads. | `replay_recording_tool`, ROM-free. |

The recording checker defaults to 5,600 frames for callers choosing their own endpoint. CTest explicitly uses
7,800 frames with the existing first-save input: both original DS versions have complete valid main-save copies
then, while at 5,600 both still have unfinished images. `replay_same_save` retains that early checkpoint with
first-bank identity/egg bytes and second-bank FF ranges; `replay_same_save_complete` runs twice to 7,800 and checks
both banks' identity/egg bytes plus all 73 blocks, checksum tables and footers (`--require-valid-image FILE`). Longer routes can pass `--input FILE --save START.sav --frames N --require-save`.
With a starting image, `--require-save` also requires changed bytes, so an untouched copied fixture cannot pass as
a newly produced save. Continuation-only checks can omit that flag and still compare/validate their images.
`--reference-save FILE` fixes the block layout to an examined image; without it, CRC checks prove internal image
integrity. These native comparisons do not establish DS interchange or trainer-AI timing fidelity.

Use `--fixed-time "YYYY-MM-DD HH:MM:SS"` to test an override: the recording must preserve that effective time, and
the second process obtains it solely from the new input file. Omitting `--input` records live headless input and
checks that all 64 host-source seeds are declared. Focused app tests separately exercise real touch sampling,
initial input, lid changes, truncated prefixes, file errors and seed exhaustion.

By default, temporary evidence is removed after the test. `--out NEW_DIRECTORY` preserves both run logs, frames and
state, the recording, isolated save files, commands and a successful `result.json`; an existing directory is refused.
The original ROM, binary, input and source save are hashed before and after. No player's save is modified.
