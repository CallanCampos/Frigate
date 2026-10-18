// Gamepads as the DS's buttons (port/app/gamepad.h). Each open pad keeps the DS buttons each of its buttons presses,
// resolved from the map and the labels SDL reports for it when it is opened or remapped, the D-pad buttons its stick
// pressed at the last poll, which the stick's hysteresis starts from, and the DS buttons it held at the last poll,
// which tell a new press from a held one when the pads take turns.
#include "app/gamepad.h"
#include "hal/input/port_input.h"
#include <stdio.h>
#include <string.h>

// SDL says a stick at rest reads within about 8000 of its centre (SDL_gamepad.h, SDL_GamepadAxis); half the travel
// leaves room for worn sticks and still answers a deliberate push
#define DEFAULT_STICK_DEAD_ZONE 16384

#define STICK_DIRECTIONS (PORT_INPUT_UP | PORT_INPUT_DOWN | PORT_INPUT_LEFT | PORT_INPUT_RIGHT)

const PortGamepadMap port_gamepad_default_map = {
    .bindings = {
        { SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_A },
        { SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_B },
        { SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_X },
        { SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_Y },
        { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_L },
        { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_R },
        { SDL_GAMEPAD_BUTTON_START, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_START },
        { SDL_GAMEPAD_BUTTON_BACK, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_SELECT },
        { SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_UP },
        { SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_DOWN },
        { SDL_GAMEPAD_BUTTON_DPAD_LEFT, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_LEFT },
        { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_RIGHT },
    },
    .leftStickAsDpad = true,
    .stickDeadZone = DEFAULT_STICK_DEAD_ZONE,
};

// The buttons by position, and the labels, as port_gamepad_describe names them
static const char *const sButtonNames[] = {
    "south",       "east",        "west",           "north",          "back",           "guide",         "start",
    "left stick",  "right stick", "left shoulder",  "right shoulder", "D-pad up",       "D-pad down",    "D-pad left",
    "D-pad right", "misc 1",      "right paddle 1", "left paddle 1",  "right paddle 2", "left paddle 2", "touchpad",
    "misc 2",      "misc 3",      "misc 4",         "misc 5",         "misc 6",
};
_Static_assert(sizeof(sButtonNames) / sizeof(sButtonNames[0]) == SDL_GAMEPAD_BUTTON_COUNT,
               "a name for each of SDL's gamepad buttons");
static const char *const sLabelNames[] = {
    NULL, "A", "B", "X", "Y", "Cross", "Circle", "Square", "Triangle",
};

typedef struct Pad {
    SDL_Gamepad *gamepad;
    SDL_JoystickID id;
    uint32_t press[SDL_GAMEPAD_BUTTON_COUNT];
    uint32_t stick;
    uint32_t previous;
} Pad;

// The open pads, in the order they were connected, and the one that drives the game
static Pad sPads[PORT_GAMEPAD_MAX_PADS];
static int sPadCount;
static SDL_JoystickID sDriver;
// The map that replaced the default one, if any
static PortGamepadMap sMap;
static bool sMapReplaced;

void port_gamepad_resolve(const PortGamepadMap *map, const SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT],
                          uint32_t press[SDL_GAMEPAD_BUTTON_COUNT]) {
    memset(press, 0, sizeof(press[0]) * SDL_GAMEPAD_BUTTON_COUNT);
    for (int i = 0; i < PORT_GAMEPAD_MAX_BINDINGS && map->bindings[i].press != 0; i++) {
        const PortGamepadBinding *binding = &map->bindings[i];
        int button = binding->button;

        if (binding->label != SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN) {
            for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
                if (labels[b] == binding->label) {
                    button = b;
                    break;
                }
            }
        }
        if (button >= 0 && button < SDL_GAMEPAD_BUTTON_COUNT) {
            press[button] |= binding->press & PORT_INPUT_ALL;
        }
    }
}

