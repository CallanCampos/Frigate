// The ROM names this file zukan_nickname.c.
#include "types.h"
#include "app/pokedex_nickname.h"
#include "battle/btlv_input.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/poke_party.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_title.h"
#include "text/system/yes_no.h"

// The nickname question of the Pokédex registration

// The BG of the windows
#define BG_TEXT 3
// The palette of the message font, and of the frame
#define PALETTE_FONT 9
#define PALETTE_FRAME 8
// The text colors of the message: the letter, its shadow and the background
#define COLOR_LETTER 1
#define COLOR_SHADOW 2
#define COLOR_BACKGROUND 15
// The frames that the question waits after the message about the box
#define WAIT_FRAMES 60

// The steps of the question
enum {
    STEP_IDLE,
    STEP_ASK,
    STEP_ANSWER,
    STEP_BOX_MESSAGE,
    STEP_BOX_MESSAGE_WAIT,
    STEP_DONE,
};

// The steps of the yes or no
enum {
    INPUT_IDLE,
    INPUT_WAIT_MESSAGE,
    INPUT_SHOW,
    INPUT_ASK,
    INPUT_ANSWERED,
};

struct PokedexNickname {
    u16 heapId;
    PartyPkm *pkm;
    StrBuf *boxMessage;
    BoxSaveAccessor *boxes;
    u32 box;
    ClActUnit *unit;
    Font *font;
    PrintQueue *printQueue;
    GameData *gameData;
    BmpWin *messageWindow;
    PrintStream *stream;
    StrBuf *message;
    u32 frameChars;
    TCBExManager *tcbManager;
    BmpWin *blankWindow;
    u32 step;
    u32 answer;
    u32 timer;
    u8 isNPokemon;
    TCB *vblankTask;
    u32 inputStep;
    u8 keyCursorFlag;
    BtlvInput *input;
    PaletteFade *paletteFade;
};

static void PokedexNickname_VBlank(TCB *tcb, void *data);
static void PokedexNickname_InitInput(PokedexNickname *nickname);
static void PokedexNickname_FreeInput(PokedexNickname *nickname);
static void PokedexNickname_UpdateInput(PokedexNickname *nickname);

// Nothing refers to this, but the ROM has it before the other tables
const s32 POKEDEX_NICKNAME_UNUSED_BUTTON_FLAGS[2] = { 1, 0 };
static const s32 sPokedexNicknameButtonFlags[2] = { 0, 0 };
static const s32 sPokedexNicknameButtonFlashRows[2] = { 4, 8 };
static const TouchRect sPokedexNicknameButtonRects[3] = {
    { 32, 80, 0, 255 },
    { 80, 128, 0, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};
static const BtlvInputButtonSet sPokedexNicknameButtonSet = { sPokedexNicknameButtonRects, sPokedexNicknameButtonFlags,
                                                              sPokedexNicknameButtonFlashRows };

static const BtlvInputKeyStop sPokedexNicknameKeyStops[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, 1 },
    { { 1, 1, 1, 1, -1, -1 }, 0, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 1, 1 },
};

