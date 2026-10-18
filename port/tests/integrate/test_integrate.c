// Tests of phase 1's integration: the host DMA (port/hal/dma), the OS allocator's 64-bit addresses
// (port/hal/os/os_alloc_host.c) and the main arena's placement, the buttons and lid (port/hal/input), the screens drawn
// by the scanline hook (port/gfx/screens.c), the PNG screenshot (port/app/present.c) and Frigate's test launcher when
// it is off (port/app/launcher.c). Each scenario runs as the game's main() would, as the launcher of port_os_run, on
// the whole program's objects.
//
//   port_integrate_tests NAME | all
#include "app/launcher.h"
#include "app/present.h"
#include "app/run_dir.h"
#include "gfx/screens.h"
#include "hal/hw/hw.h"
#include "hal/input/port_input.h"
#include "hal/os/port_os.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// The 2D renderer's colours (port/gfx/ref/ds2d.h): a 15-bit channel of 31 shows as 0xfb
#define RED_15 0x001f
#define BLUE_15 0x7c00
#define RED_SHOWN 0xfffb0000u
#define BLUE_SHOWN 0xff0000fbu
#define BLACK_SHOWN 0xff000000u
#define WHITE_SHOWN 0xffffffffu

// POWCNT1's bits (GBATEK, "DS Power Control"): the LCDs, engine A, engine B and the display swap (engine A on the top
// screen when set)
#define POWCNT_LCD 0x0001
#define POWCNT_ENGINE_A 0x0002
#define POWCNT_ENGINE_B 0x0200
#define POWCNT_SWAP 0x8000

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

static void Write16(uint32_t address, uint16_t value) {
    memcpy(port_hw_pointer(address), &value, sizeof(value));
}

static void Write32(uint32_t address, uint32_t value) {
    memcpy(port_hw_pointer(address), &value, sizeof(value));
}

static uint16_t Read16(uint32_t address) {
    uint16_t value;

    memcpy(&value, port_hw_pointer(address), sizeof(value));
    return value;
}

// DMA

static void TestDmaCopies(void) {
    u32 words[8];
    u16 halves[8];
    int i;

    for (i = 0; i < 8; i++) {
        words[i] = 0x11111111u * (u32)i;
    }
    MIi_DmaFill32(1, &words[1], 0xdeadbeef, 16, TRUE);
    Check(words[0] == 0 && words[1] == 0xdeadbeef && words[4] == 0xdeadbeef && words[5] == 0x55555555,
          "a fill covers its size in words and nothing more");
    MIi_DmaFill32(1, &words[5], 0x12345678, 8, FALSE);
    Check(words[5] == 0x55555555, "a channel set up without dmaEnable moves nothing");
    MIi_DmaFill32(1, &words[5], 0x12345678, 0, TRUE);
    Check(words[5] == 0x55555555, "a fill of 0 bytes moves nothing");

    // DMA counts up from the first unit, so a copy one word up repeats the first word, as on the DS
    words[0] = 0xcafef00d;
    MIi_DmaCopy32(2, &words[0], &words[1], 16, TRUE);
    Check(words[1] == 0xcafef00d && words[4] == 0xcafef00d && words[5] == 0x55555555,
          "an overlapping copy goes unit by unit from the start");

    for (i = 0; i < 8; i++) {
        halves[i] = (u16)(0x100 + i);
    }
    MIi_DmaCopy16(3, &halves[0], &halves[4], 6, TRUE);
    Check(halves[4] == 0x100 && halves[6] == 0x102 && halves[7] == 0x107, "a halfword copy moves size / 2 units");
    MI_WaitDma(3);
    MI_StopDma(3);
    MI_StopAllDma();
}

static int sCallbacks;

static void CountCallback(void *arg) {
    sCallbacks += (int)(intptr_t)arg;
}

static void TestDmaAsync(void) {
    u32 source[4] = { 1, 2, 3, 4 };
    u32 dest[4] = { 0 };
    u32 enabled;

    sCallbacks = 0;
    (void)OS_EnableIrq();
    MIi_DmaCopy32Async(1, source, dest, sizeof(dest), CountCallback, (void *)(intptr_t)1, TRUE);
    Check(dest[3] == 4, "the copy is done when the call returns");
    Check(sCallbacks == 1, "with interrupts on, the channel's interrupt calls back before the call returns");
    Check((reg_OS_IE & (1u << (OS_IE_DMA0_SHIFT + 1))) == 0, "the interrupt is disabled again after the callback");

    enabled = OS_DisableInterrupts();
    MIi_DmaFill32Async(2, dest, 7, sizeof(dest), CountCallback, (void *)(intptr_t)10, TRUE);
    Check(dest[0] == 7 && dest[3] == 7, "the fill is done when the call returns");
    Check(sCallbacks == 1, "with interrupts off, the callback waits");
    (void)OS_RestoreInterrupts(enabled);
    Check(sCallbacks == 11, "the callback comes when interrupts are enabled again");

    MIi_DmaCopy32Async(3, source, dest, 0, CountCallback, (void *)(intptr_t)100, TRUE);
    Check(sCallbacks == 111, "a copy of 0 bytes calls back at once");
    MIi_DmaCopy32Async(3, source, dest, sizeof(dest), NULL, NULL, TRUE);
    Check(dest[0] == 1, "a copy without a callback is done too");
}

