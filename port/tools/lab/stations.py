#!/usr/bin/env python3
"""The save lab's stations: saves minted from the oracle's saves by a recipe, which melonDS and Frigate both continue
from power-on, and which the oracle holds Frigate to as it holds a comparison sequence (port/tools/lab/README.md,
"Stations"; port/docs/adr/0032-stations-saves-minted-from-oracle-saves-held-to-melonds.md). A station is a recipe,
port/tests/stations/<name>.recipe, whose base line names one of oracle.py's SAVES; an input file for each version,
port/tests/stations/<name>_<version>.pbwinput, which plays the title and CONTINUE from power-on and then what the
station holds; and its entry in STATIONS, which pins the minted save's SHA-1 for each version, says how the two
programs' runs are compared (an oracle.Sequence, as oracle.py's SEQUENCES say it for a sequence) and what each program's
resulting save must show. A station is held at five levels, each gating the next:

  1. the minted save's structure (saveimage.validate_image against its base) and its SHA-1, pinned for the version;
  2. the recipe read back from the minted save (savelab.read): the place and the spawn CONTINUE takes, the continue
     flag set, each line's value, and every other fact the base's;
  3. melonDS boots it from power-on to the recipe's place;
  4. Frigate held to melonDS frame by frame as a sequence is, its screens and its game state on the game's count, up to
     the last frame before the input's first press of the field menu (oracle.py's compare_sequence);
  5. each program's resulting save read back as facts (the place the game saved, the continue flag cleared, the party,
     the money, the flags and the work values the minted save's), and the two saves compared with each other, their
     facts alike and their bytes as a round trip compares them: the mystery gift blocks decrypted, and only the bytes
     the station declares, each with its traced cause, apart.

oracle.py station NAME runs all five; oracle.py station-check NAME runs Frigate alone on the minted save, with levels 1,
2 and 5 on its own resulting save. Neither commits a save: the base is the oracle's, made at test time, and the minted
and resulting saves stay in the run's work directory. A station's state may be one that play can't make (the ranch's
gate without Terrell's battle on Route 20), so a station is never evidence that the story reaches its place.

Game functionality: the start menu's saved game, which names nextSpawn's zone while EVENT_FLAG_CONTINUE_SCRIPT is set
(src/ov162/startmenu.c); the field's continue at a spawn (EventFieldContinue_Create and FieldMapControl_InitSpawn,
src/ov035/event_mapchange.c); the station's zone and its NPCs (res/field/events/); the field menu's SAVE and the save's
blocks (include/save/). Standard library only.
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(HERE), "oracle"))
sys.path.insert(0, HERE)

import oracle  # noqa: E402
import saveimage  # noqa: E402
import savelab  # noqa: E402

# A station's committed files: its recipe, and its input file for each version (port/tests/stations)
STATION_RECIPE = os.path.join("port", "tests", "stations", "{name}.recipe")
STATION_INPUT = os.path.join("port", "tests", "stations", "{name}_{version}.pbwinput")
# The continue pair, which place sets in a minted save (the flag set, the work value 0) and which the game's CONTINUE
# takes: EventFieldContinue_Create reads the flag and clears it (src/ov035/event_mapchange.c), so the next SAVE writes
# it clear, and with the work value at 0 no continue script runs
CONTINUE_FLAG = "EVENT_FLAG_CONTINUE_SCRIPT"
CONTINUE_WORK = "EVENT_WORK_CONTINUE_SCRIPT"
# The facts, besides the flags and the work values, that a minted save keeps from its base but where a recipe line sets
# them, and the ones that a resulting save keeps from the minted save
MINT_KEPT = ("party", "money", "badges", "playtime", "now_spawn")
RESULT_KEPT = ("party", "money", "badges")


@dataclass(frozen=True)
class Station:
    """A station (README.md, "Stations"). base: the SAVES name of the oracle save that the recipe mints from, which the
    recipe's base line names. base_facts: facts of that base read back (dotted paths into savelab.read's result, as
    fact() takes them) which the oracle's README records independently, so that the reader is anchored on the save the
    game wrote before it reads the one the lab wrote. minted: by version, the SHA-1 of the save the recipe mints from
    the version's base. sequence: how both programs run the station's input file from power-on and how their runs are
    compared, as oracle.py runs and compares a sequence's (its save and input are empty: the save is the minted one, the
    input the station's own). compare_last: the last port frame compared, before the input's first press of the field
    menu, from which the two programs' runs part by construction (the save's dialog, oracle.py's SAVE_WRITES). expect:
    by version, the facts that each program's resulting save must show; none for a version whose input doesn't SAVE,
    whose resulting save must then be the minted save as it was (CONTINUE only reads it). save_bytes: by version, the
    bytes where the two resulting saves may differ, as RoundTrip.known declares them for a round trip (save offsets,
    both copies), each with why, a cause traced"""
    base: str
    base_facts: dict[str, object]
    minted: dict[str, str]
    sequence: oracle.Sequence
    compare_last: int
    expect: dict[str, dict[str, object]] = field(default_factory=dict)
    save_bytes: dict[str, dict[str, str]] = field(default_factory=dict)


# The Route 20 save as the oracle's README records it (README.md, "Floccesy and Route 20": zone 446 at grid (130,661),
# event work 40a3=2 and 40a5=2) and each version's input file states it
# (port/tests/oracle/floccesy_save_<version>.pbwinput)
ROUTE_20_SAVE = {"location.zone": "ZONE_ROUTE_20", "location.tile": [130, 661],
                 "works.EVENT_WORK_0x40a3": 2, "works.EVENT_WORK_0x40a5": 2}
# Floccesy Ranch's gate as the game saves it after arriving through warp 0 (the warp's facing, unk4 1, is up): the
# player state FieldMapControl_InitSpawn set from the spawn, and the arrival it keeps as nowSpawn
RANCH_GATE = {"location.zone": "ZONE_FLOCCESY_RANCH_2", "location.tile": [45, 54], "location.height": 2,
              "location.facing": "WARP_DIR_UP", "now_spawn.type": "ZONE_SPAWN_CHANGE_TYPE_WARP",
              "now_spawn.zone": "ZONE_FLOCCESY_RANCH_2", "now_spawn.warp": 0, "now_spawn.facing": "WARP_DIR_UP"}

STATIONS = {
    # Floccesy Ranch's gate, minted from the Route 20 save, the story's last save (floccesy_save): a state play can't
    # make, since Terrell's battle on Route 20 isn't fought, so never story evidence (ADR 0032). The input file plays
    # floccesy_continue's presses to CONTINUE: the start menu names the ranch, CONTINUE opens its field at the gate, and
    # the field is held frame by frame to port frame 4290; then the field menu's SAVE, whose resulting saves are read
    # back and compared as round-trip floccesy compares the Route 20 save's
    "floccesy_ranch": Station(
        base="floccesy_save",
        base_facts=ROUTE_20_SAVE,
        minted={"b2_us": "72e7ab2a3fa0db48a126895aee52a22a0fc7b073",
                "w2_us": "d82663c39d7c0d835c861ab1bb71ba3860aa18c8"},
        sequence=oracle.Sequence(
            port_frames=6200,
            oracle_frames=9000,
            sample="",
            clock="313-1108",
            lags={"b2_us": "1109=0", "w2_us": "1109=0,3241=-1,3403=0"},
            # Each traced again in this station's own runs (README.md, "Stations"): the title's and CONTINUE's, at the
            # frames floccesy_continue declares for the same presses, but that at port frame 3432 only GFL's random
            # state is the pass's work in progress (NNS G3D's is the port's there), so only it is left out
            known={
                "b2_us": {"2878": oracle.TORN_HIDE, "3001": oracle.TORN_BACK, "3101": oracle.TORN_START,
                          "3112-3129": oracle.CAMERA},
                "w2_us": {"2878": oracle.TORN_HIDE, "3001": oracle.TORN_BACK, "3101": oracle.TORN_START,
                          "3112-3128": oracle.CAMERA, "3219-3240/3": oracle.CRY, "3402": oracle.CURSOR},
            },
            known_oracle={"b2_us": {"4920": oracle.TORN_START},
                          "w2_us": {"1260": oracle.TORN_OPENING, "4852": oracle.TORN_START}},
            state_frames="1116-4290",
            state_oracle="2800-9000",
            known_state={"b2_us": {"3110-3124": oracle.CAMERA, "3447,3454,3600": oracle.MID_PASS},
                         "w2_us": {"3110-3124": oracle.CAMERA, "3447,3454,3600": oracle.MID_PASS}},
            known_random={"b2_us": {"3432": oracle.MID_PASS}, "w2_us": {"3432": oracle.MID_PASS}},
            state=oracle.STATE + oracle.RANDOM_STATE,
        ),
        compare_last=4290,
        expect={"b2_us": RANCH_GATE, "w2_us": RANCH_GATE},
        # The play time's seconds and the CRCs over them, in both games: a minute turned on neither program in these
        # runs (README.md, "Stations"), so the minute's byte, 0x19426, isn't declared
        save_bytes={"b2_us": oracle.FLOCCESY_SAVE_BYTES, "w2_us": oracle.FLOCCESY_SAVE_BYTES},
    ),
}


def recipe_path(name: str) -> str:
    """The station's recipe, an absolute path"""
    return os.path.join(oracle.REPO_ROOT, STATION_RECIPE.format(name=name))


