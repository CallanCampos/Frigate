import sys, os, struct, hashlib, collections, json

root = sys.argv[1]
out = sys.argv[2]

def narc_members(data):
    if data[:4] != b'NARC':
        return None
    off = struct.unpack_from('<H', data, 12)[0]  # header size
    # BTAF
    assert data[off:off+4] == b'BTAF', data[off:off+4]
    btaf_size, count = struct.unpack_from('<IH', data, off+4)
    fat = []
    for i in range(count):
        s, e = struct.unpack_from('<II', data, off+12+i*8)
        fat.append((s, e))
    off2 = off + btaf_size
    assert data[off2:off2+4] == b'BTNF'
    btnf_size = struct.unpack_from('<I', data, off2+4)[0]
    has_names = struct.unpack_from('<I', data, off2+8)[0] != 4 and btnf_size > 16
    off3 = off2 + btnf_size
    assert data[off3:off3+4] == b'GMIF'
    base = off3 + 8
    return [data[base+s:base+e] for s, e in fat], has_names

def lz_decompress(d):
    t = d[0]
    size = d[1] | (d[2] << 8) | (d[3] << 16)
    src = 4
    if size == 0 and len(d) >= 8:
        size = struct.unpack_from('<I', d, 4)[0]; src = 8
    out = bytearray()
    try:
        while len(out) < size:
            flags = d[src]; src += 1
            for bit in range(8):
                if len(out) >= size: break
                if flags & (0x80 >> bit):
                    if t == 0x10:
                        b1, b2 = d[src], d[src+1]; src += 2
                        n = (b1 >> 4) + 3; disp = ((b1 & 0xF) << 8 | b2) + 1
                    else:
                        b1 = d[src]; ind = b1 >> 4
                        if ind == 0:
                            b2, b3 = d[src+1], d[src+2]; src += 3
                            n = (((b1 & 0xF) << 4) | (b3 >> 4)) + 0x11; disp = ((b3 & 0xF) << 8 | d[src-1+0] ) if False else None
                            # recompute properly
                            src -= 3
                            b1, b2, b3 = d[src], d[src+1], d[src+2]; src += 3
                            n = ((b1 & 0xF) << 4 | (b2 >> 4)) + 0x11
                            disp = ((b2 & 0xF) << 8 | b3) + 1
                        elif ind == 1:
                            b1, b2, b3, b4 = d[src], d[src+1], d[src+2], d[src+3]; src += 4
                            n = ((b1 & 0xF) << 12 | b2 << 4 | b3 >> 4) + 0x111
                            disp = ((b3 & 0xF) << 8 | b4) + 1
                        else:
                            b1, b2 = d[src], d[src+1]; src += 2
                            n = (b1 >> 4) + 1; disp = ((b1 & 0xF) << 8 | b2) + 1
                    if disp > len(out): return None
                    for _ in range(n):
                        out.append(out[-disp])
                else:
                    out.append(d[src]); src += 1
    except IndexError:
        return None
    if len(out) != size: return None
    # require consumed nearly all input (padding to 4)
    if len(d) - src > 8: return None
    return bytes(out)

KNOWN = {b'RGCN':'NCGR', b'RLCN':'NCLR', b'RCSN':'NSCR', b'RECN':'NCER', b'RNAN':'NANR', b'RCMN':'NMCR', b'RAMN':'NMAR',
         b'BMD0':'NSBMD', b'BTX0':'NSBTX', b'BCA0':'NSBCA', b'BTP0':'NSBTP', b'BMA0':'NSBMA', b'BVA0':'NSBVA', b'BTA0':'NSBTA',
         b'SDAT':'SDAT', b' APS':'SPL(SPA)', b'SPA ':'SPL(SPA)', b'NARC':'NARC', b'RTFN':'NFTR', b'BCLR':'?', b'RPCN':'NCPR',
         b'SSEQ':'SSEQ', b'SBNK':'SBNK', b'SWAR':'SWAR', b'STRM':'STRM', b'BMDL':'?', b'RCLN':'?', b'RCGN':'?', b'MESG':'MESG',
         b'NTFP':'?', b'BNCL':'?', b'BNBL':'?', b'BNCD':'?'}

def classify(d):
    if len(d) == 0: return 'empty', d
    m = d[:4]
    if m in KNOWN: return KNOWN[m], d
    if d[0] in (0x10, 0x11) and len(d) > 4:
        dec = lz_decompress(d)
        if dec is not None:
            k, _ = classify_raw(dec)
            return ('LZ%02x:' % d[0]) + k, dec
    return classify_raw(d)

def classify_raw(d):
    if len(d) == 0: return 'empty', d
    m = d[:4]
    if m in KNOWN: return KNOWN[m], d
    if all(32 <= c < 127 for c in m): return 'magic:' + m.decode(), d
    return 'raw', d

results = {}
files = []
for dp, dn, fn in os.walk(root):
    for f in fn:
        p = os.path.join(dp, f)
        files.append(os.path.relpath(p, root))
files.sort()
type_counts = collections.Counter()
type_bytes = collections.Counter()
member_type_counts = collections.Counter()
member_type_bytes = collections.Counter()
member_type_dec_bytes = collections.Counter()
archives = {}
total = 0
narcs = 0
for rel in files:
    data = open(os.path.join(root, rel), 'rb').read()
    total += len(data)
    t, _ = classify(data)
    type_counts[t] += 1
    type_bytes[t] += len(data)
    info = {'size': len(data), 'type': t, 'sha1': hashlib.sha1(data).hexdigest()}
    if data[:4] == b'NARC':
        narcs += 1
        mem, named = narc_members(data)
        mtypes = collections.Counter()
        for m in mem:
            mt, dec = classify(m)
            mtypes[mt] += 1
            member_type_counts[mt] += 1
            member_type_bytes[mt] += len(m)
            member_type_dec_bytes[mt] += len(dec)
        info['members'] = len(mem)
        info['named'] = named
        info['member_types'] = dict(mtypes.most_common())
    archives[rel] = info
json.dump({'total_bytes': total, 'files': len(files), 'narcs': narcs, 'archives': archives,
           'type_counts': dict(type_counts), 'type_bytes': dict(type_bytes),
           'member_type_counts': dict(member_type_counts), 'member_type_bytes': dict(member_type_bytes),
           'member_type_dec_bytes': dict(member_type_dec_bytes)}, open(out, 'w'), indent=1)
print('files', len(files), 'bytes', total, 'narcs', narcs)
