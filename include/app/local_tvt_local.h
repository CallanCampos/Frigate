#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_H

// The parts of the Xtransceiver's calls with the game's characters (overlay 259), one per file of the ROM's
// "local_tvt_*.c". Every name here is ours

#include "types.h"
#include "app/local_tvt/local_tvt_calldata.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_graphic.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/local_tvt/local_tvt_person.h"
#include "app/local_tvt/local_tvt_util.h"
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

typedef struct LocalTvtScriptMode LocalTvtScriptMode;
typedef struct LocalTvtCallMode LocalTvtCallMode;
typedef struct LocalTvtCallSelect LocalTvtCallSelect;

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

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_H
