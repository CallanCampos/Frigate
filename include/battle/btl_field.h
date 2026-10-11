#ifndef POKEBW2_BATTLE_BTL_FIELD_H
#define POKEBW2_BATTLE_BTL_FIELD_H

// Overlay 167's btl_field.c (named by its string): the weather and the effects of the whole field, such as Trick Room
// and Gravity. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The battle in progress, whose field status btl_field.c keeps
void func_ov167_021d59a0(u32 weather);
u8 GetFieldWeather(void);
u32 func_ov167_021d59c0(void);
void FieldStatusSetWeather(u8 weather, u8 duration);
u8 func_ov167_021d59e4(void);
BOOL FieldStatusAddEffect(u32 effect, BattleCondition value);
BOOL FieldStatusRemoveEffect(u32 effect);
BOOL FieldStatusAddDependPoke(u32 effect, u8 monId);
void func_ov167_021d5a38(u8 monId);
BOOL func_ov167_021d5a48(BtlPokeCon *pokeCon, BattleMon *mon, u16 move);
void func_ov167_021d5a60(void (*callback)(u32 effect, BtlServerFlow *flow), BtlServerFlow *flow);
u32 IsFieldEffectActive(u32 fieldEffect);

// A field status of the main module's, which the clients keep up to date
BtlField *func_ov167_021d5a84(HeapID heapId);
void func_ov167_021d5aac(BtlField *field);
u8 func_ov167_021d5ad4(BtlField *field);
void func_ov167_021d5aec(BtlField *field, u8 weather, u16 turns);
void func_ov167_021d5af4(BtlField *field);
BOOL FieldStatusaddEffectCore(BtlField *field, u32 effect, BattleCondition value, BOOL addEvent);
BOOL func_ov167_021d5bc0(BtlField *field, u32 effect);
BOOL func_ov167_021d5c04(BtlField *field, u32 effect, u8 monId);
void func_ov167_021d5c60(BtlField *field, u8 monId);
BOOL CheckImprison(BtlField *field, BtlPokeCon *pokeCon, BattleMon *mon, u16 move);
void func_ov167_021d5da4(BtlField *field, void (*callback)(u32 effect, BtlServerFlow *flow), BtlServerFlow *flow);
BOOL CheckFieldEffect(BtlField *field, u32 effect);

// btl_server_flow.c's
u8 GetWeather(BtlServerFlow *serverFlow);

#endif // POKEBW2_BATTLE_BTL_FIELD_H
