#!/usr/bin/env python3
"""Give the field script commands their handlers' current names, after rename_symbol.py renamed handlers.

    sync_handler_names.py              # rewrite the manifest, the macros and the scripts
    sync_handler_names.py --dry-run    # list the macros that would be renamed

tools/script/field_commands.json names each command's handler. A handler still named by its address (func_...) is
looked up at that address in config/b2_us/arm9's symbols.txt and takes the name it has there; a handler that
rename_symbol.py renamed after the manifest last changed takes its new name the same way, through HEAD's manifest.
Then include/asm/field_script.inc is generated again, and the macros whose names changed are renamed in the scripts
of res/**/*.s, matched command by command, the old names by HEAD's field_script.py and the new by the working tree's (the base commands by number, a plugin's by plugin, overlay and number).
The build checks the scripts: they must assemble to the same bytes.
"""
import argparse
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
import tools.script.field_script as field_script  # noqa: E402

MANIFEST = ROOT / "tools" / "script" / "field_commands.json"
INC = ROOT / "include" / "asm" / "field_script.inc"
CONFIG = ROOT / "config" / "b2_us" / "arm9"


def symbols_by_address() -> dict[tuple[str, int], str]:
    """The function name at each (module, address); the module is "main" or an overlay such as "ov056"."""
    names = {}
    for path in CONFIG.rglob("symbols.txt"):
        module = path.parent.name if path.parent.parent.name == "overlays" else "main"
        for line in path.read_text().splitlines():
            m = re.match(r"(\S+) kind:function\S* addr:0x([0-9a-f]+)", line)
            if m:
                names.setdefault((module, int(m.group(2), 16)), m.group(1))
    return names


def handler_addresses(old_manifest: dict, by_address: dict) -> dict[str, tuple[str, int]]:
    """The (module, address) of each handler name of HEAD's manifest that symbols.txt no longer has."""
    current = set(by_address.values())
    at = {}
    for (module, addr), name in by_address.items():
        at.setdefault(name, (module, addr))
    found = {}
    names = config_names()
    for name in handlers(old_manifest):
        m = re.fullmatch(r"func_(?:(ov\d+)_)?([0-9a-f]{8})", name)
        if m:
            found[name] = (m.group(1) or "main", int(m.group(2), 16))
        elif name not in current and name in names:
            found[name] = names[name]
    return found


def config_names() -> dict[str, tuple[str, int]]:
    """Each function name of HEAD's symbols.txt, at its module and address, to follow a handler renamed since,
    swan's names included."""
    files = subprocess.run(["git", "ls-tree", "-r", "--name-only", "HEAD", "config/b2_us/arm9"], capture_output=True,
                           text=True, cwd=ROOT).stdout.split()
    out = {}
    for path in files:
        if not path.endswith("/symbols.txt"):
            continue
        parent = Path(path).parent
        module = parent.name if parent.parent.name == "overlays" else "main"
        text = subprocess.run(["git", "show", f"HEAD:{path}"], capture_output=True, text=True, cwd=ROOT).stdout
        for line in text.splitlines():
            m = re.match(r"(\S+) kind:function\S* addr:0x([0-9a-f]+)", line)
            if m:
                out.setdefault(m.group(1), (module, int(m.group(2), 16)))
    return out


def handlers(manifest) -> set[str]:
    out = set()
    if isinstance(manifest, dict):
        for key, value in manifest.items():
            if key == "handler" and isinstance(value, str):
                out.add(value)
            else:
                out |= handlers(value)
    elif isinstance(manifest, list):
        for value in manifest:
            out |= handlers(value)
    return out


def head_generator(tmp: Path):
    """HEAD's field_script.py, which made the macros the scripts use now: the old names come from it, in case the
    working tree's names its commands differently."""
    import importlib.util
    source = subprocess.run(["git", "show", "HEAD:tools/script/field_script.py"], capture_output=True, text=True,
                            cwd=ROOT).stdout
    if not source:
        return field_script
    path = tmp / "field_script_head.py"
    path.write_text(source)
    spec = importlib.util.spec_from_file_location("field_script_head", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def macro_names(table: Path, generator=field_script) -> dict:
    """Each command's macro name, by ("base", number) or (plugin, overlay, number)."""
    generator.TABLE = table
    base, plugins, _, _ = generator.load_commands()
    out = {("base", number): command.name for number, command in base.items()}
    for plugin, variants in plugins.items():
        for overlay, commands in variants.items():
            for number, command in commands.items():
                out[(plugin, overlay, number)] = command.name
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dry-run", action="store_true", help="list the renames and change nothing")
    args = parser.parse_args()

    raw = MANIFEST.read_text()
    by_address = symbols_by_address()
    head = subprocess.run(["git", "show", "HEAD:tools/script/field_commands.json"], capture_output=True, text=True,
                          cwd=ROOT).stdout
    renames = {}
    for name, place in handler_addresses(json.loads(head or raw), by_address).items():
        new = by_address.get(place)
        if new and new != name:
            renames[name] = new
    new_raw = re.sub(r'"handler": "([^"]+)"', lambda m: f'"handler": "{renames.get(m.group(1), m.group(1))}"', raw)

    with tempfile.TemporaryDirectory() as tmp:
        old_table, new_table = Path(tmp) / "old.json", Path(tmp) / "new.json"
        old_table.write_text(raw)
        new_table.write_text(new_raw)
        old, new = macro_names(old_table, head_generator(Path(tmp))), macro_names(new_table)
    field_script.TABLE = MANIFEST
    mapping = {}
    for key, name in old.items():
        if new[key] != name:
            if mapping.get(name, new[key]) != new[key]:
                sys.exit(f"{name} would become both {mapping[name]} and {new[key]}")
            mapping[name] = new[key]
    taken = set(old.values()) - set(mapping)
    clash = taken & set(mapping.values())
    if clash:
        sys.exit(f"new macro names already taken: {sorted(clash)}")
    named = set(by_address.values())
    for name in sorted(handlers(json.loads(new_raw)) - named):
        if not name.startswith("func_"):
            print(f"warning: no function is named {name}, and its address is unknown")
    for a, b in sorted(mapping.items()):
        print(f"{a} -> {b}")
    print(f"{len(renames)} handlers, {len(mapping)} macros")
    if args.dry_run or not mapping and new_raw == raw:
        return

    MANIFEST.write_text(new_raw)
    subprocess.run([sys.executable, str(ROOT / "tools" / "script" / "field_script.py"), "inc", str(INC)], check=True,
                   cwd=ROOT)
    if mapping:
        pattern = re.compile(r"\b(" + "|".join(map(re.escape, sorted(mapping, key=len, reverse=True))) + r")\b")
        changed = 0
        for path in sorted((ROOT / "res").rglob("*.s")):
            text = path.read_text()
            new_text = pattern.sub(lambda m: mapping[m.group(1)], text)
            if new_text != text:
                path.write_text(new_text)
                changed += 1
        print(f"{changed} script files")


if __name__ == "__main__":
    main()
