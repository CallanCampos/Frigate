// The ROM-wide equivalence test of the host's G2D loaders (port/hal/nnsys/g2d_load.c): every G2D file of the player's
// ROM, alone in NitroFS or a member of a NARC, LZ-compressed or not, is unpacked twice. One copy goes through the
// host's loader, which builds host structs; the other through a simulation of the DS's own unpack, written here from
// the decomp's g2d_*_load.c: 32-bit offsets turned into 32-bit addresses in place, the file standing at a DS address.
// Then every field reachable from the root is compared, and every pointer by where it leads: a raw data pointer must
// lead to the same offset in the file, a pointer to structs to host structs equal to the simulated ones, element by
// element, and a field the DS leaves an offset must hold the same offset. The host's copy must be unchanged.
//
// It reads NitroFS's file allocation table only: every file of the ROM is looked at, by its magic. Without the ROM it
// exits 77, which CTest counts as skipped.
#include "hal/nnsys/g2d_file.h"
#include "hal/nnsys/g2d_load.h"
#include "hal/nnsys/reloc.h"
#include "nnsys/g2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SKIP_STATUS 77
// Where the simulation puts the file: an address in main memory, far from any offset a file holds
#define DS_BASE 0x02200000u
// The mismatches printed in full for each format; the rest are counted
#define MAX_REPORTS 10

enum {
    FORMAT_NCGR,
    FORMAT_NCBR,
    FORMAT_NCLR,
    FORMAT_NCER,
    FORMAT_NMCR,
    FORMAT_NANR,
    FORMAT_NMAR,
    FORMAT_NFTR,
    FORMAT_NSCR,
    FORMAT_COUNT,
};

static const struct {
    const char *name;
    u32 signature;
} sFormats[FORMAT_COUNT] = {
    { "NCGR", 0x4e434752 }, { "NCBR", 0x4e434252 }, { "NCLR", 0x4e434c52 },
    { "NCER", 0x4e434552 }, { "NMCR", 0x4e4d4352 }, { "NANR", 0x4e414e52 },
    { "NMAR", 0x4e4d4152 }, { "NFTR", 0x4e465452 }, { "NSCR", 0x4e534352 },
};

typedef struct FormatCount {
    u32 files;
    u32 compressed;
    u32 equivalent;
    u32 mismatched;
    u32 hostFailed;
    u32 badSize;
    u32 headerLarger;  // Files whose header says they are larger than they are
    u32 embedded;      // Files found compressed inside a file that isn't a NARC
    u32 notNitro;      // NFTR files that aren't NitroSystem fonts (Game Freak's own, under the same magic)
    u32 fontVersion10; // NitroSystem fonts of version 1.0, whose glyph flags the loader clears
    u32 paletteCmp;    // NCLR files with a PCMP block too
    u32 extended;      // Banks with user extended data
    u32 vramTransfer;  // Cell banks with VRAM transfer data
} FormatCount;

static FormatCount sCounts[FORMAT_COUNT];
static u32 sReports[FORMAT_COUNT];
static int sFormat;

// Where the file being compared came from, for the reports
static u32 sFatIndex;
static int sMemberIndex; // Or the offset in a file that isn't a NARC
static BOOL sEmbedded;

// One comparison: the host's copy of the file, the simulation's, and the host tree's
typedef struct Compare {
    const u8 *host; // The host's copy
    u8 *sim;        // The simulation's copy, relocated in place
    u32 size;       // The header's size, or the file's when that is more
    u32 actual;     // The file's own bytes, which the header's size may exceed
    BOOL simFailed; // The simulation left the file
    BOOL mismatched;
    const char *format;
} Compare;

static void Report(Compare *c, const char *what, u32 expected, u64 found) {
    c->mismatched = TRUE;
    if (sReports[sFormat]++ < MAX_REPORTS) {
        printf("  %s FAT %u member %d: %s: DS 0x%08x, host 0x%llx\n", c->format, sFatIndex, sMemberIndex, what,
               expected, (unsigned long long)found);
    }
}

static u32 Get32(const u8 *p) {
    return p[0] | p[1] << 8 | p[2] << 16 | (u32)p[3] << 24;
}

// The simulation's memory: the file at DS_BASE. A read or write outside it marks the simulation failed
static BOOL SimAt(Compare *c, u32 address, u32 size, u32 *offset) {
    if (address < DS_BASE || address - DS_BASE > c->size || size > c->size - (address - DS_BASE)) {
        if (!c->simFailed && sReports[sFormat]++ < MAX_REPORTS) {
            printf("  %s FAT %u member %d: the DS's unpack leaves the file at 0x%08x\n", c->format, sFatIndex,
                   sMemberIndex, address);
        }
        c->simFailed = TRUE;
        return FALSE;
    }
    *offset = address - DS_BASE;
    return TRUE;
}

static u32 R32(Compare *c, u32 address) {
    u32 at;

    if (!SimAt(c, address, 4, &at)) {
        return 0;
    }
    return c->sim[at] | c->sim[at + 1] << 8 | c->sim[at + 2] << 16 | (u32)c->sim[at + 3] << 24;
}

static u16 R16(Compare *c, u32 address) {
    u32 at;

    if (!SimAt(c, address, 2, &at)) {
        return 0;
    }
    return (u16)(c->sim[at] | c->sim[at + 1] << 8);
}

static u8 R8(Compare *c, u32 address) {
    u32 at;

    return SimAt(c, address, 1, &at) ? c->sim[at] : 0;
}

static void W32(Compare *c, u32 address, u32 value) {
    u32 at;

    if (SimAt(c, address, 4, &at)) {
        c->sim[at] = value & 0xff;
        c->sim[at + 1] = (value >> 8) & 0xff;
        c->sim[at + 2] = (value >> 16) & 0xff;
        c->sim[at + 3] = value >> 24;
    }
}

// NNSi_G2dUnpackOffset on the DS: the field, a u32, plus the base, wrapping at 32 bits
static void UnpackOffset(Compare *c, u32 field, u32 base) {
    W32(c, field, R32(c, field) + base);
}

// The DS's unpacks, from the decomp's code, on 32-bit addresses

