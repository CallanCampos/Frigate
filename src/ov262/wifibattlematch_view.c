#include "types.h"
#include "app/livebattlematch_flow.h"
#include "app/wifibattlematch_view.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/gfd.h"
#include "field/unity_tower.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/records.h"
#include "save/tournament_block.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The backdrop of the Wi-Fi and live competition screens and what they show on it: the digital cards of the players,
// the other player's card, the icon of the link and the list of the team (the file name is the ROM's own string)

// The archive of the screens' graphics. It has no name yet
#define ARCID_WIFIBATTLEMATCH 134

#define CARD_WINDOW_MAX 30
#define CARD_ACTOR_COUNT 22
#define CARD_PARTY_COUNT 6
#define CARD_STAR_MAX 5
// The frames that a card takes to slide in or out
#define SLIDE_FRAMES 20

// The cell actors of a card, by index: the Pokémon, the player's badge and mark, the stars of the rank, and the held
// items
#define CARD_ACTOR_BADGE 7
#define CARD_ACTOR_MARK 8
#define CARD_ACTOR_STAR 9
#define CARD_ACTOR_ITEM 15

// The resources of the backdrop, by index: its palettes, characters and cells, of the main screen and then of the sub
// screen
#define RES_PALETTE 0
#define RES_CHARS 2
#define RES_CELLS 4

#define TEAM_ACTOR_COUNT 14
#define TEAM_ACTOR_ITEM 7

#define PARTNER_WINDOW_COUNT 8
#define PARTNER_WINDOW_PMS 7

struct WifiBattleMatchRes {
    u32 res[6];
    // The palette as it is now, and the two palettes that it goes between
    u16 colors[16];
    u16 colorsA[16];
    u16 colorsB[16];
    u16 angle;
};

struct WifiBattleMatchCard {
    u32 windowCount;
    BmpWin *windows[CARD_WINDOW_MAX];
    PrintWindow printWindows[CARD_WINDOW_MAX];
    ClActor *actors[CARD_ACTOR_COUNT];
    u32 iconChars[CARD_PARTY_COUNT];
    u32 unk1DC;
    u32 iconCells;
    u32 iconPalette;
    u32 badgePalette;
    u32 badgeChars;
    u32 badgeCells;
    u32 itemPalette;
    u32 itemChars;
    u32 itemCells;
    u32 slideCount;
    s16 slideX[CARD_ACTOR_COUNT];
    // The BG of the card, which the one after it is the frame of, and its palettes
    u8 bg;
    u8 palette;
    u8 iconPaletteSlot;
    u8 itemPaletteSlot;
    // CLACT_VRAM_MAIN or CLACT_VRAM_SUB
    u32 surface;
    void *records;
    u16 heapId;
    ClActUnit *unit;
};

struct WifiBattleMatchPartnerCard {
    BOOL shown;
    BmpWin *windows[PARTNER_WINDOW_COUNT];
    PrintWindow printWindows[PARTNER_WINDOW_COUNT];
    ClActor *badge;
    ClActor *stars[CARD_STAR_MAX];
    PMSDraw *pmsDraw;
    u16 badgePalette;
    u16 badgeChars;
    u16 badgeCells;
    u32 slideCount;
    u16 badgeX;
    u16 starX[CARD_STAR_MAX];
    u32 kind;
};

struct WifiBattleMatchLinkIcon {
    ClActor *actors[2];
};

struct WifiBattleMatchTeamList {
    u32 palette;
    u32 chars[CARD_PARTY_COUNT];
    u32 unk1C;
    u32 cells;
    u32 itemPalette;
    u32 itemChars;
    u32 itemCells;
    ClActor *actors[TEAM_ACTOR_COUNT];
    BmpWin *windows[CARD_PARTY_COUNT];
    PrintWindow printWindows[CARD_PARTY_COUNT];
    u32 slideCount;
    s16 slideX[TEAM_ACTOR_COUNT];
};

static u32 WifiBattleMatchView_GetRes(WifiBattleMatchRes *res, u32 index);
static void WifiBattleMatchView_BlendColor(NNS_GFD_DST_TYPE type, u16 *dst, const u16 *angle, u32 a3, u8 row,
                                           u8 index, u16 colorA, u16 colorB);
static void WifiBattleMatchView_PrintCounts(PrintWindow *printWindow, PrintQueue *queue, Font *font, u32 count1,
                                            u32 count2, HeapID heapId);
static void WifiBattleMatchView_RankCardWindows(WifiBattleMatchCard *card, u32 titleMsg,
                                                const WifiBattleMatchRankInfo *info, PlayerInfo *playerInfo,
                                                Font *font, PrintQueue *printQueue, MsgData *msgData,
                                                WordSet *wordSet, u32 colorKind, HeapID heapId);
static void WifiBattleMatchView_WifiCardWindows(WifiBattleMatchCard *card, u32 ratio,
                                                const WifiBattleMatchWifiCardInfo *info, PlayerInfo *playerInfo,
                                                Font *font, PrintQueue *printQueue, MsgData *msgData,
                                                WordSet *wordSet, HeapID heapId);
static void WifiBattleMatchView_LiveCardWindows(WifiBattleMatchCard *card, const WifiBattleMatchLiveCardInfo *info,
                                                PlayerInfo *playerInfo, Font *font, PrintQueue *printQueue,
                                                MsgData *msgData, WordSet *wordSet, HeapID heapId);
static BOOL WifiBattleMatchView_CardFlush(WifiBattleMatchCard *card, PrintQueue *printQueue);
static void WifiBattleMatchView_CardFreeWindows(WifiBattleMatchCard *card);
static void WifiBattleMatchView_CardLoadIcons(WifiBattleMatchCard *card, ClActUnit *unit, void *records,
                                              HeapID heapId);
static void WifiBattleMatchView_CardFreeIcons(WifiBattleMatchCard *card);
static void WifiBattleMatchView_CardLoadBadge(WifiBattleMatchCard *card, u32 look, ClActUnit *unit, HeapID heapId);
static void WifiBattleMatchView_CardFreeBadge(WifiBattleMatchCard *card);
static void WifiBattleMatchView_CardLoadMark(WifiBattleMatchCard *card, ClActUnit *unit, WifiBattleMatchRes *res,
                                             HeapID heapId);
static void WifiBattleMatchView_CardFreeMark(WifiBattleMatchCard *card);
static void WifiBattleMatchView_CardLoadStars(WifiBattleMatchCard *card, const WifiBattleMatchRankInfo *info,
                                              ClActUnit *unit, WifiBattleMatchRes *res, HeapID heapId);
static void WifiBattleMatchView_CardFreeStars(WifiBattleMatchCard *card);
static void WifiBattleMatchView_PartnerWindows(WifiBattleMatchPartnerCard *card,
                                               const LiveBattleMatchPlayerData *data, Font *font,
                                               PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet,
                                               u32 colorKind, HeapID heapId);
static BOOL WifiBattleMatchView_PartnerFlush(WifiBattleMatchPartnerCard *card, PrintQueue *printQueue);
static void WifiBattleMatchView_PartnerFreeWindows(WifiBattleMatchPartnerCard *card);
static void WifiBattleMatchView_PartnerLoadBadge(WifiBattleMatchPartnerCard *card, u32 look, ClActUnit *unit,
                                                 HeapID heapId);
static void WifiBattleMatchView_PartnerFreeBadge(WifiBattleMatchPartnerCard *card);
static void WifiBattleMatchView_PartnerLoadStars(WifiBattleMatchPartnerCard *card,
                                                 const LiveBattleMatchPlayerData *data, ClActUnit *unit,
                                                 WifiBattleMatchRes *res, HeapID heapId);
static void WifiBattleMatchView_PartnerFreeStars(WifiBattleMatchPartnerCard *card);
static u32 WifiBattleMatchView_GetStarCount(u32 unk0, u32 count, u32 unk2);

// The first palette of the badge's colors that each look uses
static const u8 sBadgePaletteStarts[16] = {
    2, 3, 6, 5, 4, 5, 2, 0, 1, 3, 6, 5, 4, 7, 7, 0,
};

// The windows of the Wi-Fi competition's card, of the other player's card, of the live one's card and of the rank
// card, each with its x, y, width and height
static const u8 sPartnerCardRects[8][4] = {
    { 8, 5, 8, 2 },  { 16, 5, 9, 2 }, { 25, 5, 5, 2 },  { 2, 9, 28, 2 },
    { 2, 11, 28, 2 }, { 2, 13, 28, 2 }, { 2, 16, 28, 2 }, { 2, 18, 28, 4 },
};

static const u8 sWifiCardRects[8][4] = {
    { 2, 1, 28, 4 },  { 2, 6, 28, 2 },  { 2, 9, 9, 2 },   { 11, 9, 13, 2 },
    { 25, 9, 5, 2 }, { 9, 12, 15, 2 }, { 24, 12, 6, 2 }, { 5, 18, 28, 2 },
};

static const u8 sLiveCardRects[10][4] = {
    { 2, 1, 28, 4 },  { 2, 6, 28, 2 },  { 2, 9, 9, 2 },   { 10, 9, 12, 2 }, { 25, 9, 5, 2 },
    { 10, 12, 12, 2 }, { 25, 12, 5, 2 }, { 8, 15, 14, 2 }, { 25, 15, 5, 2 }, { 5, 18, 28, 2 },
};

