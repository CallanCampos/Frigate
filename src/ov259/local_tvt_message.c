#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "pml/poke_party.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The fonts, message banks, word set and windows of the Xtransceiver's calls with the game's characters, named after
// the ROM's "local_tvt_message.c"

// How many windows can wait for their text to be drawn
#define PRINT_COUNT 32

// The frames of the windows of the upper and the lower screen
#define FRAME_CHAR 1
#define FRAME_PALETTE 13
#define SUB_FRAME_CHAR 0x140
#define SUB_FRAME_PALETTE 9

#define TALK_PALETTE 12

typedef struct {
    BmpWin *window;
    u8 pending;
} LocalTvtPrint;

struct LocalTvtMessage {
    TCBExManager *tcbManager;
    KeyCursor *cursor;
    AppPrintsysCommon printCommon;
    u16 heapId;
    StrBuf *strbuf;
    WordSet *wordSet;
    PrintQueue *queue;
    BmpWin *talkWindow;
    Font *font;
    Font *smallFont;
    PrintStream *stream;
    MsgData *menuMsgData;
    MsgData *talkMsgData;
    MsgData *pokemonMsgData;
    LocalTvtPrint prints[PRINT_COUNT];
    u16 color;
    s16 x;
    s16 y;
    u32 align;
    void (*updateTalk)(LocalTvtMessage *message);
    BOOL talkEndAllowed;
};

static void LocalTvtMessage_GetPrintPosition(LocalTvtMessage *message, BmpWin *window, const StrBuf *strbuf, s16 *x,
                                             s16 *y);
static void LocalTvtMessage_UpdateTalk(LocalTvtMessage *message);
static void LocalTvtMessage_UpdateTalkKeyWait(LocalTvtMessage *message);

// The cursor that waits for a key, 2 by 2 tiles
// clang-format off
static const u8 sLocalTvtKeyCursor[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0x11, 0x11,
    0x12, 0x11, 0x11, 0x11, 0x20, 0x11, 0x11, 0x21, 0x00, 0x12, 0x11, 0x22, 0x00, 0x20, 0x21, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
    0x22, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// clang-format on

LocalTvtMessage *LocalTvtMessage_Create(HeapID heapId) {
    LocalTvtMessage *message = GFL_HeapAllocate(heapId, sizeof(LocalTvtMessage), TRUE, "local_tvt_message.c", 108);

    message->heapId = heapId;
    message->menuMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0393, heapId);
    message->talkMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0384, heapId);
    message->pokemonMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_11, heapId);
    message->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, message->heapId);
    message->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, message->heapId);
    LoadSysMsgBox(0, FRAME_CHAR, FRAME_PALETTE, 0, message->heapId);
    LoadSysMsgBox(4, SUB_FRAME_CHAR, SUB_FRAME_PALETTE, 0, message->heapId);
    GFL_G2DIOLoadNCLR(ARCID_WINFRAME, GetSysMsgBoxPaletteDatID(0), 0, 0x20, FRAME_PALETTE * 0x20, 0x20,
                      message->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, 0, TALK_PALETTE * 0x20, 0x20, message->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 15 * 0x20, 0x20, message->heapId);
    func_020232d8();
    message->tcbManager = GFL_TCBExMgrCreate(message->heapId, message->heapId, 3, 0x100);
    message->stream = NULL;
    message->talkWindow = NULL;
    message->strbuf = GFL_StrBufCreate(0x400, message->heapId);
    message->wordSet = GFL_WordSetSystemCreateDefault(message->heapId);
    message->queue = func_02021998(message->heapId);
    message->cursor = KeyCursor_CreateEx(15, TRUE, TRUE, message->heapId, sLocalTvtKeyCursor);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG0,
                        GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 16, 4);
    return message;
}

