// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_msgbg.h"
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
#include "system/str_tool.h"
#include "system/wordset.h"
#include "text/system/0104.h"

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

static void PartySelectList_CreateMessages(PartySelectList *psl);
static void PartySelectList_LoadIconResources(PartySelectList *psl);
static void PartySelectList_CreateIcons(PartySelectList *psl);
static void PartySelectList_LoadItemResources(PartySelectList *psl);
static void PartySelectList_CreateItemIcons(PartySelectList *psl);
static int PartySelectList_GetResult(PartySelectList *psl, u32 *out);
static void PartySelectList_DeleteItemIcons(PartySelectList *psl);
static void PartySelectList_FreeItemResources(PartySelectList *psl);
static void PartySelectList_DeleteIcons(PartySelectList *psl);
static void PartySelectList_FreeIconResources(PartySelectList *psl);
static void PartySelectList_FreeMessages(PartySelectList *psl);
static void PartySelectList_CreateMessageWindow(PartySelectList *psl);
static void PartySelectList_PrintMessage(PartySelectList *psl, u32 messageId);
static BOOL PartySelectList_IsMessageDone(PartySelectList *psl);
static void PartySelectList_CreateMenu(PartySelectList *psl);
static u32 PartySelectList_UpdateMenu(PartySelectList *psl);
static void PartySelectList_FreeMenu(PartySelectList *psl);
static void PartySelectList_ClearPreview(PartySelectList *psl);
static void PartySelectList_FreeMessageWindow(PartySelectList *psl);
static void PartySelectList_ShowIcons(PartySelectList *psl, u32 mode);
static void PartySelectList_ShowItemIcons(PartySelectList *psl, u32 mode);
static void PartySelectList_CursorCallback(BmpMenuList *list, s32 value, u8 init);
static void PartySelectList_RowColorCallback(BmpMenuList *list, s32 value, u8 y);
static void PartySelectList_ShowPreview(PartySelectList *psl, u32 mode);
static void PartySelectList_FreePreviewWindow(PartySelectList *psl);
static void PartySelectList_DrawPartyDetails(PartySelectList *psl, PokeParty *party);
static void PartySelectList_CreatePreviewWindow(PartySelectList *psl);

static const ListMenuRow sPartySelectRows[] = {
    { Bank0104_Text_Party, { 0 } },
    { Bank0104_Text_BattleBox, { 1 } },
    { Bank0104_Text_Cancel, { 2 } },
};

static const ListMenuRequest sPartySelectRequest = {
    .count = NELEMS(sPartySelectRows),
    .maxShown = 3,
    .itemX = 13,
    .fgColor = 1,
    .bgColor = 15,
    .shadowColor = 2,
    .lineSpacing = 1,
    .fontSizeX = 12,
    .rowHeight = 16,
    .x = 18,
    .y = 11,
    .width = 13,
    .height = 6,
};

void *func_ov036_021c3180(Field *field, PokeParty *temoti, PokeParty *bbox, u8 partyOk, u32 bboxOk, HeapID heapId) {
    PartySelectList *psl;

    // clang-format off
    GFL_ASSERT(temoti != NULL && bbox != NULL);
    // clang-format on
    psl = GFL_HeapAllocate(heapId, sizeof(PartySelectList), TRUE, "party_select_list.c", 211);
    psl->field = field;
    psl->temoti = temoti;
    psl->bbox = bbox;
    psl->heapId = heapId;
    psl->partyOk = partyOk;
    psl->bboxOk = bboxOk;
    psl->unit = ClActUnit_Create(32, 10, psl->heapId);
    psl->font = GFL_FontCreate(ARCID_FONT, 3, 0, 0, psl->heapId);
    PartySelectList_CreateMessages(psl);
    PartySelectList_LoadIconResources(psl);
    PartySelectList_CreateIcons(psl);
    PartySelectList_LoadItemResources(psl);
    PartySelectList_CreateItemIcons(psl);
    return psl;
}

int func_ov036_021c3218(void *select, u32 *out) {
    PartySelectList *psl = select;
    int result = PartySelectList_GetResult(psl, out);

    // clang-format off
    GFL_ASSERT(psl->finish == TRUE);
    // clang-format on
    PartySelectList_DeleteItemIcons(psl);
    PartySelectList_FreeItemResources(psl);
    PartySelectList_DeleteIcons(psl);
    PartySelectList_FreeIconResources(psl);
    PartySelectList_FreeMessages(psl);
    GFL_FontFree(psl->font);
    ClActUnit_Delete(psl->unit);
    GFL_HeapFree(psl);
    return result;
}

