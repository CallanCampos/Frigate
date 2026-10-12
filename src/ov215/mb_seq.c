#include "app/mailbox.h"
#include "types.h"
#include "app/pokelist.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/mailsys.h"
#include "field/mailtool.h"
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
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bgwinfrm.h"
#include "system/bmp_winframe.h"
#include "system/cursor_move.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms_data.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_2.h"

// The mailbox screen: the 20 mails of the save's mailbox, ten to a page, on the touch screen's list, and the menu that
// reads, erases or gives one to a Pokémon. A state machine, one function per state, with the party list and the
// mail writing screen run as child processes. Names are ours, guessed

// The heaps: the screen's state, and the data of the screen
#define HEAPID_MAILBOX_PROC 0x65
#define HEAPID_MAILBOX_VIEW 0x66

// The archive with the mailbox's graphics
#define ARCID_MAILBOX 0x99

// The message file of the mailbox's texts, which the battle server flow's second file holds
#define MAILBOX_MSG_FILE TEXT_BANK_BTL_SERVER_FLOW_2

#define MAILBOX_PAGE_SIZE 10

// The windows of the screen's view
#define MAILBOX_WINDOW_COUNT 32
#define MAILBOX_WIN_FIRST_MAIL 2
#define MAILBOX_WIN_SELECTED_MAIL 22
#define MAILBOX_WIN_MESSAGE 25
#define MAILBOX_WIN_TITLE 26
#define MAILBOX_WIN_SENTENCE_1 27
#define MAILBOX_WIN_STAMP 30
#define MAILBOX_WIN_AUTHOR 31

// The frames of BGWinFrame, one per mail of the page and a last for the mail picked
#define MAILBOX_FRAME_SELECTED 10

// The cell actors of the view
#define MAILBOX_ACTOR_COUNT 8
#define MAILBOX_ACTOR_PREV_PAGE 0
#define MAILBOX_ACTOR_NEXT_PAGE 1
#define MAILBOX_ACTOR_CURSOR 2
#define MAILBOX_ACTOR_RETURN_BUTTON 3
#define MAILBOX_ACTOR_CLOSE_BUTTON 7

// The states
enum {
    MAILBOX_STATE_INIT,
    MAILBOX_STATE_EXIT,
    MAILBOX_STATE_WAIT_WIPE,
    MAILBOX_STATE_WAIT_MESSAGE,
    MAILBOX_STATE_WAIT_YES_NO,
    MAILBOX_STATE_WAIT_PALETTE_FX,
    MAILBOX_STATE_START_CHILD,
    MAILBOX_STATE_WAIT_CHILD,
    MAILBOX_STATE_WIPE_IN,
    MAILBOX_STATE_SELECT_MAIL,
    MAILBOX_STATE_WIPE_OUT,
    MAILBOX_STATE_CHANGE_PAGE,
    MAILBOX_STATE_OPEN_MENU,
    MAILBOX_STATE_CREATE_MENU,
    MAILBOX_STATE_WAIT_MENU,
    MAILBOX_STATE_READ_MAIL,
    MAILBOX_STATE_CLOSE_MAIL,
    MAILBOX_STATE_ASK_ERASE,
    MAILBOX_STATE_ERASE_NOTICE,
    MAILBOX_STATE_ASK_GIVE,
    MAILBOX_STATE_PARTY_RETURN,
    MAILBOX_STATE_GIVE_FADE_IN,
    MAILBOX_STATE_GIVE_RESULT,
    MAILBOX_STATE_MAIL_WRITTEN,
    MAILBOX_STATE_PRESS_RETURN,
    MAILBOX_STATE_PRESS_CLOSE,
    MAILBOX_STATE_BACK_TO_LIST,
    MAILBOX_STATE_DONE,
};

// What the state machine runs as child processes
enum {
    MAILBOX_CHILD_PARTY_GIVE,
    MAILBOX_CHILD_PARTY_ASK,
    MAILBOX_CHILD_MAIL_SYS,
    MAILBOX_CHILD_PARTY_PICK,
    MAILBOX_CHILD_COUNT,
};

// The messages of the question that follows picking a mail's use
#define MAILBOX_MESSAGE_ERASE 0
#define MAILBOX_MESSAGE_ERASED 1
#define MAILBOX_MESSAGE_ASK_HOLD 2
#define MAILBOX_MESSAGE_HOLD_DONE 3
#define MAILBOX_MESSAGE_BAG_FULL 4

typedef struct MailBoxView MailBoxView;

typedef struct {
    // The state machine's work. nextState is where a wipe, a message or a child process goes on to
    MailData *mails;
    MailboxProcessData *param;
    MailBoxView *view;
    u32 unk0C;
    void *childParam;
    u32 childProc;
    Font *font;
    u16 page;
    u8 cursor;
    u8 mailIndex;
    u8 partyResult;
    u8 partyPos : 7;
    u8 bagFull : 1;
    u16 item;
    BOOL mailGiven;
    u32 nextState;
    TCB *vblankTask;
    GameProcManager *procManager;
} MailBoxWork;

typedef u32 (*MailBoxStateFunc)(MailBoxWork *work);

// A window, and whether a string is being printed into it
typedef struct {
    BmpWin *window;
    u8 printing;
} MailBoxPrintState;

// A palette change of an actor or of a part of a BG's screen, in steps
typedef struct {
    u8 onScreen : 1;
    u8 target : 7;
    u8 firstPalette : 4;
    u8 secondPalette : 4;
    u8 phase;
    u8 timer;
    s8 x;
    s8 y;
    u8 width;
    u8 height;
} MailBoxPaletteFx;

struct MailBoxView {
    BGWinFrame *frames;
    u8 tileA[0x20];
    u8 tileB[0x20];
    BmpWin *windows[MAILBOX_WINDOW_COUNT];
    MailBoxPrintState printStates[MAILBOX_WINDOW_COUNT];
    PrintQueue *printQueue;
    PrintStream *stream;
    TCBExManager *tcbManager;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    u8 unk1DC;
    u8 waitForInput;
    u32 yesNoIndex;
    u32 unk1E4;
    u32 unk1E8;
    ClActUnit *clActUnit;
    ClActor *actors[MAILBOX_ACTOR_COUNT];
    u32 chars[8];
    u32 palettes[8];
    u32 cellAnims[8];
    u32 unk270[8];
    ArcTool *iconArc;
    CursorMove *cursor;
    MailData *mails[MAIL_COUNT];
    u8 pageMails[MAIL_COUNT];
    u16 mailCount;
    u16 maxPage;
    MailBoxPaletteFx paletteFx;
    u32 wipeNextState;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    AppTaskMenuItem menuItems[4];
    AppTaskMenuItem yesNoItems[2];
    PMSDraw *pmsDraw;
    PMSData stampSentence;
    ClActUnit *pmsClActUnit;
    PrintQueue *pmsPrintQueue;
};

typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 baseChar;
} MailBoxWindowTemplate;

typedef struct {
    s16 x;
    s16 y;
    s16 unk4;
    u16 sequence;
    u32 unk8;
    u32 unkC;
    u32 surface;
    u32 charIndex;
    u32 paletteIndex;
    u32 cellAnimIndex;
    u32 unk20;
    u32 unk24;
    u32 unk28;
    u32 bgPriority;
    u32 unk30;
} MailBoxActorTemplate;

typedef struct {
    u32 messageId;
    u32 type;
} MailBoxMenuItem;

// What a yes/no question does for each answer
typedef struct {
    MailBoxStateFunc yes;
    MailBoxStateFunc no;
} MailBoxYesNo;

typedef struct {
    MailBoxStateFunc start;
    MailBoxStateFunc end;
    u32 nextState;
} MailBoxChildProc;

static BOOL MailBox_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MailBox_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MailBox_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void MailBox_VBlank(TCB *tcb, void *data);
static void MailBox_SetVRAMBanks(void);
static void MailBox_InitBGs(MailBoxWork *work);
static void MailBox_FreeBGs(MailBoxWork *work);
static void MailBox_LoadBGGraphics(MailBoxWork *work);
static void MailBox_LoadSystemPalettes(MailBoxWork *work);
static void MailBox_CreateFrames(MailBoxWork *work);
static void MailBox_FreeFrames(MailBoxWork *work);
static void MailBox_LoadMessages(MailBoxView *view);
static void MailBox_FreeMessages(MailBoxView *view);
static void MailBox_CreateYesNoMenu(MailBoxWork *work);
static void MailBox_CreateMenuResources(MailBoxWork *work);
static void MailBox_FreeMenuResources(MailBoxWork *work);
static void MailBox_CreateMainMenu(MailBoxWork *work);
static void MailBox_FreeMenu(MailBoxWork *work);
static void MailBox_LoadMails(MailBoxWork *work);
static void MailBox_FreeMails(MailBoxWork *work);
static void MailBox_UpdateMailList(MailBoxWork *work);
static void MailBox_ShowSelectedFrame(MailBoxWork *work);
static BOOL MailBox_UpdatePaletteFx(MailBoxWork *work);
static void MailBox_LoadMailGraphics(MailBoxWork *work);
static void MailBox_CreatePMSDraw(MailBoxWork *work);
static void MailBox_FreePMSDraw(MailBoxWork *work);
static void MailBox_CreateWindows(MailBoxWork *work);
static void MailBox_FreeWindows(MailBoxWork *work);
static void MailBox_PrintString(BmpWin *window, StrBuf *str, MailBoxPrintState *state, PrintQueue *queue, int x, int y,
                                Font *font, u16 color, u32 align);
