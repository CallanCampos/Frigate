#include "types.h"
#include "app/pokemon_trade/pokemontrade_2d.h"
#include "app/pokemon_trade/pokemontrade_message.h"
#include "app/pokemon_trade/pokemontrade_nego.h"
#include "app/pokemon_trade/pokemontrade_proc.h"
#include "app/pokemon_trade_local.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "pml/item.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"

// The trade's 2D: the backgrounds of both screens, the strip of boxes on the lower screen with the Pokémon icons in
// it, and the sprites of the panels

// The strip: the party's two columns of three, then each box's six columns of five, each column 24 pixels wide. On
// the background, the party takes 12 tiles and each box 20
#define PARTY_COLUMNS 2
#define BOX_COLUMNS 6
#define COLUMN_ROWS 5
#define PARTY_TILES 12
#define BOX_TILES 20
// The most boxes there can be
#define MAX_BOXES 24
// The columns of icons that are set up at once, enough to cover the screen
#define ICON_COLUMNS 12

// The screen files of the strip's backgrounds: where the cells start, after the file's, the block's and the screen's
// headers, and the screens' width, in tiles
#define SCREEN_CELLS (0x24 / 2)
#define SCREEN_WIDTH 32

static u8 Poketrade2D_GetMonMarkingIcon(BoxPkm *pkm, HeapID heapId);
static void Poketrade2D_BuildBoxMarkingTable(PokemonTradeWork *wk, int box, u8 *colors);
static void Poketrade2D_BuildPartyMarkingTable(PokemonTradeWork *wk, PokeParty *party, u8 *colors);
static u16 Poketrade2D_GetBoxScreenEntry(int x, int y, PokemonTradeWork *wk, BOOL marked);
static BOOL Poketrade2D_HitTestBoxGrid(PokemonTradeWork *wk, int x, int y);
static void Poketrade2D_LoadBoxScreenData(PokemonTradeWork *wk, u32 chars);
static void Poketrade2D_FreeBoxScreenData(PokemonTradeWork *wk);
static void Poketrade2D_DeleteBoxIconRow(PokemonTradeWork *wk, int index);
static void Poketrade2D_GetGridCellPos(int column, int row, ClActorPos *pos);
static BoxPkm *Poketrade2D_ResolveGridCell(BoxSaveAccessor *boxes, int column, int row, PokemonTradeWork *wk,
                                           BOOL *isParty);
static TradeBoxEntry *Poketrade2D_GetGridCellMon(int column, int row, PokemonTradeWork *wk);
static BOOL Poketrade2D_IsBoxIconFiltered(PokemonTradeWork *wk, int species);
static void Poketrade2D_UpdateIconObjMode(PokemonTradeWork *wk, int column, int row, BoxPkm *pkm, BOOL isParty,
                                          int index);
static void Poketrade2D_CreateBoxIcon(PokemonTradeWork *wk, BoxSaveAccessor *boxes, int column, int index, BOOL async);
static void Poketrade2D_UpdateBoxIconPositions(PokemonTradeWork *wk);
static int Poketrade2D_WrapIndex12(int column);
static void Poketrade2D_HideAllBoxIcons(PokemonTradeWork *wk);
static void Poketrade2D_CreateCuredPokerusIcon(PokemonTradeWork *wk, int side, PartyPkm *pkm);
static void Poketrade2D_TaskSetMainPlaneVisible(TCB *tcb, void *data);
static void Poketrade2D_TaskSetSubPlaneVisible(TCB *tcb, void *data);
static void Poketrade2D_InitBezierCurve(TradeCurve *curve, VecFx32 *start, VecFx32 *control1, VecFx32 *control2,
                                        VecFx32 *end, int frames);
static BOOL Poketrade2D_StepBezierCurve(TradeCurve *curve);
static void Poketrade2D_Setup3DRenderState(void);
static void Poketrade2D_SetCursorSlidePositions(PokemonTradeWork *wk);
static void Poketrade2D_OnCursorAnimDone(u32 param, fx32 frame);
static void Poketrade2D_FadePaletteBank(const PokemonTradeWork *wk, u32 vram, u32 paletteMask);
static void Poketrade2D_FadePaletteCopy(PokemonTradeWork *wk, BOOL dim, u16 vram, u32 paletteMask);

// The cell actor systems of the trade demo, which has only a few sprites, and of the trade
static const ClActSysSetup sDemoClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 5, 5, 5, 5, 16, 16 };
static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 128, 128, 128, 128, 16, 16 };

static const Light sLights[] = {
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
    { { 0, -FX16_ONE, 0 }, GX_RGB(16, 16, 16) },
};

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_64_E,   GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_0123_H,
    GX_VRAM_OBJ_16_G,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_0_F,     GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The colour of a Pokémon's slot, an index into the box palette: none for an empty slot, the species' colour, or the
// colour of an egg, Manaphy's being different
static u8 Poketrade2D_GetMonMarkingIcon(BoxPkm *pkm, HeapID heapId) {
    u16 color = 0;
    BOOL encrypted = PML_PkmDecrypt(pkm);
    u32 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);

    if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
        if (!PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            u16 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
            PersonalData *personal = PML_PersonalLoad(species, form, heapId);
            color = PML_PersonalGetParam(personal, PERSONAL_COLOR);
            PML_PersonalFree(personal);
        } else {
            color = COLOR_BLUE;
            if (species != SPECIES_MANAPHY) {
                color = COLOR_WHITE;
            }
        }
        color++;
    }
    PML_PkmReEncrypt(pkm, encrypted);
    return color;
}

static void Poketrade2D_BuildBoxMarkingTable(PokemonTradeWork *wk, int box, u8 *colors) {
    u8 i;

    for (i = 0; i < 30; i++) {
        colors[i] = Poketrade2D_GetMonMarkingIcon(BoxSaveAccessor_GetPkm(wk->boxes, box, i), wk->heapId);
    }
}

static void Poketrade2D_BuildPartyMarkingTable(PokemonTradeWork *wk, PokeParty *party, u8 *colors) {
    int count = PokeParty_GetPkmCount(party);
    u8 i;

    for (i = 0; i < 6; i++) {
        if (count > i) {
            colors[i] = Poketrade2D_GetMonMarkingIcon(PokeParty_GetBoxPkm(PokeParty_GetPkm(party, i)), wk->heapId);
        } else {
            colors[i] = 0;
        }
    }
}

// Works out the colours of this machine's box, one a frame, and the party's after the last box; TRUE when that is
// done
BOOL Poketrade2D_BuildMarkingTablesFrom(PokemonTradeWork *wk, int box) {
    // One box a frame: the loop stops after its first pass, as the original's does
    while (box < wk->boxCount + 1) {
        if (box == wk->boxCount) {
            Poketrade2D_BuildPartyMarkingTable(wk, wk->party, wk->boxColors[0].party);
            return TRUE;
        }
        Poketrade2D_BuildBoxMarkingTable(wk, box, wk->boxColors[0].boxes[box]);
        break;
    }
    return FALSE;
}

void Poketrade2D_InitClactSystemMain(PokemonTradeWork *wk) {
    ClActSys_Create(&sDemoClActSetup, &sVRAMConfig, wk->heapId);
}

void Poketrade2D_InitClactSystemSub(PokemonTradeWork *wk) {
    ClActSys_Create(&sClActSetup, &sVRAMConfig, wk->heapId);
}

void Poketrade2D_LoadBackgroundGfx(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);
    int screen;

    if (PokemonTrade_IsNegoType(wk)) {
        screen = 4;
    } else {
        screen = 11;
    }
    GFL_G2DIOLoadArcNCLRDefault(arc, 9, 0, 0, 0, wk->heapId);
    if (wk->bg2Chars == 0) {
        wk->bg2Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 10, 2, 0, FALSE, wk->heapId);
    }
    GFL_G2DIOLoadNSCRSync(arc, screen, 2, 0, CHAR_POS(wk->bg2Chars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
}

void Poketrade2D_FreeBackgroundPalette(PokemonTradeWork *wk) {
    if (wk->bg2Chars != 0) {
        GFL_BGSysFreeCharMemory(2, CHAR_POS(wk->bg2Chars), CHAR_SIZE(wk->bg2Chars));
    }
    wk->bg2Chars = 0;
}

void Poketrade2D_LoadSubBackgroundGfx(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);

    GFL_G2DIOLoadNSCRSync(arc, 12, 4, 0, CHAR_POS(wk->bg5Chars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
}

void Poketrade2D_NoOp(PokemonTradeWork *wk) {
}

void Poketrade2D_LoadMainGfxAndMessageWindow(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 17, 4, 0, 0xc0, wk->heapId);
    wk->bg7Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 13, 7, 0, FALSE, wk->heapId);
    wk->bg5Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 13, 5, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    Poketrade2D_LoadBoxScreenData(wk, wk->bg7Chars);
}

void Poketrade2D_CreateFontResource(PokemonTradeWork *wk) {
    wk->cursorImage = LoadCursorImageEndOfHeap(6, 15, 0, wk->heapId);
}

void Poketrade2D_FreeMainResources(PokemonTradeWork *wk) {
    Poketrade2D_FreeBoxScreenData(wk);
    if (wk->bg5Chars != 0) {
        GFL_BGSysFreeCharMemory(5, CHAR_POS(wk->bg5Chars), CHAR_SIZE(wk->bg5Chars));
        wk->bg5Chars = 0;
    }
    if (wk->bg7Chars != 0) {
        GFL_BGSysFreeCharMemory(7, CHAR_POS(wk->bg7Chars), CHAR_SIZE(wk->bg7Chars));
        wk->bg7Chars = 0;
    }
    if (wk->cursorImage != 0) {
        GFL_BGSysFreeCharMemory(6, CHAR_POS(wk->cursorImage), CHAR_SIZE(wk->cursorImage));
        wk->cursorImage = 0;
    }
}

// Loads the common UI sprites once
void Poketrade2D_RegisterIconCharResources(PokemonTradeWork *wk) {
    if (wk->objRes[TRADE_OBJRES_PLTT_COMMON] == 0) {
        ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);

        wk->objRes[TRADE_OBJRES_CHAR_COMMON] =
            ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_SUB, wk->heapId);
        wk->objRes[TRADE_OBJRES_PLTT_COMMON] =
            ClActSys_LoadPlttRange(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_SUB, 0, 0, 3, wk->heapId);
        wk->objRes[TRADE_OBJRES_CELL_COMMON] = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(2),
                                                                      AppMenuCommon_GetTouchBarAnimFile(2), wk->heapId);
        GFL_ArcToolFree(arc);
    }
}

void Poketrade2D_LoadSummaryBackground(PokemonTradeWork *wk, int side, int page) {
    u32 screens[] = { 27, 28 };
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);

    GFL_G2DIOLoadNSCRSync(arc, screens[page], 4, 0, CHAR_POS(wk->bg5Chars), 0, FALSE, wk->heapId);
    if (side) {
        GFL_BGSysMoveBG(4, BG_MOVE_SET_X, 128);
    } else {
        GFL_BGSysMoveBG(4, BG_MOVE_SET_X, 0);
    }
    ClActor_SetVisible(wk->actors[2], FALSE);
    GFL_ArcToolFree(arc);
    GFL_BGSysSetBGEnabled(4, TRUE);
}

void Poketrade2D_ClearSummaryBackground(PokemonTradeWork *wk) {
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_X, 0);
    Poketrade2D_SetupSubBlend(wk);
    GFL_BGSysClearScr(4);
    GFL_BGSysQueueScrLoad(4);
}

// The cell of the strip's background at a tile, the strip repeating every stripTileWidth tiles
static u16 Poketrade2D_GetBoxScreenEntry(int x, int y, PokemonTradeWork *wk, BOOL marked) {
    if (x >= wk->stripTileWidth) {
        x -= wk->stripTileWidth;
    }
    if (x < PARTY_TILES) {
        if (marked) {
            return wk->stripScreens[3][SCREEN_CELLS + y * SCREEN_WIDTH + x];
        }
        return wk->stripScreens[1][SCREEN_CELLS + y * SCREEN_WIDTH + x];
    }
    x = (x - PARTY_TILES) % BOX_TILES;
    if (marked) {
        return wk->stripScreens[2][SCREEN_CELLS + y * SCREEN_WIDTH + x];
    }
    return wk->stripScreens[0][SCREEN_CELLS + y * SCREEN_WIDTH + x];
}

