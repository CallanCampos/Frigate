#ifndef POKEBW2_BATTLE_BTL_OV169_H
#define POKEBW2_BATTLE_BTL_OV169_H

#include "types.h"
#include "struct_decls.h"

// Overlay 169's functions that the battle server's flow calls. dsd reads that overlay as data, so these are its
// addresses, which the calls reach through linker veneers

void func_ov169_06898bfc(void);
void func_ov169_0689d178(void *data);
void func_ov169_0689d2a0(void *data);
void func_ov169_0689d2bc(void *data);
void func_ov169_0689d384(void *data, BtlMainModule *mainModule, BtlPokeCon *pokeCon, u32 battleStyle);
u32 func_ov169_0689d2fc(void *data, u32 arg1);
void func_ov169_0689d4c0(void *data, u8 slot, u8 clientId, BattleMon *mon, BtlPokeCon *pokeCon);
u8 func_ov169_0689d6e0(void *data, u8 clientId, u8 *positions);
BOOL DoesBattleMonExist(void *data, u8 monId);
void func_ov169_0689d678(void *data, u8 firstPos, u8 secondPos);
void func_ov169_0689d480(void *data, u8 monId);
void func_ov169_0689ced0(void *monSet, u8 count);
void func_ov169_0689cf00(void *monSet, void *copy);
void func_ov169_0689d06c(void *monSet);
void func_ov169_0689d1a4(void *data, u16 move, u32 turn, u8 monId);
void func_ov169_0689d4a8(void *data, u8 pos, u8 monId, BtlPokeCon *pokeCon);
void func_ov169_0689c814(BtlServerFlow *flow, BattleMon *mon);
void func_ov169_0689ccc4(void *monSet);
void func_ov169_0689ccd0(void *monSet, BattleMon *mon);
void func_ov169_0689ce0c(void *monSet);
BattleMon *func_ov169_0689ce14(void *monSet);
void SortBySpeed(void *monSet, BtlServerFlow *flow);
u8 GetBattlePos(void *data, u8 monId);
BOOL func_ov169_0689ce3c(void *monSet, BattleMon *mon, u32 *value);
BOOL func_ov169_0689ce80(void *monSet, BattleMon *mon, u32 *value);
void func_ov169_0689c4ec(u32 condition, BattleCondition value, BattleMon *mon, BattleHandlerString *string);
void func_ov169_0689c92c(BtlServerFlow *flow, BattleMon *mon);
BOOL func_ov169_0689cadc(u16 ability);
void func_ov169_06898d54(void (*callback)(u32 side, u32 effect, BtlServerFlow *flow), BtlServerFlow *flow);
void func_ov169_0689b938(BattleMon *mon, u32 condition, BattleCondition prev, BOOL cured, BtlServerFlow *flow);
void func_ov169_0689ba5c(BattleHandlerString *string, BattleMon *mon, u32 condition);
void func_ov169_0689d2e4(void *data, u8 monId);
BOOL func_ov169_0689d328(void *data, u32 arg1, u8 index);
u8 func_ov169_0689d30c(void *data, u32 arg1, u8 index);
void func_ov169_0689d344(void *data, u32 arg1, u8 index);
u8 GetExistPokeID(void *data, u8 pos);
u32 func_ov169_0689cb6c(u32 index);
u32 func_ov169_0689cb80(u32 index);
u32 func_ov169_0689cec0(void *monSet);
void func_ov169_0689cf54(void *monSet, BattleMon *mon, void *out);
void func_ov169_0689cfe0(void *monSet, BattleMon *mon, void *out);
BOOL func_ov169_0689ced8(void *monSet);
void func_ov169_0689cd40(void *monSet, BattleMon *mon, u32 damage, BOOL flag);
void func_ov169_0689cd9c(void *monSet, BattleMon *mon);
BOOL func_ov169_0689d724(void *data, BtlMainModule *mainModule, u8 monId);
void func_ov169_0689d1d8(void *data);
u32 func_ov169_0689cec8(void *monSet);
BattleMon *func_ov169_0689cdf8(void *monSet, u32 index);
void Condition_CheckFloating(BtlServerFlow *flow, BattleMon *mon);
u8 func_ov169_0689d77c(void *data, u8 monId);
BOOL func_ov169_06898cf4(u8 side, u32 sideEffect);
u32 func_ov169_06898ce0(u8 side, u32 sideEffect);
BOOL func_ov169_068982ac(u32 a0);
BOOL func_ov169_06898c10(u8 side, u32 effect, BattleCondition cont);
BOOL ServerDisplay_RemoveSideEffect(u8 side, u32 effect);
BOOL PosEventAdd(u32 effect, u8 pos, u8 monId, const u32 *args, u8 argCount);
BOOL func_ov169_0689cb28(u16 background);
BOOL func_ov169_0689ca84(u16 item);
u8 func_ov169_0689d35c(void *data);
void func_ov169_0689c6c8(BtlServerFlow *flow, BattleMon *target);

BOOL func_ov169_0689cb5c(u16 move);
// Whether an item is an Arceus plate, and a Genesect drive
BOOL func_ov169_0689cb08(u16 item);
BOOL func_ov169_0689cb18(u16 item);

// Move and ability checks the move handlers use
BOOL func_ov169_0689ca34(u16 move);
BOOL func_ov169_0689ca44(u16 move);
BOOL func_ov169_0689caa4(u16 ability);
BOOL func_ov169_0689cac4(u16 ability);
BOOL func_ov169_0689cba4(u16 move);
BOOL func_ov169_0689cbc4(u16 move);
BOOL func_ov169_0689cbe4(u16 move);
// Whether Mimic can't copy a move
BOOL func_ov169_0689cc04(u16 move);
const u16 *func_ov169_0689cc14(u32 *count);
BOOL func_ov169_0689d1ec(void *data, u16 move, u32 turn);
u32 GetUsedMoveCount(void *data, u16 move, u32 turn);
u16 func_ov169_0689d250(void *data, u32 turn);

#endif // POKEBW2_BATTLE_BTL_OV169_H
