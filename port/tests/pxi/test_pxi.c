// Tests of the PXI bus and the host ARM7's services (port/hal/pxi and the services beside it). Each scenario runs as
// the game's main() would, as the DS's launcher thread on the OS core, and drives the decomp's own ARM9 libraries
// (pm.c, tp.c, nvram.c, os_reset.c, ctrdg.c, the SND commands) and the host's RTC and MIC APIs against the host's
// ARM7, checking each protocol's answers and what the ARM7 leaves in the shared memory.
//
//   port_pxi_tests NAME
//
// One scenario a process, since the libraries start once (PM_Init and the like do nothing the second time).
// PORT_PXI_VERBOSE prints each scenario's log.
#include "hal/host/host.h"
#include "hal/nvram/nvram_settings.h"
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/rtc/rtc_host.h"
#include "hal/tp/tp_host.h"
#include "nitro/ctrdg.h"
#include "nitro/gx.h"
#include "nitro/mic.h"
#include "nitro/nvram.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/rtc.h"
#include "nitro/snd.h"
#include "nitro/svc.h"
#include "nitro/tp.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LOG_MAX 256
#define LOG_LINE 96

// Tags that no library uses, for the bus's own tests
#define TAG_TEST_A 15
#define TAG_TEST_B 16

// The low 32 bits of a pointer, as the ARM9's protocols send it
#define LOW32(pointer) ((u32)(uintptr_t)(pointer))

// The scenarios' main memory arena: as large as the OS layer's, and as the PXI bus needs it (pxi_bus.h's
// port_pxi_alloc_arena)
#define TEST_ARENA_SIZE (16 * 1024 * 1024)
#define TEST_ARENA_ALIGNMENT 32

static char sLog[LOG_MAX][LOG_LINE];
static int sLogCount;
static int sFailed;

static void Log(const char *format, ...) {
    va_list args;

    if (sLogCount == LOG_MAX) {
        return;
    }
    va_start(args, format);
    vsnprintf(sLog[sLogCount++], LOG_LINE, format, args);
    va_end(args);
}

static void CheckAt(int ok, const char *what, int line) {
    if (!ok) {
        printf("  check failed (line %d): %s\n", line, what);
        sFailed = 1;
    }
}

#define CHECK(condition) CheckAt((condition) != 0, #condition, __LINE__)

static void ExpectLog(const char *const *expected, int count) {
    int i;
    int ok = sLogCount == count;

    for (i = 0; ok && i < count; i++) {
        ok = strcmp(sLog[i], expected[i]) == 0;
    }
    if (!ok) {
        printf("  log differs\n  %-50s  %s\n", "got", "expected");
        for (i = 0; i < sLogCount || i < count; i++) {
            printf("  %-50s  %s\n", i < sLogCount ? sLog[i] : "", i < count ? expected[i] : "");
        }
        sFailed = 1;
    }
}

#define EXPECT_LOG(...)                                                                                                \
    do {                                                                                                               \
        static const char *const expected[] = { __VA_ARGS__ };                                                         \
        ExpectLog(expected, (int)(sizeof(expected) / sizeof(expected[0])));                                            \
    } while (0)

// The start of every scenario, in OS_Init's order where it has one: the FIFO first (the ARM7 boots there), the
// arenas, the interrupt table, the threads, then the tick and the alarms that the libraries' waits use, and the VBlank
// interrupt that WaitFrames waits for
static void VBlankHandler(void) {
    port_os_set_irq_check_flag(OS_IE_V_BLANK);
}

// The main memory arena that the scenarios run with: memory from port_pxi_alloc_arena, in place of the OS layer's own
// block, so that no scenario depends on where the host's allocator put that one. Once the OS layer takes its block from
// port_pxi_alloc_arena itself, this only does it again
static void PlaceArena(void) {
    static u8 *sBlock;
    u8 *start;

    if (sBlock == NULL) {
        sBlock = port_pxi_alloc_arena(TEST_ARENA_SIZE + TEST_ARENA_ALIGNMENT);
        if (sBlock == NULL) {
            printf("no memory for the arena\n");
            exit(2);
        }
    }
    start = (u8 *)(((uintptr_t)sBlock + TEST_ARENA_ALIGNMENT - 1) & ~(uintptr_t)(TEST_ARENA_ALIGNMENT - 1));
    OS_SetArenaLo(OS_ARENA_MAIN, start);
    OS_SetArenaHi(OS_ARENA_MAIN, start + TEST_ARENA_SIZE);
}

static void Boot(void) {
    PXI_Init();
    OS_InitArena();
    PlaceArena();
    OS_InitIrqTable();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    OS_SetIrqFunction(OS_IE_V_BLANK, VBlankHandler);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    (void)OS_EnableIrq();
    GX_VBlankIntr(TRUE);
}

static void WaitFrames(int count) {
    int i;

    for (i = 0; i < count; i++) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
}

// The bus

static void TestBusReadyTags(void) {
    static const u32 sReady[] = { PXI_FIFO_TAG_NVRAM, PXI_FIFO_TAG_RTC, PXI_FIFO_TAG_TOUCHPANEL, PXI_FIFO_TAG_SOUND,
                                  PXI_FIFO_TAG_PM,    PXI_FIFO_TAG_MIC, PXI_FIFO_TAG_OS,         PXI_FIFO_TAG_CTRDG };
    u32 i;

    Boot();
    for (i = 0; i < NELEMS(sReady); i++) {
        CHECK(PXI_IsCallbackReady(sReady[i], PXI_PROC_ARM7));
        CHECK(!PXI_IsCallbackReady(sReady[i], PXI_PROC_ARM9));
    }
    CHECK(!PXI_IsCallbackReady(PXI_FIFO_TAG_WM, PXI_PROC_ARM7));
    CHECK(!PXI_IsCallbackReady(TAG_TEST_A, PXI_PROC_ARM7));
    port_pxi_register_service(TAG_TEST_A, NULL);
    CHECK(!PXI_IsCallbackReady(TAG_TEST_A, PXI_PROC_ARM7));
    CHECK(OS_GetSystemWork()->pxiHandleChecker[PXI_PROC_ARM9] == 0);
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_RTC, NULL);
    CHECK(OS_GetSystemWork()->pxiHandleChecker[PXI_PROC_ARM9] == 0);
}

static void LoggingCallback(u32 tag, u32 data, BOOL err) {
    Log("ARM9 tag %u data %#x err %d", tag, data, err);
}

static void LoggingService(u32 tag, u32 data, BOOL err) {
    Log("ARM7 tag %u data %#x err %d", tag, data, err);
}

