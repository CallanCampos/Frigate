#include "types.h"
#include "constants/pokemon.h"
#include "field/field.h"
#include "field/union_msg.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/bmp_menulist.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/wordset.h"

#define PARTY_SELECT_LIST_MAX 6

typedef struct {
    Field *field;
    PokeParty *temoti;
    PokeParty *bbox;
    HeapID heapId;
    int partyOk;
    int bboxOk;
    ClActUnit *unit;
    ClActor *partyIcons[PARTY_SELECT_LIST_MAX];
    ClActor *bboxIcons[PARTY_SELECT_LIST_MAX];
    ClActor *partyItems[PARTY_SELECT_LIST_MAX];
    ClActor *bboxItems[PARTY_SELECT_LIST_MAX];
    u16 iconPalette;
    u16 iconCells;
    u16 partyChars[PARTY_SELECT_LIST_MAX];
    u16 bboxChars[PARTY_SELECT_LIST_MAX];
    u16 itemPalette;
    u16 itemCells;
    u16 itemChars;
    WordSet *wordSet;
    void *msgBGSys;
    MsgData *msgData;
    ListMenuUI *list;
    void *infoWin;
    void *moneyWin;
    Font *font;
    int cursor;
    u8 state;
    u8 finish;
} PartySelectList;

static void func_ov036_021c3924(PartySelectList *psl);
static void func_ov036_021c3344(PartySelectList *psl);
static void func_ov036_021c3468(PartySelectList *psl);
static void func_ov036_021c36b8(PartySelectList *psl);
static void func_ov036_021c3750(PartySelectList *psl);
static int func_ov036_021c3318(PartySelectList *psl, u32 *out);
static void func_ov036_021c3864(PartySelectList *psl);
static void func_ov036_021c3730(PartySelectList *psl);
static void func_ov036_021c3600(PartySelectList *psl);
static void func_ov036_021c340c(PartySelectList *psl);
static void func_ov036_021c3950(PartySelectList *psl);
static void func_ov036_021c3a2c(PartySelectList *psl);
static void func_ov036_021c3a50(PartySelectList *psl, u32 messageId);
static BOOL func_ov036_021c3a64(PartySelectList *psl);
static void func_ov036_021c3968(PartySelectList *psl);
static s32 func_ov036_021c39c0(PartySelectList *psl);
static void func_ov036_021c39b4(PartySelectList *psl);
static void func_ov036_021c3c18(PartySelectList *psl);
static void func_ov036_021c3a44(PartySelectList *psl);
static void func_ov036_021c3648(PartySelectList *psl, u32 mode);
static void func_ov036_021c38b4(PartySelectList *psl, u32 mode);
static void func_ov036_021c39cc(BmpMenuList *list, s32 value, u8 init);
static void func_ov036_021c39e8(BmpMenuList *list, s32 value, u8 y);
static void func_ov036_021c3bf0(PartySelectList *psl, u32 mode);
static void func_ov036_021c3aa4(PartySelectList *psl);
static void func_ov036_021c3abc(PartySelectList *psl, PokeParty *party);
static void func_ov036_021c3a70(PartySelectList *psl);

static const u32 data_ov036_021d2508[6] = { 0, 0, 1, 1, 2, 2 };
static const ListMenuRequest data_ov036_021d2520 = { 3, 3,   0,    0x0d, 0x1000, 0x2f, 0, 1, 0,
                                                     0, 0xc, 0x10, 0x12, 0xb,    0xd,  6, 0 };

void *func_ov036_021c3180(Field *field, PokeParty *temoti, PokeParty *bbox, u8 partyOk, u32 bboxOk, HeapID heapId) {
    PartySelectList *psl;

    GFL_ASSERT(temoti != NULL && bbox != NULL);
    psl = GFL_HeapAllocate(heapId, sizeof(PartySelectList), TRUE, "party_select_list.c", 211);
    psl->field = field;
    psl->temoti = temoti;
    psl->bbox = bbox;
    psl->heapId = heapId;
    psl->partyOk = partyOk;
    psl->bboxOk = bboxOk;
    psl->unit = ClActUnit_Create(32, 10, psl->heapId);
    psl->font = GFL_FontCreate(23, 3, 0, 0, psl->heapId);
    func_ov036_021c3924(psl);
    func_ov036_021c3344(psl);
    func_ov036_021c3468(psl);
    func_ov036_021c36b8(psl);
    func_ov036_021c3750(psl);
    return psl;
}