// Whether a tile of the screen is under a marked icon
static BOOL Poketrade2D_HitTestBoxGrid(PokemonTradeWork *wk, int x, int y) {
    int i, j;

    for (i = 0; i < ICON_COLUMNS; i++) {
        for (j = 0; j < COLUMN_ROWS; j++) {
            if (wk->iconMarked[i][j]) {
                int left = wk->iconPos[i][j].x - 5;
                int top = wk->iconPos[i][j].y - 8;

                if (left < 0) {
                    left -= 8;
                }
                left /= 8;
                top /= 8;
                if (left <= x && left + 3 > x && top <= y && top + 3 > y) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// Draws the strip's background at its scroll, with the marked icons' slots lit
void Poketrade2D_DrawBoxGridScreen(PokemonTradeWork *wk) {
    int scroll = wk->scrollX / 8;
    int x, y;

    Poketrade2D_GetScrollColumn(wk);
    for (y = 0; y < 24; y++) {
        for (x = 0; x < 52; x++) {
            GFL_BGSysSetScrTile(7, x, y,
                                Poketrade2D_GetBoxScreenEntry(scroll + x, y, wk, Poketrade2D_HitTestBoxGrid(wk, x, y)));
        }
    }
    wk->unk108C = wk->scrollX % 8;
    wk->unk1084 = 1;
    GFL_BGSysQueueScrLoad(7);
}

// Reads the strip's screens and moves their cells to the characters where they were loaded
static void Poketrade2D_LoadBoxScreenData(PokemonTradeWork *wk, u32 chars) {
    int i;

    wk->stripScreens[0] = GFL_ArcSysReadHeapNew(ARCID_POKEMONTRADE, 14, wk->heapId);
    wk->stripScreens[1] = GFL_ArcSysReadHeapNew(ARCID_POKEMONTRADE, 15, wk->heapId);
    wk->stripScreens[2] = GFL_ArcSysReadHeapNew(ARCID_POKEMONTRADE, 25, wk->heapId);
    wk->stripScreens[3] = GFL_ArcSysReadHeapNew(ARCID_POKEMONTRADE, 26, wk->heapId);
    for (i = 0; i < SCREEN_WIDTH * 24; i++) {
        wk->stripScreens[0][SCREEN_CELLS + i] += CHAR_POS(chars);
        wk->stripScreens[1][SCREEN_CELLS + i] += CHAR_POS(chars);
        wk->stripScreens[2][SCREEN_CELLS + i] += CHAR_POS(chars);
        wk->stripScreens[3][SCREEN_CELLS + i] += CHAR_POS(chars);
    }
    wk->scrollX = wk->stripWidth - 80;
    Poketrade2D_DrawBoxGridScreen(wk);
}

static void Poketrade2D_FreeBoxScreenData(PokemonTradeWork *wk) {
    if (wk->stripScreens[0] != NULL) {
        GFL_HeapFree(wk->stripScreens[0]);
        wk->stripScreens[0] = NULL;
        GFL_HeapFree(wk->stripScreens[1]);
        wk->stripScreens[1] = NULL;
        GFL_HeapFree(wk->stripScreens[2]);
        wk->stripScreens[2] = NULL;
        GFL_HeapFree(wk->stripScreens[3]);
        wk->stripScreens[3] = NULL;
    }
}

// Draws the names of the boxes, and the party's, into their windows
void Poketrade2D_CreateBoxWindows(PokemonTradeWork *wk) {
    int i;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    for (i = 0; i < wk->boxCount + 1; i++) {
        if (wk->boxNameWindows[i] == NULL) {
            wk->boxNameWindows[i] = BmpWin_CreateDynamic(5, 0, 0, 15, 2, 14, FALSE);
        }
        if (i == wk->boxCount) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 16, wk->drawStr);
        } else {
            loadBoxNameToStrbuf(wk->boxes, i, wk->drawStr);
        }
        GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->boxNameWindows[i]), 0);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->boxNameWindows[i]), 0, 1, wk->drawStr, wk->font);
        BmpWin_FlushChar(wk->boxNameWindows[i]);
        BmpWin_SetPosY(wk->boxNameWindows[i], 0);
    }
}

void Poketrade2D_DeleteBoxWindows(PokemonTradeWork *wk) {
    int i;

    for (i = 0; i < MAX_BOXES + 1; i++) {
        if (wk->boxNameWindows[i] != NULL) {
            BmpWin_Free(wk->boxNameWindows[i]);
            wk->boxNameWindows[i] = NULL;
        }
    }
}

// Puts the names of the boxes on screen over the strip at its scroll
void Poketrade2D_DrawBoxNames(PokemonTradeWork *wk) {
    int scroll = wk->scrollX / 8;
    int x, y, i, box;

    for (y = 0; y < 2; y++) {
        for (x = 0; x < 64; x++) {
            GFL_BGSysSetScrTile(5, x, y, 0);
        }
    }
    if (wk->scrollX < 96) {
        box = wk->boxCount;
    } else {
        box = (wk->scrollX - 96) / 160;
    }
    for (i = 0; i < 3; i++) {
        int name = box + i;

        if (name > wk->boxCount) {
            name = name - wk->boxCount - 1;
        } else if (name < 0) {
            name = name + wk->boxCount + 1;
        }
        if (name == wk->boxCount) {
            x = wk->stripTileWidth - scroll;
            if (scroll > x) {
                scroll -= wk->stripTileWidth;
            } else {
                x = -scroll;
            }
        } else {
            x = name * BOX_TILES + PARTY_TILES - scroll;
        }
        if (x >= -10 && x <= 33) {
            BmpWin_SetPosX(wk->boxNameWindows[name], x + 1);
            BmpWin_FlushMap(wk->boxNameWindows[name]);
        }
    }
    GFL_BGSysQueueScrLoad(5);
}

// Loads the icons' palette and cells, and sets up the icons of the strip's columns with their cursors, all hidden
void Poketrade2D_CreateBoxIconActors(PokemonTradeWork *wk) {
    ClActorSetup setup;
    int i, j;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);

    wk->objRes[TRADE_OBJRES_PLTT_ICON] =
        ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0x60, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_ICON] =
        ClActSys_LoadCellAnims(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_MainEng(), wk->heapId);
    for (i = 0; i < ICON_COLUMNS; i++) {
        for (j = 0; j < COLUMN_ROWS; j++) {
            setup.x = 0;
            setup.y = 0;
            setup.sequence = 1;
            setup.priority = 16;
            setup.bgPriority = 3;
            wk->iconChars[i][j] = ClActSys_LoadChars(arc, 0x55e, FALSE, CLACT_VRAM_SUB, wk->heapId);
            wk->icons[i][j] = ClActor_Create(wk->clactUnit, wk->iconChars[i][j], wk->objRes[TRADE_OBJRES_PLTT_ICON],
                                             wk->objRes[TRADE_OBJRES_CELL_ICON], &setup, CLACT_VRAM_SUB, wk->heapId);
            setup.sequence = 0;
            setup.priority = 15;
            wk->iconCursors[i][j] =
                ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                               wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
            ClActor_SetAutoAnim(wk->icons[i][j], FALSE);
            ClActor_SetVisible(wk->icons[i][j], FALSE);
            ClActor_SetVisible(wk->iconCursors[i][j], FALSE);
            ClActor_SetAutoAnim(wk->iconCursors[i][j], TRUE);
        }
    }
    GFL_ArcToolFree(arc);
}

static void Poketrade2D_DeleteBoxIconRow(PokemonTradeWork *wk, int index) {
    int i;

    for (i = 0; i < COLUMN_ROWS; i++) {
        if (wk->icons[index][i] != NULL) {
            ClActor_Delete(wk->icons[index][i]);
            wk->icons[index][i] = NULL;
        }
        if (wk->iconCursors[index][i] != NULL) {
            ClActor_Delete(wk->iconCursors[index][i]);
            wk->iconCursors[index][i] = NULL;
        }
        if (wk->iconChars[index][i] != 0) {
            ClActSys_FreeChars(wk->iconChars[index][i]);
            wk->iconChars[index][i] = 0;
        }
    }
}

void Poketrade2D_DeleteAllBoxIcons(PokemonTradeWork *wk) {
    int i;

    Poketrade2D_DeletePickedIconActor(wk);
    for (i = 0; i < ICON_COLUMNS; i++) {
        Poketrade2D_DeleteBoxIconRow(wk, i);
    }
    if (wk->objRes[TRADE_OBJRES_PLTT_ICON] != 0) {
        ClActSys_FreePltt(wk->objRes[TRADE_OBJRES_PLTT_ICON]);
        wk->objRes[TRADE_OBJRES_PLTT_ICON] = 0;
    }
    if (wk->objRes[TRADE_OBJRES_CELL_ICON] != 0) {
        ClActSys_FreeCellAnims(wk->objRes[TRADE_OBJRES_CELL_ICON]);
        wk->objRes[TRADE_OBJRES_CELL_ICON] = 0;
    }
    if (wk->iconCharData != NULL) {
        GFL_HeapFree(wk->iconCharData);
    }
    wk->iconCharData = NULL;
}

void Poketrade2D_DeleteOfferIcons(PokemonTradeWork *wk) {
    int i;

    Poketrade2D_DeleteCursorActor(wk);
    PoketradeSave_DeleteOfferStatusIcons(wk);
    for (i = 0; i < 10; i++) {
        if (wk->actors[i] != NULL) {
            ClActor_Delete(wk->actors[i]);
            wk->actors[i] = NULL;
        }
    }
    for (i = TRADE_OBJRES_PLTT; i < TRADE_OBJRES_CHAR; i++) {
        if (wk->objRes[i] != 0) {
            ClActSys_FreePltt(wk->objRes[i]);
            wk->objRes[i] = 0;
        }
    }
    for (; i < TRADE_OBJRES_CELL; i++) {
        if (wk->objRes[i] != 0) {
            ClActSys_FreeChars(wk->objRes[i]);
            wk->objRes[i] = 0;
        }
    }
    for (; i < TRADE_OBJRES_COUNT; i++) {
        if (wk->objRes[i] != 0) {
            ClActSys_FreeCellAnims(wk->objRes[i]);
            wk->objRes[i] = 0;
        }
    }
}

static void Poketrade2D_GetGridCellPos(int column, int row, ClActorPos *pos) {
    // A column's width and a row's height, and where the first row is. They are the first bytes of the file's
    // .rodata, though their reads fold
    static const u8 width = 24;
    static const u8 height = 24;
    static const u8 top = 72;

    pos->x = (column + 1) * width;
    pos->y = row * height + top;
}

// The Pokémon in a column's row of the strip, and whether it is in the party
static BoxPkm *Poketrade2D_ResolveGridCell(BoxSaveAccessor *boxes, int column, int row, PokemonTradeWork *wk,
                                           BOOL *isParty) {
    if (column >= PARTY_COLUMNS) {
        int box = PokemonTrade_GetColumnBox(column, wk);
        int slot = PokemonTrade_GetColumnSlot(column, row);

        *isParty = FALSE;
        return PokemonTrade_GetBoxPkm(boxes, box, slot, wk);
    }
    if (row < 3) {
        *isParty = TRUE;
        return PokemonTrade_GetBoxPkm(boxes, wk->boxCount, column + row * 2, wk);
    }
    return NULL;
}

static TradeBoxEntry *Poketrade2D_GetGridCellMon(int column, int row, PokemonTradeWork *wk) {
    if (column >= PARTY_COLUMNS) {
        int box = PokemonTrade_GetColumnBox(column, wk);

        return PokemonTrade_GetBoxEntry(box, PokemonTrade_GetColumnSlot(column, row), wk);
    }
    if (row < 3) {
        return PokemonTrade_GetBoxEntry(wk->boxCount, column + row * 2, wk);
    }
    return NULL;
}

// Takes the place of the cursor shown under an icon as where the stylus is
void Poketrade2D_PlaceBoxIconActors(PokemonTradeWork *wk) {
    int i, j;

    for (i = 0; i < COLUMN_ROWS; i++) {
        for (j = 0; j < ICON_COLUMNS; j++) {
            if (ClActor_IsVisible(wk->iconCursors[j][i])) {
                ClActorPos pos;

                ClActor_GetSurfacePos(wk->iconCursors[j][i], &pos, CLACT_VRAM_SUB);
                wk->touchX = pos.x;
                wk->touchY = pos.y;
            }
        }
    }
}

