// Tests of the gamepads (port/app/gamepad.h) without a device. The mapping, the stick's dead zone, the choice of the
// pad that drives and the description are called directly, with labels written out from SDL's own description of each
// pad family (SDL_gamepad.h, SDL_GamepadButton: Xbox A/B/X/Y, Nintendo Switch B/A/Y/X, GameCube A/X/B/Y, PlayStation
// Cross/Circle/Square/Triangle at south/east/west/north). Then SDL's virtual joysticks stand in for pads: they are
// connected and disconnected while the module runs, and their buttons and sticks are set as a player would press them.
// Every expected value is worked out by hand from the DS's layout (A east, B south, X north, Y west).
#include "app/gamepad.h"
#include "hal/input/port_input.h"
#include <stdio.h>
#include <string.h>

static int sChecks;
static int sFailures;

static void Expect(const char *what, int ok) {
    sChecks++;
    if (!ok) {
        sFailures++;
        printf("FAIL %s\n", what);
    }
}

static void ExpectHeld(const char *what, uint32_t got, uint32_t expected) {
    sChecks++;
    if (got != expected) {
        sFailures++;
        printf("FAIL %s: 0x%04x, expected 0x%04x\n", what, (unsigned)got, (unsigned)expected);
    }
}

// --- The stick ------------------------------------------------------------------------------------------------------

static void TestStick(void) {
    const uint32_t up = PORT_INPUT_UP;
    const uint32_t down = PORT_INPUT_DOWN;
    const uint32_t left = PORT_INPUT_LEFT;
    const uint32_t right = PORT_INPUT_RIGHT;

    ExpectHeld("stick at rest", port_gamepad_stick(0, 0, 16384), 0);
    // The dead zone is a circle: a point on it is outside
    ExpectHeld("stick just inside the dead zone", port_gamepad_stick(16383, 0, 16384), 0);
    ExpectHeld("stick on the dead zone's edge", port_gamepad_stick(16384, 0, 16384), right);
    ExpectHeld("stick full left", port_gamepad_stick(-32768, 0, 16384), left);
    ExpectHeld("stick full up", port_gamepad_stick(0, -32768, 16384), up);
    ExpectHeld("stick full down", port_gamepad_stick(0, 32767, 16384), down);
    // 45 degrees at full travel, 23170 = 32767 / sqrt(2)
    ExpectHeld("stick down and right", port_gamepad_stick(23170, 23170, 16384), right | down);
    ExpectHeld("stick up and left", port_gamepad_stick(-23170, -23170, 16384), left | up);
    // A diagonal of radius 15556 is inside; one of 16971 is outside, where per-axis thresholds of 16384 press nothing
    ExpectHeld("stick diagonal inside the dead zone", port_gamepad_stick(11000, 11000, 16384), 0);
    ExpectHeld("stick diagonal just outside the dead zone", port_gamepad_stick(12000, 12000, 16384), right | down);
    // The sectors' edges are 22.5 degrees from each axis: atan(12000 / 30000) is 21.8 degrees, atan(13000 / 30000)
    // 23.4, atan(30000 / 13000) 66.6 and atan(30000 / 12000) 68.2
    ExpectHeld("stick 21.8 degrees below the horizontal", port_gamepad_stick(30000, 12000, 16384), right);
    ExpectHeld("stick 23.4 degrees below the horizontal", port_gamepad_stick(30000, 13000, 16384), right | down);
    ExpectHeld("stick 66.6 degrees above the horizontal", port_gamepad_stick(-13000, -30000, 16384), left | up);
    ExpectHeld("stick 68.2 degrees above the horizontal", port_gamepad_stick(-12000, -30000, 16384), up);
    // Other dead zones
    ExpectHeld("no dead zone, at rest", port_gamepad_stick(0, 0, 0), 0);
    ExpectHeld("no dead zone, the least lean", port_gamepad_stick(1, 0, 0), right);
    ExpectHeld("dead zone 8000, inside", port_gamepad_stick(0, -7999, 8000), 0);
    ExpectHeld("dead zone 8000, on its edge", port_gamepad_stick(0, -8000, 8000), up);
}

