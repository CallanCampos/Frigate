#include "types.h"
#include "app/marine_tube_board.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "save/player_info.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The process that shows the Marine Tube's board

#define BG_MAIN_BOARD 1
#define BG_SUB_BACKGROUND 4
#define BG_MESSAGE MARINE_TUBE_BOARD_BG_MESSAGE

// The steps of the process
enum {
    STEP_START,
    STEP_WAIT_FADE_IN,
    STEP_PRINT,
    STEP_WAIT_INPUT,
    STEP_WAIT_FADE_OUT,
    STEP_END,
};

typedef struct {
    u16 heapId;
    MarineTubeBoardGraphic *graphic;
    MarineTubeBoardMsgWin msgWin;
    MarineTubeBoardPrinter printer;
    Font *font;
    MsgData *msgData;
    PrintQueue *printQueue;
    WordSet *wordSet;
} MarineTubeBoardWork;

static BOOL MarineTubeBoard_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MarineTubeBoard_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MarineTubeBoard_Main(GameProc *proc, u32 *state, void *param, void *work);
static void MarineTubeBoard_LoadGraphics(u16 board, HeapID heapId);

const GameProcFunctions MARINE_TUBE_BOARD_PROC_FUNCTIONS = { MarineTubeBoard_Init, MarineTubeBoard_Main,
                                                             MarineTubeBoard_Exit };

static BOOL MarineTubeBoard_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MarineTubeBoardParam *boardParam = param;
    GameData *gameData = GSYS_GetGameData(boardParam->gsys);
    MarineTubeBoardWork *wk;
    ArcTool *arc;
    HeapID heapId;
    u32 palette = 1;
    u32 gender;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MARINE_TUBE_BOARD, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MarineTubeBoardWork), HEAPID_MARINE_TUBE_BOARD);
    MI_CpuFill8(wk, 0, sizeof(MarineTubeBoardWork));
    wk->heapId = HEAPID_MARINE_TUBE_BOARD;
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_9, wk->heapId);
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    wk->graphic = MarineTubeBoardGraphic_Create(1, wk->heapId);
    gender = GetGameDataPlayerInfo(gameData)->gender;
    heapId = wk->heapId;
    if (gender != GENDER_FEMALE) {
        palette = 0;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, PALTYPE_SUB_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, BG_SUB_BACKGROUND, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, BG_SUB_BACKGROUND, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_SUB_BACKGROUND);
    MarineTubeBoard_LoadGraphics(boardParam->board, wk->heapId);
    MarineTubeBoardMsgWin_Init(&wk->msgWin, wk->font, wk->msgData, wk->printQueue, wk->wordSet, wk->heapId);
    MarineTubeBoardPrinter_Init(&wk->printer, wk->heapId);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 16, 0);
    GFL_NetSetIconScreen(FALSE, wk->heapId);
    return TRUE;
}

static BOOL MarineTubeBoard_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MarineTubeBoardWork *wk = work;
    HeapID heapId;

    MarineTubeBoardPrinter_Free(&wk->printer);
    MarineTubeBoardMsgWin_Free(&wk->msgWin);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    GFL_BGSysFreeFilledChar(BG_MESSAGE, 1, 0);
    MarineTubeBoardGraphic_Free(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    return TRUE;
}

static BOOL MarineTubeBoard_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MarineTubeBoardParam *boardParam = param;
    MarineTubeBoardWork *wk = work;
    BOOL input = FALSE;
    u32 x;
    u32 y;

    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GCTX_HIDSetUsingTouch(FALSE);
        input = TRUE;
    } else if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
        input = TRUE;
        GCTX_HIDSetUsingTouch(TRUE);
    }
    switch (*state) {
    case STEP_START:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *state = STEP_WAIT_FADE_IN;
        break;
    case STEP_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            MarineTubeBoardPrinter_Print(&wk->printer, &wk->msgWin, boardParam->board, wk->heapId);
            *state = STEP_PRINT;
        }
        break;
    case STEP_PRINT:
        if (MarineTubeBoardPrinter_Update(&wk->printer, &wk->msgWin) == TRUE) {
            *state = STEP_WAIT_INPUT;
        }
        break;
    case STEP_WAIT_INPUT:
        if (input) {
            *state = STEP_WAIT_FADE_OUT;
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        }
        break;
    case STEP_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            *state = STEP_END;
        }
        break;
    case STEP_END:
        return TRUE;
    }
    PrintQueue_Main(wk->printQueue);
    MarineTubeBoardGraphic_Update(wk->graphic);
    return FALSE;
}

static void MarineTubeBoard_LoadGraphics(u16 board, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MARINE_TUBE_BOARD, heapId);
    u32 palette;
    u32 chars;
    u32 screen;

    if (board == 0) {
        palette = 0;
        chars = 2;
        screen = 4;
    } else {
        palette = 1;
        chars = 3;
        screen = 5;
    }
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, PALTYPE_MAIN_BG, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, chars, BG_MAIN_BOARD, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, screen, BG_MAIN_BOARD, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_MAIN_BOARD);
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, PALTYPE_SUB_BG, 15 * 0x20, 0x20, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysFillChar(BG_MESSAGE, 0, 1, 0);
    LoadSysMsgBox(BG_MESSAGE, MARINE_TUBE_BOARD_FRAME_CHAR, MARINE_TUBE_BOARD_FRAME_PALETTE, 0, heapId);
}
