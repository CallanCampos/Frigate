"""Very small C scanner: finds top-level function definitions and data definitions in a C file.

Not a C parser. It strips comments, string/char literals and preprocessor lines (keeping both branches of #if), then
walks the text at brace depth 0. A `{` at depth 0 whose statement ends in `)` and has no top-level `=` is a function
definition; the defined name is the identifier just before the parameter list. Data definitions are collected as the
set of identifiers of top-level non-extern, non-typedef declarations (approximate, used only for name lookup).
"""
import re

IDENT = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
KEYWORDS = {
    "if", "while", "for", "switch", "return", "sizeof", "static", "const", "volatile", "inline", "extern", "struct",
    "union", "enum", "void", "int", "char", "short", "long", "unsigned", "signed", "float", "double", "asm",
    "__attribute__", "typedef", "register",
}


def strip(text: str) -> str:
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            i = n if j < 0 else j
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            seg = text[i:(n if j < 0 else j + 2)]
            out.append("\n" * seg.count("\n") or " ")
            i = n if j < 0 else j + 2
            continue
        if c in "\"'":
            q = c
            j = i + 1
            while j < n and text[j] != q:
                if text[j] == "\\":
                    j += 1
                j += 1
            out.append(q + q)
            i = j + 1
            continue
        out.append(c)
        i += 1
    s = "".join(out)
    # drop preprocessor lines (with continuations)
    # Keeps the first branch of every #if/#ifdef/#ifndef and drops #elif/#else branches, so braces stay balanced.
    lines = s.split("\n")
    res = []
    cont = False
    stack = []  # list of booleans: is the current branch active
    active = True
    for line in lines:
        if cont:
            cont = line.rstrip().endswith("\\")
            res.append("")
            continue
        st = line.lstrip()
        if st.startswith("#"):
            cont = line.rstrip().endswith("\\")
            d = st[1:].lstrip()
            word = d.split()[0] if d.split() else ""
            if word in ("if", "ifdef", "ifndef"):
                stack.append(active)
            elif word in ("elif", "else"):
                active = False
            elif word == "endif":
                active = stack.pop() if stack else True
            res.append("")
            continue
        res.append(line if active else "")
    return "\n".join(res)


def scan(text: str):
    """Returns (functions: dict name -> {'asm': bool, 'static': bool}, data_idents: set, lines)."""
    s = strip(text)
    funcs = {}
    data_idents = set()
    depth = 0
    stmt_start = 0
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c == "{" and depth == 0:
            stmt = s[stmt_start:i]
            st = stmt.strip()
            # top-level '=' outside parentheses?
            pd = 0
            has_eq = False
            for ch in st:
                if ch == "(":
                    pd += 1
                elif ch == ")":
                    pd -= 1
                elif ch == "=" and pd == 0:
                    has_eq = True
                    break
            # find matching close brace
            d = 0
            j = i
            while j < n:
                if s[j] == "{":
                    d += 1
                elif s[j] == "}":
                    d -= 1
                    if d == 0:
                        break
                j += 1
            if st.endswith(")") and not has_eq and not st.startswith("typedef"):
                name = func_name(st)
                if name:
                    toks = set(IDENT.findall(st.split("(")[0]))
                    funcs[name] = {"asm": "asm" in toks, "static": "static" in toks, "inline": "inline" in toks}
                stmt_start = j + 1
                i = j + 1
                continue
            if st.startswith("asm") and IDENT.findall(st)[:1] == ["asm"] and "(" not in st:
                # file-level asm block; ignore
                stmt_start = j + 1
                i = j + 1
                continue
            # struct/union/enum body or initializer: continue to ';'
            i = j + 1
            continue
        if c == ";" and depth == 0:
            stmt = s[stmt_start:i].strip()
            add_data(stmt, data_idents)
            stmt_start = i + 1
        i += 1
    return funcs, data_idents


def func_name(st: str):
    # identifier before the first top-level '(' (handles 'type name(args)')
    # For declarations like 'void (*Get(int))(void)' fall back to the identifier before the innermost param list.
    idx = st.find("(")
    head = st[:idx]
    ids = IDENT.findall(head)
    if ids and ids[-1] not in KEYWORDS:
        return ids[-1]
    m = re.search(r"\(\s*\*\s*(?:const\s+)?([A-Za-z_][A-Za-z_0-9]*)\s*\(", st)
    if m:
        return m.group(1)
    return None


def add_data(stmt: str, idents: set):
    if not stmt:
        return
    first = IDENT.findall(stmt[:40])
    if not first:
        return
    if first[0] in ("typedef", "extern") or (len(first) > 1 and first[0] == "static" and first[1] == "extern"):
        return
    # declaration head: before the initializer
    pd = 0
    cut = len(stmt)
    for k, ch in enumerate(stmt):
        if ch == "(":
            pd += 1
        elif ch == ")":
            pd -= 1
        elif ch == "=" and pd == 0:
            cut = k
            break
    head = stmt[:cut]
    while True:
        h2 = re.sub(r"\{[^{}]*\}", " ", head)
        if h2 == head:
            break
        head = h2
    # a prototype: 'T name(args)' with nothing after ')' (and no '[' before the paren group)
    if re.search(r"^[^\[(]*\b[A-Za-z_][A-Za-z_0-9]*\s*\([^()]*(\([^()]*\)[^()]*)*\)\s*$", head) and cut == len(stmt):
        return
    for ident in IDENT.findall(head):
        if ident not in KEYWORDS:
            idents.add(ident)
