// The host's ARM7 (port/hal/pxi/arm7.c and the services of port/hal/{pm,tp,nvram,rtc,reset,ctrdg,snd7,mic}): what the
// services share. Nothing outside those directories and their tests includes it.
//
// The DS's ARM7 runs Nintendo's stock ARM7 component, which the decomp doesn't have: its services are written here
// from the protocols that the decomp's ARM9 code shows (what it sends, what answer it expects, what shared memory it
// reads), at the level of the PXI words. Each service has a start function, which arm7.c calls when the ARM9 starts
// the FIFO, and the ones that work by themselves have a frame function, which arm7.c's frame hook calls at each VBlank
// on the host's stack (it must not call the SDK: it answers with port_pxi_reply and writes the shared memory).
#ifndef PORT_HAL_PXI_ARM7_H
#define PORT_HAL_PXI_ARM7_H

#include "types.h"
#include "nitro/os.h"

// The ARM7's boot (arm7.c): the system area of the shared memory, then the services. PXI_InitFifo calls it once.
// port_arm7_reset stops its frame work until the next boot (port_pxi_reset calls it)
void port_arm7_boot(void);
void port_arm7_reset(void);

// Notes the main memory arena's extent for port_pxi_pointer (pointer.c), and stops the program when the arena shares
// low 32 bits with the static data (pxi_bus.h's port_pxi_alloc_arena). PXI_SetFifoRecvCallback calls it, since the
// libraries set their callbacks before the game takes anything from the arena. port_pxi_forget_pointers drops the
// pointers the ARM9 noted (port_pxi_reset calls it)
void port_pxi_observe_arena(void);
void port_pxi_forget_pointers(void);

// The services' start functions, which also set each service's state back for a new run
void port_pm7_start(void);
void port_tp7_start(void);
void port_nvram7_start(void);
void port_rtc7_start(void);
void port_reset7_start(void);
void port_ctrdg7_start(void);
void port_snd7_start(void);
void port_mic7_start(void);

// The services' work at each VBlank: the touch panel's automatic sampling, the sound driver's updates and alarms, the
// microphone's sampling. cycles is the virtual clock (port_os_cycles) at the VBlank
void port_tp7_frame(u64 cycles);
void port_snd7_frame(u64 cycles);
void port_mic7_frame(u64 cycles);

// The firmware's user settings that the ARM7 puts in the system area at boot (nvram7.c, from port_nvram_settings), and
// the touch screen's calibration in them, which the touch panel's raw samples follow (tp7.c)
void port_nvram7_fill_user_info(NVRAMConfig *config);
void port_tp7_calibration(NVRAMConfigTpCalibData *calibration);

// The clock chip's date and time registers as the ARM7 leaves them in the shared memory (OSSystemWork's
// realTimeClock, read by rtc_api.c and os_entropy.c), for the host's clock now (rtc7.c)
void port_rtc7_store_raw(void);

// The two halves of an ARM7 answer of the SPI libraries (PM, TP, NVRAM, MIC, RTC): the command in bits 8 to 14 and
// the result in the low byte; the ARM7's SPI answers also mark the word as both the first and the last of the answer
#define PORT_ARM7_SPI_START 0x02000000
#define PORT_ARM7_SPI_END 0x01000000
#define PORT_ARM7_SPI_ANSWER(command, result)                                                                          \
    (PORT_ARM7_SPI_START | PORT_ARM7_SPI_END | (((u32)(command) & 0x7f) << 8) | ((u32)(result) & 0xff))
// The fields of an ARM9 word of those libraries: the first word of a command has START and the command in bits 8 to
// 14, the last has END, and each word after the first carries its index in bits 16 to 22 and 16 bits of data
#define PORT_ARM7_SPI_IS_START(data) (((data) & PORT_ARM7_SPI_START) != 0)
#define PORT_ARM7_SPI_IS_END(data) (((data) & PORT_ARM7_SPI_END) != 0)
#define PORT_ARM7_SPI_COMMAND(data) (((data) >> 8) & 0x7f)
#define PORT_ARM7_SPI_INDEX(data) (((data) >> 16) & 0x7f)

// A command of the SPI libraries as the ARM7 collects its words: up to 8, the first one's low byte and the 16 bits of
// data of each after it. count is the words collected so far, words the number of the last complete command
typedef struct PortArm7SpiCommand {
    u32 command;
    u32 count;
    u32 words;
    u16 data[8];
} PortArm7SpiCommand;

// Adds a word to the command. TRUE when it was the last one, so that the command is complete, with its words in data;
// a start word begins a new command, and a word out of order (not the next index) is dropped with what came before it
BOOL port_arm7_spi_collect(PortArm7SpiCommand *collected, u32 data);

#endif // PORT_HAL_PXI_ARM7_H
