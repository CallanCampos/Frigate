#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_rec.h"
#include "gfl/heap.h"
#include "gfl/std.h"

// A recorded chunk's type, in bits 4 to 6 of its header byte
typedef enum {
    BTL_REC_CHUNK_NONE,
    // The clients' actions
    BTL_REC_CHUNK_ACTIONS,
    // A turn without actions
    BTL_REC_CHUNK_TURN,
    // A timeout
    BTL_REC_CHUNK_TIMEOUT,
} BtlRecChunkType;

// A recorded chunk's header byte
static inline u8 MakeHeader(u8 num, BtlRecChunkType type, u8 chapter) {
    return (num & 0xf) | (((chapter & 1) << 7) | ((type & 7) << 4));
}

// What a client records of the server's commands
struct BtlRecorder {
    u16 size;
    u8 overflow;
    // Not used here
    u32 unk04;
    u8 data[0xc00];
    u32 capacity;
};

BtlRecorder *BtlRecorder_Create(HeapID heapId, u32 type) {
    BtlRecorder *recorder = GFL_HeapAllocate(heapId, sizeof(BtlRecorder), TRUE, "btl_rec.c", 0x67);
    if (type == 0) {
        recorder->capacity = sizeof(recorder->data);
    } else {
        recorder->capacity = 0x164;
    }
    return recorder;
}

void BtlRecorder_Delete(BtlRecorder *recorder) {
    GFL_HeapFree(recorder);
}

void BtlRecorder_AddData(BtlRecorder *recorder, const void *data, u32 size) {
    if (!recorder->overflow) {
        u32 end = recorder->size + (size - 1);
        if (end <= recorder->capacity) {
            MI_CpuCopy8((const u8 *)data + 1, &recorder->data[recorder->size], size - 1);
            recorder->size = end;
        } else {
            recorder->overflow = TRUE;
        }
    }
}

u8 BtlRec_GetCmdCheckReq(const void *data) {
    return *(const u8 *)data;
}

void *BtlRecorder_GetData(BtlRecorder *recorder, u32 *size) {
    *size = recorder->size;
    return recorder->data;
}

void BtlRecReader_Init(BtlRecReader *reader, const void *data, u32 size) {
    u32 i;

    reader->data = data;
    reader->size = size;
    reader->error = FALSE;
    for (i = 0; i < 4; i++) {
        reader->pos[i] = 0;
    }
}

void BtlRecReader_Reset(BtlRecReader *reader) {
    u32 i;

    for (i = 0; i < 4; i++) {
        reader->pos[i] = 0;
    }
}

BOOL BtlRecReader_SkipTurnChunk(BtlRecReader *reader, u8 clientId) {
    u8 header = reader->data[reader->pos[clientId]];
    BtlRecChunkType type = (header >> 4) & 7;
    u8 chapter = (header >> 7) & 1;

    if (type == BTL_REC_CHUNK_TURN) {
        reader->pos[clientId]++;
        return chapter;
    }
    return FALSE;
}

BattleAction *BtlRecReader_ReadActions(BtlRecReader *reader, u8 clientId, u8 *count, u8 *chapter) {
    u32 *pos;
    BattleAction *actions;

    if (reader->error) {
        actions = reader->actions[clientId];
        BattleAction_SetRecError(actions);
        *count = 1;
        *chapter = 0;
        return actions;
    }

    pos = &reader->pos[clientId];
    while (*pos < reader->size) {
        u8 header = reader->data[*pos];
        u8 num = header & 0xf;
        BtlRecChunkType type = (header >> 4) & 7;
        *chapter = (header >> 7) & 1;
        (*pos)++;
        if (*pos > reader->size) {
            break;
        }
        if (type == BTL_REC_CHUNK_ACTIONS) {
            BattleAction *result = NULL;
            u32 i;
            if (num == 0 || num > 4) {
                break;
            }
            for (i = 0; i < num; i++) {
                u8 byte = reader->data[(*pos)++];
                u8 id = (byte >> 5) & 7;
                u8 n = byte & 0x1f;
                if (*pos >= reader->size) {
                    break;
                }
                if (id != clientId) {
                    *pos += n * sizeof(BattleAction);
                } else {
                    MI_CpuCopy8(&reader->data[*pos], reader->actions[clientId], n * sizeof(BattleAction));
                    result = reader->actions[clientId];
                    *pos += n * sizeof(BattleAction);
                    *count = n;
                }
            }
            if (result != NULL) {
                return result;
            }
        } else if (type == BTL_REC_CHUNK_TIMEOUT) {
            actions = reader->actions[clientId];
            BattleAction_SetRecTimeout(actions);
            *count = 1;
            *chapter = 0;
            return actions;
        } else if (type != BTL_REC_CHUNK_TURN) {
            break;
        }
        if (*pos >= reader->size) {
            break;
        }
    }

    reader->error = TRUE;
    actions = reader->actions[clientId];
    BattleAction_SetRecError(actions);
    *count = 1;
    *chapter = 0;
    return actions;
}

