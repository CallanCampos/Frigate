#include "types.h"
#include "app/ui/ui_scene.h"
#include "app/ui/touchbar.h"
#include "app/pokemon_trade_local.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/mcss.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_3.h"

// The trade's messages, menus and the panels that describe a Pokémon: its name, level, stats, moves and icons. The
// ROM names no file between pokemontrade_save.c and pokemontrade_3d.c, so the name is descriptive

static void func_ov194_021bfba4(ResSprite *sprite);
static void func_ov194_021bfbc0(ResSprite *sprite, PartyPkm *pkm, int index, ClActUnit *unit, int x, int y,
                                u32 vramType, HeapID heapId);
static void func_ov194_021bfc50(ResSprite *sprite);
static void func_ov194_021bfc6c(ResSprite *sprite, PartyPkm *pkm, ClActUnit *unit, int x, int y, u32 vramType,
                                HeapID heapId, u32 plttOffset);
static void func_ov194_021c0234(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, BOOL isEgg, BOOL speciesName,
                                PokemonTradeWork *wk);
static void func_ov194_021c02e0(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, BOOL isEgg, PokemonTradeWork *wk);
static void func_ov194_021c0398(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, BOOL isEgg, PokemonTradeWork *wk);
static void func_ov194_021c03fc(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c0474(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, PokemonTradeWork *wk, BOOL light,
                                BOOL speciesName);
static void func_ov194_021c051c(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c057c(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c05ec(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c0684(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c0750(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c0790(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c0848(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c08ac(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk);
static void func_ov194_021c0acc(PokemonTradeWork *wk);
static void func_ov194_021c0e58(PokemonTradeWork *wk, PartyPkm *pkm, int side, BOOL reload);
static void func_ov194_021c0e74(PokemonTradeWork *wk, PartyPkm *pkm, int *icons);
static void func_ov194_021c0f44(PokemonTradeWork *wk, PartyPkm *pkm);
static void func_ov194_021c189c(TCB *tcb, void *work);

static void func_ov194_021bfba4(ResSprite *sprite) {
    if (sprite->actor != NULL) {
        func_0204c108(sprite->actor);
        sprite->actor = NULL;
        UIObjRes_Free(&sprite->res);
    }
}

// A type icon of the Pokémon: its first type (0) or its second (1)
static void func_ov194_021bfbc0(ResSprite *sprite, PartyPkm *pkm, int index, ClActUnit *unit, int x, int y,
                                u32 vramType, HeapID heapId) {
    UIObjResSetup param;
    u8 type = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1 + index, NULL);
    param.vramType = vramType;
    param.flags = 0;
    param.arcId = getUINarcIdx();
    param.paletteFile = func_0202d7e4();
    param.charFile = func_0202d7f4(type);
    param.cellFile = func_0202d7f8(2);
    param.animFile = func_0202d7fc(2);
    param.paletteOffset = 8;
    param.paletteStart = 0;
    param.paletteCount = 3;
    UIObjRes_Load(&sprite->res, &param, unit, heapId);
    sprite->actor = UIObjRes_CreateActor(&sprite->res, unit, x, y, 0, heapId);
    func_0204c378(sprite->actor, func_0202d7e8(type), 1);
}

static void func_ov194_021bfc50(ResSprite *sprite) {
    if (sprite->actor != NULL) {
        func_0204c108(sprite->actor);
        sprite->actor = NULL;
        UIObjRes_Free(&sprite->res);
    }
}

// The icon of the Poké Ball the Pokémon was caught in
static void func_ov194_021bfc6c(ResSprite *sprite, PartyPkm *pkm, ClActUnit *unit, int x, int y, u32 vramType,
                                HeapID heapId, u32 plttOffset) {
    UIObjResSetup param;
    u32 ball = PokeParty_GetParam(pkm, PKM_PARAM_POKEBALL, NULL);
    func_ov194_021bfc50(sprite);
    param.vramType = vramType;
    param.flags = 0;
    param.arcId = getUINarcIdx();
    param.paletteFile = func_0202d91c(ball);
    param.charFile = func_0202d928(ball);
    param.cellFile = func_0202d934(ball, 2);
    param.animFile = func_0202d93c(ball, 2);
    param.paletteOffset = plttOffset;
    param.paletteStart = 0;
    param.paletteCount = 1;
    UIObjRes_Load(&sprite->res, &param, unit, heapId);
    sprite->actor = UIObjRes_CreateActor(&sprite->res, unit, x, y, 0, heapId);
}

// Opens the message window with wk->strbuf, printed a character at a time or all at once
void func_ov194_021bfcf8(PokemonTradeWork *wk, BOOL instant, u32 x, u32 y, u32 width, u32 height) {
    BmpWin *window;
    func_ov194_021bfe70(wk);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    window = BmpWin_CreateDynamic(6, x, y, width, height, 14, 0);
    wk->msgWindow = window;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    if (!instant) {
        wk->printStream =
            func_02022268(window, 0, 0, wk->strbuf, wk->font, func_02017bcc(), wk->tcbEx, 2, wk->heapId, 15);
        AppPrintsysCommon_Init(&wk->printWait, 2);
    } else {
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, wk->strbuf, wk->font);
    }
    BmpWin_DrawFrame(window, 1, wk->cursorImage, 15);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(6);
}

// Opens the message window at the top or the bottom of the lower screen
void func_ov194_021bfdf8(PokemonTradeWork *wk, BOOL instant, BOOL bottom) {
    if (bottom) {
        func_ov194_021bfcf8(wk, instant, 1, 18, 30, 4);
    } else {
        func_ov194_021bfcf8(wk, instant, 1, 1, 30, 4);
    }
}

void func_ov194_021bfe28(PokemonTradeWork *wk) {
    func_ov194_021bfdf8(wk, FALSE, FALSE);
}

// Shows the waiting icon in the message window
void func_ov194_021bfe34(PokemonTradeWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    wk->waitIcon = WaitIcon_CreateTCBEx(wk->tcbEx, wk->msgWindow, 15, 16, wk->heapId);
}

void func_ov194_021bfe70(PokemonTradeWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    if (wk->msgWindow != NULL) {
        BmpWin_Free(wk->msgWindow);
        wk->msgWindow = NULL;
    }
}

// Closes the message window, clearing it from the screen
void func_ov194_021bfe9c(PokemonTradeWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
    if (wk->msgWindow != NULL) {
        BmpWin_ClearScreen(wk->msgWindow);
        GFL_BGSysQueueScrLoad(6);
        BmpWin_ClearFrame(wk->msgWindow, 2);
        BmpWin_Free(wk->msgWindow);
        wk->msgWindow = NULL;
    }
}

void func_ov194_021bfedc(PokemonTradeWork *wk) {
    wk->wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_3, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->drawStr = GFL_StrBufCreate(128, wk->heapId);
    wk->drawTemplate = GFL_StrBufCreate(128, wk->heapId);
    wk->strbuf = GFL_StrBufCreate(128, wk->heapId);
    wk->strbufTemplate = GFL_StrBufCreate(128, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    wk->tcbEx = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 2, 0);
    wk->keyCursor = KeyCursor_Create(15, 1, 0, wk->heapId);
    wk->taskMenuRes = AppTaskMenuRes_Create(6, 9, wk->font, wk->printQueue, wk->heapId);
}

void func_ov194_021bffac(PokemonTradeWork *wk) {
    int i;
    GFL_VBlankResetCallback();
    if (wk->tcbEx != NULL) {
        KeyCursor_Free(wk->keyCursor);
        func_ov194_021bfe70(wk);
        func_ov194_021c00fc(wk);
        if (wk->printStream != NULL) {
            func_020223cc(wk->printStream);
            wk->printStream = NULL;
        }
        if (wk->menuWin != NULL) {
            AppTaskMenuWin_Free(wk->menuWin);
            wk->menuWin = NULL;
        }
        if (wk->taskMenuRes != NULL) {
            AppTaskMenuRes_Free(wk->taskMenuRes);
            wk->taskMenuRes = NULL;
        }
        if (wk->summaryWindow != NULL) {
            BmpWin_Free(wk->summaryWindow);
            wk->summaryWindow = NULL;
        }
        for (i = 0; i < 2; i++) {
            if (wk->unk5BC[i] != NULL) {
                BmpWin_Free(wk->unk5BC[i]);
                wk->unk5BC[i] = NULL;
            }
        }
        GFL_WordSetSystemFree(wk->wordSet);
        GFL_MsgDataFree(wk->msgData);
        GFL_FontFree(wk->font);
        GFL_StrBufFree(wk->drawStr);
        GFL_StrBufFree(wk->drawTemplate);
        GFL_StrBufFree(wk->strbuf);
        GFL_StrBufFree(wk->strbufTemplate);
        func_02021c44(wk->printQueue);
        func_02021a18(wk->printQueue);
        GFL_TCBExMgrFree(wk->tcbEx);
        wk->tcbEx = NULL;
    }
}

// Whether the message has finished printing
BOOL func_ov194_021c00b0(PokemonTradeWork *wk) {
    BOOL done = TRUE;
    if (wk->printStream != NULL) {
        if (wk->msgWindow != NULL) {
            KeyCursor_Update(wk->keyCursor, wk->printStream, wk->msgWindow);
        }
        done = AppPrintsysCommon_Update(&wk->printWait, wk->printStream);
        if (done && wk->printStream != NULL) {
            func_020223cc(wk->printStream);
            wk->printStream = NULL;
        }
    }
    return done;
}

void func_ov194_021c00fc(PokemonTradeWork *wk) {
    if (wk->menu != NULL) {
        G2_BlendNone();
        AppTaskMenu_Free(wk->menu);
        wk->menu = NULL;
    }
}

// Opens a menu of the messages in items, item 5 being the one that B picks
void func_ov194_021c0120(PokemonTradeWork *wk, const u32 *items, int count, u32 right, u32 bottom) {
    AppTaskMenuInit setup;
    int i;
    setup.heapId = wk->heapId;
    setup.itemCount = count;
    setup.items = wk->menuItems;
    setup.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    setup.x = right;
    setup.y = bottom;
    setup.width = 13;
    setup.height = 3;
    for (i = 0; i < count; i++) {
        wk->menuItems[i].str = GFL_StrBufCreate(100, wk->heapId);
        GFL_MsgDataLoadStrbuf(wk->msgData, items[i], wk->menuItems[i].str);
        wk->menuItems[i].color = PRINT_COLOR(14, 15, 3);
        if (items[i] == 5) {
            wk->menuItems[i].type = APP_TASKMENU_ITEM_RETURN;
        } else {
            wk->menuItems[i].type = 0;
        }
    }
    wk->menu = AppTaskMenu_Create(&setup, wk->taskMenuRes);
    for (i = 0; i < count; i++) {
        GFL_StrBufFree(wk->menuItems[i].str);
    }
}

void func_ov194_021c0214(PokemonTradeWork *wk, const u32 *items, int count) {
    if (count == 1) {
        func_ov194_021c0120(wk, items, count, 32, 21);
    } else {
        func_ov194_021c0120(wk, items, count, 32, 24);
    }
}

// The Pokémon's name: its nickname or its species, or "Egg"
static void func_ov194_021c0234(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, BOOL isEgg, BOOL speciesName,
                                PokemonTradeWork *wk) {
    if (!isEgg) {
        if (speciesName) {
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_16, wk->drawTemplate);
            setPartyPokemonSpeciesNameToStrbuf(wk->wordSet, 0, pkm);
        } else {
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_17, wk->drawTemplate);
            loadPokemonNicknameToStrbuf(wk->wordSet, 0, pkm);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
    } else {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Egg, wk->drawStr);
    }
    GFL_TextRendererDrawToBitmap(bitmap, x, y, wk->drawStr, wk->font);
}

// The Pokédex number, regional or national
static void func_ov194_021c02e0(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, BOOL isEgg, PokemonTradeWork *wk) {
    if (!isEgg) {
        u32 number = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
        if (!wk->nationalDex) {
            u16 *table = PML_PersonalLoadRegionalDexTable(wk->heapId, 0);
            number = table[number];
            GFL_HeapFree(table);
        }
        if (number == 999) {
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_52, wk->drawStr);
        } else {
            WordSetNumber(wk->wordSet, 1, number, 3, 1, TRUE);
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_No_2, wk->drawTemplate);
            GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
        }
        GFL_TextRendererDrawToBitmap(bitmap, x, y, wk->drawStr, wk->font);
    }
}

// The species's name
static void func_ov194_021c0398(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, BOOL isEgg, PokemonTradeWork *wk) {
    if (!isEgg) {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_16, wk->drawTemplate);
        setPartyPokemonSpeciesNameToStrbuf(wk->wordSet, 0, pkm);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
        GFL_TextRendererDrawToBitmap(bitmap, x, y, wk->drawStr, wk->font);
    }
}

// The level
static void func_ov194_021c03fc(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, PokemonTradeWork *wk) {
    // The species is read and left unused
    PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
    GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Lv, wk->drawTemplate);
    WordSetNumber(wk->wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL), 3, 1, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
    GFL_TextRendererDrawToBitmap(bitmap, x, y, wk->drawStr, wk->font);
}

