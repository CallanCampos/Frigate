// The program's window, presentation and input (port/app/present.h). Phase 1's presentation: the screens in one
// streaming texture drawn by SDL's 2D renderer, stacked and letterboxed to the window, scaled by nearest neighbour.
// PLAN.md §4.6's other layouts and filters come later.
//
// The default controls, by key position (scancodes, so the same on every keyboard layout):
//   D-pad    arrow keys                A  X        B  Z        X  S        Y  A
//   L        Q                         R  W        Start  Return           Select  Right Shift or Backspace
//   touch    the left mouse button on the bottom screen, held and dragged
// and on a gamepad, by position, as on the DS: east A, south B, north X, west Y, the shoulders L and R, start and back
// Start and Select, the D-pad or the left stick the D-pad.
#include "app/present.h"
#include "gfx/screens.h"
#include "hal/input/port_input.h"
#include "hal/os/port_os.h"
#include "hal/rom/sha1.h"
#include "hal/tp/tp_host.h"
#include <SDL3/SDL.h>
#include <string.h>

#define STACKED_HEIGHT (PORT_SCREEN_HEIGHT * 2)
// The window's size at first: each DS pixel two points wide
#define WINDOW_SCALE 2
// How far the left stick must lean to press a direction of the D-pad, of 32767
#define STICK_THRESHOLD 16384

typedef struct KeyBinding {
    SDL_Scancode scancode;
    uint32_t button;
} KeyBinding;

typedef struct PadBinding {
    SDL_GamepadButton pad;
    uint32_t button;
} PadBinding;

static const KeyBinding sKeyBindings[] = {
    { SDL_SCANCODE_X, PORT_INPUT_A },
    { SDL_SCANCODE_Z, PORT_INPUT_B },
    { SDL_SCANCODE_S, PORT_INPUT_X },
    { SDL_SCANCODE_A, PORT_INPUT_Y },
    { SDL_SCANCODE_Q, PORT_INPUT_L },
    { SDL_SCANCODE_W, PORT_INPUT_R },
    { SDL_SCANCODE_RETURN, PORT_INPUT_START },
    { SDL_SCANCODE_RSHIFT, PORT_INPUT_SELECT },
    { SDL_SCANCODE_BACKSPACE, PORT_INPUT_SELECT },
    { SDL_SCANCODE_UP, PORT_INPUT_UP },
    { SDL_SCANCODE_DOWN, PORT_INPUT_DOWN },
    { SDL_SCANCODE_LEFT, PORT_INPUT_LEFT },
    { SDL_SCANCODE_RIGHT, PORT_INPUT_RIGHT },
};

static const PadBinding sPadBindings[] = {
    { SDL_GAMEPAD_BUTTON_EAST, PORT_INPUT_A },          { SDL_GAMEPAD_BUTTON_SOUTH, PORT_INPUT_B },
    { SDL_GAMEPAD_BUTTON_NORTH, PORT_INPUT_X },         { SDL_GAMEPAD_BUTTON_WEST, PORT_INPUT_Y },
    { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, PORT_INPUT_L }, { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, PORT_INPUT_R },
    { SDL_GAMEPAD_BUTTON_START, PORT_INPUT_START },     { SDL_GAMEPAD_BUTTON_BACK, PORT_INPUT_SELECT },
    { SDL_GAMEPAD_BUTTON_DPAD_UP, PORT_INPUT_UP },      { SDL_GAMEPAD_BUTTON_DPAD_DOWN, PORT_INPUT_DOWN },
    { SDL_GAMEPAD_BUTTON_DPAD_LEFT, PORT_INPUT_LEFT },  { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, PORT_INPUT_RIGHT },
};

static SDL_Window *sWindow;
static SDL_Renderer *sRenderer;
static SDL_Texture *sTexture;
static SDL_Gamepad *sGamepad;
// The touch: a press of the left button that began on the bottom screen, until the button is let go
static bool sTouching;