static void MailBox_PrintMessageId(MailBoxView *view, MsgData *msgData, u32 index, u32 messageId, int x, int y,
                                   Font *font, u16 color, u32 align);
static void MailBox_DrawMailFrame(MailBoxWork *work, u32 frame, u32 windowIndex, u32 mailIndex);
static void MailBox_PrintTitle(MailBoxWork *work);
static void MailBox_Nop1(MailBoxView *view);
static void MailBox_Nop2(MailBoxView *view);
static void MailBox_RegisterAuthorName(MailBoxWork *work, int unused);
static void MailBox_PrintMessage(MailBoxWork *work);
static void MailBox_PrintWhatWouldYouLike(MailBoxWork *work);
static void MailBox_ShowMessageWindow(MailBoxWork *work);
static void MailBox_HideMessageWindow(MailBoxWork *work);
static void MailBox_SetMessage(MailBoxWork *work, u32 message);
static void MailBox_PrintMail(MailBoxWork *work);
static void MailBox_CreateOBJs(MailBoxWork *work);
static void MailBox_FreeOBJs(MailBoxWork *work);
static void MailBox_UpdateClActUnit(MailBoxView *view);
static void MailBox_InitClAct(MailBoxView *view);
static void MailBox_FreeClAct(MailBoxView *view);
static ClActor *MailBox_CreateActor(MailBoxView *view, const MailBoxActorTemplate *template);
static void MailBox_FreeActor(MailBoxView *view, u32 index);
static void MailBox_FreeActors(MailBoxView *view);
static void MailBox_SetActorAnim(MailBoxView *view, u32 index, u32 sequence);
static void MailBox_SetActorVisible(MailBoxView *view, u32 index, BOOL visible);
static void MailBox_SetActorPos(MailBoxView *view, u32 index, int x, int y);
static void MailBox_SetActorBlend(MailBoxView *view, u32 index, BOOL blend);
static void MailBox_OpenIconArc(MailBoxView *view);
static void MailBox_CloseIconArc(MailBoxView *view);
static void MailBox_LoadActorResources(MailBoxView *view);
static void MailBox_LoadButtonResources(MailBoxView *view);
static void MailBox_ShowReadCursor(MailBoxView *view);
static BOOL MailBox_Update(MailBoxWork *work, u32 *state);
static u32 MailBox_StateInit(MailBoxWork *work);
static u32 MailBox_StateExit(MailBoxWork *work);
static u32 MailBox_StateWaitWipe(MailBoxWork *work);
static u32 MailBox_StateWaitMessage(MailBoxWork *work);
static u32 MailBox_StateWaitYesNo(MailBoxWork *work);
static u32 MailBox_StateWaitPaletteFx(MailBoxWork *work);
static u32 MailBox_StateStartChild(MailBoxWork *work);
static u32 MailBox_StateWaitChild(MailBoxWork *work);
static u32 MailBox_StateWipeIn(MailBoxWork *work);
static u32 MailBox_StateSelectMail(MailBoxWork *work);
static u32 MailBox_StateWipeOut(MailBoxWork *work);
static u32 MailBox_StateChangePage(MailBoxWork *work);
static u32 MailBox_StateOpenMenu(MailBoxWork *work);
static u32 MailBox_StateCreateMenu(MailBoxWork *work);
static u32 MailBox_StateWaitMenu(MailBoxWork *work);
static u32 MailBox_StateReadMail(MailBoxWork *work);
static u32 MailBox_StateCloseMail(MailBoxWork *work);
static u32 MailBox_StateAskErase(MailBoxWork *work);
static u32 MailBox_StateEraseNotice(MailBoxWork *work);
static u32 MailBox_StateAskGive(MailBoxWork *work);
static u32 MailBox_StatePartyReturn(MailBoxWork *work);
static u32 MailBox_StateGiveFadeIn(MailBoxWork *work);
static u32 MailBox_StateGiveResult(MailBoxWork *work);
static u32 MailBox_StateMailWritten(MailBoxWork *work);
static u32 MailBox_StatePressReturn(MailBoxWork *work);
static u32 MailBox_StatePressClose(MailBoxWork *work);
static u32 MailBox_StateBackToList(MailBoxWork *work);
static u32 MailBox_StartWipeIn(MailBoxWork *work, u32 nextState);
static u32 MailBox_StartWipeOut(MailBoxWork *work, u32 nextState);
static u32 MailBox_StartMessage(MailBoxWork *work, u8 waitForInput, u32 nextState);
static u32 MailBox_StartYesNo(MailBoxWork *work, u32 index);
static u32 MailBox_StartFrameFx(MailBoxWork *work, u32 frame, u32 nextState);
static u32 MailBox_StartPageFx(MailBoxWork *work, u32 direction, u32 nextState);
static void MailBox_SetDimmed(MailBoxView *view, BOOL dimmed);
static void MailBox_SetDimmedForReading(MailBoxView *view);
static void MailBox_ShowMail(MailBoxWork *work);
static void MailBox_ClearMail(MailBoxWork *work);
static void MailBox_TakeMailItem(MailBoxWork *work);
static void MailBox_EraseMail(MailBoxWork *work);
static void MailBox_CreatePartyParam(MailBoxWork *work, u32 mode, u16 item, u16 mailIndex);
static u32 MailBox_StartPartyList(MailBoxWork *work);
static u32 MailBox_EndPartyListCancel(MailBoxWork *work);
static u32 MailBox_EndPartyListPick(MailBoxWork *work);
static u32 MailBox_EndPartyListCancelAsk(MailBoxWork *work);
static u32 MailBox_StartMailSys(MailBoxWork *work);
static u32 MailBox_EndMailSys(MailBoxWork *work);
static u32 MailBox_EraseYes(MailBoxWork *work);
static u32 MailBox_GiveYes(MailBoxWork *work);
static u32 MailBox_GiveNo(MailBoxWork *work);
static s32 MailBox_GetTouch(MailBoxWork *work);
static void MailBox_CreateCursor(MailBoxWork *work);
static void MailBox_FreeCursor(MailBoxWork *work);
static void MailBox_CursorOn(void *work, int pos, int prevPos);
static void MailBox_CursorOff(void *work, int pos, int prevPos);
static void MailBox_CursorMoved(void *work, int pos, int prevPos);
static void MailBox_CursorTouched(void *work, int pos, int prevPos);
static void MailBox_UpdateCursor(MailBoxWork *work, u32 pos);
static BOOL MailBox_IsReturnPressed(void);
static BOOL MailBox_IsCloseKeyPressed(void);

const GameProcFunctions MAILBOX_PROC_FUNCTIONS = { MailBox_Init, MailBox_Main, MailBox_Exit };

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_B,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const MailBoxWindowTemplate sWindowTemplates[MAILBOX_WINDOW_COUNT] = {
    { 2, 0, 24, 1, 1, 1, 0x3f0 },   { 6, 0, 24, 1, 1, 1, 0x3f0 },   { 2, 4, 1, 8, 2, 1, 0x3f0 },
    { 2, 4, 1, 8, 2, 1, 0x3e0 },    { 2, 4, 1, 8, 2, 1, 0x3d0 },    { 2, 4, 1, 8, 2, 1, 0x3c0 },
    { 2, 4, 1, 8, 2, 1, 0x3b0 },    { 2, 4, 1, 8, 2, 1, 0x3a0 },    { 2, 4, 1, 8, 2, 1, 0x390 },
    { 2, 4, 1, 8, 2, 1, 0x380 },    { 2, 4, 1, 8, 2, 1, 0x370 },    { 2, 4, 1, 8, 2, 1, 0x360 },
    { 2, 4, 1, 8, 2, 1, 0x350 },    { 2, 4, 1, 8, 2, 1, 0x340 },    { 2, 4, 1, 8, 2, 1, 0x330 },
    { 2, 4, 1, 8, 2, 1, 0x320 },    { 2, 4, 1, 8, 2, 1, 0x310 },    { 2, 4, 1, 8, 2, 1, 0x300 },
    { 2, 4, 1, 8, 2, 1, 0x2f0 },    { 2, 4, 1, 8, 2, 1, 0x2e0 },    { 2, 4, 1, 8, 2, 1, 0x2d0 },
    { 2, 4, 1, 8, 2, 1, 0x2c0 },    { 1, 4, 1, 8, 2, 1, 0x3f0 },    { 1, 25, 21, 6, 2, 15, 0x3d8 },
    { 1, 10, 22, 5, 2, 15, 0x3ce }, { 0, 2, 1, 27, 4, 14, 0x1 },    { 6, 1, 1, 13, 2, 15, 0x1 },
    { 4, 3, 3, 26, 4, 1, 0x1 },     { 4, 3, 8, 26, 4, 1, 0x69 },    { 4, 3, 13, 26, 4, 1, 0xd1 },
    { 4, 8, 19, 11, 2, 1, 0x139 },  { 4, 21, 19, 8, 2, 1, 0x139 },
};

static const ClActSysSetup sClActSetup = { 0, 0, 0, 512, 4, 64, 4, 64, 0, 6, 4, 4, 32, 16, 16 };

static const MailBoxActorTemplate sActorTemplates[MAILBOX_ACTOR_COUNT] = {
    { 8, 168, 0, 4, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },   { 32, 168, 0, 5, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },
    { 64, 16, 0, 5, 0, 1, 0, 3, 1, 1, 0, 0, 0, 1, 0 },   { 224, 168, 0, 1, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },
    { 128, 160, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 }, { 88, 160, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0 },
    { 48, 160, 0, 0, 0, 0, 1, 2, 0, 0, 0, 0, 0, 1, 0 },  { 188, 168, 0, 0, 0, 0, 0, 4, 2, 2, 0, 0, 0, 3, 0 },
};

