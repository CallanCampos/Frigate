#include "types.h"
#include "save/box.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "text/system/btl_pokeparam_box_main_save_init.h"

// The PC boxes: the main block with the open box, the names, the wallpapers and how many boxes are open, and one
// block per box of 30 Pokémon. The name is the ROM's string, and trayNum, pos, BOX_MAX_TRAY, BOX_MAX_POS and
// BOXDAT_TRAYNUM_CURRENT are the names its assertions keep. Blocks 0 (main) and 1 to 24 (the boxes).

#define SAVE_BLOCK_BOX_MAIN 0
#define SAVE_BLOCK_BOX_TRAY 1

// Boxes open at the start, and how many more each unlock opens
#define BOX_START_TRAY 8
#define BOX_UNLOCK_TRAY 8
#define BOX_WALLPAPER_NORMAL_COUNT 16

struct BoxMainSave {
    u32 currentTray;
    u16 names[BOX_MAX_TRAY][BOX_NAME_LENGTH];
    u8 wallpapers[BOX_MAX_TRAY];
    u8 flags;
    u8 trayCount;
    u8 padding3DE[2];
};

typedef struct {
    BoxPkm pkm[BOX_MAX_POS];
} BoxTraySave;

struct BoxSaveAccessor {
    SaveControl *save;
};

static void BoxMainSave_Init(BoxMainSave *main);
static BoxMainSave *BoxSaveAccessor_GetMainSave(BoxSaveAccessor *boxes);
static u32 howManyNormalPokesAreInBox(BoxSaveAccessor *boxes, u32 trayNum);

void BoxMainSave_InitBlock(BoxMainSave *main) {
    BoxMainSave_Init(main);
}

u32 BoxMainSave_GetBlockSize(void) {
    return sizeof(BoxMainSave);
}

static void BoxMainSave_Init(BoxMainSave *main) {
    u32 i;
    u32 wallpaper;
    MsgData *msgData;

    for (i = 0, wallpaper = 0; i < BOX_MAX_TRAY; i++) {
        main->wallpapers[i] = wallpaper;
        wallpaper++;
        if (wallpaper >= BOX_WALLPAPER_NORMAL_COUNT) {
            wallpaper = 0;
        }
    }
    main->flags = 0;
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_POKEPARAM_BOX_MAIN_SAVE_INIT, HEAPID_USER);
    if (msgData != NULL) {
        for (i = 0; i < BOX_MAX_TRAY; i++) {
            GFL_MsgDataLoadRawStr(msgData, BtlPokeparamBoxMainSaveInit_Text_Box1 + i, main->names[i], BOX_NAME_LENGTH);
        }
        GFL_MsgDataFree(msgData);
    }
    main->currentTray = 0;
    main->trayCount = BOX_START_TRAY;
}

static BoxMainSave *BoxSaveAccessor_GetMainSave(BoxSaveAccessor *boxes) {
    return SaveControl_GetBlockPtr(boxes->save, SAVE_BLOCK_BOX_MAIN);
}

BOOL BoxSaveAccessor_InsertPkm(BoxSaveAccessor *boxes, BoxPkm *pkm) {
    u32 trayNum;
    u32 trayCount;
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    trayNum = main->currentTray;
    do {
        if (BoxSaveAccessor_InsertPkmCore(boxes, trayNum, pkm)) {
            return TRUE;
        }
        trayNum++;
        trayCount = main->trayCount;
        if (trayNum >= trayCount) {
            trayNum = 0;
        }
    } while (trayNum != main->currentTray);
    // Every open box is full: open more and put it in the first of them
    if (trayCount != BOX_MAX_TRAY) {
        main->trayCount += BOX_UNLOCK_TRAY;
        BoxSaveAccessor_InsertPkmCore(boxes, trayCount, pkm);
        return TRUE;
    }
    return FALSE;
}