static void SimUnpackUserExCellAttrBank(Compare *c, u32 attrBank) {
    UnpackOffset(c, attrBank + 4, attrBank);
    for (u16 i = 0; i < R16(c, attrBank); i++) {
        UnpackOffset(c, R32(c, attrBank + 4) + i * 4, attrBank);
    }
}

// NNS_G2dGetCellDataByIdx, for the indices the unpack asks for
static u32 SimCellByIdx(Compare *c, u32 bank, u16 idx) {
    if (R16(c, bank + 2) & NNS_G2D_CELLBK_ATTR_CELLWITHBR) {
        return R32(c, bank + 4) + idx * 16;
    }
    return R32(c, bank + 4) + idx * 8;
}

static void SimUnpackNCE(Compare *c, u32 bank) {
    u32 oamArray;

    UnpackOffset(c, bank + 4, bank);
    // GetPtrOamArrayHead_: after the last cell, of either size
    if (R16(c, bank + 2) & NNS_G2D_CELLBK_ATTR_CELLWITHBR) {
        oamArray = R32(c, bank + 4) + R16(c, bank) * 16;
    } else {
        oamArray = R32(c, bank + 4) + R16(c, bank) * 8;
    }
    for (u16 i = 0; i < R16(c, bank); i++) {
        UnpackOffset(c, SimCellByIdx(c, bank, i) + 4, oamArray);
    }
    if (R32(c, bank + 12) != 0) {
        u32 transfer;

        UnpackOffset(c, bank + 12, bank);
        transfer = R32(c, bank + 12);
        UnpackOffset(c, transfer + 4, transfer);
        W32(c, bank + 12, transfer);
    }
    if (R32(c, bank + 20) != 0) {
        UnpackOffset(c, bank + 20, bank);
        SimUnpackUserExCellAttrBank(c, R32(c, bank + 20) + 8);
    }
}

static void SimUnpackNMC(Compare *c, u32 bank) {
    UnpackOffset(c, bank + 4, bank);
    UnpackOffset(c, bank + 8, bank);
    for (u16 i = 0; i < R16(c, bank); i++) {
        UnpackOffset(c, R32(c, bank + 4) + i * 8 + 4, R32(c, bank + 8));
    }
    if (R32(c, bank + 16) != 0) {
        UnpackOffset(c, bank + 16, bank);
        SimUnpackUserExCellAttrBank(c, R32(c, bank + 16) + 8);
    }
}

static void SimUnpackNAN(Compare *c, u32 bank) {
    u32 seqArray;
    u32 frameArray;
    u32 animData;

    UnpackOffset(c, bank + 4, bank);
    UnpackOffset(c, bank + 8, bank);
    UnpackOffset(c, bank + 12, bank);
    seqArray = R32(c, bank + 4);
    frameArray = R32(c, bank + 8);
    animData = R32(c, bank + 12);
    for (u16 i = 0; i < R16(c, bank); i++) {
        u32 seq = seqArray + i * 16;

        UnpackOffset(c, seq + 12, frameArray);
        for (u16 j = 0; j < R16(c, seq); j++) {
            UnpackOffset(c, R32(c, seq + 12) + j * 8, animData);
        }
    }
    if (R32(c, bank + 20) != 0) {
        u32 attrBank;

        UnpackOffset(c, bank + 20, bank);
        attrBank = R32(c, bank + 20) + 8;
        UnpackOffset(c, attrBank + 4, attrBank);
        for (u32 s = 0; s < R16(c, attrBank); s++) {
            u32 seqAttr = R32(c, attrBank + 4) + s * 12;

            UnpackOffset(c, seqAttr + 4, attrBank);
            UnpackOffset(c, seqAttr + 8, attrBank);
            for (u32 f = 0; f < R16(c, seqAttr); f++) {
                UnpackOffset(c, R32(c, seqAttr + 8) + f * 4, attrBank);
            }
        }
    }
}

static void SimUnpackNFT(Compare *c, u32 header) {
    u32 blk = header + R16(c, header + 12);

    for (int i = 0; i < R16(c, header + 14); i++) {
        u32 data = blk + 8;

        switch (R32(c, blk)) {
        case NNS_G2D_BINBLK_SIG_FINFDATA:
            UnpackOffset(c, data + 8, header);
            if (R32(c, data + 12) != 0) {
                UnpackOffset(c, data + 12, header);
            }
            if (R32(c, data + 16) != 0) {
                UnpackOffset(c, data + 16, header);
            }
            break;
        case NNS_G2D_BINBLK_SIG_CWDHDATA:
            if (R32(c, data + 4) != 0) {
                UnpackOffset(c, data + 4, header);
            }
            break;
        case NNS_G2D_BINBLK_SIG_CMAPDATA:
            if (R32(c, data + 8) != 0) {
                UnpackOffset(c, data + 8, header);
            }
            break;
        }
        if (c->simFailed) {
            return;
        }
        blk += R32(c, blk + 4);
    }
}

// The comparisons. A value field must be the simulation's; a pointer field to raw data must lead to the same offset of
// the file; a field the DS leaves an offset must hold it

static void Same(Compare *c, const char *what, u32 ds, u64 host) {
    if (ds != host) {
        Report(c, what, ds, host);
    }
}

static void SamePointer(Compare *c, const char *what, u32 ds, const void *host) {
    if (ds >= DS_BASE && ds - DS_BASE <= c->size) {
        const u8 *expected = c->host + (ds - DS_BASE);

        if ((const u8 *)host != expected) {
            Report(c, what, ds,
                   (const u8 *)host >= c->host && (const u8 *)host <= c->host + c->size
                       ? DS_BASE + (u64)((const u8 *)host - c->host)
                       : (u64)(uintptr_t)host);
        }
    } else if ((uintptr_t)host != ds) {
        Report(c, what, ds, (u64)(uintptr_t)host);
    }
}

// A pointer to host structs: NULL exactly where the DS's is 0
static BOOL SameStructPointer(Compare *c, const char *what, u32 ds, const void *host) {
    if ((ds == 0) != (host == NULL)) {
        Report(c, what, ds, (u64)(uintptr_t)host);
        return FALSE;
    }
    return ds != 0;
}

