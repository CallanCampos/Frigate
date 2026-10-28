// Physical CARD READY/DMA COMMIT events through the actual NitroSDK OS scheduler and native core, not a scheduler mock.
// Synthetic immutable bytes exercise one-word service, IRQ/mask context, CPU stalls, bounds and stop/reset ownership.
#include "hal/card/port_card_hardware.h"
#include "hal/dma/card_dma.h"
#include "hal/os/os_internal.h"
#include "hal/os/port_os.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "hardware line %d: %s\n", __LINE__, #x);                                                   \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
#define TIMER3_IRQ (OS_IE_TIMER0 << 3)
#define TIMER2_IRQ (OS_IE_TIMER0 << 2)
static u8 sBytes[4096];
static u8 *sOut;
static unsigned sReads, sIrqs, sGeneric, sNested;
static u64 sIrqAt, sGenericAt, sStart;
static BOOL sInside;
static BOOL sObserveTimer;
static u64 sTimerLatchedAt;
static bool Read(void *context, uint32_t address, uint8_t bytes[4]) {
    CHECK(context == sBytes && address <= sizeof(sBytes) - 4);
    ++sReads;
    if (sObserveTimer) {
        CHECK((port_os_get_if() & TIMER3_IRQ) && sIrqs == 0 && sGeneric == 0);
        sTimerLatchedAt = port_os_cycles();
    }
    memcpy(bytes, sBytes + address, 4);
    return true;
}
static bool Id(void *context, uint32_t *value) {
    CHECK(context == sBytes);
    *value = 0x1fc2;
    return true;
}
static void InitDevice(void) {
    port_card_hardware_test_init((PortCardBusBackend){ sBytes, Read, Id });
    sReads = 0;
}
static void Boot(void) {
    OS_InitArena();
    OS_InitLock();
    OS_InitIrqTable();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    OS_EnableIrq();
    OS_EnableInterrupts();
    sOut = OS_AllocFromArenaLo(OS_ARENA_MAIN, 2048, 32);
    CHECK(sOut);
    memset(sOut, 0xaa, 2048);
    InitDevice();
    sStart = port_os_cycles();
    printf("high main-arena destination: %s\n", (uintptr_t)sOut > UINT32_MAX ? "yes" : "low allocator");
}
static void Start(u32 length, unsigned l1, unsigned l2) {
    PortCardBusRequest req = { PORT_CARD_BUS_READ, 256, length, (u16)l1, (u8)l2, 5, true };
    CHECK(port_card_hardware_test_start(&req, port_os_cycles()) == PORT_CARD_BUS_OK);
}
static void Pacing(void) {
    Boot();
    CHECK(port_card_dma_arm_main(1, sOut, 1024));
    Start(1024, 3, 2);
    CHECK(port_os_wait_until(sStart + 84));
    CHECK(sReads == 0 && sOut[0] == 0xaa);
    CHECK(port_os_wait_until(sStart + 85)); // READY85 stalls ARM9 through COMMIT94.
    CHECK(port_os_cycles() == sStart + 94 && sReads == 1);
    CHECK(port_card_dma_state().written == 4 && !memcmp(sOut, sBytes + 256, 4) && sOut[4] == 0xaa);
    CHECK(!(port_os_get_if() & OS_IE_CARD_DATA));
    CHECK(port_os_wait_until(sStart + 113));
    CHECK(sReads == 1);
    CHECK(port_os_wait_until(sStart + 7499));
    PortCardHardwareState hw = port_card_hardware_state();
    CHECK(!hw.busy && !hw.ready && !hw.stalled && hw.event == PORT_CARD_HW_NONE);
    CHECK(hw.last_commit == sStart + 7499 && hw.commits == 256 && sReads == 256);
    CHECK(!memcmp(sOut, sBytes + 256, 1024) && sOut[1024] == 0xaa);
    CHECK((port_os_get_if() & OS_IE_CARD_DATA) && !(port_os_get_if() & (15u << 8)));
    CHECK(port_os_stats().deviceStallCycles == 256 * 9);
    CHECK(port_card_dma_state().armed && port_card_dma_state().remaining == 0);
    MI_StopDma(1);
    CHECK(!port_card_dma_state().armed);
}
static void Absolute(void) {
    Boot();
    CHECK(port_card_dma_arm_main(0, sOut, 512));
    Start(512, 3, 2);
    CHECK(port_os_wait_until(sStart + 4000));
    CHECK(port_os_cycles() == sStart + 4000 && port_card_hardware_state().last_commit == sStart + 3777);
    CHECK(port_os_stats().deviceStallCycles == 128 * 9);
}
static void OwnWork(void) {
    Boot();
    CHECK(port_card_dma_arm_main(0, sOut, 512));
    Start(512, 3, 2);
    port_os_spin(4000);
    CHECK(port_os_cycles() == sStart + 4000 + 128 * 9 && port_card_hardware_state().last_commit == sStart + 3777);
    CHECK(port_os_stats().deviceStallCycles == 128 * 9);
}
static void CardIrq(void) {
    CHECK(!port_card_hardware_stalled());
    CHECK(OS_GetProcMode() == OS_PROCMODE_IRQ);
    sIrqAt = port_os_cycles();
    ++sIrqs;
}
static void Masked(void) {
    Boot();
    for (unsigned mode = 0; mode < 3; ++mode) {
        InitDevice();
        memset(sOut, 0xaa, 16);
        sIrqs = 0;
        OS_SetIrqFunction(OS_IE_CARD_DATA, CardIrq);
        OS_EnableIrqMask(OS_IE_CARD_DATA);
        u32 saved = 0;
        if (mode == 0) {
            saved = OS_DisableInterrupts();
        }
        if (mode == 1) {
            OS_DisableIrq();
        }
        if (mode == 2) {
            OS_DisableIrqMask(OS_IE_CARD_DATA);
        }
        u64 start = port_os_cycles();
        CHECK(port_card_dma_arm_main(2, sOut, 4));
        Start(4, 0, 0);
        CHECK(port_os_wait_until(start + 100));
        CHECK(sIrqs == 0 && (port_os_get_if() & OS_IE_CARD_DATA));
        CHECK(!memcmp(sOut, sBytes + 256, 4) && port_card_hardware_state().last_commit == start + 69);
        if (mode == 0) {
            OS_RestoreInterrupts(saved);
        }
        if (mode == 1) {
            OS_EnableIrq();
        }
        if (mode == 2) {
            OS_EnableIrqMask(OS_IE_CARD_DATA);
        }
        port_os_poll_irq();
        CHECK(sIrqs == 1 && sIrqAt == start + 100);
    }
}
static void Generic(void) {
    CHECK(!port_card_hardware_stalled());
    sGenericAt = port_os_cycles();
    ++sGeneric;
}
static void DuringStallIrq(void) {
    port_osi_stop_timer(3);
    CHECK(!port_card_hardware_stalled() && !memcmp(sOut, sBytes + 256, 4));
    sIrqAt = port_os_cycles();
    ++sIrqs;
}
static void TimerStall(void) {
    Boot();
    sIrqs = sGeneric = 0;
    OS_SetIrqFunction(TIMER3_IRQ, DuringStallIrq);
    OS_EnableIrqMask(TIMER3_IRQ);
    port_osi_start_timer(3, 0x10000 - 64, 1, TRUE);
    port_os_set_timed_work(Generic, sStart + 64);
    CHECK(port_card_dma_arm_main(0, sOut, 4));
    Start(4, 0, 0);
    CHECK(port_os_wait_until(sStart + 100));
    CHECK(sIrqs == 1 && sIrqAt == sStart + 69);
    CHECK(sGeneric == 1 && sGenericAt == sStart + 69);
}
static void EqualCycle(void) {
    Boot();
    sIrqs = sGeneric = 0;
    sObserveTimer = TRUE;
    sTimerLatchedAt = 0;
    OS_SetIrqFunction(TIMER3_IRQ, DuringStallIrq);
    OS_EnableIrqMask(TIMER3_IRQ);
    port_osi_start_timer(3, 0x10000 - 64, 1, TRUE);
    port_os_set_timed_work(Generic, sStart + 64);
    CHECK(port_os_wait_until(sStart + 4));
    CHECK(port_card_dma_arm_main(0, sOut, 4));
    Start(4, 0, 0); // Start4 + READY60 =64, exactly the timer and generic-host event cycle.
    port_os_spin(100);
    CHECK(sTimerLatchedAt == sStart + 64);
    CHECK(sIrqs == 1 && sIrqAt == sStart + 73);
    CHECK(sGeneric == 1 && sGenericAt == sStart + 73);
    CHECK(port_os_cycles() == sStart + 113 && port_os_stats().deviceStallCycles == 9);
    sObserveTimer = FALSE;
}
static void Nested(void) {
    port_osi_stop_timer(2);
    CHECK(!sInside && !port_card_hardware_stalled());
    ++sNested;
}
static void LongIrq(void) {
    port_osi_stop_timer(3);
    sInside = TRUE;
    u64 start = port_os_cycles();
    CHECK(port_card_dma_arm_main(0, sOut, 512));
    Start(512, 3, 2);
    port_osi_start_timer(2, 0x10000 - 89, 1, TRUE); // READY85, timer89, first COMMIT94.
    port_os_set_timed_work(Generic, start + 89);
    port_os_spin(4000); // The old IRQ Spin must progress hardware without running generic callbacks.
    CHECK(port_os_cycles() == start + 5152 && port_card_hardware_state().commits == 128);
    CHECK(port_card_hardware_state().last_commit == start + 3777 && !memcmp(sOut, sBytes + 256, 512));
    CHECK(sGeneric == 0 && sNested == 0 && (port_os_get_if() & TIMER2_IRQ));
    sInside = FALSE;
}
static void IrqSpin(void) {
    Boot();
    sGeneric = sNested = 0;
    sInside = FALSE;
    OS_SetIrqFunction(TIMER3_IRQ, LongIrq);
    OS_SetIrqFunction(TIMER2_IRQ, Nested);
    OS_EnableIrqMask(TIMER3_IRQ | TIMER2_IRQ);
    port_osi_start_timer(3, 0x10000 - 100, 1, TRUE);
    CHECK(port_os_wait_until(sStart + 6000));
    CHECK(port_os_cycles() == sStart + 6000 && sNested == 1 && sGeneric == 1);
    CHECK(sGenericAt == sStart + 5252 && port_os_stats().deviceStallCycles == 1152);
}
static void Halt(void) {
    Boot();
    sIrqs = 0;
    OS_SetIrqFunction(OS_IE_CARD_DATA, CardIrq);
    OS_EnableIrqMask(OS_IE_CARD_DATA);
    CHECK(port_card_dma_arm_main(0, sOut, 4));
    Start(4, 0, 0);
    OS_Halt();
    CHECK(sIrqs == 1 && sIrqAt == sStart + 69 && port_os_cycles() == sStart + 69);
    CHECK(!memcmp(sOut, sBytes + 256, 4));
}
static void Bounds(void) {
    Boot();
    u8 unknown[64];
    CHECK(!port_card_dma_arm_main(4, sOut, 4));
    CHECK(!port_card_dma_arm_main(0, unknown, 4));
    CHECK(!port_card_dma_arm_main(0, sOut + 1, 4));
    CHECK(!port_card_dma_arm_main(0, sOut, 0));
    CHECK(!port_card_dma_arm_main(0, sOut, 3));
    CHECK(port_card_dma_arm_main(1, sOut, 4));
    CHECK(!port_card_dma_arm_main(2, sOut + 32, 4));
    Start(512, 0, 0);
    CHECK(port_os_wait_until(sStart + 1000));
    CHECK(port_card_dma_state().written == 4 && !memcmp(sOut, sBytes + 256, 4));
    CHECK(sOut[4] == 0xaa && port_card_hardware_state().ready && port_card_hardware_state().busy);
    CHECK(!(port_os_get_if() & OS_IE_CARD_DATA));
    port_card_hardware_cancel();
}
static void Cancel(void) {
    Boot();
    CHECK(port_card_dma_arm_main(1, sOut, 512));
    Start(512, 0, 0);
    MI_StopDma(1);
    CHECK(port_os_wait_until(sStart + 1000));
    CHECK(port_card_hardware_state().ready && !port_card_dma_state().armed && !port_card_dma_state().written);
    CHECK(sOut[0] == 0xaa && !(port_os_get_if() & OS_IE_CARD_DATA));
    port_card_hardware_cancel();
    CHECK(port_card_dma_arm_main(1, sOut, 4));
    MIi_DmaCopy32(1, sBytes, sOut, 0, TRUE); // A zero-length no-op cannot steal ownership or write memory.
    CHECK(port_card_dma_state().armed && sOut[0] == 0xaa);
    uint64_t old;
    CHECK(port_card_dma_demand(&old));
    MI_StopAllDma();
    CHECK(port_card_dma_arm_main(1, sOut + 32, 4));
    uint64_t current;
    CHECK(port_card_dma_demand(&current));
    CHECK(current != old);
    CHECK(!port_card_dma_commit(old, 0));
    CHECK(sOut[32] == 0xaa);
    port_card_hardware_cancel();
    // Exact reset ABA regression: an old token must never authorize a new destination after reset.
    CHECK(port_card_dma_arm_main(1, sOut + 64, 4));
    CHECK(port_card_dma_demand(&old));
    port_card_dma_reset();
    CHECK(port_card_dma_arm_main(1, sOut + 96, 4));
    CHECK(port_card_dma_demand(&current));
    CHECK(current != old);
    CHECK(!port_card_dma_commit(old, 0x11223344));
    CHECK(sOut[96] == 0xaa && port_card_dma_state().written == 0);
    CHECK(port_card_dma_commit(current, 0x55667788));
    CHECK(sOut[96] == 0x88 && sOut[97] == 0x77 && port_card_dma_state().written == 4);
    port_card_hardware_cancel();
}
static void GenericConflict(void) {
    Boot();
    CHECK(port_card_dma_arm_main(1, sOut, 512));
    MIi_DmaCopy32(1, sBytes, sOut, 4, TRUE);
    CHECK(FALSE);
}
static void Inactive(void) {
    Boot();
    MIi_DmaCopy32(1, sBytes, sOut, 512, TRUE);
    CHECK(!memcmp(sOut, sBytes, 512) && port_os_cycles() == sStart);
    MI_WaitDma(1);
    port_os_spin(4000);
    CHECK(port_os_cycles() == sStart + 4000 && port_os_stats().deviceStallCycles == 0);
}
static void StopWork(void) {
    port_os_stop();
}
static void StopBoot(void) {
    Boot();
    CHECK(port_card_dma_arm_main(1, sOut, 512));
    Start(512, 0, 0);
    port_os_set_timed_work(StopWork, sStart + 65); // During first service; host stop is delivered only after COMMIT69.
    port_os_spin(1000);
    CHECK(FALSE);
}
static void AfterStop(void) {
    OS_InitThread();
    OS_InitIrqTable();
    OS_EnableIrq();
    OS_EnableInterrupts();
    CHECK(port_card_hardware_state().event == PORT_CARD_HW_NONE && !port_card_dma_state().armed);
    CHECK(port_os_wait_until(10000));
    CHECK(!memcmp(sOut, sBytes + 256, 4));
    for (unsigned i = 4; i < 512; ++i) {
        CHECK(sOut[i] == 0xaa);
    }
}
int main(int argc, char **argv) {
    for (unsigned i = 0; i < sizeof(sBytes); ++i) {
        sBytes[i] = (u8)(i * 13 + (i >> 8));
    }
    const char *name = argc > 1 ? argv[1] : "pacing";
    void (*run)(void) = Pacing;
    if (!strcmp(name, "absolute")) {
        run = Absolute;
    } else if (!strcmp(name, "own_work")) {
        run = OwnWork;
    } else if (!strcmp(name, "masked")) {
        run = Masked;
    } else if (!strcmp(name, "timer_stall")) {
        run = TimerStall;
    } else if (!strcmp(name, "irq_spin")) {
        run = IrqSpin;
    } else if (!strcmp(name, "equal_cycle")) {
        run = EqualCycle;
    } else if (!strcmp(name, "halt")) {
        run = Halt;
    } else if (!strcmp(name, "bounds")) {
        run = Bounds;
    } else if (!strcmp(name, "cancel")) {
        run = Cancel;
    } else if (!strcmp(name, "inactive")) {
        run = Inactive;
    } else if (!strcmp(name, "generic_conflict")) {
        run = GenericConflict;
    }
    if (!strcmp(name, "reset")) {
        CHECK(port_os_run(StopBoot) == PORT_OS_STOPPED);
        CHECK(port_card_hardware_state().event == PORT_CARD_HW_NONE && !port_card_dma_state().armed);
        CHECK(port_os_run(AfterStop) == PORT_OS_RETURNED);
    } else {
        CHECK(port_os_run(run) == PORT_OS_RETURNED);
    }
    printf("hardware %s PASS\n", name);
    return 0;
}