PokedexNickname *PokedexNickname_Create(HeapID heapId, PartyPkm *pkm, StrBuf *boxMessage, BoxSaveAccessor *boxes,
                                        u32 box, ClActUnit *unit, Font *font, PrintQueue *printQueue,
                                        GameData *gameData) {
    PokedexNickname *nickname = GFL_HeapAllocate(heapId, sizeof(PokedexNickname), FALSE, "zukan_nickname.c", 209);
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *template;
    BOOL keyCursor;

    MI_CpuFill8(nickname, 0, sizeof(PokedexNickname));
    nickname->heapId = heapId;
    nickname->pkm = pkm;
    nickname->boxMessage = boxMessage;
    nickname->boxes = boxes;
    nickname->box = box;
    nickname->unit = unit;
    nickname->font = font;
    nickname->printQueue = printQueue;
    nickname->gameData = gameData;
    nickname->step = STEP_IDLE;
    nickname->answer = POKEDEX_NICKNAME_UNANSWERED;
    nickname->timer = 0;
    nickname->isNPokemon = PokeParty_GetParam(pkm, PKM_PARAM_N_POKEMON, NULL);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x120, 0x20, nickname->heapId);

    wordSet = GFL_WordSetSystemCreateDefault(nickname->heapId);
    setPartyPokemonSpeciesNameToStrbuf(wordSet, 0, nickname->pkm);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_TITLE, nickname->heapId);
    template = GFL_MsgDataLoadStrbufNew(msgData, BtlServerFlowTitle_Text_GiveNicknameCaught + nickname->isNPokemon);
    nickname->message = GFL_StrBufCreate(0x60, nickname->heapId);
    GFL_WordSetFormatStrbuf(wordSet, nickname->message, template);
    GFL_StrBufFree(template);
    GFL_MsgDataFree(msgData);
    GFL_WordSetSystemFree(wordSet);

    nickname->blankWindow = BmpWin_CreateDynamic(BG_TEXT, 0, 0, 1, 1, PALETTE_FONT, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(nickname->blankWindow), 0);
    BmpWin_FlushChar(nickname->blankWindow);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(nickname->blankWindow));
    nickname->tcbManager = GFL_TCBExMgrCreate(nickname->heapId, nickname->heapId, 1, 0);

    nickname->messageWindow = BmpWin_CreateDynamic(BG_TEXT, 1, 19, 30, 4, PALETTE_FONT, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(nickname->messageWindow), COLOR_BACKGROUND);
    GFL_TextRndUpdateColorIndexLUT(COLOR_LETTER, COLOR_SHADOW, COLOR_BACKGROUND);
    nickname->stream =
        PrintStream_Create(nickname->messageWindow, 0, 0, nickname->message, nickname->font, TextSpeed_GetCurrentWait(),
                           nickname->tcbManager, 2, nickname->heapId, COLOR_BACKGROUND);
    nickname->frameChars = LoadCursorImageEndOfHeap(BG_TEXT, PALETTE_FRAME, 0, nickname->heapId);
    keyCursor = TRUE;
    if (GCTX_HIDIsUsingTouch()) {
        keyCursor = FALSE;
    }
    nickname->keyCursorFlag = keyCursor;
    PokedexNickname_InitInput(nickname);
    nickname->vblankTask = GFL_VBlankTCBAdd(PokedexNickname_VBlank, nickname, 1);
    return nickname;
}

void PokedexNickname_Free(PokedexNickname *nickname) {
    PokedexNickname_FreeInput(nickname);
    GFL_TCBRemove(nickname->vblankTask);
    PrintStream_Delete(nickname->stream);
    GFL_BGSysFreeCharMemory(BG_TEXT, CHAR_POS(nickname->frameChars), CHAR_SIZE(nickname->frameChars));
    BmpWin_Free(nickname->messageWindow);
    GFL_StrBufFree(nickname->message);
    GFL_TCBExMgrFree(nickname->tcbManager);
    BmpWin_Free(nickname->blankWindow);
    GFL_HeapFree(nickname);
}

