"""The messages that come from the game data in JSON rather than from the text files, as pokeplatinum's text banks come
from res/: a line `\\from{species.name}` in a file of data/text/ stands for the names of all the species, one per line,
from data/pokemon/. text_data.py expands these lines when it packs the text.

In the JSON, text is written as in the text files (see text_data.py), except that a line break is a real one ("\\n" in
the JSON) rather than `\\n`: control codes stay `{TTTT:a,b}`, and other characters `\\x{HHHH}`.
"""
import json
import re
import sys
from functools import cache
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import ROOT, DataError, load, load_schema  # noqa: E402
from tools.data.gen_constants import load as load_list  # noqa: E402


def to_json(line: str) -> str:
    """Returns a line of a text file as JSON text: the `\\n` escapes become line breaks."""
    out = []
    i = 0
    while i < len(line):
        if line[i] == "\\" and i + 1 < len(line):
            out.append("\n" if line[i + 1] == "n" else line[i:i + 2])
            i += 2
        else:
            out.append(line[i])
            i += 1
    return "".join(out)


def to_line(text: str) -> str:
    """Returns JSON text as a line of a text file."""
    return text.replace("\n", "\\n")


ESCAPE = re.compile(r"(\\x\{[0-9a-fA-F]+\}|\{[^}]*\}|\\.)")


def upper(line: str) -> str:
    """Returns a line in capitals, leaving its escapes and control codes as they are."""
    return "".join(part if ESCAPE.fullmatch(part) else part.upper() for part in ESCAPE.split(line))


def ordered(list_name: str) -> list[str]:
    """The constants of a list in value order, the first one of each value: a list's second names for a value, such
    as ITEM_LAST, are left out."""
    first: dict[int, str] = {}
    for name, number in load_list(list_name).items():
        first.setdefault(number, name)
    return [first[number] for number in sorted(first)]


@cache
def species() -> list[dict]:
    """The species' data.json files, in the order of species.txt (not the forms or extra records)."""
    return [read(ROOT / "data/pokemon" / name.removeprefix("SPECIES_").lower() / "data.json")
            for name in ordered("species")]


@cache
def moves() -> list[dict]:
    return [read(ROOT / "data/moves" / name.removeprefix("MOVE_").lower() / "data.json") for name in ordered("moves")]


@cache
def items() -> list[dict]:
    return [read(ROOT / "data/items" / name.removeprefix("ITEM_").lower() / "data.json") for name in ordered("items")]


@cache
def abilities() -> list[dict]:
    return validated("abilities", "data/abilities", "ABILITY_", "ability.schema.json")


@cache
def types() -> list[dict]:
    # TYPE_NULL, the type of a typeless move, has no name
    return validated("types", "data/types", "TYPE_", "type.schema.json", skip={"TYPE_NULL"})


@cache
def trainer_classes() -> list[dict]:
    return validated("trainer_classes", "data/trainer_classes", "TRAINER_CLASS_", "trainer_class.schema.json")


@cache
def places() -> list[dict]:
    return validated("places", "data/places", "PLACE_", "place.schema.json")


@cache
def natures() -> list[dict]:
    return validated("natures", "data/natures", "NATURE_", "nature.schema.json")


def validated(list_name: str, directory: str, prefix: str, schema_name: str, skip=frozenset()) -> list[dict]:
    """The files of data that only the text uses, which nothing else validates, in the order of their list."""
    schema = load_schema(ROOT / directory / schema_name)
    return [load(ROOT / directory / (name.removeprefix(prefix).lower() + ".json"), schema)
            for name in ordered(list_name) if name not in skip]


@cache
def trades() -> list[dict]:
    return [read(ROOT / "data/trades" / (name.removeprefix("TRADE_").lower() + ".json")) for name in ordered("trades")]


@cache
def trainers() -> dict[str, dict]:
    """The trainers' files by constant, in the order of trainers.txt, without TRAINER_NONE, which has none."""
    return {name: read(ROOT / "data/trainers" / (name.removeprefix("TRAINER_").lower() + ".json"))
            for name in ordered("trainers")[1:]}


def read(path: Path) -> dict:
    try:
        return json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise DataError(f"{path}: {error}") from None


def field(data: dict, key: str, where: str) -> str:
    if key not in data:
        raise DataError(f"{where}: {key} is missing")
    return data[key]


def article(data: dict) -> str:
    """The article of a species' name: name_article if the file has one, else "an" before a vowel and "a" before
    anything else."""
    return data.get("name_article") or ("an" if data["name"][:1] in "AEIOU" else "a")


def item_article(data: dict) -> str:
    """The article of an item's name: name_article if the file has one, which can be "" for none, else as a species'."""
    return data["name_article"] if "name_article" in data else ("an" if data["name"][:1] in "AEIOU" else "a")


def item_with_article(data: dict) -> str:
    if "name_with_article" in data:
        return to_line(data["name_with_article"])
    spoken = item_article(data)
    return f"{{bd01}}{spoken + ' ' if spoken else ''}{{ff00:255}}{to_line(data['name'])}"


