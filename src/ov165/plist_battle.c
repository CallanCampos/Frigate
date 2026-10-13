#include "app/pokelist.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The party list in a battle's team selection: the partners' teams of a multi battle shown beside the player's, the
// battle's messages, and the time left to pick

// A Pokémon of a partner's team: its icon, item, sex and level
typedef struct {
    BOOL dirty;
    BmpWin *sexWindow;
    BmpWin *levelWindow;
    u32 iconChars;
    ClActor *icon;
    ClActor *itemIcon;
} PokeListPartnerPkm;

struct PokeListPartner {
    BOOL dirty;
    BmpWin *window;
    PokeListPartnerPkm *pkms[6];
};

static void PokeListBattle_CreateUnit(PokeListWork *wk);
static void PokeListBattle_FreeUnit(PokeListWork *wk);
static void PokeListBattle_LoadResources(PokeListWork *wk);
static void PokeListBattle_FreeResources(PokeListWork *wk);
static PokeListPartner *PokeListBattle_CreatePartner(PokeListWork *wk, PokeListPartnerParam *partnerParam, u8 x, u8 y,
                                                     u8 spacing);
static void PokeListBattle_FreePartner(PokeListWork *wk, PokeListPartner *partner);
static void PokeListBattle_UpdatePartner(PokeListWork *wk, PokeListPartner *partner);
static PokeListPartnerPkm *PokeListBattle_CreatePartnerPkm(PokeListWork *wk, PartyPkm *pkm, u8 x, u8 y);
static void PokeListBattle_FreePartnerPkm(PokeListWork *wk, PokeListPartnerPkm *partnerPkm);
static void PokeListBattle_UpdatePartnerPkm(PokeListWork *wk, PokeListPartnerPkm *partnerPkm);
static void PokeListBattle_PrintMessage(PokeListWork *wk);
static void PokeListBattle_CreateTimer(PokeListWork *wk);
static void PokeListBattle_FreeTimer(PokeListWork *wk);
static void PokeListBattle_UpdateTimer(PokeListWork *wk);

void PokeListBattle_Init(PokeListWork *wk) {
    PokeListBattle_CreateUnit(wk);
    PokeListBattle_LoadResources(wk);
    if (wk->param->showPartners == TRUE) {
        switch (wk->param->partnerCount) {
        case 1:
            wk->partners[1] = PokeListBattle_CreatePartner(wk, &wk->param->partners[1], 10, 0, 2);
            break;
        case 2:
            wk->partners[0] = PokeListBattle_CreatePartner(wk, &wk->param->partners[0], 0, 0, 0);
            wk->partners[1] = PokeListBattle_CreatePartner(wk, &wk->param->partners[1], 11, 0, 0);
            wk->partners[2] = PokeListBattle_CreatePartner(wk, &wk->param->partners[2], 22, 0, 0);
            break;
        }
    }
    wk->shownBattleMsg = wk->param->battleMsg;
    wk->shownTime = 0;
    if (wk->param->timerEnabled == TRUE) {
        PokeListBattle_CreateTimer(wk);
    }
    wk->timerBlink = 0;
}

void PokeListBattle_Exit(PokeListWork *wk) {
    if (wk->param->timerEnabled == TRUE) {
        PokeListBattle_FreeTimer(wk);
    }
    if (wk->param->showPartners == TRUE) {
        switch (wk->param->partnerCount) {
        case 1:
            PokeListBattle_FreePartner(wk, wk->partners[1]);
            break;
        case 2:
            PokeListBattle_FreePartner(wk, wk->partners[0]);
            PokeListBattle_FreePartner(wk, wk->partners[1]);
            PokeListBattle_FreePartner(wk, wk->partners[2]);
            break;
        }
    }
    PokeListBattle_FreeResources(wk);
    PokeListBattle_FreeUnit(wk);
}

