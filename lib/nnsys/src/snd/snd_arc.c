#include "nitro/mi.h"
#include "snd_internal.h"

// NitroSystem's sound archive (NNS_SndArc): opening one, as a file of the ROM or wholly in memory, and finding the
// info of its sequences, banks, wave archives, streams, players and groups and its files' places. swan names
// NNS_SndArcGetSeqInfo, NNS_SndArcGetSeqArcInfo, NNS_SndArcGetBankInfo, NNS_SndArcGetWaveArcInfo and
// NNS_SndArcGetFileAddress; the other names are NitroSystem's by their code, and the file name is a guess

static BOOL SetupArc(NNSSndArc *arc, NNSSndHeapHandle heap, BOOL symbolLoadFlag);
static void InfoDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void FatDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);
static void SymbolDisposeCallback(void *mem, u32 size, u32 data1, u32 data2);

static NNSSndArc *sCurrent;

// An offset from base, NULL for 0
static inline void *GetAddress(const void *base, u32 offset) {
    return offset == 0 ? NULL : (u8 *)base + offset;
}

BOOL NNS_SndArcInit(NNSSndArc *arc, const char *filePath, NNSSndHeapHandle heap, BOOL symbolLoadFlag) {
    arc->info = NULL;
    arc->fat = NULL;
    arc->symbol = NULL;
    arc->loadBlockSize = 0;

    if (!FS_ConvertPathToFileID(&arc->fileId, filePath)) {
        return FALSE;
    }

    FS_InitFile(&arc->file);
    if (!FS_OpenFileFast(&arc->file, arc->fileId)) {
        return FALSE;
    }
    arc->file_open = TRUE;

    if (!SetupArc(arc, heap, symbolLoadFlag)) {
        return FALSE;
    }

    sCurrent = arc;
    return TRUE;
}

// Reads the header, and with a heap the info, the file table and, if symbolLoadFlag, the symbols
static BOOL SetupArc(NNSSndArc *arc, NNSSndHeapHandle heap, BOOL symbolLoadFlag) {
    s32 readSize;

    if (!FS_SeekFile(&arc->file, 0, FS_SEEK_SET)) {
        return FALSE;
    }
    readSize = FS_ReadFile(&arc->file, &arc->header, sizeof(arc->header));
    if (readSize != sizeof(arc->header)) {
        return FALSE;
    }

    if (heap != NNS_SND_HEAP_INVALID_HANDLE) {
        arc->info = NNS_SndHeapAlloc(heap, arc->header.infoSize, InfoDisposeCallback, (u32)arc, 0);
        if (arc->info == NULL) {
            return FALSE;
        }
        if (!FS_SeekFile(&arc->file, arc->header.infoOffset, FS_SEEK_SET)) {
            return FALSE;
        }
        readSize = FS_ReadFile(&arc->file, arc->info, arc->header.infoSize);
        if (readSize != arc->header.infoSize) {
            return FALSE;
        }

        arc->fat = NNS_SndHeapAlloc(heap, arc->header.fatSize, FatDisposeCallback, (u32)arc, 0);
        if (arc->fat == NULL) {
            return FALSE;
        }
        if (!FS_SeekFile(&arc->file, arc->header.fatOffset, FS_SEEK_SET)) {
            return FALSE;
        }
        readSize = FS_ReadFile(&arc->file, arc->fat, arc->header.fatSize);
        if (readSize != arc->header.fatSize) {
            return FALSE;
        }

        if (symbolLoadFlag && arc->header.symbolDataSize != 0) {
            arc->symbol = NNS_SndHeapAlloc(heap, arc->header.symbolDataSize, SymbolDisposeCallback, (u32)arc, 0);
            if (arc->symbol == NULL) {
                return FALSE;
            }
            if (!FS_SeekFile(&arc->file, arc->header.symbolDataOffset, FS_SEEK_SET)) {
                return FALSE;
            }
            readSize = FS_ReadFile(&arc->file, arc->symbol, arc->header.symbolDataSize);
            if (readSize != arc->header.symbolDataSize) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

void NNS_SndArcInitOnMemory(NNSSndArc *arc, void *data) {
    u32 i;

    MIi_CpuCopy32(data, arc, sizeof(arc->header));

    arc->info = GetAddress(data, arc->header.infoOffset);
    arc->fat = GetAddress(data, arc->header.fatOffset);
    arc->symbol = GetAddress(data, arc->header.symbolDataOffset);
    arc->loadBlockSize = 0;

    for (i = 0; i < arc->fat->count; i++) {
        NNSSndArcFileInfo *file = &arc->fat->files[i];

        file->mem = GetAddress(data, file->offset);
    }

    arc->file_open = FALSE;
    sCurrent = arc;
}

NNSSndArc *NNS_SndArcSetCurrent(NNSSndArc *arc) {
    NNSSndArc *old = sCurrent;

    sCurrent = arc;
    return old;
}

NNSSndArc *NNS_SndArcGetCurrent(void) {
    return sCurrent;
}

const NNSSndSeqParam *NNS_SndArcGetSeqParam(int seqNo) {
    const NNSSndArcSeqInfo *info = NNS_SndArcGetSeqInfo(seqNo);

    if (info == NULL) {
        return NULL;
    }
    return &info->param;
}

// The entry no of table, one of the tables of arc's info
static inline const void *GetInfo(const NNSSndArc *arc, const NNSSndArcOffsetTable *table, int no) {
    if (table == NULL) {
        return NULL;
    }
    if (no < 0) {
        return NULL;
    }
    if (no >= table->count) {
        return NULL;
    }
    return GetAddress(arc->info, table->offset[no]);
}

const NNSSndArcSeqInfo *NNS_SndArcGetSeqInfo(int seqNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->seqOffset), seqNo);
}

const NNSSndArcSeqArcInfo *NNS_SndArcGetSeqArcInfo(int seqArcNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->seqArcOffset), seqArcNo);
}