static const GameProcFunctions sMailSysProc = { MailSys_ProcInit, MailSys_ProcMain, MailSys_ProcExit };

static const MailBoxYesNo sYesNoFuncs[2] = {
    { MailBox_EraseYes, MailBox_StateBackToList },
    { MailBox_GiveYes, MailBox_GiveNo },
};

static const MailBoxChildProc sChildProcs[MAILBOX_CHILD_COUNT] = {
    { MailBox_StartPartyList, MailBox_EndPartyListCancel, MAILBOX_STATE_INIT },
    { MailBox_StartPartyList, MailBox_EndPartyListPick, MAILBOX_STATE_PARTY_RETURN },
    { MailBox_StartMailSys, MailBox_EndMailSys, MAILBOX_STATE_MAIL_WRITTEN },
    { MailBox_StartPartyList, MailBox_EndPartyListCancelAsk, MAILBOX_STATE_INIT },
};

static const MailBoxStateFunc sStateFuncs[] = {
    MailBox_StateInit,       MailBox_StateExit,         MailBox_StateWaitWipe,    MailBox_StateWaitMessage,
    MailBox_StateWaitYesNo,  MailBox_StateWaitPaletteFx, MailBox_StateStartChild, MailBox_StateWaitChild,
    MailBox_StateWipeIn,     MailBox_StateSelectMail,   MailBox_StateWipeOut,     MailBox_StateChangePage,
    MailBox_StateOpenMenu,   MailBox_StateCreateMenu,   MailBox_StateWaitMenu,    MailBox_StateReadMail,
    MailBox_StateCloseMail,  MailBox_StateAskErase,     MailBox_StateEraseNotice, MailBox_StateAskGive,
    MailBox_StatePartyReturn, MailBox_StateGiveFadeIn,  MailBox_StateGiveResult,  MailBox_StateMailWritten,
    MailBox_StatePressReturn, MailBox_StatePressClose,  MailBox_StateBackToList,
};