static void CompareCharacter(Compare *c, u32 at, const NNSG2dCharacterData *h) {
    Same(c, "character height", R16(c, at), h->H);
    Same(c, "character width", R16(c, at + 2), h->W);
    Same(c, "character pixelFormat", R32(c, at + 4), h->pixelFmt);
    Same(c, "character mappingType", R32(c, at + 8), h->mappingType);
    Same(c, "character characterFormat", R32(c, at + 12), h->characterFmt);
    Same(c, "character size", R32(c, at + 16), h->szByte);
    SamePointer(c, "character rawData", R32(c, at + 20), h->pRawData);
}

static void ComparePalette(Compare *c, u32 at, const NNSG2dPaletteData *h) {
    Same(c, "palette format", R32(c, at), h->fmt);
    Same(c, "palette extendedPalette", R32(c, at + 4), h->bExtendedPlt);
    Same(c, "palette size", R32(c, at + 8), h->szByte);
    SamePointer(c, "palette rawData", R32(c, at + 12), h->pRawData);
}

static void ComparePaletteCmp(Compare *c, u32 at, const NNSG2dPaletteCompressInfo *h) {
    Same(c, "pcmp numPalette", R16(c, at), h->numPalette);
    Same(c, "pcmp pad", R16(c, at + 2), h->pad16);
    SamePointer(c, "pcmp paletteIndexTable", R32(c, at + 4), h->pPlttIdxTbl);
}

static void CompareUserExCellAttrs(Compare *c, u32 blk, const NNSG2dUserExDataBlock *h) {
    const NNSG2dUserExCellAttrBank *bank = (const NNSG2dUserExCellAttrBank *)(h + 1);
    u32 attrBank = blk + 8;

    Same(c, "exdata blkTypeID", R32(c, blk), h->blkTypeID);
    Same(c, "exdata blkSize", R32(c, blk + 4), h->blkSize);
    Same(c, "cell attrs numCells", R16(c, attrBank), bank->numCells);
    Same(c, "cell attrs numAttribute", R16(c, attrBank + 2), bank->numAttribute);
    if (SameStructPointer(c, "cell attrs pCellAttrArray", R32(c, attrBank + 4), bank->pCellAttrArray)) {
        for (u16 i = 0; i < bank->numCells && !c->simFailed; i++) {
            SamePointer(c, "cell attr pAttr", R32(c, R32(c, attrBank + 4) + i * 4), bank->pCellAttrArray[i].pAttr);
        }
    }
}

static void CompareCellBank(Compare *c, u32 at, const NNSG2dCellDataBank *h) {
    BOOL withBR = (R16(c, at + 2) & NNS_G2D_CELLBK_ATTR_CELLWITHBR) != 0;
    u32 cells = R32(c, at + 4);

    Same(c, "cell bank numCells", R16(c, at), h->numCells);
    Same(c, "cell bank cellBankAttr", R16(c, at + 2), h->cellBankAttr);
    Same(c, "cell bank mappingMode", R32(c, at + 8), h->mappingMode);
    SamePointer(c, "cell bank pStringBank", R32(c, at + 16), h->pStringBank);
    if (SameStructPointer(c, "cell bank pCellDataArrayHead", cells, h->pCellDataArrayHead)) {
        for (u16 i = 0; i < h->numCells && !c->simFailed; i++) {
            const NNSG2dCellData *cell = NNS_G2dGetCellDataByIdx(h, i);
            u32 ds = cells + i * (withBR ? 16 : 8);

            Same(c, "cell numOAMAttrs", R16(c, ds), cell->numOAMAttrs);
            Same(c, "cell cellAttr", R16(c, ds + 2), cell->cellAttr);
            SamePointer(c, "cell pOamAttrArray", R32(c, ds + 4), cell->pOamAttrArray);
            if (withBR) {
                const NNSG2dCellBoundingRectS16 *rect = &((const NNSG2dCellDataWithBR *)cell)->boundingRect;

                Same(c, "cell maxX", R16(c, ds + 8), (u16)rect->maxX);
                Same(c, "cell maxY", R16(c, ds + 10), (u16)rect->maxY);
                Same(c, "cell minX", R16(c, ds + 12), (u16)rect->minX);
                Same(c, "cell minY", R16(c, ds + 14), (u16)rect->minY);
            }
        }
    }
    if (SameStructPointer(c, "cell bank pVramTransferData", R32(c, at + 12), h->pVramTransferData)) {
        u32 transfer = R32(c, at + 12);

        sCounts[FORMAT_NCER].vramTransfer++;
        Same(c, "transfer szByteMax", R32(c, transfer), h->pVramTransferData->szByteMax);
        SamePointer(c, "transfer pCellTransferDataArray", R32(c, transfer + 4),
                    h->pVramTransferData->pCellTransferDataArray);
    }
    if (SameStructPointer(c, "cell bank pExtendedData", R32(c, at + 20), h->pExtendedData)) {
        sCounts[FORMAT_NCER].extended++;
        CompareUserExCellAttrs(c, R32(c, at + 20), h->pExtendedData);
    }
}

static void CompareMultiCellBank(Compare *c, u32 at, const NNSG2dMultiCellDataBank *h) {
    u32 cells = R32(c, at + 4);

    Same(c, "mc bank numMultiCellData", R16(c, at), h->numMultiCellData);
    Same(c, "mc bank pad", R16(c, at + 2), h->pad16);
    SamePointer(c, "mc bank pHierarchyDataArray", R32(c, at + 8), h->pHierarchyDataArray);
    SamePointer(c, "mc bank pStringBank", R32(c, at + 12), h->pStringBank);
    if (SameStructPointer(c, "mc bank pMultiCellDataArray", cells, h->pMultiCellDataArray)) {
        for (u16 i = 0; i < h->numMultiCellData && !c->simFailed; i++) {
            const NNSG2dMultiCellData *cell = NNS_G2dGetMultiCellDataByIdx(h, i);

            Same(c, "mc numNodes", R16(c, cells + i * 8), cell->numNodes);
            Same(c, "mc numCellAnim", R16(c, cells + i * 8 + 2), cell->numCellAnim);
            SamePointer(c, "mc pHierDataArray", R32(c, cells + i * 8 + 4), cell->pHierDataArray);
        }
    }
    if (SameStructPointer(c, "mc bank pExtendedData", R32(c, at + 16), h->pExtendedData)) {
        sCounts[FORMAT_NMCR].extended++;
        CompareUserExCellAttrs(c, R32(c, at + 16), h->pExtendedData);
    }
}