int port_present_open(void) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("port: SDL_Init failed: %s", SDL_GetError());
        return 0;
    }
    sWindow = SDL_CreateWindow("Pokémon Black 2 and White 2 (native port)", PORT_SCREEN_WIDTH * WINDOW_SCALE,
                               STACKED_HEIGHT * WINDOW_SCALE, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE);
    if (sWindow == NULL) {
        SDL_Log("port: SDL_CreateWindow failed: %s", SDL_GetError());
        port_present_close();
        return 0;
    }
    sRenderer = SDL_CreateRenderer(sWindow, NULL);
    if (sRenderer == NULL) {
        SDL_Log("port: SDL_CreateRenderer failed: %s", SDL_GetError());
        port_present_close();
        return 0;
    }
    // The program paces itself to the DS's frame rate (port/app/main.c); waiting for the host's vertical sync as well
    // would pace twice
    SDL_SetRenderVSync(sRenderer, 0);
    SDL_SetRenderLogicalPresentation(sRenderer, PORT_SCREEN_WIDTH, STACKED_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    sTexture = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, PORT_SCREEN_WIDTH,
                                 STACKED_HEIGHT);
    if (sTexture == NULL) {
        SDL_Log("port: SDL_CreateTexture failed: %s", SDL_GetError());
        port_present_close();
        return 0;
    }
    SDL_SetTextureScaleMode(sTexture, SDL_SCALEMODE_NEAREST);
    return 1;
}

void port_present_close(void) {
    if (sGamepad != NULL) {
        SDL_CloseGamepad(sGamepad);
        sGamepad = NULL;
    }
    if (sTexture != NULL) {
        SDL_DestroyTexture(sTexture);
        sTexture = NULL;
    }
    if (sRenderer != NULL) {
        SDL_DestroyRenderer(sRenderer);
        sRenderer = NULL;
    }
    if (sWindow != NULL) {
        SDL_DestroyWindow(sWindow);
        sWindow = NULL;
    }
    SDL_Quit();
}

// Where a point of the window is on the bottom screen, in its pixels; FALSE when it is outside that screen
static bool BottomScreenPoint(float windowX, float windowY, int *x, int *y) {
    float logicalX;
    float logicalY;

    if (!SDL_RenderCoordinatesFromWindow(sRenderer, windowX, windowY, &logicalX, &logicalY)) {
        return false;
    }
    *x = (int)SDL_floorf(logicalX);
    *y = (int)SDL_floorf(logicalY) - PORT_SCREEN_HEIGHT;
    return *x >= 0 && *x < PORT_SCREEN_WIDTH && *y >= 0 && *y < PORT_SCREEN_HEIGHT;
}

static void HandleEvent(const SDL_Event *event) {
    int x;
    int y;

    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        port_os_stop();
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        if (sGamepad == NULL) {
            sGamepad = SDL_OpenGamepad(event->gdevice.which);
        }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (sGamepad != NULL && SDL_GetGamepadID(sGamepad) == event->gdevice.which) {
            SDL_CloseGamepad(sGamepad);
            sGamepad = NULL;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (event->button.button == SDL_BUTTON_LEFT && BottomScreenPoint(event->button.x, event->button.y, &x, &y)) {
            sTouching = true;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            sTouching = false;
        }
        break;
    default:
        break;
    }
}

static uint32_t HeldButtons(void) {
    const bool *keys = SDL_GetKeyboardState(NULL);
    uint32_t held = 0;
    size_t i;

    for (i = 0; i < SDL_arraysize(sKeyBindings); i++) {
        if (keys[sKeyBindings[i].scancode]) {
            held |= sKeyBindings[i].button;
        }
    }
    if (sGamepad != NULL) {
        Sint16 stickX = SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFTX);
        Sint16 stickY = SDL_GetGamepadAxis(sGamepad, SDL_GAMEPAD_AXIS_LEFTY);

        for (i = 0; i < SDL_arraysize(sPadBindings); i++) {
            if (SDL_GetGamepadButton(sGamepad, sPadBindings[i].pad)) {
                held |= sPadBindings[i].button;
            }
        }
        held |= stickX <= -STICK_THRESHOLD ? PORT_INPUT_LEFT : 0;
        held |= stickX >= STICK_THRESHOLD ? PORT_INPUT_RIGHT : 0;
        held |= stickY <= -STICK_THRESHOLD ? PORT_INPUT_UP : 0;
        held |= stickY >= STICK_THRESHOLD ? PORT_INPUT_DOWN : 0;
    }
    return held;
}

