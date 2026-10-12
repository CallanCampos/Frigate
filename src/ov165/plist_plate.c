#include "app/pokelist.h"
#include "battle/regulation.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/musical.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "system/hp_gauge.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"

// A Pokémon's plate in the party list: its name, level, HP, icon, ball, item and status

struct PokeListPlate {
    PartyPkm *pkm;
    u8 index;
    // The HP shown, which moves toward targetHp
    u16 hp;
    u16 targetHp;
    BOOL isEgg;
    BOOL selected;
    BOOL isEmpty;
    // Whether the window waits for its text to be printed, to be copied to the BG
    BOOL nameDirty;
    // The Pokémon's place in the battle's order from 0, POKELIST_ENTRY_* otherwise
    int entry;
    BmpWin *window;
    ClActRenderer *renderer;
    ClActUnit *unit;
    u32 iconChars;
    ClActor *icon;
    ClActor *ball;
    ClActor *hpBar;
    ClActor *itemIcon;
    ClActor *statusIcon;
    // Frames into the icon's hop
    u8 bounceTimer;
};

static PokeListPlate *PokeListPlate_CreateCore(PokeListWork *wk, u8 index, PartyPkm *pkm);
static void PokeListPlate_CreateActors(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_CreateIcon(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_Draw(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly);
static void PokeListPlate_DrawInfo(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly);
static void PokeListPlate_DrawHpBar(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_DrawEgg(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_SetPalette(PokeListWork *wk, PokeListPlate *plate, u32 palette);
static void PokeListPlate_RedrawNumbers(PokeListWork *wk, PokeListPlate *plate);
static void PokeListPlate_UpdateEntry(PokeListWork *wk, PokeListPlate *plate);
static u8 PokeListPlate_GetHpColor(PokeListPlate *plate);
static void PokeListPlate_GetIconPos(PokeListPlate *plate, int x, int y, ClActorPos *pos);

// The tile that each plate's top left corner is at
static const u8 sPlatePos[POKELIST_PLATE_COUNT][2] = {
    { 0, 1 }, { 16, 2 }, { 0, 7 }, { 16, 8 }, { 0, 13 }, { 16, 14 },
};

// Where the plate's screen is in the plates' screen file: the first plate's, then the others'
static const u8 sPlateScreenPos[2][2] = {
    { 0, 1 },
    { 16, 2 },
};

PokeListPlate *PokeListPlate_Create(PokeListWork *wk, u8 index, PartyPkm *pkm) {
    return PokeListPlate_CreateCore(wk, index, pkm);
}

PokeListPlate *PokeListPlate_CreateEmpty(PokeListWork *wk, u8 index) {
    return PokeListPlate_CreateCore(wk, index, NULL);
}

static PokeListPlate *PokeListPlate_CreateCore(PokeListWork *wk, u8 index, PartyPkm *pkm) {
    u8 x = sPlatePos[index][0];
    u8 y = sPlatePos[index][1];
    int windowX;
    BOOL isEmpty;
    PokeListPlate *plate;
    u8 screenX;
    u8 screenY;
    ClActSurfaceSetup surfaceSetup;
    ClActorPos pos;

    plate = GFL_HeapAllocate(wk->heapId, sizeof(PokeListPlate), FALSE, "plist_plate.c", 175);
    plate->index = index;
    plate->pkm = pkm;
    isEmpty = FALSE;
    if (pkm == NULL) {
        isEmpty = TRUE;
    }
    plate->isEmpty = isEmpty;
    plate->selected = FALSE;
    plate->nameDirty = FALSE;
    if (pkm != NULL) {
        screenX = sPlateScreenPos[index != 0 ? 1 : 0][0];
        screenY = sPlateScreenPos[index != 0 ? 1 : 0][1];
    } else {
        screenX = 0;
        screenY = 19;
    }
    windowX = x + 16;
    GFL_BGSysLoadScrArea(2, windowX, y, 16, 6, wk->plateScreen->rawData, screenX, screenY, 32, 32);
    surfaceSetup.x = 0;
    surfaceSetup.y = 0;
    surfaceSetup.w = 256;
    surfaceSetup.h = 192;
    surfaceSetup.screen = 0;
    surfaceSetup.culling = 0;
    plate->renderer = func_0204be9c(&surfaceSetup, 1, wk->heapId);
    plate->unit = func_0204bf1c(6, 0, wk->heapId);
    func_0204c018(plate->unit, plate->renderer);
    PokeListPlate_CreateActors(wk, plate);
    PokeListPlate_CreateIcon(wk, plate);
    pos.x = -x * 8;
    pos.y = -y * 8;
    func_0204bedc(plate->renderer, 0, &pos);
    plate->window = BmpWin_CreateDynamic(1, windowX, y, 16, 6, 13, TRUE);
    if (pkm != NULL) {
        u8 i;

        plate->isEgg = PokeParty_GetParam(plate->pkm, PKM_PARAM_IS_EGG, NULL);
        plate->hp = PokeParty_GetParam(plate->pkm, PKM_PARAM_HP, NULL);
        plate->targetHp = plate->hp;
        func_0204c124(plate->hpBar, TRUE);
        PokeListPlate_UpdateEntry(wk, plate);
        if (PokeList_IsBattle(wk) == TRUE) {
            for (i = 0; i < 6; i++) {
                if (plate->index + 1 == wk->param->picked[i]) {
                    plate->entry = i;
                }
            }
        }
        PokeListPlate_SetSelected(wk, plate, FALSE);
        PokeListPlate_Draw(wk, plate, FALSE);
    } else {
        plate->entry = POKELIST_ENTRY_EMPTY;
        GFL_BGSysQueueScrLoad(2);
    }
    return plate;
}

void PokeListPlate_Free(PokeListWork *wk, PokeListPlate *plate) {
    BmpWin_Free(plate->window);
    func_0204c108(plate->icon);
    func_0204c108(plate->ball);
    func_0204c108(plate->hpBar);
    func_0204becc(plate->renderer);
    func_0204bf98(plate->unit);
    func_0204b98c(plate->iconChars);
    GFL_HeapFree(plate);
}

BOOL PokeListPlate_IsPrinting(PokeListPlate *plate) {
    return plate->nameDirty;
}

void PokeListPlate_Update(PokeListWork *wk, PokeListPlate *plate) {
    if (plate->isEmpty == FALSE) {
        if (plate->nameDirty == TRUE && func_02021c1c(wk->printQueue, BmpWin_GetBitmap(plate->window)) == FALSE) {
            plate->nameDirty = FALSE;
            BmpWin_Transfer(plate->window);
        }
        if (plate->selected == TRUE && GetStatusCond(plate->pkm) == 0 && plate->hp != 0) {
            ClActorPos pos;

            PokeListPlate_GetIconPos(plate, 26, 19, &pos);
            if (plate->bounceTimer >= 10) {
                pos.y -= 6;
                if (plate->bounceTimer >= 20) {
                    plate->bounceTimer = 0;
                }
            }
            plate->bounceTimer++;
            func_0204c140(plate->icon, &pos, 0);
        }
    }
}

static void PokeListPlate_CreateActors(PokeListWork *wk, PokeListPlate *plate) {
    ClActorSetup ballSetup;
    ClActorSetup hpBarSetup;
    ClActorSetup itemSetup;
    ClActorSetup statusSetup;

    ballSetup.x = 16;
    ballSetup.y = 16;
    ballSetup.sequence = 0;
    ballSetup.priority = 1;
    ballSetup.bgPriority = 2;
    plate->ball = func_0204c040(plate->unit, wk->clResources[CL_RES_CHAR(1)], wk->clResources[CL_RES_PLTT(0)],
                                wk->clResources[CL_RES_CELL(1)], &ballSetup, 0, wk->heapId);
    func_0204c520(plate->ball, TRUE);
    hpBarSetup.x = 84;
    hpBarSetup.y = 28;
    hpBarSetup.sequence = 0;
    hpBarSetup.priority = 0;
    hpBarSetup.bgPriority = 2;
    plate->hpBar = func_0204c040(plate->unit, wk->clResources[CL_RES_CHAR(6)], wk->clResources[CL_RES_PLTT(5)],
                                 wk->clResources[CL_RES_CELL(6)], &hpBarSetup, 0, wk->heapId);
    func_0204c124(plate->hpBar, FALSE);
    itemSetup.x = 32;
    itemSetup.y = 24;
    itemSetup.sequence = 0;
    itemSetup.priority = 0;
    itemSetup.bgPriority = 2;
    plate->itemIcon = func_0204c040(plate->unit, wk->clResources[CL_RES_CHAR(3)], wk->clResources[CL_RES_PLTT(2)],
                                    wk->clResources[CL_RES_CELL(3)], &itemSetup, 0, wk->heapId);
    func_0204c124(plate->itemIcon, FALSE);
    statusSetup.x = 32;
    statusSetup.y = 42;
    statusSetup.sequence = 0;
    statusSetup.priority = 0;
    statusSetup.bgPriority = 2;
    plate->statusIcon = func_0204c040(plate->unit, wk->clResources[CL_RES_CHAR(5)], wk->clResources[CL_RES_PLTT(4)],
                                      wk->clResources[CL_RES_CELL(5)], &statusSetup, 0, wk->heapId);
    func_0204c124(plate->statusIcon, FALSE);
}

static void PokeListPlate_CreateIcon(PokeListWork *wk, PokeListPlate *plate) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(7, HEAPID_TAIL(wk->heapId));
    u32 file;
    u32 palette;
    ClActorSetup setup;

    if (plate->pkm != NULL) {
        BoxPkm *pkm = func_0201d624(plate->pkm);

        file = PokeIcon_GetFileOfPkm(pkm);
        palette = PokeIcon_GetPaletteOfPkm(pkm);
    } else {
        file = PokeParty_GetIconIndex(1, 0, 0, TRUE);
        palette = PokeIcon_GetPalette(1, 0, 0, TRUE);
    }
    plate->iconChars = func_0204b81c(arc, file, FALSE, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    setup.x = 24;
    setup.y = 16;
    setup.sequence = 1;
    setup.priority = 0;
    setup.bgPriority = 2;
    plate->icon = func_0204c040(plate->unit, plate->iconChars, wk->clResources[CL_RES_PLTT(6)],
                                wk->clResources[CL_RES_CELL(7)], &setup, 0, wk->heapId);
    func_0204c378(plate->icon, palette, 1);
    func_0204c520(plate->icon, TRUE);
    func_0204c124(plate->icon, plate->pkm != NULL ? TRUE : FALSE);
}

static void PokeListPlate_Draw(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly) {
    if (plate->isEgg == FALSE) {
        PokeListPlate_DrawInfo(wk, plate, numbersOnly);
        PokeListPlate_DrawHpBar(wk, plate);
    } else {
        PokeListPlate_DrawEgg(wk, plate);
    }
}

// Prints the name, sex and level, and the HP or what the Pokémon can do in the mode; numbersOnly draws the numbers
// at once, as the HP changes
static void PokeListPlate_DrawInfo(PokeListWork *wk, PokeListPlate *plate, BOOL numbersOnly) {
    BOOL showLearn = FALSE;
    u32 item;

    if (wk->param->mode == 6 || wk->wasMode18 == TRUE) {
        showLearn = TRUE;
    }
    if (numbersOnly == FALSE) {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

        loadPokemonNicknameToStrbuf(wordSet, 0, plate->pkm);
        PokeList_PrintWordSetString(wk, plate->window, wordSet, 3, 41, 9, 0x440);
        GFL_WordSetSystemFree(wordSet);
    }
    if (numbersOnly == FALSE && PokeParty_GetParam(plate->pkm, PKM_PARAM_SHOW_SEX, NULL) == TRUE) {
        u32 sex = PokeParty_GetParam(plate->pkm, PKM_PARAM_SEX, NULL);

        if (sex == GENDER_MALE) {
            PokeList_PrintString(wk, plate->window, 7, 105, 9, 0x14c0);
        } else if (sex == GENDER_FEMALE) {
            PokeList_PrintString(wk, plate->window, 8, 105, 9, 0xc80);
        }
    }
    if (showLearn == FALSE) {
        if ((GetStatusCond(plate->pkm) == 0 && plate->hp != 0) || plate->entry != POKELIST_ENTRY_EMPTY) {
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

            WordSetNumber(wordSet, 0, PokeParty_GetLevel(plate->pkm), 3, 0, TRUE);
            if (numbersOnly == FALSE) {
                PokeList_PrintWordSetStringSmall(wk, plate->window, wordSet, 2, 9, 35, 0x440);
            } else {
                PokeList_DrawWordSetStringSmall(wk, plate->window, wordSet, 2, 9, 35, 0x440);
            }
            GFL_WordSetSystemFree(wordSet);
            func_0204c124(plate->statusIcon, FALSE);
        } else {
            if (plate->hp == 0) {
                func_0204c488(plate->statusIcon, 6);
            } else {
                switch (GetStatusCond(plate->pkm)) {
                case 1:
                    func_0204c488(plate->statusIcon, 1);
                    break;
                case 2:
                    func_0204c488(plate->statusIcon, 3);
                    break;
                case 3:
                    func_0204c488(plate->statusIcon, 2);
                    break;
                case 4:
                    func_0204c488(plate->statusIcon, 5);
                    break;
                case 5:
                    func_0204c488(plate->statusIcon, 4);
                    break;
                }
            }
            func_0204c124(plate->statusIcon, TRUE);
        }
    }
    if (plate->entry != POKELIST_ENTRY_EMPTY) {
        u32 msgId;

        if (plate->entry == POKELIST_ENTRY_ABLE) {
            msgId = 0x8d;
        } else if (plate->entry == POKELIST_ENTRY_UNABLE) {
            msgId = 0x8e;
        } else {
            msgId = plate->entry + 0x8f;
        }
        PokeList_PrintString(wk, plate->window, msgId, 46, 31, 0x440);
        func_0204c124(plate->hpBar, FALSE);
    } else if (showLearn == TRUE) {
        u32 msgId;

        switch (PokeList_CheckLearnMove(wk, plate->pkm, plate->index)) {
        case 0:
        case 1:
            msgId = 0x95;
            break;
        case 2:
            msgId = 0x96;
            break;
        case 3:
            msgId = 0x97;
            break;
        }
        PokeList_PrintString(wk, plate->window, msgId, 29, 31, 0x440);
        func_0204c124(plate->hpBar, FALSE);
    } else if (wk->param->mode == 16) {
        u32 msgId = PokeList_CanEvolveWithItem(wk, plate->pkm, wk->param->item) == FALSE ? 0x99 : 0x98;

        PokeList_PrintString(wk, plate->window, msgId, 46, 31, 0x440);
        func_0204c124(plate->hpBar, FALSE);
    } else if (wk->wasMode19 == TRUE) {
        u32 msgId = MusicalSystem_CanJoin(plate->pkm) == TRUE ? 0x9a : 0x9b;

        PokeList_PrintString(wk, plate->window, msgId, 46, 31, 0x440);
    } else {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

        WordSetNumber(wordSet, 0, PokeParty_GetParam(plate->pkm, PKM_PARAM_MAX_HP, NULL), 3, 0, TRUE);
        if (numbersOnly == FALSE) {
            PokeList_PrintWordSetStringSmall(wk, plate->window, wordSet, 4, 91, 35, 0x440);
        } else {
            PokeList_DrawWordSetStringSmall(wk, plate->window, wordSet, 4, 91, 35, 0x440);
        }
        GFL_WordSetSystemFree(wordSet);
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, plate->hp, 3, 1, TRUE);
        if (numbersOnly == FALSE) {
            PokeList_PrintWordSetStringSmall(wk, plate->window, wordSet, 5, 57, 35, 0x440);
        } else {
            PokeList_DrawWordSetStringSmall(wk, plate->window, wordSet, 5, 57, 35, 0x440);
        }
        GFL_WordSetSystemFree(wordSet);
        if (numbersOnly == FALSE) {
            PokeList_PrintStringSmall(wk, plate->window, 6, 82, 35, 0x440);
        } else {
            PokeList_DrawStringSmall(wk, plate->window, 6, 82, 35, 0x440);
        }
    }
    item = PokeParty_GetParam(plate->pkm, PKM_PARAM_ITEM, NULL);
    if (item != 0) {
        func_0204c124(plate->itemIcon, TRUE);
        if (PML_ItemIsMail(item) == TRUE) {
            func_0204c488(plate->itemIcon, 1);
        } else {
            func_0204c488(plate->itemIcon, 0);
        }
    } else {
        func_0204c124(plate->itemIcon, FALSE);
    }
    {
        u32 color = PokeListPlate_GetHpColor(plate);
        u16 sequence = 1;

        if (color == HP_GAUGE_COLOR_NONE) {
            sequence = 0;
        } else if (GetStatusCond(plate->pkm) != 0) {
            sequence = 5;
        } else if (color == HP_GAUGE_COLOR_RED) {
            sequence = 4;
        } else if (color == HP_GAUGE_COLOR_YELLOW) {
            sequence = 3;
        } else if (color == HP_GAUGE_COLOR_GREEN) {
            sequence = 2;
        }
        func_0204c488(plate->icon, sequence);
    }
    plate->nameDirty = TRUE;
}

static void PokeListPlate_DrawHpBar(PokeListWork *wk, PokeListPlate *plate) {
    if (PokeList_IsBattle(wk) == FALSE && wk->param->mode != 6) {
        u8 color = PokeListPlate_GetHpColor(plate);
        u8 width = HPGauge_GetFill(plate->hp, PokeParty_GetParam(plate->pkm, PKM_PARAM_MAX_HP, NULL), 48);
        GFLBitmap *bitmap = BmpWin_GetBitmap(plate->window);
        u8 topColor;
        u8 bottomColor;

        if (color == HP_GAUGE_COLOR_RED || color == HP_GAUGE_COLOR_NONE) {
            topColor = 14;
            bottomColor = 15;
        } else if (color == HP_GAUGE_COLOR_YELLOW) {
            topColor = 12;
            bottomColor = 13;
        } else {
            topColor = 10;
            bottomColor = 11;
        }
        GFL_BitmapFillArea(bitmap, 61, 27, width, 1, topColor);
        GFL_BitmapFillArea(bitmap, 61, 28, width, 1, bottomColor);
        func_0204c124(plate->hpBar, TRUE);
    }
}

static void PokeListPlate_DrawEgg(PokeListWork *wk, PokeListPlate *plate) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);

    func_0202437c(wordSet, 0, name, 0, 1, 2);
    PokeList_PrintWordSetString(wk, plate->window, wordSet, 3, 41, 9, 0x440);
    GFL_StrBufFree(name);
    GFL_WordSetSystemFree(wordSet);
    func_0204c124(plate->hpBar, FALSE);
    func_0204c488(plate->icon, 1);
    plate->nameDirty = TRUE;
}

void PokeListPlate_SetSelected(PokeListWork *wk, PokeListPlate *plate, BOOL selected) {
    if (selected == TRUE) {
        ClActorPos pos;

        if (wk->state == 3 || wk->state == 4 || wk->state == 23) {
            PokeListPlate_SetPalette(wk, plate, 8);
        } else if (PokeList_IsBattle(wk) == TRUE && plate->entry < POKELIST_ENTRY_ABLE) {
            PokeListPlate_SetPalette(wk, plate, 8);
        } else {
            PokeListPlate_SetPalette(wk, plate, 6);
        }
        func_0204c488(plate->ball, 1);
        PokeListPlate_GetIconPos(plate, 26, 19, &pos);
        func_0204c140(plate->icon, &pos, 0);
        plate->bounceTimer = 0;
    } else {
        ClActorPos pos;

        if ((wk->state == 3 && wk->selectPos == plate->index) || (wk->state == 23 && wk->selectPos == plate->index) ||
            (wk->state == 4 && wk->selectPos2 == plate->index)) {
            PokeListPlate_SetPalette(wk, plate, 5);
        } else if (PokeList_IsBattle(wk) == TRUE && plate->entry < POKELIST_ENTRY_ABLE) {
            PokeListPlate_SetPalette(wk, plate, 5);
        } else {
            PokeListPlate_SetPalette(wk, plate, 3);
        }
        func_0204c488(plate->ball, 0);
        PokeListPlate_GetIconPos(plate, 24, 16, &pos);
        func_0204c140(plate->icon, &pos, 0);
    }
    plate->selected = selected;
}

// The palette of a plate: the fainted ones' palette follows each of 3 and 6
static void PokeListPlate_SetPalette(PokeListWork *wk, PokeListPlate *plate, u32 palette) {
    if (palette == 3 && PokeListPlate_GetHpColor(plate) == HP_GAUGE_COLOR_NONE) {
        GFL_BGSysSetScrPaletteNo(2, sPlatePos[plate->index][0] + 16, sPlatePos[plate->index][1], 16, 6, 4);
    } else if (palette == 6 && PokeListPlate_GetHpColor(plate) == HP_GAUGE_COLOR_NONE) {
        GFL_BGSysSetScrPaletteNo(2, sPlatePos[plate->index][0] + 16, sPlatePos[plate->index][1], 16, 6, 7);
    } else {
        GFL_BGSysSetScrPaletteNo(2, sPlatePos[plate->index][0] + 16, sPlatePos[plate->index][1], 16, 6, palette);
    }
    GFL_BGSysQueueScrLoad(2);
}

// Draws the plate offset by a step of its slide, to the left for the left column and to the right for the right
void PokeListPlate_DrawSlid(PokeListWork *wk, PokeListPlate *plate, int step) {
    u8 x = sPlatePos[plate->index][0];
    u8 y = sPlatePos[plate->index][1];
    ClActorPos pos;

    if (plate->index % 2 == 0) {
        step = -step;
    }
    BmpWin_SetPosX(plate->window, step + (x + 16));
    pos.x = -(x + step) * 8;
    pos.y = -y * 8;
    func_0204bedc(plate->renderer, 0, &pos);
    GFL_BGSysLoadScrArea(2, x + 16 + step, y, 16, 6, wk->plateScreen->rawData,
                         sPlateScreenPos[plate->index != 0 ? 1 : 0][0], sPlateScreenPos[plate->index != 0 ? 1 : 0][1],
                         32, 32);
    GFL_BGSysSetScrPaletteNo(2, x + 16 + step, y, 16, 6, 5);
    BmpWin_FlushMap(plate->window);
    GFL_BGSysQueueScrLoad(1);
    GFL_BGSysQueueScrLoad(2);
}

// Clears where the plate is drawn at a step of its slide
void PokeListPlate_ClearSlid(PokeListWork *wk, PokeListPlate *plate, int step) {
    u8 x = sPlatePos[plate->index][0];
    u8 y = sPlatePos[plate->index][1];
    u32 left;

    if (plate->index % 2 == 0) {
        step = -step;
    }
    left = x + 16 + step;
    GFL_BGSysFillScrArea(2, 0, left, y, 16, 6, 5);
    GFL_BGSysFillScrArea(1, 0, left, y, 16, 6, 5);
}

// Shows another Pokémon on the plate, as swapping two does
void PokeListPlate_SetPkm(PokeListWork *wk, PokeListPlate *plate, PartyPkm *pkm, int step) {
    u8 x = sPlatePos[plate->index][0];
    u8 y = sPlatePos[plate->index][1];
    ClActorPos origin;
    ClActorPos pos;

    if (plate->index % 2 == 0) {
        step = -step;
    }
    plate->pkm = pkm;
    plate->nameDirty = FALSE;
    plate->isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    plate->hp = PokeParty_GetParam(plate->pkm, PKM_PARAM_HP, NULL);
    plate->targetHp = plate->hp;
    func_0204c108(plate->icon);
    func_0204b98c(plate->iconChars);
    GFL_BitmapFill(BmpWin_GetBitmap(plate->window), 0);
    func_0204c124(plate->itemIcon, FALSE);
    func_0204c124(plate->statusIcon, FALSE);
    origin.x = 0;
    origin.y = 0;
    func_0204bedc(plate->renderer, 0, &origin);
    PokeListPlate_CreateIcon(wk, plate);
    pos.x = -(x + step) * 8;
    pos.y = -y * 8;
    func_0204bedc(plate->renderer, 0, &pos);
    PokeListPlate_Draw(wk, plate, FALSE);
}

void PokeListPlate_Redraw(PokeListWork *wk, PokeListPlate *plate) {
    GFL_BitmapFill(BmpWin_GetBitmap(plate->window), 0);
    PokeListPlate_Draw(wk, plate, FALSE);
}

static void PokeListPlate_RedrawNumbers(PokeListWork *wk, PokeListPlate *plate) {
    GFL_BitmapFillArea(BmpWin_GetBitmap(plate->window), 0, 26, 128, 22, 0);
    PokeListPlate_Draw(wk, plate, TRUE);
}

// Starts moving the HP shown toward the Pokémon's HP, by a hundredth of the change each frame
void PokeListPlate_StartHpChange(PokeListWork *wk, PokeListPlate *plate) {
    plate->targetHp = PokeParty_GetParam(plate->pkm, PKM_PARAM_HP, NULL);
    wk->hpStep = (plate->targetHp - plate->hp) / 100;
    if (plate->targetHp > plate->hp) {
        wk->hpStep++;
    } else {
        wk->hpStep--;
    }
}

// Moves the HP shown, and returns TRUE once it is the Pokémon's
BOOL PokeListPlate_UpdateHpChange(PokeListWork *wk, PokeListPlate *plate) {
    if (plate->hp < plate->targetHp) {
        if (plate->hp == 0) {
            PokeListPlate_SetPalette(wk, plate, 6);
        }
        if (plate->hp + wk->hpStep > plate->targetHp) {
            plate->hp = plate->targetHp;
        } else {
            plate->hp += wk->hpStep;
        }
        PokeListPlate_RedrawNumbers(wk, plate);
        return FALSE;
    }
    if (plate->hp > plate->targetHp) {
        if (plate->hp + wk->hpStep < plate->targetHp) {
            plate->hp = plate->targetHp;
        } else {
            plate->hp += wk->hpStep;
        }
        PokeListPlate_RedrawNumbers(wk, plate);
        return FALSE;
    }
    return TRUE;
}

BOOL PokeListPlate_IsValid(PokeListWork *wk, PokeListPlate *plate) {
    if (plate->isEmpty == TRUE) {
        return FALSE;
    }
    return TRUE;
}

// Where the cursor is shown on the plate
void PokeListPlate_GetCursorPos(PokeListWork *wk, PokeListPlate *plate, ClActorPos *pos) {
    pos->x = (sPlatePos[plate->index][0] + 8) * 8;
    pos->y = (sPlatePos[plate->index][1] + 3) * 8;
}

void PokeListPlate_GetTouchRect(PokeListWork *wk, PokeListPlate *plate, TouchRect *rect) {
    rect->left = sPlatePos[plate->index][0] * 8;
    rect->top = sPlatePos[plate->index][1] * 8;
    rect->right = (sPlatePos[plate->index][0] + 16) * 8;
    rect->bottom = (sPlatePos[plate->index][1] + 6) * 8;
}

int PokeListPlate_GetEntry(PokeListPlate *plate) {
    return plate->entry;
}

void PokeListPlate_SetEntry(PokeListWork *wk, PokeListPlate *plate, int entry) {
    if (plate->entry != entry) {
        if (entry < POKELIST_ENTRY_ABLE) {
            plate->entry = entry;
        } else {
            PokeListPlate_UpdateEntry(wk, plate);
        }
        PokeListPlate_Redraw(wk, plate);
    }
}

u16 PokeListPlate_GetHp(PokeListWork *wk, PokeListPlate *plate) {
    return plate->hp;
}

BOOL PokeListPlate_IsEgg(PokeListWork *wk, PokeListPlate *plate) {
    return plate->isEgg;
}

static void PokeListPlate_UpdateEntry(PokeListWork *wk, PokeListPlate *plate) {
    if (PokeList_IsBattle(wk) == FALSE) {
        plate->entry = POKELIST_ENTRY_EMPTY;
    } else if (PokeRegulation_CheckPokemonExtraRule(wk->param->regulation, wk->param->unk18, plate->pkm) == 0) {
        plate->entry = POKELIST_ENTRY_ABLE;
    } else {
        plate->entry = POKELIST_ENTRY_UNABLE;
    }
}

// Whether the Pokémon can join the battle's order: 0 if it can, 1 if the regulation bars it, 2 for a species and 3
// for an item already in the order, and 4 for a full order
u32 PokeListPlate_CheckEntry(PokeListWork *wk, PokeListPlate *plate) {
    u32 species;
    u32 item;
    u8 i;
    Regulation *regulation;

    if (plate->entry != POKELIST_ENTRY_ABLE) {
        return 1;
    }
    species = PokeParty_GetParam(plate->pkm, PKM_PARAM_SPECIES, NULL);
    item = PokeParty_GetParam(plate->pkm, PKM_PARAM_ITEM, NULL);
    regulation = wk->param->regulation;
    for (i = 0; i < 6; i++) {
        u8 picked = wk->param->picked[i];

        if (picked >= 1) {
            PartyPkm *other = wk->plates[picked - 1]->pkm;
            u32 otherSpecies = PokeParty_GetParam(other, PKM_PARAM_SPECIES, NULL);
            u32 otherItem = PokeParty_GetParam(other, PKM_PARAM_ITEM, NULL);

            if (species == otherSpecies && regulation->unk8 == 0) {
                return 2;
            }
            if (item == otherItem && item != 0 && regulation->unk9 == 0) {
                return 3;
            }
        }
    }
    if (wk->enteredCount == regulation->unk3) {
        return 4;
    }
    return 0;
}

static u8 PokeListPlate_GetHpColor(PokeListPlate *plate) {
    return HPGauge_GetColor(plate->hp, PokeParty_GetParam(plate->pkm, PKM_PARAM_MAX_HP, NULL));
}

// Where the icon goes for a point of the plate
static void PokeListPlate_GetIconPos(PokeListPlate *plate, int x, int y, ClActorPos *pos) {
    ClActorPos surfacePos;

    func_0204befc(plate->renderer, 0, &surfacePos);
    pos->x = x - surfacePos.x;
    pos->y = y - surfacePos.y;
}
