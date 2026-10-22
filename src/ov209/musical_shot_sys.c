#include "types.h"
#include "app/musical/mus_shot_info.h"
#include "app/musical/mus_shot_photo.h"
#include "app/musical/musical_shot_sys.h"
#include "constants/species.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/rtc.h"
#include "save/save_control.h"
#include "system/wipe.h"

// Overlay 209's musical_shot_sys.c: the proc that shows a musical's photo, with its information on the touch screen

typedef struct {
    HeapID heapId;
    TCB *vblankTcb;
    MusicalShotParam *param;
    MusShotPhoto *photo;
    MusShotInfo *info;
} MusicalShotSys;

static BOOL MusicalShotSys_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MusicalShotSys_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MusicalShotSys_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MusicalShotSys_VBlank(TCB *tcb, void *data);
static void MusicalShotSys_InitGraphics(MusicalShotSys *sys);
static void MusicalShotSys_ExitGraphics(MusicalShotSys *sys);

GameProcFunctions MUSICAL_SHOT_PROC_FUNCTIONS = {
    MusicalShotSys_Init,
    MusicalShotSys_Main,
    MusicalShotSys_Exit,
};

static const BGSysLCDConfig MUSICAL_SHOT_LCD_CONFIG = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

static const BGSysVRAMConfig MUSICAL_SHOT_VRAM_CONFIG = {
    GX_VRAM_BG_128_D,    GX_VRAM_BGEXTPLTT_23_G,  GX_VRAM_SUB_BG_32_H,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_16_F,    GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_012_ABC, GX_VRAM_TEXPLTT_0123_E,  GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static BOOL MusicalShotSys_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalShotParam *shotParam = param;
    MusicalShotSys *sys;
    MusicalShot *shot;
    RTCDate date;
    u8 i, j;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MUSICAL_SHOT, 0x80000);
    sys = GFL_ProcInitSubsystem(proc, sizeof(MusicalShotSys), HEAPID_MUSICAL_SHOT);
    sys->heapId = HEAPID_MUSICAL_SHOT;
    if (shotParam == NULL) {
        // Started without a parameter, from a debug menu: a made-up photo of four Reshiram
        GFL_OvlLoad(OVERLAY_ID(211));
        GFL_OvlLoad(OVERLAY_ID(210));
        sys->param = GFL_HeapAllocate(HEAPID_MUSICAL_SHOT, sizeof(MusicalShotParam), FALSE, "musical_shot_sys.c", 114);
        sys->param->shot = GFL_HeapAllocate(HEAPID_MUSICAL_SHOT, sizeof(MusicalShot), TRUE, "musical_shot_sys.c", 115);
        sys->param->loadComm = FALSE;
        sys->param->loadData = TRUE;
        shot = sys->param->shot;
        RTC_GetCachedDate(&date);
        shot->unk0_0 = 3;
        shot->tops = 2;
        shot->year = date.year;
        shot->month = date.month;
        shot->day = date.day;
        // "ポケッターリモンスターリ"
        shot->title[0] = 0x30dd;
        shot->title[1] = 0x30b1;
        shot->title[2] = 0x30c3;
        shot->title[3] = 0x30bf;
        shot->title[4] = 0x30fc;
        shot->title[5] = 0x30ea;
        shot->title[6] = 0x30e2;
        shot->title[7] = 0x30f3;
        shot->title[8] = 0x30b9;
        shot->title[9] = 0x30bf;
        shot->title[10] = 0x30fc;
        shot->title[11] = 0x30ea;
        shot->title[12] = GFL_StrBufGetTerminator();
        shot->pokes[0].species = SPECIES_RESHIRAM;
        shot->pokes[1].species = SPECIES_RESHIRAM;
        shot->pokes[2].species = SPECIES_RESHIRAM;
        shot->pokes[3].species = SPECIES_RESHIRAM;
        for (i = 0; i < 4; i++) {
            // "トレーナ１" to "トレーナ４"
            shot->pokes[i].name[0] = 0x30c8;
            shot->pokes[i].name[1] = 0x30ec;
            shot->pokes[i].name[2] = 0x30fc;
            shot->pokes[i].name[3] = 0x30ca;
            shot->pokes[i].name[4] = 0xff11 + i;
            shot->pokes[i].name[5] = 0;
            for (j = 0; j < 8; j++) {
                shot->pokes[i].equips[j].itemId = 0xff;
                shot->pokes[i].equips[j].rotation = 0;
                shot->pokes[i].equips[j].pos = 10;
            }
            shot->pokes[i].equips[0].itemId = 11;
            shot->pokes[i].equips[0].rotation = 0;
            shot->pokes[i].equips[0].pos = 7;
            shot->pokes[i].equips[1].itemId = 11;
            shot->pokes[i].equips[1].rotation = 0;
            shot->pokes[i].equips[1].pos = 8;
            shot->pokes[i].equips[2].itemId = 11;
            shot->pokes[i].equips[2].rotation = 0;
            shot->pokes[i].equips[2].pos = 6;
            shot->pokes[i].equips[3].itemId = 11;
            shot->pokes[i].equips[3].rotation = 0;
            shot->pokes[i].equips[3].pos = 4;
            shot->pokes[i].equips[4].itemId = 11;
            shot->pokes[i].equips[4].rotation = 0;
            shot->pokes[i].equips[4].pos = 2;
            shot->pokes[i].equips[5].itemId = 11;
            shot->pokes[i].equips[5].rotation = 0;
            shot->pokes[i].equips[5].pos = 1;
            shot->pokes[i].equips[6].itemId = 11;
            shot->pokes[i].equips[6].rotation = 0;
            shot->pokes[i].equips[6].pos = 0;
        }
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_R) {
            sys->param->askSave = TRUE;
        } else {
            sys->param->askSave = FALSE;
        }
        sys->param->save = NULL;
    } else {
        sys->param = shotParam;
        if (shotParam->loadData == TRUE) {
            GFL_OvlLoad(OVERLAY_ID(210));
        }
    }
    if (sys->param->loadComm == TRUE) {
        GFL_OvlLoad(OVERLAY_ID(211));
    }
    sys->vblankTcb = GFL_VBlankTCBAdd(MusicalShotSys_VBlank, sys, 0x40);
    MusicalShotSys_InitGraphics(sys);
    sys->photo = MusShotPhoto_Create(sys->param->shot, sys->heapId);
    sys->info =
        MusShotInfo_Create(sys->param->shot, sys->param->save, sys->param->askSave, sys->param->comm, sys->heapId);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, sys->heapId);
    return TRUE;
}

