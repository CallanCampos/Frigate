#ifndef POKEBW2_FIELD_MUS_COMM_FUNC_H
#define POKEBW2_FIELD_MUS_COMM_FUNC_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "struct_decls.h"

// The communication work of a show, whose layout only mus_comm_func.c needs
typedef struct MusCommWork MusCommWork;

// mus_comm_func.c, overlay 211, which the ROM names: the musical's communication between the players of a show

void func_ov211_021ef1e0(HeapID heapId, GameSystem *gsys, GameCommSys *comm, u16 value);
void func_ov211_021ef220(MusCommWork *comm);
void func_ov211_021ef3a0(MusCommWork *comm);
void func_ov211_021ef3b8(MusCommWork *comm);
void func_ov211_021ef3e4(MusCommWork *comm, PlayerInfo *info, BoxPkm *pkm, GameCommSys *gameComm, Ov210Work *ov210,
                         HeapID heapId);
// Sends a timing number to synchronize on, and whether the others reached it (or the connection failed)
void func_ov211_021ef988(MusCommWork *comm, u8 timing);
BOOL func_ov211_021ef99c(MusCommWork *comm, u8 timing);
void func_ov211_021f00c8(MusCommWork *comm);
// Whether the sound data has all arrived
BOOL func_ov211_021f0240(MusCommWork *comm);
PlayerInfo *func_ov211_021ef9c4(MusCommWork *comm, u8 index);
BoxPkm *func_ov211_021ef9e0(MusCommWork *comm, u8 index);
MusicalPoke *func_ov211_021f0094(MusCommWork *comm, u8 index);
BOOL func_ov211_021f03d8(MusCommWork *comm);
BOOL func_ov211_021f03e0(MusCommWork *comm);
u8 func_ov211_021f0470(MusCommWork *comm);
// The players' props on the stage: asks to use one, and whether it was sent; the prop each Pokémon uses (10 for
// none), which is then cleared; the position of the Pokémon in the limelight (4 or more for none), cleared with
// func_ov211_021f05b4; and the result of a prop's use
BOOL func_ov211_021f0460(MusCommWork *comm, u8 equip);
void func_ov211_021f0510(MusCommWork *comm, u8 pos, u8 equip);
u8 func_ov211_021f053c(MusCommWork *comm, u8 pos);
void func_ov211_021f056c(MusCommWork *comm, u8 pos);
u8 func_ov211_021f0598(MusCommWork *comm);
void func_ov211_021f05b4(MusCommWork *comm);
void func_ov211_021f05c0(MusCommWork *comm, u8 pos, u8 equip, u32 result);
u8 func_ov211_021f0488(MusCommWork *comm, u8 index);
u16 *func_ov211_021f0494(MusCommWork *comm, u8 index);
BOOL func_ov211_021f04a0(MusCommWork *comm);
BOOL func_ov211_021f04a4(MusCommWork *comm);
BOOL func_ov211_021f04ac(MusCommWork *comm);
void func_ov211_021f04b4(MusCommWork *comm, u32 a1, u32 a2);
void func_ov211_021f04d4(MusCommWork *comm);
void func_ov211_021f04e4(MusCommWork *comm, MusicalPoke *poke);
u32 func_ov211_021f04f0(MusCommWork *comm);
u32 func_ov211_021f04f8(MusCommWork *comm);
u8 func_ov211_021f0500(MusCommWork *comm, u8 index, u8 a2);
// The network init data, which overlay 174 gets as a number
const GFLNetInitData *func_ov211_021f0608(GameData *gameData);
// Its GameCommSys callbacks for GAME_COMM_NO_MUSICAL (see game_comm.c)
void *func_ov211_021ef230(u32 *seq, void *param);
BOOL func_ov211_021ef288(u32 *seq, void *param, void *work);
BOOL func_ov211_021ef378(u32 *seq, void *param, void *work);
void func_ov211_021ef394(u32 *seq, void *param, void *work);

#endif // POKEBW2_FIELD_MUS_COMM_FUNC_H
