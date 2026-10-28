#include "app/pokedex_info.h"
#include "types.h"
#include "app/ui/print_msg.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/types.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_title.h"

// pokedex_info.c, zukan_info.c by the ROM's own string: a Pokémon's Pokédex entry on one screen. Its sprite, its types,
// its footprint, and in windows its name, number, category, height, weight and description, in the game's language or
// in one of the six others that the Pokédex has seen it in. The sprite, types and footprint are loaded into a slot of
// two, so that showing another Pokémon swaps slots instead of waiting for the old one to be freed

// The Pokédex's graphics (ARCID_ZUKAN_GRA) members
#define POKEDEX_GRA_PALETTE 9
#define POKEDEX_GRA_PALETTE_EXTRA 2
#define POKEDEX_GRA_CHARS 0x1a
#define POKEDEX_GRA_CHARS_EXTRA 0xc
#define POKEDEX_GRA_SCREEN_MID 0x29
#define POKEDEX_GRA_SCREEN_BACK 0x20
#define POKEDEX_GRA_SCREEN_EXTRA 0x25

// The archives it reads (our names): the table of the next form of each species and form, the footprints of each
// species, and of the forms that have a footprint of their own
#define ARCID_FORM_NAME_TABLE 97
#define ARCID_POKE_FOOTPRINT 165
#define ARCID_POKE_FOOTPRINT_FORM 229

enum {
    POKEDEX_INFO_MODE_MAIN,
    POKEDEX_INFO_MODE_SUB,
    POKEDEX_INFO_MODE_DETAIL,
};

enum {
    POKEDEX_INFO_STATE_WAIT_START,
    POKEDEX_INFO_STATE_DELAY,
    POKEDEX_INFO_STATE_OPEN_WINDOW,
    POKEDEX_INFO_STATE_WAIT_READY,
    POKEDEX_INFO_STATE_SLIDE_IN,
    POKEDEX_INFO_STATE_WAIT_CRY,
    POKEDEX_INFO_STATE_DONE,
    POKEDEX_INFO_STATE_STATIC,
};

enum {
    POKEDEX_INFO_ARC_POKEDEX_GRA,
    POKEDEX_INFO_ARC_FORM_NAME_TABLE,
    POKEDEX_INFO_ARC_FONT,
    POKEDEX_INFO_ARC_APP_MENU_COMMON,
    POKEDEX_INFO_ARC_POKEGRA,
    POKEDEX_INFO_ARC_FOOTPRINT,
    POKEDEX_INFO_ARC_FOOTPRINT_FORM,
    POKEDEX_INFO_ARC_COUNT,
};

// Where the resources of the actors are loaded: the VRAM of the main or of the sub screen
typedef enum {
    CLACT_DRAW_MAIN,
    CLACT_DRAW_SUB,
} ClActDrawType;

// The windows that print the entry
enum {
    POKEDEX_INFO_WINDOW_LABEL,
    POKEDEX_INFO_WINDOW_NAME,
    POKEDEX_INFO_WINDOW_SIZE,
    POKEDEX_INFO_WINDOW_TEXT,
    POKEDEX_INFO_WINDOW_COUNT,
};

// The message files: the game's language, then the categories, heights, weights, entries and names of each language
enum {
    POKEDEX_INFO_MSG_LABELS,
    POKEDEX_INFO_MSG_CATEGORIES,
    POKEDEX_INFO_MSG_HEIGHTS,
    POKEDEX_INFO_MSG_WEIGHTS,
    POKEDEX_INFO_MSG_ENTRIES,
    POKEDEX_INFO_MSG_OTHER_CATEGORIES,
    POKEDEX_INFO_MSG_OTHER_HEIGHTS = POKEDEX_INFO_MSG_OTHER_CATEGORIES + 6,
    POKEDEX_INFO_MSG_OTHER_WEIGHTS = POKEDEX_INFO_MSG_OTHER_HEIGHTS + 6,
    POKEDEX_INFO_MSG_OTHER_ENTRIES = POKEDEX_INFO_MSG_OTHER_WEIGHTS + 6,
    POKEDEX_INFO_MSG_OTHER_NAMES = POKEDEX_INFO_MSG_OTHER_ENTRIES + 6,
    POKEDEX_INFO_MSG_COUNT = POKEDEX_INFO_MSG_OTHER_NAMES + 6,
};

#define POKEDEX_INFO_STRBUF_COUNT 16
// Japanese, French, German, Italian, Spanish and Korean. The game's own language, which has its own files, comes after
#define POKEDEX_INFO_LANGUAGE_COUNT 6
#define POKEDEX_INFO_LANGUAGE_GAME POKEDEX_INFO_LANGUAGE_COUNT

#define COLOR_LABEL PRINT_COLOR(1, 2, 0)
#define COLOR_TEXT PRINT_COLOR(15, 2, 0)

struct PokedexInfo {
    /* 0x000 */ HeapID heapId;
    /* 0x004 */ BOOL national;
    /* 0x008 */ BOOL caught;
    /* 0x00c */ u16 species;
    /* 0x00e */ u16 form;
    /* 0x010 */ u16 sex;
    /* 0x012 */ u16 rare;
    /* 0x014 */ u32 personality;
    /* 0x018 */ u32 mode;
    /* 0x01c */ u32 engine;
    /* 0x020 */ u8 bgPriority;
    /* 0x024 */ ClActUnit *unit;
    /* 0x028 */ Font *font;
    /* 0x02c */ PrintQueue *printQueue;
    /* 0x030 */ u32 midChars;
    /* 0x034 */ u32 backChars;
    /* 0x038 */ u8 unk38;
    /* 0x039 */ u8 scrollRequest;
    /* 0x03c */ u32 extraChars;
    /* 0x040 */ BOOL hasExtraChars;
    /* 0x044 */ BOOL hidden;
    /* 0x048 */ u32 palType;
    /* 0x04c */ u8 frontBG;
    /* 0x04d */ u8 midBG;
    /* 0x04e */ u8 backBG;
    /* 0x050 */ TCB *tcb;
    /* 0x054 */ u32 state;
    /* 0x058 */ BOOL ready;
    /* 0x05c */ BOOL cryRequested;
    /* 0x060 */ u32 delay;
    /* 0x064 */ u32 tick;
    /* 0x068 */ int windowEdge;
    /* 0x06c */ BmpWin *windows[POKEDEX_INFO_WINDOW_COUNT];
    /* 0x07c */ BOOL flushed[POKEDEX_INFO_WINDOW_COUNT];
    /* 0x08c */ BmpWin *labelWindows[2];
    /* 0x094 */ u32 chars[2];
    /* 0x09c */ u32 palettes[2];
    /* 0x0a4 */ u32 cellAnims[2];
    /* 0x0ac */ ClActor *sprites[2];
    /* 0x0b4 */ u32 spriteSlot;
    /* 0x0b8 */ u32 typeChars[2][2];
    /* 0x0c8 */ u32 typePalette;
    /* 0x0cc */ u32 typeCellAnims;
    /* 0x0d0 */ ClActor *typeActors[2][2];
    /* 0x0e0 */ u32 typeSlot;
    /* 0x0e4 */ BOOL allTypesLoaded;
    /* 0x0e8 */ u32 allTypeChars[TYPE_NULL];
    /* 0x12c */ ClActor *allTypeActors[TYPE_NULL];
    /* 0x170 */ u8 shownTypes[2];
    /* 0x174 */ u32 footprintChars[2];
    /* 0x17c */ u32 footprintPalette;
    /* 0x180 */ u32 footprintCellAnimsOld;
    /* 0x184 */ u32 footprintCellAnims;
    /* 0x188 */ ClActor *footprints[2];
    /* 0x190 */ u32 footprintSlot;
    /* 0x194 */ u16 glowPhase;
    /* 0x196 */ u16 glowColors[5];
    /* 0x1a0 */ u16 glowColorsA[5];
    /* 0x1aa */ u16 glowColorsB[5];
    /* 0x1b4 */ int scrollY;
    /* 0x1b8 */ MsgData *givenMsgs[3];
    /* 0x1c4 */ ArcTool *arcs[POKEDEX_INFO_ARC_COUNT];
    /* 0x1e0 */ MsgData *msgs[POKEDEX_INFO_MSG_COUNT];
    /* 0x26c */ PersonalData *personal;
    /* 0x270 */ StrBuf *labels[POKEDEX_INFO_STRBUF_COUNT];
    /* 0x2b0 */ u16 *formNameTable;
};

static void PokedexInfo_ClearDisplay(PokedexInfo *info);
static void PokedexInfo_VBlankTask(TCB *tcb, void *data);
static void PokedexInfo_InitWindows(PokedexInfo *info);
static void PokedexInfo_FreeWindows(PokedexInfo *info);
static void PokedexInfo_PrintStrbuf(HeapID heapId, BmpWin *window, PrintQueue *printQueue, Font *font, StrBuf *strbuf,
                                    u16 x, u16 y, u16 color, u32 align, WordSet *wordSet);
static void PokedexInfo_PrintMessage(HeapID heapId, BmpWin *window, MsgData *msgData, PrintQueue *printQueue,
                                     Font *font, u32 messageId, u16 x, u16 y, u16 color, u32 align, WordSet *wordSet);
