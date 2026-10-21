// TwlSDK's card_hash.c, in the LTD autoload: the reads of the ROM that check each sector against the HMAC-SHA-1
// digests of the DSi's ROM header, with caches of the sectors and of the blocks of their hashes. The ROM names none of
// it; the functions are named for what they do
#include "nitro/math.h"
#include "nitro/os.h"
#include "twl/card.h"
#include "twl/syscall.h"

#define CARD_ROM_HASH_SIZE 20
#define CARD_ROM_HASH_BLOCK_MAX 4
#define CARD_ROM_HASH_SECTOR_MAX 32
// A block's hashes are read from the start of their ROM page
#define CARD_ROM_PAGE_SIZE 0x200

static void CARDi_CompareHash(const void *hash, void *buffer, u32 length);
static void CARDi_StartRomHashRead(CARDCheckedReadContext *context);
static CARDRomHashBlock *CARDi_TouchRomHashBlock(CARDCheckedReadContext *context, u32 sector);
static void *CARDi_TouchRomHashSector(CARDCheckedReadContext *context, u32 offset);
static void CARDi_ReadRomHashImageDirect(CARDCheckedReadContext *context, void *buffer, u32 offset, u32 length);
static void CARDi_ReadRomHashImageCaching(CARDCheckedReadContext *context, void *buffer, u32 offset, u32 length);

// The SHA-1 states after the key's inner and outer pads, from which each HMAC starts
static SVCSHA1Context sInnerContext;
static SVCSHA1Context sOuterContext;

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".bss"                                                                                  \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

// The HMAC key of the DSi's digests
static const u8 sDigestKey[64] ATTRIBUTE_ALIGN(4) = {
    0x21, 0x06, 0xc0, 0xde, 0xba, 0x98, 0xce, 0x3f, 0xa6, 0x92, 0xe3, 0x9d, 0x46, 0xf2, 0xed, 0x01,
    0x76, 0xe3, 0xcc, 0x08, 0x56, 0x23, 0x63, 0xfa, 0xca, 0xd4, 0xec, 0xdf, 0x9a, 0x62, 0x78, 0x34,
    0x8f, 0x6d, 0x63, 0x3c, 0xfe, 0x22, 0xca, 0x92, 0x20, 0x88, 0x97, 0x23, 0xd2, 0xcf, 0xae, 0xc2,
    0x32, 0x67, 0x8d, 0xfe, 0xca, 0x83, 0x64, 0x98, 0xac, 0xfd, 0x3e, 0x37, 0x87, 0x46, 0x58, 0x24,
};

u32 CARD_GetCheckedReadBufferSize(const CARDTwlRomHeader *header) {
    u32 length = 0;
    length += MATH_ROUNDUP32(header->digestBlockHashes.size);
    length += MATH_ROUNDUP32(header->digestSectorSize * CARD_ROM_HASH_SECTOR_MAX);
    length += MATH_ROUNDUP32(MATH_ROUNDUP(header->digestSectorsPerBlock * CARD_ROM_HASH_SIZE, CARD_ROM_PAGE_SIZE * 3) *
                             CARD_ROM_HASH_BLOCK_MAX);
    length += MATH_ROUNDUP32(sizeof(CARDRomHashSector) * CARD_ROM_HASH_SECTOR_MAX);
    length += MATH_ROUNDUP32(sizeof(CARDRomHashBlock) * CARD_ROM_HASH_BLOCK_MAX);
    return length;
}