BOOL BoxSaveAccessor_InsertPkmCore(BoxSaveAccessor *boxes, u32 trayNum, BoxPkm *pkm) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    u32 pos;
    BoxTraySave *tray;

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    } else if (trayNum >= main->trayCount) {
        GFL_ASSERT(0);
    }
    for (pos = 0; pos < BOX_MAX_POS; pos++) {
        tray = BoxSaveAccessor_GetBox(boxes, trayNum);
        if (PML_PkmGetParam(&tray->pkm[pos], PKM_PARAM_SPECIES, NULL) == 0) {
            tray->pkm[pos] = *pkm;
            PML_PkmPPRecover(&tray->pkm[pos]);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL BoxSaveAccessor_SetPkm(BoxSaveAccessor *boxes, u32 trayNum, u32 pos, BoxPkm *pkm) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    BoxTraySave *tray;

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (trayNum < main->trayCount && pos < BOX_MAX_POS) {
        tray = BoxSaveAccessor_GetBox(boxes, trayNum);
        tray->pkm[pos] = *pkm;
        PML_PkmPPRecover(&tray->pkm[pos]);
        return TRUE;
    }
    GFL_ASSERT(0);
    return FALSE;
}

void BoxSaveAccessor_SwapPkms(BoxSaveAccessor *boxes, u32 trayNum1, u32 pos1, u32 trayNum2, u32 pos2) {
    BoxTraySave *tray1 = BoxSaveAccessor_GetBox(boxes, trayNum1);
    BoxTraySave *tray2 = BoxSaveAccessor_GetBox(boxes, trayNum2);
    BoxPkm pkm = tray1->pkm[pos1];

    tray1->pkm[pos1] = tray2->pkm[pos2];
    tray2->pkm[pos2] = pkm;
}

void BoxSaveAccessor_ClearPkm(BoxSaveAccessor *boxes, u32 trayNum, u32 pos) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    BoxTraySave *tray;

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (pos < BOX_MAX_POS && trayNum < BOX_MAX_TRAY) {
        tray = BoxSaveAccessor_GetBox(boxes, trayNum);
        PML_PkmInit(&tray->pkm[pos]);
    }
}

u32 BoxSaveAccessor_GetLastOpenedBox(BoxSaveAccessor *boxes) {
    return BoxSaveAccessor_GetMainSave(boxes)->currentTray;
}

u8 BoxSaveAccessor_GetAvailableBoxCount(BoxSaveAccessor *boxes) {
    return BoxSaveAccessor_GetMainSave(boxes)->trayCount;
}

u8 BoxSaveAccessor_UnlockMoreBoxes(BoxSaveAccessor *boxes) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (main->trayCount != BOX_MAX_TRAY) {
        main->trayCount += BOX_UNLOCK_TRAY;
    }
    return main->trayCount;
}

BOOL BoxSaveAccessor_GetNextFreeBoxSlot(BoxSaveAccessor *boxes, int *trayNum, int *pos) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    int tray;
    int i;
    int trayCount;
    BoxTraySave *trayData;

    if (*trayNum == BOXDAT_TRAYNUM_CURRENT) {
        *trayNum = main->currentTray;
    }
    tray = *trayNum;
    i = *pos;
    while (TRUE) {
        trayData = BoxSaveAccessor_GetBox(boxes, tray);
        for (; i < BOX_MAX_POS; i++) {
            if (PML_PkmGetParam(&trayData->pkm[i], PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                *trayNum = tray;
                *pos = i;
                return TRUE;
            }
        }
        tray++;
        trayCount = main->trayCount;
        if (tray >= trayCount) {
            tray = 0;
        }
        if (tray == *trayNum) {
            break;
        }
        i = 0;
    }
    // Every open box is full: open more
    if (trayCount != BOX_MAX_TRAY) {
        *trayNum = trayCount;
        *pos = 0;
        main->trayCount += BOX_UNLOCK_TRAY;
        return TRUE;
    }
    return FALSE;
}

