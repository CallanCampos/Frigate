#ifndef POKEBW2_BATTLE_BTL_FIELD_H
#define POKEBW2_BATTLE_BTL_FIELD_H

// Overlay 167's btl_field.c (named by its string): the weather and the effects of the whole field, such as Trick Room
// and Gravity. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The battle in progress, whose field status btl_field.c keeps
void FieldStatusInit(u32 weather);
u8 GetFieldWeather(void);
u32 FieldStatusGetWeatherTurns(void);
void FieldStatusSetWeather(u8 weather, u8 duration);
u8 FieldStatusDecWeatherTurns(void);
BOOL FieldStatusAddEffect(u32 effect, BattleCondition value);
BOOL FieldStatusRemoveEffect(u32 effect);
BOOL FieldStatusAddDependPoke(u32 effect, u8 monId);
void FieldStatusRemoveDependPoke(u8 monId);
BOOL FieldStatusCheckImprison(BtlPokeCon *pokeCon, BattleMon *mon, u16 move);
void FieldStatusTurnCheck(void (*callback)(u32 effect, BtlServerFlow *flow), BtlServerFlow *flow);
u32 IsFieldEffectActive(u32 fieldEffect);

// A field status of the main module's, which the clients keep up to date
BtlField *BtlField_Create(HeapID heapId);
void BtlField_Delete(BtlField *field);
u8 BtlField_GetWeather(BtlField *field);
void BtlField_SetWeather(BtlField *field, u8 weather, u16 turns);
void BtlField_ClearWeather(BtlField *field);
BOOL FieldStatusaddEffectCore(BtlField *field, unsigned int effect, BattleCondition value, BOOL addEvent);
BOOL BtlField_RemoveEffect(BtlField *field, u32 effect);
BOOL BtlField_AddDependPoke(BtlField *field, u32 effect, u8 monId);
void BtlField_RemoveDependPoke(BtlField *field, u8 monId);
BOOL CheckImprison(BtlField *field, BtlPokeCon *pokeCon, BattleMon *mon, u16 move);
void BtlField_TurnCheck(BtlField *field, void (*callback)(u32 effect, BtlServerFlow *flow), BtlServerFlow *flow);
BOOL CheckFieldEffect(BtlField *field, u32 effect);

// btl_server_flow.c's
u8 GetWeather(BtlServerFlow *serverFlow);

#endif // POKEBW2_BATTLE_BTL_FIELD_H
