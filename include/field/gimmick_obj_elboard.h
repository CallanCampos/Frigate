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

GameEvent *func_ov104_021f02fc(GameSystem *gsys, Field *field, u32 duration);
GameEventReturnCode func_ov104_021f0160(GameEvent *event, u32 *state, void *data);
BOOL func_ov104_021f0324(GimmickGateBoardEntry *entry, u32 arc, u32 index);
BOOL func_ov104_021f0334(GimmickGateBoardEntry *entry, u16 zone);
BOOL func_ov104_021f037c(GimmickGateBoardEntry *entry);
void func_ov104_021efc8c(Elboard *state);
void func_ov104_021efcc4(Elboard *state, u32 value);
s32 func_ov104_021efcf0(Elboard *state);
u16 func_ov104_021efcf4(Elboard *state);
void func_ov104_021efcfc(Elboard *state, fx32 time);
void func_ov104_021efc6c(Elboard *state, const ElboardMessageArg *arg);
u8 func_ov104_021efcf8(Elboard *state);
ElboardMessage *func_ov104_021efe88(Elboard *state, const ElboardMessageArg *arg, u32 index);
void func_ov104_021f0080(ElboardMessage *message);
void func_ov104_021f0094(ElboardMessage *message);
void func_ov104_021f00bc(Elboard *state, ElboardMessage *message, u32 amount);
void func_ov104_021f0130(ElboardMessage *message);
BOOL func_ov104_021f0150(void *dest, u32 arcId, u32 fileId);
Elboard *func_ov104_021efbd8(ElboardInit *init);

#endif // POKEBW2_FIELD_GIMMICK_OBJ_ELBOARD_H