static const u8 sRankCardRects[12][4] = {
    { 2, 4, 26, 2 },  { 9, 7, 21, 2 },  { 9, 9, 21, 2 },  { 2, 12, 20, 2 },  { 25, 12, 5, 2 },  { 2, 15, 20, 2 },
    { 23, 15, 7, 2 }, { 2, 18, 20, 2 }, { 25, 18, 5, 2 }, { 2, 21, 20, 2 }, { 25, 21, 5, 2 }, { 0, 0, 12, 19 },
};

WifiBattleMatchRes *WifiBattleMatchRes_Create(ClActUnit *unit, u32 kind, HeapID heapId) {
    WifiBattleMatchRes *res = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchRes), FALSE, "wifibattlematch_view.c", 125);
    ArcTool *arc;
    NNSG2dPaletteData *palette;
    void *file;
    u8 *rawData;

    sys_memset(res, 0, sizeof(WifiBattleMatchRes));
    arc = GFL_ArcSysCreateFileHandle(ARCID_WIFIBATTLEMATCH, HEAPID_TAIL(heapId));
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, 0, 0, HEAPID_TAIL(heapId));
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_SUB_BG, 0, 0, HEAPID_TAIL(heapId));
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 3, 0, 0, FALSE, HEAPID_TAIL(heapId));
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 7, 0, 0, FALSE, HEAPID_TAIL(heapId));
    WifiBattleMatchRes_LoadBG(res, kind, heapId);
    res->res[RES_PALETTE + CLACT_VRAM_MAIN] = func_0204bba0(arc, 1, CLACT_VRAM_MAIN, 0, HEAPID_TAIL(heapId));
    res->res[RES_CHARS + CLACT_VRAM_MAIN] = func_0204b81c(arc, 3, FALSE, CLACT_VRAM_MAIN, HEAPID_TAIL(heapId));
    res->res[RES_CELLS + CLACT_VRAM_MAIN] = func_0204bde0(arc, 4, 11, HEAPID_TAIL(heapId));
    res->res[RES_PALETTE + CLACT_VRAM_SUB] = func_0204bba0(arc, 1, CLACT_VRAM_SUB, 0, HEAPID_TAIL(heapId));
    res->res[RES_CHARS + CLACT_VRAM_SUB] = func_0204b81c(arc, 3, FALSE, CLACT_VRAM_SUB, HEAPID_TAIL(heapId));
    res->res[RES_CELLS + CLACT_VRAM_SUB] = func_0204bde0(arc, 4, 11, HEAPID_TAIL(heapId));
    file = GFL_G2DIOReadNCLRArc(arc, 0, &palette, heapId);
    rawData = palette->rawData;
    sys_memcpy(rawData, res->colorsA, sizeof(res->colorsA));
    sys_memcpy(rawData + 0x120, res->colorsB, sizeof(res->colorsB));
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x1e0, 0x20, HEAPID_TAIL(heapId));
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x1e0, 0x20, HEAPID_TAIL(heapId));
    GFL_BGSysFillChar(0, 0, 1, 0);
    LoadSysMsgBox(0, 1, 13, 0, HEAPID_TAIL(heapId));
    LoadSysMsgBox(0, 10, 14, 0, HEAPID_TAIL(heapId));
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, -256);
    return res;
}

void WifiBattleMatchRes_Free(WifiBattleMatchRes *res) {
    func_0204bcd0(res->res[RES_PALETTE + CLACT_VRAM_MAIN]);
    func_0204b98c(res->res[RES_CHARS + CLACT_VRAM_MAIN]);
    func_0204be64(res->res[RES_CELLS + CLACT_VRAM_MAIN]);
    func_0204bcd0(res->res[RES_PALETTE + CLACT_VRAM_SUB]);
    func_0204b98c(res->res[RES_CHARS + CLACT_VRAM_SUB]);
    func_0204be64(res->res[RES_CELLS + CLACT_VRAM_SUB]);
    GFL_BGSysFreeFilledChar(0, 1, 0);
    GFL_HeapFree(res);
}

void WifiBattleMatchRes_LoadBG(WifiBattleMatchRes *res, u32 kind, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WIFIBATTLEMATCH, HEAPID_TAIL(heapId));

    loadBGScrToVramByFileNoReserveNegAlign(arc, 5, 3, 0, 0, FALSE, HEAPID_TAIL(heapId));
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 7, 0, 0, FALSE, HEAPID_TAIL(heapId));
    switch (kind) {
    case 2:
        loadBGScrToVramByFileNoReserveNegAlign(arc, 8, 6, 0, 0, FALSE, HEAPID_TAIL(heapId));
        break;
    case 3:
        loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 2, 0, 0, FALSE, HEAPID_TAIL(heapId));
        break;
    case 0:
    case 1:
        loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 6, 0, 0, FALSE, HEAPID_TAIL(heapId));
        break;
    }
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, -256);
    GFL_ArcToolFree(arc);
}

static u32 WifiBattleMatchView_GetRes(WifiBattleMatchRes *res, u32 index) {
    return res->res[index];
}

void WifiBattleMatchRes_Update(WifiBattleMatchRes *res) {
    // The palette rows that are blended: the first of the BGs and the third of the main screen's OBJs
    static const u8 sRows[2] = { 0, 2 };
    int i;

    if (res->angle + 0x200 >= 0x10000) {
        res->angle = res->angle - 0xfe00;
    } else {
        res->angle += 0x200;
    }
    for (i = 0; i < 16; i++) {
        WifiBattleMatchView_BlendColor(NNS_GFD_DST_2D_BG_PLTT_MAIN, &res->colors[i], &res->angle, 0, sRows[0], i,
                                       res->colorsA[i], res->colorsB[i]);
        WifiBattleMatchView_BlendColor(NNS_GFD_DST_2D_BG_PLTT_SUB, &res->colors[i], &res->angle, 0, sRows[0], i,
                                       res->colorsA[i], res->colorsB[i]);
        WifiBattleMatchView_BlendColor(NNS_GFD_DST_2D_OBJ_PLTT_MAIN, &res->colors[i], &res->angle, 0, sRows[1], i,
                                       res->colorsA[i], res->colorsB[i]);
    }
}

// Blends two colors by the angle, and queues the result for a palette
static void WifiBattleMatchView_BlendColor(NNS_GFD_DST_TYPE type, u16 *dst, const u16 *angle, u32 a3, u8 row,
                                           u8 index, u16 colorA, u16 colorB) {
    fx16 ratio = (FX_CosIdx(*angle) + FX32_ONE) / 2;
    u8 redA = colorA & 0x1f;
    u8 greenA = (colorA & 0x3e0) >> 5;
    u8 blueA = (colorA & 0x7c00) >> 10;
    u8 redB = colorB & 0x1f;
    u8 greenB = (colorB & 0x3e0) >> 5;
    u8 blueB = (colorB & 0x7c00) >> 10;
    u8 red = redA + ((redB - redA) * ratio >> 12);
    u8 green = greenA + ((greenB - greenA) * ratio >> 12);
    u8 blue = blueA + ((blueB - blueA) * ratio >> 12);

    *dst = red | (green << 5) | (blue << 10);
    NNS_GfdRegisterNewVramTransferTask(type, row * 0x20 + index * 2, dst, 2);
}

// Prints two numbers with a message between them, ending at the right of the window
static void WifiBattleMatchView_PrintCounts(PrintWindow *printWindow, PrintQueue *queue, Font *font, u32 count1,
                                            u32 count2, HeapID heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_12, heapId);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    StrBuf *text = GFL_StrBufCreate(0x20, heapId);
    u16 right = BmpWin_GetSizeX(printWindow->window) * 8;
    // Where the second number and the message between the numbers start
    int count2X;
    int middleX;
    u16 width;
    StrBuf *format;

    format = GFL_MsgDataLoadStrbufNew(msgData, 2);
    WordSetNumber(wordSet, 0, count2, 8, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, text, format);
    width = GFL_FontGetBlockWidth(text, font, 0);
    count2X = right - width;
    PrintWindow_PrintNoColor(printWindow, queue, count2X, 0, text, font);
    GFL_StrBufFree(format);

    format = GFL_MsgDataLoadStrbufNew(msgData, 0);
    width = GFL_FontGetBlockWidth(format, font, 0);
    middleX = count2X - width;
    PrintWindow_PrintNoColor(printWindow, queue, middleX, 0, format, font);
    GFL_StrBufFree(format);

    format = GFL_MsgDataLoadStrbufNew(msgData, 1);
    WordSetNumber(wordSet, 0, count1, 8, 0, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, text, format);
    width = GFL_FontGetBlockWidth(text, font, 0);
    PrintWindow_PrintNoColor(printWindow, queue, middleX - width, 0, text, font);
    GFL_StrBufFree(format);
    GFL_StrBufFree(text);
    GFL_WordSetSystemFree(wordSet);
    GFL_MsgDataFree(msgData);
}

