#!/usr/bin/env python3
"""Count a complete file's code and data as matched in an objdiff progress report.

    complete_report.py build/b2_us/report.objdiff.json build/b2_us/report.json

objdiff scores a function by the text of its instructions, symbol names included, so a call that names its target
differently in the original's object and in ours is a mismatch even where both names are one address: a name the
configs haven't caught up with (func_ov167_0219f6ec, where the C calls BtlFaintRec_NextTurn), the runtime's two names
for one routine (_ull_mul and _ll_mul), or a string literal the compiler names (@1776). No option of `report generate`
changes that. For an incomplete file objdiff's score is the evidence there is. A complete file is linked from our
compiled object, though, and both ROMs are checked byte for byte, so each of its functions and each byte of its data
matches. This marks them so, and recomputes the measures of the report and of each category from its units.
"""
import argparse
import json
from pathlib import Path

# Byte counts, which the report writes as strings
BYTES = ["total_code", "matched_code", "total_data", "matched_data", "complete_code", "complete_data"]
COUNTS = ["total_functions", "matched_functions", "total_units", "complete_units"]


def percent(part: float, whole: float) -> float:
    return part / whole * 100 if whole else 100.0


def aggregate(units: list[dict]) -> dict:
    """Returns the measures of a group of units, as objdiff adds them up: sums, and the fuzzy match weighted by code."""
    sums = {key: sum(int(u["measures"].get(key, 0)) for u in units) for key in BYTES + COUNTS}
    fuzzy = sum(u["measures"].get("fuzzy_match_percent", 0.0) * int(u["measures"].get("total_code", 0)) for u in units)
    return {
        "fuzzy_match_percent": fuzzy / sums["total_code"] if sums["total_code"] else 100.0,
        "total_code": str(sums["total_code"]),
        "matched_code": str(sums["matched_code"]),
        "matched_code_percent": percent(sums["matched_code"], sums["total_code"]),
        "total_data": str(sums["total_data"]),
        "matched_data": str(sums["matched_data"]),
        "matched_data_percent": percent(sums["matched_data"], sums["total_data"]),
        "total_functions": sums["total_functions"],
        "matched_functions": sums["matched_functions"],
        "matched_functions_percent": percent(sums["matched_functions"], sums["total_functions"]),
        "complete_code": str(sums["complete_code"]),
        "complete_code_percent": percent(sums["complete_code"], sums["total_code"]),
        "complete_data": str(sums["complete_data"]),
        "complete_data_percent": percent(sums["complete_data"], sums["total_data"]),
        "total_units": sums["total_units"],
        "complete_units": sums["complete_units"],
    }


def complete_unit(unit: dict):
    """Marks every function, section and byte of a complete unit as matched."""
    m = unit["measures"]
    m["matched_code"], m["matched_data"] = m.get("total_code", "0"), m.get("total_data", "0")
    m["matched_functions"] = m.get("total_functions", 0)
    m["fuzzy_match_percent"] = m["matched_code_percent"] = 100.0
    m["matched_data_percent"] = m["matched_functions_percent"] = 100.0
    for item in unit.get("functions", []) + unit.get("sections", []):
        item["fuzzy_match_percent"] = 100.0


def complete_report(report: dict) -> dict:
    units = report["units"]
    for unit in units:
        if unit.get("metadata", {}).get("complete"):
            complete_unit(unit)
    report["measures"] = aggregate(units)
    for category in report.get("categories", []):
        members = [u for u in units if category["id"] in u.get("metadata", {}).get("progress_categories", [])]
        category["measures"] = aggregate(members)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", type=Path, help="the report that objdiff-cli wrote")
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    args.output.write_text(json.dumps(complete_report(json.loads(args.input.read_text()))))


if __name__ == "__main__":
    main()
