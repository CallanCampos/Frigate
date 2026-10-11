#include "types.h"
#include "app/ui/ui_scene.h"
#include "app/zukan_detail.h"
#include "app/zukan_info.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nnsys/g2d.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/gf_font.h"

// The detail screen's info page, which the ROM does not name: the Pokémon's entry on both screens, from overlay 296,
// with buttons for the languages it has been seen in, which show the entry on the sub screen in that language

#define LANGUAGE_BUTTON_COUNT 6
// No language button is pushed, or the entry is in the game's language
#define LANGUAGE_NONE LANGUAGE_BUTTON_COUNT

// The Pokédex's shortcut for the Y button, which the touch bar's check box registers
#define SHORTCUT_POKEDEX 20

#define SPECIES_SPINDA 327

enum {
    INFO_SEQ_INIT,
    INFO_SEQ_FADE_IN,
    INFO_SEQ_WAIT_FADE_IN,
    INFO_SEQ_MAIN,
    INFO_SEQ_WAIT_FADE_OUT,
    INFO_SEQ_EXIT,
};

// How the page ends
enum {
    INFO_EXIT_NONE,
    INFO_EXIT_PAGE,
    INFO_EXIT_SCREEN,
};

// A language button's animation
enum {
    BUTTON_IDLE,
    BUTTON_PUSHED,
    BUTTON_WAIT_ANIM,
    BUTTON_DONE,
};

typedef struct {
    u8 x;
    u8 y;
    u8 rect[4];
    u8 anim;
    u8 pushedAnim;
    u8 language;
} LanguageButtonData;

typedef struct {
    ClActor *actor;
    // The touch area: x, y, width and height
    u8 rect[4];
    u16 anim;
    u16 pushedAnim;
    u8 language;
    int state;
} LanguageButton;

typedef struct {
    TCB *vblankTcb;
    ZukanInfo *infoMain;
    ZukanInfo *infoSub;
    ZukanDetailBlend *blendMain;
    ZukanDetailBlend *blendSub;
    int exit;
    BOOL inputEnabled;
    LanguageButton buttons[LANGUAGE_BUTTON_COUNT];
    // The button being pushed, and the button of the language shown
    int pushed;
    int language;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    // The shown language's button glows between two palettes
    u16 glowPalettes[2][16];
    u16 glowPalette[16];
    int glowPhase;
    BOOL glowStart;
    Font *font;
    MsgData *msgData[3];
} ZukanDetailInfoWork;

static BOOL ZukanDetailInfo_Init(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static BOOL ZukanDetailInfo_Exit(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static BOOL ZukanDetailInfo_Main(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static void ZukanDetailInfo_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                    ZukanDetailCommon *common, int command);
static void ZukanDetailInfo_VBlank(TCB *tcb, void *data);
static BOOL ZukanDetailInfo_CheckTouch(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);
static BOOL ZukanDetailInfo_CheckKeys(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);
static void ZukanDetailInfo_ChangePokemon(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailInfo_SetLanguage(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common,
                                        int language);
static void ZukanDetailInfo_CreateButtons(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailInfo_FreeButtons(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailInfo_UpdateButton(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                         ZukanDetailCommon *common);
static void ZukanDetailInfo_ResetButtons(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                         ZukanDetailCommon *common, const BOOL *visible);
static void ZukanDetailInfo_GetPokemon(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common,
                                       u16 *species, u16 *form, u16 *sex, u16 *rare, u32 *personality, BOOL *caught,
                                       BOOL *languages);
static void ZukanDetailInfo_InitGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);
static void ZukanDetailInfo_FreeGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);
static void ZukanDetailInfo_StartGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);
static void ZukanDetailInfo_StopGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);
static void ZukanDetailInfo_UpdateGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common);

const ZukanDetailProcFuncs ZUKAN_DETAIL_INFO_PROC_FUNCS = {
    ZukanDetailInfo_Init, ZukanDetailInfo_Main, ZukanDetailInfo_Exit, ZukanDetailInfo_Command, NULL,
};