// The direction or diagonal of the sector a stick leans into, at x, y away from its centre (ax and ay their sizes). A
// direction is pressed while the stick leans within 67.5 degrees of it, so that each of the four directions and four
// diagonals has 45 degrees. At an angle t from the horizontal, horizontal is pressed when tan(t) <= tan(67.5) =
// 1 / (sqrt(2) - 1), which is ax >= (sqrt(2) - 1) ay, that is ax + ay >= sqrt(2) ay, squared to stay in integers
static uint32_t Sector(int x, int y, int64_t ax, int64_t ay) {
    const int64_t sum = ax + ay;
    uint32_t held = 0;

    if (sum * sum >= 2 * ay * ay) {
        held |= x < 0 ? PORT_INPUT_LEFT : PORT_INPUT_RIGHT;
    }
    if (sum * sum >= 2 * ax * ax) {
        held |= y < 0 ? PORT_INPUT_UP : PORT_INPUT_DOWN;
    }
    return held;
}

// Whether a stick at x, y still leans into the sector of held (one direction or diagonal) widened by about 4 degrees
// each side. The sectors' edges are 22.5 degrees from each axis; widened, a direction keeps ay <= ax / 2 (26.57
// degrees, tan = 1/2) and a diagonal keeps ax / 3 <= ay <= 3 ax (18.43 to 71.57 degrees, tan = 1/3 and 3), with ax
// along the direction
static bool KeepsSector(uint32_t held, int x, int y, int64_t ax, int64_t ay) {
    const uint32_t horizontal = held & (PORT_INPUT_LEFT | PORT_INPUT_RIGHT);
    const uint32_t vertical = held & (PORT_INPUT_UP | PORT_INPUT_DOWN);

    if ((horizontal == PORT_INPUT_RIGHT && x <= 0) || (horizontal == PORT_INPUT_LEFT && x >= 0) ||
        (vertical == PORT_INPUT_DOWN && y <= 0) || (vertical == PORT_INPUT_UP && y >= 0)) {
        return false;
    }
    if (horizontal == PORT_INPUT_LEFT || horizontal == PORT_INPUT_RIGHT) {
        if (vertical == 0) {
            return 2 * ay <= ax;
        }
        if (vertical == PORT_INPUT_UP || vertical == PORT_INPUT_DOWN) {
            return 3 * ay >= ax && ay <= 3 * ax;
        }
        return false;
    }
    return horizontal == 0 && (vertical == PORT_INPUT_UP || vertical == PORT_INPUT_DOWN) && 2 * ax <= ay;
}

uint32_t port_gamepad_stick(int x, int y, int deadZone, uint32_t previous) {
    const int64_t ax = x < 0 ? -(int64_t)x : x;
    const int64_t ay = y < 0 ? -(int64_t)y : y;
    const int64_t zone = deadZone;
    const int64_t release = zone - zone / 8;

    previous &= STICK_DIRECTIONS;
    if (ax == 0 && ay == 0) {
        return 0;
    }
    // From the centre, nothing until the stick reaches the dead zone's edge; once it presses, it lets go only back
    // inside seven eighths of it, and keeps its direction until it leans past the widened sector. A stick held still
    // at either edge reads a few hundred units of noise, and without these margins each wobble across the edge would
    // be a new press to the game
    if (previous == 0) {
        return ax * ax + ay * ay < zone * zone ? 0 : Sector(x, y, ax, ay);
    }
    if (ax * ax + ay * ay < release * release) {
        return 0;
    }
    return KeepsSector(previous, x, y, ax, ay) ? previous : Sector(x, y, ax, ay);
}

SDL_JoystickID port_gamepad_pick(SDL_JoystickID driver, const PortGamepadState *pads, int count) {
    bool present = false;

    for (int i = 0; i < count; i++) {
        if (driver != 0 && pads[i].id == driver) {
            if (pads[i].held != 0) {
                return driver;
            }
            present = true;
        }
    }
    for (int i = 0; i < count; i++) {
        uint32_t pressed = pads[i].held & ~pads[i].previous;

        if (present) {
            pressed &= ~pads[i].drift;
        }
        if (pressed != 0) {
            return pads[i].id;
        }
    }
    return present ? driver : 0;
}

