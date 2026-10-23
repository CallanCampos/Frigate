#ifndef POKEBW2_FIELD_APP_CALL_H
#define POKEBW2_FIELD_APP_CALL_H

#include "types.h"
#include "field/player_action.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

// Called with the app call's input and its arg
typedef BOOL (*FieldAppCallPredicate)(FieldAppCallInput *input, void *arg);

struct FieldAppCallParam {
    FieldAppCallPredicate canRetry;
    FieldAppCallPredicate callback1;
    FieldAppCallPredicate callback2;
    void *arg;
    FieldAppCallInput *input;
};

struct FieldAppCallInput {
    GameSystem *gameSystem;
    Field *field;
    GameEvent *parent;
    // The field menu's subscreen, to go back to
    u32 screenId;
    // The app to open, from the field menu's table, or -1
    s32 appId;
    // A parameter for the app, or -1
    s32 appParam;
    FieldAppCallPredicate canRetry;
    FieldAppCallPredicate callback1;
    FieldAppCallPredicate callback2;
    void *arg;
    // What the app chose, for the caller to act on: the event type from EventFieldAppCall_ConvAppResultToEventType
    // and its parameters
    u32 eventType;
    u32 eventId;
    u32 partySlot;
    u32 eventValue;
    u32 unk38;
    u32 unk3C;
    u32 unk40;
};

// The apps of FIELD_PROC_LINK_LIST, by index
#define FIELD_APP_POKELIST 0
#define FIELD_APP_ZUKAN 1
#define FIELD_APP_BAG 2
#define FIELD_APP_TRAINER_CARD 3
#define FIELD_APP_SAVE 4
#define FIELD_APP_CONFIG 5
#define FIELD_APP_INF_DUNGEON_RETIRE 6
#define FIELD_APP_POKESTATUS 7
#define FIELD_APP_MAIL 10
#define FIELD_APP_SHINKA_DEMO 11

// The results of an app: 4 opens nextApp, the others are event types for EventFieldAppCall_ConvAppResultToEventType
#define FIELD_APP_RESULT_NEXT 4

struct FieldAppCallWork {
    u16 code;
    u16 pad02;
    // The app open, the one before it, and the one to open next
    s32 appId;
    s32 prevAppId;
    s32 nextAppId;
    u32 result;
    GameEvent *event;
    FieldAppCallInput *input;
    // What the open app was started with
    void *appParam;
    FieldAppCallParam params;
    PlayerActionPerms perms;
    PlayerActionPossibilities action;
    // The party slot and the item to carry over from one app to the next
    u8 partySlot;
    u8 pad69;
    u16 item;
    u32 unk6C;
    // How the party screen or the mail screen was reached
    u32 subMode;
    BOOL unk74;
    u32 unk78;
};

// An app that the field app call can open: its proc, and how to start it, read what it left and free its param
typedef struct {
    s32 overlayId;
    const GameProcFunctions *functions;
    void *(*createParam)(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
    u32 (*getResult)(FieldAppCallWork *work, void *param);
    // Called in place of a proc, if functions is NULL
    void (*call)(FieldAppCallWork *work, s32 appParam);
    void (*freeParam)(void *param);
} FieldProcLink;

extern const FieldProcLink FIELD_PROC_LINK_LIST[15];

void *FieldAppCall_CreatePokeListParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetPokeListResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateZukanParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetZukanResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreatePStatusParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetPStatusResult(FieldAppCallWork *work, void *param);
u32 FieldAppCall_GetBagMode(GameSystem *gsys);
void *FieldAppCall_CreateBagParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetBagResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateTrainerCardParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetTrainerCardResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateTownMapParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetTownMapResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateConfigParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetConfigResult(FieldAppCallWork *work, void *param);
void FieldAppCall_CallSave(FieldAppCallWork *work, s32 appParam);
void *FieldAppCall_CreateWifiNoteParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetWifiNoteResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateMailParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetMailResult(FieldAppCallWork *work, void *param);
void FieldAppCall_FreeMailParam(void *param);
void *script_evo(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetShinkaDemoResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateBattleRecorderParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId,
                                             const void *prevParam);
u32 FieldAppCall_GetBattleRecorderResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateXtransceiverParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetXtransceiverResult(FieldAppCallWork *work, void *param);
void *FieldAppCall_CreateMedalBoxParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 FieldAppCall_GetMedalBoxResult(FieldAppCallWork *work, void *param);

GameEventReturnCode EventFieldAppCall_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventFieldAppCall_Create(FieldAppCallInput *input, u16 code);
void EventFieldAppCall_ConvAppResultToEventType(u32 result, u32 *eventType);
void FieldAppCall_ClearBagFreeSpaceFilter(FieldAppCallWork *work);
void FieldAppCallParam_Init(FieldAppCallParam *param, FieldAppCallInput *input, FieldAppCallPredicate canRetry,
                            FieldAppCallPredicate callback1, FieldAppCallPredicate callback2, void *arg);
BOOL FieldAppCallParam_CanRetry(FieldAppCallParam *param);
BOOL FieldAppCallParam_RunCallback1(FieldAppCallParam *param);
BOOL FieldAppCallParam_RunCallback2(FieldAppCallParam *param);

#endif // POKEBW2_FIELD_APP_CALL_H
