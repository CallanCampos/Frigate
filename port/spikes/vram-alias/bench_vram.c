// Timings of the remaps the game makes, for every backend this host has. Usage: bench_vram [iterations]
//
// Single remaps:
//   bank128   one 128 KiB bank between LCDC and BG-A (one VRAMCNT write)
//   bank128t  the same, then one store into each of its 8 pages where it now is: the first touch of a new mapping
//             takes a page fault per 16 KiB page
//   bank16    F between LCDC and OBJ-A (one write; two slots, with the mirror)
//   texload   GX_BeginLoadTex and GX_EndLoadTex of GX_VRAM_TEX_0123_ABCD around a 128 KiB upload through LCDC, then
//             the renderer reads one word per texture page
//   clear     MIi_CpuClear32 over the 512 KiB of BG-A with 256 KiB mapped, as codein.c does (no remap: the cost of
//             plain access through the views)
//
// Scene changes, GFL_BGSysSetVRAMBanks between two settings that differ in every bank but H: 10 GX_ResetBankFor* calls,
// then 10 GX_SetBankFor* calls, each writing VRAMCNT as gx_vramcnt.c does. Three ways to hand the registers over:
//   write     after every register write, as the hardware sees them
//   call      once at the end of each SDK call, as a PLATFORM_PC hook at the end of GX_VRAMCNT_SetLCDC_ and
//             disableBankForX_ would (20 applies)
//   batch     once at the end of GFL_BGSysSetVRAMBanks (or on the next VRAM access)
//   scene+t   "call", then one store into every mapped slot of the CPU views and one read of every mapped GPU slot,
//             which pays the first touches
//
// Steady state, per frame, from battle's setting (btlv_core.c: BG 128_D, sub BG 128_C, OBJ 64_E, sub OBJ 16_I,
// texture 01_AB, texture palette 01_FG), with the "write" and "call" ways:
//   fade4     4 texture palette tasks of 32 bytes: btlv_effect.c's per-model fade, one NNS_GFD_DST_3D_TEX_PLTT task
//             each, run by NNS_GfdDoVramTransfer in the VBlank (GX_BeginLoadTexPltt, GX_LoadTexPltt, GX_EndLoadTexPltt)
//   tex1      1 texture task of 8 KiB, as ctvt_game.c registers (GX_BeginLoadTex, GX_LoadTex, GX_EndLoadTex)
//   tex4pal4  4 texture tasks of 8 KiB and 4 palette tasks of 32 bytes
//   dual3d    GFL_G3DDual3DExecVBlank: C and D trade places between sub BG, sub OBJ and LCDC every frame (4 SDK calls)
#include "vram.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LCDC 0x80

static uint8_t cnt(int mst, int ofs) {
    return (uint8_t)(0x80 | (ofs << 3) | mst);
}

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

static int compare_u64(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}

static void report(const char *backend, const char *scenario, const char *way, uint64_t *samples, int n, double calls) {
    qsort(samples, (size_t)n, sizeof(*samples), compare_u64);
    printf("%-11s %-9s %-6s median %8.2f us   p90 %8.2f us   max %8.2f us", backend, scenario, way,
           samples[n / 2] / 1000.0, samples[n * 9 / 10] / 1000.0, samples[n - 1] / 1000.0);
    if (calls >= 0) {
        printf("   %5.1f mapping calls", calls);
    }
    printf("\n");
}

// The SDK's bookkeeping (gx_vramcnt.c's gGXState): the banks each use holds, the free banks in LCDC, and the VRAMCNT
// registers, which the Vram gets according to the way being measured
enum {
    USE_BG,
    USE_BGEXT,
    USE_SUBBG,
    USE_SUBBGEXT,
    USE_OBJ,
    USE_OBJEXT,
    USE_SUBOBJ,
    USE_SUBOBJEXT,
    USE_TEX,
    USE_TEXPLTT,
    USE_COUNT,
};

typedef enum Way {
    WAY_WRITE,
    WAY_CALL,
    WAY_BATCH,
} Way;

static const char *const WAY_NAME[] = { "write", "call", "batch" };

