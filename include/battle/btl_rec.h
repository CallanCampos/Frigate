#ifndef POKEBW2_BATTLE_BTL_REC_H
#define POKEBW2_BATTLE_BTL_REC_H

// Overlay 167's btl_rec.c (named by its string): the recording of a battle's actions, and its playback

#include "types.h"
#include "battle/btl_action.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The actions of a turn, as a chunk of the recording: the chunk's header in data[0] and data[1], then the clients in
// clientFlags, each with a byte (client ID in bits 5 to 7, action count in bits 0 to 4) and its actions
typedef struct {
    u8 size;
    u8 clientFlags;
    u8 numClients;
    // The chunk's type and whether it starts a chapter, for its header
    u8 type : 6;
    u8 chapter : 1;
    u8 overflow : 1;
    u8 data[0x3c];
} BtlRecTool;

void BtlRecTool_Init(BtlRecTool *tool, BOOL chapter);
void *BtlRecTool_MakeTurnChunk(BtlRecTool *tool, u32 *size, BOOL chapter);
void BtlRecTool_AddClientActions(BtlRecTool *tool, u8 clientId, const void *actions, u8 count);
void *BtlRecTool_FinishChunk(BtlRecTool *tool, u8 value, u32 *size);
void *BtlRecTool_MakeTimeoutChunk(BtlRecTool *tool, u32 *size);
void BtlRecTool_SetData(BtlRecTool *tool, const void *data, u32 size);
BOOL BtlRecTool_ReadClientActions(BtlRecTool *tool, u32 *pos, u8 *clientId, u8 *count, void *actions);

// A recording is a stream of chunks, each starting with a header byte: the count in bits 0 to 3, the chunk type in bits
// 4 to 6 (1 for the clients' actions, 2 for a turn without them, 3 for a timeout) and the start of a chapter in bit 7.
// Each client's actions in a type 1 chunk start with a byte holding the client ID in bits 5 to 7 and the action count
// in bits 0 to 4.

// The reader of a recorded battle's actions, which the main module holds
typedef struct {
    const u8 *data;
    u32 size : 31;
    // Set when the recording ran out or was corrupt; every client gets a blank action from then on
    u32 error : 1;
    // Each client's read position
    u32 pos[4];
    // Each client's actions of the last chunk read
    BattleAction actions[4][16];
} BtlRecReader;

BtlRecorder *BtlRecorder_Create(HeapID heapId, u32 type);
void BtlRecorder_Delete(BtlRecorder *recorder);
void BtlRecorder_AddData(BtlRecorder *recorder, const void *data, u32 size);
u8 BtlRec_GetCmdCheckReq(const void *data);
void *BtlRecorder_GetData(BtlRecorder *recorder, u32 *size);
void BtlRecReader_Init(BtlRecReader *reader, const void *data, u32 size);
void BtlRecReader_Reset(BtlRecReader *reader);
BOOL BtlRecReader_SkipTurnChunk(BtlRecReader *reader, u8 clientId);
BattleAction *BtlRecReader_ReadActions(BtlRecReader *reader, u8 clientId, u8 *count, u8 *chapter);
u32 BtlRecReader_CountChapters(BtlRecReader *reader);
BOOL BtlRecReader_IsReadComplete(BtlRecReader *reader, u8 clientId);

#endif // POKEBW2_BATTLE_BTL_REC_H
