#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "p_status_local.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"

// The bottom screen of the summary screen: the Pokémon's sprite, which turns around when touched and bounces, hops or
// spins after a swipe or a circle drawn on it, its name, level, held item, ball, markings and status. The names of the
// fields and functions are guesses

// The icons below the sprite: the markings, then the shiny star and the Pokérus face
#define MARKING_COUNT 6
enum {
    PSTA_SUB_ICON_SHINY = MARKING_COUNT,
    PSTA_SUB_ICON_POKERUS,
    PSTA_SUB_ICON_COUNT,
};

// The direction of a circle drawn on the sprite
enum {
    PSTA_SUB_CIRCLE_NONE,
    PSTA_SUB_CIRCLE_CLOCKWISE,
    PSTA_SUB_CIRCLE_COUNTERCLOCKWISE,
};

// What the sprite is doing
enum {
    PSTA_SUB_FRONT,
    PSTA_SUB_TURN_TO_BACK,
    PSTA_SUB_BACK,
    PSTA_SUB_TURN_TO_FRONT,
};

// The animations a gesture starts
enum {
    PSTA_SUB_ANIM_NONE,
    PSTA_SUB_ANIM_SWIPE_X,
    PSTA_SUB_ANIM_SWIPE_Y,
    PSTA_SUB_ANIM_CIRCLE,
    PSTA_SUB_ANIM_SWIPE_X_END,
    PSTA_SUB_ANIM_SWIPE_Y_END,
    PSTA_SUB_ANIM_CIRCLE_END,
};

// The directions of a stroke
enum {
    PSTA_SUB_DIR_NONE,
    PSTA_SUB_DIR_RIGHT,
    PSTA_SUB_DIR_DOWN,
    PSTA_SUB_DIR_LEFT,
    PSTA_SUB_DIR_UP,
};

#define LAP_COUNT 5

// A gesture drawn on the sprite: how many strokes, how long the current one has taken, where it began and which way
// it goes
typedef struct {
    u8 count;
    u8 frames;
    u8 startX;
    u8 startY;
    u32 dir;
} PStaSubGesture;

struct PStaSubWork {
    MCSS *mcss[2];
    u32 state;
    BOOL frontShown;
    BOOL noBounce;
    u32 weight;
    BmpWin *nameWindow;
    BmpWin *itemWindow;
    ClActor *ballIcon;
    // The six markings, the shiny star and the Pokérus mark
    ClActor *icons[PSTA_SUB_ICON_COUNT];
    ClActor *pokerusIcon;
    ClActor *statusIcon;
    VecFx32 bounceOffset;
    BOOL gestureDone;
    u32 anim;
    u16 animTimeout;
    u16 animFrame;
    u16 animStep;
    u16 spinSpeed;
    u16 spinTarget;
    BOOL isTouching;
    u32 lastTouchX;
    u32 lastTouchY;
    PStaSubGesture swipeX;
    PStaSubGesture swipeY;
    PStaSubGesture circle;
    u32 circleDir;
    u8 lapFrames[LAP_COUNT];
    u8 lapTimer;
    u8 lapIndex;
    u8 turnTimer;
};

static void PStaSub_PrintInfo(PStatusWork *wk, PStaSubWork *sub, BoxPkm *pkm);
static void PStaSub_UpdateAnim(PStatusWork *wk, PStaSubWork *sub);
static void PStaSub_StartAnim(PStatusWork *wk, PStaSubWork *sub);
static BOOL PStaSub_CheckGesture(PStatusWork *wk, PStaSubWork *sub, PStaSubGesture *gesture);
static BOOL PStaSub_AnimSwipeX(PStatusWork *wk, PStaSubWork *sub);
static BOOL PStaSub_AnimSwipeY(PStatusWork *wk, PStaSubWork *sub);
static BOOL PStaSub_AnimCircle(PStatusWork *wk, PStaSubWork *sub, BOOL spinning);
static void PStaSub_HandleTouch(PStatusWork *wk, PStaSubWork *sub);
static void PStaSub_HandleMarkings(PStatusWork *wk, PStaSubWork *sub);
static void PStaSub_TrackGestures(PStatusWork *wk, PStaSubWork *sub);
static void PStaSub_ResetGesture(PStaSubGesture *gesture, u32 x, u32 y);
static void PStaSub_AdvanceGesture(PStaSubGesture *gesture, u32 x, u32 y, u32 dir);
static void PStaSub_CountLap(PStatusWork *wk, PStaSubWork *sub, BOOL lap);
static void PStaSub_CreateSprites(PStatusWork *wk, PStaSubWork *sub, BoxPkm *pkm);
static void PStaSub_RemoveSprites(PStatusWork *wk, PStaSubWork *sub);
static void PStaSub_SetBounce(PStatusWork *wk, PStaSubWork *sub, fx32 bounce);
static void PStaSub_SetSpritePosition(PStatusWork *wk, PStaSubWork *sub, const VecFx32 *pos);
static void PStaSub_AdjustForSpecies(PStatusWork *wk, PStaSubWork *sub, VecFx32 *size, VecFx32 *scale);

// The markings, the shiny star and the Pokérus mark
static const u8 sIconSequences[PSTA_SUB_ICON_COUNT] = { 0, 2, 4, 6, 8, 10, 12, 13 };
static const u8 sIconXs[PSTA_SUB_ICON_COUNT] = { 179, 190, 201, 212, 223, 234, 152, 160 };

