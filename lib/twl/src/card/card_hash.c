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
static void CARDi_StartRomHashRead(CARDRomHashContext *context);
static CARDRomHashBlock *CARDi_TouchRomHashBlock(CARDRomHashContext *context, u32 sector);
static void *CARDi_TouchRomHashSector(CARDRomHashContext *context, u32 offset);
static void CARDi_ReadRomHashImageDirect(CARDRomHashContext *context, void *buffer, u32 offset, u32 length);
static void CARDi_ReadRomHashImageCaching(CARDRomHashContext *context, void *buffer, u32 offset, u32 length);

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

u32 CARD_CalcRomHashBufferLength(const CARDTwlRomHeader *header) {
    u32 length = 0;
    length += MATH_ROUNDUP32(header->digestBlockHashes.length);
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
static void CARDi_StartRomHashRead(CARDRomHashContext *context) {
    void *buffer = NULL;
    u32 offset = 0;
    u32 length = 0;
    OSIntrMode enabled = OS_DisableInterrupts();

    if (context->recent_load == NULL) {
        if (context->loading_block != NULL) {
            CARDRomHashBlock *block = context->loading_block;
            u32 position = block->offset;
            u32 head = position & (CARD_ROM_PAGE_SIZE - 1);
            block->hash = block->hash_aligned + head;
            context->recent_load = block;
            buffer = block->hash_aligned;
            offset = position - head;
            length = MATH_ROUNDUP(head + context->sectors_per_block * CARD_ROM_HASH_SIZE, CARD_ROM_PAGE_SIZE);
        } else if (context->loading_sector != NULL) {
            CARDRomHashSector *sector = context->loading_sector;
            context->recent_load = sector;
            buffer = sector->image;
            offset = sector->offset;
            length = context->bytes_per_sector;
        }
    }
    OS_RestoreInterrupts(enabled);

    if (buffer != NULL && context->ReadAsync(context->userdata, buffer, offset, length) == 0) {
        context->ReadSync(context->userdata, buffer, offset, length);
        CARD_NotifyRomHashReadAsync(context);
    }
}

void CARD_NotifyRomHashReadAsync(void *arg) {
    CARDRomHashContext *context = arg;
    OSIntrMode enabled = OS_DisableInterrupts();

    if (context->recent_load == context->loading_sector) {
        CARDRomHashSector *sector = context->loading_sector;
        context->loading_sector = sector->next;
        sector->next = context->loaded_sector;
        context->loaded_sector = sector;
    } else {
        CARDRomHashBlock *block = context->loading_block;
        context->loading_block = block->next;
        block->next = context->loaded_block;
        context->loaded_block = block;
    }
    context->recent_load = NULL;
    OS_RestoreInterrupts(enabled);
    OS_WakeupThreadDirect(context->loader);
    CARDi_StartRomHashRead(context);
}

// The block of the hashes of a sector: the cached one, moved to the front, or the least recently used one, queued to
// be read
static CARDRomHashBlock *CARDi_TouchRomHashBlock(CARDRomHashContext *context, u32 sector) {
    u32 index = sector / context->sectors_per_block;
    CARDRomHashBlock **link = &context->valid_block;
    CARDRomHashBlock *found;

    for (;;) {
        CARDRomHashBlock *block = *link;
        found = block;
        if (block->index == index) {
            *link = block->next;
            block->next = context->valid_block;
            context->valid_block = block;
            break;
        }
        if (block->next == NULL) {
            OSIntrMode enabled = OS_DisableInterrupts();
            CARDRomHashBlock *other;
            for (other = context->loaded_block; other != NULL; other = other->next) {
                if (other->index == index) {
                    found = other;
                    block = NULL;
                    break;
                }
            }
            if (block != NULL) {
                CARDRomHashBlock **tail = &context->loading_block;
                for (other = context->loading_block; other != NULL; other = other->next) {
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
                        context->sector_hash.offset + index * (context->sectors_per_block * CARD_ROM_HASH_SIZE);
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
static inline u32 CARDi_RomOffsetToHashSector(const CARDRomHashContext *context, u32 offset) {
    offset -= context->area_ntr.offset;
    if (offset >= context->area_ntr.length) {
        offset += context->area_ntr.offset - context->area_ltd.offset;
        if (offset < context->area_ltd.length) {
            offset += context->area_ntr.length;
        } else {
            OS_Terminate();
        }
    }
    return offset / context->bytes_per_sector;
}

// The image of the sector at offset if it is cached and checked, moved to the front; otherwise NULL, and the least
// recently used sector is queued to be read
static void *CARDi_TouchRomHashSector(CARDRomHashContext *context, u32 offset) {
    void *image = NULL;
    u32 index = CARDi_RomOffsetToHashSector(context, offset);
    CARDRomHashSector **link;

    for (link = &context->valid_sector;;) {
        CARDRomHashSector *sector = *link;
        if (sector->index == index) {
            *link = sector->next;
            sector->next = context->valid_sector;
            context->valid_sector = sector;
            image = sector->image;
            break;
        }
        if (sector->next == NULL) {
            OSIntrMode enabled = OS_DisableInterrupts();
            CARDRomHashSector **tail;
            sector = *link;
            *link = NULL;
            for (tail = &context->loading_sector; *tail != NULL; tail = &(*tail)->next) {
            }
            *tail = sector;
            sector->index = index;
            sector->offset = offset & ~(context->bytes_per_sector - 1);
            OS_RestoreInterrupts(enabled);
            CARDi_TouchRomHashBlock(context, index);
            CARDi_StartRomHashRead(context);
            break;
        }
        link = &sector->next;
    }
    return image;
}

void CARD_InitRomHashContext(CARDRomHashContext *context, const CARDTwlRomHeader *header, void *buffer, u32 length,
                             CARDiRomReader readSync, CARDiRomReader readAsync, void *userdata) {
    const u32 sectorSize = header->digestSectorSize;
    const u32 sectorsPerBlock = header->digestSectorsPerBlock;
    const u32 blockHashesSize = header->digestBlockHashes.length;
    u8 *lo = (u8 *)MATH_ROUNDUP32((uptr)buffer);
    u8 *hi = (u8 *)MATH_ROUNDDOWN32((uptr)&lo[length]);
    u8 *end = lo;

    context->master_hash = end;
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
        context->area_ntr = header->digestNtr;
        context->area_ltd = header->digestTwl;
        context->sector_hash = header->digestSectorHashes;
        context->block_hash = header->digestBlockHashes;
        context->bytes_per_sector = sectorSize;
        context->sectors_per_block = sectorsPerBlock;
        context->block_max = CARD_ROM_HASH_BLOCK_MAX;
        context->sector_max = CARD_ROM_HASH_SECTOR_MAX;
        context->ReadSync = readSync;
        context->ReadAsync = readAsync;
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
        context->ReadSync(context->userdata, context->master_hash, context->block_hash.offset,
                          context->block_hash.length);
        CARDi_CompareHash(header->digestBlockHashesHash, context->master_hash, context->block_hash.length);
        {
            CARDRomHashSector *sectors = context->sectors;
            int i;
            for (i = 0; i < context->sector_max; ++i) {
                sectors[i].next = &sectors[i - 1];
                sectors[i].image = &context->images[i * context->bytes_per_sector];
                sectors[i].index = 0xffffffff;
            }
            context->sectors[0].next = NULL;
            context->valid_sector = &context->sectors[context->sector_max - 1];
            context->loading_sector = NULL;
            context->loaded_sector = NULL;
        }
        {
            CARDRomHashBlock *blocks = context->blocks;
            const u32 blockLength = MATH_ROUNDUP(CARD_ROM_HASH_SIZE * sectorsPerBlock, CARD_ROM_PAGE_SIZE * 3);
            int i;
            for (i = 0; i < context->block_max; ++i) {
                blocks[i].next = &blocks[i + 1];
                blocks[i].index = 0xffffffff;
                blocks[i].hash_aligned = &context->hashes[i * blockLength];
            }
            context->valid_block = &context->blocks[0];
            context->blocks[context->block_max - 1].next = NULL;
            context->loading_block = NULL;
            context->loaded_block = NULL;
        }
    }
}

// Reads whole sectors straight into buffer, checking each against the hashes of its block as they arrive
static void CARDi_ReadRomHashImageDirect(CARDRomHashContext *context, void *buffer, u32 offset, u32 length) {
    u32 sectorSize = context->bytes_per_sector;
    u32 sectorsPerBlock = context->sectors_per_block;
    u32 position = offset;
    u32 end = length + offset;
    u32 index = CARDi_RomOffsetToHashSector(context, position);

    while (position < end) {
        u32 read = context->ReadSync(context->userdata, buffer, position, end - position);
        CARDi_TouchRomHashBlock(context, index);
        if (context->ReadAsync != NULL && position + read < end) {
            context->ReadAsync(context->userdata, NULL, position + read, end - (position + read));
        }
        while (read >= sectorSize) {
            CARDRomHashBlock *block = CARDi_TouchRomHashBlock(context, index);
            u32 i;
            for (i = index - block->index * sectorsPerBlock; i < sectorsPerBlock && read >= sectorSize;) {
                if (block != context->valid_block) {
                    OSIntrMode enabled = OS_DisableInterrupts();
                    while (context->loading_block != NULL) {
                        OS_SleepThread(NULL);
                    }
                    if (block == context->loaded_block) {
                        context->loaded_block = block->next;
                    }
                    OS_RestoreInterrupts(enabled);
                    CARDi_CompareHash(context->master_hash + block->index * CARD_ROM_HASH_SIZE, block->hash,
                                      sectorsPerBlock * CARD_ROM_HASH_SIZE);
                    block->next = context->valid_block;
                    context->valid_block = block;
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
static void CARDi_ReadRomHashImageCaching(CARDRomHashContext *context, void *buffer, u32 offset, u32 length) {
    while (length > 0) {
        u8 *image = CARDi_TouchRomHashSector(context, offset);
        if (image != NULL) {
            u32 position = offset - (offset & ~(context->bytes_per_sector - 1));
            u32 size = MATH_MIN(length, context->bytes_per_sector - position);
            MI_CpuCopy8(image + position, buffer, size);
            buffer = (u8 *)buffer + size;
            length -= size;
            offset += size;
        } else {
            u32 position = offset & ~(context->bytes_per_sector - 1);
            u32 end = offset + length;
            while (context->valid_sector != NULL && context->valid_block != NULL) {
                position += context->bytes_per_sector;
                if (position >= end) {
                    break;
                }
                CARDi_TouchRomHashSector(context, position);
            }
            for (;;) {
                OSIntrMode enabled = OS_DisableInterrupts();
                CARDRomHashBlock *block;
                CARDRomHashSector *sector = NULL;
                while ((context->loading_sector != NULL && context->loaded_sector == NULL) ||
                       (context->loading_block != NULL && context->loaded_block == NULL)) {
                    OS_SleepThread(NULL);
                }
                block = context->loaded_block;
                if (block != NULL) {
                    context->loaded_block = block->next;
                } else {
                    sector = context->loaded_sector;
                    if (sector != NULL) {
                        context->loaded_sector = sector->next;
                    }
                }
                OS_RestoreInterrupts(enabled);
                if (block != NULL) {
                    CARDi_CompareHash(context->master_hash + block->index * CARD_ROM_HASH_SIZE, block->hash,
                                      context->sectors_per_block * CARD_ROM_HASH_SIZE);
                    block->next = context->valid_block;
                    context->valid_block = block;
                } else if (sector != NULL) {
                    CARDRomHashBlock *owner = CARDi_TouchRomHashBlock(context, sector->index);
                    CARDi_CompareHash(owner->hash + (sector->index - owner->index * context->sectors_per_block) *
                                                        CARD_ROM_HASH_SIZE,
                                      sector->image, context->bytes_per_sector);
                    sector->next = context->valid_sector;
                    context->valid_sector = sector;
                } else {
                    break;
                }
            }
        }
    }
}

void CARD_ReadRomHashImage(CARDRomHashContext *context, void *buffer, u32 offset, u32 length) {
    context->loader = OS_GetCurrentThread();
    context->recent_load = NULL;
    if (length >= context->bytes_per_sector * 16 && OS_GetBootType() != OS_BOOTTYPE_ROM) {
        u32 head = (context->bytes_per_sector - offset) & (context->bytes_per_sector - 1);
        u32 body = (length - head) & ~(context->bytes_per_sector - 1);
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
