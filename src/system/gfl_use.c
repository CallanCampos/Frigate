#include "types.h"
#include "gfl/arc.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/dma_vblank.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/textprint.h"
#include "gfl/ui.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "nnsys/snd.h"
#include "system/dsi.h"
#include "system/gfl_use.h"
#include "system/net_define.h"
#include "system/save_error.h"

// The game's setup of the GFL library, which the ROM names: the heaps, the archive paths, the tasks that run in the
// HBlank and after the VBlank, the VBlank callback and the linear congruential random numbers. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except those of the types and the statics

#define ARC_PATH_COUNT 308
#define HBLANK_TCB_COUNT 4
#define VBLANK_TCB_COUNT 16
// The size of heap 9, which a DS takes from the main arena's OS heap and a DSi from heap 1
#define EXTRA_HEAP_SIZE 0x3f00

// The bits of the console type (func_0207ac24) that have a failed assert show its message
#define CONSOLE_TYPE_DEBUG_MASK 0x81000000

typedef struct {
    MATHRandContext32 rand;
    TCBManager *hblankTCBMgr;
    void *hblankTCBWork;
    TCBManager *vblankTCBMgr;
    void *vblankTCBWork;
} GFLUseWork;

static void finishFunc(void);

// The file system paths of the archives, by archive ID
static const char *NARC_PATH_LIST[ARC_PATH_COUNT] = {
// clang-format off
    "a/0/0/0", "a/0/0/1", "a/0/0/2", "a/0/0/3", "a/0/0/4", "a/0/0/5", "a/0/0/6", "a/0/0/7", "a/0/0/8", "a/0/0/9",
    "a/0/1/0", "a/0/1/1", "a/0/1/2", "a/0/1/3", "a/0/1/4", "a/0/1/5", "a/0/1/6", "a/0/1/7", "a/0/1/8", "a/0/1/9",
    "a/0/2/0", "a/0/2/1", "a/0/2/2", "a/0/2/3", "a/0/2/4", "a/0/2/5", "a/0/2/6", "a/0/2/7", "a/0/2/8", "a/0/2/9",
    "a/0/3/0", "a/0/3/1", "a/0/3/2", "a/0/3/3", "a/0/3/4", "a/0/3/5", "a/0/3/6", "a/0/3/7", "a/0/3/8", "a/0/3/9",
    "a/0/4/0", "a/0/4/1", "a/0/4/2", "a/0/4/3", "a/0/4/4", "a/0/4/5", "a/0/4/6", "a/0/4/7", "a/0/4/8", "a/0/4/9",
    "a/0/5/0", "a/0/5/1", "a/0/5/2", "a/0/5/3", "a/0/5/4", "a/0/5/5", "a/0/5/6", "a/0/5/7", "a/0/5/8", "a/0/5/9",
    "a/0/6/0", "a/0/6/1", "a/0/6/2", "a/0/6/3", "a/0/6/4", "a/0/6/5", "a/0/6/6", "a/0/6/7", "a/0/6/8", "a/0/6/9",
    "a/0/7/0", "a/0/7/1", "a/0/7/2", "a/0/7/3", "a/0/7/4", "a/0/7/5", "a/0/7/6", "a/0/7/7", "a/0/7/8", "a/0/7/9",
    "a/0/8/0", "a/0/8/1", "a/0/8/2", "a/0/8/3", "a/0/8/4", "a/0/8/5", "a/0/8/6", "a/0/8/7", "a/0/8/8", "a/0/8/9",
    "a/0/9/0", "a/0/9/1", "a/0/9/2", "a/0/9/3", "a/0/9/4", "a/0/9/5", "a/0/9/6", "a/0/9/7", "a/0/9/8", "a/0/9/9",
    "a/1/0/0", "a/1/0/1", "a/1/0/2", "a/1/0/3", "a/1/0/4", "a/1/0/5", "a/1/0/6", "a/1/0/7", "a/1/0/8", "a/1/0/9",
    "a/1/1/0", "a/1/1/1", "a/1/1/2", "a/1/1/3", "a/1/1/4", "a/1/1/5", "a/1/1/6", "a/1/1/7", "a/1/1/8", "a/1/1/9",
    "a/1/2/0", "a/1/2/1", "a/1/2/2", "a/1/2/3", "a/1/2/4", "a/1/2/5", "a/1/2/6", "a/1/2/7", "a/1/2/8", "a/1/2/9",
    "a/1/3/0", "a/1/3/1", "a/1/3/2", "a/1/3/3", "a/1/3/4", "a/1/3/5", "a/1/3/6", "a/1/3/7", "a/1/3/8", "a/1/3/9",
    "a/1/4/0", "a/1/4/1", "a/1/4/2", "a/1/4/3", "a/1/4/4", "a/1/4/5", "a/1/4/6", "a/1/4/7", "a/1/4/8", "a/1/4/9",
    "a/1/5/0", "a/1/5/1", "a/1/5/2", "a/1/5/3", "a/1/5/4", "a/1/5/5", "a/1/5/6", "a/1/5/7", "a/1/5/8", "a/1/5/9",
    "a/1/6/0", "a/1/6/1", "a/1/6/2", "a/1/6/3", "a/1/6/4", "a/1/6/5", "a/1/6/6", "a/1/6/7", "a/1/6/8", "a/1/6/9",
    "a/1/7/0", "a/1/7/1", "a/1/7/2", "a/1/7/3", "a/1/7/4", "a/1/7/5", "a/1/7/6", "a/1/7/7", "a/1/7/8", "a/1/7/9",
    "a/1/8/0", "a/1/8/1", "a/1/8/2", "a/1/8/3", "a/1/8/4", "a/1/8/5", "a/1/8/6", "a/1/8/7", "a/1/8/8", "a/1/8/9",
    "a/1/9/0", "a/1/9/1", "a/1/9/2", "a/1/9/3", "a/1/9/4", "a/1/9/5", "a/1/9/6", "a/1/9/7", "a/1/9/8", "a/1/9/9",
    "a/2/0/0", "a/2/0/1", "a/2/0/2", "a/2/0/3", "a/2/0/4", "a/2/0/5", "a/2/0/6", "a/2/0/7", "a/2/0/8", "a/2/0/9",
    "a/2/1/0", "a/2/1/1", "a/2/1/2", "a/2/1/3", "a/2/1/4", "a/2/1/5", "a/2/1/6", "a/2/1/7", "a/2/1/8", "a/2/1/9",
    "a/2/2/0", "a/2/2/1", "a/2/2/2", "a/2/2/3", "a/2/2/4", "a/2/2/5", "a/2/2/6", "a/2/2/7", "a/2/2/8", "a/2/2/9",
    "a/2/3/0", "a/2/3/1", "a/2/3/2", "a/2/3/3", "a/2/3/4", "a/2/3/5", "a/2/3/6", "a/2/3/7", "a/2/3/8", "a/2/3/9",
    "a/2/4/0", "a/2/4/1", "a/2/4/2", "a/2/4/3", "a/2/4/4", "a/2/4/5", "a/2/4/6", "a/2/4/7", "a/2/4/8", "a/2/4/9",
    "a/2/5/0", "a/2/5/1", "a/2/5/2", "a/2/5/3", "a/2/5/4", "a/2/5/5", "a/2/5/6", "a/2/5/7", "a/2/5/8", "a/2/5/9",
    "a/2/6/0", "a/2/6/1", "a/2/6/2", "a/2/6/3", "a/2/6/4", "a/2/6/5", "a/2/6/6", "a/2/6/7", "a/2/6/8", "a/2/6/9",
    "a/2/7/0", "a/2/7/1", "a/2/7/2", "a/2/7/3", "a/2/7/4", "a/2/7/5", "a/2/7/6", "a/2/7/7", "a/2/7/8", "a/2/7/9",
    "a/2/8/0", "a/2/8/1", "a/2/8/2", "a/2/8/3", "a/2/8/4", "a/2/8/5", "a/2/8/6", "a/2/8/7", "a/2/8/8", "a/2/8/9",
    "a/2/9/0", "a/2/9/1", "a/2/9/2", "a/2/9/3", "a/2/9/4", "a/2/9/5", "a/2/9/6", "a/2/9/7", "a/2/9/8", "a/2/9/9",
    "a/3/0/0", "a/3/0/1", "a/3/0/2", "a/3/0/3", "a/3/0/4", "a/3/0/5", "a/3/0/6", "a/3/0/7",
// clang-format on
};