int func_ov036_021c3218(void *select, u32 *out) {
    PartySelectList *psl = select;
    int result = func_ov036_021c3318(psl, out);

    GFL_ASSERT(psl->finish == TRUE);
    func_ov036_021c3864(psl);
    func_ov036_021c3730(psl);
    func_ov036_021c3600(psl);
    func_ov036_021c340c(psl);
    func_ov036_021c3950(psl);
    GFL_FontFree(psl->font);
    ClActUnit_Delete(psl->unit);
    GFL_HeapFree(psl);
    return result;
}

BOOL func_ov036_021c3278(void *select) {
    PartySelectList *psl = select;

    switch (psl->state) {
    case 0:
        func_ov036_021c3a2c(psl);
        func_ov036_021c3a50(psl, 3);
        psl->state++;
        break;
    case 1:
        if (func_ov036_021c3a64(psl) == TRUE) {
            func_ov036_021c3968(psl);
            psl->state++;
        }
        break;
    case 2:
        if (func_ov036_0218798c(psl->msgBGSys) == TRUE) {
            s32 result = func_ov036_021c39c0(psl);

            if (result != BMPMENULIST_NULL) {
                func_ov036_021c39b4(psl);
                func_ov036_021c3c18(psl);
                func_ov036_021c3a44(psl);
                if (result == 2 || result == BMPMENULIST_CANCEL) {
                    psl->cursor = 2;
                } else if (result == BMPMENULIST_NULL) {
                    break;
                } else {
                    psl->cursor = result;
                }
                psl->state++;
            }
        }
        break;
    default:
        psl->finish = TRUE;
        return TRUE;
    }
    return FALSE;
}

static int func_ov036_021c3318(PartySelectList *psl, u32 *out) {
    *out = 0;
    if (psl->cursor == 0 && psl->partyOk == 1) {
        *out = 1;
    } else if (psl->cursor == 1 && psl->bboxOk == 1) {
        *out = 1;
    }
    return psl->cursor;
}

static void func_ov036_021c3344(PartySelectList *psl) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(7, psl->heapId);
    int count;
    int i;

    psl->iconPalette = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), 0, 192, psl->heapId);
    psl->iconCells = ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), psl->heapId);
    count = PokeParty_GetPkmCount(psl->temoti);
    for (i = 0; i < count; i++) {
        psl->partyChars[i] = ClActSys_LoadChars(
            arc, PokeIcon_GetFileOfPkm(func_0201d620(PokeParty_GetPkm(psl->temoti, i))), 0, 0, psl->heapId);
    }
    if (psl->bbox != NULL) {
        count = PokeParty_GetPkmCount(psl->bbox);
        for (i = 0; i < count; i++) {
            psl->bboxChars[i] = ClActSys_LoadChars(
                arc, PokeIcon_GetFileOfPkm(func_0201d620(PokeParty_GetPkm(psl->bbox, i))), 0, 0, psl->heapId);
        }
    }
    GFL_ArcToolFree(arc);
}

static void func_ov036_021c340c(PartySelectList *psl) {
    int count;
    int i;

    if (psl->bbox != NULL) {
        count = PokeParty_GetPkmCount(psl->bbox);
        for (i = 0; i < count; i++) {
            ClActSys_FreeChars(psl->bboxChars[i]);
        }
    }
    count = PokeParty_GetPkmCount(psl->temoti);
    for (i = 0; i < count; i++) {
        ClActSys_FreeChars(psl->partyChars[i]);
    }
    ClActSys_FreeCellAnims(psl->iconCells);
    ClActSys_FreePltt(psl->iconPalette);
}

