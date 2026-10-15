#ifndef POKEBW2_APP_POKEMON_TRADE_LOCAL_H
#define POKEBW2_APP_POKEMON_TRADE_LOCAL_H

#include "types.h"
#include "app/pokemon_trade.h"
#include "app/pokemon_trade/pokemontrade_2d.h"
#include "app/pokemon_trade/pokemontrade_3d.h"
#include "app/pokemon_trade/pokemontrade_mcss.h"
#include "app/pokemon_trade/pokemontrade_message.h"
#include "app/pokemon_trade/pokemontrade_nego.h"
#include "app/pokemon_trade/pokemontrade_proc.h"
#include "app/pokemon_trade/pokemontrade_save.h"
#include "app/pokemon_trade/pokemontrade_touchbar.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "field/unity_tower.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/nhttp_rap.h"
#include "gfl/particle.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/mcss.h"
#include "system/net_save.h"
#include "system/printsys.h"
#include "system/time_icon.h"

// The trade's own declarations, shared by the files of overlay 194 and the trade demo overlays 192 and 193. The work
// is one struct that every file uses

// The network commands, from TRADE_NET_CMD_BASE. The library takes a command as a u16, which func_02042be8's
// prototype doesn't say, so a computed command is cast
#define TRADE_NET_CMD_BASE 0xc00
#define TRADE_NET_CMD_SELECT (TRADE_NET_CMD_BASE + 0)
#define TRADE_NET_CMD_UNK1 (TRADE_NET_CMD_BASE + 1)
#define TRADE_NET_CMD_UNK3 (TRADE_NET_CMD_BASE + 3)
#define TRADE_NET_CMD_UNK6 (TRADE_NET_CMD_BASE + 6)
#define TRADE_NET_CMD_OFFER (TRADE_NET_CMD_BASE + 7)
#define TRADE_NET_CMD_UNKA (TRADE_NET_CMD_BASE + 0xa)
#define TRADE_NET_CMD_WITHDRAW (TRADE_NET_CMD_BASE + 0xb)
#define TRADE_NET_CMD_UNKC (TRADE_NET_CMD_BASE + 0xc)
#define TRADE_NET_CMD_UNKD (TRADE_NET_CMD_BASE + 0xd)
#define TRADE_NET_CMD_UNKE (TRADE_NET_CMD_BASE + 0xe)
#define TRADE_NET_CMD_UNKF (TRADE_NET_CMD_BASE + 0xf)
#define TRADE_NET_CMD_UNK10 (TRADE_NET_CMD_BASE + 0x10)
#define TRADE_NET_CMD_CHECK_RESULT (TRADE_NET_CMD_BASE + 0x11)
#define TRADE_NET_CMD_UNK12 (TRADE_NET_CMD_BASE + 0x12)
#define TRADE_NET_CMD_UNK13 (TRADE_NET_CMD_BASE + 0x13)
#define TRADE_NET_CMD_UNK16 (TRADE_NET_CMD_BASE + 0x16)
#define TRADE_NET_CMD_UNK17 (TRADE_NET_CMD_BASE + 0x17)

// The copies of the save's data that a trade changes, to restore if saving fails
typedef struct {
    void *mail;
    void *records;
    void *survey;
    void *pokedex;
    // The wifi list's block of the players met
    void *playersMet;
    PokeParty *party;
    void *box;
    u32 unk1C;
    BOOL chatot;
} TradeBackup;

// A fade of the screens' brightness over a number of frames, from one level to another
typedef struct {
    s8 frames;
    s8 frame;
    s8 start;
    s8 end;
} TradeFade;

// The icons of a Pokémon's six markings in the summary, then of its rare and Pokérus flags, all made from one set of
// overlay 139's resources
typedef struct {
    UIObjRes res;
    BOOL loaded;
    ClActor *icons[8];
} TradeMarkIcons;

// A cubic Bézier curve that a point follows over a number of frames: the point, the curve's four control points, and
// the frame it is at
typedef struct {
    VecFx32 pos;
    VecFx32 points[4];
    int frame;
    int frames;
} TradeCurve;