// An ARM7 that answers each word with the word plus one
static void EchoService(u32 tag, u32 data, BOOL err) {
    if (!err) {
        port_pxi_reply(tag, data + 1, FALSE);
    }
}

// A word for a tag that the ARM7 has no service for comes back with the error bit; a word of the ARM7 for a tag that
// the ARM9 has no callback for goes back to the ARM7 with the error bit; the data keeps 26 bits
static void TestBusUnknownTag(void) {
    Boot();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_WM, LoggingCallback);
    Log("send WM");
    CHECK(PXI_SendWordByFifo(PXI_FIFO_TAG_WM, 0x123, FALSE) == PXI_FIFO_SUCCESS);
    Log("sent");
    port_pxi_register_service(TAG_TEST_A, LoggingService);
    port_pxi_reply(TAG_TEST_A, 0x0fedcba9, FALSE);
    Log("reply queued, %u pending", port_pxi_pending());
    PXIi_HandlerRecvFifoNotEmpty();
    Log("FIFO read, %u pending", port_pxi_pending());
    EXPECT_LOG("send WM", "ARM9 tag 10 data 0x123 err 1", "sent", "reply queued, 1 pending",
               "ARM7 tag 15 data 0x3edcba9 err 1", "FIFO read, 0 pending");
}

// An answer reaches the ARM9's callback before the send returns when interrupts are enabled; with them disabled it
// waits, until they are enabled again or the ARM9 reads the FIFO itself; answers come in order
static void TestBusReplyWhenEnabled(void) {
    u32 enabled;

    Boot();
    port_pxi_register_service(TAG_TEST_B, EchoService);
    PXI_SetFifoRecvCallback(TAG_TEST_B, LoggingCallback);
    Log("send 1");
    PXI_SendWordByFifo(TAG_TEST_B, 1, FALSE);
    Log("returned");
    enabled = OS_DisableInterrupts();
    PXI_SendWordByFifo(TAG_TEST_B, 0x10, FALSE);
    PXI_SendWordByFifo(TAG_TEST_B, 0x20, FALSE);
    Log("disabled: %u pending", port_pxi_pending());
    OS_RestoreInterrupts(enabled);
    Log("enabled: %u pending", port_pxi_pending());
    enabled = OS_DisableInterrupts();
    PXI_SendWordByFifo(TAG_TEST_B, 0x30, FALSE);
    PXIi_HandlerRecvFifoNotEmpty();
    Log("read with interrupts disabled");
    OS_RestoreInterrupts(enabled);
    Log("done");
    EXPECT_LOG("send 1", "ARM9 tag 16 data 0x2 err 0", "returned", "disabled: 2 pending", "ARM9 tag 16 data 0x11 err 0",
               "ARM9 tag 16 data 0x21 err 0", "enabled: 0 pending", "ARM9 tag 16 data 0x31 err 0",
               "read with interrupts disabled", "done");
}

static u8 sStaticBuffer[64];

// An address whose low 32 bits are in neither of port_pxi_pointer's places: past the arena by a multiple of its size,
// short of 4 GiB, and apart from the static data's low 32 bits. 0 when there is none
static uintptr_t AddressInNoPlace(const u8 *arenaStart, const u8 *arenaEnd) {
    uintptr_t size = (uintptr_t)(arenaEnd - arenaStart);
    uintptr_t step;

    for (step = 1; (step + 2) * size < ((uintptr_t)1 << 32); step++) {
        uintptr_t address = (uintptr_t)arenaEnd + step * size;

        if (port_pxi_unique_low_bits((const void *)address, (const void *)(address + 1))) {
            return address;
        }
    }
    return 0;
}

// The bus's pointers (pointer.c): the low 32 bits of an address in the static data or the main memory arena find it,
// bounded by its place; a tag's notes find their buffers, bounded by them, wherever the buffers are, and only for that
// tag; the arena that the scenarios run with (port_pxi_alloc_arena) has no low 32 bits of the static data's
static void TestBusPointer(void) {
    const u8 *end = NULL;
    const u8 *arenaEnd;
    u8 *arena;
    u8 local[16];
    uintptr_t outside;

    Boot();
    // As OS_Init's first PXI callback does, before the game takes anything from the arena
    port_pxi_observe_arena();
    arenaEnd = OS_GetArenaHi(OS_ARENA_MAIN);
    arena = OS_AllocFromArenaLo(OS_ARENA_MAIN, 256, 32);
    CHECK(arena != NULL);
    CHECK(port_pxi_unique_low_bits(arena, arenaEnd));
    CHECK(!port_pxi_unique_low_bits(sStaticBuffer, sStaticBuffer + sizeof(sStaticBuffer)));

    // The places
    CHECK(port_pxi_pointer(LOW32(sStaticBuffer), &end) == sStaticBuffer);
    CHECK(end >= sStaticBuffer + sizeof(sStaticBuffer));
    CHECK(port_pxi_pointer(LOW32(arena + 100), &end) == arena + 100);
    CHECK(end == arenaEnd);
    outside = AddressInNoPlace(arena, arenaEnd);
    CHECK(outside != 0);
    CHECK(port_pxi_pointer(LOW32(outside), &end) == NULL);

    // A noted buffer is found by any of its addresses and bounds the access to itself; another tag doesn't see the
    // note, nor does port_pxi_pointer, which find the address in its place
    port_pxi_note_pointer(TAG_TEST_A, arena + 32, 64);
    CHECK(port_pxi_noted_pointer(TAG_TEST_A, LOW32(arena + 40), &end) == arena + 40);
    CHECK(end == arena + 96);
    CHECK(port_pxi_noted_pointer(TAG_TEST_B, LOW32(arena + 40), &end) == arena + 40);
    CHECK(end == arenaEnd);
    CHECK(port_pxi_pointer(LOW32(arena + 40), &end) == arena + 40);
    CHECK(end == arenaEnd);
    // A noted pointer without a size only at its own address, bounded by its place
    port_pxi_note_pointer(TAG_TEST_A, sStaticBuffer + 8, 0);
    CHECK(port_pxi_noted_pointer(TAG_TEST_A, LOW32(sStaticBuffer + 8), &end) == sStaticBuffer + 8);
    CHECK(end >= sStaticBuffer + sizeof(sStaticBuffer));

    // A buffer in neither place, on the thread's stack: its note finds it
    port_pxi_forget_pointers();
    port_pxi_note_pointer(TAG_TEST_A, local, sizeof(local));
    CHECK(port_pxi_noted_pointer(TAG_TEST_A, LOW32(local + 4), &end) == local + 4);
    CHECK(end == local + sizeof(local));
    // The newest of the tag's notes that holds the address is the one
    port_pxi_note_pointer(TAG_TEST_A, local + 8, 4);
    CHECK(port_pxi_noted_pointer(TAG_TEST_A, LOW32(local + 9), &end) == local + 9);
    CHECK(end == local + 12);
    CHECK(port_pxi_noted_pointer(TAG_TEST_A, LOW32(local + 4), &end) == local + 4);
    CHECK(end == local + sizeof(local));
}

