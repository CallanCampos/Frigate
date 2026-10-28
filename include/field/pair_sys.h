#ifndef POKEBW2_FIELD_PAIR_SYS_H
#define POKEBW2_FIELD_PAIR_SYS_H

// Overlay 12's pair_sys.c: the NPC who follows the player, such as an ally on a route. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// What GameData keeps of the follower (GetFieldFollowerCfg)
struct FieldFollowWk {
    u16 heapId;
    // The follower's index, 0xff for none
    u8 index;
    // The actor ID the follower had before it followed
    u8 originActorId;
    u16 objCode : 15;
    // Whether the follower stays after a map change
    u16 isPersistent : 1;
    u16 scrId;
    // The trainer who battles alongside the player
    u16 trainerId;
};

FieldFollowWk *InitFieldFollowWk(HeapID heapId);
void FreeFieldFollowWk(FieldFollowWk *wk);
// Makes the follower again after a map change, behind the player when behind is 1
void TryRespawnFollowActor(GameData *gameData, u32 behind);
void updateFollowerModel(GameData *gameData);
u16 GetNowFollowerAllyTrID(GameData *gameData);

// The follower's script commands
BOOL Scrcmd_PairSys_ActorPairSet(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PairSys_ActorPairEnd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PairSys_ActorPairGetTrID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PairSys_ActorPairSetMoveEnable(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_PAIR_SYS_H
