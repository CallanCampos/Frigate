// NitroSDK's SND (snd_bank.c, a guess from the SDK's file names): the links between a bank and the wave archives
// that hold its waves, the walk over a bank's instruments, and the table of a wave archive's wave addresses.
#include "nitro/os.h"
#include "nitro/snd.h"

// NitroSDK's SND_AssignWaveArc: makes the archive the one that slot index of the bank uses, and takes the slot out of
// the list of the archive that it had
void SND_AssignWaveArc(SNDBankData *bank, int index, SNDWaveArc *waveArc) {
    SNDWaveArcLink *top;

    SNDi_LockMutex();
    if (bank->waveArcLink[index].waveArc != NULL) {
        SNDWaveArcLink *prev;

        if (waveArc == bank->waveArcLink[index].waveArc) {
            SNDi_UnlockMutex();
            return;
        }
        prev = bank->waveArcLink[index].waveArc->topLink;
        if (&bank->waveArcLink[index] == prev) {
            bank->waveArcLink[index].waveArc->topLink = bank->waveArcLink[index].next;
            DC_StoreRange(bank->waveArcLink[index].waveArc, sizeof(SNDWaveArc));
        } else {
            while (prev != NULL) {
                if (&bank->waveArcLink[index] == prev->next) {
                    break;
                }
                prev = prev->next;
            }
            prev->next = bank->waveArcLink[index].next;
            DC_StoreRange(prev, sizeof(SNDWaveArcLink));
        }
    }
    top = waveArc->topLink;
    waveArc->topLink = &bank->waveArcLink[index];
    bank->waveArcLink[index].next = top;
    bank->waveArcLink[index].waveArc = waveArc;
    SNDi_UnlockMutex();
    DC_StoreRange(bank, sizeof(SNDBankData));
    DC_StoreRange(waveArc, sizeof(SNDWaveArc));
}

// NitroSDK's SND_DestroyBank: takes the bank's slots out of the lists of the wave archives that it uses
void SND_DestroyBank(SNDBankData *bank) {
    int i;

    SNDi_LockMutex();
    for (i = 0; i < SND_BANK_TO_WAVEARC_MAX; i++) {
        SNDWaveArc *waveArc = bank->waveArcLink[i].waveArc;

        if (waveArc != NULL) {
            SNDWaveArcLink *prev = waveArc->topLink;

            if (&bank->waveArcLink[i] == prev) {
                waveArc->topLink = bank->waveArcLink[i].next;
                DC_StoreRange(waveArc, sizeof(SNDWaveArc));
            } else {
                while (prev != NULL) {
                    if (&bank->waveArcLink[i] == prev->next) {
                        break;
                    }
                    prev = prev->next;
                }
                prev->next = bank->waveArcLink[i].next;
                DC_StoreRange(prev, sizeof(SNDWaveArcLink));
            }
        }
    }
    SNDi_UnlockMutex();
}

// NitroSDK's SND_DestroyWaveArc: empties the slots of the banks that use the archive
void SND_DestroyWaveArc(SNDWaveArc *waveArc) {
    SNDWaveArcLink *link;

    SNDi_LockMutex();
    link = waveArc->topLink;
    while (link != NULL) {
        SNDWaveArcLink *next = link->next;

        link->waveArc = NULL;
        link->next = NULL;
        DC_StoreRange(link, sizeof(SNDWaveArcLink));
        link = next;
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
// instruments one by one, the other programs one. FALSE when the bank has no more. The code jumps to the test of
// the loops over the drum set and the key split, which return at the first pass
BOOL SND_GetNextInstData(const SNDBankData *bank, SNDInstData *inst, SNDInstPos *pos) {
    for (; pos->prgNo < bank->instCount; pos->prgNo++, pos->index = 0) {
        u32 offset = bank->instOffset[pos->prgNo];

        inst->type = offset;
        offset >>= 8;

        switch (inst->type) {
        case SND_INST_PCM:
        case SND_INST_PSG:
        case SND_INST_NOISE:
        case SND_INST_DIRECTPCM:
        case SND_INST_NULL: {
            const SNDInstParam *param = (const SNDInstParam *)((const u8 *)bank + offset);

            inst->param = *param;
            pos->prgNo++;
            return TRUE;
        }
        case SND_INST_DRUM_SET: {
            const SNDDrumSet *drumSet = (const SNDDrumSet *)((const u8 *)bank + offset);

            while (pos->index < drumSet->max - drumSet->min + 1) {
                *inst = drumSet->instOffset[pos->index];
                pos->index++;
                return TRUE;
            }
            break;
        }
        case SND_INST_KEY_SPLIT: {
            const SNDKeySplit *keySplit = (const SNDKeySplit *)((const u8 *)bank + offset);

            while (pos->index < 8) {
                if (keySplit->key[pos->index] == 0) {
                    break;
                }
                *inst = keySplit->instOffset[pos->index];
                pos->index++;
                return TRUE;
            }
            break;
        }
        }
    }
    return FALSE;
}

// NitroSDK's SND_GetWaveDataCount: the number of waves in the archive
u32 SND_GetWaveDataCount(const SNDWaveArc *waveArc) {
    return waveArc->waveCount;
}

// NitroSDK's SND_SetWaveDataAddress: the address that a wave of the archive is loaded at
void SND_SetWaveDataAddress(SNDWaveArc *waveArc, int index, const SNDWaveData *address) {
    SNDi_LockMutex();
    waveArc->waveOffset[index] = (u32)address;
    DC_StoreRange(&waveArc->waveOffset[index], sizeof(u32));
    SNDi_UnlockMutex();
}

// NitroSDK's SND_GetWaveDataAddress: the address of a wave of the archive. An offset in the table is from the archive,
// an address (in main memory, so from 0x02000000 on) is itself, and 0 is a wave that isn't loaded
const SNDWaveData *SND_GetWaveDataAddress(const SNDWaveArc *waveArc, int index) {
    const SNDWaveData *wave;
    u32 offset;

    SNDi_LockMutex();
    offset = waveArc->waveOffset[index];
    if (offset != 0) {
        wave = (const SNDWaveData *)offset;
        if (offset < 0x02000000) {
            wave = (const SNDWaveData *)((const u8 *)waveArc + offset);
        }
    } else {
        wave = NULL;
    }
    SNDi_UnlockMutex();
    return wave;
}