WifiBattleMatchCard *WifiBattleMatchRankCard_Create(const WifiBattleMatchRankInfo *info, u32 titleMsg, PlayerInfo *playerInfo,
                                         ClActUnit *unit, WifiBattleMatchRes *res, Font *font,
                                         PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, HeapID heapId) {
    WifiBattleMatchCard *card = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchCard), FALSE, "wifibattlematch_view.c",
                                                 553);
    ClActorPos pos;
    u32 stars;
    u32 colorKind;

    sys_memset(card, 0, sizeof(WifiBattleMatchCard));
    card->bg = 5;
    card->palette = 15;
    card->iconPaletteSlot = 3;
    card->itemPaletteSlot = 7;
    card->surface = CLACT_VRAM_SUB;
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    colorKind = 1;
    stars = WifiBattleMatchView_GetStarCount(info->unkC, info->unk10, info->unk12);
    if (stars != 5) {
        colorKind = 0;
    }
    WifiBattleMatchView_RankCardWindows(card, titleMsg, info, playerInfo, font, printQueue, msgData, wordSet,
                                        colorKind, heapId);
    WifiBattleMatchView_CardLoadBadge(card, info->look, unit, heapId);
    pos.x = 0x120;
    pos.y = 0x44;
    func_0204c140(card->actors[CARD_ACTOR_BADGE], &pos, CLACT_VRAM_SUB);
    WifiBattleMatchView_CardLoadStars(card, info, unit, res, heapId);
    GFL_BGSysSetScrPaletteNo(6, 0, 0, 32, 6, 1);
    GFL_BGSysSetScrPaletteNo(6, 0, 6, 32, 18, stars + 3);
    GFL_BGSysLoadScr(6);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    return card;
}

void WifiBattleMatchRankCard_Free(WifiBattleMatchCard *card) {
    G2S_BlendNone();
    WifiBattleMatchView_CardFreeStars(card);
    WifiBattleMatchView_CardFreeBadge(card);
    WifiBattleMatchView_CardFreeWindows(card);
    GFL_HeapFree(card);
}

WifiBattleMatchCard *WifiBattleMatchWifiCard_Create(const WifiBattleMatchWifiCardInfo *info, u32 ratio, PlayerInfo *playerInfo,
                                         ClActUnit *unit, WifiBattleMatchRes *res, Font *font,
                                         PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, void *records,
                                         BOOL main, HeapID heapId) {
    WifiBattleMatchCard *card = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchCard), FALSE, "wifibattlematch_view.c",
                                                 639);
    ClActorPos pos;
    u32 frameBG;

    sys_memset(card, 0, sizeof(WifiBattleMatchCard));
    card->records = records;
    card->heapId = heapId;
    card->unit = unit;
    if (main) {
        frameBG = 2;
        card->bg = 1;
        card->palette = 15;
        card->iconPaletteSlot = 5;
        card->itemPaletteSlot = 9;
        card->surface = CLACT_VRAM_MAIN;
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    } else {
        frameBG = 6;
        card->bg = 5;
        card->palette = 15;
        card->iconPaletteSlot = 3;
        card->itemPaletteSlot = 7;
        card->surface = CLACT_VRAM_SUB;
        gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    }
    switch (info->variant) {
    case 0:
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 0, 32, 5, 1);
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 5, 32, 18, 3);
        GFL_BGSysLoadScr(frameBG);
        break;
    case 1:
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 0, 32, 5, 1);
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 5, 32, 18, 7);
        GFL_BGSysLoadScr(frameBG);
        break;
    }
    WifiBattleMatchView_WifiCardWindows(card, ratio, info, playerInfo, font, printQueue, msgData, wordSet, heapId);
    WifiBattleMatchView_CardLoadIcons(card, unit, records, heapId);
    WifiBattleMatchView_CardLoadBadge(card, info->look, unit, heapId);
    pos.x = 0x120;
    pos.y = 0x6e;
    func_0204c140(card->actors[CARD_ACTOR_BADGE], &pos, card->surface);
    WifiBattleMatchView_CardLoadMark(card, unit, res, heapId);
    return card;
}

void WifiBattleMatchWifiCard_Free(WifiBattleMatchCard *card) {
    G2S_BlendNone();
    WifiBattleMatchView_CardFreeMark(card);
    WifiBattleMatchView_CardFreeBadge(card);
    WifiBattleMatchView_CardFreeIcons(card);
    WifiBattleMatchView_CardFreeWindows(card);
    GFL_HeapFree(card);
}


void WifiBattleMatchWifiCard_ShowPage(WifiBattleMatchCard *card, u32 page, MsgData *msgData, PrintQueue *printQueue, Font *font,
                         HeapID heapId) {
#ifdef BUGFIX
    StrBuf *text = NULL;
#else
    StrBuf *text;
#endif
    ClActorPos pos;
    s32 x;
    int i;

    GFL_BitmapFill(BmpWin_GetBitmap(card->windows[7]), 0);
    WifiBattleMatchView_CardFreeIcons(card);
    WifiBattleMatchView_CardLoadIcons(card, card->unit, card->records, card->heapId);
    x = 32 - GFL_BGSysGetBGOffsetX(card->bg);
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        pos.y = 168;
        pos.x = x + i * 32;
        if (card->actors[i] != NULL) {
            func_0204c140(card->actors[i], &pos, card->surface);
        }
        if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
            pos.x += 12;
            pos.y += 8;
            func_0204c140(card->actors[CARD_ACTOR_ITEM + i], &pos, card->surface);
        }
    }
    switch (page) {
    case 0:
        for (i = 0; i < CARD_PARTY_COUNT; i++) {
            if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
                func_0204c124(card->actors[CARD_ACTOR_ITEM + i], TRUE);
            }
            if (card->actors[i] != NULL) {
                func_0204c124(card->actors[i], TRUE);
            }
        }
        func_0204c124(card->actors[CARD_ACTOR_MARK], TRUE);
        text = GFL_MsgDataLoadStrbufNew(msgData, 17);
        PrintWindow_Print(&card->printWindows[7], printQueue, 0, 0, text, font, PRINT_COLOR(15, 2, 0));
        break;
    case 1:
        for (i = 0; i < CARD_PARTY_COUNT; i++) {
            if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
                func_0204c124(card->actors[CARD_ACTOR_ITEM + i], TRUE);
            }
            if (card->actors[i] != NULL) {
                func_0204c124(card->actors[i], TRUE);
            }
        }
        func_0204c124(card->actors[CARD_ACTOR_MARK], FALSE);
        text = GFL_MsgDataLoadStrbufNew(msgData, 18);
        PrintWindow_Print(&card->printWindows[7], printQueue, 0, 0, text, font, PRINT_COLOR(15, 2, 0));
        break;
    case 2:
        for (i = 0; i < CARD_PARTY_COUNT; i++) {
            if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
                func_0204c124(card->actors[CARD_ACTOR_ITEM + i], FALSE);
            }
            if (card->actors[i] != NULL) {
                func_0204c124(card->actors[i], FALSE);
            }
        }
        func_0204c124(card->actors[CARD_ACTOR_MARK], FALSE);
        text = GFL_MsgDataLoadStrbufNew(msgData, 19);
        PrintWindow_Print(&card->printWindows[7], printQueue, 0, 0, text, font, PRINT_COLOR(15, 2, 0));
        break;
    }
#ifdef BUGFIX
    if (text != NULL) {
        GFL_StrBufFree(text);
    }
#else
    // BUG: text is not set when the page is not 0 to 2
    GFL_StrBufFree(text);
#endif
}

WifiBattleMatchCard *WifiBattleMatchLiveCard_Create(const WifiBattleMatchLiveCardInfo *info, PlayerInfo *playerInfo,
                                         ClActUnit *unit, WifiBattleMatchRes *res, Font *font,
                                         PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, void *records,
                                         BOOL main, HeapID heapId) {
    WifiBattleMatchCard *card = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchCard), FALSE, "wifibattlematch_view.c",
                                                 852);
    ClActorPos pos;
    u32 frameBG;

    sys_memset(card, 0, sizeof(WifiBattleMatchCard));
    card->records = records;
    card->heapId = heapId;
    card->unit = unit;
    if (main) {
        frameBG = 2;
        card->bg = 1;
        card->palette = 15;
        card->iconPaletteSlot = 5;
        card->itemPaletteSlot = 9;
        card->surface = CLACT_VRAM_MAIN;
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    } else {
        frameBG = 6;
        card->bg = 5;
        card->palette = 15;
        card->iconPaletteSlot = 3;
        card->itemPaletteSlot = 7;
        card->surface = CLACT_VRAM_SUB;
        gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    }
    switch (info->variant) {
    case 0:
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 0, 32, 5, 1);
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 5, 32, 18, 3);
        GFL_BGSysLoadScr(frameBG);
        break;
    case 1:
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 0, 32, 5, 1);
        GFL_BGSysSetScrPaletteNo(frameBG, 0, 5, 32, 18, 7);
        GFL_BGSysLoadScr(frameBG);
        break;
    }
    WifiBattleMatchView_LiveCardWindows(card, info, playerInfo, font, printQueue, msgData, wordSet, heapId);
    WifiBattleMatchView_CardLoadIcons(card, unit, records, heapId);
    WifiBattleMatchView_CardLoadBadge(card, info->look, unit, heapId);
    pos.x = 0x120;
    pos.y = 0x6e;
    func_0204c140(card->actors[CARD_ACTOR_BADGE], &pos, card->surface);
    WifiBattleMatchView_CardLoadMark(card, unit, res, heapId);
    return card;
}

void WifiBattleMatchLiveCard_Free(WifiBattleMatchCard *card) {
    G2S_BlendNone();
    WifiBattleMatchView_CardFreeMark(card);
    WifiBattleMatchView_CardFreeBadge(card);
    WifiBattleMatchView_CardFreeIcons(card);
    WifiBattleMatchView_CardFreeWindows(card);
    GFL_HeapFree(card);
}

