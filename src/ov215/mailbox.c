#include "types.h"
#include "app/mailbox.h"
#include "app/mailbox_local.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "pml/mail.h"

// The mailbox screen's proc. The ROM doesn't name this file; the name is a guess, after the screen's subject

static BOOL MailBox_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MailBox_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MailBox_Exit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions MAILBOX_PROC_FUNCTIONS = { MailBox_Init, MailBox_Main, MailBox_Exit };

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
