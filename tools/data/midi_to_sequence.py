#!/usr/bin/env python3
"""Convert a standard MIDI file into a sound sequence's text, sequence.s, to add or replace a song or a sound.

    midi_to_sequence.py SONG.mid data/sound/sequences/bgm/my_song/sequence.s

Each MIDI channel that plays notes becomes a track, the first of them track 0, which allocates and opens the others.
Time is converted to the game's 48 ticks a quarter note. Notes are written as the game's sequences write them: the
track doesn't wait for a note to end (note_wait 0), and rests give the time to the next event, so a channel's chords
and overlapping notes play as they do in the MIDI file. The tempo, program changes, volume (controller 7), pan (10),
expression (11), modulation depth (1) and pitch bend are kept. A marker named loopStart and one named loopEnd (the MIDI
editors' and DS tools' convention) loop every track from the one to the other; without them the song ends.

Program numbers pick instruments of the sequence's bank (its data.json's "bank"), not General MIDI ones, so set them
in the MIDI file to the bank's instruments. A channel without notes is left out, with its controllers, and what is set
before the loop isn't set again when it loops. Check the sequence in the game or an emulator after converting it.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.sseq import assemble, key_name  # noqa: E402

TICKS_PER_QUARTER = 48
CONTROLLERS = {7: "volume", 10: "pan", 11: "expression", 1: "mod_depth"}
LOOP_START = ("loopstart", "[")
LOOP_END = ("loopend", "]")


def read_var(data: bytes, position: int) -> tuple[int, int]:
    value = 0
    while True:
        byte = data[position]
        position += 1
        value = value << 7 | byte & 0x7F
        if not byte & 0x80:
            return value, position


def read_midi(data: bytes) -> tuple[int, list[tuple[int, int, str, tuple]]]:
    """Returns a MIDI file's ticks a quarter note and its events, (tick, order, kind, values), across its tracks."""
    if data[:4] != b"MThd":
        raise SystemExit("not a standard MIDI file")
    _, _, track_count, division = struct.unpack_from(">IHHH", data, 4)
    if division & 0x8000:
        raise SystemExit("MIDI files timed in SMPTE frames aren't supported")
    position = 8 + struct.unpack_from(">I", data, 4)[0]
    events = []
    for _ in range(track_count):
        if data[position:position + 4] != b"MTrk":
            raise SystemExit("a MIDI track chunk is missing")
        end = position + 8 + struct.unpack_from(">I", data, position + 4)[0]
        position += 8
        tick, status = 0, 0
        while position < end:
            delta, position = read_var(data, position)
            tick += delta
            if data[position] & 0x80:
                status = data[position]
                position += 1
            kind, channel = status & 0xF0, status & 0x0F
            if status == 0xFF:
                meta = data[position]
                length, position = read_var(data, position + 1)
                payload = data[position:position + length]
                position += length
                if meta == 0x51 and len(payload) == 3 and int.from_bytes(payload, "big"):
                    events.append((tick, len(events), "tempo", (int.from_bytes(payload, "big"),)))
                elif meta in (0x06, 0x01):
                    events.append((tick, len(events), "marker", (payload.decode("latin-1").strip().lower(),)))
            elif status in (0xF0, 0xF7):
                length, position = read_var(data, position)
                position += length
            elif kind in (0x80, 0x90):
                key, velocity = data[position], data[position + 1]
                position += 2
                on = kind == 0x90 and velocity
                events.append((tick, len(events), "on" if on else "off", (channel, key, velocity)))
            elif kind in (0xA0, 0xB0, 0xE0):
                first, second = data[position], data[position + 1]
                position += 2
                if kind == 0xB0:
                    events.append((tick, len(events), "control", (channel, first, second)))
                elif kind == 0xE0:
                    events.append((tick, len(events), "bend", (channel, (second << 7 | first) - 8192)))
            elif kind in (0xC0, 0xD0):
                if kind == 0xC0:
                    events.append((tick, len(events), "program", (channel, data[position])))
                position += 1
        position = end
    return division, sorted(events)