void port_gamepad_describe(const uint32_t press[SDL_GAMEPAD_BUTTON_COUNT],
                           const SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT], char *out, size_t size) {
    static const uint32_t faces[] = { PORT_INPUT_A, PORT_INPUT_B, PORT_INPUT_X, PORT_INPUT_Y };
    size_t length;

    if (size == 0) {
        return;
    }
    snprintf(out, size, "DS A, B, X, Y on its");
    for (size_t k = 0; k < sizeof(faces) / sizeof(faces[0]); k++) {
        const char *name = "nothing";

        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
            if ((press[b] & faces[k]) != 0) {
                const unsigned label = (unsigned)labels[b];

                name = label != 0 && label < sizeof(sLabelNames) / sizeof(sLabelNames[0]) ? sLabelNames[label]
                                                                                          : sButtonNames[b];
                break;
            }
        }
        length = strlen(out);
        snprintf(out + length, size - length, "%s%s", k == 0 ? " " : ", ", name);
    }
}

static const PortGamepadMap *CurrentMap(void) {
    return sMapReplaced ? &sMap : &port_gamepad_default_map;
}

static Pad *FindPad(SDL_JoystickID id) {
    for (int i = 0; i < sPadCount; i++) {
        if (sPads[i].id == id) {
            return &sPads[i];
        }
    }
    return NULL;
}

// The DS buttons the pad's buttons press, from the map and the labels SDL reports for this pad
static void ResolvePad(Pad *pad) {
    SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT];

    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
        labels[b] = SDL_GetGamepadButtonLabel(pad->gamepad, (SDL_GamepadButton)b);
    }
    port_gamepad_resolve(CurrentMap(), labels, pad->press);
}

static void DescribePad(const Pad *pad, const char *what) {
    SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT];
    char layout[128];
    const char *name = SDL_GetGamepadName(pad->gamepad);

    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
        labels[b] = SDL_GetGamepadButtonLabel(pad->gamepad, (SDL_GamepadButton)b);
    }
    port_gamepad_describe(pad->press, labels, layout, sizeof(layout));
    SDL_Log("port: gamepad %s: %s (%s)", what, name != NULL ? name : "unnamed", layout);
}

static void AddPad(SDL_JoystickID id) {
    SDL_Gamepad *gamepad;
    Pad *pad;

    if (FindPad(id) != NULL) {
        return;
    }
    if (sPadCount == PORT_GAMEPAD_MAX_PADS) {
        SDL_Log("port: gamepad: %d pads are open already; this one waits until one of them is disconnected",
                PORT_GAMEPAD_MAX_PADS);
        return;
    }
    gamepad = SDL_OpenGamepad(id);
    if (gamepad == NULL) {
        SDL_Log("port: gamepad: SDL_OpenGamepad failed: %s", SDL_GetError());
        return;
    }
    pad = &sPads[sPadCount++];
    memset(pad, 0, sizeof(*pad));
    pad->gamepad = gamepad;
    pad->id = id;
    ResolvePad(pad);
    DescribePad(pad, "connected");
}

// Opens every connected pad that isn't open yet, as long as there is room, but the one with the ID gone (0 for none),
// which SDL may still list while its removal is handled
static void OpenConnected(SDL_JoystickID gone) {
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);

    if (ids == NULL) {
        return;
    }
    for (int i = 0; i < count && sPadCount < PORT_GAMEPAD_MAX_PADS; i++) {
        if (ids[i] != gone) {
            AddPad(ids[i]);
        }
    }
    SDL_free(ids);
}