// The root heaps 0 and 1, of a DS and of a DSi
static const HeapDef HEAP_SETUP_NTR[] = {
    { 0x6000, 0 },
    { 0x1a3e00, 0 },
};

static const HeapDef HEAP_SETUP_TWL[] = {
    { 0x6000, 0 },
    { 0x48ae00, 0 },
};

static u32 sVBlankCount;
static VBlankCallback sVBlankCallback;
static void *sVBlankCallbackData;
static int sOSHeap;
static void *sExtraHeapMemory;
static GFLUseWork *sWork;

void GFLAppInit(void) {
    if (!hw_isDSi()) {
        void *end = GetUserMemRegionEnd(OS_ARENA_MAIN);
        void *start = GetUserMemRegionStart(OS_ARENA_MAIN);

        start = mem_init_alloc_area(OS_ARENA_MAIN, start, end, 1);
        SetUserMemRegionStart(OS_ARENA_MAIN, start);
        sOSHeap = mem_bind_static_ram_alloc(OS_ARENA_MAIN);
        sExtraHeapMemory = malloc_device(OS_ARENA_MAIN, sOSHeap, EXTRA_HEAP_SIZE);
    }

    if (hw_isDSi()) {
        GFL_MemInit(HEAP_SETUP_TWL, 2, 193, 0);
    } else {
        GFL_MemInit(HEAP_SETUP_NTR, 2, 193, 0);
    }
    GFL_HeapDTCMInit(0x2000);
    GFL_HeapCreateChild(1, 7, 0x27000);
    if (!hw_isDSi()) {
        GFL_HeapCreateRoot(sExtraHeapMemory, EXTRA_HEAP_SIZE, 9);
    } else {
        GFL_HeapCreateChild(1, 9, EXTRA_HEAP_SIZE);
    }

    initTableArea(0);
    GFL_ArcSysInit(NARC_PATH_LIST, ARC_PATH_COUNT);
    GFL_HIDInit(0, 0);
    if (func_0207ac24() & CONSOLE_TYPE_DEBUG_MASK) {
        GFL_DebugSetVerboseAssertHandlers();
    } else {
        GFL_DebugSetAssertHandlers(NULL, NULL, finishFunc);
    }
    GFL_OvlManagerInit(0, 20, 4, 4);
    GCTX_ProcMgrInit(0);

    sWork = GFL_HeapAllocate(0, sizeof(GFLUseWork), FALSE, "gfl_use.c", 301);
    sWork->hblankTCBWork = GFL_HeapAllocate(0, GFL_TCBMgrCalcAllocSize(HBLANK_TCB_COUNT), FALSE, "gfl_use.c", 303);
    sWork->hblankTCBMgr = GFL_TCBMgrCreate(HBLANK_TCB_COUNT, sWork->hblankTCBWork);
    sWork->vblankTCBWork = GFL_HeapAllocate(0, GFL_TCBMgrCalcAllocSize(VBLANK_TCB_COUNT), FALSE, "gfl_use.c", 306);
    sWork->vblankTCBMgr = GFL_TCBMgrCreate(VBLANK_TCB_COUNT, sWork->vblankTCBWork);

    GFL_FadeCreate(0);
    mainBackupSystemInit(7, showSavegameMainError, showSavegameAltError);
    GFL_TextPrintInit(NULL);
    func_02044170();
    GFL_RandomInit();
    sVBlankCount = 0;
    GFL_VBlankResetCallback();
    GFL_SndSetVolumeControlCallbacks();
    GFL_FadeSetUpdateFreq(2);
}