u32 BoxSaveAccessor_CountEmptySlots(BoxSaveAccessor *boxes) {
    int trayNum;
    int pos;
    u32 count = 0;
    BoxTraySave *tray;

    for (trayNum = 0; trayNum < BOX_MAX_TRAY; trayNum++) {
        tray = BoxSaveAccessor_GetBox(boxes, trayNum);
        for (pos = 0; pos < BOX_MAX_POS; pos++) {
            if (PML_PkmGetParam(&tray->pkm[pos], PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                count++;
            }
        }
    }
    return count;
}

u32 countEmptySlotsInBox(BoxSaveAccessor *boxes, u32 trayNum) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    int pos;
    u32 count;
    BoxTraySave *tray;

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    GFL_ASSERT(trayNum < BOX_MAX_TRAY);
    count = 0;
    tray = BoxSaveAccessor_GetBox(boxes, trayNum);
    for (pos = 0; pos < BOX_MAX_POS; pos++) {
        if (PML_PkmGetParam(&tray->pkm[pos], PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            count++;
        }
    }
    return count;
}

void saveLastOpenedBoxIdx(BoxSaveAccessor *boxes, u32 trayNum) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (trayNum < BOX_MAX_TRAY) {
        main->currentTray = trayNum;
    }
}

u32 getBoxNumFromIdx(BoxSaveAccessor *boxes, u32 trayNum) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (trayNum < BOX_MAX_TRAY) {
        return main->wallpapers[trayNum];
    }
    return 0;
}

void BoxSaveAccessor_SetWallpaper(BoxSaveAccessor *boxes, u32 trayNum, u32 wallpaper) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (trayNum < BOX_MAX_TRAY) {
        main->wallpapers[trayNum] = wallpaper;
    }
}

void loadBoxNameToStrbuf(BoxSaveAccessor *boxes, u32 trayNum, StrBuf *str) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (trayNum < BOX_MAX_TRAY) {
        GFL_StrBufLoadString(str, main->names[trayNum]);
    } else {
        GFL_ASSERT(0);
    }
}

void getBoxNameFromStrbuf(BoxSaveAccessor *boxes, u32 trayNum, StrBuf *name) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (trayNum < BOX_MAX_TRAY) {
        GFL_StrBufStoreString(name, main->names[trayNum], BOX_NAME_LENGTH);
    }
}

u32 howManyPokesInGeneralAreInBox(BoxSaveAccessor *boxes, u32 trayNum) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    int pos;
    u32 count;
    BoxTraySave *tray;

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (trayNum < BOX_MAX_TRAY) {
        count = 0;
        tray = BoxSaveAccessor_GetBox(boxes, trayNum);
        for (pos = 0; pos < BOX_MAX_POS; pos++) {
            if (PML_PkmGetParam(&tray->pkm[pos], PKM_PARAM_SPECIES_VALID, NULL)) {
                count++;
            }
        }
        return count;
    }
    GFL_ASSERT(0);
    return 0;
}

// How many Pokémon of a box are not eggs
static u32 howManyNormalPokesAreInBox(BoxSaveAccessor *boxes, u32 trayNum) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    int pos;
    u32 count;
    BoxTraySave *tray;

    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    if (trayNum < BOX_MAX_TRAY) {
        count = 0;
        tray = BoxSaveAccessor_GetBox(boxes, trayNum);
        for (pos = 0; pos < BOX_MAX_POS; pos++) {
            if (PML_PkmGetParam(&tray->pkm[pos], PKM_PARAM_SPECIES_VALID, NULL)
                && PML_PkmGetParam(&tray->pkm[pos], PKM_PARAM_IS_EGG, NULL) == 0) {
                count++;
            }
        }
        return count;
    }
    GFL_ASSERT(0);
    return 0;
}

