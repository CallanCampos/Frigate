#ifndef POKEBW2_FIELD_EVENT_FISHING_H
#define POKEBW2_FIELD_EVENT_FISHING_H

#include "nitro/fx.h"
#include "system/game_event.h"

struct FishingEventWork {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    EncountSystem *encountSystem;
    GameRecords *records;
    MMSys *actorSystem;
    FieldPlayer *player;
    FieldActor *actor;
    s16 gridX;
    s16 gridY;
    s16 gridZ;
    u16 unk26;
    u32 playerExState;
    FieldEffectTask *effect2C;
    FieldEffectTask *effect30;
    VecFx32 playerPos;
    VecFx32 fishingPos;
    u8 unk4C[12];
    u8 faceDirection;
    u8 isPhenomenon;
    u8 noFishing;
    u8 flag5B;
    s32 timer;
    u32 elapsed;
    BtlSetup *battleSetup;
};

GameEvent *EventFieldFishing_Create(Field *field, GameSystem *gsys);
GameEventReturnCode EventFieldFishing_Callback(GameEvent *event, u32 *state, void *data);

u32 func_ov033_021795a4(FishingEventWork *work, u32 value);
u32 func_ov033_021795bc(FishingEventWork *work, u32 value);
void func_ov033_021795e8(FishingEventWork *work);
void func_ov033_02179614(FishingEventWork *work);
void func_ov033_02179628(FishingEventWork *work);
void func_ov033_02179650(FishingEventWork *work);

#endif // POKEBW2_FIELD_EVENT_FISHING_H