// The colour of each slot of a machine's boxes and party, as indices into the box palette, which
// pokemontrade_3d.c draws into the textures of the boxes' cubes
typedef struct {
    u8 boxes[24][30];
    u8 party[6];
} TradeBoxColors;

// The OBJ resources of the work's objRes: the palettes, the characters, then the cells with their animations, of the
// Pokémon icons, the lower screen's sprites (the touch bar's and the icons' cursors), the upper screen's, the common
// UI's, the trade demo's and the negotiation's icons
enum {
    TRADE_OBJRES_PLTT_ICON,
    TRADE_OBJRES_PLTT_SUB,
    TRADE_OBJRES_PLTT_MAIN,
    TRADE_OBJRES_PLTT_COMMON,
    TRADE_OBJRES_PLTT_DEMO,
    TRADE_OBJRES_PLTT_NEGO,
    TRADE_OBJRES_CHAR_SUB,
    TRADE_OBJRES_CHAR_MAIN,
    TRADE_OBJRES_CHAR_ICON0,
    TRADE_OBJRES_CHAR_ICON1,
    TRADE_OBJRES_CHAR_COMMON,
    TRADE_OBJRES_CHAR_DEMO,
    TRADE_OBJRES_CELL_ICON,
    TRADE_OBJRES_CELL_SUB,
    TRADE_OBJRES_CELL_MAIN,
    TRADE_OBJRES_CELL_COMMON,
    TRADE_OBJRES_CELL_DEMO,
    TRADE_OBJRES_CELL_NEGO,
    TRADE_OBJRES_COUNT,
};

// The first of each kind of resource in objRes
#define TRADE_OBJRES_PLTT TRADE_OBJRES_PLTT_ICON
#define TRADE_OBJRES_CHAR TRADE_OBJRES_CHAR_SUB
#define TRADE_OBJRES_CELL TRADE_OBJRES_CELL_ICON

