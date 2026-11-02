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
    // The emotion bubble of a bite, and the line in the water
    FieldEffectTask *bubble;
    FieldEffectTask *line;
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

// What Fishing_WaitBite returns: still waiting, A pressed, or the time up
enum {
    FISHING_WAIT_NONE,
    FISHING_WAIT_PRESSED,
    FISHING_WAIT_TIMEOUT,
};

u32 Fishing_StepCounter(FishingEventWork *work, u32 value);
u32 Fishing_WaitBite(FishingEventWork *work, u32 value);
void Fishing_ShowBite(FishingEventWork *work);
void Fishing_DeleteBubble(FishingEventWork *work);
void Fishing_StartLine(FishingEventWork *work);
void Fishing_DeleteLine(FishingEventWork *work);

#endif // POKEBW2_FIELD_EVENT_FISHING_H
