// The genuine NitroSDK backup stream/task/request and native PXI/OS against the host ARM7 save service
// (card_backup.c/card_task.c/card_request.c, port/hal/card/backup.c). Synthetic flash bytes need no ROM. Only
// CARDi_InitRom is supplied by this fixture; the separately compiled real reader retains its other definitions.
// Tests distinguish completed image writes from the later service acknowledgment, including cancellation/reset.
#include "gfl/backup_card.h"
#include "hal/card/card_backup_calls.h"
#include "hal/card/card_internal.h"
#include "hal/card/port_card.h"
#include "hal/host/host.h"
#include "hal/os/port_os.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/snd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "backup service %d: %s\n", __LINE__, #x);                                                  \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
#define WAIT_CYCLES ((u64)13091 * 64)
#define TEST_BYTES 768
static u8 *sBytes, *sRead;
static unsigned sCallbacks, sSound, sOther;
static CARDResult sExpected;
static char sPath[512];
static u64 sBegin;
static OSThread sWorker;
static _Alignas(16) u8 sStack[16384];
// Narrow ROM-init fixture boundary only: common CARD initialization and the task thread remain genuine SDK C.
void CARDi_InitRom(void) {
    port_card_boot();
}
static void Boot(void) {
    OS_InitArena();
    OS_InitLock();
    OS_InitIrqTable();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    PXI_Init();
    CARD_Init();
    OS_EnableIrq();
    OS_EnableInterrupts();
    CHECK(CARD_IdentifyBackup(CARD_BACKUP_TYPE_FLASH_4MBITS_EX));
    CHECK(cardi_common.cmd->spec.writePage == 25);
    sBytes = OS_AllocFromArenaLo(OS_ARENA_MAIN, TEST_BYTES, 32);
    sRead = OS_AllocFromArenaLo(OS_ARENA_MAIN, TEST_BYTES, 32);
    CHECK(sBytes && sRead && (uintptr_t)sBytes > UINT32_MAX && (uintptr_t)sRead > UINT32_MAX);
    for (unsigned i = 0; i < TEST_BYTES; i++)
        sBytes[i] = (u8)(i * 17 + 3);
}
static void Done(void *arg) {
    CHECK(arg == sBytes);
    CHECK(!(cardi_common.flag & (CARD_STAT_BUSY | CARD_STAT_REQ | CARD_STAT_TASK)));
    CHECK(CARD_GetResultCode() == sExpected);
    ++sCallbacks;
}
static void Sound(void *arg) {
    (void)arg;
    ++sSound;
}
static void Other(void) {
    ++sOther;
}
static void Stream(void) {
    Boot();
    SND_Init();
    SND_SetupAlarm(2, 1000, 1000, Sound, NULL);
    SND_StartTimer(0, 0, 1 << 2, 0);
    SND_FlushCommand(SND_COMMAND_BLOCK);
    sSound = 0; // Only callbacks during the actual backup wait count as interleaving.
    sExpected = CARD_RESULT_SUCCESS;
    sBegin = port_os_cycles();
    port_os_set_timed_work(Other, sBegin + WAIT_CYCLES / 2);
    CHECK(CARD_WriteAndVerifyBackupAsync(0x1000, sBytes, TEST_BYTES, Done, sBytes));
    CHECK(CARD_WaitBackupAsync());
    CHECK(port_os_cycles() - sBegin == 3 * WAIT_CYCLES);
    CHECK(sCallbacks == 1 && sSound > 0 && sOther == 1);
    CHECK(CARD_ReadBackup(0x1000, sRead, TEST_BYTES));
    CHECK(!memcmp(sBytes, sRead, TEST_BYTES));
    SND_StopTimer(0, 0, 1 << 2, 0);
    SND_FlushCommand(SND_COMMAND_BLOCK);
}
static void Sync(void) {
    Boot();
    memcpy(port_card_page_buffer(), sBytes, 256);
    sExpected = CARD_RESULT_SUCCESS;
    sBegin = port_os_cycles();
    CHECK(CARDi_RequestStreamCommand((u32)(uintptr_t)port_card_page_buffer(), 0x1800, 0, Done, sBytes, FALSE,
                                     CARD_REQ_WRITE_BACKUP, 1, CARD_REQUEST_MODE_SPECIAL));
    CHECK(port_os_cycles() == sBegin && sCallbacks == 1);
    sCallbacks = 0;
    CHECK(CARDi_RequestStreamCommand((u32)(uintptr_t)port_card_page_buffer(), 0x1800, 256, Done, sBytes, FALSE,
                                     CARD_REQ_WRITE_BACKUP, 1, CARD_REQUEST_MODE_SPECIAL));
    CHECK(port_os_cycles() - sBegin == WAIT_CYCLES && sCallbacks == 1);
    CHECK(!memcmp(port_card_backup_image() + 0x1800, sBytes, 256));
}

