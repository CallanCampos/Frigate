// The DS's buttons and its lid, for the program's input (port/app). The program reports which buttons the player
// holds, from the keyboard or a gamepad, and this module puts them where the game reads them: KEYINPUT (0x04000130) for
// A, B, Select, Start, the D-pad, R and L, and the ARM7's word in the shared memory (HW_BUTTON_XY_BUF) for X, Y, the
// debug button and the lid, which the ARM9 can't read from a register of its own (GBATEK, "DS Keypad"). Both hold a
// button's bit clear while it is pressed. The game reads them as NitroSDK's PAD_Read does (src/gfl/key.c). Plain C
// types only, so that files without the decomp's headers (port/app) can include it.
#ifndef PORT_HAL_INPUT_PORT_INPUT_H
#define PORT_HAL_INPUT_PORT_INPUT_H

#include <stdint.h>

// The buttons, with NitroSDK's PAD_* values (include/gfl/key.h), the bits PAD_Read gives for them
#define PORT_INPUT_A 0x0001u
#define PORT_INPUT_B 0x0002u
#define PORT_INPUT_SELECT 0x0004u
#define PORT_INPUT_START 0x0008u
#define PORT_INPUT_RIGHT 0x0010u
#define PORT_INPUT_LEFT 0x0020u
#define PORT_INPUT_UP 0x0040u
#define PORT_INPUT_DOWN 0x0080u
#define PORT_INPUT_R 0x0100u
#define PORT_INPUT_L 0x0200u
#define PORT_INPUT_X 0x0400u
#define PORT_INPUT_Y 0x0800u
#define PORT_INPUT_DEBUG 0x2000u
#define PORT_INPUT_ALL 0x2fffu

// Puts the buttons held now (PORT_INPUT_* bits; others are ignored) and the lid's state (open unless closed is
// nonzero) where the game reads them. The state holds until the next call; the program calls it once a frame, from its
// frame hook, and once before the game starts, so that the game's first read sees nothing pressed and the lid open
void port_input_set(uint32_t held, int closed);

// The buttons and the lid as last set
uint32_t port_input_held(void);
int port_input_closed(void);

#endif // PORT_HAL_INPUT_PORT_INPUT_H
