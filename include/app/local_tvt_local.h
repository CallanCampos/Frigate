#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_H

// The parts of the Xtransceiver's calls with the game's characters (overlay 259), one per file of the ROM's
// "local_tvt_*.c". Every name here is ours

#include "types.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_graphic.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/xtransceiver.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/printsys.h"

typedef struct LocalTvtBG LocalTvtBG;
typedef struct LocalTvtScriptMode LocalTvtScriptMode;
typedef struct LocalTvtCallMode LocalTvtCallMode;
typedef struct LocalTvtCallData LocalTvtCallData;
typedef struct LocalTvtCallSelect LocalTvtCallSelect;
typedef struct LocalTvtPerson LocalTvtPerson;
typedef struct LocalTvtSeq LocalTvtSeq;
typedef struct LocalTvtMenu LocalTvtMenu;
typedef struct LocalTvtPokeSelect LocalTvtPokeSelect;

// local_tvt_scriptmode.c: plays a call's script

LocalTvtScriptMode *LocalTvtScriptMode_Create(HeapID heapId, GameSystem *gsys, GameData *gameData, u16 scriptId,
                                              LocalTvtChara *chara);
void LocalTvtScriptMode_Free(LocalTvtScriptMode *mode);
BOOL LocalTvtScriptMode_Main(LocalTvtScriptMode *mode);

// local_tvt_callmode.c: the calls the player makes

// Whether the player picks the person, or the call is with LOCAL_TVT_CALL_SECRET at once
enum {
    LOCAL_TVT_CALL_MODE_SELECT,
    LOCAL_TVT_CALL_MODE_SECRET,
    LOCAL_TVT_CALL_MODE_2,
};

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    LocalTvtChara *chara;
    LocalTvtMessage *message;
    ClActUnit *unit;
    // LOCAL_TVT_CALL_MODE_*
    u32 mode;
} LocalTvtCallModeSetup;

LocalTvtCallMode *LocalTvtCallMode_Create(const LocalTvtCallModeSetup *setup, HeapID heapId);
void LocalTvtCallMode_Free(LocalTvtCallMode *mode);
BOOL LocalTvtCallMode_Main(LocalTvtCallMode *mode);

// local_tvt_callselect.c: the list of the persons to call on the lower screen

#define LOCAL_TVT_SELECT_MAX 6

// The results of the list
enum {
    LOCAL_TVT_SELECT_NONE,
    LOCAL_TVT_SELECT_CALL,
    LOCAL_TVT_SELECT_EXIT,
};

typedef struct {
    RivalDataSave *rival;
    LocalTvtMessage *message;
    ClActUnit *unit;
    MsgData *msgData;
    // The messages of the names
    u32 names[LOCAL_TVT_SELECT_MAX];
    BOOL enabled[LOCAL_TVT_SELECT_MAX];
    int count;
} LocalTvtCallSelectSetup;

LocalTvtCallSelect *LocalTvtCallSelect_Create(const LocalTvtCallSelectSetup *setup, HeapID heapId);
void LocalTvtCallSelect_Free(LocalTvtCallSelect *select);
void LocalTvtCallSelect_Main(LocalTvtCallSelect *select);
u32 LocalTvtCallSelect_GetResult(LocalTvtCallSelect *select);
int LocalTvtCallSelect_GetSelected(LocalTvtCallSelect *select);

// local_tvt_calldata.c: the table of the calls and the save data of the calls made