static void PokedexInfo_PrintEntry(PokedexInfo *info);
static void PokedexInfo_PrintEntryInLanguage(PokedexInfo *info, unsigned int language);
static void PokedexInfo_ClearWindows(PokedexInfo *info);
static void PokedexInfo_FlushWindows(PokedexInfo *info);
static void PokedexInfo_InitLabelWindows(PokedexInfo *info);
static void PokedexInfo_FreeLabelWindows(PokedexInfo *info);
static void PokedexInfo_FlushLabelWindow(PokedexInfo *info);
static void PokedexInfo_LoadSprite(PokedexInfo *info, int x, u16 y, u32 slot);
static void PokedexInfo_UnloadSprite(PokedexInfo *info, u32 slot);
static void PokedexInfo_LoadSpriteResources(PokedexInfo *info, u32 slot);
static void PokedexInfo_FreeSpriteResources(PokedexInfo *info, u32 slot);
static void PokedexInfo_CreateSpriteActor(PokedexInfo *info, int x, u16 y, u32 slot);
static void PokedexInfo_DeleteSpriteActor(PokedexInfo *info, u32 slot);
static void PokedexInfo_HideOtherSprites(PokedexInfo *info);
static void PokedexInfo_InitTypeIcons(PokedexInfo *info);
static void PokedexInfo_FreeTypeIcons(PokedexInfo *info);
static void PokedexInfo_ShowTypes(PokedexInfo *info, u32 language, u32 slot);
static void PokedexInfo_ShowTypesInGameLanguage(PokedexInfo *info, u32 type1, u32 type2, u32 slot);
static void PokedexInfo_ShowTypesInLanguage(PokedexInfo *info, u32 type1, u32 type2, u32 language, u32 slot);
static void PokedexInfo_FreeTypeActors(PokedexInfo *info, u32 slot);
static void PokedexInfo_HideOtherTypes(PokedexInfo *info);
static void PokedexInfo_InitFootprints(PokedexInfo *info);
static void PokedexInfo_FreeFootprints(PokedexInfo *info);
static void PokedexInfo_LoadFootprint(PokedexInfo *info, u32 species, u32 form, u32 slot);
static void PokedexInfo_UnloadFootprint(PokedexInfo *info, u32 slot);
static void PokedexInfo_HideOtherFootprints(PokedexInfo *info);
static void PokedexInfo_ShowEntry(PokedexInfo *info);
static void PokedexInfo_ShowEntryInLanguage(PokedexInfo *info, u32 language);
static void PokedexInfo_UpdateFootprintGlow(PokedexInfo *info);
static void PokedexInfo_SetFootprintsVisible(PokedexInfo *info, BOOL visible);
static void PokedexInfo_SetVisible(PokedexInfo *info, BOOL visible);
static void PokedexInfo_LoadResources(PokedexInfo *info);
static void PokedexInfo_FreeResources(PokedexInfo *info);
static u32 PokedexInfo_GetFootprintArcId(void);
static u32 PokedexInfo_GetFootprintFile(u32 species);
static u32 PokedexInfo_GetFootprintPaletteFile(void);
static u32 PokedexInfo_GetFootprintCellFile(int species);
static u32 PokedexInfo_GetFootprintAnimFile(int species);
static u32 PokedexInfo_GetFootprintFormArcId(void);
static u32 PokedexInfo_GetFootprintFormFile(u32 species, u32 form);

// The x of the weight and the height, right aligned, and of the entry, in each of the other languages
static const u16 sWeightX[POKEDEX_INFO_LANGUAGE_COUNT] = { 100, 100, 100, 100, 100, 100 };
static const u16 sEntryX[POKEDEX_INFO_LANGUAGE_COUNT] = { 4, 2, 2, 2, 2, 4 };
// The type icons of each language in the Pokédex's graphics
static const u16 sTypeIconFiles[POKEDEX_INFO_LANGUAGE_COUNT] = { 22, 19, 20, 21, 24, 23 };
static const u16 sHeightX[POKEDEX_INFO_LANGUAGE_COUNT] = { 93, 94, 94, 94, 94, 94 };
static const u32 sArcIds[POKEDEX_INFO_ARC_COUNT] = { ARCID_ZUKAN_GRA, ARCID_FORM_NAME_TABLE, ARCID_FONT, 0, 0, 0, 0 };
// Where each type's icon is in its language's file, as its tile row and column
static const u8 sTypeIconTiles[TYPE_NULL + 1][2] = {
    { 0, 0 }, { 2, 1 }, { 2, 0 }, { 3, 0 }, { 1, 2 }, { 1, 1 }, { 2, 3 }, { 2, 2 }, { 3, 2 },
    { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 3, 1 }, { 1, 3 }, { 4, 0 }, { 3, 3 }, { 0, 0 },
};
static const u32 sLabelMessages[POKEDEX_INFO_STRBUF_COUNT] = {
    9, 195, 10, 11, 202, 197, 198, 199, 200, 201, 209, 204, 205, 206, 207, 208,
};
// clang-format off
static const u16 sMsgBanks[POKEDEX_INFO_MSG_COUNT] = {
    TEXT_BANK_BTL_SERVER_FLOW_TITLE,
    TEXT_BANK_SPECIES_CATEGORIES,
    TEXT_BANK_POKEDEX_HEIGHTS,
    TEXT_BANK_POKEDEX_WEIGHTS,
    TEXT_BANK_POKEDEX_ENTRIES,
    TEXT_BANK_SPECIES_CATEGORIES_JA,
    TEXT_BANK_SPECIES_CATEGORIES_FR,
    TEXT_BANK_SPECIES_CATEGORIES_DE,
    TEXT_BANK_SPECIES_CATEGORIES_IT,
    TEXT_BANK_SPECIES_CATEGORIES_ES,
    TEXT_BANK_SPECIES_CATEGORIES_KO,
    TEXT_BANK_POKEDEX_HEIGHTS_JA,
    TEXT_BANK_POKEDEX_HEIGHTS_FR,
    TEXT_BANK_POKEDEX_HEIGHTS_DE,
    TEXT_BANK_POKEDEX_HEIGHTS_IT,
    TEXT_BANK_POKEDEX_HEIGHTS_ES,
    TEXT_BANK_POKEDEX_HEIGHTS_KO,
    TEXT_BANK_POKEDEX_WEIGHTS_JA,
    TEXT_BANK_POKEDEX_WEIGHTS_FR,
    TEXT_BANK_POKEDEX_WEIGHTS_DE,
    TEXT_BANK_POKEDEX_WEIGHTS_IT,
    TEXT_BANK_POKEDEX_WEIGHTS_ES,
    TEXT_BANK_POKEDEX_WEIGHTS_KO,
    TEXT_BANK_POKEDEX_ENTRIES_JA,
    TEXT_BANK_POKEDEX_ENTRIES_FR,
    TEXT_BANK_POKEDEX_ENTRIES_DE,
    TEXT_BANK_POKEDEX_ENTRIES_IT,
    TEXT_BANK_POKEDEX_ENTRIES_ES,
    TEXT_BANK_POKEDEX_ENTRIES_KO,
    TEXT_BANK_SPECIES_NAMES_JA,
    TEXT_BANK_SPECIES_NAMES_FR,
    TEXT_BANK_SPECIES_NAMES_DE,
    TEXT_BANK_SPECIES_NAMES_IT,
    TEXT_BANK_SPECIES_NAMES_ES,
    TEXT_BANK_SPECIES_NAMES_KO,
};

// clang-format on

PokedexInfo *PokedexInfo_CreateForPartyPkm(HeapID heapId, PartyPkm *pkm, BOOL national, BOOL caught, u32 mode,
                                           u32 engine, u8 bgPriority, ClActUnit *unit, Font *font,
                                           PrintQueue *printQueue, MsgData *msgData0, MsgData *msgData1,
                                           MsgData *msgData2) {
    u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u16 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    u8 sex = PokeParty_GetSex(pkm);
    u16 rare = PokeParty_IsRare(pkm);
    u32 personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);

    return PokedexInfo_Create(heapId, species, form, sex, rare, personality, national, caught, mode, engine, bgPriority,
                              unit, font, printQueue, msgData0, msgData1, msgData2);
}

