#ifndef POKEBW2_APP_BOX2_MAIN_H
#define POKEBW2_APP_BOX2_MAIN_H

#include "types.h"
#include "app/box2.h"
#include "app/name_entry.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The PC box's work and the functions of box2_main.c, which the sequences in box2_seq.c call. Names are ours

// The number of boxes once all are open
#define BOX2_TRAY_MAX 24
// No tray or position
#define BOX2_GET_NONE 0xff
// Positions: a tray's 30 Pokémon, then the party's 6, then the boxes of the box list
#define BOX2_TRAY_POKE_MAX 30
#define BOX2_PARTY_POS 30
#define BOX2_BOXLIST_POS 36

// The directions of a box change
enum {
    BOX2_TRAY_SCROLL_L,
    BOX2_TRAY_SCROLL_R,
    BOX2_TRAY_SCROLL_NONE,
};

// The actors of the cursor and the item icon
#define BOX2_ACTOR_CURSOR 4
#define BOX2_ACTOR_ITEM_ICON 12

// The frames the item icon takes to move
#define BOX2_ITEMMOVE_CNT 8

// The frames a Pokémon icon takes to move
#define BOX2_POKEMOVE_CNT 8

// Why a Pokémon can't be put where it was dropped, or picked, which Box2Bmp_PrintDenyMsg tells for some
enum {
    BOX2_MOVE_ERR_NONE,
    BOX2_MOVE_ERR_BOX_FULL,
    BOX2_MOVE_ERR_MAIL,
    BOX2_MOVE_ERR_LAST_BATTLER,
    // "Eggs cannot be selected!": the Battle Box takes no Eggs, nor does the Dream World
    BOX2_MOVE_ERR_EGG,
    BOX2_MOVE_ERR_BATTLE_BOX_LOCKED,
    // "This Pokémon doesn't seem sleepy now.": a species that can't be sent to the Dream World
    BOX2_MOVE_ERR_NOT_SLEEPY,
    // The Pokémon moved as a group don't fit where they were dropped
    BOX2_MOVE_ERR_GROUP,
};

// Where a button's press animation is: on an actor or on the BG
enum {
    BOX2_BTN_ANM_MODE_OBJ,
    BOX2_BTN_ANM_MODE_BG,
};

// The Pokémon search's criteria, which the boxes' icons are filtered by
typedef struct {
    u16 species;
    // 1 when the Pokémon must hold no item, 2 when it must hold one
    u8 item;
    u8 marks;
    // The nature and the sex, plus 1, or 0 for any
    u8 nature;
    u8 ability;
    u8 sex;
    u8 active;
} Box2SearchParam;

struct Box2SysWork {
    Box2Param *param;
    GameProcManager *procManager;
    BOOL procMgrResult;
    void *subProcWork;
    u16 subRet;
    u8 subProcType;
    u8 unk13;
    u8 tray;
    u8 trayMax;
    u8 pos;
    // The tray of the held Pokémon, or BOX2_GET_NONE
    u8 getTray;
    u8 unk18;
    u8 trayScroll;
    u8 unk1A;
    u8 unk1B;
    u8 moveMode : 4;
    u8 unk1C_4 : 2;
    u8 unk1C_6 : 2;
    u8 unk1D;
    u8 unk1E;
    u8 unk1F;
    u8 unk20;
    u8 unk21;
    u8 unk22;
    u8 unk23;
    u32 curRcvPos;
    int nextSeq;
    Box2AppWork *app;
    Box2SearchParam search;
};

// A text object on OAM
typedef struct {
    BmpOamActor *oam;
    GFLBitmap *bitmap;
} Box2FontOam;

// What runs once per frame until it returns FALSE
typedef BOOL (*Box2VFunc)(Box2SysWork *syswk);

typedef struct {
    Box2VFunc func;
    Box2VFunc freq;
    void *work;
    u16 seq;
    u16 cnt;
} Box2IrqWork;

// A button's press animation, on an actor or on the BG
typedef struct {
    // A BOX2_BTN_ANM_MODE_*
    u8 mode : 1;
    u8 id : 7;
    u8 pal1 : 4;
    u8 pal2 : 4;
    u8 seq;
    u8 cnt;
    u8 px;
    u8 py;
    u8 sx;
    u8 sy;
} Box2ButtonAnm;