BOOL PokedexNickname_Update(PokedexNickname *nickname) {
    BOOL done = FALSE;
    BOOL pressed;
    u32 state;
    WordSet *wordSet;

    switch (nickname->step) {
    case STEP_IDLE:
        break;
    case STEP_ASK:
        GFL_TCBExMgrUpdate(nickname->tcbManager);
        state = PrintStream_GetState(nickname->stream);
        if (state == PRINT_STREAM_DONE) {
            nickname->step = STEP_ANSWER;
            if (nickname->isNPokemon) {
                nickname->inputStep = INPUT_WAIT_MESSAGE;
            } else {
                nickname->inputStep = INPUT_SHOW;
            }
            break;
        }
        switch (state) {
        case PRINT_STREAM_RUNNING:
            pressed = FALSE;
            if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
                pressed = TRUE;
            } else if (GFL_HIDIsTouchHeld()) {
                pressed = TRUE;
                GCTX_HIDSetUsingTouch(TRUE);
            }
            if (pressed) {
                PrintStream_SkipWait(nickname->stream, 0);
            }
            break;
        case PRINT_STREAM_PAUSED:
            pressed = FALSE;
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
                pressed = TRUE;
            } else if (GFL_HIDIsTouchPressed()) {
                pressed = TRUE;
                GCTX_HIDSetUsingTouch(TRUE);
            }
            if (pressed) {
                PrintStream_Continue(nickname->stream);
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            break;
        }
        break;
    case STEP_ANSWER:
        if (nickname->answer == POKEDEX_NICKNAME_UNANSWERED) {
            break;
        }
        if (nickname->answer == POKEDEX_NICKNAME_NO) {
            if (nickname->boxMessage == NULL) {
                nickname->step = STEP_DONE;
                break;
            }
            GFL_BitmapFill(BmpWin_GetBitmap(nickname->messageWindow), COLOR_BACKGROUND);
            PrintStream_Delete(nickname->stream);
            GFL_StrBufFree(nickname->message);
            wordSet = GFL_WordSetSystemCreateDefault(nickname->heapId);
            loadPokemonNicknameToStrbuf(wordSet, 0, nickname->pkm);
            loadBoxNameForDisplay(wordSet, 1, nickname->boxes, nickname->box);
            nickname->message = GFL_StrBufCreate(0x60, nickname->heapId);
            GFL_WordSetFormatStrbuf(wordSet, nickname->message, nickname->boxMessage);
            GFL_WordSetSystemFree(wordSet);
            nickname->stream = PrintStream_Create(nickname->messageWindow, 0, 0, nickname->message, nickname->font,
                                                  TextSpeed_GetCurrentWait(), nickname->tcbManager, 2, nickname->heapId,
                                                  COLOR_BACKGROUND);
            nickname->step = STEP_BOX_MESSAGE;
        } else {
            nickname->step = STEP_DONE;
        }
        break;
    case STEP_BOX_MESSAGE:
        GFL_TCBExMgrUpdate(nickname->tcbManager);
        state = PrintStream_GetState(nickname->stream);
        if (state == PRINT_STREAM_DONE) {
            nickname->step = STEP_BOX_MESSAGE_WAIT;
            break;
        }
        switch (state) {
        case PRINT_STREAM_RUNNING:
            pressed = FALSE;
            if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
                pressed = TRUE;
            } else if (GFL_HIDIsTouchHeld()) {
                pressed = TRUE;
                GCTX_HIDSetUsingTouch(TRUE);
            }
            if (pressed) {
                PrintStream_SkipWait(nickname->stream, 0);
            }
            break;
        case PRINT_STREAM_PAUSED:
            pressed = FALSE;
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
                pressed = TRUE;
            } else if (GFL_HIDIsTouchPressed()) {
                pressed = TRUE;
                GCTX_HIDSetUsingTouch(TRUE);
            }
            if (pressed) {
                PrintStream_Continue(nickname->stream);
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
            }
            break;
        }
        break;
    case STEP_BOX_MESSAGE_WAIT:
        nickname->timer++;
        if (nickname->timer == WAIT_FRAMES) {
            nickname->step = STEP_DONE;
        }
        break;
    case STEP_DONE:
        done = TRUE;
        break;
    }
    PokedexNickname_UpdateInput(nickname);
    return done;
}

void PokedexNickname_Start(PokedexNickname *nickname) {
    if (nickname->step == STEP_IDLE) {
        BmpWin *window;

        BmpWin_DrawFrame(nickname->messageWindow, 1, CHAR_POS(nickname->frameChars), PALETTE_FRAME);
        window = nickname->messageWindow;
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
        nickname->step = STEP_ASK;
    }
}

