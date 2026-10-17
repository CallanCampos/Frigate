// The native port's entry point: opens the window, prepares the platform layer and runs the game's main() (renamed
// NitroMain by the build) on a thread of its own, while this thread handles the window's events.
//
//   pokeblack2 [--headless]
//
// --headless runs the game on this thread without a window, for tests and CI.
#include "hal/trap.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>

// The game's main(), from src/system/main.c
void NitroMain(void);

// The DS's two screens, stacked
enum { SCREEN_WIDTH = 256, SCREEN_HEIGHT = 192 };

static int SDLCALL run_game(void *unused) {
    (void)unused;
    SDL_Log("port: entering the game's main()");
    NitroMain();
    SDL_Log("port: the game's main() returned");
    return 0;
}

int main(int argc, char **argv) {
    int headless = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            headless = 1;
        } else {
            fprintf(stderr, "usage: %s [--headless]\n", argv[0]);
            return 2;
        }
    }

    port_trap_protect_data();

    if (headless) {
        return run_game(NULL);
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("port: SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    SDL_Window *window = SDL_CreateWindow("Pokémon Black 2 and White 2 (native port)", SCREEN_WIDTH * 2,
                                          SCREEN_HEIGHT * 2 * 2, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == NULL) {
        SDL_Log("port: SDL_CreateWindow failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Host C needs more stack than the DS gave the game's main thread
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_THREAD_CREATE_ENTRY_FUNCTION_POINTER, (void *)run_game);
    SDL_SetStringProperty(props, SDL_PROP_THREAD_CREATE_NAME_STRING, "game");
    SDL_SetNumberProperty(props, SDL_PROP_THREAD_CREATE_STACKSIZE_NUMBER, 8 * 1024 * 1024);
    SDL_Thread *game = SDL_CreateThreadWithProperties(props);
    SDL_DestroyProperties(props);
    if (game == NULL) {
        SDL_Log("port: SDL_CreateThread failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int running = 1;
    while (running) {
        SDL_Event event;
        while (SDL_WaitEventTimeout(&event, 16)) {
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                running = 0;
            }
        }
    }

    SDL_DetachThread(game);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