static void CompareFrame(Compare *c, u32 ds, const NNSG2dAnimFrameData *frame) {
    SamePointer(c, "frame pContent", R32(c, ds), frame->pContent);
    Same(c, "frame frames", R16(c, ds + 4), frame->frames);
    Same(c, "frame pad", R16(c, ds + 6), frame->pad16);
}

static void CompareAnimBank(Compare *c, u32 at, const NNSG2dAnimBankData *h, int format) {
    u32 sequences = R32(c, at + 4);
    u32 frames = R32(c, at + 8);

    Same(c, "anim bank numSequences", R16(c, at), h->numSequences);
    Same(c, "anim bank numTotalFrames", R16(c, at + 2), h->numTotalFrames);
    SamePointer(c, "anim bank pAnimationData", R32(c, at + 12), h->pAnimContents);
    SamePointer(c, "anim bank pStringBank", R32(c, at + 16), h->pStringBank);
    if (SameStructPointer(c, "anim bank pFrameArray", frames, h->pFrameArrayHead)) {
        for (u16 k = 0; k < h->numTotalFrames && !c->simFailed; k++) {
            CompareFrame(c, frames + k * 8, &h->pFrameArrayHead[k]);
        }
    }
    if (SameStructPointer(c, "anim bank pSequenceArrayHead", sequences, h->pSequenceArrayHead)) {
        for (u16 i = 0; i < h->numSequences && !c->simFailed; i++) {
            const NNSG2dAnimSequence *seq = NNS_G2dGetAnimSequenceByIdx(h, i);
            u32 ds = sequences + i * 16;
            u32 seqFrames = R32(c, ds + 12);

            Same(c, "sequence numFrames", R16(c, ds), seq->numFrames);
            Same(c, "sequence loopStartFrameIdx", R16(c, ds + 2), seq->loopStartFrameIdx);
            Same(c, "sequence animType", R32(c, ds + 4), seq->animType);
            Same(c, "sequence playMode", R32(c, ds + 8), seq->playMode);
            if (SameStructPointer(c, "sequence pAnmFrameArray", seqFrames, seq->pAnmFrameArray)) {
                // Where the DS's frames are the bank's, the host's must be the bank's too
                if (seqFrames >= frames && (seqFrames - frames) % 8 == 0 &&
                    (seqFrames - frames) / 8 + seq->numFrames <= h->numTotalFrames &&
                    seq->pAnmFrameArray != &h->pFrameArrayHead[(seqFrames - frames) / 8]) {
                    Report(c, "sequence pAnmFrameArray isn't the bank's", seqFrames,
                           (u64)(uintptr_t)seq->pAnmFrameArray);
                }
                for (u16 j = 0; j < seq->numFrames && !c->simFailed; j++) {
                    CompareFrame(c, seqFrames + j * 8, &seq->pAnmFrameArray[j]);
                }
            }
        }
    }
    if (SameStructPointer(c, "anim bank pExtendedData", R32(c, at + 20), h->pExtendedData)) {
        u32 blk = R32(c, at + 20);
        u32 attrBank = blk + 8;
        const NNSG2dUserExDataBlock *header = h->pExtendedData;
        const NNSG2dUserExAnimAttrBank *bank = (const NNSG2dUserExAnimAttrBank *)(header + 1);

        sCounts[format].extended++;
        Same(c, "exdata blkTypeID", R32(c, blk), header->blkTypeID);
        Same(c, "exdata blkSize", R32(c, blk + 4), header->blkSize);
        Same(c, "anim attrs numSequences", R16(c, attrBank), bank->numSequences);
        Same(c, "anim attrs numAttribute", R16(c, attrBank + 2), bank->numAttribute);
        if (SameStructPointer(c, "anim attrs pAnmSeqAttrArray", R32(c, attrBank + 4), bank->pAnmSeqAttrArray)) {
            for (u16 s = 0; s < bank->numSequences && !c->simFailed; s++) {
                const NNSG2dUserExAnimSequenceAttr *seqAttr = &bank->pAnmSeqAttrArray[s];
                u32 ds = R32(c, attrBank + 4) + s * 12;

                Same(c, "seq attr numFrames", R16(c, ds), seqAttr->numFrames);
                Same(c, "seq attr pad", R16(c, ds + 2), seqAttr->pad16);
                SamePointer(c, "seq attr pAttr", R32(c, ds + 4), seqAttr->pAttr);
                if (SameStructPointer(c, "seq attr pAnmFrmAttrArray", R32(c, ds + 8), seqAttr->pAnmFrmAttrArray)) {
                    for (u16 f = 0; f < seqAttr->numFrames && !c->simFailed; f++) {
                        SamePointer(c, "frame attr pAttr", R32(c, R32(c, ds + 8) + f * 4),
                                    seqAttr->pAnmFrmAttrArray[f].pAttr);
                    }
                }
            }
        }
    }
}

// A font's node pointer: NULL where the DS's is 0, else the host struct built for where the DS's leads
static void SameFontNode(Compare *c, const PortG2dFont *font, const char *what, u32 ds, const void *host, u32 kind) {
    if (ds == 0) {
        if (host != NULL) {
            Report(c, what, ds, (u64)(uintptr_t)host);
        }
        return;
    }
    if (ds < DS_BASE || ds - DS_BASE >= c->size || host != port_g2d_font_node(font, c->host + (ds - DS_BASE), kind)) {
        Report(c, what, ds, (u64)(uintptr_t)host);
    }
}

static void CompareBytes(Compare *c, const char *what, u32 ds, const void *host, u32 size) {
    u32 at;

    if (SimAt(c, ds, size, &at) && memcmp(c->sim + at, host, size) != 0) {
        Report(c, what, ds, 0);
    }
}