void PokeListBattle_Update(PokeListWork *wk) {
    if (wk->param->showPartners == TRUE) {
        switch (wk->param->partnerCount) {
        case 1:
            PokeListBattle_UpdatePartner(wk, wk->partners[1]);
            break;
        case 2:
            PokeListBattle_UpdatePartner(wk, wk->partners[0]);
            PokeListBattle_UpdatePartner(wk, wk->partners[1]);
            PokeListBattle_UpdatePartner(wk, wk->partners[2]);
            break;
        }
    }
    if (wk->shownBattleMsg != wk->param->battleMsg && wk->state == 2) {
        PokeListBattle_PrintMessage(wk);
        wk->shownBattleMsg = wk->param->battleMsg;
    }
    if (wk->param->timerEnabled == TRUE) {
        PokeListBattle_UpdateTimer(wk);
    }
    if (wk->param->unk73 == 1) {
        PokeList_TimeUp(wk);
    }
}

static void PokeListBattle_CreateUnit(PokeListWork *wk) {
    wk->partnerUnit = func_0204bf1c(36, 8, wk->heapId);
}

static void PokeListBattle_FreeUnit(PokeListWork *wk) {
    func_0204bf98(wk->partnerUnit);
}

static void PokeListBattle_LoadResources(PokeListWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0x4b, wk->heapId);
    ArcTool *iconArc;
    ArcTool *commonArc;

    GFL_G2DIOLoadArcNCLR(arc, 1, 4, 0x20, 0x20, 0x20, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0x1a0, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 9, 6, 0, 0, FALSE, wk->heapId);
    if (wk->param->showPartners == TRUE) {
        switch (wk->param->partnerCount) {
        case 1:
            loadBGScrToVramByFileNoReserveNegAlign(arc, 0x11, 6, 0, 0, FALSE, wk->heapId);
            break;
        case 2:
            loadBGScrToVramByFileNoReserveNegAlign(arc, 0x10, 6, 0, 0, FALSE, wk->heapId);
            break;
        }
    }
    GFL_ArcToolFree(arc);

    commonArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
    // BUG: The common archive's graphics are loaded from the closed handle of the first archive, as in
    // PokeList_LoadResources
#ifdef BUGFIX
    wk->clResources[CL_RES_PLTT(3)] = func_0204bba0(commonArc, func_0202d890(), 1, 0x60, wk->heapId);
    wk->clResources[CL_RES_CHAR(4)] = func_0204b81c(commonArc, func_0202d894(), FALSE, 1, wk->heapId);
    wk->clResources[CL_RES_CELL(4)] = func_0204bde0(commonArc, func_0202d898(2), func_0202d89c(2), wk->heapId);
#else
    wk->clResources[CL_RES_PLTT(3)] = func_0204bba0(arc, func_0202d890(), 1, 0x60, wk->heapId);
    wk->clResources[CL_RES_CHAR(4)] = func_0204b81c(arc, func_0202d894(), FALSE, 1, wk->heapId);
    wk->clResources[CL_RES_CELL(4)] = func_0204bde0(arc, func_0202d898(2), func_0202d89c(2), wk->heapId);
#endif
    if (wk->param->timerEnabled == TRUE) {
#ifdef BUGFIX
        GFL_G2DIOLoadArcNCLRDefault(commonArc, func_0202d820(), 4, 0x40, 0x20, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(commonArc, func_0202d824(), 5, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(commonArc, func_0202d828(), 5, 0, 0, FALSE, wk->heapId);
#else
        GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), 4, 0x40, 0x20, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, func_0202d824(), 5, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, func_0202d828(), 5, 0, 0, FALSE, wk->heapId);
#endif
        GFL_BGSysSetScrPaletteNo(5, 0, 0, 32, 32, 2);
        GFL_BGSysQueueScrLoad(5);
    }
    GFL_ArcToolFree(commonArc);

    iconArc = GFL_ArcSysCreateFileHandle(7, wk->heapId);
    wk->clResources[CL_RES_PLTT(7)] = func_0204bc48(iconArc, PokeIcon_GetPaletteFile(), 1, 0xc0, wk->heapId);
    GFL_ArcToolFree(iconArc);
    sys_memcpy16((void *)(HW_DB_BG_PLTT + 0x40), wk->timerBaseColors, sizeof(wk->timerBaseColors));
}

static void PokeListBattle_FreeResources(PokeListWork *wk) {
}

static inline PokeListPartnerPkm *PokeListBattle_CreatePartnerPkmAt(PokeListWork *wk, PartyPkm *pkm, u8 col, u8 row,
                                                                    u8 offset) {
    return PokeListBattle_CreatePartnerPkm(wk, pkm, offset + col * 8, row * 8);
}

