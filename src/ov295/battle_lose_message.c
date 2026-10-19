#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/battle_lose_message.h"
#include "field/event_battle_lose.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/title_5.h"

enum {
    STATE_SETUP,
    STATE_START_FADE_IN,
    STATE_WAIT_FADE_IN,
    STATE_WAIT_INPUT,
    STATE_WAIT_FADE_OUT,
    STATE_CLEANUP,
};

typedef struct {
    u16 heapId;
    // Whether the Trainer went to a Pokémon Center and not home
    BOOL returnNonLeague;
    PlayerInfo *playerInfo;
    MsgData *msgData;
    WordSet *wordSet;
    BmpWin *window;
    Font *font;
} BattleLoseMessage;

static BOOL BattleLoseMessage_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleLoseMessage_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BattleLoseMessage_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void BattleLoseMessage_InitBGs(HeapID heapId);
static void BattleLoseMessage_FreeBGs(void);
static void BattleLoseMessage_PrintMessage(BattleLoseMessage *msg, u32 messageId, s16 x, s16 y);

const GameProcFunctions BATTLE_LOSE_MESSAGE_PROC_FUNCTIONS = {
    BattleLoseMessage_Init,
    BattleLoseMessage_Main,
    BattleLoseMessage_Exit,
};

static BOOL BattleLoseMessage_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BattleLoseData *loseData = param;
    BattleLoseMessage *msg;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GAME_CLEAR_SAVE, 0x40000);
    msg = GFL_ProcInitSubsystem(proc, sizeof(BattleLoseMessage), HEAPID_GAME_CLEAR_SAVE);
    MIi_CpuClear32(0, msg, sizeof(BattleLoseMessage));
    msg->heapId = HEAPID_GAME_CLEAR_SAVE;
    msg->returnNonLeague = loseData->returnNonLeague;
    msg->playerInfo = loseData->playerInfo;
    return TRUE;
}

static BOOL BattleLoseMessage_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BattleLoseMessage *msg = work;
    BmpWin *window;
    u32 keys = GCTX_HIDGetPressedKeys();

    switch (*state) {
    case STATE_SETUP:
        GX_SetBankForLCDC(GX_VRAM_LCDC_ALL);
        MIi_CpuClearFast(0, (void *)HW_LCDC_VRAM, HW_LCDC_VRAM_SIZE);
        GX_DisableBankForLCDC();
        BattleLoseMessage_InitBGs(msg->heapId);
        msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, msg->heapId);
        msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_5, msg->heapId);
        msg->wordSet = GFL_WordSetSystemCreateDefault(msg->heapId);
        msg->window = BmpWin_CreateDynamic(0, 2, 7, 27, 15, 13, 1);
        if (msg->returnNonLeague == 0) {
            BattleLoseMessage_PrintMessage(msg, Title5_Text_ScurriedBackHomeProtecting, 6, 0);
        } else {
            BattleLoseMessage_PrintMessage(msg, Title5_Text_ScurriedPokemonCenterProtecting, 6, 0);
        }
        window = msg->window;
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        GFL_NetSetIconScreen(TRUE, msg->heapId);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
        (*state)++;
        break;
    case STATE_START_FADE_IN:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        reg_G2_BLDCNT = 0;
        (*state)++;
        break;
    case STATE_WAIT_FADE_IN:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case STATE_WAIT_INPUT:
        if ((keys & PAD_BUTTON_A) || (keys & PAD_BUTTON_B)) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
            (*state)++;
        }
        break;
    case STATE_WAIT_FADE_OUT:
        if (!GFL_FadeIsRunning()) {
            GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 0);
            (*state)++;
        }
        break;
    case STATE_CLEANUP:
        BmpWin_ClearFrame(msg->window, WINFRAME_TRANSFER_NOW);
        BmpWin_Free(msg->window);
        GFL_WordSetSystemFree(msg->wordSet);
        GFL_MsgDataFree(msg->msgData);
        GFL_BGSysReleaseBG(0);
        GFL_FontFree(msg->font);
        BattleLoseMessage_FreeBGs();
        return TRUE;
    }
    return FALSE;
}

static BOOL BattleLoseMessage_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GAME_CLEAR_SAVE);
    return TRUE;
}

static void BattleLoseMessage_InitBGs(HeapID heapId) {
    static const BGSysVRAMConfig sVRAMConfig = {
        GX_VRAM_BG_128_B, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
        GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
        GX_VRAM_TEX_0_A,  GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
    };
    static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
    static const BGSetup sBGSetup = {
        0,
        0,
        0x800,
        0,
        BGRES_256x256,
        GX_BG_COLORMODE_16,
        GX_BG_SCRBASE(0xf800),
        GX_BG_CHARBASE(0x00000),
        0x8000,
        GX_BG_EXTPLTT_01,
        1,
        GX_BG_AREAOVER_XLU,
        FALSE,
    };

    GFL_BGSysCreate(heapId);
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    GFL_BGSysCreateBG(0, &sBGSetup, BGMODE_TEXT);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 7, 0, 0x1a0, 0x20, heapId);
    GFL_BGSysResetStdPalette(0, 0);
    BmpWin_InitAllocator(heapId);
}

static void BattleLoseMessage_FreeBGs(void) {
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void BattleLoseMessage_PrintMessage(BattleLoseMessage *msg, u32 messageId, s16 x, s16 y) {
    StrBuf *message = GFL_StrBufCreate(0x400, msg->heapId);
    StrBuf *formatted = GFL_StrBufCreate(0x400, msg->heapId);

    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(msg->window), 15);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, message);
    copyVarForText(msg->wordSet, 1, msg->playerInfo);
    GFL_WordSetFormatStrbuf(msg->wordSet, formatted, message);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(msg->window), x, y, formatted, msg->font);
    GFL_StrBufFree(message);
    GFL_StrBufFree(formatted);
    BmpWin_FlushChar(msg->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(msg->window));
}
