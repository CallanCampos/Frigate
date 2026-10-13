#ifndef POKEBW2_APP_MAILBOX_LOCAL_H
#define POKEBW2_APP_MAILBOX_LOCAL_H

#include "types.h"
#include "app/mailbox.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "pml/mail.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"
#include "system/bgwinfrm.h"
#include "system/cursor_move.h"
#include "system/gf_font.h"
#include "system/pms_data.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The mailbox screen: the 20 mails of the save's mailbox, ten to a page, on the touch screen's list, and the menu that
// reads, erases or gives one to a Pokémon. A state machine, one function per state, with the party list and the
// mail writing screen run as child processes. Names are ours, guessed
//
// The work of the screen and what its files call of each other: mailbox.c, mb_main.c, mb_bmp.c, mb_obj.c, mb_seq.c
// and mb_ui.c of overlay 215. Only mb_seq.c is named by the ROM; the others are guesses, after the files of the PC box

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

// mb_main.c
extern const BGSysVRAMConfig MailBox_VRAMConfig;

void MailBox_VBlank(TCB *tcb, void *data);
void MailBox_SetVRAMBanks(void);
void MailBox_InitBGs(MailBoxWork *work);
void MailBox_FreeBGs(MailBoxWork *work);
void MailBox_LoadBGGraphics(MailBoxWork *work);
void MailBox_LoadSystemPalettes(MailBoxWork *work);
void MailBox_CreateFrames(MailBoxWork *work);
void MailBox_FreeFrames(MailBoxWork *work);
void MailBox_LoadMessages(MailBoxView *view);
void MailBox_FreeMessages(MailBoxView *view);
void MailBox_CreateYesNoMenu(MailBoxWork *work);
void MailBox_CreateMenuResources(MailBoxWork *work);
void MailBox_FreeMenuResources(MailBoxWork *work);
void MailBox_CreateMainMenu(MailBoxWork *work);
void MailBox_FreeMenu(MailBoxWork *work);
void MailBox_LoadMails(MailBoxWork *work);
void MailBox_FreeMails(MailBoxWork *work);
void MailBox_UpdateMailList(MailBoxWork *work);
void MailBox_ShowSelectedFrame(MailBoxWork *work);
BOOL MailBox_UpdatePaletteFx(MailBoxWork *work);
void MailBox_LoadMailGraphics(MailBoxWork *work);
void MailBox_CreatePMSDraw(MailBoxWork *work);
void MailBox_FreePMSDraw(MailBoxWork *work);

// mb_bmp.c
void MailBox_CreateWindows(MailBoxWork *work);
void MailBox_FreeWindows(MailBoxWork *work);
void MailBox_DrawMailFrame(MailBoxWork *work, u32 frame, u32 windowIndex, u32 mailIndex);
void MailBox_PrintTitle(MailBoxWork *work);
void MailBox_Nop1(MailBoxView *view);
void MailBox_Nop2(MailBoxView *view);
void MailBox_PrintWhatWouldYouLike(MailBoxWork *work);
void MailBox_ShowMessageWindow(MailBoxWork *work);
void MailBox_HideMessageWindow(MailBoxWork *work);
void MailBox_SetMessage(MailBoxWork *work, u32 message);
void MailBox_PrintMail(MailBoxWork *work);

// mb_obj.c
void MailBox_CreateOBJs(MailBoxWork *work);
void MailBox_FreeOBJs(MailBoxWork *work);
void MailBox_UpdateClActUnit(MailBoxView *view);
void MailBox_SetActorAnim(MailBoxView *view, u32 index, u32 sequence);
void MailBox_SetActorVisible(MailBoxView *view, u32 index, BOOL visible);
void MailBox_SetActorPos(MailBoxView *view, u32 index, int x, int y);
void MailBox_SetActorBlend(MailBoxView *view, u32 index, BOOL blend);
void MailBox_ShowReadCursor(MailBoxView *view);

// mb_seq.c
BOOL MailBox_Update(MailBoxWork *work, u32 *state);

// mb_ui.c
s32 MailBox_GetTouch(MailBoxWork *work);
void MailBox_CreateCursor(MailBoxWork *work);
void MailBox_FreeCursor(MailBoxWork *work);
void MailBox_UpdateCursor(MailBoxWork *work, u32 pos);
BOOL MailBox_IsReturnPressed(void);
BOOL MailBox_IsCloseKeyPressed(void);

#endif // POKEBW2_APP_MAILBOX_LOCAL_H