// Checks length bytes of buffer against their HMAC. A mismatch clears the buffer if the card was pulled out of a
// game booted from it, and ends the program otherwise
static void CARDi_CompareHash(const void *hash, void *buffer, u32 length) {
    u8 digest[CARD_ROM_HASH_SIZE];
    SVCSHA1Context context;
    u32 *actual = (u32 *)digest;
    const u32 *expected = hash;
    u32 diff;

    MI_CpuCopy8(&sInnerContext, &context, sizeof(context));
    SVC_SHA1Update(&context, buffer, length);
    SVC_SHA1GetHash(&context, actual);
    MI_CpuCopy8(&sOuterContext, &context, sizeof(context));
    SVC_SHA1Update(&context, actual, CARD_ROM_HASH_SIZE);
    SVC_SHA1GetHash(&context, actual);
    diff = 0;
    diff |= expected[0] ^ actual[0];
    diff |= expected[1] ^ actual[1];
    diff |= expected[2] ^ actual[2];
    diff |= expected[3] ^ actual[3];
    diff |= expected[4] ^ actual[4];
    if (diff != 0) {
        if (OS_GetBootType() == OS_BOOTTYPE_ROM && CARD_IsPulledOut()) {
            MI_CpuFill(buffer, 0, length);
        } else {
            OS_Terminate();
        }
    }
}

// Starts the read of the next block, or else of the next sector, unless one is being read. A block's hashes are read
// in whole pages
static void CARDi_StartRomHashRead(CARDCheckedReadContext *context) {
    void *buffer = NULL;
    u32 offset = 0;
    u32 length = 0;
    u32 enabled = OS_DisableInterrupts();

    if (context->loading == NULL) {
        if (context->loadingBlocks != NULL) {
            CARDRomHashBlock *block = context->loadingBlocks;
            u32 position = block->offset;
            u32 head = position & (CARD_ROM_PAGE_SIZE - 1);
            block->hash = block->image + head;
            context->loading = block;
            buffer = block->image;
            offset = position - head;
            length = MATH_ROUNDUP(head + context->sectorsPerBlock * CARD_ROM_HASH_SIZE, CARD_ROM_PAGE_SIZE);
        } else if (context->loadingSectors != NULL) {
            CARDRomHashSector *sector = context->loadingSectors;
            context->loading = sector;
            buffer = sector->image;
            offset = sector->offset;
            length = context->sectorSize;
        }
    }
    OS_RestoreInterrupts(enabled);

    if (buffer != NULL && context->readAsync(context->userdata, buffer, offset, length) == 0) {
        context->readSync(context->userdata, buffer, offset, length);
        CARD_OnCheckedReadDone(context);
    }
}

void CARD_OnCheckedReadDone(void *arg) {
    CARDCheckedReadContext *context = arg;
    u32 enabled = OS_DisableInterrupts();

    if (context->loading == context->loadingSectors) {
        CARDRomHashSector *sector = context->loadingSectors;
        context->loadingSectors = sector->next;
        sector->next = context->loadedSectors;
        context->loadedSectors = sector;
    } else {
        CARDRomHashBlock *block = context->loadingBlocks;
        context->loadingBlocks = block->next;
        block->next = context->loadedBlocks;
        context->loadedBlocks = block;
    }
    context->loading = NULL;
    OS_RestoreInterrupts(enabled);
    OS_WakeupThreadDirect(context->thread);
    CARDi_StartRomHashRead(context);
}

// The block of the hashes of a sector: the cached one, moved to the front, or the least recently used one, queued to
// be read
static CARDRomHashBlock *CARDi_TouchRomHashBlock(CARDCheckedReadContext *context, u32 sector) {
    u32 index = sector / context->sectorsPerBlock;
    CARDRomHashBlock **link = &context->validBlocks;
    CARDRomHashBlock *found;

    for (;;) {
        CARDRomHashBlock *block = *link;
        found = block;
        if (block->index == index) {
            *link = block->next;
            block->next = context->validBlocks;
            context->validBlocks = block;
            break;
        }
        if (block->next == NULL) {
            u32 enabled = OS_DisableInterrupts();
            CARDRomHashBlock *other;
            for (other = context->loadedBlocks; other != NULL; other = other->next) {
                if (other->index == index) {
                    found = other;
                    block = NULL;
                    break;
                }
            }
            if (block != NULL) {
                CARDRomHashBlock **tail = &context->loadingBlocks;
                for (other = context->loadingBlocks; other != NULL; other = other->next) {
                    if (other->index == index) {
                        found = other;
                        block = NULL;
                        break;
                    }
                    tail = &other->next;
                }
                if (block != NULL) {
                    *link = NULL;
                    *tail = block;
                    block->index = index;
                    block->offset =
                        context->sectorHashes.offset + index * (context->sectorsPerBlock * CARD_ROM_HASH_SIZE);
                }
            }
            OS_RestoreInterrupts(enabled);
            if (block != NULL) {
                CARDi_StartRomHashRead(context);
            }
            break;
        }
        link = &block->next;
    }
    return found;
}

