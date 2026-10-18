#ifndef POKEBW2_FIELD_COMM_PLAYER_H
#define POKEBW2_FIELD_COMM_PLAYER_H

// Overlay 12's comm_player.c: the other players of a field communication, whose actors overlay 36 shows, and the
// player's own state that is sent to them. cps, act_ctrl and COMM_PLAYER_MAX are named after the file's asserts

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

#define COMM_PLAYER_MAX 4

// Where a player is, as the players send it to each other
typedef struct {
    u32 unk0;
    VecFx32 pos;
    u16 dir;
    u8 exState;
} CommPlayerStatus;

typedef struct {
    VecFx32 pos;
    u32 unkC;
    u16 dir;
    u16 unk12;
    u8 active : 1;
    // Set while the actor is taken down for the field to close
    u8 hidden : 1;
    u8 exState : 4;
    void *actor;
} CommPlayer;

// The player's own state as it was last sent
typedef struct {
    VecFx32 pos;
    u16 dir;
    u8 exState : 4;
    u8 busy : 1;
} CommPlayerSent;

struct CommPlayerSys {
    GameSystem *gsys;
    FldCommActSys *act_ctrl;
    CommPlayer players[COMM_PLAYER_MAX];
    CommPlayerSent last;
    u32 unk88;
    u16 heapId;
    u8 unk8E;
    u8 unk8F;
    u8 unk90;
};

CommPlayerSys *func_ov012_021613d0(u8 a0, GameSystem *gsys, HeapID heapId, u8 a3);
void func_ov012_02161404(GameSystem *gsys, CommPlayerSys *cps);
void func_ov012_0216144c(GameSystem *gsys, CommPlayerSys *cps);
void func_ov012_0216148c(CommPlayerSys *cps, int index, u16 a2, const CommPlayerStatus *status);
// Takes the actors down before the field closes, and puts them back after
void func_ov012_02161544(CommPlayerSys *cps);
void func_ov012_021615a4(CommPlayerSys *cps);
BOOL func_ov012_0216165c(CommPlayerSys *cps, int index);
void func_ov012_0216168c(CommPlayerSys *cps, int index, const CommPlayerStatus *status);
// The player's own state, and whether it changed since it was last sent
BOOL func_ov012_021616b0(CommPlayerSys *cps, CommPlayerStatus *status);
// Whether another player's actor is at a grid position, with its network ID; *index is where to look from
BOOL func_ov012_021617e8(CommPlayerSys *cps, s16 x, s16 y, s16 z, u32 *netId, u32 *index);

#endif // POKEBW2_FIELD_COMM_PLAYER_H
