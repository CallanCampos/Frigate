#!/usr/bin/env python3
"""PreToolUse guard for port sessions: nothing the port does may write to the decomp's repositories.

The port is a solo project (port/docs/adr/0002-solo-project-no-upstream-posts.md). A port session never writes to
fuddlesworth/pokebw2 (upstream) or to CallanCampos/pokebw2 (the user's decomp fork): no gh write, no gh api call that
changes something, no git push. The port's own writes never link an upstream issue or pull request, because GitHub
shows such a link on upstream's timeline, and never @-mention upstream's people (UPSTREAM_PEOPLE), because a mention
can notify them. Commit messages name no issue, pull request or discussion of any repository (OWNER/REPO#N or its URL),
and a push to GitHub that would add such a commit is refused, as port/tools/sync_upstream.sh refuses it (exit 5).
Reading upstream stays allowed.

The guard reads the shell command of a Bash, Monitor or terminal panel (mcp__terminal__run_in_terminal) call and
refuses it when the command would do one of those. It splits the command the way a shell would (quotes, operators,
here-documents, command substitutions, bash -c, find -exec), so a grep or a message that only mentions these names
passes. It sees the command's own text, the files it names and the commits in the repository: a script that runs gh
or git itself is not checked. It backs up port/CLAUDE.md's rules; it doesn't replace them.

Claude Code passes the tool call as JSON on stdin. Exit code 2 blocks the call and shows stderr to Claude; exit code 0
lets it through. port/tools/bootstrap_checkout.sh registers it in .claude/settings.local.json.

Usage: port_guard.py < hook-input.json
       port_guard.py --self-test [-v]     (-v also prints why each blocked case is blocked)
"""
import json
import os
import re
import subprocess
import sys
import tempfile

PORT = "callancampos/frigate"
FORBIDDEN = {
    "fuddlesworth/pokebw2": "upstream (fuddlesworth/pokebw2)",
    "callancampos/pokebw2": "the decomp fork (CallanCampos/pokebw2)",
}
# REST ids of the same repositories (gh api repos/OWNER/REPO --jq .id), for paths such as repositories/ID/issues
FORBIDDEN_IDS = {"1404739979": "fuddlesworth/pokebw2", "1418263998": "callancampos/pokebw2"}
FORBIDDEN_REMOTES = {"upstream", "fork"}

# A link GitHub records on the linked issue's timeline. Commit links (fuddlesworth/pokebw2@<sha>) are fine.
UPSTREAM_LINK = re.compile(
    r"(?i)\bfuddlesworth/pokebw2#\d+|github\.com/fuddlesworth/pokebw2/(?:issues|pull|discussions)/\d+"
)
# An issue, pull request or discussion of any repository, in the forms GitHub links. GIT_REFERENCES are
# sync_upstream.sh's REFERENCE_SHORT and REFERENCE_URL, for git rev-list -i -E; REFERENCE is the same in Python.
GIT_REFERENCES = (
    r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+#[0-9]+",
    r"github\.com/[^/[:space:]]+/[^/[:space:]]+/(issues|pull|discussions)/[0-9]+",
)
REFERENCE = re.compile(
    r"(?i)[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+#[0-9]+|github\.com/[^/\s]+/[^/\s]+/(?:issues|pull|discussions)/[0-9]+"
)
# Upstream's people: its human contributors as its contributors API listed them on 2026-10-17, without the user
# (CallanCampos) and the claude and codex accounts. An @-mention of one can notify them.
UPSTREAM_PEOPLE = (
    "fuddlesworth", "mariano54", "TeenageMutantNinjaTurtle", "Yvonne-Aizawa", "lucidLuckylee", "maseh87",
    "manta89640", "logdog2325", "Cbow2018", "Itshimcules", "fascix",
)
# Not after a word character or a dot, so e-mail addresses and fuddlesworth/pokebw2@<sha> pass
MENTION = re.compile(r"(?i)(?<![\w.])@(?:" + "|".join(UPSTREAM_PEOPLE) + r")(?![\w-])")
MUTATION = re.compile(r"\bmutation\b")
# OWNER/REPO, HOST/OWNER/REPO, or a GitHub URL of either protocol, with anything after the repository
REPO_SPEC = re.compile(
    r"""(?ix)^
    (?: (?:https?|ssh|git)://(?:[^@/\s]+@)?(?:www\.)?github\.com(?::\d+)?/
      | [\w.-]+@github\.com:
      | (?:www\.)?github\.com/ )?
    ([\w-]+)/([\w.-]+?)(?:\.git)?(?:/.*)?$"""
)
# Only for a command the guard failed to parse: refuse it if it looks like it could write to GitHub
SUSPECT = re.compile(r"(?i)fuddlesworth|callancampos/pokebw2|\bpush\b|\bgh\b|api\.github\.com")

# gh's repository-scoped command groups and their read-only subcommands; any other subcommand writes
GH_READS = {
    "issue": {"list", "ls", "view", "status"},
    "pr": {"list", "ls", "view", "status", "diff", "checks", "checkout", "co"},
    "label": {"list", "ls"},
    "release": {"list", "ls", "view", "download", "verify", "verify-asset"},
    "repo": {"view", "list", "ls", "clone", "set-default", "gitignore", "license", "read-dir", "read-file"},
    "discussion": {"list", "ls", "view"},
    "secret": {"list", "ls"},
    "variable": {"list", "ls", "get"},
    "workflow": {"list", "ls", "view"},
    "run": {"list", "ls", "view", "watch", "download"},
    "cache": {"list", "ls"},
    "ruleset": {"list", "ls", "view", "check"},
    "agent-task": {"list", "ls", "view"},
}
GH_NESTED_READS = {("repo", "deploy-key"): {"list", "ls"}, ("repo", "autolink"): {"list", "ls", "view"}}
# Groups that never write to a repository, whatever -R says
GH_READ_ONLY_GROUPS = {
    "browse", "search", "status", "co", "help", "auth", "config", "alias", "completion", "attestation"
}
GH_API_VALUE_FLAGS = {"-H", "--header", "--hostname", "-q", "--jq", "-t", "--template", "-p", "--preview", "--cache"}
GH_BODY_FILE_FLAGS = {"-F", "--body-file", "--notes-file"}
# Flags of gh writes whose value is text, never the repository written to: a body that is an upstream URL isn't a
# target. Short flags that are booleans in some subcommand (-m and -d of gh pr merge, -a of gh pr review) aren't here.
GH_TEXT_FLAGS = {
    "-b", "--body", "-t", "--title", "--subject", "-n", "--notes", "-c", "--comment", "--description", "--homepage",
    "--label", "--milestone", "--template", "--reason",
}

SHELLS = {"bash", "sh", "zsh", "dash", "ksh"}
KEYWORDS = {"if", "then", "else", "elif", "fi", "do", "done", "while", "until", "{", "}", "!", "time", "coproc"}
ASSIGN = re.compile(r"^[A-Za-z_]\w*=")
# Commands that run the command after their own options, and those options that take a value
WRAPPERS = {
    "env": {"-u", "--unset", "-C", "--chdir", "-P", "-S", "--split-string"},
    "command": set(),
    "builtin": set(),
    "exec": {"-a"},
    "nohup": set(),
    "time": set(),
    "noglob": set(),
    "nice": {"-n"},
    "timeout": {"-s", "--signal", "-k", "--kill-after"},
    "sudo": {"-u", "-g", "-h", "-p", "-C", "-D", "-U", "-r", "-t", "-T"},
    "xargs": {"-I", "-J", "-L", "-n", "-P", "-s", "-d", "-E", "-a", "-R", "-S"},
    "caffeinate": {"-t", "-w"},
    "stdbuf": {"-i", "-o", "-e"},
}
FIND_EXEC = {"-exec", "-execdir", "-ok", "-okdir"}
OPERATORS = ("&&", "||", ";;", "|&", ";", "&", "|", "(", ")")
REDIRECT = re.compile(r"&>>?|<<<|<<-?|<>|>>|>\||[<>]&(?:-|\d+)?|[<>]")
WRITE_REDIRECTS = {">", ">>", ">|", ">&", "&>", "&>>", "<>"}
HEREDOC = re.compile(r"(?<!<)<<(?!<)(-?)[ \t]*(?:'([^'\n]*)'|\"([^\"\n]*)\"|\\?([\w.-]+))")
# Options of git commands that take a value, for picking out their revisions (-x and -s are flags of cherry-pick)
PICK_VALUE_OPTIONS = {"-m", "--mainline", "-X", "--strategy-option", "--strategy", "--cleanup"}
GIT_VALUE_OPTIONS = {
    "merge": {"-m", "--message", "-F", "--file", "-s", "--strategy", "-X", "--strategy-option", "--cleanup",
              "--into-name"},
    "cherry-pick": PICK_VALUE_OPTIONS,
    "revert": PICK_VALUE_OPTIONS,
    "rebase": {"-s", "--strategy", "-X", "--strategy-option", "-x", "--exec", "--onto"},
    "reset": set(),
    "checkout": {"-b", "-B", "--orphan", "--conflict"},
    "switch": {"-c", "-C", "--create", "--force-create", "--orphan", "--conflict"},
    "pull": {"-s", "--strategy", "-X", "--strategy-option", "--depth", "--deepen", "--shallow-since",
             "--shallow-exclude", "--upload-pack", "-j", "--jobs", "-o", "--server-option"},
}