void LocalTvtMessage_Free(LocalTvtMessage *message) {
    G2_BlendNone();
    if (message->stream != NULL) {
        func_020223cc(message->stream);
    }
    if (message->talkWindow != NULL) {
        BmpWin_Free(message->talkWindow);
    }
    KeyCursor_Free(message->cursor);
    func_02021a18(message->queue);
    GFL_WordSetSystemFree(message->wordSet);
    GFL_StrBufFree(message->strbuf);
    GFL_TCBExMgrFree(message->tcbManager);
    GFL_FontFree(message->smallFont);
    GFL_FontFree(message->font);
    GFL_MsgDataFree(message->pokemonMsgData);
    GFL_MsgDataFree(message->talkMsgData);
    GFL_MsgDataFree(message->menuMsgData);
    func_020232d8();
    GFL_HeapFree(message);
}

void LocalTvtMessage_Update(LocalTvtMessage *message) {
    int i;

    GFL_TCBExMgrUpdate(message->tcbManager);
    if (message->stream != NULL && message->talkWindow != NULL) {
        message->updateTalk(message);
    }
    func_02021a3c(message->queue);
    for (i = 0; i < PRINT_COUNT; i++) {
        PrintQueue *queue = message->queue;

        if (message->prints[i].pending) {
            if (!func_02021c1c(queue, BmpWin_GetBitmap(message->prints[i].window))) {
                BmpWin_FlushChar(message->prints[i].window);
                message->prints[i].pending = FALSE;
            }
        }
    }
}

Font *LocalTvtMessage_GetFont(LocalTvtMessage *message) {
    return message->font;
}

PrintQueue *LocalTvtMessage_GetPrintQueue(LocalTvtMessage *message) {
    return message->queue;
}

MsgData *LocalTvtMessage_GetMenuMsgData(LocalTvtMessage *message) {
    return message->menuMsgData;
}

MsgData *LocalTvtMessage_GetTalkMsgData(LocalTvtMessage *message) {
    return message->talkMsgData;
}

void LocalTvtMessage_SetColor(LocalTvtMessage *message, u16 color) {
    message->color = color;
}

void LocalTvtMessage_SetPosition(LocalTvtMessage *message, s16 x, s16 y, u32 align) {
    message->x = x;
    message->y = y;
    message->align = align;
}

void LocalTvtMessage_PrintMsg(LocalTvtMessage *message, BmpWin *window, MsgData *msgData, u32 messageId) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, messageId);

    LocalTvtMessage_Print(message, window, strbuf);
    GFL_StrBufFree(strbuf);
}

void LocalTvtMessage_Print(LocalTvtMessage *message, BmpWin *window, const StrBuf *strbuf) {
    LocalTvtPrint *print = NULL;
    PrintQueue *queue;
    int i;
    s16 x;
    s16 y;

    for (i = 0; i < PRINT_COUNT; i++) {
        if (!message->prints[i].pending) {
            print = &message->prints[i];
        }
    }
    print->window = window;
    print->pending = FALSE;
    GFL_WordSetFormatStrbuf(message->wordSet, message->strbuf, strbuf);
    GFL_BitmapFill(BmpWin_GetBitmap(window), message->color & 0x1f);
    LocalTvtMessage_GetPrintPosition(message, window, message->strbuf, &x, &y);
    queue = message->queue;
    func_02021c7c(queue, BmpWin_GetBitmap(print->window), x, y, message->strbuf, message->font, message->color);
    print->pending = TRUE;
    LocalTvtMessage_SetPosition(message, 0, 0, LOCAL_TVT_ALIGN_LEFT);
    LocalTvtMessage_SetColor(message, PRINT_COLOR(15, 2, 0));
}

