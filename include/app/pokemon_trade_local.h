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

// The network commands, from TRADE_NET_CMD_BASE. The library takes a command as a u16, which GFL_NetSendCommand's
// prototype doesn't say, so a computed command is cast. Commands 2, 4 and 5 are never sent
#define TRADE_NET_CMD_BASE 0xc00
#define TRADE_NET_CMD_SELECT (TRADE_NET_CMD_BASE + 0)
// The Pokémon offered, which the other machine acknowledges with TRADE_NET_CMD_MON_ACK
#define TRADE_NET_CMD_MON (TRADE_NET_CMD_BASE + 1)
// The side the cursor chose, before the trade's animation
#define TRADE_NET_CMD_CURSOR (TRADE_NET_CMD_BASE + 3)
// The Pokémon offered is taken back
#define TRADE_NET_CMD_CLEAR_MON (TRADE_NET_CMD_BASE + 6)
// The three Pokémon offered in a negotiation, from TRADE_NET_CMD_OFFER
#define TRADE_NET_CMD_OFFER (TRADE_NET_CMD_BASE + 7)
#define TRADE_NET_CMD_READY (TRADE_NET_CMD_BASE + 0xa)
#define TRADE_NET_CMD_WITHDRAW (TRADE_NET_CMD_BASE + 0xb)
// Where the strip is scrolled to
#define TRADE_NET_CMD_SCROLL (TRADE_NET_CMD_BASE + 0xc)
// The stamp a negotiation's player touched
#define TRADE_NET_CMD_STAMP (TRADE_NET_CMD_BASE + 0xd)
#define TRADE_NET_CMD_BOX_COLORS (TRADE_NET_CMD_BASE + 0xe)
// The label over a side's Pokémon, a TRADE_STATUS_*
#define TRADE_NET_CMD_STATUS (TRADE_NET_CMD_BASE + 0xf)
// The number of boxes and whether the mailbox is full
#define TRADE_NET_CMD_BOX_INFO (TRADE_NET_CMD_BASE + 0x10)
#define TRADE_NET_CMD_CHECK_RESULT (TRADE_NET_CMD_BASE + 0x11)
// The player's profile, for the Unity Tower's survey
#define TRADE_NET_CMD_PROFILE (TRADE_NET_CMD_BASE + 0x12)
// Whether each of the three Pokémon offered in a negotiation is from the party, from TRADE_NET_CMD_NEGO_IN_PARTY
#define TRADE_NET_CMD_NEGO_IN_PARTY (TRADE_NET_CMD_BASE + 0x13)
// Whether the Pokémon offered is from the party
#define TRADE_NET_CMD_IN_PARTY (TRADE_NET_CMD_BASE + 0x16)
#define TRADE_NET_CMD_MON_ACK (TRADE_NET_CMD_BASE + 0x17)

// The kinds of trade, by the proc that starts it. No proc starts types 5 and 9
enum {
    TRADE_TYPE_IRC,
    TRADE_TYPE_UNION,
    TRADE_TYPE_WIFICLUB,
    TRADE_TYPE_GTSNEGO,
    // The infrared trade's animation alone, which the in-game trades show
    TRADE_TYPE_IRCDEMO,
    // The Global Trade Station's animations: a Pokémon deposited, taken out and traded
    TRADE_TYPE_GTS_DEPOSIT = 6,
    TRADE_TYPE_GTS_WITHDRAW,
    TRADE_TYPE_GTS_TRADE,
};

// What a machine sends with TRADE_NET_CMD_SELECT, which each machine keeps in command: a choice of the menus, or why
// the trade can't go on
enum {
    TRADE_SELECT_NONE,
    TRADE_SELECT_OFFER,
    TRADE_SELECT_TRADE,
    TRADE_SELECT_CANCEL,
    TRADE_SELECT_END,
    // The Pokémon offered is the last that can fight in the party
    TRADE_SELECT_LAST_PKM,
    // The Pokémon offered holds mail and the other machine's mailbox is full
    TRADE_SELECT_MAIL_FULL,
    // A bad egg or an invalid species, this machine's or the other's
    TRADE_SELECT_BAD_PKM,
    TRADE_SELECT_PARTNER_BAD_PKM,
    // Never sent: its message says the Pokémon knows an HM move
    TRADE_SELECT_HM_MOVE,
    // A negotiation: both players agree on the Pokémon offered, one chose one of the other's, or wants to offer
    // others, and the answer to the trade of the two chosen
    TRADE_SELECT_NEGO_AGREE,
    TRADE_SELECT_NEGO_CHOOSE,
    TRADE_SELECT_NEGO_RESELECT,
    TRADE_SELECT_NEGO_TRADE,
    TRADE_SELECT_NEGO_CANCEL,
};