// The index of the sector of the hashed areas that holds the card's offset, counting the NITRO area's sectors first
static inline u32 CARDi_RomOffsetToHashSector(const CARDCheckedReadContext *context, u32 offset) {
    offset -= context->areaNtr.offset;
    if (offset >= context->areaNtr.size) {
        offset += context->areaNtr.offset - context->areaTwl.offset;
        if (offset < context->areaTwl.size) {
            offset += context->areaNtr.size;
        } else {
            OS_Terminate();
        }
    }
    return offset / context->sectorSize;
}

// The image of the sector at offset if it is cached and checked, moved to the front; otherwise NULL, and the least
// recently used sector is queued to be read
static void *CARDi_TouchRomHashSector(CARDCheckedReadContext *context, u32 offset) {
    void *image = NULL;
    u32 index = CARDi_RomOffsetToHashSector(context, offset);
    CARDRomHashSector **link;

    for (link = &context->validSectors;;) {
        CARDRomHashSector *sector = *link;
        if (sector->index == index) {
            *link = sector->next;
            sector->next = context->validSectors;
            context->validSectors = sector;
            image = sector->image;
            break;
        }
        if (sector->next == NULL) {
            u32 enabled = OS_DisableInterrupts();
            CARDRomHashSector **tail;
            sector = *link;
            *link = NULL;
            for (tail = &context->loadingSectors; *tail != NULL; tail = &(*tail)->next) {
            }
            *tail = sector;
            sector->index = index;
            sector->offset = offset & ~(context->sectorSize - 1);
            OS_RestoreInterrupts(enabled);
            CARDi_TouchRomHashBlock(context, index);
            CARDi_StartRomHashRead(context);
            break;
        }
        link = &sector->next;
    }
    return image;
}