void LocalTvtMessage_PrintPokemon(LocalTvtMessage *message, BmpWin *window, PartyPkm *pkm) {
    LocalTvtPrint *print = NULL;
    int i;
    StrBuf *strbuf;

    for (i = 0; i < PRINT_COUNT; i++) {
        if (!message->prints[i].pending) {
            print = &message->prints[i];
        }
    }
    print->window = window;
    print->pending = TRUE;
    GFL_BitmapFill(BmpWin_GetBitmap(window), message->color & 0x1f);
    strbuf = GFL_MsgDataLoadStrbufNew(message->pokemonMsgData, 3);
    loadPokemonNicknameToStrbuf(message->wordSet, 0, pkm);
    GFL_WordSetFormatStrbuf(message->wordSet, message->strbuf, strbuf);
    func_02021c7c(message->queue, BmpWin_GetBitmap(window), 32, 8, message->strbuf, message->font,
                  PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf);
    if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        strbuf = GFL_MsgDataLoadStrbufNew(message->pokemonMsgData, 2);
        WordSetNumber(message->wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL), 3, 0, TRUE);
        GFL_WordSetFormatStrbuf(message->wordSet, message->strbuf, strbuf);
        func_02021c7c(message->queue, BmpWin_GetBitmap(window), 0, 24, message->strbuf, message->smallFont,
                      PRINT_COLOR(15, 2, 0));
        GFL_StrBufFree(strbuf);
    }
    if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        switch (PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL)) {
        case 0:
            GFL_MsgDataLoadStrbuf(message->pokemonMsgData, 7, message->strbuf);
            func_02021c7c(message->queue, BmpWin_GetBitmap(window), 96, 8, message->strbuf, message->font,
                          PRINT_COLOR(5, 6, 0));
            break;
        case 1:
            GFL_MsgDataLoadStrbuf(message->pokemonMsgData, 8, message->strbuf);
            func_02021c7c(message->queue, BmpWin_GetBitmap(window), 96, 8, message->strbuf, message->font,
                          PRINT_COLOR(3, 4, 0));
            break;
        }
    }
}

void LocalTvtMessage_DrawFrame(LocalTvtMessage *message, BmpWin *window) {
    if (BmpWin_GetBGIndex(window) < 4) {
        BmpWin_DrawFrame(window, 2, FRAME_CHAR, FRAME_PALETTE);
    } else {
        BmpWin_DrawFrame(window, 2, SUB_FRAME_CHAR, SUB_FRAME_PALETTE);
    }
}

void LocalTvtMessage_ClearFrame(LocalTvtMessage *message, BmpWin *window) {
    BmpWin_ClearFrame(window, 2);
}

void LocalTvtMessage_SetPlayerName(LocalTvtMessage *message, GameData *gameData) {
    copyVarForText(message->wordSet, 0, GetGameDataPlayerInfo(gameData));
}

void LocalTvtMessage_SetSurveyAnswer(LocalTvtMessage *message, u32 index, u16 answer) {
    loadQuestionnaireAnswerToStrbuf(message->wordSet, index, answer);
}

void LocalTvtMessage_SetPokemonNickname(LocalTvtMessage *message, PartyPkm *pkm, u32 index) {
    loadPokemonNicknameToStrbuf(message->wordSet, index, pkm);
}

void LocalTvtMessage_SetWord(LocalTvtMessage *message, MsgData *msgData, u16 messageId, u16 index) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, messageId);

    func_0202437c(message->wordSet, index, strbuf, 2, 1, 1);
    GFL_StrBufFree(strbuf);
}

void LocalTvtMessage_SetPlaceName(LocalTvtMessage *message, GameData *gameData, u32 index) {
    loadLocationNameToStrbuf(message->wordSet, index,
                             ZoneData_GetPlaceNameID(PlayerState_GetZoneID(GameData_GetPlayerState(gameData))));
}

void LocalTvtMessage_SetNumber(LocalTvtMessage *message, s32 number, u32 digits, u32 index) {
    WordSetNumber(message->wordSet, index, number, digits, 0, TRUE);
}

void LocalTvtMessage_OpenTalkWindow(LocalTvtMessage *message, u32 position) {
    if (message->talkWindow == NULL) {
        if (position == LOCAL_TVT_TALK_TOP) {
            message->talkWindow = BmpWin_CreateDynamic(0, 1, 1, 30, 4, TALK_PALETTE, TRUE);
        } else if (position == LOCAL_TVT_TALK_BOTTOM) {
            message->talkWindow = BmpWin_CreateDynamic(0, 1, 19, 30, 4, TALK_PALETTE, TRUE);
        }
        BmpWin_DrawFrame(message->talkWindow, 1, FRAME_CHAR, FRAME_PALETTE);
        GFL_BitmapFill(BmpWin_GetBitmap(message->talkWindow), 15);
        BmpWin_FlushChar(message->talkWindow);
        BmpWin_FlushMap(message->talkWindow);
        GFL_BGSysQueueScrLoad(0);
    }
    AppPrintsysCommon_Init(&message->printCommon, 6);
}