static PokeListPartner *PokeListBattle_CreatePartner(PokeListWork *wk, PokeListPartnerParam *partnerParam, u8 x, u8 y,
                                                     u8 spacing) {
    PokeListPartner *partner = GFL_HeapAllocate(wk->heapId, sizeof(PokeListPartner), FALSE, "plist_battle.c", 323);
    u8 i;
    u8 count = PokeParty_GetPkmCount(partnerParam->party);
    StrBuf *name;

    for (i = 0; i < 6; i++) {
        if (i < count) {
            u8 col = x + (i % 2) * (spacing + 5);
            u8 row = y + (i / 2) * 6 + 4;
            int offset = 4;

            if (i % 2 != 0) {
                offset = 0;
            }
            partner->pkms[i] = PokeListBattle_CreatePartnerPkmAt(wk, PokeParty_GetPkm(partnerParam->party, i), col, row, offset);
        } else {
            partner->pkms[i] = NULL;
        }
    }
    partner->window = BmpWin_CreateDynamic(4, x + 1 + spacing / 2, y + 1, 10, 2, 14, TRUE);
    name = GFL_StrBufCreate(16, wk->heapId);
    GFL_StrBufLoadString(name, partnerParam->name);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(partner->window), 0, 0, name, wk->font, 0x3c40);
    GFL_StrBufFree(name);
    partner->dirty = TRUE;
    return partner;
}

static void PokeListBattle_FreePartner(PokeListWork *wk, PokeListPartner *partner) {
    u8 i;

    for (i = 0; i < 6; i++) {
        if (partner->pkms[i] != NULL) {
            PokeListBattle_FreePartnerPkm(wk, partner->pkms[i]);
        }
    }
    BmpWin_Free(partner->window);
    GFL_HeapFree(partner);
}

static void PokeListBattle_UpdatePartner(PokeListWork *wk, PokeListPartner *partner) {
    u8 i;

    for (i = 0; i < 6; i++) {
        if (partner->pkms[i] != NULL) {
            PokeListBattle_UpdatePartnerPkm(wk, partner->pkms[i]);
        }
    }
    if (partner->dirty == TRUE && func_02021c1c(wk->printQueue, BmpWin_GetBitmap(partner->window)) == FALSE) {
        partner->dirty = FALSE;
        BmpWin_Transfer(partner->window);
    }
}

static inline void PokeListBattle_CreateSexWindow(PokeListWork *wk, PokeListPartnerPkm *partnerPkm, BOOL isEgg, u32 sex,
                                                  u8 x, u8 y) {
    u8 subX = x % 8;
    u8 tileY = y / 8;
    u8 tileX = x / 8;

    partnerPkm->sexWindow = BmpWin_CreateDynamic(4, tileX + 3, tileY, 3, 2, 13, TRUE);
    if (isEgg == FALSE) {
        if (sex == GENDER_MALE) {
            PokeList_PrintString(wk, partnerPkm->sexWindow, 7, subX + 4, 0, 0x14c0);
        } else if (sex == GENDER_FEMALE) {
            PokeList_PrintString(wk, partnerPkm->sexWindow, 8, subX + 4, 0, 0xc80);
        }
    }
}

static inline void PokeListBattle_CreateLevelWindow(PokeListWork *wk, PokeListPartnerPkm *partnerPkm, BOOL isEgg,
                                                    u32 level, u8 x, u8 y) {
    u8 subX = x % 8;
    u8 tileY = y / 8;
    u8 tileX = x / 8;

    partnerPkm->levelWindow = BmpWin_CreateDynamic(4, tileX, tileY + 3, 6, 1, 14, TRUE);
    if (isEgg == FALSE) {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);

        WordSetNumber(wordSet, 0, level, 3, 0, TRUE);
        PokeList_PrintWordSetStringSmall(wk, partnerPkm->levelWindow, wordSet, 2, subX, 0, 0x3c40);
        GFL_WordSetSystemFree(wordSet);
    }
}

