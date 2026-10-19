#include "types.h"
#include "app/zukan_info.h"
#include "app/zukan_nickname.h"
#include "app/zukan_toroku.h"
#include "app/zukan_toroku_graphic.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"

// The Pokédex registration: the Pokémon's entry, and then the nickname question. The name is a guess from the
// parameter's name in the returns that queue it ("toroku" is registration), and the file's strings

// The BGs of the cover that the entry gives way to, and the files that it loads from the Pokédex graphics
#define BG_COVER 1
#define FILE_COVER_PALETTE 7
#define FILE_COVER_CHARS 0x11
#define FILE_COVER_SCREEN 0x28
#define COVER_CHARS_SIZE 0x800
#define COVER_SCREEN_SIZE 0x600
// The BGs of the entry, hidden once the cover is shown
#define BG_ENTRY_TOP 0
#define BG_ENTRY_BOTTOM 2

// The BGs that the entry's fade blends
#define BLEND_BGS (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3)

// The two parts of the registration
enum {
    PHASE_ENTRY,
    PHASE_NICKNAME,
};

// The steps of the entry
enum {
    ENTRY_WAIT_FADE_IN,
    ENTRY_WAIT_KEY,
    ENTRY_WAIT_SLIDE,
    ENTRY_FADE_OUT,
    ENTRY_FADE_IN,
    ENTRY_WAIT_DONE,
};

// The steps of the nickname question
enum {
    NICKNAME_WAIT_FADE_IN,
    NICKNAME_START,
    NICKNAME_WAIT_ANSWER,
    NICKNAME_WAIT_FADE_OUT,
    NICKNAME_DONE,
};

typedef struct {
    u16 heapId;
    ZukanTorokuGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    u32 phase;
    u32 step;
    // The blend of the entry's fade, in sixteenths, and the frames until it changes
    u32 blend;
    u32 blendTimer;
    ZukanInfo *info;
    ZukanNickname *nickname;
    TCB *vblankTask;
    BOOL coverLoaded;
    // The characters of the cover, as GFL_BGSysLoadArcNCGRDynamic returns them
    u32 coverChars;
} ZukanTorokuWork;

static BOOL ZukanToroku_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanToroku_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL ZukanToroku_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void ZukanToroku_LoadCover(ZukanTorokuWork *wk);
static void ZukanToroku_FreeCover(ZukanTorokuWork *wk);
static void ZukanToroku_VBlank(TCB *tcb, void *data);

const GameProcFunctions ZUKAN_TOROKU_PROC_FUNCTIONS = { ZukanToroku_Init, ZukanToroku_Main, ZukanToroku_Exit };

static BOOL ZukanToroku_Init(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanTorokuParam *prm = param;
    ZukanTorokuWork *wk;
    BOOL cryOnly;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(296));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_ZUKAN_TOROKU, 0x80000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(ZukanTorokuWork), HEAPID_ZUKAN_TOROKU);
    MI_CpuFill8(wk, 0, sizeof(ZukanTorokuWork));
    wk->heapId = HEAPID_ZUKAN_TOROKU;
    wk->graphic = ZukanTorokuGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->heapId);
    if (prm->mode == 0) {
        wk->phase = PHASE_ENTRY;
        wk->step = ENTRY_WAIT_FADE_IN;
    } else {
        wk->phase = PHASE_NICKNAME;
        wk->step = NICKNAME_WAIT_FADE_IN;
    }
    cryOnly = FALSE;
    if (prm->mode != 0) {
        cryOnly = TRUE;
    }
    wk->info =
        func_ov296_0219d6e0(wk->heapId, prm->pkm, prm->nationalDex, TRUE, cryOnly, 0, 0,
                            ZukanTorokuGraphic_GetClActUnit(wk->graphic), wk->font, wk->printQueue, NULL, NULL, NULL);
    wk->nickname =
        ZukanNickname_Create(wk->heapId, prm->pkm, prm->boxMessage, prm->boxes, prm->box,
                             ZukanTorokuGraphic_GetClActUnit(wk->graphic), wk->font, wk->printQueue, prm->gameData);
    func_02042ba8(FALSE, wk->heapId);
    if (wk->phase == PHASE_NICKNAME) {
        ZukanToroku_LoadCover(wk);
    }
    wk->vblankTask = GFL_VBlankTCBAdd(ZukanToroku_VBlank, wk, 1);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2);
    return TRUE;
}