// The touch screen's page and return buttons, and the return button of the mail
static const TouchRect sButtonRects[] = {
    { 168, 191, 8, 32 },
    { 168, 191, 32, 56 },
    { 168, 191, 232, 255 },
    { 168, 191, 194, 231 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sReturnRects[] = {
    { 160, 191, 230, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const CursorMoveCallbacks sCursorCallbacks = {
    MailBox_CursorOn,
    MailBox_CursorOff,
    MailBox_CursorMoved,
    MailBox_CursorTouched,
};

static const CursorMoveData sCursorData[] = {
    { 64, 16, 0, 0, 0, 2, 0, 1, { 0x0, 0x1f, 0x8, 0x77 } },
    { 192, 16, 0, 0, 1, 3, 0, 1, { 0x0, 0x1f, 0x88, 0xf4 } },
    { 64, 48, 0, 0, 0, 4, 2, 3, { 0x20, 0x3f, 0x8, 0x77 } },
    { 192, 48, 0, 0, 1, 5, 2, 3, { 0x20, 0x3f, 0x88, 0xf4 } },
    { 64, 80, 0, 0, 2, 6, 4, 5, { 0x40, 0x5f, 0x8, 0x77 } },
    { 192, 80, 0, 0, 3, 7, 4, 5, { 0x40, 0x5f, 0x88, 0xf4 } },
    { 64, 112, 0, 0, 4, 8, 6, 7, { 0x60, 0x7f, 0x8, 0x77 } },
    { 192, 112, 0, 0, 5, 9, 6, 7, { 0x60, 0x7f, 0x88, 0xf4 } },
    { 64, 144, 0, 0, 6, 8, 8, 9, { 0x80, 0x9f, 0x8, 0x77 } },
    { 192, 144, 0, 0, 7, 9, 8, 9, { 0x80, 0x9f, 0x88, 0xf4 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, { TOUCH_RECT_END, 0, 0, 0 } },
};

static BOOL MailBox_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MailBoxWork *mailbox;
    MailboxProcessData *data = param;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MAILBOX_PROC, 0x10000);
    mailbox = GFL_ProcInitSubsystem(proc, sizeof(MailBoxWork), HEAPID_MAILBOX_PROC);
    sys_memset32_fast(0, mailbox, sizeof(MailBoxWork));
    mailbox->param = data;
    mailbox->mails = func_02009790(data->gameData);
    mailbox->nextState = MAILBOX_STATE_WIPE_IN;
    mailbox->vblankTask = NULL;
    mailbox->procManager = CreateGameProcManager(HEAPID_MAILBOX_PROC);
    return TRUE;
}

static BOOL MailBox_Main(GameProc *proc, u32 *state, void *param, void *work) {
    if (!MailBox_Update(work, state)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MailBox_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MailBoxWork *mailbox = work;

    FreeGameProcManager(mailbox->procManager);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapStatusValidate(HEAPID_MAILBOX_PROC);
    GFL_HeapDelete(HEAPID_MAILBOX_PROC);
    return TRUE;
}

static void MailBox_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    func_0204b7c8();
}

static void MailBox_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
}

static void MailBox_InitBGs(MailBoxWork *work) {
    GFL_BGSysCreate(HEAPID_MAILBOX_VIEW);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x18000),
                       0x4000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(4, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearCharCore(4, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000),
                       0x4000, GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(5, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
        GFL_BGSysClearCharCore(5, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x0),
                       0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(6, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
        GFL_BGSysClearCharCore(6, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe000), GX_BG_CHARBASE(0x8000),
                       0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(7, &bg, BGMODE_TEXT);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x18000),
                       0x4000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(0, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysClearCharCore(0, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000),
                       0x4000, GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(1, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
        GFL_BGSysClearCharCore(1, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x0),
                       0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(2, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe000), GX_BG_CHARBASE(0x0),
                       0x4000, GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(3, &bg, BGMODE_TEXT);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MailBox_FreeBGs(MailBoxWork *work) {
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysFree();
}

static void MailBox_LoadBGGraphics(MailBoxWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAILBOX, HEAPID_MAILBOX_VIEW);
    NNSG2dCharacterData *character;
    void *file;
    u8 *chars;

    GFL_BGSysLoadArcNCGRStatic(arc, 42, 7, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 41, 7, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_G2DIOLoadArcNCLRDefault(arc, 43, PALTYPE_SUB_BG, 0, 0, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadArcNCGRStatic(arc, 38, 3, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadArcNCGRStatic(arc, 38, 1, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 37, 3, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_G2DIOLoadArcNCLRDefault(arc, 39, PALTYPE_MAIN_BG, 0, 0, HEAPID_MAILBOX_VIEW);
    file = GFL_G2DIOReadBGNCGRArc(arc, 38, TRUE, &character, HEAPID_MAILBOX_VIEW);
    chars = character->rawData;
    sys_memcpy32_fast(chars + 0x100, work->view->tileA, 0x20);
    sys_memcpy32_fast(chars + 0x100, work->view->tileB, 0x20);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

static void MailBox_LoadSystemPalettes(MailBoxWork *work) {
    LoadSysMsgBox(0, 0x3e2, 13, 0, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 7 * 0x40, 0x20, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 7 * 0x40 + 0x20, 0x20, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 7 * 0x40 + 0x20, 0x20, HEAPID_MAILBOX_VIEW);
}

static void MailBox_CreateFrames(MailBoxWork *work) {
    u32 i;
    u16 *screen;

    work->view->frames = BGWinFrame_Create(2, 11, HEAPID_MAILBOX_VIEW);
    for (i = 0; i < MAILBOX_PAGE_SIZE; i++) {
        BGWinFrame_InitFrame(work->view->frames, i, 2, 14, 4);
    }
    BGWinFrame_InitFrame(work->view->frames, MAILBOX_FRAME_SELECTED, 1, 14, 4);
    BGWinFrame_LoadScreen(work->view->frames, 0, ARCID_MAILBOX, 40, TRUE);
    screen = BGWinFrame_GetScreen(work->view->frames, 0);
    for (i = 1; i <= MAILBOX_FRAME_SELECTED; i++) {
        sys_memcpy(screen, BGWinFrame_GetScreen(work->view->frames, i), 14 * 4 * sizeof(u16));
    }
    for (i = 0; i < MAILBOX_PAGE_SIZE; i++) {
        BGWinFrame_Put(work->view->frames, i, (i % 2) * 16 + 1, (i / 2) * 4);
    }
}

static void MailBox_FreeFrames(MailBoxWork *work) {
    BGWinFrame_Delete(work->view->frames);
}

static void MailBox_LoadMessages(MailBoxView *view) {
    view->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MAILBOX_MSG_FILE, HEAPID_MAILBOX_VIEW);
    view->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_MAILBOX_VIEW);
    view->strbuf = GFL_StrBufCreate(0x400, HEAPID_MAILBOX_VIEW);
}

static void MailBox_FreeMessages(MailBoxView *view) {
    GFL_StrBufFree(view->strbuf);
    GFL_WordSetSystemFree(view->wordSet);
    GFL_MsgDataFree(view->msgData);
}

static void MailBox_CreateYesNoMenu(MailBoxWork *work) {
    AppTaskMenuInit init;

    init.heapId = HEAPID_MAILBOX_VIEW;
    init.itemCount = 2;
    init.items = work->view->yesNoItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 21;
    init.y = 8;
    init.width = 10;
    init.height = 3;
    work->view->menu = AppTaskMenu_Create(&init, work->view->menuRes);
}

static void MailBox_CreateMenuResources(MailBoxWork *work) {
    static const MailBoxMenuItem yesNoItems[2] = {
        { BtlServerFlow2_Text_Yes, 0 },
        { BtlServerFlow2_Text_No, 0 },
    };
    static const MailBoxMenuItem menuItems[4] = {
        { BtlServerFlow2_Text_Read, 0 },
        { BtlServerFlow2_Text_Erase, 0 },
        { BtlServerFlow2_Text_Give, 0 },
        { BtlServerFlow2_Text_Cancel_2, APP_TASKMENU_ITEM_RETURN },
    };
    u16 i;

    work->view->menuRes = AppTaskMenuRes_Create(0, 5, work->font, work->view->printQueue, HEAPID_MAILBOX_VIEW);
    for (i = 0; i < 4; i++) {
        work->view->menuItems[i].str = GFL_MsgDataLoadStrbufNew(work->view->msgData, menuItems[i].messageId);
        work->view->menuItems[i].color = PRINT_COLOR(14, 15, 3);
        work->view->menuItems[i].type = menuItems[i].type;
    }
    for (i = 0; i < 2; i++) {
        work->view->yesNoItems[i].str = GFL_MsgDataLoadStrbufNew(work->view->msgData, yesNoItems[i].messageId);
        work->view->yesNoItems[i].color = PRINT_COLOR(14, 15, 3);
        work->view->yesNoItems[i].type = yesNoItems[i].type;
    }
}

static void MailBox_FreeMenuResources(MailBoxWork *work) {
    int i;

    for (i = 0; i < 4; i++) {
        GFL_StrBufFree(work->view->menuItems[i].str);
    }
    GFL_StrBufFree(work->view->yesNoItems[0].str);
    GFL_StrBufFree(work->view->yesNoItems[1].str);
    AppTaskMenuRes_Free(work->view->menuRes);
}

static void MailBox_CreateMainMenu(MailBoxWork *work) {
    AppTaskMenuInit init;

    init.heapId = HEAPID_MAILBOX_VIEW;
    init.itemCount = 4;
    init.items = work->view->menuItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 20;
    init.y = 12;
    init.width = 12;
    init.height = 3;
    work->view->menu = AppTaskMenu_Create(&init, work->view->menuRes);
}

static void MailBox_FreeMenu(MailBoxWork *work) {
    AppTaskMenu_Free(work->view->menu);
}

static void MailBox_LoadMails(MailBoxWork *work) {
    u32 i;

    work->view->mailCount = 0;
    for (i = 0; i < MAIL_COUNT; i++) {
        work->view->pageMails[i] = 0xff;
        work->view->mails[i] = func_020097f4(work->mails, 0, i, HEAPID_MAILBOX_VIEW);
        if (func_020096a8(work->view->mails[i]) == TRUE) {
            work->view->pageMails[work->view->mailCount] = i;
            work->view->mailCount++;
        }
    }
    if (work->view->mailCount == 0) {
        work->view->maxPage = 0;
    } else {
        work->view->maxPage = (work->view->mailCount - 1) / MAILBOX_PAGE_SIZE;
    }
}

static void MailBox_FreeMails(MailBoxWork *work) {
    u32 i;

    for (i = 0; i < MAIL_COUNT; i++) {
        GFL_HeapFree(work->view->mails[i]);
    }
}

static void MailBox_UpdateMailList(MailBoxWork *work) {
    u16 first = MAILBOX_PAGE_SIZE * work->page;
    u8 *mails = &work->view->pageMails[first];
    u16 window = first + MAILBOX_WIN_FIRST_MAIL;
    u16 frame = 0;
    u16 i;

    for (i = 0; i < MAILBOX_PAGE_SIZE; i++) {
        if (mails[i] == 0xff) {
            BGWinFrame_Hide(work->view->frames, frame);
        } else {
            MailBox_DrawMailFrame(work, frame, window, mails[i]);
            BGWinFrame_Show(work->view->frames, frame);
        }
        window++;
        frame++;
    }
}

static void MailBox_ShowSelectedFrame(MailBoxWork *work) {
    MailBox_DrawMailFrame(work, MAILBOX_FRAME_SELECTED, MAILBOX_WIN_SELECTED_MAIL, work->mailIndex);
    BGWinFrame_Put(work->view->frames, MAILBOX_FRAME_SELECTED, 4, 7);
}

static BOOL MailBox_UpdatePaletteFx(MailBoxWork *work) {
    MailBoxPaletteFx *fx = &work->view->paletteFx;

    switch (fx->phase) {
    case 0:
        if (!fx->onScreen) {
            func_0204c378(work->view->actors[fx->target], fx->firstPalette, TRUE);
        } else {
            GFL_BGSysSetScrPaletteNo(fx->target, fx->x, fx->y, fx->width, fx->height, fx->firstPalette);
            GFL_BGSysQueueScrLoad(fx->target);
        }
        fx->phase++;
        break;
    case 1:
        fx->timer++;
        if (fx->timer == 4) {
            if (!fx->onScreen) {
                func_0204c378(work->view->actors[fx->target], fx->secondPalette, TRUE);
            } else {
                GFL_BGSysSetScrPaletteNo(fx->target, fx->x, fx->y, fx->width, fx->height, fx->secondPalette);
                GFL_BGSysQueueScrLoad(fx->target);
            }
            fx->timer = 0;
            fx->phase++;
        }
        break;
    case 2:
        fx->timer++;
        if (fx->timer == 2) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static void MailBox_LoadMailGraphics(MailBoxWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAILBOX, HEAPID_MAILBOX_VIEW);
    u8 design = func_02009750(work->view->mails[work->mailIndex]);

    GFL_BGSysLoadArcNCGRStatic(arc, design + 12, 5, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_G2DIOLoadArcNCLRDefault(arc, design, PALTYPE_SUB_BG, 0, 0x40, HEAPID_MAILBOX_VIEW);
    loadBGScrToVramByFileNoReserveNegAlign(arc, design + 24, 5, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
}

static void MailBox_CreatePMSDraw(MailBoxWork *work) {
    work->view->pmsPrintQueue = func_02021998(HEAPID_MAILBOX_VIEW);
    work->view->pmsClActUnit = func_0204bf1c(8, 2, HEAPID_MAILBOX_VIEW);
    work->view->pmsDraw = PMSDraw_Create(work->view->pmsClActUnit, CLACT_VRAM_SUB, work->view->pmsPrintQueue, work->font,
                                         4, 4, HEAPID_MAILBOX_VIEW);
    PMSDraw_SetBackColor(work->view->pmsDraw, 0);
}

static void MailBox_FreePMSDraw(MailBoxWork *work) {
    PMSDraw_Delete(work->view->pmsDraw);
    func_02021a18(work->view->pmsPrintQueue);
    func_0204bf98(work->view->pmsClActUnit);
}

static void MailBox_CreateWindows(MailBoxWork *work) {
    u32 i;
    BmpWin *window;

    BmpWin_InitAllocator(HEAPID_MAILBOX_VIEW);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_MAILBOX_VIEW);
    work->view->tcbManager = GFL_TCBExMgrCreate(HEAPID_MAILBOX_VIEW, HEAPID_MAILBOX_VIEW, 32, 32);
    work->view->printQueue = func_02021998(HEAPID_MAILBOX_VIEW);
    for (i = 0; i < MAILBOX_WINDOW_COUNT; i++) {
        const MailBoxWindowTemplate *template = &sWindowTemplates[i];

        MailBoxPrintState *printStates;

        work->view->windows[i] = BmpWin_CreateDynamic(template->bg, template->x, template->y, template->width,
                                                       template->height, template->palette, TRUE);
        printStates = work->view->printStates;
        printStates[i].window = work->view->windows[i];
        printStates[i].printing = FALSE;
    }
    for (i = 0; i < MAILBOX_WINDOW_COUNT - 10; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->view->windows[i]), 4);
        BmpWin_FlushChar(work->view->windows[i]);
    }
    window = work->view->windows[0];
    BmpWin_TransferNow(window);
    window = work->view->windows[1];
    BmpWin_TransferNow(window);
}

static void MailBox_FreeWindows(MailBoxWork *work) {
    u32 i;

    func_02021a18(work->view->printQueue);
    GFL_TCBExMgrFree(work->view->tcbManager);
    for (i = 0; i < MAILBOX_WINDOW_COUNT; i++) {
        BmpWin_Free(work->view->windows[i]);
    }
    GFL_FontFree(work->font);
    BmpWin_FreeAllocator();
}

#define MAILBOX_ALIGN_LEFT 0
#define MAILBOX_ALIGN_RIGHT 1
#define MAILBOX_ALIGN_CENTER 2

// Starts printing a string into the window the state belongs to
static void MailBox_PrintString(BmpWin *window, StrBuf *str, MailBoxPrintState *state, PrintQueue *queue, int x, int y,
                                Font *font, u16 color, u32 align) {
    if (align == MAILBOX_ALIGN_RIGHT) {
        x -= GFL_FontGetBlockWidth(str, font, 0);
    } else if (align == MAILBOX_ALIGN_CENTER) {
        x -= GFL_FontGetBlockWidth(str, font, 0) / 2;
    }
    func_02021c7c(queue, BmpWin_GetBitmap(state->window), x, y, str, font, color);
    state->printing = TRUE;
}

static void MailBox_PrintMessageId(MailBoxView *view, MsgData *msgData, u32 index, u32 messageId, int x, int y,
                                   Font *font, u16 color, u32 align) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, messageId);

    MailBox_PrintString(view->windows[index], str, &view->printStates[index], view->printQueue, x, y, font, color,
                        align);
    GFL_StrBufFree(str);
}

static void MailBox_DrawMailFrame(MailBoxWork *work, u32 frame, u32 windowIndex, u32 mailIndex) {
    MailData *mail;
    GFLBitmap *tileA;
    GFLBitmap *tileB;
    BmpWin *window;
    MailBoxPrintState *printState;
    u32 i;

    tileA = GFL_BitmapWrap(work->view->tileA, 1, 1, 0x20, HEAPID_MAILBOX_VIEW);
    tileB = GFL_BitmapWrap(work->view->tileB, 1, 1, 0x20, HEAPID_MAILBOX_VIEW);
    window = work->view->windows[windowIndex];
    mail = work->view->mails[mailIndex];
    printState = &work->view->printStates[windowIndex];
    GFL_BitmapFill(tileA, 4);
    GFL_BitmapFill(tileB, 4);
    GFL_StrBufLoadString(work->view->strbuf, func_02009734(mail));
    for (i = 0; i < 8; i++) {
        GFL_BitmapCopyArea(BmpWin_GetBitmap(window), tileA, 0, 0, i * 8, 0, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
        GFL_BitmapCopyArea(BmpWin_GetBitmap(window), tileB, 0, 0, i * 8, 8, 8, 8, GFL_BITMAP_NO_COLOR_KEY);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(window), 4);
    if (func_0200974c(mail) == 0) {
        MailBox_PrintString(window, work->view->strbuf, printState, work->view->printQueue, 0, 0, work->font,
                            PRINT_COLOR(5, 6, 4), MAILBOX_ALIGN_LEFT);
    } else {
        MailBox_PrintString(window, work->view->strbuf, printState, work->view->printQueue, 0, 0, work->font,
                            PRINT_COLOR(7, 8, 4), MAILBOX_ALIGN_LEFT);
    }
    BGWinFrame_WriteBmpWin(work->view->frames, frame, window);
    GFL_BitmapFree(tileB);
    GFL_BitmapFree(tileA);
}

static void MailBox_PrintTitle(MailBoxWork *work) {
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(work->view->windows[MAILBOX_WIN_TITLE]), 0);
    MailBox_PrintMessageId(work->view, work->view->msgData, MAILBOX_WIN_TITLE, BtlServerFlow2_Text_Mailbox, 52, 0,
                           work->font, PRINT_COLOR(1, 2, 0), MAILBOX_ALIGN_CENTER);
    window = work->view->windows[MAILBOX_WIN_TITLE];
    BmpWin_Transfer(window);
}

static void MailBox_Nop1(MailBoxView *view) {
}

static void MailBox_Nop2(MailBoxView *view) {
}

static void MailBox_RegisterAuthorName(MailBoxWork *work, int unused) {
    MailData *mail = work->view->mails[work->mailIndex];
    StrBuf *name = GFL_StrBufCreate(16, HEAPID_MAILBOX_VIEW);

    GFL_StrBufLoadString(name, func_02009734(mail));
    func_0202437c(work->view->wordSet, 0, name, func_0200974c(mail), 1, 2);
    GFL_StrBufFree(name);
}

static void MailBox_PrintMessage(MailBoxWork *work) {
    BmpWin *window = work->view->windows[MAILBOX_WIN_MESSAGE];

    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    work->view->stream = func_02022268(window, 0, 0, work->view->strbuf, work->font, func_02017bcc(),
                                       work->view->tcbManager, 0, HEAPID_MAILBOX_VIEW, 15);
}

static void MailBox_PrintWhatWouldYouLike(MailBoxWork *work) {
    StrBuf *str;

    MailBox_RegisterAuthorName(work, 0);
    str = GFL_MsgDataLoadStrbufNew(work->view->msgData, BtlServerFlow2_Text_WhatWouldLikeS);
    GFL_WordSetFormatStrbuf(work->view->wordSet, work->view->strbuf, str);
    GFL_StrBufFree(str);
    MailBox_PrintMessage(work);
}

static void MailBox_ShowMessageWindow(MailBoxWork *work) {
    BmpWin *window;

    BmpWin_DrawFrame(work->view->windows[MAILBOX_WIN_MESSAGE], 2, 0x3e2, 13);
    window = work->view->windows[MAILBOX_WIN_MESSAGE];
    BmpWin_Transfer(window);
}

static void MailBox_HideMessageWindow(MailBoxWork *work) {
    BmpWin_ClearFrame(work->view->windows[MAILBOX_WIN_MESSAGE], 2);
    GFL_BGSysQueueScrLoad(0);
}

static void MailBox_SetMessage(MailBoxWork *work, u32 message) {
    switch (message) {
    case MAILBOX_MESSAGE_ERASE:
        GFL_MsgDataLoadStrbuf(work->view->msgData, BtlServerFlow2_Text_MessageWillLostOk, work->view->strbuf);
        break;
    case MAILBOX_MESSAGE_ERASED:
        GFL_MsgDataLoadStrbuf(work->view->msgData, BtlServerFlow2_Text_MessageErased, work->view->strbuf);
        break;
    case MAILBOX_MESSAGE_ASK_HOLD:
        GFL_MsgDataLoadStrbuf(work->view->msgData, BtlServerFlow2_Text_LetPokemonHoldMail, work->view->strbuf);
        break;
    case MAILBOX_MESSAGE_HOLD_DONE:
        GFL_MsgDataLoadStrbuf(work->view->msgData, BtlServerFlow2_Text_MailsMessageErasedPut, work->view->strbuf);
        break;
    case MAILBOX_MESSAGE_BAG_FULL:
        GFL_MsgDataLoadStrbuf(work->view->msgData, BtlServerFlow2_Text_BagFullMailDiscarded, work->view->strbuf);
        break;
    }
    MailBox_PrintMessage(work);
}

static void MailBox_PrintMail(MailBoxWork *work) {
    u32 i;
    BmpWin *window;

    for (i = 0; i < 3; i++) {
        PMSData *sentence = func_0200976c(work->view->mails[work->mailIndex], i);

        if (PMSData_IsNotEmpty(sentence)) {
            PMSDraw_Print(work->view->pmsDraw, work->view->windows[MAILBOX_WIN_SENTENCE_1 + i], sentence, i);
            BmpWin_Transfer(work->view->windows[MAILBOX_WIN_SENTENCE_1 + i]);
        }
    }
    work->view->stampSentence.type = 6;
    work->view->stampSentence.id = 0;
    work->view->stampSentence.words[0] = func_02009764(work->view->mails[work->mailIndex]);
    if (work->view->stampSentence.words[0] != 0) {
        PMSDraw_Print(work->view->pmsDraw, work->view->windows[MAILBOX_WIN_STAMP], &work->view->stampSentence, 3);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(work->view->windows[MAILBOX_WIN_AUTHOR]), 0);
    GFL_StrBufLoadString(work->view->strbuf, func_02009734(work->view->mails[work->mailIndex]));
    MailBox_PrintString(work->view->windows[MAILBOX_WIN_AUTHOR], work->view->strbuf,
                        &work->view->printStates[MAILBOX_WIN_AUTHOR], work->view->printQueue, 0, 2, work->font,
                        PRINT_COLOR(1, 2, 0), MAILBOX_ALIGN_LEFT);
    window = work->view->windows[MAILBOX_WIN_AUTHOR];
    BmpWin_Transfer(window);
}

static void MailBox_CreateOBJs(MailBoxWork *work) {
    u32 i;

    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    for (i = 0; i < 8; i++) {
        work->view->chars[i] = 0;
        work->view->palettes[i] = 0;
        work->view->cellAnims[i] = 0;
    }
    MailBox_OpenIconArc(work->view);
    MailBox_InitClAct(work->view);
    MailBox_LoadButtonResources(work->view);
    MailBox_LoadActorResources(work->view);
    for (i = 0; i < 8; i++) {
        work->view->actors[i] = MailBox_CreateActor(work->view, &sActorTemplates[i]);
        func_0204c520(work->view->actors[i], TRUE);
    }
    if (work->view->mailCount <= MAILBOX_PAGE_SIZE) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    } else if (work->page == 0) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
    } else {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    }
    MailBox_SetActorVisible(work->view, 4, FALSE);
    MailBox_SetActorVisible(work->view, 5, FALSE);
    MailBox_SetActorVisible(work->view, 6, FALSE);
}

static void MailBox_FreeOBJs(MailBoxWork *work) {
    MailBox_FreeActors(work->view);
    MailBox_FreeClAct(work->view);
    MailBox_CloseIconArc(work->view);
}

static void MailBox_UpdateClActUnit(MailBoxView *view) {
    if (view->clActUnit != NULL) {
        func_0204b794();
    }
}

static void MailBox_InitClAct(MailBoxView *view) {
    ClActSys_Create(&sClActSetup, &sVRAMConfig, HEAPID_MAILBOX_VIEW);
    view->clActUnit = func_0204bf1c(8, 0, HEAPID_MAILBOX_VIEW);
    func_0204c028(view->clActUnit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MailBox_FreeClAct(MailBoxView *view) {
    // BUG: the palette and the cell animations are freed with each other's function. Both are resource 0, so it
    // works
#ifdef BUGFIX
    func_0204be64(view->cellAnims[0]);
    func_0204bcd0(view->palettes[0]);
#else
    func_0204be64(view->palettes[0]);
    func_0204bcd0(view->cellAnims[0]);
#endif
    func_0204bf98(view->clActUnit);
    func_0204b758();
}

static ClActor *MailBox_CreateActor(MailBoxView *view, const MailBoxActorTemplate *template) {
    ClActorSetup setup;

    setup.x = template->x;
    setup.y = template->y;
    setup.sequence = template->sequence;
    setup.priority = 0;
    setup.bgPriority = template->bgPriority;
    return func_0204c040(view->clActUnit, view->chars[template->charIndex], view->palettes[template->paletteIndex],
                         view->cellAnims[template->cellAnimIndex], &setup, template->surface,
                         HEAPID_MAILBOX_VIEW);
}

static void MailBox_FreeActor(MailBoxView *view, u32 index) {
    if (view->actors[index] != NULL) {
        func_0204c108(view->actors[index]);
        view->actors[index] = NULL;
    }
}

static void MailBox_FreeActors(MailBoxView *view) {
    u32 i;

    for (i = 0; i < MAILBOX_ACTOR_COUNT; i++) {
        MailBox_FreeActor(view, i);
    }
}

static void MailBox_SetActorAnim(MailBoxView *view, u32 index, u32 sequence) {
    func_0204c488(view->actors[index], sequence);
}

static void MailBox_SetActorVisible(MailBoxView *view, u32 index, BOOL visible) {
    func_0204c124(view->actors[index], visible);
}

static void MailBox_SetActorPos(MailBoxView *view, u32 index, int x, int y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    if (y < 256) {
        func_0204c140(view->actors[index], &pos, 0);
    } else {
        func_0204c140(view->actors[index], &pos, 1);
    }
    func_0204c4d4(view->actors[index], 0);
}

static void MailBox_SetActorBlend(MailBoxView *view, u32 index, BOOL blend) {
    if (blend == TRUE) {
        func_0204c318(view->actors[index], GX_OAM_MODE_XLU);
    } else {
        func_0204c318(view->actors[index], GX_OAM_MODE_NORMAL);
    }
}

static void MailBox_OpenIconArc(MailBoxView *view) {
    view->iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_MAILBOX_VIEW);
}

static void MailBox_CloseIconArc(MailBoxView *view) {
    GFL_ArcToolFree(view->iconArc);
}

static void MailBox_LoadActorResources(MailBoxView *view) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_MAILBOX_VIEW);
    u32 i;

    for (i = 0; i <= 2; i++) {
        view->chars[i] = func_0204b81c(arc, func_0202d894(), FALSE, TRUE, HEAPID_MAILBOX_VIEW);
    }
    view->palettes[0] = func_0204bba0(arc, PokeIcon_GetPaletteFile(), 1, 0, HEAPID_MAILBOX_VIEW);
    view->cellAnims[0] = func_0204bde0(arc, func_0202d898(0), func_0202d89c(0), HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
}

static void MailBox_LoadButtonResources(MailBoxView *view) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAILBOX, HEAPID_MAILBOX_VIEW);

    view->chars[3] = func_0204b81c(arc, 44, TRUE, 0, HEAPID_MAILBOX_VIEW);
    view->palettes[1] = func_0204bba0(arc, 47, 0, 0, HEAPID_MAILBOX_VIEW);
    view->cellAnims[1] = func_0204bde0(arc, 45, 46, HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_MAILBOX_VIEW);
    view->chars[4] = func_0204b81c(arc, func_0202d814(), FALSE, 0, HEAPID_MAILBOX_VIEW);
    view->palettes[2] = func_0204bba0(arc, func_0202d810(), 0, 0x80, HEAPID_MAILBOX_VIEW);
    view->cellAnims[2] = func_0204bde0(arc, func_0202d818(0), func_0202d81c(0), HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
}

static void MailBox_ShowReadCursor(MailBoxView *view) {
    MailBox_SetActorPos(view, MAILBOX_ACTOR_CURSOR, 224, 176);
    MailBox_SetActorAnim(view, MAILBOX_ACTOR_CURSOR, 4);
}

static BOOL MailBox_Update(MailBoxWork *work, u32 *state) {
    *state = sStateFuncs[*state](work);
    if (*state == MAILBOX_STATE_DONE) {
        return FALSE;
    }
    if (work->view != NULL) {
        BGWinFrame_UpdateMoves(work->view->frames);
        MailBox_UpdateClActUnit(work->view);
        if (work->view->printQueue != NULL) {
            int i;

            func_02021a3c(work->view->printQueue);
            for (i = 0; i < MAILBOX_WIN_SENTENCE_1; i++) {
                PrintQueue *queue = work->view->printQueue;
                MailBoxPrintState *printState = &work->view->printStates[i];

                if (printState->printing) {
                    if (func_02021c1c(queue, BmpWin_GetBitmap(printState->window)) == FALSE) {
                        BmpWin_FlushChar(printState->window);
                        printState->printing = FALSE;
                    }
                }
            }
        }
        if (work->view->tcbManager != NULL) {
            GFL_TCBExMgrUpdate(work->view->tcbManager);
        }
        if (work->view->pmsPrintQueue != NULL) {
            int i;

            PMSDraw_Main(work->view->pmsDraw);
            func_02021a3c(work->view->pmsPrintQueue);
            for (i = MAILBOX_WIN_SENTENCE_1; i < MAILBOX_WINDOW_COUNT; i++) {
                PrintQueue *queue = work->view->printQueue;
                MailBoxPrintState *printState = &work->view->printStates[i];

                if (printState->printing) {
                    if (func_02021c1c(queue, BmpWin_GetBitmap(printState->window)) == FALSE) {
                        BmpWin_FlushChar(printState->window);
                        printState->printing = FALSE;
                    }
                }
            }
        }
    }
    return TRUE;
}

static u32 MailBox_StateInit(MailBoxWork *work) {
    if (work->vblankTask != NULL) {
        GFL_TCBRemove(work->vblankTask);
        work->vblankTask = NULL;
    }
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MAILBOX_VIEW, 0x60000);
    work->view = GFL_HeapAllocate(HEAPID_MAILBOX_VIEW, sizeof(MailBoxView), FALSE, "mb_seq.c", 268);
    sys_memset32_fast(0, work->view, sizeof(MailBoxView));
    MailBox_LoadMails(work);
    MailBox_SetVRAMBanks();
    MailBox_InitBGs(work);
    MailBox_LoadBGGraphics(work);
    MailBox_LoadSystemPalettes(work);
    MailBox_CreateWindows(work);
    MailBox_CreateFrames(work);
    MailBox_CreateOBJs(work);
    MailBox_CreatePMSDraw(work);
    MailBox_CreateCursor(work);
    MailBox_LoadMessages(work->view);
    MailBox_PrintTitle(work);
    MailBox_UpdateMailList(work);
    MailBox_CreateMenuResources(work);
    work->vblankTask = GFL_VBlankTCBAdd(MailBox_VBlank, work, 1);
    func_02042ba8(FALSE, HEAPID_MAILBOX_VIEW);
    return work->nextState;
}

static u32 MailBox_StateExit(MailBoxWork *work) {
    if (work->vblankTask != NULL) {
        GFL_TCBRemove(work->vblankTask);
        work->vblankTask = NULL;
    }
    MailBox_FreeMenuResources(work);
    MailBox_FreeMessages(work->view);
    MailBox_FreeCursor(work);
    MailBox_FreePMSDraw(work);
    MailBox_FreeOBJs(work);
    MailBox_FreeFrames(work);
    MailBox_FreeWindows(work);
    MailBox_FreeBGs(work);
    MailBox_FreeMails(work);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_HeapFree(work->view);
    work->view = NULL;
    GFL_HeapDelete(HEAPID_MAILBOX_VIEW);
    return work->nextState;
}

static u32 MailBox_StateWaitWipe(MailBoxWork *work) {
    if (GFL_WipeIsFinished() == TRUE) {
        return work->view->wipeNextState;
    }
    return MAILBOX_STATE_WAIT_WIPE;
}

static u32 MailBox_StateWaitMessage(MailBoxWork *work) {
    if (func_020223b4(work->view->stream) == PRINT_STREAM_DONE) {
        if (!work->view->waitForInput) {
            func_020223cc(work->view->stream);
            return work->nextState;
        }
        if (func_0203da48() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            func_020223cc(work->view->stream);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return work->nextState;
        }
    }
    return MAILBOX_STATE_WAIT_MESSAGE;
}

static u32 MailBox_StateWaitYesNo(MailBoxWork *work) {
    AppTaskMenu_Update(work->view->menu);
    if (!AppTaskMenu_IsFlashFinished(work->view->menu)) {
        return MAILBOX_STATE_WAIT_YES_NO;
    }
    switch (AppTaskMenu_GetCursorPos(work->view->menu)) {
    case 0:
        AppTaskMenu_Free(work->view->menu);
        return sYesNoFuncs[work->view->yesNoIndex].yes(work);
    case 1:
        AppTaskMenu_Free(work->view->menu);
        return sYesNoFuncs[work->view->yesNoIndex].no(work);
    default:
        return MAILBOX_STATE_WAIT_YES_NO;
    }
}

static u32 MailBox_StateWaitPaletteFx(MailBoxWork *work) {
    if (MailBox_UpdatePaletteFx(work) == FALSE) {
        return work->nextState;
    }
    return MAILBOX_STATE_WAIT_PALETTE_FX;
}

static u32 MailBox_StateStartChild(MailBoxWork *work) {
    sChildProcs[work->childProc].start(work);
    return MAILBOX_STATE_WAIT_CHILD;
}

static u32 MailBox_StateWaitChild(MailBoxWork *work) {
    if (GFL_ProcMgrUpdate(work->procManager) == TRUE) {
        return MAILBOX_STATE_WAIT_CHILD;
    }
    work->nextState = sChildProcs[work->childProc].end(work);
    return sChildProcs[work->childProc].nextState;
}

static u32 MailBox_StateWipeIn(MailBoxWork *work) {
    return MailBox_StartWipeIn(work, MAILBOX_STATE_SELECT_MAIL);
}

static u32 MailBox_StateSelectMail(MailBoxWork *work) {
    u32 result;

    if (func_02021c0c(work->view->printQueue) == FALSE) {
        return MAILBOX_STATE_SELECT_MAIL;
    }
    switch (MailBox_GetTouch(work)) {
    case 0:
        if (work->page != 0) {
            work->page--;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 0, MAILBOX_STATE_CHANGE_PAGE);
        }
        return MAILBOX_STATE_SELECT_MAIL;
    case 1:
        if (work->page != work->view->maxPage) {
            work->page++;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 1, MAILBOX_STATE_CHANGE_PAGE);
        }
        return MAILBOX_STATE_SELECT_MAIL;
    case 2:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_RETURN_BUTTON, 9);
        work->nextState = MAILBOX_STATE_WIPE_OUT;
        return MAILBOX_STATE_PRESS_RETURN;
    case 3:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 8);
        work->nextState = MAILBOX_STATE_WIPE_OUT;
        return MAILBOX_STATE_PRESS_CLOSE;
    }
    result = CursorMove_Update(work->view->cursor);
    switch (result) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        if (work->view->pageMails[MAILBOX_PAGE_SIZE * work->page + result] == 0xff) {
            break;
        }
        work->cursor = result;
        work->mailIndex = work->view->pageMails[MAILBOX_PAGE_SIZE * work->page + result];
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return MailBox_StartFrameFx(work, result, MAILBOX_STATE_OPEN_MENU);
    case CURSOR_MOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_RETURN_BUTTON, 9);
        work->nextState = MAILBOX_STATE_WIPE_OUT;
        return MAILBOX_STATE_PRESS_RETURN;
    case CURSOR_MOVE_EDGE_RIGHT:
        CursorMove_GetPos(work->view->cursor);
        if (work->page != work->view->maxPage) {
            work->page++;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 1, MAILBOX_STATE_CHANGE_PAGE);
        }
        break;
    case CURSOR_MOVE_EDGE_LEFT:
        CursorMove_GetPos(work->view->cursor);
        if (work->page != 0) {
            work->page--;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return MailBox_StartPageFx(work, 0, MAILBOX_STATE_CHANGE_PAGE);
        }
        break;
    case CURSOR_MOVE_MOVED:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSOR_MOVE_CURSOR_ON:
        break;
    default:
        if (MailBox_IsCloseKeyPressed()) {
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 8);
            work->nextState = MAILBOX_STATE_WIPE_OUT;
            return MAILBOX_STATE_PRESS_CLOSE;
        }
        break;
    }
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StateWipeOut(MailBoxWork *work) {
    work->nextState = MAILBOX_STATE_DONE;
    return MailBox_StartWipeOut(work, MAILBOX_STATE_EXIT);
}