static PokeListPartnerPkm *PokeListBattle_CreatePartnerPkm(PokeListWork *wk, PartyPkm *pkm, u8 x, u8 y) {
    PokeListPartnerPkm *partnerPkm =
        GFL_HeapAllocate(wk->heapId, sizeof(PokeListPartnerPkm), FALSE, "plist_battle.c", 409);
    BOOL isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(7, wk->heapId);
    BoxPkm *boxPkm = func_0201d624(pkm);
    u32 palette;
    u32 item;
    u32 sex;
    u32 level;
    ClActorSetup iconSetup;
    ClActorSetup itemSetup;

    partnerPkm->iconChars = func_0204b81c(arc, PokeIcon_GetFileOfPkm(boxPkm), FALSE, 1, wk->heapId);
    GFL_ArcToolFree(arc);
    palette = PokeIcon_GetPaletteOfPkm(boxPkm);
    iconSetup.x = x + 16;
    iconSetup.y = y + 8;
    iconSetup.sequence = 1;
    iconSetup.priority = 16;
    iconSetup.bgPriority = 2;
    partnerPkm->icon = func_0204c040(wk->partnerUnit, partnerPkm->iconChars, wk->clResources[CL_RES_PLTT(7)],
                                     wk->clResources[CL_RES_CELL(7)], &iconSetup, 1, wk->heapId);
    func_0204c378(partnerPkm->icon, palette, 1);
    func_0204c520(partnerPkm->icon, TRUE);
    item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    itemSetup.x = x + 32;
    itemSetup.y = y + 16;
    itemSetup.sequence = 0;
    itemSetup.priority = 8;
    itemSetup.bgPriority = 2;
    partnerPkm->itemIcon =
        func_0204c040(wk->partnerUnit, wk->clResources[CL_RES_CHAR(4)], wk->clResources[CL_RES_PLTT(3)],
                      wk->clResources[CL_RES_CELL(4)], &itemSetup, 1, wk->heapId);
    if (item != 0) {
        func_0204c124(partnerPkm->itemIcon, TRUE);
        if (PML_ItemIsMail(item) == TRUE) {
            func_0204c488(partnerPkm->itemIcon, 1);
        } else {
            func_0204c488(partnerPkm->itemIcon, 0);
        }
    } else {
        func_0204c124(partnerPkm->itemIcon, FALSE);
    }
    sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    PokeListBattle_CreateSexWindow(wk, partnerPkm, isEgg, sex, x, y);
    level = PokeParty_GetLevel(pkm);
    PokeListBattle_CreateLevelWindow(wk, partnerPkm, isEgg, level, x, y);
    partnerPkm->dirty = TRUE;
    return partnerPkm;
}

static void PokeListBattle_FreePartnerPkm(PokeListWork *wk, PokeListPartnerPkm *partnerPkm) {
    BmpWin_Free(partnerPkm->sexWindow);
    BmpWin_Free(partnerPkm->levelWindow);
    func_0204c108(partnerPkm->itemIcon);
    func_0204c108(partnerPkm->icon);
    func_0204b98c(partnerPkm->iconChars);
    GFL_HeapFree(partnerPkm);
}

static void PokeListBattle_UpdatePartnerPkm(PokeListWork *wk, PokeListPartnerPkm *partnerPkm) {
    if (partnerPkm->dirty == TRUE && func_02021c1c(wk->printQueue, BmpWin_GetBitmap(partnerPkm->sexWindow)) == FALSE &&
        func_02021c1c(wk->printQueue, BmpWin_GetBitmap(partnerPkm->levelWindow)) == FALSE) {
        partnerPkm->dirty = FALSE;
        BmpWin_Transfer(partnerPkm->sexWindow);
        BmpWin_Transfer(partnerPkm->levelWindow);
    }
}

// Shows the battle's message for the selection, with the partner's name in a multi battle
void PokeListBattle_ShowMessage(PokeListWork *wk) {
    PokeListBattle_PrintMessage(wk);
    wk->shownBattleMsg = wk->param->battleMsg;
}

static void PokeListBattle_PrintMessage(PokeListWork *wk) {
    if (wk->param->battleMsg != 0) {
        if (PokeListMessage_IsOpen(wk, wk->message) == FALSE) {
            PokeListMessage_Open(wk, wk->message, POKELIST_MESSAGE_WINDOW_SHORTER);
        }
        if (wk->param->partnerCount == 1) {
            StrBuf *name = GFL_StrBufCreate(16, wk->heapId);

            GFL_StrBufLoadString(name, wk->param->partners[1].name);
            PokeListMessage_CreateWordSet(wk, wk->message);
            PokeListMessage_SetString(wk, wk->message, 0, name, wk->param->partners[1].gender);
            PokeListMessage_Print(wk, wk->message, 0xb5);
            PokeListMessage_FreeWordSet(wk, wk->message);
            GFL_StrBufFree(name);
        } else {
            PokeListMessage_Print(wk, wk->message, wk->param->battleMsg + 0xb5);
        }
    }
}