class Word(str):
    """A shell word with quotes removed, and the here-document bodies read inside it, as in -m "$(cat <<'EOF' ...)"."""

    bodies = ()


def make_word(text, bodies=()):
    word = Word(text)
    if bodies:
        word.bodies = tuple(bodies)
    return word


def parse_repo(text):
    """'owner/repo' in lower case when text is a repository spec or a GitHub URL, else None."""
    match = REPO_SPEC.match(text.strip())
    return f"{match[1]}/{match[2]}".lower() if match else None


def computed(text):
    """Whether a word's value is only known when the shell runs it."""
    return "$" in text or "`" in text


def graphql_computed(text):
    """Whether a GraphQL document still holds shell expansions: $(...), a backtick, ${...}, or a $NAME that isn't one
    of its own variables, which the operation declares as $NAME:."""
    if "$(" in text or "`" in text or "${" in text:
        return True
    declared = set(re.findall(r"\$(\w+)\s*:", text))
    return any(name not in declared for name in re.findall(r"\$(\w+)", text))


# --- Shell syntax ---------------------------------------------------------------------------------------------------


def match_paren(text, i):
    """Index of the ')' closing the '(' at i, skipping quoted text, or len(text) if it never closes."""
    depth = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == "\\":
            i += 2
            continue
        if c == "'":
            j = text.find("'", i + 1)
            i = n if j < 0 else j + 1
            continue
        if c == '"':
            i = read_double(text, i + 1, [])[0]
            continue
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return n


def find_backtick(text, i):
    """Index of the backtick closing the one before i, or len(text)."""
    while i < len(text):
        if text[i] == "\\":
            i += 2
        elif text[i] == "`":
            return i
        else:
            i += 1
    return len(text)


