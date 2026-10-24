#ifndef POKEBW2_FIELD_TRIAL_HOUSE_H
#define POKEBW2_FIELD_TRIAL_HOUSE_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "field/bsubway_scr.h"
#include "field/delivery_beacon.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct TrialHouseWork {
    BSubwayTrainer trainer;
    u16 heapId;
    u8 unk122[2];
    u32 capacity;
    PokeParty *party;
    u32 battleType;
    u32 selectionFlag;
    // The battle's statistics, which TrialHouseCalcPointScore scores
    u16 stats[12];
    BattleExaminationSave *saveBuffer;
    u32 initState;
};

struct TrialHouseEventData {
    // How the Battle Test's data is received
    DeliveryInit init;
    DeliveryBeaconWork *subwork;
    GameSystem *gsys;
    TrialHouseWork *work;
    u16 *result;
    s32 timeout;
};

struct TrialHouseEffectEvent {
    GameSystem *gsys;
    void *buffer;
    u32 mode;
    FieldSaveAnime *effect;
};

extern const char data_ov033_0217c630[];

struct TrialHouseWork *CreateTrialHouseWk(GameSystem *gsys);
void TrialHouse_LoadDeliveryState(GameSystem *gsys, struct TrialHouseWork *work);
void TrialHouseWorkDelete(void *unused, struct TrialHouseWork **work);
void TrialHouse_SetBattleMode(TrialHouseWork *work, u32 mode);
void func_ov033_0217adbc(TrialHouseWork *work, u32 selectionFlag);
u32 TrialHouse_LoadTrainer(GameSystem *gsys, TrialHouseWork *work, u32 mode);
void TrialHouse_BuildTrainerFromMode(TrialHouseWork *work, u32 mode);
void TrialHouse_LoadDeliveredTrainer(GameSystem *gsys, TrialHouseWork *work, u32 mode);
u32 TrialHouse_GetTrainerObjCode(TrialHouseWork *work);
// The Trainer's message in a balloon over the actor
GameEvent *TrialHouse_CreateTrainerMsgEvent(GameSystem *gsys, TrialHouseWork *work, u32 index, u32 actorId);
GameEvent *func_ov033_0217aee8(GameSystem *gsys, TrialHouseWork *work, u16 *result);
GameEventReturnCode TrialHouse_DeliveryTaskMain(GameEvent *event, u32 *state, void *data);
u8 TrialHouse_IsDeliveryIdReceived(TrialHouseSave *save, u32 index);
void TrialHouse_MarkDeliveryIdReceived(TrialHouseSave *save, u32 index);
void TrialHouseCalcPointScore(GameSystem *gsys, TrialHouseWork *work, u16 *rankOut, u16 *pointsOut);
u32 TrialHouse_GetDeliveryState(GameSystem *gsys, TrialHouseWork *work);
GameEvent *TrialHouse_CreateSaveTask(GameSystem *gsys, TrialHouseWork *work, u32 mode);
u32 TrialHouse_GetRecordSlotsState(GameSystem *gsys);
GameEventReturnCode TrialHouse_SaveTaskMain(GameEvent *event, u32 *state, void *data);

// Overlay 12's event_trial_house.c
// The party screen for picking the Pokémon to enter, from the party or the Battle Box; result is set to whether
// Pokémon were picked
GameEvent *CallTrialHouseTeamSelect(GameSystem *gsys, TrialHouseWork *work, u32 mode, BOOL battleBox, u16 *result);
GameEvent *CallTrialHouseBattle(GameSystem *gsys, TrialHouseWork *work);
void SyncTrialHouseWkStatsFromBattle(TrialHouseWork *work, BtlSetup *setup);
// Overlay 313's results screen
GameEvent *CallTrialHouseResults(GameSystem *gsys, u32 a1, u32 a2);

#endif // POKEBW2_FIELD_TRIAL_HOUSE_H