u32 PokedexNickname_GetAnswer(PokedexNickname *nickname) {
    return nickname->answer;
}

static void PokedexNickname_VBlank(TCB *tcb, void *data) {
    PokedexNickname *nickname = data;

    if (nickname->paletteFade != NULL) {
        PaletteFade_Transfer(nickname->paletteFade);
    }
}

static void PokedexNickname_InitInput(PokedexNickname *nickname) {
    GFL_OvlLoad(OVERLAY_ID(168));
    nickname->paletteFade = PaletteFade_Create(nickname->heapId);
    PaletteFade_SetTransferAll(nickname->paletteFade, TRUE);
    PaletteFade_AllocBuffer(nickname->paletteFade, 1, 0x1e0, nickname->heapId);
    PaletteFade_AllocBuffer(nickname->paletteFade, 3, 0x1e0, nickname->heapId);
    nickname->input = BtlvInput_CreateSimple(nickname->gameData, 0, nickname->paletteFade, nickname->font,
                                             &nickname->keyCursorFlag, nickname->heapId);
    nickname->inputStep = INPUT_IDLE;
}

static void PokedexNickname_FreeInput(PokedexNickname *nickname) {
    BtlvInput_Delete(nickname->input);
    PaletteFade_FreeBuffer(nickname->paletteFade, 1);
    PaletteFade_FreeBuffer(nickname->paletteFade, 3);
    PaletteFade_Free(nickname->paletteFade);
    nickname->paletteFade = NULL;
    GFL_OvlUnload(OVERLAY_ID(168));
}

static void PokedexNickname_UpdateInput(PokedexNickname *nickname) {
    switch (nickname->inputStep) {
    case INPUT_IDLE:
        break;
    case INPUT_WAIT_MESSAGE: {
        BOOL pressed = FALSE;

        if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GCTX_HIDSetUsingTouch(FALSE);
            pressed = TRUE;
        } else if (GFL_HIDIsTouchHeld()) {
            pressed = TRUE;
            GCTX_HIDSetUsingTouch(TRUE);
        }
        if (pressed) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            nickname->answer = POKEDEX_NICKNAME_NO;
            nickname->inputStep = INPUT_ANSWERED;
        }
        break;
    }
    case INPUT_SHOW: {
        MsgData *msgData;
        BtlvInputYesNoParam param;
        nickname->keyCursorFlag = GCTX_HIDIsUsingTouch() ? FALSE : TRUE;
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_YES_NO, nickname->heapId);
        param.strs[0] = GFL_MsgDataLoadStrbufNew(msgData, YesNo_Text_Yes);
        param.strs[1] = GFL_MsgDataLoadStrbufNew(msgData, YesNo_Text_No);
        param.canCancel = TRUE;
        BtlvInput_SetScreen(nickname->input, 4, &param);
        GFL_StrBufFree(param.strs[0]);
        GFL_StrBufFree(param.strs[1]);
        GFL_MsgDataFree(msgData);
        nickname->inputStep = INPUT_ASK;
        break;
    }
    case INPUT_ASK: {
        s32 button = BtlvInput_CheckInput(nickname->input, &sPokedexNicknameButtonSet, sPokedexNicknameKeyStops);

        if (button != -1) {
            BtlvInput_SetScreen(nickname->input, 0, NULL);
            nickname->inputStep = INPUT_ANSWERED;
            if (button == 0) {
                nickname->answer = POKEDEX_NICKNAME_YES;
            } else {
                nickname->answer = POKEDEX_NICKNAME_NO;
            }
            GCTX_HIDSetUsingTouch(nickname->keyCursorFlag == FALSE);
        }
        break;
    }
    case INPUT_ANSWERED:
        break;
    }
    BtlvInput_Update(nickname->input);
}