static void CompareFont(Compare *c, const PortG2dFont *font) {
    u32 header = DS_BASE;
    u32 blk = header + R16(c, header + 12);
    BOOL firstInfo = TRUE;

    for (int i = 0; i < R16(c, header + 14) && !c->simFailed; i++) {
        u32 kind = R32(c, blk);
        u32 data = blk + 8;
        const void *at = c->host + (data - DS_BASE);

        if (kind == NNS_G2D_BINBLK_SIG_FINFDATA) {
            const NNSG2dFontInformation *info = port_g2d_font_node(font, at, kind);

            if (info == NULL || (firstInfo && info != &font->info)) {
                Report(c, "FINF node", data, (u64)(uintptr_t)info);
            } else {
                Same(c, "FINF fontType", R8(c, data), info->fontType);
                Same(c, "FINF linefeed", R8(c, data + 1), (u8)info->linefeed);
                Same(c, "FINF alterCharIndex", R16(c, data + 2), info->alterCharIndex);
                Same(c, "FINF defaultWidth.left", R8(c, data + 4), (u8)info->defaultWidth.left);
                Same(c, "FINF defaultWidth.glyphWidth", R8(c, data + 5), info->defaultWidth.glyphWidth);
                Same(c, "FINF defaultWidth.charWidth", R8(c, data + 6), (u8)info->defaultWidth.charWidth);
                Same(c, "FINF encoding", R8(c, data + 7), info->encoding);
                SamePointer(c, "FINF pGlyph", R32(c, data + 8), info->pGlyph);
                SameFontNode(c, font, "FINF pWidth", R32(c, data + 12), info->pWidth, NNS_G2D_BINBLK_SIG_CWDHDATA);
                SameFontNode(c, font, "FINF pMap", R32(c, data + 16), info->pMap, NNS_G2D_BINBLK_SIG_CMAPDATA);
            }
            firstInfo = FALSE;
        } else if (kind == NNS_G2D_BINBLK_SIG_CWDHDATA) {
            const NNSG2dFontWidth *width = port_g2d_font_node(font, at, kind);

            if (width == NULL) {
                Report(c, "CWDH node", data, 0);
            } else {
                Same(c, "CWDH indexBegin", R16(c, data), width->indexBegin);
                Same(c, "CWDH indexEnd", R16(c, data + 2), width->indexEnd);
                SameFontNode(c, font, "CWDH pNext", R32(c, data + 4), width->pNext, NNS_G2D_BINBLK_SIG_CWDHDATA);
                if (width->indexEnd >= width->indexBegin) {
                    CompareBytes(c, "CWDH widthTable", data + 8, width->widthTable,
                                 (width->indexEnd - width->indexBegin + 1) * 3u);
                }
            }
        } else if (kind == NNS_G2D_BINBLK_SIG_CMAPDATA) {
            const NNSG2dFontCodeMap *map = port_g2d_font_node(font, at, kind);

            if (map == NULL) {
                Report(c, "CMAP node", data, 0);
            } else {
                u32 bytes = 0;

                Same(c, "CMAP ccodeBegin", R16(c, data), map->ccodeBegin);
                Same(c, "CMAP ccodeEnd", R16(c, data + 2), map->ccodeEnd);
                Same(c, "CMAP mappingMethod", R16(c, data + 4), map->mappingMethod);
                Same(c, "CMAP reserved", R16(c, data + 6), map->reserved);
                SameFontNode(c, font, "CMAP pNext", R32(c, data + 8), map->pNext, NNS_G2D_BINBLK_SIG_CMAPDATA);
                switch (map->mappingMethod) {
                case NNS_G2D_MAPMETHOD_DIRECT:
                    bytes = 2;
                    break;
                case NNS_G2D_MAPMETHOD_TABLE:
                    bytes = map->ccodeEnd >= map->ccodeBegin ? (map->ccodeEnd - map->ccodeBegin + 1) * 2u : 0;
                    break;
                case NNS_G2D_MAPMETHOD_SCAN:
                    bytes = 2 + R16(c, data + 12) * 4u;
                    break;
                }
                CompareBytes(c, "CMAP mapInfo", data + 12, map->mapInfo, bytes);
            }
        }
        blk += R32(c, blk + 4);
    }
}

// The offset of the block of kind in the file, or 0: NNS_G2dFindBinaryBlock, reading only inside the file
static u32 FindBlock(const u8 *file, u32 size, u32 kind) {
    u32 at = file[12] | file[13] << 8;
    u16 numBlocks = file[14] | file[15] << 8;

    for (u16 i = 0; i < numBlocks && at + 8 <= size; i++) {
        u32 blockKind = file[at] | file[at + 1] << 8 | file[at + 2] << 16 | (u32)file[at + 3] << 24;
        u32 blockSize = file[at + 4] | file[at + 5] << 8 | file[at + 6] << 16 | (u32)file[at + 7] << 24;

        if (blockKind == kind) {
            return at;
        }
        if (blockSize == 0) {
            break;
        }
        at += blockSize;
    }
    return 0;
}

// The tree's range must lie in the file's own bytes, so that freeing the next heap block leaves it
static void CheckRange(Compare *c, const void *key, PortRelocKind kind) {
    const void *begin;
    size_t size;

    if (!port_reloc_range(key, kind, &begin, &size)) {
        Report(c, "no tree registered", 0, 0);
    } else if ((const u8 *)begin < c->host || (const u8 *)begin + size > c->host + c->actual) {
        Report(c, "the tree's range leaves the file", c->actual, (u64)((const u8 *)begin + size - c->host));
    }
}