void WifiBattleMatchLiveCard_ShowPage(WifiBattleMatchCard *card, u32 page, MsgData *msgData, PrintQueue *printQueue, Font *font,
                         HeapID heapId) {
#ifdef BUGFIX
    StrBuf *text = NULL;
#else
    StrBuf *text;
#endif
    ClActorPos pos;
    s32 x;
    int i;

    GFL_BitmapFill(BmpWin_GetBitmap(card->windows[9]), 0);
    WifiBattleMatchView_CardFreeIcons(card);
    WifiBattleMatchView_CardLoadIcons(card, card->unit, card->records, card->heapId);
    x = 32 - GFL_BGSysGetBGOffsetX(card->bg);
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        pos.y = 168;
        pos.x = x + i * 32;
        if (card->actors[i] != NULL) {
            func_0204c140(card->actors[i], &pos, card->surface);
        }
        if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
            pos.x += 12;
            pos.y += 8;
            func_0204c140(card->actors[CARD_ACTOR_ITEM + i], &pos, card->surface);
        }
    }
    switch (page) {
    case 0:
        for (i = 0; i < CARD_PARTY_COUNT; i++) {
            if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
                func_0204c124(card->actors[CARD_ACTOR_ITEM + i], TRUE);
            }
            if (card->actors[i] != NULL) {
                func_0204c124(card->actors[i], TRUE);
            }
        }
        func_0204c124(card->actors[CARD_ACTOR_MARK], TRUE);
        text = GFL_MsgDataLoadStrbufNew(msgData, 17);
        PrintWindow_Print(&card->printWindows[9], printQueue, 0, 0, text, font, PRINT_COLOR(15, 2, 0));
        break;
    case 1:
        for (i = 0; i < CARD_PARTY_COUNT; i++) {
            if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
                func_0204c124(card->actors[CARD_ACTOR_ITEM + i], TRUE);
            }
            if (card->actors[i] != NULL) {
                func_0204c124(card->actors[i], TRUE);
            }
        }
        func_0204c124(card->actors[CARD_ACTOR_MARK], FALSE);
        text = GFL_MsgDataLoadStrbufNew(msgData, 18);
        PrintWindow_Print(&card->printWindows[9], printQueue, 0, 0, text, font, PRINT_COLOR(15, 2, 0));
        break;
    case 2:
        for (i = 0; i < CARD_PARTY_COUNT; i++) {
            if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
                func_0204c124(card->actors[CARD_ACTOR_ITEM + i], FALSE);
            }
            if (card->actors[i] != NULL) {
                func_0204c124(card->actors[i], FALSE);
            }
        }
        func_0204c124(card->actors[CARD_ACTOR_MARK], FALSE);
        text = GFL_MsgDataLoadStrbufNew(msgData, 19);
        PrintWindow_Print(&card->printWindows[9], printQueue, 0, 0, text, font, PRINT_COLOR(15, 2, 0));
        break;
    }
#ifdef BUGFIX
    if (text != NULL) {
        GFL_StrBufFree(text);
    }
#else
    // BUG: text is not set when the page is not 0 to 2
    GFL_StrBufFree(text);
#endif
}

BOOL WifiBattleMatchCard_Flush(WifiBattleMatchCard *card, PrintQueue *printQueue) {
    return WifiBattleMatchView_CardFlush(card, printQueue);
}

BOOL WifiBattleMatchCard_SlideIn(WifiBattleMatchCard *card) {
    BOOL done;
    s32 offset;
    ClActorPos start;
    ClActorPos pos;
    int i;

    if (GFL_BGSysGetBGOffsetX(card->bg) == 0) {
        return TRUE;
    }
    done = FALSE;
    if (card->slideCount == 0) {
        for (i = 0; i < CARD_ACTOR_COUNT; i++) {
            if (card->actors[i] != NULL) {
                func_0204c178(card->actors[i], &start, card->surface);
                card->slideX[i] = start.x;
            }
        }
        GFL_SndSEPlay(SEQ_SE_SYS_75);
    }
    offset = (card->slideCount << 8) / SLIDE_FRAMES - 256;
    for (i = 0; i < CARD_ACTOR_COUNT; i++) {
        if (card->actors[i] != NULL) {
            func_0204c178(card->actors[i], &pos, card->surface);
            pos.x = card->slideX[i] - (card->slideCount << 8) / SLIDE_FRAMES;
            func_0204c140(card->actors[i], &pos, card->surface);
        }
    }
    if (card->slideCount++ >= SLIDE_FRAMES) {
        offset = 0;
        card->slideCount = 0;
        done = TRUE;
    }
    GFL_BGSysMoveBGReq(card->bg + 1, BG_MOVE_SET_X, offset);
    GFL_BGSysMoveBGReq(card->bg, BG_MOVE_SET_X, offset);
    return done;
}

BOOL WifiBattleMatchCard_SlideOut(WifiBattleMatchCard *card) {
    BOOL done = FALSE;
    s32 offset;
    ClActorPos start;
    ClActorPos pos;
    int i;

    if (card->slideCount == 0) {
        for (i = 0; i < CARD_ACTOR_COUNT; i++) {
            if (card->actors[i] != NULL) {
                func_0204c178(card->actors[i], &start, card->surface);
                card->slideX[i] = start.x;
            }
        }
        GFL_SndSEPlay(SEQ_SE_SYS_75);
    }
    offset = (card->slideCount << 8) / SLIDE_FRAMES;
    for (i = 0; i < CARD_ACTOR_COUNT; i++) {
        if (card->actors[i] != NULL) {
            func_0204c178(card->actors[i], &pos, card->surface);
            pos.x = card->slideX[i] - (card->slideCount << 8) / SLIDE_FRAMES;
            func_0204c140(card->actors[i], &pos, card->surface);
        }
    }
    if (card->slideCount++ >= SLIDE_FRAMES) {
        offset = 256;
        card->slideCount = 0;
        done = TRUE;
    }
    GFL_BGSysMoveBGReq(card->bg + 1, BG_MOVE_SET_X, offset);
    GFL_BGSysMoveBGReq(card->bg, BG_MOVE_SET_X, offset);
    return done;
}