static void PokeListBattle_CreateTimer(PokeListWork *wk) {
    StrBuf *label;

    wk->timerWindows[0] = BmpWin_CreateDynamic(4, 7, 21, 11, 3, 14, TRUE);
    wk->timerWindows[1] = BmpWin_CreateDynamic(4, 20, 21, 5, 3, 14, TRUE);
    label = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xb9);
    func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->timerWindows[0]), 0, 4, label, wk->font, 0x3c40);
    GFL_StrBufFree(label);
    wk->timerWindowDirty[0] = TRUE;
}

static void PokeListBattle_FreeTimer(PokeListWork *wk) {
    BmpWin_Free(wk->timerWindows[0]);
    BmpWin_Free(wk->timerWindows[1]);
}

// Prints the time left once it changes, and makes the timer blink red for the last ten seconds
static void PokeListBattle_UpdateTimer(PokeListWork *wk) {
    if (wk->shownTime != wk->param->timeLeft &&
        func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->timerWindows[1])) == FALSE) {
        u16 time = wk->param->timeLeft;
        u8 minutes = time / 60;
        u8 seconds = time % 60;
        StrBuf *str = GFL_StrBufCreate(32, wk->heapId);
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        StrBuf *format;
        s32 width;

        if (minutes == 0) {
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xbb);
            WordSetNumber(wordSet, 1, seconds, 2, 1, TRUE);
        } else {
            format = GFL_MsgDataLoadStrbufNew(wk->msgData, 0xba);
            WordSetNumber(wordSet, 0, minutes, 2, 1, TRUE);
            WordSetNumber(wordSet, 1, seconds, 2, 2, TRUE);
        }
        GFL_BitmapFill(BmpWin_GetBitmap(wk->timerWindows[1]), 0);
        GFL_WordSetFormatStrbuf(wordSet, str, format);
        width = GFL_FontGetBlockWidth(str, wk->font, 0);
        func_02021c7c(wk->printQueue, BmpWin_GetBitmap(wk->timerWindows[1]), 36 - width, 4, str, wk->font, 0x3c40);
        GFL_WordSetSystemFree(wordSet);
        GFL_StrBufFree(str);
        GFL_StrBufFree(format);
        wk->timerWindowDirty[1] = TRUE;
        wk->shownTime = wk->param->timeLeft;
    }
    if (wk->param->timeLeft == 0) {
        PokeList_TimeUp(wk);
    }
    if (wk->timerWindowDirty[1] == TRUE &&
        func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->timerWindows[1])) == FALSE) {
        wk->timerWindowDirty[1] = FALSE;
        BmpWin_Transfer(wk->timerWindows[1]);
    }
    if (wk->timerWindowDirty[0] == TRUE &&
        func_02021c1c(wk->printQueue, BmpWin_GetBitmap(wk->timerWindows[0])) == FALSE) {
        wk->timerWindowDirty[0] = FALSE;
        BmpWin_Transfer(wk->timerWindows[0]);
    }
    if (wk->param->timeLeft <= 10) {
        u8 i;
        u8 level = ((FX_CosIdx(wk->timerBlink * 0x10000 / 60) + FX32_ONE) * 32) / (FX32_ONE * 2);

        for (i = 0; i < 16; i++) {
            u16 color = wk->timerBaseColors[i];
            u8 g = (color & 0x3e0) >> 5;
            u8 b = (color & 0x7c00) >> 10;
            u8 r = color & 0x1f;

            r = r + level > 31 ? 31 : r + level;
            wk->timerColors[i] = GX_RGB(r, g, b);
        }
        NNS_GfdRegisterNewVramTransferTask(31, 0x40, wk->timerColors, sizeof(wk->timerColors));
        wk->timerBlink++;
        if (wk->timerBlink >= 60) {
            wk->timerBlink = 0;
        }
    }
}