// Unpacks one block both ways and compares; FALSE when the host's loader refused the file
static BOOL CheckBlock(Compare *c, int format, PortRelocKind kind, u32 blockKind, u8 *hostFile, u32 fileSize) {
    u32 blk = FindBlock(c->sim, c->size, blockKind);
    u32 at = DS_BASE + blk + 8;
    void *root;

    if (blk == 0) {
        return TRUE;
    }
    root = port_g2d_unpack(kind, hostFile, fileSize, hostFile + blk + 8);
    if (root == NULL) {
        if (sReports[sFormat]++ < MAX_REPORTS) {
            printf("  %s FAT %u member %d: the host's loader refuses it: %s\n", c->format, sFatIndex, sMemberIndex,
                   port_g2d_last_error());
        }
        sCounts[format].hostFailed++;
        return FALSE;
    }
    CheckRange(c, hostFile + blk + 8, kind);
    switch (kind) {
    case PORT_RELOC_G2D_CHARACTER:
        W32(c, at + 20, R32(c, at + 20) + at);
        CompareCharacter(c, at, root);
        break;
    case PORT_RELOC_G2D_PALETTE:
        W32(c, at + 12, R32(c, at + 12) + at);
        ComparePalette(c, at, root);
        break;
    case PORT_RELOC_G2D_PALETTE_CMP:
        sCounts[format].paletteCmp++;
        W32(c, at + 4, R32(c, at + 4) + at);
        ComparePaletteCmp(c, at, root);
        break;
    case PORT_RELOC_G2D_CELL_BANK:
        SimUnpackNCE(c, at);
        CompareCellBank(c, at, root);
        break;
    case PORT_RELOC_G2D_MULTICELL:
        SimUnpackNMC(c, at);
        CompareMultiCellBank(c, at, root);
        break;
    case PORT_RELOC_G2D_ANIM_BANK:
        SimUnpackNAN(c, at);
        CompareAnimBank(c, at, root, format);
        break;
    default:
        break;
    }
    return TRUE;
}

// Whether an NFTR file is a NitroSystem font: its blocks lie in it, and its width and code map blocks hold what their
// fields say. The game's own fonts (Game Freak's, read by its font code) share the magic but not the layout; the DWC
// utility's fonts, which NNS_G2dFontInitUTF16 loads, are NitroSystem's
static BOOL IsNitroFont(const u8 *file, u32 size) {
    u32 at = file[12] | file[13] << 8;
    u16 numBlocks = file[14] | file[15] << 8;

    for (u16 i = 0; i < numBlocks; i++) {
        u32 kind;
        u32 blockSize;
        const u8 *data;

        if (at + 8 > size) {
            return FALSE;
        }
        kind = file[at] | file[at + 1] << 8 | file[at + 2] << 16 | (u32)file[at + 3] << 24;
        blockSize = file[at + 4] | file[at + 5] << 8 | file[at + 6] << 16 | (u32)file[at + 7] << 24;
        if (blockSize < 8 || blockSize > size - at) {
            return FALSE;
        }
        data = file + at + 8;
        if (kind == NNS_G2D_BINBLK_SIG_CWDHDATA) {
            u16 begin = data[0] | data[1] << 8;
            u16 end = data[2] | data[3] << 8;

            if (blockSize < 16 || begin > end || (end - begin + 1) * 3u > blockSize - 16) {
                return FALSE;
            }
        } else if (kind == NNS_G2D_BINBLK_SIG_CMAPDATA) {
            u16 begin = data[0] | data[1] << 8;
            u16 end = data[2] | data[3] << 8;
            u16 method = data[4] | data[5] << 8;
            u32 need;

            if (blockSize < 22 || begin > end || method > NNS_G2D_MAPMETHOD_SCAN) {
                return FALSE;
            }
            need = method == NNS_G2D_MAPMETHOD_DIRECT  ? 2
                   : method == NNS_G2D_MAPMETHOD_TABLE ? (end - begin + 1) * 2u
                                                       : 2 + (data[12] | data[13] << 8) * 4u;
            if (need > blockSize - 20) {
                return FALSE;
            }
        }
        at += blockSize;
    }
    return TRUE;
}

