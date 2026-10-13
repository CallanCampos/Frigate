#include "types.h"
#include "app/codein.h"
#include "app/name_entry.h"
#include "app/ui/print_msg.h"
#include "app/ui/ui_scene.h"
#include "app/wf2dmap/wifi_2dchar.h"
#include "app/wifi_note.h"
#include "app/wificlub.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "dwc/dwc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nnsys/g2d.h"
#include "pml/poke_graphic.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/aeabi.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/blink_palanm.h"
#include "system/bmp_winframe.h"
#include "system/codein_param.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/net_define.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/union_view.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/0418.h"

// The Pal Pad. The ROM has no name for this file; "wifi_note.c" is a guess from pokeplatinum, whose Pal Pad has the
// same five screens: the menu, the friend roster, the name and code entry, the player's own code and a friend's
// record. Our names

// The archive of the screen's graphics
#define ARCID_WIFI_NOTE 62

// The screens, the mode of the work
enum {
    WIFI_NOTE_MODE_SELECT,
    WIFI_NOTE_MODE_FRIEND_LIST,
    WIFI_NOTE_MODE_CODE_IN,
    WIFI_NOTE_MODE_MY_CODE,
    WIFI_NOTE_MODE_FRIEND_INFO,
    // Leaves the Pal Pad for the bag, or for the field
    WIFI_NOTE_MODE_RETURN,
    WIFI_NOTE_MODE_EXIT,
};

// The sequences of the menu that the other screens ask for
enum {
    MODE_SELECT_SEQ_INIT,
    MODE_SELECT_SEQ_FADE_IN,
    MODE_SELECT_SEQ_INIT_NO_FADE,
    MODE_SELECT_SEQ_MAIN,
    MODE_SELECT_SEQ_BUTTON,
    MODE_SELECT_SEQ_FADE_OUT,
    MODE_SELECT_SEQ_FADE_OUT_WAIT,
    // Back from the code entry with a name and a code to register
    MODE_SELECT_SEQ_CODE_INIT,
    MODE_SELECT_SEQ_CODE_FADE_IN,
    MODE_SELECT_SEQ_CODE_MESSAGE,
    MODE_SELECT_SEQ_CODE_YES_NO,
    MODE_SELECT_SEQ_MESSAGE,
};

// The menu's cursor: its three buttons, then the return and X buttons
enum {
    MODE_SELECT_FRIEND_LIST,
    MODE_SELECT_CODE_IN,
    MODE_SELECT_MY_CODE,
    MODE_SELECT_RETURN,
    MODE_SELECT_EXIT,
};

enum {
    FRIEND_LIST_SEQ_INIT,
    FRIEND_LIST_SEQ_INIT_KEEP_POS,
    FRIEND_LIST_SEQ_MAIN,
    FRIEND_LIST_SEQ_SCROLL_START,
    FRIEND_LIST_SEQ_SCROLL,
    FRIEND_LIST_SEQ_SCROLL_END,
    FRIEND_LIST_SEQ_BUTTON,
    FRIEND_LIST_SEQ_MENU_MESSAGE,
    FRIEND_LIST_SEQ_MENU,
    FRIEND_LIST_SEQ_INFO,
    FRIEND_LIST_SEQ_INFO_WAIT,
    // Back from a friend's record
    FRIEND_LIST_SEQ_INFO_RETURN,
    FRIEND_LIST_SEQ_NAME,
    FRIEND_LIST_SEQ_NAME_WAIT,
    // Back from the name entry, with a new name or without
    FRIEND_LIST_SEQ_NAME_RETURN,
    FRIEND_LIST_SEQ_NAME_CANCEL,
    FRIEND_LIST_SEQ_FADE_IN,
    FRIEND_LIST_SEQ_DELETE_MESSAGE,
    FRIEND_LIST_SEQ_DELETE_MESSAGE_WAIT,
    FRIEND_LIST_SEQ_DELETE_YES_NO,
    FRIEND_LIST_SEQ_DELETE_START,
    FRIEND_LIST_SEQ_DELETE,
    FRIEND_LIST_SEQ_DELETE_END,
    FRIEND_LIST_SEQ_CODE,
    FRIEND_LIST_SEQ_CODE_WAIT,
    FRIEND_LIST_SEQ_RETURN,
    FRIEND_LIST_SEQ_EXIT,
    FRIEND_LIST_SEQ_FADE_OUT,
    FRIEND_LIST_SEQ_FADE_OUT_WAIT,
};

// What the player did in the friend roster
enum {
    FRIEND_LIST_INPUT_NONE,
    FRIEND_LIST_INPUT_PREV_PAGE,
    FRIEND_LIST_INPUT_NEXT_PAGE,
    FRIEND_LIST_INPUT_RETURN,
    FRIEND_LIST_INPUT_EXIT,
    FRIEND_LIST_INPUT_DECIDE,
    FRIEND_LIST_INPUT_X,
};

#define FRIEND_LIST_PAGE_COUNT 4
#define FRIEND_LIST_PAGE_SIZE 8
// The cursor's positions after a page's friends
#define FRIEND_LIST_POS_RETURN 8
#define FRIEND_LIST_POS_EXIT 9
// The roster draws the page shown and, while it scrolls, the one that comes in from a side
#define FRIEND_LIST_AREA_COUNT 3
#define FRIEND_LIST_SCROLL_FRAMES 24

// Which page the roster scrolls to: the one before comes in from the left, in the third area
enum {
    FRIEND_LIST_SCROLL_PREV = 2,
    FRIEND_LIST_SCROLL_NEXT,
};

enum {
    CODE_IN_SEQ_NEW_INIT,
    CODE_IN_SEQ_NEW_NAME,
    CODE_IN_SEQ_NEW_CODE,
    CODE_IN_SEQ_NEW_END,
    // Only the name, of a friend of the roster
    CODE_IN_SEQ_NAME_INIT,
    CODE_IN_SEQ_NAME,
    CODE_IN_SEQ_NAME_END,
};

enum {
    MY_CODE_SEQ_INIT,
    MY_CODE_SEQ_MAIN,
    MY_CODE_SEQ_RETURN,
    MY_CODE_SEQ_EXIT,
    MY_CODE_SEQ_FADE_OUT,
    MY_CODE_SEQ_FADE_OUT_WAIT,
};

enum {
    FRIEND_INFO_SEQ_INIT,
    // The next friend's record, after one was shown
    FRIEND_INFO_SEQ_INIT_NEXT,
    FRIEND_INFO_SEQ_MAIN,
    FRIEND_INFO_SEQ_SCROLL_START,
    FRIEND_INFO_SEQ_SCROLL,
    FRIEND_INFO_SEQ_SCROLL_END,
    FRIEND_INFO_SEQ_CHANGE,
    FRIEND_INFO_SEQ_RETURN,
    FRIEND_INFO_SEQ_EXIT,
    FRIEND_INFO_SEQ_FADE_OUT,
    FRIEND_INFO_SEQ_FADE_OUT_WAIT,
};

// What the player did in a friend's record
enum {
    FRIEND_INFO_INPUT_RETURN,
    FRIEND_INFO_INPUT_EXIT,
    FRIEND_INFO_INPUT_PREV,
    FRIEND_INFO_INPUT_NEXT,
};

#define FRIEND_INFO_AREA_COUNT 3
#define FRIEND_INFO_SCREEN_COUNT 4
#define FRIEND_INFO_WINDOW_COUNT 10
#define FRIEND_INFO_SCROLL_FRAMES 8

// The keys that take the cursor back from the touch screen
#define WIFI_NOTE_KEYS (PAD_BUTTON_A | PAD_BUTTON_B | PAD_PLUS_KEY_MASK | PAD_BUTTON_X | PAD_BUTTON_Y)

#define WIFI_NOTE_PRINT_WINDOW_MAX 64
#define WIFI_NOTE_COLOR_WHITE 0x3c40
#define WIFI_NOTE_COLOR_BLACK 0x440
#define WIFI_NOTE_MENU_COLOR 0x39e3

// The friends of the Wi-Fi list that the roster shows
typedef struct {
    u32 count;
    // How many of them the player has met, and so have a record
    u32 metCount;
    // Their indices in the Wi-Fi list
    u8 index[WIFI_LIST_FRIEND_MAX];
    // Their numbers among the friends met, 0xff for the others
    u8 number[WIFI_LIST_FRIEND_MAX];
} WifiNoteFriends;

// What the screens share
typedef struct {
    // Whether the player is using the touch screen
    BOOL touch;
    GameData *gameData;
    u8 seq;
    // The screen and the sequence to go to when the current screen ends
    u8 nextMode;
    u8 nextSeq;
    // The friend picked in the roster
    s8 friendPos;
    u8 textSpeed;
    u8 frameType;
    // Whether L and R turn the roster's pages
    u8 lrEnabled;
    WifiNoteFriends friends;
    // The name and the Friend Code entered
    StrBuf *name;
    StrBuf *code;
    // The name that the name entry starts with
    const u16 *oldName;
    BOOL hasOldName;
} WifiNoteData;

// A screen of the archive
typedef struct {
    void *buffer[2];
    NNSG2dScreenData *screen[2];
} WifiNoteScreens;

typedef struct {
    WordSet *wordSet;
    MsgData *msgData;
    ArcTool *arc;
    PrintWindow title;
    StrBuf *titleStr;
    StrBuf *titleTemp;
    StrBuf *str;
    StrBuf *temp;
    u32 unk24;
    TCBExManager *tcbManager;
    PrintStream *stream;
    BOOL streamPaused;
    PrintQueue *queue;
    // The windows to send to VRAM once their text is printed
    PrintWindow *windows[WIFI_NOTE_PRINT_WINDOW_MAX];
    ClActUnit *unit;
    // The actors that scroll with the roster's pages
    ClActUnit *scrollUnit;
    ClActRenderer *renderer;
    // The resources of the screen's own archive, and the menus' common ones
    u32 chars[2];
    u32 palettes[2];
    u32 cellAnims[2];
    ClActor *arrows[2];
    ClActor *returnButton;
    ClActor *exitButton;
    WifiNoteScreens screens;
    Font *font;
    u16 heapId;
    // The cursor's and the page tabs' blinking palettes
    BlinkPalAnm *blink[2];
} WifiNoteDraw;

typedef struct {
    u32 cursor;
    u32 unk4;
    // The flash of a picked button
    u16 buttonSeq;
    u16 buttonCount;
    PrintWindow menuWindow;
    void *screenBuffer;
    NNSG2dScreenData *screen;
    PrintWindow msgWindow;
    u32 unk24[2];
    StrBuf *msgStr;
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
} WifiNoteModeSelect;

// A part of BG 2 in tiles, negative when it is to the left of the screen
typedef struct {
    s16 x;
    s16 y;
    u8 width;
    u8 height;
} WifiNoteScreenArea;

// A page of the roster
typedef struct {
    WifiNoteScreenArea area;
    PrintWindow *nameWindow;
    // The friends' trainers, or the empty frames of the friends not met and of the free places
    WF_2DCWK *chars[FRIEND_LIST_PAGE_SIZE];
    ClActor *frames[FRIEND_LIST_PAGE_SIZE];
} WifiNoteListArea;

typedef struct {
    s16 page;
    s16 prevPage;
    s16 pos;
    s16 prevPos;
    u16 buttonSeq;
    u16 buttonCount;
    WifiNoteListArea areas[FRIEND_LIST_AREA_COUNT];
    PrintWindow nameWindows[2];
    s16 scrollCount;
    // FRIEND_LIST_SCROLL_*
    u16 scrollDir;
    WF_2DCSYS *charSys;
    PrintWindow msgWindow;
    StrBuf *msgStr;
    u32 unk114;
    // The menus of a friend met, and of one only registered by code
    AppTaskMenuItem menuItems[2][4];
    AppTaskMenuItem yesNoItems[2];
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    u32 menuType;
    s32 wait;
    ClActor *eraser;
    ClActor *pageCursor;
    fx32 eraserFrame;
} WifiNoteFriendList;

typedef struct {
    GameProcManager *procManager;
    NameEntryParam *nameParam;
    CodeInParam *codeParam;
} WifiNoteCodeIn;

typedef struct {
    PrintWindow msgWindow;
    PrintWindow codeWindow;
    void *screenBuffer;
    NNSG2dScreenData *screen;
    u32 unk18;
} WifiNoteMyCode;

// A window's place in tiles
typedef struct {
    u8 x;
    u8 y;
    u8 width;
    u8 height;
} WifiNoteWindowRect;

// A page of a friend's record
typedef struct {
    WifiNoteScreenArea area;
    u32 charOffset;
    PrintWindow *windows[8];
    u32 windowCounts[8];
} WifiNoteInfoArea;

typedef struct {
    void *buffer;
    NNSG2dScreenData *screen;
} WifiNoteInfoScreen;

typedef struct {
    s16 page;
    s32 scrollCount;
    // 0 for the friend before, 1 for the one after
    u32 scrollDir;
    u16 wait;
    // Whether a record was shown before this one
    u16 moved;
    u32 unk10;
    WifiNoteInfoArea areas[FRIEND_INFO_AREA_COUNT];
    WifiNoteInfoScreen screens[FRIEND_INFO_SCREEN_COUNT];
    PrintWindow areaWindows[2];
    PrintWindow windows[FRIEND_INFO_WINDOW_COUNT];
    ClActor *trainer;
    u32 trainerChars;
    u32 trainerPalette;
    u32 trainerCellAnims;
} WifiNoteFriendInfo;

typedef struct {
    u32 mode;
    WifiNoteDraw draw;
    WifiNoteData data;
    WifiNoteParam *param;
    WifiNoteModeSelect modeSelect;
    WifiNoteFriendList friendList;
    WifiNoteCodeIn codeIn;
    WifiNoteMyCode myCode;
    WifiNoteFriendInfo friendInfo;
    TCB *vblankTask;
    s32 wait;
} WifiNoteWork;

// An item of the roster's menu, and the sequence it goes to
typedef struct {
    u32 msg;
    u32 seq;
} WifiNoteListMenuItem;

// Where a friend of a page is, in tiles from the page's corner
typedef struct {
    s16 x;
    s16 y;
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
    u16 unk8;
} WifiNoteListPos;

typedef void (*WifiNoteScrollFunc)(WifiNoteFriendList *list, WifiNoteDraw *draw, s16 page, s16 prevPage);

static BOOL WifiNote_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiNote_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiNote_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static void WifiNote_VBlank(TCB *tcb, void *data);
static void WifiNote_InitDraw(WifiNoteWork *wk, HeapID heapId);
static void WifiNote_ExitDraw(WifiNoteWork *wk);
static void WifiNote_CreateWindow(WifiNoteDraw *draw, PrintWindow *window, u8 bg, u8 x, u8 y, u8 width, u8 height,
                                  u8 palette, u32 charOffset, u8 color);
