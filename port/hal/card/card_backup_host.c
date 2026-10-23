// NitroSDK's backup commands on the host: the decomp's card_backup.c, compiled as it is from its own path through this
// file (port/hal/card/replaced.txt keeps it out of the game's own sources). Two things can't run on a 64-bit host as
// written, so they are renamed out of the way before the include and written again below, where they reach the file's
// statics:
//   - CARDi_RequestStreamCommand takes the game's address as a u32, since card.h's inline functions (CARD_ReadBackup,
//     CARD_ReadBackupAsync, CARD_WriteAndVerifyBackupAsync) cast the pointer to one, and its task,
//     CARDi_RequestStreamCommandCore, copies each page between the page buffer and that address through CARDiCommon's
//     32-bit src or dst;
//   - the ARM7 side (port/hal/card/backup.c) needs the page buffer, which the ARM9 names to it only in the command
//     block's 32-bit words.
// The game's calls of the three inline functions reach the host's port_card_read_backup and
// port_card_write_and_verify_backup instead (port/hal/card/card_backup_calls.h), which keep the game's pointer whole
// for the task. A call of CARDi_RequestStreamCommand itself with a game address, from a file that doesn't have that
// header, stops the program with a message: the pointer it was cut from is gone, and no guess of it is safe. Once
// card.h's interface carries the address as uptr, a game change (CARDiCommon's src and dst,
// CARDi_RequestStreamCommand's parameters and the three inline functions' casts), the decomp's own task works and this
// file only names the page buffer.
//
// card.h comes first, so that its prototype of CARDi_RequestStreamCommand keeps the name, which the host's version has
#include "hal/card/card_backup_calls.h"
#include "hal/card/port_card.h"
#include "nitro/card.h"

#define CARDi_RequestStreamCommand CARDi_DsRequestStreamCommand
#define CARDi_RequestStreamCommandCore CARDi_DsRequestStreamCommandCore
// The decomp's file keeps addresses in 32-bit words, as the DS does. Its warnings about them, and those of the warning
// groups the game's own files aren't built with, are silenced for the file alone, as port/CMakeLists.txt does for every
// file of the game (pokebw2_game); the compile probe counts them from the file's own path (port/tools/compile-probe).
// The host's code below keeps every warning of the platform layer
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
#include "lib/nitro/src/card/card_backup.c"
#pragma GCC diagnostic pop
#undef CARDi_RequestStreamCommand
#undef CARDi_RequestStreamCommandCore

#include "hal/card/card_internal.h"
#include <stdio.h>
#include <stdlib.h>

static void CARDi_RequestStreamCommandCore(CARDiCommon *p);

// The game's side of the command in progress, as host pointers: where a read puts the next page, or where a write
// takes it from. NULL when the command doesn't go that way, and for the SPECIAL mode, which hands both addresses on as
// they are
static u8 *sReadInto;
static const u8 *sWriteFrom;

u8 *port_card_page_buffer(void) {
    return sCardBackupBuffer;
}

// card_backup.c's CARDi_RequestStreamCommandCore, with the game's side of each page through sReadInto or sWriteFrom
static void CARDi_RequestStreamCommandCore(CARDiCommon *p) {
    int reqType = p->reqType;
    CARDRequestMode reqMode = p->reqMode;
    int reqRetry = p->reqRetry;
    u32 size = CARD_BACKUP_PAGE_SIZE;
    SDK_REFER_SYMBOL(cardi_backup_assert);
    if (reqType == CARD_REQ_ERASE_SECTOR_BACKUP) {
        size = CARD_GetBackupSectorSize();
    } else if (reqType == CARD_REQ_ERASE_SUBSECTOR_BACKUP) {
        size = cardi_common.cmd->spec.subsectSize;
    }
    do {
        u32 len = p->len;
        if (size < len) {
            len = size;
        }
        p->cmd->len = len;
        if ((p->flag & CARD_STAT_CANCEL) != 0) {
            p->flag &= ~CARD_STAT_CANCEL;
            p->cmd->result = CARD_RESULT_CANCELED;
            return;
        }
        switch (reqMode) {
        case CARD_REQUEST_MODE_RECV:
            DC_InvalidateRange(sCardBackupBuffer, len);
            p->cmd->src = p->src;
            p->cmd->dst = (u32)(uintptr_t)sCardBackupBuffer;
            break;
        case CARD_REQUEST_MODE_SEND:
        case CARD_REQUEST_MODE_SEND_VERIFY:
            MI_CpuCopy8(sWriteFrom, sCardBackupBuffer, len);
            DC_FlushRange(sCardBackupBuffer, len);
            DC_WaitWriteBufferEmpty();
            p->cmd->src = (u32)(uintptr_t)sCardBackupBuffer;
            p->cmd->dst = p->dst;
            break;
        case CARD_REQUEST_MODE_SPECIAL:
            p->cmd->src = p->src;
            p->cmd->dst = p->dst;
            break;
        }
        if (!CARDi_Request(p, reqType, reqRetry)) {
            break;
        }
        if (reqMode == CARD_REQUEST_MODE_SEND_VERIFY) {
            if (!CARDi_Request(p, CARD_REQ_VERIFY_BACKUP, 1)) {
                break;
            }
        } else if (reqMode == CARD_REQUEST_MODE_RECV) {
            MI_CpuCopy8(sCardBackupBuffer, sReadInto, len);
        }
        p->src += len;
        p->dst += len;
        if (sReadInto != NULL) {
            sReadInto += len;
        }
        if (sWriteFrom != NULL) {
            sWriteFrom += len;
        }
        p->len -= len;
    } while (p->len != 0);
}

