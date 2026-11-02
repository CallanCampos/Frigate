#!/usr/bin/env python3
"""The documents check: fails when one of Frigate's Markdown documents has a relative link to a file the repository
doesn't have, an anchor that no heading of its target makes, or a table row that GitHub splits into another number of
cells than its header's. It touches no game functionality itself: it reads the documents that tell a modder where each
part of the game lives and how Frigate runs it (port/README.md, port/docs/ARCHITECTURE.md's feature map, every
directory's guide, the records), as GitHub renders them.

    check_docs.py [--root DIR] [--allowlist FILE] [DOCUMENT...]

The documents are every Markdown file under port/ and the three of Frigate's own under .github/ (the landing page
README.md, CONTRIBUTING.md and PULL_REQUEST_TEMPLATE/port.md; upstream's templates there are upstream's), as git lists
them, tracked or new but not ignored. DOCUMENT... checks only those (paths from the repository's root), against the
whole tree. It prints each problem as DOCUMENT:LINE: KIND: what is wrong, the kinds being

  missing-file        a relative link (an inline link or image, a reference definition, an HTML href or src) to a
                      path that is neither a file git lists nor a directory holding one; a link that starts with / is
                      from the repository's root, as GitHub reads it;
  outside-repository  a relative link that climbs out of the repository;
  missing-anchor      a link's #fragment that is neither a heading's anchor in its Markdown target, by GitHub's rule
                      (the heading's text as it renders, entities decoded and escaped characters kept, lowercased,
                      without what isn't a letter, a mark, a digit, a letter number such as Ⅻ, a circled or squared
                      letter, _, - or a space, and its spaces made hyphens; a heading whose anchor an earlier one has
                      gets the first of -1, -2 and so on that none has), nor an HTML id or name there; or a fragment
                      on another file (or on a Markdown file shown as source, ?plain=1) that isn't one of GitHub's line
                      anchors, #L10 or #L10-L20;
  missing-line        such a line anchor past the end of its file;
  row-cells           a table row with another number of cells than the table's header. GitHub splits a cell at every
                      | that has no backslash right before it, in a code span too (so \\| keeps its pipe in the cell,
                      and so does \\\\|, after an escaped backslash), drops the cells past the header's number and
                      makes up the missing ones empty. A line straight after a table, without the blank line that
                      ends it, is one of its rows, unless it starts another block: a heading, a list item, a quote, a
                      fence, a thematic break, an HTML block (a line of inline HTML, such as <span>x</span>, is a row)
                      or a code block, indented four columns past the table's header row;
  delimiter-cells     a header row and the delimiter row under it (| --- | --- |) with different numbers of cells,
                      where GitHub shows no table at all;
  stray-row           a line that starts and ends with | outside any table: a row that a blank line cut off from the
                      table above it, or one with no header and delimiter row.

Links and rows in fenced code blocks, code spans and HTML comments are left alone, as GitHub doesn't render them, and so
are headings and HTML ids in fenced blocks and comments, and links with a scheme (https:, mailto:). A block quote's
tables are checked as the quote shows them, its > markers taken off. A link's text may run over several lines, as
wrapped prose has it. The check doesn't follow a list's structure: it reads a table's rows, and the code block that
ends one, against the indentation of the table's header row. Nor does it take a heading inside a block quote or after
a list marker (> ## Title, - ## Title) for an anchor, as GitHub does, so a link to one fails. No document rule names a
line length, so lines aren't measured.

The allowlist (check_docs_allowlist.tsv, next to this script; --allowlist FILE for another) names the problems that a
document must keep, each with its reason: a record, which is added to and never rewritten
(port/docs/SYNC.md#records-are-added-to), may keep one. A row gives the document, the kind, the subject (a link's
target as written, or a row's first cell as GitHub splits it) and the reason; it allows that problem wherever it is in
the document, so that lines added above it don't make it stale. Nothing is skipped silently: an allowed problem is
printed with its reason. The check fails, status 1, on any problem the allowlist doesn't name, on a row that names no
problem (when it checks every document) and on a row it can't read; status 0 otherwise. Its tests are
port/tools/tests/test_check_docs.py, and CI's sync-tooling job runs both (.github/workflows/port-ci.yml).
"""
import argparse
import html
import re
import subprocess
import sys
import unicodedata
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from posixpath import normpath
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]
ALLOWLIST = Path(__file__).resolve().with_name("check_docs_allowlist.tsv")
GITHUB_DOCUMENTS = (".github/README.md", ".github/CONTRIBUTING.md", ".github/PULL_REQUEST_TEMPLATE/port.md")
KINDS = ("missing-file", "outside-repository", "missing-anchor", "missing-line", "row-cells", "delimiter-cells",
         "stray-row")