void CARD_SetUpCheckedReads(CARDCheckedReadContext *context, const CARDTwlRomHeader *header, void *buffer, u32 length,
                            CARDiRomReader readSync, CARDiRomReader readAsync, void *userdata) {
    const u32 sectorSize = header->digestSectorSize;
    const u32 sectorsPerBlock = header->digestSectorsPerBlock;
    const u32 blockHashesSize = header->digestBlockHashes.size;
    u8 *lo = (u8 *)MATH_ROUNDUP32((uptr)buffer);
    u8 *hi = (u8 *)MATH_ROUNDDOWN32((uptr)&lo[length]);
    u8 *end = lo;

    context->masterHash = end;
    end += MATH_ROUNDUP32(blockHashesSize);
    context->images = end;
    end += MATH_ROUNDUP32(sectorSize * CARD_ROM_HASH_SECTOR_MAX);
    context->hashes = end;
    end += MATH_ROUNDUP32(MATH_ROUNDUP(CARD_ROM_HASH_SIZE * sectorsPerBlock, CARD_ROM_PAGE_SIZE * 3) *
                          CARD_ROM_HASH_BLOCK_MAX);
    context->sectors = (CARDRomHashSector *)end;
    end += MATH_ROUNDUP32(sizeof(*context->sectors) * CARD_ROM_HASH_SECTOR_MAX);
    context->blocks = (CARDRomHashBlock *)end;
    end += MATH_ROUNDUP32(sizeof(*context->blocks) * CARD_ROM_HASH_BLOCK_MAX);
    if (end > hi) {
        OS_Terminate();
    } else {
        MI_CpuClear32(lo, (u32)(end - lo));
        context->areaNtr = header->digestNtr;
        context->areaTwl = header->digestTwl;
        context->sectorHashes = header->digestSectorHashes;
        context->blockHashes = header->digestBlockHashes;
        context->sectorSize = sectorSize;
        context->sectorsPerBlock = sectorsPerBlock;
        context->blockCount = CARD_ROM_HASH_BLOCK_MAX;
        context->sectorCount = CARD_ROM_HASH_SECTOR_MAX;
        context->readSync = readSync;
        context->readAsync = readAsync;
        context->userdata = userdata;
        {
            u8 ipad[64];
            u8 opad[64];
            int i;
            for (i = 0; i < 64; ++i) {
                ipad[i] = sDigestKey[i] ^ 0x36;
                opad[i] = sDigestKey[i] ^ 0x5c;
            }
            SVC_SHA1Init(&sInnerContext);
            SVC_SHA1Init(&sOuterContext);
            SVC_SHA1Update(&sInnerContext, ipad, sizeof(ipad));
            SVC_SHA1Update(&sOuterContext, opad, sizeof(opad));
        }
        context->readSync(context->userdata, context->masterHash, context->blockHashes.offset,
                          context->blockHashes.size);
        CARDi_CompareHash(header->digestBlockHashesHash, context->masterHash, context->blockHashes.size);
        {
            CARDRomHashSector *sectors = context->sectors;
            int i;
            for (i = 0; i < context->sectorCount; ++i) {
                sectors[i].next = &sectors[i - 1];
                sectors[i].image = &context->images[i * context->sectorSize];
                sectors[i].index = 0xffffffff;
            }
            context->sectors[0].next = NULL;
            context->validSectors = &context->sectors[context->sectorCount - 1];
            context->loadingSectors = NULL;
            context->loadedSectors = NULL;
        }
        {
            CARDRomHashBlock *blocks = context->blocks;
            const u32 blockLength = MATH_ROUNDUP(CARD_ROM_HASH_SIZE * sectorsPerBlock, CARD_ROM_PAGE_SIZE * 3);
            int i;
            for (i = 0; i < context->blockCount; ++i) {
                blocks[i].next = &blocks[i + 1];
                blocks[i].index = 0xffffffff;
                blocks[i].image = &context->hashes[i * blockLength];
            }
            context->validBlocks = &context->blocks[0];
            context->blocks[context->blockCount - 1].next = NULL;
            context->loadingBlocks = NULL;
            context->loadedBlocks = NULL;
        }
    }
}

// Reads whole sectors straight into buffer, checking each against the hashes of its block as they arrive
static void CARDi_ReadRomHashImageDirect(CARDCheckedReadContext *context, void *buffer, u32 offset, u32 length) {
    u32 sectorSize = context->sectorSize;
    u32 sectorsPerBlock = context->sectorsPerBlock;
    u32 position = offset;
    u32 end = length + offset;
    u32 index = CARDi_RomOffsetToHashSector(context, position);

    while (position < end) {
        u32 read = context->readSync(context->userdata, buffer, position, end - position);
        CARDi_TouchRomHashBlock(context, index);
        if (context->readAsync != NULL && position + read < end) {
            context->readAsync(context->userdata, NULL, position + read, end - (position + read));
        }
        while (read >= sectorSize) {
            CARDRomHashBlock *block = CARDi_TouchRomHashBlock(context, index);
            u32 i;
            for (i = index - block->index * sectorsPerBlock; i < sectorsPerBlock && read >= sectorSize;) {
                if (block != context->validBlocks) {
                    u32 enabled = OS_DisableInterrupts();
                    while (context->loadingBlocks != NULL) {
                        OS_SleepThread(NULL);
                    }
                    if (block == context->loadedBlocks) {
                        context->loadedBlocks = block->next;
                    }
                    OS_RestoreInterrupts(enabled);
                    CARDi_CompareHash(context->masterHash + block->index * CARD_ROM_HASH_SIZE, block->hash,
                                      sectorsPerBlock * CARD_ROM_HASH_SIZE);
                    block->next = context->validBlocks;
                    context->validBlocks = block;
                }
                CARDi_CompareHash(block->hash + i * CARD_ROM_HASH_SIZE, buffer, sectorSize);
                position += sectorSize;
                read -= sectorSize;
                buffer = (u8 *)buffer + sectorSize;
                i++;
                index++;
            }
        }
    }
}