// --- The mapping ----------------------------------------------------------------------------------------------------

// The labels SDL documents for each family, at south, east, west and north; every other button has none
static void Labels(SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT], SDL_GamepadButtonLabel south,
                   SDL_GamepadButtonLabel east, SDL_GamepadButtonLabel west, SDL_GamepadButtonLabel north) {
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
        labels[b] = SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN;
    }
    labels[SDL_GAMEPAD_BUTTON_SOUTH] = south;
    labels[SDL_GAMEPAD_BUTTON_EAST] = east;
    labels[SDL_GAMEPAD_BUTTON_WEST] = west;
    labels[SDL_GAMEPAD_BUTTON_NORTH] = north;
}

static void XboxLabels(SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT]) {
    Labels(labels, SDL_GAMEPAD_BUTTON_LABEL_A, SDL_GAMEPAD_BUTTON_LABEL_B, SDL_GAMEPAD_BUTTON_LABEL_X,
           SDL_GAMEPAD_BUTTON_LABEL_Y);
}

static void SwitchLabels(SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT]) {
    Labels(labels, SDL_GAMEPAD_BUTTON_LABEL_B, SDL_GAMEPAD_BUTTON_LABEL_A, SDL_GAMEPAD_BUTTON_LABEL_Y,
           SDL_GAMEPAD_BUTTON_LABEL_X);
}

static void GameCubeLabels(SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT]) {
    Labels(labels, SDL_GAMEPAD_BUTTON_LABEL_A, SDL_GAMEPAD_BUTTON_LABEL_X, SDL_GAMEPAD_BUTTON_LABEL_B,
           SDL_GAMEPAD_BUTTON_LABEL_Y);
}

static void PlayStationLabels(SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT]) {
    Labels(labels, SDL_GAMEPAD_BUTTON_LABEL_CROSS, SDL_GAMEPAD_BUTTON_LABEL_CIRCLE, SDL_GAMEPAD_BUTTON_LABEL_SQUARE,
           SDL_GAMEPAD_BUTTON_LABEL_TRIANGLE);
}

static void NoLabels(SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT]) {
    Labels(labels, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN,
           SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN);
}

// press against the face buttons it should have, south, east, west and north, and nothing on any other button but
// those of the default map's other bindings when withRest is set
static void ExpectFaces(const char *what, const uint32_t press[SDL_GAMEPAD_BUTTON_COUNT], uint32_t south, uint32_t east,
                        uint32_t west, uint32_t north, int withRest) {
    char name[128];
    uint32_t expected[SDL_GAMEPAD_BUTTON_COUNT] = { 0 };

    expected[SDL_GAMEPAD_BUTTON_SOUTH] = south;
    expected[SDL_GAMEPAD_BUTTON_EAST] = east;
    expected[SDL_GAMEPAD_BUTTON_WEST] = west;
    expected[SDL_GAMEPAD_BUTTON_NORTH] = north;
    if (withRest) {
        expected[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] = PORT_INPUT_L;
        expected[SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER] = PORT_INPUT_R;
        expected[SDL_GAMEPAD_BUTTON_START] = PORT_INPUT_START;
        expected[SDL_GAMEPAD_BUTTON_BACK] = PORT_INPUT_SELECT;
        expected[SDL_GAMEPAD_BUTTON_DPAD_UP] = PORT_INPUT_UP;
        expected[SDL_GAMEPAD_BUTTON_DPAD_DOWN] = PORT_INPUT_DOWN;
        expected[SDL_GAMEPAD_BUTTON_DPAD_LEFT] = PORT_INPUT_LEFT;
        expected[SDL_GAMEPAD_BUTTON_DPAD_RIGHT] = PORT_INPUT_RIGHT;
    }
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
        snprintf(name, sizeof(name), "%s, button %d", what, b);
        ExpectHeld(name, press[b], expected[b]);
    }
}