static void WifiNoteData_Init(WifiNoteData *data, WifiNoteParam *param, HeapID heapId);
static void WifiNoteData_Exit(WifiNoteData *data, WifiNoteParam *param);
static void WifiNoteData_SetNext(WifiNoteData *data, u8 mode, u8 seq);
static void WifiNoteData_SetEntered(WifiNoteData *data, const StrBuf *name, const StrBuf *code);
static u32 WifiNoteData_AddFriend(WifiNoteData *data, StrBuf *code, StrBuf *name);
static void WifiNoteData_SetOldName(WifiNoteData *data, const u16 *name);
static void WifiNoteData_SetFriendPos(WifiNoteData *data, s32 pos);
static void WifiNoteData_MakeFriends(WifiNoteData *data);
static void WifiNoteDraw_Init(WifiNoteDraw *draw, WifiNoteData *data, HeapID heapId);
static void WifiNoteDraw_Main(WifiNoteDraw *draw);
static void WifiNoteDraw_VBlank(WifiNoteDraw *draw);
static void WifiNoteDraw_Exit(WifiNoteDraw *draw);
static void WifiNoteDraw_InitVRAM(void);
static void WifiNoteDraw_InitBG(WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteDraw_ExitBG(WifiNoteDraw *draw);
static void WifiNoteDraw_InitClAct(WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteDraw_ExitClAct(WifiNoteDraw *draw);
static void WifiNoteDraw_InitMsg(WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteDraw_ExitMsg(WifiNoteDraw *draw);
static void WifiNoteDraw_InitTitle(WifiNoteDraw *draw, const WifiNoteData *data, HeapID heapId);
static void WifiNoteDraw_ExitTitle(WifiNoteDraw *draw);
static void WifiNoteDraw_LoadScreens(WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteDraw_FreeScreens(WifiNoteDraw *draw);
static void WifiNoteDraw_PrintTitle(WifiNoteDraw *draw, PrintWindow *window, u32 msg);
static void WifiNoteDraw_HideTitle(WifiNoteDraw *draw);
static void WifiNoteDraw_SetFriendCode(WifiNoteDraw *draw, u64 code);
static void WifiNoteDraw_SetFriendName(WifiNoteDraw *draw, WifiNoteData *data, u32 index, HeapID heapId);
static BOOL WifiNoteDraw_SetLastDate(WifiNoteDraw *draw, WifiNoteData *data, u32 index);
static void WifiNoteDraw_SetNumber(WifiNoteDraw *draw, u32 number);
static void WifiNoteDraw_InitActors(WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteDraw_ExitActors(WifiNoteDraw *draw);
static void WifiNoteDraw_SetArrowsVisible(WifiNoteDraw *draw, BOOL visible);
static void WifiNoteDraw_SetArrowsAnimating(WifiNoteDraw *draw, BOOL animating);
static void WifiNoteDraw_StartArrowAnim(WifiNoteDraw *draw, u32 arrow, u16 sequence);
static AppTaskMenu *WifiNote_CreateYesNoMenu(AppTaskMenuItem *items, AppTaskMenuRes **res, BOOL low, HeapID heapId);
static s32 WifiNote_UpdateMenu(AppTaskMenu *menu);
static void WifiNoteModeSelect_Init(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static BOOL WifiNoteModeSelect_Main(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteModeSelect_Exit(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteModeSelect_InitDraw(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId);
static void WifiNoteModeSelect_ExitDraw(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteModeSelect_Show(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static BOOL WifiNoteModeSelect_Keys(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static s32 WifiNoteModeSelect_Touch(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static BOOL WifiNoteModeSelect_Input(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteModeSelect_SetNext(WifiNoteModeSelect *select, WifiNoteData *data);
static void WifiNoteModeSelect_Hide(WifiNoteModeSelect *select, WifiNoteDraw *draw);
static void WifiNoteModeSelect_StartMessage(WifiNoteModeSelect *select, WifiNoteDraw *draw, u32 msg, HeapID heapId);
static BOOL WifiNote_UpdateStream(PrintStream *stream, BOOL *paused);
static BOOL WifiNoteModeSelect_IsMessageDone(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteModeSelect_ClearMessage(WifiNoteModeSelect *select, WifiNoteDraw *draw);
static BOOL WifiNoteModeSelect_CanEnter(WifiNoteModeSelect *select, WifiNoteData *data);
static void WifiNoteFriendList_Init(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static BOOL WifiNoteFriendList_Main(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteFriendList_Exit(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteFriendList_InitDraw(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId);
static void WifiNoteFriendList_ExitDraw(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteFriendList_Show(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteFriendList_Hide(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static u32 WifiNoteFriendList_Decide(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, u32 input);
static u32 WifiNoteFriendList_Keys(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static u32 WifiNoteFriendList_Touch(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, BOOL *touched);
static u32 WifiNoteFriendList_Input(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteFriendList_ScrollStart(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                           HeapID heapId);
static BOOL WifiNoteFriendList_Scroll(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static BOOL WifiNoteFriendList_ScrollEnd(WifiNoteFriendList *list, WifiNoteDraw *draw);
static u64 WifiNoteFriendList_GetFriendKey(WifiNoteFriendList *list, WifiNoteData *data);
static void WifiNoteFriendList_StartMenuMessage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                                HeapID heapId);
static void WifiNoteFriendList_OpenMenu(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId);
static u32 WifiNoteFriendList_UpdateMenu(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static BOOL WifiNoteFriendList_StartCodeMessage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                                HeapID heapId);
static void WifiNoteFriendList_StartDeleteMessage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                                  HeapID heapId);
static BOOL WifiNoteFriendList_OpenYesNo(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                         HeapID heapId);
static void WifiNoteFriendList_DrawPage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, s32 page,
                                        s32 pos, u32 area, HeapID heapId);
static void WifiNoteFriendList_SetPage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, s32 page,
                                       s32 pos, HeapID heapId);
static void WifiNoteFriendList_ScrollFunc0(WifiNoteFriendList *list, WifiNoteDraw *draw, s16 page, s16 prevPage);
static void WifiNoteFriendList_ScrollFunc2(WifiNoteFriendList *list, WifiNoteDraw *draw, s16 page, s16 prevPage);
static void WifiNoteFriendList_RequestScroll(WifiNoteFriendList *list, WifiNoteData *data, u32 dir);
static void WifiNoteFriendList_StartMessage(WifiNoteFriendList *list, WifiNoteDraw *draw, u32 msg, HeapID heapId);
static BOOL WifiNoteFriendList_IsMessageDone(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteFriendList_ClearMessage(WifiNoteFriendList *list, WifiNoteDraw *draw);
static void WifiNoteFriendList_EraseMessage(WifiNoteFriendList *list);
static s32 WifiNoteFriendList_GetFriend(WifiNoteFriendList *list);
static void WifiNoteFriendList_StartErase(WifiNoteFriendList *list, WifiNoteDraw *draw);
static BOOL WifiNoteFriendList_UpdateErase(WifiNoteFriendList *list, WifiNoteDraw *draw);
static void WifiNoteFriendList_EndErase(WifiNoteFriendList *list, WifiNoteDraw *draw);
static void WifiNoteListArea_Init(WifiNoteListArea *area, WifiNoteDraw *draw, const WifiNoteScreenArea *screenArea,
                                  u32 charOffset, HeapID heapId);
static void WifiNoteListArea_Exit(WifiNoteListArea *area, WifiNoteDraw *draw);
static void WifiNoteListArea_DrawBG(WifiNoteListArea *area, WifiNoteDraw *draw, WifiNoteScreens *screens);
static void WifiNoteListArea_DrawFriends(WifiNoteListArea *area, WifiNoteData *data, WifiNoteDraw *draw,
                                         WF_2DCSYS *charSys, WifiNoteScreens *screens, WifiNoteFriends *friends,
                                         s32 page, HeapID heapId);
static void WifiNoteListArea_Draw(WifiNoteListArea *area, WifiNoteData *data, WifiNoteDraw *draw, WF_2DCSYS *charSys,
                                  WifiNoteScreens *screens, WifiNoteFriends *friends, s32 page, HeapID heapId);
static void WifiNoteListArea_Clear(WifiNoteListArea *area, WifiNoteDraw *draw, WifiNoteScreens *screens);
static void WifiNoteListArea_DeleteActors(WifiNoteListArea *area, WifiNoteDraw *draw);
static void WifiNoteListArea_DrawFriend(WifiNoteListArea *area, WF_2DCSYS *charSys, WifiNoteDraw *draw,
                                        WifiNoteScreens *screens, s32 friendPos, WifiList *wifiList, u32 index,
                                        HeapID heapId);
static void WifiNoteListArea_DrawFrame(WifiNoteListArea *area, WifiNoteDraw *draw, WifiNoteScreens *screens, s32 pos,
                                       HeapID heapId);
static BOOL WifiNoteCodeIn_Main(WifiNoteCodeIn *codeIn, WifiNoteWork *wk, WifiNoteData *data, WifiNoteDraw *draw,
                                HeapID heapId);
static NameEntryParam *WifiNoteCodeIn_CreateNameParam(WifiNoteCodeIn *codeIn, WifiNoteData *data, BOOL isNew,
                                                      HeapID heapId);
static void WifiNoteMyCode_Init(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static BOOL WifiNoteMyCode_Main(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteMyCode_Exit(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw);
static s32 WifiNoteMyCode_Input(WifiNoteMyCode *myCode, WifiNoteData *data);
static void WifiNoteMyCode_InitDraw(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteMyCode_ExitDraw(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteMyCode_Show(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteMyCode_Hide(WifiNoteMyCode *myCode, WifiNoteDraw *draw);
static void WifiNoteFriendInfo_Init(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static BOOL WifiNoteFriendInfo_Main(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static u32 WifiNoteFriendInfo_Decide(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, u32 input);
static u32 WifiNoteFriendInfo_Keys(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw);
static u32 WifiNoteFriendInfo_Touch(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, BOOL *touched);
static u32 WifiNoteFriendInfo_Input(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteFriendInfo_ScrollStart(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw,
                                           HeapID heapId);
static BOOL WifiNoteFriendInfo_Scroll(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteFriendInfo_InitDraw(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId);
static void WifiNoteFriendInfo_ExitDraw(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw);
static void WifiNoteFriendInfo_Show(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteFriendInfo_Hide(WifiNoteFriendInfo *info, WifiNoteDraw *draw);
static void WifiNoteFriendInfo_HideForNext(WifiNoteFriendInfo *info, WifiNoteDraw *draw);
static void WifiNoteFriendInfo_Draw(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteFriendInfo_HideWindows(WifiNoteFriendInfo *info);
static void WifiNoteFriendInfo_SetPage(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, u32 area,
                                       s32 page, HeapID heapId);
static void WifiNoteFriendInfo_DrawTabs(WifiNoteFriendInfo *info, WifiNoteDraw *draw, u32 tab);
static void WifiNoteFriendInfo_ClearTabs(WifiNoteFriendInfo *info, WifiNoteDraw *draw);
static BOOL WifiNoteData_MoveFriend(WifiNoteData *data, u32 dir);
static BOOL WifiNoteData_CanMoveFriend(WifiNoteData *data, u32 dir);
static void WifiNoteFriendInfo_LoadScreens(WifiNoteInfoScreen *screens, WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteFriendInfo_FreeScreens(WifiNoteInfoScreen *screens);
static void WifiNoteFriendInfo_CreateWindows(WifiNoteFriendInfo *info, WifiNoteDraw *draw);
static void WifiNoteFriendInfo_FreeWindows(WifiNoteFriendInfo *info, WifiNoteDraw *draw);
static void WifiNoteInfoArea_Init(WifiNoteInfoArea *area, WifiNoteDraw *draw, const WifiNoteScreenArea *screenArea,
                                  u32 charOffset, HeapID heapId);
static void WifiNoteInfoArea_Exit(WifiNoteInfoArea *area);
static void WifiNoteInfoArea_SetWindows(WifiNoteInfoArea *area, WifiNoteDraw *draw, u32 index, u32 count,
                                        const WifiNoteWindowRect *rects, HeapID heapId);
static void WifiNoteInfoArea_ResetWindows(WifiNoteInfoArea *area, u32 index);
static void WifiNoteInfoArea_HideWindows(WifiNoteInfoArea *area, u32 index);
static void WifiNoteInfoArea_Hide(WifiNoteInfoArea *area, WifiNoteDraw *draw);
static void WifiNote_ClearScreenArea(WifiNoteDraw *draw, const WifiNoteScreenArea *area);
static void WifiNoteFriendInfo_Print(PrintWindow *window, WifiNoteData *data, WifiNoteDraw *draw, u32 msg, u32 x, u32 y,
                                     u32 color);
static u64 WifiNote_StrToFriendKey(const StrBuf *str, BOOL *valid);
static BOOL WifiNote_IsNextPressed(WifiNoteData *data);
static void WifiNoteDraw_InitBlink(WifiNoteDraw *draw, HeapID heapId);
static void WifiNoteDraw_ExitBlink(WifiNoteDraw *draw);
static void WifiNoteModeSelect_SetButtonPalette(u32 button, u32 palette);
static void WifiNoteModeSelect_ClearCursor(WifiNoteDraw *draw, u32 button);
static void WifiNoteModeSelect_SetCursor(WifiNoteDraw *draw, u32 button);
static void WifiNoteModeSelect_MoveCursor(WifiNoteDraw *draw, u32 button, u32 prevButton);
static void WifiNoteModeSelect_DrawCursor(WifiNoteModeSelect *select);
static BOOL WifiNoteModeSelect_UpdateButton(WifiNoteModeSelect *select);
static void WifiNoteFriendList_SetButtonPalette(u32 pos, u32 palette);
static void WifiNoteFriendList_ClearCursor(WifiNoteDraw *draw, u32 pos);
static void WifiNoteFriendList_SetCursor(WifiNoteDraw *draw, u32 pos);
static void WifiNoteFriendList_MoveCursor(WifiNoteDraw *draw, u32 pos, u32 prevPos);
static void WifiNoteFriendList_DrawCursor(WifiNoteFriendList *list);
static BOOL WifiNoteFriendList_UpdateButton(WifiNoteFriendList *list);
static void WifiNoteDraw_StartReturnAnim(WifiNoteDraw *draw);
static void WifiNoteDraw_StartExitAnim(WifiNoteDraw *draw);
static void WifiNote_SetBrightness(BOOL normal);
static void WifiNoteFriendList_SetPageCursor(WifiNoteFriendList *list, s32 page);
static void WifiNoteDraw_ShowTitle(WifiNoteDraw *draw);
static void WifiNoteDraw_ClearPrintWindows(WifiNoteDraw *draw);
static void WifiNoteDraw_AddPrintWindow(WifiNoteDraw *draw, PrintWindow *window);
static void WifiNoteDraw_FlushPrintWindows(WifiNoteDraw *draw);
static void WifiNote_ShowWindow(BmpWin *window);
static void WifiNoteFriendInfo_FreeTrainer(WifiNoteFriendInfo *info);

// The most digits that WifiNote_StrToFriendKey reads. The compiler folds it into the code, but only a table is
// still in the ROM's data, and only a table's element is compared as an int
static const u8 sWifiNoteFriendKeyDigitsMax[1] = { 18 };

static const BGSysVRAMConfig sWifiNoteVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,        GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

// The NSCRs of the roster's page and of a record's tabs
static const u8 sWifiNoteScreenFiles[2] = { 8, 9 };

// The row of a record's tabs in their screen
static const u8 sWifiNoteFriendInfoTabSrcY[2] = { 24, 16 };

// The messages of WifiNoteData_AddFriend's results
static const u8 sWifiNoteModeSelectAddFriendMsgs[3] = {
    0,
    Bank0418_Text_FriendCodeWrong,
    Bank0418_Text_FriendCodeHasAlready,
};

// The windows of a record's eight parts, which nothing reads. The compiler sorts a file's data by size, and not
// stably: this order of the definitions, and the places of the VRAM config and the surface setup, are what give the
// ROM's order
static const WifiNoteWindowRect sWifiNoteFriendInfoRects6[1] = { { 2, 5, 20, 11 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects1[1] = { { 2, 5, 22, 11 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects5[1] = { { 2, 5, 20, 11 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects0[1] = { { 2, 5, 12, 2 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects3[1] = { { 2, 1, 14, 2 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects2[1] = { { 5, 5, 9, 2 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects7[1] = { { 1, 5, 27, 2 } };
static const WifiNoteWindowRect sWifiNoteFriendInfoRects4[1] = { { 2, 5, 26, 11 } };

// The screens of the menu's buttons, then of its return and X buttons
static const u8 sWifiNoteModeSelectModes[5] = {
    WIFI_NOTE_MODE_FRIEND_LIST, WIFI_NOTE_MODE_CODE_IN, WIFI_NOTE_MODE_MY_CODE,
    WIFI_NOTE_MODE_RETURN,      WIFI_NOTE_MODE_EXIT,
};

static const u8 sWifiNoteFriendInfoWindowCounts[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };

// The return and X buttons
static const TouchRect sWifiNoteMyCodeTouchRects[] = {
    { 168, 191, 232, 255 },
    { 168, 191, 200, 223 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const u32 sWifiNoteFriendInfoCharOffsets[FRIEND_INFO_AREA_COUNT] = { 1, 501, 501 };

static const u8 sWifiNoteFriendInfoTabX[2][8] = {
    { 0, 14, 28, 0, 0, 0, 0, 0 },
    { 0, 4, 8, 12, 16, 20, 24, 28 },
};

static const u8 sWifiNoteFriendInfoTabWidth[2][8] = {
    { 14, 14, 4, 0, 0, 0, 0, 0 },
    { 4, 4, 4, 4, 4, 4, 4, 4 },
};

// The page shown, and the pages to its right and to its left
static const WifiNoteScreenArea sWifiNoteScreenAreas[3] = {
    { 0, 0, 32, 24 },
    { 32, 0, 32, 24 },
    { -32, 0, 32, 24 },
};

// The up and down arrows, then the return and X buttons
static const TouchRect sWifiNoteFriendInfoTouchRects[] = {
    { 168, 191, 16, 39 },   { 168, 191, 48, 71 },        { 168, 191, 232, 255 },
    { 168, 191, 200, 223 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static ClActSurfaceSetup sWifiNoteSurfaceSetup = { 0, 0, 256, 192, 0, 0 };

static const TouchRect sWifiNoteModeSelectTouchRects[] = {
    { 42, 69, 19, 236 },    { 82, 109, 19, 236 },   { 122, 149, 19, 236 },
    { 168, 191, 232, 255 }, { 168, 191, 200, 223 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const WifiNoteWindowRect sWifiNoteFriendInfoWindowRects[FRIEND_INFO_WINDOW_COUNT] = {
    { 26, 1, 5, 2 },  { 14, 5, 14, 2 },  { 23, 8, 4, 2 },  { 23, 10, 4, 2 }, { 27, 8, 4, 2 },
    { 27, 10, 4, 2 }, { 14, 13, 12, 2 }, { 27, 13, 4, 2 }, { 2, 17, 17, 2 }, { 20, 17, 10, 2 },
};

// A page's eight friends, the return and X buttons, and the four page tabs
static const TouchRect sWifiNoteFriendListTouchRects[] = {
    { 32, 64, 16, 128 },    { 64, 96, 16, 128 },    { 96, 128, 16, 128 },        { 128, 160, 16, 128 },
    { 32, 64, 128, 240 },   { 64, 96, 128, 240 },   { 96, 128, 128, 240 },       { 128, 160, 128, 240 },
    { 168, 191, 232, 255 }, { 168, 191, 200, 223 }, { 168, 191, 8, 31 },         { 168, 191, 32, 55 },
    { 168, 191, 56, 79 },   { 168, 191, 80, 103 },  { TOUCH_RECT_END, 0, 0, 0 },
};

const GameProcFunctions WIFI_NOTE_PROC_FUNCTIONS = { WifiNote_ProcInit, WifiNote_ProcMain, WifiNote_ProcExit };

static const WifiNoteListMenuItem sWifiNoteFriendListMenu[2][4] = {
    {
        { Bank0418_Text_Details, FRIEND_LIST_SEQ_INFO },
        { Bank0418_Text_RewriteName, FRIEND_LIST_SEQ_NAME },
        { Bank0418_Text_Delete, FRIEND_LIST_SEQ_DELETE_MESSAGE },
        { Bank0418_Text_Cancel, FRIEND_LIST_SEQ_MAIN },
    },
    {
        { Bank0418_Text_CheckFriendCode, FRIEND_LIST_SEQ_CODE },
        { Bank0418_Text_RewriteName, FRIEND_LIST_SEQ_NAME },
        { Bank0418_Text_Delete, FRIEND_LIST_SEQ_DELETE_MESSAGE },
        { Bank0418_Text_Cancel, FRIEND_LIST_SEQ_MAIN },
    },
};

static const WifiNoteListPos sWifiNoteFriendListPos[FRIEND_LIST_PAGE_SIZE] = {
    { 2, 4, 15, 4, 2, 2, 8 },  { 2, 8, 15, 4, 2, 2, 8 },  { 2, 12, 15, 4, 2, 2, 8 },  { 2, 16, 15, 4, 2, 2, 8 },
    { 16, 4, 15, 4, 2, 2, 8 }, { 16, 8, 15, 4, 2, 2, 8 }, { 16, 12, 15, 4, 2, 2, 8 }, { 16, 16, 15, 4, 2, 2, 8 },
};

static const BGSetup sWifiNoteBGSetups[5] = {
    {
        0,
        0,
        0x800,
        0,
        BGRES_256x256,
        GX_BG_COLORMODE_16,
        GX_BG_SCRBASE(0xc000),
        GX_BG_CHARBASE(0x00000),
        0x4000,
        GX_BG_EXTPLTT_01,
        3,
        GX_BG_AREAOVER_XLU,
        FALSE,
    },
    {
        0,
        0,
        0x800,
        0,
        BGRES_256x256,
        GX_BG_COLORMODE_16,
        GX_BG_SCRBASE(0xc800),
        GX_BG_CHARBASE(0x04000),
        0x8000,
        GX_BG_EXTPLTT_01,
        0,
        GX_BG_AREAOVER_XLU,
        FALSE,
    },
    {
        0,
        0,
        0x1000,
        0,
        BGRES_512x256,
        GX_BG_COLORMODE_16,
        GX_BG_SCRBASE(0xd000),
        GX_BG_CHARBASE(0x10000),
        0x8000,
        GX_BG_EXTPLTT_01,
        2,
        GX_BG_AREAOVER_XLU,
        FALSE,
    },
    {
        0,
        0,
        0x1000,
        0,
        BGRES_512x256,
        GX_BG_COLORMODE_16,
        GX_BG_SCRBASE(0xe000),
        GX_BG_CHARBASE(0x18000),
        0x8000,
        GX_BG_EXTPLTT_01,
        1,
        GX_BG_AREAOVER_XLU,
        FALSE,
    },
    {
        0,
        0,
        0x800,
        0,
        BGRES_256x256,
        GX_BG_COLORMODE_16,
        GX_BG_SCRBASE(0xf800),
        GX_BG_CHARBASE(0x00000),
        0x8000,
        GX_BG_EXTPLTT_01,
        0,
        GX_BG_AREAOVER_XLU,
        FALSE,
    },
};

// What the roster does every six frames of its scroll
static WifiNoteScrollFunc sWifiNoteFriendListScrollFuncs[4] = { WifiNoteFriendList_ScrollFunc0, NULL,
                                                                WifiNoteFriendList_ScrollFunc2, NULL };

// The up and down arrows, then the return and X buttons, with the menus' common graphics
static ClActorSetup sWifiNoteButtonSetups[4] = {
    { 16, 168, 2, 8, 2 },
    { 48, 168, 3, 8, 2 },
    { 232, 168, 1, 0, 2 },
    { 200, 168, 0, 0, 2 },
};

static const WifiNoteWindowRect *sWifiNoteFriendInfoRects[8] = {
    sWifiNoteFriendInfoRects0, sWifiNoteFriendInfoRects1, sWifiNoteFriendInfoRects2, sWifiNoteFriendInfoRects3,
    sWifiNoteFriendInfoRects4, sWifiNoteFriendInfoRects5, sWifiNoteFriendInfoRects6, sWifiNoteFriendInfoRects7,
};

static inline void WifiNote_HideWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

static inline void WifiNote_ShowWindowNow(BmpWin *window) {
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

static inline void WifiNote_LoadScreen(u8 bg, const NNSG2dScreenData *screen) {
    GFL_BGSysLoadScrAreaAll(bg, screen->rawData, 0, 0, screen->width / 8, screen->height / 8);
}

static BOOL WifiNote_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiNoteWork *wk;

    GFL_OvlLoad(OVERLAY_WIFICLUB);
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_WIFI_NOTE, 0xc0000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(WifiNoteWork), HEAPID_WIFI_NOTE);
    sys_memset(wk, 0, sizeof(WifiNoteWork));
    wk->param = param;
    wk->vblankTask = NULL;
    WifiNoteData_Init(&wk->data, wk->param, HEAPID_WIFI_NOTE);
    WifiNoteDraw_Init(&wk->draw, &wk->data, HEAPID_WIFI_NOTE);
    if (wk->vblankTask == NULL) {
        wk->vblankTask = GFL_VBlankTCBAdd(WifiNote_VBlank, wk, 16);
    }
    return TRUE;
}

static BOOL WifiNote_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    WifiNoteWork *wk = work;
    BOOL done;

    switch (*state) {
    case 0:
        switch (wk->mode) {
        case WIFI_NOTE_MODE_SELECT:
            done = WifiNoteModeSelect_Main(&wk->modeSelect, &wk->data, &wk->draw, HEAPID_WIFI_NOTE);
            break;
        case WIFI_NOTE_MODE_FRIEND_LIST:
            done = WifiNoteFriendList_Main(&wk->friendList, &wk->data, &wk->draw, HEAPID_WIFI_NOTE);
            break;
        case WIFI_NOTE_MODE_CODE_IN:
            done = WifiNoteCodeIn_Main(&wk->codeIn, wk, &wk->data, &wk->draw, HEAPID_WIFI_NOTE);
            break;
        case WIFI_NOTE_MODE_MY_CODE:
            done = WifiNoteMyCode_Main(&wk->myCode, &wk->data, &wk->draw, HEAPID_WIFI_NOTE);
            break;
        case WIFI_NOTE_MODE_FRIEND_INFO:
            done = WifiNoteFriendInfo_Main(&wk->friendInfo, &wk->data, &wk->draw, HEAPID_WIFI_NOTE);
            break;
        case WIFI_NOTE_MODE_RETURN:
            wk->param->result = WIFI_NOTE_RESULT_RETURN;
            return TRUE;
        case WIFI_NOTE_MODE_EXIT:
            wk->param->result = WIFI_NOTE_RESULT_EXIT;
            return TRUE;
        }
        if (done == TRUE) {
            wk->mode = wk->data.nextMode;
            wk->data.seq = wk->data.nextSeq;
            wk->wait = 4;
            *state = 1;
        }
        break;
    case 1:
        wk->wait--;
        if (wk->wait <= 0) {
            *state = 0;
        }
        break;
    }
    WifiNoteDraw_Main(&wk->draw);
    return FALSE;
}

static BOOL WifiNote_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiNoteWork *wk = work;

    GFL_TCBRemove(wk->vblankTask);
    wk->vblankTask = NULL;
    WifiNoteDraw_Exit(&wk->draw);
    WifiNoteData_Exit(&wk->data, wk->param);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_WIFI_NOTE);
    GFL_OvlUnload(OVERLAY_APP_UI);
    GFL_OvlUnload(OVERLAY_WIFICLUB);
    return TRUE;
}

static void WifiNote_VBlank(TCB *tcb, void *data) {
    WifiNoteWork *wk = data;

    WifiNoteDraw_VBlank(&wk->draw);
}

// Starts the screen's graphics again after the name or code entry, and ends them before it
static void WifiNote_InitDraw(WifiNoteWork *wk, HeapID heapId) {
    WifiNoteDraw_Init(&wk->draw, &wk->data, heapId);
    if (wk->vblankTask == NULL) {
        wk->vblankTask = GFL_VBlankTCBAdd(WifiNote_VBlank, wk, 16);
    }
}

static void WifiNote_ExitDraw(WifiNoteWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
    wk->vblankTask = NULL;
    WifiNoteDraw_Exit(&wk->draw);
}

// charOffset is not used
static void WifiNote_CreateWindow(WifiNoteDraw *draw, PrintWindow *window, u8 bg, u8 x, u8 y, u8 width, u8 height,
                                  u8 palette, u32 charOffset, u8 color) {
    window->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(window->window), color);
    BmpWin_FlushChar(window->window);
    WifiNoteDraw_AddPrintWindow(draw, window);
}

static void WifiNoteData_Init(WifiNoteData *data, WifiNoteParam *param, HeapID heapId) {
    data->gameData = param->gameData;
    data->name = GFL_StrBufCreate(128, heapId);
    data->code = GFL_StrBufCreate(128, heapId);
    data->touch = func_0203d554();
    data->textSpeed = func_02017bcc();
    data->frameType = 0;
    data->lrEnabled = TRUE;
}

static void WifiNoteData_Exit(WifiNoteData *data, WifiNoteParam *param) {
    func_0203d564(data->touch);
    GFL_StrBufFree(data->name);
    GFL_StrBufFree(data->code);
}

static void WifiNoteData_SetNext(WifiNoteData *data, u8 mode, u8 seq) {
    data->nextMode = mode;
    data->nextSeq = seq;
}

static void WifiNoteData_SetEntered(WifiNoteData *data, const StrBuf *name, const StrBuf *code) {
    if (name != NULL) {
        GFL_StrBufCopy(data->name, name);
    }
    if (code != NULL) {
        GFL_StrBufCopy(data->code, code);
    }
}

// Registers a friend by Friend Code in the first free place of the Wi-Fi list. Returns 0, or 1 when the code is
// wrong and 2 when it is in the list already
static u32 WifiNoteData_AddFriend(WifiNoteData *data, StrBuf *code, StrBuf *name) {
    int i;
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    u64 myKey = func_02057ec4(func_02009f7c(wifiList));

    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (!func_0200a138(wifiList, i)) {
            BOOL valid;
            int index;
            u64 key = WifiNote_StrToFriendKey(code, &valid);

            if (valid == TRUE && key != myKey) {
                if (!func_020578cc(func_02009f7c(wifiList), key)) {
                    return 1;
                }
                if (!func_02011980(wifiList, key, &index)) {
                    return 2;
                }
                func_02057ef8(func_0200a0e4(wifiList, i), key);
                func_0200a11c(wifiList, i, name);
                func_0200a034(wifiList, i, WIFI_LIST_FRIEND_GENDER, WIFI_LIST_GENDER_UNKNOWN);
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

static void WifiNoteData_SetOldName(WifiNoteData *data, const u16 *name) {
    data->oldName = name;
    data->hasOldName = TRUE;
}

static void WifiNoteData_SetFriendPos(WifiNoteData *data, s32 pos) {
    data->friendPos = pos;
}

static void WifiNoteData_MakeFriends(WifiNoteData *data) {
    int i;
    WifiList *wifiList;

    sys_memset(&data->friends, 0, sizeof(WifiNoteFriends));
    wifiList = GameData_GetWifiList(data->gameData);
    data->friends.count = 0;
    for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
        if (func_0200a138(wifiList, i) == TRUE) {
            data->friends.index[data->friends.count] = i;
            if (func_02009f80(wifiList, i, WIFI_LIST_FRIEND_GENDER) == WIFI_LIST_GENDER_UNKNOWN) {
                data->friends.number[data->friends.count] = 0xff;
            } else {
                data->friends.number[data->friends.count] = data->friends.metCount++;
            }
            data->friends.count++;
        }
    }
}

static void WifiNoteDraw_Init(WifiNoteDraw *draw, WifiNoteData *data, HeapID heapId) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
    GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
    GFL_BGSysSetDisplayLayout(0);
    draw->arc = GFL_ArcSysCreateFileHandle(ARCID_WIFI_NOTE, heapId);
    draw->heapId = heapId;
    WifiNoteDraw_InitVRAM();
    WifiNoteDraw_InitBG(draw, heapId);
    WifiNoteDraw_InitClAct(draw, heapId);
    WifiNoteDraw_InitMsg(draw, heapId);
    WifiNoteDraw_InitTitle(draw, data, heapId);
    WifiNoteDraw_InitActors(draw, heapId);
    WifiNoteDraw_LoadScreens(draw, heapId);
    WifiNoteDraw_InitBlink(draw, heapId);
    func_02042ba8(TRUE, HEAPID_WIFI_NOTE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void WifiNoteDraw_Main(WifiNoteDraw *draw) {
    if (draw->queue != NULL) {
        func_02021a3c(draw->queue);
        WifiNoteDraw_FlushPrintWindows(draw);
    }
    if (draw->tcbManager != NULL) {
        GFL_TCBExMgrUpdate(draw->tcbManager);
    }
    func_0204b794();
}

static void WifiNoteDraw_VBlank(WifiNoteDraw *draw) {
    func_0204b7c8();
}

static void WifiNoteDraw_Exit(WifiNoteDraw *draw) {
    WifiNoteDraw_ExitBlink(draw);
    WifiNoteDraw_ExitActors(draw);
    WifiNoteDraw_FreeScreens(draw);
    WifiNoteDraw_ExitTitle(draw);
    WifiNoteDraw_ExitMsg(draw);
    WifiNoteDraw_ExitClAct(draw);
    WifiNoteDraw_ExitBG(draw);
    GFL_ArcToolFree(draw->arc);
}

static void WifiNoteDraw_InitVRAM(void) {
    GFL_BGSysSetVRAMBanks(&sWifiNoteVRAMConfig);
}

static void WifiNoteDraw_InitBG(WifiNoteDraw *draw, HeapID heapId) {
    int i;

    GFL_BGSysCreate(heapId);
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&config);
    }
    for (i = 0; i < 5; i++) {
        GFL_BGSysCreateBG(i, &sWifiNoteBGSetups[i], BGMODE_TEXT);
        GFL_BGSysClearCharCore(i, BGSYS_TILE_SIZE_16, 0, heapId);
        GFL_BGSysFillScr(i, 0);
        GFL_BGSysSetBGEnabled(i, TRUE);
    }
    GFL_G2DIOLoadArcNCLRDefault(draw->arc, 0, 0, 0, 0xc0, heapId);
    GFL_G2DIOLoadArcNCLRDefault(draw->arc, 0, 4, 0, 0xc0, heapId);
    GFL_BGSysLoadArcNCGRStatic(draw->arc, 3, 0, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(draw->arc, 3, 2, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(draw->arc, 3, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(draw->arc, 5, 0, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(draw->arc, 14, 4, 0, 0, FALSE, heapId);
}

static void WifiNoteDraw_ExitBG(WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < 5; i++) {
        GFL_BGSysReleaseBG(i);
    }
    GFL_BGSysFree();
}

static void WifiNoteDraw_InitClAct(WifiNoteDraw *draw, HeapID heapId) {
    ClActSysSetup setup = data_02093f08;
    ArcTool *arc;

    setup.oamStartMain = 16;
    setup.oamCountMain = 112;
    ClActSys_Create(&setup, &sWifiNoteVRAMConfig, heapId);
    draw->unit = func_0204bf1c(32, 0, heapId);
    func_0204c028(draw->unit);
    draw->renderer = func_0204be9c(&sWifiNoteSurfaceSetup, 1, heapId);
    draw->scrollUnit = func_0204bf1c(18, 1, heapId);
    func_0204c018(draw->scrollUnit, draw->renderer);
    draw->chars[0] = func_0204b81c(draw->arc, 4, FALSE, CLACT_VRAM_MAIN, heapId);
    draw->palettes[0] = func_0204bba0(draw->arc, 1, CLACT_VRAM_MAIN, 0, heapId);
    draw->cellAnims[0] = func_0204bde0(draw->arc, 15, 16, heapId);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    draw->chars[1] = func_0204b81c(arc, func_0202d814(), FALSE, CLACT_VRAM_MAIN, heapId);
    draw->palettes[1] = func_0204bba0(arc, func_0202d810(), CLACT_VRAM_MAIN, 0x60, heapId);
    draw->cellAnims[1] = func_0204bde0(arc, func_0202d818(2), func_0202d81c(2), heapId);
    GFL_ArcToolFree(arc);
}

static void WifiNoteDraw_ExitClAct(WifiNoteDraw *draw) {
    u32 i;

    for (i = 0; i < 2; i++) {
        func_0204b98c(draw->chars[i]);
    }
    for (i = 0; i < 2; i++) {
        func_0204bcd0(draw->palettes[i]);
    }
    for (i = 0; i < 2; i++) {
        func_0204be64(draw->cellAnims[i]);
    }
    func_0204bf98(draw->scrollUnit);
    func_0204becc(draw->renderer);
    func_0204bf98(draw->unit);
    func_0204b758();
}

static void WifiNoteDraw_InitMsg(WifiNoteDraw *draw, HeapID heapId) {
    func_020232d8();
    draw->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    draw->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0418, heapId);
    draw->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    draw->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 3, 0x100);
    draw->stream = NULL;
    draw->queue = func_02021998(heapId);
}

static void WifiNoteDraw_ExitMsg(WifiNoteDraw *draw) {
    func_02021c44(draw->queue);
    func_02021a18(draw->queue);
    draw->queue = NULL;
    if (draw->stream != NULL) {
        func_020223cc(draw->stream);
        draw->stream = NULL;
    }
    GFL_TCBExMgrFree(draw->tcbManager);
    draw->tcbManager = NULL;
    GFL_FontFree(draw->font);
    GFL_MsgDataFree(draw->msgData);
    GFL_WordSetSystemFree(draw->wordSet);
}

static void WifiNoteDraw_InitTitle(WifiNoteDraw *draw, const WifiNoteData *data, HeapID heapId) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x140, 0x20, heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, heapId);
    LoadSysMsgBox(1, 70, 11, data->frameType, heapId);
    BmpWin_InitAllocator(heapId);
    WifiNote_CreateWindow(draw, &draw->title, 1, 1, 0, 24, 3, 10, 100, 0);
    draw->titleStr = GFL_StrBufCreate(128, heapId);
    draw->titleTemp = GFL_StrBufCreate(128, heapId);
    draw->str = GFL_StrBufCreate(128, heapId);
    draw->temp = GFL_StrBufCreate(128, heapId);
}

static void WifiNoteDraw_ExitTitle(WifiNoteDraw *draw) {
    GFL_StrBufFree(draw->str);
    GFL_StrBufFree(draw->temp);
    GFL_StrBufFree(draw->titleStr);
    GFL_StrBufFree(draw->titleTemp);
    BmpWin_Free(draw->title.window);
    BmpWin_FreeAllocator();
}

static void WifiNoteDraw_LoadScreens(WifiNoteDraw *draw, HeapID heapId) {
    int i;

    for (i = 0; i < 2; i++) {
        draw->screens.buffer[i] =
            GFL_G2DIOReadNSCRArc(draw->arc, sWifiNoteScreenFiles[i], FALSE, &draw->screens.screen[i], heapId);
    }
}

static void WifiNoteDraw_FreeScreens(WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < 2; i++) {
        GFL_HeapFree(draw->screens.buffer[i]);
    }
}

static void WifiNoteDraw_PrintTitle(WifiNoteDraw *draw, PrintWindow *window, u32 msg) {
    GFL_BitmapFill(BmpWin_GetBitmap(window->window), 0);
    BmpWin_FlushChar(window->window);
    GFL_MsgDataLoadStrbuf(draw->msgData, msg, draw->titleTemp);
    GFL_WordSetFormatStrbuf(draw->wordSet, draw->titleStr, draw->titleTemp);
    PrintStrAligned(window, draw->queue, 0, 4, draw->titleStr, draw->font, WIFI_NOTE_COLOR_WHITE, PRINT_ALIGN_LEFT);
    WifiNote_ShowWindow(window->window);
}

static void WifiNoteDraw_HideTitle(WifiNoteDraw *draw) {
    WifiNote_HideWindow(draw->title.window);
}

// Sets a Friend Code's three groups of digits as the words 1 to 3
static void WifiNoteDraw_SetFriendCode(WifiNoteDraw *draw, u64 code) {
    WordSetNumber(draw->wordSet, 1, code / 100000000, 4, 2, TRUE);
    WordSetNumber(draw->wordSet, 2, (code / 10000) % 10000, 4, 2, TRUE);
    WordSetNumber(draw->wordSet, 3, code % 10000, 4, 2, TRUE);
}

// Sets a friend's name as the word 0
static void WifiNoteDraw_SetFriendName(WifiNoteDraw *draw, WifiNoteData *data, u32 index, HeapID heapId) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    PlayerInfo *playerInfo = func_02008b0c(heapId);
    StrBuf *name = GFL_StrBufCreate(10, heapId);

    GFL_StrBufLoadString(name, func_0200a0f0(wifiList, index));
    copyTrainerNameFromStrbuf(playerInfo, name);
    GFL_StrBufFree(name);
    copyVarForText(draw->wordSet, 0, playerInfo);
    GFL_HeapFree(playerInfo);
}

// Sets the date the player last played with a friend, and returns whether there is one
static BOOL WifiNoteDraw_SetLastDate(WifiNoteDraw *draw, WifiNoteData *data, u32 index) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    BOOL result = TRUE;
    u32 number;

    number = func_02009f80(wifiList, index, WIFI_LIST_FRIEND_LAST_DAY);
    if (number == 0) {
        result = FALSE;
    }
    WordSetNumber(draw->wordSet, 2, number, 2, 0, TRUE);
    number = func_02009f80(wifiList, index, WIFI_LIST_FRIEND_LAST_YEAR);
    WordSetNumber(draw->wordSet, 0, number, 4, 1, TRUE);
    number = func_02009f80(wifiList, index, WIFI_LIST_FRIEND_LAST_MONTH);
    WordSetNumber(draw->wordSet, 1, number, 2, 0, TRUE);
    return result;
}

static void WifiNoteDraw_SetNumber(WifiNoteDraw *draw, u32 number) {
    WordSetNumber(draw->wordSet, 0, number, 4, 1, TRUE);
}

static void WifiNoteDraw_InitActors(WifiNoteDraw *draw, HeapID heapId) {
    int i;

    for (i = 0; i < 2; i++) {
        draw->arrows[i] = func_0204c040(draw->unit, draw->chars[1], draw->palettes[1], draw->cellAnims[1],
                                        &sWifiNoteButtonSetups[i], 0, heapId);
        func_0204c520(draw->arrows[i], TRUE);
        func_0204c124(draw->arrows[i], FALSE);
    }
    draw->returnButton = func_0204c040(draw->unit, draw->chars[1], draw->palettes[1], draw->cellAnims[1],
                                       &sWifiNoteButtonSetups[2], 0, heapId);
    func_0204c124(draw->returnButton, TRUE);
    draw->exitButton = func_0204c040(draw->unit, draw->chars[1], draw->palettes[1], draw->cellAnims[1],
                                     &sWifiNoteButtonSetups[3], 0, heapId);
    func_0204c124(draw->exitButton, TRUE);
}

static void WifiNoteDraw_ExitActors(WifiNoteDraw *draw) {
    int i;

    func_0204c108(draw->exitButton);
    func_0204c108(draw->returnButton);
    for (i = 0; i < 2; i++) {
        func_0204c108(draw->arrows[i]);
    }
}

static void WifiNoteDraw_SetArrowsVisible(WifiNoteDraw *draw, BOOL visible) {
    int i;

    for (i = 0; i < 2; i++) {
        func_0204c124(draw->arrows[i], visible);
    }
}

static void WifiNoteDraw_SetArrowsAnimating(WifiNoteDraw *draw, BOOL animating) {
    int i;

    for (i = 0; i < 2; i++) {
        func_0204c520(draw->arrows[i], animating);
    }
}

static void WifiNoteDraw_StartArrowAnim(WifiNoteDraw *draw, u32 arrow, u16 sequence) {
    func_0204c520(draw->arrows[arrow], TRUE);
    func_0204c488(draw->arrows[arrow], sequence);
    func_0204c56c(draw->arrows[arrow]);
}

// A yes/no menu at the bottom right, above the message window if low
static AppTaskMenu *WifiNote_CreateYesNoMenu(AppTaskMenuItem *items, AppTaskMenuRes **res, BOOL low, HeapID heapId) {
    AppTaskMenuInit init;

    init.heapId = heapId;
    init.itemCount = 2;
    init.items = items;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    if (low == TRUE) {
        init.y = 18;
    } else {
        init.y = 12;
    }
    init.width = 8;
    init.height = 3;
    return AppTaskMenu_Create(&init, *res);
}

// Returns the item picked once its flash is over, and frees the menu, or -1
static s32 WifiNote_UpdateMenu(AppTaskMenu *menu) {
    AppTaskMenu_Update(menu);
    if (AppTaskMenu_IsFlashFinished(menu) == TRUE) {
        s32 pos = AppTaskMenu_GetCursorPos(menu);

        AppTaskMenu_Free(menu);
        return pos;
    }
    return -1;
}

static void WifiNoteModeSelect_Init(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    data->touch = func_0203d554();
    WifiNoteModeSelect_InitDraw(select, data, draw, heapId);
}

static BOOL WifiNoteModeSelect_Main(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    BOOL result;

    switch (data->seq) {
    case MODE_SELECT_SEQ_INIT:
        WifiNoteModeSelect_Init(select, data, draw, HEAPID_WIFI_NOTE);
        WifiNoteModeSelect_Show(select, data, draw);
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq = MODE_SELECT_SEQ_FADE_IN;
        break;
    case MODE_SELECT_SEQ_FADE_IN:
        if (GFL_WipeIsFinished()) {
            data->seq = MODE_SELECT_SEQ_MAIN;
        }
        break;
    case MODE_SELECT_SEQ_INIT_NO_FADE:
        WifiNoteModeSelect_Init(select, data, draw, HEAPID_WIFI_NOTE);
        WifiNoteModeSelect_Show(select, data, draw);
        data->seq = MODE_SELECT_SEQ_MAIN;
        break;
    case MODE_SELECT_SEQ_MAIN:
        BlinkPalAnm_Main(draw->blink[0]);
        result = WifiNoteModeSelect_Input(select, data, draw);
        if (result) {
            if (select->cursor == MODE_SELECT_RETURN) {
                WifiNoteDraw_StartReturnAnim(draw);
            } else if (select->cursor == MODE_SELECT_EXIT) {
                WifiNoteDraw_StartExitAnim(draw);
            } else {
                select->buttonSeq = 0;
            }
            data->seq = MODE_SELECT_SEQ_BUTTON;
        }
        break;
    case MODE_SELECT_SEQ_BUTTON:
        if (select->cursor == MODE_SELECT_RETURN) {
            if (func_0204c560(draw->returnButton) == TRUE) {
                break;
            }
        } else if (select->cursor == MODE_SELECT_EXIT) {
            if (func_0204c560(draw->exitButton) == TRUE) {
                break;
            }
        } else if (WifiNoteModeSelect_UpdateButton(select) == TRUE) {
            break;
        }
        if (WifiNoteModeSelect_CanEnter(select, data) == TRUE) {
            WifiNoteModeSelect_SetNext(select, data);
            if (select->cursor == MODE_SELECT_RETURN) {
                data->seq = MODE_SELECT_SEQ_FADE_OUT;
            } else if (select->cursor == MODE_SELECT_EXIT) {
                data->seq = MODE_SELECT_SEQ_FADE_OUT;
            } else if (select->cursor == MODE_SELECT_CODE_IN) {
                data->seq = MODE_SELECT_SEQ_FADE_OUT;
            } else {
                WifiNoteModeSelect_Hide(select, draw);
                WifiNoteModeSelect_Exit(select, data, draw);
                return TRUE;
            }
        } else {
            WifiNoteModeSelect_StartMessage(select, draw, Bank0418_Text_FriendRosterFull, heapId);
            data->seq = MODE_SELECT_SEQ_MESSAGE;
        }
        break;
    case MODE_SELECT_SEQ_FADE_OUT:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq++;
        break;
    case MODE_SELECT_SEQ_FADE_OUT_WAIT:
        if (GFL_WipeIsFinished() == TRUE) {
            WifiNoteModeSelect_Exit(select, data, draw);
            return TRUE;
        }
        break;
    case MODE_SELECT_SEQ_CODE_INIT:
        WifiNoteModeSelect_Init(select, data, draw, HEAPID_WIFI_NOTE);
        WifiNoteModeSelect_Show(select, data, draw);
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq = MODE_SELECT_SEQ_CODE_FADE_IN;
        break;
    case MODE_SELECT_SEQ_CODE_FADE_IN:
        if (GFL_WipeIsFinished()) {
            PlayerInfo *playerInfo = func_02008b0c(heapId);
            u64 code;

            copyTrainerNameFromStrbuf(playerInfo, data->name);
            copyVarForText(draw->wordSet, 0, playerInfo);
            GFL_HeapFree(playerInfo);
            code = WifiNote_StrToFriendKey(data->code, &result);
            if (result) {
                WifiNoteDraw_SetFriendCode(draw, code);
                WifiNoteModeSelect_StartMessage(select, draw, Bank0418_Text_WillRegistered, heapId);
                data->seq = MODE_SELECT_SEQ_CODE_MESSAGE;
            } else {
                WifiNoteModeSelect_StartMessage(select, draw, Bank0418_Text_FriendCodeWrong, heapId);
                data->seq = MODE_SELECT_SEQ_MESSAGE;
            }
        }
        break;
    case MODE_SELECT_SEQ_CODE_MESSAGE:
        if (WifiNoteModeSelect_IsMessageDone(select, data, draw)) {
            select->menu = WifiNote_CreateYesNoMenu(select->yesNoItems, &select->menuRes, TRUE, heapId);
            data->seq = MODE_SELECT_SEQ_CODE_YES_NO;
        }
        break;
    case MODE_SELECT_SEQ_CODE_YES_NO:
        switch (WifiNote_UpdateMenu(select->menu)) {
        case 0: {
            u32 error;

            data->touch = func_0203d554();
            error = WifiNoteData_AddFriend(data, data->code, data->name);
            if (error == 0) {
                WifiNoteModeSelect_ClearMessage(select, draw);
                WifiNoteModeSelect_DrawCursor(select);
                data->seq = MODE_SELECT_SEQ_MAIN;
            } else {
                WifiNoteModeSelect_StartMessage(select, draw, sWifiNoteModeSelectAddFriendMsgs[error], heapId);
                data->seq = MODE_SELECT_SEQ_MESSAGE;
            }
            break;
        }
        case 1:
            data->touch = func_0203d554();
            WifiNoteModeSelect_ClearMessage(select, draw);
            WifiNoteModeSelect_DrawCursor(select);
            data->seq = MODE_SELECT_SEQ_MAIN;
            break;
        }
        break;
    case MODE_SELECT_SEQ_MESSAGE:
        if (WifiNoteModeSelect_IsMessageDone(select, data, draw)) {
            if (WifiNote_IsNextPressed(data) == TRUE) {
                WifiNoteModeSelect_ClearMessage(select, draw);
                WifiNoteModeSelect_DrawCursor(select);
                data->seq = MODE_SELECT_SEQ_MAIN;
            }
        }
        break;
    }
    return FALSE;
}

static void WifiNoteModeSelect_Exit(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    WifiNoteModeSelect_ExitDraw(select, data, draw);
}

static void WifiNoteModeSelect_InitDraw(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId) {
    int i;
    StrBuf *str;
    StrBuf *temp;

    WifiNote_CreateWindow(draw, &select->menuWindow, 3, 4, 5, 24, 20, 10, 1, 0);
    WifiNote_CreateWindow(draw, &select->msgWindow, 1, 2, 19, 27, 4, 10, 172, 0);
    select->msgStr = GFL_StrBufCreate(128, heapId);
    str = GFL_StrBufCreate(128, heapId);
    temp = GFL_StrBufCreate(128, heapId);
    copyVarForText(draw->wordSet, 0, GetGameDataPlayerInfo(data->gameData));
    for (i = 0; i < 3; i++) {
        u32 width;

        GFL_MsgDataLoadStrbuf(draw->msgData, Bank0418_Text_CheckFriendRoster + i, temp);
        GFL_WordSetFormatStrbuf(draw->wordSet, str, temp);
        width = GFL_FontGetBlockWidth(str, draw->font, 0);
        PrintStrAligned(&select->menuWindow, draw->queue, (192 - width) / 2, i * 40 + 8, str, draw->font,
                        WIFI_NOTE_COLOR_BLACK, PRINT_ALIGN_LEFT);
    }
    WifiNote_ShowWindow(select->menuWindow.window);
    GFL_StrBufFree(str);
    GFL_StrBufFree(temp);
    select->screenBuffer = GFL_G2DIOReadNSCRArc(draw->arc, 13, FALSE, &select->screen, heapId);
    select->yesNoItems[0].str = GFL_MsgDataLoadStrbufNew(draw->msgData, Bank0418_Text_Yes);
    select->yesNoItems[0].color = WIFI_NOTE_MENU_COLOR;
    select->yesNoItems[0].type = 0;
    select->yesNoItems[1].str = GFL_MsgDataLoadStrbufNew(draw->msgData, Bank0418_Text_No);
    select->yesNoItems[1].color = WIFI_NOTE_MENU_COLOR;
    select->yesNoItems[1].type = 0;
    select->menuRes = AppTaskMenuRes_Create(1, 14, draw->font, draw->queue, heapId);
}

static void WifiNoteModeSelect_ExitDraw(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    WifiNoteDraw_ClearPrintWindows(draw);
    WifiNoteDraw_AddPrintWindow(draw, &draw->title);
    AppTaskMenuRes_Free(select->menuRes);
    GFL_StrBufFree(select->yesNoItems[0].str);
    GFL_StrBufFree(select->yesNoItems[1].str);
    GFL_StrBufFree(select->msgStr);
    BmpWin_Free(select->msgWindow.window);
    BmpWin_Free(select->menuWindow.window);
    GFL_HeapFree(select->screenBuffer);
}

static void WifiNoteModeSelect_Show(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    WifiNoteDraw_PrintTitle(draw, &draw->title, Bank0418_Text_PalPad);
    WifiNote_LoadScreen(2, select->screen);
    GFL_BGSysMoveBG(2, 0, 0);
    GFL_BGSysMoveBG(2, 3, 0);
    if (data->touch == FALSE) {
        WifiNoteModeSelect_SetCursor(draw, select->cursor);
    } else {
        GFL_BGSysQueueScrLoad(2);
    }
}

// Returns whether a button was picked
static BOOL WifiNoteModeSelect_Keys(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        WifiNoteModeSelect_ClearCursor(draw, select->cursor);
        func_0203d564(FALSE);
        data->touch = FALSE;
        select->cursor = MODE_SELECT_RETURN;
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        WifiNoteModeSelect_ClearCursor(draw, select->cursor);
        func_0203d564(FALSE);
        data->touch = FALSE;
        select->cursor = MODE_SELECT_EXIT;
        return TRUE;
    }
    if (data->touch == TRUE) {
        if (GCTX_HIDGetPressedKeys() & WIFI_NOTE_KEYS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            WifiNoteModeSelect_MoveCursor(draw, select->cursor, select->cursor);
            func_0203d564(FALSE);
            data->touch = FALSE;
        }
        return FALSE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        BlinkPalAnm_InitAnime(draw->blink[0]);
        return TRUE;
    }
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        if (select->cursor != 0) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            WifiNoteModeSelect_MoveCursor(draw, select->cursor - 1, select->cursor);
            select->cursor--;
            return FALSE;
        }
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        if (select->cursor < MODE_SELECT_MY_CODE) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            WifiNoteModeSelect_MoveCursor(draw, select->cursor + 1, select->cursor);
            select->cursor++;
            return FALSE;
        }
    }
    return FALSE;
}

static s32 WifiNoteModeSelect_Touch(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    s32 hit = func_0203da0c(sWifiNoteModeSelectTouchRects);

    if (hit != TOUCH_RECT_NONE) {
        if (hit == MODE_SELECT_RETURN) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            WifiNoteModeSelect_ClearCursor(draw, select->cursor);
        } else if (hit == MODE_SELECT_EXIT) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            WifiNoteModeSelect_ClearCursor(draw, select->cursor);
        } else {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            WifiNoteModeSelect_MoveCursor(draw, hit, select->cursor);
        }
        select->cursor = hit;
        func_0203d564(TRUE);
        data->touch = TRUE;
    }
    return hit;
}

static BOOL WifiNoteModeSelect_Input(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    if (WifiNoteModeSelect_Touch(select, data, draw) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return WifiNoteModeSelect_Keys(select, data, draw);
}

static void WifiNoteModeSelect_SetNext(WifiNoteModeSelect *select, WifiNoteData *data) {
    WifiNoteData_SetNext(data, sWifiNoteModeSelectModes[select->cursor], 0);
}

static void WifiNoteModeSelect_Hide(WifiNoteModeSelect *select, WifiNoteDraw *draw) {
    WifiNoteDraw_HideTitle(draw);
    GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 24, 16);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysFillScrArea(3, 0, 0, 0, 32, 24, 16);
    GFL_BGSysQueueScrLoad(3);
    WifiNote_HideWindow(select->menuWindow.window);
}

static void WifiNoteModeSelect_StartMessage(WifiNoteModeSelect *select, WifiNoteDraw *draw, u32 msg, HeapID heapId) {
    StrBuf *temp;

    if (draw->stream != NULL) {
        func_020223cc(draw->stream);
        draw->stream = NULL;
    }
    temp = GFL_StrBufCreate(128, heapId);
    GFL_BitmapFill(BmpWin_GetBitmap(select->msgWindow.window), 15);
    GFL_MsgDataLoadStrbuf(draw->msgData, msg, temp);
    GFL_WordSetFormatStrbuf(draw->wordSet, select->msgStr, temp);
    draw->stream = func_02022268(select->msgWindow.window, 0, 0, select->msgStr, draw->font, func_02017bcc(),
                                 draw->tcbManager, 10, draw->heapId, 0);
    BmpWin_DrawFrame(select->msgWindow.window, 2, 70, 11);
    WifiNote_ShowWindowNow(select->msgWindow.window);
    GFL_StrBufFree(temp);
    WifiNote_SetBrightness(FALSE);
}

// Runs a message's stream, which the A and B buttons and the touch screen speed up and continue. Returns FALSE once
// the message is over
static BOOL WifiNote_UpdateStream(PrintStream *stream, BOOL *paused) {
    switch (func_020223b4(stream)) {
    case 0:
        if (func_0203da48() == TRUE || (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            func_020223e0(stream, 0);
        }
        *paused = FALSE;
        break;
    case 1:
        if (*paused == FALSE) {
            if (func_0203da48() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                func_020223bc(stream);
                *paused = TRUE;
            }
        }
        break;
    case 2:
        func_020223cc(stream);
        *paused = FALSE;
        return FALSE;
    }
    return TRUE;
}

static BOOL WifiNoteModeSelect_IsMessageDone(WifiNoteModeSelect *select, WifiNoteData *data, WifiNoteDraw *draw) {
    if (!WifiNote_UpdateStream(draw->stream, &draw->streamPaused)) {
        data->touch = FALSE;
        return TRUE;
    }
    return FALSE;
}

static void WifiNoteModeSelect_ClearMessage(WifiNoteModeSelect *select, WifiNoteDraw *draw) {
    WifiNote_SetBrightness(TRUE);
    BmpWin_ClearFrame(select->msgWindow.window, 0);
    WifiNote_HideWindow(select->msgWindow.window);
}

// Whether the screen of the cursor's button can be entered: the code entry needs a free place in the Wi-Fi list
static BOOL WifiNoteModeSelect_CanEnter(WifiNoteModeSelect *select, WifiNoteData *data) {
    if (select->cursor == MODE_SELECT_CODE_IN) {
        int i;
        WifiList *wifiList = GameData_GetWifiList(data->gameData);

        for (i = 0; i < WIFI_LIST_FRIEND_MAX; i++) {
            if (!func_0200a138(wifiList, i)) {
                return TRUE;
            }
        }
    } else {
        return TRUE;
    }
    return FALSE;
}

static void WifiNoteFriendList_Init(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    WifiNoteFriendList_InitDraw(list, data, draw, heapId);
}

static BOOL WifiNoteFriendList_Main(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    switch (data->seq) {
    case FRIEND_LIST_SEQ_INIT:
        list->page = 0;
        list->prevPage = 0;
        list->pos = 0;
        list->prevPos = 0;
        // fallthrough
    case FRIEND_LIST_SEQ_INIT_KEEP_POS:
        WifiNoteFriendList_Init(list, data, draw, HEAPID_WIFI_NOTE);
        WifiNoteData_MakeFriends(data);
        WifiNoteFriendList_Show(list, data, draw, heapId);
        data->seq = FRIEND_LIST_SEQ_MAIN;
        break;
    case FRIEND_LIST_SEQ_MAIN:
        BlinkPalAnm_Main(draw->blink[0]);
        BlinkPalAnm_Main(draw->blink[1]);
        switch (WifiNoteFriendList_Input(list, data, draw)) {
        case FRIEND_LIST_INPUT_NONE:
            break;
        case FRIEND_LIST_INPUT_PREV_PAGE:
            WifiNoteFriendList_RequestScroll(list, data, FRIEND_LIST_SCROLL_PREV);
            WifiNoteFriendList_SetPageCursor(list, list->page);
            BlinkPalAnm_InitAnime(draw->blink[1]);
            BlinkPalAnm_Main(draw->blink[1]);
            break;
        case FRIEND_LIST_INPUT_NEXT_PAGE:
            WifiNoteFriendList_RequestScroll(list, data, FRIEND_LIST_SCROLL_NEXT);
            WifiNoteFriendList_SetPageCursor(list, list->page);
            BlinkPalAnm_InitAnime(draw->blink[1]);
            BlinkPalAnm_Main(draw->blink[1]);
            break;
        case FRIEND_LIST_INPUT_RETURN:
            data->seq = FRIEND_LIST_SEQ_RETURN;
            break;
        case FRIEND_LIST_INPUT_EXIT:
            data->seq = FRIEND_LIST_SEQ_EXIT;
            break;
        case FRIEND_LIST_INPUT_DECIDE:
            data->seq = FRIEND_LIST_SEQ_BUTTON;
            break;
        }
        break;
    case FRIEND_LIST_SEQ_SCROLL_START:
        WifiNoteFriendList_ScrollStart(list, data, draw, heapId);
        data->seq = FRIEND_LIST_SEQ_SCROLL;
        break;
    case FRIEND_LIST_SEQ_SCROLL:
        if (WifiNoteFriendList_Scroll(list, data, draw, heapId) == TRUE) {
            data->seq = FRIEND_LIST_SEQ_SCROLL_END;
        }
        break;
    case FRIEND_LIST_SEQ_SCROLL_END:
        if (WifiNoteFriendList_ScrollEnd(list, draw) == TRUE) {
            data->seq = FRIEND_LIST_SEQ_MAIN;
        }
        break;
    case FRIEND_LIST_SEQ_BUTTON:
        if (WifiNoteFriendList_UpdateButton(list) != TRUE) {
            WifiNoteData_SetFriendPos(data, WifiNoteFriendList_GetFriend(list));
            WifiNoteFriendList_StartMenuMessage(list, data, draw, heapId);
            data->seq = FRIEND_LIST_SEQ_MENU_MESSAGE;
        }
        break;
    case FRIEND_LIST_SEQ_MENU_MESSAGE:
        if (WifiNoteFriendList_IsMessageDone(list, data, draw) == TRUE) {
            WifiNoteFriendList_OpenMenu(list, data, draw, heapId);
            data->seq = FRIEND_LIST_SEQ_MENU;
        }
        break;
    case FRIEND_LIST_SEQ_MENU:
        data->seq = WifiNoteFriendList_UpdateMenu(list, data, draw);
        break;
    case FRIEND_LIST_SEQ_INFO:
        list->wait = 8;
        data->seq = FRIEND_LIST_SEQ_INFO_WAIT;
        break;
    case FRIEND_LIST_SEQ_INFO_WAIT:
        list->wait--;
        if (list->wait <= 0) {
            WifiNoteFriendList_Hide(list, data, draw);
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_FRIEND_INFO, FRIEND_INFO_SEQ_INIT);
            WifiNoteFriendList_Exit(list, data, draw);
            return TRUE;
        }
        break;
    case FRIEND_LIST_SEQ_INFO_RETURN:
        list->page = data->friendPos / FRIEND_LIST_PAGE_SIZE;
        list->pos = data->friendPos % FRIEND_LIST_PAGE_SIZE;
        data->seq = FRIEND_LIST_SEQ_INIT_KEEP_POS;
        break;
    case FRIEND_LIST_SEQ_NAME: {
        s32 friend;
        WifiList *wifiList;

        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, heapId);
        friend = WifiNoteFriendList_GetFriend(list);
        wifiList = GameData_GetWifiList(data->gameData);
        WifiNoteData_SetOldName(data, func_0200a0f0(wifiList, data->friends.index[friend]));
        data->seq = FRIEND_LIST_SEQ_NAME_WAIT;
        break;
    }
    case FRIEND_LIST_SEQ_NAME_WAIT:
        if (GFL_WipeIsFinished()) {
            WifiNoteFriendList_Hide(list, data, draw);
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_CODE_IN, CODE_IN_SEQ_NAME_INIT);
            WifiNoteFriendList_Exit(list, data, draw);
            return TRUE;
        }
        break;
    case FRIEND_LIST_SEQ_NAME_RETURN: {
        s32 friend = WifiNoteFriendList_GetFriend(list);
        WifiList *wifiList = GameData_GetWifiList(data->gameData);

        func_0200a11c(wifiList, data->friends.index[friend], data->name);
    }
        // fallthrough
    case FRIEND_LIST_SEQ_NAME_CANCEL:
        WifiNoteFriendList_Init(list, data, draw, HEAPID_WIFI_NOTE);
        WifiNoteFriendList_Show(list, data, draw, heapId);
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq = FRIEND_LIST_SEQ_FADE_IN;
        break;
    case FRIEND_LIST_SEQ_FADE_IN:
        if (GFL_WipeIsFinished()) {
            data->seq = FRIEND_LIST_SEQ_MAIN;
        }
        break;
    case FRIEND_LIST_SEQ_DELETE_MESSAGE:
        WifiNoteFriendList_StartDeleteMessage(list, data, draw, heapId);
        data->seq = FRIEND_LIST_SEQ_DELETE_MESSAGE_WAIT;
        break;
    case FRIEND_LIST_SEQ_DELETE_MESSAGE_WAIT:
        if (WifiNoteFriendList_OpenYesNo(list, data, draw, heapId) == TRUE) {
            data->seq = FRIEND_LIST_SEQ_DELETE_YES_NO;
        }
        break;
    case FRIEND_LIST_SEQ_DELETE_YES_NO:
        switch (WifiNote_UpdateMenu(list->menu)) {
        case 0:
            data->touch = func_0203d554();
            WifiNoteFriendList_ClearMessage(list, draw);
            data->seq = FRIEND_LIST_SEQ_DELETE_START;
            break;
        case 1:
            data->touch = func_0203d554();
            WifiNoteFriendList_ClearMessage(list, draw);
            data->seq = FRIEND_LIST_SEQ_MAIN;
            break;
        }
        break;
    case FRIEND_LIST_SEQ_DELETE_START:
        WifiNoteFriendList_StartErase(list, draw);
        data->seq = FRIEND_LIST_SEQ_DELETE;
        break;
    case FRIEND_LIST_SEQ_DELETE:
        if (WifiNoteFriendList_UpdateErase(list, draw) == TRUE) {
            WifiList *wifiList = GameData_GetWifiList(data->gameData);

            func_0200a190(wifiList, data->friends.index[WifiNoteFriendList_GetFriend(list)]);
            WifiNoteData_MakeFriends(data);
            WifiNoteFriendList_DrawPage(list, data, draw, list->page, list->pos, 0, heapId);
            data->seq = FRIEND_LIST_SEQ_DELETE_END;
        }
        break;
    case FRIEND_LIST_SEQ_DELETE_END:
        WifiNoteFriendList_EndErase(list, draw);
        data->seq = FRIEND_LIST_SEQ_MAIN;
        break;
    case FRIEND_LIST_SEQ_CODE:
        if (WifiNoteFriendList_StartCodeMessage(list, data, draw, heapId) == TRUE) {
            data->seq = FRIEND_LIST_SEQ_CODE_WAIT;
        } else {
            data->seq = FRIEND_LIST_SEQ_MAIN;
        }
        break;
    case FRIEND_LIST_SEQ_CODE_WAIT:
        if (WifiNoteFriendList_IsMessageDone(list, data, draw)) {
            if (WifiNote_IsNextPressed(data) == TRUE) {
                data->seq = FRIEND_LIST_SEQ_MAIN;
                WifiNoteFriendList_ClearMessage(list, draw);
            }
        }
        break;
    case FRIEND_LIST_SEQ_RETURN:
        if (func_0204c560(draw->returnButton) != TRUE) {
            WifiNoteFriendList_Hide(list, data, draw);
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_SELECT, MODE_SELECT_SEQ_INIT_NO_FADE);
            WifiNoteFriendList_Exit(list, data, draw);
            return TRUE;
        }
        break;
    case FRIEND_LIST_SEQ_EXIT:
        if (func_0204c560(draw->exitButton) != TRUE) {
            data->seq = FRIEND_LIST_SEQ_FADE_OUT;
        }
        break;
    case FRIEND_LIST_SEQ_FADE_OUT:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq = FRIEND_LIST_SEQ_FADE_OUT_WAIT;
        break;
    case FRIEND_LIST_SEQ_FADE_OUT_WAIT:
        if (GFL_WipeIsFinished()) {
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_EXIT, 0);
            WifiNoteFriendList_Exit(list, data, draw);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void WifiNoteFriendList_Exit(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    WifiNoteFriendList_ExitDraw(list, data, draw);
}

static void WifiNoteFriendList_InitDraw(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId) {
    int i;

    for (i = 0; i < 2; i++) {
        WifiNote_CreateWindow(draw, &list->nameWindows[i], 3, MATH_ABS(sWifiNoteScreenAreas[i].x) + 4,
                              MATH_ABS(sWifiNoteScreenAreas[i].y) + 4, 26, 17, 10, 0, 0);
    }
    for (i = 0; i < FRIEND_LIST_AREA_COUNT; i++) {
        WifiNoteListArea_Init(&list->areas[i], draw, &sWifiNoteScreenAreas[i], 0, heapId);
    }
    list->areas[0].nameWindow = &list->nameWindows[0];
    list->areas[2].nameWindow = &list->nameWindows[1];
    list->areas[1].nameWindow = &list->nameWindows[1];
    list->charSys = WF_2DC_SysInit(draw->scrollUnit, NULL, 16, heapId);
    WF_2DC_UnionResSet(list->charSys, CLACT_VRAM_MAIN, 2, 0xe0, heapId);
    for (i = 0; i < 4; i++) {
        list->menuItems[0][i].str = GFL_MsgDataLoadStrbufNew(draw->msgData, sWifiNoteFriendListMenu[0][i].msg);
        list->menuItems[0][i].color = WIFI_NOTE_MENU_COLOR;
        list->menuItems[0][i].type = 0;
        list->menuItems[1][i].str = GFL_MsgDataLoadStrbufNew(draw->msgData, sWifiNoteFriendListMenu[1][i].msg);
        list->menuItems[1][i].color = WIFI_NOTE_MENU_COLOR;
        list->menuItems[1][i].type = 0;
    }
    list->menuItems[0][3].type = APP_TASKMENU_ITEM_RETURN;
    list->menuItems[1][3].type = APP_TASKMENU_ITEM_RETURN;
    list->yesNoItems[0].str = GFL_MsgDataLoadStrbufNew(draw->msgData, Bank0418_Text_Yes);
    list->yesNoItems[0].color = WIFI_NOTE_MENU_COLOR;
    list->yesNoItems[0].type = 0;
    list->yesNoItems[1].str = GFL_MsgDataLoadStrbufNew(draw->msgData, Bank0418_Text_No);
    list->yesNoItems[1].color = WIFI_NOTE_MENU_COLOR;
    list->yesNoItems[1].type = 0;
    list->menuRes = AppTaskMenuRes_Create(1, 14, draw->font, draw->queue, heapId);
    WifiNote_CreateWindow(draw, &list->msgWindow, 1, 2, 1, 27, 4, 10, 0, 15);
    list->msgStr = GFL_StrBufCreate(128, heapId);
    {
        ClActorSetup setup = { 0, 0, 1, 0, 0 };

        list->eraser =
            func_0204c040(draw->unit, draw->chars[0], draw->palettes[0], draw->cellAnims[0], &setup, 0, heapId);
        func_0204c124(list->eraser, FALSE);
    }
    {
        ClActorSetup setup = { 56, 180, 2, 0, 0 };

        list->pageCursor =
            func_0204c040(draw->unit, draw->chars[0], draw->palettes[0], draw->cellAnims[0], &setup, 0, heapId);
    }
}

static void WifiNoteFriendList_ExitDraw(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    int i;

    WifiNoteDraw_ClearPrintWindows(draw);
    WifiNoteDraw_AddPrintWindow(draw, &draw->title);
    func_0204c108(list->pageCursor);
    func_0204c108(list->eraser);
    BmpWin_Free(list->msgWindow.window);
    GFL_StrBufFree(list->msgStr);
    AppTaskMenuRes_Free(list->menuRes);
    GFL_StrBufFree(list->yesNoItems[0].str);
    GFL_StrBufFree(list->yesNoItems[1].str);
    for (i = 0; i < 4; i++) {
        GFL_StrBufFree(list->menuItems[0][i].str);
        GFL_StrBufFree(list->menuItems[1][i].str);
    }
    WF_2DC_UnionResDel(list->charSys);
    WF_2DC_SysExit(list->charSys);
    for (i = 0; i < FRIEND_LIST_AREA_COUNT; i++) {
        WifiNoteListArea_Exit(&list->areas[i], draw);
    }
    for (i = 0; i < 2; i++) {
        BmpWin_Free(list->nameWindows[i].window);
    }
}

static void WifiNoteFriendList_Show(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    WifiNoteDraw_PrintTitle(draw, &draw->title, Bank0418_Text_FriendRoster);
    GFL_BGSysQueueScrLoad(0);
    WifiNoteFriendList_DrawPage(list, data, draw, list->page, list->pos, 0, heapId);
}

static void WifiNoteFriendList_Hide(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    int i;

    WifiNoteDraw_HideTitle(draw);
    for (i = 0; i < FRIEND_LIST_AREA_COUNT; i++) {
        WifiNoteListArea_Clear(&list->areas[i], draw, &draw->screens);
    }
    GFL_BGSysQueueScrLoad(0);
}

// Returns the FRIEND_LIST_INPUT_* that an input comes to, with its sound
static u32 WifiNoteFriendList_Decide(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, u32 input) {
    if (input == FRIEND_LIST_INPUT_NONE) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        WifiNoteDraw_StartReturnAnim(draw);
        return FRIEND_LIST_INPUT_RETURN;
    }
    if (input == FRIEND_LIST_INPUT_X) {
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        WifiNoteDraw_StartExitAnim(draw);
        return FRIEND_LIST_INPUT_EXIT;
    }
    if (input == FRIEND_LIST_INPUT_DECIDE) {
        if (list->pos == FRIEND_LIST_POS_RETURN) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            WifiNoteDraw_StartReturnAnim(draw);
            return FRIEND_LIST_INPUT_RETURN;
        }
        if (list->pos == FRIEND_LIST_POS_EXIT) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            WifiNoteDraw_StartExitAnim(draw);
            return FRIEND_LIST_INPUT_EXIT;
        }
        if (data->friends.count <= list->page * FRIEND_LIST_PAGE_SIZE + list->pos) {
            return FRIEND_LIST_INPUT_NONE;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        list->buttonSeq = 0;
        return FRIEND_LIST_INPUT_DECIDE;
    }
    if (input == FRIEND_LIST_INPUT_PREV_PAGE) {
        return FRIEND_LIST_INPUT_PREV_PAGE;
    }
    if (input == FRIEND_LIST_INPUT_NEXT_PAGE) {
        return FRIEND_LIST_INPUT_NEXT_PAGE;
    }
    return FRIEND_LIST_INPUT_NONE;
}

static u32 WifiNoteFriendList_Keys(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    u16 prevPos;
    BOOL turn;
    BOOL next;
    BOOL moved;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        data->touch = FALSE;
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_NONE);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        func_0203d564(FALSE);
        data->touch = FALSE;
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_X);
    }
    if (data->touch == TRUE) {
        if (GCTX_HIDGetPressedKeys() & WIFI_NOTE_KEYS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            WifiNoteFriendList_MoveCursor(draw, list->pos, list->pos);
            func_0203d564(FALSE);
            data->touch = FALSE;
        }
        return FRIEND_LIST_INPUT_NONE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_DECIDE);
    }
    prevPos = list->pos;
    moved = FALSE;
    turn = FALSE;
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        if (list->pos == 0) {
            list->pos = 3;
        } else if (list->pos == 4) {
            list->pos = 7;
        } else {
            list->pos--;
        }
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        if (list->pos == 3) {
            list->pos = 0;
        } else if (list->pos == 7) {
            list->pos = 4;
        } else {
            list->pos++;
        }
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        if (list->pos < 4) {
            list->pos += 4;
            turn = TRUE;
            next = FALSE;
        } else {
            list->pos -= 4;
        }
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        if (list->pos < 4) {
            list->pos += 4;
        } else {
            list->pos -= 4;
            turn = TRUE;
            next = TRUE;
        }
        moved = TRUE;
    } else if (data->lrEnabled) {
        if (data->lrEnabled && (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L)) {
            turn = TRUE;
            next = FALSE;
        } else if (data->lrEnabled && (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R)) {
            turn = TRUE;
            next = TRUE;
        }
    }
    if (turn == TRUE) {
        if (next) {
            list->prevPage = list->page;
            list->page = (list->page + 1) % FRIEND_LIST_PAGE_COUNT;
            return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_NEXT_PAGE);
        }
        list->prevPage = list->page;
        list->page--;
        if (list->page < 0) {
            list->page += FRIEND_LIST_PAGE_COUNT;
        }
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_PREV_PAGE);
    }
    if (moved == TRUE) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WifiNoteFriendList_MoveCursor(draw, list->pos, prevPos);
        list->prevPos = prevPos;
    }
    return FRIEND_LIST_INPUT_NONE;
}

static u32 WifiNoteFriendList_Touch(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, BOOL *touched) {
    u32 hit = func_0203da0c(sWifiNoteFriendListTouchRects);
    s8 turn;

    if (hit == TOUCH_RECT_NONE) {
        return FRIEND_LIST_INPUT_NONE;
    }
    if (hit <= FRIEND_LIST_PAGE_SIZE - 1) {
        *touched = TRUE;
        WifiNoteFriendList_ClearCursor(draw, list->pos);
        list->pos = hit;
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_DECIDE);
    }
    if (hit == FRIEND_LIST_POS_RETURN) {
        *touched = TRUE;
        list->pos = hit;
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_DECIDE);
    }
    if (hit == FRIEND_LIST_POS_EXIT) {
        *touched = TRUE;
        list->pos = hit;
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_DECIDE);
    }
    turn = hit - 10 - list->page;
    if (turn != 0) {
        *touched = TRUE;
        list->prevPage = list->page;
        list->page += turn;
        if (turn >= 0) {
            list->page %= FRIEND_LIST_PAGE_COUNT;
            return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_NEXT_PAGE);
        }
        if (list->page < 0) {
            list->page += FRIEND_LIST_PAGE_COUNT;
        }
        return WifiNoteFriendList_Decide(list, data, draw, FRIEND_LIST_INPUT_PREV_PAGE);
    }
    return FRIEND_LIST_INPUT_NONE;
}

static u32 WifiNoteFriendList_Input(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    BOOL touched = FALSE;
    u32 input = WifiNoteFriendList_Touch(list, data, draw, &touched);

    if (touched) {
        func_0203d564(TRUE);
        data->touch = TRUE;
        return input;
    }
    return WifiNoteFriendList_Keys(list, data, draw);
}

static void WifiNoteFriendList_ScrollStart(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                           HeapID heapId) {
    u32 area;

    if (list->scrollDir == FRIEND_LIST_SCROLL_PREV) {
        area = 2;
    } else {
        area = 1;
    }
    WifiNoteFriendList_DrawPage(list, data, draw, list->page, list->pos, area, heapId);
    WifiNoteFriendList_ClearCursor(draw, list->pos);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static BOOL WifiNoteFriendList_Scroll(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    ClActorPos pos;
    s32 scroll;

    if (!func_02021c0c(draw->queue)) {
        return FALSE;
    }
    if (list->scrollCount > FRIEND_LIST_SCROLL_FRAMES) {
        u32 area;

        pos.x = sWifiNoteSurfaceSetup.x;
        pos.y = sWifiNoteSurfaceSetup.y;
        func_0204bedc(draw->renderer, 0, &pos);
        if (list->scrollDir == FRIEND_LIST_SCROLL_PREV) {
            area = 2;
        } else {
            area = 1;
        }
        WifiNoteListArea_DeleteActors(&list->areas[area], draw);
        WifiNoteFriendList_SetPage(list, data, draw, list->page, list->pos, heapId);
        return TRUE;
    }
    scroll = list->scrollCount * 256 / FRIEND_LIST_SCROLL_FRAMES;
    if (list->scrollDir == FRIEND_LIST_SCROLL_PREV) {
        scroll = -scroll;
    }
    GFL_BGSysMoveBGReq(2, 0, scroll);
    GFL_BGSysMoveBGReq(3, 0, scroll);
    pos.x = sWifiNoteSurfaceSetup.x;
    pos.y = sWifiNoteSurfaceSetup.y;
    pos.x += scroll;
    func_0204bedc(draw->renderer, 0, &pos);
    if (list->scrollCount % 6 != 0) {
        WifiNoteScrollFunc func = sWifiNoteFriendListScrollFuncs[list->scrollCount / 6];

        if (func != NULL) {
            func(list, draw, list->page, list->prevPage);
        }
    }
    list->scrollCount++;
    return FALSE;
}

static BOOL WifiNoteFriendList_ScrollEnd(WifiNoteFriendList *list, WifiNoteDraw *draw) {
    if (func_02021c0c(draw->queue) == TRUE) {
        GFL_BGSysMoveBGReq(2, 0, 0);
        GFL_BGSysMoveBGReq(3, 0, 0);
        WifiNote_ShowWindow(list->areas[0].nameWindow->window);
        return TRUE;
    }
    return FALSE;
}

// The friend key of the cursor's friend, 0 for a friend met in person
static u64 WifiNoteFriendList_GetFriendKey(WifiNoteFriendList *list, WifiNoteData *data) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);

    return func_02057e3c(func_0200a0e4(wifiList, data->friends.index[WifiNoteFriendList_GetFriend(list)]));
}

static void WifiNoteFriendList_StartMenuMessage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                                HeapID heapId) {
    WifiNoteDraw_SetFriendName(draw, data, data->friends.index[WifiNoteFriendList_GetFriend(list)], heapId);
    WifiNoteFriendList_StartMessage(list, draw, Bank0418_Text_WhatWouldLikeS, heapId);
}

static void WifiNoteFriendList_OpenMenu(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    u32 gender =
        func_02009f80(wifiList, data->friends.index[WifiNoteFriendList_GetFriend(list)], WIFI_LIST_FRIEND_GENDER);
    AppTaskMenuInit init;

    init.heapId = heapId;
    init.itemCount = 4;
    if (gender == WIFI_LIST_GENDER_UNKNOWN) {
        if (WifiNoteFriendList_GetFriendKey(list, data) != 0) {
            init.items = list->menuItems[1];
        } else {
            init.items = list->menuItems[0];
        }
        list->menuType = 1;
    } else {
        init.items = list->menuItems[0];
        list->menuType = 0;
    }
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 17;
    init.height = 3;
    list->menu = AppTaskMenu_Create(&init, list->menuRes);
}

// Returns the sequence to go to
static u32 WifiNoteFriendList_UpdateMenu(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    u32 pos;
    u32 seq;

    AppTaskMenu_Update(list->menu);
    if (AppTaskMenu_IsFlashFinished(list->menu) == TRUE) {
        pos = AppTaskMenu_GetCursorPos(list->menu);
        AppTaskMenu_Free(list->menu);
    } else {
        return FRIEND_LIST_SEQ_MENU;
    }
    if (pos == 3) {
        data->touch = func_0203d554();
        seq = FRIEND_LIST_SEQ_MAIN;
    } else {
        data->touch = func_0203d554();
        seq = sWifiNoteFriendListMenu[list->menuType][pos].seq;
    }
    if (seq == FRIEND_LIST_SEQ_MAIN) {
        WifiNoteFriendList_ClearMessage(list, draw);
    } else if (seq == FRIEND_LIST_SEQ_INFO || seq == FRIEND_LIST_SEQ_NAME) {
        WifiNoteFriendList_ClearMessage(list, draw);
    } else {
        WifiNoteFriendList_EraseMessage(list);
    }
    return seq;
}

static BOOL WifiNoteFriendList_StartCodeMessage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                                HeapID heapId) {
    u64 key = WifiNoteFriendList_GetFriendKey(list, data);

    WifiNoteDraw_SetFriendName(draw, data, data->friends.index[WifiNoteFriendList_GetFriend(list)], heapId);
    if (key != 0) {
        WifiNoteDraw_SetFriendCode(draw, key);
        WifiNoteFriendList_StartMessage(list, draw, Bank0418_Text_SFriendCode_3, heapId);
    } else {
        WifiNoteFriendList_StartMessage(list, draw, Bank0418_Text_PleaseGoWiFi, heapId);
    }
    return TRUE;
}

static void WifiNoteFriendList_StartDeleteMessage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                                  HeapID heapId) {
    WifiNoteDraw_SetFriendName(draw, data, data->friends.index[WifiNoteFriendList_GetFriend(list)], heapId);
    WifiNoteFriendList_StartMessage(list, draw, Bank0418_Text_WillDeletedSureAbout, heapId);
}

static BOOL WifiNoteFriendList_OpenYesNo(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw,
                                         HeapID heapId) {
    if (!WifiNoteFriendList_IsMessageDone(list, data, draw)) {
        return FALSE;
    }
    list->menu = WifiNote_CreateYesNoMenu(list->yesNoItems, &list->menuRes, FALSE, heapId);
    return TRUE;
}

// Draws a page of the roster in an area. With area 0, the page shown, it becomes the current page
static void WifiNoteFriendList_DrawPage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, s32 page,
                                        s32 pos, u32 area, HeapID heapId) {
    if (area == 0) {
        list->page = page;
        list->pos = pos;
        WifiNoteFriendList_SetPageCursor(list, page);
    }
    WifiNoteListArea_DrawBG(&list->areas[area], draw, &draw->screens);
    WifiNoteListArea_Draw(&list->areas[area], data, draw, list->charSys, &draw->screens, &data->friends, page, heapId);
    WifiNoteFriendList_DrawCursor(list);
}

static void WifiNoteFriendList_SetPage(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw, s32 page,
                                       s32 pos, HeapID heapId) {
    list->pos = pos;
    list->page = page;
    WifiNoteFriendList_SetPageCursor(list, page);
    WifiNoteListArea_DrawBG(&list->areas[0], draw, &draw->screens);
    WifiNoteListArea_DrawFriends(&list->areas[0], data, draw, list->charSys, &draw->screens, &data->friends, page,
                                 heapId);
    WifiNoteFriendList_DrawCursor(list);
}

static void WifiNoteFriendList_ScrollFunc0(WifiNoteFriendList *list, WifiNoteDraw *draw, s16 page, s16 prevPage) {
}

static void WifiNoteFriendList_ScrollFunc2(WifiNoteFriendList *list, WifiNoteDraw *draw, s16 page, s16 prevPage) {
}

static void WifiNoteFriendList_RequestScroll(WifiNoteFriendList *list, WifiNoteData *data, u32 dir) {
    data->seq = FRIEND_LIST_SEQ_SCROLL_START;
    list->scrollDir = dir;
    list->scrollCount = 0;
}

static void WifiNoteFriendList_StartMessage(WifiNoteFriendList *list, WifiNoteDraw *draw, u32 msg, HeapID heapId) {
    StrBuf *temp;

    if (draw->stream != NULL) {
        func_020223cc(draw->stream);
        draw->stream = NULL;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(list->msgWindow.window), 15);
    temp = GFL_StrBufCreate(128, heapId);
    GFL_MsgDataLoadStrbuf(draw->msgData, msg, temp);
    GFL_WordSetFormatStrbuf(draw->wordSet, list->msgStr, temp);
    draw->stream = func_02022268(list->msgWindow.window, 0, 0, list->msgStr, draw->font, func_02017bcc(),
                                 draw->tcbManager, 10, draw->heapId, 0);
    BmpWin_DrawFrame(list->msgWindow.window, 2, 70, 11);
    WifiNote_ShowWindowNow(list->msgWindow.window);
    GFL_StrBufFree(temp);
    WifiNote_SetBrightness(FALSE);
}

static BOOL WifiNoteFriendList_IsMessageDone(WifiNoteFriendList *list, WifiNoteData *data, WifiNoteDraw *draw) {
    if (!WifiNote_UpdateStream(draw->stream, &draw->streamPaused)) {
        data->touch = FALSE;
        return TRUE;
    }
    return FALSE;
}

static void WifiNoteFriendList_ClearMessage(WifiNoteFriendList *list, WifiNoteDraw *draw) {
    BmpWin_ClearFrame(list->msgWindow.window, 1);
    WifiNoteDraw_ShowTitle(draw);
    WifiNoteFriendList_DrawCursor(list);
    WifiNote_SetBrightness(TRUE);
}

static void WifiNoteFriendList_EraseMessage(WifiNoteFriendList *list) {
    GFL_BitmapFill(BmpWin_GetBitmap(list->msgWindow.window), 15);
    BmpWin_FlushChar(list->msgWindow.window);
}

// The cursor's place among the roster's friends
static s32 WifiNoteFriendList_GetFriend(WifiNoteFriendList *list) {
    s32 friend = list->page * FRIEND_LIST_PAGE_SIZE;

    return friend + list->pos;
}

// The eraser that rubs a deleted friend out, in an OBJ window over the friend
static void WifiNoteFriendList_StartErase(WifiNoteFriendList *list, WifiNoteDraw *draw) {
    ClActorPos pos;

    func_0204c124(list->eraser, TRUE);
    func_0204c488(list->eraser, 1);
    func_0204c56c(list->eraser);
    list->eraserFrame = 0;
    func_0204c520(list->eraser, TRUE);
    pos.x = sWifiNoteFriendListPos[list->pos].x * 8;
    pos.y = sWifiNoteFriendListPos[list->pos].y * 8 - 4;
    func_0204c140(list->eraser, &pos, 0);
    G2_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2, FALSE);
    G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_BG3 |
                              GX_WND_PLANEMASK_OBJ,
                          TRUE);
    GX_SetVisibleWnd(GX_WNDMASK_OW);
    GFL_SndSEPlay(SEQ_SE_SYS_41);
}

// Returns TRUE once the eraser's animation is over
static BOOL WifiNoteFriendList_UpdateErase(WifiNoteFriendList *list, WifiNoteDraw *draw) {
    fx32 frame;

    if (!func_0204c560(list->eraser)) {
        return TRUE;
    }
    frame = func_0204c4f8(list->eraser);
    if ((u32)(frame - list->eraserFrame) >= 2) {
        list->eraserFrame = frame;
    }
    return FALSE;
}

static void WifiNoteFriendList_EndErase(WifiNoteFriendList *list, WifiNoteDraw *draw) {
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    func_0204c124(list->eraser, FALSE);
    func_0204c520(list->eraser, FALSE);
}

static void WifiNoteListArea_Init(WifiNoteListArea *area, WifiNoteDraw *draw, const WifiNoteScreenArea *screenArea,
                                  u32 charOffset, HeapID heapId) {
    area->area = *screenArea;
}

static void WifiNoteListArea_Exit(WifiNoteListArea *area, WifiNoteDraw *draw) {
}

static void WifiNoteListArea_DrawBG(WifiNoteListArea *area, WifiNoteDraw *draw, WifiNoteScreens *screens) {
    GFL_BGSysLoadScrAreaAll(2, screens->screen[0]->rawData, MATH_ABS(area->area.x), MATH_ABS(area->area.y),
                            screens->screen[0]->width / 8, screens->screen[0]->height / 8);
    GFL_BGSysQueueScrLoad(2);
}

static void WifiNoteListArea_DrawFriends(WifiNoteListArea *area, WifiNoteData *data, WifiNoteDraw *draw,
                                         WF_2DCSYS *charSys, WifiNoteScreens *screens, WifiNoteFriends *friends,
                                         s32 page, HeapID heapId) {
    int first;
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    GFLBitmap *bitmap = BmpWin_GetBitmap(area->nameWindow->window);
    BOOL printed = FALSE;
    int i;

    GFL_BitmapFill(bitmap, 0);
    WifiNoteListArea_DeleteActors(area, draw);
    first = page * FRIEND_LIST_PAGE_SIZE;
    for (i = 0; i < FRIEND_LIST_PAGE_SIZE; i++) {
        if (friends->count > first + i) {
            WifiNoteListArea_DrawFriend(area, charSys, draw, screens, i, wifiList, friends->index[first + i], heapId);
            printed = TRUE;
        } else {
            WifiNoteListArea_DrawFrame(area, draw, screens, i, heapId);
        }
    }
    if (!printed) {
        BmpWin_FlushChar(area->nameWindow->window);
    }
}

static void WifiNoteListArea_Draw(WifiNoteListArea *area, WifiNoteData *data, WifiNoteDraw *draw, WF_2DCSYS *charSys,
                                  WifiNoteScreens *screens, WifiNoteFriends *friends, s32 page, HeapID heapId) {
    WifiNoteListArea_DrawFriends(area, data, draw, charSys, screens, friends, page, heapId);
    WifiNote_ShowWindow(area->nameWindow->window);
}

static void WifiNoteListArea_Clear(WifiNoteListArea *area, WifiNoteDraw *draw, WifiNoteScreens *screens) {
    WifiNoteListArea_DeleteActors(area, draw);
    WifiNote_HideWindow(area->nameWindow->window);
    GFL_BGSysFillScrArea(2, 0, MATH_ABS(area->area.x), MATH_ABS(area->area.y), area->area.width, area->area.height, 0);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysFillScrArea(3, 0, MATH_ABS(area->area.x), MATH_ABS(area->area.y), area->area.width, area->area.height, 0);
    GFL_BGSysQueueScrLoad(3);
}

static void WifiNoteListArea_DeleteActors(WifiNoteListArea *area, WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < FRIEND_LIST_PAGE_SIZE; i++) {
        if (area->chars[i] != NULL) {
            WF_2DC_WkDel(area->chars[i]);
            area->chars[i] = NULL;
        }
        if (area->frames[i] != NULL) {
            func_0204c108(area->frames[i]);
            area->frames[i] = NULL;
        }
    }
}

// A friend's trainer, or an empty frame for a friend not met, and the name in the color of the gender
static void WifiNoteListArea_DrawFriend(WifiNoteListArea *area, WF_2DCSYS *charSys, WifiNoteDraw *draw,
                                        WifiNoteScreens *screens, s32 friendPos, WifiList *wifiList, u32 index,
                                        HeapID heapId) {
    WF_2DC_WKDATA charData;
    u16 colors[3];
    u32 view;
    u32 gender;
    StrBuf *name;
    u32 pos = friendPos;

    colors[0] = 0x14c0;
    colors[1] = 0xc80;
    colors[2] = WIFI_NOTE_COLOR_BLACK;
    view = func_02009f80(wifiList, index, WIFI_LIST_FRIEND_TRAINER_VIEW);
    gender = func_02009f80(wifiList, index, WIFI_LIST_FRIEND_GENDER);
    if (gender == WIFI_LIST_GENDER_UNKNOWN) {
        WifiNoteListArea_DrawFrame(area, draw, screens, pos, heapId);
    } else {
        charData.x = (area->area.x + sWifiNoteFriendListPos[pos].x) * 8 + 8;
        charData.y = (area->area.y + sWifiNoteFriendListPos[pos].y) * 8 + 10;
        charData.priority = 8;
        charData.bgPriority = 1;
        area->chars[pos] = WF_2DC_WkAdd(charSys, &charData, view, heapId);
        WF_2DC_WkPatAnmStart(area->chars[pos], 0, 1);
    }
    name = GFL_StrBufCreate(128, heapId);
    GFL_StrBufLoadString(name, func_0200a0f0(wifiList, index));
    PrintStrAligned(area->nameWindow, draw->queue, sWifiNoteFriendListPos[pos].x * 8,
                    sWifiNoteFriendListPos[pos].y * 8 - 24, name, draw->font, colors[gender], PRINT_ALIGN_LEFT);
    GFL_StrBufFree(name);
}

static void WifiNoteListArea_DrawFrame(WifiNoteListArea *area, WifiNoteDraw *draw, WifiNoteScreens *screens, s32 pos,
                                       HeapID heapId) {
    ClActorSetup setup;

    setup.x = (area->area.x + sWifiNoteFriendListPos[pos].x) * 8 + 16;
    setup.y = (area->area.y + sWifiNoteFriendListPos[pos].y) * 8 + 16;
    setup.sequence = 0;
    setup.priority = 8;
    setup.bgPriority = 1;
    area->frames[pos] =
        func_0204c040(draw->scrollUnit, draw->chars[0], draw->palettes[0], draw->cellAnims[0], &setup, 0, heapId);
}

// The name entry and then the code input for a new friend, or the name entry alone for a friend of the roster. The
// screen's graphics and the menus' overlay make room for them
static BOOL WifiNoteCodeIn_Main(WifiNoteCodeIn *codeIn, WifiNoteWork *wk, WifiNoteData *data, WifiNoteDraw *draw,
                                HeapID heapId) {
    switch (data->seq) {
    case CODE_IN_SEQ_NEW_INIT: {
        int blocks[CODEIN_BLOCK_COUNT];

        GFL_OvlUnload(OVERLAY_APP_UI);
        codeIn->nameParam = WifiNoteCodeIn_CreateNameParam(codeIn, data, TRUE, heapId);
        blocks[0] = 4;
        blocks[1] = 4;
        blocks[2] = 4;
        codeIn->codeParam = CodeInParam_Create(heapId, CODEIN_MODE_FRIEND_CODE, 0, 12, blocks);
        WifiNote_ExitDraw(wk);
        codeIn->procManager = CreateGameProcManager(heapId);
        QueueGameProc(codeIn->procManager, OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, codeIn->nameParam);
        data->seq = CODE_IN_SEQ_NEW_NAME;
        break;
    }
    case CODE_IN_SEQ_NEW_NAME:
        if (!GFL_ProcMgrUpdate(codeIn->procManager)) {
            if (codeIn->nameParam->unk1C == 0) {
                QueueGameProc(codeIn->procManager, OVERLAY_CODEIN, &CODEIN_PROC_FUNCTIONS, codeIn->codeParam);
                data->seq = CODE_IN_SEQ_NEW_CODE;
            } else {
                FreeGameProcManager(codeIn->procManager);
                WifiNoteData_SetNext(data, WIFI_NOTE_MODE_SELECT, MODE_SELECT_SEQ_INIT);
                data->seq = CODE_IN_SEQ_NEW_END;
            }
        }
        break;
    case CODE_IN_SEQ_NEW_CODE:
        if (!GFL_ProcMgrUpdate(codeIn->procManager)) {
            FreeGameProcManager(codeIn->procManager);
            WifiNoteData_SetEntered(data, codeIn->nameParam->name, codeIn->codeParam->str);
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_SELECT, MODE_SELECT_SEQ_CODE_INIT);
            data->seq = CODE_IN_SEQ_NEW_END;
        }
        break;
    case CODE_IN_SEQ_NEW_END:
        func_ov012_02165ae8(codeIn->nameParam);
        CodeInParam_Free(codeIn->codeParam);
        GFL_OvlLoad(OVERLAY_APP_UI);
        WifiNote_InitDraw(wk, heapId);
        return TRUE;
    case CODE_IN_SEQ_NAME_INIT:
        GFL_OvlUnload(OVERLAY_APP_UI);
        codeIn->nameParam = WifiNoteCodeIn_CreateNameParam(codeIn, data, FALSE, heapId);
        WifiNote_ExitDraw(wk);
        codeIn->procManager = CreateGameProcManager(heapId);
        QueueGameProc(codeIn->procManager, OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, codeIn->nameParam);
        data->seq = CODE_IN_SEQ_NAME;
        break;
    case CODE_IN_SEQ_NAME:
        if (!GFL_ProcMgrUpdate(codeIn->procManager)) {
            FreeGameProcManager(codeIn->procManager);
            data->touch = func_0203d554();
            if (codeIn->nameParam->unk1C == 0) {
                WifiNoteData_SetNext(data, WIFI_NOTE_MODE_FRIEND_LIST, FRIEND_LIST_SEQ_NAME_RETURN);
                WifiNoteData_SetEntered(data, codeIn->nameParam->name, NULL);
                data->seq = CODE_IN_SEQ_NAME_END;
            } else {
                WifiNoteData_SetNext(data, WIFI_NOTE_MODE_FRIEND_LIST, FRIEND_LIST_SEQ_NAME_CANCEL);
                data->seq = CODE_IN_SEQ_NAME_END;
            }
        }
        break;
    case CODE_IN_SEQ_NAME_END:
        func_ov012_02165ae8(codeIn->nameParam);
        GFL_OvlLoad(OVERLAY_APP_UI);
        WifiNote_InitDraw(wk, heapId);
        return TRUE;
    }
    return FALSE;
}

// The name entry of a friend, with the friend's trainer if the player has met them
static NameEntryParam *WifiNoteCodeIn_CreateNameParam(WifiNoteCodeIn *codeIn, WifiNoteData *data, BOOL isNew,
                                                      HeapID heapId) {
    TrainerGameInfoSave *gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(data->gameData));
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    u32 index = data->friends.index[data->friendPos];
    u32 gender = func_02009f80(wifiList, index, WIFI_LIST_FRIEND_GENDER);
    NameEntryParam *param;

    if (isNew == TRUE || gender == WIFI_LIST_GENDER_UNKNOWN) {
        param = setupNameEntry(heapId, NAME_ENTRY_FRIEND, 0xffff, 0, 7, NULL, gameInfo);
    } else {
        param = setupNameEntry(heapId, NAME_ENTRY_FRIEND, func_02009f80(wifiList, index, WIFI_LIST_FRIEND_TRAINER_VIEW),
                               0, 7, NULL, gameInfo);
    }
    if (data->hasOldName == TRUE) {
        GFL_StrBufLoadString(param->name, data->oldName);
        data->hasOldName = FALSE;
    }
    return param;
}

static void WifiNoteMyCode_Init(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    WifiNoteMyCode_InitDraw(myCode, data, draw, heapId);
}

static BOOL WifiNoteMyCode_Main(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    switch (data->seq) {
    case MY_CODE_SEQ_INIT:
        WifiNoteMyCode_Init(myCode, data, draw, HEAPID_WIFI_NOTE);
        WifiNoteMyCode_Show(myCode, data, draw);
        data->seq = MY_CODE_SEQ_MAIN;
        break;
    case MY_CODE_SEQ_MAIN: {
        s32 input = WifiNoteMyCode_Input(myCode, data);

        if (input == 0) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            WifiNoteDraw_StartReturnAnim(draw);
            data->seq = MY_CODE_SEQ_RETURN;
        } else if (input == 1) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            WifiNoteDraw_StartExitAnim(draw);
            data->seq = MY_CODE_SEQ_EXIT;
        }
        break;
    }
    case MY_CODE_SEQ_RETURN:
        if (func_0204c560(draw->returnButton) != TRUE) {
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_SELECT, MODE_SELECT_SEQ_INIT_NO_FADE);
            WifiNoteMyCode_Hide(myCode, draw);
            WifiNoteMyCode_Exit(myCode, data, draw);
            return TRUE;
        }
        break;
    case MY_CODE_SEQ_EXIT:
        if (func_0204c560(draw->exitButton) != TRUE) {
            data->seq = MY_CODE_SEQ_FADE_OUT;
        }
        break;
    case MY_CODE_SEQ_FADE_OUT:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq = MY_CODE_SEQ_FADE_OUT_WAIT;
        break;
    case MY_CODE_SEQ_FADE_OUT_WAIT:
        if (GFL_WipeIsFinished() == TRUE) {
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_EXIT, 0);
            WifiNoteMyCode_Exit(myCode, data, draw);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void WifiNoteMyCode_Exit(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw) {
    WifiNoteMyCode_ExitDraw(myCode, data, draw);
}

// Returns 0 for the return button and 1 for the X button, or -1
static s32 WifiNoteMyCode_Input(WifiNoteMyCode *myCode, WifiNoteData *data) {
    s32 hit = func_0203da0c(sWifiNoteMyCodeTouchRects);

    if (hit != TOUCH_RECT_NONE) {
        func_0203d564(TRUE);
        data->touch = TRUE;
        return hit;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        data->touch = FALSE;
        return 0;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        func_0203d564(FALSE);
        data->touch = FALSE;
        return 1;
    }
    return TOUCH_RECT_NONE;
}

// Prints a Friend Code's twelve digits from the right, one digit to a tile, in groups of four
static inline void WifiNoteMyCode_PrintCode(PrintWindow *window, StrBuf *str, u64 code, PrintQueue *queue, Font *font) {
    u16 digits[10] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9' };
    int gap = 0;
    u8 width = BmpWin_GetSizeX(window->window);
    int i;

    for (i = 1; i <= 12; i++) {
        GFL_StrBufClear(str);
        GFL_StrBufAppend(str, digits[code % 10]);
        PrintStrAligned(window, queue, (width - i) * 8 - gap + 1, 0, str, font, WIFI_NOTE_COLOR_WHITE,
                        PRINT_ALIGN_LEFT);
        if (i % 4 == 0) {
            gap += 8;
        }
        code /= 10;
    }
}

static void WifiNoteMyCode_InitDraw(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    StrBuf *str;
    StrBuf *temp;
    u64 code;

    WifiNote_CreateWindow(draw, &myCode->msgWindow, 3, 2, 10, 30, 8, 10, 1, 0);
    WifiNote_CreateWindow(draw, &myCode->codeWindow, 3, 9, 6, 14, 2, 10, 241, 0);
    str = GFL_StrBufCreate(192, heapId);
    temp = GFL_StrBufCreate(128, heapId);
    code = func_02057ec4(func_02009f7c(GameData_GetWifiList(data->gameData)));
    if (code != 0) {
        GFL_MsgDataLoadStrbuf(draw->msgData, Bank0418_Text_ByExchangingFriendCodes, str);
        PrintStrAligned(&myCode->msgWindow, draw->queue, 4, 0, str, draw->font, WIFI_NOTE_COLOR_BLACK,
                        PRINT_ALIGN_LEFT);
        WifiNote_ShowWindow(myCode->msgWindow.window);
        WifiNoteMyCode_PrintCode(&myCode->codeWindow, str, code, draw->queue, draw->font);
        WifiNote_ShowWindow(myCode->codeWindow.window);
    } else {
        GFL_MsgDataLoadStrbuf(draw->msgData, Bank0418_Text_ObtainFriendCodePlease, str);
        PrintStrAligned(&myCode->msgWindow, draw->queue, 4, 0, str, draw->font, WIFI_NOTE_COLOR_BLACK,
                        PRINT_ALIGN_LEFT);
        WifiNote_ShowWindow(myCode->msgWindow.window);
    }
    GFL_StrBufFree(str);
    GFL_StrBufFree(temp);
    myCode->screenBuffer = GFL_G2DIOReadNSCRArc(draw->arc, 6, FALSE, &myCode->screen, heapId);
}

static void WifiNoteMyCode_ExitDraw(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw) {
    WifiNoteDraw_ClearPrintWindows(draw);
    WifiNoteDraw_AddPrintWindow(draw, &draw->title);
    BmpWin_Free(myCode->msgWindow.window);
    BmpWin_Free(myCode->codeWindow.window);
    GFL_HeapFree(myCode->screenBuffer);
}

static void WifiNoteMyCode_Show(WifiNoteMyCode *myCode, WifiNoteData *data, WifiNoteDraw *draw) {
    copyVarForText(draw->wordSet, 0, GetGameDataPlayerInfo(data->gameData));
    WifiNoteDraw_PrintTitle(draw, &draw->title, Bank0418_Text_SFriendCode_2);
    WifiNote_LoadScreen(2, myCode->screen);
    GFL_BGSysQueueScrLoad(2);
}

static void WifiNoteMyCode_Hide(WifiNoteMyCode *myCode, WifiNoteDraw *draw) {
    WifiNoteDraw_HideTitle(draw);
    GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 24, 0);
    GFL_BGSysQueueScrLoad(2);
    WifiNote_HideWindow(myCode->msgWindow.window);
    WifiNote_HideWindow(myCode->codeWindow.window);
    GFL_BGSysFillScrArea(3, 0, 0, 0, 32, 24, 0);
    GFL_BGSysQueueScrLoad(3);
}

static void WifiNoteFriendInfo_Init(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    info->unk10 = 1;
    WifiNoteFriendInfo_InitDraw(info, data, draw, heapId);
}

static BOOL WifiNoteFriendInfo_Main(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    switch (data->seq) {
    case FRIEND_INFO_SEQ_INIT:
        info->moved = FALSE;
        info->page = 0;
        // fallthrough
    case FRIEND_INFO_SEQ_INIT_NEXT:
        WifiNoteFriendInfo_Init(info, data, draw, heapId);
        WifiNoteFriendInfo_Show(info, data, draw, heapId);
        data->seq = FRIEND_INFO_SEQ_MAIN;
        break;
    case FRIEND_INFO_SEQ_MAIN:
        data->seq = WifiNoteFriendInfo_Input(info, data, draw);
        break;
    case FRIEND_INFO_SEQ_SCROLL_START:
        WifiNoteFriendInfo_ScrollStart(info, data, draw, heapId);
        WifiNoteFriendInfo_Scroll(info, data, draw, heapId);
        data->seq = FRIEND_INFO_SEQ_SCROLL;
        break;
    case FRIEND_INFO_SEQ_SCROLL:
        if (WifiNoteFriendInfo_Scroll(info, data, draw, heapId) == TRUE) {
            data->seq = FRIEND_INFO_SEQ_SCROLL_END;
        }
        break;
    case FRIEND_INFO_SEQ_SCROLL_END:
        if (func_02021c0c(draw->queue) == TRUE) {
            GFL_BGSysMoveBGReq(2, 0, 0);
            GFL_BGSysMoveBGReq(3, 0, 0);
            data->seq = FRIEND_INFO_SEQ_MAIN;
        }
        break;
    case FRIEND_INFO_SEQ_CHANGE:
        // The first change is at once; holding the key repeats it every four frames
        if (info->moved) {
            info->wait--;
            if (info->wait != 0) {
                break;
            }
        }
        if (WifiNoteData_MoveFriend(data, info->scrollDir)) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            WifiNoteFriendInfo_HideForNext(info, draw);
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_FRIEND_INFO, FRIEND_INFO_SEQ_INIT_NEXT);
            info->moved = TRUE;
            WifiNoteFriendInfo_ExitDraw(info, data, draw);
            return TRUE;
        }
        data->seq = FRIEND_INFO_SEQ_MAIN;
        break;
    case FRIEND_INFO_SEQ_RETURN:
        if (func_0204c560(draw->returnButton) != TRUE) {
            WifiNoteFriendInfo_Hide(info, draw);
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_FRIEND_LIST, FRIEND_LIST_SEQ_INFO_RETURN);
            WifiNoteFriendInfo_ExitDraw(info, data, draw);
            return TRUE;
        }
        break;
    case FRIEND_INFO_SEQ_EXIT:
        if (func_0204c560(draw->exitButton) != TRUE) {
            data->seq = FRIEND_INFO_SEQ_FADE_OUT;
        }
        break;
    case FRIEND_INFO_SEQ_FADE_OUT:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, heapId);
        data->seq = FRIEND_INFO_SEQ_FADE_OUT_WAIT;
        break;
    case FRIEND_INFO_SEQ_FADE_OUT_WAIT:
        if (GFL_WipeIsFinished()) {
            WifiNoteData_SetNext(data, WIFI_NOTE_MODE_EXIT, 0);
            WifiNoteFriendInfo_ExitDraw(info, data, draw);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Returns the sequence that an input goes to, with its sound
static u32 WifiNoteFriendInfo_Decide(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, u32 input) {
    switch (input) {
    case FRIEND_INFO_INPUT_RETURN:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        WifiNoteDraw_StartReturnAnim(draw);
        return FRIEND_INFO_SEQ_RETURN;
    case FRIEND_INFO_INPUT_EXIT:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        WifiNoteDraw_StartExitAnim(draw);
        return FRIEND_INFO_SEQ_EXIT;
    case FRIEND_INFO_INPUT_PREV:
        if (WifiNoteData_CanMoveFriend(data, 0) == TRUE) {
            info->wait = 4;
            info->scrollDir = 0;
            WifiNoteDraw_StartArrowAnim(draw, 1, 11);
            return FRIEND_INFO_SEQ_CHANGE;
        }
        break;
    case FRIEND_INFO_INPUT_NEXT:
        if (WifiNoteData_CanMoveFriend(data, 1) == TRUE) {
            info->wait = 4;
            info->scrollDir = 1;
            WifiNoteDraw_StartArrowAnim(draw, 0, 10);
            return FRIEND_INFO_SEQ_CHANGE;
        }
        break;
    }
    return FRIEND_INFO_SEQ_MAIN;
}

static u32 WifiNoteFriendInfo_Keys(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw) {
    if (data->touch == TRUE) {
        if (GCTX_HIDGetPressedKeys() & WIFI_NOTE_KEYS) {
            func_0203d564(FALSE);
            data->touch = FALSE;
        }
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_RETURN);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_EXIT);
    }
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_UP) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_PREV);
    }
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_DOWN) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_NEXT);
    }
    return FRIEND_INFO_SEQ_MAIN;
}