// The sex's symbol, in the colors of a light or a dark background. A Nidoran's name already shows it, unless the
// name shown is the nickname
static void func_ov194_021c0474(PartyPkm *pkm, GFLBitmap *bitmap, int x, int y, PokemonTradeWork *wk, BOOL light,
                                BOOL speciesName) {
    int sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    u32 showSex = PokeParty_GetParam(pkm, PKM_PARAM_NIDORAN_NICKNAME, NULL);
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 color;
    if (sex > GENDER_FEMALE || !showSex) {
        return;
    }
    if (speciesName && species == SPECIES_NIDORAN_F) {
        return;
    }
    if (speciesName && species == SPECIES_NIDORAN_M) {
        return;
    }
    if (light) {
        if (sex == GENDER_FEMALE) {
            color = PRINT_COLOR(12, 13, 0);
        } else {
            color = PRINT_COLOR(14, 15, 0);
        }
    } else {
        if (sex == GENDER_FEMALE) {
            color = PRINT_COLOR(3, 4, 0);
        } else {
            color = PRINT_COLOR(5, 6, 0);
        }
    }
    GFL_MsgDataLoadStrbuf(wk->msgData, sex + 46, wk->drawStr);
    GFL_TextRendererDrawToBitmapEx(bitmap, x, y, wk->drawStr, wk->font, color);
}

