// Gamepads as the DS's buttons (port/app/gamepad.c), through SDL3's gamepad API: every connected pad is open, pads come
// and go while the game runs, and one of them at a time drives the game. What each control does is one table, a
// PortGamepadMap, which a settings file can replace (port_gamepads_set_map); port_gamepad_default_map is the DS's own
// layout. The mapping, the left stick's dead zone and the choice of the pad that drives are plain functions of their
// inputs, so the tests run them without a device (port/tests/app/test_gamepad.c). Everything runs on the program's one
// thread, from the window's event loop (port/app/present.c).
#ifndef PORT_APP_GAMEPAD_H
#define PORT_APP_GAMEPAD_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The most bindings a map holds, and the most pads open at once (one connected beyond them waits, with a message, and
// is opened when one of the open pads is disconnected)
#define PORT_GAMEPAD_MAX_BINDINGS 32
#define PORT_GAMEPAD_MAX_PADS 8

// One control of the pad and the DS buttons it presses (PORT_INPUT_* bits, port/hal/input/port_input.h). The control
// is a button by its position (SDL_GAMEPAD_BUTTON_SOUTH is the bottom face button whatever it says on it), or, when
// label isn't SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, the button that SDL says carries that label on the pad at hand, and the
// one at button's position on a pad with no button labelled so (a PlayStation pad has no lettered buttons)
typedef struct PortGamepadBinding {
    SDL_GamepadButton button;
    SDL_GamepadButtonLabel label;
    uint32_t press;
} PortGamepadBinding;

// What a pad's controls do. The bindings run up to the first whose press is 0; several may press the same DS button,
// and a button may press several. With leftStickAsDpad, the left stick also presses the D-pad: nothing while it leans
// less than stickDeadZone (of 32767) from its centre, else the direction or diagonal it leans toward, each of the
// eight taking a sector of 45 degrees, with a margin at both edges once it presses (port_gamepad_stick)
typedef struct PortGamepadMap {
    PortGamepadBinding bindings[PORT_GAMEPAD_MAX_BINDINGS];
    bool leftStickAsDpad;
    int stickDeadZone;
} PortGamepadMap;

// The DS's layout, which is a Nintendo pad's: A on the right face button, B on the bottom one, X on the top one, Y on
// the left one, by position on every pad (on an Xbox pad, the DS's A is its B button). L and R on the shoulder buttons,
// Start on the start button, Select on the back button (Select, Share, View or minus); the D-pad, and the left stick
// as the D-pad beyond half its travel
extern const PortGamepadMap port_gamepad_default_map;

// The DS buttons each of a pad's buttons presses under map (press, indexed by SDL_GamepadButton), given the label SDL
// reports for each of its buttons (labels, indexed the same way; SDL_GetGamepadButtonLabel)
void port_gamepad_resolve(const PortGamepadMap *map, const SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT],
                          uint32_t press[SDL_GAMEPAD_BUTTON_COUNT]);

// The D-pad's buttons (PORT_INPUT_UP, DOWN, LEFT, RIGHT) that a stick at x, y presses (SDL's axes: -32768 up or left,
// 32767 down or right), given what it pressed at the last poll (previous; 0 at first). From the centre: none inside
// the circle of radius deadZone, else the one or two of the sector it leans into. Once it presses, with hysteresis, so
// that a stick held still at an edge doesn't flicker: none only back inside seven eighths of deadZone, and the same
// direction or diagonal until the stick leans about 4 degrees past its sector's edge
uint32_t port_gamepad_stick(int x, int y, int deadZone, uint32_t previous);

// A pad's DS buttons now and at the last poll, for port_gamepad_pick, and which of the ones it holds now its stick only
// drifts into: pressed from near the dead zone's edge, not pushed beyond halfway from there to the end of its travel
typedef struct PortGamepadState {
    SDL_JoystickID id;
    uint32_t held;
    uint32_t previous;
    uint32_t drift;
} PortGamepadState;

// Which pad drives the game, given the one that drove it last (0 for none) and every open pad in the order they were
// connected: the pad that drove keeps driving while it holds anything; when it holds nothing, the first pad that
// presses something new (a button it didn't hold at the last poll) takes over. While the pad that drove is still
// connected, a stick's drift doesn't take the game from it: a stick resting near the dead zone's edge (a pad on a desk,
// a worn stick) presses once and holds that press, where a deliberate push goes well past it. A pad that drove and
// holds nothing stays the one that drives until another presses something. 0 when no pad has driven, or the one that
// did is gone
SDL_JoystickID port_gamepad_pick(SDL_JoystickID driver, const PortGamepadState *pads, int count);

// Writes which of the pad's buttons are the DS's A, B, X and Y under press (port_gamepad_resolve), by the labels SDL
// reports for them, or by position for a button with no label: "DS A, B, X, Y on its B, A, Y, X" on an Xbox pad with
// the DS's layout
void port_gamepad_describe(const uint32_t press[SDL_GAMEPAD_BUTTON_COUNT],
                           const SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT], char *out, size_t size);

// The pads themselves. port_gamepads_open opens the pads already connected (SDL's gamepad subsystem must be
// initialized); port_gamepads_event handles SDL's gamepad events (a pad added, removed or remapped) and returns false
// for any other event; port_gamepads_held reads every open pad as SDL last updated it and returns the DS buttons the
// pad that drives holds (port_gamepad_pick); port_gamepads_set_map replaces the map for every pad, open or to come;
// port_gamepads_close closes them all
void port_gamepads_open(void);
bool port_gamepads_event(const SDL_Event *event);
uint32_t port_gamepads_held(void);
void port_gamepads_set_map(const PortGamepadMap *map);
void port_gamepads_close(void);

// The pad that drives the game, 0 for none, and how many pads are open: for the tests
SDL_JoystickID port_gamepads_driver(void);
int port_gamepads_count(void);

#endif // PORT_APP_GAMEPAD_H
