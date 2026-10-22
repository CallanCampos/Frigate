#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/snd.h"
#include "snd_internal.h"

// NitroSystem's loading of the sound archive's files into a sound heap (NNS_SndArcLoad): a group, a sequence with its
// bank and the bank's wave archives, a sequence archive or a wave archive. Each file is loaded once, then found again
// through the archive's file table, and its heap block's dispose callback clears it from the table and stops what
// plays from it. A wave archive loaded wave by wave keeps a table of its waves' addresses and loads only the waves its
// banks use. The file name is a guess, and so are the statics' names

// The kinds of file in a group
enum {
    SNDARC_TYPE_SEQ,
    SNDARC_TYPE_BANK,
    SNDARC_TYPE_WAVEARC,
    SNDARC_TYPE_SEQARC,
};

// A wave archive whose waves are loaded one by one
#define NNS_SND_ARC_WAVEARC_SINGLE_LOAD (1 << 0)

// Loads also leave room for the heap's alignment
#define LOAD_PADDING 32

static NNSiSndArcLoadResult LoadGroup(int groupNo, NNSSndHeapHandle heap);
static NNSiSndArcLoadResult LoadSeqArcByNo(int seqArcNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                           const void **pData);
static NNSiSndArcLoadResult LoadWaveArcByNo(int waveArcNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                            SNDWaveArc **pData);
static void *LoadFile(u32 fileId, NNSSndHeapDisposeCallback callback, u32 data1, u32 data2, NNSSndHeapHandle heap);
static void *LoadSeq(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr);
static void *LoadSeqArc(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr);
static void *LoadBank(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr);
static void *LoadWaveArc(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr);
static void *LoadWaveArcTable(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr);
static void ClearFileAddress(void *mem, NNSSndArc *arc, u32 fileId);
static void SeqDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void BankDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void WaveArcDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void WaveArcTableDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void SingleWaveDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static BOOL LoadSingleWave(SNDWaveArc *waveArc, int index, u32 fileId, NNSSndHeapHandle heap);
static BOOL LoadWavesOfBank(SNDWaveArc *waveArc, const SNDBankData *bank, int index, u32 fileId, NNSSndHeapHandle heap);

// Where a wave archive loaded wave by wave reads its head, to learn the size of its tables
static SNDWaveArc sWaveArcHeader;

BOOL NNS_SndArcLoadGroup(int groupNo, NNSSndHeapHandle heap) {
    return LoadGroup(groupNo, heap) == NNSi_SND_ARC_LOAD_SUCCESS;
}

BOOL NNS_SndArcLoadSeq(int seqNo, NNSSndHeapHandle heap) {
    return NNSi_SndArcLoadSeq(seqNo, 0xff, heap, TRUE, NULL) == NNSi_SND_ARC_LOAD_SUCCESS;
}

BOOL NNS_SndArcLoadWaveArc(int waveArcNo, NNSSndHeapHandle heap) {
    return LoadWaveArcByNo(waveArcNo, 0xff, heap, TRUE, NULL) == NNSi_SND_ARC_LOAD_SUCCESS;
}

BOOL NNS_SndArcLoadSeqEx(int seqNo, u32 loadFlag, NNSSndHeapHandle heap) {
    return NNSi_SndArcLoadSeq(seqNo, loadFlag, heap, TRUE, NULL) == NNSi_SND_ARC_LOAD_SUCCESS;
}

static NNSiSndArcLoadResult LoadGroup(int groupNo, NNSSndHeapHandle heap) {
    const NNSSndArcGroupInfo *groupInfo = NNS_SndArcGetGroupInfo(groupNo);
    const NNSSndArcGroupItem *item;
    NNSiSndArcLoadResult result;
    u32 i;

    if (groupInfo == NULL) {
        return NNSi_SND_ARC_LOAD_ERROR_INVALID_GROUP_NO;
    }
    for (i = 0; i < groupInfo->count; i++) {
        item = &groupInfo->item[i];
        switch (item->type) {
        case SNDARC_TYPE_SEQ:
            result = NNSi_SndArcLoadSeq(item->index, item->loadFlag, heap, TRUE, NULL);
            if (result != NNSi_SND_ARC_LOAD_SUCCESS) {
                return result;
            }
            break;
        case SNDARC_TYPE_SEQARC:
            result = LoadSeqArcByNo(item->index, item->loadFlag, heap, TRUE, NULL);
            if (result != NNSi_SND_ARC_LOAD_SUCCESS) {
                return result;
            }
            break;
        case SNDARC_TYPE_BANK:
            result = NNSi_SndArcLoadBank(item->index, item->loadFlag, heap, TRUE, NULL);
            if (result != NNSi_SND_ARC_LOAD_SUCCESS) {
                return result;
            }
            break;
        case SNDARC_TYPE_WAVEARC:
            result = LoadWaveArcByNo(item->index, item->loadFlag, heap, TRUE, NULL);
            if (result != NNSi_SND_ARC_LOAD_SUCCESS) {
                return result;
            }
            break;
        }
    }
    return NNSi_SND_ARC_LOAD_SUCCESS;
}

