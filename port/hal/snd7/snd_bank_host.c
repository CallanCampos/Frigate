// NitroSDK's snd_bank.c on the host, in place of the decomp's lib/nitro/src/snd/snd_bank.c
// (port/hal/snd7/replaced.txt): the links between a bank and its wave archives, the walk over a bank's instruments, and
// a wave archive's table of wave addresses. A bank (SBNK) and a wave archive (SWAR) are files the ARM9 loads and the
// ARM7 reads, and they keep ARM9 addresses in 32-bit fields: a bank's four slots link to their archive and to the next
// bank's slot that uses it, an archive links to the first such slot, and a wave the sound heap loads by itself has its
// address in the archive's table in place of its offset in the file (an offset is below 0x02000000, an address of main
// memory above it). The DS's snd.h declares the links as pointers, which on a 64-bit host would be 8 bytes and move
// every field after them.
//
// So these functions keep the files' own layout, field by field at the DS's offsets, and put in each 32-bit field a
// handle (snd7_handle): 0x80000000 and the index of the host pointer in a table, which the host's ARM7 resolves
// (snd7_handle_pointer). A handle is never taken for an offset, since it is above 0x02000000, and is the same for the
// same pointer. The ARM7 reads the same fields. On a host the decomp's snd.h declares the link fields as 32-bit words
// too (port/docs/game-changes.md), so NitroSystem's loader and the game, which read a wave archive's waveCount and
// waveOffset themselves, find them at the DS's offsets. The handles are also the game's to use (snd_handle.h, and
// port_host.h for the decomp's files), for NitroSystem's file table, which keeps file addresses the same way.
#include "hal/pxi/pxi_bus.h"
#include "hal/snd7/snd7_internal.h"
#include "hal/snd7/snd_handle.h"
#include "nitro/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The DS's layout of the files' heads
#define WAVEARC_TOP_LINK 0x18
#define WAVEARC_WAVE_COUNT 0x38
#define WAVEARC_OFFSET_TABLE 0x3c
#define BANK_WAVEARC_LINK 0x18
#define BANK_INST_COUNT 0x38
#define BANK_INST_OFFSET 0x3c
#define LINK_SIZE 8
#define LINK_WAVEARC 0
#define LINK_NEXT 4

// The handles: their pointers, and an open-addressing table from a pointer to its handle's index (0 for none)
static struct {
    const void **pointers;
    u32 count;
    u32 capacity;
    u32 *index;
    u32 indexSize;
} sHandles;

static u32 HashPointer(const void *pointer, u32 size) {
    u64 value = (u64)(uintptr_t)pointer;

    value ^= value >> 33;
    value *= 0xff51afd7ed558ccdull;
    value ^= value >> 33;
    return (u32)value & (size - 1);
}

static void Grow(void) {
    u32 newCapacity = sHandles.capacity != 0 ? sHandles.capacity * 2 : 256;
    u32 newIndexSize = newCapacity * 2;
    const void **pointers = realloc(sHandles.pointers, newCapacity * sizeof(*pointers));
    u32 *index = calloc(newIndexSize, sizeof(*index));
    u32 i;

    if (pointers == NULL || index == NULL) {
        fprintf(stderr, "port: sound: no memory for the bank handles\n");
        abort();
    }
    for (i = 0; i < sHandles.count; i++) {
        u32 slot = HashPointer(pointers[i], newIndexSize);

        while (index[slot] != 0) {
            slot = (slot + 1) & (newIndexSize - 1);
        }
        index[slot] = i + 1;
    }
    free(sHandles.index);
    sHandles.pointers = pointers;
    sHandles.capacity = newCapacity;
    sHandles.index = index;
    sHandles.indexSize = newIndexSize;
}