// The buttons of Japanese, French, German, Italian, Spanish and Korean
static const LanguageButtonData sZukanDetailInfoLanguageButtons[LANGUAGE_BUTTON_COUNT] = {
    { 2, 91, { 2, 91, 16, 15 }, 8, 15, 1 },    { 17, 91, { 19, 91, 20, 15 }, 9, 16, 3 },
    { 38, 91, { 40, 91, 20, 15 }, 10, 27, 5 }, { 59, 91, { 61, 91, 20, 15 }, 11, 17, 4 },
    { 80, 91, { 82, 91, 20, 15 }, 12, 18, 7 }, { 101, 91, { 103, 91, 20, 15 }, 13, 19, 8 },
};

void ZukanDetailInfo_InitParam(ZukanDetailInfoParam *param, HeapID heapId) {
    param->heapId = heapId;
}

static BOOL ZukanDetailInfo_Init(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailInfoParam *param = param_;
    ZukanDetailInfoWork *wk;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_OvlLoad(OVERLAY_ZUKAN_INFO);
    wk = ZukanDetailProcSys_AllocWork(sys, sizeof(ZukanDetailInfoWork), param->heapId);
    sys_memset(wk, 0, sizeof(ZukanDetailInfoWork));
    wk->vblankTcb = GFL_VBlankTCBAdd(ZukanDetailInfo_VBlank, wk, 1);
    wk->pushed = LANGUAGE_NONE;
    wk->language = LANGUAGE_NONE;
    wk->glowStart = FALSE;
    wk->blendMain = ZukanDetailBlend_Create(param->heapId);
    wk->blendSub = ZukanDetailBlend_Create(param->heapId);
    ZukanDetailBlend_InitPlanes(wk->blendMain);
    ZukanDetailBlend_InitPlanes(wk->blendSub);
    ZukanDetailBlend_SetOut(0, wk->blendMain);
    ZukanDetailBlend_SetOut(1, wk->blendSub);
    wk->exit = INFO_EXIT_NONE;
    wk->inputEnabled = TRUE;
    return TRUE;
}

