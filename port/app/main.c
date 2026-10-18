// Frigate's entry point: reads the options, opens the window, prepares the platform layer and runs the game's main()
// (renamed NitroMain by the build) on the OS core (port/hal/os). Everything runs on this one thread: the game and its
// SDK threads on fibers, and the platform's own work on the dispatcher's stack: the scanline hook, which draws each
// line of both screens (port/gfx/screens.c), and at each VBlank the geometry engine's swap (port/gfx/ge/ge_hal.h) and
// this file's frame hook, which takes the player's input, presents the frame (port/app/present.c) and paces to the DS's
// 59.8261 frames a second. The sound the ARM7's driver makes goes to the default playback device (port/audio).
//
//   frigate-black2 [--help] [--headless] [--frames N] [--screenshot PATH] [--audio-dump PATH] [--input FILE]
//                  [--rom PATH] [--save-dir PATH] [--fixed-time TIME] [--run-dir DIR] [--run-screens LIST]
//                  [--run-dump SPEC]... [--sound-trace PATH] [--watchdog SECONDS] [--main-arena-size BYTES]
//                  [--vram-backend NAME] [--heap-scale-root N] [--heap-scale-child N]
//
//   --help              prints what Frigate is and this usage, then exits
//   --headless          runs without a window, without pacing and without a sound device, for tests and CI
//   --frames N          runs N frames, then exits with status 0
//   --screenshot PATH   writes both screens as a PNG (top above bottom) when the run ends: after the last frame of
//                       --frames N, or when the window is closed
//   --audio-dump PATH   writes the sound as it is made to a WAV file, with or without --headless
//   --input FILE        plays the buttons, lid and touch of an input file instead of the window's (port/app/replay.h);
//                       its time line (or the format's default) sets the clock unless --fixed-time is given
//   --rom PATH          the player's ROM (port/hal/rom)
//   --save-dir PATH     where saves go (port/hal/card)
//   --fixed-time TIME   the clock's time at boot, "YYYY-MM-DD HH:MM:SS", moving with the virtual clock (port/hal/rtc)
//   --run-dir DIR       writes a run directory for the melonDS oracle's compare.py (port/app/run_dir.h): frames.tsv
//                       with a row at every frame (the video frame, NitroSDK's VBlank count, GFL's frame count, the
//                       game frame and each screen's hash), and run.txt
//   --run-screens LIST  the frames whose screens --run-dir also writes as frame_NNNNNN.png: N, A-B, A-B/STEP, all or
//                       none, comma-separated, as pbw-oracle's --screens (none by default)
//   --run-dump SPEC     memory --run-dir also writes to memory.txt, as pbw-oracle's --dump: [LABEL=][*]SYMBOL[+OFFSET]
//                       :SIZE[@LIST], a global of the program by name (statics aren't visible), * for where the
//                       pointer there points, at the frames of LIST (the last of --frames by default); repeatable
//   --sound-trace PATH  writes the sound driver's state after each run of its thread, as pbw-oracle's --sound-trace
//                       does the DS's (port/app/sound_trace.h)
//   --watchdog SECONDS  stops with a report and status 73 when a frame takes longer than that in host time: the game
//                       hangs in a loop that never reaches the platform layer (30 by default with --headless; 0 is
//                       off, the default with a window)
//   --main-arena-size B the host's main memory arena, in bytes (a K or M suffix multiplies)
//   --vram-backend NAME how VRAM banks are mapped: shm, mach-entry, mach-remap or copy (port/hal/gx/vram.h)
//   --heap-scale-root N, --heap-scale-child N
//                       how many times their DS size the game's root heaps and child heaps get, 1 to 64 (4 and 2 by
//                       default, port/hal/gfl/heap_policy.c)
//
// The controls are in port/app/present.c. Run from an app with stderr on /dev/null, as the Finder opens it, the
// program writes its messages to ~/Library/Logs/Frigate/<program>.log instead (port/app/message_log.h).
//
// Exit statuses: 0 when the run ends normally, 1 when it can't go on (the window didn't open, a write to the run
// directory or the sound trace failed, or a module ran out of memory or found a build that can't work, each with a
// "port:" line), 2 for bad options, 70 when the game reaches code or data with no C (port/hal/trap.c), 71 when it stops
// itself (OS_Terminate), 72 when every thread waits for good, 73 for the watchdog, 74 when the game resets the console
// (OS_ResetSystem), which the port can't do in place yet; the modules' own: 65 (a save file the port can't use), 66 (no
// usable ROM), 67 (the PXI bus's arena check). port/docs/ARCHITECTURE.md, "How a run ends", has the table.

// POSIX's signals and alarm() for the watchdog, which glibc hides from strict C11 (the target's -std=c11)
#define _DEFAULT_SOURCE