// The labels of the stats
static void func_ov194_021c051c(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 6; i++) {
        GFL_MsgDataLoadStrbuf(wk->msgData, i + 30, wk->drawStr);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y + i * 16, wk->drawStr, wk->font);
    }
}

// The held item
static void func_ov194_021c057c(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    u32 item;
    GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_23, wk->drawTemplate);
    item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    if (item != 0) {
        loadItemNameToStrbuf(wk->wordSet, 0, item);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, wk->drawStr, wk->font);
    }
}

// The HP and the maximum
static void func_ov194_021c05ec(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    u32 hp, maxHp;
    GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_19, wk->drawTemplate);
    hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
    maxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
    WordSetNumber(wk->wordSet, 0, hp, 3, 1, TRUE);
    WordSetNumber(wk->wordSet, 1, maxHp, 3, 1, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, wk->drawStr, wk->font);
}

// The other stats
static void func_ov194_021c0684(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    int i = 0;
    u32 params[] = { PKM_PARAM_ATTACK, PKM_PARAM_DEFENSE, PKM_PARAM_SP_ATTACK, PKM_PARAM_SP_DEFENSE, PKM_PARAM_SPEED };
    for (; i < 5; i++) {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_20, wk->drawTemplate);
        WordSetNumber(wk->wordSet, 0, PokeParty_GetParam(pkm, params[i], NULL), 3, 1, TRUE);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y + i * 16, wk->drawStr, wk->font);
    }
}