// A Pokémon icon that moves to a new position
typedef struct {
    // The icon's index into pokeIconId
    u16 iconPos;
    u16 mvPos;
    u16 dfPos;
    // 0 when the entry is unused
    u16 flag;
    u32 mx;
    u32 my;
    s16 vx;
    s16 vy;
    s16 dx;
    s16 dy;
} Box2PokeMoveData;

// The Pokémon that move at once, and where the move started and ends
typedef struct {
    Box2PokeMoveData data[12];
    u16 cnt;
    u16 mode;
    u32 getPos;
    u32 putPos;
    // Where the Pokémon was dropped
    u32 setPos;
} Box2PokeMoveWork;

// An item of a menu: its message, and 1 for the item that closes the menu
typedef struct {
    u16 msgId;
    u16 type;
} Box2MenuItem;

// An area of the lower screen, with its right edge in it
typedef struct {
    u8 left;
    u8 right;
    u8 top;
    u8 bottom;
} Box2Area;

// The work of the name entry for a box's name
typedef struct {
    NameEntryParam *param;
    StrBuf *name;
} Box2NameInWork;

// Releasing a Pokémon
typedef struct {
    ClActor *cap;
    // The next box slot whose Pokémon's moves to check
    u16 checkCnt;
    u8 checkFlag;
    u8 scaleCnt;
    f32 scale;
} Box2PokeFreeWork;

// The scroll of the box list by touch
typedef struct {
    s16 cnt;
    s16 dir;
} Box2BoxListDrag;

// The item icon's move
typedef struct {
    u16 putPos;
    u16 setPos;
    u32 mvMode;
    u32 nowX;
    u32 nowY;
    u32 mx;
    u32 my;
    u32 mvX : 1;
    u32 mvY : 1;
    u32 cnt : 30;
} Box2ItemMoveWork;

// What the upper screen shows of a Pokémon
typedef struct {
    BoxPkm *pkm;
    u16 species;
    u16 item;
    u32 pid;
    u8 type1;
    u8 type2;
    u8 ability;
    u8 nature;
    u16 mark;
    u8 level : 7;
    u8 egg : 1;
    u8 sex : 4;
    // 0 for Nidoran and eggs, whose sex isn't shown
    u8 sexPut : 1;
    u8 rare : 1;
    // 1 while infected, 2 once cured
    u8 pokerus : 2;
    u16 moveId[4];
} Box2PokeInfo;

// The cursor's move to a position
typedef struct {
    u8 px;
    u8 py;
    u8 vx;
    u8 vy;
    u32 mx : 1;
    u32 my : 1;
    u32 cnt : 30;
} Box2CursorMoveWork;

typedef struct {
    s32 left;
    s32 top;
    s32 right;
    s32 bottom;
    u16 anmMax;
    u16 anmCnt;
    u8 anm;
    u8 prevAnm;
    u8 startPos;
    u8 endPos;
} Box2RangeSelect;

struct Box2AppWork {
    BmpOamSys *bmpOam;
    Box2FontOam fontOam[10];
    u32 keyRepeatWait;
    u32 keyRepeatStart;
    TCB *vtask;
    Box2IrqWork vfunk;
    int vfuncNextSeq;
    PaletteFade *palFade;
    CursorMove *cursorMove;
    BGWinFrame *bgWinFrame;
    Font *font;
    Font *smallFont;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *expandBuf;
    PrintQueue *printQueue;
    u32 unk98;
    PrintWindow windows[28];
    u8 flushChar[4];
    u32 cursorChars;
    BOOL nationalDex;
    u16 *regionalDex;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *yesNoRes;
    AppTaskMenu *yesNoMenu;
    u16 ynID;
    Box2ButtonAnm bawk;
    ArcTool *pokeIconArc;
    u8 pokeIconChar[30][0x200];
    u8 pokeIconPal[30];
    u8 pokeIconId[66];
    BOOL pokeIconExist[30];
    u8 trayIconChar[BOX2_TRAY_MAX][0x400];
    ClActUnit *clunit;
    ClActor *actors[145];
    u32 chrRes[129];
    u8 unkA2E0[0x204];
    u32 palRes[11];
    u32 cellRes[13];
    u16 pokegraSwap;
    u16 unkA546;
    u32 oldCurPos;
    u16 getItem;
    u16 getItemInitPos;
    u8 unkA550;
    u8 unkA551;
    u8 unkA552;
    // Why the last move check failed, a BOX2_MOVE_ERR_*
    u8 moveErr;
    u32 unkA554;
    s8 wallPx;
    u8 wallArea;
    u8 unkA55A;
    u8 wallpaperPos;
    u8 unkA55C;
    u8 unkA55D;
    // Where the hand puts a Pokémon or an item
    u8 pokePutKey;
    // The box of the box list the cursor is on
    u8 unkA55F;
    u32 tpx;
    u32 tpy;
    int wipeSeq;
    int wait;
    // The work of the current sequence, such as a Box2PokeFreeWork
    void *subWork;
    int subSeq;
    int msgNextSeq;
    u8 rangeFlags[30];
    u8 rangeWidth;
    u8 rangeHeight;
    Box2RangeSelect rangeSelect;
    BOOL unkA5B4;
    BOOL unkA5B8;
};