NNSiSndArcLoadResult NNSi_SndArcLoadSeq(int seqNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                        const SNDSequenceData **pData) {
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(seqNo);
    const SNDSequenceData *seq;
    NNSiSndArcLoadResult result;

    if (info == NULL) {
        return NNSi_SND_ARC_LOAD_ERROR_INVALID_SEQ_NO;
    }
    result = NNSi_SndArcLoadBank(info->param.bankNo, loadFlag, heap, bSetAddr, NULL);
    if (result != NNSi_SND_ARC_LOAD_SUCCESS) {
        return result;
    }
    if (loadFlag & NNS_SND_ARC_LOAD_SEQ) {
        seq = LoadSeq(info->fileId, heap, bSetAddr);
        if (seq == NULL) {
            return NNSi_SND_ARC_LOAD_ERROR_FAILED_LOAD_SEQ;
        }
    } else {
        seq = NNS_SndArcGetFileAddress(info->fileId);
    }
    if (pData != NULL) {
        *pData = seq;
    }
    return NNSi_SND_ARC_LOAD_SUCCESS;
}

static NNSiSndArcLoadResult LoadSeqArcByNo(int seqArcNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                           const void **pData) {
    const NNSSndArcSeqArcInfo *info = NNS_SndArcGetSeqArcInfo(seqArcNo);
    const void *seqArc;

    if (info == NULL) {
        return NNSi_SND_ARC_LOAD_ERROR_INVALID_SEQARC_NO;
    }
    if (loadFlag & NNS_SND_ARC_LOAD_SEQARC) {
        seqArc = LoadSeqArc(info->fileId, heap, bSetAddr);
        if (seqArc == NULL) {
            return NNSi_SND_ARC_LOAD_ERROR_FAILED_LOAD_SEQARC;
        }
    } else {
        seqArc = NNS_SndArcGetFileAddress(info->fileId);
    }
    if (pData != NULL) {
        *pData = seqArc;
    }
    return NNSi_SND_ARC_LOAD_SUCCESS;
}

NNSiSndArcLoadResult NNSi_SndArcLoadBank(int bankNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                         SNDBankData **pData) {
    const NNSSndArcBankInfo *info = NNS_SndArcGetBankInfo(bankNo);
    const NNSSndArcWaveArcInfo *waveArcInfo;
    SNDBankData *bank;
    SNDWaveArc *waveArc;
    NNSiSndArcLoadResult result;
    int i;

    if (info == NULL) {
        return NNSi_SND_ARC_LOAD_ERROR_INVALID_BANK_NO;
    }
    if (loadFlag & NNS_SND_ARC_LOAD_BANK) {
        bank = LoadBank(info->fileId, heap, bSetAddr);
        if (bank == NULL) {
            return NNSi_SND_ARC_LOAD_ERROR_FAILED_LOAD_BANK;
        }
    } else {
        bank = (SNDBankData *)NNS_SndArcGetFileAddress(info->fileId);
    }
    for (i = 0; i < NNS_SND_ARC_BANK_TO_WAVEARC_NUM; i++) {
        if (info->waveArcNo[i] == NNS_SND_ARC_INVALID_WAVEARC_NO) {
            continue;
        }
        waveArcInfo = NNS_SndArcGetWaveArcInfo(info->waveArcNo[i]);
        if (waveArcInfo == NULL) {
            return NNSi_SND_ARC_LOAD_ERROR_INVALID_WAVEARC_NO;
        }
        result = LoadWaveArcByNo(info->waveArcNo[i], loadFlag, heap, bSetAddr, &waveArc);
        if (result != NNSi_SND_ARC_LOAD_SUCCESS) {
            return result;
        }
        if ((waveArcInfo->flags & NNS_SND_ARC_WAVEARC_SINGLE_LOAD) && (loadFlag & NNS_SND_ARC_LOAD_WAVE)) {
            if (!LoadWavesOfBank(waveArc, bank, i, waveArcInfo->fileId, heap)) {
                return NNSi_SND_ARC_LOAD_ERROR_FAILED_LOAD_WAVE;
            }
        }
        if (bank != NULL && waveArc != NULL) {
            SND_AssignWaveArc(bank, i, waveArc);
        }
    }
    if (pData != NULL) {
        *pData = bank;
    }
    return NNSi_SND_ARC_LOAD_SUCCESS;
}

