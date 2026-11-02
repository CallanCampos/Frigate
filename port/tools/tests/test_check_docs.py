"""The documents check (port/tools/check_docs.py) on fixture trees: no game functionality, no ROM, no network.

Each test writes a small repository of Markdown documents into a scratch directory and runs the check on it as CI
does. A clean tree that uses every form the check reads (links to files, directories, headings of the same document
and of another, repeated and punctuated headings, HTML ids, line anchors, a Markdown file's source lines, links whose
text runs over two lines, angle brackets and titles, reference definitions, HTML sources, escaped pipes in a table's
code span, a table without outer pipes, a list item's code span continued on a line that starts with |) must pass,
and so must links that GitHub doesn't render (code spans, fenced blocks, HTML comments). Each negative control seeds
one problem, which must be reported with its kind and line: a missing file (in an inline link, a link over two lines,
a reference definition, an HTML src), a link out of the repository, a missing anchor (in another document, in the same
one, a heading inside a fenced block, a fragment on a file that isn't Markdown), a line anchor past the end of a file
or of a Markdown file's source, a row split by a code span's pipes, a short row, a line straight after a table, a
header over a delimiter row of another width, a row cut off by a blank line and a stray row. The allowlist's controls:
an allowed problem passes and is printed with its reason, wherever it moves and at each place it repeats, and a row
that names no problem, a row without a reason, an unknown kind or a row of the wrong width fails. The anchors' rule is
held to headings whose anchors GitHub's renderer gave.
"""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest

CHECKER = Path(__file__).resolve().parents[1] / 'check_docs.py'
spec = importlib.util.spec_from_file_location('check_docs', CHECKER)
check_docs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_docs)

OTHER = '''\
# Other

## A section with `code`

## Repeated

## Repeated

## Odd heading — with a dash

<a id="explicit"></a>
'''

CLEAN = '''\
# Guide

The [other document](other.md), its [section](other.md#a-section-with-code), a [directory](../tools/) and
[this one's own heading](#tables-that-github-reads-right), with a link whose text runs over two lines: [the
second heading of a name](other.md#repeated-1 "a title"), an image ![logo](../logo.svg), a link in angle brackets
[odd](<other.md#odd-heading--with-a-dash>), an id [explicit](other.md#explicit), a line of a script
[line 2](../tools/run.sh#L2) and its lines [1 to 3](../tools/run.sh#L1-L3), a Markdown file's source line
[3](other.md?plain=1#L3), the root's [path](/port/docs/other.md), a [reference][ref] and an
[address](https://example.invalid/missing.md).

Code is left alone: `[not a link](missing.md)` and

```sh
[not a link either](missing.md)
| not | a | row |
```

<!-- [a comment's link](missing.md)
| nor | a | row |
-->

<p align="center"><img alt="logo" src="../logo.svg"></p>

[ref]: other.md#a-section-with-code

## Tables that GitHub reads right

| Code | Why |
| --- | --- |
| `x \\|\\| y` | escaped pipes in a code span |
| [a link](other.md) | b |

Left | Right
:--- | ---:
no outer pipes | fine

- A list item whose code span goes on to the next line, `(a << 8)
  | b`: a continuation, not a row.
'''


class DocsFixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        subprocess.run(['git', 'init', '-q', str(self.root)], check=True)
        self.write('port/docs/other.md', OTHER)
        self.write('port/tools/run.sh', 'one\ntwo\nthree\n')
        self.write('port/logo.svg', '<svg/>\n')

    def tearDown(self):
        self.temp.cleanup()

    def write(self, path, text):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding='utf-8')

    def check(self, *args):
        result = subprocess.run([sys.executable, str(CHECKER), '--root', str(self.root), *args],
                                capture_output=True, text=True)
        return result.returncode, result.stdout

    def problems(self, document):
        """The check's findings on a tree with DOCUMENT as port/docs/guide.md, as (line, kind) pairs."""
        self.write('port/docs/guide.md', textwrap.dedent(document))
        status, output = self.check()
        found = []
        for line in output.splitlines():
            if line.startswith('port/docs/guide.md:'):
                _, number, kind = line.split(':')[:3]
                found.append((int(number), kind.strip()))
        return status, found, output


class CleanTreeTests(DocsFixture):
    def test_every_form_the_check_reads_passes(self):
        status, found, output = self.problems(CLEAN)
        self.assertEqual(found, [], output)
        self.assertEqual(status, 0, output)
        self.assertIn('check_docs: 2 documents, 0 problems (0 allowed)', output)

    def test_only_the_ports_documents_are_read(self):
        self.write('docs/upstream.md', '[upstream\'s own](missing.md)\n')
        self.write('.github/pull_request_template.md', '[upstream\'s template](missing.md)\n')
        self.write('.github/README.md', '[Frigate\'s landing page](../port/docs/other.md#repeated)\n')
        status, output = self.check()
        self.assertEqual(status, 0, output)
        self.assertIn('check_docs: 2 documents', output)

    def test_the_landing_page_is_read(self):
        self.write('.github/README.md', '[Frigate\'s landing page](../port/docs/missing.md)\n')
        status, output = self.check()
        self.assertEqual(status, 1, output)
        self.assertIn('.github/README.md:1: missing-file', output)

    def test_ignored_files_are_not_link_targets(self):
        self.write('.gitignore', 'build/\n')
        self.write('build/out.txt', 'made by a build\n')
        status, found, output = self.problems('[a build\'s output](../../build/out.txt)\n')
        self.assertEqual(found, [(1, 'missing-file')], output)


