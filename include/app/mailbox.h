#ifndef POKEBW2_APP_MAILBOX_H
#define POKEBW2_APP_MAILBOX_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The mailbox, overlay 215 (mailsys.c, mailtool.c and mailview.c)
#define OVERLAY_MAILBOX OVERLAY_ID(215)

struct MailboxProcessData {
    GameData *gameData;
    u32 result;
};

extern const GameProcFunctions MAILBOX_PROC_FUNCTIONS;

#endif // POKEBW2_APP_MAILBOX_H