#include "app/message_log.h"
#include "app/present.h"
#include "app/replay.h"
#include "app/run_dir.h"
#include "app/sound_trace.h"
#include "app/symbols.h"
#include "audio/audio.h"
#include "gfx/ge/ge_hal.h"
#include "gfx/screens.h"
#include "hal/gfl/gfl_frame.h"
#include "hal/hw/hw.h"
#include "hal/input/port_input.h"
#include "hal/os/port_app.h"
#include "hal/os/port_os.h"
#include "hal/rom/port_rom.h"
#include "hal/rom/rom_manifest.h"
#include "hal/rtc/rtc_host.h"
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
    "headless",    "help",     "frames",          "screenshot",   "audio-dump",      "input",
    "rom",         "save-dir", "fixed-time",      "run-dir",      "run-screens",     "run-dump",
    "sound-trace", "watchdog", "main-arena-size", "vram-backend", "heap-scale-root", "heap-scale-child",
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
static int sRunDirFailed;
static int sWindowOpen;
// Pacing: the host time of the first paced frame, and the frames since
static Uint64 sPaceStartNs;
static Uint64 sPacedFrames;
// The geometry engine's readback registers, for the I/O registers' accessors (port/hal/hw/hw.h)
static const PortHwG3Readbacks sG3Readbacks = {
    port_g3d_gxstat, port_g3d_ram_count, port_g3d_vtxram_count, port_g3d_clipmtx_result, port_g3d_vecmtx_result,
};

// What --help says before the usage: the product, the game this program runs, and that it is a fan project
// (port/hal/os/port_app.h)
static void About(void) {
    fprintf(stderr, "%s — %s\n%s\n%s\n\n", PORT_PRODUCT_NAME, port_rom_manifest_game, PORT_PRODUCT_DESCRIPTION,
            PORT_PRODUCT_DISCLAIMER);
}

