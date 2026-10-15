#ifndef POKEBW2_FIELD_MAILSYS_H
#define POKEBW2_FIELD_MAILSYS_H

#include "types.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/app_taskmenu.h"
#include "system/pms_data.h"
#include "system/printsys.h"

// The mail screen (overlay 215's mailsys.c and mailview.c): shows a mail, or has the player write one. The names are
// ours, guessed

// Heaps of overlay 215 (not from swan): the screen's proc, and the mail screen itself
#define HEAPID_MAIL 0x63
#define HEAPID_MAIL_SCREEN 0x64

#define MAIL_WORD_COUNT 3

typedef struct MailSysWork MailSysWork;

// What a mail screen shows, and what it returns: the mail as a copy that the player edits
typedef struct MailEdit {
    // The MAIL_PARAM_MODE_* on entry. Then what the screen chose: a sentence from 0 to 2 or the word 3 to input,
    // MAIL_EDIT_DONE to keep the mail, or MAIL_EDIT_CANCEL
    u16 mode;
    // The sentence or word that the input screen edits, and the cursor to come back to
    u8 selected;
    u8 cursor;
    u32 trainerId;
    u8 unk08;
    u8 unk09;
    u8 unk0A;
    u8 design;
    StrBuf *name;
    u8 unk10[6];
    PMSData words[MAIL_WORD_COUNT];
    // Whether the design gives the sentence a preset one to start from
    u8 hasPreset[MAIL_WORD_COUNT];
    u8 unk31;
    // The mail's easy word
    u16 word;
} MailEdit;

#define MAIL_EDIT_DONE 4
#define MAIL_EDIT_CANCEL 0xffff

// A touch or key mode change, called with the mail screen's work
typedef BOOL (*MailSysModeFunc)(MailSysWork *work);

typedef struct {
    BmpWin *win;
    // Whether its text is drawn
    u8 drawn;
} MailSysBmpWin;

// The work of the mail screen's proc (mailsys.c's MailSys_EditInit), whose screens mailview.c sets up
struct MailSysWork {
    u32 heapId;
    // The step of MailSys_Update
    u32 seq;
    // The step of the phase or of the loading
    u16 subSeq;
    // Whether the keys drive the cursor rather than the touch screen
    u8 keyMode;
    // MAIL_PHASE_*
    u8 phase;
    u8 initialPhase;
    u32 confirming;
    u8 unk14;
    s8 textSpeed;
    u8 cursor;
    u8 savedCursor;
    // The cursor's pulse, and whether it fades back
    u8 blinkLevel;
    u8 blinkFalling;
    u8 blinkTarget;
    u8 blinkCurrent;
    MailEdit *edit;
    MsgData *msgData;
    Font *font;
    u32 unk28[3];
    StrBuf *msgBuf;
    PaletteFade *palFade;
    MailSysModeFunc onKeyMode;
    MailSysModeFunc onTouchMode;
    BmpWin *windows[7];
    MailSysBmpWin entries[6];
    u8 unk90[8];
    PrintQueue *printQueue;
    PrintStream *printStream;
    ClActUnit *clUnit;
    u8 unkA4[0x50];
    void *tcbBuffer;
    TCBManager *tcbMgr;
    TCB *blinkTask;
    TCBExManager *tcbEx;
    TCB *vblankTask;
    AppTaskMenuRes *menuRes;
    AppTaskMenu *menu;
    AppTaskMenuItem menuItems[2];
    AppTaskMenuWin *confirmButton;
    AppTaskMenuWin *cancelButton;
    AppTaskMenuItem buttonItems[2];
    PMSDraw *pmsDraw;
    PMSData pmsWord;
    ClActUnit *pmsClUnit;
    PrintQueue *pmsQueue;
};

// The phases: the mail only shown, and the mail written
#define MAIL_PHASE_VIEW 0
#define MAIL_PHASE_WRITE 1

// The mail screen's proc, which the field app call runs. It runs the screen of the mail as a child proc
extern const GameProcFunctions MAILSYS_PROC_FUNCTIONS;

// The proc for the field app call's table (MWCC counts this read, ahead of the table's definition, as a reference,
// which puts the table among the other .rodata objects of mailsys.c as it is in the ROM)
static inline const GameProcFunctions *MailSys_GetProcFunctions(void) {
    return &MAILSYS_PROC_FUNCTIONS;
}

// The VRAM banks of the mail screen
extern const BGSysVRAMConfig MailSys_VRAMConfig;

BOOL MailSys_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
BOOL MailSys_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
BOOL MailSys_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
BOOL MailSys_OnTouchMode(MailSysWork *work);
BOOL MailSys_OnKeyMode(MailSysWork *work);
// The task that pulses the cursor's palette
void MailSys_UpdateBlink(TCB *tcb, void *data);

#endif // POKEBW2_FIELD_MAILSYS_H