static void WifiBattleMatchView_RankCardWindows(WifiBattleMatchCard *card, u32 titleMsg,
                                                const WifiBattleMatchRankInfo *info, PlayerInfo *playerInfo,
                                                Font *font, PrintQueue *printQueue, MsgData *msgData,
                                                WordSet *wordSet, u32 colorKind, HeapID heapId) {
    StrBuf *format;
    StrBuf *text;
    u32 i;
    int j;
    BOOL draw;
    u16 color;
    BmpWin *window;

    card->windowCount = 11;
    for (i = 0; i < card->windowCount; i++) {
        card->windows[i] = BmpWin_CreateDynamic(5, sRankCardRects[i][0], sRankCardRects[i][1], sRankCardRects[i][2],
                                                sRankCardRects[i][3], 15, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(card->windows[i]), 0);
        window = card->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        if (card->windows[i] != NULL) {
            PrintWindow_Init(&card->printWindows[i], card->windows[i]);
        }
    }
    format = GFL_StrBufCreate(128, heapId);
    text = GFL_StrBufCreate(128, heapId);
    for (j = 0; j < 11; j++) {
        color = colorKind == 1 ? PRINT_COLOR(15, 2, 0) : PRINT_COLOR(1, 2, 0);
        draw = TRUE;

        switch (j) {
        case 0:
            GFL_MsgDataLoadStrbuf(msgData, titleMsg, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 1:
            GFL_MsgDataLoadStrbuf(msgData, info->titleMsg + 2, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 2:
            GFL_MsgDataLoadStrbuf(msgData, 7, format);
            copyVarForText(wordSet, 0, playerInfo);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 3:
            if (titleMsg != 0) {
                GFL_MsgDataLoadStrbuf(msgData, 8, text);
            } else {
                draw = FALSE;
            }
            break;
        case 4:
            if (titleMsg != 0) {
                GFL_MsgDataLoadStrbuf(msgData, 9, format);
                WordSetNumber(wordSet, 0, info->unk4, 4, 1, TRUE);
                GFL_WordSetFormatStrbuf(wordSet, text, format);
            } else {
                draw = FALSE;
            }
            break;
        case 5:
            GFL_MsgDataLoadStrbuf(msgData, 10, text);
            break;
        case 6:
            GFL_MsgDataLoadStrbuf(msgData, 11, format);
            WordSetNumber(wordSet, 0, info->unkC, 6, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        case 7:
            GFL_MsgDataLoadStrbuf(msgData, 12, text);
            break;
        case 8:
            GFL_MsgDataLoadStrbuf(msgData, 13, format);
            WordSetNumber(wordSet, 0, info->unk10, 5, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        case 9:
            GFL_MsgDataLoadStrbuf(msgData, 14, text);
            break;
        case 10:
            GFL_MsgDataLoadStrbuf(msgData, 15, format);
            WordSetNumber(wordSet, 0, info->unk12, 5, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        }
        if (draw) {
            PrintWindow_Print(&card->printWindows[j], printQueue, 0, 0, text, font, color);
        }
    }
    GFL_StrBufFree(format);
    GFL_StrBufFree(text);
}

static void WifiBattleMatchView_WifiCardWindows(WifiBattleMatchCard *card, u32 ratio,
                                                const WifiBattleMatchWifiCardInfo *info, PlayerInfo *playerInfo,
                                                Font *font, PrintQueue *printQueue, MsgData *msgData,
                                                WordSet *wordSet, HeapID heapId) {
    StrBuf *format;
    StrBuf *text;
    u32 i;
    int j;
    BOOL draw;
    u16 color;
    BmpWin *window;

    card->windowCount = 8;
    for (i = 0; i < card->windowCount; i++) {
        card->windows[i] = BmpWin_CreateDynamic(card->bg, sWifiCardRects[i][0], sWifiCardRects[i][1],
                                                sWifiCardRects[i][2], sWifiCardRects[i][3], card->palette, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(card->windows[i]), 0);
        window = card->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        if (card->windows[i] != NULL) {
            PrintWindow_Init(&card->printWindows[i], card->windows[i]);
        }
    }
    format = GFL_StrBufCreate(128, heapId);
    text = GFL_StrBufCreate(128, heapId);
    for (j = 0; j < 8; j++) {
        draw = TRUE;
        color = PRINT_COLOR(1, 2, 0);

        switch (j) {
        case 0:
            GFL_StrBufLoadString(text, (const u16 *)info->data);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 1: {
            u32 start = info->startDate;
            u32 end = info->endDate;

            GFL_MsgDataLoadStrbuf(msgData, 16, format);
            WordSetNumber(wordSet, 0, (u16)(2000 + (u8)(start >> 24)), 4, 0, TRUE);
            loadMonthToStrbuf(wordSet, 1, (u8)(start >> 16));
            WordSetNumber(wordSet, 2, (u8)(start >> 8), 2, 0, TRUE);
            WordSetNumber(wordSet, 3, (u16)(2000 + (u8)(end >> 24)), 4, 0, TRUE);
            loadMonthToStrbuf(wordSet, 4, (u8)(end >> 16));
            WordSetNumber(wordSet, 5, (u8)(end >> 8), 2, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            color = PRINT_COLOR(15, 2, 0);
            break;
        }
        case 2:
            textCopy(playerInfo->name, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 3:
            GFL_MsgDataLoadStrbuf(msgData, 8, text);
            break;
        case 4:
            GFL_MsgDataLoadStrbuf(msgData, 9, format);
            WordSetNumber(wordSet, 0, info->unkA0, 4, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        case 5:
            GFL_MsgDataLoadStrbuf(msgData, 10, text);
            break;
        case 6:
            if (ratio != 0) {
                WifiBattleMatchView_PrintCounts(&card->printWindows[j], printQueue, font, info->unkA4,
                                                info->unkA8, heapId);
                draw = FALSE;
            } else {
                GFL_MsgDataLoadStrbuf(msgData, 11, format);
                WordSetNumber(wordSet, 0, info->unkA4, 6, 1, TRUE);
                GFL_WordSetFormatStrbuf(wordSet, text, format);
            }
            break;
        case 7:
            GFL_MsgDataLoadStrbuf(msgData, 17, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        }
        if (draw) {
            PrintWindow_Print(&card->printWindows[j], printQueue, 0, 0, text, font, color);
        }
    }
    GFL_StrBufFree(format);
    GFL_StrBufFree(text);
}

static void WifiBattleMatchView_LiveCardWindows(WifiBattleMatchCard *card, const WifiBattleMatchLiveCardInfo *info,
                                                PlayerInfo *playerInfo, Font *font, PrintQueue *printQueue,
                                                MsgData *msgData, WordSet *wordSet, HeapID heapId) {
    StrBuf *format;
    StrBuf *text;
    u32 i;
    int j;
    BOOL draw;
    u16 color;
    BmpWin *window;

    card->windowCount = 10;
    for (i = 0; i < card->windowCount; i++) {
        card->windows[i] = BmpWin_CreateDynamic(card->bg, sLiveCardRects[i][0], sLiveCardRects[i][1],
                                                sLiveCardRects[i][2], sLiveCardRects[i][3], card->palette, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(card->windows[i]), 0);
        window = card->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        if (card->windows[i] != NULL) {
            PrintWindow_Init(&card->printWindows[i], card->windows[i]);
        }
    }
    format = GFL_StrBufCreate(128, heapId);
    text = GFL_StrBufCreate(128, heapId);
    for (j = 0; j < 10; j++) {
        draw = TRUE;
        color = PRINT_COLOR(1, 2, 0);

        switch (j) {
        case 0:
            GFL_StrBufLoadString(text, (const u16 *)info->data);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 1: {
            u32 start = info->startDate;
            u32 end = info->endDate;

            GFL_MsgDataLoadStrbuf(msgData, 16, format);
            WordSetNumber(wordSet, 0, (u16)(2000 + (u8)(start >> 24)), 4, 0, TRUE);
            loadMonthToStrbuf(wordSet, 1, (u8)(start >> 16));
            WordSetNumber(wordSet, 2, (u8)(start >> 8), 2, 0, TRUE);
            WordSetNumber(wordSet, 3, (u16)(2000 + (u8)(end >> 24)), 4, 0, TRUE);
            loadMonthToStrbuf(wordSet, 4, (u8)(end >> 16));
            WordSetNumber(wordSet, 5, (u8)(end >> 8), 2, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            color = PRINT_COLOR(15, 2, 0);
            break;
        }
        case 2:
            textCopy(playerInfo->name, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 3:
            GFL_MsgDataLoadStrbuf(msgData, 12, text);
            break;
        case 4:
            GFL_MsgDataLoadStrbuf(msgData, 13, format);
            WordSetNumber(wordSet, 0, info->unkA0, 5, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        case 5:
            GFL_MsgDataLoadStrbuf(msgData, 14, text);
            break;
        case 6:
            GFL_MsgDataLoadStrbuf(msgData, 15, format);
            WordSetNumber(wordSet, 0, info->unkA2, 5, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        case 7:
            GFL_MsgDataLoadStrbuf(msgData, 10, text);
            break;
        case 8:
            WifiBattleMatchView_PrintCounts(&card->printWindows[j], printQueue, font, info->unkA4, info->unkA6,
                                            heapId);
            draw = FALSE;
            break;
        case 9:
            GFL_MsgDataLoadStrbuf(msgData, 17, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        }
        if (draw) {
            PrintWindow_Print(&card->printWindows[j], printQueue, 0, 0, text, font, color);
        }
    }
    GFL_StrBufFree(format);
    GFL_StrBufFree(text);
}

static BOOL WifiBattleMatchView_CardFlush(WifiBattleMatchCard *card, PrintQueue *printQueue) {
    BOOL done = TRUE;
    u32 i;

    for (i = 0; i < card->windowCount; i++) {
        if (card->windows[i] != NULL) {
            PrintWindow_Flush(&card->printWindows[i], printQueue);
            done &= !card->printWindows[i].flushPending;
        }
    }
    return done;
}

static void WifiBattleMatchView_CardFreeWindows(WifiBattleMatchCard *card) {
    u32 i;

    for (i = 0; i < card->windowCount; i++) {
        if (card->windows[i] != NULL) {
            BmpWin_Free(card->windows[i]);
        }
    }
}

static void WifiBattleMatchView_CardLoadIcons(WifiBattleMatchCard *card, ClActUnit *unit, void *records,
                                              HeapID heapId) {
    u8 form;
    u8 gender;
    u8 hasItem;
    ArcTool *arc;
    u16 species;
    ClActorSetup setup;
    ClActorSetup itemSetup;
    int i;

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    card->iconPalette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), card->surface, card->iconPaletteSlot << 5, HEAPID_TAIL(heapId));
    card->iconCells = func_0204bde0(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), heapId);
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        species = TournamentTeam_GetField(records, 0, i);
        form = TournamentTeam_GetField(records, 1, i);
        gender = TournamentTeam_GetField(records, 3, i);

        species = species == 0 ? 1 : species;
        card->iconChars[i] =
            func_0204b81c(arc, PokeParty_GetIconIndex(species, form, gender, FALSE), FALSE, card->surface, HEAPID_TAIL(heapId));
    }
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    card->itemPalette = func_0204bbb8(arc, func_0202d890(), card->surface, (card->iconPaletteSlot + 3) << 5, 0, 1,
                                      HEAPID_TAIL(heapId));
    card->itemChars = func_0204b81c(arc, func_0202d894(), FALSE, card->surface, HEAPID_TAIL(heapId));
    card->itemCells = func_0204bde0(arc, func_0202d898(2), func_0202d89c(2), heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.y = 168;
    setup.sequence = 1;
    setup.bgPriority = 1;
    setup.priority = 1;
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        species = TournamentTeam_GetField(records, 0, i);
        form = TournamentTeam_GetField(records, 1, i);
        gender = TournamentTeam_GetField(records, 3, i);
        hasItem = TournamentTeam_GetField(records, 2, i);

        if (species != 0) {
            setup.x = i * 32 + 288;
            card->actors[i] = func_0204c040(unit, card->iconChars[i], card->iconPalette, card->iconCells, &setup,
                                            card->surface, heapId);
            func_0204c378(card->actors[i], PokeIcon_GetPalette(species, form, gender, FALSE), 0);
            if (hasItem) {
                itemSetup = setup;
                itemSetup.x += 12;
                itemSetup.y += 8;
                itemSetup.priority = 0;
                itemSetup.sequence = 0;
                card->actors[CARD_ACTOR_ITEM + i] = func_0204c040(unit, card->itemChars, card->itemPalette,
                                                                  card->itemCells, &itemSetup, card->surface, heapId);
            } else {
                card->actors[CARD_ACTOR_ITEM + i] = NULL;
            }
        } else {
            card->actors[i] = NULL;
            card->actors[CARD_ACTOR_ITEM + i] = NULL;
        }
    }
}

static void WifiBattleMatchView_CardFreeIcons(WifiBattleMatchCard *card) {
    int i;

    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        if (card->actors[i] != NULL) {
            func_0204c108(card->actors[i]);
            card->actors[i] = NULL;
        }
        if (card->actors[CARD_ACTOR_ITEM + i] != NULL) {
            func_0204c108(card->actors[CARD_ACTOR_ITEM + i]);
            card->actors[CARD_ACTOR_ITEM + i] = NULL;
        }
    }
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        func_0204b98c(card->iconChars[i]);
    }
    func_0204be64(card->iconCells);
    func_0204bcd0(card->iconPalette);
    func_0204b98c(card->itemChars);
    func_0204bcd0(card->itemPalette);
    func_0204be64(card->itemCells);
}

static void WifiBattleMatchView_CardLoadBadge(WifiBattleMatchCard *card, u32 look, ClActUnit *unit, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(31, heapId);
    ClActorSetup setup;

    card->badgePalette =
        func_0204bbb8(arc, 0, card->surface, card->itemPaletteSlot << 5, sBadgePaletteStarts[look], 1, heapId);
    card->badgeChars = func_0204b81c(arc, look + 49, FALSE, card->surface, heapId);
    card->badgeCells = func_0204bde0(arc, 65, 66, heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.y = 0;
    setup.x = 0;
    setup.bgPriority = 1;
    card->actors[CARD_ACTOR_BADGE] = func_0204c040(unit, card->badgeChars, card->badgePalette, card->badgeCells,
                                                   &setup, card->surface, heapId);
    func_0204c378(card->actors[CARD_ACTOR_BADGE], 0, 1);
}

static void WifiBattleMatchView_CardFreeBadge(WifiBattleMatchCard *card) {
    func_0204c108(card->actors[CARD_ACTOR_BADGE]);
    func_0204b98c(card->badgeChars);
    func_0204be64(card->badgeCells);
    func_0204bcd0(card->badgePalette);
}

static void WifiBattleMatchView_CardLoadMark(WifiBattleMatchCard *card, ClActUnit *unit, WifiBattleMatchRes *res,
                                             HeapID heapId) {
    ClActorSetup setup;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 280;
    setup.y = 152;
    setup.sequence = 7;
    setup.bgPriority = 1;
    card->actors[CARD_ACTOR_MARK] = func_0204c040(
        unit, WifiBattleMatchView_GetRes(res, RES_CHARS + card->surface),
        WifiBattleMatchView_GetRes(res, RES_PALETTE + card->surface),
        WifiBattleMatchView_GetRes(res, RES_CELLS + card->surface), &setup, card->surface, heapId);
}

static void WifiBattleMatchView_CardFreeMark(WifiBattleMatchCard *card) {
    func_0204c108(card->actors[CARD_ACTOR_MARK]);
}

static void WifiBattleMatchView_CardLoadStars(WifiBattleMatchCard *card, const WifiBattleMatchRankInfo *info,
                                              ClActUnit *unit, WifiBattleMatchRes *res, HeapID heapId) {
    u32 count = WifiBattleMatchView_GetStarCount(info->unkC, info->unk10, info->unk12);
    ClActorSetup setup;
    u32 i;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 256;
    setup.y = 16;
    for (i = 0; i < count; i++) {
        setup.x += 16;
        setup.sequence = i + 2;
        card->actors[CARD_ACTOR_STAR + i] = func_0204c040(
            unit, WifiBattleMatchView_GetRes(res, RES_CHARS + CLACT_VRAM_SUB),
            WifiBattleMatchView_GetRes(res, RES_PALETTE + CLACT_VRAM_SUB),
            WifiBattleMatchView_GetRes(res, RES_CELLS + CLACT_VRAM_SUB), &setup, CLACT_VRAM_SUB, heapId);
    }
}

static void WifiBattleMatchView_CardFreeStars(WifiBattleMatchCard *card) {
    int i;

    for (i = CARD_ACTOR_STAR; i < CARD_ACTOR_STAR + CARD_STAR_MAX; i++) {
        if (card->actors[i] != NULL) {
            func_0204c108(card->actors[i]);
            card->actors[i] = NULL;
        }
    }
}

WifiBattleMatchPartnerCard *WifiBattleMatchPartnerCard_Create(const LiveBattleMatchPlayerData *data, ClActUnit *unit,
                                                WifiBattleMatchRes *res, Font *font, PrintQueue *printQueue,
                                                MsgData *msgData, WordSet *wordSet, u32 kind, u32 shown, u32 variant,
                                                HeapID heapId) {
    WifiBattleMatchPartnerCard *card = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchPartnerCard), FALSE,
                                                        "wifibattlematch_view.c", 2386);
    u32 stars;
    u32 colorKind = 0;

    sys_memset(card, 0, sizeof(WifiBattleMatchPartnerCard));
    card->shown = shown;
    card->kind = kind;
    card->pmsDraw = PMSDraw_Create(unit, CLACT_VRAM_MAIN, printQueue, font, 5, 1, heapId);
    PMSDraw_SetFollowScroll(card->pmsDraw, TRUE);
    PMSDraw_SetBackColor(card->pmsDraw, 0);
    stars = WifiBattleMatchView_GetStarCount(data->record0, data->record1, data->record2);
    if (kind == 2 || kind == 3) {
        colorKind = stars == 5;
    }
    WifiBattleMatchView_PartnerWindows(card, data, font, printQueue, msgData, wordSet, colorKind, heapId);
    WifiBattleMatchView_PartnerLoadBadge(card, func_02008bf4((PlayerInfo *)&data->info), unit, heapId);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_WIFIBATTLEMATCH, 9, 2, 0, 0, FALSE, HEAPID_TAIL(heapId));
    switch (kind) {
    case 2:
    case 3:
        WifiBattleMatchView_PartnerLoadStars(card, data, unit, res, heapId);
        GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 2, 1);
        GFL_BGSysSetScrPaletteNo(2, 0, 3, 32, 32, stars + 3);
        GFL_BGSysSetScrPaletteNo(2, 0, 3, 1, 2, 1);
        GFL_BGSysSetScrPaletteNo(2, 31, 3, 1, 2, 1);
        break;
    case 0:
    case 1:
        GFL_BGSysLoadScrArea(2, 0, 0, 32, 2, GFL_BGSysIsScrHeapExists(2), 0, 24, 32, 32);
        switch (variant) {
        case 0:
            GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 2, 1);
            GFL_BGSysSetScrPaletteNo(2, 0, 3, 32, 32, 3);
            GFL_BGSysSetScrPaletteNo(2, 0, 3, 1, 2, 1);
            GFL_BGSysSetScrPaletteNo(2, 31, 3, 1, 2, 1);
            break;
        case 1:
            GFL_BGSysSetScrPaletteNo(2, 0, 0, 32, 2, 1);
            GFL_BGSysSetScrPaletteNo(2, 0, 3, 32, 32, 7);
            GFL_BGSysSetScrPaletteNo(2, 0, 3, 1, 2, 1);
            GFL_BGSysSetScrPaletteNo(2, 31, 3, 1, 2, 1);
            break;
        }
        break;
    }
    GFL_BGSysLoadScr(2);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, -256);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    return card;
}

void WifiBattleMatchPartnerCard_Free(WifiBattleMatchPartnerCard *card) {
    G2_BlendNone();
    switch (card->kind) {
    case 0:
    case 1:
        break;
    case 2:
    case 3:
        WifiBattleMatchView_PartnerFreeStars(card);
        break;
    }
    WifiBattleMatchView_PartnerFreeBadge(card);
    WifiBattleMatchView_PartnerFreeWindows(card);
    PMSDraw_Delete(card->pmsDraw);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, -256);
    GFL_HeapFree(card);
}

BOOL WifiBattleMatchPartnerCard_Flush(WifiBattleMatchPartnerCard *card, PrintQueue *printQueue) {
    return WifiBattleMatchView_PartnerFlush(card, printQueue);
}

BOOL WifiBattleMatchPartnerCard_SlideIn(WifiBattleMatchPartnerCard *card) {
    BOOL done = FALSE;
    s32 offset;
    ClActorPos start;
    ClActorPos pos;
    int i;

    if (card->slideCount == 0) {
        if (card->badge != NULL) {
            func_0204c178(card->badge, &start, CLACT_VRAM_MAIN);
            card->badgeX = start.x;
        }
        for (i = 0; i < CARD_STAR_MAX; i++) {
            if (card->stars[i] != NULL) {
                func_0204c178(card->stars[i], &start, CLACT_VRAM_MAIN);
                card->starX[i] = start.x;
            }
        }
        GFL_SndSEPlay(SEQ_SE_SYS_75);
    }
    offset = (card->slideCount << 8) / SLIDE_FRAMES - 256;
    if (card->badge != NULL) {
        func_0204c178(card->badge, &pos, CLACT_VRAM_MAIN);
        pos.x = card->badgeX - (card->slideCount << 8) / SLIDE_FRAMES;
        func_0204c140(card->badge, &pos, CLACT_VRAM_MAIN);
    }
    for (i = 0; i < CARD_STAR_MAX; i++) {
        if (card->stars[i] != NULL) {
            func_0204c178(card->stars[i], &pos, CLACT_VRAM_MAIN);
            pos.x = card->starX[i] - (card->slideCount << 8) / SLIDE_FRAMES;
            func_0204c140(card->stars[i], &pos, CLACT_VRAM_MAIN);
        }
    }
    if (card->slideCount++ >= SLIDE_FRAMES) {
        offset = 0;
        card->slideCount = 0;
        done = TRUE;
    }
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, offset);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, offset);
    return done;
}

