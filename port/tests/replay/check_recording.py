#!/usr/bin/env python3
"""Record actual BW2 play, replay it in a fresh process, and compare complete native results.

Exercises app/record.c and replay.c through the real program, with isolated copies of the same starting save.
Checks every frame/counter/screen hash, the NNS and random-state payloads, effective clock/seed metadata and exact
save bytes. saveimage.py checks both main-save copies. This is native determinism, not DS interchange or AI parity.
The default CTest route formats New Game's save; --input/--save/--frames also support longer local story routes.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ORACLE_TOOLS = Path(__file__).resolve().parents[2] / "tools" / "oracle"
sys.path.insert(0, str(ORACLE_TOOLS))
import compare  # noqa: E402
import oracle  # noqa: E402
import saveimage  # noqa: E402


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_game(args: argparse.Namespace, directory: Path, input_path: Path | None,
             recording: Path | None, rom: Path, fixed_time: str | None) -> dict:
    directory.mkdir()
    saves = directory / "saves"
    saves.mkdir()
    if args.save is not None:
        shutil.copyfile(args.save, saves / f"{oracle.rom_game_code(str(rom))}.sav")
    command = [str(args.binary), "--headless", "--watchdog", "30", "--frames", str(args.frames),
               "--rom", str(rom), "--save-dir", str(saves), "--run-dir", str(directory),
               "--run-screens", args.screens]
    if input_path is not None:
        command += ["--input", str(input_path)]
    if recording is not None:
        command += ["--record", str(recording)]
    if fixed_time is not None:
        command += ["--fixed-time", fixed_time]
    # These native handles are the same ones oracle.py's reviewed story comparisons use. Payloads, never ASLR
    # addresses, are compared. Include every frame after the common boot boundary; no timing or battle exclusions.
    for spec in ("NNS_G3dGlb=NNS_G3dGlb:612", "gfl_lc=**port_gfl_use_work:8",
                 "gfl_mt=**port_gfl_std_tables:4", "gfl_mt_index=**port_gfl_std_tables+2496:4"):
        command += ["--run-dump", f"{spec}@1116-{args.frames}"]
    with (directory / "output.log").open("wb") as output:
        done = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT, timeout=args.timeout)
    if done.returncode != 0:
        raise ValueError(f"{directory.name}: exit {done.returncode}; see {directory / 'output.log'}")
    return {"command": command, "status": done.returncode}


def compare_runs(first_path: Path, second_path: Path, frames: int) -> dict:
    first, second = (compare.load_run(str(path)) for path in (first_path, second_path))
    expected_frames = set(range(1, frames + 1))
    if set(first.frames) != expected_frames or set(second.frames) != expected_frames:
        raise ValueError("record/replay did not both complete every requested frame")
    for frame in range(1, frames + 1):
        if first.frames[frame] != second.frames[frame]:
            raise ValueError(f"frame {frame}: screen/counter/input-clock results differ")
    first_memory, second_memory = (compare.load_memory(run) for run in (first, second))
    sizes = {"NNS_G3dGlb": 612, "gfl_lc": 8, "gfl_mt": 4, "gfl_mt_index": 4}
    expected_memory = {(frame, label) for frame in range(1116, frames + 1) for label in sizes}
    if set(first_memory) != expected_memory or set(second_memory) != expected_memory:
        raise ValueError("record/replay did not produce all requested state samples")
    for key in sorted(expected_memory):
        if len(first_memory[key][1]) != sizes[key[1]] or len(second_memory[key][1]) != sizes[key[1]]:
            raise ValueError(f"frame {key[0]}: {key[1]} state has the wrong size")
        if first_memory[key][1] != second_memory[key][1]:
            raise ValueError(f"frame {key[0]}: {key[1]} state differs")
    for field in ("time", "clock", "seeds", "completed", "status"):
        if field not in first.info or first.info[field] != second.info.get(field):
            raise ValueError(f"effective run metadata differs: {field}")
    seed_fields = {key for key in first.info if re.fullmatch(r"seed_\d+", key)}
    if not seed_fields or seed_fields != {key for key in second.info if re.fullmatch(r"seed_\d+", key)}:
        raise ValueError("missing or different used-seed metadata")
    if first.info["status"] != "0" or first.info["completed"] != str(frames):
        raise ValueError("run metadata does not report successful completion")
    if seed_fields != {f"seed_{index}" for index in range(1, int(first.info["seeds"]) + 1)}:
        raise ValueError("used-seed metadata does not account for every request")
    for key in seed_fields:
        if first.info[key] != second.info[key] or not first.info[key].endswith(" given"):
            raise ValueError(f"{key}: recording/replay used a different or undeclared seed")
    return {"frames_equal": frames, "state_payloads_equal": len(expected_memory),
            "effective_time": first.info["time"], "clock": first.info["clock"],
            "used_seeds": len(seed_fields)}


def check_header(path: Path, version: str, info: dict) -> dict:
    lines = oracle.input_lines(str(path))
    header = {line.split()[0]: line.split(maxsplit=1)[1]
              for line in lines if line.split()[0] in ("version", "frames", "time", "clock", "initial")}
    for key, expected in (("version", version), ("frames", "video"),
                          ("time", info["effective_time"]), ("clock", info["clock"])):
        if header.get(key) != expected:
            raise ValueError(f"recording header {key}: {header.get(key)!r} != {expected!r}")
    seeds = oracle.input_seeds(str(path))
    if "initial" not in header or not 1 <= len(seeds) <= 64:
        raise ValueError("recording lacks explicit initial input or a bounded seed list")
    return {"declared_seeds": len(seeds), "recording_sha256": digest(path)}


def check_saves(first_path: Path, second_path: Path, require_save: bool,
                reference: Path | None) -> dict[str, str]:
    def images(directory: Path) -> dict[str, bytes]:
        files = {}
        for path in (directory / "saves").iterdir():
            if not path.is_file() or path.suffix != ".sav":
                raise ValueError(f"unexpected or unfinished save output: {path}")
            files[path.name] = path.read_bytes()
        return files

    first, second = images(first_path), images(second_path)
    if first.keys() != second.keys() or (require_save and not first):
        raise ValueError("record/replay produced different save files, or no required save")
    result = {}
    for name, data in first.items():
        if data != second[name]:
            raise ValueError(f"{name}: recorded/replayed save bytes differ")
        # An optional examined reference fixes discovery to that layout. Without one this checks internal image
        # integrity, not authenticity; both banks, all 73 blocks and the checksum tables/footers remain mandatory.
        problems = saveimage.validate_image(data, reference.read_bytes() if reference is not None else None)
        if problems:
            raise ValueError(f"{name}: " + "; ".join(problems[:4]))
        result[name] = hashlib.sha256(data).hexdigest()
    return result


def check(args: argparse.Namespace, work: Path, rom: Path) -> dict:
    immutable = [args.binary, rom] + [path for path in (args.input, args.save, args.reference_save) if path is not None]
    before = {str(path): digest(path) for path in immutable}
    recording = work / "recorded.pbwinput"
    first, second = work / "record", work / "replay"
    commands = [run_game(args, first, args.input, recording, rom, args.fixed_time),
                run_game(args, second, recording, None, rom, None)]
    result = compare_runs(first, second, args.frames)
    result.update(check_header(recording, args.version, result))
    if args.input is None and result["declared_seeds"] != 64:
        raise ValueError("a live recording did not predeclare all 64 seeds")
    if args.input is not None and oracle.input_seeds(str(args.input)) != oracle.input_seeds(str(recording)):
        raise ValueError("recording changed the replay's explicit seed list")
    result["saves_sha256"] = check_saves(first, second, args.require_save, args.reference_save)
    if args.fixed_time is not None and result["effective_time"] != args.fixed_time:
        raise ValueError("recording lost the explicit fixed-time override")
    if before != {str(path): digest(path) for path in immutable}:
        raise ValueError("binary, ROM, input or source save changed during the experiment")
    result.update(status="PASS", version=args.version, commands=commands, immutable_sha256=before)
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--version", choices=("b2_us", "w2_us"), required=True)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--input", type=Path, help="omit to record live headless input with host entropy")
    parser.add_argument("--save", type=Path, help="starting image copied separately for both runs")
    parser.add_argument("--reference-save", type=Path, help="examined save whose block layout validates outputs")
    parser.add_argument("--frames", type=int, default=5600)
    parser.add_argument("--fixed-time")
    parser.add_argument("--screens", default="none")
    parser.add_argument("--require-save", action="store_true")
    parser.add_argument("--timeout", type=int, default=1200, help="seconds per process")
    parser.add_argument("--out", type=Path, help="new directory in which to preserve evidence; never overwrites")
    args = parser.parse_args(argv)
    if not 1116 <= args.frames <= 100000000 or args.timeout <= 0:
        parser.error("frames must be 1116..100000000 and timeout must be positive")
    for name in ("binary", "input", "save", "reference_save"):
        path = getattr(args, name)
        if path is not None:
            setattr(args, name, path.resolve())
    rom = Path(oracle.rom_path(args.version, str(args.rom) if args.rom is not None else None)).resolve()
    reason = oracle.rom_matches(args.version, str(rom))
    if reason:
        print(f"SKIP: {reason}")
        return 66
    temporary = None
    try:
        if args.out is None:
            temporary = tempfile.TemporaryDirectory(prefix="record-replay-")
            work = Path(temporary.name)
        else:
            work = args.out.resolve()
            work.mkdir(parents=True, exist_ok=False)
        result = check(args, work, rom)
        (work / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(f"PASS: {args.version}: {result['frames_equal']} identical frames, "
              f"{result['state_payloads_equal']} state payloads, {len(result['saves_sha256'])} valid identical saves")
        return 0
    except (OSError, ValueError, compare.RunError, subprocess.TimeoutExpired) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    finally:
        if temporary is not None:
            temporary.cleanup()


if __name__ == "__main__":
    sys.exit(main())
