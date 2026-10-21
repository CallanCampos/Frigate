#ifndef POKEBW2_TWL_CARD_H
#define POKEBW2_TWL_CARD_H

#include "types.h"
#include "nitro/card.h"

// TwlSDK's part of the card library in the LTD autoload: the interface of the DSi's new DMA channels, which CARD reads
// the ROM with on a DSi
extern const CARDDmaInterface CARDi_NdmaInterface;
// TwlSDK's part of CARDi_InitRom for a DSi, in the LTD autoload
void CARDi_InitTwlRom(void);

// card_hash.c: the ROM reads that check what they read against the keyed hashes of the DSi's ROM header. A sector of
// the ROM is checked against its hash in the table of the sectors' hashes, and a run of those hashes, a block, against
// its hash in the table of the blocks' hashes, which the header's own hash covers. The context caches the last sectors
// and blocks read

// A sector in the cache: its index among the checked sectors, its offset in the ROM and its image
typedef struct CARDRomHashSector {
    struct CARDRomHashSector *next;
    u32 index;
    u32 offset;
    void *image;
} CARDRomHashSector;

// A block of the sectors' hashes in the cache: its index, the offset of its hashes in the ROM, the hashes, and the
// buffer they are read into, from the ROM page they start in
typedef struct CARDRomHashBlock {
    struct CARDRomHashBlock *next;
    u32 index;
    u32 offset;
    u8 *hash;
    u8 *image;
} CARDRomHashBlock;

typedef struct CARDCheckedReadContext {
    ROMHeaderRegion areaNtr;
    ROMHeaderRegion areaTwl;
    ROMHeaderRegion sectorHashes;
    ROMHeaderRegion blockHashes;
    u32 sectorSize;
    u32 sectorsPerBlock;
    u32 blockCount;
    u32 sectorCount;
    // The functions that read the ROM, waiting for the end or not, and their first argument. The second returns 0 when
    // it can't read without waiting
    void *userdata;
    CARDiRomReader readSync;
    CARDiRomReader readAsync;
    // The thread that reads, which an asynchronous read wakes when it is done
    OSThread *thread;
    // The sector or block being read, and the lists of the sectors and of the blocks: to read, read but not checked,
    // and checked, the most recently used first
    void *loading;
    CARDRomHashSector *loadingSectors;
    CARDRomHashSector *loadedSectors;
    CARDRomHashSector *validSectors;
    CARDRomHashBlock *loadingBlocks;
    CARDRomHashBlock *loadedBlocks;
    CARDRomHashBlock *validBlocks;
    // The table of the blocks' hashes, the images of the sectors and the hashes of the blocks, and the entries of each
    u8 *masterHash;
    u8 *images;
    u8 *hashes;
    CARDRomHashSector *sectors;
    CARDRomHashBlock *blocks;
} CARDCheckedReadContext;

// The size of the buffer that CARD_SetUpCheckedReads needs for a ROM's header
u32 CARD_GetCheckedReadBufferSize(const CARDTwlRomHeader *header);
// Lays the context's caches out in the buffer and reads and checks the table of the blocks' hashes; ends the program
// if the buffer is too small or the table doesn't match
void CARD_SetUpCheckedReads(CARDCheckedReadContext *context, const CARDTwlRomHeader *header, void *buffer, u32 length,
                            CARDiRomReader readSync, CARDiRomReader readAsync, void *userdata);
// The callback of an asynchronous read of the context's: moves the sector or block read to the list of those to check
// and starts the next read
void CARD_OnCheckedReadDone(void *arg);
// Reads length bytes of the ROM at offset into buffer, and checks them
void CARD_ReadRomChecked(CARDCheckedReadContext *context, void *buffer, u32 offset, u32 length);

#endif // POKEBW2_TWL_CARD_H