// Host memory whose low 32 bits are among the static data's, in another 4 GiB of the address space, or NULL
#define OVERLAP_SIZE (1024 * 1024)

static u8 *MapOverlappingBlock(void) {
    uintptr_t page = (uintptr_t)port_host_page_size();
    uintptr_t low = LOW32(sStaticBuffer) & ~(page - 1);
    uintptr_t window = (uintptr_t)sStaticBuffer & ~(uintptr_t)0xffffffff;
    uintptr_t k;

    for (k = 2; k < 256; k++) {
        void *block = port_host_map_near(window + (k << 32) + low, OVERLAP_SIZE);

        if (block == NULL) {
            continue;
        }
        if (!port_pxi_unique_low_bits(block, (u8 *)block + OVERLAP_SIZE)) {
            return block;
        }
        port_host_unmap(block, OVERLAP_SIZE);
    }
    return NULL;
}

// An arena with addresses that have the low 32 bits of static ones, which the OS layer's block can be when it doesn't
// come from port_pxi_alloc_arena: the bus stops the program as soon as it sees it, before any pointer in it could be
// taken for a static one. CTest expects pointer.c's message
static void TestBusArenaOverlap(void) {
    u8 *block = MapOverlappingBlock();

    Boot();
    CHECK(block != NULL);
    if (block == NULL) {
        return;
    }
    OS_SetArenaLo(OS_ARENA_MAIN, block);
    OS_SetArenaHi(OS_ARENA_MAIN, block + OVERLAP_SIZE);
    PXI_SetFifoRecvCallback(TAG_TEST_A, LoggingCallback);
    Log("the arena's low 32 bits went unnoticed");
    CHECK(FALSE);
}

// Power management

static void TestPmBatteryBacklight(void) {
    u32 battery = 0xff;
    u32 top = 0xff;
    u32 bottom = 0xff;

    Boot();
    PM_Init();
    CHECK(PM_GetBattery(&battery) == PM_RESULT_SUCCESS);
    CHECK(battery == PM_BATTERY_HIGH);
    CHECK(PM_GetBackLight(&top, &bottom) == PM_RESULT_SUCCESS);
    CHECK(top == PM_BACKLIGHT_ON && bottom == PM_BACKLIGHT_ON);
    CHECK(PM_SetBackLight(PM_LCD_TOP, PM_BACKLIGHT_OFF) == PM_RESULT_SUCCESS);
    CHECK(PM_GetBackLight(&top, &bottom) == PM_RESULT_SUCCESS);
    CHECK(top == PM_BACKLIGHT_OFF && bottom == PM_BACKLIGHT_ON);
    CHECK(PM_SetBackLight(PM_LCD_ALL, PM_BACKLIGHT_OFF) == PM_RESULT_SUCCESS);
    CHECK(PM_GetBackLight(&top, &bottom) == PM_RESULT_SUCCESS);
    CHECK(top == PM_BACKLIGHT_OFF && bottom == PM_BACKLIGHT_OFF);
    CHECK(PM_SetBackLight(PM_LCD_ALL, PM_BACKLIGHT_ON) == PM_RESULT_SUCCESS);
    CHECK(PM_GetBackLight(&top, &bottom) == PM_RESULT_SUCCESS);
    CHECK(top == PM_BACKLIGHT_ON && bottom == PM_BACKLIGHT_ON);
    CHECK(*(vu32 *)PORT_HW_ADDR(0x02fff684) == 0);
}

// SystemInit's sequence (machine_use.c): the amplifier is off; and the game's microphone code turns it on
static void TestPmAmp(void) {
    int amp = -1;

    Boot();
    PM_Init();
    reg_GX_POWCNT |= REG_GX_POWCNT_LCD_MASK;
    CHECK(PM_GetAmp(&amp) == PM_RESULT_SUCCESS);
    CHECK(amp == PM_AMP_OFF);
    CHECK(PM_SetAmp(PM_AMP_ON) == PM_RESULT_SUCCESS);
    CHECK(PM_GetAmp(&amp) == PM_RESULT_SUCCESS);
    CHECK(amp == PM_AMP_ON);
    CHECK(PM_SetAmpGain(1) == PM_RESULT_SUCCESS);
    CHECK(PM_SetAmpGainLevel(40) == PM_RESULT_SUCCESS);
    CHECK(PM_SetAmp(PM_AMP_OFF) == PM_RESULT_SUCCESS);
    CHECK(PM_GetAmp(&amp) == PM_RESULT_SUCCESS);
    CHECK(amp == PM_AMP_OFF);
}

// The sleep's words, as PM_GoSleepMode sends them (it can't run whole on a host: it waits for VBlanks in a loop with
// no SDK call, which nothing on the host ends, and a host's lid never closes to start it): the ARM7 prepares, takes the
// trigger and wakes the system at once. A command the ARM7 doesn't know comes back with the error bit
static void TestPmSleep(void) {
    Boot();
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_PM, LoggingCallback);
    PXI_SendWordByFifo(PXI_FIFO_TAG_PM, 0x03000000 | (0x60 << 8), FALSE);
    PXI_SendWordByFifo(PXI_FIFO_TAG_PM, 0x02000000 | (0x62 << 8) | 0x44, FALSE);
    PXI_SendWordByFifo(PXI_FIFO_TAG_PM, 0x01000000 | (1 << 16) | 0x1234, FALSE);
    PXI_SendWordByFifo(PXI_FIFO_TAG_PM, 0x03000000 | (0x55 << 8), FALSE);
    EXPECT_LOG("ARM9 tag 8 data 0x3006000 err 0", "ARM9 tag 8 data 0x3006200 err 0", "ARM9 tag 8 data 0x3006300 err 0",
               "ARM9 tag 8 data 0x3005500 err 1");
}

static void TestPmPowerOff(void) {
    Boot();
    PM_Init();
    reg_GX_POWCNT |= REG_GX_POWCNT_LCD_MASK;
    PM_ForceToPowerOff();
    Log("PM_ForceToPowerOff returned");
    sFailed = 1;
}

// The touch panel