PokedexInfo *PokedexInfo_Create(HeapID heapId, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL national,
                                BOOL caught, u32 mode, u32 engine, u8 bgPriority, ClActUnit *unit, Font *font,
                                PrintQueue *printQueue, MsgData *msgData0, MsgData *msgData1, MsgData *msgData2) {
    PokedexInfo *info = GFL_HeapAllocate(heapId, sizeof(PokedexInfo), FALSE, "zukan_info.c", 1225);
    void *file;
    int x;
    int y;
    u8 i;
    ArcTool *arc;
    u32 size;
    NNSG2dPaletteData *palette;
    u16 *colors;
    u16 formCount;

    MI_CpuFill8(info, 0, sizeof(PokedexInfo));
    info->heapId = heapId;
    info->species = species;
    info->form = form;
    info->sex = sex;
    info->rare = rare;
    info->personality = personality;
    info->national = national;
    info->caught = caught;
    info->mode = mode;
    info->engine = engine;
    info->bgPriority = bgPriority;
    info->unit = unit;
    info->font = font;
    info->printQueue = PrintQueue_CreateDefault(info->heapId);
    info->givenMsgs[0] = msgData0;
    info->givenMsgs[1] = msgData1;
    info->givenMsgs[2] = msgData2;

    if (info->engine == 0) {
        info->palType = PALTYPE_MAIN_BG;
        info->frontBG = 3;
        info->midBG = 2;
        info->backBG = 0;
    } else {
        info->palType = PALTYPE_SUB_BG;
        info->frontBG = 7;
        info->midBG = 6;
        info->backBG = 4;
    }
    GFL_BGSysSetBGPriority(info->frontBG, info->bgPriority);
    GFL_BGSysSetBGPriority(info->midBG, (u8)(info->bgPriority + 1));
    GFL_BGSysSetBGPriority(info->backBG, (u8)(info->bgPriority + 2));
    info->unk38 = 0;
    info->scrollRequest = 0;

    switch (info->mode) {
    case POKEDEX_INFO_MODE_MAIN:
        info->state = POKEDEX_INFO_STATE_WAIT_START;
        break;
    case POKEDEX_INFO_MODE_SUB:
        info->state = POKEDEX_INFO_STATE_WAIT_CRY;
        break;
    case POKEDEX_INFO_MODE_DETAIL:
        info->state = POKEDEX_INFO_STATE_STATIC;
        break;
    }
    info->ready = FALSE;
    info->cryRequested = FALSE;
    info->delay = 0;
    info->tick = 0;
    info->glowPhase = 0;
    if (info->mode == POKEDEX_INFO_MODE_DETAIL && info->engine == 0) {
        info->scrollY = -24;
    } else {
        info->scrollY = 0;
    }

    PokedexInfo_LoadResources(info);
    formCount = PML_PersonalGetParam(info->personal, PERSONAL_FORM_COUNT);
    if (info->form >= formCount) {
        info->form = 0;
    }

    arc = info->arcs[POKEDEX_INFO_ARC_POKEDEX_GRA];
    GFL_G2DIOLoadArcNCLRDefault(arc, POKEDEX_GRA_PALETTE, info->palType, 0, 0x60, info->heapId);
    info->midChars = GFL_BGSysLoadArcNCGRDynamic(arc, POKEDEX_GRA_CHARS, info->midBG, 0x1000, FALSE, info->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, POKEDEX_GRA_SCREEN_MID, info->midBG, CHAR_POS(info->midChars), 0x800,
                                           FALSE, info->heapId);
    info->backChars = GFL_BGSysLoadArcNCGRDynamic(arc, POKEDEX_GRA_CHARS, info->backBG, 0x1000, FALSE, info->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, POKEDEX_GRA_SCREEN_BACK, info->backBG, CHAR_POS(info->backChars), 0x800,
                                           FALSE, info->heapId);
    info->hasExtraChars = FALSE;
    info->hidden = FALSE;

    if (info->mode == POKEDEX_INFO_MODE_DETAIL && info->engine == 1) {
        GFL_BGSysFillScrArea(info->midBG, 0, 0, 0, 0x20, 3, 0x11);
        arc = info->arcs[POKEDEX_INFO_ARC_POKEDEX_GRA];
        GFL_G2DIOLoadArcNCLR(arc, POKEDEX_GRA_PALETTE_EXTRA, info->palType, 0, 0x80, 0x40, info->heapId);
        info->extraChars =
            GFL_BGSysLoadArcNCGRDynamic(arc, POKEDEX_GRA_CHARS_EXTRA, info->backBG, 0x2000, FALSE, info->heapId);
        info->hasExtraChars = TRUE;
        GFL_G2DIOLoadNSCRSync(arc, POKEDEX_GRA_SCREEN_EXTRA, info->backBG, 0x400, CHAR_POS(info->extraChars), 0x800,
                              FALSE, info->heapId);
        GFL_BGSysSetScrPaletteNo(info->backBG, 0, 0x20, 0x20, 0x15, 4);
        GFL_BGSysSetScrPaletteNo(info->backBG, 0, 0x35, 0x20, 3, 5);
    }

    arc = info->arcs[POKEDEX_INFO_ARC_POKEDEX_GRA];
    size = GFL_ArcToolGetDataLength(arc, POKEDEX_GRA_PALETTE);
    file = GFL_HeapAllocate(info->heapId, size, FALSE, "zukan_info.c", 1425);
    GFL_ArcToolRead(arc, POKEDEX_GRA_PALETTE, file);
    NNS_G2dGetUnpackedPaletteData(file, &palette);
    colors = palette->pRawData;
    for (i = 0; i < 5; i++) {
        info->glowColorsA[i] = colors[0x11 + i];
        info->glowColorsB[i] = colors[0x21 + i];
    }
    GFL_HeapFree(file);

    PokedexInfo_InitLabelWindows(info);
    PokedexInfo_FlushLabelWindow(info);
    info->tcb = GFL_VBlankTCBAdd(PokedexInfo_VBlankTask, info, 1);
    PokedexInfo_InitTypeIcons(info);
    PokedexInfo_InitFootprints(info);
    if (info->mode == POKEDEX_INFO_MODE_MAIN || info->mode == POKEDEX_INFO_MODE_DETAIL) {
        PokedexInfo_InitWindows(info);
        PokedexInfo_ShowEntry(info);
    }

    for (i = 0; i < 2; i++) {
        info->sprites[i] = NULL;
    }
    info->spriteSlot = 0;
    switch (info->mode) {
    case POKEDEX_INFO_MODE_MAIN:
    case POKEDEX_INFO_MODE_DETAIL:
        x = 0x32;
        y = 0x48;
        break;
    case POKEDEX_INFO_MODE_SUB:
        x = 0x80;
        y = 0x48;
        break;
    }
    PokedexInfo_LoadSprite(info, x, y + info->scrollY, info->spriteSlot);

    if (info->mode == POKEDEX_INFO_MODE_MAIN) {
        info->windowEdge = 0x88;
        GX_SetVisibleWnd(GX_WNDMASK_W0);
        G2_SetWnd0Position(0, 8, 100, info->windowEdge);
        G2_SetWnd0InsidePlane(0x1f, TRUE);
        G2_SetWndOutsidePlane(0x1f, FALSE);
        G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 0x10, -16);
    }

    GFL_BGSysMoveBG(info->midBG, BG_MOVE_SET_Y, -info->scrollY);
    GFL_BGSysMoveBG(info->backBG, BG_MOVE_SET_Y, -info->scrollY);
    GFL_BGSysMoveBG(info->frontBG, BG_MOVE_SET_Y, -info->scrollY);
    GFL_BGSysLoadScr(info->midBG);
    GFL_BGSysLoadScr(info->backBG);
    return info;
}

void PokedexInfo_Free(PokedexInfo *info) {
    u8 i;

    PokedexInfo_Show(info);
    info->hidden = FALSE;
    for (i = 0; i < 2; i++) {
        PokedexInfo_UnloadSprite(info, i);
    }
    if (info->mode == POKEDEX_INFO_MODE_DETAIL) {
        PokedexInfo_FreeEntry(info);
    }
    PokedexInfo_FreeFootprints(info);
    PokedexInfo_FreeTypeIcons(info);
    GFL_TCBRemove(info->tcb);
    PokedexInfo_FreeLabelWindows(info);
    if (info->hasExtraChars) {
        GFL_BGSysFreeCharMemory(info->backBG, CHAR_POS(info->extraChars), CHAR_SIZE(info->extraChars));
        info->hasExtraChars = FALSE;
    }
    GFL_BGSysFreeCharMemory(info->backBG, CHAR_POS(info->backChars), CHAR_SIZE(info->backChars));
    GFL_BGSysFreeCharMemory(info->midBG, CHAR_POS(info->midChars), CHAR_SIZE(info->midChars));
    PokedexInfo_FreeResources(info);
    PrintQueue_Clear(info->printQueue);
    PrintQueue_Free(info->printQueue);
    GFL_HeapFree(info);
}

void PokedexInfo_Update(PokedexInfo *info) {
    ClActorPos pos;
    BOOL xDone;
    BOOL yDone;

    if (info->mode == POKEDEX_INFO_MODE_MAIN) {
        PokedexInfo_UpdateFootprintGlow(info);
    }

    info->tick++;
    if (info->tick == 4) {
        info->scrollRequest |= 1;
        info->tick = 0;
    }

    switch (info->state) {
    case POKEDEX_INFO_STATE_WAIT_START:
    case POKEDEX_INFO_STATE_DONE:
    case POKEDEX_INFO_STATE_STATIC:
        break;
    case POKEDEX_INFO_STATE_DELAY:
        info->delay++;
        if (info->delay == 15) {
            info->state = POKEDEX_INFO_STATE_OPEN_WINDOW;
        }
        break;
    case POKEDEX_INFO_STATE_OPEN_WINDOW:
        info->windowEdge--;
        G2_SetWnd0Position(0, 8, 100, info->windowEdge);
        if (info->windowEdge == 0x10) {
            GX_SetVisibleWnd(GX_WNDMASK_NONE);
            G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 0x10, 0);
            PokeVoice_Play(info->species, info->form, 0x40, 0, 0, 0, 0, NULL);
            info->state = POKEDEX_INFO_STATE_WAIT_READY;
        }
        break;
    case POKEDEX_INFO_STATE_WAIT_READY:
        if (info->ready) {
            info->state = POKEDEX_INFO_STATE_SLIDE_IN;
        }
        break;
    case POKEDEX_INFO_STATE_SLIDE_IN:
        xDone = FALSE;
        yDone = FALSE;
        ClActor_GetPos(info->sprites[info->spriteSlot], &pos);
        pos.x++;
        pos.y++;
        if (pos.x > 0x80) {
            pos.x = 0x80;
            xDone = TRUE;
        }
        if (pos.y > 0x48) {
            pos.y = 0x48;
            yDone = TRUE;
        }
        ClActor_SetPos(info->sprites[info->spriteSlot], &pos);
        if (xDone && yDone) {
            info->state = POKEDEX_INFO_STATE_DONE;
        }
        break;
    case POKEDEX_INFO_STATE_WAIT_CRY:
        if (info->cryRequested) {
            PokeVoice_Play(info->species, info->form, 0x40, 0, 0, 0, 0, NULL);
            info->state = POKEDEX_INFO_STATE_DONE;
        }
        break;
    }

    if (info->mode == POKEDEX_INFO_MODE_MAIN || info->mode == POKEDEX_INFO_MODE_DETAIL) {
        PokedexInfo_FlushWindows(info);
    }
    PrintQueue_Main(info->printQueue);
}