static u32 MailBox_StateChangePage(MailBoxWork *work) {
    if (work->page == 0) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, TRUE);
    } else {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, TRUE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    }
    MailBox_UpdateMailList(work);
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StateOpenMenu(MailBoxWork *work) {
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, FALSE);
    MailBox_SetDimmed(work->view, TRUE);
    MailBox_ShowSelectedFrame(work);
    MailBox_ShowMessageWindow(work);
    MailBox_PrintWhatWouldYouLike(work);
    return MailBox_StartMessage(work, FALSE, MAILBOX_STATE_CREATE_MENU);
}

static u32 MailBox_StateCreateMenu(MailBoxWork *work) {
    MailBox_CreateMainMenu(work);
    return MAILBOX_STATE_WAIT_MENU;
}

static u32 MailBox_StateWaitMenu(MailBoxWork *work) {
    AppTaskMenu_Update(work->view->menu);
    if (!AppTaskMenu_IsFlashFinished(work->view->menu)) {
        return MAILBOX_STATE_WAIT_MENU;
    }
    switch (AppTaskMenu_GetCursorPos(work->view->menu) + 2) {
    case 2:
        MailBox_FreeMenu(work);
        MailBox_HideMessageWindow(work);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, FALSE);
        MailBox_Nop1(work->view);
        MailBox_ShowReadCursor(work->view);
        MailBox_ShowMail(work);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 14);
        MailBox_SetDimmedForReading(work->view);
        return MAILBOX_STATE_READ_MAIL;
    case 3:
        MailBox_FreeMenu(work);
        MailBox_SetMessage(work, MAILBOX_MESSAGE_ERASE);
        return MailBox_StartMessage(work, FALSE, MAILBOX_STATE_ASK_ERASE);
    case 4:
        MailBox_FreeMenu(work);
        MailBox_CreatePartyParam(work, 14, PML_ItemGetMailItemID(func_02009750(work->view->mails[work->mailIndex])),
                                 work->mailIndex);
        work->childProc = MAILBOX_CHILD_PARTY_GIVE;
        work->nextState = MAILBOX_STATE_START_CHILD;
        return MailBox_StartWipeOut(work, 1);
    case 0:
    case 5:
        MailBox_FreeMenu(work);
        MailBox_HideMessageWindow(work);
        BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
        MailBox_SetDimmed(work->view, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, TRUE);
        return MAILBOX_STATE_SELECT_MAIL;
    default:
        return MAILBOX_STATE_WAIT_MENU;
    }
}