// What the server's check of the other player's Pokémon found, or the HTTP error that kept it from answering
enum {
    TRADE_CHECK_OK,
    TRADE_CHECK_INVALID,
    TRADE_CHECK_HTTP_400 = 0xf0,
    TRADE_CHECK_HTTP_401,
    TRADE_CHECK_HTTP_408,
    TRADE_CHECK_HTTP_ERROR = 0xff,
};

// The label over each side's Pokémon in a negotiation, from its message; TRADE_STATUS_DRAWN once it is drawn
enum {
    TRADE_STATUS_SELECTING,
    TRADE_STATUS_SELECTED,
    TRADE_STATUS_WAITING,
    TRADE_STATUS_DRAWN,
};

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
    // The play time's last save, which the party's copy restores
    u32 lastSaved;
    // Whether the Chatot's cry was forgotten
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
    NHttpRap *http;
    u8 unk4[2];
    u8 unk6[0x2];
    PokemonTradeParam *param;
    // The 3D effects of the trade demo, which overlays 192 and 193 allocate
    PokemonTrade3DWork *work3D;
    // The colours of each machine's box slots, by network ID
    TradeBoxColors boxColors[2];
    // The window over each side's Pokémon on the upper screen: its panel, or its label in a negotiation
    BmpWin *sideWindows[2];
    // The windows of each side's small panel on the upper screen: the Pokémon's name and level, and its held item
    BmpWin *cardWindows[2];
    BmpWin *cardItemWindows[2];
    // The windows of a negotiation's upper screen: what a GTS Negotiation's player wants, and the players' names
    BmpWin *headerWindow;
    BmpWin *nameWindow;
    // The Pokémon each machine offers, by network ID
    PartyPkm *pkm[2];
    // Whether the Pokémon each machine offers is from its party, by network ID
    u32 inParty[2];
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
    // The initial searched for in the list of letters, plus one, or 0
    u32 searchInitial;
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
    // Whether the held icon has moved from where it was picked up
    u32 heldMoved;
    // The Pokémon icon held by the stylus, where it was picked up and the stylus's offset from it
    ClActor *heldIcon;
    ClActorPos heldPos;
    s16 heldOffsetX;
    s16 heldOffsetY;
    // Where the held icon was the frame before, which gives the motion it is let go with
    ClActorPos heldPrevPos;
    // Whether the held icon is still only tapped, not dragged away
    u32 heldTap;
    // The cursor of the keys in the strip
    int cursorColumn;
    int cursorRow;
    int heldSlot;
    int heldBox;
    int selectSlot;
    int selectBox;
    u8 unkF8C[0x8];
    // The Pokémon the player picked, whose summary shows or which is taken out of the offers
    int targetSlot;
    int targetBox;
    // The network ID, plus one, of the machine whose Pokémon arrived, to show once the summary is closed
    int pendingSideMon;
    // The side the other machine's cursor chose
    int partnerCursor;
    // The Pokémon each player offers in a negotiation: their slots, boxes and copies
    int negoSlot[2][3];
    int negoBox[2][3];
    // The cursor over the panels of a negotiation
    ClActor *negoCursor;
    PartyPkm *negoPkm[2][3];
    // Whether each Pokémon a player offers in a negotiation is from the party
    u32 negoInParty[2][3];
    // The icons of the Pokémon each player offers in a negotiation, and their characters
    ClActor *negoIcons[2][3];
    u32 negoIconChars[2][3];
    // Never created, only freed with the panels of a negotiation
    BmpWin *offerListWindows[4];
    // In a GTS Negotiation, the kind of Pokémon each player wants and the level of the first's, or -1
    u32 wantedKind[2];
    u32 wantedLevel[2];
    u8 unk1058[0x8];
    // Whether the other machine sent TRADE_NET_CMD_READY
    u32 partnerReady;
    // Where each machine's Pokémon arrives
    PartyPkm *recvPkm[3];
    u8 unk1070[0x4];
    u32 unk1074;
    // The stylus's x on the scroll bar last frame, and the speed of the scroll
    s16 scrollTouchX;
    s16 scrollSpeed;
    s16 scrollX;
    // Where the other machine's strip is scrolled to
    s16 partnerScrollX;
    s16 firstColumn;
    u8 unk1082[0x2];
    // Whether the V-blank sets the strip's BGs to bgScrollX, and whether the stylus was on the strip last frame
    u32 bgScrollPending;
    u32 browseTouchHeld;
    int bgScrollX;
    int unk1090;
    u32 bgmTimer;
    u16 typeIconPage;
    // Whether the Pokémon the server found invalid were renamed, before checking them again
    u16 checkedPkmRenamed;
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
    // The kind of trade, a TRADE_TYPE_*
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
    // The frames left of each side's panel of the Pokémon offered, and of each side's stamp
    u8 offerCardTimers[2];
    u8 stampTimers[2];
    // The results of the server's check of the Pokémon, TRADE_CHECK_*
    u8 checkResult;
    u8 partnerCheckResult;
    // The label over each side's Pokémon, a TRADE_STATUS_*
    u8 statusLabels[2];
    // Whether the labels over the sides' Pokémon are drawn
    u8 showStatusLabels;
    // Whether the lower screen's BGs, and the trade demo's BGs 6 and 7, are set up
    u8 subBGsCreated;
    u8 demoBGsCreated;
    // For the trade's animation: whether the Pokémon sent goes back to the partner, its original trainer, and whether
    // it doesn't hop, and the same of the Pokémon received
    u8 sentPkmReturns;
    u8 recvPkmReturns;
    u8 sentPkmNoBounce;
    u8 recvPkmNoBounce[0x1];
    // Frames to wait before going on, after a message
    u8 waitTimer;
    u8 partnerBoxCount;
    // Whether a negotiation's summary is open
    u8 inSummary;
    u8 partnerMailBoxFull;
    u8 boxCursorDone;
    u8 nationalDex;
    // Whether a negotiated trade, which keeps no copies, is in the save data but not yet saved: a lost connection
    // then shows its error
    u8 tradeUnsaved;
    u8 bgmCount;
    // Whether the upper screen's windows wait for the V-blank
    u8 wndSetupPending;
    // How many times the server checked the Pokémon
    u8 checkCount;
    // Whether a Pokémon was sent and not yet acknowledged, and whether one arrived and is to be acknowledged
    u8 awaitingMonAck : 4;
    u8 monAckPending : 4;
    // The icons of the party and of every box
    TradeBoxEntry boxEntries[726];
};