static BOOL ZukanDetailInfo_Exit(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailInfoParam *param = param_;
    ZukanDetailInfoWork *wk = work;

    ZukanDetailInfo_StopGlow(param, wk, common);
    ZukanDetailInfo_FreeGlow(param, wk, common);
    func_ov296_0219dbcc(wk->infoSub);
    func_ov296_0219dbcc(wk->infoMain);
    ZukanDetailInfo_FreeButtons(param, wk, common);
    GFL_FontFree(wk->font);
    GFL_MsgDataFree(wk->msgData[2]);
    GFL_MsgDataFree(wk->msgData[1]);
    GFL_MsgDataFree(wk->msgData[0]);
    ZukanDetailBlend_Free(wk->blendSub);
    ZukanDetailBlend_Free(wk->blendMain);
    GFL_TCBRemove(wk->vblankTcb);
    ZukanDetailProcSys_FreeWork(sys);
    GFL_OvlUnload(OVERLAY_ZUKAN_INFO);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL ZukanDetailInfo_Main(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailInfoParam *param = param_;
    ZukanDetailInfoWork *wk = work;
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    ZukanDetailHeadbar *headbar = ZukanDetailCommon_GetHeadbar(common);

    switch (*seq) {
    case INFO_SEQ_INIT: {
        u8 bg;
        PokeDexSave *pokedex;
        ClActUnit *unit;
        BOOL national;
        u16 species;
        u16 form;
        u16 sex;
        u16 rare;
        u32 personality;
        BOOL caught;
        BOOL languages[LANGUAGE_BUTTON_COUNT];

        *seq = INFO_SEQ_FADE_IN;
        wk->font = GFL_FontCreate(ARCID_FONT, 0, 1, FALSE, param->heapId);
        wk->msgData[0] = GFL_MsgSysLoadData(TRUE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_SPECIES_CATEGORIES, param->heapId);
        wk->msgData[1] = GFL_MsgSysLoadData(TRUE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEDEX_HEIGHTS, param->heapId);
        wk->msgData[2] = GFL_MsgSysLoadData(TRUE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEDEX_WEIGHTS, param->heapId);
        for (bg = 0; bg <= 7; bg++) {
            if (bg != 1 && bg != 5) {
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_X, 0);
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_Y, 0);
                GFL_BGSysClearBG(bg);
            }
        }

        if (ZukanDetailTouchbar_GetState(touchbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_GENERAL, ZUKAN_DETAIL_PAGE_INFO - 1,
                                        ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            ZukanDetailTouchbar_Appear(touchbar, 0);
        } else {
            ZukanDetailTouchbar_SetPage(touchbar, ZUKAN_DETAIL_PAGE_INFO - 1);
        }
        ZukanDetailTouchbar_SetActive(touchbar, FALSE);
        ZukanDetailTouchbar_SetCheck(
            touchbar, GameData_IsShortcutRegistered(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX));
        if (ZukanDetailHeadbar_GetState(headbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailHeadbar_SetTitle(headbar, 0);
            ZukanDetailHeadbar_Appear(headbar);
        }

        ZukanDetailInfo_CreateButtons(param, wk, common);
        pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
        unit = ZukanDetailGraphic_GetClActUnit(ZukanDetailCommon_GetGraphic(common));
        switch (func_0200d1f8(pokedex)) {
        case 0:
            national = FALSE;
            break;
        case 1:
            national = TRUE;
            break;
        case 2:
            national = PokeDex_IsNationalObtained(pokedex);
            break;
        }
        ZukanDetailInfo_GetPokemon(param, wk, common, &species, &form, &sex, &rare, &personality, &caught, languages);
        wk->infoMain = func_ov296_0219d768(param->heapId, species, form, sex, rare, personality, national, caught, 2, 0,
                                           1, unit, wk->font, ZukanDetailCommon_GetPrintQueue(common), wk->msgData[0],
                                           wk->msgData[1], wk->msgData[2]);
        wk->infoSub = func_ov296_0219d768(param->heapId, species, form, sex, rare, personality, national, caught, 2, 1,
                                          1, unit, wk->font, ZukanDetailCommon_GetPrintQueue(common), wk->msgData[0],
                                          wk->msgData[1], wk->msgData[2]);
        func_ov296_0219e0c8(wk->infoSub);
        ZukanDetailInfo_ResetButtons(param, wk, common, languages);
        ZukanDetailInfo_InitGlow(param, wk, common);
        ZukanDetailInfo_StartGlow(param, wk, common);
        break;
    }
    case INFO_SEQ_FADE_IN:
        *seq = INFO_SEQ_WAIT_FADE_IN;
        ZukanDetailBlend_StartIn(wk->blendMain);
        ZukanDetailBlend_StartIn(wk->blendSub);
        break;
    case INFO_SEQ_WAIT_FADE_IN:
        if (!ZukanDetailBlend_IsActive(wk->blendMain) &&
            ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
            ZukanDetailTouchbar_Unlock(touchbar);
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            *seq = INFO_SEQ_MAIN;
        }
        break;
    case INFO_SEQ_MAIN:
        if (wk->exit != INFO_EXIT_NONE) {
            *seq = INFO_SEQ_WAIT_FADE_OUT;
            ZukanDetailBlend_StartOut(wk->blendMain);
            ZukanDetailBlend_StartOut(wk->blendSub);
            ZukanDetailHeadbar_Disappear(headbar);
            if (wk->exit == INFO_EXIT_SCREEN) {
                ZukanDetailTouchbar_Disappear(touchbar, 0);
            }
        } else if (!ZukanDetailTouchbar_IsArrowTriggered(touchbar) && wk->inputEnabled) {
            if (!ZukanDetailInfo_CheckTouch(param, wk, common)) {
                ZukanDetailInfo_CheckKeys(param, wk, common);
            }
        }
        break;
    case INFO_SEQ_WAIT_FADE_OUT:
        if (!ZukanDetailBlend_IsActive(wk->blendMain) && !ZukanDetailBlend_IsActive(wk->blendSub) &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
            if (wk->exit == INFO_EXIT_SCREEN) {
                if (ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
                    *seq = INFO_SEQ_EXIT;
                }
            } else {
                *seq = INFO_SEQ_EXIT;
            }
        }
        break;
    case INFO_SEQ_EXIT:
        return TRUE;
    }

    if (*seq >= INFO_SEQ_WAIT_FADE_IN) {
        ZukanDetailInfo_UpdateButton(param, wk, common);
        func_ov296_0219dc74(wk->infoMain);
        func_ov296_0219dc74(wk->infoSub);
        if (wk->pushed == LANGUAGE_NONE) {
            ZukanDetailInfo_UpdateGlow(param, wk, common);
        }
    }
    ZukanDetailBlend_Update(wk->blendMain, wk->blendSub);
    return FALSE;
}

static void ZukanDetailInfo_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                    ZukanDetailCommon *common, int command) {
    ZukanDetailInfoWork *wk = work;

    if (wk != NULL) {
        ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
        BOOL unlock = FALSE;

        // Input waits while the bar's icons play their animations
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE_TOUCH:
        case ZUKAN_DETAIL_CMD_RETURN_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH:
        case ZUKAN_DETAIL_CMD_CHECK_TOUCH:
        case ZUKAN_DETAIL_CMD_MAP_TOUCH:
        case ZUKAN_DETAIL_CMD_VOICE_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_TOUCH:
            wk->inputEnabled = FALSE;
            break;
        }
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
        case ZUKAN_DETAIL_CMD_CUR_D:
        case ZUKAN_DETAIL_CMD_CUR_U:
        case ZUKAN_DETAIL_CMD_CHECK:
        case ZUKAN_DETAIL_CMD_MAP:
        case ZUKAN_DETAIL_CMD_VOICE:
        case ZUKAN_DETAIL_CMD_FORM:
            unlock = TRUE;
            wk->inputEnabled = TRUE;
            break;
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH: {
            u16 species = ZukanDetailCommon_GetSpecies(common);

            ZukanDetailCommon_GoNext(common);
            if (species != ZukanDetailCommon_GetSpecies(common)) {
                ZukanDetailInfo_ChangePokemon(param, wk, common);
            }
            break;
        }
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH: {
            u16 species = ZukanDetailCommon_GetSpecies(common);

            ZukanDetailCommon_GoPrev(common);
            if (species != ZukanDetailCommon_GetSpecies(common)) {
                ZukanDetailInfo_ChangePokemon(param, wk, common);
            }
            break;
        }
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_NONE:
            break;
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
            wk->exit = INFO_EXIT_SCREEN;
            break;
        case ZUKAN_DETAIL_CMD_MAP:
        case ZUKAN_DETAIL_CMD_VOICE:
        case ZUKAN_DETAIL_CMD_FORM:
            wk->exit = INFO_EXIT_PAGE;
            break;
        case ZUKAN_DETAIL_CMD_CUR_D:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_CUR_U:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_CHECK:
            GameData_SetKeyItemRegistration(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX,
                                            ZukanDetailTouchbar_GetCheck(touchbar));
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        default:
            if (unlock) {
                ZukanDetailTouchbar_Unlock(touchbar);
            }
            break;
        }
    }
}

