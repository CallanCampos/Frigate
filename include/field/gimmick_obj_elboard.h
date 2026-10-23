#ifndef POKEBW2_FIELD_GIMMICK_OBJ_ELBOARD_H
#define POKEBW2_FIELD_GIMMICK_OBJ_ELBOARD_H

#include "types.h"
#include "gfl/g3d.h"
#include "struct_decls.h"
#include "system/game_event.h"

// The electric news board in the gates of field_gimmick_gate.c

struct ElboardInit {
    u16 heapId;
    // The most messages the board holds
    u8 capacity;
    u8 unk03;
    u8 unk04;
    G3DActor *actor;
};

struct ElboardMessageArg {
    u16 kind;
    u16 padding;
    const char *name;
    const char *plName;
    u32 messageArc;
    u32 messageFile;
    u32 messageId;
    WordSet *wordSet;
};

GameEvent *GimmickGateEvent_Create(GameSystem *gsys, Field *field, u32 duration);
GameEventReturnCode GimmickGateEvent_ViewBoard(GameEvent *event, u32 *state, void *data);
BOOL GimmickGateBoardEntry_Load(GimmickGateBoardEntry *entry, u32 arc, u32 index);
BOOL GimmickGateBoardEntry_MatchesZone(GimmickGateBoardEntry *entry, u16 zone);
BOOL GimmickGateBoardEntry_MatchesVersion(GimmickGateBoardEntry *entry);
void Elboard_Free(Elboard *state);
void Elboard_Update(Elboard *state, u32 value);
s32 Elboard_GetTime(Elboard *state);
u16 Elboard_GetHeapID(Elboard *state);
void Elboard_SetTime(Elboard *state, fx32 time);
void Elboard_AddMessage(Elboard *state, const ElboardMessageArg *arg);
u8 Elboard_GetMessageCount(Elboard *state);
ElboardMessage *ElboardMessage_Create(Elboard *state, const ElboardMessageArg *arg, u32 index);
void ElboardMessage_Free(ElboardMessage *message);
void ElboardMessage_Start(ElboardMessage *message);
void Elboard_UpdateMessage(Elboard *state, ElboardMessage *message, u32 amount);
void ElboardMessage_Stop(ElboardMessage *message);
BOOL GimmickGateZoneData_Load(GimmickGateZoneData *dest, u32 arcId, u32 fileId);
Elboard *Elboard_Create(ElboardInit *init);

#endif // POKEBW2_FIELD_GIMMICK_OBJ_ELBOARD_H