u32 snd7_handle(const void *pointer) {
    u32 slot;

    if (pointer == NULL) {
        return 0;
    }
    if (sHandles.count == sHandles.capacity) {
        Grow();
    }
    slot = HashPointer(pointer, sHandles.indexSize);
    while (sHandles.index[slot] != 0) {
        if (sHandles.pointers[sHandles.index[slot] - 1] == pointer) {
            return SND7_HANDLE_FLAG | sHandles.index[slot];
        }
        slot = (slot + 1) & (sHandles.indexSize - 1);
    }
    sHandles.pointers[sHandles.count++] = pointer;
    sHandles.index[slot] = sHandles.count;
    return SND7_HANDLE_FLAG | sHandles.count;
}

void *snd7_handle_pointer(u32 handle) {
    u32 index = handle & ~SND7_HANDLE_FLAG;

    if (!(handle & SND7_HANDLE_FLAG) || index == 0 || index > sHandles.count) {
        return NULL;
    }
    return (void *)(uintptr_t)sHandles.pointers[index - 1];
}

uint32_t port_snd_handle(const void *pointer) {
    return snd7_handle(pointer);
}

void *port_snd_pointer(uint32_t handle) {
    return snd7_handle_pointer(handle);
}

// The files' 32-bit fields
static u32 Read32(const void *base, u32 offset) {
    u32 value;

    memcpy(&value, (const u8 *)base + offset, sizeof(value));
    return value;
}

static void Write32(void *base, u32 offset, u32 value) {
    memcpy((u8 *)base + offset, &value, sizeof(value));
}

static u8 *Link(SNDBankData *bank, int index) {
    return (u8 *)bank + BANK_WAVEARC_LINK + LINK_SIZE * (u32)index;
}

// Takes a bank's slot out of the list of the wave archive it links to
static void Unlink(u8 *link) {
    u8 *waveArc = snd7_handle_pointer(Read32(link, LINK_WAVEARC));
    u32 handle = snd7_handle(link);
    u32 prev;

    if (waveArc == NULL) {
        return;
    }
    prev = Read32(waveArc, WAVEARC_TOP_LINK);
    if (prev == handle) {
        Write32(waveArc, WAVEARC_TOP_LINK, Read32(link, LINK_NEXT));
        return;
    }
    while (prev != 0) {
        u8 *prevLink = snd7_handle_pointer(prev);

        if (prevLink == NULL) {
            return;
        }
        if (Read32(prevLink, LINK_NEXT) == handle) {
            Write32(prevLink, LINK_NEXT, Read32(link, LINK_NEXT));
            return;
        }
        prev = Read32(prevLink, LINK_NEXT);
    }
}

// NitroSDK's SND_AssignWaveArc: makes the archive the one that slot index of the bank uses, and takes the slot out of
// the list of the archive that it had
void SND_AssignWaveArc(SNDBankData *bank, int index, SNDWaveArc *waveArc) {
    u8 *link = Link(bank, index);
    u32 current;

    SNDi_LockMutex();
    current = Read32(link, LINK_WAVEARC);
    if (current != 0) {
        if (snd7_handle_pointer(current) == waveArc) {
            SNDi_UnlockMutex();
            return;
        }
        Unlink(link);
    }
    Write32(link, LINK_NEXT, Read32(waveArc, WAVEARC_TOP_LINK));
    Write32(waveArc, WAVEARC_TOP_LINK, snd7_handle(link));
    Write32(link, LINK_WAVEARC, snd7_handle(waveArc));
    SNDi_UnlockMutex();
}

// NitroSDK's SND_DestroyBank: takes the bank's slots out of the lists of the wave archives that it uses
void SND_DestroyBank(SNDBankData *bank) {
    int i;

    SNDi_LockMutex();
    for (i = 0; i < SND_BANK_TO_WAVEARC_MAX; i++) {
        if (Read32(Link(bank, i), LINK_WAVEARC) != 0) {
            Unlink(Link(bank, i));
        }
    }
    SNDi_UnlockMutex();
}