// The label of the moves
static void func_ov194_021c0750(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_MovesLearned, wk->drawStr);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, wk->drawStr, wk->font);
}

// The moves
static void func_ov194_021c0790(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 4; i++) {
        u32 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);
        if (move != 0) {
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_24, wk->drawTemplate);
            loadMoveNameToStrbuf(wk->wordSet, 0, move);
            GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y + i * 16, wk->drawStr, wk->font);
        }
    }
}

// The nature
static void func_ov194_021c0848(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_NATURES, wk->heapId);
    GFL_MsgDataLoadStrbuf(msgData, PokeParty_GetNature(pkm), wk->drawStr);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, wk->drawStr, wk->font);
    GFL_MsgDataFree(msgData);
}

// The ability
static void func_ov194_021c08ac(PartyPkm *pkm, BmpWin *window, int x, int y, PokemonTradeWork *wk) {
    GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_22, wk->drawTemplate);
    loadAbilityNameToStrbuf(wk->wordSet, 0, PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL));
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), x, y, wk->drawStr, wk->font);
}

// The panel of a side's Pokémon on the upper screen: its name, sex, level, item and ball
void func_ov194_021c0918(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    int windowX[] = { 1, 18 };
    int ballX[] = { 0, 136 };
    int ballPalette[] = { 12, 5 };
    BOOL isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1c0, 0x20, wk->heapId);
    if (wk->unk5BC[side] != NULL) {
        BmpWin_Free(wk->unk5BC[side]);
    }
    wk->unk5BC[side] = BmpWin_CreateDynamic(3, windowX[side], 1, 14, 20, 14, 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    func_ov194_021c0234(pkm, BmpWin_GetBitmap(wk->unk5BC[side]), 16, 0, isEgg, func_ov194_021b783c(wk), wk);
    if (!isEgg) {
        func_ov194_021c0474(pkm, BmpWin_GetBitmap(wk->unk5BC[side]), 80, 0, wk, FALSE, func_ov194_021b783c(wk));
        GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
        func_ov194_021c03fc(pkm, BmpWin_GetBitmap(wk->unk5BC[side]), 16, 19, wk);
        func_ov194_021c057c(pkm, wk->unk5BC[side], 16, 128, wk);
        func_ov194_021bfc6c(&wk->ballIcons[side + 1], pkm, wk->clactUnit, ballX[side] + 16, 16, 0, wk->heapId,
                            ballPalette[side]);
        func_0204c468(wk->ballIcons[side + 1].actor, 1);
    } else {
        func_ov194_021bfc50(&wk->ballIcons[side + 1]);
    }
    BmpWin_FlushChar(wk->unk5BC[side]);
    BmpWin_FlushMap(wk->unk5BC[side]);
    GFL_BGSysQueueScrLoad(3);
}

void func_ov194_021c0aac(PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 3; i++) {
        func_ov194_021bfc50(&wk->ballIcons[i]);
    }
}

static void func_ov194_021c0acc(PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 4; i++) {
        func_ov194_021bfba4(&wk->typeIcons[i]);
    }
}