static u32 MailBox_StateReadMail(MailBoxWork *work) {
    if (MailBox_IsReturnPressed() == TRUE) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_RETURN_BUTTON, 9);
        work->nextState = MAILBOX_STATE_CLOSE_MAIL;
        return MAILBOX_STATE_PRESS_RETURN;
    }
    return MAILBOX_STATE_READ_MAIL;
}

static u32 MailBox_StateCloseMail(MailBoxWork *work) {
    MailBox_ClearMail(work);
    MailBox_UpdateCursor(work, CursorMove_GetPos(work->view->cursor));
    MailBox_Nop2(work->view);
    BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, TRUE);
    MailBox_SetDimmed(work->view, FALSE);
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StateAskErase(MailBoxWork *work) {
    return MailBox_StartYesNo(work, 0);
}

static u32 MailBox_StateEraseNotice(MailBoxWork *work) {
    MailBox_SetMessage(work, MAILBOX_MESSAGE_ASK_HOLD);
    return MailBox_StartMessage(work, FALSE, MAILBOX_STATE_ASK_GIVE);
}

static u32 MailBox_StateAskGive(MailBoxWork *work) {
    return MailBox_StartYesNo(work, 1);
}

static u32 MailBox_StatePartyReturn(MailBoxWork *work) {
    if (work->partyResult != 6) {
        work->nextState = MAILBOX_STATE_GIVE_FADE_IN;
        return MAILBOX_STATE_INIT;
    }
    work->childProc = MAILBOX_CHILD_MAIL_SYS;
    return MAILBOX_STATE_START_CHILD;
}