PStaSubWork *PStaSub_Create(PStatusWork *wk) {
    PStaSubWork *sub = GFL_HeapAllocate(wk->heapId, sizeof(PStaSubWork), TRUE, "p_sta_sub.c", 280);

    sub->mcss[0] = NULL;
    sub->mcss[1] = NULL;
    sub->isTouching = FALSE;
    sub->frontShown = TRUE;
    sub->state = PSTA_SUB_FRONT;
    sub->anim = PSTA_SUB_ANIM_NONE;
    return sub;
}

void PStaSub_Free(PStatusWork *wk, PStaSubWork *sub) {
    GFL_HeapFree(sub);
}

void PStaSub_Main(PStatusWork *wk, PStaSubWork *sub) {
    if (sub->state == PSTA_SUB_FRONT || sub->state == PSTA_SUB_BACK) {
        PStaSub_HandleTouch(wk, sub);
        PStaSub_HandleMarkings(wk, sub);
        PStaSub_UpdateAnim(wk, sub);
    } else if (sub->state == PSTA_SUB_TURN_TO_FRONT) {
        VecFx32 target = { FX32_CONST(-41), 0, FX32_CONST(101) };
        VecFx32 scale = { FX32_ONE, FX32_CONST(2.2), FX32_ONE };
        VecFx32 offset = { 0, 0, 0 };

        sub->turnTimer++;
        if (sub->turnTimer > 5 && sub->frontShown == FALSE) {
            MCSS_Show(sub->mcss[0]);
            MCSS_Hide(sub->mcss[1]);
            sub->frontShown = TRUE;
        }
        if (sub->turnTimer >= 10) {
            PStaSub_SetBounce(wk, sub, 0);
            sub->state = PSTA_SUB_FRONT;
        } else {
            target.x = FX32_CONST(25) * sub->turnTimer / 10 - FX32_CONST(66);
            offset.y = FX_SinIdx((u16)(sub->turnTimer * 0x8000 / 10)) * 32;
            if (sub->noBounce == TRUE) {
                offset.y = 0;
            }
            PStaSub_SetBounce(wk, sub, offset.y);
        }
        func_0201ab54(sub->mcss[0], &offset);
        func_0201ab54(sub->mcss[1], &offset);
        func_0201ac0c(sub->mcss[0], &scale);
        GFL_G3DCameraSetLookatPos(wk->camera, &target);
        GFL_G3DCameraFlush(wk->camera);
    } else if (sub->state == PSTA_SUB_TURN_TO_BACK) {
        VecFx32 target = { FX32_CONST(-66), 0, FX32_CONST(101) };
        VecFx32 scale = { FX32_CONST(1.8), FX32_CONST(2.5), FX32_ONE };

        sub->turnTimer++;
        if (sub->turnTimer < 3) {
            fx32 scaleXs[2] = { FX32_CONST(1.2), FX32_CONST(1.4) };
            fx32 scaleYs[2] = { FX32_CONST(2.2), FX32_CONST(2.3) };
            fx32 targetXs[2] = { FX32_CONST(-57), FX32_CONST(-60) };
            u8 frame = sub->turnTimer - 1;

            scale.x = scaleXs[frame];
            scale.y = scaleYs[frame];
            target.x = targetXs[frame];
            GFL_G3DCameraSetLookatPos(wk->camera, &target);
            GFL_G3DCameraFlush(wk->camera);
            func_0201ac0c(sub->mcss[0], &scale);
        } else {
            MCSS_Hide(sub->mcss[0]);
            MCSS_Show(sub->mcss[1]);
            func_0201ac0c(sub->mcss[1], &scale);
            GFL_G3DCameraSetLookatPos(wk->camera, &target);
            GFL_G3DCameraFlush(wk->camera);
            sub->frontShown = FALSE;
            sub->state = PSTA_SUB_BACK;
            sub->isTouching = FALSE;
        }
    }
}

void PStaSub_LoadResources(PStatusWork *wk, PStaSubWork *sub, ArcTool *arc) {
    loadBGScrToVramByFileNoReserveNegAlign(arc, 64, 2, 0, 0, FALSE, wk->heapId);
}

void PStaSub_FreeResources(PStatusWork *wk, PStaSubWork *sub) {
    PStaSub_Unload(wk, sub);
}

