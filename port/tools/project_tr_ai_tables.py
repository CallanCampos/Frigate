#!/usr/bin/env python3
"""Project exact trainer-AI commands from src/ov170/tr_ai.c for ROM-free VM regressions.

The owning source's headers/types/prototypes and five unchanged definitions are compiled, without its unrelated
120-callback table or battle initialization. This avoids ELF/COFF/ASan retention differences; no linker exception,
replacement command body or sanitizer exclusion is used. Deliberately accepts this source's simple C definitions,
not a general C parser. Missing/duplicate definitions or malformed lexical boundaries fail before replacing output.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import tempfile

FUNCTIONS = ("AIIfResultInList", "AIIfResultNotInList", "AIEnd", "AIJumpByMoveEffect", "GetMoveData")
TABLE = r"(?m)^static\s+const\s+VMCommand\s+sTrAICommands\s*\[\s*\]\s*=\s*\{"


def code_mask(text: str) -> str:
    """Keep code positions/newlines, masking comments and literals so their braces cannot delimit C bodies."""
    masked = list(text)
    i = 0
    while i < len(text):
        start = i
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            if end < 0:
                raise ValueError("unterminated block comment")
            i = end + 2
        elif text.startswith("//", i):
            i += 2
            while i < len(text) and text[i] != "\n":
                if text.startswith("\\\r\n", i):
                    i += 3
                elif text.startswith("\\\n", i):
                    i += 2
                else:
                    i += 1
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < len(text) and text[i] != quote:
                if text[i] in "\r\n":
                    raise ValueError("newline in C literal")
                if text.startswith("\\\r\n", i):
                    i += 3
                elif text[i] == "\\":
                    if i + 1 < len(text) and text[i + 1] == "\r":
                        raise ValueError("raw CR in C literal")
                    i += 2
                else:
                    i += 1
            if i >= len(text):
                raise ValueError("unterminated C literal")
            i += 1
        else:
            i += 1
            continue
        for j in range(start, i):
            if text[j] != "\n":
                masked[j] = " "
    return "".join(masked)


def project(text: str, source: str) -> str:
    mask = code_mask(text)
    tables = list(re.finditer(TABLE, mask))
    if len(tables) != 1:
        raise ValueError("expected one trainer-AI command table boundary")
    prefix_end = tables[0].start()
    definitions = []
    for name in FUNCTIONS:
        pattern = rf"(?m)^[A-Za-z_][\w \t*]*\b{re.escape(name)}\s*\([^;{{}}]*\)\s*\{{"
        found = list(re.finditer(pattern, mask))
        if len(found) != 1 or found[0].start() < prefix_end:
            raise ValueError(f"expected one complete definition after the command table: {name}")
        match = found[0]
        depth = 1
        end = match.end()
        while end < len(mask) and depth:
            depth += (mask[end] == "{") - (mask[end] == "}")
            end += 1
        if depth:
            raise ValueError(f"unclosed function body: {name}")
        definitions.append((match.start(), end))
    definitions.sort()
    if any(a[1] > b[0] for a, b in zip(definitions, definitions[1:])):
        raise ValueError("overlapping command definitions")
    # Relative/original paths in #line keep diagnostics on the actual game owner; escape only C string syntax.
    filename = source.replace("\\", "\\\\").replace('"', '\\"')
    out = ["// Generated from actual trainer-AI source; do not edit.\n", f'#line 1 "{filename}"\n', text[:prefix_end]]
    for start, end in definitions:
        line = text.count("\n", 0, start) + 1
        out.extend((f'\n#line {line} "{filename}"\n', text[start:end], "\n"))
    return "".join(out)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    try:
        with args.source.open("r", encoding="utf-8", newline="") as stream:
            output = project(stream.read(), str(args.source))
    except ValueError as error:
        parser.error(str(error))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n", dir=args.out.parent,
                                         delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(output)
        os.replace(temporary, args.out)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    main()
