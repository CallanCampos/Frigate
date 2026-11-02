#!/usr/bin/env python3
"""Pokéstar Studios' movies in res/pokewood/movies/<n>/, a directory for each movie by its number: dump writes them
from the ROM once, and pack builds the archives from them, which the build does.

    pokewood_data.py dump extract/b2_us/files res/pokewood/movies
    pokewood_data.py pack res/pokewood/movies CAST MOVIES

- movie.json (ARCID_POKEWOOD_MOVIE, a/2/6/9, BtlScriptedRules in btl_main.h, which event_pokewood.c, btl_main.c,
  pokewood_system.c, scrcmd_pokewood.c, pkwd_book.c and pkwd_briefing.c read parts of) is the movie's script: its
  rules, trainers, music, the messages by turn and its twelve scenes, each with its messages and the choices it offers.
  The archive's file after the movies' is res/pokewood/default_record.bin, a recorded movie (PokewoodBlock in
  save/pokewood.h, a battle recording) that loadPokewoodAndCheck loads as POKEWOOD_DEFAULT_MOVIE_FILE.
- cast.json (ARCID_POKEWOOD_CAST, a/2/7/2, PokewoodCastFile in event_pokewood.c) is the movie's cast: the Pokémon of
  the player's side and of the opponent's, which EventPokewood_CreateParty makes the battle's parties of. The movies
  from the first without one have none.

Species, items, moves, natures, trainer classes and BGM are their constants, and -1 is none. Both versions have the
same files. movie.schema.json and cast.schema.json describe the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

CAST, MOVIES = "a/2/7/2", "a/2/6/9"
DEFAULT_RECORD = "default_record.bin"
SIDES = ["player", "opponent"]
COUNTS = struct.Struct("<2h")
# PokewoodCastMember
MEMBER = struct.Struct("<hhHHhhhh4H12h")
STATS = ["hp", "attack", "defense", "speed", "special_attack", "special_defense"]
SEXES = ["GENDER_MALE", "GENDER_FEMALE", "GENDER_UNKNOWN"]


def check(condition: bool, what: str):
    if not condition:
        raise ValueError(what)


def member_json(data: bytes, offset: int) -> dict:
    species, shown, level, form, sex, nature, ability, item, *rest = MEMBER.unpack_from(data, offset)
    moves, stats = rest[:4], rest[4:]
    check(0 <= sex < len(SEXES), "a cast member's sex isn't known")
    return {"species": name("SPECIES_", species), "shown_species": name("SPECIES_", shown), "level": level,
            "form": form, "sex": SEXES[sex], "nature": name("NATURE_", nature), "ability": ability,
            "held_item": name("ITEM_", item), "moves": [name("MOVE_", move) for move in moves],
            "ivs": dict(zip(STATS, stats[0::2])), "evs": dict(zip(STATS, stats[1::2]))}


def member_bytes(member: dict, where: str) -> bytes:
    moves = [value(move, where) for move in member["moves"]] + [0] * (4 - len(member["moves"]))
    return MEMBER.pack(value(member["species"], where), value(member["shown_species"], where), member["level"],
                       member["form"], SEXES.index(member["sex"]), value(member["nature"], where), member["ability"],
                       value(member["held_item"], where), *moves,
                       *(member[kind][stat] for stat in STATS for kind in ("ivs", "evs")))


# BtlScriptedRules: the rules, the trainers, the music, the turn events, the unread records, a reserved run of -1, the
# messages by turn, then the scenes
HEADER = struct.Struct("<12h4h8h6h24h30h20h20h")
assert HEADER.size == 0xF8
RESERVED = 30
# BtlStudioScene, with BtlStudioChoice
SCENE = struct.Struct("<6h20h10h24h")
assert SCENE.size == 0x78
SCENES, CHOICES, STEPS = 12, 4, 10
CHOICE_FIELDS = ["message", "message2", "unk04", "unk06", "points", "unk0a"]


def movie_json(data: bytes) -> dict:
    check(len(data) == HEADER.size + SCENES * SCENE.size, f"a movie is {len(data)} bytes, not the movies' size")
    fields = list(HEADER.unpack_from(data))
    (movie, unk02, multiplier, cast_count, director, turn_limit, ideal_turn, ending_scene, rule, species1, species2,
     unk16) = fields[:12]
    classes, (unk20, recorded, unk24, bgm, win_bgm, unk2a, unk2c, star_level) = fields[12:16], fields[16:24]
    events, unk3c = fields[24:30], fields[30:54]
    reserved, messages = fields[54:54 + RESERVED], fields[54 + RESERVED:]
    check(all(r == -1 for r in reserved), "a movie's reserved halfwords aren't -1")
    scenes = []
    for i in range(SCENES):
        turn, unk02_, unk04, unk06, reaction, score, *rest = SCENE.unpack_from(data, HEADER.size + i * SCENE.size)
        scene_messages, steps, choices = rest[:2 * STEPS], rest[2 * STEPS:3 * STEPS], rest[3 * STEPS:]
        scenes.append({"turn": turn, "unk02": unk02_, "unk04": unk04, "unk06": unk06, "reaction": reaction,
                       "score": score, "messages": [scene_messages[:STEPS], scene_messages[STEPS:]], "events": steps,
                       "choices": [dict(zip(CHOICE_FIELDS, choices[6 * c:6 * c + 6])) for c in range(CHOICES)]})
    return {"$schema": "../../movie.schema.json", "movie": movie, "unk02": unk02, "score_multiplier": multiplier,
            "cast_count": cast_count, "director": director, "turn_limit": turn_limit, "ideal_turn": ideal_turn,
            "ending_scene": ending_scene, "rule": rule,
            "species": [name("SPECIES_", species1), name("SPECIES_", species2)], "unk16": unk16,
            "trainer_classes": {sex: {"film": name("TRAINER_CLASS_", classes[2 * g]),
                                      "watch": name("TRAINER_CLASS_", classes[2 * g + 1])}
                                for g, sex in enumerate(["male", "female"])},
            "unk20": unk20, "recorded_trainer_class": name("TRAINER_CLASS_", recorded), "unk24": unk24,
            "bgm": name("SEQ_", bgm), "win_bgm": name("SEQ_", win_bgm), "unk2a": name("SEQ_", unk2a),
            "unk2c": name("SEQ_", unk2c), "star_level": star_level,
            "turn_events": [{"turn": events[2 * i], "value": events[2 * i + 1]} for i in range(3)],
            "unk3c": [unk3c[4 * i:4 * i + 4] for i in range(6)],
            "turn_messages": [messages[:20], messages[20:]], "scenes": scenes}


def movie_bytes(movie: dict, where: str) -> bytes:
    if len(movie["scenes"]) != SCENES:
        raise DataError(f"{where}: a movie has {SCENES} scenes")
    classes = movie["trainer_classes"]
    data = HEADER.pack(
        movie["movie"], movie["unk02"], movie["score_multiplier"], movie["cast_count"], movie["director"],
        movie["turn_limit"], movie["ideal_turn"], movie["ending_scene"], movie["rule"],
        *(value(s, where) for s in movie["species"]), movie["unk16"],
        *(value(classes[sex][side], where) for sex in ("male", "female") for side in ("film", "watch")),
        movie["unk20"], value(movie["recorded_trainer_class"], where), movie["unk24"],
        *(value(movie[key], where) for key in ("bgm", "win_bgm", "unk2a", "unk2c")), movie["star_level"],
        *(e[key] for e in movie["turn_events"] for key in ("turn", "value")),
        *(n for record in movie["unk3c"] for n in record), *([-1] * RESERVED),
        *movie["turn_messages"][0], *movie["turn_messages"][1])
    for scene in movie["scenes"]:
        data += SCENE.pack(scene["turn"], scene["unk02"], scene["unk04"], scene["unk06"], scene["reaction"],
                           scene["score"], *scene["messages"][0], *scene["messages"][1], *scene["events"],
                           *(c[key] for c in scene["choices"] for key in CHOICE_FIELDS))
    return data


def cast_json(data: bytes) -> dict:
    counts = COUNTS.unpack_from(data)
    check(all(count >= 0 for count in counts), "a cast has a negative count")
    check(len(data) == COUNTS.size + MEMBER.size * sum(counts), "a cast file isn't its members' size")
    members = [member_json(data, COUNTS.size + MEMBER.size * i) for i in range(sum(counts))]
    return {"$schema": "../../cast.schema.json", "player": members[:counts[0]], "opponent": members[counts[0]:]}


def cast_bytes(cast: dict, where: str) -> bytes:
    data = COUNTS.pack(*(len(cast[side]) for side in SIDES))
    return data + b"".join(member_bytes(member, f"{where}: {side} {i}") for side in SIDES
                           for i, member in enumerate(cast[side]))


def dump(files: Path, output: Path):
    casts = read_narc((files / CAST).read_bytes())
    movies = read_narc((files / MOVIES).read_bytes())
    for movie, data in enumerate(casts):
        write(output / str(movie) / "cast.json", cast_json(data))
    for movie, data in enumerate(movies[:-1]):
        write(output / str(movie) / "movie.json", movie_json(data))
    (output.parent / DEFAULT_RECORD).write_bytes(movies[-1])
    print(f"wrote {len(casts)} movies' casts and {len(movies) - 1} movies to {output}, and the default record")


def numbered(root: Path, file_name: str) -> list[Path]:
    paths = []
    while (root / str(len(paths)) / file_name).exists():
        paths.append(root / str(len(paths)) / file_name)
    return paths


def pack(root: Path, outputs: list[Path]):
    cast_schema = load_schema(root.parent / "cast.schema.json")
    movie_schema = load_schema(root.parent / "movie.schema.json")
    casts = [cast_bytes(load(path, cast_schema), label(path)) for path in numbered(root, "cast.json")]
    movies = [movie_bytes(load(path, movie_schema), label(path)) for path in numbered(root, "movie.json")]
    movies.append((root.parent / DEFAULT_RECORD).read_bytes())
    for output, archive in zip(outputs, [casts, movies]):
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(write_narc(archive))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/pokewood/movies/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build Pokéstar Studios' archives")
    pack_parser.add_argument("root", type=Path)
    pack_parser.add_argument("outputs", type=Path, nargs=2, metavar="ARCHIVE",
                             help="the casts' and the movies' archives")
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.root, args.outputs)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
