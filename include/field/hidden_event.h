#ifndef POKEBW2_FIELD_HIDDEN_EVENT_H
#define POKEBW2_FIELD_HIDDEN_EVENT_H

#include "types.h"
#include "field/player_action.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct HiddenArea {
    u16 x;
    u16 z;
    u16 width;
    u16 height;
};

// What the field menu or a shortcut asks of a hidden move
struct HiddenEventArgs {
    u16 partySlot;
    u16 kind;
    // Fly's destination
    u32 value;
};

// The data of the events that call a hidden move's script
struct HiddenEventData {
    // 0x19740205
    u32 magic;
    FieldActor *actor;
    GameSystem *gsys;
    HiddenEventArgs args;
};

typedef u32 (*HiddenCheckFunc)(PlayerActionPossibilities *context);
typedef GameEvent *(*HiddenCtorFunc)(HiddenEventArgs *args, PlayerActionPossibilities *context);

typedef struct {
    HiddenCtorFunc create;
    HiddenCheckFunc check;
} HiddenEventSpec;

extern const HiddenEventSpec HIDEN_EVENTS_NORMAL[11];
extern const HiddenEventSpec HIDEN_EVENTS_RUINS[11];

void CalcPlayerActionPossibilities(Field *field, PlayerActionPossibilities *action);
HiddenCheckFunc GetHidenEventCheckFunc(PlayerActionPossibilities *context, u32 kind);
HiddenCtorFunc GetHidenEventCtorFunc(PlayerActionPossibilities *context, u32 kind);
u32 CheckAllowHidenEvent(u32 kind, PlayerActionPossibilities *context);
void func_ov012_02159418(HiddenEventArgs *args, u16 partySlot, u16 kind, u32 value);
GameEvent *CreateHidenEvent(u32 kind, HiddenEventArgs *args, PlayerActionPossibilities *context);
BOOL func_ov012_02159440(PlayerActionPossibilities *context);
u32 func_ov012_02159b5c(PlayerActionPossibilities *context, u32 value);
BOOL func_ov012_02159b70(const HiddenArea *area, u16 x, u16 z, u16 flag, Field *field);
void func_ov012_02159b40(HiddenEventData *data, HiddenEventArgs *param, PlayerActionPossibilities *context);
u32 EventCutCall_Check(PlayerActionPossibilities *context);
GameEvent *EventCutCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventCutCall_Callback(GameEvent *event, u32 *state, void *data);
u32 EventSurfCall_Check(PlayerActionPossibilities *context);
GameEvent *EventSurfCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventSurfCall_Callback(GameEvent *event, u32 *state, void *data);
u32 EventWaterfallCall_Check(PlayerActionPossibilities *context);
GameEvent *EventWaterfallCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventWaterfallCall_Callback(GameEvent *event, u32 *state, void *data);
u32 EventStrengthCall_Check(PlayerActionPossibilities *context);
GameEvent *EventStrengthCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventStrengthCall_Callback(GameEvent *event, u32 *state, void *data);
u32 func_ov012_02159644(PlayerActionPossibilities *context);
GameEvent *func_ov012_02159658(HiddenEventArgs *args, PlayerActionPossibilities *context);
GameEvent *EventFly_Create(HiddenEventArgs *args, PlayerActionPossibilities *context);
u32 EventFly_Check(PlayerActionPossibilities *context);
u32 EventFlash_Check(PlayerActionPossibilities *context);
GameEvent *EventFlash_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventFlash_Callback(GameEvent *event, u32 *state, void *data);
u32 EventDigCall_Check(PlayerActionPossibilities *context);
GameEvent *EventDigCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventDigCall_Callback(GameEvent *event, u32 *state, void *data);
u32 EventTeleportCall_Check(PlayerActionPossibilities *context);
GameEvent *EventTeleportCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventTeleportCall_Callback(GameEvent *event, u32 *state, void *data);
u32 EventDivingCall_Check(PlayerActionPossibilities *context);
GameEvent *EventDivingCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventDivingCall_Callback(GameEvent *event, u32 *state, void *data);
u32 func_ov012_02159984(PlayerActionPossibilities *context);
GameEvent *func_ov012_02159998(HiddenEventArgs *args, PlayerActionPossibilities *context);
GameEventReturnCode EventRuinsStrengthCall_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventRuinsStrengthCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventRuinsFlash_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventRuinsFlash_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);

#endif // POKEBW2_FIELD_HIDDEN_EVENT_H
