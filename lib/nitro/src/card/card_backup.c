// NitroSDK's CARD library, the backup memory: telling which kind it is, and reading and writing it a page at a time.
// The file name is a guess, from NitroSDK's card_backup.c; the library's middleware signature "[SDK+NINTENDO:BACKUP]"
// is passed by the first and the fourth function here and nowhere else.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The page that the commands move a page of the backup through
#define CARD_BACKUP_PAGE_SIZE 256

static void CARDi_RequestStreamCommandCore(CARDiCommon *p);
static void CARDi_IdentifyBackupCore(CARDBackupType type);
static void CARDi_IdentifyBackupTask(CARDiCommon *p);
static BOOL CARDi_BeginBackupCommand(u32 permit, MIDmaCallback callback, void *arg);
static u32 CARD_GetBackupSectorSize(void);

static u8 sCardBackupBuffer[CARD_BACKUP_PAGE_SIZE] ATTRIBUTE_ALIGN(32);

// The task of every command that moves a number of bytes: runs it a page at a time
static void CARDi_RequestStreamCommandCore(CARDiCommon *p) {
    int reqType = p->reqType;
    CARDRequestMode reqMode = p->reqMode;
    int reqRetry = p->reqRetry;
    u32 size = CARD_BACKUP_PAGE_SIZE;
    OSi_ReferSymbol(CARDi_BackupSignature);
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
            p->cmd->dst = (u32)sCardBackupBuffer;
            break;
        case CARD_REQUEST_MODE_SEND:
        case CARD_REQUEST_MODE_SEND_VERIFY:
            MI_CpuCopy8((void *)p->src, sCardBackupBuffer, len);
            DC_FlushRange(sCardBackupBuffer, len);
            DC_WaitWriteBufferEmpty();
            p->cmd->src = (u32)sCardBackupBuffer;
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
            MI_CpuCopy8(sCardBackupBuffer, (void *)p->dst, len);
        }
        p->src += len;
        p->dst += len;
        p->len -= len;
    } while (p->len != 0);
}

// Fills in the command block's description of the backup memory of this type: its sizes, its timings and the requests
// it can execute. A type that isn't known is answered as unsupported
static void CARDi_IdentifyBackupCore(CARDBackupType type) {
    CARDiCommandArg *cmd = cardi_common.cmd;
    u32 size;
    u32 device;
    u32 vendor;
    MI_CpuFill8(&cmd->spec, 0, sizeof(cmd->spec));
    cmd->type = type;
    // The requests that every kind can execute: 0 to 5 and 13
    cmd->spec.caps = 0x203f;
    if (type == 0) {
        return;
    }
    size = 1 << ((type >> 8) & 0xff);
    device = type & 0xff;
    vendor = (type >> 16) & 0xff;
    cmd->spec.totalSize = size;
    cmd->spec.initialStatus = 0xff;
    if (device == CARD_BACKUP_TYPE_DEVICE_EEPROM) {
        switch (size) {
        case 0x200:
            cmd->spec.pageSize = 0x10;
            cmd->spec.addrWidth = 1;
            cmd->spec.programPage = 5;
            cmd->spec.initialStatus = 0xf0;
            break;
        case 0x2000:
            cmd->spec.pageSize = 0x20;
            cmd->spec.addrWidth = 2;
            cmd->spec.programPage = 5;
            cmd->spec.initialStatus = 0;
            break;
        case 0x10000:
            cmd->spec.pageSize = 0x80;
            cmd->spec.addrWidth = 2;
            cmd->spec.programPage = 10;
            cmd->spec.initialStatus = 0;
            break;
        case 0x20000:
            cmd->spec.pageSize = 0x100;
            cmd->spec.addrWidth = 3;
            cmd->spec.programPage = 5;
            cmd->spec.initialStatus = 0;
            break;
        default:
            goto unsupported;
        }
        cmd->spec.sectSize = cmd->spec.pageSize;
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_READ_BACKUP);
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_PROGRAM_BACKUP);
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_VERIFY_BACKUP);
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_STATUS);
        return;
    }
    if (device == CARD_BACKUP_TYPE_DEVICE_FLASH) {
        switch (size) {
        case 0x40000:
        case 0x80000:
        case 0x100000:
            cmd->spec.writePage = 25;
            cmd->spec.writePageTotal = 300;
            cmd->spec.erasePage = 300;
            cmd->spec.eraseSector = 5000;
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_PAGE_BACKUP);
            break;
        case 0x200000:
            cmd->spec.writePage = 23;
            cmd->spec.writePageTotal = 300;
            cmd->spec.eraseSector = 500;
            cmd->spec.eraseSectorTotal = 5000;
            cmd->spec.eraseChip = 10000;
            cmd->spec.eraseChipTotal = 60000;
            cmd->spec.initialStatus = 0;
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_PAGE_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_CHIP_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_STATUS);
            break;
        case 0x400000:
            cmd->spec.eraseSector = 600;
            cmd->spec.eraseSectorTotal = 3000;
            cmd->spec.eraseSubsector = 70;
            cmd->spec.eraseSubsectorTotal = 150;
            cmd->spec.eraseChip = 23000;
            cmd->spec.eraseChipTotal = 800000;
            cmd->spec.initialStatus = 0;
            cmd->spec.subsectSize = 0x1000;
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_SUBSECTOR_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_CHIP_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_STATUS);
            break;
        case 0x800000:
            if (vendor == 0) {
                cmd->spec.eraseSector = 1000;
                cmd->spec.eraseSectorTotal = 3000;
                cmd->spec.eraseChip = 68000;
                cmd->spec.eraseChipTotal = 160000;
                cmd->spec.initialStatus = 0;
                cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_CHIP_BACKUP);
                cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_STATUS);
            } else if (vendor == 1) {
                cmd->spec.eraseSector = 1000;
                cmd->spec.eraseSectorTotal = 3000;
                cmd->spec.eraseChip = 68000;
                cmd->spec.eraseChipTotal = 160000;
                cmd->spec.initialStatus = 0x84;
                cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_CHIP_BACKUP);
                cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_STATUS);
            }
            break;
        default:
            goto unsupported;
        }
        cmd->spec.sectSize = 0x10000;
        cmd->spec.pageSize = cmd->spec.sectSize >> 8;
        cmd->spec.addrWidth = 3;
        cmd->spec.programPage = 5;
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_READ_BACKUP);
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_PROGRAM_BACKUP);
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_VERIFY_BACKUP);
        cmd->spec.caps |= CARD_CAPS(CARD_REQ_ERASE_SECTOR_BACKUP);
        return;
    }
    if (device == CARD_BACKUP_TYPE_DEVICE_FRAM) {
        if (size == 0x2000 || size == 0x8000) {
            cmd->spec.pageSize = size;
            cmd->spec.sectSize = size;
            cmd->spec.addrWidth = 2;
            cmd->spec.initialStatus = 0;
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_READ_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_PROGRAM_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_VERIFY_BACKUP);
            cmd->spec.caps |= CARD_CAPS(CARD_REQ_WRITE_STATUS);
            return;
        }
    }
