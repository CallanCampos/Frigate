#!/usr/bin/env python3
"""Find editable field graphics by zone filename, for example nacrene_city_7."""

import json
import sys
from pathlib import Path

from graphics_paths import directories


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.data.datajson import value  # noqa: E402


def find(zone):
    path = ROOT / "data/zones" / f"{zone}.json"
    header = json.loads(path.read_text())
    area_path = ROOT / "data/areas" / (header["area"].removeprefix("AREA_").lower() + ".json")
    area = json.loads(area_path.read_text())
    print(f"Zone: {path.relative_to(ROOT)}")
    print(f"Area: {area_path.relative_to(ROOT)}")
    print(f"Textures: {directories('a/0/1/4')[value(area['texture_set'], zone)].relative_to(ROOT)}")
    kind = "exterior" if area["exterior"] else "interior"
    if isinstance(area["prop_bundle"], str):
        bundle = value(area["prop_bundle"], zone)
        props, textures = (("a/2/2/5", "a/1/7/4") if kind == "exterior" else ("a/2/2/6", "a/1/7/5"))
        print(f"Props: {directories(props)[bundle].relative_to(ROOT)}")
        print(f"Prop textures: {directories(textures)[bundle].relative_to(ROOT)}")
    matrix = header["matrix"].removeprefix("MAP_MATRIX_").lower()
    matrix_path = ROOT / "data/map_matrices" / f"{matrix}.json"
    if not matrix_path.exists():
        print(f"Map matrix: {header['matrix']} (no named matrix file)")
        return
    models = sorted({value(cell, zone) for row in json.loads(matrix_path.read_text())["maps"]
                     for cell in row if cell is not None})
    print(f"Map matrix: {matrix_path.relative_to(ROOT)} ({len(models)} models)")
    if len(models) > 100:
        print("Shared overworld matrix: inspect its grid to identify the specific model.")
    else:
        folders = directories("a/0/0/8")
        for model in models:
            print(f"  {folders[model].relative_to(ROOT)}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: find_graphics.py ZONE_NAME")
    find(sys.argv[1])
