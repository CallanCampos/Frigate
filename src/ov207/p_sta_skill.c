#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/math.h"
#include "p_status_local.h"
#include "pml/hm_check.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "system/app_menu_common.h"
#include "system/hp_gauge.h"

// The summary screen's skill page: the stats and ability, and the moves as plates of sprites, with the details of the
// move picked, swapping two moves, and, when a new move is learned, picking the move to forget. The names of the
// fields and functions are guesses

#define SKILL_WINDOW_COUNT 13
// Four moves, and the move to learn
#define PLATE_COUNT 5
#define PLATE_NEW 4
#define CURSOR_NONE 0xff
#define SLOT_NONE 5

// What a plate's animation shows, added to its slot times PLATE_COUNT
enum {
    PLATE_ANIM_NORMAL,
    PLATE_ANIM_SELECTED,
    PLATE_ANIM_HELD,
};

typedef struct {
    u8 x;
    u8 y;
} PlatePos;

// A move's plate: its frame and type icon, and the bitmap of its name and PP shown as sprites
typedef struct {
    u8 slot;
    BOOL isDirty;
    ClActor *plate;
    ClActor *typeIcon;
    GFLBitmap *bitmap;
    PStaOamActor *oam;
} SkillPlate;

struct PStaSkillWork {
    BOOL isShown;
    BOOL redrawStats;
    BOOL redrawDetail;
    // Not cleared when the work is allocated, so it is read before it is first set
    BOOL redrawMessage;
    BOOL isSwapPending;
    BOOL isSwapping;
    BOOL isConfirming;
    // Not cleared when the work is allocated, so it is read before it is first set
    BOOL isDragging;
    BOOL isExiting;
    BOOL forgetConfirmed;
    u8 exitTimer;
    u8 cursor;
    u8 swapSrc;
    u8 moveCount;
    u32 dragStartX;
    u32 dragStartY;
    PStaScreen screens[5];
    BmpWin *windows[SKILL_WINDOW_COUNT];
    // The stats' frame, the move category, a cursor that is never shown, the held move and the touched move
    ClActor *actors[5];
    PStaOam *oam;
    GFLBitmap *forgetBitmap;
    PStaOamActor *forgetOam;
    SkillPlate plates[PLATE_COUNT];
};

static void PStaSkill_PrintStats(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_DrawStats(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_PrintDetail(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_DrawDetail(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_PrintStatValues(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_DrawHpBar(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_PrintMoveDetail(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_HandleInput(PStatusWork *wk, PStaSkillWork *skill);
static BOOL PStaSkill_HandleKeys(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_HandleTouch(PStatusWork *wk, PStaSkillWork *skill);
static BOOL PStaSkill_HandleForgetKeys(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_HandleForgetTouch(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_MoveCursor(PStatusWork *wk, PStaSkillWork *skill);
static void PStaSkill_SelectMove(PStatusWork *wk, PStaSkillWork *skill, u8 slot, BOOL force);
static void PStaSkill_SwapMoves(PStatusWork *wk, PStaSkillWork *skill, u8 slotA, u8 slotB);
static void PStaSkill_ShowForgetPrompt(PStatusWork *wk, PStaSkillWork *skill, BOOL show);
static void PStaSkill_ShowHmMessage(PStatusWork *wk, PStaSkillWork *skill, BOOL isHm);
static u16 PStaSkill_GetStatColor(PStatusWork *wk, PStaSkillWork *skill, u8 nature, u32 stat);
static void PStaSkill_CreatePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate, u8 slot);
static void PStaSkill_FreePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate);
static void PStaSkill_UpdatePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate);
static void PStaSkill_LoadPlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate);
static void PStaSkill_DrawPlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate);
static void PStaSkill_UnloadPlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate);
static void PStaSkill_HidePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate);
static void PStaSkill_GetPlateRect(SkillPlate *plate, TouchRect *rect);
static void PStaSkill_GetPlatePos(SkillPlate *plate, ClActorPos *pos);
static void PStaSkill_SetPlateAnim(SkillPlate *plate, u32 anim);

static const PlatePos sPlatePos[PLATE_COUNT] = { { 1, 2 }, { 1, 6 }, { 1, 10 }, { 1, 14 }, { 1, 20 } };

static const PStaWindowSetup sSkillWindows[SKILL_WINDOW_COUNT] = {
    { 8, 4, 16, 2 },   { 17, 6, 6, 1 },  { 8, 7, 14, 2 },  { 8, 9, 14, 2 },  { 8, 11, 14, 2 },
    { 8, 13, 14, 2 },  { 8, 15, 14, 2 }, { 6, 18, 20, 6 }, { 10, 6, 14, 2 }, { 10, 8, 14, 2 },
    { 10, 10, 14, 2 }, { 1, 13, 30, 6 }, { 4, 20, 24, 4 },
};

PStaSkillWork *PStaSkill_Create(PStatusWork *wk) {
    PStaSkillWork *skill = GFL_HeapAllocate(wk->heapId, sizeof(PStaSkillWork), FALSE, "p_sta_skill.c", 334);
    u8 i;

    skill->isShown = FALSE;
    skill->redrawStats = FALSE;
    skill->redrawDetail = FALSE;
    skill->isSwapPending = FALSE;
    skill->isSwapping = FALSE;
    skill->isConfirming = FALSE;
    skill->forgetConfirmed = FALSE;
    skill->swapSrc = SLOT_NONE;
    skill->cursor = CURSOR_NONE;
    if (wk->param->mode == PSTATUS_MODE_FORGET_MOVE && wk->param->move != MOVE_NONE) {
        skill->moveCount = PLATE_COUNT;
    } else {
        skill->moveCount = PLATE_NEW;
    }
    for (i = 0; i < PLATE_COUNT; i++) {
        skill->plates[i].slot = CURSOR_NONE;
        skill->plates[i].isDirty = FALSE;
    }
    return skill;
}

void PStaSkill_Free(PStatusWork *wk, PStaSkillWork *skill) {
    GFL_HeapFree(skill);
}

void PStaSkill_Main(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;
    BOOL done;
    BmpWin *window;

    if (skill->redrawStats == FALSE && skill->redrawDetail == FALSE && skill->isSwapPending == FALSE &&
        skill->isExiting == FALSE) {
        PStaSkill_HandleInput(wk, skill);
    }
    if (skill->redrawStats == TRUE) {
        done = TRUE;
        for (i = 0; i <= 7; i++) {
            if (func_02021c1c(wk->printQueue, BmpWin_GetBitmap(skill->windows[i])) == TRUE) {
                done = FALSE;
                break;
            }
        }
        if (done == TRUE) {
            for (i = 0; i <= 7; i++) {
                window = skill->windows[i];
                BmpWin_FlushChar(window);
                BmpWin_FlushMap(window);
                GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
            }
            PStaSkill_DrawStats(wk, skill);
            skill->redrawStats = FALSE;
        }
    }
    if (skill->redrawDetail == TRUE) {
        done = TRUE;
        for (i = 8; i <= 11; i++) {
            if (func_02021c1c(wk->printQueue, BmpWin_GetBitmap(skill->windows[i])) == TRUE) {
                done = FALSE;
                break;
            }
        }
        if (done == TRUE) {
            for (i = 8; i <= 11; i++) {
                window = skill->windows[i];
                BmpWin_FlushChar(window);
                BmpWin_FlushMap(window);
                GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
            }
            PStaSkill_DrawDetail(wk, skill);
            skill->redrawDetail = FALSE;
        }
    }
    if (skill->redrawMessage == TRUE && func_02021c1c(wk->printQueue, BmpWin_GetBitmap(skill->windows[12])) == FALSE) {
        window = skill->windows[12];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
        skill->redrawMessage = FALSE;
    }
    if (skill->isSwapPending == TRUE && func_02021c0c(wk->printQueue) == TRUE) {
        skill->isSwapping = FALSE;
        PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_NORMAL);
        skill->swapSrc = SLOT_NONE;
        PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        func_0204c124(skill->actors[3], FALSE);
        skill->isSwapPending = FALSE;
    }
    for (i = 0; i < PLATE_COUNT; i++) {
        PStaSkill_UpdatePlate(wk, skill, &skill->plates[i]);
    }
    if (skill->isExiting == TRUE) {
        skill->exitTimer++;
        if (skill->exitTimer % 8 < 4) {
            if (skill->forgetConfirmed == TRUE) {
                func_0204c488(skill->plates[skill->cursor].plate, 15);
            } else {
                PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_NORMAL);
            }
        } else if (skill->forgetConfirmed == TRUE) {
            func_0204c488(skill->plates[skill->cursor].plate, 16);
        } else {
            PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_SELECTED);
        }
        if (skill->exitTimer > 16) {
            wk->seq = PSTA_SEQ_EXIT;
        }
    }
}