// card_backup.c's CARDi_RequestStreamCommand, with the game's side as host pointers, set once the last command is done
// (CARDi_BeginBackupCommand waits for it), so that the next one can't change them under the last one's feet.
// CARDiCommon's src and dst get what the DS's would hold, the game's address cut to 32 bits, which only the ARM7's
// checks see
static BOOL CARDi_RequestStream(u32 src, u32 dest, u8 *readInto, const u8 *writeFrom, u32 size, MIDmaCallback callback,
                                void *arg, BOOL async, CARDRequest request, int retries, CARDRequestMode mode) {
    CARDi_BeginBackupCommand(
        mode == CARD_REQUEST_MODE_RECV ? CARD_ACCESS_LEVEL_BACKUP_READ : CARD_ACCESS_LEVEL_BACKUP_WRITE, callback, arg);
    sReadInto = readInto;
    sWriteFrom = writeFrom;
    cardi_common.src = src;
    cardi_common.dst = dest;
    cardi_common.len = size;
    cardi_common.reqType = request;
    cardi_common.reqRetry = retries;
    cardi_common.reqMode = mode;
    return CARDi_ExecuteOldTypeTask(CARDi_RequestStreamCommandCore, async);
}

BOOL port_card_read_backup(u32 src, void *dest, u32 size, MIDmaCallback callback, void *arg, BOOL async) {
    return CARDi_RequestStream(src, (u32)(uintptr_t)dest, dest, NULL, size, callback, arg, async, CARD_REQ_READ_BACKUP,
                               1, CARD_REQUEST_MODE_RECV);
}

BOOL port_card_write_and_verify_backup(u32 dest, const void *src, u32 size, MIDmaCallback callback, void *arg) {
    return CARDi_RequestStream((u32)(uintptr_t)src, dest, NULL, src, size, callback, arg, TRUE, CARD_REQ_WRITE_BACKUP,
                               CARD_RETRY_COUNT_MAX, CARD_REQUEST_MODE_SEND_VERIFY);
}

// card_backup.c's CARDi_RequestStreamCommand, for a caller that gives the game's address in 32 bits: only the SPECIAL
// mode, whose two addresses go to the ARM7 as they are, can run. A read or write of game memory stops the program,
// since the host can't tell which pointer the address was cut from (above)
BOOL CARDi_RequestStreamCommand(u32 src, u32 dest, u32 size, MIDmaCallback callback, void *arg, BOOL async,
                                CARDRequest request, int retries, CARDRequestMode mode) {
    if (mode != CARD_REQUEST_MODE_SPECIAL) {
        fprintf(stderr,
                "port: card: a backup %s of %u bytes got the game's buffer as %#x, cut to 32 bits, through "
                "CARDi_RequestStreamCommand; the host takes the buffer only as a whole pointer: the calling file needs "
                "port/hal/card/card_backup_calls.h (port/cmake/rom.cmake), or card.h's interface uptr\n",
                mode == CARD_REQUEST_MODE_RECV ? "read" : "write", size, mode == CARD_REQUEST_MODE_RECV ? dest : src);
        exit(PORT_CARD_EXIT_STATUS);
    }
    return CARDi_RequestStream(src, dest, NULL, NULL, size, callback, arg, async, request, retries, mode);
}