// The initial of each species' English name, from 0 for A, which the search by initial looks for. The two after the
// last species are the egg's
static u8 sSpeciesInitials[] = {
    0xff, 0x01, 0x08, 0x15, 0x02, 0x02, 0x02, 0x12, 0x16, 0x01, 0x02, 0x0c, 0x01, 0x16, 0x0a, 0x01, 0x0f, 0x0f, 0x0f,
    0x11, 0x11, 0x12, 0x05, 0x04, 0x00, 0x0f, 0x11, 0x12, 0x12, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x02, 0x02, 0x15,
    0x0d, 0x09, 0x16, 0x19, 0x06, 0x0e, 0x06, 0x15, 0x0f, 0x0f, 0x15, 0x15, 0x03, 0x03, 0x0c, 0x0f, 0x0f, 0x06, 0x0c,
    0x0f, 0x06, 0x00, 0x0f, 0x0f, 0x0f, 0x00, 0x0a, 0x00, 0x0c, 0x0c, 0x0c, 0x01, 0x16, 0x15, 0x13, 0x13, 0x06, 0x06,
    0x06, 0x0f, 0x11, 0x12, 0x12, 0x0c, 0x0c, 0x05, 0x03, 0x03, 0x12, 0x03, 0x06, 0x0c, 0x12, 0x02, 0x06, 0x07, 0x06,
    0x0e, 0x03, 0x07, 0x0a, 0x0a, 0x15, 0x04, 0x04, 0x04, 0x02, 0x0c, 0x07, 0x07, 0x0b, 0x0a, 0x16, 0x11, 0x11, 0x02,
    0x13, 0x0a, 0x07, 0x12, 0x06, 0x12, 0x12, 0x12, 0x0c, 0x12, 0x09, 0x04, 0x0c, 0x0f, 0x13, 0x0c, 0x06, 0x0b, 0x03,
    0x04, 0x15, 0x09, 0x05, 0x0f, 0x0e, 0x0e, 0x0a, 0x0a, 0x00, 0x12, 0x00, 0x19, 0x0c, 0x03, 0x03, 0x03, 0x0c, 0x0c,
    0x02, 0x01, 0x0c, 0x02, 0x10, 0x13, 0x13, 0x02, 0x05, 0x12, 0x05, 0x07, 0x0d, 0x0b, 0x0b, 0x12, 0x00, 0x02, 0x02,
    0x0b, 0x0f, 0x02, 0x08, 0x13, 0x13, 0x0d, 0x17, 0x0c, 0x05, 0x00, 0x01, 0x0c, 0x00, 0x12, 0x0f, 0x07, 0x12, 0x09,
    0x00, 0x12, 0x12, 0x18, 0x16, 0x10, 0x04, 0x14, 0x0c, 0x12, 0x0c, 0x14, 0x16, 0x06, 0x0f, 0x05, 0x03, 0x06, 0x12,
    0x12, 0x06, 0x10, 0x12, 0x12, 0x07, 0x12, 0x13, 0x14, 0x12, 0x0c, 0x12, 0x0f, 0x02, 0x11, 0x0e, 0x03, 0x0c, 0x12,
    0x07, 0x07, 0x0a, 0x0f, 0x03, 0x0f, 0x12, 0x12, 0x13, 0x07, 0x12, 0x04, 0x0c, 0x0c, 0x01, 0x11, 0x04, 0x12, 0x0b,
    0x0f, 0x13, 0x0b, 0x07, 0x02, 0x13, 0x06, 0x12, 0x13, 0x02, 0x01, 0x0c, 0x0c, 0x12, 0x0f, 0x0c, 0x19, 0x0b, 0x16,
    0x12, 0x01, 0x02, 0x03, 0x0b, 0x0b, 0x0b, 0x12, 0x0d, 0x12, 0x13, 0x12, 0x16, 0x0f, 0x11, 0x0a, 0x06, 0x12, 0x0c,
    0x12, 0x01, 0x12, 0x15, 0x12, 0x0d, 0x0d, 0x12, 0x16, 0x0b, 0x04, 0x0c, 0x07, 0x00, 0x0d, 0x12, 0x03, 0x12, 0x0c,
    0x00, 0x0b, 0x00, 0x0c, 0x0c, 0x04, 0x0c, 0x0f, 0x0c, 0x15, 0x08, 0x11, 0x06, 0x12, 0x02, 0x12, 0x16, 0x16, 0x0d,
    0x02, 0x13, 0x12, 0x06, 0x12, 0x13, 0x15, 0x05, 0x02, 0x02, 0x12, 0x00, 0x19, 0x12, 0x0b, 0x12, 0x01, 0x16, 0x02,
    0x02, 0x01, 0x02, 0x0b, 0x02, 0x00, 0x00, 0x05, 0x0c, 0x02, 0x0a, 0x12, 0x01, 0x03, 0x03, 0x13, 0x02, 0x00, 0x16,
    0x12, 0x06, 0x12, 0x12, 0x16, 0x02, 0x07, 0x06, 0x11, 0x0b, 0x01, 0x12, 0x12, 0x01, 0x0c, 0x0c, 0x11, 0x11, 0x11,
    0x0b, 0x0b, 0x0a, 0x06, 0x11, 0x09, 0x03, 0x13, 0x06, 0x13, 0x02, 0x0c, 0x08, 0x0f, 0x0f, 0x04, 0x12, 0x12, 0x12,
    0x01, 0x01, 0x0a, 0x0a, 0x12, 0x0b, 0x0b, 0x01, 0x11, 0x02, 0x11, 0x12, 0x01, 0x01, 0x16, 0x0c, 0x02, 0x15, 0x0f,
    0x01, 0x05, 0x02, 0x02, 0x12, 0x06, 0x00, 0x03, 0x03, 0x01, 0x0b, 0x0c, 0x07, 0x06, 0x0f, 0x02, 0x12, 0x12, 0x01,
    0x01, 0x01, 0x0c, 0x07, 0x02, 0x12, 0x06, 0x06, 0x06, 0x0c, 0x11, 0x0b, 0x07, 0x07, 0x12, 0x03, 0x02, 0x13, 0x02,
    0x05, 0x0b, 0x0c, 0x12, 0x00, 0x16, 0x0c, 0x0b, 0x11, 0x13, 0x04, 0x0c, 0x13, 0x18, 0x0b, 0x06, 0x06, 0x0c, 0x0f,
    0x06, 0x0f, 0x03, 0x05, 0x11, 0x14, 0x0c, 0x00, 0x03, 0x0f, 0x07, 0x11, 0x06, 0x02, 0x0f, 0x0c, 0x03, 0x12, 0x00,
    0x15, 0x12, 0x12, 0x12, 0x13, 0x0f, 0x04, 0x0e, 0x03, 0x12, 0x0f, 0x16, 0x0b, 0x07, 0x12, 0x0f, 0x0b, 0x0f, 0x12,
    0x0f, 0x12, 0x0f, 0x12, 0x0c, 0x0c, 0x0f, 0x13, 0x14, 0x01, 0x19, 0x11, 0x01, 0x06, 0x16, 0x12, 0x03, 0x04, 0x00,
    0x13, 0x06, 0x02, 0x13, 0x0f, 0x12, 0x13, 0x12, 0x12, 0x12, 0x0b, 0x15, 0x16, 0x12, 0x02, 0x16, 0x0f, 0x0b, 0x01,
    0x12, 0x0a, 0x0a, 0x03, 0x03, 0x0c, 0x03, 0x02, 0x12, 0x12, 0x12, 0x18, 0x02, 0x13, 0x02, 0x00, 0x00, 0x13, 0x06,
    0x19, 0x19, 0x0c, 0x02, 0x06, 0x06, 0x06, 0x12, 0x03, 0x11, 0x03, 0x12, 0x15, 0x15, 0x15, 0x03, 0x12, 0x04, 0x0a,
    0x04, 0x05, 0x00, 0x05, 0x09, 0x00, 0x09, 0x06, 0x05, 0x05, 0x0a, 0x0a, 0x0a, 0x13, 0x04, 0x04, 0x04, 0x01, 0x0b,
    0x0b, 0x02, 0x00, 0x05, 0x07, 0x02, 0x01, 0x02, 0x12, 0x00, 0x12, 0x0c, 0x0c, 0x03, 0x06, 0x06, 0x0f, 0x01, 0x01,
    0x11, 0x01, 0x15, 0x0c, 0x07, 0x03, 0x03, 0x19, 0x07, 0x0b, 0x15, 0x02, 0x13, 0x15, 0x13, 0x13, 0x11, 0x19, 0x0b,
    0x0a, 0x0a, 0x0c, 0x06, 0x04, 0x01,
};

// Whether a species has the initial searched for
static BOOL Poketrade2D_IsBoxIconFiltered(PokemonTradeWork *wk, int species) {
    if (wk->unk800 != 0 && wk->unk800 - 1 == sSpeciesInitials[species]) {
        return TRUE;
    }
    return FALSE;
}

BOOL Poketrade2D_IsBoxIconMatch(u32 species, u32 initial) {
    if (initial == sSpeciesInitials[species]) {
        return TRUE;
    }
    return FALSE;
}

// Greys an icon out or not
void Poketrade2D_SetIconObjMode(PokemonTradeWork *wk, ClActor *icon, BOOL grey) {
    ClActor_SetObjMode(icon, grey ? 1 : 0);
}

static void Poketrade2D_UpdateIconObjMode(PokemonTradeWork *wk, int column, int row, BoxPkm *pkm, BOOL isParty,
                                          int index) {
    BOOL grey = FALSE;

    if (PoketradeNego_IsMonOwnOffer(wk, column, row)) {
        grey = TRUE;
    }
    Poketrade2D_SetIconObjMode(wk, wk->icons[index][row], grey);
}

// Where an image's characters are in a screen's OBJ VRAM, as NNS_G2dGetImageLocation(), inlined
static inline u32 ImageProxy_GetLocation(const NNSG2dImageProxy *proxy, int type) {
    return proxy->vramLocation.baseAddrOfVram[type];
}

// Sets up the icons of a column of the strip in the icon column at an index, loading the characters of the ones that
// changed, in the V-blank if asked and the icon is on screen
static void Poketrade2D_CreateBoxIcon(PokemonTradeWork *wk, BoxSaveAccessor *boxes, int column, int index, BOOL async) {
    int i;
    BoxPkm *pkm;
    TradeBoxEntry *entry;
    BOOL isParty;
    NNSG2dImageProxy proxy;
    ClActorPos pos;
    ClActorPos screenPos;

    if (column == wk->cursorColumn && !GCTX_HIDIsUsingTouch()) {
        ClActor_SetVisible(wk->iconCursors[index][wk->cursorRow], TRUE);
    }
    for (i = 0; i < COLUMN_ROWS; i++) {
        int species, form, sex;

        pkm = Poketrade2D_ResolveGridCell(boxes, column, i, wk, &isParty);
        entry = Poketrade2D_GetGridCellMon(column, i, wk);
        if (entry == NULL) {
            wk->iconColumns[index][i] = 0xff;
            continue;
        }
        if (entry->species == SPECIES_NONE) {
            wk->iconColumns[index][i] = 0xff;
            continue;
        }
        wk->iconColumns[index][i] = column;
        form = entry->form;
        species = entry->species;
        sex = entry->sex;
        if (species == wk->iconSpecies[index][i] && form == wk->iconForms[index][i] && sex == wk->iconSexes[index][i]) {
            ClActor_SetVisible(wk->icons[index][i], TRUE);
            Poketrade2D_UpdateIconObjMode(wk, column, i, pkm, isParty, index);
        } else if (species != wk->iconSpecies[index][i] ||
                   (species == wk->iconSpecies[index][i] && form != wk->iconForms[index][i]) ||
                   (species == wk->iconSpecies[index][i] && sex != wk->iconSexes[index][i])) {
            int box, slot;

            wk->iconSpecies[index][i] = species;
            wk->iconForms[index][i] = form;
            wk->iconSexes[index][i] = sex;
            Poketrade2D_GetGridCellPos(column, i, &pos);
            ClActor_GetImageProxy(wk->icons[index][i], &proxy);
            box = PokemonTrade_GetColumnBox(column, wk);
            slot = PokemonTrade_GetColumnSlot(column, i);
            slot += box * 30;
            ClActor_GetSurfacePos(wk->icons[index][i], &screenPos, CLACT_VRAM_SUB);
            if (async == TRUE && screenPos.x >= -16 && screenPos.x <= 272) {
                if (!NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_OBJ_CHAR_SUB,
                                                        proxy.vramLocation.baseAddrOfVram[NNS_G2D_VRAM_TYPE_2DSUB],
                                                        wk->iconCharData + slot * 0x200, 0x200)) {
                    MI_CpuCopy8(
                        wk->iconCharData + slot * 0x200,
                        (void *)((u32)G2S_GetOBJCharPtr() + ImageProxy_GetLocation(&proxy, NNS_G2D_VRAM_TYPE_2DSUB)),
                        0x200);
                }
            } else {
                MI_CpuCopy8(
                    wk->iconCharData + slot * 0x200,
                    (void *)((u32)G2S_GetOBJCharPtr() + ImageProxy_GetLocation(&proxy, NNS_G2D_VRAM_TYPE_2DSUB)),
                    0x200);
            }
            Poketrade2D_UpdateIconObjMode(wk, column, i, pkm, isParty, index);
            ClActor_SetPalette(wk->icons[index][i], PokeIcon_GetPaletteOfPkm(pkm), CLACT_VRAM_SUB);
            ClActor_SetAutoAnim(wk->icons[index][i], FALSE);
            ClActor_SetVisible(wk->icons[index][i], TRUE);
        }
        if (!PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) && Poketrade2D_IsBoxIconFiltered(wk, species)) {
            wk->iconMarked[index][i] = TRUE;
        }
    }
}

