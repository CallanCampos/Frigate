#include "types.h"
#include "app/dpc_main.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "save/hall_of_fame.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The Hall of Fame PC's set-up: the display, the palette fade of the Pokémon, the text and the teams. The ROM doesn't
// name this file; the name is a guess, after box2_main.c

static void DpcMain_VBlank(TCB *tcb, void *data);

static const BGSysVRAMConfig sDpcMainVramBanks = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

void DpcMain_InitVBlank(DendouPCWork *wk) {
    wk->vblankTask = GFL_VBlankTCBAdd(DpcMain_VBlank, wk, 0);
}

void DpcMain_ExitVBlank(DendouPCWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
}

static void DpcMain_VBlank(TCB *tcb, void *data) {
    DendouPCWork *wk = data;

    GFL_BGSysUpdate();
    func_0204b7c8();
    PaletteFade_Transfer(wk->palFade);
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}

void DpcMain_InitVram(void) {
    GFL_BGSysInitVRAM(GX_VRAM_NONE);
    GFL_BGSysSetVRAMBanks(&sDpcMainVramBanks);
}

const BGSysVRAMConfig *DpcMain_GetVramBanks(void) {
    return &sDpcMainVramBanks;
}

void DpcMain_InitBg(void) {
    GFL_BGSysCreate(HEAPID_DENDOU_PC);

    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
        GFL_BGSysSetLCDConfig(&config);
    }

    // The touch screen, which the main engine drives
    {
        BGSetup setup = {
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
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x1000,
            0,
            BGRES_512x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xd800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
    }

    // The top screen
    {
        BGSetup setup = {
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
        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
    }

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, TRUE);
}

void DpcMain_ExitBg(void) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysFree();
}

void DpcMain_LoadBgGraphic(void) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_DENDOU_PC, HEAPID_TAIL(HEAPID_DENDOU_PC));

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, PALTYPE_MAIN_BG, 0, 0x80, HEAPID_DENDOU_PC);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_SUB_BG, 0, 0x40, HEAPID_DENDOU_PC);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 0, 0, 0, TRUE, HEAPID_DENDOU_PC);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 4, 0, 0, TRUE, HEAPID_DENDOU_PC);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, 0, 0, 0, TRUE, HEAPID_DENDOU_PC);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, 1, 0, 0, TRUE, HEAPID_DENDOU_PC);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 2, 0, 0, TRUE, HEAPID_DENDOU_PC);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 7, 4, 0, 0, TRUE, HEAPID_DENDOU_PC);
    GFL_ArcToolFree(arc);

    // The title's window takes the bottom rows of the background's screen
    GFL_BGSysLoadScrAreaAll(3, (u16 *)GFL_BGSysIsScrHeapExists(0) + 21 * 32, 0, 21, 32, 3);

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 15 * 0x20, 0x20, HEAPID_DENDOU_PC);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 15 * 0x20, 0x20, HEAPID_DENDOU_PC);
}

void DpcMain_InitPaletteFade(DendouPCWork *wk) {
    wk->palFade = PaletteFade_Create(HEAPID_DENDOU_PC);
    PaletteFade_AllocBuffer(wk->palFade, PALFADE_BUFFER_MAIN_OBJ, 0x200, HEAPID_DENDOU_PC);
}

void DpcMain_ExitPaletteFade(DendouPCWork *wk) {
    PaletteFade_FreeBuffer(wk->palFade, PALFADE_BUFFER_MAIN_OBJ);
    PaletteFade_Free(wk->palFade);
}

void DpcMain_BlendPokePalettes(DendouPCWork *wk) {
    u32 i;

    // Both sets of actors, of which one is shown
    for (i = 0; i < HALL_OF_FAME_TEAM_SIZE; i++) {
        PaletteFade_BlendPalettes(wk->palFade, PALFADE_BUFFER_MAIN_OBJ, (1 << i) | (1 << (i + HALL_OF_FAME_TEAM_SIZE)),
                                  wk->fadeNow[i], 0);
    }
    PaletteFade_SetTransferAll(wk->palFade, TRUE);
}