static void TestTpSampling(void) {
    TPData raw;

    Boot();
    TP_Init();
    port_tp_set_touch(1, 100, 50);
    TP_RequestSamplingAsync();
    CHECK(TP_WaitRawResult(&raw) == 0);
    CHECK(raw.touch == 1 && raw.validity == TP_VALIDITY_VALID);
    CHECK(raw.x == 100 * 16 + 8 && raw.y == 50 * 16 + 8);
    port_tp_set_touch(0, 0, 0);
    TP_RequestSamplingAsync();
    CHECK(TP_WaitRawResult(&raw) == 0);
    CHECK(raw.touch == 0);
    port_tp_set_touch(1, 300, -5);
    TP_RequestSamplingAsync();
    CHECK(TP_WaitRawResult(&raw) == 0);
    CHECK(raw.x == 255 * 16 + 8 && raw.y == 8);
    CHECK(TP_CheckError(TP_REQUEST_COMMAND_FLAG_SAMPLING) == 0);
}

// The user settings' calibration takes every pixel of the screen to itself, through the decomp's TP code (initTouchpad
// in gfl/touchpanel.c sets it the same way)
static void TestTpCalibration(void) {
    TPCalibrateParam calibrate;
    TPData raw;
    TPData point;
    int x;
    int y;
    int wrong = 0;

    Boot();
    TP_Init();
    CHECK(TP_GetUserInfo(&calibrate) == TRUE);
    CHECK(calibrate.xDotSize != 0 && calibrate.yDotSize != 0);
    TP_SetCalibrateParam(&calibrate);
    for (y = 0; y < 192; y++) {
        for (x = 0; x < 256; x++) {
            port_tp_set_touch(1, x, y);
            TP_RequestSamplingAsync();
            if (TP_WaitRawResult(&raw) != 0) {
                wrong++;
                continue;
            }
            TP_GetCalibratedPoint(&point, &raw);
            if (point.x != x || point.y != y || point.touch != 1 || point.validity != TP_VALIDITY_VALID) {
                if (wrong < 5) {
                    Log("(%d, %d) gives (%u, %u)", x, y, point.x, point.y);
                }
                wrong++;
            }
        }
    }
    Log("calibration x0 %d y0 %d dot %d %d: %d wrong", calibrate.x0, calibrate.y0, calibrate.xDotSize,
        calibrate.yDotSize, wrong);
    CHECK(wrong == 0);
}

// The automatic sampling, as GFL's touch panel code starts it: 4 samples a frame into a ring, each a word that moves
// the latest index on
static void TestTpAutoSampling(void) {
    static TPData sSamples[9];
    TPCalibrateParam calibrate;
    TPData raw;
    TPData point;
    u16 index;

    Boot();
    TP_Init();
    TP_GetUserInfo(&calibrate);
    TP_SetCalibrateParam(&calibrate);
    port_tp_set_touch(1, 10, 20);
    TP_RequestAutoSamplingStartAsync(0, 4, sSamples, NELEMS(sSamples));
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_ON);
    CHECK(TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_ON) == 0);
    index = TP_GetLatestIndexInAuto();
    WaitFrames(1);
    CHECK(TP_GetLatestIndexInAuto() == (index + 4) % NELEMS(sSamples));
    TP_GetLatestRawPointInAuto(&raw);
    TP_GetCalibratedPoint(&point, &raw);
    CHECK(point.touch == 1 && point.x == 10 && point.y == 20);
    // A second start is refused while one runs
    TP_RequestAutoSamplingStartAsync(0, 4, sSamples, NELEMS(sSamples));
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_ON);
    CHECK(TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_ON) != 0);
    TP_RequestAutoSamplingStopAsync();
    TP_WaitBusy(TP_REQUEST_COMMAND_FLAG_AUTO_OFF);
    CHECK(TP_CheckError(TP_REQUEST_COMMAND_FLAG_AUTO_OFF) == 0);
    index = TP_GetLatestIndexInAuto();
    WaitFrames(2);
    CHECK(TP_GetLatestIndexInAuto() == index);
}

// The firmware's user settings and the NVRAM

// The firmware's CRC-16 (CRC-16/ARC), written again here so that the test doesn't take the HAL's word for it
static u16 Crc16(u16 crc, const u8 *data, u32 size) {
    u32 i;
    int bit;

    for (i = 0; i < size; i++) {
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            crc = (crc & 1) ? (u16)((crc >> 1) ^ 0xa001) : (u16)(crc >> 1);
        }
    }
    return crc;
}

static void TestNvramUserInfo(void) {
    static const u16 sNickname[] = { 'P', 'l', 'a', 'y', 'e', 'r', 0 };
    const NVRAMConfig *config = (const NVRAMConfig *)HW_NVRAM_USER_INFO;
    OSOwnerInfo info;
    OSOwnerInfoEx infoEx;
    u8 mac[6];
    static const u8 sMac[6] = { 0x02, 0x42, 0x57, 0x32, 0x00, 0x01 };

    Boot();
    OS_GetOwnerInfo(&info);
    CHECK(info.language == 1);
    CHECK(info.favoriteColor == 11);
    CHECK(info.birthday.month == 6 && info.birthday.day == 23);
    CHECK(info.nickNameLength == 6);
    CHECK(memcmp(info.nickName, sNickname, sizeof(sNickname)) == 0);
    CHECK(info.commentLength == 0 && info.comment[0] == 0);
    OS_GetOwnerInfoEx(&infoEx);
    CHECK(infoEx.language == 1 && infoEx.country == 0);
    OS_GetMacAddress(mac);
    CHECK(memcmp(mac, sMac, 6) == 0);
    CHECK(OS_GetOwnerRtcOffset() == 0);
    CHECK(config->unk_00 == 5);
    CHECK((config->unk_70 >> 16) == Crc16(0xffff, (const u8 *)config, 0x70));
    CHECK(config->touch.screenX1 == 32 && config->touch.rawX1 == 32 * 16 + 8);
}

