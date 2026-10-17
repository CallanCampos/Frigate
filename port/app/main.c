// The native port's entry point: reads the options, opens the window, prepares the platform layer and runs the game's
// main() (renamed NitroMain by the build) on the OS core (port/hal/os). Everything runs on this one thread: the game
// and its SDK threads on fibers, and the platform's own work on the dispatcher's stack: the scanline hook, which draws
// each line of both screens (port/gfx/screens.c), and at each VBlank the geometry engine's swap (port/gfx/ge/ge_hal.h)
// and this file's frame hook, which takes the player's input, presents the frame (port/app/present.c) and paces to the
// DS's 59.8261 frames a second.
//
//   pokeblack2 [--headless] [--frames N] [--screenshot PATH] [--rom PATH] [--save-dir PATH] [--fixed-time TIME]
//              [--watchdog SECONDS] [--main-arena-size BYTES] [--vram-backend NAME]
//
//   --headless          runs without a window and without pacing, for tests and CI
//   --frames N          runs N frames, then exits with status 0
//   --screenshot PATH   writes both screens as a PNG (top above bottom) when the run ends: after the last frame of
//                       --frames N, or when the window is closed
//   --rom PATH          the player's ROM (port/hal/rom)
//   --save-dir PATH     where saves go (port/hal/card)
//   --fixed-time TIME   the clock's time at boot, "YYYY-MM-DD HH:MM:SS", moving with the virtual clock (port/hal/rtc)
//   --watchdog SECONDS  stops with a report and status 73 when a frame takes longer than that in host time: the game
//                       hangs in a loop that never reaches the platform layer (30 by default with --headless; 0 is
//                       off, the default with a window)
//   --main-arena-size B the host's main memory arena, in bytes (a K or M suffix multiplies)
//   --vram-backend NAME how VRAM banks are mapped: shm, mach-entry, mach-remap or copy (port/hal/gx/vram.h)
//
// The controls are in port/app/present.c.
//
// Exit statuses: 0 when the run ends normally, 2 for bad options, 70 when the game reaches code or data with no C
// (port/hal/trap.c), 71 when it stops itself (OS_Terminate), 72 when every thread waits for good, 73 for the watchdog,
// 74 when the game resets the console (OS_ResetSystem), which the port can't do in place yet; the modules' own: 65 (a
// save file the port can't use), 66 (no usable ROM), 67 (the PXI bus's arena check).

// POSIX's signals and alarm() for the watchdog, which glibc hides from strict C11 (the target's -std=c11)
#define _DEFAULT_SOURCE

#include "app/present.h"
#include "gfx/ge/ge_hal.h"
#include "gfx/screens.h"
#include "hal/hw/hw.h"
#include "hal/input/port_input.h"
#include "hal/os/port_app.h"
#include "hal/os/port_os.h"
#include "hal/rom/port_rom.h"
#include "hal/trap.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#endif

// The game's main(), from src/system/main.c
void NitroMain(void);

// The watchdog's seconds with --headless, unless --watchdog says otherwise: far more than any frame of the game takes,
// sanitizers included, so that a test or CI run that hangs ends with a report
#define DEFAULT_HEADLESS_WATCHDOG 30

enum {
    EXIT_BAD_OPTIONS = 2,
    EXIT_DEADLOCK = 72,
    EXIT_WATCHDOG = 73,
};

// The options that never take a value, and every option the program and its modules know
static const char *const sFlags[] = { "headless", "help" };
static const char *const sOptions[] = {
    "headless",
    "help",
    "frames",
    "screenshot",
    "rom",
    "save-dir",
    "fixed-time",
    "watchdog",
    "main-arena-size",
    "vram-backend",
    "heap-scale-root",
    "heap-scale-child",
};

// The arguments, which port_app_option reads too (port/hal/os/app_options.c)
static int sArgc;
static char **sArgv;
static int sHeadless;
static unsigned long sFrameLimit;
static unsigned sWatchdogSeconds;
static unsigned long sFrames;
static const char *sScreenshotPath;
static int sScreenshotWritten;
static int sWindowOpen;
// Pacing: the host time of the first paced frame, and the frames since
static Uint64 sPaceStartNs;
static Uint64 sPacedFrames;
// The geometry engine's readback registers, for the I/O registers' accessors (port/hal/hw/hw.h)
static const PortHwG3Readbacks sG3Readbacks = {
    port_g3d_gxstat, port_g3d_ram_count, port_g3d_vtxram_count, port_g3d_clipmtx_result, port_g3d_vecmtx_result,
};