// The column of the strip at the left edge of the screen
int Poketrade2D_GetScrollColumn(PokemonTradeWork *wk) {
    int x = wk->scrollX;
    int column;

    if (x < 48) {
        column = 0;
    } else if (x < 96) {
        column = 1;
    } else {
        int box, offset;
        int boxesX = x - 96;

        box = boxesX / 160;
        offset = boxesX - box * 160;
        column = box * BOX_COLUMNS + PARTY_COLUMNS;
        if (offset >= 8) {
            if (offset >= 152) {
                column += BOX_COLUMNS - 1;
            } else {
                column += (offset - 8) / 24;
            }
        }
    }
    return column;
}

// Whether the cursor's column is on screen, and the third column on screen, where the cursor goes if not
BOOL Poketrade2D_GetVisibleColumnRange(PokemonTradeWork *wk, int *column) {
    int first = Poketrade2D_GetScrollColumn(wk);
    int last = first + 10;

    *column = first + 2;
    if (first + 2 >= wk->columnCount) {
        *column = first + 2 - wk->columnCount;
    }
    if (last >= wk->columnCount) {
        int cursor = wk->cursorColumn;

        if (cursor < 20) {
            cursor = cursor + 1 + wk->columnCount;
        }
        if (first <= cursor && cursor <= last) {
            return TRUE;
        }
    } else if (first <= wk->cursorColumn && wk->cursorColumn <= last) {
        return TRUE;
    }
    return FALSE;
}

// Sets up the icons for the strip's scroll when the columns on screen changed, and places them
void Poketrade2D_UpdateVisibleColumns(BoxSaveAccessor *boxes, PokemonTradeWork *wk, BOOL async) {
    int i, j;
    int column = Poketrade2D_GetScrollColumn(wk);

    if (wk->firstColumn != column) {
        wk->firstColumn = column;
        for (i = 0; i < ICON_COLUMNS; i++) {
            for (j = 0; j < COLUMN_ROWS; j++) {
                ClActor_SetVisible(wk->iconCursors[i][j], FALSE);
                wk->iconMarked[i][j] = FALSE;
            }
        }
        wk->unk1090 = column;
        Poketrade2D_HideAllBoxIcons(wk);
        for (i = 0; i < ICON_COLUMNS; i++) {
            Poketrade2D_CreateBoxIcon(wk, boxes, column, Poketrade2D_WrapIndex12(wk->firstColumn + i), async);
            column++;
            if (column >= wk->columnCount) {
                column = 0;
            }
        }
    }
    Poketrade2D_UpdateBoxIconPositions(wk);
}

// Places the icons and their cursors where their columns are at the strip's scroll, lifting the one the cursor is on
static void Poketrade2D_UpdateBoxIconPositions(PokemonTradeWork *wk) {
    int i, j;
    int wrap = 0;
    int column = wk->firstColumn;

    for (i = 0; i < ICON_COLUMNS; i++) {
        int index = Poketrade2D_WrapIndex12(wk->firstColumn + i);

        if (column >= wk->columnCount) {
            column -= wk->columnCount;
            wrap = wk->stripWidth;
        }
        for (j = 0; j < COLUMN_ROWS; j++) {
            int y = j * 24 + 32;
            int x;
            ClActorPos pos, markPos;

            if (column == 0) {
                x = 28;
            } else if (column == 1) {
                x = 60;
            } else {
                x = (column - PARTY_COLUMNS) / BOX_COLUMNS * 160;
                x += (column - PARTY_COLUMNS) % BOX_COLUMNS * 24 + 20;
                x += 96;
            }
            pos.x = wrap + (x - wk->scrollX);
            pos.y = y;
            markPos = pos;
            if (PoketradeNego_IsMonPeerSelected(wk, column, j) && GCTX_HIDIsUsingTouch() == TRUE) {
                pos.y -= 8;
                ClActor_SetPriority(wk->icons[index][j], 11);
            } else {
                ClActor_SetPriority(wk->icons[index][j], 28 - i);
            }
            ClActor_SetSurfacePos(wk->icons[index][j], &pos, CLACT_VRAM_SUB);
            if (wk->iconMarked[index][j]) {
                wk->iconPos[index][j] = markPos;
            }
            pos.y = y + 3;
            ClActor_SetSurfacePos(wk->iconCursors[index][j], &pos, CLACT_VRAM_SUB);
        }
        column++;
    }
}

// Copies the characters of the icons of a box's Pokémon, or of the party's after the last box, one box a call; the
// first call allocates room for all of them
void Poketrade2D_LoadBoxIconGfx(PokemonTradeWork *wk, int box) {
    ArcTool *arc;
    int i, slot;
    void *file;
    NNSG2dCharacterData *chars;

    if (box == 0) {
        wk->iconCharData = GFL_HeapAllocate(wk->heapId, (MAX_BOXES * 30 + 6) * 0x200, FALSE, "pokemontrade_2d.c", 1291);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);
    // One box a frame: the loop stops after its first pass, as the original's does
    while (box < MAX_BOXES + 1) {
        slot = box * 30;
        for (i = 0; i < 30; i++) {
            BoxPkm *pkm;
            u32 icon;

            if (box == MAX_BOXES && i == 6) {
                break;
            }
            pkm = PokemonTrade_GetBoxPkm(wk->boxes, box, i, wk);
            if (pkm != NULL) {
                icon = PokeIcon_GetFileOfPkm(pkm);
            } else {
                icon = PokeParty_GetIconIndex(0, 0, 0, FALSE);
            }
            file = GFL_G2DIOReadBGNCGRArc(arc, icon, FALSE, &chars, wk->heapId);
            MI_CpuCopy8(chars->pRawData, wk->iconCharData + slot * 0x200, 0x200);
            slot++;
            GFL_HeapFree(file);
        }
        break;
    }
    GFL_ArcToolFree(arc);
}

// The icon column that a column of the strip is set up in
static int Poketrade2D_WrapIndex12(int column) {
    return column % ICON_COLUMNS;
}

static void Poketrade2D_HideAllBoxIcons(PokemonTradeWork *wk) {
    int i, j;

    for (i = 0; i < ICON_COLUMNS; i++) {
        for (j = 0; j < COLUMN_ROWS; j++) {
            ClActor_SetVisible(wk->icons[i][j], FALSE);
        }
    }
}

// The Pokémon icon at a point of the screen. If asked, the box and slot of its Pokémon, NULL if the icon shows none,
// and the column of the strip, the row and the icon column it is in
ClActor *Poketrade2D_HitTestBoxIcons(PokemonTradeWork *wk, int x, int y, int *box, int *slot, int *column, int *row,
                                     int *index) {
    int i, j;
    int iconColumn;

    for (i = 0; i < ICON_COLUMNS; i++) {
        for (j = 0; j < COLUMN_ROWS; j++) {
            ClActorPos pos;

            ClActor_GetSurfacePos(wk->icons[i][j], &pos, CLACT_VRAM_SUB);
            if (pos.x <= x && x < pos.x + 24 && pos.y <= y && y < pos.y + 24) {
                if (box != NULL) {
                    iconColumn = wk->iconColumns[i][j];
                    if (iconColumn == 0xff) {
                        return NULL;
                    }
                    if (PokemonTrade_GetColumnSlot(iconColumn, j) != -1) {
                        *box = PokemonTrade_GetColumnBox(iconColumn, wk);
                        *slot = PokemonTrade_GetColumnSlot(iconColumn, j);
                    } else {
                        return NULL;
                    }
                }
                if (column != NULL) {
                    *column = iconColumn;
                    *row = j;
                    *index = i;
                }
                return wk->icons[i][j];
            }
        }
    }
    return NULL;
}

// Sets up the trade demo's BGs 6 and 7 on the lower screen, with the graphics of the trade or of the other demo
void Poketrade2D_SetupSummaryBgs(PokemonTradeWork *wk, BOOL other) {
    ArcTool *arc;

    Poketrade2D_SetDisplayConfig(1);
    wk->demoBGsCreated = TRUE;
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe800),
                          GX_BG_CHARBASE(0x10000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(6, 0, 1, 0);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_256,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(7, &setup, BGMODE_EXTENDED);
    }
    if (!other) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE_DEMO, wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 5, 6, 0x6000, 0, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 4, 7, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 7, 0, 0, FALSE, wk->heapId);
        GFL_BGSysMoveBG(7, BG_MOVE_SET_X, 0);
        GFL_ArcToolFree(arc);
    } else {
        arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE_2D, wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 1, 6, 0x6000, 0, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 0, 7, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 2, 7, 0, 0, FALSE, wk->heapId);
        GFL_BGSysMoveBG(7, BG_MOVE_SET_X, 0);
        GFL_ArcToolFree(arc);
    }
}

void Poketrade2D_ExitSummaryBgs(PokemonTradeWork *wk) {
    if (wk->demoBGsCreated) {
        GFL_BGSysFreeFilledChar(6, 1, 0);
        GFL_BGSysReleaseBG(6);
        GFL_BGSysReleaseBG(7);
        wk->demoBGsCreated = FALSE;
    }
}

// Sets up the VRAM and the cell actor system, with every BG hidden
void Poketrade2D_InitDisplayAndClact(PokemonTradeWork *wk) {
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    ClActSys_Create(&sClActSetup, &sVRAMConfig, wk->heapId);
    Poketrade2D_SetDisplayConfig(0);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
}

// Sets up the upper screen's BGs 1 to 3, over the 3D of BG 0
void Poketrade2D_SetupMainBgs(PokemonTradeWork *wk) {
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe000),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(3, 0, 1, 0);
        GFL_BGSysFillScrArea(3, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(3);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x08000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          3,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(2, 0, 1, 0);
        GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(2);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x08000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(1);
    }
    GFL_BGSysSet3DBGPriority(0);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGPriority(0, 0);
}

// The 3D system calls this once it is set up
static void Poketrade2D_Setup3DRenderState(void) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    G3X_SetShading(GX_SHADING_HIGHLIGHT);
    G3X_AntiAlias(TRUE);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    G3X_SetFog(TRUE, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x8000, 0);
    G3X_SetClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
    for (i = 0; i < NELEMS(sLights); i++) {
        GFL_G3DSysLightSet(i, &sLights[i]);
    }
    G2_SetBG0Priority(2);
}

// Sets up the 3D system and the upper screen's camera
void Poketrade2D_InitG3DSystem(PokemonTradeWork *wk) {
    // Nothing reads it, but the original's .rodata has these 16 zero bytes after the camera's vectors. Its type and
    // name are a guess: the eight colors that G3X_SetEdgeColorTable takes, which overlay 207's p_sta_sys.c sets in
    // the same place, just before G3X_EdgeMarking(FALSE), in a setup otherwise like this one. These four are declared
    // in the only order that gives the original's .rodata, not for a reason the code shows
    static const GXRgb sEdgeColors[8] = { 0 };
    // The camera: where it is, what it looks at, and which way is up. Nothing reads the up either: the camera is
    // given the local copy below, whose initializer the original has as well; copying sCameraUp into it loses that
    static const VecFx32 sCameraPos = { 0, 0, FX32_CONST(200) };
    static const VecFx32 sCameraTarget = { 0, 0, 0 };
    static const VecFx32 sCameraUp = { 0, FX32_ONE, 0 };

    GFL_G3DSysCreate(FALSE, 2, FALSE, 1, 0, HEAPID_TAIL(wk->heapId), Poketrade2D_Setup3DRenderState);
    {
        HeapID heapId = HEAPID_TAIL(wk->heapId);
        VecFx32 up = { 0, FX32_ONE, 0 };

        // A 40 degree field of view
        wk->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                         FX_CosIdx(DEG_TO_IDX(20)), FX32_CONST(4.0 / 3.0), 0, FX32_ONE,
                                         FX32_CONST(1024), 0, &sCameraPos, &up, &sCameraTarget, heapId);
    }
    GFL_G3DCameraFlush(wk->camera);
    G3X_EdgeMarking(FALSE);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
}

