#include "types.h"
#include "constants/arc.h"
#include "constants/species.h"
#include "field/event_chatot.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_msgbg.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/chatter.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"

// The sprite of the Chatot
static const ClActorSetup sSpriteSetup = { 128, 72, 0, 0, 0 };

GameEvent *EventPerapRecord_Create(GameSystem *gsys, Field *field, u8 partyIndex) {
    GameEvent *event;
    ChatotEventWork *work;
    PokeParty *party;

    event = GameEvent_Create(gsys, NULL, EventPerapRecord_Main, sizeof(ChatotEventWork));
    work = GameEvent_GetData(event);
    MI_CpuFill8(work, 0, sizeof(ChatotEventWork));
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

GameEventReturnCode EventPerapRecord_Main(GameEvent *event, u32 *state, void *data) {
    ChatotEventWork *work = data;
    StrBuf *message;
    PokeVoiceChatterInfo chatterInfo;

    switch (*state) {
    case 0:
        EventPerapRecord_PauseActors(work);
        *state = 1;
        break;
    case 1:
        work->msgData = MsgBGPrinter_CreateMsgData(work->msgBGSys, 0x6c);
        work->talkWindow = MsgBGPrinter_CreateBottomSysWin(work->msgBGSys, work->msgData);
        work->wordSet = GFL_WordSetSystemCreateDefault(21);
        work->strbuf = GFL_StrBufCreate(0x400, 21);
        EventPerapRecord_SetupWindow(work);
        if (doesChatotExist(work->chatter) == TRUE) {
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 0);
            *state = 2;
        } else {
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 6);
            *state = 4;
        }
        break;
    case 2:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            work->yesNo = MsgBGPrinter_CreateYesNoMenu(work->msgBGSys, 0);
            *state = 3;
        }
        break;
    case 3:
        switch (MsgBGPrinter_YesNoMenuMain(work->yesNo)) {
        case 0:
            MsgBGPrinter_DeleteMenuFunc(work->yesNo);
            MsgBGPrinter_ClearSysWin(work->talkWindow);
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 6);
            *state = 4;
            break;
        case 1:
            MsgBGPrinter_DeleteMenuFunc(work->yesNo);
            *state = 13;
            break;
        }
        break;
    case 4:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            GCTX_HIDBlockSleep(8);
            setupMic(21);
            work->waitIcon =
                WaitIcon_Create(GFL_VBlankGetTCBMgr(), MsgBGPrinter_GetSysWinBmpWin(work->talkWindow), 15, 16, 21);
            *state = 5;
        }
        break;
    case 5:
        SndMic_AddCount(2);
        if (SndMic_IsOverLimit() == TRUE) {
            WaitIcon_Free(work->waitIcon);
            MsgBGPrinter_ClearSysWin(work->talkWindow);
            MsgBGPrinter_SysWinPrintMsg(work->talkWindow, 0, 0, 1);
            GameEvent_ChainNext(event, EventBGMPushWait_Create(work->gsys, 6));
            *state = 6;
        }
        break;
    case 6:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            if (SndMic_StartRecording(EventPerapRecord_OnRecorded, &work->recorded) == 0) {
                *state = 7;
            } else {
                message = GFL_MsgDataLoadStrbufNew(work->msgData, 3);
                loadPokemonNicknameToStrbuf(work->wordSet, 0, work->pkm);
                GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, message);
                GFL_StrBufFree(message);
                MsgBGPrinter_ClearSysWin(work->talkWindow);
                MsgBGPrinter_SysWinPrintStr(work->talkWindow, 0, 0, work->strbuf);
                ampOffFreeBlocks();
                GCTX_HIDUnblockSleep(8);
                GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 30));
                *state = 12;
            }
        }
        break;
    case 7:
        if (work->recorded == TRUE) {
            SndMic_EncodeChatter(work->chatter);
            ampOffFreeBlocks();
            GCTX_HIDUnblockSleep(8);
            GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 30));
            *state = 8;
        }
        break;
    case 8:
        if (!EventPerapRecord_HopPokeIcon(work)) {
            message = GFL_MsgDataLoadStrbufNew(work->msgData, 2);
            loadPokemonNicknameToStrbuf(work->wordSet, 0, work->pkm);
            GFL_WordSetFormatStrbuf(work->wordSet, work->strbuf, message);
            GFL_StrBufFree(message);
            MsgBGPrinter_ClearSysWin(work->talkWindow);
            MsgBGPrinter_SysWinPrintStr(work->talkWindow, 0, 0, work->strbuf);
            *state = 9;
        }
        break;
    case 9:
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
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
        if (MsgBGPrinter_SysWinMain(work->talkWindow) == TRUE) {
            *state = 13;
        }
        break;
    case 13:
        EventPerapRecord_Cleanup(work);
        GFL_StrBufFree(work->strbuf);
        GFL_WordSetSystemFree(work->wordSet);
        MsgBGPrinter_DeleteSysWin(work->talkWindow);
        MsgBGPrinter_DeleteMsgData(work->msgData);
        *state = 14;
        break;
    case 14:
        EventPerapRecord_ResumeActors(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void EventPerapRecord_OnRecorded(u32 result, u32 *done) {
    *done = TRUE;
}

void EventPerapRecord_PauseActors(ChatotEventWork *work) {
    DisableAllActorsMovement(Field_GetActorSystem(GSYS_GetField(work->gsys)));
}

void EventPerapRecord_ResumeActors(ChatotEventWork *work) {
    EnableAllActorsMovement(Field_GetActorSystem(GSYS_GetField(work->gsys)));
}

void EventPerapRecord_CreatePokeIcon(ChatotEventWork *work) {
    BoxPkm *pkm;
    ArcTool *arc;
    BOOL encrypted;
    ClActorSetup setup;

    work->unit = ClActUnit_Create(1, 0, 21);
    pkm = PokeParty_GetBoxPkm(work->pkm);
    arc = MakePokeGraArcHandle(21);
    encrypted = PML_PkmDecrypt(pkm);
    work->chars = PokeGra_LoadClActCharsByBoxData(arc, pkm, POKEGRA_DIR_FRONT, 0, 21);
    work->palette = PokeGra_LoadClActPaletteByBoxData(arc, pkm, POKEGRA_DIR_FRONT, 0, 0xc0, 21);
    work->cellAnims = PokeGra_LoadClActCellAnimsByBoxData(pkm, POKEGRA_DIR_FRONT, 1, 0, 21);
    PML_PkmReEncrypt(pkm, encrypted);
    GFL_ArcToolFree(arc);
    setup = sSpriteSetup;
    work->sprite = ClActor_Create(work->unit, work->chars, work->palette, work->cellAnims, &setup, 0, 21);
}

void EventPerapRecord_DeletePokeIcon(ChatotEventWork *work) {
    ClActor_Delete(work->sprite);
    ClActSys_FreeChars(work->chars);
    ClActSys_FreePltt(work->palette);
    ClActSys_FreeCellAnims(work->cellAnims);
    ClActUnit_Delete(work->unit);
}

u32 EventPerapRecord_HopPokeIcon(ChatotEventWork *work) {
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
    ClActor_GetSurfacePos(work->sprite, &position, 0);
    position.y -= work->animOffset;
    ClActor_SetSurfacePos(work->sprite, &position, 0);
    work->animOffset--;
    return 1;
}

void EventPerapRecord_SetupWindow(ChatotEventWork *work) {
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
    EventPerapRecord_CreatePokeIcon(work);
}

void EventPerapRecord_Cleanup(ChatotEventWork *work) {
    EventPerapRecord_DeletePokeIcon(work);
    BmpWin_ClearFrame(work->window, WINFRAME_TRANSFER_VBLANK);
    BmpWin_Free(work->window);
}