// What NVRAM_ReadDataBytes gets: the user settings' copies, the header's fields, a slot, into static memory and into
// the arena (DWC's buffers are in its heaps); a read past the flash fails
static void TestNvramRead(void) {
    static u8 sBuffer[0x100];
    u8 *arena;
    u16 crc;

    Boot();
    NVRAM_Init();
    arena = OS_AllocFromArenaLo(OS_ARENA_MAIN, 0x100, 32);
    CHECK(NVRAM_ReadDataBytes(0x3fe00, 0x100, sBuffer) == NVRAM_RESULT_SUCCESS);
    CHECK(memcmp(sBuffer, (const void *)HW_NVRAM_USER_INFO, 0x70) == 0);
    crc = (u16)(sBuffer[0x72] | sBuffer[0x73] << 8);
    CHECK(crc == Crc16(0xffff, sBuffer, 0x70));
    CHECK(sBuffer[0x74] == 0xff);
    CHECK(NVRAM_ReadDataBytes(0x3ff00, 0x74, arena) == NVRAM_RESULT_SUCCESS);
    CHECK(memcmp(arena, sBuffer, 0x70) == 0);
    CHECK(arena[0x70] == 1 && arena[0x71] == 0);
    CHECK(NVRAM_ReadDataBytes(0x20, 2, sBuffer) == NVRAM_RESULT_SUCCESS);
    CHECK(sBuffer[0] == 0xc0 && sBuffer[1] == 0x7f);
    CHECK(NVRAM_ReadDataBytes(0x36, 6, sBuffer) == NVRAM_RESULT_SUCCESS);
    CHECK(sBuffer[0] == 0x02 && sBuffer[5] == 0x01);
    CHECK(NVRAM_ReadDataBytes(0x3fa00, 0x100, sBuffer) == NVRAM_RESULT_SUCCESS);
    CHECK(sBuffer[0xe7] == 0xff);
    CHECK((u16)(sBuffer[0xfe] | sBuffer[0xff] << 8) == Crc16(0, sBuffer, 0xfe));
    CHECK(NVRAM_ReadDataBytes(0x3fff0, 0x20, sBuffer) == NVRAM_RESULT_ILLEGAL_PARAMETER);
}

// A write across a page, which nvram.c splits and polls the flash's status for on an alarm, then a read and verify
static void TestNvramWrite(void) {
    static u8 sData[24];
    static u8 sBack[24];
    int i;

    Boot();
    NVRAM_Init();
    for (i = 0; i < (int)sizeof(sData); i++) {
        sData[i] = (u8)(0xa0 + i);
    }
    CHECK(NVRAM_WriteDataBytes(0x3faf4, sizeof(sData), sData) == NVRAM_RESULT_SUCCESS);
    CHECK(NVRAM_ReadDataBytes(0x3faf4, sizeof(sBack), sBack) == NVRAM_RESULT_SUCCESS);
    CHECK(memcmp(sData, sBack, sizeof(sData)) == 0);
    CHECK(NVRAM_VerifyDataBytes(0x3faf4, sizeof(sData), sData) == NVRAM_RESULT_SUCCESS);
    sData[20] ^= 1;
    CHECK(NVRAM_VerifyDataBytes(0x3faf4, sizeof(sData), sData) == NVRAM_RESULT_VERIFY_ERROR);
    Log("frame %u", port_os_frame());
}

// The clock

static void TestRtcFixedTime(void) {
    RTCDate date;
    RTCTime time;
    const OSSystemWork *work = OS_GetSystemWork();

    Boot();
    // The registers the ARM7 left at boot, for os_entropy.c: BCD, with the afternoon flag
    CHECK(work->realTimeClock[0] == (0x12 | 0x06 << 8 | 0x23 << 16 | 6u << 24));
    CHECK(work->realTimeClock[1] == (0x13 | 0x40 | 0x45 << 8 | 0x56 << 16));
    RTC_Init();
    CHECK(RTC_GetDateTime(&date, &time) == RTC_RESULT_SUCCESS);
    CHECK(date.year == 12 && date.month == 6 && date.day == 23 && date.week == RTC_WEEK_SATURDAY);
    CHECK(time.hour == 13 && time.minute == 45 && time.second == 56);
    CHECK(RTC_GetDate(&date) == RTC_RESULT_SUCCESS);
    CHECK(date.year == 12 && date.month == 6 && date.day == 23);
    CHECK(RTC_GetTime(&time) == RTC_RESULT_SUCCESS);
    CHECK(time.hour == 13 && time.minute == 45 && time.second == 56);
}

// The fixed clock runs with the virtual clock, over a leap day and the end of the chip's century
static void TestRtcRunsWithClock(void) {
    RTCDate date;
    RTCTime time;

    CHECK(port_rtc_set_fixed_time("2012-02-28 23:59:58"));
    Boot();
    RTC_Init();
    WaitFrames(120);
    CHECK(RTC_GetDateTime(&date, &time) == RTC_RESULT_SUCCESS);
    Log("after 120 frames: %02u-%02u-%02u %02u:%02u:%02u", (u32)date.year, (u32)date.month, (u32)date.day, time.hour,
        time.minute, time.second);
    CHECK(date.month == 2 && date.day == 29 && date.week == RTC_WEEK_WEDNESDAY);
    CHECK(time.hour == 0 && time.minute == 0 && time.second == 0);
    CHECK(!port_rtc_set_fixed_time("2012-02-30 00:00:00"));
    CHECK(!port_rtc_set_fixed_time("1999-12-31 23:59:59"));
    CHECK(!port_rtc_set_fixed_time("2012-02-28 23:59"));
    CHECK(port_rtc_set_fixed_time("2099-12-31 23:59:59"));
    WaitFrames(60);
    CHECK(RTC_GetDateTime(&date, &time) == RTC_RESULT_SUCCESS);
    CHECK(date.year == 0 && date.month == 1 && date.day == 1);
}

static void LogRtcDone(int result, void *arg) {
    Log("callback %d (%s)", result, (const char *)arg);
}

// The asynchronous reads call back from the interrupt: before the call returns when interrupts are enabled, when they
// are enabled again otherwise; a read while one runs is busy
static void TestRtcAsync(void) {
    RTCDate date;
    RTCTime time;
    u32 enabled;

    Boot();
    RTC_Init();
    Log("async");
    CHECK(RTC_GetDateTimeAsync(&date, &time, LogRtcDone, "enabled") == RTC_RESULT_SUCCESS);
    Log("returned");
    enabled = OS_DisableInterrupts();
    CHECK(RTC_GetDateAsync(&date, LogRtcDone, "disabled") == RTC_RESULT_SUCCESS);
    CHECK(RTC_GetTimeAsync(&time, LogRtcDone, "second") == RTC_RESULT_BUSY);
    Log("returned with interrupts disabled");
    OS_RestoreInterrupts(enabled);
    Log("enabled");
    EXPECT_LOG("async", "callback 0 (enabled)", "returned", "returned with interrupts disabled",
               "callback 0 (disabled)", "enabled");
}

static void TestRtcHostTime(void) {
    RTCDate date;
    RTCTime clock;
    time_t now;
    struct tm local;

    Boot();
    RTC_Init();
    CHECK(RTC_GetDateTime(&date, &clock) == RTC_RESULT_SUCCESS);
    now = time(NULL);
    // Standard C's localtime, which every host has; the test has one thread
    local = *localtime(&now);
    // The same date unless the host's clock passed midnight in between
    CHECK((date.year == (u32)(local.tm_year - 100) && date.month == (u32)local.tm_mon + 1 &&
           date.day == (u32)local.tm_mday) ||
          (local.tm_hour == 0 && local.tm_min == 0));
    CHECK(date.week == (RTCWeek)local.tm_wday || (local.tm_hour == 0 && local.tm_min == 0));
}