// Sets up the lower screen's BGs
void Poketrade2D_SetupSubBgs(PokemonTradeWork *wk) {
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe000),
                          GX_BG_CHARBASE(0x10000),
                          0x10000,
                          GX_BG_EXTPLTT_01,
                          1,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(4, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x1000,
                          0,
                          BGRES_512x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xc000),
                          GX_BG_CHARBASE(0x10000),
                          0x10000,
                          GX_BG_EXTPLTT_01,
                          2,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(5, 0, 1, 0);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x1000,
                          0,
                          BGRES_512x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x08000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          3,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(7, 0, 0, 0, 64, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(7);
    }
    {
        BGSetup setup = { 0,
                          0,
                          0x800,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xe800),
                          GX_BG_CHARBASE(0x00000),
                          0x8000,
                          GX_BG_EXTPLTT_01,
                          0,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(6, 0, 1, 0);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 24, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(6);
    }
    wk->subBGsCreated = TRUE;
}

void Poketrade2D_ExitSubBgs(PokemonTradeWork *wk) {
    if (wk->subBGsCreated == TRUE) {
        GFL_BGSysFreeFilledChar(4, 1, 0);
        GFL_BGSysFreeFilledChar(6, 1, 0);
        GFL_BGSysReleaseBG(4);
        GFL_BGSysReleaseBG(5);
        GFL_BGSysReleaseBG(6);
        GFL_BGSysReleaseBG(7);
        wk->subBGsCreated = FALSE;
    }
}

// The trade's display, and the trade demo's, whose lower screen shows a bitmap BG
static const BGSysLCDConfig sLCDConfigs[] = {
    { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D },
    { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_3, GX_BG0_AS_3D },
};

// Sets the display up for the trade, or for the trade demo
void Poketrade2D_SetDisplayConfig(int config) {
    GFL_BGSysSetLCDConfig(&sLCDConfigs[config]);
}

// Loads the sprites of the upper screen once
void Poketrade2D_RegisterMarkerResources(PokemonTradeWork *wk) {
    if (wk->objRes[TRADE_OBJRES_CHAR_MAIN] == 0) {
        ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);

        wk->objRes[TRADE_OBJRES_CHAR_MAIN] = ClActSys_LoadChars(arc, 21, FALSE, CLACT_VRAM_MAIN, wk->heapId);
        wk->objRes[TRADE_OBJRES_PLTT_MAIN] = ClActSys_LoadPlttRange(arc, 18, CLACT_VRAM_MAIN, 0xc0, 0, 6, wk->heapId);
        wk->objRes[TRADE_OBJRES_CELL_MAIN] = ClActSys_LoadCellAnims(arc, 20, 19, wk->heapId);
        GFL_ArcToolFree(arc);
    }
}

void Poketrade2D_ReleaseMarkerResources(PokemonTradeWork *wk) {
    if (wk->objRes[TRADE_OBJRES_CHAR_MAIN] != 0) {
        ClActSys_FreeChars(wk->objRes[TRADE_OBJRES_CHAR_MAIN]);
        ClActSys_FreePltt(wk->objRes[TRADE_OBJRES_PLTT_MAIN]);
        ClActSys_FreeCellAnims(wk->objRes[TRADE_OBJRES_CELL_MAIN]);
        wk->objRes[TRADE_OBJRES_CHAR_MAIN] = 0;
        wk->objRes[TRADE_OBJRES_PLTT_MAIN] = 0;
        wk->objRes[TRADE_OBJRES_CELL_MAIN] = 0;
    }
}

// Loads the sprites of the lower screen, and shows the one at the bottom of the strip
void Poketrade2D_RegisterIconSpriteResources(PokemonTradeWork *wk) {
    ClActorSetup setup;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);

    wk->objRes[TRADE_OBJRES_CHAR_SUB] = ClActSys_LoadChars(arc, 24, FALSE, CLACT_VRAM_SUB, wk->heapId);
    wk->objRes[TRADE_OBJRES_PLTT_SUB] = ClActSys_LoadPlttRange(arc, 18, CLACT_VRAM_SUB, 0xc0, 0, 6, wk->heapId);
    wk->objRes[TRADE_OBJRES_CELL_SUB] = ClActSys_LoadCellAnims(arc, 23, 22, wk->heapId);
    GFL_ArcToolFree(arc);
    setup.x = 128;
    setup.y = 180;
    setup.sequence = 2;
    setup.priority = 14;
    setup.bgPriority = 0;
    wk->actors[2] = ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                                   wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetAutoAnim(wk->actors[2], FALSE);
    ClActor_SetVisible(wk->actors[2], TRUE);
}

void Poketrade2D_LoadMainPalette(PokemonTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);

    GFL_G2DIOLoadNSCRSync(arc, 29, 2, 0, CHAR_POS(wk->bg2Chars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
}

// Sets up the sprites of the two Pokémon traded, over the upper screen's panels, and the arrows between them
void Poketrade2D_CreateTouchbarActors(PokemonTradeWork *wk) {
    ArcTool *arc;
    BoxPkm *pkm0, *pkm1;
    ClActorSetup setup;
    ClActorSetup iconSetup;

    setup.x = 128;
    setup.y = 16;
    setup.sequence = 19;
    setup.priority = 14;
    setup.bgPriority = 1;
    wk->actors[3] = ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                                   wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetAutoAnim(wk->actors[3], TRUE);
    ClActor_SetVisible(wk->actors[3], TRUE);
    setup.x = 96;
    setup.y = 16;
    setup.sequence = 0;
    setup.priority = 11;
    setup.bgPriority = 1;
    wk->actors[4] = ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                                   wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetAutoAnim(wk->actors[4], TRUE);
    if (GCTX_HIDIsUsingTouch()) {
        ClActor_SetVisible(wk->actors[4], FALSE);
    }

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);
    pkm0 = PokeParty_ToBoxPkm(PokemonTrade_GetPkm(wk, 0));
    pkm1 = PokeParty_ToBoxPkm(PokemonTrade_GetPkm(wk, 1));
    wk->objRes[TRADE_OBJRES_CHAR_ICON0] =
        ClActSys_LoadChars(arc, PokeIcon_GetFileOfPkm(pkm0), FALSE, CLACT_VRAM_SUB, wk->heapId);
    wk->objRes[TRADE_OBJRES_CHAR_ICON1] =
        ClActSys_LoadChars(arc, PokeIcon_GetFileOfPkm(pkm1), FALSE, CLACT_VRAM_SUB, wk->heapId);
    iconSetup.x = 96;
    iconSetup.y = 12;
    iconSetup.sequence = 0;
    iconSetup.priority = 12;
    iconSetup.bgPriority = 1;
    wk->actors[5] =
        ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_ICON0], wk->objRes[TRADE_OBJRES_PLTT_ICON],
                       wk->objRes[TRADE_OBJRES_CELL_ICON], &iconSetup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetAutoAnim(wk->actors[5], FALSE);
    ClActor_SetVisible(wk->actors[5], TRUE);
    ClActor_SetPalette(wk->actors[5], PokeIcon_GetPaletteOfPkm(pkm0), CLACT_VRAM_SUB);
    iconSetup.x = 160;
    wk->actors[6] =
        ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_ICON1], wk->objRes[TRADE_OBJRES_PLTT_ICON],
                       wk->objRes[TRADE_OBJRES_CELL_ICON], &iconSetup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetAutoAnim(wk->actors[6], FALSE);
    ClActor_SetVisible(wk->actors[6], TRUE);
    ClActor_SetPalette(wk->actors[6], PokeIcon_GetPaletteOfPkm(pkm1), CLACT_VRAM_SUB);
    GFL_ArcToolFree(arc);
}

// Places the icons of the two Pokémon, the one of the side picked raised unless they are level
void Poketrade2D_PlaceTouchbarActors(PokemonTradeWork *wk, int side, BOOL level) {
    ClActorPos pos;

    if (level) {
        pos.x = 96;
        pos.y = 12;
        ClActor_SetSurfacePos(wk->actors[5], &pos, CLACT_VRAM_SUB);
        pos.x = 160;
        pos.y = 12;
        ClActor_SetSurfacePos(wk->actors[6], &pos, CLACT_VRAM_SUB);
    } else {
        pos.x = 96;
        pos.y = side == 0 ? 8 : 12;
        ClActor_SetSurfacePos(wk->actors[5], &pos, CLACT_VRAM_SUB);
        pos.x = 160;
        pos.y = side != 0 ? 8 : 12;
        ClActor_SetSurfacePos(wk->actors[6], &pos, CLACT_VRAM_SUB);
    }
}

void Poketrade2D_DeleteTouchbarActors(PokemonTradeWork *wk) {
    if (wk->actors[3] != NULL) {
        ClActor_Delete(wk->actors[3]);
    }
    if (wk->actors[4] != NULL) {
        ClActor_Delete(wk->actors[4]);
    }
    if (wk->actors[5] != NULL) {
        ClActor_Delete(wk->actors[5]);
    }
    if (wk->actors[6] != NULL) {
        ClActor_Delete(wk->actors[6]);
    }
    wk->actors[3] = NULL;
    wk->actors[4] = NULL;
    wk->actors[5] = NULL;
    wk->actors[6] = NULL;
    if (wk->objRes[TRADE_OBJRES_CHAR_ICON0] != 0) {
        ClActSys_FreeChars(wk->objRes[TRADE_OBJRES_CHAR_ICON0]);
    }
    if (wk->objRes[TRADE_OBJRES_CHAR_ICON1] != 0) {
        ClActSys_FreeChars(wk->objRes[TRADE_OBJRES_CHAR_ICON1]);
    }
    wk->objRes[TRADE_OBJRES_CHAR_ICON0] = 0;
    wk->objRes[TRADE_OBJRES_CHAR_ICON1] = 0;
}

// Where the windows of the letters of the search by initial go, in tiles
static u32 sInitialWindowPos[][2] = {
    { 1, 9 },   { 4, 9 },   { 7, 9 },  { 10, 9 }, { 13, 9 },  { 16, 9 },  { 19, 9 },  { 22, 9 },  { 25, 9 },
    { 28, 9 },  { 1, 12 },  { 4, 12 }, { 7, 12 }, { 10, 12 }, { 13, 12 }, { 16, 12 }, { 19, 12 }, { 22, 12 },
    { 25, 12 }, { 28, 12 }, { 1, 15 }, { 4, 15 }, { 7, 15 },  { 10, 15 }, { 13, 15 }, { 16, 15 },
};

// Opens the search by initial: its screen, and a window for each letter
void Poketrade2D_LoadSubScreenGfx(PokemonTradeWork *wk) {
    ArcTool *arc;
    int i;

    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(6, FALSE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 16, 4, 0, CHAR_POS(wk->bg5Chars), 0, FALSE, wk->heapId);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_X, -4);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_X, -4);
    GFL_ArcToolFree(arc);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    for (i = 0; i < NELEMS(sInitialWindowPos); i++) {
        wk->initialWindows[i] =
            BmpWin_CreateDynamic(6, sInitialWindowPos[i][0], sInitialWindowPos[i][1], 2, 2, 14, FALSE);
        GFL_MsgDataLoadStrbuf(wk->msgData, 53 + i, wk->drawStr);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->initialWindows[i]), 0, 0, wk->drawStr, wk->font);
        BmpWin_FlushMap(wk->initialWindows[i]);
        BmpWin_FlushChar(wk->initialWindows[i]);
    }
    GFL_BGSysQueueScrLoad(6);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
}

// Closes the search by initial
void Poketrade2D_ClearSubScreen(PokemonTradeWork *wk) {
    int i;

    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_X, 0);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_X, 0);
    if (wk->actors[2] != NULL) {
        ClActor_SetVisible(wk->actors[2], TRUE);
    }
    if (wk->actors[7] != NULL) {
        ClActor_SetVisible(wk->actors[7], TRUE);
    }
    for (i = 0; i < NELEMS(wk->initialWindows); i++) {
        if (wk->initialWindows[i] != NULL) {
            BmpWin_ClearScreen(wk->initialWindows[i]);
            BmpWin_ClearFrame(wk->initialWindows[i], 2);
            BmpWin_Free(wk->initialWindows[i]);
            wk->initialWindows[i] = NULL;
        }
    }
    GFL_BGSysQueueScrLoad(6);
}