typedef struct Sdk {
    Vram *vram;
    Way way;
    uint8_t vramcnt[VRAM_BANK_COUNT];
    uint16_t uses[USE_COUNT];
    uint16_t lcdc;
} Sdk;

// A setting of every use: the VRAMCNT value of each bank it takes, 0 for the banks it doesn't
typedef uint8_t Setting[USE_COUNT][VRAM_BANK_COUNT];

// X: BG 256_AB, sub BG 32_H, OBJ 64_E, OBJ extended palette F, sub OBJ 16_I, texture 01_CD, texture palette 0_G.
// Y: BG 128_C, sub BG 48_HI, OBJ 32_FG, sub OBJ 128_D, texture 01_AB, texture palette 0123_E
static const Setting SCENE_X = {
    [USE_BG] = { [VRAM_BANK_A] = 0x81, [VRAM_BANK_B] = 0x89 },
    [USE_SUBBG] = { [VRAM_BANK_H] = 0x81 },
    [USE_OBJ] = { [VRAM_BANK_E] = 0x82 },
    [USE_OBJEXT] = { [VRAM_BANK_F] = 0x85 },
    [USE_SUBOBJ] = { [VRAM_BANK_I] = 0x82 },
    [USE_TEX] = { [VRAM_BANK_C] = 0x83, [VRAM_BANK_D] = 0x8b },
    [USE_TEXPLTT] = { [VRAM_BANK_G] = 0x83 },
};
static const Setting SCENE_Y = {
    [USE_BG] = { [VRAM_BANK_C] = 0x81 },
    [USE_SUBBG] = { [VRAM_BANK_H] = 0x81, [VRAM_BANK_I] = 0x81 },
    [USE_OBJ] = { [VRAM_BANK_F] = 0x82, [VRAM_BANK_G] = 0x8a },
    [USE_SUBOBJ] = { [VRAM_BANK_D] = 0x84 },
    [USE_TEX] = { [VRAM_BANK_A] = 0x83, [VRAM_BANK_B] = 0x8b },
    [USE_TEXPLTT] = { [VRAM_BANK_E] = 0x83 },
};
// btlv_core.c's setting for a battle
static const Setting SCENE_BATTLE = {
    [USE_BG] = { [VRAM_BANK_D] = 0x81 },
    [USE_SUBBG] = { [VRAM_BANK_C] = 0x84 },
    [USE_OBJ] = { [VRAM_BANK_E] = 0x82 },
    [USE_SUBOBJ] = { [VRAM_BANK_I] = 0x82 },
    [USE_TEX] = { [VRAM_BANK_A] = 0x83, [VRAM_BANK_B] = 0x8b },
    [USE_TEXPLTT] = { [VRAM_BANK_F] = 0x83, [VRAM_BANK_G] = 0x8b },
};

static void sdk_write(Sdk *sdk, int bank, uint8_t value) {
    sdk->vramcnt[bank] = value;
    if (sdk->way == WAY_WRITE) {
        vram_set_vramcnt(sdk->vram, bank, value);
    }
}

// The end of an SDK call: every one that writes VRAMCNT ends in GX_VRAMCNT_SetLCDC_ or disableBankForX_
static void sdk_call_end(Sdk *sdk) {
    if (sdk->way == WAY_CALL) {
        vram_set_all_vramcnt(sdk->vram, sdk->vramcnt);
    }
}

// The end of a batch of SDK calls, or a VRAM access after them
static void sdk_batch_end(Sdk *sdk) {
    if (sdk->way == WAY_BATCH) {
        vram_set_all_vramcnt(sdk->vram, sdk->vramcnt);
    }
}

// GX_VRAMCNT_SetLCDC_
static void sdk_set_lcdc_(Sdk *sdk, uint16_t banks) {
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        if (banks & (1 << b)) {
            sdk_write(sdk, b, LCDC);
        }
    }
}

// GX_ResetBankForX: the use's banks go to LCDC
static uint16_t sdk_reset(Sdk *sdk, int use) {
    uint16_t banks = sdk->uses[use];

    sdk->uses[use] = 0;
    sdk->lcdc |= banks;
    sdk_set_lcdc_(sdk, banks);
    sdk_call_end(sdk);
    return banks;
}

