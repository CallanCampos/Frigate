#include "types.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"

// A message file starts with this header, followed by the offset of each language's block. A block is its size,
// followed by an entry for each message and then the messages
typedef struct {
    u16 langCount;
    u16 lineCount;
    u32 maxBlockSize;
    u32 unk8;
    u32 langOffsets[0];
} MsgFileHeader;

typedef struct {
    u32 offset;
    u16 length;
    u16 unk6;
} MsgEntry;

struct MsgData {
    MsgFileHeader *header;
    // The block of the language, when it is preloaded
    void *block;
    // The entry of the last message read from the archive
    MsgEntry entry;
    // Where the file is in the archive, or 0 and NULL when the file is in memory
    u32 fileOffset;
    ArcTool *arc;
    HeapID heapId;
    u8 lang;
    u8 preloaded;
};

static u32 GFL_MsgDataDecryptString(u16 *str, u32 length, u32 messageId);
static void GFL_MsgDataDecryptStrbuf(StrBuf *strbuf, u32 messageId);
static MsgFileHeader *GFL_MsgDataLoadHeader(ArcTool *arc, u32 offset, HeapID heapId);
static void GFL_MsgDataPreloadFull(MsgData *msgData);
static void GFL_MsgDataUpdateLanguage(MsgData *msgData);
static u8 GFL_MsgDataInitValidLangID(const MsgFileHeader *header);

static u8 sDefaultLangID;

MsgData *GFL_MsgSysLoadData(BOOL preload, u16 arcId, u16 fileId, HeapID heapId) {
    MsgData *msgData = GFL_HeapAllocate(heapId, sizeof(MsgData), FALSE, "msgdata.c", 104);

    msgData->heapId = heapId;
    msgData->arc = GFL_ArcSysCreateFileHandle(arcId, heapId);
    msgData->fileOffset = GFL_ArcToolGetDataOfs(msgData->arc, fileId);
    msgData->preloaded = preload;
    msgData->header = GFL_MsgDataLoadHeader(msgData->arc, msgData->fileOffset, msgData->heapId);
    msgData->lang = GFL_MsgDataInitValidLangID(msgData->header);
    if (!msgData->preloaded) {
        msgData->block = NULL;
    } else {
        msgData->block = GFL_HeapAllocate(msgData->heapId, msgData->header->maxBlockSize, FALSE, "msgdata.c", 123);
        GFL_MsgDataPreloadFull(msgData);
    }
    return msgData;
}

void GFL_MsgDataFree(MsgData *msgData) {
    if (msgData->arc != NULL) {
        GFL_HeapFree(msgData->header);
        if (msgData->block != NULL) {
            GFL_HeapFree(msgData->block);
        }
        GFL_ArcToolFree(msgData->arc);
    }
    GFL_HeapFree(msgData);
}

MsgData *GFL_MsgDataCreateFromHandle(void *file, HeapID heapId) {
    MsgData *msgData = GFL_HeapAllocate(heapId, sizeof(MsgData), FALSE, "msgdata.c", 166);

    msgData->heapId = heapId;
    msgData->arc = NULL;
    msgData->fileOffset = 0;
    msgData->preloaded = TRUE;
    msgData->header = file;
    msgData->lang = GFL_MsgDataInitValidLangID(file);
    GFL_MsgDataPreloadFull(msgData);
    return msgData;
}

void GFL_MsgDataLoadStrbuf(MsgData *msgData, u32 messageId, StrBuf *strbuf) {
    MsgEntry *entry;

    if (messageId >= msgData->header->lineCount) {
        GFL_StrBufClear(strbuf);
        return;
    }
    GFL_MsgDataUpdateLanguage(msgData);
    if (!msgData->preloaded) {
        GFL_ArcToolSeek(msgData->arc, msgData->fileOffset + msgData->header->langOffsets[msgData->lang] +
                                          (messageId * sizeof(MsgEntry) + sizeof(u32)));
        GFL_ArcToolReadRaw(msgData->arc, sizeof(MsgEntry), &msgData->entry);
        entry = &msgData->entry;
    } else {
        entry = (MsgEntry *)((u8 *)msgData->block + sizeof(u32)) + messageId;
    }
    if (!msgData->preloaded) {
        u32 size = entry->length * sizeof(u16);
        u16 *buf = GFL_HeapAllocate(HEAPID_TAIL(msgData->heapId), size, FALSE, "msgdata.c", 214);

        if (buf != NULL) {
            GFL_ArcToolSeek(msgData->arc,
                            entry->offset + (msgData->fileOffset + msgData->header->langOffsets[msgData->lang]));
            GFL_ArcToolReadRaw(msgData->arc, size, buf);
            GFL_StrBufCopyString(strbuf, buf, entry->length);
            GFL_HeapFree(buf);
        }
    } else {
        GFL_StrBufCopyString(strbuf, (u16 *)((u8 *)msgData->block + entry->offset), entry->length);
    }
    GFL_MsgDataDecryptStrbuf(strbuf, messageId);
}

static void GFL_MsgDataDecryptStrbuf(StrBuf *strbuf, u32 messageId) {
    u32 length = GFL_StrBufGetCharCount(strbuf);

    GFL_StrBufInsertTerminator(
        strbuf, GFL_MsgDataDecryptString((u16 *)GFL_StrBufGetStringPtr(strbuf), length + 1, messageId));
}