static void WifiBattleMatchView_PartnerWindows(WifiBattleMatchPartnerCard *card,
                                               const LiveBattleMatchPlayerData *data, Font *font,
                                               PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet,
                                               u32 colorKind, HeapID heapId) {
    StrBuf *format;
    StrBuf *text;
    int i;
    int j;
    u16 color;
    BOOL draw;
    BmpWin *window;

    for (i = 0; i < PARTNER_WINDOW_COUNT; i++) {
        card->windows[i] = BmpWin_CreateDynamic(1, sPartnerCardRects[i][0], sPartnerCardRects[i][1],
                                                sPartnerCardRects[i][2], sPartnerCardRects[i][3], 15, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(card->windows[i]), 0);
        window = card->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        if (i != PARTNER_WINDOW_PMS) {
            PrintWindow_Init(&card->printWindows[i], card->windows[i]);
        }
    }
    format = GFL_StrBufCreate(128, heapId);
    text = GFL_StrBufCreate(128, heapId);
    for (j = 0; j < PARTNER_WINDOW_COUNT; j++) {
        color = colorKind == 1 ? PRINT_COLOR(15, 2, 0) : PRINT_COLOR(1, 2, 0);
        draw = TRUE;

        switch (j) {
        case 0:
            GFL_StrBufLoadString(text, GetPlayerName((PlayerInfo *)&data->info));
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 1:
            if (card->kind == 3) {
                GFL_MsgDataLoadStrbuf(msgData, 12, text);
                color = PRINT_COLOR(15, 2, 0);
            } else if (card->shown != 0) {
                GFL_MsgDataLoadStrbuf(msgData, 8, text);
                color = PRINT_COLOR(15, 2, 0);
            } else {
                draw = FALSE;
            }
            break;
        case 2:
            if (card->kind == 3) {
                GFL_MsgDataLoadStrbuf(msgData, 13, format);
                WordSetNumber(wordSet, 0, data->record1, 5, 1, TRUE);
                GFL_WordSetFormatStrbuf(wordSet, text, format);
                color = PRINT_COLOR(15, 2, 0);
            } else if (card->shown != 0) {
                GFL_MsgDataLoadStrbuf(msgData, 9, format);
                WordSetNumber(wordSet, 0, data->unk34, 4, 1, TRUE);
                GFL_WordSetFormatStrbuf(wordSet, text, format);
                color = PRINT_COLOR(15, 2, 0);
            } else {
                draw = FALSE;
            }
            break;
        case 3:
            GFL_MsgDataLoadStrbuf(msgData, 20, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 4:
            GFL_MsgDataLoadStrbuf(msgData, 22, format);
            loadCountryToStrbuf(wordSet, 0, UnityTowerVisitor_GetCountry((PlayerInfo *)&data->info));
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        case 5: {
            u32 country;

            GFL_MsgDataLoadStrbuf(msgData, 23, format);
            country = UnityTowerVisitor_GetCountry((PlayerInfo *)&data->info);
            loadCountryAreaToStrbuf(wordSet, 0, country, UnityTowerVisitor_GetProvince((PlayerInfo *)&data->info));
            GFL_WordSetFormatStrbuf(wordSet, text, format);
            break;
        }
        case 6:
            GFL_MsgDataLoadStrbuf(msgData, 21, text);
            color = PRINT_COLOR(15, 2, 0);
            break;
        case 7:
            PMSDraw_SetColor(card->pmsDraw, color);
            PMSDraw_Print(card->pmsDraw, card->windows[j], &data->sentence, 0);
            draw = FALSE;
            break;
        }
        if (draw) {
            PrintWindow_Print(&card->printWindows[j], printQueue, 0, 0, text, font, color);
        }
    }
    GFL_StrBufFree(format);
    GFL_StrBufFree(text);
}

static BOOL WifiBattleMatchView_PartnerFlush(WifiBattleMatchPartnerCard *card, PrintQueue *printQueue) {
    BOOL done = TRUE;
    int i;

    for (i = 0; i < PARTNER_WINDOW_COUNT; i++) {
        if (i != PARTNER_WINDOW_PMS) {
            PrintWindow_Flush(&card->printWindows[i], printQueue);
            done &= !card->printWindows[i].flushPending;
        } else {
            PMSDraw_Main(card->pmsDraw);
            done &= PMSDraw_IsPrintEnd(card->pmsDraw);
        }
    }
    return done;
}

static void WifiBattleMatchView_PartnerFreeWindows(WifiBattleMatchPartnerCard *card) {
    int i;

    for (i = 0; i < PARTNER_WINDOW_COUNT; i++) {
        if (card->windows[i] != NULL) {
            BmpWin_ClearScreen(card->windows[i]);
            BmpWin_Free(card->windows[i]);
        }
    }
}

static void WifiBattleMatchView_PartnerLoadBadge(WifiBattleMatchPartnerCard *card, u32 look, ClActUnit *unit,
                                                 HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(31, heapId);
    ClActorSetup setup;

    card->badgePalette = func_0204bbb8(arc, 0, CLACT_VRAM_MAIN, 320, sBadgePaletteStarts[look], 1, heapId);
    card->badgeChars = func_0204b81c(arc, look + 49, FALSE, CLACT_VRAM_MAIN, heapId);
    card->badgeCells = func_0204bde0(arc, 65, 66, heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 288;
    setup.y = 44;
    card->badge = func_0204c040(unit, card->badgeChars, card->badgePalette, card->badgeCells, &setup, CLACT_VRAM_MAIN,
                                heapId);
    func_0204c378(card->badge, 0, 1);
}

static void WifiBattleMatchView_PartnerFreeBadge(WifiBattleMatchPartnerCard *card) {
    func_0204c108(card->badge);
    func_0204b98c(card->badgeChars);
    func_0204be64(card->badgeCells);
    func_0204bcd0(card->badgePalette);
}

static void WifiBattleMatchView_PartnerLoadStars(WifiBattleMatchPartnerCard *card,
                                                 const LiveBattleMatchPlayerData *data, ClActUnit *unit,
                                                 WifiBattleMatchRes *res, HeapID heapId) {
    u32 count = WifiBattleMatchView_GetStarCount(data->record0, data->record1, data->record2);
    ClActorSetup setup;
    u32 i;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 256;
    setup.y = 16;
    for (i = 0; i < count; i++) {
        setup.x += 16;
        setup.sequence = i + 2;
        card->stars[i] = func_0204c040(unit, WifiBattleMatchView_GetRes(res, RES_CHARS + CLACT_VRAM_MAIN),
                                       WifiBattleMatchView_GetRes(res, RES_PALETTE + CLACT_VRAM_MAIN),
                                       WifiBattleMatchView_GetRes(res, RES_CELLS + CLACT_VRAM_MAIN), &setup,
                                       CLACT_VRAM_MAIN, heapId);
    }
}

static void WifiBattleMatchView_PartnerFreeStars(WifiBattleMatchPartnerCard *card) {
    int i;

    for (i = 0; i < CARD_STAR_MAX; i++) {
        if (card->stars[i] != NULL) {
            func_0204c108(card->stars[i]);
            card->stars[i] = NULL;
        }
    }
}

// The number of stars of a rank, by the number of matches
static u32 WifiBattleMatchView_GetStarCount(u32 unk0, u32 count, u32 unk2) {
    if (count >= 1000) {
        return 5;
    }
    if (count >= 500) {
        return 4;
    }
    if (count >= 200) {
        return 3;
    }
    if (count >= 100) {
        return 2;
    }
    if (count >= 50) {
        return 1;
    }
    return 0;
}

WifiBattleMatchLinkIcon *WifiBattleMatchLinkIcon_Create(ClActUnit *unit, WifiBattleMatchRes *res, HeapID heapId) {
    WifiBattleMatchLinkIcon *icon = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchLinkIcon), FALSE,
                                                     "wifibattlematch_view.c", 3055);
    ClActorSetup setup;

    sys_memset(icon, 0, sizeof(WifiBattleMatchLinkIcon));
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 1;
    setup.priority = 1;
    setup.bgPriority = 1;
    icon->actors[0] = func_0204c040(unit, WifiBattleMatchView_GetRes(res, RES_CHARS + CLACT_VRAM_MAIN),
                                    WifiBattleMatchView_GetRes(res, RES_PALETTE + CLACT_VRAM_MAIN),
                                    WifiBattleMatchView_GetRes(res, RES_CELLS + CLACT_VRAM_MAIN), &setup,
                                    CLACT_VRAM_MAIN, heapId);
    func_0204c520(icon->actors[0], TRUE);
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 1;
    icon->actors[1] = func_0204c040(unit, WifiBattleMatchView_GetRes(res, RES_CHARS + CLACT_VRAM_MAIN),
                                    WifiBattleMatchView_GetRes(res, RES_PALETTE + CLACT_VRAM_MAIN),
                                    WifiBattleMatchView_GetRes(res, RES_CELLS + CLACT_VRAM_MAIN), &setup,
                                    CLACT_VRAM_MAIN, heapId);
    func_0204c520(icon->actors[1], TRUE);
    return icon;
}

