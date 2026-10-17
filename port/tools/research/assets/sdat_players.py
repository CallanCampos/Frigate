import sys, struct, collections
d = open(sys.argv[1], 'rb').read()
symb_off, symb_size, info_off, info_size, fat_off, fat_size, file_off, file_size = struct.unpack_from('<8I', d, 0x10)
recoffs = struct.unpack_from('<8I', d, info_off+8)
nfat = struct.unpack_from('<I', d, fat_off+8)[0]
fat = [struct.unpack_from('<II', d, fat_off+12+i*16) for i in range(nfat)]
def recs(k):
    base = info_off + recoffs[k]
    n = struct.unpack_from('<I', d, base)[0]
    return [struct.unpack_from('<I', d, base+4+i*4)[0] for i in range(n)]
# players: maxSeq u16, chBitMask u16, heapSize u32
for i, o in enumerate(recs(4)):
    if o:
        maxseq, mask, heap = struct.unpack_from('<HHI', d, info_off+o)
        print('player', i, 'maxSeq', maxseq, 'chmask', hex(mask), 'heap', heap)
for i, o in enumerate(recs(6)):
    if o:
        cnt = d[info_off+o]; chans = list(d[info_off+o+1:info_off+o+17])[:cnt]
        print('player2', i, cnt, chans)
for i, o in enumerate(recs(7)):
    if o:
        fid, vol, prio, player, flags = struct.unpack_from('<IBBBB', d, info_off+o)
        print('strm', i, 'file', fid & 0xffffff, 'vol', vol, 'prio', prio, 'player', player, 'flags', flags)
# seq info: fileId u32, bank u16, vol u8, cpr u8, ppr u8, player u8
pc = collections.Counter()
for i, o in enumerate(recs(0)):
    if o:
        fid, bank, vol, cpr, ppr, player = struct.unpack_from('<IHBBBB', d, info_off+o)
        pc[player] += 1
print('seqs per player', dict(pc))
# groups
for i, o in enumerate(recs(5)):
    if o:
        n = struct.unpack_from('<I', d, info_off+o)[0]
        print('group', i, 'entries', n)
# wave formats inside SWARs (sample)
fmt = collections.Counter(); nwaves = 0; ws = 0
for i, o in enumerate(recs(3)):
    if not o: continue
    fid = struct.unpack_from('<I', d, info_off+o)[0] & 0xffffff
    s, l = fat[fid]
    b = d[s:s+l]
    if b[:4] != b'SWAR': continue
    n = struct.unpack_from('<I', b, 0x38)[0]
    for j in range(n):
        wo = struct.unpack_from('<I', b, 0x3C+j*4)[0]
        fmt[b[wo]] += 1; nwaves += 1
        rate = struct.unpack_from('<H', b, wo+2)[0]
print('waves', nwaves, {['PCM8','PCM16','ADPCM'][k]: v for k, v in fmt.items()})