unsupported:
    cmd->type = 0;
    cmd->spec.totalSize = 0;
    cardi_common.cmd->result = CARD_RESULT_UNSUPPORTED;
}

static void CARDi_IdentifyBackupTask(CARDiCommon *p) {
    CARDi_Request(p, CARD_REQ_IDENTIFY, 1);
    p->cmd->src = 0;
    p->cmd->dst = (u32)sCardBackupBuffer;
    p->cmd->len = 1;
    CARDi_Request(p, CARD_REQ_READ_BACKUP, 1);
}

// Begins a backup command: checks that the game may use the card in this way, then waits until the last command is
// done
static BOOL CARDi_BeginBackupCommand(u32 permit, MIDmaCallback callback, void *arg) {
    OSi_ReferSymbol(CARDi_BackupSignature);
    CARD_CheckEnabled();
    if ((CARDi_GetAccessLevel() & permit) != permit) {
        sys_exit();
    }
    return CARDi_WaitForTask(&cardi_common, TRUE, callback, arg);
}

BOOL CARDi_RequestStreamCommand(u32 src, u32 dest, u32 size, MIDmaCallback callback, void *arg, BOOL async,
                                CARDRequest request, int retries, CARDRequestMode mode) {
    CARDi_BeginBackupCommand(
        mode == CARD_REQUEST_MODE_RECV ? CARD_ACCESS_LEVEL_BACKUP_READ : CARD_ACCESS_LEVEL_BACKUP_WRITE, callback, arg);
    cardi_common.src = src;
    cardi_common.dst = dest;
    cardi_common.len = size;
    cardi_common.reqType = request;
    cardi_common.reqRetry = retries;
    cardi_common.reqMode = mode;
    return CARDi_ExecuteOldTypeTask(CARDi_RequestStreamCommandCore, async);
}

BOOL CARD_IdentifyBackup(CARDBackupType type) {
    if (type == 0) {
        sys_exit();
    }
    CARDi_BeginBackupCommand(0, NULL, NULL);
    CARDi_IdentifyBackupCore(type);
    return CARDi_ExecuteOldTypeTask(CARDi_IdentifyBackupTask, FALSE);
}

// The size of a sector of the backup memory
static u32 CARD_GetBackupSectorSize(void) {
    return cardi_common.cmd->spec.sectSize;
}

BOOL CARD_WaitBackupAsync(void) {
    return CARDi_WaitAsync();
}

BOOL CARD_TryWaitBackupAsync(void) {
    return CARDi_TryWaitAsync();
}

void CARD_CancelBackupAsync(void) {
    u32 irq = OS_DisableInterrupts();
    cardi_common.flag |= CARD_STAT_CANCEL;
    OS_RestoreInterrupts(irq);
}