void LocalTvtMessage_StartTalk(LocalTvtMessage *message, MsgData *msgData, u32 messageId, u32 keyWait) {
    if (message->stream != NULL) {
        func_020223cc(message->stream);
        message->stream = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(message->talkWindow), 15);
    GFL_MsgDataLoadStrbuf(msgData, messageId, message->strbuf);
    if (message->wordSet != NULL) {
        StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, messageId);

        GFL_WordSetFormatStrbuf(message->wordSet, message->strbuf, strbuf);
        GFL_StrBufFree(strbuf);
    }
    message->stream = func_02022268(message->talkWindow, 0, 0, message->strbuf, message->font, func_02017bcc(),
                                    message->tcbManager, 2, message->heapId, 15);
    switch (keyWait) {
    case FALSE:
    default:
        message->updateTalk = LocalTvtMessage_UpdateTalk;
        break;
    case TRUE:
        message->updateTalk = LocalTvtMessage_UpdateTalkKeyWait;
        message->talkEndAllowed = FALSE;
        break;
    }
}

BOOL LocalTvtMessage_IsTalkFinished(LocalTvtMessage *message) {
    if (message->stream == NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL LocalTvtMessage_IsTalkPaused(LocalTvtMessage *message) {
    if (func_020223b4(message->stream) != PRINT_STREAM_RUNNING) {
        return TRUE;
    }
    return FALSE;
}

BOOL LocalTvtMessage_IsTalkDone(LocalTvtMessage *message) {
    if (message->stream != NULL) {
        if (func_020223b4(message->stream) == PRINT_STREAM_DONE) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

static void LocalTvtMessage_GetPrintPosition(LocalTvtMessage *message, BmpWin *window, const StrBuf *strbuf, s16 *x,
                                             s16 *y) {
    switch (message->align) {
    case LOCAL_TVT_ALIGN_LEFT:
        *x = message->x;
        *y = message->y;
        break;
    case LOCAL_TVT_ALIGN_CENTER: {
        u32 centerX = GFL_BitmapGetWidth(BmpWin_GetBitmap(window)) / 2;
        u32 centerY = GFL_BitmapGetHeight(BmpWin_GetBitmap(window)) / 2;

        centerX -= GFL_FontGetBlockWidth(strbuf, message->font, 0) / 2;
        centerY -= GFL_FontGetBlockHeight(strbuf, message->font) / 2;
        *x = centerX + message->x;
        *y = centerY + message->y;
        break;
    }
    }
}

static void LocalTvtMessage_UpdateTalk(LocalTvtMessage *message) {
    KeyCursor_Update(message->cursor, message->stream, message->talkWindow);
    if (AppPrintsysCommon_Update(&message->printCommon, message->stream) == TRUE) {
        func_020223cc(message->stream);
        message->stream = NULL;
    }
}

static void LocalTvtMessage_UpdateTalkKeyWait(LocalTvtMessage *message) {
    KeyCursor_Update(message->cursor, message->stream, message->talkWindow);
    if (LocalTvtMessage_IsTalkDone(message)) {
        if (!message->talkEndAllowed) {
            return;
        }
        if (!AppPrintsysCommon_IsPressed(6)) {
            return;
        }
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
    }
    if (AppPrintsysCommon_Update(&message->printCommon, message->stream) == TRUE) {
        func_020223cc(message->stream);
        message->stream = NULL;
    }
}

void LocalTvtMessage_AllowTalkEnd(LocalTvtMessage *message) {
    KeyCursor_ShowWhenDone(message->cursor);
    message->talkEndAllowed = TRUE;
}