// NitroSDK's SND_DestroyWaveArc: empties the slots of the banks that use the archive
void SND_DestroyWaveArc(SNDWaveArc *waveArc) {
    u32 handle;

    SNDi_LockMutex();
    handle = Read32(waveArc, WAVEARC_TOP_LINK);
    while (handle != 0) {
        u8 *link = snd7_handle_pointer(handle);

        if (link == NULL) {
            break;
        }
        handle = Read32(link, LINK_NEXT);
        Write32(link, LINK_WAVEARC, 0);
        Write32(link, LINK_NEXT, 0);
    }
    SNDi_UnlockMutex();
}

// NitroSDK's SND_GetFirstInstDataPos: the position of the first program
SNDInstPos SND_GetFirstInstDataPos(const SNDBankData *bank) {
    SNDInstPos pos;

    pos.prgNo = 0;
    pos.index = 0;
    return pos;
}

// NitroSDK's SND_GetNextInstData: reads the instrument at pos and moves on. A drum set or a key split yields its
// instruments one by one, the other programs one. FALSE when the bank has no more
BOOL SND_GetNextInstData(const SNDBankData *bank, SNDInstData *inst, SNDInstPos *pos) {
    const u8 *file = (const u8 *)bank;

    for (; pos->prgNo < Read32(file, BANK_INST_COUNT); pos->prgNo++, pos->index = 0) {
        u32 offset = Read32(file, BANK_INST_OFFSET + 4 * pos->prgNo);
        const u8 *data = file + (offset >> 8);

        inst->type = (u8)offset;
        switch (inst->type) {
        case SND_INST_PCM:
        case SND_INST_PSG:
        case SND_INST_NOISE:
        case SND_INST_DIRECTPCM:
        case SND_INST_NULL:
            memcpy(&inst->param, data, sizeof(inst->param));
            pos->prgNo++;
            return TRUE;
        case SND_INST_DRUM_SET:
            if (pos->index < (u32)(data[1] - data[0] + 1)) {
                memcpy(inst, data + 2 + sizeof(SNDInstData) * pos->index, sizeof(*inst));
                pos->index++;
                return TRUE;
            }
            break;
        case SND_INST_KEY_SPLIT:
            if (pos->index < 8 && data[pos->index] != 0) {
                memcpy(inst, data + 8 + sizeof(SNDInstData) * pos->index, sizeof(*inst));
                pos->index++;
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

// NitroSDK's SND_GetWaveDataCount: the number of waves in the archive
u32 SND_GetWaveDataCount(const SNDWaveArc *waveArc) {
    return Read32(waveArc, WAVEARC_WAVE_COUNT);
}

// NitroSDK's SND_SetWaveDataAddress: the address that a wave of the archive is loaded at, as a handle
void SND_SetWaveDataAddress(SNDWaveArc *waveArc, int index, const SNDWaveData *address) {
    SNDi_LockMutex();
    Write32(waveArc, WAVEARC_OFFSET_TABLE + 4 * (u32)index, snd7_handle(address));
    SNDi_UnlockMutex();
}

// NitroSDK's SND_GetWaveDataAddress: the address of a wave of the archive. An offset in the table is from the archive,
// a handle is the wave's address, and 0 is a wave that isn't loaded. An address put there by other code is found
// among the ARM9's memory by its low 32 bits
const SNDWaveData *SND_GetWaveDataAddress(const SNDWaveArc *waveArc, int index) {
    const SNDWaveData *wave;
    u32 entry;

    SNDi_LockMutex();
    entry = Read32(waveArc, WAVEARC_OFFSET_TABLE + 4 * (u32)index);
    if (entry == 0) {
        wave = NULL;
    } else if (entry < 0x02000000) {
        wave = (const SNDWaveData *)((const u8 *)waveArc + entry);
    } else if (entry & SND7_HANDLE_FLAG && snd7_handle_pointer(entry) != NULL) {
        wave = snd7_handle_pointer(entry);
    } else {
        wave = port_pxi_pointer(entry, NULL);
    }
    SNDi_UnlockMutex();
    return wave;
}