def input_path(name: str, version: str) -> str:
    """The station's input file for the version, an absolute path"""
    return os.path.join(oracle.REPO_ROOT, STATION_INPUT.format(name=name, version=version))


def fact(facts: dict, path: str) -> object:
    """The fact at a dotted path of savelab.read's result ("location.tile", "works.EVENT_WORK_0x40a3", "party.0.level",
    a list's items by their index); a KeyError for a path the read doesn't have"""
    value: object = facts
    for part in path.split("."):
        if isinstance(value, dict) and part in value:
            value = value[part]
        elif isinstance(value, list) and part.isdigit() and int(part) < len(value):
            value = value[int(part)]
        else:
            raise KeyError(path)
    return value


def named(paths: list[str], kind: str) -> set[str]:
    """The flags ("flags") or work values ("works") that dotted paths name"""
    return {path.split(".")[1] for path in paths if path.startswith(kind + ".") and path.count(".") >= 1}


def read_facts(station: Station, data: bytes, constants: savelab.Constants, what: str, flags: set[str] | None = None,
               works: set[str] | None = None) -> dict:
    """A save's facts as a station reads them (savelab.read): every flag that is set, by name, and each flag and work
    value that its base_facts and expect paths name, and flags and works besides, so that a clear flag or a zero work
    value a path names is still in the read; a savelab.LabError for a save the reader refuses"""
    paths = list(station.base_facts) + [path for facts in station.expect.values() for path in facts]
    return savelab.read(data, constants, flags=sorted(named(paths, "flags") | (flags or set())),
                        works=sorted(named(paths, "works") | (works or set())), all_flags=True, what=what)