// Closes the summary
void func_ov194_021c0aec(PokemonTradeWork *wk, BOOL hideWindows) {
    func_ov194_021c4cfc(&wk->infoIcons[1]);
    func_ov194_021c4cfc(&wk->infoIcons[2]);
    func_ov194_021c5060(wk);
    if (hideWindows) {
        GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    }
    func_ov194_021c0aac(wk);
    func_ov194_021c0acc(wk);
    func_ov194_021c4cfc(&wk->infoIcons[0]);
    func_ov194_021c49e8(wk);
    func_ov194_021c24dc(wk, 0);
    func_ov194_021c24dc(wk, 1);
    BmpWin_ClearScreen(wk->summaryWindow);
    BmpWin_Free(wk->summaryWindow);
    wk->summaryWindow = NULL;
    func_ov194_021c2d74(wk);
}

// Opens the summary of the Pokémon
void func_ov194_021c0b6c(PokemonTradeWork *wk, PartyPkm *pkm) {
    GFL_BGSysSetBGEnabled(3, FALSE);
    func_ov194_021c2034(wk);
    func_ov194_021bfe9c(wk);
    func_ov194_021c123c(wk, 0);
    func_ov194_021c123c(wk, 1);
    func_ov194_021c475c(wk);
    func_ov194_021c0fa0(wk, pkm, 0, TRUE);
    if (!PokemonTrade_IsNegoType(wk)) {
        func_ov194_021c479c(wk);
        func_ov194_021c2d34(wk);
        GFL_BGSysSetEnabledBGsA(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ);
        GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 |
                                GX_PLANEMASK_OBJ);
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR,
                                 GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -8);
        wk->unk11F9 = 1;
        GFL_VBlankTCBAdd(func_ov194_021c189c, wk, 0);
    }
    TouchBar_SetIconVisible(wk->touchBar, 1, TRUE);
}

// The page of the Pokémon's summary on the lower screen: its stats (0) or its moves (1)
void func_ov194_021c0c04(PokemonTradeWork *wk, int page, PartyPkm *pkm) {
    int windowX[] = { 0, 16 };
    int ballX[] = { 0, 128 };
    int side = 0;
    BOOL isEgg;
    if (wk->touchX < 128) {
        side = 1;
    }
    func_ov194_021c5098(wk, 11, 4);
    if (wk->summaryWindow != NULL) {
        BmpWin_Free(wk->summaryWindow);
    }
    wk->summaryWindow = BmpWin_CreateDynamic(6, windowX[side], 0, 16, 18, 11, 0);
    GFL_TextRndUpdateColorIndexLUT(14, 15, 0);
    isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (page == 0) {
        func_ov194_021c0234(pkm, BmpWin_GetBitmap(wk->summaryWindow), 32, 0, isEgg, FALSE, wk);
        if (!isEgg) {
            func_ov194_021c03fc(pkm, BmpWin_GetBitmap(wk->summaryWindow), 32, 16, wk);
            func_ov194_021c051c(pkm, wk->summaryWindow, 8, 32, wk);
            func_ov194_021c057c(pkm, wk->summaryWindow, 24, 128, wk);
            func_ov194_021c05ec(pkm, wk->summaryWindow, 64, 32, wk);
            func_ov194_021c0684(pkm, wk->summaryWindow, 80, 48, wk);
            func_ov194_021c0474(pkm, BmpWin_GetBitmap(wk->summaryWindow), 96, 0, wk, FALSE, FALSE);
            func_ov194_021c4c00(wk, side, pkm);
            func_ov194_021c4d18(wk, side, 0, pkm);
            func_ov194_021bfc6c(&wk->ballIcons[0], pkm, wk->clactUnit, ballX[side] + 16, 16, 1, wk->heapId, 13);
        } else {
            func_ov194_021bfc50(&wk->ballIcons[0]);
        }
    } else {
        func_ov194_021bfc50(&wk->ballIcons[0]);
        func_ov194_021c4cfc(&wk->infoIcons[0]);
        func_ov194_021c4cfc(&wk->infoIcons[1]);
        func_ov194_021c4cfc(&wk->infoIcons[2]);
        if (!isEgg) {
            func_ov194_021c0750(pkm, wk->summaryWindow, 8, 0, wk);
            func_ov194_021c0790(pkm, wk->summaryWindow, 16, 16, wk);
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Nature, wk->drawStr);
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->summaryWindow), 8, 80, wk->drawStr, wk->font);
            func_ov194_021c0848(pkm, wk->summaryWindow, 16, 96, wk);
            GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Ability, wk->drawStr);
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->summaryWindow), 8, 112, wk->drawStr, wk->font);
            func_ov194_021c08ac(pkm, wk->summaryWindow, 16, 128, wk);
        }
    }
    func_ov194_021c2ef0(wk, side, page);
    BmpWin_FlushMap(wk->summaryWindow);
    BmpWin_FlushChar(wk->summaryWindow);
    GFL_BGSysQueueScrLoad(6);
}