static void func_ov036_021c3468(PartySelectList *psl) {
    ClActorSetup setup = { 0, 32, 0, 21, 0 };
    int count = PokeParty_GetPkmCount(psl->temoti);
    int bboxCount;
    u32 species;
    u32 form;
    u32 species2;
    u32 form2;
    int i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm;
        BOOL egg;
        u32 sex;
        u32 palette;

        setup.x = i * 40 + 24;
        psl->partyIcons[i] =
            ClActor_Create(psl->unit, psl->partyChars[i], psl->iconPalette, psl->iconCells, &setup, 0, psl->heapId);
        pkm = PokeParty_GetPkm(psl->temoti, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        if (PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL) == SPECIES_EGG) {
            egg = TRUE;
        } else {
            egg = FALSE;
        }
        sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
        palette = PokeIcon_GetPalette(species, form, sex, egg);
        ClActor_SetPalette(psl->partyIcons[i], (u8)palette, 1);
        ClActor_SetVisible(psl->partyIcons[i], FALSE);
    }
    if (psl->bbox != NULL) {
        bboxCount = PokeParty_GetPkmCount(psl->bbox);
        for (i = 0; i < bboxCount; i++) {
            PartyPkm *pkm;
            BOOL egg;
            u32 sex;
            u32 palette;

            setup.x = i * 40 + 24;
            psl->bboxIcons[i] =
                ClActor_Create(psl->unit, psl->bboxChars[i], psl->iconPalette, psl->iconCells, &setup, 0, psl->heapId);
            pkm = PokeParty_GetPkm(psl->bbox, i);
            species2 = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            form2 = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
            if (PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL) == SPECIES_EGG) {
                egg = TRUE;
            } else {
                egg = FALSE;
            }
            sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
            palette = PokeIcon_GetPalette(species2, form2, sex, egg);
            ClActor_SetPalette(psl->bboxIcons[i], (u8)palette, 1);
            ClActor_SetVisible(psl->bboxIcons[i], FALSE);
        }
    }
}

static void func_ov036_021c3600(PartySelectList *psl) {
    int count = PokeParty_GetPkmCount(psl->temoti);
    int i;

    for (i = 0; i < count; i++) {
        ClActor_Delete(psl->partyIcons[i]);
    }
    if (psl->bbox != NULL) {
        count = PokeParty_GetPkmCount(psl->bbox);
        for (i = 0; i < count; i++) {
            ClActor_Delete(psl->bboxIcons[i]);
        }
    }
}

static void func_ov036_021c3648(PartySelectList *psl, u32 mode) {
    int i;

    for (i = 0; i < PARTY_SELECT_LIST_MAX; i++) {
        if (psl->partyIcons[i] != NULL) {
            ClActor_SetVisible(psl->partyIcons[i], FALSE);
        }
        if (psl->bboxIcons[i] != NULL) {
            ClActor_SetVisible(psl->bboxIcons[i], FALSE);
        }
    }
    if (mode == 0) {
        for (i = 0; i < PARTY_SELECT_LIST_MAX; i++) {
            if (psl->partyIcons[i] != NULL) {
                ClActor_SetVisible(psl->partyIcons[i], TRUE);
            }
        }
    } else if (mode == 1) {
        for (i = 0; i < PARTY_SELECT_LIST_MAX; i++) {
            if (psl->bboxIcons[i] != NULL) {
                ClActor_SetVisible(psl->bboxIcons[i], TRUE);
            }
        }
    }
}

static void func_ov036_021c36b8(PartySelectList *psl) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(0x52, psl->heapId);

    psl->itemPalette = ClActSys_LoadPlttRange(arc, func_0202d890(), 0, 0x120, 0, 1, psl->heapId);
    psl->itemCells = ClActSys_LoadCellAnims(arc, func_0202d898(1), func_0202d89c(1), psl->heapId);
    psl->itemChars = ClActSys_LoadChars(arc, func_0202d894(), 0, 0, psl->heapId);
    GFL_ArcToolFree(arc);
}

static void func_ov036_021c3730(PartySelectList *psl) {
    ClActSys_FreeChars(psl->itemChars);
    ClActSys_FreeCellAnims(psl->itemCells);
    ClActSys_FreePltt(psl->itemPalette);
}