BOOL func_ov036_021c3278(void *select) {
    PartySelectList *psl = select;

    switch (psl->state) {
    case 0:
        PartySelectList_CreateMessageWindow(psl);
        PartySelectList_PrintMessage(psl, Bank0104_Text_PleaseSelectWhetherUse);
        psl->state++;
        break;
    case 1:
        if (PartySelectList_IsMessageDone(psl) == TRUE) {
            PartySelectList_CreateMenu(psl);
            psl->state++;
        }
        break;
    case 2:
        if (func_ov036_0218798c(psl->msgBGSys) == TRUE) {
            u32 result = PartySelectList_UpdateMenu(psl);

            if (result != BMPMENULIST_NULL) {
                PartySelectList_FreeMenu(psl);
                PartySelectList_ClearPreview(psl);
                PartySelectList_FreeMessageWindow(psl);
                // Unsigned, so the cases are tested in the order 2, BMPMENULIST_CANCEL, BMPMENULIST_NULL
                switch (result) {
                case 2: // Cancel, chosen from the list or with B
                case BMPMENULIST_CANCEL:
                    psl->cursor = 2;
                    psl->state++;
                    break;
                case BMPMENULIST_NULL:
                    break;
                default:
                    psl->cursor = result;
                    psl->state++;
                    break;
                }
            }
        }
        break;
    default:
        psl->finish = TRUE;
        return TRUE;
    }
    return FALSE;
}

static int PartySelectList_GetResult(PartySelectList *psl, u32 *out) {
    *out = 0;
    if (psl->cursor == 0 && psl->partyOk == 1) {
        *out = 1;
    } else if (psl->cursor == 1 && psl->bboxOk == 1) {
        *out = 1;
    }
    return psl->cursor;
}

static void PartySelectList_LoadIconResources(PartySelectList *psl) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, psl->heapId);
    int count;
    int i;

    psl->iconPalette = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 192, psl->heapId);
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

