#!/usr/bin/env python3
"""Find editable field graphics by zone filename, for example nacrene_city_7."""

import json
import sys
from pathlib import Path

from graphics_paths import asset_dir


ROOT = Path(__file__).resolve().parents[2]


def find(zone):
    path = ROOT / "data/zones" / f"{zone}.json"
    header = json.loads(path.read_text())
    area_id = header["area"]
    area = json.loads((ROOT / "data/areas" / f"{area_id:03d}.json").read_text())
    print(f"Zone: {path.relative_to(ROOT)}")
    print(f"Area: {area_id}")
    print(f"Textures: {asset_dir(ROOT / 'data/map_texture_sets', area['texture_set'], 3).relative_to(ROOT)}")
    kind = "exterior" if area["exterior"] else "interior"
    bundle = area["prop_bundle"]
    prop_root = ROOT / "data/field_prop_models" / kind
    if bundle in range(len([p for p in prop_root.iterdir() if p.is_dir()])):
        print(f"Props: {asset_dir(prop_root, bundle, 3).relative_to(ROOT)}")
        print(f"Prop textures: {asset_dir(ROOT / 'data/field_prop_textures' / kind, bundle, 3).relative_to(ROOT)}")
    matrix = header["matrix"].removeprefix("MAP_MATRIX_").lower()
    matrix_path = ROOT / "data/map_matrices" / f"{matrix}.json"
    if not matrix_path.exists():
        print(f"Map matrix: {header['matrix']} (no named matrix file)")
        return
    chunks = sorted({chunk for row in json.loads(matrix_path.read_text())["maps"]
                     for chunk in row if chunk is not None})
    print(f"Map matrix: {matrix_path.relative_to(ROOT)} ({len(chunks)} chunks)")
    if len(chunks) > 100:
        print("Shared overworld matrix: inspect its grid to identify the specific chunk.")
    else:
        root = ROOT / "data/map_models"
        for chunk in chunks:
            print(f"  {asset_dir(root, chunk, 4).relative_to(ROOT)}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: find_graphics.py ZONE_NAME")
    find(sys.argv[1])