static void func_ov194_021c0e58(PokemonTradeWork *wk, PartyPkm *pkm, int side, BOOL reload) {
    func_ov194_021c24dc(wk, side);
    if (wk->summaryWindow != NULL) {
        BmpWin_Free(wk->summaryWindow);
    }
}

// The type icons, on the page that icons gives: the two to show, then the two to hide
static void func_ov194_021c0e74(PokemonTradeWork *wk, PartyPkm *pkm, int *icons) {
    func_ov194_021bfba4(&wk->typeIcons[icons[0]]);
    func_ov194_021bfba4(&wk->typeIcons[icons[1]]);
    func_ov194_021bfbc0(&wk->typeIcons[icons[0]], pkm, 0, wk->clactUnit, 176, 95, 0, wk->heapId);
    if (PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL) != PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL)) {
        func_ov194_021bfbc0(&wk->typeIcons[icons[1]], pkm, 1, wk->clactUnit, 210, 95, 0, wk->heapId);
    }
    if (wk->typeIcons[icons[2]].actor != NULL) {
        func_0204c124(wk->typeIcons[icons[2]].actor, FALSE);
    }
    if (wk->typeIcons[icons[3]].actor != NULL) {
        func_0204c124(wk->typeIcons[icons[3]].actor, FALSE);
    }
}

// Draws the type icons on the other page, and turns to it
static void func_ov194_021c0f44(PokemonTradeWork *wk, PartyPkm *pkm) {
    if (wk->typeIconPage) {
        int icons[] = { 0, 1, 2, 3 };
        func_ov194_021c0e74(wk, pkm, icons);
        wk->typeIconPage = 0;
    } else {
        int icons[] = { 2, 3, 0, 1 };
        func_ov194_021c0e74(wk, pkm, icons);
        wk->typeIconPage = 1;
    }
}

// The summary of a side's Pokémon on the upper screen, with its sprite
void func_ov194_021c0fa0(PokemonTradeWork *wk, PartyPkm *pkm, int side, BOOL reload) {
    int i;
    VecFx32 pos;
    BOOL isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    func_ov194_021c0e58(wk, pkm, side, reload);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1c0, 0x20, wk->heapId);
    if (wk->mcss[side] == NULL) {
        func_ov194_021c23a4(wk, side, 1, pkm, 0);
    } else if (reload) {
        func_ov194_021c23a4(wk, side, 1, pkm, 0);
    }
    pos.x = FX32_CONST(56);
    pos.y = FX32_CONST(4);
    pos.z = 0;
    MCSS_SetPosition(wk->mcss[side], &pos);
    MCSS_Show(wk->mcss[side]);
    if (wk->mcss[1 - side] != NULL) {
        MCSS_Hide(wk->mcss[1 - side]);
    }
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    wk->summaryWindow = BmpWin_CreateDynamic(3, 1, 0, 31, 24, 14, 0);
    func_ov194_021c0234(pkm, BmpWin_GetBitmap(wk->summaryWindow), 16, 0, isEgg, FALSE, wk);
    if (!isEgg) {
        func_ov194_021c02e0(pkm, BmpWin_GetBitmap(wk->summaryWindow), 0, 16, isEgg, wk);
        func_ov194_021c0398(pkm, BmpWin_GetBitmap(wk->summaryWindow), 56, 16, isEgg, wk);
        func_ov194_021c03fc(pkm, BmpWin_GetBitmap(wk->summaryWindow), 96, 0, wk);
        func_ov194_021c0474(pkm, BmpWin_GetBitmap(wk->summaryWindow), 80, 0, wk, FALSE, FALSE);
        func_ov194_021c051c(pkm, wk->summaryWindow, 0, 32, wk);
        for (i = 0; i < 2; i++) {
            GFL_MsgDataLoadStrbuf(wk->msgData, i + 36, wk->drawStr);
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->summaryWindow), 0, i * 16 + 136, wk->drawStr, wk->font);
        }
        func_ov194_021c05ec(pkm, wk->summaryWindow, 56, 32, wk);
        func_ov194_021c0684(pkm, wk->summaryWindow, 72, 48, wk);
        func_ov194_021c0848(pkm, wk->summaryWindow, 56, 136, wk);
        func_ov194_021c08ac(pkm, wk->summaryWindow, 56, 152, wk);
        func_ov194_021c057c(pkm, wk->summaryWindow, 40, 168, wk);
        func_ov194_021c0750(pkm, wk->summaryWindow, 152, 113, wk);
        func_ov194_021c0790(pkm, wk->summaryWindow, 152, 128, wk);
        func_ov194_021c0f44(wk, pkm);
        func_ov194_021c4c00(wk, 2, pkm);
        func_ov194_021bfc6c(&wk->ballIcons[0], pkm, wk->clactUnit, 12, 7, 0, wk->heapId, 12);
    } else {
        if (wk->infoIcons[0].actor != NULL) {
            func_0204c124(wk->infoIcons[0].actor, FALSE);
        }
        func_ov194_021c0acc(wk);
        func_ov194_021c0aac(wk);
    }
    func_ov194_021c4ec0(wk, pkm, isEgg);
    func_ov194_021c4d18(wk, 0, 1, pkm);
    BmpWin_FlushMap(wk->summaryWindow);
    BmpWin_FlushChar(wk->summaryWindow);
    GFL_BGSysQueueScrLoad(3);
}