static void RemovePad(SDL_JoystickID id) {
    Pad *pad = FindPad(id);
    int slot;

    if (pad == NULL) {
        return;
    }
    slot = (int)(pad - sPads);
    SDL_CloseGamepad(pad->gamepad);
    memmove(&sPads[slot], &sPads[slot + 1], sizeof(sPads[0]) * (size_t)(sPadCount - slot - 1));
    sPadCount--;
    if (sDriver == id) {
        sDriver = 0;
    }
    // A pad that was connected while every slot was taken gets this one
    OpenConnected(id);
}

void port_gamepads_open(void) {
    OpenConnected(0);
}

bool port_gamepads_event(const SDL_Event *event) {
    Pad *pad;

    switch (event->type) {
    case SDL_EVENT_GAMEPAD_ADDED:
        AddPad(event->gdevice.which);
        return true;
    case SDL_EVENT_GAMEPAD_REMOVED:
        RemovePad(event->gdevice.which);
        return true;
    case SDL_EVENT_GAMEPAD_REMAPPED:
        // A new mapping can move the buttons and change their labels
        pad = FindPad(event->gdevice.which);
        if (pad != NULL) {
            ResolvePad(pad);
            DescribePad(pad, "remapped");
        }
        return true;
    default:
        return false;
    }
}

uint32_t port_gamepads_held(void) {
    const PortGamepadMap *map = CurrentMap();
    PortGamepadState states[PORT_GAMEPAD_MAX_PADS];
    uint32_t stick[PORT_GAMEPAD_MAX_PADS];
    SDL_JoystickID driver;
    uint32_t held = 0;

    for (int i = 0; i < sPadCount; i++) {
        const Pad *pad = &sPads[i];

        states[i].id = pad->id;
        states[i].held = 0;
        states[i].previous = pad->previous;
        for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
            if (pad->press[b] != 0 && SDL_GetGamepadButton(pad->gamepad, (SDL_GamepadButton)b)) {
                states[i].held |= pad->press[b];
            }
        }
        stick[i] = 0;
        if (map->leftStickAsDpad) {
            stick[i] = port_gamepad_stick(SDL_GetGamepadAxis(pad->gamepad, SDL_GAMEPAD_AXIS_LEFTX),
                                          SDL_GetGamepadAxis(pad->gamepad, SDL_GAMEPAD_AXIS_LEFTY), map->stickDeadZone,
                                          pad->stick);
            states[i].held |= stick[i];
        }
        states[i].drift = 0;
        if (stick[i] != 0) {
            const int64_t x = SDL_GetGamepadAxis(pad->gamepad, SDL_GAMEPAD_AXIS_LEFTX);
            const int64_t y = SDL_GetGamepadAxis(pad->gamepad, SDL_GAMEPAD_AXIS_LEFTY);
            const int64_t push = map->stickDeadZone + (SDL_JOYSTICK_AXIS_MAX - map->stickDeadZone) / 2;

            if (x * x + y * y < push * push) {
                states[i].drift = stick[i];
            }
        }
    }
    driver = port_gamepad_pick(sDriver, states, sPadCount);
    for (int i = 0; i < sPadCount; i++) {
        sPads[i].stick = stick[i];
        sPads[i].previous = states[i].held;
        if (states[i].id == driver) {
            held = states[i].held;
        }
    }
    sDriver = driver;
    return held;
}

void port_gamepads_set_map(const PortGamepadMap *map) {
    sMap = *map;
    sMapReplaced = true;
    for (int i = 0; i < sPadCount; i++) {
        ResolvePad(&sPads[i]);
    }
}

void port_gamepads_close(void) {
    for (int i = 0; i < sPadCount; i++) {
        SDL_CloseGamepad(sPads[i].gamepad);
    }
    sPadCount = 0;
    sDriver = 0;
}

SDL_JoystickID port_gamepads_driver(void) {
    return sDriver;
}

int port_gamepads_count(void) {
    return sPadCount;
}
