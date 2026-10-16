#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_H

// The parts of the Xtransceiver's calls with the game's characters (overlay 259), one per file of the ROM's
// "local_tvt_*.c". Every name here is ours

#include "types.h"
#include "app/local_tvt/local_tvt_calldata.h"
#include "app/local_tvt/local_tvt_callmode.h"
#include "app/local_tvt/local_tvt_callselect.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_graphic.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/local_tvt/local_tvt_person.h"
#include "app/local_tvt/local_tvt_person_chelen.h"
#include "app/local_tvt/local_tvt_person_doctor.h"
#include "app/local_tvt/local_tvt_person_mother.h"
#include "app/local_tvt/local_tvt_person_support.h"
#include "app/local_tvt/local_tvt_scriptmode.h"
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

// local_tvt_person_*.c
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