void PStaSub_CreateActors(PStatusWork *wk, PStaSubWork *sub) {
    ClActorSetup ballSetup;
    ClActorSetup iconSetup;
    ClActorSetup pokerusSetup;
    ClActorSetup statusSetup;
    u8 i;

    ballSetup.x = 168;
    ballSetup.y = 8;
    ballSetup.priority = 10;
    ballSetup.bgPriority = 1;
    ballSetup.sequence = 0;
    sub->ballIcon = func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(2)], wk->clResources[PSTA_RES_PLTT(1)],
                                  wk->clResources[PSTA_RES_CELL(2)], &ballSetup, 0, wk->heapId);
    func_0204c124(sub->ballIcon, FALSE);
    iconSetup.y = 127;
    iconSetup.priority = 10;
    iconSetup.bgPriority = 1;
    for (i = 0; i < PSTA_SUB_ICON_COUNT; i++) {
        iconSetup.x = sIconXs[i];
        iconSetup.sequence = sIconSequences[i];
        sub->icons[i] =
            func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(3)], wk->clResources[PSTA_RES_PLTT(2)],
                          wk->clResources[PSTA_RES_CELL(3)], &iconSetup, 0, wk->heapId);
        func_0204c124(sub->icons[i], TRUE);
    }
    pokerusSetup.x = 236;
    pokerusSetup.y = 26;
    pokerusSetup.priority = 10;
    pokerusSetup.bgPriority = 1;
    pokerusSetup.sequence = 0;
    sub->pokerusIcon =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(10)], wk->clResources[PSTA_RES_PLTT(9)],
                      wk->clResources[PSTA_RES_CELL(9)], &pokerusSetup, 0, wk->heapId);
    func_0204c124(sub->pokerusIcon, FALSE);
    statusSetup.x = 241;
    statusSetup.y = 31;
    statusSetup.priority = 10;
    statusSetup.bgPriority = 1;
    statusSetup.sequence = 0;
    sub->statusIcon =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(11)], wk->clResources[PSTA_RES_PLTT(10)],
                      wk->clResources[PSTA_RES_CELL(10)], &statusSetup, 0, wk->heapId);
    func_0204c124(sub->statusIcon, FALSE);
}

void PStaSub_FreeActors(PStatusWork *wk, PStaSubWork *sub) {
    u8 i;

    func_0204c108(sub->statusIcon);
    func_0204c108(sub->pokerusIcon);
    for (i = 0; i < PSTA_SUB_ICON_COUNT; i++) {
        func_0204c108(sub->icons[i]);
    }
    func_0204c108(sub->ballIcon);
}

void PStaSub_Load(PStatusWork *wk, PStaSubWork *sub) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u32 species;
    void *personal;

    if (wk->param->dataType == PSTATUS_DATA_PARTY) {
        sub->frontShown = PokeParty_GetSlotExists(wk->param->party, wk->partyIndex);
    } else {
        sub->frontShown = TRUE;
    }
    if (wk->isEgg == FALSE) {
        species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
        personal = PML_PersonalLoad(species, PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL), wk->heapId);
        if (PML_PersonalGetParam(personal, PERSONAL_NO_BOUNCE) == TRUE) {
            sub->noBounce = TRUE;
        } else {
            sub->noBounce = FALSE;
        }
        sub->weight = PML_PersonalGetParam(personal, PERSONAL_WEIGHT);
        PML_PersonalFree(personal);
    } else {
        sub->noBounce = FALSE;
        sub->weight = 0;
    }
    sub->nameWindow = BmpWin_CreateDynamic(1, 19, 0, 13, 4, 14, 1);
    sub->itemWindow = BmpWin_CreateDynamic(1, 19, 17, 13, 4, 14, 1);
    PStaSub_PrintInfo(wk, sub, pkm);
}