COLUMNS = ("document", "kind", "subject", "reason")

FENCE = re.compile(r"^\s*(`{3,}|~{3,})")
ATX_HEADING = re.compile(r"^ {0,3}(#{1,6})(?:[ \t]+(.*?))?(?:[ \t]+#+)?[ \t]*$")
SETEXT_UNDERLINE = re.compile(r"^ {0,3}(?:=+|-+)[ \t]*$")
HTML_HEADING = re.compile(r"^ {0,3}<h([1-6])(?:\s[^>]*)?>", re.IGNORECASE)
DELIMITER_ROW = re.compile(r"^\s*\|?\s*:?-+:?\s*(?:\|\s*:?-+:?\s*)*\|?\s*$")
# The lines that open an HTML block, CommonMark's seven kinds as GitHub's cmark-gfm has them: <script, <pre, <style or
# <textarea; a comment; <?; a declaration; CDATA; a block-level tag, opening or closing; and a whole tag, opening or
# closing, alone on its line. A line that only starts with inline HTML (<span>x</span>) opens none.
HTML_BLOCK_TAGS = ("address|article|aside|base|basefont|blockquote|body|caption|center|col|colgroup|dd|details|dialog|"
                   "dir|div|dl|dt|fieldset|figcaption|figure|footer|form|frame|frameset|h[1-6]|head|header|hr|html|"
                   "iframe|legend|li|link|main|menu|menuitem|nav|noframes|ol|optgroup|option|p|param|section|source|"
                   "summary|table|tbody|td|tfoot|th|thead|title|tr|track|ul")
HTML_ATTRIBUTE = r"""\s+[A-Za-z_:][A-Za-z0-9_.:-]*(?:\s*=\s*(?:[^\s"'=<>`]+|'[^']*'|"[^"]*"))?"""
HTML_BLOCK = (r"<(?:script|pre|style|textarea)(?:\s|>|$)|<!--|<\?|<![A-Za-z]|<!\[CDATA\["
              rf"|</?(?:{HTML_BLOCK_TAGS})(?:\s|/?>|$)"
              rf"|(?:<[A-Za-z][A-Za-z0-9-]*(?:{HTML_ATTRIBUTE})*\s*/?>|</[A-Za-z][A-Za-z0-9-]*\s*>)\s*$")
# What ends a table besides a blank line, and starts no paragraph: a heading, a list item (an empty one too), a quote, a
# fence, a thematic break, an HTML block
BLOCK_START = re.compile(r"^\s*(?:#{1,6}(?:\s|$)|[-+*](?:\s|$)|\d{1,9}[.)](?:\s|$)|>|`{3,}|~{3,}|(?:[-*_]\s*){3,}$"
                         rf"|(?i:{HTML_BLOCK}))")
QUOTE_MARKER = re.compile(r"^ {0,3}> ?")
REFERENCE_DEFINITION = re.compile(r"^ {0,3}\[[^\]]+\]:[ \t]*(<[^>\n]*>|\S+)", re.MULTILINE)
HTML_LINK = re.compile(r"""\b(?:href|src)\s*=\s*(?:"([^"]*)"|'([^']*)')""", re.IGNORECASE)
HTML_ID = re.compile(r"""\b(?:id|name)\s*=\s*(?:"([^"]*)"|'([^']*)')""", re.IGNORECASE)
LINE_ANCHOR = re.compile(r"^L(\d+)(?:-L(\d+))?$")
SCHEME = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")
# An entity that CommonMark decodes: a decimal or hexadecimal reference, or a name, each with its semicolon
ENTITY = re.compile(r"&(?:#[0-9]{1,7}|#[xX][0-9A-Fa-f]{1,6}|[A-Za-z][A-Za-z0-9]{1,31});")
# Underscores that GitHub renders as emphasis: a run that opens at no letter or digit before it and a run as long that
# closes at none after it, around text that neither starts nor ends with a space. One without its pair stays text.
UNDERSCORE_EMPHASIS = re.compile(r"(?<![^\W_])(_+)(?!\s)(.+?)(?<!\s)\1(?![^\W_])")
# The symbols that Unicode counts as letters (Other_Alphabetic), which GitHub keeps in an anchor: the circled and
# squared Latin letters (Ⓐ, 🄰, 🅐, 🅰)
ALPHABETIC_SYMBOLS = ((0x24B6, 0x24E9), (0x1F130, 0x1F149), (0x1F150, 0x1F169), (0x1F170, 0x1F189))


