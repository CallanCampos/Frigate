// The program's window, presentation and input (port/app/present.h): what the player sees of the DS's two screens, and
// how the player presses its buttons and touches its bottom screen. The screens, as port/gfx/screens.c drew them, go
// into one streaming texture drawn by SDL's 2D renderer, stacked and letterboxed to the window, scaled by nearest
// neighbour; PLAN.md §4.6's other layouts and filters come later. The keyboard and gamepads become the DS's buttons
// (port/hal/input), and the left mouse button on the bottom screen its touch screen (port/hal/tp).
//
// The default controls. The keyboard, by key position (scancodes, so the same on every keyboard layout), and any
// gamepad at the same time; the gamepad's buttons by position, as on the DS, which is a Nintendo pad's layout:
//
//   DS button   keyboard                     gamepad (Xbox label; PlayStation label)
//   A           X                            right face button (B; Circle)
//   B           Z                            bottom face button (A; Cross)
//   X           S                            top face button (Y; Triangle)
//   Y           A                            left face button (X; Square)
//   L, R        Q, W                         left and right shoulder buttons
//   Start       Return                       start (Menu; Options; plus on a Nintendo pad)
//   Select      Right Shift or Backspace     back (View; Share or Create; minus on a Nintendo pad)
//   D-pad       arrow keys                   D-pad, and the left stick beyond half its travel, diagonals included
//   touch       the left mouse button on the bottom screen, held and dragged
//
// The stick presses once past half its travel and lets go back inside seven eighths of that, and it keeps a direction
// until it leans about 4 degrees into the next one's sector, so that a stick held near an edge doesn't flicker. Pads
// can be connected and disconnected while the game runs, and several can be connected (eight open at once; a ninth
// waits for a free place): the first that presses something drives the game until it lets go of everything, and then
// the next to press something takes over. Each pad says when it connects which of its buttons, by the labels SDL
// reports, are the DS's A, B, X and Y. The gamepad's controls are one table, port_gamepad_default_map
// (port/app/gamepad.c), which a settings file will be able to replace through port_gamepads_set_map (none is read yet).
#include "app/present.h"
#include "app/gamepad.h"
#include "app/window_icon.h"
#include "gfx/screens.h"
#include "hal/input/port_input.h"
#include "hal/os/port_app.h"
#include "hal/os/port_os.h"
#include "hal/rom/rom_manifest.h"
#include "hal/tp/tp_host.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#define STACKED_HEIGHT (PORT_SCREEN_HEIGHT * 2)
// The window's size at first: each DS pixel two points wide
#define WINDOW_SCALE 2

typedef struct KeyBinding {
    SDL_Scancode scancode;
    uint32_t button;
} KeyBinding;

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

static SDL_Window *sWindow;
static SDL_Renderer *sRenderer;
static SDL_Texture *sTexture;
// The touch: a press of the left button that began on the bottom screen, until the button is let go
static bool sTouching;

int port_present_open(void) {
    char title[128];

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("port: SDL_Init failed: %s", SDL_GetError());
        return 0;
    }
    // The product and the game it runs: "Frigate — Pokémon Black 2"
    snprintf(title, sizeof(title), "%s — %s", PORT_PRODUCT_NAME, port_rom_manifest_game);
    sWindow = SDL_CreateWindow(title, PORT_SCREEN_WIDTH * WINDOW_SCALE, STACKED_HEIGHT * WINDOW_SCALE,
                               SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE);
    if (sWindow == NULL) {
        SDL_Log("port: SDL_CreateWindow failed: %s", SDL_GetError());
        port_present_close();
        return 0;
    }
    // Frigate's icon for the version, on Linux and Windows (on macOS the app's icon is the Dock's)
    port_window_icon_set(sWindow);
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
    // The pads connected now; those that come later arrive as events
    port_gamepads_open();
    return 1;
}

void port_present_close(void) {
    port_gamepads_close();
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

// One window event: closing the window stops the game at the next frame hook, a gamepad coming, going or remapped
// goes to port/app/gamepad.c, and the left mouse button pressed on the bottom screen starts a touch, which lasts until
// it is released anywhere. The keyboard has no events here: port_present_poll reads its state once a frame. A touch
// screen works through the mouse events SDL makes of its touches
static void HandleEvent(const SDL_Event *event) {
    int x;
    int y;

    switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        port_os_stop();
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED:
    case SDL_EVENT_GAMEPAD_REMAPPED:
        port_gamepads_event(event);
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

// The buttons held on the keyboard and on the pad that drives the game, together
static uint32_t HeldButtons(void) {
    const bool *keys = SDL_GetKeyboardState(NULL);
    uint32_t held = 0;

    for (size_t i = 0; i < SDL_arraysize(sKeyBindings); i++) {
        if (keys[sKeyBindings[i].scancode]) {
            held |= sKeyBindings[i].button;
        }
    }
    return held | port_gamepads_held();
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