static void CheckFile(const u8 *bytes, u32 size, BOOL compressed) {
    int format = -1;
    u32 signature;
    u32 fileSize;
    u32 extent;
    Compare c = { 0 };
    u8 *original;
    u8 *hostFile;
    BOOL ok = TRUE;

    if (size < 16) {
        return;
    }
    signature = bytes[0] | bytes[1] << 8 | bytes[2] << 16 | (u32)bytes[3] << 24;
    for (int i = 0; i < FORMAT_COUNT; i++) {
        if (sFormats[i].signature == signature) {
            format = i;
        }
    }
    if (format < 0) {
        return;
    }
    sFormat = format;
    sCounts[format].files++;
    if (compressed) {
        sCounts[format].compressed++;
    }
    if (sEmbedded) {
        sCounts[format].embedded++;
    }
    if (format == FORMAT_NFTR && !IsNitroFont(bytes, size)) {
        sCounts[format].notNitro++;
        return;
    }
    fileSize = bytes[8] | bytes[9] << 8 | bytes[10] << 16 | (u32)bytes[11] << 24;
    if (fileSize < 16 || fileSize > 64u << 20) {
        if (sReports[sFormat]++ < MAX_REPORTS) {
            printf("  %s FAT %u member %d: the header's size 0x%x can't be the file's (0x%x)\n", sFormats[format].name,
                   sFatIndex, sMemberIndex, fileSize, size);
        }
        sCounts[format].badSize++;
        return;
    }
    if (format == FORMAT_NSCR) {
        // Nothing to relocate: the screen data is the file's own bytes on both
        NNSG2dScreenData *screen;

        if (NNS_G2dGetUnpackedScreenData((void *)bytes, &screen) && (const u8 *)screen > bytes) {
            sCounts[format].equivalent++;
        } else {
            sCounts[format].mismatched++;
        }
        return;
    }

    // A header may say the file is larger than it is (as the game loads it, the rest of its block): both sides see the
    // header's size, with zeros past the file's bytes, and the tree must not reach them
    if (fileSize > size) {
        sCounts[format].headerLarger++;
    }
    extent = fileSize > size ? fileSize : size;
    original = calloc(1, extent);
    memcpy(original, bytes, size);
    hostFile = malloc(extent);
    c.sim = malloc(extent);
    memcpy(hostFile, original, extent);
    memcpy(c.sim, original, extent);
    c.host = hostFile;
    c.size = extent;
    c.actual = size;
    c.format = sFormats[format].name;

    switch (format) {
    case FORMAT_NCGR:
    case FORMAT_NCBR:
        ok = CheckBlock(&c, format, PORT_RELOC_G2D_CHARACTER, NNS_G2D_BINBLK_SIG_CHARACTERDATA, hostFile, fileSize);
        break;
    case FORMAT_NCLR:
        ok = CheckBlock(&c, format, PORT_RELOC_G2D_PALETTE, NNS_G2D_BINBLK_SIG_PALETTEDATA, hostFile, fileSize) &&
             CheckBlock(&c, format, PORT_RELOC_G2D_PALETTE_CMP, NNS_G2D_BINBLK_SIG_PALETTECOMPINFO, hostFile,
                        fileSize);
        break;
    case FORMAT_NCER:
        ok = CheckBlock(&c, format, PORT_RELOC_G2D_CELL_BANK, NNS_G2D_BLKSIG_CELLBANK, hostFile, fileSize);
        break;
    case FORMAT_NMCR:
        ok = CheckBlock(&c, format, PORT_RELOC_G2D_MULTICELL, NNS_G2D_BLKSIG_MULTICELLBANK, hostFile, fileSize);
        break;
    case FORMAT_NANR:
    case FORMAT_NMAR:
        ok = CheckBlock(&c, format, PORT_RELOC_G2D_ANIM_BANK, NNS_G2D_BLKSIG_ANIMBANK, hostFile, fileSize);
        break;
    case FORMAT_NFTR: {
        PortG2dFont *font = port_g2d_unpack(PORT_RELOC_G2D_FONT, hostFile, fileSize, hostFile);

        if (font == NULL) {
            if (sReports[sFormat]++ < MAX_REPORTS) {
                printf("  NFTR FAT %u member %d: the host's loader refuses it: %s\n", sFatIndex, sMemberIndex,
                       port_g2d_last_error());
            }
            sCounts[format].hostFailed++;
            ok = FALSE;
            break;
        }
        CheckRange(&c, hostFile, PORT_RELOC_G2D_FONT);
        SimUnpackNFT(&c, DS_BASE);
        CompareFont(&c, font);
        // The loader the game calls: the same tree, its first FINF, and for a version 1.0 file the glyphs' flags
        // cleared in the file, as the DS clears them
        {
            NNSG2dFontInformation *info;
            u32 finf = FindBlock(c.sim, c.size, NNS_G2D_BINBLK_SIG_FINFDATA);
            u16 version = bytes[6] | bytes[7] << 8;

            if (!NNSi_G2dGetUnpackedFont(hostFile, &info) || finf == 0 || info != &font->info) {
                Report(&c, "NNSi_G2dGetUnpackedFont's FINF", finf, (u64)(uintptr_t)info);
            } else if (version < 0x101) {
                u32 glyph = R32(&c, DS_BASE + finf + 8 + 8) - DS_BASE;

                sCounts[format].fontVersion10++;
                if (glyph + 8 <= extent) {
                    original[glyph + 7] = 0;
                }
            }
        }
        break;
    }
    }

    if (ok) {
        // The host leaves the file as it was
        if (memcmp(hostFile, original, extent) != 0) {
            Report(&c, "the host changed the file", 0, 0);
        }
        if (c.simFailed || c.mismatched) {
            sCounts[format].mismatched++;
        } else {
            sCounts[format].equivalent++;
        }
    }
    port_reloc_release_range(hostFile, extent);
    if (port_reloc_count() != 0) {
        Report(&c, "trees left after releasing the file", 0, port_reloc_count());
        port_reloc_release_all();
    }
    free(hostFile);
    free(c.sim);
    free(original);
}

// LZ77 as the DS's (type 0x10) or extended (0x11), reading and writing only inside the buffers; NULL when the data
// isn't such a stream
static u8 *Uncompress(const u8 *src, u32 srcSize, u32 *outSize, u32 limit) {
    u32 size;
    u32 s = 4;
    u32 d = 0;
    u8 *out;
    BOOL extended;

    if (srcSize < 4 || (src[0] != 0x10 && src[0] != 0x11)) {
        return NULL;
    }
    extended = src[0] == 0x11;
    size = src[1] | src[2] << 8 | (u32)src[3] << 16;
    if (size < 16 || size > 32u << 20) {
        return NULL;
    }
    *outSize = size;
    if (limit != 0 && limit < size) {
        size = limit;
    }
    out = malloc(size);
    while (d < size) {
        u8 flags;

        if (s >= srcSize) {
            free(out);
            return NULL;
        }
        flags = src[s++];
        for (int block = 0; block < 8 && d < size; block++, flags <<= 1) {
            u32 length;
            u32 distance;

            if (!(flags & 0x80)) {
                if (s >= srcSize) {
                    free(out);
                    return NULL;
                }
                out[d++] = src[s++];
                continue;
            }
            // A copy takes two bytes, or three or four for the extended format's longer lengths
            if (s + 2 > srcSize) {
                free(out);
                return NULL;
            }
            if (!extended) {
                length = 3 + (src[s] >> 4);
            } else if (src[s] >> 4 >= 2) {
                length = 1 + (src[s] >> 4);
            } else if (src[s] >> 4 == 0) {
                if (s + 3 > srcSize) {
                    free(out);
                    return NULL;
                }
                length = 0x11 + ((src[s] & 0xf) << 4 | src[s + 1] >> 4);
                s++;
            } else {
                if (s + 4 > srcSize) {
                    free(out);
                    return NULL;
                }
                length = 0x111 + ((src[s] & 0xf) << 12 | src[s + 1] << 4 | src[s + 2] >> 4);
                s += 2;
            }
            distance = ((src[s] & 0xf) << 8 | src[s + 1]) + 1;
            s += 2;
            if (distance > d) {
                free(out);
                return NULL;
            }
            for (; length != 0 && d < size; length--, d++) {
                out[d] = out[d - distance];
            }
        }
    }
    return out;
}

static void CheckMember(const u8 *bytes, u32 size) {
    u8 *uncompressed;
    u32 uncompressedSize;

    if (size >= 4 && bytes[0] == 'R' && bytes[3] == 'N') {
        CheckFile(bytes, size, FALSE);
        return;
    }
    uncompressed = Uncompress(bytes, size, &uncompressedSize, 0);
    if (uncompressed != NULL) {
        if (uncompressed[0] == 'R' && uncompressed[3] == 'N') {
            CheckFile(uncompressed, uncompressedSize, TRUE);
        }
        free(uncompressed);
    }
}