void PStaSkill_LoadResources(PStatusWork *wk, PStaSkillWork *skill, ArcTool *arc) {
    skill->screens[0].file = GFL_G2DIOReadNSCRArc(arc, 74, FALSE, &skill->screens[0].screen, wk->heapId);
    skill->screens[1].file = GFL_G2DIOReadNSCRArc(arc, 75, FALSE, &skill->screens[1].screen, wk->heapId);
    skill->screens[2].file = GFL_G2DIOReadNSCRArc(arc, 76, FALSE, &skill->screens[2].screen, wk->heapId);
    skill->screens[3].file = GFL_G2DIOReadNSCRArc(arc, 77, FALSE, &skill->screens[3].screen, wk->heapId);
    skill->screens[4].file = GFL_G2DIOReadNSCRArc(arc, 78, FALSE, &skill->screens[4].screen, wk->heapId);
    skill->oam = PStaOam_Create(wk->heapId, wk->actorUnit);
}

void PStaSkill_FreeResources(PStatusWork *wk, PStaSkillWork *skill) {
    if (skill->isShown == TRUE) {
        if (wk->param->mode == PSTATUS_MODE_FORGET_MOVE) {
            PStaSkill_UnloadForget(wk, skill);
        } else {
            PStaSkill_Unload(wk, skill);
        }
    }
    PStaOam_Free(skill->oam);
    GFL_HeapFree(skill->screens[0].file);
    GFL_HeapFree(skill->screens[1].file);
    GFL_HeapFree(skill->screens[2].file);
    GFL_HeapFree(skill->screens[3].file);
    GFL_HeapFree(skill->screens[4].file);
}

void PStaSkill_CreateActors(PStatusWork *wk, PStaSkillWork *skill) {
    ClActorSetup setup;
    u8 i;

    setup.x = 159;
    setup.y = 52;
    setup.priority = 10;
    setup.bgPriority = 1;
    setup.sequence = 0;
    skill->actors[0] =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(7)], wk->clResources[PSTA_RES_PLTT(5)],
                      wk->clResources[PSTA_RES_CELL(6)], &setup, 1, wk->heapId);
    func_0204c124(skill->actors[0], FALSE);
    setup.x = 172;
    setup.y = 56;
    skill->actors[1] =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(4)], wk->clResources[PSTA_RES_PLTT(4)],
                      wk->clResources[PSTA_RES_CELL(4)], &setup, 1, wk->heapId);
    func_0204c124(skill->actors[1], FALSE);
    setup.bgPriority = 0;
    skill->actors[2] =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(9)], wk->clResources[PSTA_RES_PLTT(11)],
                      wk->clResources[PSTA_RES_CELL(8)], &setup, 0, wk->heapId);
    func_0204c124(skill->actors[2], FALSE);
    setup.sequence = 5;
    setup.priority = 8;
    skill->actors[3] =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(9)], wk->clResources[PSTA_RES_PLTT(11)],
                      wk->clResources[PSTA_RES_CELL(8)], &setup, 0, wk->heapId);
    func_0204c124(skill->actors[3], FALSE);
    func_0204c520(skill->actors[3], TRUE);
    setup.sequence = 2;
    setup.priority = 4;
    setup.bgPriority = 0;
    skill->actors[4] =
        func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(9)], wk->clResources[PSTA_RES_PLTT(11)],
                      wk->clResources[PSTA_RES_CELL(8)], &setup, 0, wk->heapId);
    func_0204c124(skill->actors[4], FALSE);
    func_0204c520(skill->actors[4], TRUE);
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_CreatePlate(wk, skill, &skill->plates[i], i);
    }
}

void PStaSkill_FreeActors(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_FreePlate(wk, skill, &skill->plates[i]);
    }
    func_0204c108(skill->actors[0]);
    func_0204c108(skill->actors[1]);
    func_0204c108(skill->actors[2]);
    func_0204c108(skill->actors[3]);
    func_0204c108(skill->actors[4]);
}

void PStaSkill_Load(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    for (i = 0; i < SKILL_WINDOW_COUNT; i++) {
        skill->windows[i] = BmpWin_CreateDynamic(4, sSkillWindows[i].x, sSkillWindows[i].y, sSkillWindows[i].width,
                                                 sSkillWindows[i].height, 14, 1);
    }
    BmpWin_SetPalette(skill->windows[1], 13);
    PStaSkill_PrintStats(wk, skill);
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_LoadPlate(wk, skill, &skill->plates[i]);
    }
    skill->cursor = CURSOR_NONE;
    skill->isShown = TRUE;
    skill->isExiting = FALSE;
    skill->exitTimer = 0;
}