const NNSSndArcBankInfo *NNS_SndArcGetBankInfo(int bankNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->bankOffset), bankNo);
}

const NNSSndArcWaveArcInfo *NNS_SndArcGetWaveArcInfo(int waveArcNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->waveArcOffset), waveArcNo);
}

const NNSSndArcStrmInfo *NNS_SndArcGetStrmInfo(int strmNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->strmOffset), strmNo);
}

const NNSSndArcPlayerInfo *NNS_SndArcGetPlayerInfo(int playerNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->playerOffset), playerNo);
}

const NNSSndArcStrmPlayerInfo *NNS_SndArcGetStrmPlayerInfo(int playerNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->strmPlayerOffset), playerNo);
}

const NNSSndArcGroupInfo *NNS_SndArcGetGroupInfo(int groupNo) {
    return GetInfo(sCurrent, GetAddress(sCurrent->info, sCurrent->info->groupOffset), groupNo);
}

u32 NNS_SndArcGetFileOffset(u32 fileId) {
    const NNSSndArcFat *fat = sCurrent->fat;

    if (fileId >= fat->count) {
        return 0;
    }
    return fat->files[fileId].offset;
}

u32 NNS_SndArcGetFileSize(u32 fileId) {
    const NNSSndArcFat *fat = sCurrent->fat;

    if (fileId >= fat->count) {
        return 0;
    }
    return fat->files[fileId].size;
}

// Reads at most size bytes from offset in the file, loadBlockSize bytes at a time
int NNS_SndArcReadFile(u32 fileId, void *buffer, int size, int offset) {
    NNSSndArc *arc = sCurrent;
    const NNSSndArcFileInfo *fileInfo;
    int blockSize;
    int readSize;

    if (fileId >= arc->fat->count) {
        return -1;
    }
    fileInfo = &arc->fat->files[fileId];

    blockSize = arc->loadBlockSize;
    if (blockSize == 0) {
        blockSize = size;
    }

    readSize = 0;
    while (readSize < size) {
        int len = size - readSize;
        s32 result;

        if (len > blockSize) {
            len = blockSize;
        }
        if (len > fileInfo->size - offset) {
            len = fileInfo->size - offset;
        }
        if (len == 0) {
            break;
        }

        if (!FS_SeekFile(&arc->file, fileInfo->offset + offset, FS_SEEK_SET)) {
            return -1;
        }
        result = FS_ReadFile(&arc->file, buffer, len);
        if (result < 0) {
            return result;
        }

        readSize += result;
        offset += result;
        buffer = (u8 *)buffer + result;
    }

    return readSize;
}

FSFileID NNSi_SndArcGetFileID(void) {
    return sCurrent->fileId;
}

const char *NNSi_SndArcGetFilePath(void) {
    return sCurrent->filePath;
}

const void *NNS_SndArcGetFileAddress(u32 fileId) {
    const NNSSndArcFat *fat = sCurrent->fat;

    if (fileId >= fat->count) {
        return NULL;
    }
    return fat->files[fileId].mem;
}

void NNS_SndArcSetFileAddress(u32 fileId, const void *address) {
    sCurrent->fat->files[fileId].mem = address;
}

void NNS_SndArcSetLoadBlockSize(int size) {
    sCurrent->loadBlockSize = size;
}

static void InfoDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    NNSSndArc *arc = (NNSSndArc *)data1;

    arc->info = NULL;
}

static void FatDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    NNSSndArc *arc = (NNSSndArc *)data1;

    arc->fat = NULL;
}

static void SymbolDisposeCallback(void *mem, u32 size, u32 data1, u32 data2) {
    NNSSndArc *arc = (NNSSndArc *)data1;

    arc->symbol = NULL;
}