static void IoError(void) {
    Boot();
    // The chip image is already loaded. Replacing the output name with a directory makes atomic file replacement
    // fail later, without inventing a cartridge error or preventing the genuine SDK service reply.
    CHECK(port_host_make_directory(sPath) == 0);
    memcpy(port_card_page_buffer(), sBytes, 256);
    sExpected = CARD_RESULT_SUCCESS;
    sBegin = port_os_cycles();
    CHECK(CARDi_RequestStreamCommand((u32)(uintptr_t)port_card_page_buffer(), 0x1800, 256, Done, sBytes, FALSE,
                                     CARD_REQ_WRITE_BACKUP, 1, CARD_REQUEST_MODE_SPECIAL));
    CHECK(port_os_wait_until(sBegin + 3 * WAIT_CYCLES));
    CHECK(sCallbacks == 1 && !memcmp(port_card_backup_image() + 0x1800, sBytes, 256));
    CHECK(rmdir(sPath) == 0);
}

static void CancelWorker(void *arg) {
    (void)arg;
    OS_Sleep(10);
    CARD_CancelBackupAsync();
}
static void Cancel(void) {
    Boot();
    sExpected = CARD_RESULT_CANCELED;
    OS_CreateThread(&sWorker, CancelWorker, NULL, sStack + sizeof(sStack), sizeof(sStack), 3);
    OS_WakeupThreadDirect(&sWorker);
    sBegin = port_os_cycles();
    CHECK(CARD_WriteAndVerifyBackupAsync(0x2000, sBytes, TEST_BYTES, Done, sBytes));
    CHECK(!CARD_WaitBackupAsync());
    CHECK(CARD_GetResultCode() == CARD_RESULT_CANCELED);
    CHECK(sCallbacks == 1 && port_os_cycles() - sBegin == WAIT_CYCLES);
    CHECK(!memcmp(port_card_backup_image() + 0x2000, sBytes, 256));
    CHECK(port_card_backup_image()[0x2100] == 0xff);
}
static void InterruptWorker(void *arg) {
    OS_Sleep(10);
    CHECK(!memcmp(port_card_backup_image() + 0x3000, sBytes, 256));
    if (arg == (void *)1)
        port_pxi_reset();
    else if (arg == (void *)2)
        port_card_set_save_path(sPath);
    port_os_stop();
}
static unsigned sResetKind;
static void Retire(void) {
    Boot();
    OS_CreateThread(&sWorker, InterruptWorker, (void *)(uintptr_t)sResetKind, sStack + sizeof(sStack), sizeof(sStack),
                    3);
    OS_WakeupThreadDirect(&sWorker);
    sExpected = CARD_RESULT_SUCCESS;
    CHECK(CARD_WriteAndVerifyBackupAsync(0x3000, sBytes, 256, Done, sBytes));
    CARD_WaitBackupAsync();
    CHECK(0); // whole-run stop does not resume or promise CARD task cleanup
}
static void AfterStop(void) {
    // Reboot of real PXI/OS: the old suspended SDK task is not resumed and no old reply may enter this bus.
    OS_InitArena();
    OS_InitLock();
    OS_InitIrqTable();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    PXI_Init();
    port_card_boot();
    OS_EnableIrq();
    OS_EnableInterrupts();
    CHECK(port_os_wait_until(2 * WAIT_CYCLES));
    CHECK(port_pxi_pending() == 0 && sCallbacks == 0);
}
static void Invalid(void) {
    Boot();
    CARDiCommandArg *cmd = cardi_common.cmd;
    cmd->src = 0;
    cmd->dst = 0;
    cmd->len = 1;
    CHECK(!CARDi_Request(&cardi_common, CARD_REQ_WRITE_BACKUP, 1));
    CHECK(cmd->result == CARD_RESULT_ERROR);
    CHECK(!CARDi_Request(&cardi_common, 31, 1));
    CHECK(cmd->result == CARD_RESULT_UNSUPPORTED);
    CHECK(!CARDi_Request(&cardi_common, 32, 1));
    CHECK(cmd->result == CARD_RESULT_UNSUPPORTED);
    cmd->type = CARD_BACKUP_TYPE_DEFINE(CARD_BACKUP_TYPE_DEVICE_FLASH, 255, 0xff);
    CHECK(!CARDi_Request(&cardi_common, CARD_REQ_IDENTIFY, 1));
    CHECK(cmd->result == CARD_RESULT_NO_RESPONSE);
}
static unsigned sErrors;
static void GameError(BOOL noResponse) {
    CHECK(!noResponse);
    ++sErrors;
}
static BOOL sFail;
static void GameWrapper(void) {
    Boot();
    SND_Init();
    SND_SetupAlarm(2, 1000, 1000, Sound, NULL);
    SND_StartTimer(0, 0, 1 << 2, 0);
    SND_FlushCommand(SND_COMMAND_BLOCK);
    sSound = 0;
    if (sFail)
        cardi_common.cmd->spec.caps &= ~(UINT32_C(1) << CARD_REQ_WRITE_BACKUP);
    sBegin = port_os_cycles();
    port_os_set_timed_work(Other, sBegin + WAIT_CYCLES / 2);
    BOOL result = writeFlashWaitSuccess(0x4000, sBytes, TEST_BYTES, GameError);
    CHECK(result == !sFail);
    CHECK(sErrors == (sFail ? 1 : 0));
    CHECK(!getLockIDStatus());
    CHECK(!(cardi_common.flag & (CARD_STAT_BUSY | CARD_STAT_REQ | CARD_STAT_TASK)));
    if (!sFail) {
        CHECK(port_os_cycles() - sBegin == 3 * WAIT_CYCLES);
        CHECK(sSound > 0 && sOther == 1);
        CHECK(readFlashWaitSuccess(0x4000, sRead, TEST_BYTES, NULL));
        CHECK(!memcmp(sBytes, sRead, TEST_BYTES));
        CHECK(!memcmp(port_card_backup_image() + 0x4000, sBytes, TEST_BYTES));
    } else
        CHECK(CARD_GetResultCode() == CARD_RESULT_UNSUPPORTED);
    SND_StopTimer(0, 0, 1 << 2, 0);
    SND_FlushCommand(SND_COMMAND_BLOCK);
}
int main(int argc, char **argv) {
    CHECK(argc == 2);
    snprintf(sPath, sizeof(sPath), "%s/backup-service-%ld.sav", port_host_temporary_directory(), (long)getpid());
    remove(sPath);
    port_card_set_save_path(sPath);
    if (!strcmp(argv[1], "game_wrapper") || !strcmp(argv[1], "game_error")) {
        sFail = !strcmp(argv[1], "game_error");
        CHECK(port_os_run(GameWrapper) == PORT_OS_RETURNED);
    } else if (!strcmp(argv[1], "stream"))
        CHECK(port_os_run(Stream) == PORT_OS_RETURNED);
    else if (!strcmp(argv[1], "sync"))
        CHECK(port_os_run(Sync) == PORT_OS_RETURNED);
    else if (!strcmp(argv[1], "io_error"))
        CHECK(port_os_run(IoError) == PORT_OS_RETURNED);
    else if (!strcmp(argv[1], "cancel"))
        CHECK(port_os_run(Cancel) == PORT_OS_RETURNED);
    else if (!strcmp(argv[1], "invalid"))
        CHECK(port_os_run(Invalid) == PORT_OS_RETURNED);
    else {
        sResetKind = !strcmp(argv[1], "bus_reset") ? 1 : !strcmp(argv[1], "path_reset") ? 2 : 0;
        CHECK(port_os_run(Retire) == PORT_OS_STOPPED);
        CHECK(sCallbacks == 0);
        CHECK(port_os_run(AfterStop) == PORT_OS_RETURNED);
    }
    port_card_set_save_path(NULL);
    remove(sPath);
    return 0;
}