static void TestDmaGxCommand(void) {
    static const u32 list[3] = { 0x00000015, 0x12345678, 0x9abcdef0 };
    const PortG3Write *log;
    size_t count;

    sCallbacks = 0;
    (void)OS_EnableIrq();
    (void)port_hw_g3_take(&count);
    MI_SendGXCommandAsync(1, list, sizeof(list), CountCallback, (void *)(intptr_t)1);
    log = port_hw_g3_take(&count);
    Check(count == 3, "every word of the display list is logged");
    Check(count == 3 && log[0].port == 0x04000400 && log[0].value == list[0] && log[2].value == list[2],
          "the words go to GXFIFO in order");
    Check(sCallbacks == 1, "the list's callback comes from the channel's interrupt");
    MI_SendGXCommandAsyncFast(2, list, 8, CountCallback, (void *)(intptr_t)1);
    log = port_hw_g3_take(&count);
    Check(count == 2 && log[1].value == list[1], "the fast send logs its words too");
    Check(sCallbacks == 2, "and calls back");
}

// The OS allocator and the arenas

static void TestOsAlloc(void) {
    enum { BLOCK_SIZE = 0x10000 };
    u8 *block = malloc(BLOCK_SIZE);
    u8 *end = block + BLOCK_SIZE;
    u8 *window = port_hw_pointer(0x02000000);
    void *lo;
    void *cell;
    int heap;
    int extra;

    // The table at an address aligned for its pointers but not to 32 bytes, so that the free memory's start is rounded
    lo = OS_InitAlloc(OS_ARENA_MAIN, block + 8, end, 2);
    Check((u8 *)lo > block && (u8 *)lo < end, "OS_InitAlloc's free memory is inside the block, whole address kept");
    Check(((uintptr_t)lo & 31) == 0, "OS_InitAlloc rounds the free memory's start up to 32 bytes");
    heap = OS_CreateHeap(OS_ARENA_MAIN, lo, end - 7);
    Check(heap == 0, "OS_CreateHeap makes the first heap");
    cell = OS_AllocFromHeap(OS_ARENA_MAIN, heap, 100);
    Check(cell != NULL && (u8 *)cell > (u8 *)lo && (u8 *)cell < end && ((uintptr_t)cell & 31) == 0,
          "an allocation is in the heap's memory and aligned");
    OS_FreeToHeap(OS_ARENA_MAIN, heap, cell);
    Check(OS_AllocFromHeap(OS_ARENA_MAIN, heap, 100) == cell, "a freed cell is given out again");

    extra = OS_CreateExtraHeap(OS_ARENA_MAIN);
    Check(extra == 1, "OS_CreateExtraHeap takes the next free heap");
    cell = OS_AllocFromHeap(OS_ARENA_MAIN, extra, 64);
    Check((u8 *)cell >= window && (u8 *)cell < window + 0x4000,
          "the extra heap is the 16 KiB at 0x02000000's place in the main memory window");
    Check(OS_CreateExtraHeap(OS_ARENA_MAIN) == -1, "the extra heap is made once");
    free(block);
}

static void TestMainArena(void) {
    void *lo;
    void *hi;

    OS_InitArena();
    lo = OS_GetArenaLo(OS_ARENA_MAIN);
    hi = OS_GetArenaHi(OS_ARENA_MAIN);
    Check(lo != NULL && hi > lo, "the main arena exists");
    Check(port_pxi_unique_low_bits(lo, hi), "no address of the main arena has the low 32 bits of a static one");
}

// Input

