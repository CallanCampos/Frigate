"""Shared code for the game data in JSON (data/pokemon/ and the like): constants by name and by value, validation
against the data's JSON schemas, and writing JSON the way the sources are laid out.

The packers that build archives from the JSON run under the build's python3, so this uses only the standard library.
"""
import json
import re
import sys
from functools import cache
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.gen_constants import LISTS_DIR, parse  # noqa: E402

ROOT = LISTS_DIR.parents[1]
HEADERS_DIR = ROOT / "include" / "constants"
DEFINE = re.compile(r"^#define (\w+) (.+?)\s*(?://.*)?$", re.MULTILINE)
EXPRESSION = re.compile(r"[\w\s()<>|+\-*]+")


@cache
def constants() -> dict[str, int]:
    """Returns every constant of the lists in data/constants/ and of the hand-written headers in include/constants/,
    by name. A header's value can be a number, (1 << N) or another constant."""
    values: dict[str, int] = {}
    for path in sorted(LISTS_DIR.glob("*.txt")):
        values.update(parse(path))
    pending = []
    for path in sorted(HEADERS_DIR.glob("*.h")):
        pending += DEFINE.findall(path.read_text())
    while pending:
        unresolved = []
        for name, value in pending:
            try:
                values[name] = evaluate(value, values)
            except NameError:
                unresolved.append((name, value))
            except SyntaxError:
                pass
        if len(unresolved) == len(pending):
            break
        pending = unresolved
    return values


def evaluate(expression: str, values: dict[str, int]) -> int:
    if not EXPRESSION.fullmatch(expression):
        raise SyntaxError(expression)
    words = {word: values[word] for word in re.findall(r"\b[A-Za-z_]\w*\b", expression) if word in values}
    result = eval(re.sub(r"\b(0x[0-9a-fA-F]+|\d+)[uUlL]*\b", r"\1", expression), {"__builtins__": {}}, words)
    if not isinstance(result, int):
        raise SyntaxError(expression)
    return result


@cache
def names(prefix: str) -> dict[int, str]:
    """Returns the first constant with a prefix for each value, to write a value by its name."""
    found: dict[int, str] = {}
    for name, value in constants().items():
        if name.startswith(prefix):
            found.setdefault(value, name)
    return found


def name(prefix: str, value: int) -> str | int:
    """Returns a value's constant with a prefix, or the value itself if it has none."""
    return names(prefix).get(value, value)


def id_name(prefix: str, value: int) -> str | int:
    """Returns an ID's constant with a prefix, or else the ID in hex, as "0x4176", as IDs are read; 0 stays 0."""
    found = name(prefix, value)
    return found if isinstance(found, str) or not value else f"{value:#06x}"


HEX = re.compile(r"0x[0-9a-fA-F]+")


def value(item: str | int, where: str) -> int:
    """Returns the value of a number, a number in hex ("0x4176") or a constant's name."""
    if isinstance(item, int) and not isinstance(item, bool):
        return item
    if isinstance(item, str) and HEX.fullmatch(item):
        return int(item, 16)
    if isinstance(item, str) and item in constants():
        return constants()[item]
    raise DataError(f"{where}: {item!r} is not a number or a known constant")


class DataError(Exception):
    pass


def load_schema(path: Path) -> dict:
    return json.loads(path.read_text())


