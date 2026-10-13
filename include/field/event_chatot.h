#ifndef POKEBW2_FIELD_EVENT_CHATOT_H
#define POKEBW2_FIELD_EVENT_CHATOT_H

#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/text_speed.h"
#include "system/app_keycursor.h"
#include "system/gf_font.h"
#include "gfl/str.h"
#include "system/game_event.h"

struct ChatotEventWork {
    void *chatter;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    FieldPlayer *player;
    PartyPkm *pkm;
    MsgData *msgData;
    void *msgBGSys;
    void *talkWindow;
    void *yesNo;
    WordSet *wordSet;
    StrBuf *strbuf;
    BmpWin *window;
    WaitIcon *waitIcon;
    ClActUnit *unit;
    ClActor *sprite;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u8 animFrame;
    s8 animOffset;
    u8 partyIndex;
    u8 unk4F;
    u32 voice;
    // Set by the microphone's callback once the recording is done
    u32 recorded;
};

GameEvent *func_ov033_02178ca8(GameSystem *gsys, Field *field, u8 partyIndex);
GameEventReturnCode func_ov033_02178d10(GameEvent *event, u32 *state, void *data);
void func_ov033_02178fcc(u32 result, u32 *done);
void func_ov033_02178fd4(ChatotEventWork *work);
void func_ov033_02178fe8(ChatotEventWork *work);
void func_ov033_02178ffc(ChatotEventWork *work);
void func_ov033_021790a0(ChatotEventWork *work);
u32 func_ov033_021790c4(ChatotEventWork *work);
void func_ov033_02179140(ChatotEventWork *work);
void func_ov033_021791a8(ChatotEventWork *work);

#endif // POKEBW2_FIELD_EVENT_CHATOT_H