// Shows the icon of a Pokémon's held item, a letter for mail, over a side's panel or, for side 2, the summary
void Poketrade2D_CreateMarkingIcon(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    PokemonTradeSprite *sprite = &wk->infoIcons[0];
    UIObjResSetup setup;
    int x, y;
    BOOL mail = FALSE;
    u32 item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);

    if (PML_ItemIsMail(item)) {
        mail = TRUE;
    }
    if (sprite->actor != NULL) {
        if (item == ITEM_NONE) {
            ClActor_SetVisible(sprite->actor, FALSE);
            return;
        }
        ClActor_SetSequence(sprite->actor, mail);
        ClActor_SetVisible(sprite->actor, TRUE);
        return;
    }
    if (item != ITEM_NONE) {
        switch (side) {
        case 0:
        case 1:
            x = side * 128 + 16;
            y = 132;
            setup.vramType = CLACT_VRAM_SUB;
            setup.paletteOffset = 12;
            break;
        case 2:
            x = 32;
            y = 172;
            setup.vramType = CLACT_VRAM_MAIN;
            setup.paletteOffset = 6;
            break;
        }
        setup.flags = 0;
        setup.arcId = getUINarcIdx();
        setup.paletteFile = AppMenuCommon_GetItemIconPaletteFile();
        setup.charFile = AppMenuCommon_GetItemIconCharFile();
        setup.cellFile = AppMenuCommon_GetItemIconCellFile(2);
        setup.animFile = AppMenuCommon_GetItemIconAnimFile(2);
        setup.paletteStart = 0;
        setup.paletteCount = 1;
        UIObjRes_Load(&sprite->res, &setup, wk->clactUnit, wk->heapId);
        sprite->actor = UIObjRes_CreateActor(&sprite->res, wk->clactUnit, x, y, mail, wk->heapId);
    }
}

void Poketrade2D_DeleteMarkingIcon(PokemonTradeSprite *sprite) {
    if (sprite->actor != NULL) {
        ClActor_Delete(sprite->actor);
        UIObjRes_Free(&sprite->res);
        sprite->actor = NULL;
    }
}

// Shows the Pokérus icon of a Pokémon that has it, over a side's panel or in the summary, or the icon of a Pokémon
// that had it
void Poketrade2D_CreatePokerusIcon(PokemonTradeWork *wk, int side, BOOL summary, PartyPkm *pkm) {
    PokemonTradeSprite *sprite = &wk->infoIcons[1];
    UIObjResSetup setup;
    BOOL pokerus = pokerusDuration(pkm);

    if (sprite->actor != NULL) {
        if (!pokerus) {
            ClActor_SetVisible(sprite->actor, FALSE);
        } else {
            ClActor_SetVisible(sprite->actor, TRUE);
        }
        return;
    }
    if (!pokerus) {
        if (!summary) {
            Poketrade2D_CreateCuredPokerusIcon(wk, side, pkm);
        }
        return;
    }
    if (summary) {
        setup.vramType = CLACT_VRAM_MAIN;
        setup.paletteOffset = 0;
    } else {
        setup.vramType = CLACT_VRAM_SUB;
        setup.paletteOffset = 11;
    }
    setup.flags = 0;
    setup.arcId = getUINarcIdx();
    setup.paletteFile = AppMenuCommon_GetStatusIconPaletteFile();
    setup.charFile = AppMenuCommon_GetStatusIconCharFile();
    setup.cellFile = AppMenuCommon_GetStatusIconCellFile(2);
    setup.animFile = AppMenuCommon_GetStatusIconAnimFile(2);
    setup.paletteStart = 0;
    setup.paletteCount = 1;
    UIObjRes_Load(&sprite->res, &setup, wk->clactUnit, wk->heapId);
    if (summary) {
        sprite->actor = UIObjRes_CreateActor(&sprite->res, wk->clactUnit, 244, 104, 0, wk->heapId);
    } else {
        sprite->actor = UIObjRes_CreateActor(&sprite->res, wk->clactUnit, side * 128 + 108, 24, 0, wk->heapId);
    }
}

// Shows the icon of a Pokémon that had Pokérus over a side's panel
static void Poketrade2D_CreateCuredPokerusIcon(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    PokemonTradeSprite *sprite = &wk->infoIcons[2];
    UIObjResSetup setup;
    BOOL cured = pokeHasPkrs(pkm);

    if (cured) {
        // The same test of an existing icon as Poketrade2D_CreatePokerusIcon's, though it can only be shown here
        if (sprite->actor != NULL) {
            if (!cured) {
                ClActor_SetVisible(sprite->actor, FALSE);
            } else {
                ClActor_SetVisible(sprite->actor, TRUE);
            }
            return;
        }
        setup.vramType = CLACT_VRAM_SUB;
        setup.paletteOffset = 14;
        setup.flags = 0;
        setup.arcId = getUINarcIdx();
        setup.paletteFile = AppMenuCommon_GetMarkingIconPaletteFile();
        setup.charFile = AppMenuCommon_GetMarkingIconCharFile(2);
        setup.cellFile = AppMenuCommon_GetMarkingIconCellFile(2);
        setup.animFile = AppMenuCommon_GetMarkingIconAnimFile(2);
        setup.paletteStart = 0;
        setup.paletteCount = 1;
        UIObjRes_Load(&sprite->res, &setup, wk->clactUnit, wk->heapId);
        sprite->actor = UIObjRes_CreateActor(&sprite->res, wk->clactUnit, side * 128 + 108, 20, 13, wk->heapId);
    }
}

// Shows the icons of a Pokémon's markings in the summary, each lit if it is set, and the rare and Pokérus flags,
// which show only when set. An egg shows no rare flag
void Poketrade2D_CreateSummaryIcons(PokemonTradeWork *wk, PartyPkm *pkm, BOOL isEgg) {
    u32 marks = PokeParty_GetParam(pkm, PKM_PARAM_MARKINGS, NULL);
    UIObjResSetup setup;
    u32 xs[] = { 25, 26, 27, 28, 29, 30, 20, 21 };
    // The animation of each icon when set and when not, in pairs, -1 for none
    int anims[] = { 1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 12, -1, 13, -1 };
    int i;

    setup.vramType = CLACT_VRAM_MAIN;
    setup.flags = 0;
    setup.arcId = getUINarcIdx();
    setup.paletteFile = AppMenuCommon_GetMarkingIconPaletteFile();
    setup.charFile = AppMenuCommon_GetMarkingIconCharFile(2);
    setup.cellFile = AppMenuCommon_GetMarkingIconCellFile(2);
    setup.animFile = AppMenuCommon_GetMarkingIconAnimFile(2);
    setup.paletteOffset = 13;
    setup.paletteStart = 0;
    setup.paletteCount = 1;
    if (!isEgg && PokeParty_IsRare(pkm)) {
        marks |= 1 << 6;
    }
    if (pokeHasPkrs(pkm)) {
        marks |= 1 << 7;
    }
    if (!wk->markIcons.loaded) {
        UIObjRes_Load(&wk->markIcons.res, &setup, wk->clactUnit, wk->heapId);
        wk->markIcons.loaded = TRUE;
    }
    for (i = 0; i < 8; i++) {
        ClActor *icon = wk->markIcons.icons[i];
        int anim;

        if (marks & (1 << i)) {
            anim = anims[i * 2];
        } else {
            anim = anims[i * 2 + 1];
        }
        if (icon != NULL) {
            if (anim == -1) {
                ClActor_SetVisible(icon, FALSE);
            } else if (isEgg && i == 6) {
                ClActor_SetVisible(icon, FALSE);
            } else {
                ClActor_SetSequence(icon, anim);
                ClActor_SetVisible(icon, TRUE);
            }
        } else {
            wk->markIcons.icons[i] = UIObjRes_CreateActor(&wk->markIcons.res, wk->clactUnit, xs[i] * 8, 101,
                                                          anim == -1 ? anims[i * 2] : anim, wk->heapId);
            icon = wk->markIcons.icons[i];
            if (anim == -1) {
                ClActor_SetVisible(icon, FALSE);
            } else if (isEgg && i == 6) {
                ClActor_SetVisible(icon, FALSE);
            }
        }
    }
}

void Poketrade2D_DeleteSummaryIcons(PokemonTradeWork *wk) {
    TradeMarkIcons *marks = &wk->markIcons;
    int i;

    for (i = 0; i < 8; i++) {
        if (marks->icons[i] != NULL) {
            ClActor_Delete(marks->icons[i]);
            marks->icons[i] = NULL;
        }
    }
    if (marks->loaded) {
        UIObjRes_Free(&marks->res);
        marks->loaded = FALSE;
    }
}

// Loads the box palette into a BG palette slot, with the colours of the UI's last four after it
void Poketrade2D_UploadWindowPalette(PokemonTradeWork *wk, u32 palette, u32 type) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, type, palette * 32, 32, wk->heapId);
    GFL_G2DIOLoadNCLR(getUINarcIdx(), 31, type, 0x1c, palette * 32 + 0x1c, 4, wk->heapId);
}

void Poketrade2D_DeleteOfferIconActor(PokemonTradeWork *wk, int side, int index) {
    if (wk->negoIcons[side][index] != NULL) {
        ClActor_Delete(wk->negoIcons[side][index]);
        wk->negoIcons[side][index] = NULL;
        ClActSys_FreeChars(wk->negoIconChars[side][index]);
    }
}

void Poketrade2D_SetOfferIconsVisible(PokemonTradeWork *wk, int side, BOOL visible) {
    int i;

    for (i = 0; i < 3; i++) {
        if (wk->negoIcons[side][i] != NULL) {
            ClActor_SetVisible(wk->negoIcons[side][i], visible);
        }
    }
}

// Where the icons of the Pokémon of a negotiation go, for each side: in the row on the main screen, and on the sub
// screen's panels
static const u32 sNegoIconPosMain[6][2] = {
    { 32, 96 }, { 64, 96 }, { 96, 96 }, { 168, 96 }, { 200, 96 }, { 232, 96 },
};
static const u32 sNegoIconPosSub[6][2] = {
    { 24, 46 }, { 24, 94 }, { 24, 142 }, { 152, 46 }, { 152, 94 }, { 152, 142 },
};

// Sets up the icon of a Pokémon offered in a negotiation, in the row of the main screen or on the sub screen's panel
void Poketrade2D_CreateOfferIconActor(PokemonTradeWork *wk, int side, int index, PartyPkm *pkm, BOOL onMain,
                                      BOOL visible) {
    BoxPkm *boxPkm = PokeParty_GetBoxPkm(pkm);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);
    ClActorSetup setup;
    u32 vramType;

    Poketrade2D_DeleteOfferIconActor(wk, side, index);
    if (onMain) {
        setup.x = sNegoIconPosMain[side * 3 + index][0];
        setup.y = sNegoIconPosMain[side * 3 + index][1];
        vramType = CLACT_VRAM_MAIN;
    } else {
        setup.x = sNegoIconPosSub[side * 3 + index][0];
        setup.y = sNegoIconPosSub[side * 3 + index][1];
        vramType = CLACT_VRAM_SUB;
    }
    setup.sequence = 1;
    setup.priority = 16;
    setup.bgPriority = 1;
    wk->negoIconChars[side][index] =
        ClActSys_LoadChars(arc, PokeIcon_GetFileOfPkm(boxPkm), FALSE, vramType, wk->heapId);
    wk->negoIcons[side][index] =
        ClActor_Create(wk->clactUnit, wk->negoIconChars[side][index], wk->objRes[TRADE_OBJRES_PLTT_NEGO],
                       wk->objRes[TRADE_OBJRES_CELL_NEGO], &setup, vramType, wk->heapId);
    ClActor_SetPalette(wk->negoIcons[side][index], PokeIcon_GetPaletteOfPkm(boxPkm), CLACT_VRAM_SUB);
    ClActor_SetAutoAnim(wk->negoIcons[side][index], FALSE);
    ClActor_SetVisible(wk->negoIcons[side][index], visible);
    GFL_ArcToolFree(arc);
}

// Places the icons on the panels of a negotiation, the one picked raised
void Poketrade2D_UpdateOfferIconHighlight(PokemonTradeWork *wk, int side, int index) {
    int i, j;
    ClActorPos pos;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            if (wk->negoIcons[i][j] != NULL) {
                pos.x = sNegoIconPosSub[i * 3 + j][0];
                pos.y = sNegoIconPosSub[i * 3 + j][1];
                if (i == side && j == index) {
                    pos.y -= 4;
                }
                ClActor_SetSurfacePos(wk->negoIcons[i][j], &pos, CLACT_VRAM_SUB);
            }
        }
    }
}

void Poketrade2D_DeleteAllOfferIconActors(PokemonTradeWork *wk) {
    int j, i;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 3; j++) {
            Poketrade2D_DeleteOfferIconActor(wk, i, j);
        }
    }
}