void PStaSub_Draw(PStatusWork *wk, PStaSubWork *sub) {
    BoxPkm *boxPkm = PStatus_GetBoxPkm(wk);
    PartyPkm *pkm = PStatus_GetPartyPkm(wk);
    BmpWin *window;
    u32 ball;
    void *file;
    NNSG2dPaletteData *palette;
    NNSG2dCharacterData *chars;
    u32 markings;
    u8 i;
    u8 bit;

    PStaSub_CreateSprites(wk, sub, boxPkm);
    window = sub->nameWindow;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    window = sub->itemWindow;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));

    ball = PML_PkmGetParam(boxPkm, PKM_PARAM_POKEBALL, NULL);
    file = GFL_G2DIOReadNCLR(getUINarcIdx(), func_0202d91c(ball), &palette, wk->heapId);
    func_0204bd10(wk->clResources[PSTA_RES_PLTT(1)], palette, 1);
    GFL_HeapFree(file);
    file = GFL_G2DIOReadBGNCGR(getUINarcIdx(), func_0202d928(ball), FALSE, &chars, wk->heapId);
    func_0204ba40(wk->clResources[PSTA_RES_CHAR(2)], chars);
    GFL_HeapFree(file);
    func_0204c124(sub->ballIcon, TRUE);

    markings = PML_PkmGetParam(boxPkm, PKM_PARAM_MARKINGS, NULL);
    for (i = 0, bit = 1; i < MARKING_COUNT; bit <<= 1, i++) {
        if (markings & bit) {
            func_0204c488(sub->icons[i], i * 2 + 1);
        } else {
            func_0204c488(sub->icons[i], i * 2);
        }
    }
    if (doesPokeHavePokerus(boxPkm) == TRUE) {
        func_0204c124(sub->icons[PSTA_SUB_ICON_POKERUS], TRUE);
    } else {
        func_0204c124(sub->icons[PSTA_SUB_ICON_POKERUS], FALSE);
    }
    if (PML_PkmIsRare(boxPkm) == TRUE && wk->isEgg == FALSE) {
        func_0204c124(sub->icons[PSTA_SUB_ICON_SHINY], TRUE);
    } else {
        func_0204c124(sub->icons[PSTA_SUB_ICON_SHINY], FALSE);
    }

    if (sub->frontShown == TRUE) {
        VecFx32 target = { FX32_CONST(-41), 0, FX32_CONST(101) };
        VecFx32 scale = { FX32_ONE, FX32_CONST(2.2), FX32_ONE };
        VecFx32 offset = { 0, 0, 0 };
        VecFx32 pos = { FX32_CONST(4.7), 0, FX32_CONST(-32) };

        func_0201ab54(sub->mcss[0], &offset);
        PStaSub_SetSpritePosition(wk, sub, &pos);
        func_0201ac0c(sub->mcss[0], &scale);
        GFL_G3DCameraSetLookatPos(wk->camera, &target);
        GFL_G3DCameraFlush(wk->camera);
        sub->state = PSTA_SUB_FRONT;
    } else {
        VecFx32 target = { FX32_CONST(-66), 0, FX32_CONST(101) };
        VecFx32 scale = { FX32_CONST(1.8), FX32_CONST(2.5), FX32_ONE };
        VecFx32 offset = { 0, 0, 0 };
        VecFx32 pos = { FX32_CONST(4.7), 0, FX32_CONST(-32) };

        func_0201ab54(sub->mcss[0], &offset);
        PStaSub_SetSpritePosition(wk, sub, &pos);
        func_0201ac0c(sub->mcss[1], &scale);
        GFL_G3DCameraSetLookatPos(wk->camera, &target);
        GFL_G3DCameraFlush(wk->camera);
        sub->state = PSTA_SUB_BACK;
    }

    if ((u8)GetStatusCond(pkm) != 0) {
        func_0204c488(sub->statusIcon, AppMenuCommon_GetStatusIcon(pkm));
        func_0204c124(sub->statusIcon, TRUE);
        func_0204c124(sub->pokerusIcon, FALSE);
    } else {
        func_0204c124(sub->statusIcon, FALSE);
        if (doesPokerusHaveDuration(boxPkm) == TRUE) {
            func_0204c124(sub->pokerusIcon, TRUE);
        } else {
            func_0204c124(sub->pokerusIcon, FALSE);
        }
    }

    {
        VecFx32 offset = { 0, 0, 0 };

        func_0201ab54(sub->mcss[0], &offset);
        func_0201ab54(sub->mcss[1], &offset);
    }
    sub->swipeX.count = 0;
    sub->swipeX.frames = 0;
    sub->swipeY.count = 0;
    sub->swipeY.frames = 0;
    sub->isTouching = FALSE;
    sub->anim = PSTA_SUB_ANIM_NONE;
    sub->circle.count = 0;
    sub->circle.frames = 0;
}

void PStaSub_Unload(PStatusWork *wk, PStaSubWork *sub) {
    if (wk->param->dataType == PSTATUS_DATA_PARTY) {
        PokeParty_SetSlotExists(wk->param->party, wk->shownPartyIndex, sub->frontShown);
    }
    BmpWin_Free(sub->itemWindow);
    BmpWin_Free(sub->nameWindow);
}

void PStaSub_Clear(PStatusWork *wk, PStaSubWork *sub) {
    PStaSub_RemoveSprites(wk, sub);
    func_0204c124(sub->ballIcon, FALSE);
}

static void PStaSub_PrintInfo(PStatusWork *wk, PStaSubWork *sub, BoxPkm *pkm) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    StrBuf *str = GFL_StrBufCreate(32, wk->heapId);
    StrBuf *name = GFL_StrBufCreate(32, wk->heapId);
    StrBuf *format;
    u32 sex;

    PML_PkmGetParam(pkm, PKM_PARAM_NICKNAME, name);
    func_0202437c(wordSet, 0, name, 0, 1, 2);
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, 0);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(sub->nameWindow), 25, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(name);
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
    GFL_WordSetSystemFree(wordSet);

    if (wk->isEgg == FALSE && PML_PkmGetParam(pkm, PKM_PARAM_SHOW_SEX, NULL) == TRUE) {
        sex = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL);
        if (sex == GENDER_MALE) {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, 1);
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(sub->nameWindow), 89, 1, str, wk->font,
                          PRINT_COLOR(5, 6, 0));
            GFL_StrBufFree(str);
        } else if (sex == GENDER_FEMALE) {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, 2);
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(sub->nameWindow), 89, 1, str, wk->font,
                          PRINT_COLOR(3, 4, 0));
            GFL_StrBufFree(str);
        }
    }

    if (wk->isEgg == FALSE) {
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        str = GFL_StrBufCreate(16, wk->heapId);
        WordSetNumber(wordSet, 0, PML_PkmGetLevel(pkm), 3, 0, 1);
        format = GFL_MsgDataLoadStrbufNew(wk->msgData, 3);
        GFL_WordSetFormatStrbuf(wordSet, str, format);
        func_02021c7c(wk->printQueue, BmpWin_GetBitmap(sub->nameWindow), 9, 17, str, wk->font, PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(format);
        GFL_StrBufFree(str);
        GFL_WordSetSystemFree(wordSet);
    }

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, 4);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(sub->itemWindow), 10, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);

    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    str = GFL_StrBufCreate(16, wk->heapId);
    loadItemNameToStrbuf(wordSet, 0, PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL));
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, 5);
    GFL_WordSetFormatStrbuf(wordSet, str, format);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(sub->itemWindow), 6, 17, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(format);
    GFL_StrBufFree(str);
    GFL_WordSetSystemFree(wordSet);
}