static u32 WifiNoteFriendInfo_Touch(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, BOOL *touched) {
    s32 hit = func_0203da0c(sWifiNoteFriendInfoTouchRects);

    if (hit == TOUCH_RECT_NONE) {
        return FRIEND_INFO_SEQ_MAIN;
    }
    *touched = TRUE;
    if (hit == 0) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_NEXT);
    }
    if (hit == 1) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_PREV);
    }
    if (hit == 2) {
        return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_RETURN);
    }
    return WifiNoteFriendInfo_Decide(info, data, draw, FRIEND_INFO_INPUT_EXIT);
}

static u32 WifiNoteFriendInfo_Input(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw) {
    BOOL touched = FALSE;
    u32 seq;

    if (!func_02021c0c(draw->queue)) {
        return FRIEND_INFO_SEQ_MAIN;
    }
    seq = WifiNoteFriendInfo_Touch(info, data, draw, &touched);
    if (touched) {
        func_0203d564(TRUE);
        data->touch = TRUE;
        return seq;
    }
    return WifiNoteFriendInfo_Keys(info, data, draw);
}

static void WifiNoteFriendInfo_ScrollStart(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw,
                                           HeapID heapId) {
    u32 area;

    info->scrollCount = 0;
    WifiNoteDraw_SetArrowsAnimating(draw, FALSE);
    if (info->scrollDir == 2) {
        area = 2;
    } else {
        area = 1;
    }
    WifiNoteFriendInfo_SetPage(info, data, draw, area, info->page, heapId);
    GFL_SndSEPlay(SEQ_SE_SELECT1);
}