@dataclass
class Problem:
    document: str
    line: int
    kind: str
    subject: str
    message: str


def listed_files(root: Path) -> set[str]:
    """Every file git lists in ROOT's working tree, tracked or new but not ignored, that is there."""
    output = subprocess.run(["git", "-C", str(root), "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
                            capture_output=True, check=True).stdout
    names = (name.decode("utf-8", "surrogateescape") for name in output.split(b"\0") if name)
    return {name for name in names if (root / name).is_file()}


def directories_of(files: set[str]) -> set[str]:
    found = set()
    for name in files:
        parent = PurePosixPath(name).parent
        while str(parent) != ".":
            found.add(str(parent))
            parent = parent.parent
    return found


def documents_of(files: set[str]) -> list[str]:
    return sorted(name for name in files
                  if name.endswith(".md") and (name.startswith("port/") or name in GITHUB_DOCUMENTS))


def blank_keeping_lines(text: str) -> str:
    """The text with every character but its line ends made a space, so that offsets and lines stay where they were."""
    return re.sub(r"[^\n]", " ", text)


def mask_comments(text: str) -> str:
    """HTML comments blanked, over as many lines as they run."""
    return re.sub(r"<!--.*?-->", lambda m: blank_keeping_lines(m.group(0)), text, flags=re.DOTALL)


def mask_code_spans(text: str) -> str:
    """A block's code spans blanked: a run of backticks to the next run of as many, as CommonMark pairs them. Escaped
    punctuation goes too, so that \\[ or \\] starts or ends no link."""
    out = list(text)
    i = 0
    while i < len(text):
        if text[i] == "\\" and i + 1 < len(text) and text[i + 1] != "\n":
            out[i] = out[i + 1] = " "
            i += 2
            continue
        if text[i] != "`":
            i += 1
            continue
        j = i
        while j < len(text) and text[j] == "`":
            j += 1
        run = j - i
        closer = re.compile(r"(?<!`)`{%d}(?!`)" % run).search(text, j)
        if closer is None:
            i = j
            continue
        for k in range(i, closer.end()):
            if out[k] != "\n":
                out[k] = " "
        i = closer.end()
    return "".join(out)


def link_destination(text: str, start: int) -> str | None:
    """The destination of an inline link whose ( is at START, as CommonMark reads it: <...>, or characters other than
    spaces with balanced parentheses, then an optional title, then ). None when that isn't a link's."""
    i = start + 1
    i = skip_spaces(text, i, allow_line_end=True)
    if i < len(text) and text[i] == "<":
        end = text.find(">", i + 1)
        if end < 0 or "\n" in text[i + 1:end]:
            return None
        destination = text[i + 1:end]
        i = end + 1
    else:
        depth, begin = 0, i
        while i < len(text) and not text[i].isspace():
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                if depth == 0:
                    break
                depth -= 1
            i += 1
        destination = text[begin:i]
    i = skip_spaces(text, i, allow_line_end=True)
    if i < len(text) and text[i] in "\"'(":
        closing = ")" if text[i] == "(" else text[i]
        end = text.find(closing, i + 1)
        if end < 0:
            return None
        i = skip_spaces(text, end + 1, allow_line_end=True)
    return destination if i < len(text) and text[i] == ")" else None


def skip_spaces(text: str, i: int, allow_line_end: bool) -> int:
    line_ends = 0
    while i < len(text) and text[i] in " \t\n":
        if text[i] == "\n":
            line_ends += 1
            if not allow_line_end or line_ends > 1:
                break
        i += 1
    return i


def opens_link(text: str, close: int) -> bool:
    """Whether the ] at CLOSE has a [ before it in the block that it closes."""
    depth = 0
    for i in range(close - 1, -1, -1):
        if text[i] == "]":
            depth += 1
        elif text[i] == "[":
            if depth == 0:
                return True
            depth -= 1
    return False


def heading_text(source: str) -> str:
    """A heading's text as GitHub renders it, for its anchor: code spans' contents, links' and HTML's text, entities
    decoded, without the marks of emphasis, and escaped characters kept as text, so that \\_ is an underscore."""
    literals = []

    def literal(text):
        literals.append(text)
        return f"\0{len(literals) - 1}\0"

    text = re.sub(r"(`+)(.+?)(?<!`)\1(?!`)", lambda m: literal(m.group(2).strip()), source)
    text = re.sub(r"!\[[^\]]*\]\([^)]*\)", "", text)
    text = re.sub(r"\[([^\]]*)\](?:\([^)]*\)|\[[^\]]*\])", r"\1", text)
    text = re.sub(r"<[^>]+>", "", text)
    text = re.sub(r"\\([!-/:-@\[-`{-~])", lambda m: literal(m.group(1)), text)
    text = ENTITY.sub(lambda m: literal(html.unescape(m.group(0))), text)
    text = text.replace("*", "")
    while UNDERSCORE_EMPHASIS.search(text):
        text = UNDERSCORE_EMPHASIS.sub(r"\2", text)
    return re.sub(r"\0(\d+)\0", lambda m: literals[int(m.group(1))], text).strip()


def slug(heading: str) -> str:
    """The anchor GitHub gives a Markdown heading of this source, before its -1, -2 for a repeat."""
    return text_slug(heading_text(heading))


def text_slug(text: str) -> str:
    """The anchor GitHub gives a heading whose rendered text is TEXT: lowercased, without what isn't a letter, a mark,
    a digit, a letter number (Ⅻ), a circled or squared letter, _, - or a space, and its spaces made hyphens."""
    kept = []
    for ch in text.lower():
        category = unicodedata.category(ch)
        if (ch in "- " or category[0] in "LM" or category in ("Nd", "Nl", "Pc")
                or any(first <= ord(ch) <= last for first, last in ALPHABETIC_SYMBOLS)):
            kept.append(ch)
    return "".join(kept).replace(" ", "-")


class Tree:
    """The repository's files and its Markdown documents' anchors, read once."""

    def __init__(self, root: Path):
        self.root = root
        self.files = listed_files(root)
        self.directories = directories_of(self.files)
        self.texts = {}
        self.anchor_sets = {}

    def text(self, name: str) -> str:
        if name not in self.texts:
            self.texts[name] = (self.root / name).read_text(encoding="utf-8", errors="replace")
        return self.texts[name]

    def anchors(self, name: str) -> set[str]:
        if name not in self.anchor_sets:
            self.anchor_sets[name] = document_anchors(mask_comments(self.text(name)))
        return self.anchor_sets[name]


def document_anchors(text: str) -> set[str]:
    """Every anchor a Markdown document has: its headings' slugs, an HTML heading's (<h1>) from its text as it is,
    spaces and all, and its HTML ids and names. A heading whose slug an earlier heading has taken gets the first of -1,
    -2 and so on that none has, counted per slug, as GitHub numbers them. TEXT has its HTML comments blanked."""
    found, counts = set(), {}
    lines = text.split("\n")
    fence = None
    paragraph = []
    i = 0
    while i < len(lines):
        line = lines[i]
        fence, fenced = fence_state(fence, line)
        i += 1
        if fenced:
            paragraph = []
            continue
        base = None
        match = ATX_HEADING.match(line)
        html_heading = HTML_HEADING.match(line)
        if match:
            base = slug(match.group(2) or "")
            paragraph = []
        elif html_heading:
            block = line
            while f"</h{html_heading.group(1)}" not in block.lower() and i < len(lines):
                block += "\n" + lines[i]
                i += 1
            inner = re.search(r"<h[1-6][^>]*>(.*?)(?:</h[1-6]\s*>|$)", block, re.IGNORECASE | re.DOTALL).group(1)
            base = text_slug(html.unescape(re.sub(r"<[^>]*>", "", inner)))
            paragraph = []
            line = block
        elif SETEXT_UNDERLINE.match(line) and paragraph and not paragraph[-1].strip().startswith("|"):
            base = slug(" ".join(part.strip() for part in paragraph))
            paragraph = []
        elif not line.strip():
            paragraph = []
        elif not paragraph and BLOCK_START.match(line):
            paragraph = []
        else:
            paragraph.append(line)
        if base is not None:
            anchor = base
            while anchor in counts:
                counts[base] += 1
                anchor = f"{base}-{counts[base]}"
            counts[anchor] = 0
            found.add(anchor)
        for match in HTML_ID.finditer(line):
            found.add(match.group(1) if match.group(1) is not None else match.group(2))
    return found


def fence_state(fence, line: str):
    """(the open fence after LINE, whether LINE is fenced): a fence opens with three or more ` or ~ and closes with as
    many or more of the same."""
    match = FENCE.match(line)
    if fence is None:
        if match:
            return match.group(1), True
        return None, False
    if match and match.group(1)[0] == fence[0] and len(match.group(1)) >= len(fence) \
            and not line.strip()[len(match.group(1)):].strip():
        return None, True
    return fence, True


def split_cells(row: str) -> list[str]:
    """A table row's cells as GitHub splits them: at every | that has no backslash right before it, in code spans too,
    without the empty cells before a leading | and after a trailing one. cmark-gfm reads a backslash and the | after it
    as one escaped pipe even when the backslash is itself escaped, so \\\\| keeps its pipe in the cell too."""
    text = row.strip()
    cells, current = [], []
    for i, ch in enumerate(text):
        if ch == "|" and (i == 0 or text[i - 1] != "\\"):
            cells.append("".join(current))
            current = []
        else:
            current.append(ch)
    cells.append("".join(current))
    if text.startswith("|"):
        cells = cells[1:]
    if len(cells) > 1 and text.endswith("|") and not text.endswith("\\|"):
        cells = cells[:-1]
    return [cell.strip() for cell in cells]


def looks_like_row(line: str) -> bool:
    text = line.strip()
    return len(text) > 1 and text.startswith("|") and text.endswith("|") and not text.endswith("\\|")


def counted(number: int, noun: str) -> str:
    return f"{number} {noun}" if number == 1 else f"{number} {noun}s"


def indentation(line: str) -> int:
    """A line's indentation in columns, a tab taking it to the next multiple of 4 as CommonMark counts."""
    expanded = line.expandtabs(4)
    return len(expanded) - len(expanded.lstrip(" "))


def fence_flags(lines: list[str]) -> list[bool]:
    """Whether each line is in a fenced code block, its fences included."""
    flags, fence = [], None
    for line in lines:
        fence, inside = fence_state(fence, line)
        flags.append(inside)
    return flags


def quotes(lines: list[str], fenced: list[bool]):
    """Each block quote outside fenced blocks, as (its first line's index, its lines without one level of > marker)."""
    i = 0
    while i < len(lines):
        if fenced[i] or not QUOTE_MARKER.match(lines[i]):
            i += 1
            continue
        j = i
        while j < len(lines) and not fenced[j] and QUOTE_MARKER.match(lines[j]):
            j += 1
        yield i, [QUOTE_MARKER.sub("", line, count=1) for line in lines[i:j]]
        i = j


def check_tables(document: str, lines: list[str], fenced: list[bool], first: int = 0) -> tuple[list[Problem], set[int]]:
    """The tables' problems, and the indexes of the lines that are table rows (whose text is no paragraph's). FIRST is
    the index in the document of LINES' first line; a block quote's lines are checked again without their markers."""
    problems, rows = [], set()
    i = 0
    after_table = False
    while i < len(lines):
        line = lines[i]
        if fenced[i]:
            after_table = False
            i += 1
            continue
        delimiter = (i + 1 < len(lines) and not fenced[i + 1] and "|" in lines[i + 1]
                     and DELIMITER_ROW.match(lines[i + 1]) and line.strip() and not BLOCK_START.match(line))
        if delimiter:
            header = split_cells(line)
            columns = len(split_cells(lines[i + 1]))
            if len(header) != columns:
                problems.append(Problem(document, first + i + 1, "delimiter-cells", header[0],
                                        f"a table header of {counted(len(header), 'cell')} over a delimiter row of "
                                        f"{columns}, so GitHub shows no table here"))
                j = i + 2
                while j < len(lines) and lines[j].strip() and not fenced[j]:
                    j += 1
                rows.update(range(first + i, first + j))
                i, after_table = j, False
                continue
            rows.update((first + i, first + i + 1))
            code = indentation(line) + 4  # a line indented this far past the header is a code block, not a row
            j = i + 2
            while (j < len(lines) and lines[j].strip() and not fenced[j] and not BLOCK_START.match(lines[j])
                   and indentation(lines[j]) < code):
                cells = split_cells(lines[j])
                rows.add(first + j)
                if len(cells) != len(header):
                    if len(cells) > len(header):
                        why = (": GitHub splits a cell at every | without a backslash right before it, in a code span "
                               f"too, and drops the cells past the header's {len(header)}")
                    elif not looks_like_row(lines[j]):
                        why = ": a line straight after a table is one of its rows, and a blank line ends the table"
                    else:
                        why = ", so GitHub makes up the missing cells empty"
                    problems.append(Problem(document, first + j + 1, "row-cells", cells[0],
                                            f"a table row of {counted(len(cells), 'cell')} under a header of "
                                            f"{len(header)}{why}"))
                j += 1
            after_table = True
            while j < len(lines) and lines[j].strip() and not fenced[j] and indentation(lines[j]) >= code:
                j, after_table = j + 1, False  # the code block that ends the table: no row of it
            i = j
            continue
        if looks_like_row(line):
            if after_table:
                why = "a blank line cuts it off from the table above"
            else:
                why = "no header and delimiter row above it"
            problems.append(Problem(document, first + i + 1, "stray-row", split_cells(line)[0],
                                    f"a table row outside a table: {why}"))
        elif line.strip():
            after_table = False
        i += 1
    for start, quoted in quotes(lines, fenced):
        quoted_problems, quoted_rows = check_tables(document, quoted, fence_flags(quoted), first + start)
        problems += quoted_problems
        rows |= quoted_rows
    return problems, rows


def blocks_of(lines: list[str], fenced: list[bool], rows: set[int]):
    """The document's blocks of inline text, as (first line's index, text): paragraphs and the like run to a blank line,
    a heading or a table; each table row is a block of its own."""
    block, first = [], 0
    for i, line in enumerate(lines):
        alone = i in rows or ATX_HEADING.match(line)
        if fenced[i] or not line.strip() or alone:
            if block:
                yield first, "\n".join(block)
                block = []
            if alone and not fenced[i]:
                yield i, line
            continue
        if not block:
            first = i
        block.append(line)
    if block:
        yield first, "\n".join(block)


def link_targets(text: str):
    """(offset, target) of each link in a block whose code spans are blanked: inline links and images, reference
    definitions, HTML hrefs and srcs."""
    for match in re.finditer(r"\]\(", text):
        if opens_link(text, match.start()):
            destination = link_destination(text, match.start() + 1)
            if destination is not None:
                yield match.start(), destination
    for line_start in [0] + [m.end() for m in re.finditer(r"\n", text)]:
        definition = REFERENCE_DEFINITION.match(text, line_start)
        if definition:
            yield line_start, definition.group(1).strip("<>")
    for match in HTML_LINK.finditer(text):
        yield match.start(), match.group(1) if match.group(1) is not None else match.group(2)


def check_link(tree: Tree, document: str, line: int, target: str) -> Problem | None:
    if not target or SCHEME.match(target) or target.startswith("//"):
        return None
    path, _, fragment = target.partition("#")
    path, _, query = path.partition("?")
    path, fragment = unquote(path), unquote(fragment)
    if path:
        base = "" if path.startswith("/") else str(PurePosixPath(document).parent)
        resolved = normpath(str(PurePosixPath(base, path.lstrip("/"))))
        if resolved == ".." or resolved.startswith("../"):
            return Problem(document, line, "outside-repository", target,
                           f"links to {target}, outside the repository")
        if resolved != "." and resolved not in tree.files and resolved not in tree.directories:
            return Problem(document, line, "missing-file", target,
                           f"links to {target}, but the repository has no {resolved}")
    else:
        resolved = document
    if not fragment or resolved not in tree.files:
        return None
    if resolved.endswith(".md") and "plain=1" not in query.split("&"):
        if fragment not in tree.anchors(resolved):
            return Problem(document, line, "missing-anchor", target,
                           f"links to {target}, but no heading of {resolved} makes the anchor #{fragment}, nor any id")
        return None
    numbers = LINE_ANCHOR.match(fragment)
    if not numbers:
        return Problem(document, line, "missing-anchor", target,
                       f"links to {target}, but {resolved} shows as source here, whose only anchors are lines, #L10 or "
                       "#L10-L20")
    length = tree.text(resolved).count("\n") + (0 if tree.text(resolved).endswith("\n") else 1)
    if int(numbers.group(2) or numbers.group(1)) > length:
        return Problem(document, line, "missing-line", target,
                       f"links to {target}, past the end of {resolved}, which has {length} lines")
    return None


def check_document(tree: Tree, document: str) -> list[Problem]:
    text = mask_comments(tree.text(document))
    lines = text.split("\n")
    fenced = fence_flags(lines)
    problems, rows = check_tables(document, lines, fenced)
    for first, block in blocks_of(lines, fenced, rows):
        masked = mask_code_spans(block)
        for offset, target in link_targets(masked):
            line = first + block.count("\n", 0, offset) + 1
            problem = check_link(tree, document, line, target)
            if problem:
                problems.append(problem)
    return sorted(problems, key=lambda p: p.line)


def read_allowlist(path: Path) -> tuple[dict, list[str]]:
    """The allowlist's rows by (document, kind, subject), and what is wrong with any row it can't take."""
    rows, errors = {}, []
    if not path.exists():
        return rows, errors
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        fields = line.split("\t")
        if not line.strip() or line.startswith("#") or tuple(fields) == COLUMNS:
            continue
        if len(fields) != len(COLUMNS):
            errors.append(f"{path.name}:{number}: {len(fields)} columns, not {len(COLUMNS)} ({', '.join(COLUMNS)})")
            continue
        document, kind, subject, reason = (field.strip() for field in fields)
        if kind not in KINDS:
            errors.append(f"{path.name}:{number}: no kind {kind!r}; the kinds are {', '.join(KINDS)}")
        elif not reason:
            errors.append(f"{path.name}:{number}: no reason: every exception says why the document keeps it")
        else:
            rows[(document, kind, subject)] = {"reason": reason, "line": number, "used": False}
    return rows, errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--root", type=Path, default=ROOT, help="the checkout to check (default: this one)")
    parser.add_argument("--allowlist", type=Path,
                        help="the known exceptions (default: port/tools/check_docs_allowlist.tsv of the checkout)")
    parser.add_argument("documents", nargs="*", help="only these documents, from the repository's root")
    args = parser.parse_args()
    root = args.root.resolve()
    allowlist = args.allowlist or root / ALLOWLIST.relative_to(ROOT)

    tree = Tree(root)
    documents = documents_of(tree.files)
    if args.documents:
        wanted = [name.replace("\\", "/") for name in args.documents]
        unknown = [name for name in wanted if name not in documents]
        if unknown:
            print(f"check_docs: not one of the documents it checks: {', '.join(unknown)}")
            return 1
        documents = wanted
    allowed, errors = read_allowlist(allowlist)

    failed = passed = 0
    for document in documents:
        for problem in check_document(tree, document):
            row = allowed.get((problem.document, problem.kind, problem.subject))
            where = f"{problem.document}:{problem.line}: {problem.kind}: {problem.message}"
            if row:
                row["used"] = True
                passed += 1
                print(f"{where} (allowed: {row['reason']})")
            else:
                failed += 1
                print(where)
    stale = [] if args.documents else [(key, row) for key, row in allowed.items() if not row["used"]]
    for (document, kind, subject), row in stale:
        print(f"{allowlist.name}:{row['line']}: allows a {kind} in {document} ({subject}) that the check no longer "
              "finds: remove the row")
    for error in errors:
        print(error)
    print(f"check_docs: {counted(len(documents), 'document')}, {counted(failed + passed, 'problem')} "
          f"({passed} allowed), {counted(len(stale) + len(errors), 'allowlist row')} to fix")
    return 1 if failed or stale or errors else 0


if __name__ == "__main__":
    sys.exit(main())