static void PStaSub_UpdateAnim(PStatusWork *wk, PStaSubWork *sub) {
    switch (sub->anim) {
    case PSTA_SUB_ANIM_NONE:
        if (sub->swipeX.count > 10) {
            sub->anim = PSTA_SUB_ANIM_SWIPE_X;
            PStaSub_StartAnim(wk, sub);
        }
        if (sub->swipeY.count > 10) {
            sub->anim = PSTA_SUB_ANIM_SWIPE_Y;
            PStaSub_StartAnim(wk, sub);
        }
        if (sub->circle.count > 20) {
            sub->anim = PSTA_SUB_ANIM_CIRCLE;
            PStaSub_StartAnim(wk, sub);
        }
        break;
    case PSTA_SUB_ANIM_SWIPE_X:
        PStaSub_AnimSwipeX(wk, sub);
        if (PStaSub_CheckGesture(wk, sub, &sub->swipeX) == FALSE) {
            sub->anim = PSTA_SUB_ANIM_SWIPE_X_END;
        }
        break;
    case PSTA_SUB_ANIM_SWIPE_X_END:
        if (PStaSub_AnimSwipeX(wk, sub) == TRUE) {
            sub->anim = PSTA_SUB_ANIM_NONE;
        }
        break;
    case PSTA_SUB_ANIM_SWIPE_Y:
        PStaSub_AnimSwipeY(wk, sub);
        if (PStaSub_CheckGesture(wk, sub, &sub->swipeY) == FALSE) {
            sub->anim = PSTA_SUB_ANIM_SWIPE_Y_END;
        }
        break;
    case PSTA_SUB_ANIM_SWIPE_Y_END:
        if (PStaSub_AnimSwipeY(wk, sub) == TRUE) {
            sub->anim = PSTA_SUB_ANIM_NONE;
        }
        break;
    case PSTA_SUB_ANIM_CIRCLE:
        PStaSub_AnimCircle(wk, sub, TRUE);
        if (PStaSub_CheckGesture(wk, sub, &sub->circle) == FALSE) {
            sub->anim = PSTA_SUB_ANIM_CIRCLE_END;
        }
        break;
    case PSTA_SUB_ANIM_CIRCLE_END:
        if (PStaSub_AnimCircle(wk, sub, FALSE) == TRUE) {
            MCSS_ResumeAnimation(sub->mcss[0]);
            sub->anim = PSTA_SUB_ANIM_NONE;
        }
        break;
    }
}

static void PStaSub_StartAnim(PStatusWork *wk, PStaSubWork *sub) {
    sub->gestureDone = TRUE;
    sub->animStep = 0;
    sub->spinTarget = 0;
    sub->animFrame = 0;
    sub->spinSpeed = 0;
    sub->animTimeout = 0;
}

static BOOL PStaSub_CheckGesture(PStatusWork *wk, PStaSubWork *sub, PStaSubGesture *gesture) {
    if (gesture->count < 10) {
        sub->animTimeout++;
        if (sub->animTimeout > 24) {
            return FALSE;
        }
    } else {
        sub->animTimeout = 0;
    }
    return TRUE;
}