def mismatch(facts: dict, path: str, wanted: object, what: str) -> str | None:
    """None when the fact at path is wanted, else what was expected and what was read"""
    try:
        have = fact(facts, path)
    except KeyError:
        return f"{what} has no fact {path}"
    return None if have == wanted else f"{what}'s {path} is {have!r}, not {wanted!r}"


def kept(before: dict, after: dict, keys: tuple[str, ...], flags_left: set[str], works_left: set[str], what: str,
         source: str) -> list[str]:
    """Where after's facts aren't before's: each fact of keys, every work value (one the read leaves out is 0) and every
    flag that is set, but the flags and work values left out; each with both values"""
    failures = []
    for key in keys:
        if before.get(key) != after.get(key):
            failures.append(f"{what}'s {key} is {after.get(key)!r}, where {source}'s is {before.get(key)!r}")
    for work in sorted((set(before["works"]) | set(after["works"])) - works_left):
        if before["works"].get(work, 0) != after["works"].get(work, 0):
            failures.append(f"{what}'s {work} is {after['works'].get(work, 0)}, where {source}'s is "
                            f"{before['works'].get(work, 0)}")
    was = set(before["set_flags"]) - flags_left
    now = set(after["set_flags"]) - flags_left
    for flag in sorted(was - now, key=str):
        failures.append(f"{what} has {flag} clear, where {source} has it set")
    for flag in sorted(now - was, key=str):
        failures.append(f"{what} has {flag} set, where {source} has it clear")
    return failures