static void ZukanDetailInfo_VBlank(TCB *tcb, void *data) {
}

static BOOL ZukanDetailInfo_CheckTouch(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                       ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    BOOL pushed = FALSE;

    if (wk->pushed == LANGUAGE_NONE) {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            u8 i;

            for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
                if (func_0204c138(wk->buttons[i].actor) && wk->buttons[i].rect[0] <= x &&
                    x < wk->buttons[i].rect[0] + wk->buttons[i].rect[2] && wk->buttons[i].rect[1] <= y &&
                    y < wk->buttons[i].rect[1] + wk->buttons[i].rect[3]) {
                    if (wk->language != i) {
                        wk->pushed = i;
                        pushed = TRUE;
                    }
                    break;
                }
            }
        }
    }

    if (pushed) {
        ZukanDetailInfo_SetLanguage(param, wk, common, wk->pushed);
        wk->buttons[wk->pushed].state = BUTTON_PUSHED;
        func_0204c488(wk->buttons[wk->pushed].actor, wk->buttons[wk->pushed].pushedAnim);
        GFL_SndSEPlay(SEQ_SE_SELECT3);
        func_0203d564(TRUE);
        ZukanDetailTouchbar_SetActive(touchbar, FALSE);
    }
    return pushed;
}

// L and R move to the previous and next language seen, and with none chosen yet, A or either of them to the first
static BOOL ZukanDetailInfo_CheckKeys(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    BOOL pushed = FALSE;

    if (wk->pushed == LANGUAGE_NONE) {
        u32 typed = GCTX_HIDGetTypedKeys();
        u32 pressed = GCTX_HIDGetPressedKeys();
        u8 i;

        if (wk->language == LANGUAGE_NONE) {
            if ((typed & (PAD_BUTTON_L | PAD_BUTTON_R)) || (pressed & PAD_BUTTON_A)) {
                for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
                    if (func_0204c138(wk->buttons[i].actor)) {
                        wk->pushed = i;
                        pushed = TRUE;
                        break;
                    }
                }
            }
        } else if ((typed & PAD_BUTTON_R) || (pressed & PAD_BUTTON_A)) {
            for (i = 1; i < LANGUAGE_BUTTON_COUNT; i++) {
                u8 button = (wk->language + i) % LANGUAGE_BUTTON_COUNT;

                if (func_0204c138(wk->buttons[button].actor)) {
                    wk->pushed = button;
                    pushed = TRUE;
                    break;
                }
            }
        } else if (typed & PAD_BUTTON_L) {
            for (i = 1; i < LANGUAGE_BUTTON_COUNT; i++) {
                u8 button = (wk->language + LANGUAGE_BUTTON_COUNT - i) % LANGUAGE_BUTTON_COUNT;

                if (func_0204c138(wk->buttons[button].actor)) {
                    wk->pushed = button;
                    pushed = TRUE;
                    break;
                }
            }
        }
    }

    if (pushed) {
        ZukanDetailInfo_SetLanguage(param, wk, common, wk->pushed);
        wk->buttons[wk->pushed].state = BUTTON_PUSHED;
        func_0204c488(wk->buttons[wk->pushed].actor, wk->buttons[wk->pushed].pushedAnim);
        GFL_SndSEPlay(SEQ_SE_SELECT3);
        func_0203d564(FALSE);
        ZukanDetailTouchbar_SetActive(touchbar, FALSE);
    }
    return pushed;
}