u32 BtlRecReader_CountChapters(BtlRecReader *reader) {
    u32 pos, chapters;

    pos = 0;
    chapters = 0;
    while (pos < reader->size) {
        u8 header = reader->data[pos++];
        u8 num = header & 0xf;
        BtlRecChunkType type = (header >> 4) & 7;
        u8 chapter = (header >> 7) & 1;
        if (chapter) {
            chapters++;
        }
        if (pos >= reader->size) {
            break;
        }
        if (type != BTL_REC_CHUNK_ACTIONS) {
            pos += num;
        } else {
            u32 i;
            for (i = 0; i < num; i++) {
                u8 n = reader->data[pos++] & 0x1f;
                pos += n * sizeof(BattleAction);
                if (pos >= reader->size) {
                    break;
                }
            }
        }
    }
    return chapters;
}

BOOL BtlRecReader_IsReadComplete(BtlRecReader *reader, u8 clientId) {
    if (!reader->error) {
        u32 pos = reader->pos[clientId];
        if (pos == reader->size) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void BtlRecTool_Init(BtlRecTool *tool, BOOL chapter) {
    MI_CpuFill8(tool, 0, sizeof(BtlRecTool));
    tool->chapter = chapter;
}

void *BtlRecTool_MakeTurnChunk(BtlRecTool *tool, u32 *size, BOOL chapter) {
    tool->data[0] = 4;
    tool->data[1] = MakeHeader(0, BTL_REC_CHUNK_TURN, chapter);
    *size = 2;
    return tool->data;
}

void BtlRecTool_AddClientActions(BtlRecTool *tool, u8 clientId, const BattleAction *actions, u8 count) {
    if (tool->size == 0) {
        tool->type = 1;
        tool->size = 2;
    }
    if (!(tool->clientFlags & (1 << clientId))) {
        u32 bytes = count * sizeof(BattleAction);
        u32 end = tool->size + (bytes + 1);
        if (end <= sizeof(tool->data)) {
            tool->clientFlags |= (1 << clientId);
            tool->numClients++;
            tool->data[tool->size] = (clientId << 5) | count;
            MI_CpuCopy8(actions, &tool->data[tool->size + 1], bytes);
            tool->size = end;
        } else {
            tool->overflow = TRUE;
        }
    }
}

void *BtlRecTool_FinishChunk(BtlRecTool *tool, u8 value, u32 *size) {
    if (!tool->overflow) {
        tool->data[0] = value;
        tool->data[1] = MakeHeader(tool->numClients, tool->type, tool->chapter);
        *size = tool->size;
        return tool->data;
    }
    return NULL;
}

void *BtlRecTool_MakeTimeoutChunk(BtlRecTool *tool, u32 *size) {
    tool->data[0] = 0;
    tool->data[1] = MakeHeader(0, BTL_REC_CHUNK_TIMEOUT, FALSE);
    *size = 2;
    return tool->data;
}

void BtlRecTool_SetData(BtlRecTool *tool, const void *data, u32 size) {
    MI_CpuCopy8(data, tool->data, size);
    tool->size = size;
    tool->clientFlags = 0;
    tool->numClients = 0;
    tool->type = 0;
    tool->chapter = 0;
    tool->overflow = 0;
}

BOOL BtlRecTool_ReadClientActions(BtlRecTool *tool, u32 *pos, u8 *clientId, u8 *count, BattleAction *actions) {
    if (*pos == 0) {
        *pos = 2;
    }
    if (*pos < tool->size) {
        u8 byte = tool->data[*pos];
        u32 bytes;
        *clientId = (byte >> 5) & 7;
        *count = byte & 0x1f;
        bytes = *count * sizeof(BattleAction);
        (*pos)++;
        MI_CpuCopy8(&tool->data[*pos], actions, bytes);
        *pos += bytes;
        return TRUE;
    }
    return FALSE;
}