static BOOL WifiNoteFriendInfo_Scroll(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    s32 scroll;

    if (info->scrollCount > FRIEND_INFO_SCROLL_FRAMES) {
        WifiNoteFriendInfo_SetPage(info, data, draw, 0, info->page, heapId);
        WifiNoteDraw_SetArrowsAnimating(draw, TRUE);
        WifiNoteFriendInfo_DrawTabs(info, draw, 0xff);
        return TRUE;
    }
    scroll = info->scrollCount * 256 / FRIEND_INFO_SCROLL_FRAMES;
    if (info->scrollDir == 2) {
        scroll = -scroll;
    }
    GFL_BGSysMoveBG(3, 0, scroll);
    info->scrollCount++;
    return FALSE;
}

static void WifiNoteFriendInfo_InitDraw(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw,
                                        HeapID heapId) {
    int i;
    int j;

    WifiNoteFriendInfo_LoadScreens(info->screens, draw, heapId);
    WifiNoteFriendInfo_CreateWindows(info, draw);
    for (i = 0; i < 2; i++) {
        WifiNote_CreateWindow(draw, &info->areaWindows[i], 3, MATH_ABS(sWifiNoteScreenAreas[i].x) + 2,
                              MATH_ABS(sWifiNoteScreenAreas[i].y) + 1, 26, 18, 10, 0, 0);
    }
    for (i = 0; i < FRIEND_INFO_AREA_COUNT; i++) {
        WifiNoteInfoArea_Init(&info->areas[i], draw, &sWifiNoteScreenAreas[i], sWifiNoteFriendInfoCharOffsets[i],
                              heapId);
        for (j = 0; j < 8; j++) {
            WifiNoteInfoArea_SetWindows(&info->areas[i], draw, j, sWifiNoteFriendInfoWindowCounts[j],
                                        sWifiNoteFriendInfoRects[j], heapId);
            info->areas[0].windows[j] = &info->areaWindows[0];
            info->areas[1].windows[j] = &info->areaWindows[1];
            info->areas[2].windows[j] = &info->areaWindows[1];
        }
    }
}