// The OS tag and the cartridge slot

static void TestResetCommand(void) {
    Boot();
    OS_InitReset();
    CHECK(!OS_IsResetOccurred());
    OSi_SendToPxi(OS_PXI_COMMAND_RESET);
    CHECK(OS_IsResetOccurred());
}

static void TestCtrdgEmpty(void) {
    u32 words[4] = { 1, 2, 3, 4 };
    u32 fill = 0xabcd1234;
    u16 halves[3] = { 1, 2, 3 };

    Boot();
    CTRDG_Init();
    CHECK(PXI_IsCallbackReady(PXI_FIFO_TAG_CTRDG, PXI_PROC_ARM9));
    CHECK(CTRDG_MODULE_INFO->romCheck == 0xffff);
    CHECK(!CTRDG_IsExisting());
    SVC_CpuSet(&fill, words, SVC_CPUSET_FILL | SVC_CPUSET_32BIT | 3);
    CHECK(words[0] == fill && words[2] == fill && words[3] == 4);
    SVC_CpuSet(&halves[1], &halves[0], 1);
    CHECK(halves[0] == 2 && halves[2] == 3);
}

// The microphone hears silence: a buffer that doesn't loop fills and the full callback comes; one that loops runs on
static u8 sMicBuffer[0x400] ATTRIBUTE_ALIGN(32);
static int sMicFull;

static void MicFull(MICResult result, void *arg) {
    sMicFull++;
}

static void TestMicSilence(void) {
    MICAutoParam param;
    int frames = 0;
    u32 i;
    BOOL silent = TRUE;

    Boot();
    PM_Init();
    MIC_Init();
    memset(sMicBuffer, 0x55, sizeof(sMicBuffer));
    param.type = MIC_SAMPLING_TYPE_8BIT;
    param.buffer = sMicBuffer;
    param.size = sizeof(sMicBuffer);
    param.rate = MIC_SAMPLING_RATE_8180;
    param.loop = FALSE;
    param.fullCallback = MicFull;
    param.fullArg = NULL;
    CHECK(MIC_StartLimitedSampling(&param) == MIC_RESULT_SUCCESS);
    while (sMicFull == 0 && frames < 60) {
        WaitFrames(1);
        frames++;
    }
    // 1024 samples at 8180 Hz: 125 ms, 7.5 frames
    Log("full after %d frames", frames);
    CHECK(sMicFull == 1 && frames >= 7 && frames <= 9);
    for (i = 0; i < sizeof(sMicBuffer); i++) {
        silent &= sMicBuffer[i] == 0x80;
    }
    CHECK(silent);
    CHECK(MIC_GetLastSamplingAddress() == sMicBuffer + sizeof(sMicBuffer) - 1);
    // The shared memory's word, which OS_GetLowEntropyData mixes in: the last sample's place over mic.c's fixed base,
    // the same wherever the host put the buffer
    CHECK(OS_GetSystemWork()->micLastAddress == 0x02000000 + sizeof(sMicBuffer) - 1);
    CHECK(MIC_StopLimitedSampling() == MIC_RESULT_ILLEGAL_STATUS);

    memset(sMicBuffer, 0x55, sizeof(sMicBuffer));
    param.type = MIC_SAMPLING_TYPE_SIGNED_12BIT;
    param.loop = TRUE;
    CHECK(MIC_StartAutoSampling(&param) == MIC_RESULT_SUCCESS);
    WaitFrames(20);
    CHECK(sMicFull == 1);
    CHECK(sMicBuffer[0] == 0 && sMicBuffer[sizeof(sMicBuffer) - 1] == 0);
    CHECK(MIC_StopAutoSampling() == MIC_RESULT_SUCCESS);
}

// Sound

// The packets are done as they are sent: the shared work's count follows the queue's tags, and taking every command of
// the pool back never waits
static void TestSndInit(void) {
    int i;
    u32 tag;

    Boot();
    SND_Init();
    CHECK(SNDi_SharedWork != NULL);
    CHECK(SNDi_GetFinishedCommandTag() == 1);
    CHECK(SND_GetPlayerStatus() == 0 && SND_GetChannelStatus() == 0);
    for (i = 0; i < 1000; i++) {
        SND_SetMasterVolume(i & 0x7f);
        if (i % 100 == 99) {
            SND_FlushCommand(SND_COMMAND_NOBLOCK);
        }
    }
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK | SND_COMMAND_IMMEDIATE);
    SND_WaitForCommandProc(tag);
    CHECK(SND_IsFinishedCommandTag(tag));
    Log("finished %u, frame %u", SNDi_GetFinishedCommandTag(), port_os_frame());
    CHECK(port_os_frame() == 0);
}

// Sequences in static memory, as the game's sound heap is: a wait of 96 ticks (a second at 120) and the end; a loop
// of waits; and three tracks that the first opens, the longest of which waits 144 ticks. The driver's first periodic
// update comes 0x10000 ticks of 64 cycles (7.5 frames) after it starts, and a player's tick counter doesn't count the
// tick it ends on
static u8 sSeqEnds[] = { 0xe1, 120, 0, 0x80, 96, 0xff };
static u8 sSeqLoops[] = { 0x80, 10, 0x94, 0, 0, 0 };
static u8 sSeqTracks[] = {
    0xfe, 0x07, 0x00,                         // tracks 0, 1 and 2
    0x93, 1,    16,   0,    0,                // track 1 at 16
    0x93, 2,    23,   0,    0,                // track 2 at 23
    0x80, 48,   0xff,                         // track 0 waits 48 and ends
    0xc7, 0,    0x30, 100,  0x81, 0x01, 0xff, // track 1, at 16: note-wait off, a note of 129 that doesn't wait, the end
    0xd4, 3,    0x80, 0x30, 0xfc, 0xff,       // track 2, at 23: a loop of 3 waits of 48, the end
};

// Variables, a comparison and a conditional jump: a count of 3 waits of 24 ticks, which the ARM9 sees run down in the
// shared work
static u8 sSeqVariables[] = {
    0xb0, 0,    3, 0,    // variable 0 = 3
    0x80, 24,            // 4: wait 24
    0xb2, 0,    1, 0,    // variable 0 -= 1
    0xbd, 0,    0, 0,    // compare: variable 0 != 0
    0xa2, 0x94, 4, 0, 0, // if so, jump to 4
    0xff,
};