void port_present_poll(void) {
    SDL_Event event;
    float windowX;
    float windowY;
    int x = 0;
    int y = 0;

    while (SDL_PollEvent(&event)) {
        HandleEvent(&event);
    }
    port_input_set(HeldButtons(), 0);
    if (sTouching) {
        SDL_GetMouseState(&windowX, &windowY);
        // Outside the screen the point is clamped to its edge (port_tp_set_touch)
        (void)BottomScreenPoint(windowX, windowY, &x, &y);
    }
    port_tp_set_touch(sTouching, x, y);
}

void port_present_frame(void) {
    const SDL_Rect top = { 0, 0, PORT_SCREEN_WIDTH, PORT_SCREEN_HEIGHT };
    const SDL_Rect bottom = { 0, PORT_SCREEN_HEIGHT, PORT_SCREEN_WIDTH, PORT_SCREEN_HEIGHT };
    const int pitch = PORT_SCREEN_WIDTH * (int)sizeof(uint32_t);

    if (sRenderer == NULL) {
        return;
    }
    SDL_UpdateTexture(sTexture, &top, port_screens_pixels(PORT_SCREEN_TOP), pitch);
    SDL_UpdateTexture(sTexture, &bottom, port_screens_pixels(PORT_SCREEN_BOTTOM), pitch);
    SDL_SetRenderDrawColor(sRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sRenderer);
    SDL_RenderTexture(sRenderer, sTexture, NULL, NULL);
    SDL_RenderPresent(sRenderer);
}

int port_present_screenshot(const char *path) {
    SDL_Surface *surface = SDL_CreateSurface(PORT_SCREEN_WIDTH, STACKED_HEIGHT, SDL_PIXELFORMAT_ARGB8888);
    const size_t rowBytes = PORT_SCREEN_WIDTH * sizeof(uint32_t);
    int ok;

    if (surface == NULL) {
        SDL_Log("port: screenshot: SDL_CreateSurface failed: %s", SDL_GetError());
        return 0;
    }
    for (int screen = 0; screen < 2; screen++) {
        const uint32_t *pixels = port_screens_pixels(screen);

        for (int line = 0; line < PORT_SCREEN_HEIGHT; line++) {
            uint8_t *row = (uint8_t *)surface->pixels + (size_t)(screen * PORT_SCREEN_HEIGHT + line) * surface->pitch;

            memcpy(row, pixels + line * PORT_SCREEN_WIDTH, rowBytes);
        }
    }
    ok = SDL_SavePNG(surface, path);
    if (!ok) {
        SDL_Log("port: screenshot: can't write %s: %s", path, SDL_GetError());
    }
    SDL_DestroySurface(surface);
    return ok;
}

void port_present_screen_hash(int screen, char hex[41]) {
    static const char digits[] = "0123456789abcdef";
    const uint32_t *pixels = port_screens_pixels(screen);
    uint8_t row[PORT_SCREEN_WIDTH * 3];
    uint8_t digest[PORT_SHA1_DIGEST_SIZE];
    PortSha1 sha1;

    port_sha1_init(&sha1);
    for (int line = 0; line < PORT_SCREEN_HEIGHT; line++) {
        for (int x = 0; x < PORT_SCREEN_WIDTH; x++) {
            uint32_t pixel = pixels[line * PORT_SCREEN_WIDTH + x];

            row[x * 3] = (uint8_t)(pixel >> 16);
            row[x * 3 + 1] = (uint8_t)(pixel >> 8);
            row[x * 3 + 2] = (uint8_t)pixel;
        }
        port_sha1_update(&sha1, row, sizeof(row));
    }
    port_sha1_final(&sha1, digest);
    for (int i = 0; i < PORT_SHA1_DIGEST_SIZE; i++) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 15];
    }
    hex[40] = '\0';
}