void PokedexInfo_FreeEntry(PokedexInfo *info) {
    PokedexInfo_ClearWindows(info);
    PokedexInfo_FreeWindows(info);
    PokedexInfo_UnloadFootprint(info, info->footprintSlot);
    PokedexInfo_FreeTypeActors(info, info->typeSlot);
}

static void PokedexInfo_ClearDisplay(PokedexInfo *info) {
    PokedexInfo_ClearWindows(info);
    PokedexInfo_UnloadFootprint(info, info->footprintSlot);
    PokedexInfo_FreeTypeActors(info, info->typeSlot);
}

void PokedexInfo_SetReady(PokedexInfo *info) {
    info->ready = TRUE;
}

void PokedexInfo_RequestCry(PokedexInfo *info) {
    info->cryRequested = TRUE;
}

BOOL PokedexInfo_IsSlidingIn(PokedexInfo *info) {
    BOOL sliding = FALSE;

    if (info->state == POKEDEX_INFO_STATE_SLIDE_IN) {
        sliding = TRUE;
    }
    return sliding;
}

BOOL PokedexInfo_IsDone(PokedexInfo *info) {
    BOOL done = FALSE;

    if (info->state == POKEDEX_INFO_STATE_DONE) {
        done = TRUE;
    }
    return done;
}

void PokedexInfo_Start(PokedexInfo *info) {
    if (info->state == POKEDEX_INFO_STATE_WAIT_START) {
        info->state = POKEDEX_INFO_STATE_DELAY;
    }
}

void PokedexInfo_SetPokemon(PokedexInfo *info, u16 species, u16 form, u16 sex, u16 rare, u32 personality, BOOL caught) {
    int x;
    int y;
    u16 formCount;

    if (info->personal != NULL) {
        PML_PersonalFree(info->personal);
    }
    info->personal = PML_PersonalLoad(species, form, info->heapId);

    if (info->spriteSlot != 0) {
        info->spriteSlot = 0;
    } else {
        info->spriteSlot = 1;
    }
    if (info->typeSlot != 0) {
        info->typeSlot = 0;
    } else {
        info->typeSlot = 1;
    }
    if (info->footprintSlot != 0) {
        info->footprintSlot = 0;
    } else {
        info->footprintSlot = 1;
    }
    PokedexInfo_UnloadSprite(info, info->spriteSlot);
    PokedexInfo_ClearDisplay(info);

    info->species = species;
    info->form = form;
    info->sex = sex;
    info->rare = rare;
    info->personality = personality;
    info->caught = caught;
    formCount = PML_PersonalGetParam(info->personal, PERSONAL_FORM_COUNT);
    if (info->form >= formCount) {
        info->form = 0;
    }

    PokedexInfo_FlushLabelWindow(info);
    PokedexInfo_ShowEntry(info);
    switch (info->mode) {
    case POKEDEX_INFO_MODE_MAIN:
    case POKEDEX_INFO_MODE_DETAIL:
        x = 0x32;
        y = 0x48;
        break;
    case POKEDEX_INFO_MODE_SUB:
        x = 0x80;
        y = 0x48;
        break;
    }
    PokedexInfo_LoadSprite(info, x, y + info->scrollY, info->spriteSlot);
    PokedexInfo_SetVisible(info, !info->hidden);
}

void PokedexInfo_SetLanguage(PokedexInfo *info, u32 language) {
    if (info->typeSlot != 0) {
        info->typeSlot = 0;
    } else {
        info->typeSlot = 1;
    }
    PokedexInfo_ClearWindows(info);
    PokedexInfo_FreeTypeActors(info, info->typeSlot);
    PokedexInfo_ShowTypes(info, language, info->typeSlot);
    PokedexInfo_SetFootprintsVisible(info, info->caught);
    if (language == POKEDEX_INFO_LANGUAGE_GAME || info->species > SPECIES_ARCEUS) {
        PokedexInfo_PrintEntry(info);
    } else {
        PokedexInfo_PrintEntryInLanguage(info, language);
    }
}

void PokedexInfo_SetPokemonInLanguage(PokedexInfo *info, u16 species, u16 form, u16 sex, u16 rare, u32 personality,
                                      BOOL caught, u32 language) {
    int x;
    int y;
    u16 formCount;

    if (info->personal != NULL) {
        PML_PersonalFree(info->personal);
    }
    info->personal = PML_PersonalLoad(species, form, info->heapId);

    if (info->spriteSlot != 0) {
        info->spriteSlot = 0;
    } else {
        info->spriteSlot = 1;
    }
    if (info->typeSlot != 0) {
        info->typeSlot = 0;
    } else {
        info->typeSlot = 1;
    }
    if (info->footprintSlot != 0) {
        info->footprintSlot = 0;
    } else {
        info->footprintSlot = 1;
    }
    PokedexInfo_UnloadSprite(info, info->spriteSlot);
    PokedexInfo_ClearDisplay(info);

    info->species = species;
    info->form = form;
    info->sex = sex;
    info->rare = rare;
    info->personality = personality;
    info->caught = caught;
    formCount = PML_PersonalGetParam(info->personal, PERSONAL_FORM_COUNT);
    if (info->form >= formCount) {
        info->form = 0;
    }

    PokedexInfo_FlushLabelWindow(info);
    if (language == POKEDEX_INFO_LANGUAGE_GAME || info->species > SPECIES_ARCEUS) {
        PokedexInfo_ShowEntry(info);
    } else {
        PokedexInfo_ShowEntryInLanguage(info, language);
    }
    switch (info->mode) {
    case POKEDEX_INFO_MODE_MAIN:
    case POKEDEX_INFO_MODE_DETAIL:
        x = 0x32;
        y = 0x48;
        break;
    case POKEDEX_INFO_MODE_SUB:
        x = 0x80;
        y = 0x48;
        break;
    }
    PokedexInfo_LoadSprite(info, x, y + info->scrollY, info->spriteSlot);
    PokedexInfo_SetVisible(info, !info->hidden);
}

void PokedexInfo_Hide(PokedexInfo *info) {
    BOOL visible;

    if (!info->hidden) {
        visible = TRUE;
        info->hidden = TRUE;
        GFL_BGSysSetBGEnabled(info->frontBG, FALSE);
        GFL_BGSysSetBGEnabled(info->midBG, FALSE);
        GFL_BGSysMoveBGReq(info->backBG, BG_MOVE_SET_Y, -0x100);
        if (info->hidden) {
            visible = FALSE;
        }
        PokedexInfo_SetVisible(info, visible);
    }
}

void PokedexInfo_Show(PokedexInfo *info) {
    BOOL visible;

    if (info->hidden) {
        info->hidden = FALSE;
        visible = TRUE;
        GFL_BGSysSetBGEnabled(info->frontBG, TRUE);
        GFL_BGSysSetBGEnabled(info->midBG, TRUE);
        GFL_BGSysMoveBGReq(info->backBG, BG_MOVE_SET_Y, -info->scrollY);
        if (info->hidden) {
            visible = FALSE;
        }
        PokedexInfo_SetVisible(info, visible);
    }
}

static void PokedexInfo_VBlankTask(TCB *tcb, void *data) {
    PokedexInfo *info = data;

    if (info->scrollRequest & 1) {
        GFL_BGSysMoveBGReq(info->backBG, BG_MOVE_RIGHT, 1);
    }
    info->unk38 = 0;
    info->scrollRequest = 0;
}

static void PokedexInfo_InitWindows(PokedexInfo *info) {
    u32 palType = PALTYPE_MAIN_BG;
    u32 size;
    int i;

    info->formNameTable =
        GFL_ArcToolReadHeapNewLZGetLen(info->arcs[POKEDEX_INFO_ARC_FORM_NAME_TABLE], 0, FALSE, info->heapId, &size);
    if (info->engine != 0) {
        palType = PALTYPE_SUB_BG;
    }
    GFL_G2DIOLoadArcNCLRDefault(info->arcs[POKEDEX_INFO_ARC_FONT], 5, palType, 0x60, 0x20, info->heapId);
    {
        // The position and size of each window
        u8 rects[POKEDEX_INFO_WINDOW_COUNT][4] = {
            { 2, 0, 28, 3 },
            { 16, 4, 15, 5 },
            { 18, 12, 13, 5 },
            { 2, 17, 28, 7 },
        };

        for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
            info->windows[i] =
                BmpWin_CreateDynamic(info->frontBG, rects[i][0], rects[i][1], rects[i][2], rects[i][3], 3, TRUE);
            GFL_BitmapFill(BmpWin_GetBitmap(info->windows[i]), 0);
        }
    }
    for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
        info->flushed[i] = TRUE;
    }
}

static void PokedexInfo_FreeWindows(PokedexInfo *info) {
    int i;

    PrintQueue_Clear(info->printQueue);
    for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
        info->flushed[i] = TRUE;
        BmpWin_Free(info->windows[i]);
    }
    GFL_HeapFree(info->formNameTable);
}