// The machine's own network ID
static inline int PokemonTrade_GetMyNetId(void) {
    return GFL_NetGetHandleNetId(NetHandle_GetOwn());
}

// Sends a command of one byte to the other machine
static inline BOOL PokemonTrade_SendSelect(u8 command) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_SELECT, 1, &command);
}

// Whether the screen was touched, switching the controls to the touch screen if so
static inline BOOL PokemonTrade_GetTouched(void) {
    BOOL touched = GFL_HIDIsTouchPressed();
    if (touched) {
        GCTX_HIDSetUsingTouch(TRUE);
    }
    return touched;
}

// The pressed keys, switching the controls to the keys when there are any
static inline u32 PokemonTrade_GetPressedKeys(void) {
    u32 keys = GCTX_HIDGetPressedKeys();
    if (keys) {
        GCTX_HIDSetUsingTouch(FALSE);
    }
    return keys;
}

// The trade demo, overlays 192 and 193
void PokemonTradeIrcDemo_Start(PokemonTradeWork *wk);
void PokemonTradeDemoCommon_Start(PokemonTradeWork *wk);
void PokemonTradeDemoDown_Start(PokemonTradeWork *wk);
void PokemonTradeDemoMiddle_Start(PokemonTradeWork *wk);

#endif // POKEBW2_APP_POKEMON_TRADE_LOCAL_H