class LinkControlTests(DocsFixture):
    def test_missing_file(self):
        self.assertEqual(self.problems('Text.\n\nA [missing file](nope.md).\n')[1], [(3, 'missing-file')])

    def test_missing_file_in_a_link_over_two_lines(self):
        status, found, output = self.problems('A link whose [text runs\nover two lines](nope.md).\n')
        self.assertEqual(found, [(2, 'missing-file')], output)
        self.assertEqual(status, 1)

    def test_missing_file_in_a_reference_definition(self):
        self.assertEqual(self.problems('A [reference][ref].\n\n[ref]: nope.md\n')[1], [(3, 'missing-file')])

    def test_missing_file_in_an_html_source(self):
        self.assertEqual(self.problems('<img alt="x" src="nope.svg">\n')[1], [(1, 'missing-file')])

    def test_link_out_of_the_repository(self):
        self.assertEqual(self.problems('[out](../../../outside.md)\n')[1], [(1, 'outside-repository')])

    def test_missing_anchor_in_another_document(self):
        self.assertEqual(self.problems('[absent](other.md#absent)\n')[1], [(1, 'missing-anchor')])

    def test_missing_anchor_in_the_same_document(self):
        self.assertEqual(self.problems('# Guide\n\n[absent](#absent)\n')[1], [(3, 'missing-anchor')])

    def test_a_heading_in_a_fenced_block_is_no_anchor(self):
        found = self.problems('```md\n# Fenced\n```\n\n[fenced](#fenced)\n')[1]
        self.assertEqual(found, [(5, 'missing-anchor')])

    def test_a_repeated_headings_count_is_its_own(self):
        self.assertEqual(self.problems('[third](other.md#repeated-2)\n')[1], [(1, 'missing-anchor')])

    def test_a_fragment_on_a_file_that_isnt_markdown(self):
        self.assertEqual(self.problems('[main](../tools/run.sh#main)\n')[1], [(1, 'missing-anchor')])

    def test_line_anchor_past_the_end(self):
        found = self.problems('[four](../tools/run.sh#L4) and [two to four](../tools/run.sh#L2-L4)\n')[1]
        self.assertEqual(found, [(1, 'missing-line'), (1, 'missing-line')])

    def test_line_anchor_past_the_end_of_a_markdown_files_source(self):
        self.assertEqual(self.problems('[source](other.md?plain=1#L99)\n')[1], [(1, 'missing-line')])


class TableControlTests(DocsFixture):
    TABLE = '| One | Two |\n| --- | --- |\n'

    def test_row_split_by_a_code_spans_pipes(self):
        status, found, output = self.problems(self.TABLE + '| `x || y` | z |\n')
        self.assertEqual(found, [(3, 'row-cells')], output)
        self.assertIn('a table row of 4 cells under a header of 2', output)
        self.assertEqual(status, 1)

    def test_short_row(self):
        found = self.problems(self.TABLE + '| a | b |\n| only one |\n')[1]
        self.assertEqual(found, [(4, 'row-cells')])

    def test_a_line_straight_after_a_table_is_a_row(self):
        status, found, output = self.problems(self.TABLE + '| a | b |\ntext right after the table\n')
        self.assertEqual(found, [(4, 'row-cells')], output)
        self.assertIn('a blank line ends the table', output)

    def test_a_heading_or_a_list_ends_a_table(self):
        found = self.problems(self.TABLE + '| a | b |\n## Heading\n\n' + self.TABLE + '| c | d |\n- item\n')[1]
        self.assertEqual(found, [])

    def test_header_over_a_delimiter_row_of_another_width(self):
        found = self.problems('| One | Two | Three |\n| --- | --- |\n| a | b | c |\n')[1]
        self.assertEqual(found, [(1, 'delimiter-cells')])

    def test_row_cut_off_by_a_blank_line(self):
        status, found, output = self.problems(self.TABLE + '| a | b |\n\n| c | cut off |\n')
        self.assertEqual(found, [(5, 'stray-row')], output)
        self.assertIn('a blank line cuts it off from the table above', output)

    def test_stray_row(self):
        status, found, output = self.problems('Text.\n\n| a stray row | outside any table |\n')
        self.assertEqual(found, [(3, 'stray-row')], output)
        self.assertIn('no header and delimiter row above it', output)


