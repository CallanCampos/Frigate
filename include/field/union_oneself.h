#ifndef POKEBW2_FIELD_UNION_ONESELF_H
#define POKEBW2_FIELD_UNION_ONESELF_H

#include "types.h"
#include "field/union_comm.h"
#include "struct_decls.h"

// union_oneself.c, overlay 34, which the ROM names: the Union Room's steps on the field, for this machine (its situ)
// and for the players heard, whose actors it runs

// The steps of this machine (UnionSelf.step): a row of union_oneself.c's table, with a function to begin the step, one
// to run it and one to end it. The rows from UNION_STEP_JOIN_DRAW to UNION_STEP_JOIN_EGG are those of the activities,
// UNION_STEP_FOR_ACTIVITY(activity)
enum {
    UNION_STEP_IDLE = 0,
    UNION_STEP_RESET,
    UNION_STEP_NONE,
    UNION_STEP_COUNTER,
    UNION_STEP_GREETING,
    UNION_STEP_ASK,
    UNION_STEP_ASKED,
    UNION_STEP_PARENT,
    UNION_STEP_OFFER,
    UNION_STEP_OFFER_PARTY,
    UNION_STEP_RULE_SELECT,
    UNION_STEP_ANSWER,
    UNION_STEP_ANSWER_MEMBER,
    UNION_STEP_CARD,
    UNION_STEP_JOIN_DRAW,
    UNION_STEP_JOIN_BATTLE_0,
    UNION_STEP_JOIN_BATTLE_1,
    UNION_STEP_JOIN_BATTLE_2,
    UNION_STEP_JOIN_BATTLE_3,
    UNION_STEP_JOIN_BATTLE_4,
    UNION_STEP_JOIN_BATTLE_5,
    UNION_STEP_JOIN_BATTLE_6,
    UNION_STEP_JOIN_BATTLE_7,
    UNION_STEP_JOIN_BATTLE_8,
    UNION_STEP_JOIN_BATTLE_9,
    UNION_STEP_JOIN_BATTLE_10,
    UNION_STEP_JOIN_BATTLE_11,
    UNION_STEP_JOIN_BATTLE_12,
    UNION_STEP_JOIN_BATTLE_13,
    UNION_STEP_JOIN_BATTLE_14,
    UNION_STEP_JOIN_BATTLE_15,
    UNION_STEP_JOIN_MULTI_0,
    UNION_STEP_JOIN_MULTI_1,
    UNION_STEP_JOIN_MULTI_2,
    UNION_STEP_JOIN_MULTI_3,
    UNION_STEP_TRADE,
    UNION_STEP_JOIN_EGG,
    UNION_STEP_LEAVE_2,
    UNION_STEP_RECRUIT,
    UNION_STEP_LEAVE,
    UNION_STEP_COLOSSEUM,
    UNION_STEP_COLOSSEUM_SEND,
    UNION_STEP_COLOSSEUM_IDLE,
    UNION_STEP_COLOSSEUM_SIT,
    UNION_STEP_COLOSSEUM_STAND,
    UNION_STEP_PARTY_SELECT,
    UNION_STEP_PARTY_WAIT,
    UNION_STEP_PARTY_SEND,
    UNION_STEP_BATTLE_LIST,
    UNION_STEP_QUIT,
    UNION_STEP_COLOSSEUM_END,
    UNION_STEP_COLOSSEUM_CARD,
    UNION_STEP_END,
};

#define UNION_STEP_FOR_ACTIVITY(activity) ((activity) + 11)

// The activities that this file names; the others are a number of the menu
#define UNION_ACTIVITY_TRADE 0x18
#define UNION_ACTIVITY_EGG_TRADE 0x19
#define UNION_ACTIVITY_NONE 0x1b

// A step of an actor, for the player heard that it stands for
typedef BOOL (*UnionActorFunc)(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);

void UnionOneself_ClearActors(UnionSystem *unisys);
BOOL UnionActor_IsId(u16 id);
void UnionOneself_UpdateActors(UnionSystem *unisys, GameData *gameData, Field *field);
void UnionOneself_UpdateMarks(UnionSystem *unisys, GameData *gameData, Field *field);
BOOL UnionActor_StepAppear(UnionSystem *unisys, UnionActorSlot *slot, Field *field, u8 *seq);
void UnionOneself_CheckEntries(UnionSystem *unisys);
GameEvent *UnionOneself_CreateCommandEvent(GameSystem *gsys, Field *field);
BOOL UnionOneself_ReturnFalse(void);
u8 UnionOneself_RunStep(UnionSystem *unisys, Field *field);
void UnionOneself_SetUpdateDisabled(UnionSystem *unisys, Field *field, u8 disable);

// Functions outside this file that it calls, without their own header yet
// (main) whether the trainer ID is one in the rival data
BOOL Misc_IsMetTrainer(RivalDataSave *rival, u32 trainerId);
// (overlay 36) adds the player of a beacon heard to the visitors the field shows
void func_ov036_021b445c(UnionSystem *unisys, UnionBeaconEntry *entry, const u16 *look, u8 state);
// (overlay 36) sets the callbacks that add and remove a colosseum player to the screen the object stands for
void *func_ov036_021c3f1c(void *obj, UnionSystem *unisys, void (*add)(UnionSystem *, PlayerInfo *, const u8 *),
                          void (*remove)(UnionSystem *, int, const u8 *));

#endif // POKEBW2_FIELD_UNION_ONESELF_H