// GX_SetBankForX: the new banks' registers, then every free bank to LCDC
static void sdk_set(Sdk *sdk, int use, const uint8_t setting[VRAM_BANK_COUNT]) {
    uint16_t banks = 0;

    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        if (setting[b] != 0) {
            banks |= (uint16_t)(1 << b);
        }
    }
    sdk->lcdc = (sdk->lcdc | sdk->uses[use]) & ~banks;
    sdk->uses[use] = banks;
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        if (setting[b] != 0) {
            sdk_write(sdk, b, setting[b]);
        }
    }
    sdk_set_lcdc_(sdk, sdk->lcdc);
    sdk_call_end(sdk);
}

// GX_SetBankForLCDC
static void sdk_set_bank_for_lcdc(Sdk *sdk, uint16_t banks) {
    sdk->lcdc |= banks;
    sdk_set_lcdc_(sdk, banks);
    sdk_call_end(sdk);
}

// GFL_BGSysSetVRAMBanks: every use reset, then every use set, in its order
static const int GFL_USE_ORDER[USE_COUNT] = {
    USE_BG, USE_BGEXT, USE_SUBBG, USE_SUBBGEXT, USE_OBJ, USE_OBJEXT, USE_SUBOBJ, USE_SUBOBJEXT, USE_TEX, USE_TEXPLTT,
};

static void gfl_set_vram_banks(Sdk *sdk, const Setting setting) {
    for (int i = 0; i < USE_COUNT; i++) {
        sdk_reset(sdk, GFL_USE_ORDER[i]);
    }
    for (int i = 0; i < USE_COUNT; i++) {
        sdk_set(sdk, GFL_USE_ORDER[i], setting[GFL_USE_ORDER[i]]);
    }
    sdk_batch_end(sdk);
}

static void sdk_init(Sdk *sdk, Vram *vram, Way way, const Setting setting) {
    memset(sdk, 0, sizeof(*sdk));
    sdk->vram = vram;
    sdk->way = WAY_BATCH;
    sdk->lcdc = 0x1ff;
    memset(sdk->vramcnt, LCDC, sizeof(sdk->vramcnt));
    vram_set_all_vramcnt(vram, sdk->vramcnt);
    gfl_set_vram_banks(sdk, setting);
    sdk->way = way;
}

// One NNS_GfdDoVramTransfer task of the 3D kinds: the Begin takes the use's banks into LCDC, the CPU copies, the End
// gives them back
static void transfer_task(Sdk *sdk, int use, const uint8_t *src, size_t size, size_t offset) {
    uint8_t saved[VRAM_BANK_COUNT];
    uint16_t banks;
    int first = 0;

    memcpy(saved, sdk->vramcnt, sizeof(saved));
    banks = sdk_reset(sdk, use);
    sdk_batch_end(sdk);
    while (first < VRAM_BANK_COUNT && !(banks & (1 << first))) {
        first++;
    }
    memcpy(vram_view(sdk->vram, VRAM_VIEW_LCDC) + ((size_t)gVramBankFirstPage[first] << VRAM_SLOT_SHIFT) + offset, src,
           size);
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        if (!(banks & (1 << b))) {
            saved[b] = 0;
        }
    }
    sdk_set(sdk, use, saved);
    sdk_batch_end(sdk);
}

// GFL_G3DDual3DSetupTopScreen and GFL_G3DDual3DSetupBottomScreen
static void dual3d_frame(Sdk *sdk, bool top) {
    static const uint8_t SUB_BG_128_C[VRAM_BANK_COUNT] = { [VRAM_BANK_C] = 0x84 };
    static const uint8_t SUB_OBJ_128_D[VRAM_BANK_COUNT] = { [VRAM_BANK_D] = 0x84 };

    if (top) {
        sdk_reset(sdk, USE_SUBOBJ);
        sdk_set(sdk, USE_SUBBG, SUB_BG_128_C);
        sdk_set_bank_for_lcdc(sdk, 1 << VRAM_BANK_D);
    } else {
        sdk_reset(sdk, USE_SUBBG);
        sdk_set(sdk, USE_SUBOBJ, SUB_OBJ_128_D);
        sdk_set_bank_for_lcdc(sdk, 1 << VRAM_BANK_C);
    }
    sdk_batch_end(sdk);
}