static BOOL MusicalShotSys_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalShotSys *sys = work;

    if (GFL_WipeIsFinished() == FALSE) {
        return FALSE;
    }
    GX_SetMasterBrightness(-16);
    GXS_SetMasterBrightness(-16);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0, 0, 31, 31);
    MusShotInfo_Delete(sys->info);
    MusShotPhoto_Delete(sys->photo);
    MusicalShotSys_ExitGraphics(sys);
    GFL_TCBRemove(sys->vblankTcb);
    if (sys->param->loadComm == TRUE) {
        GFL_OvlUnload(OVERLAY_ID(211));
    }
    if (sys->param->loadData == TRUE) {
        GFL_OvlUnload(OVERLAY_ID(210));
    }
    if (param == NULL) {
        GFL_HeapFree(sys->param->shot);
        GFL_HeapFree(sys->param);
        GFL_OvlUnload(OVERLAY_ID(211));
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MUSICAL_SHOT);
    return TRUE;
}

static BOOL MusicalShotSys_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalShotSys *sys = work;

    MusShotPhoto_Main(sys->photo);
    MusShotInfo_Main(sys->info);
    ClActSys_Draw();
    if (MusShotInfo_IsFinished(sys->info) == TRUE) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, sys->heapId);
        return TRUE;
    }
    return FALSE;
}

static void MusicalShotSys_VBlank(TCB *tcb, void *data) {
    ClActSys_VBlank();
}

static void MusicalShotSys_InitGraphics(MusicalShotSys *sys) {
    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
    Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetVRAMBanks(&MUSICAL_SHOT_VRAM_CONFIG);
    GFL_BGSysCreate(sys->heapId);
    BmpWin_InitAllocator(sys->heapId);
    GFL_BGSysSetLCDConfig(&MUSICAL_SHOT_LCD_CONFIG);
    ClActSys_Create(&data_02093f08, &MUSICAL_SHOT_VRAM_CONFIG, sys->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void MusicalShotSys_ExitGraphics(MusicalShotSys *sys) {
    ClActSys_Delete();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}