void GFLAppUpdate(void) {
    GCTX_HIDUpdate();
    func_02011818();
    GCTX_ProcMgrUpdate();
    func_02044198();
}

void func_02005668(void) {
}

void GFL_HBlankTCBUpdate(void) {
    GFL_TCBMgrUpdate(sWork->hblankTCBMgr);
}

TCB *GFL_HBlankTCBAdd(TCBFunc func, void *data, u32 priority) {
    return GFL_TCBMgrAddTask(sWork->hblankTCBMgr, func, data, priority);
}

void GFL_VBlankExecCallback(void) {
    if (sVBlankCallback != NULL) {
        sVBlankCallback(sVBlankCallbackData);
    }
}

BOOL GFL_VBlankSetCallback(VBlankCallback callback, void *data) {
    if (sVBlankCallback != NULL) {
        return FALSE;
    }
    sVBlankCallback = callback;
    sVBlankCallbackData = data;
    return TRUE;
}

void GFL_VBlankResetCallback(void) {
    sVBlankCallback = NULL;
    sVBlankCallbackData = NULL;
}

void GFL_VBlankUpdate(void) {
    GFL_TCBMgrUpdate(sWork->vblankTCBMgr);
    GFL_BGSysUpdate();
    func_0204f864();
    GFL_FadeUpdate();
    sVBlankCount++;
}