static void Usage(void) {
    fprintf(stderr,
            "usage: %s [--headless] [--frames N] [--screenshot PATH] [--rom PATH] [--save-dir PATH] "
            "[--fixed-time \"YYYY-MM-DD HH:MM:SS\"] [--watchdog SECONDS] [--main-arena-size BYTES] "
            "[--vram-backend shm|mach-entry|mach-remap|copy] [--heap-scale-root N] [--heap-scale-child N]\n",
            sArgc > 0 ? sArgv[0] : "pokeblack2");
}

static int IsKnownOption(const char *name, size_t length) {
    for (size_t i = 0; i < sizeof(sOptions) / sizeof(sOptions[0]); i++) {
        if (strlen(sOptions[i]) == length && strncmp(sOptions[i], name, length) == 0) {
            return 1;
        }
    }
    return 0;
}

// Every argument is a known option (--name or --name=value) or the value of the one before it
static int CheckArguments(void) {
    for (int i = 1; i < sArgc; i++) {
        const char *arg = sArgv[i];
        if (strncmp(arg, "--", 2) == 0) {
            const char *equals = strchr(arg, '=');
            size_t length = equals != NULL ? (size_t)(equals - arg - 2) : strlen(arg + 2);
            if (length == 0) {
                return 0;
            }
            if (!IsKnownOption(arg + 2, length)) {
                fprintf(stderr, "port: unknown option %.*s\n", (int)(length + 2), arg);
                return 0;
            }
            if (equals == NULL && !port_app_is_flag(arg + 2, length) && i + 1 < sArgc &&
                strncmp(sArgv[i + 1], "--", 2) != 0) {
                i++;
            }
        } else {
            return 0;
        }
    }
    return 1;
}

// A count from an option, with an optional K or M suffix when scaled: fallback when the option is absent, -1 when it
// isn't a number of zero or more
static long long CountOption(const char *name, int scaled, long long fallback) {
    const char *value = port_app_option(name);
    char *end;
    long long count;

    if (value == NULL) {
        return fallback;
    }
    errno = 0;
    count = strtoll(value, &end, 10);
    if (scaled && (*end == 'K' || *end == 'k')) {
        count *= 1024;
        end++;
    } else if (scaled && (*end == 'M' || *end == 'm')) {
        count *= 1024 * 1024;
        end++;
    }
    if (errno != 0 || end == value || *end != '\0' || count < 0) {
        fprintf(stderr, "port: --%s needs a number, not \"%s\"\n", name, value);
        return -1;
    }
    return count;
}

// The watchdog: a frame that takes longer than --watchdog seconds of host time means the game spins in a loop that
// never reaches the platform layer (a register or a flag that only the hardware would change). It is the only place
// where host time stops the game, and it only reports
#ifndef _WIN32
static void OnWatchdog(int signal) {
    static const char message[] = "\nport: watchdog: no frame for too long; the game spins where the platform layer "
                                  "never runs\nBacktrace:\n";
    void *frames[64];
    int count;

    (void)signal;
    (void)!write(STDERR_FILENO, message, sizeof(message) - 1);
    count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    _Exit(EXIT_WATCHDOG);
}
#endif

static void KickWatchdog(void) {
#ifndef _WIN32
    if (sWatchdogSeconds != 0) {
        alarm(sWatchdogSeconds);
    }
#endif
}

static void StartWatchdog(void) {
#ifndef _WIN32
    if (sWatchdogSeconds != 0) {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = OnWatchdog;
        sigemptyset(&action.sa_mask);
        sigaction(SIGALRM, &action, NULL);
        KickWatchdog();
    }
#endif
}

// Waits until the frame's time on the DS's clock: 560,190 cycles of 33.514 MHz, 16.715 ms. A host that falls more
// than a few frames behind starts counting again instead of hurrying to catch up
static void Pace(void) {
    // The cycles' nanoseconds are cycles * 10^6 / 33,514, split so that the product can't overflow
    const Uint64 kiloHertz = PORT_OS_CYCLES_PER_SECOND / 1000;
    const Uint64 frameCycleNs = (Uint64)PORT_OS_CYCLES_PER_FRAME * 1000000;
    Uint64 now = SDL_GetTicksNS();
    Uint64 due;

    if (sPacedFrames == 0) {
        sPaceStartNs = now;
    }
    sPacedFrames++;
    due = sPaceStartNs + sPacedFrames / kiloHertz * frameCycleNs + sPacedFrames % kiloHertz * frameCycleNs / kiloHertz;
    if (now < due) {
        SDL_DelayPrecise(due - now);
    } else if (now - due > 4 * SDL_NS_PER_SECOND / 60) {
        sPacedFrames = 0;
    }
}