// Shifts by amounts of 32 or more and of 256, as the ARM's register shifts do them, and ordinary ones
static u8 sSeqShifts[] = {
    0xb0, 0, 3,    0,    // variable 0 = 3
    0xb5, 0, 40,   0,    // << 40: 0
    0xb0, 1, 0xfb, 0xff, // variable 1 = -5
    0xb5, 1, 0xd8, 0xff, // >> 40: the sign, -1
    0xb0, 2, 3,    0,    // variable 2 = 3
    0xb5, 2, 0,    1,    // << 256: the amount's low byte is 0, 3
    0xb0, 3, 0x40, 0,    // variable 3 = 0x40
    0xb5, 3, 4,    0,    // << 4: 0x400
    0xb0, 4, 0,    0x80, // variable 4 = -32768
    0xb5, 4, 0xff, 0xff, // >> 1: -16384
    0x80, 1, 0xff,
};

// A sequence that leads out of its memory, which the test puts at the top of the arena: a track opened past it, which
// stays closed; a wait of 10 ticks; a call past it, which ends the track
static const u8 sSeqOutside[] = {
    0x93, 1,    0xf0, 0xff, 0xff, // track 1 at 0xfffff0
    0x80, 10,                     // wait 10
    0x95, 0xff, 0xff, 0xff,       // call 0xffffff
};

static void StartSequence(int player, u8 *data, u32 offset) {
    u32 tag;

    SND_PrepareSeq(player, data, offset, NULL);
    SND_StartPreparedSeq(player);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
}

static void TestSndSequenceEnds(void) {
    u32 start;

    Boot();
    SND_Init();
    StartSequence(3, sSeqEnds, 0);
    start = port_os_frame();
    CHECK(SND_GetPlayerStatus() == 1u << 3);
    WaitFrames(55);
    CHECK(SND_GetPlayerStatus() == 1u << 3);
    while (SND_GetPlayerStatus() != 0 && port_os_frame() - start < 120) {
        WaitFrames(1);
    }
    Log("ended after %u frames, %u ticks", port_os_frame() - start, SND_GetPlayerTickCounter(3));
    CHECK(port_os_frame() - start >= 67 && port_os_frame() - start <= 69);
    CHECK(SND_GetPlayerTickCounter(3) == 96);
}

static void TestSndSequenceLoops(void) {
    u32 tag;

    Boot();
    SND_Init();
    StartSequence(0, sSeqLoops, 0);
    WaitFrames(300);
    CHECK(SND_GetPlayerStatus() == 1);
    CHECK(SND_GetPlayerTickCounter(0) > 400);
    SND_PauseSeq(0, TRUE);
    SND_FlushCommand(SND_COMMAND_BLOCK);
    tag = SND_GetPlayerTickCounter(0);
    WaitFrames(10);
    CHECK(SND_GetPlayerTickCounter(0) == tag);
    SND_StopSeq(0);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
    CHECK(SND_GetPlayerStatus() == 0);
}

static void TestSndSequenceTracks(void) {
    u32 start;

    Boot();
    SND_Init();
    StartSequence(1, sSeqTracks, 0);
    start = port_os_frame();
    while (SND_GetPlayerStatus() != 0 && port_os_frame() - start < 200) {
        WaitFrames(1);
    }
    // Track 2's three waits of 48 ticks: 1.5 s
    Log("ended after %u frames, %u ticks", port_os_frame() - start, SND_GetPlayerTickCounter(1));
    CHECK(SND_GetPlayerTickCounter(1) == 144);
    CHECK(port_os_frame() - start >= 97 && port_os_frame() - start <= 99);
}

static void TestSndSequenceVariables(void) {
    u32 start;

    Boot();
    SND_Init();
    SNDi_SharedWork->player[2].variable[0] = 77;
    StartSequence(2, sSeqVariables, 0);
    start = port_os_frame();
    CHECK(SNDi_SharedWork->player[2].variable[0] == SND_VARIABLE_UNSET);
    WaitFrames(8);
    CHECK(SNDi_SharedWork->player[2].variable[0] == 3);
    while (SND_GetPlayerStatus() != 0 && port_os_frame() - start < 120) {
        WaitFrames(1);
    }
    Log("ended after %u frames, %u ticks", port_os_frame() - start, SND_GetPlayerTickCounter(2));
    CHECK(SND_GetPlayerTickCounter(2) == 72);
    CHECK(SNDi_SharedWork->player[2].variable[0] == 0);
}

static void TestSndSequenceShifts(void) {
    u32 start;

    Boot();
    SND_Init();
    StartSequence(4, sSeqShifts, 0);
    start = port_os_frame();
    while (SND_GetPlayerStatus() != 0 && port_os_frame() - start < 60) {
        WaitFrames(1);
    }
    CHECK(SND_GetPlayerStatus() == 0);
    CHECK(SNDi_SharedWork->player[4].variable[0] == 0);
    CHECK(SNDi_SharedWork->player[4].variable[1] == -1);
    CHECK(SNDi_SharedWork->player[4].variable[2] == 3);
    CHECK(SNDi_SharedWork->player[4].variable[3] == 0x400);
    CHECK(SNDi_SharedWork->player[4].variable[4] == -16384);
}

// The jumps, calls and tracks that lead out of the sequence's memory end there, without reading past it
static void TestSndSequenceOutside(void) {
    u8 *data;
    u32 start;

    Boot();
    SND_Init();
    // The last bytes of the arena, which the scenario takes from its top
    data = (u8 *)OS_GetArenaHi(OS_ARENA_MAIN) - sizeof(sSeqOutside);
    OS_SetArenaHi(OS_ARENA_MAIN, data);
    memcpy(data, sSeqOutside, sizeof(sSeqOutside));
    StartSequence(5, data, 0);
    start = port_os_frame();
    while (SND_GetPlayerStatus() != 0 && port_os_frame() - start < 60) {
        WaitFrames(1);
    }
    Log("ended after %u frames, %u ticks", port_os_frame() - start, SND_GetPlayerTickCounter(5));
    CHECK(SND_GetPlayerStatus() == 0);
    CHECK(SND_GetPlayerTickCounter(5) == 10);
}

static void InvalidateSeq(const void *start, const void *end, u32 flushFlags) {
    u32 tag;

    SND_InvalidateSeqData(start, end);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(flushFlags);
    SND_WaitForCommandProc(tag);
}

