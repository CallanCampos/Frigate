#include "types.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "field/event_sound.h"
#include "field/event_chatot.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/msg.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/text_speed.h"
#include "system/app_keycursor.h"
#include "system/gf_font.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/chatter.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The sprite of the Chatot
static const ClActorSetup sSpriteSetup = { 128, 72, 0, 0, 0 };

GameEvent *func_ov033_02178ca8(GameSystem *gsys, Field *field, u8 partyIndex) {
    GameEvent *event;
    ChatotEventWork *work;
    PokeParty *party;

    event = GameEvent_Create(gsys, NULL, func_ov033_02178d10, sizeof(ChatotEventWork));
    work = GameEvent_GetData(event);
    sys_memset(work, 0, sizeof(ChatotEventWork));
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->chatter = getChatterBlockAddress(GameData_GetSaveControl(work->gameData));
    work->field = field;
    work->player = Field_GetPlayer(field);
    party = GameData_GetParty(work->gameData);
    work->pkm = PokeParty_GetPkm(party, partyIndex);
    work->msgBGSys = Field_GetMsgBGSys(work->field);
    work->partyIndex = partyIndex;
    work->recorded = 0;
    return event;
}

GameEventReturnCode func_ov033_02178d10(GameEvent *event, u32 *state, void *data) {
    ChatotEventWork *work = data;
    StrBuf *message;
    PokeVoiceChatterInfo chatterInfo;

    switch (*state) {
    case 0:
        func_ov033_02178fd4(work);
        *state = 1;
        break;
    case 1:
        work->msgData = func_ov036_021879a0(work->msgBGSys, 0x6c);
        work->talkWindow = func_ov036_0218845c(work->msgBGSys, work->msgData);
        work->wordSet = GFL_WordSetSystemCreateDefault(21);
        work->strbuf = GFL_StrBufCreate(0x400, 21);
        func_ov033_02179140(work);
        if (doesChatotExist(work->chatter) == TRUE) {
            func_ov036_0218836c(work->talkWindow, 0, 0, 0);
            *state = 2;
        } else {
            func_ov036_0218836c(work->talkWindow, 0, 0, 6);
            *state = 4;
        }
        break;
    case 2:
        if (func_ov036_021883e8(work->talkWindow) == TRUE) {
            work->yesNo = func_ov036_021880d4(work->msgBGSys, 0);
            *state = 3;
        }
        break;
    case 3:
        switch (func_ov036_0218816c(work->yesNo)) {
        case 0:
            func_ov036_02187ea0(work->yesNo);
            func_ov036_02188474(work->talkWindow);
            func_ov036_0218836c(work->talkWindow, 0, 0, 6);
            *state = 4;
            break;
        case 1:
            func_ov036_02187ea0(work->yesNo);
            *state = 13;
            break;
        }
        break;
    case 4:
        if (func_ov036_021883e8(work->talkWindow) == TRUE) {
            GCTX_HIDBlockSleep(8);
            setupMic(21);
            work->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), func_ov036_02188494(work->talkWindow), 15, 16, 21);
            *state = 5;
        }
        break;
    case 5:
        func_02006e0c(2);
        if (func_02006e3c() == TRUE) {
            WaitIcon_Free(work->waitIcon);
            func_ov036_02188474(work->talkWindow);
            func_ov036_0218836c(work->talkWindow, 0, 0, 1);
            GameEvent_ChainNext(event, EventBGMPushWait_Create(work->gsys, 6));
            *state = 6;
        }
        break;
    case 6:
        if (func_ov036_021883e8(work->talkWindow) == TRUE) {
            if (func_02006e80(func_ov033_02178fcc, &work->recorded) == 0) {
                *state = 7;
            } else {
                message = GFL_MsgDataLoadStrbufNew(work->msgData, 3);
                loadPokemonNicknameToStrbuf(work->wordSet, 0, work->pkm);
                GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, message);
                GFL_StrBufFree(message);
                func_ov036_02188474(work->talkWindow);
                func_ov036_021883b0(work->talkWindow, 0, 0, work->strbuf);
                ampOffFreeBlocks();
                GCTX_HIDUnblockSleep(8);
                GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 30));
                *state = 12;
            }
        }
        break;
    case 7:
        if (work->recorded == TRUE) {
            func_02006ec0(work->chatter);
            ampOffFreeBlocks();
            GCTX_HIDUnblockSleep(8);
            GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 30));
            *state = 8;
        }
        break;
    case 8:
        if (!func_ov033_021790c4(work)) {
            message = GFL_MsgDataLoadStrbufNew(work->msgData, 2);
            loadPokemonNicknameToStrbuf(work->wordSet, 0, work->pkm);
            GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, message);
            GFL_StrBufFree(message);
            func_ov036_02188474(work->talkWindow);
            func_ov036_021883b0(work->talkWindow, 0, 0, work->strbuf);
            *state = 9;
        }
        break;
    case 9:
        if (func_ov036_021883e8(work->talkWindow) == TRUE) {
            *state = 10;
        }
        break;
    case 10:
        PokeVoice_CreateChatterInfo(&chatterInfo);
        work->voice = PokeVoice_Play(SPECIES_CHATOT, 0, 64, 0, 0, 0, 0, &chatterInfo);
        *state = 11;
        break;
    case 11:
        if (!PokeVoice_IsPlaying(work->voice)) {
            *state = 13;
        }
        break;
    case 12:
        if (func_ov036_021883e8(work->talkWindow) == TRUE) {
            *state = 13;
        }
        break;
    case 13:
        func_ov033_021791a8(work);
        GFL_StrBufFree(work->strbuf);
        GFL_WordSetSystemFree(work->wordSet);
        func_ov036_02188338(work->talkWindow);
        func_ov036_021879b8(work->msgData);
        *state = 14;
        break;
    case 14:
        func_ov033_02178fe8(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void func_ov033_02178fcc(u32 result, u32 *done) {
    *done = TRUE;
}

void func_ov033_02178fd4(ChatotEventWork *work) {
    DisableAllActorsMovement(Field_GetActorSystem(GSYS_GetField(work->gsys)));
}

void func_ov033_02178fe8(ChatotEventWork *work) {
    EnableAllActorsMovement(Field_GetActorSystem(GSYS_GetField(work->gsys)));
}

void func_ov033_02178ffc(ChatotEventWork *work) {
    BoxPkm *pkm;
    ArcTool *arc;
    BOOL encrypted;
    ClActorSetup setup;

    work->unit = func_0204bf1c(1, 0, 21);
    pkm = func_0201d620(work->pkm);
    arc = MakePokeGraArcHandle(21);
    encrypted = PML_PkmDecrypt(pkm);
    work->chars = PokeGra_LoadClActCharsByBoxData(arc, pkm, 0, 0, 21);
    work->palette = PokeGra_LoadClActPaletteByBoxData(arc, pkm, 0, 0, 0xc0, 21);
    work->cellAnims = PokeGra_LoadClActCellAnimsByBoxData(pkm, 0, 1, 0, 21);
    PML_PkmReEncrypt(pkm, encrypted);
    GFL_ArcToolFree(arc);
    setup = sSpriteSetup;
    work->sprite = func_0204c040(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0, 21);
}

void func_ov033_021790a0(ChatotEventWork *work) {
    func_0204c108(work->sprite);
    func_0204b98c(work->chars);
    func_0204bcd0(work->palette);
    func_0204be64(work->cellAnims);
    func_0204bf98(work->unit);
}

u32 func_ov033_021790c4(ChatotEventWork *work) {
    ClActorPos position;

    if (work->animFrame == 0) {
        work->animOffset = -4;
    }
    if (work->animOffset == -4) {
        work->animOffset = 3;
        work->animFrame++;
        if (work->animFrame == 3) {
            return 0;
        }
    }
    func_0204c178(work->sprite, &position, 0);
    position.y -= work->animOffset;
    func_0204c140(work->sprite, &position, 0);
    work->animOffset--;
    return 1;
}

void func_ov033_02179140(ChatotEventWork *work) {
    u32 paletteId;
    GFLBitmap *bitmap;

    paletteId = GetSysMsgBoxPaletteDatID(0);
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, paletteId, 0, 0, 0, 32, 21);
    work->window = BmpWin_CreateDynamic(1, 10, 3, 12, 12, 0, 1);
    bitmap = BmpWin_GetBitmap(work->window);
    GFL_BitmapFill(bitmap, 17);
    BmpWin_FlushChar(work->window);
    BmpWin_FlushMap(work->window);
    BmpWin_DrawFrame(work->window, WINFRAME_TRANSFER_VBLANK, 1, 0);
    func_ov033_02178ffc(work);
}

void func_ov033_021791a8(ChatotEventWork *work) {
    func_ov033_021790a0(work);
    BmpWin_ClearFrame(work->window, WINFRAME_TRANSFER_VBLANK);
    BmpWin_Free(work->window);
}