// Shows the Pokémon the list moved to, in the game's language unless it has been seen in the one shown
static void ZukanDetailInfo_ChangePokemon(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                          ZukanDetailCommon *common) {
    BOOL ownLanguage;
    u16 species;
    u16 form;
    u16 sex;
    u16 rare;
    u32 personality;
    BOOL caught;
    BOOL languages[LANGUAGE_BUTTON_COUNT];

    ZukanDetailInfo_GetPokemon(param, wk, common, &species, &form, &sex, &rare, &personality, &caught, languages);
    ownLanguage = TRUE;
    func_ov296_0219de50(wk->infoMain, species, form, sex, rare, personality, caught);
    if (wk->language != LANGUAGE_NONE) {
        if (languages[wk->language]) {
            ownLanguage = FALSE;
        } else {
            ZukanDetailInfo_StopGlow(param, wk, common);
            wk->language = LANGUAGE_NONE;
        }
    }
    if (ownLanguage) {
        func_ov296_0219e0c8(wk->infoSub);
    } else {
        func_ov296_0219dfb0(wk->infoSub, species, form, sex, rare, personality, caught, wk->language);
    }
    ZukanDetailInfo_ResetButtons(param, wk, common, languages);
}

static void ZukanDetailInfo_SetLanguage(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common,
                                        int language) {
    if (wk->language != language) {
        if (wk->language == LANGUAGE_NONE) {
            u16 species;
            u16 form;
            u16 sex;
            u16 rare;
            u32 personality;
            BOOL caught;
            BOOL languages[LANGUAGE_BUTTON_COUNT];

            ZukanDetailInfo_GetPokemon(param, wk, common, &species, &form, &sex, &rare, &personality, &caught,
                                       languages);
            func_ov296_0219dfb0(wk->infoSub, species, form, sex, rare, personality, caught, language);
            func_ov296_0219e114(wk->infoSub);
        } else {
            func_ov296_0219df4c(wk->infoSub, language);
        }
        ZukanDetailInfo_StopGlow(param, wk, common);
        wk->language = language;
        ZukanDetailInfo_StartGlow(param, wk, common);
    }
}