void PStaSkill_Draw(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    GFL_BGSysLoadScrArea(2, 0, 0, 32, 24, skill->screens[0].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(2);
    PStaSkill_DrawStats(wk, skill);
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_DrawPlate(wk, skill, &skill->plates[i]);
    }
    func_0204c488(wk->buttons[PSTA_BUTTON_SKILL], 4);
}

void PStaSkill_Unload(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_UnloadPlate(wk, skill, &skill->plates[i]);
    }
    for (i = 0; i < SKILL_WINDOW_COUNT; i++) {
        BmpWin_Free(skill->windows[i]);
    }
    skill->isShown = FALSE;
}

void PStaSkill_Clear(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_HidePlate(wk, skill, &skill->plates[i]);
    }
    GFL_BGSysFillScrArea(1, 0, 0, 0, 19, 21, 16);
    GFL_BGSysLoadScr(1);
    GFL_BGSysFillScrAsync(4, 0);
    GFL_BGSysQueueScrLoad(4);
    func_0204c124(skill->actors[0], FALSE);
    func_0204c124(skill->actors[1], FALSE);
    func_0204c124(wk->typeIcons[0], FALSE);
    func_0204c124(wk->typeIcons[1], FALSE);
    func_0204c488(wk->buttons[PSTA_BUTTON_SKILL], 1);
}

static void PStaSkill_PrintStats(PStatusWork *wk, PStaSkillWork *skill) {
    PStaSkill_PrintStatValues(wk, skill);
    PStaSkill_DrawHpBar(wk, skill);
}

static void PStaSkill_DrawStats(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;
    BmpWin *window;

    GFL_BGSysFillScrAsync(4, 0);
    for (i = 0; i <= 7; i++) {
        window = skill->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    }
    GFL_BGSysBufferScrDefault(6, skill->screens[1].screen->rawData, skill->screens[1].screen->size);
    GFL_BGSysQueueScrLoad(6);
    GFL_BGSysLoadScrArea(5, 0, 0, 32, 3, skill->screens[3].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(5);
    func_0204c124(skill->actors[0], TRUE);
    func_0204c124(skill->actors[1], FALSE);
    func_0204c124(wk->typeIcons[0], FALSE);
    func_0204c124(wk->typeIcons[1], FALSE);
}

static void PStaSkill_PrintDetail(PStatusWork *wk, PStaSkillWork *skill) {
    PStaSkill_PrintMoveDetail(wk, skill);
}

static void PStaSkill_DrawDetail(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;
    BmpWin *window;
    BoxPkm *pkm;
    u32 type1;
    u32 type2;
    ClActorPos pos;
    NNSG2dImageProxy proxy1;
    NNSG2dImageProxy proxy2;

    GFL_BGSysFillScrAsync(4, 0);
    for (i = 8; i <= 11; i++) {
        window = skill->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    }
    GFL_BGSysLoadScrCore(6, skill->screens[2].screen->rawData, skill->screens[2].screen->size, 0);
    GFL_BGSysBufferScrDefault(6, skill->screens[2].screen->rawData, skill->screens[2].screen->size);
    GFL_BGSysQueueScrLoad(6);
    GFL_BGSysLoadScrArea(5, 0, 0, 32, 3, skill->screens[4].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(5);
    func_0204c124(skill->actors[0], FALSE);
    func_0204c124(skill->actors[1], TRUE);
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_DrawPlate(wk, skill, &skill->plates[i]);
    }
    pkm = PStatus_GetBoxPkm(wk);
    type1 = PML_PkmGetParam(pkm, PKM_PARAM_TYPE1, NULL);
    type2 = PML_PkmGetParam(pkm, PKM_PARAM_TYPE2, NULL);
    func_0204bb58(wk->typeIconChars[type1], &proxy1);
    func_0204c3e4(wk->typeIcons[0], &proxy1);
    func_0204c378(wk->typeIcons[0], func_0202d7e8(type1), 1);
    pos.x = 208;
    pos.y = 120;
    func_0204c140(wk->typeIcons[0], &pos, 0);
    func_0204c124(wk->typeIcons[0], TRUE);
    func_0204c468(wk->typeIcons[0], 0);
    if (type1 != type2) {
        func_0204bb58(wk->typeIconChars[type2], &proxy2);
        func_0204c3e4(wk->typeIcons[1], &proxy2);
        func_0204c378(wk->typeIcons[1], func_0202d7e8(type2), 1);
        pos.x = 240;
        pos.y = 120;
        func_0204c140(wk->typeIcons[1], &pos, 0);
        func_0204c124(wk->typeIcons[1], TRUE);
        func_0204c468(wk->typeIcons[1], 0);
    } else {
        func_0204c124(wk->typeIcons[1], FALSE);
    }
}

static void PStaSkill_PrintStatValues(PStatusWork *wk, PStaSkillWork *skill) {
    PartyPkm *pkm = PStatus_GetPartyPkm(wk);
    u32 nature = PokeParty_GetParam(pkm, PKM_PARAM_NATURE, NULL);
    WordSet *wordSet;
    u32 ability;
    MsgData *msgData;
    StrBuf *str;

    PStatus_PrintToWindow(wk, skill->windows[0], 0x6e, 25, 1, PRINT_COLOR(15, 2, 0));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[0], wordSet, 0x77, 89, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);
    PStatus_PrintToWindow(wk, skill->windows[0], 0x75, 93, 1, PRINT_COLOR(1, 2, 0));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[0], wordSet, 0x76, 121, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);

    PStatus_PrintToWindow(wk, skill->windows[2], 0x6f, 1, 1, PStaSkill_GetStatColor(wk, skill, nature, 1));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_ATTACK, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[2], wordSet, 0x78, 105, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);
    PStatus_PrintToWindow(wk, skill->windows[3], 0x70, 1, 1, PStaSkill_GetStatColor(wk, skill, nature, 2));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_DEFENSE, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[3], wordSet, 0x79, 105, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);
    PStatus_PrintToWindow(wk, skill->windows[4], 0x71, 1, 1, PStaSkill_GetStatColor(wk, skill, nature, 3));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_SP_ATTACK, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[4], wordSet, 0x7a, 105, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);
    PStatus_PrintToWindow(wk, skill->windows[5], 0x72, 1, 1, PStaSkill_GetStatColor(wk, skill, nature, 4));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_SP_DEFENSE, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[5], wordSet, 0x7b, 105, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);
    PStatus_PrintToWindow(wk, skill->windows[6], 0x73, 1, 1, PStaSkill_GetStatColor(wk, skill, nature, 5));
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    WordSetNumber(wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_SPEED, NULL), 3, 1, 1);
    PStatus_PrintFormattedRightToWindow(wk, skill->windows[6], wordSet, 0x7c, 105, 1, PRINT_COLOR(1, 2, 0));
    GFL_WordSetSystemFree(wordSet);

    PStatus_PrintToWindow(wk, skill->windows[7], 0x74, 5, 1, PRINT_COLOR(15, 2, 0));
    ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_DESCRIPTIONS, wk->heapId);
    str = GFL_MsgDataLoadStrbufNew(msgData, ability);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(skill->windows[7]), 5, 17, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_NAMES, wk->heapId);
    str = GFL_MsgDataLoadStrbufNew(msgData, ability);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(skill->windows[7]), 65, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    skill->redrawStats = TRUE;
}

