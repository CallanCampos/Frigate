"""Stable archive IDs with readable asset directory names."""

import re
from functools import lru_cache


def named_dir(parent, index, width, name):
    slug = re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_") or "unnamed"
    return parent / f"{slug}__{index:0{width}d}"


@lru_cache(maxsize=None)
def directory_index(parent, width):
    result = {}
    for path in parent.iterdir():
        if not path.is_dir():
            continue
        match = re.search(rf"(?:^|__)(\d{{{width}}})$|^(\d{{{width}}})_", path.name)
        if match:
            index = int(match.group(1) or match.group(2))
            if index in result:
                raise ValueError(f"multiple asset directories for {parent}/{index:0{width}d}")
            result[index] = path
    return result


def asset_dir(parent, index, width, name=None):
    match = directory_index(parent, width).get(index) if parent.exists() else None
    if match is not None:
        return match
    if parent.exists():
        directory_index.cache_clear()
        match = directory_index(parent, width).get(index)
        if match is not None:
            return match
    if name is None:
        raise FileNotFoundError(f"missing asset directory for {parent}/{index:0{width}d}")
    return named_dir(parent, index, width, name)