def read_double(text, i, subs):
    """Reads a double-quoted string from just after its quote: (index after the closing quote, its contents)."""
    out = []
    n = len(text)
    while i < n:
        c = text[i]
        if c == "\\" and i + 1 < n:
            out.append(text[i + 1] if text[i + 1] in '"\\$`' else text[i : i + 2] if text[i + 1] != "\n" else "")
            i += 2
        elif c == '"':
            return i + 1, "".join(out)
        elif text.startswith("$(", i):
            j = match_paren(text, i + 1)
            subs.append(text[i + 2 : j])
            out.append(text[i : j + 1])
            i = j + 1
        elif c == "`":
            j = find_backtick(text, i + 1)
            subs.append(text[i + 1 : j])
            out.append(text[i : j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return n, "".join(out)


def split_heredocs(text):
    """Removes here-document bodies, which are data. Returns the rest of the text, the bodies a shell reads as
    commands, and every body with whether the shell expands it, keyed by the position of its << in the rest."""
    lines = text.split("\n")
    kept, shell_bodies, heredocs = [], [], {}
    offset = 0
    i = 0
    while i < len(lines):
        line = lines[i]
        kept.append(line)
        i += 1
        for match in HEREDOC.finditer(line):
            delimiter = match[2] if match[2] is not None else match[3] if match[3] is not None else match[4]
            end = i
            while end < len(lines) and (lines[end].lstrip("\t") if match[1] else lines[end]) != delimiter:
                end += 1
            if end == len(lines):
                continue  # no closing line: it wasn't a here-document
            body = "\n".join(lines[i:end])
            expands = match[4] is not None and "\\" not in match[0]
            heredocs[offset + match.start()] = (body, expands)
            words = Guard.unwrap(re.split(r"[;&|(]", line[: match.start()])[-1].split(), {})
            after = line[match.end() :]
            if (words and os.path.basename(words[0]) in SHELLS) or re.search(r"\|\s*(ba|z|da|k)?sh\b", after):
                shell_bodies.append(body)
            i = end + 1
        offset += len(line) + 1
    return "\n".join(kept), shell_bodies, heredocs


def tokenize(text, subs, heredocs=None):
    """Splits a command line into ('word', Word), ('redir', (op, target, body, expands)) and ('op', text) tokens,
    with quotes removed. A redirection's target is its file, here-document delimiter or here-string, and a
    here-document's body comes from heredocs (split_heredocs). The text of each $(...), `...`, <(...) and >(...) is
    appended to subs, to be checked on its own."""
    heredocs = heredocs or {}
    tokens = []
    word = start = redirect = None
    quoted = False
    i, n = 0, len(text)

    def end_word():
        nonlocal word, start, quoted, redirect
        if word is not None:
            if redirect is not None:
                op, at = redirect
                body, expands = heredocs.get(at, (None, False)) if op in ("<<", "<<-") else (None, False)
                tokens.append(("redir", (op, word, body, expands)))
                redirect = None
            else:
                bodies = [body for at, (body, _) in heredocs.items() if start <= at < i]
                tokens.append(("word", make_word(word, bodies)))
        word = start = None
        quoted = False

    while i < n:
        c = text[i]
        at = i
        if c in " \t":
            end_word()
            i += 1
        elif c == "\n":
            end_word()
            tokens.append(("op", ";"))
            i += 1
        elif c == "#" and word is None:
            j = text.find("\n", i)
            i = n if j < 0 else j
        elif c == "\\":
            if text.startswith("\\\n", i):
                i += 2
                continue
            word = (word or "") + text[i + 1 : i + 2]
            i += 2
        elif c == "'":
            j = text.find("'", i + 1)
            j = n if j < 0 else j
            word = (word or "") + text[i + 1 : j]
            quoted = True
            i = j + 1
        elif c == '"':
            i, part = read_double(text, i + 1, subs)
            word = (word or "") + part
            quoted = True
        elif c == "`":
            j = find_backtick(text, i + 1)
            subs.append(text[i + 1 : j])
            word = (word or "") + text[i : j + 1]
            i = j + 1
        elif text.startswith("$(", i) or (c in "<>" and text.startswith("(", i + 1)):
            j = match_paren(text, i + 1)
            subs.append(text[i + 2 : j])
            word = (word or "") + text[i : j + 1]
            i = j + 1
        elif text.startswith("${", i):
            j = text.find("}", i)
            j = n - 1 if j < 0 else j
            word = (word or "") + text[i : j + 1]
            i = j + 1
        elif c in "<>" or text.startswith("&>", i):
            if word is not None and not quoted and word.isdigit():
                word = start = None  # the file descriptor of 2>&1 and the like
            else:
                end_word()
            op = REDIRECT.match(text, i).group()
            i += len(op)
            if not re.fullmatch(r"[<>]&(?:-|\d+)", op):
                redirect = (op, at)  # the next word is its file, delimiter or here-string
        elif c in ";&|()":
            end_word()
            op = next(o for o in OPERATORS if text.startswith(o, i))
            tokens.append(("op", op))
            i += len(op)
        else:
            word = (word or "") + c
            i += 1
        if word is not None and start is None:
            start = at
    end_word()
    return tokens


def simple_commands(tokens):
    """Yields ('cmd', words, redirections) for each simple command in order, and ('(', None, None) /
    (')', None, None) around subshells."""
    words, redirs = [], []
    for kind, value in tokens:
        if kind == "word":
            words.append(value)
            continue
        if kind == "redir":
            redirs.append(value)
            continue
        if words or redirs:
            yield "cmd", words, redirs
            words, redirs = [], []
        if value in ("(", ")"):
            yield value, None, None
    if words or redirs:
        yield "cmd", words, redirs


# --- The checks -----------------------------------------------------------------------------------------------------


def run_git(cwd, args):
    """git's stripped stdout in cwd, or None when it fails or prints nothing."""
    try:
        result = subprocess.run(["git", "-C", cwd, *args], capture_output=True, text=True, timeout=5)
    except (OSError, subprocess.SubprocessError):
        return None
    out = result.stdout.strip()
    return out if result.returncode == 0 and out else None


class Guard:
    def __init__(self, cwd, env, git=run_git):
        self.cwd = cwd
        self.env = {"GH_REPO": env["GH_REPO"]} if env.get("GH_REPO") else {}
        self.vars = dict(self.env)  # shell variables the command sets, for R=...; gh ... -R $R
        self.git = git
        self.added_remotes = {}  # push URLs that git remote add, git remote set-url or git config set earlier
        self.rewrites = []  # url.<base>.pushInsteadOf settings toward a forbidden repository set earlier
        self.pending = []  # revisions that git merge, rebase, reset, checkout or pull bring in before a push
        self.written = set()  # files that redirections, tee, cp and mv write earlier; None when one can't be told
        self.redirs, self.material = [], ""  # the current simple command's redirections and its own text
        self.problems = []
        self.text = ""

    def run(self, command):
        """The reasons to refuse command, or an empty list."""
        self.text = command
        self.check(command, 0)
        return self.problems

    def block(self, reason):
        if reason not in self.problems:
            self.problems.append(reason)

    def check(self, text, depth):
        if depth > 8:
            if SUSPECT.search(text):
                self.block("the command nests too deeply to check; split it into plain commands")
            return
        text, shell_bodies, heredocs = split_heredocs(text)
        subs = []
        stack = []
        for kind, words, redirs in simple_commands(tokenize(text, subs, heredocs)):
            if kind == "(":
                stack.append(self.cwd)
            elif kind == ")":
                if stack:
                    self.cwd = stack.pop()
            else:
                self.command(words, depth, redirs)
        while stack:
            self.cwd = stack.pop()
        for nested in shell_bodies + subs:
            self.nested(nested, depth)

    def nested(self, text, depth):
        """Checks a command that a child shell runs, so its cd doesn't carry over."""
        saved = self.cwd, dict(self.env), dict(self.vars)
        self.check(text, depth + 1)
        self.cwd, self.env, self.vars = saved

    def expand(self, word):
        """word with $NAME and ${NAME} replaced where the command itself set NAME."""
        if "$" not in word:
            return word
        expanded = re.sub(r"\$\{(\w+)\}|\$(\w+)", lambda m: self.vars.get(m[1] or m[2], m[0]), word)
        return make_word(expanded, getattr(word, "bodies", ()))

    def command(self, words, depth, redirs=()):
        words = [self.expand(w) for w in words]
        # What the command itself says: its words, the here-documents read inside them and its own stdin
        self.redirs = list(redirs)
        parts = []
        for word in words:
            parts += [word, *getattr(word, "bodies", ())]
        for op, target, body, _ in self.redirs:
            parts += [body] if body is not None else [target] if op == "<<<" else []
        self.material = "\n".join(parts)
        for op, target, _, _ in self.redirs:
            if op in WRITE_REDIRECTS:
                self.written.add(self.join(self.cwd, target))
        assigned = {}
        words = self.unwrap(words, assigned)
        env = {**self.env, **assigned}
        if not words:
            # Only assignments: shell variables for the commands after them, which reach gh's environment only
            # when they are exported already
            self.vars.update(assigned)
            for key in assigned.keys() & self.env.keys():
                self.env[key] = assigned[key]
            return
        name = os.path.basename(words[0])
        args = words[1:]
        if name in SHELLS:
            # bash -c COMMAND, after options such as --norc, -o pipefail or -lc
            i = 0
            while i < len(args) and args[i] != "--" and args[i].startswith(("-", "+")):
                if args[i] in ("-o", "+o", "-O", "+O", "--rcfile", "--init-file"):
                    i += 2
                    continue
                if not args[i].startswith("--") and "c" in args[i][1:]:
                    if i + 1 < len(args):
                        self.nested(args[i + 1], depth)
                    break
                i += 1
        elif name == "eval":
            self.nested(" ".join(args), depth)
        elif name in ("cd", "pushd"):
            target = next((a for a in args if not a.startswith("-") or a == "-"), os.path.expanduser("~"))
            self.cwd = None if target == "-" else self.join(self.cwd, target)
        elif name == "popd":
            self.cwd = None
        elif name == "export":
            for arg in args:
                if ASSIGN.match(arg):
                    key, _, value = arg.partition("=")
                    self.env[key] = self.vars[key] = value
                elif arg in self.vars:
                    self.env[arg] = self.vars[arg]
        elif name == "unset":
            for arg in args:
                self.env.pop(arg, None)
                self.vars.pop(arg, None)
        elif name == "find":
            # find ... -exec COMMAND {} ; runs COMMAND
            k = 0
            while k < len(args):
                if args[k] in FIND_EXEC:
                    end = next((j for j in range(k + 1, len(args)) if args[j] in (";", "+")), len(args))
                    if end > k + 1:
                        self.command(args[k + 1 : end], depth, redirs)
                    k = end
                k += 1
        elif name == "tee":
            self.written.update(self.join(self.cwd, a) for a in args if not a.startswith("-"))
        elif name in ("cp", "mv", "install", "ln", "rsync"):
            positional = [a for a in args if not a.startswith("-")]
            if len(positional) >= 2:
                self.written.add(self.join(self.cwd, positional[-1]))
        elif name == "git":
            self.git_command(args, env)
        elif name == "gh":
            self.gh(args, env)
        elif name == "curl":
            self.curl(args)

    @staticmethod
    def unwrap(words, env):
        """Strips keywords, VAR=value prefixes (into env) and wrappers such as env, sudo and xargs."""
        i = 0
        while i < len(words):
            word = words[i]
            if word in KEYWORDS:
                i += 1
            elif ASSIGN.match(word):
                key, _, value = word.partition("=")
                env[key] = value
                i += 1
            elif os.path.basename(word) in WRAPPERS:
                wrapper = os.path.basename(word)
                i += 1
                while i < len(words):
                    arg = words[i]
                    if arg == "--":
                        i += 1
                        break
                    if wrapper == "env" and ASSIGN.match(arg):
                        key, _, value = arg.partition("=")
                        env[key] = value
                        i += 1
                    elif arg.startswith("-") and arg != "-":
                        i += 2 if arg in WRAPPERS[wrapper] else 1
                    else:
                        break
                if wrapper == "timeout":
                    i += 1  # the duration
            else:
                break
        return words[i:]

    @staticmethod
    def join(base, path):
        """The directory path names from base, or None when it can't be known before the shell runs."""
        if computed(path) or any(c in path for c in "*?["):
            return None
        path = os.path.expanduser(path)
        if not os.path.isabs(path):
            if base is None:
                return None
            path = os.path.join(base, path)
        return os.path.normpath(path)

    def read(self, path):
        """A file's text, up to 1 MB, or None."""
        if path in ("", "-") or computed(path):
            return None
        full = self.join(self.cwd, path) if not os.path.isabs(os.path.expanduser(path)) else os.path.expanduser(path)
        try:
            with open(full, encoding="utf-8", errors="replace") as f:
                return f.read(1 << 20)
        except (OSError, TypeError):
            return None

    def written_here(self, path):
        """Whether an earlier part of the command may write path, so that what the file holds now isn't what the
        command reads."""
        return None in self.written or self.join(self.cwd, path) in self.written

    def stdin_source(self):
        """Where the current command's standard input comes from: ('heredoc', body, expands), ('herestring', text),
        ('file', path), or ('pipe',) when the guard can't see it: a pipe, or whatever Claude Code gives it."""
        for op, target, body, expands in reversed(self.redirs):
            if op in ("<<", "<<-"):
                return ("heredoc", body, expands) if body is not None else ("pipe",)
            if op == "<<<":
                return ("herestring", self.expand(target))
            if op in ("<", "<>"):
                return ("file", target)
        return ("pipe",)

    def check_message(self, what, files=(), extra=(), any_repository=False):
        """Refuses a write whose text links an upstream issue or pull request (any repository's issue, pull request
        or discussion for a commit message) or @-mentions upstream's people. The text is the command's own words and
        here-documents, extra, and the files it reads, "-" for stdin. When a file can't be read, is written earlier
        in the command or comes through a pipe, the whole command is searched instead."""
        texts = [self.material, *extra]
        whole = False
        for path in files:
            if path == "-":
                source = self.stdin_source()
                if source[0] != "file":
                    whole = whole or source[0] == "pipe"  # a here-document is in the material already
                    continue
                path = source[1]
            content = self.read(path)
            texts.append(content or "")
            whole = whole or content is None or self.written_here(path)
        if whole:
            texts.append(self.text)
        pattern = REFERENCE if any_repository else UPSTREAM_LINK
        for text in texts:
            match = pattern.search(text or "")
            if match and any_repository:
                self.block(
                    f"{what} names {match[0]}: port commit messages name only the port's own issues, as a bare #N, "
                    "and other items by title, and a push of a commit that names one is refused, because GitHub "
                    "records a reference to the port on that item's timeline"
                )
                break
            if match:
                self.block(
                    f"{what} links {match[0]}, and GitHub would show that on upstream's timeline: write it as plain "
                    "text without the link (upstream pull request 345), or link a commit (fuddlesworth/pokebw2@<sha>)"
                )
                break
        for text in texts:
            match = MENTION.search(text or "")
            if match:
                self.block(
                    f"{what} @-mentions {match[0][1:]}, who contributes to upstream, and a mention can notify them: "
                    "write the name without the @"
                )
                break

    # --- gh ---

    def default_repo(self, env):
        """The repository gh uses when a command names none: GH_REPO, else the clone's gh repo set-default.
        None when it can't be told. Without a default gh takes the first remote, and `upstream` sorts first."""
        if env.get("GH_REPO"):
            return parse_repo(env["GH_REPO"]) or env["GH_REPO"].lower()
        if self.cwd is None:
            return None
        out = self.git(self.cwd, ["config", "--get-regexp", r"^remote\..*\.gh-resolved$"])
        repos = set()
        for line in (out or "").splitlines():
            key, _, value = line.partition(" ")
            if value == "base":
                url = self.git(self.cwd, ["remote", "get-url", key[len("remote.") : -len(".gh-resolved")]])
                repos.add(parse_repo(url) if url else None)
            else:
                repos.add(parse_repo(value))
        return repos.pop() if len(repos) == 1 else None

    def gh(self, args, env):
        repo_flags, rest = [], []
        i = 0
        while i < len(args):
            arg = args[i]
            if arg in ("-R", "--repo"):
                repo_flags.append(args[i + 1] if i + 1 < len(args) else "")
                i += 2
                continue
            if arg.startswith("--repo="):
                repo_flags.append(arg[len("--repo=") :])
            elif arg.startswith("-R") and len(arg) > 2:
                repo_flags.append(arg[2:])
            else:
                rest.append(arg)
            i += 1
        if "--help" in rest or "-h" in rest:
            return
        names = [a for a in rest if not a.startswith("-")]
        group = names[0] if names else ""
        sub = names[1] if len(names) > 1 else ""
        if group == "api":
            self.gh_api(rest[rest.index("api") + 1 :], env)
            return
        command = f"gh {group} {sub}".strip()
        if group not in GH_READS:
            # An extension or a group that isn't repository-scoped: only a write -R names can be told
            for value in repo_flags:
                if group not in GH_READ_ONLY_GROUPS and parse_repo(value) in FORBIDDEN:
                    self.block(f"`{command}` names {FORBIDDEN[parse_repo(value)]}")
            return
        if not sub or sub in GH_READS[group]:
            return
        nested = GH_NESTED_READS.get((group, sub))
        if nested is not None and (len(names) < 3 or names[2] in nested):
            return
        if (group, sub) == ("issue", "develop") and ("--list" in rest or "-l" in rest):
            return

        # A write. Its repositories: -R, and any argument that is one of these repositories or a URL into one, other
        # than the text of a body, title or comment
        targets = {parse_repo(v) or ("?" + v if computed(v) else v.lower()) for v in repo_flags}
        skip = set()
        after_sub = rest.index(sub) + 1
        if (group, sub) == ("label", "clone"):
            # gh label clone SOURCE reads SOURCE and writes to -R
            skip.add(next((k for k in range(after_sub, len(rest)) if not rest[k].startswith("-")), -1))
        for k, arg in enumerate(rest):
            if k in skip or k < after_sub or rest[k - 1] in GH_TEXT_FLAGS:
                continue
            if (group, sub) == ("repo", "sync") and k > 0 and rest[k - 1] in ("-s", "--source"):
                continue  # gh repo sync --source reads the source
            if arg.startswith("--source="):
                continue
            value = arg.split("=", 1)[1] if arg.startswith("--") and "=" in arg else arg
            if value.startswith("-") or arg.split("=", 1)[0] in GH_TEXT_FLAGS:
                continue
            repo = parse_repo(value)
            if repo in FORBIDDEN or repo == PORT:
                targets.add(repo)
        if not targets and (group, sub) not in (("repo", "create"), ("repo", "new"), ("repo", "sync")):
            targets.add(self.default_repo(env))
        for target in targets:
            if target is None:
                self.block(
                    f"`{command}` names no repository, and this checkout's gh default isn't the port; "
                    "add -R CallanCampos/Frigate"
                )
            elif target.startswith("?"):
                self.block(f"`{command}` names its repository through a variable ({target[1:]}); write it out")
            elif target in FORBIDDEN:
                self.block(f"`{command}` writes to {FORBIDDEN[target]}")
        files = [rest[k + 1] for k in range(len(rest) - 1) if rest[k] in GH_BODY_FILE_FLAGS]
        files += [a.split("=", 1)[1] for a in rest if a.startswith(("--body-file=", "--notes-file="))]
        self.check_message(f"`{command}`", files)

    def gh_api(self, args, env):
        endpoint = method = input_file = None
        fields = []
        i = 0
        while i < len(args):
            arg = args[i]
            value = args[i + 1] if i + 1 < len(args) else ""
            if arg in ("-X", "--method"):
                method, i = value, i + 2
                continue
            if arg in ("-f", "--raw-field", "-F", "--field"):
                fields.append((arg, value))
                i += 2
                continue
            if arg == "--input":
                input_file, i = value, i + 2
                continue
            if arg in GH_API_VALUE_FLAGS:
                i += 2
                continue
            if arg.startswith("--method="):
                method = arg.split("=", 1)[1]
            elif arg.startswith("-X") and len(arg) > 2:
                method = arg[2:]
            elif arg.startswith(("--raw-field=", "--field=")):
                fields.append((arg.split("=", 1)[0], arg.split("=", 1)[1]))
            elif arg.startswith(("-f", "-F")) and len(arg) > 2:
                fields.append((arg[:2], arg[2:]))
            elif arg.startswith("--input="):
                input_file = arg.split("=", 1)[1]
            elif not arg.startswith("-") and endpoint is None:
                endpoint = arg
            i += 1
        if endpoint is None:
            return
        method = (method or ("POST" if fields or input_file is not None else "GET")).upper()
        # -F key=@file reads the value from a file, and @- from stdin
        files = [v.split("=", 1)[1][1:] for f, v in fields if f in ("-F", "--field") and "=@" in v]
        if input_file is not None:
            files.append(input_file)
        path = re.sub(r"(?i)^https?://(api|uploads)\.github\.com", "", endpoint).lstrip("/")
        if path.lower().split("?")[0] == "graphql":
            self.gh_graphql(fields, input_file)
            return
        if method in ("GET", "HEAD"):
            return
        lowered = path.lower()
        if computed(path):
            self.block(f"`gh api -X {method} {endpoint}` builds its path from a variable; write it out")
        for repo, label in FORBIDDEN.items():
            if re.search(rf"(?:^|/){re.escape(repo)}(?=$|[/?#])", lowered):
                self.block(f"`gh api -X {method} {endpoint}` writes to {label}")
        for match in re.finditer(r"(?:^|/)repositories/(\d+)(?=$|[/?#])", lowered):
            if match[1] in FORBIDDEN_IDS:
                self.block(f"`gh api -X {method} {endpoint}` writes to {FORBIDDEN[FORBIDDEN_IDS[match[1]]]}")
        if re.search(r"\{owner\}|\{repo\}|(?:^|/):(?:owner|repo)(?=$|/)", lowered):
            target = self.default_repo(env)
            if target is None:
                self.block(
                    f"`gh api -X {method} {endpoint}` fills {{owner}}/{{repo}} from a default that isn't the port "
                    "here; write repos/CallanCampos/Frigate"
                )
            elif target in FORBIDDEN:
                self.block(f"`gh api -X {method} {endpoint}` writes to {FORBIDDEN[target]}")
        self.check_message(f"`gh api -X {method}`", files)

    def document(self, source, texts, unseen):
        """Adds what a GraphQL document read from source holds to texts, or why the guard can't see it to unseen.
        source is a file, "-" for stdin, or ("inline", text)."""
        if source == "-":
            source = self.stdin_source()
            if source[0] == "heredoc":
                _, body, expands = source
                if expands and graphql_computed(body):
                    unseen.append("a here-document that the shell expands")
                else:
                    texts.append(body)
                return
            if source[0] == "herestring":
                source = ("inline", source[1])
            elif source[0] == "file":
                source = source[1]
            else:
                unseen.append("a pipe or stdin")
                return
        if isinstance(source, tuple):
            if graphql_computed(source[1]):
                unseen.append("a value the shell computes")
            else:
                texts.append(source[1])
            return
        content = self.read(source)
        if content is None:
            unseen.append(f"a file it can't read ({source})")
            return
        texts.append(content)
        if self.written_here(source):
            texts.append(self.text)  # the command rewrites the file first, likely from text it holds

    def gh_graphql(self, fields, input_file):
        """A GraphQL mutation names no repository the guard could check, so any is refused, and so is a document the
        guard can't see. Only the query field holds the operation; the other fields are its variables."""
        texts, unseen = [], []
        for flag, word in fields:
            key, _, value = word.partition("=")
            if key.strip() != "query":
                continue
            if flag in ("-F", "--field") and value.startswith("@"):
                self.document(value[1:], texts, unseen)
            else:
                self.document(("inline", value), texts, unseen)
        if input_file is not None:
            self.document(input_file, texts, unseen)
        advice = (
            "the guard can't tell which repository a mutation changes. Use a gh subcommand with -R "
            "CallanCampos/Frigate or a REST path under repos/CallanCampos/Frigate; for a read, pass the "
            "query inline (-f query='...'), in a quoted here-document or in a file"
        )
        if any(MUTATION.search(text) for text in texts):
            self.block(f"`gh api graphql` sends a mutation: {advice}")
        elif unseen:
            self.block(f"`gh api graphql` takes its query from {unseen[0]}, so it could be a mutation: {advice}")

    def curl(self, args):
        urls = [a for a in args if re.search(r"(?i)(api|uploads)\.github\.com", a)]
        if not urls:
            return
        method = None
        data = False
        bodies = []  # (value, whether @ names a file) of each -d, --data-*, --json
        opaque = False  # a form or an upload, whose content the guard doesn't read
        for k, arg in enumerate(args):
            value = args[k + 1] if k + 1 < len(args) else ""
            if arg in ("-X", "--request") and k + 1 < len(args):
                method = args[k + 1].upper()
            elif arg.startswith("-X") and len(arg) > 2:
                method = arg[2:].upper()
            elif arg in ("-d", "--data", "--data-ascii", "--data-binary", "--json", "--data-raw"):
                bodies.append((value, arg != "--data-raw"))
            elif arg == "--data-urlencode":
                bodies.append((value.split("=", 1)[-1] if "=" in value.split("@", 1)[0] else value, False))
                if re.match(r"^[^=@]*@", value):
                    bodies[-1] = ("@" + value.split("@", 1)[1], True)
            elif arg.startswith("-d") and len(arg) > 2:
                bodies.append((arg[2:], True))
            elif arg.startswith(("--data=", "--data-ascii=", "--data-binary=", "--json=", "--data-raw=")):
                bodies.append((arg.split("=", 1)[1], not arg.startswith("--data-raw=")))
            elif arg.startswith(("-F", "--form", "-T", "--upload-file")):
                opaque = True
            if arg.startswith(("-d", "--data", "--json", "-F", "--form", "-T", "--upload-file")):
                data = True
        if "-G" in args or "--get" in args:
            method = "GET"
        if (method or ("POST" if data else "GET")) in ("GET", "HEAD"):
            return
        for url in urls:
            lowered = url.lower()
            for repo, label in FORBIDDEN.items():
                if re.search(rf"/{re.escape(repo)}(?=$|[/?#])", lowered):
                    self.block(f"`curl` sends a {method or 'POST'} to {label}")
            for match in re.finditer(r"/repositories/(\d+)(?=$|[/?#])", lowered):
                if match[1] in FORBIDDEN_IDS:
                    self.block(f"`curl` sends a {method or 'POST'} to {FORBIDDEN[FORBIDDEN_IDS[match[1]]]}")
        if any(re.search(r"(?i)api\.github\.com/graphql(?=$|[/?#])", url) for url in urls):
            texts, unseen = [], ["a form or an upload"] if opaque else []
            for value, at_file in bodies:
                if at_file and value.startswith("@"):
                    self.document(value[1:], texts, unseen)
                else:
                    self.document(("inline", value), texts, unseen)
            if any(MUTATION.search(text) for text in texts):
                self.block("`curl` sends a GraphQL mutation, and the guard can't tell which repository it changes")
            elif unseen:
                self.block(
                    f"`curl` sends GitHub's GraphQL API a body from {unseen[0]}, so it could be a mutation; for a "
                    "read, use gh api graphql -f query='...'"
                )

    # --- git ---

    def git_command(self, args, env):
        cwd = self.cwd
        configs = []
        i = 0
        while i < len(args) and args[i].startswith("-"):
            arg = args[i]
            value = args[i + 1] if i + 1 < len(args) else ""
            if arg == "-C":
                cwd = self.join(cwd, value)
                i += 2
            elif arg == "-c":
                configs.append(value)
                i += 2
            elif arg in ("--git-dir", "--work-tree"):
                cwd = None
                i += 2
            elif arg in ("--namespace", "--config-env", "--super-prefix"):
                i += 2
            else:
                if arg.startswith(("--git-dir=", "--work-tree=")):
                    cwd = None
                i += 1
        if "GIT_DIR" in env or "GIT_WORK_TREE" in env:
            cwd = None
        if i >= len(args):
            return
        sub, rest = args[i], args[i + 1 :]
        if sub == "remote" and rest and rest[0] in ("add", "set-url"):
            positional = [
                a for k, a in enumerate(rest) if k > 0 and not a.startswith("-") and rest[k - 1] not in ("-t", "-m")
            ]
            if len(positional) >= 2:
                self.added_remotes.setdefault(positional[0], []).insert(0, positional[1])
        elif sub == "config":
            self.git_config(rest)
        elif sub == "push":
            self.git_push(rest, cwd, configs)
        elif sub in ("commit", "merge"):
            self.git_message(sub, rest, cwd)
        elif sub in ("cherry-pick", "revert") and not {"--continue", "--abort", "--skip", "--quit"} & set(rest):
            # The new commits take the picked commits' messages; a revert's names the reverted commit's subject
            revisions = self.revisions(sub, rest)
            messages = self.messages(cwd, ["--no-walk", *revisions], subject=sub == "revert") if revisions else None
            self.check_message(f"`git {sub}`", extra=[messages or ""], any_repository=True)
        if sub in ("merge", "rebase", "reset", "checkout", "switch"):
            self.pending += self.revisions(sub, rest)
            self.pending += [rest[k + 1] for k in range(len(rest) - 1) if rest[k] == "--onto"]
        elif sub == "pull":
            positional = self.revisions(sub, rest)
            if len(positional) >= 2:
                self.pending += [f"{positional[0]}/{b.split(':', 1)[0]}" for b in positional[1:]]

    @staticmethod
    def revisions(sub, rest):
        """The arguments of git SUB that can be revisions: its operands before --, without its options' values."""
        options = GIT_VALUE_OPTIONS.get(sub, set())
        out = []
        for k, arg in enumerate(rest):
            if arg == "--":
                break
            if not arg.startswith("-") and (k == 0 or rest[k - 1] not in options):
                out.append(arg)
        return out

    def messages(self, cwd, revisions, subject=False):
        """The messages of the commits that revisions name, as git log gives them, or None."""
        if cwd is None or any(computed(r) for r in revisions):
            return None
        return self.git(cwd, ["log", "--format=%s" if subject else "--format=%B", *revisions, "--"])

    def git_message(self, sub, rest, cwd):
        """Checks the message of git commit or git merge, wherever it comes from."""
        files = [rest[k + 1] for k in range(len(rest) - 1) if rest[k] in ("-F", "--file")]
        files += [a.split("=", 1)[1] for a in rest if a.startswith("--file=")]
        files += [a[2:] for a in rest if a.startswith("-F") and len(a) > 2]
        extra = []
        if sub == "commit":
            # -C, -c and the rest take another commit's message, and --fixup and --squash its subject
            options = ("--reuse-message", "--reedit-message", "--fixup", "--squash")
            reused = [rest[k + 1] for k in range(len(rest) - 1) if rest[k] in ("-C", "-c", *options)]
            reused += [a.split("=", 1)[1] for a in rest if a.startswith(tuple(o + "=" for o in options))]
            reused += [a[2:] for a in rest if a.startswith(("-C", "-c")) and len(a) > 2]
            reused = [r.split(":", 1)[1] if r.startswith(("amend:", "reword:")) else r for r in reused]
            if reused:
                extra.append(self.messages(cwd, ["--no-walk", *reused]) or "")
            given = re.compile(r"^(-m|--message|-F|--file|-C|-c|--reuse-message|--reedit-message|--fixup|--squash)")
            if cwd is not None and not any(given.match(a) for a in rest):
                # git commit --no-edit after a merge, cherry-pick -n or merge --squash takes the message git prepared
                for name in ("MERGE_MSG", "SQUASH_MSG"):
                    path = self.git(cwd, ["rev-parse", "--git-path", name])
                    if path:
                        try:
                            with open(os.path.join(cwd, path), encoding="utf-8", errors="replace") as f:
                                extra.append(f.read(1 << 20))
                        except OSError:
                            pass
        elif "--squash" in rest:
            revisions = self.revisions(sub, rest)
            if revisions:
                extra.append(self.messages(cwd, [*revisions, "--not", "HEAD"]) or "")
        self.check_message(f"`git {sub}`'s message", files, extra, any_repository=True)

    def git_config(self, rest):
        """Notes git config settings of push URLs and URL rewrites for the pushes after them."""
        if {"--get", "--get-all", "--get-regexp", "--unset", "--unset-all", "-l", "--list"} & set(rest):
            return
        positional = [
            a for k, a in enumerate(rest)
            if not a.startswith("-")
            and (k == 0 or rest[k - 1] not in ("-f", "--file", "--blob", "--type", "--default", "--comment", "--value"))
        ]
        if positional[:1] == ["set"]:
            positional = positional[1:]
        if len(positional) < 2:
            return
        key, value = positional[0], positional[1]
        remote = re.fullmatch(r"(?i)remote\.(.+)\.(?:push)?url", key)
        if remote:
            self.added_remotes.setdefault(remote[1], []).insert(0, value)
        rewrite = re.fullmatch(r"(?i)url\.(.+)\.(?:push)?insteadof", key)
        if rewrite and parse_repo(rewrite[1]) in FORBIDDEN:
            self.rewrites.append(rewrite[1])

    def remote_urls(self, name, cwd):
        """The URLs git pushes to for a remote name, with those the command set earlier, or None when it isn't a
        remote here."""
        out = self.git(cwd, ["remote", "get-url", "--push", "--all", name]) if cwd else None
        urls = [*self.added_remotes.get(name, []), *(out.splitlines() if out else [])]
        return urls or None

    def git_push(self, args, cwd, configs):
        remote = repo_option = None
        refspecs, flags = [], set()
        i = 0
        while i < len(args):
            arg = args[i]
            if arg == "--":
                tail = args[i + 1 :]
                if remote is None and tail:
                    remote, tail = tail[0], tail[1:]
                refspecs += tail
                break
            if arg == "--repo":
                repo_option = args[i + 1] if i + 1 < len(args) else ""
                i += 2
                continue
            if arg in ("-o", "--push-option", "--receive-pack", "--exec"):
                i += 2
                continue
            if arg.startswith("--repo="):
                repo_option = arg.split("=", 1)[1]
            elif arg.startswith("-"):
                flags.add(arg.split("=", 1)[0])
            elif remote is None:
                remote = arg
            else:
                refspecs.append(arg)
            i += 1
        remote = remote or repo_option
        for config in configs:
            key, _, value = config.partition("=")
            rewrite = re.fullmatch(r"(?i)url\.(.+)\.(?:push)?insteadof", key)
            repo = parse_repo(rewrite[1] if rewrite else value)
            if repo in FORBIDDEN:
                self.block(f"`git push` is given -c {config}, which points at {FORBIDDEN[repo]}")
        for base in self.rewrites:
            label = FORBIDDEN[parse_repo(base)]
            self.block(f"`git push` after url.{base}.pushInsteadOf, which sends pushes to {label}")
        if remote is None:
            if cwd is None:
                self.block("`git push` without a remote, in a directory the guard can't tell; name the remote")
                return
            branch = self.git(cwd, ["symbolic-ref", "--quiet", "--short", "HEAD"])
            keys = [f"branch.{branch}.pushRemote", "remote.pushDefault", f"branch.{branch}.remote"]
            remote = next(
                (r for r in (self.git(cwd, ["config", "--get", k]) for k in keys if branch or "branch" not in k) if r),
                "origin",
            )
        if computed(remote):
            self.block(f"`git push {remote}` names its remote through a variable; write it out")
            return
        if remote.lower() in FORBIDDEN_REMOTES:
            self.block(f"`git push` to the remote `{remote}`: the port never pushes to the decomp's repositories")
            return
        repo = parse_repo(remote)
        if repo in FORBIDDEN:
            self.block(f"`git push` to {FORBIDDEN[repo]}")
            return
        urls = self.remote_urls(remote, cwd)
        if urls is None:
            if "/" not in remote and ":" not in remote:
                if cwd is None:
                    self.block(
                        f"`git push {remote}` in a directory the guard can't tell; run it from the port checkout"
                    )
                return  # no remote of that name: the push fails
            urls = [remote]  # a URL or a path
        for url in urls:
            repo = parse_repo(url)
            if repo in FORBIDDEN:
                self.block(f"`git push {remote}`: that remote pushes to {FORBIDDEN[repo]}")
                return
        self.check_pushed_commits(remote, urls, refspecs, flags, cwd)

    def resolve(self, cwd, revision):
        """The commit a revision names in cwd, or None."""
        if computed(revision):
            return None
        return self.git(cwd, ["rev-parse", "--verify", "--quiet", "--end-of-options", f"{revision}^{{commit}}"])

    def check_pushed_commits(self, remote, urls, refspecs, flags, cwd):
        """Refuses a push to GitHub that would add commits whose messages name an issue, pull request or discussion,
        as sync_upstream.sh refuses them (exit 5): GitHub records a reference to the repository pushed to on that
        item's timeline. The commits are those the pushed refs reach and the remote's tracking refs don't, with
        what git merge, rebase, reset, checkout or pull bring in earlier in the same command."""
        if flags & {"--dry-run", "-n"} or not any("github.com" in url.lower() for url in urls):
            return
        if cwd is None:
            self.block(
                f"`git push {remote}` runs in a directory the guard can't tell, so it can't check the commits it "
                "adds; run it from the port checkout"
            )
            return
        sources = []
        if flags & {"--all", "--branches", "--mirror"}:
            sources.append("--branches")
        if flags & {"--tags", "--mirror"}:
            sources.append("--tags")
        if not flags & {"--delete", "-d"}:
            for spec in refspecs:
                source = spec.lstrip("+").split(":", 1)[0]
                if not source:
                    continue  # :branch deletes
                if computed(source):
                    sources.append("--branches")
                elif "*" in source:
                    sources.append(f"--glob={source}")
                else:
                    sources.append(self.resolve(cwd, source) or "HEAD")  # a branch the command creates first
        if not refspecs and not sources:
            sources.append("HEAD")
        sources += [commit for commit in (self.resolve(cwd, r) for r in self.pending) if commit]
        if not sources:
            return
        excludes = [] if "/" in remote or ":" in remote else [f"--remotes={remote}"]
        targets = {parse_repo(url) for url in urls} - {None}
        if remote != "origin" and targets & {parse_repo(url) for url in self.remote_urls("origin", cwd) or []}:
            excludes.append("--remotes=origin")
        query = ["-i", "-E", *(f"--grep={pattern}" for pattern in GIT_REFERENCES), *dict.fromkeys(sources), "--not",
                 *excludes, "--"]
        count = self.git(cwd, ["rev-list", "--count", *query])
        if count is None:
            self.block(f"`git push {remote}`: the guard couldn't list the commits it adds; push a branch by name")
        elif count != "0":
            shas = (self.git(cwd, ["rev-list", "--abbrev-commit", "--abbrev=9", "--max-count=5", *query]) or "")
            self.block(
                f"`git push {remote}` would add commits whose messages name an issue, pull request or discussion "
                f"(OWNER/REPO#N or its URL), {count} in all: {' '.join(shas.split())}. On GitHub each records a "
                "reference to the port on that item's timeline, so the push is refused, as sync_upstream.sh refuses "
                "it (exit 5). Reword a port commit's message before it is pushed (a bare #N for the port's issues, a "
                "title for others); upstream's commits wait for the user's decision. If they are on the remote "
                "already, fetch it first"
            )


MESSAGE = """port_guard refused this command. The port is a solo project: a port session never writes to
fuddlesworth/pokebw2 (upstream) or CallanCampos/pokebw2 (the decomp fork), and the port's writes never link
upstream issues or pull requests or @-mention upstream's people (port/docs/adr/0002-solo-project-no-upstream-posts.md).
{reasons}
Port writes name the port: gh ... -R CallanCampos/Frigate, git push origin. Decomp work, claims included,
belongs to a session the user starts in the decomp checkout. Stop and tell the user; don't route around this guard."""


def main():
    if sys.argv[1:2] == ["--self-test"]:
        sys.exit(self_test())
    try:
        call = json.load(sys.stdin)
        tool_input = call.get("tool_input") or {}
        command = tool_input.get("command")
        cwd = call.get("cwd") or os.getcwd()
        if tool_input.get("tab_id"):
            cwd = None  # a terminal tab that earlier commands left in a directory the guard can't know
        elif isinstance(tool_input.get("cwd"), str) and tool_input["cwd"]:
            cwd = Guard.join(cwd, tool_input["cwd"])  # a new terminal tab's directory
    except (ValueError, AttributeError):
        return
    if not isinstance(command, str) or not command.strip():
        return
    try:
        problems = Guard(cwd, os.environ).run(command)
    except Exception as error:  # a parser bug must not let a write through
        problems = []
        if SUSPECT.search(command):
            problems = [f"the guard failed on this command ({type(error).__name__}: {error}); split it up"]
    if problems:
        print(MESSAGE.format(reasons="\n".join(f"- {p}" for p in problems)), file=sys.stderr)
        sys.exit(2)


# --- Self-test ------------------------------------------------------------------------------------------------------

PORT_URL = "https://github.com/CallanCampos/Frigate.git"
UPSTREAM_URL = "https://github.com/fuddlesworth/pokebw2.git"
FORK_URL = "git@github.com:CallanCampos/pokebw2.git"
LINKED = "DWC: list the access points\n\nThe rest of fuddlesworth/pokebw2#315.\n"
# Fake checkouts: the port clone after bootstrap, the same clone without gh repo set-default, and the decomp checkout.
# refs are the revisions that resolve; messages, a commit's own message; unpushed, the messages of the commits that a
# revision reaches and origin's tracking refs don't.
CHECKOUTS = {
    "/w/port": {
        "remote.origin.url": PORT_URL,
        "remote.upstream.url": UPSTREAM_URL,
        "remote.upstream.pushurl": "DISABLED-port-never-pushes-upstream",
        "remote.origin.gh-resolved": "base",
        "branch.main.remote": "origin",
        "HEAD": "main",
        "refs": {"HEAD", "main", "upstream/main", "26cd037c8", "linked", "plain"},
        "messages": {"26cd037c8": LINKED, "plain": "Port the renderer\n"},
        "unpushed": {"upstream/main": [LINKED], "linked": [LINKED], "26cd037c8": [LINKED]},
    },
    "/w/port-nodefault": {
        "remote.origin.url": PORT_URL,
        "remote.upstream.url": UPSTREAM_URL,
        "branch.main.remote": "origin",
        "HEAD": "main",
    },
    "/w/decomp": {
        "remote.origin.url": UPSTREAM_URL,
        "remote.fork.url": FORK_URL,
        "branch.main.remote": "origin",
        "HEAD": "main",
    },
}


def fake_git(cwd, args):
    config = CHECKOUTS.get(cwd)
    if config is None:
        return None
    if args[:2] == ["config", "--get-regexp"]:
        pattern = re.compile(args[2])
        return "\n".join(f"{k} {v}" for k, v in config.items() if isinstance(v, str) and pattern.search(k)) or None
    if args[:2] == ["config", "--get"]:
        return config.get(args[2])
    if args[0] == "symbolic-ref":
        return config.get("HEAD")
    if args[:2] == ["remote", "get-url"]:
        name = args[-1]
        return (config.get(f"remote.{name}.pushurl") if "--push" in args else None) or config.get(f"remote.{name}.url")
    if args[0] == "rev-parse" and "--verify" in args:
        name = args[-1].removesuffix("^{commit}")
        return name if name in config.get("refs", ()) else None
    revisions = [a for a in args[1 : args.index("--not") if "--not" in args else None] if not a.startswith("-")]
    if args[0] == "log":
        table = config.get("unpushed", {}) if "--not" in args else config.get("messages", {})
        found = [m for r in revisions for m in ([table.get(r)] if "--not" not in args else table.get(r, [])) if m]
        return "\n".join(found) or None
    if args[0] == "rev-list":
        found = {m for r in revisions for m in config.get("unpushed", {}).get(r, []) if REFERENCE.search(m)}
        return str(len(found)) if "--count" in args else "\n".join("0123abcde" for _ in found) or None
    return None


def self_test():
    with tempfile.TemporaryDirectory() as tmp:
        linked = os.path.join(tmp, "linked.md")
        with open(linked, "w") as f:
            f.write("Port notes.\n\nFollows https://github.com/fuddlesworth/pokebw2/pull/345.\n")
        plain = os.path.join(tmp, "plain.md")
        with open(plain, "w") as f:
            f.write("Merged upstream at fuddlesworth/pokebw2@462e90193 (upstream pull request 345).\n")
        query = os.path.join(tmp, "query.graphql")
        with open(query, "w") as f:
            f.write("query { viewer { login } }\n")
        missing = os.path.join(tmp, "missing.graphql")
        port, decomp, nodefault = "/w/port", "/w/decomp", "/w/port-nodefault"
        upstream_env = {"GH_REPO": "fuddlesworth/pokebw2"}
        heredoc_link = 'git commit -m "$(cat <<\'EOF\'\nRecord the sync\n\nRefs fuddlesworth/pokebw2#315\nEOF\n)"'
        heredoc_text = (
            "git commit -F - <<'EOF'\nAdd port_guard (blocks git push upstream; gh issue comment -R "
            "fuddlesworth/pokebw2)\n\nThe Action merges upstream's main (and its PR #345).\nEOF"
        )
        mutation = 'mutation { addDiscussionComment(input:{discussionId:"D_1", body:"hi"}) { comment { id } } }'
        cases = [
            # (blocked, cwd, command, environment)
            (True, port, "gh issue comment 80 --repo fuddlesworth/pokebw2 --body 'Claiming overlay 033'", {}),
            (True, port, "gh issue close 80 -R fuddlesworth/pokebw2 --reason completed", {}),
            (True, port, "gh pr create -R fuddlesworth/pokebw2 --title t --body b", {}),
            (True, port, "gh pr create --repo=CallanCampos/pokebw2 --fill", {}),
            (True, port, "gh issue comment https://github.com/fuddlesworth/pokebw2/issues/80 --body hi", {}),
            (True, port, "gh pr merge --merge https://github.com/fuddlesworth/pokebw2/pull/3", {}),
            (True, port, "gh issue transfer 3 fuddlesworth/pokebw2 -R CallanCampos/Frigate", {}),
            (True, port, "gh repo fork fuddlesworth/pokebw2 --clone=false", {}),
            (True, port, "gh label create port -R https://github.com/fuddlesworth/pokebw2", {}),
            (True, port, "gh pr review 345 -R fuddlesworth/pokebw2 --approve", {}),
            (True, decomp, "gh issue create --title t --body b", {}),
            (True, nodefault, "gh issue create --title t --body b", {}),
            (True, port, "GH_REPO=fuddlesworth/pokebw2 gh issue create -t t -b b", {}),
            (True, port, "export GH_REPO=fuddlesworth/pokebw2; gh pr comment 3 -b hi", {}),
            (True, nodefault, "GH_REPO=CallanCampos/Frigate; gh issue create -t t -b b", {}),
            (False, nodefault, "GH_REPO=CallanCampos/Frigate; export GH_REPO; gh issue create -t t", {}),
            (True, port, "gh release create v1", upstream_env),
            (True, port, "gh issue comment 5 -R \"$REPO\" -b hi", {}),
            (True, port, 'cd "$DECOMP" && gh issue comment 5 -b hi', {}),
            (True, port, "find . -name x -exec gh issue comment 80 -R fuddlesworth/pokebw2 -b hi \\;", {}),
            (True, port, "gh api repos/fuddlesworth/pokebw2/issues/80/comments -f body=hi", {}),
            (True, port, "gh api -X POST /repos/fuddlesworth/pokebw2/issues/80/reactions -f content=+1", {}),
            (True, port, "gh api --method PUT user/starred/fuddlesworth/pokebw2", {}),
            (True, port, "gh api -XDELETE https://api.github.com/repos/CallanCampos/pokebw2/git/refs/heads/x", {}),
            (True, port, "gh api repositories/1404739979/issues --input body.json", {}),
            (True, decomp, "gh api repos/{owner}/{repo}/issues -f title=t", {}),
            (True, port, "gh api graphql -f query='mutation { addReaction(input:{subjectId:\"I_1\"}) { x } }'", {}),
            (True, port, f"gh api graphql -F query=@- <<'EOF'\n{mutation}\nEOF", {}),
            (True, port, f"echo '{mutation}' | gh api graphql -F query=@-", {}),
            (True, port, "gh api graphql -f query=\"$(cat m.graphql)\"", {}),
            (True, port, "gh api graphql -f query=\"$Q\"", {}),
            (True, port, f"cat > {missing} <<'EOF'\n{mutation}\nEOF\ngh api graphql -F query=@{missing}", {}),
            (True, port, f"gh api graphql --input {missing}", {}),
            (True, port, "curl -s https://api.github.com/graphql -d '{\"query\":\"" + mutation.replace('"', "'")
             + "\"}'", {}),
            (True, port, "curl -s -X POST https://api.github.com/graphql --data @-", {}),
            (True, port, "curl -s -X POST https://api.github.com/repos/fuddlesworth/pokebw2/issues -d '{}'", {}),
            (True, port, "git push upstream main", {}),
            (True, port, "git push fork HEAD:refs/heads/x", {}),
            (True, port, "git push -u https://github.com/fuddlesworth/pokebw2.git HEAD:main", {}),
            (True, port, "git push git@github.com:CallanCampos/pokebw2.git x", {}),
            (True, port, "git push --repo=upstream", {}),
            (True, port, "git -C /w/decomp push origin HEAD:main", {}),
            (True, port, "cd /w/decomp && git push", {}),
            (True, port, "(cd /w/decomp && git push)", {}),
            (True, port, "git -c remote.origin.pushurl=https://github.com/fuddlesworth/pokebw2 push origin main", {}),
            (True, port, "git -c url.https://github.com/fuddlesworth/pokebw2.pushInsteadOf=https://github.com/"
             "CallanCampos/Frigate push origin main", {}),
            (True, port, "git config remote.origin.pushurl https://github.com/fuddlesworth/pokebw2 && git push", {}),
            (True, port, "git config set remote.origin.pushurl git@github.com:CallanCampos/pokebw2.git; "
             "git push origin main", {}),
            (True, port, "git config url.https://github.com/fuddlesworth/pokebw2.pushInsteadOf "
             "https://github.com/CallanCampos/Frigate && git push origin main", {}),
            (True, port, "git remote add up2 https://github.com/fuddlesworth/pokebw2 && git push up2 main", {}),
            (True, port, "git push $REMOTE main", {}),
            (True, port, "git remote add -t main up3 https://github.com/CallanCampos/pokebw2 && git push up3", {}),
            (True, port, "bash --norc -o pipefail -c 'git push fork x'", {}),
            (True, port, "sudo bash <<'EOF'\ngit push upstream x\nEOF", {}),
            (True, port, "bash -c 'gh issue comment 1 -R fuddlesworth/pokebw2 -b hi'", {}),
            (True, port, 'echo "$(gh issue comment 1 -R fuddlesworth/pokebw2 -b hi)"', {}),
            (True, port, "bash <<'EOF'\ngit push fork x\nEOF", {}),
            (True, port, "cat <<'EOF' | sh\ngit push upstream main\nEOF", {}),
            (True, port, "sudo -u me env GH_REPO=fuddlesworth/pokebw2 gh release create v1", {}),
            (True, port, "ls | xargs -n1 gh issue close -R fuddlesworth/pokebw2", {}),
            (True, port, "gh pr create -R CallanCampos/Frigate -t t -b 'Follows fuddlesworth/pokebw2#315'", {}),
            (True, port, f"gh issue create -R CallanCampos/Frigate -t t --body-file {linked}", {}),
            (True, port, "echo 'Refs fuddlesworth/pokebw2#3' | gh issue create -R CallanCampos/Frigate "
             "-t t -F -", {}),
            (True, port, f"cat > {missing} <<'EOF'\nFollows fuddlesworth/pokebw2#315\nEOF\n"
             f"gh issue create -R CallanCampos/Frigate -t t -F {missing}", {}),
            (True, port, "gh issue comment 1 -R CallanCampos/Frigate -b 'thanks @fuddlesworth'", {}),
            (True, port, "git commit -m 'Port the renderer (https://github.com/fuddlesworth/pokebw2/pull/345)'", {}),
            (True, port, "git commit -m 'Work around libsdl-org/SDL#1234'", {}),
            (True, port, "git commit -m 'Thanks @Mariano54 for the notes'", {}),
            (True, port, heredoc_link, {}),
            (True, port, "git commit -F - <<'EOF'\nRefs fuddlesworth/pokebw2#3\nEOF", {}),
            (True, port, "git commit -C 26cd037c8", {}),
            (True, port, "git cherry-pick 26cd037c8 && git push origin HEAD:main", {}),
            (True, port, "git cherry-pick -x -s 26cd037c8", {}),
            (True, port, "git switch -c topic upstream/main && git push -u origin topic", {}),
            (True, port, "git merge --squash upstream/main", {}),
            (True, port, "git push origin linked:main", {}),
            (True, port, "git fetch upstream && git merge upstream/main && git push origin main", {}),
            (False, port, "gh issue view 80 -R fuddlesworth/pokebw2 --comments", {}),
            (False, port, "gh issue list --repo fuddlesworth/pokebw2 --state open --json number,title", {}),
            (False, port, "gh pr view 345 -R fuddlesworth/pokebw2 --json files", {}),
            (False, port, "gh pr checks 345 -R fuddlesworth/pokebw2 --watch", {}),
            (False, port, "gh issue develop --list 80 -R fuddlesworth/pokebw2", {}),
            (False, port, "gh repo clone fuddlesworth/pokebw2 /tmp/x", {}),
            (False, port, "gh co 345 -R fuddlesworth/pokebw2", {}),
            (False, port, "gh search issues pokebw2 --owner fuddlesworth", {}),
            (False, port, "gh label clone fuddlesworth/pokebw2 -R CallanCampos/Frigate", {}),
            (False, port, "gh workflow list --all -R CallanCampos/Frigate", {}),
            (False, port, "R=CallanCampos/Frigate; gh workflow disable build.yml -R $R", {}),
            (True, port, "R=fuddlesworth/pokebw2; gh pr comment 3 -R \"${R}\" -b hi", {}),
            (False, port, "gh issue create -R CallanCampos/Frigate -t 'Sync blocked' -b 'upstream fails'", {}),
            (False, port, "gh issue create --title t --body b", {}),
            (False, port, "gh pr merge https://github.com/CallanCampos/Frigate/pull/3 --merge", {}),
            (False, port, f"gh pr create -R CallanCampos/Frigate -t t -F {plain}", {}),
            (False, port, "gh issue comment 5 -R CallanCampos/Frigate -b "
             "https://github.com/fuddlesworth/pokebw2/commit/462e90193", {}),
            (False, port, "gh issue create -R CallanCampos/Frigate -t sync -b fuddlesworth/pokebw2", {}),
            (False, port, "gh issue view https://github.com/fuddlesworth/pokebw2/issues/315 --json body; "
             "gh issue create -R CallanCampos/Frigate -t t -b b", {}),
            (False, port, "cat > notes.md <<'EOF'\nSee https://github.com/fuddlesworth/pokebw2/issues/315\nEOF\n"
             "gh issue create -R CallanCampos/Frigate -t t -b b", {}),
            (False, port, "gh issue comment 1 -R CallanCampos/Frigate -b 'see @port/CLAUDE.md; "
             "x@fuddlesworth.dev'", {}),
            (False, port, "gh api repos/{owner}/{repo}/issues -f title=t", {}),
            (False, port, "gh api repos/fuddlesworth/pokebw2/commits/main/check-runs --jq .total_count", {}),
            (False, port, "gh api -X GET repos/fuddlesworth/pokebw2/issues -f state=open", {}),
            (False, port, "gh api graphql -f query='query { repository(owner:\"fuddlesworth\") { id } }'", {}),
            (False, port, "gh api graphql -F owner=fuddlesworth -f query='query($owner:String!) "
             "{ repository(owner:$owner, name:\"pokebw2\") { id } }'", {}),
            (False, port, "gh api graphql -F query=@- <<'EOF'\nquery { viewer { login } }\nEOF", {}),
            (False, port, f"gh api graphql -F query=@{query}", {}),
            (False, port, "echo '{\"query\": \"mutation\"}' > /tmp/x.json; "
             "gh api graphql -f query='query{viewer{login}}'", {}),
            (False, port, "curl -s -H \"Authorization: bearer $(gh auth token)\" https://api.github.com/graphql "
             "-d '{\"query\":\"query { viewer { login } }\"}'", {}),
            (False, port, "gh api repos/CallanCampos/Frigate/issues -f body=fuddlesworth/pokebw2@462e901", {}),
            (False, port, "git push origin HEAD:main", {}),
            (False, port, "git push", {}),
            (False, port, "git push origin main 2>&1 | tail -5", {}),
            (False, port, "git push origin --delete old-branch", {}),
            (False, port, "git push --dry-run origin linked:main", {}),
            (False, port, "git checkout -b new-branch && git push -u origin new-branch", {}),
            (False, port, "git fetch upstream && git merge upstream/main", {}),
            (False, port, "git remote set-url --push upstream DISABLED-port-never-pushes-upstream", {}),
            (False, port, "git log --oneline upstream/main | head -5", {}),
            (False, port, "git log -1 --format=%B 26cd037c8 | grep -c 'fuddlesworth/pokebw2#'; git commit -m Sync", {}),
            (False, port, "git commit -C plain", {}),
            (False, port, "git commit -m 'Merge upstream fuddlesworth/pokebw2@462e90193; fixes #3'", {}),
            (False, port, "grep -rn 'fuddlesworth/pokebw2' port/docs | head", {}),
            (False, port, "find port -name '*.md' -exec grep -l 'fuddlesworth/pokebw2#' {} +", {}),
            (False, port, 'echo "never: git push upstream main; gh issue comment 1 -R fuddlesworth/pokebw2"', {}),
            (False, port, heredoc_text, {}),
            (False, port, "port/tools/sync_upstream.sh", {}),
        ]
        failures = 0
        for blocked, cwd, command, env in cases:
            problems = Guard(cwd, env, fake_git).run(command)
            ok = bool(problems) == blocked
            failures += not ok
            shown = command.replace("\n", "\\n")
            shown = shown if len(shown) <= 96 else shown[:93] + "..."
            print(f"{'ok  ' if ok else 'FAIL'} {'block' if blocked else 'allow'}  {shown}")
            if not ok or (blocked and "-v" in sys.argv):
                for problem in problems:
                    print(f"       {problem}")
        blocked_count = sum(1 for c in cases if c[0])
        print(f"{len(cases) - failures} of {len(cases)} cases pass ({blocked_count} blocked, "
              f"{len(cases) - blocked_count} allowed)")
        return 1 if failures else 0


if __name__ == "__main__":
    main()
