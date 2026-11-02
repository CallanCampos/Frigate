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

// The hidden moves' events, in the order of HM_EVENTS_NORMAL and HM_EVENTS_RUINS
enum {
    HM_EVENT_CUT,
    HM_EVENT_SURF,
    HM_EVENT_WATERFALL,
    HM_EVENT_STRENGTH,
    HM_EVENT_FLY,
    HM_EVENT_FLASH,
    HM_EVENT_TELEPORT,
    HM_EVENT_DIG,
    HM_EVENT_SWEET_SCENT,
    HM_EVENT_CHATTER,
    HM_EVENT_DIVE,
    HM_EVENT_MAX,
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

extern const HiddenEventSpec HM_EVENTS_NORMAL[HM_EVENT_MAX];
extern const HiddenEventSpec HM_EVENTS_RUINS[HM_EVENT_MAX];

void CalcPlayerActionPossibilities(Field *field, PlayerActionPossibilities *action);
HiddenCheckFunc GetHMEventCheckFunc(PlayerActionPossibilities *context, u32 kind);
HiddenCtorFunc GetHMEventCtorFunc(PlayerActionPossibilities *context, u32 kind);
u32 CheckAllowHMEvent(u32 kind, PlayerActionPossibilities *context);
void HiddenEventArgs_Set(HiddenEventArgs *args, u16 partySlot, u16 kind, u32 value);
GameEvent *CreateHMEvent(u32 kind, HiddenEventArgs *args, PlayerActionPossibilities *context);
BOOL HMEvent_CheckPairFlag(PlayerActionPossibilities *context);
u32 HMEvent_CheckContextFlag(PlayerActionPossibilities *context, u32 value);
BOOL HiddenArea_ContainsUnlessFlag(const HiddenArea *area, u16 x, u16 z, u16 flag, Field *field);
void HiddenEventData_Init(HiddenEventData *data, HiddenEventArgs *param, PlayerActionPossibilities *context);
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
u32 EventSweetScentCall_Check(PlayerActionPossibilities *context);
GameEvent *EventSweetScentCall_Create(HiddenEventArgs *args, PlayerActionPossibilities *context);
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
u32 EventChatterCall_Check(PlayerActionPossibilities *context);
GameEvent *EventChatterCall_Create(HiddenEventArgs *args, PlayerActionPossibilities *context);
GameEventReturnCode EventRuinsStrengthCall_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventRuinsStrengthCall_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);
GameEventReturnCode EventRuinsFlash_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventRuinsFlash_Create(HiddenEventArgs *param, PlayerActionPossibilities *context);

#endif // POKEBW2_FIELD_HIDDEN_EVENT_H