static void WifiNoteFriendInfo_ExitDraw(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw) {
    int i;
    int j;

    WifiNoteDraw_ClearPrintWindows(draw);
    WifiNoteDraw_AddPrintWindow(draw, &draw->title);
    for (i = 0; i < FRIEND_INFO_AREA_COUNT; i++) {
        for (j = 0; j < 8; j++) {
            WifiNoteInfoArea_ResetWindows(&info->areas[i], j);
        }
        WifiNoteInfoArea_Exit(&info->areas[i]);
    }
    for (i = 0; i < 2; i++) {
        BmpWin_Free(info->areaWindows[i].window);
    }
    WifiNoteFriendInfo_FreeWindows(info, draw);
    WifiNoteFriendInfo_FreeScreens(info->screens);
}

static void WifiNoteFriendInfo_Show(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    WifiNoteDraw_SetFriendName(draw, data, data->friends.index[data->friendPos], heapId);
    WifiNoteDraw_PrintTitle(draw, &draw->title, Bank0418_Text_Record);
    WifiNote_LoadScreen(0, info->screens[0].screen);
    GFL_BGSysQueueScrLoad(0);
    WifiNote_LoadScreen(2, info->screens[1].screen);
    GFL_BGSysQueueScrLoad(2);
    WifiNoteFriendInfo_Draw(info, data, draw, heapId);
    WifiNoteFriendInfo_SetPage(info, data, draw, 0, info->page, heapId);
    // The arrows are gray when there is no other friend with a record
    if (WifiNoteData_CanMoveFriend(data, 0) == TRUE) {
        WifiNoteDraw_StartArrowAnim(draw, 1, 3);
        WifiNoteDraw_StartArrowAnim(draw, 0, 2);
    } else {
        WifiNoteDraw_StartArrowAnim(draw, 1, 17);
        WifiNoteDraw_StartArrowAnim(draw, 0, 16);
    }
    WifiNoteDraw_SetArrowsVisible(draw, TRUE);
}

