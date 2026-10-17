// The host ARM7's power management (PXI tag PM), for the decomp's lib/nitro/src/spi/pm.c. Its protocol, from pm.c: a
// command is one or two words, the first with the command in bits 8 to 14 and a byte of data, the second with 16 bits
// of data; the ARM7 answers with the command and a byte, or with the error bit when it can't take the command (pm.c
// gives PM_RESULT_BUSY then for a request or the sleep, PM_RESULT_ERROR otherwise). A request (0x61) carries what is
// asked in the first byte and its argument in the second word, and its answer's byte is the value read.
//
// What the host's console is: the battery is good, both backlights are on until the game turns them off, the screens'
// power follows the game's requests, and the sound amplifier, its gain and the LED request keep what the game sets.
// The DS's ARM7 also notifies the ARM9 of a low battery and of the DSi's exit requests; the host never does, so the
// exit and battery callbacks stay inert. The sleep has no lid to wait for: the system wakes up right after the ARM9
// sends the trigger, so PM_GoSleepMode returns at once (the game sleeps only when the lid closes, which a host's never
// does). The power-off request ends the program, after the answer.
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/pxi.h"
#include <stdio.h>

// pm.c's commands and requests
#define PM_COMMAND_SLEEP_PREPARE 0x60
#define PM_COMMAND_REQUEST 0x61
#define PM_COMMAND_SLEEP_START 0x62
#define PM_COMMAND_WOKE_UP 0x63

#define PM_REQUEST_LCD_POWER_1 1
#define PM_REQUEST_LCD_POWER_3 3
#define PM_REQUEST_BACKLIGHT_BOTTOM_ON 4
#define PM_REQUEST_BACKLIGHT_BOTTOM_OFF 5
#define PM_REQUEST_BACKLIGHT_TOP_ON 6
#define PM_REQUEST_BACKLIGHT_TOP_OFF 7
#define PM_REQUEST_BACKLIGHT_ALL_ON 8
#define PM_REQUEST_BACKLIGHT_ALL_OFF 9
#define PM_REQUEST_POWER_OFF 0xe
#define PM_REQUEST_READ_REGISTER 0xf
#define PM_REQUEST_SET_AMP 0x10
#define PM_REQUEST_SET_AMP_GAIN 0x11
#define PM_REQUEST_UNK_12 0x12
#define PM_REQUEST_SET_AMP_GAIN_LEVEL 0x16

// What a register read gives (the argument of PM_REQUEST_READ_REGISTER): the battery (non-zero when low), the
// backlights (a bit for each), the amplifier (non-zero when on) and a value of the DSi's (0x8)
#define PM_REGISTER_BATTERY 0
#define PM_REGISTER_BACKLIGHT 3
#define PM_REGISTER_AMP 6
#define PM_BACKLIGHT_TOP 0x08
#define PM_BACKLIGHT_BOTTOM 0x04

static struct {
    PortArm7SpiCommand command;
    u8 backlights;
    u8 amp;
    u8 ampGain;
    u8 ampGainLevel;
    u8 unk12;
} sPm;

static void Answer(u32 command, u32 value) {
    port_pxi_reply(PXI_FIFO_TAG_PM, PORT_ARM7_SPI_ANSWER(command, value), FALSE);
}

// A request: the value the answer carries
static u32 Request(u32 request, u32 argument) {
    switch (request) {
    case PM_REQUEST_BACKLIGHT_BOTTOM_ON:
        sPm.backlights |= PM_BACKLIGHT_BOTTOM;
        break;
    case PM_REQUEST_BACKLIGHT_BOTTOM_OFF:
        sPm.backlights &= ~PM_BACKLIGHT_BOTTOM;
        break;
    case PM_REQUEST_BACKLIGHT_TOP_ON:
        sPm.backlights |= PM_BACKLIGHT_TOP;
        break;
    case PM_REQUEST_BACKLIGHT_TOP_OFF:
        sPm.backlights &= ~PM_BACKLIGHT_TOP;
        break;
    case PM_REQUEST_BACKLIGHT_ALL_ON:
        sPm.backlights = PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM;
        break;
    case PM_REQUEST_BACKLIGHT_ALL_OFF:
        sPm.backlights = 0;
        break;
    case PM_REQUEST_READ_REGISTER:
        switch (argument) {
        case PM_REGISTER_BATTERY:
            return 0;
        case PM_REGISTER_BACKLIGHT:
            return sPm.backlights;
        case PM_REGISTER_AMP:
            return sPm.amp;
        default:
            return 0;
        }
    case PM_REQUEST_SET_AMP:
        sPm.amp = argument != 0;
        break;
    case PM_REQUEST_SET_AMP_GAIN:
        sPm.ampGain = (u8)argument;
        break;
    case PM_REQUEST_UNK_12:
        sPm.unk12 = (u8)argument;
        break;
    case PM_REQUEST_SET_AMP_GAIN_LEVEL:
        sPm.ampGainLevel = (u8)argument;
        break;
    default:
        // The screens' power (PM_REQUEST_LCD_POWER_1 to _3), which the ARM9 switches itself, and anything else
        break;
    }
    return 0;
}

static void PmService(u32 tag, u32 data, BOOL err) {
    if (err || !port_arm7_spi_collect(&sPm.command, data)) {
        return;
    }
    switch (sPm.command.command) {
    case PM_COMMAND_SLEEP_PREPARE:
        Answer(PM_COMMAND_SLEEP_PREPARE, 0);
        break;
    case PM_COMMAND_REQUEST: {
        u32 request = sPm.command.data[0];
        u32 value = Request(request, sPm.command.words > 1 ? sPm.command.data[1] : 0);

        Answer(PM_COMMAND_REQUEST, value);
        if (request == PM_REQUEST_POWER_OFF) {
            fprintf(stderr, "port: the game turned the console off (PM_ForceToPowerOff) at frame %u\n",
                    port_os_frame());
            port_os_stop();
        }
        break;
    }
    case PM_COMMAND_SLEEP_START:
        // The answer with a result of 0, then the end of the sleep
        Answer(PM_COMMAND_SLEEP_START, 0);
        Answer(PM_COMMAND_WOKE_UP, 0);
        break;
    default:
        port_pxi_reply(PXI_FIFO_TAG_PM, data, TRUE);
        break;
    }
}

void port_pm7_start(void) {
    sPm.command.count = 0;
    sPm.backlights = PM_BACKLIGHT_TOP | PM_BACKLIGHT_BOTTOM;
    sPm.amp = 0;
    sPm.ampGain = 0;
    sPm.ampGainLevel = 0;
    sPm.unk12 = 0;
    port_pxi_register_service(PXI_FIFO_TAG_PM, PmService);
}