// Prints a string. x is where it starts, or where it ends for a right alignment; a centered one ignores it. With a
// word set, the string is a format for it
static void PokedexInfo_PrintStrbuf(HeapID heapId, BmpWin *window, PrintQueue *printQueue, Font *font, StrBuf *strbuf,
                                    u16 x, u16 y, u16 color, u32 align, WordSet *wordSet) {
    u16 left;
    u16 width;
    GFLBitmap *bitmap = BmpWin_GetBitmap(window);
    StrBuf *formatted;

    if (wordSet != NULL) {
        formatted = GFL_StrBufCreate(0x80, heapId);
        GFL_WordSetFormatStrbuf(wordSet, formatted, strbuf);
    } else {
        formatted = strbuf;
    }

    switch (align) {
    case PRINT_ALIGN_LEFT:
        left = x;
        break;
    case PRINT_ALIGN_RIGHT:
        width = GFL_FontGetBlockWidth(formatted, font, 0);
        left = x - width;
        break;
    case PRINT_ALIGN_CENTER:
        width = GFL_FontGetBlockWidth(formatted, font, 0);
        left = (GFL_BitmapGetWidth(bitmap) - width) / 2;
        break;
    }
    PrintQueue_PrintColor(printQueue, bitmap, left, y, formatted, font, color);

    if (wordSet != NULL) {
        GFL_StrBufFree(formatted);
    }
}

static void PokedexInfo_PrintMessage(HeapID heapId, BmpWin *window, MsgData *msgData, PrintQueue *printQueue,
                                     Font *font, u32 messageId, u16 x, u16 y, u16 color, u32 align, WordSet *wordSet) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(msgData, messageId);

    PokedexInfo_PrintStrbuf(heapId, window, printQueue, font, strbuf, x, y, color, align, wordSet);
    GFL_StrBufFree(strbuf);
}

static void PokedexInfo_PrintEntry(PokedexInfo *info) {
    MsgData *labels = info->msgs[POKEDEX_INFO_MSG_LABELS];
    MsgData *categories = info->msgs[POKEDEX_INFO_MSG_CATEGORIES];
    MsgData *heights = info->msgs[POKEDEX_INFO_MSG_HEIGHTS];
    MsgData *weights = info->msgs[POKEDEX_INFO_MSG_WEIGHTS];
    MsgData *entries = info->msgs[POKEDEX_INFO_MSG_ENTRIES];
    u16 message = info->species;
    u16 n = 0;
    u8 i;
    u16 number;
    WordSet *wordSet;
    StrBuf *name;
    u32 messageId;

    // The messages of a form follow its species's through the table of next forms
    for (; n != info->form; n++) {
        message = info->formNameTable[message];
        if (message == 0) {
            break;
        }
    }
    if (message == 0) {
        message = info->species;
    }

    if (info->mode != POKEDEX_INFO_MODE_DETAIL) {
        PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_LABEL], labels, info->printQueue,
                                 info->font, BtlServerFlowTitle_Text_PokedexRegistrationCompleted, 0, 5, COLOR_TEXT,
                                 PRINT_ALIGN_CENTER, NULL);
    }

    number = info->species;
    if (!info->national) {
        u16 *table = PML_PersonalLoadRegionalDexTable(info->heapId, 0);

        number = table[info->species];
        GFL_HeapFree(table);
    }
    if (number != 999) {
        wordSet = GFL_WordSetSystemCreateDefault(info->heapId);
        WordSetNumber(wordSet, 0, number, 3, NUM_PAD_ZERO, TRUE);
        PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], info->printQueue, info->font,
                                info->labels[0], 8, 5, COLOR_LABEL, PRINT_ALIGN_LEFT, wordSet);
        GFL_WordSetSystemFree(wordSet);
    } else {
        PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], info->printQueue, info->font,
                                info->labels[1], 8, 5, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);
    }

    name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, info->species);
    PrintQueue_PrintColor(info->printQueue, BmpWin_GetBitmap(info->windows[POKEDEX_INFO_WINDOW_NAME]), 0x30, 5, name,
                          info->font, COLOR_LABEL);
    GFL_StrBufFree(name);

    messageId = 0;
    if (info->caught) {
        messageId = info->species;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], categories, info->printQueue,
                             info->font, messageId, 0, 22, COLOR_LABEL, PRINT_ALIGN_CENTER, NULL);

    PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], info->printQueue, info->font,
                            info->labels[2], 0, 4, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);
    PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], info->printQueue, info->font,
                            info->labels[3], 0, 20, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);

    messageId = 0;
    if (info->caught) {
        messageId = message;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], heights, info->printQueue,
                             info->font, messageId, 101, 4, COLOR_LABEL, PRINT_ALIGN_RIGHT, NULL);
    messageId = 0;
    if (info->caught) {
        messageId = message;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], weights, info->printQueue,
                             info->font, messageId, 102, 20, COLOR_LABEL, PRINT_ALIGN_RIGHT, NULL);
    messageId = 0;
    if (info->caught) {
        messageId = message;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_TEXT], entries, info->printQueue,
                             info->font, messageId, 2, 5, COLOR_TEXT, PRINT_ALIGN_LEFT, NULL);

    for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
        info->flushed[i] = FALSE;
    }
    PokedexInfo_FlushWindows(info);
}

static void PokedexInfo_PrintEntryInLanguage(PokedexInfo *info, unsigned int language) {
    MsgData *labels = info->msgs[POKEDEX_INFO_MSG_LABELS];
    MsgData *categories = info->msgs[POKEDEX_INFO_MSG_OTHER_CATEGORIES + language];
    MsgData *heights = info->msgs[POKEDEX_INFO_MSG_OTHER_HEIGHTS + language];
    MsgData *weights = info->msgs[POKEDEX_INFO_MSG_OTHER_WEIGHTS + language];
    MsgData *entries = info->msgs[POKEDEX_INFO_MSG_OTHER_ENTRIES + language];
    MsgData *names = info->msgs[POKEDEX_INFO_MSG_OTHER_NAMES + language];
    u16 message = info->species;
    u16 number;
    u16 messageId;
    u8 i;
    WordSet *wordSet;

    // Giratina's Origin Forme and Shaymin's Sky Forme have messages after the last species of the generations
    if (message == SPECIES_GIRATINA && info->form != 0) {
        message = SPECIES_ARCEUS + 1;
    } else if (message == SPECIES_SHAYMIN && info->form != 0) {
        message = SPECIES_ARCEUS + 2;
    }

    if (info->mode != POKEDEX_INFO_MODE_DETAIL) {
        PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_LABEL], labels, info->printQueue,
                                 info->font, BtlServerFlowTitle_Text_PokedexRegistrationCompleted, 0, 5, COLOR_TEXT,
                                 PRINT_ALIGN_CENTER, NULL);
    }

    number = info->species;
    if (!info->national) {
        u16 *table = PML_PersonalLoadRegionalDexTable(info->heapId, 0);

        number = table[info->species];
        GFL_HeapFree(table);
    }
    if (number != 999) {
        wordSet = GFL_WordSetSystemCreateDefault(info->heapId);
        if (language == 0) {
            WordSetNumber(wordSet, 0, number, 3, NUM_PAD_ZERO, FALSE);
        } else {
            WordSetNumber(wordSet, 0, number, 3, NUM_PAD_ZERO, TRUE);
        }
        PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], info->printQueue, info->font,
                                info->labels[0], 8, 5, COLOR_LABEL, PRINT_ALIGN_LEFT, wordSet);
        GFL_WordSetSystemFree(wordSet);
    } else {
        PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], info->printQueue, info->font,
                                info->labels[1], 8, 5, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);
    }

    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], names, info->printQueue, info->font,
                             info->species, 0x30, 5, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);

    messageId = 0;
    if (info->caught) {
        messageId = info->species;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_NAME], categories, info->printQueue,
                             info->font, messageId, 0, 22, COLOR_LABEL, PRINT_ALIGN_CENTER, NULL);

    PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], info->printQueue, info->font,
                            info->labels[4 + language], 0, 4, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);
    PokedexInfo_PrintStrbuf(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], info->printQueue, info->font,
                            info->labels[10 + language], 0, 20, COLOR_LABEL, PRINT_ALIGN_LEFT, NULL);

    messageId = 0;
    if (info->caught) {
        messageId = message;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], heights, info->printQueue,
                             info->font, messageId, sHeightX[language], 4, COLOR_LABEL, PRINT_ALIGN_RIGHT, NULL);
    messageId = 0;
    if (info->caught) {
        messageId = message;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_SIZE], weights, info->printQueue,
                             info->font, messageId, sWeightX[language], 20, COLOR_LABEL, PRINT_ALIGN_RIGHT, NULL);
    messageId = 0;
    if (info->caught) {
        messageId = info->species;
    }
    PokedexInfo_PrintMessage(info->heapId, info->windows[POKEDEX_INFO_WINDOW_TEXT], entries, info->printQueue,
                             info->font, messageId, sEntryX[language], 5, COLOR_TEXT, PRINT_ALIGN_LEFT, NULL);

    for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
        info->flushed[i] = FALSE;
    }
    PokedexInfo_FlushWindows(info);
}

static void PokedexInfo_ClearWindows(PokedexInfo *info) {
    int i;

    PrintQueue_Clear(info->printQueue);
    for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
        info->flushed[i] = TRUE;
        GFL_BitmapFill(BmpWin_GetBitmap(info->windows[i]), 0);
    }
}

static void PokedexInfo_FlushWindows(PokedexInfo *info) {
    int i;

    for (i = 0; i < POKEDEX_INFO_WINDOW_COUNT; i++) {
        if (!info->flushed[i] && !PrintQueue_IsBitmapPending(info->printQueue, BmpWin_GetBitmap(info->windows[i]))) {
            BmpWin_Transfer(info->windows[i]);
            info->flushed[i] = TRUE;
        }
    }
}