static NNSiSndArcLoadResult LoadWaveArcByNo(int waveArcNo, u32 loadFlag, NNSSndHeapHandle heap, BOOL bSetAddr,
                                            SNDWaveArc **pData) {
    const NNSSndArcWaveArcInfo *info = NNS_SndArcGetWaveArcInfo(waveArcNo);
    SNDWaveArc *waveArc;

    if (info == NULL) {
        return NNSi_SND_ARC_LOAD_ERROR_INVALID_WAVEARC_NO;
    }
    if (loadFlag & NNS_SND_ARC_LOAD_WAVE) {
        if (info->flags & NNS_SND_ARC_WAVEARC_SINGLE_LOAD) {
            waveArc = LoadWaveArcTable(info->fileId, heap, bSetAddr);
        } else {
            waveArc = LoadWaveArc(info->fileId, heap, bSetAddr);
        }
        if (waveArc == NULL) {
            return NNSi_SND_ARC_LOAD_ERROR_FAILED_LOAD_WAVE;
        }
    } else {
        waveArc = (SNDWaveArc *)NNS_SndArcGetFileAddress(info->fileId);
    }
    if (pData != NULL) {
        *pData = waveArc;
    }
    return NNSi_SND_ARC_LOAD_SUCCESS;
}

static void *LoadFile(u32 fileId, NNSSndHeapDisposeCallback callback, u32 data1, u32 data2, NNSSndHeapHandle heap) {
    u32 size = NNS_SndArcGetFileSize(fileId);
    void *buffer;

    if (size == 0) {
        return NULL;
    }
    if (heap == NULL) {
        return NULL;
    }
    buffer = NNS_SndHeapAlloc(heap, size + LOAD_PADDING, callback, data1, data2);
    if (buffer == NULL) {
        return NULL;
    }
    if (NNS_SndArcReadFile(fileId, buffer, size, 0) != size) {
        return NULL;
    }
    DC_StoreRange(buffer, size);
    return buffer;
}

static void *LoadSeq(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr) {
    void *seq = NNS_SndArcGetFileAddress(fileId);

    if (seq == NULL) {
        seq = LoadFile(fileId, SeqDisposeCallback, bSetAddr ? (u32)NNS_SndArcGetCurrent() : 0, fileId, heap);
        if (bSetAddr && seq != NULL) {
            NNS_SndArcSetFileAddress(fileId, seq);
        }
    }
    return seq;
}

static void *LoadSeqArc(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr) {
    void *seqArc = NNS_SndArcGetFileAddress(fileId);

    if (seqArc == NULL) {
        seqArc = LoadFile(fileId, SeqDisposeCallback, bSetAddr ? (u32)NNS_SndArcGetCurrent() : 0, fileId, heap);
        if (bSetAddr && seqArc != NULL) {
            NNS_SndArcSetFileAddress(fileId, seqArc);
        }
    }
    return seqArc;
}

static void *LoadBank(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr) {
    void *bank = NNS_SndArcGetFileAddress(fileId);

    if (bank == NULL) {
        bank = LoadFile(fileId, BankDisposeCallback, bSetAddr ? (u32)NNS_SndArcGetCurrent() : 0, fileId, heap);
        if (bSetAddr && bank != NULL) {
            NNS_SndArcSetFileAddress(fileId, bank);
        }
    }
    return bank;
}

static void *LoadWaveArc(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr) {
    void *waveArc = NNS_SndArcGetFileAddress(fileId);

    if (waveArc == NULL) {
        waveArc = LoadFile(fileId, WaveArcDisposeCallback, bSetAddr ? (u32)NNS_SndArcGetCurrent() : 0, fileId, heap);
        if (bSetAddr && waveArc != NULL) {
            NNS_SndArcSetFileAddress(fileId, waveArc);
        }
    }
    return waveArc;
}

// Loads a wave archive's head and two tables: the waves' addresses, all NULL, then their offsets in the file
static void *LoadWaveArcTable(u32 fileId, NNSSndHeapHandle heap, BOOL bSetAddr) {
    SNDWaveArc *waveArc = (SNDWaveArc *)NNS_SndArcGetFileAddress(fileId);
    u32 size;
    int tableSize;
    int result;

    if (waveArc == NULL) {
        if (NNS_SndArcReadFile(fileId, &sWaveArcHeader, sizeof(SNDWaveArc), 0) != sizeof(SNDWaveArc)) {
            return NULL;
        }
        tableSize = sWaveArcHeader.waveCount * sizeof(u32);
        size = sizeof(SNDWaveArc) + tableSize * 2;
        if (heap == NULL) {
            return NULL;
        }
        waveArc = NNS_SndHeapAlloc(heap, size + LOAD_PADDING, WaveArcTableDisposeCallback,
                                   bSetAddr ? (u32)NNS_SndArcGetCurrent() : 0, fileId);
        if (waveArc == NULL) {
            return NULL;
        }
        result = NNS_SndArcReadFile(fileId, waveArc, sizeof(SNDWaveArc) + tableSize, 0);
        if (result != (int)sizeof(SNDWaveArc) + tableSize) {
            return NULL;
        }
        MI_CpuCopy8(waveArc->waveOffset, &waveArc->waveOffset[waveArc->waveCount], tableSize);
        MI_CpuFill8(waveArc->waveOffset, 0, tableSize);
        DC_StoreRange(waveArc, size);
        if (bSetAddr) {
            NNS_SndArcSetFileAddress(fileId, waveArc);
        }
    }
    return waveArc;
}