// G2D files inside a file that is neither a NARC nor one itself (the DWC utility's archive, the Download Play
// children): at any offset, a G2D header whose size fits in the rest, or an LZ stream whose first bytes are a G2D
// signature
static BOOL IsSignature(u32 signature) {
    for (int i = 0; i < FORMAT_COUNT; i++) {
        if (sFormats[i].signature == signature) {
            return TRUE;
        }
    }
    return FALSE;
}

static void ScanEmbedded(const u8 *bytes, u32 size) {
    for (u32 at = 0; at + 16 < size; at++) {
        u8 *head;
        u8 *file;
        u32 fileSize;

        sMemberIndex = (int)at;
        if (bytes[at] == 'R' && bytes[at + 3] == 'N' && bytes[at + 4] == 0xff && bytes[at + 5] == 0xfe &&
            IsSignature(Get32(bytes + at))) {
            fileSize = Get32(bytes + at + 8);
            if (fileSize >= 16 && fileSize <= size - at) {
                // Copied to an aligned buffer, where a loaded file is
                file = malloc(fileSize);
                memcpy(file, bytes + at, fileSize);
                sEmbedded = TRUE;
                CheckFile(file, fileSize, FALSE);
                sEmbedded = FALSE;
                free(file);
            }
            continue;
        }
        if (bytes[at] != 0x10 && bytes[at] != 0x11) {
            continue;
        }
        head = Uncompress(bytes + at, size - at, &fileSize, 4);
        if (head == NULL) {
            continue;
        }
        if (IsSignature(Get32(head)) && (file = Uncompress(bytes + at, size - at, &fileSize, 0)) != NULL) {
            sEmbedded = TRUE;
            CheckFile(file, fileSize, TRUE);
            sEmbedded = FALSE;
            free(file);
        }
        free(head);
    }
}

// A NARC: its file allocation block, then its name block, then its image block, whose data the allocations index
static void CheckNarc(const u8 *narc, u32 size) {
    u32 fatb = narc[12] | narc[13] << 8;
    u32 fntb;
    u32 fimg;
    u16 count;

    if (fatb + 12 > size || memcmp(narc + fatb, "BTAF", 4) != 0) {
        return;
    }
    count = narc[fatb + 8] | narc[fatb + 9] << 8;
    fntb = fatb + Get32(narc + fatb + 4);
    if (fntb + 8 > size || memcmp(narc + fntb, "BTNF", 4) != 0) {
        return;
    }
    fimg = fntb + Get32(narc + fntb + 4);
    if (fimg + 8 > size || memcmp(narc + fimg, "GMIF", 4) != 0 || fatb + 12 + count * 8u > size) {
        return;
    }
    for (u16 i = 0; i < count; i++) {
        u32 start = Get32(narc + fatb + 12 + i * 8);
        u32 end = Get32(narc + fatb + 16 + i * 8);

        sMemberIndex = i;
        if (start <= end && fimg + 8 + end <= size) {
            CheckMember(narc + fimg + 8 + start, end - start);
        }
    }
}

int port_nnsys_rom_test(const char *romPath) {
    FILE *rom = fopen(romPath, "rb");
    u8 header[0x200];
    u32 fatOffset;
    u32 fatSize;
    u8 *fat;
    u32 total = 0;
    u32 bad = 0;

    if (rom == NULL) {
        printf("no ROM at %s: skipped\n", romPath);
        return SKIP_STATUS;
    }
    if (fread(header, 1, sizeof(header), rom) != sizeof(header)) {
        fclose(rom);
        printf("%s is too short for a ROM: skipped\n", romPath);
        return SKIP_STATUS;
    }
    fatOffset = Get32(header + 0x48);
    fatSize = Get32(header + 0x4c);
    fat = malloc(fatSize);
    fseek(rom, fatOffset, SEEK_SET);
    if (fread(fat, 1, fatSize, rom) != fatSize) {
        printf("can't read the ROM's file allocation table\n");
        return 1;
    }
    for (u32 i = 0; i < fatSize / 8; i++) {
        u32 start = Get32(fat + i * 8);
        u32 end = Get32(fat + i * 8 + 4);
        u8 *bytes;

        if (end <= start || end - start > 64u << 20) {
            continue;
        }
        bytes = malloc(end - start);
        fseek(rom, start, SEEK_SET);
        if (fread(bytes, 1, end - start, rom) == end - start) {
            sFatIndex = i;
            sMemberIndex = -1;
            if (end - start >= 16 && memcmp(bytes, "NARC", 4) == 0) {
                CheckNarc(bytes, end - start);
            } else if (end - start >= 4 && bytes[0] == 'R' && bytes[3] == 'N') {
                CheckFile(bytes, end - start, FALSE);
            } else {
                CheckMember(bytes, end - start);
                ScanEmbedded(bytes, end - start);
            }
        }
        free(bytes);
    }
    free(fat);
    fclose(rom);

    printf("%-5s %7s %10s %8s %10s %10s %7s %7s %7s %5s %8s %8s\n", "", "files", "compressed", "embedded", "equivalent",
           "mismatched", "refused", "badsize", "larger", "pcmp", "extended", "transfer");
    for (int i = 0; i < FORMAT_COUNT; i++) {
        const FormatCount *count = &sCounts[i];

        printf("%-5s %7u %10u %8u %10u %10u %7u %7u %7u %5u %8u %8u\n", sFormats[i].name, count->files,
               count->compressed, count->embedded, count->equivalent, count->mismatched, count->hostFailed,
               count->badSize, count->headerLarger, count->paletteCmp, count->extended, count->vramTransfer);
        total += count->files;
        bad += count->mismatched + count->hostFailed + count->badSize;
    }
    printf("animation frames the DS relocates more than once (sequences sharing them): %u; never (no sequence's): %u\n",
           port_g2d_anim_stats()->framesSharedBySequences, port_g2d_anim_stats()->framesNeverRelocated);
    printf(
        "NFTR: %u of Game Freak's own fonts under the magic, not NitroSystem's (not compared); %u NitroSystem fonts of "
        "version 1.0\n",
        sCounts[FORMAT_NFTR].notNitro, sCounts[FORMAT_NFTR].fontVersion10);
    total -= sCounts[FORMAT_NFTR].notNitro;
    printf("%u G2D files, %u not equivalent\n", total, bad);
    port_reloc_release_all();
    return total != 0 && bad == 0 ? 0 : 1;
}