static u32 MailBox_StateGiveFadeIn(MailBoxWork *work) {
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, FALSE);
    MailBox_SetDimmed(work->view, TRUE);
    return MailBox_StartWipeIn(work, MAILBOX_STATE_GIVE_RESULT);
}

static u32 MailBox_StateGiveResult(MailBoxWork *work) {
    MailBox_ShowMessageWindow(work);
    return MailBox_GiveNo(work);
}

static u32 MailBox_StateMailWritten(MailBoxWork *work) {
    if (!work->mailGiven) {
        work->nextState = MAILBOX_STATE_GIVE_FADE_IN;
        return MAILBOX_STATE_INIT;
    }
    MailBox_CreatePartyParam(work, 11, work->item, work->partyPos);
    work->childProc = MAILBOX_CHILD_PARTY_PICK;
    return MAILBOX_STATE_START_CHILD;
}

static u32 MailBox_StatePressReturn(MailBoxWork *work) {
    if (!func_0204c560(work->view->actors[MAILBOX_ACTOR_RETURN_BUTTON])) {
        work->param->result = 0;
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CLOSE_BUTTON, 0);
        return work->nextState;
    }
    return MAILBOX_STATE_PRESS_RETURN;
}

static u32 MailBox_StatePressClose(MailBoxWork *work) {
    if (!func_0204c560(work->view->actors[MAILBOX_ACTOR_CLOSE_BUTTON])) {
        work->param->result = 1;
        return work->nextState;
    }
    return MAILBOX_STATE_PRESS_CLOSE;
}

static u32 MailBox_StateBackToList(MailBoxWork *work) {
    MailBox_HideMessageWindow(work);
    BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
    MailBox_UpdateCursor(work, CursorMove_GetPos(work->view->cursor));
    MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_CURSOR, TRUE);
    MailBox_SetDimmed(work->view, FALSE);
    return MAILBOX_STATE_SELECT_MAIL;
}

static u32 MailBox_StartWipeIn(MailBoxWork *work, u32 nextState) {
    GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_MAILBOX_VIEW);
    work->view->wipeNextState = nextState;
    return MAILBOX_STATE_WAIT_WIPE;
}

static u32 MailBox_StartWipeOut(MailBoxWork *work, u32 nextState) {
    GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_MAILBOX_VIEW);
    work->view->wipeNextState = nextState;
    return MAILBOX_STATE_WAIT_WIPE;
}

