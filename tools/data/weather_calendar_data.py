#!/usr/bin/env python3
"""The weather calendar, res/field/weather/calendar.json (ARCID_CALENDAR, a/0/9/6): dump writes it from the ROM once,
and pack builds the archive from it, which the build does.

    weather_calendar_data.py dump extract/b2_us/files res/field/weather/calendar.json
    weather_calendar_data.py pack res/field/weather/calendar.json ARCHIVE

The calendar gives CALENDAR_ZONE_MAX zones a weather for each day of the year (calender.c): file 1 is their count
and each zone's CalendarZone, its ZONE_* and the offset of its days in file 0, which has a byte for each of 366 days,
February with 29, that Calendar_GetDayOfYear counts. A zone the calendar doesn't have keeps its own weather
(GetZoneEnvFlagsWeather). The file gives each zone's days by month, each day's weather by its WEATHER_* name of
system/zone_weather.h.

Both versions have the same file. calendar.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, name, value, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ZONE_MAX = 24  # CALENDAR_ZONE_MAX
MONTHS = {"january": 31, "february": 29, "march": 31, "april": 30, "may": 31, "june": 30, "july": 31, "august": 31,
          "september": 30, "october": 31, "november": 30, "december": 31}
DAYS = sum(MONTHS.values())
ZONE = struct.Struct("<HH")  # CalendarZone
# The weathers of system/zone_weather.h
WEATHERS = ["WEATHER_SUNNY", "WEATHER_FLAKE", "WEATHER_RAIN", "WEATHER_SANDSTORM", "WEATHER_BLIZZARD",
            "WEATHER_FLUTTER", "WEATHER_THUNDERSTORM", "WEATHER_GUSTY_RAIN", "WEATHER_DIAMOND_DUST", "WEATHER_TINT_9",
            "WEATHER_TINT_10", "WEATHER_TINT_11", "WEATHER_SANDSTORM_2", "WEATHER_TINT_13", "WEATHER_TINT_14"]


def dump(files: Path, output: Path):
    days, zones = read_narc((files / "a/0/9/6").read_bytes())
    (count,) = struct.unpack_from("<H", zones)
    if count != ZONE_MAX or len(zones) != 2 + ZONE.size * ZONE_MAX or len(days) != DAYS * ZONE_MAX:
        raise ValueError("the calendar isn't 24 zones of 366 days")
    entries = []
    for i in range(ZONE_MAX):
        zone, offset = ZONE.unpack_from(zones, 2 + ZONE.size * i)
        if offset != DAYS * i or any(day >= len(WEATHERS) for day in days[offset:offset + DAYS]):
            raise ValueError(f"zone {i}'s days aren't in order or have an unknown weather")
        weather, start = {}, offset
        for month, length in MONTHS.items():
            weather[month] = [WEATHERS[day] for day in days[start:start + length]]
            start += length
        entries.append({"zone": name("ZONE_", zone), "weather": weather})
    write(output, {"$schema": "calendar.schema.json", "zones": entries}, width=1000)
    print(f"wrote the weather of {len(entries)} zones to {output}")


def pack(path: Path, output: Path):
    zones = load(path, load_schema(path.parent / "calendar.schema.json"))["zones"]
    days = b"".join(bytes(WEATHERS.index(day) for month in MONTHS for day in zone["weather"][month]) for zone in zones)
    table = struct.pack("<H", len(zones)) + b"".join(
        ZONE.pack(value(zone["zone"], f"{label(path)}: zone {i}"), DAYS * i) for i, zone in enumerate(zones))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([days, table]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write calendar.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the weather calendar's archive")
    pack_parser.add_argument("path", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.path, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()