void Box2Main_InitVBlank(Box2SysWork *syswk);
void Box2Main_ExitVBlank(Box2SysWork *syswk);
// Sets the function that runs each frame
void Box2Main_VFuncSet(Box2AppWork *app, Box2VFunc func);
// Sets the function that Box2Main_VFuncReqSet makes the next to run
void Box2Main_VFuncReq(Box2AppWork *app, Box2VFunc func);
void Box2Main_VFuncReqSet(Box2AppWork *app);
void Box2Main_InitVramBanks(void);
const BGSysVRAMConfig *Box2Main_GetVramBanks(void);
void Box2Main_InitBg(Box2SysWork *syswk);
void Box2Main_ExitBg(Box2SysWork *syswk);
void Box2Main_LoadBgGraphics(Box2SysWork *syswk);
void Box2Main_InitPaletteFade(Box2SysWork *syswk);
void Box2Main_ExitPaletteFade(Box2SysWork *syswk);
void Box2Main_SetBlendAlpha(BOOL enabled);
void Box2Main_InitMsg(Box2SysWork *syswk);
void Box2Main_ExitMsg(Box2SysWork *syswk);
void Box2Main_InitYesNo(Box2SysWork *syswk);
void Box2Main_ExitYesNo(Box2SysWork *syswk);
void Box2Main_OpenYesNo(Box2SysWork *syswk, u32 pos);
// Runs the press animation of a button; FALSE once it is done
BOOL Box2Main_ButtonAnmMain(Box2SysWork *syswk);
// Starts the press animation of a frame's button
void Box2Main_SetFrameButtonAnm(Box2SysWork *syswk, u32 frame);
void Box2Main_InitSettings(Box2SysWork *syswk);
void Box2Main_PutMessageFrame(Box2SysWork *syswk);
// Shows the previous or next box
void Box2Main_ScrollTray(Box2SysWork *syswk, BOOL right);
void Box2Main_StartBoxListScroll(Box2SysWork *syswk, u32 a1);
void Box2Main_OpenBoxList(Box2SysWork *syswk);
BOOL Box2Main_IsTrayScrollRight(Box2SysWork *syswk, u32 from, u32 to);
// Shows the cursor and leaves touch mode
void Box2Main_ShowCursor(Box2SysWork *syswk);
u32 Box2Main_GetPokeParam(Box2SysWork *syswk, u16 pos, u16 tray, u32 param, void *buf);
void Box2Main_SetPokeParam(Box2SysWork *syswk, u32 pos, u32 tray, u32 param, u32 value);
// The Pokémon at a position of a tray, or of the party when tray is BOX2_GET_NONE
BoxPkm *Box2Main_GetBoxPkm(Box2SysWork *syswk, u32 tray, u32 pos);
void Box2Main_ClearPokeData(Box2SysWork *syswk, u32 tray, u32 pos);
// Moves the Pokémon of the move that just ended in the save data
void Box2Main_PokeDataMove(Box2SysWork *syswk);
// The box that a tray icon of the box list shows
u32 Box2Main_GetScrolledTray(Box2SysWork *syswk, u32 tray);
BOOL Box2Main_BattlePokeCheck(Box2SysWork *syswk, u32 pos);
BOOL Box2Main_PokeItemFormChange(Box2SysWork *syswk, BoxPkm *pkm);
// Recalculates the stats of the party Pokémon at pos, after a forme change
void Box2Main_RecalcPartyStats(Box2SysWork *syswk, u32 pos);
void Box2Main_InitDexData(Box2SysWork *syswk);
void Box2Main_ExitDexData(Box2SysWork *syswk);
BOOL Box2Main_IsSpeciesFlagged(Box2SysWork *syswk, u32 pos);
BOOL Box2Main_PokeItemMoveCheck(Box2SysWork *syswk, u32 getPos, u32 putPos);
void Box2Main_PokeFreeCreate(Box2SysWork *syswk);
void Box2Main_PokeFreeExit(Box2SysWork *syswk);
BOOL Box2Main_PokeFreeMoveCheck(Box2SysWork *syswk);
void Box2Main_UpdateChatter(Box2SysWork *syswk);
u8 Box2Main_GetTrayScroll(Box2SysWork *syswk, s8 mv);
// Shows a wallpaper, scrolling from the direction dir, a BOX2_TRAY_SCROLL_*
void Box2Main_WallPaperSet(Box2SysWork *syswk, u32 wallpaper, u32 dir);
void Box2Main_WallPaperChange(Box2SysWork *syswk, u32 wallpaper);
// The wallpaper of a box
u32 Box2Main_GetWallPaperNumber(Box2SysWork *syswk, u32 tray);
BOOL Box2Main_PokeInfoPutCore(Box2SysWork *syswk, u32 tray, u32 pos);
BOOL Box2Main_PokeInfoPut(Box2SysWork *syswk, u32 pos);
void Box2Main_PokeInfoRewrite(Box2SysWork *syswk, u32 pos);
void Box2Main_PokeInfoOff(Box2SysWork *syswk);
void Box2Main_PokeSelectOff(Box2SysWork *syswk);
void Box2Main_MarkingPutMain(Box2SysWork *syswk, u32 mark);
void Box2Main_MarkingPutSub(Box2SysWork *syswk, u32 mark);
// The sub procs: the summary, the bag, the name entry for a box's name and the search
int Box2Main_PokeStatusCall(Box2SysWork *syswk);
int Box2Main_PokeStatusExit(Box2SysWork *syswk);
int Box2Main_BagCall(Box2SysWork *syswk);
int Box2Main_BagExit(Box2SysWork *syswk);
int Box2Main_NameInCall(Box2SysWork *syswk);
int Box2Main_NameInExit(Box2SysWork *syswk);
int Box2Main_BoxSearchCall(Box2SysWork *syswk);
int Box2Main_BoxSearchExit(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMoveTouchParty(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMoveTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyPokeFreeSort(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyInPokeMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncTrayScrollLeft(Box2SysWork *syswk);
BOOL Box2Main_VFuncTrayScrollRight(Box2SysWork *syswk);
BOOL Box2Main_VFuncFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncMarkingFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListSlideOut(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListSlideIn(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyOutTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyInTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncCursorMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncCursorMoveFrame(Box2SysWork *syswk);
void Box2Main_HandGetPokeSet(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMoveGetKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncPokeMovePutKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncPartyOutPutKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeMenuClose(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeGetTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeMenuOpen(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeFrameMove(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemIconHide(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangePartyGetTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemIconPutBack(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeGetKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangePutKey(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeKeyCancel(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeBoxPartyGetTouch(Box2SysWork *syswk);
BOOL Box2Main_VFuncItemArrangeMenuCancel(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListScrollLeft(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxListScrollRight(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxMoveScrollLeft(Box2SysWork *syswk);
BOOL Box2Main_VFuncBoxMoveScrollRight(Box2SysWork *syswk);
BOOL Box2Main_VFuncRangeMoveTouch(Box2SysWork *syswk);
void Box2Main_ClearRangeFlags(Box2SysWork *syswk);
void Box2Main_SetRangeFlags(Box2SysWork *syswk);
// How many Pokémon the picked range holds
u32 Box2Main_GetRangeCount(Box2AppWork *app);
BOOL Box2Main_RangePutCheck(Box2SysWork *syswk, u32 tray, int pos);
u32 Box2Main_GetRowWidth(Box2SysWork *syswk, u32 pos);
void Box2Main_UpdateRangeIconsTouch(Box2SysWork *syswk);
void Box2Main_UpdateRangeIconsPos(Box2SysWork *syswk, u32 pos);

#endif // POKEBW2_APP_BOX2_MAIN_H