LocalTvtCallData *LocalTvtCallData_Create(GameSystem *gsys, GameData *gameData, HeapID heapId);
void LocalTvtCallData_Free(LocalTvtCallData *data);
// The background and the object of the face of a person the player calls, for the calls made so far
u32 LocalTvtCallData_GetBackground(LocalTvtCallData *data, u16 person);
u32 LocalTvtCallData_GetObject(LocalTvtCallData *data, u16 person);
u32 LocalTvtCallData_GetBadgeCount(LocalTvtCallData *data);
GameData *LocalTvtCallData_GetGameData(LocalTvtCallData *data);
// The messages of the call with a person, for the calls made so far
u16 LocalTvtCallData_GetGreetingMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetChatMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetByeMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetExtraMsg(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_CanCall(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_IsListed(LocalTvtCallData *data, u16 person);
// The background and the object of a face in a zone, by its battle background and weather
u32 LocalTvtCallData_GetZoneBackground(LocalTvtCallData *data, u16 zoneId);
u32 LocalTvtCallData_GetZoneObject(LocalTvtCallData *data, u16 zoneId);
// The calls made with a person, which count up to a maximum
u16 LocalTvtCallData_GetCallCount(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_IsPlayerMale(LocalTvtCallData *data);
void LocalTvtCallData_AddCall(LocalTvtCallData *data, u16 person, u32 max);
void LocalTvtCallData_Set0200ff68(LocalTvtCallData *data, u32 value);
void LocalTvtCallData_Set0200ff78(LocalTvtCallData *data);
BOOL LocalTvtCallData_Get0200ff8c(LocalTvtCallData *data);
void LocalTvtCallData_Reset0200ffcc(LocalTvtCallData *data);
BOOL LocalTvtCallData_IsSecretUnknown(LocalTvtCallData *data);

// local_tvt_person.c: the call with a person, who greets and then talks about the topic the player picks

#define LOCAL_TVT_TOPIC_MAX 4

typedef struct {
    LocalTvtMessage *message;
    LocalTvtChara *chara;
    ClActUnit *unit;
    LocalTvtCallData *callData;
} LocalTvtPersonSetup;

// A step of a call, which returns TRUE once it is over. The work starts with the step's own counter
typedef BOOL (*LocalTvtPersonFunc)(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

// What each local_tvt_person_*.c gives of its person
typedef struct {
    void *work;
    LocalTvtPersonFunc greet;
    LocalTvtPersonFunc topics[LOCAL_TVT_TOPIC_MAX];
    void (*free)(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
    // The messages of the topics in the menu
    u32 topicNames[LOCAL_TVT_TOPIC_MAX];
    u32 topicCount;
    // The person's question before the menu
    u32 menuMsg;
    MsgData *msgData;
} LocalTvtPersonFuncs;

LocalTvtPerson *LocalTvtPerson_Create(const LocalTvtPersonFuncs *funcs, const LocalTvtPersonSetup *setup,
                                      HeapID heapId);
void LocalTvtPerson_Free(LocalTvtPerson *person);
void LocalTvtPerson_Main(LocalTvtPerson *person);
BOOL LocalTvtPerson_IsEnd(LocalTvtPerson *person);
// Ends the call; the topic that says goodbye calls it
void LocalTvtPerson_End(LocalTvtPerson *person);

// local_tvt_person_*.c
void LocalTvtSupport_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtMother_Setup(LocalTvtPersonFuncs *funcs, LocalTvtMessage *message, MsgData *msgData, GameData *gameData,
                          HeapID heapId);
void LocalTvtDoctor_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtChelen_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtRival_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtSecret_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, LocalTvtCallData *callData,
                          BOOL unk, HeapID heapId);
// The face of the person, which depends on the player's gender
u16 LocalTvtSecret_GetFacePerson(LocalTvtCallData *data);
// The message of the name of the person, whose name the player learns later
u16 LocalTvtSecret_GetNameMsg(LocalTvtCallData *data);
// How often the Xtransceiver rings before the person answers
u32 LocalTvtSecret_GetRingCount(LocalTvtCallData *data);
// The stage of the calls with the person for a count of calls, and what a stage has
u32 LocalTvtSecret_GetStage(int callCount);
u32 LocalTvtSecret_GetStage0378(u32 stage);
u32 LocalTvtSecret_GetStageCount(u32 stage);

// local_tvt_util.c

// The part of the day by the season's hours of light
u32 LocalTvtUtil_GetTime(GameData *gameData);
// The backgrounds of both screens
LocalTvtBG *LocalTvtBG_Create(GameData *gameData, HeapID heapId, u32 unk04);
void LocalTvtBG_Free(LocalTvtBG *bg);

// A sequence of functions with a step each, until one ends it
typedef void (*LocalTvtSeqFunc)(LocalTvtSeq *seq, int *step, void *work);

LocalTvtSeq *LocalTvtSeq_Create(void *work, LocalTvtSeqFunc func, HeapID heapId);
void LocalTvtSeq_Free(LocalTvtSeq *seq);
void LocalTvtSeq_Main(LocalTvtSeq *seq);
BOOL LocalTvtSeq_IsEnd(LocalTvtSeq *seq);
void LocalTvtSeq_SetNext(LocalTvtSeq *seq, LocalTvtSeqFunc func);
void LocalTvtSeq_End(LocalTvtSeq *seq);

// The list of the party on the lower screen, of which the player picks a Pokémon
typedef struct {
    LocalTvtMessage *message;
    ClActUnit *unit;
    PokeParty *party;
} LocalTvtPokeSelectSetup;

// The results of the list
enum {
    LOCAL_TVT_POKE_SELECT_NONE,
    LOCAL_TVT_POKE_SELECT_DECIDE,
    LOCAL_TVT_POKE_SELECT_CANCEL,
};

LocalTvtPokeSelect *LocalTvtPokeSelect_Create(const LocalTvtPokeSelectSetup *setup, HeapID heapId);
void LocalTvtPokeSelect_Free(LocalTvtPokeSelect *select);
void LocalTvtPokeSelect_Main(LocalTvtPokeSelect *select);
void LocalTvtPokeSelect_SetActive(LocalTvtPokeSelect *select, BOOL active);
void LocalTvtPokeSelect_Restart(LocalTvtPokeSelect *select);
u32 LocalTvtPokeSelect_GetResult(LocalTvtPokeSelect *select);
PartyPkm *LocalTvtPokeSelect_GetPkm(LocalTvtPokeSelect *select);

// The menu of topics on the lower screen
typedef struct {
    LocalTvtMessage *message;
    ClActUnit *unit;
    MsgData *msgData;
    u32 items[LOCAL_TVT_TOPIC_MAX];
    u32 count;
    u32 cursor;
} LocalTvtMenuSetup;

LocalTvtMenu *LocalTvtMenu_Create(const LocalTvtMenuSetup *setup, HeapID heapId);
void LocalTvtMenu_Free(LocalTvtMenu *menu);
void LocalTvtMenu_Main(LocalTvtMenu *menu);
BOOL LocalTvtMenu_IsEnd(LocalTvtMenu *menu);
u32 LocalTvtMenu_GetSelected(LocalTvtMenu *menu);

// The touch bar of the lower screen with its return button, which the B button presses as well
typedef struct LocalTvtReturnBar LocalTvtReturnBar;

LocalTvtReturnBar *LocalTvtReturnBar_Create(ClActUnit *unit, HeapID heapId);
void LocalTvtReturnBar_Free(LocalTvtReturnBar *bar);
void LocalTvtReturnBar_Main(LocalTvtReturnBar *bar);
BOOL LocalTvtReturnBar_IsPressed(LocalTvtReturnBar *bar);
// Whether the button's animation of being pressed is over
BOOL LocalTvtReturnBar_IsAnimEnd(LocalTvtReturnBar *bar);

// The input of a list by the keys or the touch screen, whichever the player used last. The cursor shows only with
// the keys
typedef struct LocalTvtKeyTouch LocalTvtKeyTouch;

// What the callbacks of the input return
enum {
    LOCAL_TVT_INPUT_NONE,
    // The cursor moved, or the touch only picked an entry
    LOCAL_TVT_INPUT_MOVE,
    LOCAL_TVT_INPUT_DECIDE,
};

typedef struct {
    void *work;
    int (*touch)(void *work);
    int (*key)(void *work);
    // Draws the list with or without its cursor
    void (*draw)(void *work, BOOL touchMode);
} LocalTvtKeyTouchSetup;

LocalTvtKeyTouch *LocalTvtKeyTouch_Create(const LocalTvtKeyTouchSetup *setup, HeapID heapId);
void LocalTvtKeyTouch_Free(LocalTvtKeyTouch *input);
void LocalTvtKeyTouch_Main(LocalTvtKeyTouch *input);
BOOL LocalTvtKeyTouch_IsDecided(LocalTvtKeyTouch *input);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_H
