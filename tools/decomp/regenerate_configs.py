#!/usr/bin/env python3
"""Regenerate the dsd configs of every version, such as after improving dsd's analysis.

The source files listed in each delinks.txt are kept. Afterwards, names are imported again from swan, and the fixes in
config/fixes.txt and the names in config/names.txt are applied. Any other manual changes to the configs are lost.
"""
import argparse
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import ROOT  # noqa: E402

VERSIONS = ["b2_us", "w2_us"]
PRIMARY = "b2_us"


def saved_files(config: Path) -> dict[Path, str]:
    """Returns the file entries of every delinks.txt, which follow the first blank line."""
    files = {}
    for delinks in config.rglob("delinks.txt"):
        text = delinks.read_text()
        _, sep, rest = text.partition("\n\n")
        if sep and rest.strip():
            files[delinks.relative_to(config)] = rest
    return files


def run(*command: str):
    print("+", " ".join(command))
    subprocess.run(command, cwd=ROOT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("swan", type=Path, help="path to a clone of the swan repository")
    parser.add_argument("--dsd", type=Path, default=ROOT / "tools" / "dsd")
    args = parser.parse_args()

    python = sys.executable
    scripts = ROOT / "tools" / "decomp"
    for version in VERSIONS:
        config = ROOT / "config" / version
        files = saved_files(config)
        shutil.rmtree(config, ignore_errors=True)
        shutil.rmtree(ROOT / "build" / version, ignore_errors=True)
        run(str(args.dsd), "init", "--rom-config", f"extract/{version}/config.yaml", "--output-path", f"config/{version}",
            "--build-path", f"build/{version}", "--allow-unknown-function-calls")
        for path, entries in files.items():
            delinks = config / path
            delinks.write_text(delinks.read_text().rstrip("\n") + "\n\n" + entries)

    other = next(v for v in VERSIONS if v != PRIMARY)
    (ROOT / "build").mkdir(exist_ok=True)
    version_map = (python, str(scripts / "version_map.py"), PRIMARY, other, "-o", "build/version_map.tsv",
                   "--symbols-output", "build/version_map_symbols.tsv")
    run(*version_map)
    run(python, str(scripts / "import_swan.py"), str(args.swan), "--primary", PRIMARY, "--other", other,
        "--map", "build/version_map.tsv", "--symbols-map", "build/version_map_symbols.tsv")
    run(python, str(scripts / "config_fixes.py"), "apply")
    # The fixes add symbols, which the names in config/names.txt need paired across versions
    run(*version_map)
    run(python, str(scripts / "rename_symbol.py"), "--apply")


if __name__ == "__main__":
    main()
