#!/usr/bin/env python3
"""Write objdiff.json for one version, with paths relative to the repository root.

dsd writes paths relative to the version's config directory, such as `config/b2_us/arm9/../../../build/...`. Normalizing
them makes them match the ninja targets that objdiff asks ninja to rebuild.

Each unit also gets progress categories, which objdiff copies into the report and ByteMatch shows separately: the
module group it's in (ARM9 main with its autoloads, or the overlays) and what wrote it (the game, or one of the
libraries of lib/).
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from configure import ENCODING_FLAG, FILE_FLAGS, MERGED_SOURCES, source_compiler  # noqa: E402

# The libraries of lib/, by directory, for their category names
LIBRARY_NAMES = {
    "dpw": "DPW",
    "dsprot": "DS Protect",
    "dwc": "DWC",
    "nitro": "NitroSDK",
    "nnsys": "NitroSystem",
    "spl": "SPL",
    "twl": "TwlSDK",
}


def normalize(value):
    if isinstance(value, dict):
        return {key: normalize(item) for key, item in value.items()}
    if isinstance(value, list):
        return [normalize(item) for item in value]
    if isinstance(value, str) and "../" in value:
        return os.path.normpath(value)
    return value


def source_modules(version: str) -> dict[str, str]:
    """Maps each source file of the version's delinks to the module group it's in, main or overlays."""
    modules = {}
    arm9 = ROOT / "config" / version / "arm9"
    for delinks in arm9.rglob("delinks.txt"):
        group = "overlays" if delinks.parent.parent.name == "overlays" else "main"
        for line in delinks.read_text().splitlines():
            if line.endswith(":") and not line[0].isspace():
                modules[line[:-1]] = group
    return modules


def categorize(config: dict, version: str):
    """Gives every unit its progress categories and lists them in the config."""
    modules = source_modules(version)
    libraries = set()
    for unit in config.get("units", []):
        metadata = unit.setdefault("metadata", {})
        source = metadata.get("source_path")
        if source:
            group = modules.get(source, "main")
            # A part of a merged file is its whole's code, in its whole's library (config/merged_sources.toml)
            parts = Path(MERGED_SOURCES.get(source, source)).parts
            if parts[0] == "lib":
                libraries.add(parts[1])
                origin = ["lib", f"lib/{parts[1]}"]
            else:
                origin = ["game"]
        else:
            # A gap, code not in any source file yet, named after its module, such as _dsd_gap@ov036_2. An overlay's is
            # the game's, while main's may be the game's or a library's, so it counts toward neither.
            group = "overlays" if unit["name"].split("@", 1)[-1].startswith("ov") else "main"
            origin = ["game"] if group == "overlays" else []
        metadata["progress_categories"] = [group, *origin]
    config["progress_categories"] = [
        {"id": "main", "name": "ARM9 main"},
        {"id": "overlays", "name": "Overlays"},
        {"id": "game", "name": "Game code"},
        {"id": "lib", "name": "Libraries"},
        *({"id": f"lib/{name}", "name": LIBRARY_NAMES.get(name, name)} for name in sorted(libraries)),
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version")
    parser.add_argument("--dsd", required=True)
    parser.add_argument("--compiler", required=True, help="decomp.me compiler name")
    parser.add_argument("--c-flags", required=True, help="compiler flags for decomp.me scratches")
    parser.add_argument("-o", "--output", type=Path, default=ROOT / "objdiff.json")
    args = parser.parse_args()

    result = subprocess.run(
        [args.dsd, "objdiff", "--config-path", f"config/{args.version}/arm9/config.yaml", "--stdout", "--scratch",
         "--compiler", args.compiler, "--c-flags", args.c_flags, "--custom-make", "ninja"],
        cwd=ROOT, capture_output=True, text=True, check=True,
    )
    config = normalize(json.loads(result.stdout))
    # A part of a merged file compares its whole's object, which holds its code (config/merged_sources.toml)
    for unit in config.get("units", []):
        source = unit.get("metadata", {}).get("source_path")
        if source in MERGED_SOURCES and "base_path" in unit:
            whole = Path(MERGED_SOURCES[source]).with_suffix(".o").as_posix()
            unit["base_path"] = unit["base_path"].replace(Path(source).with_suffix(".o").as_posix(), whole)
            unit.pop("scratch", None)
    # A source file that isn't written yet has no object to compare, only the delinked one
    for unit in config.get("units", []):
        source = unit.get("metadata", {}).get("source_path")
        if source and source not in MERGED_SOURCES and not (ROOT / source).exists():
            unit.pop("base_path", None)
            unit.pop("scratch", None)
    # --c-flags remains the default for ordinary units. Game overrides replace those
    # defaults with effective build flags; scratch contexts are UTF-8, not Shift JIS.
    for unit in config.get("units", []):
        source = unit.get("metadata", {}).get("source_path")
        if source and source.startswith("src/") and source in FILE_FLAGS and "scratch" in unit:
            settings = source_compiler(Path(source))
            unit["scratch"]["c_flags"] = " ".join(flag for flag in settings.flags if flag != ENCODING_FLAG)
    categorize(config, args.version)
    args.output.write_text(json.dumps(config, indent=2) + "\n")


if __name__ == "__main__":
    main()