// Closes a side's panel on the upper screen
void func_ov194_021c123c(PokemonTradeWork *wk, int side) {
    if (wk->unk5BC[side] != NULL) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->unk5BC[side]), 0);
        BmpWin_ClearScreen(wk->unk5BC[side]);
        BmpWin_Free(wk->unk5BC[side]);
        GFL_BGSysQueueScrLoad(3);
        wk->unk5BC[side] = NULL;
        func_ov194_021bfc50(&wk->ballIcons[side + 1]);
    }
}

// Closes the page of the summary
void func_ov194_021c1288(PokemonTradeWork *wk, BOOL clear) {
    func_ov194_021bfc50(&wk->ballIcons[0]);
    func_ov194_021c4cfc(&wk->infoIcons[0]);
    func_ov194_021c4cfc(&wk->infoIcons[1]);
    func_ov194_021c4cfc(&wk->infoIcons[2]);
    if (wk->summaryWindow != NULL) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->summaryWindow), 0);
        if (clear) {
            BmpWin_ClearScreen(wk->summaryWindow);
            BmpWin_FlushChar(wk->summaryWindow);
        }
        BmpWin_Free(wk->summaryWindow);
        wk->summaryWindow = NULL;
    }
}

// The panels of a negotiation: each player's name and the Pokémon they offer
void func_ov194_021c12ec(PokemonTradeWork *wk, u32 a1) {
    int i, j;
    PlayerInfo *infos[2];
    func_ov194_021c5abc(wk);
    infos[0] = wk->partnerInfo;
    infos[1] = wk->myInfo;
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    for (i = 0; i < 2; i++) {
        GFL_MsgDataLoadStrbuf(wk->msgData, AbilityHandlers3_Text_Empty_51, wk->drawTemplate);
        copyVarForText(wk->wordSet, 0, infos[i]);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->drawStr, wk->drawTemplate);
        GFL_TextRendererDrawToBitmap(wk->negoBitmaps[i * 4], 0, 0, wk->drawStr, wk->font);
    }
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    for (i = 0; i < 2; i++) {
        GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
        for (j = 0; j < 3; j++) {
            PartyPkm *pkm = wk->negoPkm[1 - i][j];
            if (wk->negoSlot[1 - i][j] != -1) {
                BOOL isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
                func_ov194_021c5138(wk, i, j, pkm, 0, 1);
                func_ov194_021c0234(pkm, wk->negoBitmaps[i * 4 + 1 + j], 0, 0, isEgg, func_ov194_021b783c(wk), wk);
                if (!isEgg) {
                    func_ov194_021c0474(pkm, wk->negoBitmaps[i * 4 + 1 + j], 56, 16, wk, FALSE,
                                        func_ov194_021b783c(wk));
                    func_ov194_021c03fc(pkm, wk->negoBitmaps[i * 4 + 1 + j], 0, 16, wk);
                }
            }
        }
    }
    func_ov194_021c5c80(wk);
}

void func_ov194_021c1484(PokemonTradeWork *wk) {
    int i;
    for (i = 0; i < 4; i++) {
        if (wk->unk1038[i] != NULL) {
            BmpWin_Free(wk->unk1038[i]);
            wk->unk1038[i] = NULL;
        }
    }
    func_ov194_021c5bf0(wk);
}

// Dims the upper screen, but for its windows
void func_ov194_021c14b0(PokemonTradeWork *wk) {
    gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR,
                             GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -8);
    G2S_SetWnd0InsidePlane(GX_PLANEMASK_ALL, TRUE);
    G2S_SetWnd0Position(0, 0, 255, 192);
    G2S_SetWnd1InsidePlane(GX_PLANEMASK_ALL, TRUE);
    G2S_SetWnd1Position(255, 0, 0, 192);
    G2S_SetWndOutsidePlane(GX_PLANEMASK_ALL, FALSE);
    GXS_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
}