struct PokemonTradeWork {
    // The HTTP connection of a trade over Wi-Fi
    NHttpRap *unk0;
    u8 unk4[2];
    u8 unk6[0x2];
    PokemonTradeParam *param;
    // The 3D effects of the trade demo, which overlays 192 and 193 allocate
    PokemonTrade3DWork *work3D;
    // The colours of each machine's box slots, by network ID
    TradeBoxColors boxColors[2];
    BmpWin *unk5BC[2];
    BmpWin *unk5C4[2];
    BmpWin *unk5CC[2];
    BmpWin *unk5D4;
    BmpWin *unk5D8;
    // The Pokémon each machine offers, by network ID
    PartyPkm *pkm[2];
    u32 unk5E4[2];
    // The step that runs each frame
    PokemonTradeState state;
    // The icon that shows while the machines wait on each other
    WaitIcon *waitIcon;
    HeapID heapId;
    u8 unk5F6[0x16];
    TradeBackup backup;
    // The number of boxes; the party is the box after the last
    int boxCount;
    // The columns of the scrolling strip of Pokémon: two of the party, then six for each box
    int columnCount;
    int stripTileWidth;
    int stripWidth;
    u8 unk640[0x4];
    // The message cursor's place in the BG's characters
    u32 cursorImage;
    AppTaskMenuWin *menuWin;
    AppTaskMenu *menu;
    // The items of the menu, built for each menu
    AppTaskMenuItem menuItems[8];
    AppTaskMenuRes *taskMenuRes;
    // The window of a Pokémon's summary
    BmpWin *summaryWindow;
    // The windows of the boxes' names over the strip, and the party's
    BmpWin *boxNameWindows[25];
    BmpWin *msgWindow;
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    PrintStream *printStream;
    // The text of the panels, and its message before the words go in
    StrBuf *drawStr;
    StrBuf *drawTemplate;
    StrBuf *strbuf;
    StrBuf *strbufTemplate;
    PrintQueue *printQueue;
    TCBExManager *tcbEx;
    u8 unk748[0x4];
    KeyCursor *keyCursor;
    // The windows of the letters to search the boxes by initial
    BmpWin *initialWindows[26];
    u8 unk7B8[0x48];
    u32 unk800;
    u8 unk804[0xc];
    G3DCamera *camera;
    u8 unk814[0x4];
    TradeFade *fade;
    u8 unk81C[0x4];
    // The 3D scene in front of the sprites, and its index in the manager
    G3DManager *sceneMgr;
    u16 scene;
    u8 unk826[0x2];
    BoxSaveAccessor *boxes;
    GameData *gameData;
    PlayerInfo *myInfo;
    PlayerInfo *partnerInfo;
    PokeParty *party;
    // The scene's camera
    G3DCamera *sceneCamera;
    MCSSSystem *mcssSys;
    MCSS *mcss[4];
    u32 unk854;
    u32 unk858;
    // The moves of the demo's sprites
    TradeMcssMove *mcssMoves[4];
    // The BG characters of frames 2, 5 and 7, as GFL_BGSysLoadArcNCGRDynamic returns them
    u32 bg2Chars;
    u32 bg5Chars;
    u32 bg7Chars;
    u8 unk878[0x4];
    // The OBJ resources, indexed by TRADE_OBJRES_*
    u32 objRes[18];
    // The Poké Ball icons of the summary and of the two sides
    PokemonTradeSprite ballIcons[3];
    // The type icons: two pages of two, the page turning in the summary
    PokemonTradeSprite typeIcons[4];
    // The icons of the held item, of Pokérus and of Pokérus cured, over a side's panel or in the summary
    PokemonTradeSprite infoIcons[3];
    // The icons of the summary's markings and of its rare and Pokérus flags
    TradeMarkIcons markIcons;
    TouchBar *touchBar;
    ClActUnit *clactUnit;
    TCB *vblankTcb;
    // The Pokémon icons of the twelve columns of the strip that are set up, five to a column: their characters, their
    // sprites and the cursors under them
    u32 iconChars[12][5];
    u8 unkABC[0x4];
    ClActor *icons[12][5];
    ClActor *iconCursors[12][5];
    // What each icon shows, to redraw it only when that changes, and the column of the strip it is in, or 0xff
    u16 iconSpecies[12][5];
    u8 iconForms[12][5];
    u8 iconSexes[12][5];
    u8 iconColumns[12][5];
    // Where each marked icon is, and whether it is marked, matching the box the partner looks at
    ClActorPos iconPos[12][5];
    u8 iconMarked[12][5];
    // The save between the two machines
    NetSave *netSave;
    ClActor *actors[10];
    // The cursor over a box of the box list, and whether its animation has ended
    ClActor *boxCursor;
    u8 unkF28[0x4];
    int timer;
    u8 unkF30[0x4];
    // Whether the screen was touched last frame
    BOOL touchHeld;
    u8 unkF38[0x1c];
    int touchX;
    int touchY;
    u32 unkF5C;
    // The Pokémon icon held by the stylus, where it was picked up and the stylus's offset from it
    ClActor *heldIcon;
    ClActorPos heldPos;
    s16 heldOffsetX;
    s16 heldOffsetY;
    // Where the held icon was the frame before, which gives the motion it is let go with
    ClActorPos heldPrevPos;
    u32 unkF70;
    // The cursor of the keys in the strip
    int cursorColumn;
    int cursorRow;
    int heldSlot;
    int heldBox;
    int selectSlot;
    int selectBox;
    u8 unkF8C[0x8];
    int unkF94;
    int unkF98;
    u32 unkF9C;
    int unkFA0;
    // The Pokémon each player offers in a negotiation: their slots, boxes and copies
    int negoSlot[2][3];
    int negoBox[2][3];
    // The cursor over the panels of a negotiation
    ClActor *negoCursor;
    PartyPkm *negoPkm[2][3];
    u32 unkFF0[2][3];
    // The icons of the Pokémon each player offers in a negotiation, and their characters
    ClActor *negoIcons[2][3];
    u32 negoIconChars[2][3];
    BmpWin *unk1038[4];
    u32 unk1048[2];
    u32 unk1050[2];
    u8 unk1058[0x8];
    u32 unk1060;
    // Where each machine's Pokémon arrives
    PartyPkm *recvPkm[3];
    u8 unk1070[0x4];
    u32 unk1074;
    // The stylus's x on the scroll bar last frame, and the speed of the scroll
    s16 scrollTouchX;
    s16 scrollSpeed;
    s16 scrollX;
    s16 unk107E;
    s16 firstColumn;
    u8 unk1082[0x2];
    u32 unk1084;
    u32 unk1088;
    int unk108C;
    int unk1090;
    u32 bgmTimer;
    u16 typeIconPage;
    u16 unk109A;
    // The scene of pokemontrade_3d.c's table that is set up, or -1
    int sceneId;
    // The side, or the one of the six Pokémon of a negotiation, that the cursor is on
    int cursor;
    // Copies of the lower screen's OBJ and BG palettes, kept while they are dimmed
    void *savedObjPalette;
    void *savedBGPalette;
    // The stamps a negotiation's players send: for each side its balloon and then its stamp, and the four buttons
    ClActor *stamps[4];
    ClActor *stampButtons[4];
    // The panels of a negotiation, for each side the player's name and then the three Pokémon: their sprites,
    // resources and bitmaps
    ClActor *negoPanels[8];
    u32 negoPanelPltt;
    u32 negoPanelCells;
    GFLBitmap *negoBitmaps[8];
    u32 negoPanelChars[8];
    // The palette of the box slots' colours, and the same a step darker
    GXRgb boxPalette[16];
    GXRgb boxPaletteDim[16];
    // The strip's screen files, read whole: the box and party backgrounds, plain and marked
    u16 *stripScreens[4];
    // The characters of every Pokémon's icon in the boxes, 0x200 bytes each
    u8 *iconCharData;
    u8 unk1188[0x4];
    int type;
    // The curve the carried icon flies along when it is let go, and the frames left of its flight
    TradeCurve curve;
    // The wait on the message being printed
    AppPrintsysCommon printWait;
    s16 curveTimer;
    // How far the panels of a negotiation have slid off
    s16 panelSlide;
    // The command each machine sent last
    u8 command[2];
    u8 unk11E2[2];
    u8 unk11E4[2];
    // The results of the server's check of the Pokémon
    u8 checkResult;
    u8 partnerCheckResult;
    u8 unk11E8[2];
    // The other machine's number of boxes
    u8 unk11EA;
    // Whether the lower screen's BGs, and the trade demo's BGs 6 and 7, are set up
    u8 subBGsCreated;
    u8 demoBGsCreated;
    // Frames to wait before going on, after a message
    u8 unk11ED;
    u8 unk11EE;
    u8 unk11EF;
    u8 unk11F0[0x1];
    u8 waitTimer;
    u8 partnerBoxCount;
    u8 unk11F3;
    u8 unk11F4;
    u8 boxCursorDone;
    u8 nationalDex;
    u8 unk11F7;
    u8 bgmCount;
    u8 unk11F9;
    // How many times the server checked the Pokémon
    u8 checkCount;
    u8 unk11FB_0 : 4;
    u8 unk11FB_4 : 4;
    // The icons of the party and of every box
    TradeBoxEntry boxEntries[726];
};

// The machine's own network ID
static inline int PokemonTrade_GetMyNetId(void) {
    return func_02042a6c(func_02040440());
}

// Sends a command of one byte to the other machine
static inline BOOL PokemonTrade_SendSelect(u8 command) {
    return func_02042be8(func_02040440(), TRADE_NET_CMD_SELECT, 1, &command);
}

// Whether the screen was touched, switching the controls to the touch screen if so
static inline BOOL PokemonTrade_GetTouched(void) {
    BOOL touched = func_0203da48();
    if (touched) {
        func_0203d564(TRUE);
    }
    return touched;
}

// The pressed keys, switching the controls to the keys when there are any
static inline u32 PokemonTrade_GetPressedKeys(void) {
    u32 keys = GCTX_HIDGetPressedKeys();
    if (keys) {
        func_0203d564(FALSE);
    }
    return keys;
}

// The trade demo, overlays 192 and 193
void PokemonTradeIrcDemo_Start(PokemonTradeWork *wk);
void PokemonTradeDemoCommon_Start(PokemonTradeWork *wk);
void PokemonTradeDemoDown_Start(PokemonTradeWork *wk);
void PokemonTradeDemoMiddle_Start(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_LOCAL_H