static void TestDefaultMap(void) {
    SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT];
    uint32_t press[SDL_GAMEPAD_BUTTON_COUNT];

    Expect("the default map has the stick as the D-pad", port_gamepad_default_map.leftStickAsDpad);
    Expect("the default dead zone is half the stick's travel", port_gamepad_default_map.stickDeadZone == 16384);
    // By position, whatever the labels
    NoLabels(labels);
    port_gamepad_resolve(&port_gamepad_default_map, labels, press);
    ExpectFaces("default map, no labels", press, PORT_INPUT_B, PORT_INPUT_A, PORT_INPUT_Y, PORT_INPUT_X, 1);
    XboxLabels(labels);
    port_gamepad_resolve(&port_gamepad_default_map, labels, press);
    ExpectFaces("default map, Xbox labels", press, PORT_INPUT_B, PORT_INPUT_A, PORT_INPUT_Y, PORT_INPUT_X, 1);
    PlayStationLabels(labels);
    port_gamepad_resolve(&port_gamepad_default_map, labels, press);
    ExpectFaces("default map, PlayStation labels", press, PORT_INPUT_B, PORT_INPUT_A, PORT_INPUT_Y, PORT_INPUT_X, 1);
}

static void TestLabelMap(void) {
    // The DS's A, B, X and Y on the buttons labelled so, at the DS's positions where no button is
    const PortGamepadMap byLabel = {
        .bindings = {
            { SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LABEL_A, PORT_INPUT_A },
            { SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_LABEL_B, PORT_INPUT_B },
            { SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_LABEL_X, PORT_INPUT_X },
            { SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_LABEL_Y, PORT_INPUT_Y },
        },
    };
    SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT];
    uint32_t press[SDL_GAMEPAD_BUTTON_COUNT];

    XboxLabels(labels);
    port_gamepad_resolve(&byLabel, labels, press);
    ExpectFaces("by label, Xbox", press, PORT_INPUT_A, PORT_INPUT_B, PORT_INPUT_X, PORT_INPUT_Y, 0);
    SwitchLabels(labels);
    port_gamepad_resolve(&byLabel, labels, press);
    ExpectFaces("by label, Switch", press, PORT_INPUT_B, PORT_INPUT_A, PORT_INPUT_Y, PORT_INPUT_X, 0);
    GameCubeLabels(labels);
    port_gamepad_resolve(&byLabel, labels, press);
    ExpectFaces("by label, GameCube", press, PORT_INPUT_A, PORT_INPUT_X, PORT_INPUT_B, PORT_INPUT_Y, 0);
    PlayStationLabels(labels);
    port_gamepad_resolve(&byLabel, labels, press);
    ExpectFaces("by label, PlayStation", press, PORT_INPUT_B, PORT_INPUT_A, PORT_INPUT_Y, PORT_INPUT_X, 0);

    // SDL's labels for the families agree with its description of them, which the cases above are written from
    for (int b = 0; b < SDL_GAMEPAD_BUTTON_COUNT; b++) {
        char name[96];

        XboxLabels(labels);
        snprintf(name, sizeof(name), "SDL's label for an Xbox One pad's button %d", b);
        Expect(name, SDL_GetGamepadButtonLabelForType(SDL_GAMEPAD_TYPE_XBOXONE, (SDL_GamepadButton)b) == labels[b]);
        SwitchLabels(labels);
        snprintf(name, sizeof(name), "SDL's label for a Switch Pro pad's button %d", b);
        Expect(name, SDL_GetGamepadButtonLabelForType(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO, (SDL_GamepadButton)b) ==
                         labels[b]);
        PlayStationLabels(labels);
        snprintf(name, sizeof(name), "SDL's label for a PS4 pad's button %d", b);
        Expect(name, SDL_GetGamepadButtonLabelForType(SDL_GAMEPAD_TYPE_PS4, (SDL_GamepadButton)b) == labels[b]);
    }
}