static void PStaSkill_DrawHpBar(PStatusWork *wk, PStaSkillWork *skill) {
    PartyPkm *pkm = PStatus_GetPartyPkm(wk);
    u16 maxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
    u16 hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
    u8 color = HPGauge_GetColor(hp, maxHp);
    u8 width = HPGauge_GetFill(hp, maxHp, 48);
    GFLBitmap *bitmap = BmpWin_GetBitmap(skill->windows[1]);
    u8 top;
    u8 bottom;

    if (color == HP_GAUGE_COLOR_RED || color == HP_GAUGE_COLOR_NONE) {
        top = 7;
        bottom = 8;
    } else if (color == HP_GAUGE_COLOR_YELLOW) {
        top = 9;
        bottom = 10;
    } else {
        top = 5;
        bottom = 6;
    }
    GFL_BitmapFillArea(bitmap, 0, 3, width, 1, top);
    GFL_BitmapFillArea(bitmap, 0, 4, width, 1, bottom);
}

static void PStaSkill_PrintMoveDetail(PStatusWork *wk, PStaSkillWork *skill) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u8 i;
    u32 move;
    u32 category;
    u32 power;
    WordSet *wordSet;
    u32 accuracy;
    MsgData *msgData;
    StrBuf *str;
    NNSG2dImageProxy proxy;

    for (i = 8; i <= 11; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(skill->windows[i]), 0);
    }
    if (skill->cursor < PLATE_NEW) {
        move = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + skill->cursor, NULL);
    } else if (skill->cursor == CURSOR_NONE) {
        if (wk->param->mode == PSTATUS_MODE_FORGET_MOVE && skill->moveCount == PLATE_COUNT) {
            move = wk->param->move;
        } else {
            move = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1, NULL);
        }
    } else {
        move = wk->param->move;
    }
    PStatus_PrintToWindow(wk, skill->windows[8], 0x95, 1, 1, PRINT_COLOR(15, 2, 0));
    category = PML_MoveGetCategory(move);
    func_0204bb58(wk->clResources[PSTA_RES_CHAR(4) + category], &proxy);
    func_0204c3e4(skill->actors[1], &proxy);
    func_0204c378(skill->actors[1], func_0202d800(category), 1);
    PStatus_PrintToWindow(wk, skill->windows[9], 0x93, 1, 1, PRINT_COLOR(15, 2, 0));
    power = PML_MoveGetBasePower(move);
    if (power <= 1) {
        PStatus_PrintToWindow(wk, skill->windows[9], 0x9a, 87, 1, PRINT_COLOR(1, 2, 0));
    } else {
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, power, 3, 1, 1);
        PStatus_PrintFormattedToWindow(wk, skill->windows[9], wordSet, 0x96, 81, 1, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(wordSet);
    }
    PStatus_PrintToWindow(wk, skill->windows[10], 0x94, 1, 1, PRINT_COLOR(15, 2, 0));
    accuracy = PML_MoveGetParam(move, MOVE_PARAM_ACCURACY);
    if (PML_MoveIsAlwaysHit(move) == TRUE || accuracy == 0) {
        PStatus_PrintToWindow(wk, skill->windows[10], 0x9a, 87, 1, PRINT_COLOR(1, 2, 0));
    } else {
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, accuracy, 3, 1, 1);
        PStatus_PrintFormattedToWindow(wk, skill->windows[10], wordSet, 0x97, 81, 1, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(wordSet);
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_13, wk->heapId);
    str = GFL_MsgDataLoadStrbufNew(msgData, move);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(skill->windows[11]), 1, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    skill->redrawMessage = FALSE;
    skill->redrawDetail = TRUE;
}

void PStaSkill_LoadForget(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;
    PStaOamSetup setup;

    for (i = 0; i < SKILL_WINDOW_COUNT; i++) {
        skill->windows[i] = BmpWin_CreateDynamic(4, sSkillWindows[i].x, sSkillWindows[i].y, sSkillWindows[i].width,
                                                 sSkillWindows[i].height, 14, 1);
    }
    BmpWin_SetPalette(skill->windows[1], 13);
    PStaSkill_PrintDetail(wk, skill);
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_LoadPlate(wk, skill, &skill->plates[i]);
    }
    if (skill->moveCount == PLATE_COUNT) {
        skill->forgetBitmap = GFL_BitmapCreate(13, 3, 32, wk->heapId);
        PStatus_PrintCentered(wk, skill->forgetBitmap, 0xc1, 52, 1, PRINT_COLOR(1, 2, 0));
        setup.x = 24;
        setup.y = 168;
        setup.palette = wk->clResources[PSTA_RES_PLTT(8)];
        setup.paletteOffset = 0;
        setup.priority = 0;
        setup.bgPriority = 0;
        setup.surface = 0;
        setup.vramType = 0;
        setup.bitmap = skill->forgetBitmap;
        skill->forgetOam = PStaOam_CreateActor(skill->oam, &setup);
        PStaOam_SetVisible(skill->forgetOam, FALSE);
    }
    skill->cursor = CURSOR_NONE;
    skill->isShown = TRUE;
    skill->isExiting = FALSE;
    skill->exitTimer = 0;
}

void PStaSkill_DrawForget(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    GFL_BGSysLoadScrArea(2, 0, 0, 32, 24, skill->screens[0].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(2);
    PStaSkill_DrawDetail(wk, skill);
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_DrawPlate(wk, skill, &skill->plates[i]);
    }
    func_0204c488(wk->buttons[PSTA_BUTTON_SKILL], 4);
    if (skill->moveCount == PLATE_COUNT) {
        skill->cursor = PLATE_NEW;
    } else {
        skill->cursor = 0;
    }
    PStatus_EnableInput(wk, FALSE);
    PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
}

void PStaSkill_UnloadForget(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    if (skill->moveCount == PLATE_COUNT) {
        GFL_BitmapFree(skill->forgetBitmap);
        PStaOam_FreeActor(skill->forgetOam);
    }
    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_UnloadPlate(wk, skill, &skill->plates[i]);
    }
    for (i = 0; i < SKILL_WINDOW_COUNT; i++) {
        BmpWin_Free(skill->windows[i]);
    }
    skill->isShown = FALSE;
}