static void func_ov036_021c3750(PartySelectList *psl) {
    ClActorSetup setup = { 0, 40, 0, 20, 0 };
    int count = PokeParty_GetPkmCount(psl->temoti);
    int i;

    for (i = 0; i < count; i++) {
        u32 item = PokeParty_GetParam(PokeParty_GetPkm(psl->temoti, i), PKM_PARAM_ITEM, NULL);

        if (item != 0) {
            if (PML_ItemIsMail(item) == TRUE) {
                setup.sequence = 1;
            } else {
                setup.sequence = 0;
            }
            setup.x = i * 40 + 32;
            psl->partyItems[i] =
                ClActor_Create(psl->unit, psl->itemChars, psl->itemPalette, psl->itemCells, &setup, 0, psl->heapId);
            ClActor_SetVisible(psl->partyItems[i], FALSE);
        }
    }
    if (psl->bbox != NULL) {
        count = PokeParty_GetPkmCount(psl->bbox);
        for (i = 0; i < count; i++) {
            u32 item = PokeParty_GetParam(PokeParty_GetPkm(psl->bbox, i), PKM_PARAM_ITEM, NULL);

            if (item != 0) {
                if (PML_ItemIsMail(item) == TRUE) {
                    setup.sequence = 1;
                } else {
                    setup.sequence = 0;
                }
                setup.x = i * 40 + 32;
                psl->bboxItems[i] =
                    ClActor_Create(psl->unit, psl->itemChars, psl->itemPalette, psl->itemCells, &setup, 0, psl->heapId);
                ClActor_SetVisible(psl->bboxItems[i], FALSE);
            }
        }
    }
}

static void func_ov036_021c3864(PartySelectList *psl) {
    int count = PokeParty_GetPkmCount(psl->temoti);
    int i;

    for (i = 0; i < count; i++) {
        if (psl->partyItems[i] != NULL) {
            ClActor_Delete(psl->partyItems[i]);
        }
    }
    if (psl->bbox != NULL) {
        count = PokeParty_GetPkmCount(psl->bbox);
        for (i = 0; i < count; i++) {
            if (psl->bboxItems[i] != NULL) {
                ClActor_Delete(psl->bboxItems[i]);
            }
        }
    }
}

static void func_ov036_021c38b4(PartySelectList *psl, u32 mode) {
    int i;

    for (i = 0; i < PARTY_SELECT_LIST_MAX; i++) {
        if (psl->partyItems[i] != NULL) {
            ClActor_SetVisible(psl->partyItems[i], FALSE);
        }
        if (psl->bboxItems[i] != NULL) {
            ClActor_SetVisible(psl->bboxItems[i], FALSE);
        }
    }
    if (mode == 0) {
        for (i = 0; i < PARTY_SELECT_LIST_MAX; i++) {
            if (psl->partyItems[i] != NULL) {
                ClActor_SetVisible(psl->partyItems[i], TRUE);
            }
        }
    } else if (mode == 1) {
        for (i = 0; i < PARTY_SELECT_LIST_MAX; i++) {
            if (psl->bboxItems[i] != NULL) {
                ClActor_SetVisible(psl->bboxItems[i], TRUE);
            }
        }
    }
}

static void func_ov036_021c3924(PartySelectList *psl) {
    void *msgBGSys = Field_GetMsgBGSys(psl->field);

    psl->msgData = func_ov036_021879a0(msgBGSys, 0x68);
    psl->msgBGSys = msgBGSys;
    psl->wordSet = GFL_WordSetSystemCreateDefault(psl->heapId);
}

static void func_ov036_021c3950(PartySelectList *psl) {
    GFL_WordSetSystemFree(psl->wordSet);
    func_ov036_021879b8(psl->msgData);
}

static void func_ov036_021c3968(PartySelectList *psl) {
    ListMenuOption *options = func_ov036_02187f0c(data_ov036_021d2508, 3, psl->msgData, psl->heapId);

    psl->list = func_ov036_02187e1c(psl->msgBGSys, &data_ov036_021d2520, options, 0, 0, func_ov036_021c39cc,
                                    func_ov036_021c39e8, psl);
}

static void func_ov036_021c39b4(PartySelectList *psl) {
    func_ov036_02187ea0(psl->list);
}