u32 howManyTotalPokesAreInBoxes(BoxSaveAccessor *boxes) {
    u32 count;
    u32 trayNum;

    for (trayNum = 0, count = 0; trayNum < BOX_MAX_TRAY; trayNum++) {
        count += howManyPokesInGeneralAreInBox(boxes, trayNum);
    }
    return count;
}

u32 howManyNormalPokesAreInAllBoxes(BoxSaveAccessor *boxes) {
    u32 count;
    u32 trayNum;

    for (trayNum = 0, count = 0; trayNum < BOX_MAX_TRAY; trayNum++) {
        count += howManyNormalPokesAreInBox(boxes, trayNum);
    }
    return count;
}

u32 BoxSaveAccessor_GetPkmParam(BoxSaveAccessor *boxes, u32 trayNum, u32 pos, u32 param, void *buffer) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    BoxTraySave *tray;

    GFL_ASSERT((trayNum<BOX_MAX_TRAY)||(trayNum == BOXDAT_TRAYNUM_CURRENT));
    GFL_ASSERT(pos<BOX_MAX_POS);
    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    tray = BoxSaveAccessor_GetBox(boxes, trayNum);
    return PML_PkmGetParam(&tray->pkm[pos], param, buffer);
}

BoxPkm *BoxSaveAccessor_GetPkm(BoxSaveAccessor *boxes, u32 trayNum, u32 pos) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);
    BoxTraySave *tray;

    GFL_ASSERT(((trayNum<BOX_MAX_TRAY)||(trayNum == BOXDAT_TRAYNUM_CURRENT)));
    GFL_ASSERT((pos<BOX_MAX_POS));
    if (trayNum == BOXDAT_TRAYNUM_CURRENT) {
        trayNum = main->currentTray;
    }
    tray = BoxSaveAccessor_GetBox(boxes, trayNum);
    return &tray->pkm[pos];
}

BoxPkm *copyBoxedPkmToBuf(BoxSaveAccessor *boxes, u32 trayNum, u32 pos, HeapID heapId) {
    BoxPkm *pkm = GFL_HeapAllocate(heapId, sizeof(BoxPkm), FALSE, "box_savedata.c", 891);

    sys_memcpy(BoxSaveAccessor_GetPkm(boxes, trayNum, pos), pkm, sizeof(BoxPkm));
    return pkm;
}

void BoxSaveAccessor_FreePkmCopy(BoxPkm *pkm) {
    GFL_HeapFree(pkm);
}

void BoxSaveAccessor_SetFlags(BoxSaveAccessor *boxes, u32 flags) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    main->flags |= flags;
}

BOOL BoxSaveAccessor_CheckFlags(BoxSaveAccessor *boxes, u32 flags) {
    BoxMainSave *main = BoxSaveAccessor_GetMainSave(boxes);

    if (main->flags & flags) {
        return TRUE;
    }
    return FALSE;
}

u32 getSizeofPokeBox(void) {
    return sizeof(BoxPkm) * BOX_MAX_POS;
}

BoxSaveAccessor *BoxSaveAccessor_Create(HeapID heapId, SaveControl *save) {
    BoxSaveAccessor *boxes = GFL_HeapAllocate(heapId, sizeof(BoxSaveAccessor), FALSE, "box_savedata.c", 972);

    boxes->save = save;
    return boxes;
}

void BoxSaveAccessor_Free(BoxSaveAccessor *boxes) {
    GFL_HeapFree(boxes);
}

void initFullBox(void *tray) {
    u32 pos;

    for (pos = 0; pos < BOX_MAX_POS; pos++) {
        PML_PkmInit(&((BoxTraySave *)tray)->pkm[pos]);
    }
}

u32 getSizeofBox(void) {
    return sizeof(BoxTraySave);
}

void *BoxSaveAccessor_GetBox(BoxSaveAccessor *boxes, u32 trayNum) {
    return SaveControl_GetBlockPtr(boxes->save, SAVE_BLOCK_BOX_TRAY + trayNum);
}