// The small panel of a side's Pokémon on the upper screen
void func_ov194_021c1530(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    int windowX[] = { 1, 18 };
    int ballX[] = { 0, 136 };
    int colors[] = { 9, 8, 11, 10 };
    int ballPalette[] = { 12, 5 };
    BOOL isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    if (wk->unk5C4[side] != NULL) {
        BmpWin_Free(wk->unk5C4[side]);
        BmpWin_Free(wk->unk5CC[side]);
    }
    wk->unk5C4[side] = BmpWin_CreateDynamic(1, windowX[side], 0, 14, 5, 5, 0);
    wk->unk5CC[side] = BmpWin_CreateDynamic(1, windowX[side], 17, 13, 2, 5, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->unk5C4[side]), colors[side * 2] | (colors[side * 2] << 4));
    GFL_BitmapFill(BmpWin_GetBitmap(wk->unk5CC[side]), colors[side * 2 + 1] | (colors[side * 2 + 1] << 4));
    GFL_TextRndUpdateColorIndexLUT(1, 2, colors[side * 2]);
    func_ov194_021c0234(pkm, BmpWin_GetBitmap(wk->unk5C4[side]), 16, 4, isEgg, func_ov194_021b783c(wk), wk);
    if (!isEgg) {
        func_ov194_021c0474(pkm, BmpWin_GetBitmap(wk->unk5C4[side]), 80, 4, wk, TRUE, func_ov194_021b783c(wk));
        func_ov194_021c03fc(pkm, BmpWin_GetBitmap(wk->unk5C4[side]), 16, 19, wk);
        GFL_TextRndUpdateColorIndexLUT(1, 2, colors[side * 2 + 1]);
        func_ov194_021c057c(pkm, wk->unk5CC[side], 0, 0, wk);
        func_ov194_021bfc6c(&wk->ballIcons[side + 1], pkm, wk->clactUnit, ballX[side] + 16, 16, 0, wk->heapId,
                            ballPalette[side]);
    } else {
        func_ov194_021c0aac(wk);
    }
    BmpWin_FlushChar(wk->unk5C4[side]);
    BmpWin_FlushChar(wk->unk5CC[side]);
    BmpWin_FlushMap(wk->unk5C4[side]);
    BmpWin_FlushMap(wk->unk5CC[side]);
    GFL_BGSysQueueScrLoad(1);
    wk->unk11E2[side] = 120;
}

void func_ov194_021c1740(PokemonTradeWork *wk, int side) {
    if (wk->unk5C4[side] != NULL) {
        BmpWin_Free(wk->unk5C4[side]);
        BmpWin_Free(wk->unk5CC[side]);
        func_ov194_021bfc50(&wk->ballIcons[side + 1]);
        wk->unk5C4[side] = NULL;
        wk->unk5CC[side] = NULL;
    }
}

// A button of the message msg at the bottom of the lower screen, in place of the touch bar's
AppTaskMenuWin *func_ov194_021c1788(PokemonTradeWork *wk, u32 msg) {
    wk->menuItems[0].str = GFL_StrBufCreate(100, wk->heapId);
    GFL_MsgDataLoadStrbuf(wk->msgData, msg, wk->menuItems[0].str);
    wk->menuItems[0].color = PRINT_COLOR(14, 15, 3);
    wk->menuItems[0].type = 0;
    wk->menuWin = AppTaskMenuWin_CreateEx(wk->taskMenuRes, &wk->menuItems[0], 8, 21, 16, 3, 0, 1, wk->heapId);
    GFL_StrBufFree(wk->menuItems[0].str);
    func_0204c124(wk->actors[2], FALSE);
    TouchBar_SetIconVisible(wk->touchBar, 8, FALSE);
    return wk->menuWin;
}

// Closes the button, showing the touch bar's again
void func_ov194_021c1820(PokemonTradeWork *wk, BOOL showActor, BOOL showButton) {
    if (!func_02021c0c(wk->printQueue)) {
        func_02021c44(wk->printQueue);
    }
    if (showActor) {
        func_0204c124(wk->actors[2], TRUE);
    }
    if (showButton) {
        TouchBar_SetIconVisible(wk->touchBar, 8, func_ov194_021bc098(wk));
    }
    if (wk->menuWin != NULL) {
        AppTaskMenuWin_Free(wk->menuWin);
        wk->menuWin = NULL;
        GFL_BGSysQueueScrLoad(6);
    }
}

// Whether the button was touched
BOOL func_ov194_021c1884(PokemonTradeWork *wk) {
    static const TouchRect sButtonRect[] = {
        { 160, 192, 64, 192 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };
    if (func_0203da0c(sButtonRect) == 0) {
        return TRUE;
    }
    return FALSE;
}

// Sets the windows of the upper screen once a frame has passed
static void func_ov194_021c189c(TCB *tcb, void *work) {
    PokemonTradeWork *wk = work;
    G2S_SetWnd1InsidePlane(GX_PLANEMASK_OBJ, FALSE);
    G2S_SetWnd1Position(48, 0, 208, 32);
    G2S_SetWnd0InsidePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_OBJ, FALSE);
    G2S_SetWnd0Position(224, 169, 255, 192);
    G2S_SetWndOutsidePlane(GX_PLANEMASK_ALL, TRUE);
    GXS_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    wk->unk11F9 = 0;
    GFL_TCBRemove(tcb);
}