static BOOL PStaSub_AnimSwipeX(PStatusWork *wk, PStaSubWork *sub) {
    VecFx32 pos = { FX32_CONST(216), FX32_CONST(72), 0 };
    VecFx32 offset = { 0, 0, 0 };
    s16 sin;
    u8 width;
    int dist;

    if (sub->noBounce == FALSE) {
        sin = FX_SinIdx((u16)(sub->animFrame * 0x8000 / 15));
        offset.y = sin * 16 + FX32_CONST(wk->happiness >> 6);
        PStaSub_SetBounce(wk, sub, offset.y);
    } else {
        offset.y = 0;
        PStaSub_SetBounce(wk, sub, offset.y);
    }
    width = (wk->happiness >> 6) + 16;
    dist = sub->animFrame * width / 15;
    switch (sub->animStep) {
    case 0:
        pos.x += FX32_CONST(dist);
        break;
    case 1:
        pos.x += FX32_CONST(width - dist);
        break;
    case 2:
        pos.x -= FX32_CONST(dist);
        break;
    case 3:
        pos.x -= FX32_CONST(width - dist);
        break;
    }
    MCSS_SetPosition(sub->mcss[0], &pos);
    func_0201ab54(sub->mcss[0], &offset);
    sub->animFrame++;
    if (sub->animFrame > 15) {
        sub->animFrame = 0;
        sub->animStep++;
        if (sub->animStep > 3) {
            sub->animStep = 0;
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL PStaSub_AnimSwipeY(PStatusWork *wk, PStaSubWork *sub) {
    VecFx32 offset = { 0, 0, 0 };
    s16 sin;

    if (sub->noBounce == FALSE) {
        sin = FX_SinIdx((u16)(sub->animFrame * 0x8000 / 20));
        offset.y = sin * 32 + FX32_CONST(wk->happiness >> 6);
        PStaSub_SetBounce(wk, sub, offset.y);
    }
    func_0201ab54(sub->mcss[0], &offset);
    sub->animFrame++;
    if (sub->animFrame > 20) {
        sub->animFrame = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL PStaSub_AnimCircle(PStatusWork *wk, PStaSubWork *sub, BOOL spinning) {
    VecFx32 offset = { 0, 0, 0 };
    u32 lap;
    u8 i;

    if (sub->noBounce == TRUE) {
        return TRUE;
    }
    if (spinning == TRUE) {
        lap = 0;
        for (i = 0; i < LAP_COUNT; i++) {
            lap += sub->lapFrames[i];
        }
        lap /= LAP_COUNT;
        if (lap < 10) {
            lap = 10;
        }
        if (lap > 20) {
            lap = 20;
        }
        sub->spinTarget = (u16)(20 - lap) * 0x1800 / 10 + 0x800;
        offset.y += FX_SinIdx(sub->animFrame * 0x222) * 4;
        sub->animFrame++;
        if (sub->animFrame > 120) {
            sub->animFrame = 0;
        }
    } else {
        sub->spinTarget = 0;
    }
    if (sub->spinSpeed < sub->spinTarget) {
        sub->spinSpeed++;
    }
    if (sub->spinSpeed > sub->spinTarget) {
        if (spinning == FALSE) {
            if (sub->spinSpeed >= 32) {
                sub->spinSpeed -= 32;
            } else {
                sub->spinSpeed = 0;
            }
        } else {
            sub->spinSpeed--;
        }
    }
    offset.y += FX32_CONST(sub->spinSpeed) / 16;
    PStaSub_SetBounce(wk, sub, offset.y);
    func_0201ab54(sub->mcss[0], &offset);
    MCSS_PauseAnimation(sub->mcss[0]);
    if (sub->spinSpeed == 0 && sub->spinTarget == 0) {
        return TRUE;
    }
    return FALSE;
}

static void PStaSub_HandleTouch(PStatusWork *wk, PStaSubWork *sub) {
    TouchRect rects[3] = {
        { 0, 120, 160, 255 },
        { 0, 120, 120, 255 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };
    u32 x;
    u32 y;
    BOOL pressed;
    BOOL held;
    s32 hit;
    u8 i;

    func_0203da84(&x, &y);
    if (wk->isEgg) {
        return;
    }
    pressed = func_0203da48();
    held = func_0203da2c();
    hit = func_0203d9c8(rects);
    if (sub->state == PSTA_SUB_FRONT) {
        if (hit == 0 || (hit == 1 && pressed == FALSE)) {
            if (pressed == TRUE) {
                sub->isTouching = TRUE;
                sub->gestureDone = FALSE;
                PStaSub_ResetGesture(&sub->swipeX, x, y);
                PStaSub_ResetGesture(&sub->swipeY, x, y);
                PStaSub_ResetGesture(&sub->circle, x, y);
                for (i = 0; i < LAP_COUNT; i++) {
                    sub->lapFrames[i] = 0;
                }
                sub->lapTimer = 0;
                sub->lapIndex = 0;
                sub->circleDir = PSTA_SUB_CIRCLE_NONE;
            } else if (held == TRUE && sub->isTouching == TRUE) {
                PStaSub_TrackGestures(wk, sub);
            }
            sub->lastTouchX = x;
            sub->lastTouchY = y;
        } else {
            if (sub->isTouching == TRUE && sub->gestureDone == FALSE && sub->anim == PSTA_SUB_ANIM_NONE) {
                sub->state = PSTA_SUB_TURN_TO_BACK;
                sub->turnTimer = 0;
            }
            PStaSub_ResetGesture(&sub->swipeX, x, y);
            PStaSub_ResetGesture(&sub->swipeY, x, y);
            PStaSub_ResetGesture(&sub->circle, x, y);
            sub->isTouching = FALSE;
        }
    } else if (sub->state == PSTA_SUB_BACK) {
        if (hit == 0 || (hit == 1 && pressed == FALSE)) {
            if (pressed == TRUE) {
                sub->isTouching = TRUE;
            }
        } else if (sub->isTouching == TRUE) {
            sub->isTouching = FALSE;
            sub->state = PSTA_SUB_TURN_TO_FRONT;
            sub->turnTimer = 0;
        }
    }
}

static void PStaSub_HandleMarkings(PStatusWork *wk, PStaSubWork *sub) {
    TouchRect rects[7] = {
        { 127, 143, 179, 189 }, { 127, 143, 190, 200 }, { 127, 143, 201, 211 },      { 127, 143, 212, 222 },
        { 127, 143, 223, 233 }, { 127, 143, 234, 244 }, { TOUCH_RECT_END, 0, 0, 0 },
    };
    s32 hit;
    BoxPkm *pkm;
    u32 markings;
    u8 i;
    u8 bit;

    if (wk->param->mode == PSTATUS_MODE_LOCK_MARKINGS) {
        return;
    }
    hit = func_0203da0c(rects);
    if (hit == TOUCH_RECT_NONE) {
        return;
    }
    bit = 1;
    pkm = PStatus_GetBoxPkm(wk);
    markings = PML_PkmGetParam(pkm, PKM_PARAM_MARKINGS, NULL);
    markings ^= 1 << hit;
    PML_PkmSetParam(pkm, PKM_PARAM_MARKINGS, markings);
    for (i = 0; i < MARKING_COUNT; i++) {
        if (markings & bit) {
            func_0204c488(sub->icons[i], i * 2 + 1);
        } else {
            func_0204c488(sub->icons[i], i * 2);
        }
        bit <<= 1;
    }
}

static void PStaSub_TrackGestures(PStatusWork *wk, PStaSubWork *sub) {
    u32 x;
    u32 y;
    s8 dx;
    s8 dy;
    u32 dir;
    BOOL lap;

    func_0203da84(&x, &y);

    dx = x - sub->swipeX.startX;
    dy = y - sub->swipeX.startY;
    if ((sub->swipeX.dir == PSTA_SUB_DIR_NONE || sub->swipeX.dir == PSTA_SUB_DIR_LEFT) && dx > 16) {
        PStaSub_AdvanceGesture(&sub->swipeX, x, y, PSTA_SUB_DIR_RIGHT);
    }
    if ((sub->swipeX.dir == PSTA_SUB_DIR_NONE || sub->swipeX.dir == PSTA_SUB_DIR_RIGHT) && dx < -16) {
        PStaSub_AdvanceGesture(&sub->swipeX, x, y, PSTA_SUB_DIR_LEFT);
    }
    if (dy > 16 || dy < -16 || sub->swipeX.frames > 20) {
        PStaSub_ResetGesture(&sub->swipeX, x, y);
    }
    sub->swipeX.frames++;

    dx = x - sub->swipeY.startX;
    dy = y - sub->swipeY.startY;
    if ((sub->swipeY.dir == PSTA_SUB_DIR_NONE || sub->swipeY.dir == PSTA_SUB_DIR_UP) && dy > 16) {
        PStaSub_AdvanceGesture(&sub->swipeY, x, y, PSTA_SUB_DIR_DOWN);
    }
    if ((sub->swipeY.dir == PSTA_SUB_DIR_NONE || sub->swipeY.dir == PSTA_SUB_DIR_DOWN) && dy < -16) {
        PStaSub_AdvanceGesture(&sub->swipeY, x, y, PSTA_SUB_DIR_UP);
    }
    if (dx > 16 || dx < -16 || sub->swipeY.frames > 20) {
        PStaSub_ResetGesture(&sub->swipeY, x, y);
    }
    sub->swipeY.frames++;

    dx = x - sub->circle.startX;
    dy = y - sub->circle.startY;
    dir = PSTA_SUB_DIR_NONE;
    lap = FALSE;
    if (dx > 16) {
        dir = PSTA_SUB_DIR_RIGHT;
    } else if (dx < -16) {
        dir = PSTA_SUB_DIR_LEFT;
    } else if (dy > 16) {
        dir = PSTA_SUB_DIR_DOWN;
    } else if (dy < -16) {
        dir = PSTA_SUB_DIR_UP;
    }
    if (dir != PSTA_SUB_DIR_NONE && dir != sub->circle.dir) {
        if (sub->circle.dir + 1 == dir || (sub->circle.dir == PSTA_SUB_DIR_UP && dir == PSTA_SUB_DIR_RIGHT)) {
            if (sub->circleDir == PSTA_SUB_CIRCLE_NONE || sub->circleDir == PSTA_SUB_CIRCLE_CLOCKWISE) {
                sub->circleDir = PSTA_SUB_CIRCLE_CLOCKWISE;
                PStaSub_AdvanceGesture(&sub->circle, x, y, dir);
                if (sub->circle.dir == PSTA_SUB_DIR_UP) {
                    lap = TRUE;
                }
            } else {
                sub->circleDir = PSTA_SUB_CIRCLE_NONE;
                PStaSub_ResetGesture(&sub->circle, x, y);
            }
        } else if (sub->circle.dir - 1 == dir || (sub->circle.dir == PSTA_SUB_DIR_RIGHT && dir == PSTA_SUB_DIR_UP)) {
            if (sub->circleDir == PSTA_SUB_CIRCLE_COUNTERCLOCKWISE || sub->circleDir == PSTA_SUB_CIRCLE_NONE) {
                sub->circleDir = PSTA_SUB_CIRCLE_COUNTERCLOCKWISE;
                PStaSub_AdvanceGesture(&sub->circle, x, y, dir);
                if (sub->circle.dir == PSTA_SUB_DIR_UP) {
                    lap = TRUE;
                }
            } else {
                sub->circleDir = PSTA_SUB_CIRCLE_NONE;
                PStaSub_ResetGesture(&sub->circle, x, y);
            }
        } else if (sub->circle.dir == PSTA_SUB_DIR_NONE) {
            sub->circle.dir = dir;
        }
    }
    if (sub->circle.frames > 20) {
        sub->circleDir = PSTA_SUB_CIRCLE_NONE;
        PStaSub_ResetGesture(&sub->circle, x, y);
    }
    PStaSub_CountLap(wk, sub, lap);
    sub->circle.frames++;
}

static void PStaSub_ResetGesture(PStaSubGesture *gesture, u32 x, u32 y) {
    gesture->count = 0;
    gesture->dir = PSTA_SUB_DIR_NONE;
    gesture->frames = 0;
    gesture->startX = x;
    gesture->startY = y;
}

static void PStaSub_AdvanceGesture(PStaSubGesture *gesture, u32 x, u32 y, u32 dir) {
    gesture->count++;
    gesture->dir = dir;
    gesture->frames = 0;
    gesture->startX = x;
    gesture->startY = y;
}

static void PStaSub_CountLap(PStatusWork *wk, PStaSubWork *sub, BOOL lap) {
    if (lap == TRUE) {
        sub->lapFrames[sub->lapIndex] = sub->lapTimer;
        sub->lapTimer = 0;
        sub->lapIndex++;
        if (sub->lapIndex >= LAP_COUNT) {
            sub->lapIndex = 0;
        }
    }
    sub->lapTimer++;
}

static void PStaSub_CreateSprites(PStatusWork *wk, PStaSubWork *sub, BoxPkm *boxPkm) {
    VecFx32 size = { FX32_CONST(16), FX32_CONST(16), FX32_ONE };
    VecFx32 scale = { FX32_ONE, FX32_CONST(2.2), FX32_ONE };
    VecFx32 backScale = { FX32_CONST(1.8), FX32_CONST(2.5), FX32_ONE };
    VecFx32 pos = { FX32_CONST(4.7), 0, FX32_CONST(-32) };
    PartyPkm *pkm = PStatus_GetPartyPkm(wk);

    PStaSub_AdjustForSpecies(wk, sub, &size, &scale);
    sub->mcss[0] = func_0201c14c(wk->mcssSys, pkm, 0, FX32_CONST(216), FX32_CONST(72), 0);
    func_0201aeb0(sub->mcss[0], 2);
    func_0201ac5c(sub->mcss[0], DEG_TO_IDX(300));
    func_0201ac0c(sub->mcss[0], &scale);
    func_0201c290(sub->mcss[0]);
    func_0201abb8(sub->mcss[0], &size);
    sub->mcss[1] = func_0201c14c(wk->mcssSys, pkm, 1, FX32_CONST(216), FX32_CONST(72), 0);
    func_0201aeb0(sub->mcss[1], 2);
    func_0201ac5c(sub->mcss[1], DEG_TO_IDX(300));
    func_0201ac0c(sub->mcss[1], &backScale);
    func_0201c290(sub->mcss[1]);
    func_0201abb8(sub->mcss[1], &size);
    PStaSub_SetBounce(wk, sub, 0);
    PStaSub_SetSpritePosition(wk, sub, &pos);
    if (sub->frontShown == TRUE) {
        MCSS_Show(sub->mcss[0]);
        MCSS_Hide(sub->mcss[1]);
    } else {
        MCSS_Show(sub->mcss[1]);
        MCSS_Hide(sub->mcss[0]);
    }
}

static void PStaSub_RemoveSprites(PStatusWork *wk, PStaSubWork *sub) {
    MCSS_Hide(sub->mcss[0]);
    MCSS_Hide(sub->mcss[1]);
    MCSSSys_Remove(wk->mcssSys, sub->mcss[0]);
    MCSSSys_Remove(wk->mcssSys, sub->mcss[1]);
    sub->mcss[0] = NULL;
    sub->mcss[1] = NULL;
}

static void PStaSub_SetBounce(PStatusWork *wk, PStaSubWork *sub, fx32 bounce) {
    VecFx32 pos = { FX32_CONST(4.7), 0, FX32_CONST(-32) };

    sub->bounceOffset.x = FX_Mul(FX_SinIdx(DEG_TO_IDX(300)), bounce);
    sub->bounceOffset.y = FX_Mul(FX_CosIdx(DEG_TO_IDX(300)), bounce);
    sub->bounceOffset.z = 0;
    PStaSub_SetSpritePosition(wk, sub, &pos);
}

static void PStaSub_SetSpritePosition(PStatusWork *wk, PStaSubWork *sub, const VecFx32 *pos) {
    VecFx32 sum;

    VEC_Add(pos, &sub->bounceOffset, &sum);
    func_0201ac70(sub->mcss[0], &sum);
    func_0201ac70(sub->mcss[1], &sum);
}

static void PStaSub_AdjustForSpecies(PStatusWork *wk, PStaSubWork *sub, VecFx32 *size, VecFx32 *scale) {
    u32 species = PML_PkmGetParam(PStatus_GetBoxPkm(wk), PKM_PARAM_SPECIES, NULL);

    if (species == SPECIES_CHARMELEON) {
        scale->y = FX32_CONST(2.6);
        size->y = FX32_CONST(15.3);
    } else if (species == SPECIES_VULPIX) {
        size->y = FX32_CONST(15.7);
    } else if (species == SPECIES_SEEL) {
        size->y = size->x = FX32_CONST(16.4);
    } else if (species == SPECIES_CYNDAQUIL) {
        size->x = FX32_CONST(15.9);
        size->y = FX32_CONST(15.7);
    } else if (species == SPECIES_MAREEP) {
        size->y = size->x = FX32_CONST(16.2);
    } else if (species == SPECIES_DELIBIRD) {
        size->y = size->x = FX32_CONST(15.8);
    } else if (species == SPECIES_TYRANITAR) {
        size->y = size->x = FX32_CONST(15.8);
    } else if (species == SPECIES_MANECTRIC) {
        size->y = size->x = FX32_CONST(15.5);
    } else if (species == SPECIES_PLUSLE) {
        size->y = size->x = FX32_CONST(15.7);
    } else if (species == SPECIES_MONFERNO) {
        size->x = FX32_CONST(15.7);
        size->y = FX32_CONST(15.6);
    }
}