// Decrypts a message up to its terminator, and returns its length
static u32 GFL_MsgDataDecryptString(u16 *str, u32 length, u32 messageId) {
    u16 terminator = GFL_StrBufGetTerminator();
    u16 key = (messageId + 3) * 0x2983;
    u32 i;

    for (i = 0; i < length; i++) {
        *str ^= key;
        if (*str == terminator) {
            return i;
        }
        str++;
        key = ((key & 0xe000) >> 13) | (u16)(key << 3);
    }
    return length;
}



StrBuf *GFL_MsgDataLoadStrbufNew(MsgData *msgData, u32 messageId) {
    MsgEntry *entry;
    StrBuf *strbuf;

    GFL_MsgDataUpdateLanguage(msgData);
    if (!msgData->preloaded) {
        GFL_ArcToolSeek(msgData->arc, msgData->fileOffset + msgData->header->langOffsets[msgData->lang] +
                                          (messageId * sizeof(MsgEntry) + sizeof(u32)));
        GFL_ArcToolReadRaw(msgData->arc, sizeof(MsgEntry), &msgData->entry);
        entry = &msgData->entry;
    } else {
        entry = (MsgEntry *)((u8 *)msgData->block + sizeof(u32)) + messageId;
    }
    strbuf = GFL_StrBufCreate(entry->length, msgData->heapId);
    GFL_MsgDataLoadStrbuf(msgData, messageId, strbuf);
    return strbuf;
}

u32 GFL_MsgDataGetLineCount(MsgData *msgData) {
    return msgData->header->lineCount;
}

void GFL_MsgDataLoadRawStr(MsgData *msgData, u32 messageId, u16 *dest, u32 size) {
    MsgEntry *entry;
    u32 length;

    GFL_MsgDataUpdateLanguage(msgData);
    if (!msgData->preloaded) {
        GFL_ArcToolSeek(msgData->arc, msgData->fileOffset + msgData->header->langOffsets[msgData->lang] +
                                          (messageId * sizeof(MsgEntry) + sizeof(u32)));
        GFL_ArcToolReadRaw(msgData->arc, sizeof(MsgEntry), &msgData->entry);
        entry = &msgData->entry;
    } else {
        entry = (MsgEntry *)((u8 *)msgData->block + sizeof(u32)) + messageId;
    }
    length = entry->length;
    if (length > size) {
        length = size;
    }
    if (!msgData->preloaded) {
        u32 bytes = length * sizeof(u16);

        if (bytes > size * sizeof(u16)) {
            bytes = size * sizeof(u16);
        }
        GFL_ArcToolSeek(msgData->arc,
                        entry->offset + (msgData->fileOffset + msgData->header->langOffsets[msgData->lang]));
        GFL_ArcToolReadRaw(msgData->arc, bytes, dest);
    } else {
        u16 *src = (u16 *)((u8 *)msgData->block + entry->offset);
        u32 i;

        for (i = 0; i < length; i++) {
            dest[i] = src[i];
        }
    }
    length = GFL_MsgDataDecryptString(dest, length, messageId);
    dest[length] = GFL_StrBufGetTerminator();
}

static MsgFileHeader *GFL_MsgDataLoadHeader(ArcTool *arc, u32 offset, HeapID heapId) {
    u8 buf[sizeof(MsgFileHeader)];
    MsgFileHeader *header;
    MsgFileHeader *result;

    GFL_ArcToolSeek(arc, offset);
    header = (MsgFileHeader *)buf;
    GFL_ArcToolReadRaw(arc, sizeof(MsgFileHeader), header);
    result = GFL_HeapAllocate(heapId, sizeof(MsgFileHeader) + header->langCount * sizeof(u32), FALSE, "msgdata.c", 499);
    sys_memcpy(header, result, sizeof(MsgFileHeader));
    GFL_ArcToolReadRaw(arc, result->langCount * sizeof(u32), result->langOffsets);
    return result;
}

static inline u32 GFL_MsgDataGetLangOffset(const MsgData *msgData) {
    return msgData->header->langOffsets[msgData->lang];
}

static void GFL_MsgDataPreloadFull(MsgData *msgData) {
    u32 offset = GFL_MsgDataGetLangOffset(msgData);
    u32 size;

    if (msgData->arc != NULL) {
        GFL_ArcToolSeek(msgData->arc, msgData->fileOffset + offset);
        GFL_ArcToolReadRaw(msgData->arc, sizeof(u32), &size);
        GFL_ArcToolSeek(msgData->arc, msgData->fileOffset + offset);
        GFL_ArcToolReadRaw(msgData->arc, size, msgData->block);
    } else {
        msgData->block = (u8 *)msgData->header + offset;
    }
}

// Follows a change of the default language
static void GFL_MsgDataUpdateLanguage(MsgData *msgData) {
    u8 lang = GFL_MsgDataInitValidLangID(msgData->header);

    if (msgData->lang != lang) {
        msgData->lang = lang;
        if (msgData->preloaded == TRUE) {
            GFL_MsgDataPreloadFull(msgData);
        }
    }
}

// The default language, or the first if the file does not have it
static u8 GFL_MsgDataInitValidLangID(const MsgFileHeader *header) {
    u8 lang = GFL_MsgDataGetDefaultLangID();

    if (lang >= header->langCount) {
        lang = 0;
    }
    return lang;
}

void GFL_MsgDataSetDefaultLangID(u8 langId) {
    sDefaultLangID = langId;
}

u8 GFL_MsgDataGetDefaultLangID(void) {
    return sDefaultLangID;
}