void PStaSkill_ClearForget(PStatusWork *wk, PStaSkillWork *skill) {
    u8 i;

    for (i = 0; i < skill->moveCount; i++) {
        PStaSkill_HidePlate(wk, skill, &skill->plates[i]);
    }
    GFL_BGSysFillScrArea(1, 0, 0, 0, 19, 21, 16);
    GFL_BGSysLoadScr(1);
    GFL_BGSysFillScrAsync(4, 0);
    GFL_BGSysQueueScrLoad(4);
    func_0204c124(skill->actors[0], FALSE);
    func_0204c124(skill->actors[1], FALSE);
    func_0204c124(wk->typeIcons[0], FALSE);
    func_0204c124(wk->typeIcons[1], FALSE);
    func_0204c488(wk->buttons[PSTA_BUTTON_SKILL], 1);
}

static void PStaSkill_HandleInput(PStatusWork *wk, PStaSkillWork *skill) {
    if (wk->param->mode != PSTATUS_MODE_FORGET_MOVE) {
        if (PStaSkill_HandleKeys(wk, skill) == FALSE) {
            PStaSkill_HandleTouch(wk, skill);
        }
    } else if (PStaSkill_HandleForgetKeys(wk, skill) == FALSE) {
        PStaSkill_HandleForgetTouch(wk, skill);
    }
    if (func_0203da2c() == FALSE) {
        skill->isDragging = FALSE;
    }
}

static BOOL PStaSkill_HandleKeys(PStatusWork *wk, PStaSkillWork *skill) {
    ClActorPos pos;

    if (skill->isDragging == TRUE) {
        return FALSE;
    }
    if (wk->isInputEnabled == TRUE) {
        if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
            skill->cursor = 0;
            wk->isTouch = FALSE;
            PStatus_EnableInput(wk, FALSE);
            PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
            PStaSkill_PrintDetail(wk, skill);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return TRUE;
        }
    } else if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_B && skill->isSwapping == FALSE) {
        PStatus_EnableInput(wk, TRUE);
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_NORMAL);
        func_0204c124(skill->actors[2], FALSE);
        PStaSkill_PrintStats(wk, skill);
        wk->isTouch = FALSE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return TRUE;
    } else if (wk->isTouch == TRUE) {
        if (GCTX_HIDGetPressedKeys() != 0) {
            wk->isTouch = FALSE;
            skill->swapSrc = SLOT_NONE;
            PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return TRUE;
        }
    } else if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
        if (wk->param->mode != PSTATUS_MODE_LOCK_MARKINGS) {
            if (skill->isSwapping == FALSE) {
                skill->isSwapping = TRUE;
                skill->swapSrc = skill->cursor;
                PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_HELD);
                PStaSkill_GetPlatePos(&skill->plates[skill->swapSrc], &pos);
                func_0204c140(skill->actors[3], &pos, 0);
                func_0204c124(skill->actors[3], TRUE);
                func_0204c56c(skill->actors[3]);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            } else if (skill->swapSrc != skill->cursor) {
                PStaSkill_SwapMoves(wk, skill, skill->swapSrc, skill->cursor);
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
            } else {
                skill->isSwapping = FALSE;
                PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
                func_0204c124(skill->actors[3], FALSE);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            return TRUE;
        }
    } else if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_B && skill->isSwapping == TRUE) {
        PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_NORMAL);
        PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        func_0204c124(skill->actors[3], FALSE);
        skill->isSwapping = FALSE;
        skill->swapSrc = SLOT_NONE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return TRUE;
    } else if (GCTX_HIDGetPressedKeys() == PAD_KEY_UP || GCTX_HIDGetPressedKeys() == PAD_KEY_DOWN) {
        PStaSkill_MoveCursor(wk, skill);
        return TRUE;
    }
    return FALSE;
}

static void PStaSkill_HandleTouch(PStatusWork *wk, PStaSkillWork *skill) {
    ClActorPos touchPos;
    ClActorPos heldPos;
    TouchRect rects[PLATE_COUNT + 1];
    u8 i;
    s32 hit;

    if (wk->touchHit == PSTA_BUTTON_BACK && wk->isInputEnabled == FALSE) {
        PStatus_EnableInput(wk, TRUE);
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_NORMAL);
        func_0204c124(skill->actors[2], FALSE);
        PStaSkill_PrintStats(wk, skill);
        if (skill->isSwapping == TRUE) {
            PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_NORMAL);
            func_0204c124(skill->actors[3], FALSE);
            skill->isSwapping = FALSE;
            skill->swapSrc = SLOT_NONE;
        }
        wk->isTouch = TRUE;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return;
    }
    for (i = 0; i < PLATE_COUNT; i++) {
        PStaSkill_GetPlateRect(&skill->plates[i], &rects[i]);
    }
    rects[PLATE_COUNT].top = TOUCH_RECT_END;
    if (func_0203da48() == TRUE) {
        hit = func_0203da0c(rects);
        if (hit == TOUCH_RECT_NONE) {
            return;
        }
        if (PML_PkmGetParam(PStatus_GetBoxPkm(wk), PKM_PARAM_MOVE1 + hit, NULL) == MOVE_NONE) {
            return;
        }
        if (wk->isInputEnabled == TRUE) {
            PStatus_EnableInput(wk, FALSE);
        }
        if (wk->param->mode != PSTATUS_MODE_LOCK_MARKINGS) {
            skill->isDragging = TRUE;
            if (skill->isSwapping != TRUE) {
                skill->swapSrc = hit;
                PStaSkill_GetPlatePos(&skill->plates[hit], &heldPos);
                func_0204c140(skill->actors[4], &heldPos, 0);
                func_0204c124(skill->actors[4], TRUE);
                if (hit == 0) {
                    func_0204c488(skill->actors[4], 3);
                } else if (hit == 3) {
                    func_0204c488(skill->actors[4], 4);
                } else {
                    func_0204c488(skill->actors[4], 2);
                }
            }
            skill->dragStartX = wk->touchX;
            skill->dragStartY = wk->touchY;
            PStaSkill_GetPlatePos(&skill->plates[hit], &touchPos);
            func_0204c140(skill->actors[3], &touchPos, 0);
            func_0204c124(skill->actors[3], TRUE);
            func_0204c56c(skill->actors[3]);
        }
        wk->isTouch = TRUE;
        PStaSkill_SelectMove(wk, skill, hit, TRUE);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
    } else if (func_0203da2c() == TRUE) {
        if (skill->isDragging != TRUE) {
            return;
        }
        hit = func_0203d9c8(rects);
        if (skill->isSwapping == FALSE) {
            if (hit == TOUCH_RECT_NONE) {
                skill->isDragging = FALSE;
                func_0204c124(skill->actors[4], FALSE);
                func_0204c124(skill->actors[3], FALSE);
            } else if (MATH_ABS((int)(skill->dragStartY - wk->touchY)) > 8) {
                PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_HELD);
                skill->isSwapping = TRUE;
                func_0204c124(skill->actors[4], FALSE);
            }
        }
        if (skill->isSwapping == TRUE) {
            if (hit != TOUCH_RECT_NONE) {
                if (PML_PkmGetParam(PStatus_GetBoxPkm(wk), PKM_PARAM_MOVE1 + hit, NULL) == MOVE_NONE) {
                    return;
                }
                PStaSkill_SelectMove(wk, skill, hit, FALSE);
                return;
            }
            PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_NORMAL);
            func_0204c124(skill->actors[4], FALSE);
            func_0204c124(skill->actors[3], FALSE);
            skill->isSwapping = FALSE;
            skill->isDragging = FALSE;
            PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
            skill->swapSrc = SLOT_NONE;
        }
    } else if (skill->isDragging == TRUE) {
        func_0204c124(skill->actors[4], FALSE);
        func_0204c124(skill->actors[3], FALSE);
        if (skill->isSwapping == TRUE) {
            if (skill->swapSrc != skill->cursor) {
                PStaSkill_SwapMoves(wk, skill, skill->swapSrc, skill->cursor);
                GFL_SndSEPlay(SEQ_SE_DECIDE2);
                return;
            }
            skill->isSwapping = FALSE;
            PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        }
    }
}

