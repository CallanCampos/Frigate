#ifndef POKEBW2_FIELD_APP_CALL_H
#define POKEBW2_FIELD_APP_CALL_H

#include "types.h"
#include "struct_decls.h"
#include "field/player_action.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
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
#define FIELD_APP_BAG 2
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

// The procs of FIELD_PROC_LINK_LIST's apps that no header declares yet
extern const GameProcFunctions data_ov012_0216dd78;
extern const GameProcFunctions CONFIG_PANEL_PROC_FUNCTIONS;
extern const GameProcFunctions TOWNMAP_PROC_FUNCTIONS;
extern const GameProcFunctions MEDALBOX_PROC_FUNCTIONS;

// Overlay 12: the trainer card's param, whose canEdit is FALSE in the Union Room
void *func_ov012_02169b04(GameData *gameData, HeapID heapId, BOOL canEdit);

void *func_ov012_0215b7d8(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215b9cc(FieldAppCallWork *work, void *param);
void *func_ov012_0215bad4(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215bb44(FieldAppCallWork *work, void *param);
void *func_ov012_0215bb70(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215bcf0(FieldAppCallWork *work, void *param);
u32 func_ov012_0215bd1c(GameSystem *gsys);
void *func_ov012_0215bd48(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215bdd0(FieldAppCallWork *work, void *param);
void *func_ov012_0215bef4(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215bf58(FieldAppCallWork *work, void *param);
void *func_ov012_0215bf8c(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215bff8(FieldAppCallWork *work, void *param);
void *func_ov012_0215c094(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c0cc(FieldAppCallWork *work, void *param);
void func_ov012_0215c0dc(FieldAppCallWork *work, s32 appParam);
void *func_ov012_0215c10c(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c138(FieldAppCallWork *work, void *param);
void *func_ov012_0215c160(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c218(FieldAppCallWork *work, void *param);
void func_ov012_0215c2c8(void *param);
void *script_evo(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c3a4(FieldAppCallWork *work, void *param);
void *func_ov012_0215c3d0(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c3fc(FieldAppCallWork *work, void *param);
void *func_ov012_0215c438(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c474(FieldAppCallWork *work, void *param);
void *func_ov012_0215c488(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam);
u32 func_ov012_0215c4b4(FieldAppCallWork *work, void *param);

GameEventReturnCode EventFieldAppCall_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventFieldAppCall_Create(FieldAppCallInput *input, u16 code);
void EventFieldAppCall_ConvAppResultToEventType(u32 result, u32 *eventType);
void func_ov012_0215b754(FieldAppCallWork *work);
void func_ov012_0215b76c(FieldAppCallParam *param, FieldAppCallInput *input, FieldAppCallPredicate canRetry,
                          FieldAppCallPredicate callback1, FieldAppCallPredicate callback2, void *arg);
BOOL FieldAppCallParam_CanRetry(FieldAppCallParam *param);
BOOL func_ov012_0215b7a8(FieldAppCallParam *param);
BOOL func_ov012_0215b7c0(FieldAppCallParam *param);

#endif // POKEBW2_FIELD_APP_CALL_H