static void TestMapEdges(void) {
    const PortGamepadMap shared = {
        .bindings = {
            { SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_A | PORT_INPUT_B },
            { SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_A },
            { SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_X },
        },
    };
    // The table ends at the first binding that presses nothing; a button out of range and bits that aren't DS buttons
    // are left out
    const PortGamepadMap ended = {
        .bindings = {
            { SDL_GAMEPAD_BUTTON_INVALID, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_A },
            { SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, 0x10000u | PORT_INPUT_Y },
            { SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, 0 },
            { SDL_GAMEPAD_BUTTON_NORTH, SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN, PORT_INPUT_X },
        },
    };
    SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT];
    uint32_t press[SDL_GAMEPAD_BUTTON_COUNT];

    NoLabels(labels);
    port_gamepad_resolve(&shared, labels, press);
    ExpectFaces("bindings that share buttons", press, PORT_INPUT_A | PORT_INPUT_B | PORT_INPUT_X, PORT_INPUT_A, 0, 0,
                0);
    port_gamepad_resolve(&ended, labels, press);
    ExpectFaces("a table's end", press, 0, 0, PORT_INPUT_Y, 0, 0);
}

// --- The pad that drives --------------------------------------------------------------------------------------------

static void TestPick(void) {
    // Pads 1 and 2, connected in that order
    PortGamepadState pads[2] = { { 1, 0, 0 }, { 2, 0, 0 } };

    Expect("nothing pressed, no pad drives", port_gamepad_pick(0, pads, 2) == 0);
    pads[1].held = PORT_INPUT_A;
    Expect("the first press drives", port_gamepad_pick(0, pads, 2) == 2);
    pads[0].held = PORT_INPUT_B;
    pads[1].previous = PORT_INPUT_A;
    Expect("another pad's press while it holds", port_gamepad_pick(2, pads, 2) == 2);
    pads[0].previous = PORT_INPUT_B;
    pads[1].held = 0;
    Expect("the driver lets go; the other's press is old", port_gamepad_pick(2, pads, 2) == 2);
    pads[0].held = PORT_INPUT_B | PORT_INPUT_X;
    pads[1].previous = 0;
    Expect("a new press takes over", port_gamepad_pick(2, pads, 2) == 1);
    pads[0].held = 0;
    pads[0].previous = PORT_INPUT_B | PORT_INPUT_X;
    Expect("the new driver lets go and stays", port_gamepad_pick(1, pads, 2) == 1);
    pads[0].previous = 0;
    pads[0].held = PORT_INPUT_A;
    pads[1].held = PORT_INPUT_B;
    Expect("two new presses, no driver: the first connected", port_gamepad_pick(0, pads, 2) == 1);
    Expect("two new presses, the second driver idle: the first connected",
           port_gamepad_pick(2, (PortGamepadState[]){ { 1, PORT_INPUT_A, 0 }, { 2, 0, 0 } }, 2) == 1);
    Expect("the driver is gone, nothing new",
           port_gamepad_pick(2, (PortGamepadState[]){ { 1, PORT_INPUT_B, PORT_INPUT_B } }, 1) == 0);
    Expect("the driver is gone, a new press",
           port_gamepad_pick(2, (PortGamepadState[]){ { 1, PORT_INPUT_B, 0 } }, 1) == 1);
    Expect("no pads", port_gamepad_pick(3, pads, 0) == 0);
}

// --- The description ------------------------------------------------------------------------------------------------

static void TestDescribe(void) {
    SDL_GamepadButtonLabel labels[SDL_GAMEPAD_BUTTON_COUNT];
    uint32_t press[SDL_GAMEPAD_BUTTON_COUNT];
    char text[128];

    NoLabels(labels);
    port_gamepad_resolve(&port_gamepad_default_map, labels, press);
    port_gamepad_describe(press, labels, text, sizeof(text));
    Expect("description, no labels", strcmp(text, "DS A, B, X, Y on its east, south, north, west") == 0);
    XboxLabels(labels);
    port_gamepad_describe(press, labels, text, sizeof(text));
    Expect("description, Xbox", strcmp(text, "DS A, B, X, Y on its B, A, Y, X") == 0);
    SwitchLabels(labels);
    port_gamepad_describe(press, labels, text, sizeof(text));
    Expect("description, Switch", strcmp(text, "DS A, B, X, Y on its A, B, X, Y") == 0);
    PlayStationLabels(labels);
    port_gamepad_describe(press, labels, text, sizeof(text));
    Expect("description, PlayStation", strcmp(text, "DS A, B, X, Y on its Circle, Cross, Triangle, Square") == 0);
    press[SDL_GAMEPAD_BUTTON_NORTH] = 0;
    press[SDL_GAMEPAD_BUTTON_WEST] = 0;
    press[SDL_GAMEPAD_BUTTON_LEFT_SHOULDER] |= PORT_INPUT_X;
    port_gamepad_describe(press, labels, text, sizeof(text));
    Expect("description, X on a shoulder and no Y",
           strcmp(text, "DS A, B, X, Y on its Circle, Cross, left shoulder, nothing") == 0);
    // Cut to the buffer: "DS A, B, " and the terminator
    port_gamepad_describe(press, labels, text, 10);
    Expect("description, cut short", strcmp(text, "DS A, B, ") == 0);
}