static void PokedexInfo_InitLabelWindows(PokedexInfo *info) {
    GFLBitmap *src = GFL_G2DIOLoadBitmap(ARCID_ZUKAN_GRA, POKEDEX_GRA_CHARS, FALSE, info->heapId);
    GFLBitmap *dest;

    // The caught label's window
    info->labelWindows[0] = BmpWin_CreateDynamic(info->midBG, 12, 4, 4, 3, 0, FALSE);
    dest = BmpWin_GetBitmap(info->labelWindows[0]);
    GFL_BitmapCopyArea(src, dest, 8, 0, 0, 0, 0x20, 0x18, 0);

    // The not caught label's window, with its pieces
    info->labelWindows[1] = BmpWin_CreateDynamic(info->midBG, 12, 4, 4, 3, 0, FALSE);
    dest = BmpWin_GetBitmap(info->labelWindows[1]);
    GFL_BitmapCopyArea(src, dest, 0xa8, 8, 0, 0, 0x20, 8, 0);
    GFL_BitmapCopyArea(src, BmpWin_GetBitmap(info->labelWindows[1]), 0xf8, 8, 0, 8, 8, 8, 0);
    GFL_BitmapCopyArea(src, BmpWin_GetBitmap(info->labelWindows[1]), 0x28, 0x10, 8, 8, 0x18, 8, 0);
    GFL_BitmapCopyArea(src, BmpWin_GetBitmap(info->labelWindows[1]), 0x80, 0, 0, 0x10, 8, 8, 0);
    GFL_BitmapCopyArea(src, BmpWin_GetBitmap(info->labelWindows[1]), 0x58, 0x10, 8, 0x10, 0x18, 8, 0);
    GFL_BitmapFree(src);
}

static void PokedexInfo_FreeLabelWindows(PokedexInfo *info) {
    BmpWin_Free(info->labelWindows[0]);
    BmpWin_Free(info->labelWindows[1]);
}

static void PokedexInfo_FlushLabelWindow(PokedexInfo *info) {
    if (info->caught) {
        BmpWin_Transfer(info->labelWindows[0]);
    } else {
        BmpWin_Transfer(info->labelWindows[1]);
    }
}

static void PokedexInfo_LoadSprite(PokedexInfo *info, int x, u16 y, u32 slot) {
    PokedexInfo_LoadSpriteResources(info, slot);
    PokedexInfo_CreateSpriteActor(info, x, y, slot);
    PokedexInfo_HideOtherSprites(info);
}

static void PokedexInfo_UnloadSprite(PokedexInfo *info, u32 slot) {
    if (info->sprites[slot] != NULL) {
        PokedexInfo_DeleteSpriteActor(info, slot);
        PokedexInfo_FreeSpriteResources(info, slot);
        info->sprites[slot] = NULL;
    }
}

static void PokedexInfo_LoadSpriteResources(PokedexInfo *info, u32 slot) {
    BOOL sub = FALSE;
    ArcTool *arc;

    if (info->engine != 0) {
        sub = TRUE;
    }
    arc = info->arcs[POKEDEX_INFO_ARC_POKEGRA];

    info->chars[slot] = PokeGra_LoadClActChars(arc, info->species, info->form, info->sex, info->rare, 0, FALSE,
                                               info->personality, sub, info->heapId);
    info->palettes[slot] = PokeGra_LoadClActPalette(arc, info->species, info->form, info->sex, info->rare, 0, FALSE,
                                                    sub, slot * 32, info->heapId);
    info->cellAnims[slot] =
        PokeGra_LoadClActCellAnims(info->species, info->form, info->sex, info->rare, 0, FALSE, 2, sub, info->heapId);
}

static void PokedexInfo_FreeSpriteResources(PokedexInfo *info, u32 slot) {
    ClActSys_FreePltt(info->palettes[slot]);
    ClActSys_FreeChars(info->chars[slot]);
    ClActSys_FreeCellAnims(info->cellAnims[slot]);
}

static void PokedexInfo_CreateSpriteActor(PokedexInfo *info, int x, u16 y, u32 slot) {
    BOOL sub = FALSE;
    ClActorSetup setup;

    if (info->engine != 0) {
        sub = TRUE;
    }

    MI_CpuFill8(&setup, 0, sizeof(setup));
    setup.x = x;
    setup.y = y;
    info->sprites[slot] = ClActor_Create(info->unit, info->chars[slot], info->palettes[slot], info->cellAnims[slot],
                                         &setup, sub, info->heapId);
    ClActor_SetPriority(info->sprites[slot], 0);
    if (info->mode == POKEDEX_INFO_MODE_DETAIL) {
        ClActor_SetObjMode(info->sprites[slot], 1);
    }
    if (PML_PersonalGetParam(info->personal, PERSONAL_NO_FLIP) == 0) {
        ClActor_SetFlip(info->sprites[slot], 1, TRUE);
    }
}

static void PokedexInfo_DeleteSpriteActor(PokedexInfo *info, u32 slot) {
    ClActor_Delete(info->sprites[slot]);
}

static void PokedexInfo_HideOtherSprites(PokedexInfo *info) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (i != info->spriteSlot && info->sprites[i] != NULL) {
            ClActor_SetVisible(info->sprites[i], FALSE);
        }
    }
}

static void PokedexInfo_InitTypeIcons(PokedexInfo *info) {
    ClActDrawType draw = info->engine == 0 ? CLACT_DRAW_MAIN : CLACT_DRAW_SUB;
    u32 surface = CLACT_SURFACE_MAIN;
    ArcTool *arc;
    ClActorSetup setup;
    u8 i;
    u8 j;

    if (info->engine != 0) {
        surface = CLACT_SURFACE_SUB;
    }
    arc = info->arcs[POKEDEX_INFO_ARC_APP_MENU_COMMON];
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            info->typeChars[i][j] = -1;
        }
    }
    info->typeSlot = 0;
    info->typePalette =
        ClActSys_LoadPlttRange(arc, AppMenuCommon_GetTypeIconPaletteFile(), draw, 0x40, 0, 3, info->heapId);
    info->typeCellAnims = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTypeIconCellFile(2),
                                                 AppMenuCommon_GetTypeIconAnimFile(2), info->heapId);

    if (info->mode == POKEDEX_INFO_MODE_DETAIL && info->engine == 0) {
        setup.x = 0;
        setup.y = 0;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 0;
        info->allTypesLoaded = TRUE;
        info->shownTypes[0] = TYPE_NULL;
        info->shownTypes[1] = TYPE_NULL;
        for (i = 0; i < TYPE_NULL; i++) {
            info->allTypeChars[i] =
                ClActSys_LoadChars(arc, AppMenuCommon_GetTypeIconCharFile(i), FALSE, draw, info->heapId);
            info->allTypeActors[i] = ClActor_Create(info->unit, info->allTypeChars[i], info->typePalette,
                                                    info->typeCellAnims, &setup, surface, info->heapId);
            ClActor_SetPalette(info->allTypeActors[i], AppMenuCommon_GetTypeIconPaletteIndex(i), TRUE);
            ClActor_SetPriority(info->allTypeActors[i], 2);
            ClActor_SetObjMode(info->allTypeActors[i], 1);
            ClActor_SetVisible(info->allTypeActors[i], FALSE);
        }
    } else {
        info->allTypesLoaded = FALSE;
    }
}

static void PokedexInfo_FreeTypeIcons(PokedexInfo *info) {
    u8 i;

    for (i = 0; i < 2; i++) {
        PokedexInfo_FreeTypeActors(info, i);
    }
    if (info->allTypesLoaded) {
        for (i = 0; i < TYPE_NULL; i++) {
            ClActor_Delete(info->allTypeActors[i]);
            ClActSys_FreeChars(info->allTypeChars[i]);
        }
        info->allTypesLoaded = FALSE;
    }
    ClActSys_FreeCellAnims(info->typeCellAnims);
    ClActSys_FreePltt(info->typePalette);
}

// Shows the Pokémon's types in the slot, in the game's language or in another one
static void PokedexInfo_ShowTypes(PokedexInfo *info, u32 language, u32 slot) {
    PersonalData *personal = info->personal;
    u8 type1 = PML_PersonalGetParam(personal, PERSONAL_TYPE_1);
    u8 type2 = PML_PersonalGetParam(personal, PERSONAL_TYPE_2);

    if (type1 == type2) {
        type2 = TYPE_NULL;
    }
    if (language == POKEDEX_INFO_LANGUAGE_GAME) {
        PokedexInfo_ShowTypesInGameLanguage(info, type1, type2, slot);
    } else {
        PokedexInfo_ShowTypesInLanguage(info, type1, type2, language, slot);
    }
    PokedexInfo_HideOtherTypes(info);
}