static void WifiNoteFriendInfo_Hide(WifiNoteFriendInfo *info, WifiNoteDraw *draw) {
    WifiNoteDraw_SetArrowsVisible(draw, FALSE);
    WifiNoteFriendInfo_ClearTabs(info, draw);
    WifiNoteFriendInfo_HideWindows(info);
    GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 32, 0);
    GFL_BGSysQueueScrLoad(2);
    WifiNote_LoadScreen(0, info->screens[2].screen);
    GFL_BGSysQueueScrLoad(0);
    WifiNoteDraw_HideTitle(draw);
    WifiNoteFriendInfo_FreeTrainer(info);
}

static void WifiNoteFriendInfo_HideForNext(WifiNoteFriendInfo *info, WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < FRIEND_INFO_AREA_COUNT; i++) {
        WifiNoteInfoArea_Hide(&info->areas[i], draw);
    }
    WifiNoteDraw_HideTitle(draw);
    WifiNoteFriendInfo_FreeTrainer(info);
}

static void WifiNoteFriendInfo_Draw(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, HeapID heapId) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    u32 i;

    func_02009f80(wifiList, data->friends.index[data->friendPos], WIFI_LIST_FRIEND_GENDER);
    for (i = 0; i < FRIEND_INFO_WINDOW_COUNT; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(info->windows[i].window), 0);
    }
    PrintFraction(&info->windows[0], draw->queue, draw->font, BmpWin_GetWidth1(info->windows[0].window) * 8 / 2, 0,
                  WIFI_NOTE_COLOR_WHITE, data->friends.number[data->friendPos] + 1, data->friends.metCount, heapId);
    WifiNoteFriendInfo_Print(&info->windows[1], data, draw, Bank0418_Text_BattleRecord, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteFriendInfo_Print(&info->windows[2], data, draw, Bank0418_Text_Win, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteFriendInfo_Print(&info->windows[3], data, draw, Bank0418_Text_Lose, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteDraw_SetNumber(draw, func_02009f80(wifiList, data->friends.index[data->friendPos], WIFI_LIST_FRIEND_WINS));
    WifiNoteFriendInfo_Print(&info->windows[4], data, draw, Bank0418_Text_Empty_9, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteDraw_SetNumber(draw,
                           func_02009f80(wifiList, data->friends.index[data->friendPos], WIFI_LIST_FRIEND_LOSSES));
    WifiNoteFriendInfo_Print(&info->windows[5], data, draw, Bank0418_Text_Empty_9, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteFriendInfo_Print(&info->windows[6], data, draw, Bank0418_Text_PokemonTrades, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteDraw_SetNumber(draw, func_02009f80(wifiList, data->friends.index[data->friendPos], WIFI_LIST_FRIEND_DRAWS));
    WifiNoteFriendInfo_Print(&info->windows[7], data, draw, Bank0418_Text_Empty_10, 0, 0, WIFI_NOTE_COLOR_WHITE);
    WifiNoteFriendInfo_Print(&info->windows[8], data, draw, Bank0418_Text_DateLastPlayed, 0, 0, WIFI_NOTE_COLOR_WHITE);
    if (WifiNoteDraw_SetLastDate(draw, data, data->friends.index[data->friendPos])) {
        WifiNoteFriendInfo_Print(&info->windows[9], data, draw, Bank0418_Text_Empty_13, 0, 0, WIFI_NOTE_COLOR_WHITE);
    } else {
        BmpWin_FlushChar(info->windows[9].window);
    }
    for (i = 0; i < FRIEND_INFO_WINDOW_COUNT; i++) {
        WifiNote_ShowWindow(info->windows[i].window);
    }
    {
        ClActorSetup setup = { 56, 80, 0, 0, 0 };
        u32 trainerClass =
            func_0202b5e8(func_02009f80(wifiList, data->friends.index[data->friendPos], WIFI_LIST_FRIEND_TRAINER_VIEW));
        ArcTool *arc = MakeTrGraArcHandle(heapId);

        info->trainerChars = TrGra_LoadClActChars(arc, trainerClass, CLACT_VRAM_MAIN, heapId);
        info->trainerPalette = TrGra_LoadClActPalette(arc, trainerClass, CLACT_VRAM_MAIN, 0xc0, heapId);
        info->trainerCellAnims = TrGra_LoadClActCellAnims(trainerClass, 2, CLACT_VRAM_MAIN, heapId);
        GFL_ArcToolFree(arc);
        info->trainer = func_0204c040(draw->unit, info->trainerChars, info->trainerPalette, info->trainerCellAnims,
                                      &setup, 0, heapId);
    }
}

static void WifiNoteFriendInfo_HideWindows(WifiNoteFriendInfo *info) {
    int i;

    for (i = 0; i < FRIEND_INFO_WINDOW_COUNT; i++) {
        WifiNote_HideWindow(info->windows[i].window);
    }
}

// With area 0, the page shown, the page becomes the current one
static void WifiNoteFriendInfo_SetPage(WifiNoteFriendInfo *info, WifiNoteData *data, WifiNoteDraw *draw, u32 area,
                                       s32 page, HeapID heapId) {
    if (area == 0) {
        info->page = page;
    }
}

// Draws the row of the record's tabs, and a tab over it as the current one unless it is 0xff
static void WifiNoteFriendInfo_DrawTabs(WifiNoteFriendInfo *info, WifiNoteDraw *draw, u32 tab) {
    if (info->unk10 == 1) {
        GFL_BGSysLoadScrArea(2, 0, 20, 32, 4, draw->screens.screen[1]->rawData, 0, 12,
                             draw->screens.screen[1]->width / 8, draw->screens.screen[1]->height / 8);
    } else {
        GFL_BGSysLoadScrArea(2, 0, 20, 32, 4, draw->screens.screen[1]->rawData, 0, 20,
                             draw->screens.screen[1]->width / 8, draw->screens.screen[1]->height / 8);
    }
    if (tab == 0xff) {
        GFL_BGSysQueueScrLoad(2);
        return;
    }
    GFL_BGSysLoadScrArea(2, sWifiNoteFriendInfoTabX[info->unk10][tab], 20,
                         sWifiNoteFriendInfoTabWidth[info->unk10][tab], 4, draw->screens.screen[1]->rawData,
                         sWifiNoteFriendInfoTabX[info->unk10][tab], sWifiNoteFriendInfoTabSrcY[info->unk10],
                         draw->screens.screen[1]->width / 8, draw->screens.screen[1]->height / 8);
    GFL_BGSysQueueScrLoad(2);
}

static void WifiNoteFriendInfo_ClearTabs(WifiNoteFriendInfo *info, WifiNoteDraw *draw) {
    GFL_BGSysLoadScrArea(2, 0, 20, 32, 4, draw->screens.screen[1]->rawData, 0, 28, draw->screens.screen[1]->width / 8,
                         draw->screens.screen[1]->height / 8);
    GFL_BGSysQueueScrLoad(2);
}

// Moves to the friend with a record before (dir 0) or after the current one, if there is another
static BOOL WifiNoteData_MoveFriend(WifiNoteData *data, u32 dir) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    int pos = data->friendPos;
    u32 i;

    if (dir == 0) {
        for (i = 0; i < data->friends.count - 1; i++) {
            pos--;
            if (pos < 0) {
                pos += data->friends.count;
            }
            if (func_02009f80(wifiList, data->friends.index[pos], WIFI_LIST_FRIEND_GENDER) !=
                WIFI_LIST_GENDER_UNKNOWN) {
                data->friendPos = pos;
                return TRUE;
            }
        }
    } else {
        for (i = 0; i < data->friends.count - 1; i++) {
            pos = (pos + 1) % data->friends.count;
            if (func_02009f80(wifiList, data->friends.index[pos], WIFI_LIST_FRIEND_GENDER) !=
                WIFI_LIST_GENDER_UNKNOWN) {
                data->friendPos = pos;
                return TRUE;
            }
        }
    }
    return FALSE;
}

static BOOL WifiNoteData_CanMoveFriend(WifiNoteData *data, u32 dir) {
    WifiList *wifiList = GameData_GetWifiList(data->gameData);
    int pos = data->friendPos;
    u32 i;

    if (dir == 0) {
        for (i = 0; i < data->friends.count - 1; i++) {
            pos--;
            if (pos < 0) {
                pos += data->friends.count;
            }
            if (data->friendPos == pos) {
                break;
            }
            if (func_02009f80(wifiList, data->friends.index[pos], WIFI_LIST_FRIEND_GENDER) !=
                WIFI_LIST_GENDER_UNKNOWN) {
                return TRUE;
            }
        }
    } else {
        for (i = 0; i < data->friends.count - 1; i++) {
            pos = (pos + 1) % data->friends.count;
            if (data->friendPos == pos) {
                break;
            }
            if (func_02009f80(wifiList, data->friends.index[pos], WIFI_LIST_FRIEND_GENDER) !=
                WIFI_LIST_GENDER_UNKNOWN) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static void WifiNoteFriendInfo_LoadScreens(WifiNoteInfoScreen *screens, WifiNoteDraw *draw, HeapID heapId) {
    u16 files[FRIEND_INFO_SCREEN_COUNT] = { 10, 11, 5, 12 };
    int i;

    for (i = 0; i < FRIEND_INFO_SCREEN_COUNT; i++) {
        screens->buffer = GFL_G2DIOReadNSCRArc(draw->arc, files[i], FALSE, &screens->screen, heapId);
        screens++;
    }
}

static void WifiNoteFriendInfo_FreeScreens(WifiNoteInfoScreen *screens) {
    int i;

    for (i = 0; i < FRIEND_INFO_SCREEN_COUNT; i++) {
        GFL_HeapFree(screens->buffer);
        screens++;
    }
}

static void WifiNoteFriendInfo_CreateWindows(WifiNoteFriendInfo *info, WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < FRIEND_INFO_WINDOW_COUNT; i++) {
        WifiNote_CreateWindow(draw, &info->windows[i], 1, sWifiNoteFriendInfoWindowRects[i].x,
                              sWifiNoteFriendInfoWindowRects[i].y, sWifiNoteFriendInfoWindowRects[i].width,
                              sWifiNoteFriendInfoWindowRects[i].height, 10, 0, 0);
    }
}

static void WifiNoteFriendInfo_FreeWindows(WifiNoteFriendInfo *info, WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < FRIEND_INFO_WINDOW_COUNT; i++) {
        BmpWin_Free(info->windows[i].window);
    }
}

static void WifiNoteInfoArea_Init(WifiNoteInfoArea *area, WifiNoteDraw *draw, const WifiNoteScreenArea *screenArea,
                                  u32 charOffset, HeapID heapId) {
    int i;

    area->area = *screenArea;
    area->charOffset = charOffset;
    for (i = 0; i < 8; i++) {
        area->windowCounts[i] = 0;
    }
}

static void WifiNoteInfoArea_Exit(WifiNoteInfoArea *area) {
}

static void WifiNoteInfoArea_SetWindows(WifiNoteInfoArea *area, WifiNoteDraw *draw, u32 index, u32 count,
                                        const WifiNoteWindowRect *rects, HeapID heapId) {
    area->windowCounts[index] = count;
}

static void WifiNoteInfoArea_ResetWindows(WifiNoteInfoArea *area, u32 index) {
    if (area->windows[index] != NULL) {
        u8 i;

        for (i = 0; i < area->windowCounts[index]; i++) {
        }
        area->windows[index] = NULL;
        area->windowCounts[index] = 0;
    }
}

static void WifiNoteInfoArea_HideWindows(WifiNoteInfoArea *area, u32 index) {
    u32 i;

    for (i = 0; i < area->windowCounts[index]; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(area->windows[index]->window), 0);
        WifiNote_HideWindow(area->windows[index]->window);
    }
}

static void WifiNoteInfoArea_Hide(WifiNoteInfoArea *area, WifiNoteDraw *draw) {
    int i;

    for (i = 0; i < 8; i++) {
        WifiNoteInfoArea_HideWindows(area, i);
    }
    WifiNote_ClearScreenArea(draw, &area->area);
}

static void WifiNote_ClearScreenArea(WifiNoteDraw *draw, const WifiNoteScreenArea *area) {
    GFL_BGSysFillScrArea(3, 0, MATH_ABS(area->x), MATH_ABS(area->y), 32, 24, 0);
    GFL_BGSysQueueScrLoad(3);
}

static void WifiNoteFriendInfo_Print(PrintWindow *window, WifiNoteData *data, WifiNoteDraw *draw, u32 msg, u32 x, u32 y,
                                     u32 color) {
    GFL_MsgDataLoadStrbuf(draw->msgData, msg, draw->temp);
    GFL_WordSetFormatStrbuf(draw->wordSet, draw->str, draw->temp);
    PrintStrAligned(window, draw->queue, x, y, draw->str, draw->font, color, PRINT_ALIGN_LEFT);
}

// Reads a Friend Code of ASCII or full-width digits. valid is whether the whole string was read
static u64 WifiNote_StrToFriendKey(const StrBuf *str, BOOL *valid) {
    u16 terminator = GFL_StrBufGetTerminator();
    const u16 *chars = GFL_StrBufGetStringPtr(str);
    u64 key = 0;
    u16 i;

    *valid = FALSE;
    for (i = 0; chars[i] != terminator; i++) {
        u32 digit;

        if (i >= sWifiNoteFriendKeyDigitsMax[0]) {
            return key;
        }
        digit = (u16)(chars[i] - L'0');
        if (digit >= 10) {
            digit = (u16)(chars[i] - 0xff10);
            if (digit >= 10) {
                return key;
            }
        }
        key *= 10;
        key += digit;
    }
    *valid = TRUE;
    return key;
}

// Whether the player touched the screen or pressed A or B to go on
static BOOL WifiNote_IsNextPressed(WifiNoteData *data) {
    if (func_0203da48() == TRUE) {
        func_0203d564(TRUE);
        data->touch = TRUE;
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        func_0203d564(FALSE);
        data->touch = FALSE;
        return TRUE;
    }
    return FALSE;
}

static void WifiNoteDraw_InitBlink(WifiNoteDraw *draw, HeapID heapId) {
    draw->blink[0] = BlinkPalAnm_Create(0x30, 16, 0, heapId);
    BlinkPalAnm_SetPalBufferArcTool(draw->blink[0], draw->arc, 0, 0x20, 0x30);
    draw->blink[1] = BlinkPalAnm_Create(0x10, 16, 0xfffe, heapId);
    BlinkPalAnm_SetPalBufferArcTool(draw->blink[1], draw->arc, 2, 0, 0x10);
}

static void WifiNoteDraw_ExitBlink(WifiNoteDraw *draw) {
    BlinkPalAnm_Free(draw->blink[1]);
    BlinkPalAnm_Free(draw->blink[0]);
}

static void WifiNoteModeSelect_SetButtonPalette(u32 button, u32 palette) {
    GFL_BGSysSetScrPaletteNo(2, 2, (button + 1) * 5, 28, 4, palette);
    GFL_BGSysQueueScrLoad(2);
}

static void WifiNoteModeSelect_ClearCursor(WifiNoteDraw *draw, u32 button) {
    WifiNoteModeSelect_SetButtonPalette(button, 1);
    BlinkPalAnm_InitAnime(draw->blink[0]);
}

static void WifiNoteModeSelect_SetCursor(WifiNoteDraw *draw, u32 button) {
    WifiNoteModeSelect_SetButtonPalette(button, 3);
    BlinkPalAnm_InitAnime(draw->blink[0]);
}

static void WifiNoteModeSelect_MoveCursor(WifiNoteDraw *draw, u32 button, u32 prevButton) {
    WifiNoteModeSelect_ClearCursor(draw, prevButton);
    WifiNoteModeSelect_SetCursor(draw, button);
}

// No button has the cursor while the player uses the touch screen
static void WifiNoteModeSelect_DrawCursor(WifiNoteModeSelect *select) {
    u32 i;

    if (func_0203d554() == TRUE) {
        for (i = 0; i < 3; i++) {
            WifiNoteModeSelect_SetButtonPalette(i, 1);
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (i == select->cursor) {
                WifiNoteModeSelect_SetButtonPalette(i, 3);
            } else {
                WifiNoteModeSelect_SetButtonPalette(i, 1);
            }
        }
    }
}

// Flashes the picked button twice. Returns FALSE once it is over
static BOOL WifiNoteModeSelect_UpdateButton(WifiNoteModeSelect *select) {
    switch (select->buttonSeq) {
    case 0:
        WifiNoteModeSelect_SetButtonPalette(select->cursor, 1);
        select->buttonCount = 0;
        select->buttonSeq++;
        break;
    case 1:
        select->buttonCount++;
        if (select->buttonCount == 4) {
            WifiNoteModeSelect_SetButtonPalette(select->cursor, 2);
            select->buttonCount = 0;
            select->buttonSeq++;
        }
        break;
    case 2:
        select->buttonCount++;
        if (select->buttonCount == 4) {
            WifiNoteModeSelect_SetButtonPalette(select->cursor, 1);
            select->buttonCount = 0;
            select->buttonSeq++;
        }
        break;
    case 3:
        select->buttonCount++;
        if (select->buttonCount == 4) {
            WifiNoteModeSelect_SetButtonPalette(select->cursor, 2);
            select->buttonCount = 0;
            select->buttonSeq++;
        }
        break;
    case 4:
        select->buttonCount++;
        if (select->buttonCount == 4) {
            select->buttonSeq = 0;
            select->buttonCount = 0;
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static void WifiNoteFriendList_SetButtonPalette(u32 pos, u32 palette) {
    GFL_BGSysSetScrPaletteNo(2, pos / 4 * 14 + 2, pos % 4 * 4 + 4, 14, 4, palette);
    GFL_BGSysQueueScrLoad(2);
}

static void WifiNoteFriendList_ClearCursor(WifiNoteDraw *draw, u32 pos) {
    WifiNoteFriendList_SetButtonPalette(pos, 1);
    BlinkPalAnm_InitAnime(draw->blink[0]);
}

static void WifiNoteFriendList_SetCursor(WifiNoteDraw *draw, u32 pos) {
    WifiNoteFriendList_SetButtonPalette(pos, 3);
    BlinkPalAnm_InitAnime(draw->blink[0]);
}

static void WifiNoteFriendList_MoveCursor(WifiNoteDraw *draw, u32 pos, u32 prevPos) {
    WifiNoteFriendList_ClearCursor(draw, prevPos);
    WifiNoteFriendList_SetCursor(draw, pos);
}

static void WifiNoteFriendList_DrawCursor(WifiNoteFriendList *list) {
    u32 i;

    if (func_0203d554() == TRUE) {
        for (i = 0; i < FRIEND_LIST_PAGE_SIZE; i++) {
            WifiNoteFriendList_SetButtonPalette(i, 1);
        }
    } else {
        for (i = 0; i < FRIEND_LIST_PAGE_SIZE; i++) {
            if (i == list->pos) {
                WifiNoteFriendList_SetButtonPalette(i, 3);
            } else {
                WifiNoteFriendList_SetButtonPalette(i, 1);
            }
        }
    }
}

static BOOL WifiNoteFriendList_UpdateButton(WifiNoteFriendList *list) {
    switch (list->buttonSeq) {
    case 0:
        WifiNoteFriendList_SetButtonPalette(list->pos, 1);
        list->buttonCount = 0;
        list->buttonSeq++;
        break;
    case 1:
        list->buttonCount++;
        if (list->buttonCount == 4) {
            WifiNoteFriendList_SetButtonPalette(list->pos, 2);
            list->buttonCount = 0;
            list->buttonSeq++;
        }
        break;
    case 2:
        list->buttonCount++;
        if (list->buttonCount == 4) {
            WifiNoteFriendList_SetButtonPalette(list->pos, 1);
            list->buttonCount = 0;
            list->buttonSeq++;
        }
        break;
    case 3:
        list->buttonCount++;
        if (list->buttonCount == 4) {
            WifiNoteFriendList_SetButtonPalette(list->pos, 2);
            list->buttonCount = 0;
            list->buttonSeq++;
        }
        break;
    case 4:
        list->buttonCount++;
        if (list->buttonCount == 4) {
            list->buttonSeq = 0;
            list->buttonCount = 0;
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static void WifiNoteDraw_StartReturnAnim(WifiNoteDraw *draw) {
    func_0204c4d4(draw->returnButton, 0);
    func_0204c488(draw->returnButton, 9);
    func_0204c520(draw->returnButton, TRUE);
}

static void WifiNoteDraw_StartExitAnim(WifiNoteDraw *draw) {
    func_0204c4d4(draw->exitButton, 0);
    func_0204c488(draw->exitButton, 8);
    func_0204c520(draw->exitButton, TRUE);
}

// Darkens the screen behind a message, or shows it as it is
static void WifiNote_SetBrightness(BOOL normal) {
    if (normal == TRUE) {
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR,
                                 GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, 0);
    } else {
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR,
                                 GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ, -8);
    }
}

static void WifiNoteFriendList_SetPageCursor(WifiNoteFriendList *list, s32 page) {
    func_0204c4d4(list->pageCursor, 0);
    func_0204c488(list->pageCursor, page + 2);
}

static void WifiNoteDraw_ShowTitle(WifiNoteDraw *draw) {
    BmpWin_FlushMap(draw->title.window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(draw->title.window));
}

static void WifiNoteDraw_ClearPrintWindows(WifiNoteDraw *draw) {
    u32 i;

    for (i = 0; i < WIFI_NOTE_PRINT_WINDOW_MAX; i++) {
        draw->windows[i] = NULL;
    }
}

static void WifiNoteDraw_AddPrintWindow(WifiNoteDraw *draw, PrintWindow *window) {
    u32 i;

    for (i = 0; i < WIFI_NOTE_PRINT_WINDOW_MAX; i++) {
        if (draw->windows[i] == NULL) {
            draw->windows[i] = window;
            return;
        }
    }
}

static void WifiNoteDraw_FlushPrintWindows(WifiNoteDraw *draw) {
    u32 i;

    for (i = 0; i < WIFI_NOTE_PRINT_WINDOW_MAX; i++) {
        if (draw->windows[i] == NULL) {
            break;
        }
        PrintWindow_Flush(draw->windows[i], draw->queue);
    }
}

static void WifiNote_ShowWindow(BmpWin *window) {
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

static void WifiNoteFriendInfo_FreeTrainer(WifiNoteFriendInfo *info) {
    func_0204b98c(info->trainerChars);
    func_0204bcd0(info->trainerPalette);
    func_0204be64(info->trainerCellAnims);
    func_0204c108(info->trainer);
}