static s32 func_ov036_021c39c0(PartySelectList *psl) {
    return ListMenuUI_Update(psl->list);
}

static void func_ov036_021c39cc(BmpMenuList *list, s32 value, u8 init) {
    PartySelectList *psl = BmpMenuList_GetWork(list);

    // The party or the Battle Box, and not a negative result
    if ((u32)value < 2) {
        func_ov036_021c3bf0(psl, value);
    } else {
        func_ov036_021c3c18(psl);
    }
}

static void func_ov036_021c39e8(BmpMenuList *list, s32 value, u8 y) {
    PartySelectList *psl = BmpMenuList_GetWork(list);

    if (value == 0 && psl->partyOk == 1) {
        BmpMenuList_SetOverrideColors(list, 1, 0, 2);
        return;
    }
    if (value == 1) {
        BmpMenuList_SetOverrideColors(list, 1, 0, 2);
        return;
    }
    BmpMenuList_SetOverrideColors(list, 1, 0, 2);
}

static void func_ov036_021c3a2c(PartySelectList *psl) {
    psl->infoWin = func_ov036_0218845c(psl->msgBGSys, psl->msgData);
}

static void func_ov036_021c3a44(PartySelectList *psl) {
    func_ov036_02188338(psl->infoWin);
}

static void func_ov036_021c3a50(PartySelectList *psl, u32 messageId) {
    func_ov036_0218836c(psl->infoWin, 0, 0, messageId);
}

static BOOL func_ov036_021c3a64(PartySelectList *psl) {
    return func_ov036_021883e8(psl->infoWin);
}

static void func_ov036_021c3a70(PartySelectList *psl) {
    if (psl->moneyWin == NULL) {
        psl->moneyWin = FieldMsgBG_CreateMoneyWin(psl->msgBGSys, psl->msgData, 1, 2, 30, 6);
    }
}

static void func_ov036_021c3aa4(PartySelectList *psl) {
    if (psl->moneyWin != NULL) {
        func_ov036_02187c1c(psl->moneyWin);
        psl->moneyWin = NULL;
    }
}

static void func_ov036_021c3abc(PartySelectList *psl, PokeParty *party) {
    StrBuf *text;
    StrBuf *format;
    int count;
    int i;

    func_ov036_021c3a70(psl);
    func_ov036_02187c7c(psl->moneyWin);
    text = GFL_StrBufCreate(64, psl->heapId);
    format = GFL_StrBufCreate(64, psl->heapId);
    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        u32 species;
        u32 sex;
        u32 level;
        u32 showSex;
        int offset;

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
            showSex = PokeParty_GetParam(pkm, PKM_PARAM_SHOW_SEX, NULL);
            level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
            GFL_MsgDataLoadStrbuf(psl->msgData, 4, format);
            WordSetNumber(psl->wordSet, 0, level, 3, 0, 1);
            GFL_WordSetFormatStrbuf(psl->wordSet, text, format);
            offset = i * 40;
            func_ov036_02187c58(psl->moneyWin, offset, 32, text, 0x440, psl->font);
            if (sex != 2) {
                if ((species == 29 || species == 32) && showSex == 0) {
                    continue;
                }
                if (sex == 0) {
                    func_ov036_02187c34(psl->moneyWin, offset + 31, 16, 5);
                } else {
                    func_ov036_02187c34(psl->moneyWin, offset + 31, 16, 6);
                }
            }
        }
    }
    GFL_StrBufFree(text);
    GFL_StrBufFree(format);
}

static void func_ov036_021c3bf0(PartySelectList *psl, u32 mode) {
    PokeParty *party;

    if (mode == 0) {
        party = psl->temoti;
    } else {
        party = psl->bbox;
    }
    func_ov036_021c3abc(psl, party);
    func_ov036_021c3648(psl, mode);
    func_ov036_021c38b4(psl, mode);
}

static void func_ov036_021c3c18(PartySelectList *psl) {
    func_ov036_021c3aa4(psl);
    func_ov036_021c3648(psl, 2);
    func_ov036_021c38b4(psl, 2);
}