// SND_InvalidateSeqData stops the players with a track reading in the range, and no other. The DS's driver compares a
// track's position with both ends of the range inclusively (SND_InvalidateSeq's cmp and cmpls), so the byte at the end
// counts too: a range given as its data's end, one past the last byte, also stops a track that reads the next object's
// first byte. Here sSeqLoops follows sSeqEnds in an optimized build, so all of sSeqEnds, end included, stops player 0
// as it would on the DS; the test's ranges don't depend on where the compiler puts the arrays. Two frames in, each
// track still reads its sequence's first byte (the driver's first periodic update comes at 7.5). The driver's
// playerStatus follows at its next run: the wake-up that SND_WaitForCommandProc sends while the driver holds the
// packet, as the DS's does, or SND_COMMAND_IMMEDIATE's
static void TestSndInvalidate(void) {
    Boot();
    SND_Init();
    StartSequence(0, sSeqLoops, 0);
    StartSequence(1, sSeqVariables, 0);
    WaitFrames(2);
    CHECK(SND_GetPlayerStatus() == 3);
    // sSeqEnds up to its last byte: no track reads there
    InvalidateSeq(sSeqEnds, sSeqEnds + sizeof(sSeqEnds) - 1, SND_COMMAND_BLOCK);
    CHECK(SND_GetPlayerStatus() == 3);
    // An empty range at player 0's position: its end counts, so player 0 stops and player 1 plays on
    InvalidateSeq(sSeqLoops, sSeqLoops, SND_COMMAND_BLOCK);
    CHECK(SND_GetPlayerStatus() == 2);
    // All of player 1's sequence
    InvalidateSeq(sSeqVariables, sSeqVariables + sizeof(sSeqVariables), SND_COMMAND_BLOCK | SND_COMMAND_IMMEDIATE);
    CHECK(SND_GetPlayerStatus() == 0);
}

static int sAlarms;

static void AlarmHandler(void *arg) {
    sAlarms++;
}

// An alarm of 8000 ticks of 64 cycles, as the stream player sets them: 512,000 cycles, a little less than a frame
static void TestSndAlarm(void) {
    int before;

    Boot();
    SND_Init();
    SND_SetupAlarm(2, 8000, 8000, AlarmHandler, NULL);
    SND_StartTimer(0, 0, 1 << 2, 0);
    SND_FlushCommand(SND_COMMAND_BLOCK);
    WaitFrames(20);
    Log("%d alarms in 20 frames", sAlarms);
    CHECK(sAlarms >= 20 && sAlarms <= 22);
    SND_StopTimer(0, 0, 1 << 2, 0);
    SND_FlushCommand(SND_COMMAND_BLOCK);
    before = sAlarms;
    WaitFrames(5);
    CHECK(sAlarms == before);
}

static SNDDriverInfo sDriverInfo;

// The driver's state: a player that never played has the zeros the ARM7's work starts with (track 0 for each of its
// tracks, as on the DS); one that plays has its tracks, and a track's channel
static void TestSndDriverInfo(void) {
    SNDTrackInfo track;
    u32 tag;

    Boot();
    SND_Init();
    StartSequence(3, sSeqEnds, 0);
    WaitFrames(8);
    SND_ReadDriverInfo(&sDriverInfo);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
    CHECK(sDriverInfo.player[0].trackIndex[0] == 0);
    CHECK(sDriverInfo.player[3].trackIndex[1] == SND_TRACK_INDEX_NONE);
    CHECK(SND_ReadTrackInfo(&sDriverInfo, 3, 0, &track));
    CHECK(track.volume == 127 && track.pan == 64 && track.chCount == 0);
    CHECK(!SND_ReadTrackInfo(&sDriverInfo, 3, 1, &track));
}

typedef struct Test {
    const char *name;
    void (*main)(void);
} Test;

static const Test sTests[] = {
    { "bus_ready_tags", TestBusReadyTags },
    { "bus_unknown_tag", TestBusUnknownTag },
    { "bus_reply_when_enabled", TestBusReplyWhenEnabled },
    { "bus_pointer", TestBusPointer },
    { "bus_arena_overlap", TestBusArenaOverlap },
    { "pm_battery_backlight", TestPmBatteryBacklight },
    { "pm_amp", TestPmAmp },
    { "pm_sleep", TestPmSleep },
    { "tp_sampling", TestTpSampling },
    { "tp_calibration", TestTpCalibration },
    { "tp_auto_sampling", TestTpAutoSampling },
    { "nvram_user_info", TestNvramUserInfo },
    { "nvram_read", TestNvramRead },
    { "nvram_write", TestNvramWrite },
    { "rtc_fixed_time", TestRtcFixedTime },
    { "rtc_runs_with_clock", TestRtcRunsWithClock },
    { "rtc_async", TestRtcAsync },
    { "rtc_host_time", TestRtcHostTime },
    { "reset_command", TestResetCommand },
    { "ctrdg_empty", TestCtrdgEmpty },
    { "mic_silence", TestMicSilence },
    { "snd_init", TestSndInit },
    { "snd_sequence_ends", TestSndSequenceEnds },
    { "snd_sequence_loops", TestSndSequenceLoops },
    { "snd_sequence_tracks", TestSndSequenceTracks },
    { "snd_sequence_variables", TestSndSequenceVariables },
    { "snd_sequence_shifts", TestSndSequenceShifts },
    { "snd_sequence_outside", TestSndSequenceOutside },
    { "snd_invalidate", TestSndInvalidate },
    { "snd_alarm", TestSndAlarm },
    { "snd_driver_info", TestSndDriverInfo },
};

static void PrintLog(void) {
    int i;

    for (i = 0; i < sLogCount; i++) {
        printf("  | %s\n", sLog[i]);
    }
}

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "";
    size_t i;
    PortOsResult result;

    setvbuf(stdout, NULL, _IONBF, 0);
    // A scenario that hangs reports where, before CTest's timeout kills it (the host's watchdog, port/hal/host)
    port_host_watchdog_start(50, "hang: the scenario didn't end\n", 3);
    if (strcmp(name, "pm_power_off") == 0) {
        result = port_os_run(TestPmPowerOff);
        CHECK(result == PORT_OS_STOPPED);
        printf("power off: %s\n", sFailed ? "FAIL" : "ok");
        return sFailed;
    }
    for (i = 0; i < NELEMS(sTests); i++) {
        if (strcmp(name, sTests[i].name) == 0) {
            if (strncmp(name, "rtc_", 4) == 0 && strcmp(name, "rtc_host_time") != 0 &&
                strcmp(name, "rtc_runs_with_clock") != 0) {
                port_rtc_set_fixed_time("2012-06-23 13:45:56");
            }
            result = port_os_run(sTests[i].main);
            CHECK(result == PORT_OS_RETURNED);
            printf("%s: %s\n", name, sFailed ? "FAIL" : "ok");
            if (sFailed || getenv("PORT_PXI_VERBOSE") != NULL) {
                PrintLog();
            }
            return sFailed;
        }
    }
    printf("usage: %s NAME\n", argv[0]);
    return 2;
}