// Makes the copy of an icon that the stylus carries
void Poketrade2D_CreateFloatingIconActor(PokemonTradeWork *wk, int column, int row, int x, int y, BoxPkm *pkm) {
    ClActorSetup setup;

    setup.x = x;
    setup.y = y;
    setup.sequence = 1;
    setup.priority = 0;
    setup.bgPriority = 0;
    wk->actors[9] = ClActor_Create(wk->clactUnit, wk->iconChars[column][row], wk->objRes[TRADE_OBJRES_PLTT_ICON],
                                   wk->objRes[TRADE_OBJRES_CELL_ICON], &setup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetPalette(wk->actors[9], PokeIcon_GetPaletteOfPkm(pkm), CLACT_VRAM_SUB);
}

// Lets the carried icon go: it flies on in the direction the stylus moved it and curves off to the upper left
void Poketrade2D_StartIconFlight(PokemonTradeWork *wk) {
    ClActorPos pos;
    VecFx32 points[4];
    fx32 x, y, dx, dy, length;

    ClActor_GetSurfacePos(wk->actors[9], &pos, CLACT_VRAM_SUB);
    dx = (pos.x - wk->heldPrevPos.x) * FX32_ONE;
    dy = (pos.y - wk->heldPrevPos.y) * FX32_ONE;
    points[0].x = pos.x * FX32_ONE;
    points[0].y = pos.y * FX32_ONE;
    points[0].z = 0;
    points[1].x = (pos.x * 2 - wk->heldPrevPos.x) * FX32_ONE;
    points[1].y = (pos.y * 2 - wk->heldPrevPos.y) * FX32_ONE;
    points[1].z = 0;
    points[3].x = FX32_CONST(28);
    points[3].y = 0;
    points[3].z = 0;
    x = points[0].x - points[3].x;
    y = points[0].y / 2;
    length = FX_Sqrt(FX_Mul(x, x) + FX_Mul(y, y)) / 15;
    points[2].x = x + FX_Mul(dx, length);
    points[2].y = y + FX_Mul(dy, length);
    points[2].z = 0;
    Poketrade2D_InitBezierCurve(&wk->curve, &points[0], &points[1], &points[2], &points[3], 19);
    wk->curveTimer = 21;
}

// Moves the icon that was let go along its curve; TRUE when its flight is over
BOOL Poketrade2D_UpdateIconFlight(PokemonTradeWork *wk) {
    ClActorPos pos;

    if (wk->curveTimer != 0) {
        wk->curveTimer--;
        if (wk->curveTimer == 0) {
            return TRUE;
        }
        ClActor_GetSurfacePos(wk->actors[9], &pos, CLACT_VRAM_SUB);
        if (pos.y < 0) {
            wk->curveTimer = 1;
        }
        if (wk->curveTimer == 1) {
            Poketrade2D_DeleteFloatingIconActor(wk);
        } else {
            Poketrade2D_StepBezierCurve(&wk->curve);
            pos.x = wk->curve.pos.x / FX32_ONE;
            pos.y = wk->curve.pos.y / FX32_ONE;
            ClActor_SetSurfacePos(wk->actors[9], &pos, CLACT_VRAM_SUB);
        }
    }
    return FALSE;
}

void Poketrade2D_DeleteFloatingIconActor(PokemonTradeWork *wk) {
    if (wk->actors[9] != NULL) {
        ClActor_Delete(wk->actors[9]);
        wk->actors[9] = NULL;
    }
}

void Poketrade2D_SetupSubBlend(PokemonTradeWork *wk) {
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, 0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3,
                       8, 8);
}

// Shows a stamp's button, under the lower screen's panels
void Poketrade2D_CreateMarkerActor(PokemonTradeWork *wk, int index) {
    ClActorSetup setup;

    if (wk->stampButtons[index] == NULL) {
        setup.x = index * 24 + 8;
        setup.y = 144;
        setup.sequence = index + 11;
        setup.priority = 50;
        setup.bgPriority = 1;
        wk->stampButtons[index] =
            ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                           wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
    }
    ClActor_SetAutoAnim(wk->stampButtons[index], TRUE);
    ClActor_SetVisible(wk->stampButtons[index], TRUE);
}

void Poketrade2D_SetMarkerActorVisible(PokemonTradeWork *wk, int index, BOOL visible) {
    ClActor_SetVisible(wk->stampButtons[index], visible);
}

void Poketrade2D_DeleteMarkerActor(PokemonTradeWork *wk, int index) {
    if (wk->stampButtons[index] != NULL) {
        ClActor_Delete(wk->stampButtons[index]);
        wk->stampButtons[index] = NULL;
    }
}

// Plays the animation of a stamp's button being pressed
void Poketrade2D_SetMarkerActorAnim(PokemonTradeWork *wk, u32 index) {
    ClActor_SetSequence(wk->stampButtons[index], index + 15);
}

// Shows a stamp in a side's balloon
void Poketrade2D_CreateMarkerButtonActor(PokemonTradeWork *wk, u32 index, int side) {
    ClActorSetup setup;

    setup.x = side * 206 + 8;
    setup.y = 160;
    setup.sequence = 4;
    setup.priority = 1;
    setup.bgPriority = 0;
    if (wk->stamps[side * 2] == NULL) {
        wk->stamps[side * 2] =
            ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_MAIN], wk->objRes[TRADE_OBJRES_PLTT_MAIN],
                           wk->objRes[TRADE_OBJRES_CELL_MAIN], &setup, CLACT_VRAM_MAIN, wk->heapId);
        ClActor_SetAutoAnim(wk->stamps[side * 2], TRUE);
        ClActor_SetVisible(wk->stamps[side * 2], TRUE);
    } else {
        ClActor_SetVisible(wk->stamps[side * 2], TRUE);
    }
    if (wk->stamps[side * 2 + 1] == NULL) {
        setup.priority = 0;
        setup.sequence = index;
        wk->stamps[side * 2 + 1] =
            ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_MAIN], wk->objRes[TRADE_OBJRES_PLTT_MAIN],
                           wk->objRes[TRADE_OBJRES_CELL_MAIN], &setup, CLACT_VRAM_MAIN, wk->heapId);
        ClActor_SetAutoAnim(wk->stamps[side * 2 + 1], TRUE);
        ClActor_SetVisible(wk->stamps[side * 2 + 1], TRUE);
    } else {
        ClActor_SetSequence(wk->stamps[side * 2 + 1], index);
    }
}

void Poketrade2D_DeleteMarkerButtonPair(PokemonTradeWork *wk, int side) {
    if (wk->stamps[side * 2 + 1] != NULL) {
        ClActor_Delete(wk->stamps[side * 2 + 1]);
        wk->stamps[side * 2 + 1] = NULL;
    }
    if (wk->stamps[side * 2] != NULL) {
        ClActor_Delete(wk->stamps[side * 2]);
        wk->stamps[side * 2] = NULL;
    }
}

// Puts the cursor on one of the six panels of a negotiation's Pokémon, or hides it for -1
void Poketrade2D_MovePickedIconActor(PokemonTradeWork *wk, int cursor) {
    ClActorPos positions[] = {
        { 64, 48 }, { 64, 96 }, { 64, 144 }, { 192, 48 }, { 192, 96 }, { 192, 144 },
    };
    ClActorSetup setup;

    if (cursor == -1) {
        if (wk->negoCursor != NULL) {
            ClActor_SetVisible(wk->negoCursor, FALSE);
        }
        return;
    }
    if (wk->negoCursor == NULL) {
        setup.x = 64;
        setup.y = 48;
        setup.sequence = 23;
        setup.priority = 0;
        setup.bgPriority = 1;
        wk->negoCursor =
            ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                           wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
    }
    ClActor_SetAutoAnim(wk->negoCursor, TRUE);
    ClActor_SetVisible(wk->negoCursor, TRUE);
    ClActor_SetSurfacePos(wk->negoCursor, &positions[cursor], CLACT_VRAM_SUB);
}

void Poketrade2D_DeletePickedIconActor(PokemonTradeWork *wk) {
    if (wk->negoCursor != NULL) {
        ClActor_Delete(wk->negoCursor);
        wk->negoCursor = NULL;
    }
}

// Slides the panels of a negotiation in, each side from its edge, 4 pixels a frame; TRUE when they are in
BOOL Poketrade2D_StepCursorSlide(PokemonTradeWork *wk) {
    // The 4 of the file's .rodata, though its reads fold. Whose it is is a guess: as the 4 of
    // Poketrade2D_UpdateOfferIconHighlight's icon raise it gives the same layout, but this is the 4 the file uses most
    static const int speed = 4;
    ClActorPos pos, leftPos, rightPos;
    int i;

    if (wk->panelSlide < 128) {
        wk->panelSlide += speed;
    } else {
        return TRUE;
    }
    for (i = 0; i < 8; i++) {
        ClActor_GetSurfacePos(wk->negoPanels[i], &pos, CLACT_VRAM_SUB);
        if (i / 4) {
            pos.x += speed;
        } else {
            pos.x -= speed;
        }
        ClActor_SetSurfacePos(wk->negoPanels[i], &pos, CLACT_VRAM_SUB);
    }
    for (i = 0; i < 3; i++) {
        if (wk->negoIcons[0][i] != NULL) {
            ClActor_GetSurfacePos(wk->negoIcons[0][i], &leftPos, CLACT_VRAM_SUB);
            leftPos.x -= speed;
            ClActor_SetSurfacePos(wk->negoIcons[0][i], &leftPos, CLACT_VRAM_SUB);
        }
    }
    for (i = 0; i < 3; i++) {
        if (wk->negoIcons[1][i] != NULL) {
            ClActor_GetSurfacePos(wk->negoIcons[1][i], &rightPos, CLACT_VRAM_SUB);
            rightPos.x += speed;
            ClActor_SetSurfacePos(wk->negoIcons[1][i], &rightPos, CLACT_VRAM_SUB);
        }
    }
    GFL_BGSysMoveBG(5, BG_MOVE_SET_X, -wk->panelSlide);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_X, wk->panelSlide - 128);
    return FALSE;
}

// Moves the panels of a negotiation and their icons off the screen, each side past its edge, for them to slide in
static void Poketrade2D_SetCursorSlidePositions(PokemonTradeWork *wk) {
    ClActorPos pos, leftPos, rightPos;
    int i;

    for (i = 0; i < 8; i++) {
        ClActor_GetSurfacePos(wk->negoPanels[i], &pos, CLACT_VRAM_SUB);
        if (i / 4) {
            pos.x -= 128;
            ClActor_SetBgPriority(wk->negoPanels[i], 3);
        } else {
            pos.x += 128;
        }
        ClActor_SetSurfacePos(wk->negoPanels[i], &pos, CLACT_VRAM_SUB);
    }
    for (i = 0; i < 3; i++) {
        if (wk->negoIcons[0][i] != NULL) {
            ClActor_GetSurfacePos(wk->negoIcons[0][i], &leftPos, CLACT_VRAM_SUB);
            leftPos.x += 128;
            ClActor_SetSurfacePos(wk->negoIcons[0][i], &leftPos, CLACT_VRAM_SUB);
        }
    }
    for (i = 0; i < 3; i++) {
        if (wk->negoIcons[1][i] != NULL) {
            ClActor_GetSurfacePos(wk->negoIcons[1][i], &rightPos, CLACT_VRAM_SUB);
            rightPos.x -= 128;
            ClActor_SetSurfacePos(wk->negoIcons[1][i], &rightPos, CLACT_VRAM_SUB);
            ClActor_SetBgPriority(wk->negoIcons[1][i], 3);
        }
    }
    GFL_BGSysSetBGPriority(4, 2);
    GFL_BGSysSetBGPriority(5, 3);
}

// Sets up the lower screen's BGs for a negotiation, with its panels off the screen
void Poketrade2D_SetupCursorBg(PokemonTradeWork *wk) {
    ArcTool *arc;

    wk->cursorImage = LoadCursorImageEndOfHeap(6, 15, 0, wk->heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);
    wk->bg5Chars = GFL_BGSysLoadArcNCGRDynamic(arc, 13, 5, 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 7, 4, 0, CHAR_POS(wk->bg5Chars), 0, FALSE, wk->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 7, 5, 0, CHAR_POS(wk->bg5Chars), 0, FALSE, wk->heapId);
    {
        BGSetup setup = { 0,
                          0,
                          0x1000,
                          0,
                          BGRES_256x256,
                          GX_BG_COLORMODE_16,
                          GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x10000),
                          0x10000,
                          GX_BG_EXTPLTT_01,
                          3,
                          GX_BG_AREAOVER_XLU,
                          FALSE };

        GFL_BGSysReleaseBG(7);
        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
    }
    GFL_G2DIOLoadNSCRSync(arc, 8, 7, 0, CHAR_POS(wk->bg5Chars), 0, FALSE, wk->heapId);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_X, -128);
    GFL_BGSysMoveBG(7, BG_MOVE_SET_X, 0);
    GFL_BGSysSetBGPriority(4, 3);
    GFL_G2DIOLoadNSCRSync(arc, 11, 2, 0, CHAR_POS(wk->bg2Chars), 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    wk->panelSlide = 0;
    Poketrade2D_SetCursorSlidePositions(wk);
    GFL_BGSysSetEnabledBGsB(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 |
                            GX_PLANEMASK_OBJ);
}