// Clears the file from arc's file table if mem is where it was loaded
static void ClearFileAddress(void *mem, NNSSndArc *arc, u32 fileId) {
    OSIntrMode enabled;
    NNSSndArc *current;

    if (arc != NULL) {
        enabled = OS_DisableInterrupts();
        current = NNS_SndArcSetCurrent(arc);
        if (mem == NNS_SndArcGetFileAddress(fileId)) {
            NNS_SndArcSetFileAddress(fileId, NULL);
        }
        NNS_SndArcSetCurrent(current);
        OS_RestoreInterrupts(enabled);
    }
}

static void SeqDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    ClearFileAddress(mem, (NNSSndArc *)data1, data2);
    SND_InvalidateSeqData(mem, (u8 *)mem + size);
}

static void BankDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    ClearFileAddress(mem, (NNSSndArc *)data1, data2);
    SND_InvalidateBankData(mem, (u8 *)mem + size);
    SND_DestroyBank(mem);
}

static void WaveArcDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    ClearFileAddress(mem, (NNSSndArc *)data1, data2);
    SND_InvalidateWaveData(mem, (u8 *)mem + size);
    SND_DestroyWaveArc(mem);
}

static void WaveArcTableDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    ClearFileAddress(mem, (NNSSndArc *)data1, data2);
    SND_DestroyWaveArc(mem);
}

// data1 is the wave archive, data2 the wave's index
static void SingleWaveDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    if (mem == SND_GetWaveDataAddress((SNDWaveArc *)data1, (int)data2)) {
        SND_SetWaveDataAddress((SNDWaveArc *)data1, (int)data2, NULL);
    }
    SND_InvalidateWaveData(mem, (u8 *)mem + size);
}

// The offset of the wave after the one at fileOffset, where that wave ends
static inline u32 GetNextWaveOffset(const u32 *fileOffset) {
    return fileOffset[1];
}

static BOOL LoadSingleWave(SNDWaveArc *waveArc, int index, u32 fileId, NNSSndHeapHandle heap) {
    u32 waveCount;
    u32 offset;
    u32 size;
    void *wave;

    if (SND_GetWaveDataAddress(waveArc, index) != NULL) {
        return TRUE;
    }
    // The wave runs to the next one's offset, the last to the end of the file. The table's second half keeps the
    // waves' offsets in the file, since the first half is overwritten with the loaded waves
    waveCount = SND_GetWaveDataCount(waveArc);
    offset = waveArc->waveOffset[waveArc->waveCount + index];
    if (index < waveCount - 1) {
        size = GetNextWaveOffset(&waveArc->waveOffset[waveArc->waveCount + index]) - offset;
    } else {
        size = waveArc->fileHeader.fileSize - offset;
    }
    if (heap == NULL) {
        return FALSE;
    }
    wave = NNS_SndHeapAlloc(heap, size + LOAD_PADDING, SingleWaveDisposeCallback, (u32)waveArc, (u32)index);
    if (wave == NULL) {
        return FALSE;
    }
    if (NNS_SndArcReadFile(fileId, wave, size, offset) != size) {
        return FALSE;
    }
    DC_StoreRange(wave, size);
    SND_SetWaveDataAddress(waveArc, index, wave);
    return TRUE;
}

// Loads the waves of a wave archive loaded wave by wave that the bank's instruments play, index being the archive's
// place among the bank's
static BOOL LoadWavesOfBank(SNDWaveArc *waveArc, const SNDBankData *bank, int index, u32 fileId,
                            NNSSndHeapHandle heap) {
    SNDInstPos pos = SND_GetFirstInstDataPos(bank);
    SNDInstData inst;

    if (bank == NULL) {
        return FALSE;
    }
    while (SND_GetNextInstData(bank, &inst, &pos)) {
        if (inst.type == SND_INST_PCM && index == inst.param.wave[1]) {
            if (!LoadSingleWave(waveArc, inst.param.wave[0], fileId, heap)) {
                return FALSE;
            }
        }
    }
    return TRUE;
}
