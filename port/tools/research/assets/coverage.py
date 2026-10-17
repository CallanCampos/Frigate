import sys, json, ast, re
W = sys.argv[1]
src = open(W + '/configure.py').read()
d = json.load(open(sys.argv[2]))
built = set()
for m in re.finditer(r'"(a/\d/\d/\d|swan_sound_data\.sdat)"', src):
    built.add(m.group(1))
total = d['total_bytes']
cov = sum(d['archives'][p]['size'] for p in built if p in d['archives'])
print('built paths', len(built), 'bytes', cov, 'of', total, '%.1f%%' % (100*cov/total))
nosdat = [p for p in built if p != 'swan_sound_data.sdat']
cov2 = sum(d['archives'][p]['size'] for p in nosdat)
print('excluding sdat', len(nosdat), cov2, '%.1f%%' % (100*cov2/total))
print(sorted(built))
# biggest unconverted archives
rest = sorted(((a['size'], p, a.get('members',0), ', '.join(f'{k}:{v}' for k,v in list(a.get('member_types',{}).items())[:3])) for p,a in d['archives'].items() if p not in built), reverse=True)
for r in rest[:25]: print(*r)