static void TestInput(void) {
    u16 keys;

    port_input_set(PORT_INPUT_A | PORT_INPUT_UP | PORT_INPUT_X | PORT_INPUT_L, 0);
    Check(Read16(0x04000130) == (0x03ff & ~(PORT_INPUT_A | PORT_INPUT_UP | PORT_INPUT_L)),
          "KEYINPUT holds A, Up and L clear while pressed");
    Check(Read16(0x02ffffa8) == (0x2c00 & ~PORT_INPUT_X), "the ARM7's word holds X clear, Y and debug set");
    // PAD_Read as src/gfl/key.c and NitroSDK read the two
    keys = (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ 0x2fff) & 0x2fff);
    Check(keys == (PORT_INPUT_A | PORT_INPUT_UP | PORT_INPUT_X | PORT_INPUT_L), "PAD_Read gives the buttons held");
    Check(!PAD_DetectFold(), "the lid is open");
    Check(port_input_held() == (PORT_INPUT_A | PORT_INPUT_UP | PORT_INPUT_X | PORT_INPUT_L), "the state reads back");

    port_input_set(0xffffffffu, 1);
    keys = (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ 0x2fff) & 0x2fff);
    Check(keys == PORT_INPUT_ALL, "every button PAD_Read knows can be held, and nothing else");
    Check(PAD_DetectFold(), "the lid closes");

    port_input_set(0, 0);
    keys = (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ 0x2fff) & 0x2fff);
    Check(keys == 0 && !PAD_DetectFold(), "nothing held and the lid open");
}

// The screens

static int sFrames;

static void CountFrames(void) {
    sFrames++;
}

static int ScreenIs(int screen, uint32_t color) {
    const uint32_t *pixels = port_screens_pixels(screen);

    for (int i = 0; i < PORT_SCREEN_WIDTH * PORT_SCREEN_HEIGHT; i++) {
        if (pixels[i] != color) {
            return 0;
        }
    }
    return 1;
}

// Runs a whole frame and more: the run begins inside line 0, which only the second frame draws
static void RunTwoFrames(void) {
    int frames = sFrames;

    port_os_spin(2 * (uint64_t)PORT_OS_CYCLES_PER_FRAME);
    Check(sFrames == frames + 2, "two frames passed");
}

