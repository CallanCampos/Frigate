"""A survey of the sound archive (SDAT): its blocks, its files, and its records by kind.

    sdat_survey.py SOUND_DATA.sdat

Prints the archive's blocks and file table, then, for each kind of record (sequences, sequence archives, banks,
wave archives, players, groups, streams), how many there are with their names, and how sequences split between
music, sound effects and cries: the material the sound driver plays (port/hal/snd7). Research of 2026-10-16
(port/docs/research/assets-arm7-sound.md).
"""
import sys, struct, collections
d = open(sys.argv[1], 'rb').read()
assert d[:4] == b'SDAT'
hdr_size, nblocks = struct.unpack_from('<HH', d, 0xC)
symb_off, symb_size, info_off, info_size, fat_off, fat_size, file_off, file_size = struct.unpack_from('<8I', d, 0x10)
print('size', len(d), 'blocks', nblocks, 'symb', symb_off, symb_size, 'info', info_off, info_size, 'fat', fat_off, fat_size, 'file', file_off, file_size)
# FAT
assert d[fat_off:fat_off+4] == b'FAT '
nfiles = struct.unpack_from('<I', d, fat_off+8)[0]
fat = [struct.unpack_from('<II', d, fat_off+12+i*16) for i in range(nfiles)]
print('fat files', nfiles)
kinds = ['SEQ', 'SEQARC', 'BANK', 'WAVEARC', 'PLAYER', 'GROUP', 'PLAYER2', 'STRM']
recoffs = struct.unpack_from('<8I', d, info_off+8)
def names(kind_idx):
    if not symb_size: return None
    off = struct.unpack_from('<I', d, symb_off+8+kind_idx*4)[0]
    if off == 0: return None
    base = symb_off + off
    n = struct.unpack_from('<I', d, base)[0]
    res = []
    for i in range(n):
        if kind_idx == 1:
            p = struct.unpack_from('<I', d, base+4+i*8)[0]
        else:
            p = struct.unpack_from('<I', d, base+4+i*4)[0]
        if p == 0: res.append(None); continue
        e = d.index(b'\0', symb_off+p)
        res.append(d[symb_off+p:e].decode('ascii', 'replace'))
    return res
file_users = collections.defaultdict(list)
for k, ro in enumerate(recoffs):
    base = info_off + ro
    n = struct.unpack_from('<I', d, base)[0]
    offs = [struct.unpack_from('<I', d, base+4+i*4)[0] for i in range(n)]
    valid = [o for o in offs if o]
    nm = names(k)
    print(kinds[k], 'entries', n, 'valid', len(valid), 'named', sum(1 for x in (nm or []) if x))
    if k in (0, 1, 2, 3, 7):
        sizes = 0; cnt = 0; fmt = collections.Counter(); total_samples = 0
        for i, o in enumerate(offs):
            if not o: continue
            fid = struct.unpack_from('<I', d, info_off+o)[0] & 0xFFFFFF
            file_users[fid].append((kinds[k], i))
            if fid < nfiles:
                s, l = fat[fid]
                sizes += l; cnt += 1
                if k == 7:
                    blob = d[s:s+l]
                    if blob[:4] == b'STRM':
                        # HEAD block at 0x10
                        h = 0x18
                        wtype, loop, ch = blob[h], blob[h+1], blob[h+2]
                        rate = struct.unpack_from('<H', blob, h+4)[0]
                        nsamp = struct.unpack_from('<I', blob, h+0xC)[0]
                        fmt[(['PCM8','PCM16','ADPCM'][wtype], ch, rate)] += 1
                        total_samples += nsamp / rate
        print('   files', cnt, 'bytes', sizes, dict(fmt) if fmt else '', ('stream seconds %.0f' % total_samples) if k == 7 else '')
        if nm and k == 7: print('   names', [x for x in nm if x][:40])
        if nm and k == 4: pass
    if k == 4:
        print('   players', [x for x in (nm or [])][:40])
    if k == 6:
        print('   player2', [x for x in (nm or [])][:20])
    if k == 5:
        print('   groups', [x for x in (nm or [])][:20], '...')
    if k == 0 and nm:
        bgm = [x for x in nm if x and x.startswith('SEQ_BGM')]
        se = [x for x in nm if x and x.startswith('SEQ_SE')]
        pv = [x for x in nm if x and x.startswith('SEQ_PV')]
        oth = [x for x in nm if x and not x.startswith(('SEQ_BGM','SEQ_SE','SEQ_PV'))]
        print('   BGM', len(bgm), 'SE', len(se), 'PV', len(pv), 'other', len(oth), oth[:15])
    if k == 3 and nm:
        pv = [x for x in nm if x and x.startswith('WAVE_ARC_PV')]
        print('   wavearc PV', len(pv))
