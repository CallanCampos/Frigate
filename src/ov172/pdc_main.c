// The Entree Forest capture's scene (overlay 172): the battle view's graphics, the Pokémon of the dream appearing, the
// choice between the bag and leaving, and the throw of a Poké Ball. Its seq is the step of the current proc. The name is
// the ROM's string, from GFL_HeapAllocate's assert

#include "types.h"
#include "app/pdc_main.h"
#include "app/pdc_msg.h"
#include "battle/b_bag_main.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_input.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/trainer_data_savedata.h"
#include "system/gf_font.h"

// The effects of the battle view that the scene plays: the Pokémon appearing, and its entrance
#define PDC_EFFECT_APPEAR 0x231
#define PDC_EFFECT_APPEAR_END 0x23e

// The proc's return when the player leaves
#define PDC_RESULT_CAUGHT 1
#define PDC_RESULT_LEFT 2

typedef u32 (*PdcMainProc)(PdcMain *work);

struct PdcMain {
    PdcWork *pdc;
    PartyPkm *pkm;
    Font *font;
    Font *font2;
    BtlvInput *input;
    PdcMsg *msg;
    TCBExManager *tcbManager;
    BBagParam bag;
    u32 seq;
    s32 timer;
    PdcMainProc proc;
    HeapID heapId;
    void *bagCursor;
    u8 keyCursorFlag;
};

// The steps of the bag proc
enum {
    PDC_BAG_START,
    PDC_BAG_WAIT_FADE_OUT,
    PDC_BAG_WAIT_BAG,
    PDC_BAG_WAIT_FADE_IN_NO_BALL,
    PDC_BAG_WAIT_FADE_IN_BALL,
    PDC_BAG_WAIT_CALL,
    PDC_BAG_WAIT_THROW,
    PDC_BAG_WAIT_JOINED,
    PDC_BAG_WAIT_JINGLE,
    PDC_BAG_WAIT_JINGLE_END,
    PDC_BAG_WAIT_EFFECT_END,
    PDC_BAG_WAIT_KEY,
    PDC_BAG_WAIT_FADE_OUT_END,
};

static void PdcMain_SetProc(PdcMain *work, PdcMainProc proc);
static void PdcMain_InitGraphics(HeapID heapId);
static void PdcMain_ExitGraphics(void);
static u32 PdcMain_ProcIntro(PdcMain *work);
static u32 PdcMain_ProcAsk(PdcMain *work);
static u32 PdcMain_ProcBag(PdcMain *work);
static u32 PdcMain_ProcLeave(PdcMain *work);
static void PdcMain_FadeIn(u32 slowness);

static const s32 sButtonFlags[2] = { 0, 0 };
static const s32 sButtonFlashRows[2] = { 4, 16 };
static const TouchRect sButtonRects[3] = {
    { 144, 192, 0, 80 },
    { 152, 192, 88, 168 },
    { TOUCH_RECT_END, 0, 0, 0 },
};
static const BtlvInputButtonSet sButtonSet = { sButtonRects, sButtonFlags, sButtonFlashRows };

static const BtlvInputKeyStop sKeyStops[2] = {
    { { 0, 0, 0, 0, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 1, 0, KEY_CURSOR_NONE },
    { { 1, 1, 1, 1, -1, -1 }, KEY_CURSOR_NONE, KEY_CURSOR_NONE, 0, KEY_CURSOR_NONE, 1, KEY_CURSOR_NONE },
};

static const ClActSysSetup sClActSysSetup = {
    0, 0, 0, 512, 4, 124, 4, 124, 0, 56, 48, 48, 48, 16, 16,
};

PdcMain *PdcMain_Create(PdcWork *pdc, HeapID heapId) {
    PdcMain *work = GFL_HeapAllocate(heapId, sizeof(PdcMain), TRUE, "pdc_main.c", 94);
    BtlvEffectSetup *setup;
    GameData *gameData;

    work->heapId = heapId;
    work->pdc = pdc;
    PdcMain_InitGraphics(work->heapId);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->font2 = GFL_FontCreate(ARCID_FONT, 2, 0, FALSE, work->heapId);
    work->seq = 0;
    u16 trainerTypes[4] = { 0, 0, 0xffff, 0xffff };
    setup = BtlvEffect_Create(0, 0, (const BtlFieldEnv *)PdcWork_GetFieldStatus(pdc), FALSE, trainerTypes, NULL, NULL,
                              work->heapId);
    BtlvEffect_Init(setup, work->font2, work->heapId);
    GFL_HeapFree(setup);
    gameData = PdcWork_GetGameData(pdc);
    work->input = BtlvInput_CreateSimple(gameData, 0, BtlvEffect_GetPaletteFade(), work->font, &work->keyCursorFlag,
                                         work->heapId);
    work->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 0x40, 0x80);
    work->pkm = PdcWork_GetPkm(pdc);
    work->msg = PdcMsg_Create(work->font, &PdcWork_GetTrainerData(pdc)->config, work->tcbManager, work->heapId);
    work->bagCursor = BagCursor_Create(work->heapId);
    PdcMain_SetProc(work, PdcMain_ProcIntro);
    return work;
}

