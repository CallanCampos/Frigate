"""Names for field graphics, derived from the game's map and area references."""

import json
from collections import defaultdict
from functools import lru_cache
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2] / "data"


@lru_cache(maxsize=1)
def references():
    zones = defaultdict(list)
    areas = {}
    for path in (ROOT / "zones").glob("*.json"):
        data = json.loads(path.read_text())
        if "area" in data:
            zones[data["area"]].append(path.stem)
    for path in (ROOT / "areas").glob("*.json"):
        if path.stem.isdigit():
            areas[int(path.stem)] = json.loads(path.read_text())
    chunks = defaultdict(list)
    for path in (ROOT / "map_matrices").glob("*.json"):
        data = json.loads(path.read_text())
        if "maps" not in data:
            continue
        for row in data["maps"]:
            for chunk in row:
                if chunk is not None:
                    chunks[chunk].append(path.stem)
    return zones, areas, chunks


def location(names):
    names = sorted(set(names))
    if not names:
        return None
    if len(names) == 1:
        return names[0]
    parts = [name.split("_") for name in names]
    common = []
    for tokens in zip(*parts):
        if len(set(tokens)) != 1:
            break
        common.append(tokens[0])
    return "_".join(common) if len(common) >= 2 else None


def map_model_name(chunk, embedded):
    matrices = references()[2].get(chunk, [])
    names = [name for name in matrices if name != "overworld" and not name.isdigit()]
    place = location(names) or (sorted(names, key=len)[0] if names else
                                "overworld" if "overworld" in matrices else None)
    return f"{place}_{embedded}" if place else embedded


def map_texture_name(set_id, first_texture):
    zones, areas, _ = references()
    names = [zone for area_id, area in areas.items() if area["texture_set"] == set_id
             for zone in zones[area_id]]
    place = location(names)
    return f"{place}_textures" if place else f"shared_{first_texture}_textures"


def prop_bundle_name(kind, bundle_id, first_model):
    zones, areas, _ = references()
    names = [zone for area_id, area in areas.items() if area["prop_bundle"] == bundle_id
             and area["exterior"] == (kind == "exterior") for zone in zones[area_id]]
    place = location(names)
    return f"{place}_props" if place else f"shared_{first_model}_props"
