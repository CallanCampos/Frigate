// The DS's buttons and lid on the host (port/hal/input/port_input.h)
#include "hal/input/port_input.h"
#include "nitro/hw.h"
#include "nitro/os.h"

// KEYINPUT's buttons: A to L, bits 0-9
#define KEYINPUT_MASK 0x03ffu
// The ARM7's word: X (bit 10), Y (bit 11) and the debug button (bit 13), clear while pressed, and the lid (bit 15,
// PAD_DETECT_FOLD_MASK), set while it is closed
#define XY_BUTTON_MASK (PORT_INPUT_X | PORT_INPUT_Y | PORT_INPUT_DEBUG)

static uint32_t sHeld;
static int sClosed;

void port_input_set(uint32_t held, int closed) {
    sHeld = held & PORT_INPUT_ALL;
    sClosed = closed != 0;
    reg_PAD_KEYINPUT = (u16)(~sHeld & KEYINPUT_MASK);
    *(vu16 *)HW_BUTTON_XY_BUF = (u16)((~sHeld & XY_BUTTON_MASK) | (sClosed ? PAD_DETECT_FOLD_MASK : 0));
}

uint32_t port_input_held(void) {
    return sHeld;
}

int port_input_closed(void) {
    return sClosed;
}
