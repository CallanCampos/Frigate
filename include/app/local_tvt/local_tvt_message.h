#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_MESSAGE_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_MESSAGE_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// local_tvt_message.c: the fonts, message banks and windows

// How LocalTvtMessage_SetPosition places the text
enum {
    LOCAL_TVT_ALIGN_LEFT,
    LOCAL_TVT_ALIGN_CENTER,
};

// Where the window of a talk is
enum {
    LOCAL_TVT_TALK_TOP,
    LOCAL_TVT_TALK_BOTTOM,
};

LocalTvtMessage *LocalTvtMessage_Create(HeapID heapId);
void LocalTvtMessage_Free(LocalTvtMessage *message);
void LocalTvtMessage_Update(LocalTvtMessage *message);
Font *LocalTvtMessage_GetFont(LocalTvtMessage *message);
PrintQueue *LocalTvtMessage_GetPrintQueue(LocalTvtMessage *message);
MsgData *LocalTvtMessage_GetMenuMsgData(LocalTvtMessage *message);
MsgData *LocalTvtMessage_GetTalkMsgData(LocalTvtMessage *message);
// The color and position of the next LocalTvtMessage_Print, which resets them
void LocalTvtMessage_SetColor(LocalTvtMessage *message, u16 color);
void LocalTvtMessage_SetPosition(LocalTvtMessage *message, s16 x, s16 y, u32 align);
void LocalTvtMessage_PrintMsg(LocalTvtMessage *message, BmpWin *window, MsgData *msgData, u32 messageId);
void LocalTvtMessage_Print(LocalTvtMessage *message, BmpWin *window, const StrBuf *strbuf);
// The nickname, level and gender of a party Pokémon
void LocalTvtMessage_PrintPokemon(LocalTvtMessage *message, BmpWin *window, PartyPkm *pkm);
void LocalTvtMessage_DrawFrame(LocalTvtMessage *message, BmpWin *window);
void LocalTvtMessage_ClearFrame(LocalTvtMessage *message, BmpWin *window);
// The words of the messages
void LocalTvtMessage_SetPlayerName(LocalTvtMessage *message, GameData *gameData);
void LocalTvtMessage_SetSurveyAnswer(LocalTvtMessage *message, u32 index, u16 answer);
void LocalTvtMessage_SetPokemonNickname(LocalTvtMessage *message, PartyPkm *pkm, u32 index);
void LocalTvtMessage_SetWord(LocalTvtMessage *message, MsgData *msgData, u16 messageId, u16 index);
void LocalTvtMessage_SetPlaceName(LocalTvtMessage *message, GameData *gameData, u32 index);
void LocalTvtMessage_SetNumber(LocalTvtMessage *message, s32 number, u32 digits, u32 index);
// The talk of a face: its window at the top or bottom, and the message printed a character at a time
void LocalTvtMessage_OpenTalkWindow(LocalTvtMessage *message, u32 position);
// With keyWait == TRUE, retain the completed stream until ending is allowed and A/B or touch is pressed.
void LocalTvtMessage_StartTalk(LocalTvtMessage *message, MsgData *msgData, u32 messageId, u32 keyWait);
// True when no talk stream exists, including before the first talk.
BOOL LocalTvtMessage_IsTalkFinished(LocalTvtMessage *message);
// Requires an existing talk stream. True for any state other than RUNNING, including retained DONE.
BOOL LocalTvtMessage_IsTalkPaused(LocalTvtMessage *message);
// True while an existing talk stream has reached the end of its text.
BOOL LocalTvtMessage_IsTalkDone(LocalTvtMessage *message);
// Permits the current key-wait talk to end once its text is DONE and A/B or touch is pressed.
void LocalTvtMessage_AllowTalkEnd(LocalTvtMessage *message);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_MESSAGE_H