static void PartySelectList_FreeIconResources(PartySelectList *psl) {
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

static void PartySelectList_CreateIcons(PartySelectList *psl) {
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

static void PartySelectList_DeleteIcons(PartySelectList *psl) {
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

static void PartySelectList_ShowIcons(PartySelectList *psl, u32 mode) {
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

static void PartySelectList_LoadItemResources(PartySelectList *psl) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_APP_MENU_COMMON, psl->heapId);

    psl->itemPalette =
        ClActSys_LoadPlttRange(arc, AppMenuCommon_GetItemIconPaletteFile(), CLACT_VRAM_MAIN, 0x120, 0, 1, psl->heapId);
    psl->itemCells = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetItemIconCellFile(1),
                                            AppMenuCommon_GetItemIconAnimFile(1), psl->heapId);
    psl->itemChars = ClActSys_LoadChars(arc, AppMenuCommon_GetItemIconCharFile(), 0, 0, psl->heapId);
    GFL_ArcToolFree(arc);
}

static void PartySelectList_FreeItemResources(PartySelectList *psl) {
    ClActSys_FreeChars(psl->itemChars);
    ClActSys_FreeCellAnims(psl->itemCells);
    ClActSys_FreePltt(psl->itemPalette);
}

static void PartySelectList_CreateItemIcons(PartySelectList *psl) {
    ClActorSetup setup = { 0, 40, 0, 20, 0 };
    int count = PokeParty_GetPkmCount(psl->temoti);
    int i;

    for (i = 0; i < count; i++) {
        u32 item = PokeParty_GetParam(PokeParty_GetPkm(psl->temoti, i), PKM_PARAM_ITEM, NULL);

        if (item != ITEM_NONE) {
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

            if (item != ITEM_NONE) {
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

static void PartySelectList_DeleteItemIcons(PartySelectList *psl) {
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

static void PartySelectList_ShowItemIcons(PartySelectList *psl, u32 mode) {
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

static void PartySelectList_CreateMessages(PartySelectList *psl) {
    void *msgBGSys = Field_GetMsgBGSys(psl->field);

    psl->msgData = func_ov036_021879a0(msgBGSys, TEXT_BANK_0104);
    psl->msgBGSys = msgBGSys;
    psl->wordSet = GFL_WordSetSystemCreateDefault(psl->heapId);
}

static void PartySelectList_FreeMessages(PartySelectList *psl) {
    GFL_WordSetSystemFree(psl->wordSet);
    func_ov036_021879b8(psl->msgData);
}

static void PartySelectList_CreateMenu(PartySelectList *psl) {
    ListMenuOption *options =
        func_ov036_02187f0c(sPartySelectRows, NELEMS(sPartySelectRows), psl->msgData, psl->heapId);

    psl->list = func_ov036_02187e1c(psl->msgBGSys, &sPartySelectRequest, options, 0, 0, PartySelectList_CursorCallback,
                                    PartySelectList_RowColorCallback, psl);
}

static void PartySelectList_FreeMenu(PartySelectList *psl) {
    func_ov036_02187ea0(psl->list);
}

static u32 PartySelectList_UpdateMenu(PartySelectList *psl) {
    return ListMenuUI_Update(psl->list);
}

static void PartySelectList_CursorCallback(BmpMenuList *list, s32 value, u8 init) {
    PartySelectList *psl = BmpMenuList_GetWork(list);

    // The party or the Battle Box, and not a negative result
    if ((u32)value < 2) {
        PartySelectList_ShowPreview(psl, value);
    } else {
        PartySelectList_ClearPreview(psl);
    }
}

static void PartySelectList_RowColorCallback(BmpMenuList *list, s32 value, u8 y) {
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

static void PartySelectList_CreateMessageWindow(PartySelectList *psl) {
    psl->infoWin = func_ov036_0218845c(psl->msgBGSys, psl->msgData);
}

static void PartySelectList_FreeMessageWindow(PartySelectList *psl) {
    func_ov036_02188338(psl->infoWin);
}

static void PartySelectList_PrintMessage(PartySelectList *psl, u32 messageId) {
    func_ov036_0218836c(psl->infoWin, 0, 0, messageId);
}

static BOOL PartySelectList_IsMessageDone(PartySelectList *psl) {
    return func_ov036_021883e8(psl->infoWin);
}

static void PartySelectList_CreatePreviewWindow(PartySelectList *psl) {
    if (psl->moneyWin == NULL) {
        psl->moneyWin = FieldMsgBG_CreateMoneyWin(psl->msgBGSys, psl->msgData, 1, 2, 30, 6);
    }
}

static void PartySelectList_FreePreviewWindow(PartySelectList *psl) {
    if (psl->moneyWin != NULL) {
        func_ov036_02187c1c(psl->moneyWin);
        psl->moneyWin = NULL;
    }
}

static void PartySelectList_DrawPartyDetails(PartySelectList *psl, PokeParty *party) {
    StrBuf *text;
    StrBuf *format;
    int count;
    int i;

    PartySelectList_CreatePreviewWindow(psl);
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
            GFL_MsgDataLoadStrbuf(psl->msgData, Bank0104_Text_L, format);
            WordSetNumber(psl->wordSet, 0, level, 3, NUM_PAD_NONE, TRUE);
            GFL_WordSetFormatStrbuf(psl->wordSet, text, format);
            offset = i * 40;
            func_ov036_02187c58(psl->moneyWin, offset, 32, text, 0x440, psl->font);
            if (sex != GENDER_UNKNOWN) {
                if ((species == SPECIES_NIDORAN_F || species == SPECIES_NIDORAN_M) && showSex == 0) {
                    continue;
                }
                // The male and the female sign
                if (sex == GENDER_MALE) {
                    func_ov036_02187c34(psl->moneyWin, offset + 31, 16, Bank0104_Text_Empty);
                } else {
                    func_ov036_02187c34(psl->moneyWin, offset + 31, 16, Bank0104_Text_Empty_2);
                }
            }
        }
    }
    GFL_StrBufFree(text);
    GFL_StrBufFree(format);
}

static void PartySelectList_ShowPreview(PartySelectList *psl, u32 mode) {
    PokeParty *party;

    if (mode == 0) {
        party = psl->temoti;
    } else {
        party = psl->bbox;
    }
    PartySelectList_DrawPartyDetails(psl, party);
    PartySelectList_ShowIcons(psl, mode);
    PartySelectList_ShowItemIcons(psl, mode);
}

static void PartySelectList_ClearPreview(PartySelectList *psl) {
    PartySelectList_FreePreviewWindow(psl);
    PartySelectList_ShowIcons(psl, 2);
    PartySelectList_ShowItemIcons(psl, 2);
}
