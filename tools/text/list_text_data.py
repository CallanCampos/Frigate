#!/usr/bin/env python3
"""Write the text of the constant lists whose data is the code's as JSON, one file per constant, from the ROM's text,
once:

    list_text_data.py extract/b2_us/files data

data/abilities/<ability>.json has an ability's name and description, data/types/<type>.json a type's name,
data/trainer_classes/<class>.json a trainer class's name and name with its article, data/natures/<nature>.json a
nature's name, and data/places/<place>.json a place's name, named after their constants. They have no archive of their own: the text files take them with
\\from{abilities.name} and the like (see text_sources.py), which validates them against their schemas.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import write  # noqa: E402
from tools.text.text_data import message_lines  # noqa: E402
from tools.text.text_sources import ordered, to_json  # noqa: E402

# Files of the system messages (a/0/0/2) with the abilities' names and descriptions, the types' names, the trainer
# classes' names and names with their articles, and the natures' names
ABILITY_NAMES, ABILITY_DESCRIPTIONS, TYPE_NAMES = 374, 375, 398
CLASS_NAMES, CLASS_NAMES_WITH_ARTICLE, NATURE_NAMES = 383, 485, 27
PLACE_NAMES = 109


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("files", type=Path, help="the extracted files/ directory")
    parser.add_argument("output", type=Path, help="the data directory")
    args = parser.parse_args()
    text = args.files / "a/0/0/2"
    names, descriptions = message_lines(text, ABILITY_NAMES), message_lines(text, ABILITY_DESCRIPTIONS)
    for i, ability in enumerate(ordered("abilities")):
        write(args.output / "abilities" / f"{ability.removeprefix('ABILITY_').lower()}.json",
              {"$schema": "ability.schema.json", "name": to_json(names[i]), "description": to_json(descriptions[i])})
    type_names = message_lines(text, TYPE_NAMES)
    for i, type_name in enumerate(type_names):
        write(args.output / "types" / f"{ordered('types')[i].removeprefix('TYPE_').lower()}.json",
              {"$schema": "type.schema.json", "name": to_json(type_name)})
    class_names, with_article = message_lines(text, CLASS_NAMES), message_lines(text, CLASS_NAMES_WITH_ARTICLE)
    for i, trainer_class in enumerate(ordered("trainer_classes")):
        data = {"$schema": "trainer_class.schema.json", "name": to_json(class_names[i].removeprefix("\\c"))}
        if class_names[i].startswith("\\c"):
            data["compress_name"] = True
        data["name_with_article"] = to_json(with_article[i])
        write(args.output / "trainer_classes" / f"{trainer_class.removeprefix('TRAINER_CLASS_').lower()}.json", data)
    natures = message_lines(text, NATURE_NAMES)
    for i, nature in enumerate(ordered("natures")):
        write(args.output / "natures" / f"{nature.removeprefix('NATURE_').lower()}.json",
              {"$schema": "nature.schema.json", "name": to_json(natures[i])})
    places = message_lines(text, PLACE_NAMES)
    for i, place in enumerate(ordered("places")):
        write(args.output / "places" / f"{place.removeprefix('PLACE_').lower()}.json",
              {"$schema": "place.schema.json", "name": to_json(places[i])})
    print(f"wrote {len(names)} abilities, {len(type_names)} types, {len(class_names)} trainer classes, "
          f"{len(natures)} natures and {len(places)} places to {args.output}")


if __name__ == "__main__":
    main()
