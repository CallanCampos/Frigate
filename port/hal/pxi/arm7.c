// The host's ARM7: its boot, when the ARM9 starts the PXI FIFO, and its work at each frame. On the DS the firmware
// copies the user settings into the shared memory before the game starts, the ARM7 component notes what it found in
// the cartridge slot and reads the clock, and then registers the PXI callbacks of its libraries while the ARM9 waits in
// PXI_InitFifo. Here the same happens in PXI_InitFifo (pxi_fifo.c), and each service is a file of its own:
//   NVRAM 4 (nvram7.c, with the user settings), RTC 5 (rtc7.c), TOUCHPANEL 6 (tp7.c), SOUND 7 (port/hal/snd7, the
//   sound driver), PM 8 (pm7.c), MIC 9 (mic.c), OS 12 (reset7.c), CTRDG 13 (ctrdg7.c).
// The card's tags (FS 11 and CARD 14) belong to the card's module, and the wireless and DSi ones (WM, NWM, FATFS,
// CAMERA, SCFG, SNDEX) have no service yet: a word sent with them comes back with the error bit, as from an ARM7 that
// has no callback for the tag.
#include "hal/pxi/arm7.h"
#include "hal/os/port_os.h"
#include "hal/pxi/pxi_bus.h"

// Whether the ARM7 runs (from its boot until the bus is reset), and whether its frame hook was added: the OS core
// keeps the hooks for the whole process
static BOOL sBooted;
static BOOL sFrameHookAdded;

// The ARM7's work at each VBlank, on the host's stack
static void Arm7Frame(void) {
    u64 cycles;

    if (!sBooted) {
        return;
    }
    cycles = port_os_cycles();
    port_tp7_frame(cycles);
    port_snd7_frame(cycles);
    port_mic7_frame(cycles);
}

void port_arm7_boot(void) {
    // What the firmware and the ARM7 leave in the system area: the user settings first, since the services read them
    port_nvram7_start();
    port_rtc7_start();
    port_ctrdg7_start();
    port_tp7_start();
    port_pm7_start();
    port_reset7_start();
    port_snd7_start();
    port_mic7_start();
    if (!sFrameHookAdded) {
        port_os_add_frame_hook(Arm7Frame);
        sFrameHookAdded = TRUE;
    }
    sBooted = TRUE;
}

void port_arm7_reset(void) {
    sBooted = FALSE;
}

BOOL port_arm7_spi_collect(PortArm7SpiCommand *collected, u32 data) {
    u32 index;

    if (PORT_ARM7_SPI_IS_START(data)) {
        collected->command = PORT_ARM7_SPI_COMMAND(data);
        collected->count = 1;
        collected->data[0] = data & 0xff;
    } else {
        index = PORT_ARM7_SPI_INDEX(data);
        if (collected->count == 0 || index != collected->count || index >= NELEMS(collected->data)) {
            collected->count = 0;
            return FALSE;
        }
        collected->data[index] = data & 0xffff;
        collected->count++;
    }
    if (PORT_ARM7_SPI_IS_END(data)) {
        collected->words = collected->count;
        collected->count = 0;
        return TRUE;
    }
    return FALSE;
}