static BOOL PStaSkill_HandleForgetKeys(PStatusWork *wk, PStaSkillWork *skill) {
    BOOL isHm;
    ClActorPos pos;

    if (wk->isTouch == TRUE && GCTX_HIDGetPressedKeys() != 0) {
        wk->isTouch = FALSE;
        if (skill->isConfirming == FALSE) {
            skill->swapSrc = SLOT_NONE;
            PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        }
    }
    if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_B) {
        if (skill->isConfirming == FALSE) {
            wk->isTouch = FALSE;
            wk->exitResult = PSTATUS_RESULT_BACK;
            wk->param->result = PSTATUS_RESULT_BACK;
            wk->seq = PSTA_SEQ_EXIT;
            func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
            wk->pressedButton = wk->buttons[PSTA_BUTTON_BACK];
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            return TRUE;
        }
        wk->isTouch = FALSE;
        PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_NORMAL);
        PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        func_0204c124(skill->actors[3], FALSE);
        skill->cursor = skill->swapSrc;
        skill->swapSrc = SLOT_NONE;
        skill->isConfirming = FALSE;
        PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
        PStaSkill_ShowForgetPrompt(wk, skill, FALSE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else if (skill->isConfirming) {
        if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
            wk->isTouch = FALSE;
            wk->exitResult = PSTATUS_RESULT_BACK;
            wk->param->slot = skill->swapSrc;
            wk->param->result = PSTATUS_RESULT_FORGET;
            skill->isExiting = TRUE;
            skill->forgetConfirmed = TRUE;
            wk->pressedButton = NULL;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    } else if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
        wk->isTouch = FALSE;
        if (skill->cursor < PLATE_NEW) {
            BoxPkm *pkm = PStatus_GetBoxPkm(wk);

            isHm = isPkmMoveHmMove(wk->param->gameData, PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + skill->cursor, NULL),
                                   wk->heapId);
            if (isHm == FALSE || wk->canForgetHm == TRUE) {
                if (wk->param->move == MOVE_NONE) {
                    wk->exitResult = PSTATUS_RESULT_BACK;
                    wk->param->result = PSTATUS_RESULT_FORGET;
                    wk->param->slot = skill->cursor;
                    skill->isExiting = TRUE;
                    wk->pressedButton = NULL;
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                } else {
                    skill->isConfirming = TRUE;
                    skill->swapSrc = skill->cursor;
                    PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_HELD);
                    skill->cursor = PLATE_NEW;
                    PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
                    PStaSkill_GetPlatePos(&skill->plates[skill->swapSrc], &pos);
                    func_0204c140(skill->actors[3], &pos, 0);
                    func_0204c124(skill->actors[3], TRUE);
                    func_0204c56c(skill->actors[3]);
                    PStaSkill_ShowForgetPrompt(wk, skill, TRUE);
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                }
            } else {
                PStaSkill_ShowHmMessage(wk, skill, isHm);
            }
        } else {
            wk->exitResult = PSTATUS_RESULT_BACK;
            wk->param->result = PSTATUS_RESULT_BACK;
            skill->isExiting = TRUE;
            wk->pressedButton = NULL;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        return TRUE;
    } else if (GCTX_HIDGetPressedKeys() == PAD_KEY_UP || GCTX_HIDGetPressedKeys() == PAD_KEY_DOWN) {
        PStaSkill_MoveCursor(wk, skill);
        return TRUE;
    }
    return FALSE;
}