static void Usage(void) {
    fprintf(stderr,
            "usage: %s [--headless] [--frames N] [--screenshot PATH] "
            "[--audio-dump PATH] [--input FILE] [--rom PATH] [--save-dir PATH] "
            "[--fixed-time \"YYYY-MM-DD HH:MM:SS\"] [--run-dir DIR] [--run-screens LIST] "
            "[--run-dump [LABEL=][*]SYMBOL[+OFFSET]:SIZE[@LIST]]... [--sound-trace PATH] [--watchdog SECONDS] "
            "[--main-arena-size BYTES] "
            "[--vram-backend shm|mach-entry|mach-remap|copy] [--heap-scale-root N] [--heap-scale-child N]\n",
            sArgc > 0 ? sArgv[0] : "frigate");
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

// The run directory's row of this frame, with its PNG file if --run-screens has it. NitroSDK's VBlank count is the
// OS core's count of VBlanks, which the port's ARM7 side keeps in the shared memory at the VBlank itself
static void RunDirFrame(void) {
    PortRunDirFrame frame;

    frame.frame = (uint32_t)sFrames;
    frame.vblank = port_os_frame();
    frame.counter = port_gfl_frame_count();
    frame.gameFrame = port_replay_game_frame();
    if (!port_run_dir_frame(&frame, port_screens_pixels(PORT_SCREEN_TOP), port_screens_pixels(PORT_SCREEN_BOTTOM))) {
        sRunDirFailed = 1;
        port_os_stop();
    }
}

// Opens --run-dir with what run.txt says of the run: the clock's time is --fixed-time's, else the input file's (or the
// format's default), else the host's
static int OpenRunDir(const char *dir, const char *inputPath) {
    const char *program = strrchr(sArgv[0], '/');
    const char *rom = port_app_option("rom");
    const char *time = port_app_option("fixed-time");
    PortRunDirInfo info;

    info.program = program != NULL ? program + 1 : sArgv[0];
    info.rom = rom != NULL ? rom : port_rom_default_path();
    info.time = time != NULL ? time : inputPath != NULL ? port_replay_time() : "host";
    info.input = inputPath;
    info.inputFrames = port_replay_frames();
    info.frames = (uint32_t)sFrameLimit;
    return port_run_dir_open(dir, port_app_option("run-screens"), port_present_screenshot, &info);
}

// Every --run-dump, as --run-dump SPEC or --run-dump=SPEC, into the open run directory
static int AddRunDumps(void) {
    static const char option[] = "--run-dump";
    const size_t length = sizeof(option) - 1;

    for (int i = 1; i < sArgc; i++) {
        const char *spec = NULL;
        char error[256];

        if (strncmp(sArgv[i], option, length) != 0) {
            continue;
        }
        if (sArgv[i][length] == '=') {
            spec = sArgv[i] + length + 1;
        } else if (sArgv[i][length] == '\0' && i + 1 < sArgc && strncmp(sArgv[i + 1], "--", 2) != 0) {
            spec = sArgv[++i];
        } else if (sArgv[i][length] != '\0') {
            continue;
        }
        if (spec == NULL || !port_run_dir_add_dump(spec, port_symbol_address, error, sizeof(error))) {
            fprintf(stderr, "port: --run-dump %s: %s\n", spec != NULL ? spec : "",
                    spec != NULL ? error : "needs a spec");
            return 0;
        }
    }
    return 1;
}

// The program's frame hook, at each VBlank on the dispatcher's stack: the scanline hook has drawn the frame's 192
// lines by now. The input taken here is what the game's VBlank work and next frame read
static void AppFrame(void) {
    sFrames++;
    if (sWindowOpen) {
        port_present_poll();
    }
    // The game clock, and a replay's input in place of what the window took
    port_replay_vblank((uint32_t)sFrames);
    if (port_run_dir_active()) {
        RunDirFrame();
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
    const char *audioDump;
    const char *soundTrace;
    const char *inputPath;
    const char *runDir;
    PortOsResult result;
    int status = 0;
    // Before anything is written: an app opened from the Finder writes its messages to a log (port/app/message_log.h)
    const char *logPath = port_message_log_open();

    sArgc = argc;
    sArgv = argv;
    port_app_set_arguments(argc, argv, sFlags, sizeof(sFlags) / sizeof(sFlags[0]));
    if (!CheckArguments() || port_app_option("help") != NULL) {
        if (port_app_option("help") != NULL) {
            About();
        }
        Usage();
        return port_app_option("help") != NULL ? 0 : EXIT_BAD_OPTIONS;
    }
    sHeadless = port_app_option("headless") != NULL;
    // And it shows the reason for a stop the player can act on, with a window, since nobody reads its log at once
    if (logPath != NULL && !sHeadless) {
        port_app_fatal_notice = port_message_log_alert;
    }
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
    audioDump = port_app_option("audio-dump");
    if (audioDump != NULL && *audioDump == '\0') {
        fprintf(stderr, "port: --audio-dump needs a path\n");
        Usage();
        return EXIT_BAD_OPTIONS;
    }
    soundTrace = port_app_option("sound-trace");
    if (soundTrace != NULL && *soundTrace == '\0') {
        fprintf(stderr, "port: --sound-trace needs a path\n");
        Usage();
        return EXIT_BAD_OPTIONS;
    }
    inputPath = port_app_option("input");
    if (inputPath != NULL && *inputPath == '\0') {
        fprintf(stderr, "port: --input needs a file\n");
        Usage();
        return EXIT_BAD_OPTIONS;
    }
    if (inputPath != NULL) {
        if (!port_replay_open(inputPath)) {
            return EXIT_BAD_OPTIONS;
        }
        // The file's time is the clock's unless --fixed-time says otherwise (port/hal/rtc reads that at boot)
        if (port_app_option("fixed-time") == NULL && !port_rtc_set_fixed_time(port_replay_time())) {
            fprintf(stderr, "port: --input %s: its time \"%s\" isn't a time the clock can hold\n", inputPath,
                    port_replay_time());
            return EXIT_BAD_OPTIONS;
        }
    }
    runDir = port_app_option("run-dir");
    if ((runDir != NULL && *runDir == '\0') ||
        (runDir == NULL && (port_app_option("run-screens") != NULL || port_app_option("run-dump") != NULL))) {
        fprintf(stderr, "port: --run-dir needs a directory, and --run-screens and --run-dump need --run-dir\n");
        Usage();
        return EXIT_BAD_OPTIONS;
    }
    if (runDir != NULL && (!OpenRunDir(runDir, inputPath) || !AddRunDumps())) {
        port_run_dir_close(EXIT_BAD_OPTIONS);
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
        // The sound device; a host without one runs the game silently, so its absence only gets port_audio_open's
        // message
        port_audio_open();
    }
    // The WAV dump asked for, with or without a device: a path that can't be written is a bad option
    if (audioDump != NULL && !port_audio_dump_open(audioDump)) {
        port_audio_close();
        if (sWindowOpen) {
            port_present_close();
        }
        return EXIT_BAD_OPTIONS;
    }
    if (soundTrace != NULL && !port_sound_trace_open(soundTrace)) {
        port_audio_close();
        if (sWindowOpen) {
            port_present_close();
        }
        return EXIT_BAD_OPTIONS;
    }
    // Nothing pressed, the lid open and the pen up, as the game's first reads must see, or a replay's first frame; then
    // the screens' line drawing
    port_input_set(0, 0);
    if (port_replay_active()) {
        port_replay_start();
    }
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
    // The device and the dump's WAV header; a run that stops the process itself (a trap, OS_Terminate) never gets here,
    // so its dump keeps the header it was opened with, which counts no samples
    port_audio_close();
    if (!port_sound_trace_close() || sRunDirFailed) {
        status = 1;
    }
    port_run_dir_close(status);
    port_replay_close();
    if (sWindowOpen) {
        port_present_close();
    }
    return status;
}
