#include "field/encounter_effect_capture.h"
#include "types.h"
#include "field/encounter_effect.h"
#include "field/field.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nnsys/gfd.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The capture is a 256x256 direct color texture in a whole VRAM bank
#define CAPTURE_TEX_SIZE (256 * 256 * sizeof(u16))
// The size of a VRAM bank of A to D in the texture image slots
#define TEX_BANK_SIZE 0x20000

static GameEventReturnCode EncEffCaptureFlash_Callback(GameEvent *event, u32 *state, void *data);
static void EncEffCapture_StartCapture(EncEffCaptureWork *work);
static void EncEffCapture_CaptureTask(TCB *tcb, void *data);
static NNSGfdTexKey EncEffCapture_GetTexKey(void);
static GameEventReturnCode EncEffCapture_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *EncEffCapture_CreateFlashEvent(GameSystem *gsys, const VecFx32 *pos, GameEvent *(*init)(GameSystem *gsys),
                                          void (*render)(EncEffGrid *grid)) {
    GameEvent *event;
    EncEffCaptureWork *work;

    event = GameEvent_Create(gsys, NULL, EncEffCaptureFlash_Callback, sizeof(EncEffCaptureWork));
    work = GameEvent_GetData(event);
    sys_memset(work, 0, sizeof(EncEffCaptureWork));
    work->camPos = *pos;
    work->init = init;
    work->render = render;
    return event;
}

GameEvent *EncEffCapture_CreateEvent(GameSystem *gsys, const VecFx32 *pos, GameEvent *(*init)(GameSystem *gsys),
                                     void (*render)(EncEffGrid *grid)) {
    GameEvent *event;
    EncEffCaptureWork *work;

    event = GameEvent_Create(gsys, NULL, EncEffCapture_Callback, sizeof(EncEffCaptureWork));
    work = GameEvent_GetData(event);
    sys_memset(work, 0, sizeof(EncEffCaptureWork));
    work->camPos = *pos;
    work->init = init;
    work->render = render;
    return event;
}

// Sets up the camera and the captured screen's texture, then calls the effect's render function
void EncEffCapture_Draw(void *data) {
    EncEffCaptureWork *work = data;

    gfxPerspective(FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)), FX32_ONE * 4 / 3, FX32_ONE, FX32_CONST(1024),
                   FX32_ONE, TRUE, NULL);
    gfxReset3D();
    {
        VecFx32 camUp = { 0, FX32_ONE, 0 };
        VecFx32 target = { 0, 0, 0 };
        VecFx32 camPos = work->camPos;

        gfxLookAt(&camPos, &camUp, &target, TRUE, NULL);
    }
    G3_TexImageParam(GX_TEXFMT_DIRECT, GX_TEXGEN_TEXCOORD, GX_TEXSIZE_S256, GX_TEXSIZE_T256, GX_TEXREPEAT_NONE,
                     GX_TEXFLIP_NONE, GX_TEXPLTTCOLOR0_USE, NNS_GfdGetTexKeyAddr(work->texKey));
    work->render(work->effectWork);
}

static GameEventReturnCode EncEffCaptureFlash_Callback(GameEvent *event, u32 *state, void *data) {
    EncEffCaptureWork *work = data;
    GameSystem *gsys;
    Field *field;
    GameEvent *next;

    gsys = GameEvent_GetGameSystem(event);
    field = GSYS_GetField(gsys);
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_WHITE, 0, 16, 0);
        (*state)++;
        // Fall through to check the fade in the same frame
    case 1:
        if (!GFL_FadeIsRunning()) {
            GFL_FadeSet(FADE_ENGINE_A_WHITE, 16, 0, 0);
            (*state)++;
        }
        break;
    case 2:
        if (!GFL_FadeIsRunning()) {
            work->flashes++;
            if (work->flashes < 2) {
                GFL_FadeSet(FADE_ENGINE_A_WHITE, 0, 16, 0);
                *state = 1;
            } else {
                (*state)++;
            }
        }
        break;
    case 3:
        EncEffCapture_StartCapture(work);
        (*state)++;
        break;
    case 4:
        if (work->captured) {
            GFL_BGSysSetBGPriority(0, 0);
            GFL_BGSysSetBGPriority(2, 3);
            work->renderMode = Field_GetRenderMode(field);
            work->texBanks = gfxGetTextureBanks();
            work->addedTexBank = FALSE;
            if (!(work->texBanks & GX_VRAM_D)) {
                work->texBanks |= GX_VRAM_D;
                work->addedTexBank = TRUE;
            }
            gfxDisableLCDCBanks();
            gfxSetTextureBanks(work->texBanks);
            work->texKey = EncEffCapture_GetTexKey();
            (*state)++;
        }
        break;
    case 5:
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysSetBGEnabled(0, TRUE);
        Field_SetRenderMode(field, 2);
        next = work->init(gsys);
        work->effectWork = EncEff_GetWorkArea(Field_GetEncEff(field));
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 6:
        if (work->addedTexBank == TRUE) {
            work->texBanks &= ~GX_VRAM_D;
            gfxSetTextureBanks(work->texBanks);
        }
        Field_SetRenderMode(field, work->renderMode);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Turns bank D over to the LCDC and captures the 3D screen to it at the next V-blank
