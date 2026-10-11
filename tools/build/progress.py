#!/usr/bin/env python3
"""Print a summary of an objdiff progress report."""
import argparse
import json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report")
    args = parser.parse_args()

    measures = json.load(open(args.report))["measures"]
    code, total_code = int(measures.get("matched_code", 0)), int(measures["total_code"])
    functions, total_functions = measures.get("matched_functions", 0), measures["total_functions"]
    units, total_units = measures.get("complete_units", 0), measures["total_units"]
    print(f"Code:      {code:>9,} / {total_code:>9,} bytes ({code / total_code:.3%})")
    print(f"Functions: {functions:>9,} / {total_functions:>9,} ({functions / total_functions:.3%})")
    print(f"Files:     {units:>9,} / {total_units:>9,} complete")


if __name__ == "__main__":
    main()