def mint(name: str, version: str, base: bytes,
         constants: savelab.Constants | None = None) -> tuple[bytes | None, list[dict], list[str]]:
    """Levels 1 and 2 of a station for a version: the station's recipe minted from base, which must be the version's
    save of the station's base (its SHA-1 the one SAVES pins, its facts the base_facts), the minted save's structure and
    its SHA-1 the one STATIONS pins, and the recipe read back from it. Returns the minted save (None when it couldn't be
    minted), what each recipe line set (savelab.mint's list) and the failures, each saying what was expected and what
    was read"""
    station = STATIONS[name]
    constants = constants or savelab.Constants()
    failures = []
    # The base: the version's save of the station's chain, held to its pin and to the facts recorded for it, so that a
    # base of the other version fails here for its version, not further on for the pin of a save minted from it
    digest = savelab.sha1(base)
    pinned = oracle.SAVES[station.base].sha1.get(version)
    if digest != pinned:
        failures.append(f"the base's SHA-1 is {digest}, where SAVES pins {pinned or 'none'} for {station.base} in "
                        f"{version}")
    try:
        base_facts = read_facts(station, base, constants, "the base")
    except savelab.LabError as error:
        return None, [], failures + [f"the base can't be read: {error}"]
    for path, wanted in station.base_facts.items():
        problem = mismatch(base_facts, path, wanted, "the base")
        if problem:
            failures.append(problem)
    if failures:
        return None, [], failures
    try:
        with open(recipe_path(name), encoding="utf-8") as f:
            recipe = f.read()
        minted, applied = savelab.mint(recipe, base, os.path.basename(recipe_path(name)), constants)
    except (OSError, savelab.LabError) as error:
        return None, [], [f"the recipe doesn't mint: {error}"]
    bases = [line for line in applied if "base" in line]
    if [line.get("base") for line in bases] != [station.base]:
        failures.append(f"the recipe's base lines name {[line.get('base') for line in bases]}, not {station.base}")
    elif bases[0]["version"] != version:
        failures.append(f"the recipe's base line reports {bases[0]['version']}, not {version}")

    # Level 1: the minted save's structure, and its SHA-1 the one STATIONS pins for the version
    for problem in saveimage.validate_image(minted, reference=base):
        failures.append(f"the minted save fails the checks of its base's layout: {problem}")
    made = savelab.sha1(minted)
    pin = station.minted.get(version)
    if pin is None:
        failures.append(f"STATIONS pins no minted save of {name} for {version}; this mint is {made}")
    elif made != pin:
        failures.append(f"the minted save is {made}, STATIONS pins {pin}: the minter, the recipe or the base changed")

    # Level 2: the recipe read back from the minted save, every line's value, and every other fact the base's
    lines = {key: line for line in applied for key in line if key not in ("line", "base", "version")}
    flags_set = {str(line["flag"]) for line in applied if "flag" in line}
    works_set = {str(line["work"]) for line in applied if "work" in line}
    try:
        facts = read_facts(station, minted, constants, "the minted save", flags=flags_set, works=works_set)
    except savelab.LabError as error:
        return minted, applied, failures + [f"the minted save can't be read: {error}"]
    place = lines.get("place", {}).get("place")
    if place:
        wanted = {"location.zone": place["zone"], "location.facing": place["facing"]}
        wanted["next_spawn.zone"] = place["zone"]
        if isinstance(place["arrival"], dict):
            wanted["location.tile"] = place["arrival"]["tile"]
            wanted["location.height"] = place["arrival"]["height"]
        if "warp" in place:
            wanted.update({"next_spawn.type": "ZONE_SPAWN_CHANGE_TYPE_WARP", "next_spawn.warp": place["warp"]})
        else:
            wanted.update({"next_spawn.type": "ZONE_SPAWN_CHANGE_TYPE_POSITION", "next_spawn.warp": -1})
        for path, value in wanted.items():
            problem = mismatch(facts, path, value, "the minted save")
            if problem:
                failures.append(problem)
        for path, value in ((f"flags.{CONTINUE_FLAG}", True), (f"works.{CONTINUE_WORK}", 0)):
            problem = mismatch(facts, path, value, "the minted save")
            if problem:
                failures.append(f"{problem}: place sets the continue pair so that CONTINUE takes the spawn")
    for line in applied:
        if "flag" in line:
            wanted_line = (f"flags.{line['flag']}", line["set"])
        elif "work" in line:
            wanted_line = (f"works.{line['work']}", line["value"])
        elif "money" in line:
            wanted_line = ("money", line["money"])
        elif "badges" in line:
            wanted_line = ("badges.bits", (1 << line["badges"]) - 1)
        elif "playtime" in line:
            wanted_line = ("playtime", line["playtime"])
        else:
            continue
        problem = mismatch(facts, *wanted_line, f"the minted save (recipe line {line['line']})")
        if problem:
            failures.append(problem)
    # Without a place line the minted save keeps the base's place, its spawns and its continue pair too
    keys = tuple(key for key in MINT_KEPT if key not in lines) + (() if place else ("location", "next_spawn"))
    pair = ({CONTINUE_FLAG}, {CONTINUE_WORK}) if place else (set(), set())
    failures += kept(base_facts, facts, keys, flags_set | pair[0], works_set | pair[1], "the minted save", "the base")
    return minted, applied, failures