static BOOL ZukanToroku_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanTorokuWork *wk = work;

    GFL_TCBRemove(wk->vblankTask);
    ZukanToroku_FreeCover(wk);
    ZukanNickname_Free(wk->nickname);
    func_ov296_0219dbcc(wk->info);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    ZukanTorokuGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_ZUKAN_TOROKU);
    GFL_OvlUnload(OVERLAY_ID(296));
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL ZukanToroku_Main(GameProc *proc, u32 *state, void *param, void *work) {
    ZukanTorokuWork *wk = work;
    ZukanTorokuParam *prm = param;
    BOOL done = FALSE;
    BOOL pressed;
    BOOL touched;
    BOOL nicknameDone;

    func_ov296_0219dc74(wk->info);
    nicknameDone = ZukanNickname_Update(wk->nickname);
    switch (wk->phase) {
    case PHASE_ENTRY:
        switch (wk->step) {
        case ENTRY_WAIT_FADE_IN:
            if (!GFL_FadeIsRunning()) {
                func_ov296_0219de44(wk->info);
                wk->step = ENTRY_WAIT_KEY;
            }
            break;
        case ENTRY_WAIT_KEY:
            pressed = FALSE;
            touched = func_0203da48();
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                func_0203d564(FALSE);
                pressed = TRUE;
            }
            if (touched) {
                pressed = TRUE;
                func_0203d564(TRUE);
            }
            if (pressed) {
                func_ov296_0219de14(wk->info);
                wk->step = ENTRY_WAIT_SLIDE;
            }
            break;
        case ENTRY_WAIT_SLIDE:
            if (func_ov296_0219de24(wk->info) || func_ov296_0219de34(wk->info)) {
                wk->blendTimer = 0;
                wk->blend = 16;
                wk->step = ENTRY_FADE_OUT;
            }
            break;
        case ENTRY_FADE_OUT:
            wk->blendTimer++;
            if (wk->blendTimer == 4) {
                wk->blendTimer = 0;
                wk->blend--;
            }
            G2x_SetBlendBrightnessExt_(REG_BLDCNT_ADDR, BLEND_BGS, BLEND_BGS, wk->blend, 0, wk->blend - 16);
            if (wk->blend == 0) {
                func_ov296_0219ddcc(wk->info);
                ZukanToroku_LoadCover(wk);
                wk->step = ENTRY_FADE_IN;
            }
            break;
        case ENTRY_FADE_IN:
            wk->blendTimer++;
            if (wk->blendTimer == 4) {
                wk->blendTimer = 0;
                wk->blend++;
            }
            G2x_SetBlendBrightnessExt_(REG_BLDCNT_ADDR, BLEND_BGS, BLEND_BGS, wk->blend, 0, wk->blend - 16);
            if (wk->blend == 16) {
                wk->step = ENTRY_WAIT_DONE;
            }
            break;
        case ENTRY_WAIT_DONE:
            if (func_ov296_0219de34(wk->info)) {
                wk->phase = PHASE_NICKNAME;
                wk->step = NICKNAME_START;
            }
            break;
        }
        break;
    case PHASE_NICKNAME:
        switch (wk->step) {
        case NICKNAME_WAIT_FADE_IN:
            if (!GFL_FadeIsRunning()) {
                func_ov296_0219de1c(wk->info);
                wk->step = NICKNAME_START;
            }
            break;
        case NICKNAME_START:
            ZukanNickname_Start(wk->nickname);
            wk->step = NICKNAME_WAIT_ANSWER;
            break;
        case NICKNAME_WAIT_ANSWER:
            if (nicknameDone == TRUE) {
                GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 2);
                wk->step = NICKNAME_WAIT_FADE_OUT;
            }
            break;
        case NICKNAME_WAIT_FADE_OUT:
            if (!GFL_FadeIsRunning()) {
                switch (ZukanNickname_GetAnswer(wk->nickname)) {
                case ZUKAN_NICKNAME_NO:
                    prm->nickname = FALSE;
                    break;
                case ZUKAN_NICKNAME_YES:
                    prm->nickname = TRUE;
                    break;
                }
                wk->step = NICKNAME_DONE;
                done = TRUE;
            }
            break;
        }
        break;
    }
    PrintQueue_Main(wk->printQueue);
    ZukanTorokuGraphic_Update(wk->graphic);
    ZukanTorokuGraphic_Begin3D(wk->graphic);
    ZukanTorokuGraphic_End3D(wk->graphic);
    return done;
}

// Loads the cover that follows the entry
static void ZukanToroku_LoadCover(ZukanTorokuWork *wk) {
    ArcTool *arc;

    wk->coverLoaded = TRUE;
    GFL_BGSysClearScr(3);
    GFL_BGSysSetBGPriority(1, 3);
    GFL_BGSysSetBGPriority(3, 0);
    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, FILE_COVER_PALETTE, 0, 0, 0x20, wk->heapId);
    wk->coverChars = GFL_BGSysLoadArcNCGRDynamic(arc, FILE_COVER_CHARS, BG_COVER, COVER_CHARS_SIZE, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, FILE_COVER_SCREEN, BG_COVER, (u16)wk->coverChars, COVER_SCREEN_SIZE,
                                           FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_COVER);
}

static void ZukanToroku_FreeCover(ZukanTorokuWork *wk) {
    GFL_BGSysFreeCharMemory(BG_COVER, CHAR_POS(wk->coverChars), CHAR_SIZE(wk->coverChars));
}

// Hides the entry's BGs once the cover is loaded
static void ZukanToroku_VBlank(TCB *tcb, void *data) {
    ZukanTorokuWork *wk = data;

    if (wk->coverLoaded) {
        GFL_BGSysSetBGEnabled(BG_ENTRY_TOP, FALSE);
        GFL_BGSysSetBGEnabled(BG_ENTRY_BOTTOM, FALSE);
        wk->coverLoaded = FALSE;
    }
}