def message_order() -> list[str]:
    """The trainers whose messages the trainer message table has, in its order: the ones of
    data/trainers/message_order.json, then any other trainer with messages, in ID order."""
    listed = json.loads((ROOT / "data/trainers/message_order.json").read_text())
    for name in listed:
        if not trainers().get(name, {}).get("messages"):
            raise DataError(f"data/trainers/message_order.json: {name} is not a trainer with messages")
    rest = [name for name, data in trainers().items() if data.get("messages") and name not in listed]
    return listed + rest


def trainer_messages() -> list[str]:
    return [to_line(message["text"]) for name in message_order() for message in trainers()[name]["messages"]]


def pokedex_lines(path: list[str]) -> list[str]:
    """species.pokedex.<field>, species.pokedex.forms.<field>, species.pokedex.<language>.<field> and
    species.pokedex.<language>.forms.<field>: a field of every species' Pokédex text, of their alternate forms', in
    species order, of the species that have it in a language, which come first, or of their forms in the language."""
    lines = []
    for i, data in enumerate(species()):
        pokedex = field(data, "pokedex", f"species {i}")
        if path[0] == "forms":
            # SPECIES_NONE has a count of 0
            alternate_forms = max(data["forms"]["count"] - 1, 0)
            forms = pokedex.get("forms", [])
            if len(forms) != alternate_forms:
                raise DataError(f"species {i}: pokedex.forms has {len(forms)} forms for {alternate_forms} "
                                "alternate forms")
            lines += [to_line(form[path[1]]) for form in forms]
        elif len(path) == 1:
            lines.append(to_line(field(pokedex, path[0], f"species {i}: pokedex")))
        elif path[1] == "forms":
            lines += [to_line(form[path[2]]) for form in pokedex["languages"].get(path[0], {}).get("forms", [])]
        else:
            language = pokedex["languages"].get(path[0], {})
            if path[1] not in language:
                break
            lines.append(to_line(language[path[1]]))
    return lines


def species_lines(key: str, format_line=to_line) -> list[str]:
    return [format_line(field(data, key, f"species {i}")) for i, data in enumerate(species())]


SOURCES = {
    "species.name": lambda: species_lines("name"),
    "species.name_upper": lambda: species_lines("name", lambda name: upper(to_line(name))),
    # From species 1: SPECIES_NONE's line is empty
    "species.name_with_article": lambda: [f"{{bd01}}{article(data)} {{ff00:255}}{to_line(data['name'])}"
                                          for data in species()[1:]],

    "moves.name": lambda: [to_line(data["name"]) for data in moves()],
    "moves.name_upper": lambda: [upper(to_line(data["name"])) for data in moves()],
    "moves.description": lambda: [to_line(data["description"]) for data in moves()],
    "items.name": lambda: [to_line(data["name"]) for data in items()],
    # From item 1: ITEM_NONE's are "???"
    "items.name_with_article": lambda: [item_with_article(data) for data in items()[1:]],
    "items.name_plural": lambda: [to_line(data["name_plural"]) for data in items()[1:]],
    "items.description": lambda: [to_line(data["description"]) for data in items()],
    "abilities.name": lambda: [to_line(data["name"]) for data in abilities()],
    "abilities.description": lambda: [to_line(data["description"]) for data in abilities()],
    # From ability 1: ABILITY_NONE's name in capitals isn't its name's
    "abilities.name_upper": lambda: [upper(to_line(data["name"])) for data in abilities()[1:]],
    "types.name": lambda: [to_line(data["name"]) for data in types()],
    "types.name_upper": lambda: [upper(to_line(data["name"])) for data in types()],
    "trainer_classes.name": lambda: [("\\c" if data.get("compress_name") else "") + to_line(data["name"])
                                     for data in trainer_classes()],
    "trainer_classes.name_with_article": lambda: [to_line(data["name_with_article"]) for data in trainer_classes()],
    "natures.name": lambda: [to_line(data["name"]) for data in natures()],
    "places.name": lambda: [to_line(data["name"]) for data in places()],
    # Two lines per trade, its nickname and its trainer's name, the message IDs trade_data.py gives it
    "trades.names": lambda: [to_line(data[key]) for data in trades() for key in ("nickname", "trainer_name")],
    "trainers.name": lambda: [("\\c" if data.get("compress_name", True) else "") + to_line(data["name"])
                              for data in trainers().values()],
    "trainers.messages": trainer_messages,
}


def facility_lines(source: str) -> list[str] | None:
    """facilities.<facility>.names or .messages: a battle facility's trainers' names or messages (facility_data.py)."""
    parts = source.split(".")
    if len(parts) != 3 or parts[0] != "facilities" or parts[2] not in ("names", "messages"):
        return None
    from tools.data.facility_data import FACILITIES, text_lines

    if parts[1] not in FACILITIES:
        raise DataError(f"\\from{{{source}}}: no facility {parts[1]}")
    return [to_line(line) for line in text_lines(parts[1], parts[2])]


def expand(source: str) -> list[str]:
    if source.startswith("species.pokedex."):
        return pokedex_lines(source.split(".")[2:])
    lines = facility_lines(source)
    if lines is not None:
        return lines
    if source not in SOURCES:
        raise DataError(f"\\from{{{source}}}: no such source; there are {', '.join(SOURCES)}")
    return SOURCES[source]()