static void ZukanDetailInfo_CreateButtons(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                          ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    ClActUnit *unit = ZukanDetailGraphic_GetClActUnit(ZukanDetailCommon_GetGraphic(common));
    ClActSurface surface = CLACT_SURFACE_MAIN;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);
    ClActorSetup setup;
    u8 i;

    wk->palette = ZukanDetailTouchbar_GetIconPalette(touchbar);
    wk->chars = func_0204b81c(arc, 13, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->cellAnims = func_0204bde0(arc, 28, 45, param->heapId);
    GFL_ArcToolFree(arc);

    for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
        sys_memset(&setup, 0, sizeof(ClActorSetup));
        wk->buttons[i].rect[0] = sZukanDetailInfoLanguageButtons[i].rect[0];
        wk->buttons[i].rect[1] = sZukanDetailInfoLanguageButtons[i].rect[1];
        wk->buttons[i].rect[2] = sZukanDetailInfoLanguageButtons[i].rect[2];
        wk->buttons[i].rect[3] = sZukanDetailInfoLanguageButtons[i].rect[3];
        wk->buttons[i].anim = sZukanDetailInfoLanguageButtons[i].anim;
        wk->buttons[i].pushedAnim = sZukanDetailInfoLanguageButtons[i].pushedAnim;
        wk->buttons[i].language = sZukanDetailInfoLanguageButtons[i].language;
        wk->buttons[i].state = BUTTON_IDLE;
        setup.x = sZukanDetailInfoLanguageButtons[i].x;
        setup.y = sZukanDetailInfoLanguageButtons[i].y;
        setup.sequence = wk->buttons[i].anim;
        wk->buttons[i].actor =
            func_0204c040(unit, wk->chars, wk->palette, wk->cellAnims, &setup, surface, param->heapId);
        func_0204c520(wk->buttons[i].actor, TRUE);
        func_0204c318(wk->buttons[i].actor, 1);
    }
}

static void ZukanDetailInfo_FreeButtons(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                        ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
        func_0204c108(wk->buttons[i].actor);
    }
    func_0204b98c(wk->chars);
    func_0204be64(wk->cellAnims);
}

static void ZukanDetailInfo_UpdateButton(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                         ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);

    if (wk->pushed != LANGUAGE_NONE) {
        u8 i = wk->pushed;

        switch (wk->buttons[i].state) {
        case BUTTON_IDLE:
            break;
        case BUTTON_PUSHED:
            wk->buttons[i].state = BUTTON_WAIT_ANIM;
            break;
        case BUTTON_WAIT_ANIM:
            if (!func_0204c560(wk->buttons[i].actor)) {
                wk->buttons[i].state = BUTTON_DONE;
            }
            break;
        case BUTTON_DONE:
            func_0204c488(wk->buttons[i].actor, wk->buttons[i].anim);
            wk->buttons[i].state = BUTTON_IDLE;
            wk->pushed = LANGUAGE_NONE;
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            break;
        }
    }
}

// Shows the buttons of the languages the Pokémon has been seen in
static void ZukanDetailInfo_ResetButtons(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                         ZukanDetailCommon *common, const BOOL *visible) {
    u8 i;

    for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
        func_0204c124(wk->buttons[i].actor, visible[i]);
        wk->buttons[i].state = BUTTON_IDLE;
        func_0204c488(wk->buttons[i].actor, wk->buttons[i].anim);
    }
    wk->pushed = LANGUAGE_NONE;
}