static void WriteScreenshot(void) {
    if (sScreenshotPath != NULL && !sScreenshotWritten) {
        sScreenshotWritten = 1;
        if (port_present_screenshot(sScreenshotPath)) {
            SDL_Log("port: wrote the screens after frame %lu to %s", sFrames, sScreenshotPath);
        }
    }
}

// The program's frame hook, at each VBlank on the dispatcher's stack: the scanline hook has drawn the frame's 192
// lines by now. The input taken here is what the game's VBlank work and next frame read
static void AppFrame(void) {
    sFrames++;
    if (sWindowOpen) {
        port_present_poll();
    }
    if (port_app_present != NULL) {
        port_app_present();
    }
    if (sFrameLimit != 0 && sFrames >= sFrameLimit) {
        WriteScreenshot();
        port_os_stop();
    }
    if (!sHeadless) {
        Pace();
    }
    KickWatchdog();
}

int main(int argc, char **argv) {
    long long frames;
    long long watchdog;
    long long arenaSize;
    PortOsResult result;
    int status = 0;

    sArgc = argc;
    sArgv = argv;
    port_app_set_arguments(argc, argv, sFlags, sizeof(sFlags) / sizeof(sFlags[0]));
    if (!CheckArguments() || port_app_option("help") != NULL) {
        Usage();
        return port_app_option("help") != NULL ? 0 : EXIT_BAD_OPTIONS;
    }
    sHeadless = port_app_option("headless") != NULL;
    frames = CountOption("frames", 0, 0);
    watchdog = CountOption("watchdog", 0, sHeadless ? DEFAULT_HEADLESS_WATCHDOG : 0);
    arenaSize = CountOption("main-arena-size", 1, 0);
    if (frames < 0 || watchdog < 0 || arenaSize < 0) {
        Usage();
        return EXIT_BAD_OPTIONS;
    }
    sFrameLimit = (unsigned long)frames;
    sWatchdogSeconds = (unsigned)watchdog;
    sScreenshotPath = port_app_option("screenshot");
    if (sScreenshotPath != NULL && *sScreenshotPath == '\0') {
        fprintf(stderr, "port: --screenshot needs a path\n");
        Usage();
        return EXIT_BAD_OPTIONS;
    }
    if (arenaSize != 0) {
        port_os_set_main_arena_size((size_t)arenaSize);
    }

    port_hw_init();
    port_trap_protect_data();
    if (!sHeadless) {
        if (!port_present_open()) {
            return 1;
        }
        sWindowOpen = 1;
        port_app_present = port_present_frame;
    }
    // Nothing pressed, the lid open and the pen up, as the game's first reads must see; then the screens' line drawing
    port_input_set(0, 0);
    port_screens_attach();
    // The geometry engine at power-on, now that the I/O registers exist, its readback registers, and its swap at each
    // VBlank
    port_g3d_reset();
    port_hw_attach_g3_readbacks(&sG3Readbacks);
    port_os_add_frame_hook(port_g3d_vblank);

    port_os_add_frame_hook(AppFrame);
    StartWatchdog();
    SDL_Log("port: entering the game's main()");
    // The ROM before the game, so that its check (and the message when there is none) comes first; the CARD library
    // would open it at CARD_Init otherwise
    port_rom_boot();
    result = port_os_run(NitroMain);
    switch (result) {
    case PORT_OS_STOPPED:
        SDL_Log("port: stopped after %lu frames", sFrames);
        break;
    case PORT_OS_RETURNED:
        SDL_Log("port: the game's main() returned");
        break;
    case PORT_OS_DEADLOCK:
        SDL_Log("port: stopped: every thread waits for an interrupt that nothing can raise");
        status = EXIT_DEADLOCK;
        break;
    }

    // A run that ended before its frame count (the window closed, a deadlock) still leaves its last frame
    WriteScreenshot();
    if (sWindowOpen) {
        port_present_close();
    }
    return status;
}
