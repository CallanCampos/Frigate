#ifndef POKEBW2_FIELD_MEDAL_H
#define POKEBW2_FIELD_MEDAL_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// A medal's entry in archive 0xeb
typedef struct {
    u8 unk00[9];
    // An index into MEDAL_TYPE_TO_CATEGORY, and the medal's field effect after 0x4e
    u8 type;
    // Whether Mr. Medal hints at the medal while it is unknown
    u8 hintable;
    u8 unk0B;
} MedalData;

void DiscoverInitialMedalsCore(MedalBox *box, HeapID heapId);
void DiscoverInitialMedals(MedalBox *box, HeapID heapId);
FieldActor *GetMrMedalActorIndex(Field *field);
u32 GetMrMedalActorUID(GameData *gameData);
u32 CountMedalsByStatus(FieldScriptEnv *env, u32 status);
u32 GetHintableMedalCount(FieldScriptEnv *env);

// The medals' script commands
BOOL s026E_MedalGetCount(VM *vm, FieldScriptEnv *env);
BOOL s0271_MedalAcknowledge(VM *vm, FieldScriptEnv *env);
BOOL s0272_MedalGetSpinTradeActor(VM *vm, FieldScriptEnv *env);
BOOL s0273_MedalGive(VM *vm, FieldScriptEnv *env);
BOOL s029F_MedalDiscover(VM *vm, FieldScriptEnv *env);
BOOL s029E_MedalGetFieldEffectID(VM *vm, FieldScriptEnv *env);
BOOL s02A0_MedalIsObtained(VM *vm, FieldScriptEnv *env);
BOOL s02A1_MedalGetMostCompleteCategory(VM *vm, FieldScriptEnv *env);
BOOL s02E1_MedalDiscoverInitial(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_MEDAL_H
