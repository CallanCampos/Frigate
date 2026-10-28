#include "types.h"
#include "app/pokedex_info.h"
#include "app/pokedex_nickname.h"
#include "app/pokedex_registration.h"
#include "app/pokedex_registration_graphic.h"
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
// parameter's name in the returns that queue it (ZukanToroku, Pokédex registration), and the file's strings

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
    PokedexRegistrationGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    u32 phase;
    u32 step;
    // The blend of the entry's fade, in sixteenths, and the frames until it changes
    u32 blend;
    u32 blendTimer;
    PokedexInfo *info;
    PokedexNickname *nickname;
    TCB *vblankTask;
    BOOL coverLoaded;
    // The characters of the cover, as GFL_BGSysLoadArcNCGRDynamic returns them
    u32 coverChars;
} PokedexRegistrationWork;

static BOOL PokedexRegistration_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexRegistration_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PokedexRegistration_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void PokedexRegistration_LoadCover(PokedexRegistrationWork *wk);
static void PokedexRegistration_FreeCover(PokedexRegistrationWork *wk);
static void PokedexRegistration_VBlank(TCB *tcb, void *data);

const GameProcFunctions POKEDEX_REGISTRATION_PROC_FUNCTIONS = { PokedexRegistration_Init, PokedexRegistration_Main,
                                                                PokedexRegistration_Exit };

static BOOL PokedexRegistration_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexRegistrationParam *prm = param;
    PokedexRegistrationWork *wk;
    BOOL cryOnly;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_OvlLoad(OVERLAY_ID(296));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_POKEDEX_REGISTRATION, 0x80000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PokedexRegistrationWork), HEAPID_POKEDEX_REGISTRATION);
    MI_CpuFill8(wk, 0, sizeof(PokedexRegistrationWork));
    wk->heapId = HEAPID_POKEDEX_REGISTRATION;
    wk->graphic = PokedexRegistrationGraphic_Create(1, wk->heapId);
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
    wk->info = PokedexInfo_CreateForPartyPkm(wk->heapId, prm->pkm, prm->nationalDex, TRUE, cryOnly, 0, 0,
                                             PokedexRegistrationGraphic_GetClActUnit(wk->graphic), wk->font,
                                             wk->printQueue, NULL, NULL, NULL);
    wk->nickname = PokedexNickname_Create(wk->heapId, prm->pkm, prm->boxMessage, prm->boxes, prm->box,
                                          PokedexRegistrationGraphic_GetClActUnit(wk->graphic), wk->font,
                                          wk->printQueue, prm->gameData);
    GFL_NetSetIconScreen(FALSE, wk->heapId);
    if (wk->phase == PHASE_NICKNAME) {
        PokedexRegistration_LoadCover(wk);
    }
    wk->vblankTask = GFL_VBlankTCBAdd(PokedexRegistration_VBlank, wk, 1);
    GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 2);
    return TRUE;
}

static BOOL PokedexRegistration_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexRegistrationWork *wk = work;

    GFL_TCBRemove(wk->vblankTask);
    PokedexRegistration_FreeCover(wk);
    PokedexNickname_Free(wk->nickname);
    PokedexInfo_Free(wk->info);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    PokedexRegistrationGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_POKEDEX_REGISTRATION);
    GFL_OvlUnload(OVERLAY_ID(296));
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL PokedexRegistration_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PokedexRegistrationWork *wk = work;
    PokedexRegistrationParam *prm = param;
    BOOL done = FALSE;
    BOOL pressed;
    BOOL touched;
    BOOL nicknameDone;

    PokedexInfo_Update(wk->info);
    nicknameDone = PokedexNickname_Update(wk->nickname);
    switch (wk->phase) {
    case PHASE_ENTRY:
        switch (wk->step) {
        case ENTRY_WAIT_FADE_IN:
            if (!GFL_FadeIsRunning()) {
                PokedexInfo_Start(wk->info);
                wk->step = ENTRY_WAIT_KEY;
            }
            break;
        case ENTRY_WAIT_KEY:
            pressed = FALSE;
            touched = GFL_HIDIsTouchPressed();
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
                pressed = TRUE;
            }
            if (touched) {
                pressed = TRUE;
                GCTX_HIDSetUsingTouch(TRUE);
            }
            if (pressed) {
                PokedexInfo_SetReady(wk->info);
                wk->step = ENTRY_WAIT_SLIDE;
            }
            break;
        case ENTRY_WAIT_SLIDE:
            if (PokedexInfo_IsSlidingIn(wk->info) || PokedexInfo_IsDone(wk->info)) {
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
                PokedexInfo_FreeEntry(wk->info);
                PokedexRegistration_LoadCover(wk);
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
            if (PokedexInfo_IsDone(wk->info)) {
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
                PokedexInfo_RequestCry(wk->info);
                wk->step = NICKNAME_START;
            }
            break;
        case NICKNAME_START:
            PokedexNickname_Start(wk->nickname);
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
                switch (PokedexNickname_GetAnswer(wk->nickname)) {
                case POKEDEX_NICKNAME_NO:
                    prm->nickname = FALSE;
                    break;
                case POKEDEX_NICKNAME_YES:
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
    PokedexRegistrationGraphic_Update(wk->graphic);
    PokedexRegistrationGraphic_Begin3D(wk->graphic);
    PokedexRegistrationGraphic_End3D(wk->graphic);
    return done;
}

// Loads the cover that follows the entry
static void PokedexRegistration_LoadCover(PokedexRegistrationWork *wk) {
    ArcTool *arc;

    wk->coverLoaded = TRUE;
    GFL_BGSysClearScr(3);
    GFL_BGSysSetBGPriority(1, 3);
    GFL_BGSysSetBGPriority(3, 0);
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, FILE_COVER_PALETTE, 0, 0, 0x20, wk->heapId);
    wk->coverChars = GFL_BGSysLoadArcNCGRDynamic(arc, FILE_COVER_CHARS, BG_COVER, COVER_CHARS_SIZE, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, FILE_COVER_SCREEN, BG_COVER, (u16)wk->coverChars, COVER_SCREEN_SIZE,
                                           FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(BG_COVER);
}

static void PokedexRegistration_FreeCover(PokedexRegistrationWork *wk) {
    GFL_BGSysFreeCharMemory(BG_COVER, CHAR_POS(wk->coverChars), CHAR_SIZE(wk->coverChars));
}

// Hides the entry's BGs once the cover is loaded
static void PokedexRegistration_VBlank(TCB *tcb, void *data) {
    PokedexRegistrationWork *wk = data;

    if (wk->coverLoaded) {
        GFL_BGSysSetBGEnabled(BG_ENTRY_TOP, FALSE);
        GFL_BGSysSetBGEnabled(BG_ENTRY_BOTTOM, FALSE);
        wk->coverLoaded = FALSE;
    }
}