// One store into each mapped slot of a CPU view and one load from each mapped GPU slot
static void touch_mapped(Vram *v) {
    for (int view = 0; view < VRAM_VIEW_COUNT; view++) {
        for (int slot = 0; slot < vram_view_slot_count(view); slot++) {
            volatile uint32_t *p = (volatile uint32_t *)vram_slot_ptr(v, view, slot);
            if (p == NULL) {
                continue;
            }
            if (view < VRAM_CPU_VIEW_COUNT) {
                *p = (uint32_t)slot;
            } else {
                (void)*p;
            }
        }
    }
}

static double calls_per_iteration(Vram *v, uint32_t before, int n) {
    if (vram_backend(v) == VRAM_BACKEND_COPY) {
        return -1;
    }
    return (double)(vram_stats(v)->os_calls - before) / n;
}

static void bench_singles(Vram *v, const char *name, uint64_t *samples, int n, const uint8_t *upload) {
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        vram_set_vramcnt(v, b, LCDC);
    }
    for (int i = 0; i < n; i++) {
        uint64_t t = now_ns();
        vram_set_vramcnt(v, VRAM_BANK_A, (i & 1) == 0 ? cnt(1, 0) : LCDC);
        samples[i] = now_ns() - t;
    }
    report(name, "bank128", "", samples, n, -1);

    for (int i = 0; i < n; i++) {
        uint8_t *at = vram_view(v, (i & 1) == 0 ? VRAM_VIEW_BG_A : VRAM_VIEW_LCDC);
        uint64_t t = now_ns();
        vram_set_vramcnt(v, VRAM_BANK_A, (i & 1) == 0 ? cnt(1, 0) : LCDC);
        for (int p = 0; p < 8; p++) {
            *(volatile uint32_t *)(at + ((size_t)p << VRAM_SLOT_SHIFT)) = (uint32_t)i;
        }
        samples[i] = now_ns() - t;
    }
    report(name, "bank128t", "", samples, n, -1);

    for (int i = 0; i < n; i++) {
        uint64_t t = now_ns();
        vram_set_vramcnt(v, VRAM_BANK_F, (i & 1) == 0 ? cnt(2, 0) : LCDC);
        samples[i] = now_ns() - t;
    }
    report(name, "bank16", "", samples, n, -1);

    // GX_VRAM_TEX_0123_ABCD
    for (int b = 0; b < 4; b++) {
        vram_set_vramcnt(v, b, cnt(3, b));
    }
    for (int i = 0; i < n; i++) {
        uint64_t t = now_ns();
        volatile uint32_t sum = 0;
        for (int b = 0; b < 4; b++) {
            vram_set_vramcnt(v, b, LCDC);
        }
        memcpy(vram_view(v, VRAM_VIEW_LCDC), upload, 0x20000);
        for (int b = 0; b < 4; b++) {
            vram_set_vramcnt(v, b, cnt(3, b));
        }
        for (int s = 0; s < 32; s++) {
            sum += *(uint32_t *)vram_slot_ptr(v, VRAM_VIEW_TEX, s);
        }
        samples[i] = now_ns() - t;
    }
    report(name, "texload", "", samples, n, -1);

    // GX_VRAM_BG_256_AB
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        vram_set_vramcnt(v, b, LCDC);
    }
    vram_set_vramcnt(v, VRAM_BANK_A, cnt(1, 0));
    vram_set_vramcnt(v, VRAM_BANK_B, cnt(1, 1));
    for (int i = 0; i < n; i++) {
        uint64_t t = now_ns();
        memset(vram_view(v, VRAM_VIEW_BG_A), (int)i, gVramViewSize[VRAM_VIEW_BG_A]);
        samples[i] = now_ns() - t;
    }
    report(name, "clear", "", samples, n, -1);
}