def validate(data, schema: dict, where: str, root: dict | None = None):
    """Checks data against a schema, using the keywords the data's schemas use: type, properties, required,
    additionalProperties, items, prefixItems, minItems, maxItems, minimum, maximum, pattern, enum, anyOf and $ref to
    $defs.
    "x-constant" names the prefix of the constants a string may be."""
    root = root or schema
    at = f"{where}: " if where else ""
    if "$ref" in schema:
        schema = root["$defs"][schema["$ref"].removeprefix("#/$defs/")]
    if "anyOf" in schema:
        errors = []
        for option in schema["anyOf"]:
            try:
                validate(data, option, where, root)
                break
            except DataError as error:
                errors.append(str(error))
        else:
            raise DataError(" / or: ".join(errors))
    types = schema.get("type")
    if types is not None:
        types = [types] if isinstance(types, str) else types
        checks = {"object": dict, "array": list, "string": str, "boolean": bool, "integer": int,
                  "null": type(None)}
        if not any(isinstance(data, checks[t]) and not (t == "integer" and isinstance(data, bool)) for t in types):
            raise DataError(f"{at}{json.dumps(data)} is not {' or '.join(types)}")
    if "enum" in schema and data not in schema["enum"]:
        raise DataError(f"{at}{json.dumps(data)} is not one of {schema['enum']}")
    if isinstance(data, dict):
        properties = schema.get("properties", {})
        for key in schema.get("required", []):
            if key not in data:
                raise DataError(f"{at}{key} is missing")
        for key, item in data.items():
            if key in properties:
                validate(item, properties[key], f"{where}.{key}" if where else key, root)
            elif schema.get("additionalProperties") is False:
                raise DataError(f"{at}{key} is not a field here")
    if isinstance(data, list):
        if len(data) < schema.get("minItems", 0) or len(data) > schema.get("maxItems", len(data)):
            raise DataError(f"{at}{len(data)} items, not {schema.get('minItems')} to {schema.get('maxItems')}")
        prefix = schema.get("prefixItems", [])
        for i, item in enumerate(data):
            item_schema = prefix[i] if i < len(prefix) else schema.get("items")
            if item_schema:
                validate(item, item_schema, f"{where}[{i}]", root)
    if isinstance(data, int) and not isinstance(data, bool):
        if data < schema.get("minimum", data) or data > schema.get("maximum", data):
            raise DataError(f"{at}{data} is not from {schema.get('minimum')} to {schema.get('maximum')}")
    if isinstance(data, str):
        if "pattern" in schema and not re.search(schema["pattern"], data):
            raise DataError(f"{at}{data!r} does not match {schema['pattern']}")
        prefix = schema.get("x-constant")
        if prefix and not (data.startswith(prefix) and data in constants()) and not (
                schema.get("x-hex") and HEX.fullmatch(data)):
            raise DataError(f"{at}{data!r} is not a known {prefix}* constant")


def load(path: Path, schema: dict) -> dict:
    """Reads and validates a data file."""
    try:
        data = json.loads(path.read_text())
    except json.JSONDecodeError as error:
        raise DataError(f"{path}: {error}") from None
    try:
        validate(data, schema, "")
    except DataError as error:
        raise DataError(f"{label(path)}: {error}") from None
    return data


def label(path: Path) -> str:
    """Returns a path for messages: relative to the repository when it is in it."""
    path = path.resolve()
    return path.relative_to(ROOT).as_posix() if path.is_relative_to(ROOT) else str(path)


def dumps(data, indent: int = 0, width: int = 120) -> str:
    """Writes JSON with four-space indents: an object with a field per line, and an array on one line when it only
    holds numbers, strings and booleans and fits, so that a list of moves or a pair of a level and a move reads as
    one line."""
    if isinstance(data, list) and data and all(not isinstance(v, (dict, list)) for v in data):
        one_line = "[ " + ", ".join(json.dumps(v, ensure_ascii=False) for v in data) + " ]"
        if indent + len(one_line) <= width:
            return one_line
    if not isinstance(data, (dict, list)):
        return json.dumps(data, ensure_ascii=False)
    if not data:
        return "{}" if isinstance(data, dict) else "[]"
    inner = " " * (indent + 4)
    if isinstance(data, dict):
        items = [f"{inner}{json.dumps(k, ensure_ascii=False)}: {dumps(v, indent + 4, width)}" for k, v in data.items()]
        return "{\n" + ",\n".join(items) + "\n" + " " * indent + "}"
    items = [inner + dumps(item, indent + 4, width) for item in data]
    return "[\n" + ",\n".join(items) + "\n" + " " * indent + "]"


def write(path: Path, data, width: int = 120):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(dumps(data, width=width) + "\n")