static void ZukanDetailInfo_GetPokemon(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common,
                                       u16 *species, u16 *form, u16 *sex, u16 *rare, u32 *personality, BOOL *caught,
                                       BOOL *languages) {
    PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
    u16 shown = ZukanDetailCommon_GetSpecies(common);
    u32 shownForm;
    u32 shownSex;
    u32 shownRare;
    u32 shownPersonality = 0;
    BOOL shownCaught;
    BOOL seenLanguages[LANGUAGE_BUTTON_COUNT];
    u8 i;

    func_0200d3c8(pokedex, shown, &shownSex, &shownRare, &shownForm, param->heapId);
    if (shown == SPECIES_SPINDA) {
        shownPersonality = func_0200da18(pokedex, shownPersonality);
    }
    shownCaught = PokeDex_IsCaught(pokedex, shown);
    for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
        seenLanguages[i] = func_0200db5c(pokedex, shown, wk->buttons[i].language);
    }

    *species = shown;
    *form = shownForm;
    *sex = shownSex;
    *rare = shownRare;
    *personality = shownPersonality;
    *caught = shownCaught;
    for (i = 0; i < LANGUAGE_BUTTON_COUNT; i++) {
        languages[i] = seenLanguages[i];
    }
}

// Reads the two palettes that the shown language's button glows between
static void ZukanDetailInfo_InitGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common) {
    NNSG2dPaletteData *palette;
    void *file = GFL_G2DIOReadNCLR(ARCID_ZUKAN_GRA, 3, &palette, param->heapId);
    u16 *data = palette->rawData;

    sys_memcpy(data + 16, wk->glowPalettes[0], 32);
    sys_memcpy(data + 48, wk->glowPalettes[1], 32);
    GFL_HeapFree(file);
    wk->glowPhase = 0;
    sys_memcpy(wk->glowPalettes[0], wk->glowPalette, 32);
    NNS_GfdRegisterNewVramTransferTask(14, 6 * 32, wk->glowPalette, 32);
}

static void ZukanDetailInfo_FreeGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common) {
}

static void ZukanDetailInfo_StartGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common) {
    if (wk->language != LANGUAGE_NONE) {
        wk->glowPhase = 0;
        wk->glowStart = TRUE;
    }
}

static void ZukanDetailInfo_StopGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk, ZukanDetailCommon *common) {
    if (wk->language != LANGUAGE_NONE) {
        func_0204c378(wk->buttons[wk->language].actor, 0, 0);
        wk->glowStart = FALSE;
    }
}

// Blends the glowing palette between the two palettes with a cosine, and loads it into OBJ palette 6
static void ZukanDetailInfo_UpdateGlow(ZukanDetailInfoParam *param, ZukanDetailInfoWork *wk,
                                       ZukanDetailCommon *common) {
    s16 ratio;
    u8 i;

    if (wk->glowStart) {
        if (wk->language != LANGUAGE_NONE) {
            func_0204c378(wk->buttons[wk->language].actor, 2, 0);
        }
        wk->glowStart = FALSE;
    }

    if (wk->glowPhase + 0x400 >= 0x10000) {
        wk->glowPhase = wk->glowPhase + 0x400 - 0x10000;
    } else {
        wk->glowPhase += 0x400;
    }
    ratio = (FX_CosIdx(wk->glowPhase) + FX32_ONE) / 2;

    for (i = 0; i < 16; i++) {
        u16 from = wk->glowPalettes[0][i];
        u16 to = wk->glowPalettes[1][i];
        u8 r0 = from & 0x1f;
        u8 g0 = (from & 0x3e0) >> 5;
        u8 b0 = (from & 0x7c00) >> 10;
        u8 r1 = to & 0x1f;
        u8 g1 = (to & 0x3e0) >> 5;
        u8 b1 = (to & 0x7c00) >> 10;
        u8 r = r0 + (((r1 - r0) * ratio) >> FX32_SHIFT);
        u8 g = g0 + (((g1 - g0) * ratio) >> FX32_SHIFT);
        u8 b = b0 + (((b1 - b0) * ratio) >> FX32_SHIFT);

        wk->glowPalette[i] = r | (g << 5) | (b << 10);
    }
    NNS_GfdRegisterNewVramTransferTask(14, 6 * 32, wk->glowPalette, 32);
}