static void bench_scenes(Vram *v, const char *name, uint64_t *samples, int n) {
    for (int way = WAY_WRITE; way <= WAY_BATCH; way++) {
        Sdk sdk;
        uint32_t calls;

        sdk_init(&sdk, v, (Way)way, SCENE_Y);
        calls = vram_stats(v)->os_calls;
        for (int i = 0; i < n; i++) {
            uint64_t t = now_ns();
            gfl_set_vram_banks(&sdk, (i & 1) == 0 ? SCENE_X : SCENE_Y);
            samples[i] = now_ns() - t;
        }
        report(name, "scene", WAY_NAME[way], samples, n, calls_per_iteration(v, calls, n));
    }
    {
        Sdk sdk;

        sdk_init(&sdk, v, WAY_CALL, SCENE_Y);
        for (int i = 0; i < n; i++) {
            uint64_t t = now_ns();
            gfl_set_vram_banks(&sdk, (i & 1) == 0 ? SCENE_X : SCENE_Y);
            touch_mapped(v);
            samples[i] = now_ns() - t;
        }
        report(name, "scene+t", "call", samples, n, -1);
    }
}

static void bench_frames(Vram *v, const char *name, uint64_t *samples, int n, const uint8_t *upload) {
    static const char *const SCENARIOS[] = { "fade4", "tex1", "tex4pal4", "dual3d" };

    for (int scenario = 0; scenario < 4; scenario++) {
        for (int way = WAY_WRITE; way <= WAY_CALL; way++) {
            Sdk sdk;
            uint32_t calls;

            sdk_init(&sdk, v, (Way)way, SCENE_BATTLE);
            if (scenario == 3) {
                // The dual 3D demos keep C and D for the captures; start as after a bottom screen frame
                static const uint8_t NONE[VRAM_BANK_COUNT] = { 0 };
                sdk.way = WAY_BATCH;
                sdk_set(&sdk, USE_BG, NONE);
                dual3d_frame(&sdk, false);
                sdk.way = (Way)way;
            }
            calls = vram_stats(v)->os_calls;
            for (int i = 0; i < n; i++) {
                uint64_t t = now_ns();
                switch (scenario) {
                case 0:
                    for (int k = 0; k < 4; k++) {
                        transfer_task(&sdk, USE_TEXPLTT, upload, 32, (size_t)k * 32);
                    }
                    break;
                case 1:
                    transfer_task(&sdk, USE_TEX, upload, 0x2000, 0);
                    break;
                case 2:
                    for (int k = 0; k < 4; k++) {
                        transfer_task(&sdk, USE_TEX, upload, 0x2000, (size_t)k * 0x2000);
                        transfer_task(&sdk, USE_TEXPLTT, upload, 32, (size_t)k * 32);
                    }
                    break;
                case 3:
                    dual3d_frame(&sdk, (i & 1) == 0);
                    break;
                }
                samples[i] = now_ns() - t;
            }
            report(name, SCENARIOS[scenario], WAY_NAME[way], samples, n, calls_per_iteration(v, calls, n));
        }
    }
}

static void bench_backend(VramBackend backend, int n) {
    const char *name = gVramBackendName[backend];
    uint64_t *samples = calloc((size_t)n, sizeof(*samples));
    uint8_t *upload = malloc(0x20000);
    Vram *v = vram_create(backend, 0, NULL);

    if (v == NULL) {
        printf("%-11s not available on this host (%s)\n", name, strerror(errno));
        free(samples);
        free(upload);
        return;
    }
    memset(upload, 0x5a, 0x20000);
    for (int b = 0; b < VRAM_BANK_COUNT; b++) {
        vram_set_vramcnt(v, b, LCDC);
    }
    memset(vram_view(v, VRAM_VIEW_LCDC), 1, gVramViewSize[VRAM_VIEW_LCDC]);

    bench_singles(v, name, samples, n, upload);
    bench_scenes(v, name, samples, n);
    bench_frames(v, name, samples, n, upload);

    vram_destroy(v);
    free(samples);
    free(upload);
}

int main(int argc, char **argv) {
    int n = argc >= 2 ? atoi(argv[1]) : 2000;

    if (n < 10) {
        n = 10;
    }
    printf("%d iterations per scenario, times per iteration (a scene change or a frame)\n", n);
    for (int backend = 0; backend < VRAM_BACKEND_COUNT; backend++) {
        bench_backend(backend, n);
    }
    return 0;
}