static void PStaSkill_HandleForgetTouch(PStatusWork *wk, PStaSkillWork *skill) {
    ClActorPos pos;
    TouchRect rects[PLATE_COUNT + 1];
    u8 i;
    s32 hit;
    BOOL isHm;

    if (wk->touchHit == PSTA_BUTTON_BACK) {
        if (skill->isConfirming == FALSE) {
            wk->exitResult = PSTATUS_RESULT_BACK;
            wk->param->result = PSTATUS_RESULT_BACK;
            wk->seq = PSTA_SEQ_EXIT;
            func_0204c488(wk->buttons[PSTA_BUTTON_BACK], 9);
            wk->pressedButton = wk->buttons[PSTA_BUTTON_BACK];
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            return;
        }
        wk->isTouch = TRUE;
        PStaSkill_SetPlateAnim(&skill->plates[skill->swapSrc], PLATE_ANIM_NORMAL);
        PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
        func_0204c124(skill->actors[3], FALSE);
        skill->swapSrc = SLOT_NONE;
        skill->isConfirming = FALSE;
        if (skill->moveCount == PLATE_COUNT) {
            PStaSkill_SelectMove(wk, skill, PLATE_NEW, FALSE);
            PStaSkill_PrintDetail(wk, skill);
        } else {
            skill->cursor = CURSOR_NONE;
            PStaSkill_SelectMove(wk, skill, CURSOR_NONE, FALSE);
        }
        PStaSkill_ShowForgetPrompt(wk, skill, FALSE);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return;
    }
    for (i = 0; i < PLATE_COUNT; i++) {
        PStaSkill_GetPlateRect(&skill->plates[i], &rects[i]);
    }
    rects[PLATE_COUNT].top = TOUCH_RECT_END;
    hit = func_0203da0c(rects);
    if (hit == TOUCH_RECT_NONE) {
        return;
    }
    if (skill->isConfirming == FALSE) {
        if (hit < PLATE_NEW) {
            if (PML_PkmGetParam(PStatus_GetBoxPkm(wk), PKM_PARAM_MOVE1 + hit, NULL) == MOVE_NONE) {
                return;
            }
            isHm = isPkmMoveHmMove(wk->param->gameData,
                                   PML_PkmGetParam(PStatus_GetBoxPkm(wk), PKM_PARAM_MOVE1 + hit, NULL), wk->heapId);
            if (isHm == FALSE || wk->canForgetHm == TRUE) {
                if (wk->param->move == MOVE_NONE) {
                    wk->exitResult = PSTATUS_RESULT_BACK;
                    wk->param->result = PSTATUS_RESULT_FORGET;
                    wk->param->slot = hit;
                    skill->isExiting = TRUE;
                    wk->pressedButton = NULL;
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    PStaSkill_SelectMove(wk, skill, hit, FALSE);
                    PStaSkill_SetPlateAnim(&skill->plates[hit], PLATE_ANIM_HELD);
                    return;
                }
                wk->isTouch = TRUE;
                skill->isConfirming = TRUE;
                PStaSkill_SelectMove(wk, skill, hit, FALSE);
                skill->swapSrc = hit;
                PStaSkill_SetPlateAnim(&skill->plates[hit], PLATE_ANIM_HELD);
                skill->cursor = PLATE_NEW;
                PStaSkill_SelectMove(wk, skill, skill->cursor, FALSE);
                PStaSkill_GetPlatePos(&skill->plates[skill->swapSrc], &pos);
                func_0204c140(skill->actors[3], &pos, 0);
                func_0204c124(skill->actors[3], TRUE);
                func_0204c56c(skill->actors[3]);
                PStaSkill_ShowForgetPrompt(wk, skill, TRUE);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                return;
            }
            PStaSkill_SelectMove(wk, skill, hit, FALSE);
            skill->swapSrc = hit;
            PStaSkill_SetPlateAnim(&skill->plates[hit], PLATE_ANIM_HELD);
            PStaSkill_ShowHmMessage(wk, skill, isHm);
            return;
        }
        PStaSkill_SelectMove(wk, skill, hit, FALSE);
        wk->isTouch = TRUE;
        wk->exitResult = PSTATUS_RESULT_BACK;
        wk->param->result = PSTATUS_RESULT_BACK;
        skill->isExiting = TRUE;
        wk->pressedButton = NULL;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
    } else if (hit == PLATE_NEW) {
        wk->isTouch = TRUE;
        wk->exitResult = PSTATUS_RESULT_BACK;
        wk->param->slot = skill->swapSrc;
        wk->param->result = PSTATUS_RESULT_FORGET;
        skill->isExiting = TRUE;
        skill->forgetConfirmed = TRUE;
        wk->pressedButton = NULL;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
    }
}

static void PStaSkill_MoveCursor(PStatusWork *wk, PStaSkillWork *skill) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    int cursor = skill->cursor;
    int dir;
    u16 move;

    if (GCTX_HIDGetPressedKeys() == PAD_KEY_UP) {
        dir = -1;
    } else {
        dir = 1;
    }
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    do {
        cursor += dir;
        if (cursor < 0) {
            cursor = skill->moveCount - 1;
        } else if (cursor > skill->moveCount - 1) {
            cursor = 0;
        }
        if (cursor < PLATE_NEW) {
            move = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + cursor, NULL);
        } else {
            move = wk->param->move;
        }
    } while (move == MOVE_NONE);
    PStaSkill_SelectMove(wk, skill, cursor, FALSE);
}

static void PStaSkill_SelectMove(PStatusWork *wk, PStaSkillWork *skill, u8 slot, BOOL force) {
    ClActorPos pos;

    if (wk->isTouch == FALSE) {
        PStaSkill_GetPlatePos(&skill->plates[slot], &pos);
        func_0204c140(skill->actors[2], &pos, 0);
        func_0204c124(skill->actors[2], FALSE);
    } else {
        func_0204c124(skill->actors[2], FALSE);
    }
    if (skill->cursor != skill->swapSrc || skill->isSwapping == FALSE) {
        if (skill->cursor != CURSOR_NONE) {
            PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_NORMAL);
        }
    } else {
        PStaSkill_SetPlateAnim(&skill->plates[skill->cursor], PLATE_ANIM_HELD);
    }
    PStaSkill_SetPlateAnim(&skill->plates[slot], PLATE_ANIM_SELECTED);
    if (skill->cursor != slot || force == TRUE) {
        skill->cursor = slot;
        PStaSkill_PrintDetail(wk, skill);
    }
}

static void PStaSkill_SwapMoves(PStatusWork *wk, PStaSkillWork *skill, u8 slotA, u8 slotB) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u32 moveA;
    u32 ppA;
    u32 ppUpA;
    u32 moveB;
    u32 ppB;
    u32 ppUpB;

    PStatus_SetDecrypted(wk, TRUE);
    moveA = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + slotA, NULL);
    ppA = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP + slotA, NULL);
    ppUpA = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slotA, NULL);
    moveB = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + slotB, NULL);
    ppB = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP + slotB, NULL);
    ppUpB = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slotB, NULL);
    PML_PkmSetMove(pkm, moveB, slotA);
    PML_PkmSetMove(pkm, moveA, slotB);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + slotA, ppB);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slotA, ppUpB);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP + slotB, ppA);
    PML_PkmSetParam(pkm, PKM_PARAM_MOVE1_PP_UP + slotB, ppUpA);
    PStatus_SetDecrypted(wk, FALSE);
    PStaOam_SwapBitmaps(skill->plates[slotA].oam, skill->plates[slotB].oam);
    skill->plates[slotA].isDirty = TRUE;
    skill->plates[slotB].isDirty = TRUE;
    PStaSkill_PrintDetail(wk, skill);
    skill->isSwapPending = TRUE;
}

static void PStaSkill_ShowForgetPrompt(PStatusWork *wk, PStaSkillWork *skill, BOOL show) {
    SkillPlate *plate = &skill->plates[PLATE_NEW];

    if (show == TRUE) {
        func_0204c488(plate->plate, 16);
        PStaOam_SetVisible(plate->oam, FALSE);
        func_0204c124(plate->typeIcon, FALSE);
        PStaOam_Upload(skill->forgetOam);
        PStaOam_SetVisible(skill->forgetOam, TRUE);
    } else {
        PStaOam_SetVisible(skill->forgetOam, FALSE);
        func_0204c124(plate->typeIcon, TRUE);
        if (skill->cursor == PLATE_NEW) {
            PStaSkill_SetPlateAnim(plate, PLATE_ANIM_SELECTED);
        } else {
            PStaSkill_SetPlateAnim(plate, PLATE_ANIM_NORMAL);
        }
        PStaOam_SetVisible(plate->oam, TRUE);
    }
}