class AllowlistTests(DocsFixture):
    HEADER = 'document\tkind\tsubject\treason\n'

    def allow(self, *rows):
        self.write('port/tools/check_docs_allowlist.tsv', self.HEADER + ''.join('\t'.join(row) + '\n' for row in rows))

    def test_an_allowed_problem_passes_and_says_why(self):
        self.write('port/docs/guide.md', '| One | Two |\n| --- | --- |\n| Guard sites | `a || b` |\n')
        self.allow(('port/docs/guide.md', 'row-cells', 'Guard sites', 'a record'))
        status, output = self.check()
        self.assertEqual(status, 0, output)
        self.assertIn('port/docs/guide.md:3: row-cells:', output)
        self.assertIn('(allowed: a record)', output)
        self.assertIn('1 problem (1 allowed)', output)

    def test_an_allowed_link_stays_allowed_wherever_it_moves(self):
        self.write('port/docs/guide.md', 'New text above.\n\nMore.\n\n[gone](gone.md)\n')
        self.allow(('port/docs/guide.md', 'missing-file', 'gone.md', 'a record names a file since removed'))
        status, output = self.check()
        self.assertEqual(status, 0, output)
        self.assertIn('port/docs/guide.md:5: missing-file:', output)

    def test_one_row_allows_each_place_of_its_problem(self):
        self.write('port/docs/guide.md', '[gone](gone.md)\n\nAgain, [gone](gone.md).\n')
        self.allow(('port/docs/guide.md', 'missing-file', 'gone.md', 'a record names a file since removed'))
        status, output = self.check()
        self.assertEqual(status, 0, output)
        self.assertIn('2 problems (2 allowed)', output)

    def test_a_row_that_names_no_problem_fails(self):
        self.write('port/docs/guide.md', CLEAN)
        self.allow(('port/docs/guide.md', 'missing-file', 'fixed.md', 'was a record\'s'))
        status, output = self.check()
        self.assertEqual(status, 1, output)
        self.assertIn('that the check no longer finds: remove the row', output)

    def test_one_document_leaves_the_other_rows_alone(self):
        self.write('port/docs/guide.md', CLEAN)
        self.allow(('port/docs/old.md', 'missing-file', 'gone.md', 'another document\'s'))
        status, output = self.check('port/docs/guide.md')
        self.assertEqual(status, 0, output)

    def test_rows_it_cant_take_fail(self):
        self.write('port/docs/guide.md', CLEAN)
        for row, error in ((('port/docs/guide.md', 'missing-file', 'nope.md', ''), 'no reason'),
                           (('port/docs/guide.md', 'no-such-kind', 'nope.md', 'why'), "no kind 'no-such-kind'"),
                           (('port/docs/guide.md', 'missing-file', 'nope.md'), '3 columns, not 4')):
            with self.subTest(error=error):
                self.allow(row)
                status, output = self.check()
                self.assertEqual(status, 1, output)
                self.assertIn(error, output)

    def test_an_unknown_document_fails(self):
        status, output = self.check('port/docs/nope.md')
        self.assertEqual(status, 1, output)
        self.assertIn('not one of the documents it checks', output)


class AnchorRuleTests(unittest.TestCase):
    # Headings of port/docs and the anchors GitHub's renderer gave them (the contents API's HTML, 2026-11-02)
    def test_githubs_anchors(self):
        for heading, anchor in (
                ("R10: string length respects the host object's bounds",
                 'r10-string-length-respects-the-host-objects-bounds'),
                ("The map sweep's café", 'the-map-sweeps-café'),
                ('4.5 HAL and display model', '45-hal-and-display-model'),
                ('Phase 5: the whole game, hardened', 'phase-5-the-whole-game-hardened'),
                ('Delegated approval of App-authored sync PRs', 'delegated-approval-of-app-authored-sync-prs')):
            with self.subTest(heading=heading):
                self.assertEqual(check_docs.slug(heading), anchor)

    def test_markup_and_punctuation(self):
        for heading, anchor in (('A section with `code`', 'a-section-with-code'),
                                ('Odd heading — with a dash', 'odd-heading--with-a-dash'),
                                ('`sync_upstream.sh` stops', 'sync_upstreamsh-stops'),
                                ('_Emphasis_ and foo_bar', 'emphasis-and-foo_bar'),
                                ('A [link](x.md) in it', 'a-link-in-it')):
            with self.subTest(heading=heading):
                self.assertEqual(check_docs.slug(heading), anchor)

    def test_repeats_and_html_headings(self):
        anchors = check_docs.document_anchors('# Same\n\n## Same\n\nSame\n---\n\n<h2 align="center">\n  Wide\n</h2>\n')
        self.assertEqual(anchors, {'same', 'same-1', 'same-2', '--wide'})


if __name__ == '__main__':
    unittest.main()
