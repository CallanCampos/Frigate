<!-- Native player-input reproduction: app/record.c, replay.c, oracle/pbwinput and the host seed boundary. -->
# 0027: Record effective video input with explicit initial state, clock and bounded seeds

- **Status:** accepted
- **Date:** 2026-10-28
- **Plan:** §4.5 (input and clock), §6 (reproduction and verification)

## Context

A player's report needs the input that the game actually read. Raw window events lose held-state semantics;
game-frame coalescing loses touch or lid transitions when several VBlanks share one game pass. The effective
clock and startup/battle seeds also affect reproduction. Input files declare seeds before frame changes and
hold their final state indefinitely: their last change is not an end-of-session marker.

Legacy `pbwinput 1` files apply frame 1 both at power-on and at the first VBlank. Live input can change between
those observations. A recording must preserve both states without deferring a player's input or inventing a hold.

## Decision

Record effective buttons, lid and touch after replay/window input at each video-frame boundary, before ARM7 touch
sampling. Stream only changes, with constant session memory. Normalize replay input to video frames; playback
continues to own input when `--input` and `--record` are combined. Replay-to-live handoff needs a separate explicit
boundary and review.

Add the optional `initial BUTTONS TOUCH` header to the existing format. It states the preboot input and holds until
the first frame change. Existing files retain their current behavior. In files with this header, frame rows commit
only with their terminating newline. An unterminated numeric frame tail can be discarded as an interrupted write;
malformed complete rows and header errors remain errors. Both the native player and test oracle use the shared
initial-state getter. Older readers reject the new directive instead of silently changing the input.

Write the effective RTC once, honoring explicit overrides and replay running/pinned mode. Live recording captures
host local time once and thereafter follows virtual time. Ordinary live play keeps its existing clock policy.
Predeclare 64 seeds from the host OS for a live recording, or preserve a replay's explicit bounded list. Reject an
empty replay seed list before startup. When the list is exhausted, flush/close the prefix and exit with status 1
before an unrecorded fallback builder can modify game state or supply a seed. Non-recorded behavior is unchanged.

Create a new destination exclusively and reject aliases of the active save, input and other outputs. Flush the
complete header before starting the game, then flush complete change rows. Detected I/O failures stop the run;
attempt rollback to the committed offset in this newly created file. Failure to create is status 2; write, flush,
close and exhaustion failures are status 1. A kill during header creation, filesystem failure or a failed rollback
does not guarantee a recoverable file. A valid header plus complete frame rows remains a useful crash prefix.

## Alternatives

- Buffer the whole session or rewrite the file after each seed: unbounded memory or repeated rewriting; weaker
  crash-prefix behavior.
- Record only fixed time or permit silent seed fallback: missing entropy prevents faithful reproduction.
- Reuse frame 1 for two different initial observations: loses an input transition or changes sampling.
- Automatically hand off after the final replay change: changes existing files that intentionally hold a button.

## Consequences and verification

The shared format/parser, app writer and opt-in host seed policy keep game C unchanged. Focused tests cover initial
input, actual touch sampling and release across repeated game frames, header/row parsing, bounded exhaustion,
exclusive destinations and I/O failures. [Whole-program checks](../../tests/replay/README.md) record and replay the
real executable in separate processes, compare frame/counter/screen hashes and random/render-state payloads, and
compare saves byte for byte with both main-save copies validated. Longer story routes use isolated fixture saves.

Reproduction requires a compatible build, the same ROM, starting save and settings. This mechanism preserves
native behavior; it does not establish DS trainer-AI timing fidelity, cross-build compatibility or save interchange.
An unlimited recording or a new seed-stream format requires another decision.