BOOL DpcMain_IsPaletteFading(DendouPCWork *wk) {
    if (PaletteFade_GetActiveMask(wk->palFade) != 0) {
        return TRUE;
    }
    PaletteFade_SetTransferAll(wk->palFade, FALSE);
    return FALSE;
}

void DpcMain_SetBlend(void) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_OBJ, 16, 8);
}

void DpcMain_InitMsg(DendouPCWork *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM_3, HEAPID_DENDOU_PC);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_DENDOU_PC);
    wk->smallFont = GFL_FontCreate(ARCID_FONT, 3, 0, FALSE, HEAPID_DENDOU_PC);
    wk->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_DENDOU_PC);
    wk->printQueue = PrintQueue_CreateDefault(HEAPID_DENDOU_PC);
    wk->expandBuf = GFL_StrBufCreate(0x400, HEAPID_DENDOU_PC);
}

void DpcMain_ExitMsg(DendouPCWork *wk) {
    GFL_StrBufFree(wk->expandBuf);
    PrintQueue_Free(wk->printQueue);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_FontFree(wk->smallFont);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData);
}

// The buffers that the save's functions copy the Pokémon's names into
static inline void DpcMain_CreateTeamStrings(DendouPCWork *wk) {
    u16 i, j;

    for (i = 0; i < HALL_OF_FAME_TEAMS; i++) {
        for (j = 0; j < HALL_OF_FAME_TEAM_SIZE; j++) {
            wk->teams[i].pokemon[j].nickname = GFL_StrBufCreate(0x20, HEAPID_DENDOU_PC);
            wk->teams[i].pokemon[j].trainerName = GFL_StrBufCreate(0x20, HEAPID_DENDOU_PC);
        }
    }
}

void DpcMain_LoadTeams(DendouPCWork *wk) {
    u16 i, j;

    DpcMain_CreateTeamStrings(wk);

    if (wk->param->mode == DENDOU_PC_MODE_FIRST_CLEAR) {
        HallOfFameTeam *team = func_020179c8(wk->param->gameData);

        wk->teams[0].count = HallOfFameTeam_GetPokemonCount(team);
        HallOfFameTeam_GetDate(team, &wk->teams[0].date);
        for (i = 0; i < wk->teams[0].count; i++) {
            HallOfFameTeam_GetPokemon(team, i, &wk->teams[0].pokemon[i]);
        }
        wk->pageMax = 1;
    } else {
        SaveControl *save = GameData_GetSaveControl(wk->param->gameData);

        if (SaveControl_OpenExtraBlock(save, SAVE_EXTRA_ID_HALL_OF_FAME, HEAPID_DENDOU_PC) == 1) {
            HallOfFameSave *hallOfFame = getAddressOfExtraSaveBlk(save, SAVE_EXTRA_ID_HALL_OF_FAME, 0);

            if (hallOfFame != NULL) {
                for (i = 0; i < HALL_OF_FAME_TEAMS; i++) {
                    wk->teams[i].count = HallOfFameSave_GetPokemonCount(hallOfFame, i);
                    wk->teams[i].number = HallOfFameSave_GetEntryNumber(hallOfFame, i);
                    HallOfFameSave_GetDate(hallOfFame, i, &wk->teams[i].date);
                    if (wk->teams[i].count != 0) {
                        for (j = 0; j < wk->teams[i].count; j++) {
                            HallOfFameSave_GetPokemon(hallOfFame, i, j, &wk->teams[i].pokemon[j]);
                        }
                        wk->pageMax++;
                    }
                }
            }
        }
        freeIntermediateSaveExtraBlksAfterLoad(save, SAVE_EXTRA_ID_HALL_OF_FAME);
    }
}

void DpcMain_FreeTeams(DendouPCWork *wk) {
    u16 i, j;

    for (i = 0; i < HALL_OF_FAME_TEAMS; i++) {
        for (j = 0; j < HALL_OF_FAME_TEAM_SIZE; j++) {
            GFL_StrBufFree(wk->teams[i].pokemon[j].nickname);
            GFL_StrBufFree(wk->teams[i].pokemon[j].trainerName);
        }
    }
}