// Sets up the panels of a negotiation: their bitmaps, the characters they are copied to, and their sprites
void Poketrade2D_CreateOfferWindows(PokemonTradeWork *wk) {
    ClActorPos positions[] = {
        { 96, 32 }, { 104, 64 }, { 104, 112 }, { 104, 160 }, { 224, 32 }, { 232, 64 }, { 232, 112 }, { 232, 160 },
    };
    ClActorSetup setup;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);
    int i;

    wk->negoPanelPltt = ClActSys_LoadPlttRange(arc, 0, CLACT_VRAM_SUB, 0x1c0, 0, 1, wk->heapId);
    wk->negoPanelCells = ClActSys_LoadCellAnims(arc, 2, 1, wk->heapId);
    for (i = 0; i < 8; i++) {
        wk->negoBitmaps[i] = GFL_BitmapCreate(16, 6, 32, wk->heapId);
        wk->negoPanelChars[i] = ClActSys_LoadChars(arc, 3, FALSE, CLACT_VRAM_SUB, wk->heapId);
    }
    for (i = 0; i < 8; i++) {
        setup.x = positions[i].x;
        setup.y = positions[i].y;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 1;
        wk->negoPanels[i] = ClActor_Create(wk->clactUnit, wk->negoPanelChars[i], wk->negoPanelPltt, wk->negoPanelCells,
                                           &setup, CLACT_VRAM_SUB, wk->heapId);
        ClActor_SetVisible(wk->negoPanels[i], TRUE);
    }
    GFL_ArcToolFree(arc);
}

void Poketrade2D_DeleteOfferWindowActors(PokemonTradeWork *wk) {
    int i;

    for (i = 0; i < 8; i++) {
        if (wk->negoPanels[i] != NULL) {
            ClActor_Delete(wk->negoPanels[i]);
            wk->negoPanels[i] = NULL;
            GFL_BitmapFree(wk->negoBitmaps[i]);
            wk->negoBitmaps[i] = NULL;
            ClActSys_FreeChars(wk->negoPanelChars[i]);
            wk->negoPanelChars[i] = 0;
        }
    }
    if (wk->negoPanelPltt != 0) {
        ClActSys_FreePltt(wk->negoPanelPltt);
        wk->negoPanelPltt = 0;
    }
#ifdef BUGFIX
    if (wk->negoPanelCells != 0) {
        ClActSys_FreeCellAnims(wk->negoPanelCells);
        wk->negoPanelCells = 0;
    }
#else
    // BUG: This tests the palette again, freed just above, so the cells are never freed, and it would free a panel's
    // characters
    if (wk->negoPanelPltt != 0) {
        ClActSys_FreeCellAnims(wk->negoPanelChars[2]);
        wk->negoPanelPltt = 0;
    }
#endif
}

// Copies the bitmaps of the panels of a negotiation to their characters, which take the 8 tiles of a row of the
// bitmap two rows apart
void Poketrade2D_UpdateOfferWindowActors(PokemonTradeWork *wk) {
    u8 order[] = { 0, 2, 4, 6, 1, 3, 5, 7 };
    int i, j;

    for (i = 0; i < 8; i++) {
        u32 dest = ClActSys_GetCharVramAddr(wk->negoPanelChars[i], TRUE);
        u8 *pixels = GFL_BitmapGetPixelData(wk->negoBitmaps[i]);

        DC_FlushRange(pixels, GFL_BitmapCalcPixelDataSize(wk->negoBitmaps[i]));
        for (j = 0; j < 8; j++) {
            GXS_LoadOBJ(pixels + order[j] * 0x100, dest, 0x100);
            dest += 0x100;
        }
    }
}

// Shows the background of a side's panel on the sub screen
void Poketrade2D_LoadMonPalette(PokemonTradeWork *wk, int side, PartyPkm *pkm) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEMONTRADE, wk->heapId);
    NNSG2dScreenData *screen;
    void *file;
    u16 *cells;
    u32 i;

    if (side == 0) {
        file = GFL_G2DIOReadNSCRArc(arc, 5, FALSE, &screen, wk->heapId);
    } else {
        file = GFL_G2DIOReadNSCRArc(arc, 6, FALSE, &screen, wk->heapId);
    }
    cells = (u16 *)screen->rawData;
    for (i = 0; i < screen->szByte / 2; i++) {
        cells[i] += CHAR_POS(wk->bg2Chars);
    }
    if (side == 0) {
        GFL_BGSysLoadScrAreaAll(1, cells, 0, 0, 16, 24);
    } else {
        GFL_BGSysLoadScrAreaAll(1, screen->rawData, 16, 0, 16, 24);
    }
    GFL_HeapFree(file);
    GFL_BGSysSetBGPriority(3, 2);
    GFL_BGSysSetBGPriority(2, 3);
    GFL_BGSysSetBGPriority(1, 1);
    GFL_BGSysSetBGPriority(0, 0);
    GFL_ArcToolFree(arc);
    GFL_BGSysQueueScrLoad(1);
}

static void Poketrade2D_OnCursorAnimDone(u32 param, fx32 frame) {
    PokemonTradeWork *wk = (PokemonTradeWork *)param;

    wk->boxCursorDone = TRUE;
}

// Shows the cursor over a box of the box list, at its top left corner
void Poketrade2D_CreateCursorActor(PokemonTradeWork *wk, u8 x, u8 y) {
    ClActorSetup setup;
    ClActorCallback callback;

    setup.x = x + 12;
    setup.y = y + 12;
    setup.sequence = 24;
    setup.priority = 0;
    setup.bgPriority = 0;
    Poketrade2D_DeleteCursorActor(wk);
    wk->boxCursor = ClActor_Create(wk->clactUnit, wk->objRes[TRADE_OBJRES_CHAR_SUB], wk->objRes[TRADE_OBJRES_PLTT_SUB],
                                   wk->objRes[TRADE_OBJRES_CELL_SUB], &setup, CLACT_VRAM_SUB, wk->heapId);
    ClActor_SetVisible(wk->boxCursor, TRUE);
    ClActor_SetAutoAnim(wk->boxCursor, TRUE);
    wk->boxCursorDone = FALSE;
    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.param = (u32)wk;
    callback.func = Poketrade2D_OnCursorAnimDone;
    ClActor_SetAnimCallback(wk->boxCursor, &callback);
}

void Poketrade2D_DeleteCursorActor(PokemonTradeWork *wk) {
    if (wk->boxCursor != NULL) {
        ClActor_Delete(wk->boxCursor);
        wk->boxCursor = NULL;
        wk->boxCursorDone = FALSE;
    }
}

// Removes the box list's cursor once its animation has ended; TRUE while it plays
BOOL Poketrade2D_ClearCursorIfDisabled(PokemonTradeWork *wk) {
    if (wk->boxCursorDone) {
        Poketrade2D_DeleteCursorActor(wk);
        return FALSE;
    }
    return TRUE;
}

// Dims the palettes of a standard palette memory (PALFADE_VRAM_*) in paletteMask
static void Poketrade2D_FadePaletteBank(const PokemonTradeWork *wk, u32 vram, u32 paletteMask) {
    PaletteFade *fade = PaletteFade_Create(wk->heapId);
    u8 *colors;
    int i;

    PaletteFade_AllocBuffer(fade, vram, 0x200, wk->heapId);
    PaletteFade_LoadFromVRAM(fade, vram, 0, 0x200);
    PaletteFade_BlendBuffer(fade, vram, 0, 0x100, 6, 0);
    colors = (u8 *)PaletteFade_GetFadedBuffer(fade, vram);
    for (i = 0; i < 16; i++) {
        if (paletteMask & (1 << i)) {
            DC_FlushRange(colors + i * 32, 32);
            switch (vram) {
            case PALFADE_VRAM_SUB_OBJ:
                GXS_LoadOBJPltt(colors + i * 32, i * 32, 32);
                break;
            case PALFADE_VRAM_SUB_BG:
                GXS_LoadBGPltt(colors + i * 32, i * 32, 32);
                break;
            case PALFADE_VRAM_MAIN_OBJ:
                GX_LoadOBJPltt(colors + i * 32, i * 32, 32);
                break;
            case PALFADE_VRAM_MAIN_BG:
                GX_LoadBGPltt(colors + i * 32, i * 32, 32);
                break;
            }
        }
    }
    PaletteFade_FreeBuffer(fade, vram);
    PaletteFade_Free(fade);
}

// Dims the palettes in paletteMask of the lower screen's OBJ or BG palettes, keeping a copy of them, or puts the copy
// back
static void Poketrade2D_FadePaletteCopy(PokemonTradeWork *wk, BOOL dim, u16 vram, u32 paletteMask) {
    if (dim) {
        if (vram == PALFADE_VRAM_SUB_OBJ) {
            MI_CpuCopy8((void *)HW_DB_OBJ_PLTT, wk->savedObjPalette, 0x200);
        } else if (vram == PALFADE_VRAM_SUB_BG) {
            MI_CpuCopy8((void *)HW_DB_BG_PLTT, wk->savedBGPalette, 0x200);
        }
        Poketrade2D_FadePaletteBank(wk, vram, paletteMask);
    } else if (vram == PALFADE_VRAM_SUB_OBJ) {
        DC_FlushRange(wk->savedObjPalette, 0x200);
        GXS_LoadOBJPltt(wk->savedObjPalette, 0, 0x200);
    } else if (vram == PALFADE_VRAM_SUB_BG) {
        DC_FlushRange(wk->savedBGPalette, 0x200);
        GXS_LoadBGPltt(wk->savedBGPalette, 0, 0x200);
    }
}

// Dims the lower screen's OBJ palettes but the first, or brings them back
void Poketrade2D_FadePaletteAll(PokemonTradeWork *wk, BOOL dim) {
    Poketrade2D_FadePaletteCopy(wk, dim, PALFADE_VRAM_SUB_OBJ, 0xfffe);
}

// Sets the planes of the main screen shown, in the next V-blank
void Poketrade2D_ScheduleMainPlaneVisible(PokemonTradeWork *wk, int planes) {
    GFL_TCBMgrAddTask(GFL_VBlankGetTCBMgr(), Poketrade2D_TaskSetMainPlaneVisible, (void *)planes, 10);
}

// Sets the planes of the sub screen shown, in the next V-blank
void Poketrade2D_ScheduleSubPlaneVisible(PokemonTradeWork *wk, int planes) {
    GFL_TCBMgrAddTask(GFL_VBlankGetTCBMgr(), Poketrade2D_TaskSetSubPlaneVisible, (void *)planes, 10);
}

static void Poketrade2D_TaskSetMainPlaneVisible(TCB *tcb, void *data) {
    GFL_BGSysSetEnabledBGsA((int)data);
    GFL_TCBRemove(tcb);
}

static void Poketrade2D_TaskSetSubPlaneVisible(TCB *tcb, void *data) {
    GFL_BGSysSetEnabledBGsB((int)data);
    GFL_TCBRemove(tcb);
}

// Starts a point along the curve from start to end, over frames
static void Poketrade2D_InitBezierCurve(TradeCurve *curve, VecFx32 *start, VecFx32 *control1, VecFx32 *control2,
                                        VecFx32 *end, int frames) {
    curve->pos = *start;
    curve->points[0] = *start;
    curve->points[1] = *control1;
    curve->points[2] = *control2;
    curve->points[3] = *end;
    curve->frame = 0;
    curve->frames = frames;
}

// Moves the point to the curve's next frame; TRUE once it is at the end
static BOOL Poketrade2D_StepBezierCurve(TradeCurve *curve) {
    fx32 t, t2, s, s2, b0, b1, b2, b3;

    if (curve->frame < curve->frames - 1) {
        t = FX_Div(FX32_CONST(curve->frame), FX32_CONST(curve->frames));
        t2 = FX_Mul(t, t);
        s = FX32_ONE - t;
        s2 = FX_Mul(s, s);
        b1 = FX_Mul(3 * t, s2);
        b0 = FX_Mul(s2, s);
        b2 = FX_Mul(3 * t2, s);
        b3 = FX_Mul(t2, t);
        curve->pos.x = FX_Mul(curve->points[0].x, b0) + FX_Mul(curve->points[1].x, b1) +
                       FX_Mul(curve->points[2].x, b2) + FX_Mul(curve->points[3].x, b3);
        curve->pos.y = FX_Mul(curve->points[0].y, b0) + FX_Mul(curve->points[1].y, b1) +
                       FX_Mul(curve->points[2].y, b2) + FX_Mul(curve->points[3].y, b3);
        curve->pos.z = FX_Mul(curve->points[0].z, b0) + FX_Mul(curve->points[1].z, b1) +
                       FX_Mul(curve->points[2].z, b2) + FX_Mul(curve->points[3].z, b3);
        curve->frame++;
        return FALSE;
    }
    curve->pos = curve->points[3];
    return TRUE;
}
