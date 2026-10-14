"""The directories of the field graphics. Each file of a field graphics archive is a directory named after its constant
in a data/constants/ list, in whose order the archive has them, and each model of a prop bundle is a directory of its
bundle's, named after the model and numbered where a name repeats."""

import re
from pathlib import Path

from gen_constants import ordered_files


ROOT = Path(__file__).resolve().parents[2]
# Each archive's list, the prefix of its constants and the directory of the files named after them. A prop bundle and
# its textures are the same file of their two archives, so they share a list.
ARCHIVES = {
    "a/0/0/8": ("map_models", "MAP_MODEL_", "data/map_models"),
    "a/0/1/4": ("map_texture_sets", "MAP_TEXTURE_SET_", "data/map_texture_sets"),
    "a/2/2/5": ("field_props_exterior", "FIELD_PROPS_EXTERIOR_", "data/field_prop_models/exterior"),
    "a/2/2/6": ("field_props_interior", "FIELD_PROPS_INTERIOR_", "data/field_prop_models/interior"),
    "a/1/7/4": ("field_props_exterior", "FIELD_PROPS_EXTERIOR_", "data/field_prop_textures/exterior"),
    "a/1/7/5": ("field_props_interior", "FIELD_PROPS_INTERIOR_", "data/field_prop_textures/interior"),
}


def directories(archive, root=None):
    """Returns the directories of an archive's files in archive order, under root or else the archive's own."""
    list_name, prefix, directory = ARCHIVES[archive]
    return ordered_files(list_name, prefix, Path(root) if root is not None else ROOT / directory, "")


def slug(name):
    return re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_") or "unnamed"


def numbered(names):
    """Returns each name as a directory name, the second and later of a repeated name numbered from _2, skipping a
    number that another name already has."""
    names = [slug(name) for name in names]
    taken = set(names)
    counts = {}
    result = []
    for name in names:
        if name not in counts:
            counts[name] = 1
            result.append(name)
            continue
        number = counts[name] + 1
        while f"{name}_{number}" in taken:
            number += 1
        counts[name] = number
        taken.add(f"{name}_{number}")
        result.append(f"{name}_{number}")
    return result