// --- Virtual pads ---------------------------------------------------------------------------------------------------

#define VIRTUAL_BUTTONS (SDL_GAMEPAD_BUTTON_DPAD_RIGHT + 1)

// A virtual gamepad with SDL's buttons up to the D-pad and the left stick: its joystick's button n is the gamepad's
// button n, and its axes 0 and 1 the stick's
static SDL_JoystickID AttachPad(const char *name, Uint16 vendor, Uint16 product) {
    SDL_VirtualJoystickDesc desc;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = vendor;
    desc.product_id = product;
    desc.nbuttons = VIRTUAL_BUTTONS;
    desc.button_mask = (1u << VIRTUAL_BUTTONS) - 1;
    desc.naxes = 2;
    desc.axis_mask = 1u << SDL_GAMEPAD_AXIS_LEFTX | 1u << SDL_GAMEPAD_AXIS_LEFTY;
    desc.name = name;
    return SDL_AttachVirtualJoystick(&desc);
}

// Lets SDL take the virtual pads' new state and send its events to the module, as the window's event loop does, then
// polls the pads as the frame hook does
static uint32_t Frame(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        port_gamepads_event(&event);
    }
    return port_gamepads_held();
}

static void TestVirtualPads(void) {
    // A Switch Pro Controller's and an Xbox One controller's USB IDs, so that SDL labels their buttons as those pads'
    const SDL_JoystickID p = AttachPad("test pad P", 0x057e, 0x2009);
    const SDL_JoystickID q = AttachPad("test pad Q", 0x045e, 0x02ea);
    const PortGamepadMap byLabel = {
        .bindings = {
            { SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_LABEL_A, PORT_INPUT_A },
            { SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_LABEL_B, PORT_INPUT_B },
        },
    };
    SDL_Joystick *padP;
    SDL_Joystick *padQ;
    SDL_Joystick *padR;
    SDL_JoystickID r;
    int base = port_gamepads_count();

    Expect("virtual pads attached", p != 0 && q != 0);
    if (p == 0 || q == 0) {
        printf("SDL_AttachVirtualJoystick: %s\n", SDL_GetError());
        return;
    }
    padP = SDL_OpenJoystick(p);
    padQ = SDL_OpenJoystick(q);
    ExpectHeld("two pads connected, nothing pressed", Frame(), 0);
    Expect("both pads open", port_gamepads_count() == base + 2);
    Expect("no pad drives yet", port_gamepads_driver() == 0);

    // Q's right face button, which SDL labels B on an Xbox pad, is the DS's A
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_EAST, true);
    ExpectHeld("Q presses its right face button", Frame(), PORT_INPUT_A);
    Expect("Q drives", port_gamepads_driver() == q);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_SOUTH, true);
    ExpectHeld("P presses while Q holds", Frame(), PORT_INPUT_A);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_EAST, false);
    ExpectHeld("Q lets go; P's press is old", Frame(), 0);
    Expect("Q still drives", port_gamepads_driver() == q);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_NORTH, true);
    ExpectHeld("P presses something new", Frame(), PORT_INPUT_B | PORT_INPUT_X);
    Expect("P drives", port_gamepads_driver() == p);
    SDL_SetJoystickVirtualAxis(padQ, 0, 12000);
    SDL_SetJoystickVirtualAxis(padQ, 1, 12000);
    ExpectHeld("Q leans its stick while P holds", Frame(), PORT_INPUT_B | PORT_INPUT_X);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_SOUTH, false);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_NORTH, false);
    ExpectHeld("P lets go; Q's stick is old", Frame(), 0);
    SDL_SetJoystickVirtualAxis(padQ, 0, -30000);
    SDL_SetJoystickVirtualAxis(padQ, 1, 0);
    ExpectHeld("Q's stick turns left", Frame(), PORT_INPUT_LEFT);
    Expect("Q drives again", port_gamepads_driver() == q);
    SDL_SetJoystickVirtualAxis(padQ, 0, 0);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_START, true);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_BACK, true);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
    ExpectHeld("Q's start, back, shoulders and D-pad", Frame(),
               PORT_INPUT_START | PORT_INPUT_SELECT | PORT_INPUT_L | PORT_INPUT_R | PORT_INPUT_UP);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_START, false);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_BACK, false);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, false);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_DPAD_UP, false);
    ExpectHeld("Q lets go of everything", Frame(), 0);

    // A map by label follows the labels SDL reports for each pad: Q's bottom button, labelled A, is now the DS's A,
    // and P's right one, labelled A on a Switch pad, stays the DS's A
    port_gamepads_set_map(&byLabel);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_SOUTH, true);
    ExpectHeld("by label, Q's bottom button", Frame(), PORT_INPUT_A);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_SOUTH, false);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_EAST, true);
    ExpectHeld("by label, Q's right button", Frame(), PORT_INPUT_B);
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_EAST, false);
    ExpectHeld("by label, Q lets go", Frame(), 0);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_EAST, true);
    ExpectHeld("by label, P's right button", Frame(), PORT_INPUT_A);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_EAST, false);
    ExpectHeld("by label, P lets go", Frame(), 0);
    port_gamepads_set_map(&port_gamepad_default_map);

    // Q, driving, is disconnected while it holds a button
    SDL_SetJoystickVirtualButton(padQ, SDL_GAMEPAD_BUTTON_WEST, true);
    ExpectHeld("Q presses its left face button", Frame(), PORT_INPUT_Y);
    Expect("Q drives before it goes", port_gamepads_driver() == q);
    SDL_CloseJoystick(padQ);
    SDL_DetachVirtualJoystick(q);
    ExpectHeld("Q is gone", Frame(), 0);
    Expect("Q's pad is closed", port_gamepads_count() == base + 1);
    Expect("no pad drives once Q is gone", port_gamepads_driver() == 0);

    // A pad connected while the game runs
    r = AttachPad("test pad R", 0, 0);
    padR = SDL_OpenJoystick(r);
    ExpectHeld("R connected", Frame(), 0);
    Expect("R's pad is open", port_gamepads_count() == base + 2);
    SDL_SetJoystickVirtualButton(padR, SDL_GAMEPAD_BUTTON_WEST, true);
    ExpectHeld("R presses its left face button", Frame(), PORT_INPUT_Y);
    Expect("R drives", port_gamepads_driver() == r);
    SDL_SetJoystickVirtualButton(padP, SDL_GAMEPAD_BUTTON_EAST, true);
    ExpectHeld("P presses while R holds", Frame(), PORT_INPUT_Y);

    port_gamepads_close();
    Expect("every pad closed", port_gamepads_count() == 0 && port_gamepads_driver() == 0);
    SDL_CloseJoystick(padR);
    SDL_CloseJoystick(padP);
    SDL_DetachVirtualJoystick(r);
    SDL_DetachVirtualJoystick(p);
}

int main(void) {
    TestStick();
    TestDefaultMap();
    TestLabelMap();
    TestMapEdges();
    TestPick();
    TestDescribe();

    // Only the virtual pads: SDL's drivers for real ones (HIDAPI, IOKit, the GameController framework) stay off, so a
    // pad on the machine that runs the tests doesn't take part
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_IOKIT, "0");
    SDL_SetHint(SDL_HINT_JOYSTICK_MFI, "0");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        Expect("SDL's gamepad subsystem starts", 0);
        printf("SDL_Init: %s\n", SDL_GetError());
    } else {
        port_gamepads_open();
        TestVirtualPads();
        SDL_Quit();
    }

    printf("%d checks, %d failed\n", sChecks, sFailures);
    return sFailures != 0;
}