static u32 MailBox_StartMessage(MailBoxWork *work, u8 waitForInput, u32 nextState) {
    work->view->waitForInput = waitForInput;
    work->nextState = nextState;
    return MAILBOX_STATE_WAIT_MESSAGE;
}

static u32 MailBox_StartYesNo(MailBoxWork *work, u32 index) {
    work->view->yesNoIndex = index;
    MailBox_CreateYesNoMenu(work);
    return MAILBOX_STATE_WAIT_YES_NO;
}

static u32 MailBox_StartFrameFx(MailBoxWork *work, u32 frame, u32 nextState) {
    MailBoxPaletteFx *fx;
    s8 x;
    s8 y;
    u16 width;
    u16 height;

    BGWinFrame_GetPos(work->view->frames, frame, &x, &y);
    BGWinFrame_GetSize(work->view->frames, frame, &width, &height);
    fx = &work->view->paletteFx;
    fx->onScreen = TRUE;
    fx->target = 2;
    fx->firstPalette = 2;
    fx->secondPalette = 1;
    fx->phase = 0;
    fx->timer = 0;
    fx->x = x;
    fx->y = y;
    fx->width = width;
    fx->height = height;
    work->nextState = nextState;
    return MAILBOX_STATE_WAIT_PALETTE_FX;
}

static u32 MailBox_StartPageFx(MailBoxWork *work, u32 direction, u32 nextState) {
    MailBoxPaletteFx *fx = &work->view->paletteFx;

    fx->onScreen = FALSE;
    fx->target = direction;
    fx->firstPalette = 1;
    fx->secondPalette = 0;
    fx->phase = 0;
    fx->timer = 0;
    work->nextState = nextState;
    return MAILBOX_STATE_WAIT_PALETTE_FX;
}

static void MailBox_SetDimmed(MailBoxView *view, BOOL dimmed) {
    if (dimmed == TRUE) {
        MailBox_SetActorBlend(view, 0, TRUE);
        MailBox_SetActorBlend(view, 1, TRUE);
        MailBox_SetActorBlend(view, 3, TRUE);
        MailBox_SetActorBlend(view, 7, TRUE);
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 4, 8, 4, 12);
    } else {
        MailBox_SetActorBlend(view, 0, FALSE);
        MailBox_SetActorBlend(view, 1, FALSE);
        MailBox_SetActorBlend(view, 3, FALSE);
        MailBox_SetActorBlend(view, 7, FALSE);
        G2_BlendNone();
    }
}

static void MailBox_SetDimmedForReading(MailBoxView *view) {
    MailBox_SetActorBlend(view, 0, TRUE);
    MailBox_SetActorBlend(view, 1, TRUE);
    MailBox_SetActorBlend(view, 3, FALSE);
    MailBox_SetActorBlend(view, 7, FALSE);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 4, 8, 4, 12);
}

static void MailBox_ShowMail(MailBoxWork *work) {
    MailBox_LoadMailGraphics(work);
    MailBox_PrintMail(work);
}

static void MailBox_ClearMail(MailBoxWork *work) {
    int i;

    for (i = 0; i < 3; i++) {
        if (PMSData_IsNotEmpty(func_0200976c(work->view->mails[work->mailIndex], i))) {
            PMSDraw_Clear(work->view->pmsDraw, i, FALSE);
        }
    }
    if (work->view->stampSentence.words[0] != 0) {
        PMSDraw_Clear(work->view->pmsDraw, i, FALSE);
    }
    GFL_BGSysClearScr(4);
    GFL_BGSysClearScr(5);
}

static void MailBox_TakeMailItem(MailBoxWork *work) {
    BagSave *bag = GameData_GetBag(work->param->gameData);

    work->item = PML_ItemGetMailItemID(func_02009750(work->view->mails[work->mailIndex]));
    if (BagSave_AddItem(bag, work->item, 1, HEAPID_MAILBOX_VIEW) == TRUE) {
        work->bagFull = FALSE;
    } else {
        work->bagFull = TRUE;
    }
}

static void MailBox_EraseMail(MailBoxWork *work) {
    func_020097d0(work->mails, 0, work->mailIndex);
}

static void MailBox_CreatePartyParam(MailBoxWork *work, u32 mode, u16 item, u16 mailIndex) {
    PokeListParam *param = PokeListParam_Create(work->param->gameData, mode, GameData_GetParty(work->param->gameData),
                                                HEAPID_MAILBOX_PROC);

    param->item = item;
    param->unk6C = mailIndex;
    work->childParam = param;
}

static u32 MailBox_StartPartyList(MailBoxWork *work) {
    PokeListParam *param = work->childParam;

    param->index = work->partyPos;
    QueueGameProc(work->procManager, OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, work->childParam);
    return 0;
}

static u32 MailBox_EndPartyListCancel(MailBoxWork *work) {
    GFL_HeapFree(work->childParam);
    return MAILBOX_STATE_WIPE_IN;
}

static u32 MailBox_EndPartyListPick(MailBoxWork *work) {
    PokeListParam *param = work->childParam;

    work->partyPos = param->index;
    work->partyResult = param->result;
    GFL_HeapFree(param);
    return 0;
}

static u32 MailBox_EndPartyListCancelAsk(MailBoxWork *work) {
    GFL_HeapFree(work->childParam);
    return MAILBOX_STATE_WIPE_IN;
}

static u32 MailBox_StartMailSys(MailBoxWork *work) {
    work->childParam = MailTool_CreateWriteParam(work->param->gameData, 2, work->partyPos, PML_ItemGetMailID(work->item),
                                           HEAPID_MAILBOX_PROC);
    QueueGameProc(work->procManager, OVERLAY_NONE, &sMailSysProc, work->childParam);
    return 0;
}

static u32 MailBox_EndMailSys(MailBoxWork *work) {
    work->mailGiven = MailTool_IsWritten(work->childParam);
    if (work->mailGiven == TRUE) {
        MailTool_GiveMail(work->childParam,
                            PokeParty_GetPkm(GameData_GetParty(work->param->gameData), work->partyPos));
        BagSave_SubItem(GameData_GetBag(work->param->gameData), work->item, 1, HEAPID_MAILBOX_PROC);
    }
    MailTool_FreeParam(work->childParam);
    return 0;
}

static u32 MailBox_EraseYes(MailBoxWork *work) {
    MailBox_TakeMailItem(work);
    MailBox_EraseMail(work);
    MailBox_FreeMails(work);
    MailBox_LoadMails(work);
    if (work->view->mailCount <= MAILBOX_PAGE_SIZE) {
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_PREV_PAGE, FALSE);
        MailBox_SetActorVisible(work->view, MAILBOX_ACTOR_NEXT_PAGE, FALSE);
    }
    if (work->page > work->view->maxPage) {
        work->page = work->view->maxPage;
        work->cursor = 0;
    }
    MailBox_UpdateMailList(work);
    BGWinFrame_Hide(work->view->frames, MAILBOX_FRAME_SELECTED);
    MailBox_SetMessage(work, MAILBOX_MESSAGE_ERASED);
    return MailBox_StartMessage(work, TRUE, MAILBOX_STATE_ERASE_NOTICE);
}

static u32 MailBox_GiveYes(MailBoxWork *work) {
    MailBox_CreatePartyParam(work, 9, work->item, 0);
    work->childProc = MAILBOX_CHILD_PARTY_ASK;
    work->nextState = MAILBOX_STATE_START_CHILD;
    return MailBox_StartWipeOut(work, 1);
}

static u32 MailBox_GiveNo(MailBoxWork *work) {
    if (!work->bagFull) {
        MailBox_SetMessage(work, MAILBOX_MESSAGE_HOLD_DONE);
    } else {
        MailBox_SetMessage(work, MAILBOX_MESSAGE_BAG_FULL);
    }
    return MailBox_StartMessage(work, TRUE, MAILBOX_STATE_BACK_TO_LIST);
}

static s32 MailBox_GetTouch(MailBoxWork *work) {
    return func_0203da0c(sButtonRects);
}

static void MailBox_CreateCursor(MailBoxWork *work) {
    u32 cursor;

    if (work->page > work->view->maxPage) {
        work->page = work->view->maxPage;
        cursor = 0;
    } else {
        cursor = work->cursor;
    }
    work->view->cursor = CursorMove_Create(sCursorData, &sCursorCallbacks, work, TRUE, cursor, HEAPID_MAILBOX_VIEW);
    MailBox_UpdateCursor(work, cursor);
}

static void MailBox_FreeCursor(MailBoxWork *work) {
    CursorMove_Delete(work->view->cursor);
}

static void MailBox_CursorOn(void *work, int pos, int prevPos) {
}

static void MailBox_CursorOff(void *work, int pos, int prevPos) {
}

static void MailBox_CursorMoved(void *work, int pos, int prevPos) {
    MailBox_UpdateCursor(work, pos);
}

static void MailBox_CursorTouched(void *work, int pos, int prevPos) {
    MailBox_UpdateCursor(work, pos);
}

static void MailBox_UpdateCursor(MailBoxWork *work, u32 pos) {
    const CursorMoveData *data = CursorMove_GetData(work->view->cursor, pos);

    MailBox_SetActorPos(work->view, MAILBOX_ACTOR_CURSOR, data->x, data->y);
    if (pos == 10) {
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CURSOR, 4);
    } else {
        MailBox_SetActorAnim(work->view, MAILBOX_ACTOR_CURSOR, 5);
    }
}

static BOOL MailBox_IsReturnPressed(void) {
    if (func_0203da0c(sReturnRects) != -1) {
        return TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MailBox_IsCloseKeyPressed(void) {
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        return TRUE;
    }
    return FALSE;
}