void WifiBattleMatchLinkIcon_Free(WifiBattleMatchLinkIcon *icon) {
    func_0204c108(icon->actors[1]);
    func_0204c108(icon->actors[0]);
    GFL_HeapFree(icon);
}

void WifiBattleMatchLinkIcon_Update(WifiBattleMatchLinkIcon *icon) {
}

void WifiBattleMatchLinkIcon_SetShown(WifiBattleMatchLinkIcon *icon, BOOL shown) {
    if (shown == TRUE) {
        GFL_SndSEPlay(SEQ_SE_SYS_102);
    } else {
        GFL_SndPlayerStop(2);
    }
    func_0204c124(icon->actors[0], shown);
    func_0204c124(icon->actors[1], shown);
}

WifiBattleMatchTeamList *WifiBattleMatchTeamList_Create(BattleBoxSave *battleBox, ClActUnit *unit, WifiBattleMatchRes *res,
                                             Font *font, PrintQueue *printQueue, MsgData *msgData, HeapID heapId) {
    WifiBattleMatchTeamList *list = GFL_HeapAllocate(heapId, sizeof(WifiBattleMatchTeamList), FALSE,
                                                     "wifibattlematch_view.c", 3210);
    PokeParty *party;
    ArcTool *arc;
    ClActorSetup setup;
    ClActorSetup itemSetup;
    ClActorPos pos;
    u8 x;
    u8 level;
    WordSet *wordSet;
    StrBuf *text;
    StrBuf *levelFormat;
    StrBuf *maleText;
    StrBuf *femaleText;
    PartyPkm *pkm;
    int i;

    sys_memset(list, 0, sizeof(WifiBattleMatchTeamList));
    party = convertBoxedPokeSetToParty(battleBox, HEAPID_TAIL(heapId));
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_WIFIBATTLEMATCH, 7, 2, 0, 0, FALSE, HEAPID_TAIL(heapId));
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    list->palette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_MAIN, 160, HEAPID_TAIL(heapId));
    list->cells = func_0204bde0(arc, PokeIcon_GetCellFile_Main(), getOBJTileMapping_MainEng(), HEAPID_TAIL(heapId));
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            list->chars[i] =
                func_0204b81c(arc, PokeIcon_GetFileOfPkm(func_0201d624(pkm)), FALSE, CLACT_VRAM_MAIN, HEAPID_TAIL(heapId));
        }
    }
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    i = 0;
    list->itemPalette = func_0204bbb8(arc, func_0202d890(), CLACT_VRAM_MAIN, 256, i, 1, HEAPID_TAIL(heapId));
    list->itemCells = func_0204bde0(arc, func_0202d898(2), func_0202d89c(2), HEAPID_TAIL(heapId));
    list->itemChars = func_0204b81c(arc, func_0202d894(), FALSE, CLACT_VRAM_MAIN, HEAPID_TAIL(heapId));
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 24;
    setup.y = 44;
    setup.sequence = 1;
    setup.priority = 1;
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            setup.x = i * 40 + 24;
            list->actors[i] = func_0204c040(unit, list->chars[i], list->palette, list->cells, &setup, CLACT_VRAM_MAIN,
                                            heapId);
            func_0204c378(list->actors[i], PokeIcon_GetPaletteOfPkm(func_0201d624(pkm)), 0);
            if (PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL)) {
                itemSetup = setup;
                itemSetup.x += 12;
                itemSetup.y += 8;
                itemSetup.priority = 0;
                itemSetup.sequence = 0;
                list->actors[TEAM_ACTOR_ITEM + i] = func_0204c040(unit, list->itemChars, list->itemPalette,
                                                                  list->itemCells, &itemSetup, CLACT_VRAM_MAIN, heapId);
            }
        }
    }
    wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(heapId));
    text = GFL_StrBufCreate(128, HEAPID_TAIL(heapId));
    levelFormat = GFL_MsgDataLoadStrbufNew(msgData, 99);
    maleText = GFL_MsgDataLoadStrbufNew(msgData, 100);
    femaleText = GFL_MsgDataLoadStrbufNew(msgData, 101);
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            BmpWin *window;

            list->windows[i] = BmpWin_CreateDynamic(1, i * 5 + 1, 4, 5, 6, 15, TRUE);
            window = list->windows[i];
            BmpWin_FlushChar(window);
            BmpWin_FlushMap(window);
            GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
            PrintWindow_Init(&list->printWindows[i], list->windows[i]);
            if (PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL) == 0) {
                PrintWindow_Print(&list->printWindows[i], printQueue, 32, 0, maleText, font, PRINT_COLOR(5, 6, 0));
            } else if (PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL) == 1) {
                PrintWindow_Print(&list->printWindows[i], printQueue, 32, 0, femaleText, font,
                                  PRINT_COLOR(3, 4, 0));
            }
            level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            WordSetNumber(wordSet, 0, level, 3, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, text, levelFormat);
            x = 20 - GFL_FontGetBlockWidth(text, font, 0) / 2;
            PrintWindow_Print(&list->printWindows[i], printQueue, x, 36, text, font, PRINT_COLOR(1, 2, 0));
        }
    }
    GFL_StrBufFree(femaleText);
    GFL_StrBufFree(maleText);
    GFL_StrBufFree(levelFormat);
    GFL_StrBufFree(text);
    GFL_WordSetSystemFree(wordSet);
    for (i = 0; i < TEAM_ACTOR_COUNT; i++) {
        if (list->actors[i] != NULL) {
            func_0204c178(list->actors[i], &pos, CLACT_VRAM_MAIN);
            pos.x += 256;
            func_0204c140(list->actors[i], &pos, CLACT_VRAM_MAIN);
        }
    }
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, -256);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_X, -256);
    GFL_HeapFree(party);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_BG2, GX_PLANEMASK_BG3, 15, 3);
    return list;
}