static void PStaSkill_ShowHmMessage(PStatusWork *wk, PStaSkillWork *skill, BOOL isHm) {
    skill->redrawMessage = TRUE;
    GFL_BitmapFill(BmpWin_GetBitmap(skill->windows[12]), 0);
    if (isHm == TRUE) {
        PStatus_PrintToWindow(wk, skill->windows[12], 0x9c, 0, 0, PRINT_COLOR(15, 2, 0));
    }
    GFL_SndSEPlay(SEQ_SE_BEEP);
}

static u16 PStaSkill_GetStatColor(PStatusWork *wk, PStaSkillWork *skill, u8 nature, u32 stat) {
    switch (statAffectedByNature(nature, stat)) {
    case 1:
        return PRINT_COLOR(15, 9, 0);
    case -1:
        return PRINT_COLOR(15, 10, 0);
    case 0:
    default:
        return PRINT_COLOR(15, 2, 0);
    }
}

static void PStaSkill_CreatePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate, u8 slot) {
    ClActorSetup setup;

    plate->isDirty = FALSE;
    plate->slot = slot;
    setup.x = sPlatePos[slot].x * 8;
    setup.y = sPlatePos[slot].y * 8;
    setup.priority = 10;
    setup.bgPriority = 0;
    setup.sequence = plate->slot;
    plate->plate = func_0204c040(wk->actorUnit, wk->clResources[PSTA_RES_CHAR(8)], wk->clResources[PSTA_RES_PLTT(6)],
                                 wk->clResources[PSTA_RES_CELL(7)], &setup, 0, wk->heapId);
    func_0204c124(plate->plate, FALSE);
    setup.x = sPlatePos[plate->slot].x * 8 + 26;
    setup.y = sPlatePos[plate->slot].y * 8 + 9;
    setup.priority = 8;
    setup.bgPriority = 0;
    setup.sequence = 0;
    plate->typeIcon = func_0204c040(wk->actorUnit, wk->typeIconChars[0], wk->clResources[PSTA_RES_PLTT(3)],
                                    wk->clResources[PSTA_RES_CELL(4)], &setup, 0, wk->heapId);
    func_0204c124(plate->typeIcon, FALSE);
}

static void PStaSkill_FreePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate) {
    func_0204c108(plate->plate);
    func_0204c108(plate->typeIcon);
    plate->slot = CURSOR_NONE;
}

static void PStaSkill_UpdatePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate) {
}

static void PStaSkill_LoadPlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u32 move;
    u32 pp;
    u32 maxPp;
    MsgData *msgData;
    StrBuf *str;
    WordSet *ppWordSet;
    WordSet *maxPpWordSet;
    PStaOamSetup setup;

    if (plate->slot < PLATE_NEW) {
        move = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + plate->slot, NULL);
        pp = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_PP + plate->slot, NULL);
        maxPp = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1_MAX_PP + plate->slot, NULL);
    } else {
        move = wk->param->move;
        maxPp = pp = PML_MoveGetParam(move, MOVE_PARAM_PP);
    }
    plate->bitmap = GFL_BitmapCreate(11, 4, 32, wk->heapId);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, wk->heapId);
    str = GFL_MsgDataLoadStrbufNew(msgData, move);
    func_02021c7c(wk->printQueue, plate->bitmap, 3, 2, str, wk->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
    if (move != MOVE_NONE) {
        PStatus_Print(wk, plate->bitmap, 0x87, 13, 17, PRINT_COLOR(1, 2, 0));
        ppWordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(ppWordSet, 0, pp, 3, 1, 1);
        PStatus_PrintFormattedRight(wk, plate->bitmap, ppWordSet, 0x88, 57, 17, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(ppWordSet);
        PStatus_Print(wk, plate->bitmap, 0x75, 57, 17, PRINT_COLOR(1, 2, 0));
        maxPpWordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(maxPpWordSet, 0, maxPp, 3, 1, 1);
        PStatus_PrintFormatted(wk, plate->bitmap, maxPpWordSet, 0x8d, 61, 17, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(maxPpWordSet);
    } else {
        PStatus_Print(wk, plate->bitmap, 0x99, 49, 17, PRINT_COLOR(1, 2, 0));
    }
    setup.x = (sPlatePos[plate->slot].x + 5) * 8;
    setup.y = sPlatePos[plate->slot].y * 8;
    setup.palette = wk->clResources[PSTA_RES_PLTT(8)];
    setup.paletteOffset = 0;
    setup.priority = 6;
    setup.bgPriority = 0;
    setup.surface = 0;
    setup.vramType = 0;
    setup.bitmap = plate->bitmap;
    plate->oam = PStaOam_CreateActor(skill->oam, &setup);
    plate->isDirty = TRUE;
}

static void PStaSkill_DrawPlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u32 move;
    u8 type;
    NNSG2dImageProxy proxy;

    if (plate->slot < PLATE_NEW) {
        move = PML_PkmGetParam(pkm, PKM_PARAM_MOVE1 + plate->slot, NULL);
    } else {
        move = wk->param->move;
    }
    if (plate->isDirty == TRUE) {
        if (move != MOVE_NONE) {
            type = PML_MoveGetType(move);
            func_0204bb58(wk->typeIconChars[type], &proxy);
            func_0204c3e4(plate->typeIcon, &proxy);
            func_0204c378(plate->typeIcon, func_0202d7e8(type), 1);
            func_0204c124(plate->typeIcon, TRUE);
        } else {
            func_0204c124(plate->typeIcon, FALSE);
        }
        PStaOam_Upload(plate->oam);
        func_0204c124(plate->plate, TRUE);
        plate->isDirty = FALSE;
    }
}

static void PStaSkill_UnloadPlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate) {
    GFL_BitmapFree(plate->bitmap);
    PStaOam_FreeActor(plate->oam);
}

static void PStaSkill_HidePlate(PStatusWork *wk, PStaSkillWork *skill, SkillPlate *plate) {
    func_0204c124(plate->plate, FALSE);
    func_0204c124(plate->typeIcon, FALSE);
}

static void PStaSkill_GetPlateRect(SkillPlate *plate, TouchRect *rect) {
    if (plate->slot < PLATE_COUNT) {
        rect->top = sPlatePos[plate->slot].y * 8;
        rect->bottom = rect->top + 32;
        rect->left = sPlatePos[plate->slot].x * 8;
        rect->right = rect->left + 136;
    } else {
        rect->top = 0;
        rect->bottom = 0;
        rect->left = 0;
        rect->right = 0;
    }
}

static void PStaSkill_GetPlatePos(SkillPlate *plate, ClActorPos *pos) {
    pos->x = sPlatePos[plate->slot].x * 8;
    pos->y = sPlatePos[plate->slot].y * 8;
}

static void PStaSkill_SetPlateAnim(SkillPlate *plate, u32 anim) {
    func_0204c488(plate->plate, plate->slot + anim * PLATE_COUNT);
}