static void EncEffCapture_StartCapture(EncEffCaptureWork *work) {
    work->captured = FALSE;
    gfxSetLCDCBanks(GX_VRAM_D);
    GFL_VBlankTCBAdd(EncEffCapture_CaptureTask, &work->captured, 0);
}

static void EncEffCapture_CaptureTask(TCB *tcb, void *data) {
    BOOL *captured = data;

    GX_SetCapture(GX_CAPTURE_SIZE_256x192, GX_CAPTURE_MODE_A, GX_CAPTURE_SRCA_3D, GX_CAPTURE_SRCB_VRAM_0x00000,
                  GX_CAPTURE_DEST_VRAM_D_0x00000, 16, 0);
    GFL_TCBRemove(tcb);
    if (captured != NULL) {
        *captured = TRUE;
    }
}

// The key of bank D in the texture VRAM: the texture image slots are the texture banks in order
static NNSGfdTexKey EncEffCapture_GetTexKey(void) {
    u32 texBanks = gfxGetTextureBanks();
    u32 offset = 0;

    // BUG: `|=` for `&`, which is always true, so banks A to C are all counted, texture banks or not. This is right
    // only while they all hold textures (system/screentex.c has the same bug)
#ifdef BUGFIX
    if (texBanks & GX_VRAM_C) {
#else
    if (texBanks |= GX_VRAM_C) {
#endif
        offset += TEX_BANK_SIZE;
    }
#ifdef BUGFIX
    if (texBanks & GX_VRAM_B) {
#else
    if (texBanks |= GX_VRAM_B) {
#endif
        offset += TEX_BANK_SIZE;
    }
#ifdef BUGFIX
    if (texBanks & GX_VRAM_A) {
#else
    if (texBanks |= GX_VRAM_A) {
#endif
        offset += TEX_BANK_SIZE;
    }
    return NNS_GfdMakeTexKey(offset, CAPTURE_TEX_SIZE, FALSE);
}

static GameEventReturnCode EncEffCapture_Callback(GameEvent *event, u32 *state, void *data) {
    EncEffCaptureWork *work = data;
    GameSystem *gsys;
    Field *field;
    GameEvent *next;

    gsys = GameEvent_GetGameSystem(event);
    field = GSYS_GetField(gsys);
    switch (*state) {
    case 0:
        EncEffCapture_StartCapture(work);
        (*state)++;
        break;
    case 1:
        if (work->captured) {
            GFL_BGSysSetBGPriority(0, 0);
            GFL_BGSysSetBGPriority(2, 3);
            work->renderMode = Field_GetRenderMode(field);
            work->texBanks = gfxGetTextureBanks();
            work->addedTexBank = FALSE;
            if (!(work->texBanks & GX_VRAM_D)) {
                work->texBanks |= GX_VRAM_D;
                work->addedTexBank = TRUE;
            }
            gfxDisableLCDCBanks();
            gfxSetTextureBanks(work->texBanks);
            work->texKey = EncEffCapture_GetTexKey();
            (*state)++;
        }
        break;
    case 2:
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysSetBGEnabled(0, TRUE);
        Field_SetRenderMode(field, 2);
        next = work->init(gsys);
        work->effectWork = EncEff_GetWorkArea(Field_GetEncEff(field));
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 3:
        if (work->addedTexBank == TRUE) {
            work->texBanks &= ~GX_VRAM_D;
            gfxSetTextureBanks(work->texBanks);
        }
        Field_SetRenderMode(field, work->renderMode);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
