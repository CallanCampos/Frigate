"""Summarize [Decomp] Overlay NNN issues and their claims from gh_issues.json (gh issue list ... --json ...)."""
import json, re, sys
d = json.load(open(sys.argv[1] if len(sys.argv) > 1 else 'gh_issues.json'))
ov = [i for i in d if i['title'].startswith('[Decomp] Overlay')]
print('overlay issues', len(ov), 'open', sum(i['state'] == 'OPEN' for i in ov), 'closed', sum(i['state'] == 'CLOSED' for i in ov))
rows = []
for i in sorted(ov, key=lambda x: x['title']):
    if i['state'] != 'OPEN':
        continue
    claims = {}
    for c in sorted(i['comments'], key=lambda c: c['createdAt']):
        a = c['author']['login']; b = c['body'].lower()
        rel = re.search(r"releas(e|ing) (my|the|our)?\s*(overlay \d+ )?(game-code range )?claim|unclaim|claim (is )?released|releasing overlay|i (thought i )?released", b)
        clm = re.search(r"(^|\. |\n)claiming\b|claiming (the rest of )?overlay|keeping my claim|claim on the [^.]* (is )?still active", b)
        if rel:
            claims[a] = ('released', c['createdAt'][:10])
        elif clm:
            claims[a] = ('claimed', c['createdAt'][:10])
    active = [f"{a} ({d_})" for a, (s, d_) in claims.items() if s == 'claimed']
    released = [f"{a} ({d_})" for a, (s, d_) in claims.items() if s == 'released']
    m = re.search(r'Current source coverage: ([^\n]*)', i['body'])
    rows.append((i['number'], i['title'].replace('[Decomp] ', ''), len(i['comments']), ', '.join(active) or '-',
                 ', '.join(released) or '-', m.group(1) if m else ''))
for r in rows:
    print('| #%d | %s | %d | %s | %s | %s |' % r)
