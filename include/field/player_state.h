#ifndef POKEBW2_FIELD_PLAYER_STATE_H
#define POKEBW2_FIELD_PLAYER_STATE_H

#include "types.h"
#include "field/zone.h"
#include "nitro/fx.h"
#include "save/player_info.h"
#include "struct_decls.h"

// How the player is moving, which FieldPlayerState_GetExState returns. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
typedef enum {
    FLD_PLAYER_EXSTATE_NONE = 0x0,
    FLD_PLAYER_EXSTATE_CYCLING = 0x1,
    FLD_PLAYER_EXSTATE_SURF = 0x2,
    FLD_PLAYER_EXSTATE_DIVE = 0x3,
} PlayerExState;

struct PlayerState {
    u16 zoneId;
    u16 unk2;
    VecFx32 position;
    RailPosition railPos;
    u16 rotation;
    u8 unk1A;
    u8 isRail;
    u16 objCode;
    u8 unk1E[2];
    PlayerInfo playerInfo;
    u32 exState;
};

u32 FieldPlayerState_GetExState(PlayerState *playerState);
u16 FieldPlayerState_GetObjCode(PlayerState *playerState);
void FieldPlayerState_SetObjCode(PlayerState *playerState, u16 objCode);
u16 PlayerState_CalcDirection(PlayerState *playerState);
VecFx32 *PlayerState_GetWPos(PlayerState *playerState);
RailPosition *PlayerState_GetRailPos(PlayerState *playerState);
// Whether the player is on a rail, which PlayerState_SetIsRail sets
u8 func_0201753c(PlayerState *playerState);
u16 PlayerState_GetZoneID(PlayerState *playerState);
void PlayerState_SetIsRail(PlayerState *playerState, BOOL isRail);
void PlayerState_SetRailPos(PlayerState *playerState, const RailPosition *pos);
void PlayerState_SetRotation(PlayerState *playerState, u16 angle);
u16 func_0201754c(PlayerState *playerState); // the rotation that PlayerState_SetRotation sets
void PlayerState_SetRotationByDir(PlayerState *playerState, u16 dir);
void PlayerState_SetWPos(PlayerState *playerState, VecFx32 *pos);
void PlayerState_SetZoneID(PlayerState *playerState, u16 zoneId);
void SetPlayerSpecialState(PlayerState *playerState, u32 state);

#endif // POKEBW2_FIELD_PLAYER_STATE_H