static void TestScreens(void) {
    // Display mode 1 (graphics) with nothing enabled: each engine shows its backdrop, BG palette colour 0
    Write32(0x04000000, 0x00010000);
    Write32(0x04001000, 0x00010000);
    Write16(0x05000000, RED_15);
    Write16(0x05000400, BLUE_15);

    Write16(0x04000304, POWCNT_LCD | POWCNT_ENGINE_A | POWCNT_ENGINE_B | POWCNT_SWAP);
    RunTwoFrames();
    Check(ScreenIs(PORT_SCREEN_TOP, RED_SHOWN), "with the swap bit set, engine A is on the top screen");
    Check(ScreenIs(PORT_SCREEN_BOTTOM, BLUE_SHOWN), "and engine B on the bottom one");

    Write16(0x04000304, POWCNT_LCD | POWCNT_ENGINE_A | POWCNT_ENGINE_B);
    RunTwoFrames();
    Check(ScreenIs(PORT_SCREEN_TOP, BLUE_SHOWN), "without it, engine B is on the top screen");
    Check(ScreenIs(PORT_SCREEN_BOTTOM, RED_SHOWN), "and engine A on the bottom one");

    Write16(0x04000304, POWCNT_LCD | POWCNT_ENGINE_A | POWCNT_SWAP);
    RunTwoFrames();
    Check(ScreenIs(PORT_SCREEN_BOTTOM, WHITE_SHOWN), "engine B's screen is white while the engine is off");
    Check(ScreenIs(PORT_SCREEN_TOP, RED_SHOWN), "engine A's is unaffected");
    // The screens' hashes, as --run-dir writes them in frames.tsv: a white screen's is the oracle's at power-on
    // (port/tests/oracle/boot_b2_us.tsv, frame 1), and a red one's, bytes fb 00 00 for each pixel, is Python's
    // hashlib's, which shows the channels' order
    {
        char hash[41];

        port_run_dir_screen_hash(port_screens_pixels(PORT_SCREEN_BOTTOM), hash);
        Check(strcmp(hash, "76449f41286e8a2a18554c535b750bf6a958f4c9") == 0, "a white screen's hash is the oracle's");
        port_run_dir_screen_hash(port_screens_pixels(PORT_SCREEN_TOP), hash);
        Check(strcmp(hash, "a26dc35c554a9abdd211fc48eab366a9e2f0af58") == 0, "a red screen's hash is its RGB bytes'");
    }

    Write16(0x04000304, POWCNT_ENGINE_A | POWCNT_ENGINE_B | POWCNT_SWAP);
    RunTwoFrames();
    Check(ScreenIs(PORT_SCREEN_TOP, BLACK_SHOWN) && ScreenIs(PORT_SCREEN_BOTTOM, BLACK_SHOWN),
          "both screens are black while the LCDs are off");

    // A raster change: the backdrop turned blue during line 95, as an HBlank handler would change it, shows from line
    // 96
    Write16(0x04000304, POWCNT_LCD | POWCNT_ENGINE_A | POWCNT_ENGINE_B | POWCNT_SWAP);
    RunTwoFrames();
    while (port_os_vcount() != 95) {
        port_os_spin(PORT_OS_CYCLES_PER_LINE);
    }
    Write16(0x05000000, BLUE_15);
    port_os_spin((uint64_t)(PORT_OS_LINES_PER_FRAME - 95) * PORT_OS_CYCLES_PER_LINE);
    {
        const uint32_t *top = port_screens_pixels(PORT_SCREEN_TOP);

        Check(top[95 * PORT_SCREEN_WIDTH] == RED_SHOWN && top[96 * PORT_SCREEN_WIDTH] == BLUE_SHOWN,
              "each line is drawn with the registers as they are at its start");
    }

    // The screenshot of what is shown now: engine A blue from line 96, engine B blue
    {
        char path[] = "/tmp/port_integrate_XXXXXX.png";
        int fd = mkstemps(path, 4);
        SDL_Surface *loaded;
        SDL_Surface *argb;
        int ok = 0;

        Check(fd >= 0, "a temporary file");
        if (fd >= 0) {
            close(fd);
            Check(port_present_screenshot(path), "the screenshot is written");
            loaded = SDL_LoadPNG(path);
            Check(loaded != NULL && loaded->w == PORT_SCREEN_WIDTH && loaded->h == 2 * PORT_SCREEN_HEIGHT,
                  "it is a 256x384 PNG");
            if (loaded != NULL) {
                argb = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_ARGB8888);
                if (argb != NULL) {
                    ok = 1;
                    for (int screen = 0; screen < 2 && ok; screen++) {
                        const uint32_t *pixels = port_screens_pixels(screen);

                        for (int y = 0; y < PORT_SCREEN_HEIGHT && ok; y++) {
                            const uint8_t *row =
                                (const uint8_t *)argb->pixels + (size_t)(screen * PORT_SCREEN_HEIGHT + y) * argb->pitch;

                            ok = memcmp(row, pixels + y * PORT_SCREEN_WIDTH, PORT_SCREEN_WIDTH * 4) == 0;
                        }
                    }
                    SDL_DestroySurface(argb);
                }
                SDL_DestroySurface(loaded);
            }
            Check(ok, "it holds the top screen above the bottom one, pixel for pixel");
            unlink(path);
        }
    }
}

// Frigate's test launcher (port/app/launcher.c, ADR 0012) without --launch: CONTINUE's seam asks it before the field
// opens (src/ov035/event_mapchange.c), and it must answer 0 without touching the event, so that the field opens as
// before; it never stops the run, and leaves the run's exit status alone
static void TestLauncherOff(void) {
    Check(port_launcher_take_field_open(NULL) == 0, "without --launch the launcher leaves the field to open");
    Check(!port_launcher_finished(), "without --launch the launcher never stops the run");
    Check(port_launcher_end() == PORT_LAUNCH_STATUS_DONE, "without --launch the launcher leaves the status at 0");
}

typedef struct Test {
    const char *name;
    void (*run)(void);
} Test;

static const Test sTests[] = {
    { "dma_copies", TestDmaCopies }, { "dma_async", TestDmaAsync },       { "dma_gx_command", TestDmaGxCommand },
    { "os_alloc", TestOsAlloc },     { "main_arena", TestMainArena },     { "input", TestInput },
    { "screens", TestScreens },      { "launcher_off", TestLauncherOff },
};

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "all";
    int all = strcmp(name, "all") == 0;
    int failures = 0;
    int ran = 0;

    setvbuf(stdout, NULL, _IONBF, 0);
    port_hw_init();
    port_os_add_frame_hook(CountFrames);
    port_screens_attach();
    for (size_t i = 0; i < sizeof(sTests) / sizeof(sTests[0]); i++) {
        if (all || strcmp(name, sTests[i].name) == 0) {
            sFailed = 0;
            Check(port_os_run(sTests[i].run) == PORT_OS_RETURNED, "the run ended normally");
            printf("%s: %s\n", sTests[i].name, sFailed ? "FAIL" : "ok");
            failures += sFailed;
            ran++;
        }
    }
    if (ran == 0) {
        printf("no test named %s\n", name);
        return 2;
    }
    return failures != 0;
}