TCB *GFL_VBlankTCBAdd(TCBFunc func, void *data, u32 priority) {
    return GFL_TCBMgrAddTask(sWork->vblankTCBMgr, func, data, priority);
}

TCBManager *GFL_VBlankGetTCBMgr(void) {
    return sWork->vblankTCBMgr;
}

void GFL_RandomInitLC(MATHRandContext32 *context) {
    buildSeed(context);
}

void GFL_RandomInit(void) {
    GFL_RandomInitLC(&sWork->rand);
    GFL_RandomUpdateMT(GFL_RandomLC(0));
}

u32 GFL_RandomLC(u32 max) {
    return MATH_Rand32(&sWork->rand, max);
}

u32 GFL_RandomLCAlt(u32 max) {
    MATHRandContext32 *context = &sWork->rand;

    context->x = context->mul * context->x + context->add;
    return (u32)(((context->x >> 32) * max) >> 32);
}

const char *GFL_ArcSysGetResourcePath(u32 arcId) {
    return NARC_PATH_LIST[arcId];
}

// Waits for L, R, X and Y to be held together, after a failed assert's message
static void finishFunc(void) {
    while (TRUE) {
        u32 keys;

        GCTX_HIDUpdate();
        keys = GCTX_HIDGetHeldKeys();
        if ((keys & PAD_BUTTON_L) && (keys & PAD_BUTTON_R) && (keys & PAD_BUTTON_X) && (keys & PAD_BUTTON_Y)) {
            break;
        }
    }
}

void GFL_SndMute(void *work) {
    NNS_SndSetMasterVolume(0);
}

void GFL_SndSetMaxVol(void *work) {
    NNS_SndSetMasterVolume(127);
}

void GFL_SndSetVolumeControlCallbacks(void) {
    GCTX_HIDSetLidCallbacks(GFL_SndMute, GFL_SndSetMaxVol, NULL);
}

BOOL GFL_SndIsVolumeControlCallbackSet(void) {
    if (GCTX_HIDGetLidCloseCallback() == GFL_SndMute) {
        return TRUE;
    }
    return FALSE;
}