u32 PdcMain_Update(PdcMain *work) {
    GFL_TCBExMgrUpdate(work->tcbManager);
    BtlvEffect_Main();
    BtlvInput_Update(work->input);
    if (work->proc != NULL) {
        return work->proc(work);
    }
    return 0;
}

void PdcMain_Free(PdcMain *work) {
    GFL_TCBExMgrFree(work->tcbManager);
    PdcMsg_Free(work->msg);
    BtlvEffect_Exit();
    BtlvInput_Delete(work->input);
    GFL_FontFree(work->font);
    GFL_FontFree(work->font2);
    PdcMain_ExitGraphics();
    GFL_HeapFree(work->bagCursor);
    GFL_HeapFree(work);
}

static void PdcMain_SetProc(PdcMain *work, PdcMainProc proc) {
    work->seq = 0;
    work->proc = proc;
}

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_3, GX_BG0_AS_3D };
static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_D,  GX_VRAM_BGEXTPLTT_NONE, GX_VRAM_SUB_BG_128_C,  GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I, GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_01_FG,  GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static void PdcMain_InitGraphics(HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    func_020232d0();
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, heapId, NULL);
    GFL_G3DSysSetSwapBufferParams(1, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    G3X_AntiAlias(TRUE);
    gfxSetFog(FALSE, 0, 0, 0);
    G2_SetBG0Priority(1);
    ClActSys_Create(&sClActSysSetup, &sVRAMConfig, heapId);
    GFL_BGSysSet3DBGPriority(1);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void PdcMain_ExitGraphics(void) {
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    func_0204b758();
    GFL_G3DSysFree();
}

// The Pokémon appears and the messages introduce it
static u32 PdcMain_ProcIntro(PdcMain *work) {
    switch (work->seq) {
    case 0:
        PdcMsg_StartHide(work->msg);
        work->seq++;
        break;
    case 1:
        if (PdcMsg_IsHideDone(work->msg)) {
            BtlvEffect_AddPokemon(work->pkm, 1);
            BtlvEffect_Start(PDC_EFFECT_APPEAR);
            PdcMain_FadeIn(2);
            work->seq++;
        }
        break;
    case 2:
        if (!BtlvEffect_IsBusy()) {
            PdcMsg_StartSawDream(work->msg, work->pdc);
            work->seq++;
        }
        break;
    case 3:
        if (PdcMsg_Step(work->msg)) {
            BtlvEffect_AddGaugeByPkm(PdcWork_GetPokedex(work->pdc), PdcWork_GetPkm(work->pdc), 1);
            PdcMsg_StartHide(work->msg);
            work->seq++;
        }
        break;
    case 4:
        if (PdcMsg_IsHideDone(work->msg)) {
            BtlvEffect_Start(PDC_EFFECT_APPEAR_END);
            work->seq++;
        }
        break;
    case 5:
        if (!BtlvEffect_IsBusy()) {
            PdcMain_SetProc(work, PdcMain_ProcAsk);
        }
        break;
    }
    return 0;
}

// The choice between the bag and leaving
static u32 PdcMain_ProcAsk(PdcMain *work) {
    switch (work->seq) {
    case 0:
        PdcMsg_StartWhatWill(work->msg, work->pdc);
        work->seq++;
        break;
    case 1:
        if (PdcMsg_Step(work->msg)) {
            BtlvInput_SetScreen(work->input, 7, NULL);
            work->seq++;
        }
        break;
    case 2: {
        s32 button = BtlvInput_CheckInput(work->input, &sButtonSet, sKeyStops);

        if (button != -1) {
            if (button != 0) {
                PdcMain_SetProc(work, PdcMain_ProcLeave);
            } else {
                PdcMain_SetProc(work, PdcMain_ProcBag);
            }
        }
        break;
    }
    }
    return 0;
}

// The bag, the throw of the ball and the Pokémon joining the party
static u32 PdcMain_ProcBag(PdcMain *work) {
    switch (work->seq) {
    case PDC_BAG_START:
        work->bag.bag = PdcWork_GetBag(work->pdc);
        work->bag.bagCursor = work->bagCursor;
        work->bag.mode = 3;
        work->bag.font = work->font;
        work->bag.heapId = work->heapId;
        work->bag.item = 0;
        work->bag.quit = FALSE;
        work->bag.shooterEnergy = 0;
        work->bag.shooterSpent = 0;
        work->bag.done = FALSE;
        work->bag.usingKeys = &work->keyCursorFlag;
        work->bag.playSound = TRUE;
        BtlvInput_StartPaletteFade(work->input, 1);
        GFL_OvlLoad(OVERLAY_ID(285));
        work->seq++;
        break;
    case PDC_BAG_WAIT_FADE_OUT:
        if (!BtlvInput_IsFading(work->input)) {
            GFL_OvlLoad(OVERLAY_ID(286));
            BBagMain_Start(&work->bag);
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_BAG:
        if (work->bag.done) {
            GFL_OvlUnload(OVERLAY_ID(285));
            GFL_OvlUnload(OVERLAY_ID(286));
            BtlvInput_StartFadeInWithGraphics(work->input);
            if (work->bag.item == 0) {
                work->seq = PDC_BAG_WAIT_FADE_IN_NO_BALL;
            } else {
                BagSave_SubItem(PdcWork_GetBag(work->pdc), work->bag.item, 1, work->heapId);
                work->seq = PDC_BAG_WAIT_FADE_IN_BALL;
            }
        }
        break;
    case PDC_BAG_WAIT_FADE_IN_NO_BALL:
        if (!BtlvInput_IsFading(work->input)) {
            PdcMain_SetProc(work, PdcMain_ProcAsk);
        }
        break;
    case PDC_BAG_WAIT_FADE_IN_BALL:
        if (!BtlvInput_IsFading(work->input)) {
            PdcMsg_StartComeHere(work->msg, work->pdc);
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_CALL:
        if (PdcMsg_Step(work->msg)) {
            BtlvEffect_StartEffect23A(1, work->bag.item, 4, 1, 0);
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_THROW:
        if (!BtlvEffect_IsBusy()) {
            PdcMsg_StartJoinedParty(work->msg, work->pdc);
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_JOINED:
        if (PdcMsg_ConsumeFinished(work->msg)) {
            GFL_SndBGMPlay(SEQ_ME_POKEGET, 0xffff);
        }
        if (PdcMsg_Step(work->msg)) {
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_JINGLE:
        if (!GFL_SndBGMIsPlaying()) {
            BtlvEffect_Resume();
            GFL_SndBGMPlay(SEQ_BGM_WIN1, 0xffff);
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_JINGLE_END:
        if (!BtlvEffect_IsBusy()) {
            work->timer = 80;
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_EFFECT_END:
        if ((GCTX_HIDGetPressedKeys() & 3) || func_0203da48() || --work->timer == 0) {
            GFL_FadeSet(3, 0, 16, 2);
            work->seq++;
        }
        break;
    case PDC_BAG_WAIT_KEY:
        if (!GFL_FadeIsRunning()) {
            u16 ball = PML_ItemGetMonsBallID(work->bag.item);

            PokeParty_SetParam(PdcWork_GetPkm(work->pdc), 0x98, ball);
            return PDC_RESULT_CAUGHT;
        }
        break;
    }
    return 0;
}

// The player leaves
static u32 PdcMain_ProcLeave(PdcMain *work) {
    switch (work->seq) {
    case 0:
        PdcMsg_StartLeft(work->msg, work->pdc);
        work->seq++;
        break;
    case 1:
        if (PdcMsg_Step(work->msg)) {
            GFL_FadeSet(3, 0, 16, 2);
            GFL_SndBGMFadeOut(8);
            work->seq++;
        }
        break;
    case 2:
        if (!GFL_FadeIsRunning()) {
            return PDC_RESULT_LEFT;
        }
        break;
    }
    return 0;
}

// Fades in from black, or from white if the screens were white
static void PdcMain_FadeIn(u32 slowness) {
    if (gfxRegGetMasterBrightness(REG_MASTER_BRIGHT_ADDR) <= 0) {
        GFL_FadeSet(3, 16, 0, slowness);
    } else {
        GFL_FadeSet(12, 16, 0, slowness);
    }
}