void WifiBattleMatchTeamList_Free(WifiBattleMatchTeamList *list) {
    int i;

    G2_BlendNone();
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        if (list->windows[i] != NULL) {
            BmpWin_ClearScreen(list->windows[i]);
            BmpWin_Free(list->windows[i]);
        }
    }
    for (i = 0; i < TEAM_ACTOR_COUNT; i++) {
        if (list->actors[i] != NULL) {
            func_0204c108(list->actors[i]);
        }
    }
    func_0204b98c(list->itemChars);
    func_0204be64(list->itemCells);
    func_0204bcd0(list->itemPalette);
    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        if (list->actors[i] != NULL) {
            func_0204b98c(list->chars[i]);
        }
    }
    func_0204be64(list->cells);
    func_0204bcd0(list->palette);
    GFL_HeapFree(list);
}

BOOL WifiBattleMatchTeamList_SlideIn(WifiBattleMatchTeamList *list) {
    BOOL done;
    s32 offset;
    ClActorPos start;
    ClActorPos pos;
    int i;

    if (GFL_BGSysGetBGOffsetX(2) == 0) {
        return TRUE;
    }
    done = FALSE;
    if (list->slideCount == 0) {
        for (i = 0; i < TEAM_ACTOR_COUNT; i++) {
            if (list->actors[i] != NULL) {
                func_0204c178(list->actors[i], &start, CLACT_VRAM_MAIN);
                list->slideX[i] = start.x;
            }
        }
    }
    offset = (list->slideCount << 8) / SLIDE_FRAMES - 256;
    for (i = 0; i < TEAM_ACTOR_COUNT; i++) {
        if (list->actors[i] != NULL) {
            func_0204c178(list->actors[i], &pos, CLACT_VRAM_MAIN);
            pos.x = list->slideX[i] - (list->slideCount << 8) / SLIDE_FRAMES;
            func_0204c140(list->actors[i], &pos, CLACT_VRAM_MAIN);
        }
    }
    if (list->slideCount++ >= SLIDE_FRAMES) {
        offset = 0;
        list->slideCount = 0;
        done = TRUE;
    }
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, offset);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, offset);
    return done;
}

BOOL WifiBattleMatchTeamList_SlideOut(WifiBattleMatchTeamList *list) {
    BOOL done = FALSE;
    s32 offset;
    ClActorPos start;
    ClActorPos pos;
    int i;

    if (list->slideCount == 0) {
        for (i = 0; i < TEAM_ACTOR_COUNT; i++) {
            if (list->actors[i] != NULL) {
                func_0204c178(list->actors[i], &start, CLACT_VRAM_MAIN);
                list->slideX[i] = start.x;
            }
        }
    }
    offset = (list->slideCount << 8) / SLIDE_FRAMES;
    for (i = 0; i < TEAM_ACTOR_COUNT; i++) {
        if (list->actors[i] != NULL) {
            func_0204c178(list->actors[i], &pos, CLACT_VRAM_MAIN);
            pos.x = list->slideX[i] - (list->slideCount << 8) / SLIDE_FRAMES;
            func_0204c140(list->actors[i], &pos, CLACT_VRAM_MAIN);
        }
    }
    if (list->slideCount++ >= SLIDE_FRAMES) {
        offset = 256;
        list->slideCount = 0;
        done = TRUE;
    }
    GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, offset);
    GFL_BGSysMoveBGReq(1, BG_MOVE_SET_X, offset);
    return done;
}

BOOL WifiBattleMatchTeamList_Flush(WifiBattleMatchTeamList *list, PrintQueue *printQueue) {
    BOOL done = TRUE;
    int i;

    for (i = 0; i < CARD_PARTY_COUNT; i++) {
        if (list->windows[i] != NULL) {
            PrintWindow_Flush(&list->printWindows[i], printQueue);
            done &= !list->printWindows[i].flushPending;
        }
    }
    return done;
}