def result_failures(name: str, version: str, minted: bytes, result: bytes, who: str,
                    constants: savelab.Constants | None = None) -> list[str]:
    """Level 5 for one program's resulting save (who names the program): a save as the game writes one, with the
    minted save's layout; with the version's expect, a save the game wrote, at the facts expect names, with the continue
    flag cleared, nextSpawn as minted (the game never clears it) and the party, the money, the badges, every work value
    and every flag the minted save's, but the continue flag and the flags and work values expect names; without expect,
    the minted save as it was, which CONTINUE only reads. The failures, each with what was expected and what was read"""
    station = STATIONS[name]
    constants = constants or savelab.Constants()
    problems = saveimage.validate_image(result, reference=minted)
    if problems:
        return [f"{who}'s resulting save fails the checks of the minted save's layout: {problem}"
                for problem in problems[:5]]
    expected = station.expect.get(version)
    if expected is None:
        if result != minted:
            return [f"{who}'s resulting save is {savelab.sha1(result)}, not the minted save {savelab.sha1(minted)}: "
                    f"{name}'s input file for {version} doesn't SAVE, so the game only reads the save"]
        return []
    if result == minted:
        return [f"{who}'s resulting save is the minted save as it was: the game never wrote it, so SAVE didn't run"]
    flags_left = named(list(expected), "flags")
    works_left = named(list(expected), "works")
    try:
        before = read_facts(station, minted, constants, "the minted save")
        after = read_facts(station, result, constants, f"{who}'s resulting save")
    except savelab.LabError as error:
        return [f"{who}'s resulting save can't be read: {error}"]
    failures = []
    for path, wanted in expected.items():
        problem = mismatch(after, path, wanted, f"{who}'s resulting save")
        if problem:
            failures.append(problem)
    for path, wanted in ((f"flags.{CONTINUE_FLAG}", False), (f"works.{CONTINUE_WORK}", 0)):
        problem = mismatch(after, path, wanted, f"{who}'s resulting save")
        if problem:
            failures.append(f"{problem}: CONTINUE clears the continue flag, and SAVE writes it so")
    if after["next_spawn"] != before["next_spawn"]:
        failures.append(f"{who}'s resulting save's next_spawn is {after['next_spawn']!r}, where the minted save's is "
                        f"{before['next_spawn']!r}: the game never clears it")
    failures += kept(before, after, RESULT_KEPT, flags_left | {CONTINUE_FLAG}, works_left | {CONTINUE_WORK},
                     f"{who}'s resulting save", "the minted save")
    return failures


def compare_results(name: str, version: str, oracle_save: bytes, port_save: bytes,
                    constants: savelab.Constants | None = None) -> list[str]:
    """Level 5 between the programs: the two resulting saves' facts alike, but their SHA-1s and their play times, which
    each program counts with its own clock's ticks (oracle.py's PLAY_TIME); then their bytes, each save's mystery gift
    blocks decrypted with its own key (oracle.mystery_gift_decrypted), alike but where the station declares them for
    the version (save_bytes). The facts are compared whatever save_bytes declares, so that a declaration can hide a byte
    but never a fact. The failures, each with both values"""
    station = STATIONS[name]
    constants = constants or savelab.Constants()
    failures = []
    try:
        theirs = read_facts(station, oracle_save, constants, "melonDS's resulting save")
        ours = read_facts(station, port_save, constants, "Frigate's resulting save")
    except savelab.LabError as error:
        failures.append(f"a resulting save can't be read: {error}")
    else:
        for key in sorted((set(theirs) | set(ours)) - {"sha1", "playtime"}):
            if theirs.get(key) == ours.get(key):
                continue
            if key == "set_flags":
                failures.append(f"the resulting saves' flags differ: set on melonDS only "
                                f"{sorted(set(theirs[key]) - set(ours[key]), key=str)}, on Frigate only "
                                f"{sorted(set(ours[key]) - set(theirs[key]), key=str)}")
            else:
                failures.append(f"the resulting saves' {key} differ: melonDS's {theirs.get(key)!r}, Frigate's "
                                f"{ours.get(key)!r}")
    plain = {}
    for who, data in (("melonDS", oracle_save), ("Frigate", port_save)):
        plain[who], problems = oracle.mystery_gift_decrypted(data)
        failures += [f"{who}'s resulting save: {problem}" for problem in problems]
    declared = station.save_bytes.get(version, {})
    undeclared = oracle.undeclared_differences(plain["Frigate"], plain["melonDS"], declared)
    if undeclared:
        failures.append("the resulting saves differ where the station declares nothing for " + version + ": "
                        + ", ".join(f"{first:#x}-{last:#x}" for first, last in undeclared))
    return failures