static void PokedexInfo_ShowTypesInGameLanguage(PokedexInfo *info, u32 type1, u32 type2, u32 slot) {
    if (info->allTypesLoaded) {
        BOOL sub = FALSE;
        ClActorPos pos[2];

        if (info->engine != 0) {
            sub = TRUE;
        }
        pos[0].x = 168;
        pos[0].y = info->scrollY + 84;
        pos[1].x = 208;
        pos[1].y = info->scrollY + 84;

        if (info->shownTypes[0] != TYPE_NULL) {
            if (info->shownTypes[0] != type1 && info->shownTypes[0] != type2) {
                ClActor_SetVisible(info->allTypeActors[info->shownTypes[0]], FALSE);
                info->shownTypes[0] = TYPE_NULL;
            } else {
                if (info->shownTypes[0] == type2) {
                    ClActor_SetSurfacePos(info->allTypeActors[info->shownTypes[0]], &pos[1], sub);
                }
                ClActor_SetVisible(info->allTypeActors[info->shownTypes[0]], TRUE);
            }
        }
        if (info->shownTypes[1] != TYPE_NULL) {
            if (info->shownTypes[1] != type1 && info->shownTypes[1] != type2) {
                ClActor_SetVisible(info->allTypeActors[info->shownTypes[1]], FALSE);
                info->shownTypes[1] = TYPE_NULL;
            } else {
                if (info->shownTypes[1] == type1) {
                    ClActor_SetSurfacePos(info->allTypeActors[info->shownTypes[1]], &pos[0], sub);
                }
                ClActor_SetVisible(info->allTypeActors[info->shownTypes[1]], TRUE);
            }
        }
        if (type1 != TYPE_NULL) {
            info->shownTypes[0] = type1;
            ClActor_SetVisible(info->allTypeActors[info->shownTypes[0]], TRUE);
            ClActor_SetSurfacePos(info->allTypeActors[info->shownTypes[0]], &pos[0], sub);
        }
        if (type2 != TYPE_NULL) {
            info->shownTypes[1] = type2;
            ClActor_SetVisible(info->allTypeActors[info->shownTypes[1]], TRUE);
            ClActor_SetSurfacePos(info->allTypeActors[info->shownTypes[1]], &pos[1], sub);
        }
    } else {
        ArcTool *arc;
        ClActorSetup setups[2] = { { 0 }, { 0 } };
        u32 draw = CLACT_DRAW_MAIN;
        u8 types[2];
        ClActSurface surface;
        int i;

        if (info->engine != 0) {
            draw = CLACT_DRAW_SUB;
        }
        surface = info->engine == 0 ? CLACT_SURFACE_MAIN : CLACT_SURFACE_SUB;
        types[0] = type1;
        types[1] = type2;
        setups[0].x = 168;
        setups[0].y = info->scrollY + 84;
        setups[1].x = 208;
        setups[1].y = info->scrollY + 84;
        arc = info->arcs[POKEDEX_INFO_ARC_APP_MENU_COMMON];

        for (i = 0; i < 2; i++) {
            info->typeChars[slot][i] = -1;
            if (types[i] != TYPE_NULL) {
                info->typeChars[slot][i] =
                    ClActSys_LoadChars(arc, AppMenuCommon_GetTypeIconCharFile(types[i]), FALSE, draw, info->heapId);
            }
        }
        for (i = 0; i < 2; i++) {
            if (info->typeChars[slot][i] != -1) {
                info->typeActors[slot][i] = ClActor_Create(info->unit, info->typeChars[slot][i], info->typePalette,
                                                           info->typeCellAnims, &setups[i], surface, info->heapId);
                ClActor_SetPalette(info->typeActors[slot][i], AppMenuCommon_GetTypeIconPaletteIndex(types[i]), TRUE);
                ClActor_SetPriority(info->typeActors[slot][i], 2);
                ClActor_SetObjMode(info->typeActors[slot][i], 1);
            }
        }
    }
}

static void PokedexInfo_ShowTypesInLanguage(PokedexInfo *info, u32 type1, u32 type2, u32 language, u32 slot) {
    ArcTool *arc;
    ClActorSetup setups[2] = { { 0 }, { 0 } };
    u8 types[2];
    NNSG2dCharacterData *character;
    void *file;
    u8 *tiles;
    int i;
    u32 column;
    u32 row;
    u8 type;
    u32 draw = CLACT_DRAW_MAIN;
    ClActSurface surface;

    if (info->engine != 0) {
        draw = CLACT_DRAW_SUB;
    }
    surface = info->engine == 0 ? CLACT_SURFACE_MAIN : CLACT_SURFACE_SUB;
    types[0] = type1;
    types[1] = type2;
    setups[0].x = 168;
    setups[0].y = info->scrollY + 84;
    setups[1].x = 208;
    setups[1].y = info->scrollY + 84;
    arc = info->arcs[POKEDEX_INFO_ARC_APP_MENU_COMMON];

    file = GFL_G2DIOReadBGNCGRArc(info->arcs[POKEDEX_INFO_ARC_POKEDEX_GRA], sTypeIconFiles[language], FALSE, &character,
                                  info->heapId);
    tiles = character->pRawData;
    for (i = 0; i < 2; i++) {
        info->typeChars[slot][i] = -1;
        type = types[i];
        if (type != TYPE_NULL) {
            info->typeChars[slot][i] =
                ClActSys_LoadChars(arc, AppMenuCommon_GetTypeIconCharFile(type), FALSE, draw, info->heapId);
            column = sTypeIconTiles[type][1] << 7;
            row = sTypeIconTiles[type][0] << 11;
            ClActSys_WriteCharData(info->typeChars[slot][i], &tiles[row + column], 0x80, 0, draw);
            ClActSys_WriteCharData(info->typeChars[slot][i], &tiles[row + 0x400 + column], 0x80, 0x80, draw);
        }
    }
    GFL_HeapFree(file);

    for (i = 0; i < 2; i++) {
        if (info->typeChars[slot][i] != -1) {
            info->typeActors[slot][i] = ClActor_Create(info->unit, info->typeChars[slot][i], info->typePalette,
                                                       info->typeCellAnims, &setups[i], surface, info->heapId);
            ClActor_SetPalette(info->typeActors[slot][i], AppMenuCommon_GetTypeIconPaletteIndex(types[i]), TRUE);
            ClActor_SetPriority(info->typeActors[slot][i], 2);
            ClActor_SetObjMode(info->typeActors[slot][i], 1);
        }
    }
}

static void PokedexInfo_FreeTypeActors(PokedexInfo *info, u32 slot) {
    int i;

    if (!info->allTypesLoaded) {
        for (i = 0; i < 2; i++) {
            if (info->typeChars[slot][i] != -1) {
                ClActor_Delete(info->typeActors[slot][i]);
            }
        }
        for (i = 0; i < 2; i++) {
            if (info->typeChars[slot][i] != -1) {
                ClActSys_FreeChars(info->typeChars[slot][i]);
                info->typeChars[slot][i] = -1;
            }
        }
    }
}

static void PokedexInfo_HideOtherTypes(PokedexInfo *info) {
    u8 slot;
    u8 i;

    if (!info->allTypesLoaded) {
        for (slot = 0; slot < 2; slot++) {
            if (slot != info->typeSlot) {
                for (i = 0; i < 2; i++) {
                    if (info->typeChars[slot][i] != -1) {
                        ClActor_SetVisible(info->typeActors[slot][i], FALSE);
                    }
                }
            }
        }
    }
}

static void PokedexInfo_InitFootprints(PokedexInfo *info) {
    BOOL sub;
    u8 i;
    ArcTool *arc;

    for (i = 0; i < 2; i++) {
        info->footprints[i] = NULL;
    }
    info->footprintSlot = 0;
    sub = FALSE;
    if (info->engine != 0) {
        sub = TRUE;
    }
    arc = info->arcs[POKEDEX_INFO_ARC_FOOTPRINT];
    info->footprintPalette =
        ClActSys_LoadPlttRange(arc, PokedexInfo_GetFootprintPaletteFile(), sub, 0xa0, 0, 1, info->heapId);
    info->footprintCellAnimsOld = ClActSys_LoadCellAnims(arc, PokedexInfo_GetFootprintCellFile(0),
                                                         PokedexInfo_GetFootprintAnimFile(0), info->heapId);
    info->footprintCellAnims =
        ClActSys_LoadCellAnims(arc, PokedexInfo_GetFootprintCellFile(SPECIES_ARCEUS + 1),
                               PokedexInfo_GetFootprintAnimFile(SPECIES_ARCEUS + 1), info->heapId);
}

static void PokedexInfo_FreeFootprints(PokedexInfo *info) {
    u8 i;

    for (i = 0; i < 2; i++) {
        PokedexInfo_UnloadFootprint(info, i);
    }
    ClActSys_FreeCellAnims(info->footprintCellAnims);
    ClActSys_FreeCellAnims(info->footprintCellAnimsOld);
    ClActSys_FreePltt(info->footprintPalette);
}

static void PokedexInfo_LoadFootprint(PokedexInfo *info, u32 species, u32 form, u32 slot) {
    u32 draw = CLACT_DRAW_MAIN;
    ClActSurface surface;
    ArcTool *arc;
    u32 formFile;
    ClActorSetup setup;

    if (info->engine != 0) {
        draw = CLACT_DRAW_SUB;
    }
    surface = info->engine == 0 ? CLACT_SURFACE_MAIN : CLACT_SURFACE_SUB;
    formFile = PokedexInfo_GetFootprintFormFile(species, form);
    if (formFile == 0xffff) {
        arc = info->arcs[POKEDEX_INFO_ARC_FOOTPRINT];
        info->footprintChars[slot] =
            ClActSys_LoadChars(arc, PokedexInfo_GetFootprintFile(species), TRUE, draw, info->heapId);
    } else {
        info->footprintChars[slot] =
            ClActSys_LoadChars(info->arcs[POKEDEX_INFO_ARC_FOOTPRINT_FORM], formFile, TRUE, draw, info->heapId);
    }

    setup.x = 120;
    setup.y = info->scrollY + 88;
    setup.sequence = 0;
    setup.priority = 1;
    setup.bgPriority = 0;
    if (species <= SPECIES_ARCEUS && formFile != 0xffff) {
        info->footprints[slot] = ClActor_Create(info->unit, info->footprintChars[slot], info->footprintPalette,
                                                info->footprintCellAnimsOld, &setup, surface, info->heapId);
    } else {
        info->footprints[slot] = ClActor_Create(info->unit, info->footprintChars[slot], info->footprintPalette,
                                                info->footprintCellAnims, &setup, surface, info->heapId);
    }
    ClActor_SetPriority(info->footprints[slot], 1);
    ClActor_SetObjMode(info->footprints[slot], 1);
    PokedexInfo_HideOtherFootprints(info);
}