// Reads through the cache of sectors, checking the sectors and blocks as they arrive
static void CARDi_ReadRomHashImageCaching(CARDCheckedReadContext *context, void *buffer, u32 offset, u32 length) {
    while (length > 0) {
        u8 *image = CARDi_TouchRomHashSector(context, offset);
        if (image != NULL) {
            u32 position = offset - (offset & ~(context->sectorSize - 1));
            u32 size = MATH_MIN(length, context->sectorSize - position);
            MI_CpuCopy8(image + position, buffer, size);
            buffer = (u8 *)buffer + size;
            length -= size;
            offset += size;
        } else {
            u32 position = offset & ~(context->sectorSize - 1);
            u32 end = offset + length;
            while (context->validSectors != NULL && context->validBlocks != NULL) {
                position += context->sectorSize;
                if (position >= end) {
                    break;
                }
                CARDi_TouchRomHashSector(context, position);
            }
            for (;;) {
                u32 enabled = OS_DisableInterrupts();
                CARDRomHashBlock *block;
                CARDRomHashSector *sector = NULL;
                while ((context->loadingSectors != NULL && context->loadedSectors == NULL) ||
                       (context->loadingBlocks != NULL && context->loadedBlocks == NULL)) {
                    OS_SleepThread(NULL);
                }
                block = context->loadedBlocks;
                if (block != NULL) {
                    context->loadedBlocks = block->next;
                } else {
                    sector = context->loadedSectors;
                    if (sector != NULL) {
                        context->loadedSectors = sector->next;
                    }
                }
                OS_RestoreInterrupts(enabled);
                if (block != NULL) {
                    CARDi_CompareHash(context->masterHash + block->index * CARD_ROM_HASH_SIZE, block->hash,
                                      context->sectorsPerBlock * CARD_ROM_HASH_SIZE);
                    block->next = context->validBlocks;
                    context->validBlocks = block;
                } else if (sector != NULL) {
                    CARDRomHashBlock *owner = CARDi_TouchRomHashBlock(context, sector->index);
                    CARDi_CompareHash(owner->hash + (sector->index - owner->index * context->sectorsPerBlock) *
                                                        CARD_ROM_HASH_SIZE,
                                      sector->image, context->sectorSize);
                    sector->next = context->validSectors;
                    context->validSectors = sector;
                } else {
                    break;
                }
            }
        }
    }
}

void CARD_ReadRomChecked(CARDCheckedReadContext *context, void *buffer, u32 offset, u32 length) {
    context->thread = OS_GetCurrentThread();
    context->loading = NULL;
    if (length >= context->sectorSize * 16 && OS_GetBootType() != OS_BOOTTYPE_ROM) {
        u32 head = (context->sectorSize - offset) & (context->sectorSize - 1);
        u32 body = (length - head) & ~(context->sectorSize - 1);
        if (head != 0) {
            CARDi_ReadRomHashImageCaching(context, buffer, offset, head);
            offset += head;
            length -= head;
            buffer = (u8 *)buffer + head;
        }
        if (body != 0) {
            CARDi_ReadRomHashImageDirect(context, buffer, offset, body);
            offset += body;
            length -= body;
            buffer = (u8 *)buffer + body;
        }
    }
    CARDi_ReadRomHashImageCaching(context, buffer, offset, length);
}

#pragma section LTDMAIN end