def convert(midi: bytes) -> str:
    division, events = read_midi(midi)

    def ticks(tick: int) -> int:
        return round(tick * TICKS_PER_QUARTER / division)

    channels = sorted({values[0] for _, _, kind, values in events if kind == "on"})
    if not channels:
        raise SystemExit("the MIDI file plays no notes")
    if len(channels) > 16:
        raise SystemExit("a sequence has at most 16 tracks")
    markers = {values[0]: ticks(tick) for tick, _, kind, values in events if kind == "marker"}
    loop_start = next((markers[name] for name in LOOP_START if name in markers), None)
    loop_end = next((markers[name] for name in LOOP_END if name in markers), None)
    if (loop_start is None) != (loop_end is None):
        raise SystemExit("a loop needs both a loopStart and a loopEnd marker")
    song_end = max(ticks(tick) for tick, *_ in events)
    if loop_end is not None:
        song_end = loop_end
    lines = []
    for track, channel in enumerate(channels):
        # Each track's commands at their ticks: settings first, then notes, whose lengths come from their note-offs.
        # A key played again while it's held is a note of its own, ended by the first note-off; a note still held at
        # the song's end or its loop's ends there
        timeline: dict[int, list[str]] = {}
        held: dict[int, list[tuple[int, int]]] = {}

        def add_note(key: int, start: int, velocity: int, at: int):
            length = max(1, min(at, song_end) - start)
            timeline.setdefault(start, []).append(f"note {key_name(key)}, {velocity}, {length}")

        for tick, _, kind, values in events:
            at = ticks(tick)
            if loop_end is not None and at > loop_end and kind != "off":
                continue
            if kind == "tempo" and track == 0:
                timeline.setdefault(at, []).append(f"tempo {round(60_000_000 / values[0])}")
            if kind in ("tempo", "marker") or values[0] != channel:
                continue
            if kind == "on":
                held.setdefault(values[1], []).append((at, values[2]))
            elif kind == "off" and held.get(values[1]):
                start, velocity = held[values[1]].pop(0)
                add_note(values[1], start, velocity, at)
            elif kind == "program":
                timeline.setdefault(at, []).insert(0, f"program {values[1]}")
            elif kind == "control" and values[1] in CONTROLLERS:
                timeline.setdefault(at, []).insert(0, f"{CONTROLLERS[values[1]]} {values[2]}")
            elif kind == "bend":
                timeline.setdefault(at, []).insert(0, f"pitch_bend {max(-128, min(127, values[1] // 64))}")
        for key, notes in held.items():
            for start, velocity in notes:
                add_note(key, start, velocity, song_end)
        lines.append(f"track_{track:02d}:")
        if track == 0:
            lines.append(f"    alloc_tracks {sum(1 << number for number in range(len(channels))):#06x}")
            lines += [f"    open_track {number}, track_{number:02d}" for number in range(1, len(channels))]
        lines.append("    note_wait 0")
        now = 0
        for at in sorted(timeline):
            if loop_start is not None and now <= loop_start <= at and f"track_{track:02d}_loop:" not in lines:
                if loop_start > now:
                    lines.append(f"    rest {loop_start - now}")
                    now = loop_start
                lines.append(f"track_{track:02d}_loop:")
            if at > now:
                lines.append(f"    rest {at - now}")
                now = at
            lines += [f"    {command}" for command in timeline[at]]
        if loop_start is not None and f"track_{track:02d}_loop:" not in lines:
            if loop_start > now:
                lines.append(f"    rest {loop_start - now}")
                now = loop_start
            lines.append(f"track_{track:02d}_loop:")
        if song_end > now:
            lines.append(f"    rest {song_end - now}")
        lines.append(f"    jump track_{track:02d}_loop" if loop_start is not None else "    end")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("midi", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    text = convert(args.midi.read_bytes())
    assemble(text, str(args.output))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text)
    print(f"wrote {args.output}")


if __name__ == "__main__":
    main()