static void PokedexInfo_UnloadFootprint(PokedexInfo *info, u32 slot) {
    if (info->footprints[slot] != NULL) {
        ClActor_Delete(info->footprints[slot]);
        ClActSys_FreeChars(info->footprintChars[slot]);
        info->footprints[slot] = NULL;
    }
}

static void PokedexInfo_HideOtherFootprints(PokedexInfo *info) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (i != info->footprintSlot && info->footprints[i] != NULL) {
            ClActor_SetVisible(info->footprints[i], FALSE);
        }
    }
}

static void PokedexInfo_ShowEntry(PokedexInfo *info) {
    PokedexInfo_ShowTypes(info, POKEDEX_INFO_LANGUAGE_GAME, info->typeSlot);
    PokedexInfo_LoadFootprint(info, info->species, info->form, info->footprintSlot);
    PokedexInfo_SetFootprintsVisible(info, info->caught);
    PokedexInfo_PrintEntry(info);
}

static void PokedexInfo_ShowEntryInLanguage(PokedexInfo *info, u32 language) {
    PokedexInfo_ShowTypes(info, language, info->typeSlot);
    PokedexInfo_LoadFootprint(info, info->species, info->form, info->footprintSlot);
    PokedexInfo_SetFootprintsVisible(info, info->caught);
    PokedexInfo_PrintEntryInLanguage(info, language);
}

// The footprint's frame glows: a few of the palette's colors go back and forth between two sets
static void PokedexInfo_UpdateFootprintGlow(PokedexInfo *info) {
    s16 ratio;
    u8 i;

    if (info->glowPhase + 0x400 >= 0x10000) {
        info->glowPhase = info->glowPhase + 0x400 - 0x10000;
    } else {
        info->glowPhase += 0x400;
    }
    ratio = (FX_CosIdx(info->glowPhase) + 0x1000) / 2;

    for (i = 0; i < 5; i++) {
        u16 a = info->glowColorsA[i];
        u16 b = info->glowColorsB[i];
        u8 aR = a & 0x1f;
        u8 aG = (a & 0x3e0) >> 5;
        u8 aB = (a & 0x7c00) >> 10;
        u8 bR = b & 0x1f;
        u8 bG = (b & 0x3e0) >> 5;
        u8 bB = (b & 0x7c00) >> 10;
        u8 r = aR + ((bR - aR) * ratio >> 12);
        u8 g = aG + ((bG - aG) * ratio >> 12);
        u8 blue = aB + ((bB - aB) * ratio >> 12);

        info->glowColors[i] = r | (g << 5) | (blue << 10);
    }
    NNS_GfdRegisterNewVramTransferTask(info->engine == 0 ? NNS_GFD_DST_2D_BG_PLTT_MAIN : NNS_GFD_DST_2D_BG_PLTT_SUB,
                                       0x22, info->glowColors, sizeof(info->glowColors));
}

static void PokedexInfo_SetFootprintsVisible(PokedexInfo *info, BOOL visible) {
    u8 i;

    ClActor_SetVisible(info->footprints[info->footprintSlot], visible);
    if (info->allTypesLoaded) {
        if (info->shownTypes[0] != TYPE_NULL) {
            ClActor_SetVisible(info->allTypeActors[info->shownTypes[0]], visible);
        }
        if (info->shownTypes[1] != TYPE_NULL) {
            ClActor_SetVisible(info->allTypeActors[info->shownTypes[1]], visible);
        }
    } else {
        for (i = 0; i < 2; i++) {
            if (info->typeChars[info->typeSlot][i] != -1) {
                ClActor_SetVisible(info->typeActors[info->typeSlot][i], visible);
            }
        }
    }
}

static void PokedexInfo_SetVisible(PokedexInfo *info, BOOL visible) {
    ClActor_SetVisible(info->sprites[info->spriteSlot], visible);
    if (!info->caught) {
        visible = FALSE;
    }
    PokedexInfo_SetFootprintsVisible(info, visible);
}

static void PokedexInfo_LoadResources(PokedexInfo *info) {
    u8 i;

    for (i = 0; i < POKEDEX_INFO_ARC_COUNT; i++) {
        switch (i) {
        case POKEDEX_INFO_ARC_APP_MENU_COMMON:
            info->arcs[i] = GFL_ArcSysCreateFileHandle(getUINarcIdx(), info->heapId);
            break;
        case POKEDEX_INFO_ARC_POKEGRA:
            info->arcs[i] = MakePokeGraArcHandle(info->heapId);
            break;
        case POKEDEX_INFO_ARC_FOOTPRINT:
            info->arcs[i] = GFL_ArcSysCreateFileHandle(PokedexInfo_GetFootprintArcId(), info->heapId);
            break;
        case POKEDEX_INFO_ARC_FOOTPRINT_FORM:
            info->arcs[i] = GFL_ArcSysCreateFileHandle(PokedexInfo_GetFootprintFormArcId(), info->heapId);
            break;
        default:
            info->arcs[i] = GFL_ArcSysCreateFileHandle(sArcIds[i], info->heapId);
            break;
        }
    }

    for (i = 0; i < POKEDEX_INFO_MSG_COUNT; i++) {
        switch (i) {
        case POKEDEX_INFO_MSG_CATEGORIES:
            if (info->givenMsgs[0] != NULL) {
                info->msgs[i] = info->givenMsgs[0];
            } else {
                info->msgs[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sMsgBanks[i], info->heapId);
            }
            break;
        case POKEDEX_INFO_MSG_HEIGHTS:
            if (info->givenMsgs[1] != NULL) {
                info->msgs[i] = info->givenMsgs[1];
            } else {
                info->msgs[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sMsgBanks[i], info->heapId);
            }
            break;
        case POKEDEX_INFO_MSG_WEIGHTS:
            if (info->givenMsgs[2] != NULL) {
                info->msgs[i] = info->givenMsgs[2];
            } else {
                info->msgs[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sMsgBanks[i], info->heapId);
            }
            break;
        default:
            info->msgs[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, sMsgBanks[i], info->heapId);
            break;
        }
    }

    info->personal = PML_PersonalLoad(info->species, info->form, info->heapId);
    for (i = 0; i < POKEDEX_INFO_STRBUF_COUNT; i++) {
        info->labels[i] = GFL_MsgDataLoadStrbufNew(info->msgs[POKEDEX_INFO_MSG_LABELS], sLabelMessages[i]);
    }
}

static void PokedexInfo_FreeResources(PokedexInfo *info) {
    u8 i;

    for (i = 0; i < POKEDEX_INFO_STRBUF_COUNT; i++) {
        GFL_StrBufFree(info->labels[i]);
    }
    if (info->personal != NULL) {
        PML_PersonalFree(info->personal);
    }
    for (i = 0; i < POKEDEX_INFO_MSG_COUNT; i++) {
        switch (i) {
        case POKEDEX_INFO_MSG_CATEGORIES:
            if (info->givenMsgs[0] == NULL) {
                GFL_MsgDataFree(info->msgs[i]);
            }
            break;
        case POKEDEX_INFO_MSG_HEIGHTS:
            if (info->givenMsgs[1] == NULL) {
                GFL_MsgDataFree(info->msgs[i]);
            }
            break;
        case POKEDEX_INFO_MSG_WEIGHTS:
            if (info->givenMsgs[2] == NULL) {
                GFL_MsgDataFree(info->msgs[i]);
            }
            break;
        default:
            GFL_MsgDataFree(info->msgs[i]);
            break;
        }
    }
    for (i = 0; i < POKEDEX_INFO_ARC_COUNT; i++) {
        GFL_ArcToolFree(info->arcs[i]);
    }
}

static u32 PokedexInfo_GetFootprintArcId(void) {
    return ARCID_POKE_FOOTPRINT;
}

// The first files of the archive are not species'
static u32 PokedexInfo_GetFootprintFile(u32 species) {
    return species + 5;
}

static u32 PokedexInfo_GetFootprintPaletteFile(void) {
    return 0;
}

// The cells of the generations before the fifth are smaller than those of the fifth
static u32 PokedexInfo_GetFootprintCellFile(int species) {
    if (species <= SPECIES_ARCEUS) {
        return 2;
    }
    return 4;
}

static u32 PokedexInfo_GetFootprintAnimFile(int species) {
    if (species <= SPECIES_ARCEUS) {
        return 1;
    }
    return 3;
}

static u32 PokedexInfo_GetFootprintFormArcId(void) {
    return ARCID_POKE_FOOTPRINT_FORM;
}

// The file of a form that has a footprint of its own, or 0xffff
static u32 PokedexInfo_GetFootprintFormFile(u32 species, u32 form) {
    if (species == SPECIES_GIRATINA && form == 1) {
        return 0;
    }
    if (species == SPECIES_SHAYMIN && form == 1) {
        return 1;
    }
    if (species == SPECIES_KYUREM && form == 1) {
        return 2;
    }
    if (species == SPECIES_KYUREM && form == 2) {
        return 3;
    }
    if (species == SPECIES_KELDEO && form == 1) {
        return 4;
    }
    if (species == SPECIES_TORNADUS && form == 1) {
        return 5;
    }
    if (species == SPECIES_THUNDURUS && form == 1) {
        return 6;
    }
    if (species == SPECIES_LANDORUS && form == 1) {
        return 7;
    }
    return 0xffff;
}
