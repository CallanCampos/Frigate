#include "types.h"
#include "gfl/heap.h"
#include "gfl/heapsys.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nnsys/fnd.h"

// The heap API over heapsys.c, which stops the game when the heap system fails. The debug messages the failures printed
// are compiled out, which leaves the calls that computed their arguments

// What GFL_HeapDumpProc reports for the block it visits
typedef struct {
    u16 line;
    void *ptr;
    u32 size;
} HeapDumpState;

static void GFL_HeapHandleAllocResult(u16 result, HeapID parentHeapId, HeapID heapId);
static void GFL_HeapSetBlockDebugInfo(void *ptr, const char *file, u16 line);
static BOOL GFL_HeapDebugCopyFileName(char *dest, const char *file);
static void GFL_HeapDumpAllocFailure(HeapID heapId, u32 size, const char *file, u16 line);
static void GFL_HeapDebugNotifyTrackedAlloc(void *ptr, u32 size);
static void GFL_HeapDebugNotifyTrackedFree(void *ptr);
static void GFL_HeapDebugTraceAlloc(HeapID heapId, void *ptr, u32 size);
static void GFL_HeapDebugTraceFree(void *ptr);
static void GFL_HeapDeleteCleanCheck(HeapID heapId);
static void GFL_HeapDumpProc(void *block, NNSFndHeapHandle heap, u32 param);

// The heap whose allocations and frees the debug build reported, or -1 for none
static int sTrackHeapId = -1;

static HeapDumpState sHeapDumpState;
static char sHeapDumpFile[0x14];

void GFL_MemInit(const HeapDef *defs, u32 rootCount, u32 maxHeapIds, u32 reserveSize) {
    BOOL ok = GFL_HeapMgrInit(defs, rootCount, maxHeapIds, reserveSize);

    OS_GetArenaLo(OS_ARENA_MAIN);
    OS_GetArenaHi(OS_ARENA_MAIN);
    if (!ok) {
        GFL_HeapGetLastResult();
        sys_exit();
    }
}

void GFL_HeapCreateChild(HeapID parentHeapId, HeapID heapId, u32 size) {
    if (!GFL_HeapAddChild(parentHeapId, heapId, size)) {
        GFL_HeapHandleAllocResult(GFL_HeapGetLastResult(), parentHeapId, heapId);
    }
}

void GFL_HeapCreateRoot(void *memory, u32 size, HeapID heapId) {
    if (!GFL_HeapAddRoot(memory, size, heapId)) {
        GFL_HeapHandleAllocResult(GFL_HeapGetLastResult(), 0, heapId);
    }
}

static void GFL_HeapHandleAllocResult(u16 result, HeapID parentHeapId, HeapID heapId) {
    switch (result) {
    case HEAP_RESULT_1:
        sys_exit();
        break;
    case HEAP_RESULT_2:
        sys_exit();
        break;
    case HEAP_RESULT_3:
        sys_exit();
        break;
    case HEAP_RESULT_4:
        sys_exit();
        break;
    case HEAP_RESULT_5:
        sys_exit();
        break;
    }
}

void GFL_HeapDelete(HeapID heapId) {
    GFL_HeapDeleteCleanCheck(heapId);
    if (!GFL_HeapDeleteCore(heapId)) {
        switch (GFL_HeapGetLastResult()) {
        case HEAP_RESULT_1:
            sys_exit();
            break;
        case HEAP_RESULT_2:
            sys_exit();
            break;
        }
    }
}

void *GFL_HeapAllocate(HeapID heapId, u32 size, BOOL clear, const char *file, u16 line) {
    void *ptr = GFL_HeapAllocateCore(heapId, size);

    if (ptr == NULL) {
        GFL_HeapGetLastResult();
        GFL_HeapDumpAllocFailure(heapId, size, file, line);
        sys_exit();
    } else {
        GFL_HeapSetBlockDebugInfo(ptr, file, line);
        GFL_HeapDebugTraceAlloc(heapId, ptr, size);
        if (clear) {
            MIi_CpuClear32(0, ptr, size);
        }
    }
    return ptr;
}

void GFL_HeapFree(void *ptr) {
    GFL_HeapDebugTraceFree(ptr);
    GFL_HeapFreeCore(ptr);
}

void GFL_HeapCreateAllocator(NNSFndAllocator *allocator, HeapID heapId, int alignment) {
    if (!GFL_HeapCreateAllocatorCore(allocator, heapId, alignment)) {
        sys_exit();
    }
}

void GFL_HeapResize(void *ptr, u32 size) {
    if (!GFL_HeapResizeCore(ptr, size)) {
        switch (GFL_HeapGetLastResult()) {
        case HEAP_RESULT_1:
            sys_exit();
            break;
        case HEAP_RESULT_2:
            sys_exit();
            break;
        case HEAP_RESULT_3:
            sys_exit();
            break;
        case HEAP_RESULT_4:
            break;
        case HEAP_RESULT_5:
            break;
        }
    }
}

u32 GFL_HeapGetFreeSize(HeapID heapId) {
    u32 size = GFL_HeapGetFreeSizeCore(heapId);

    if (size == 0) {
        sys_exit();
    }
    return size;
}

BOOL GFL_HeapStatusValidate(HeapID heapId) {
    BOOL ok = GFL_HeapStatusCheck(heapId);

    if (!ok) {
        switch (GFL_HeapGetLastResult()) {
        case HEAP_RESULT_1:
            break;
        case HEAP_RESULT_2:
            break;
        case HEAP_RESULT_3:
            sys_exit();
            break;
        }
    }
    return ok;
}

static void GFL_HeapSetBlockDebugInfo(void *ptr, const char *file, u16 line) {
    HeapDebugInfo *info = GFL_HeapGetBlockDebugInfoPtr(ptr);
    int i;

    for (i = 0; i < (int)sizeof(info->file); i++) {
        info->file[i] = file[i];
        if (file[i] == '\0') {
            break;
        }
    }
    info->line = line;
}

// Copies a block's file name, returning whether it looks like one: printable, and with an extension unless it fills
// the name
static BOOL GFL_HeapDebugCopyFileName(char *dest, const char *file) {
    int len;
    int i;

    for (len = 0; len < (int)sizeof(((HeapDebugInfo *)NULL)->file); len++) {
        if (file[len] == '\0') {
            break;
        }
        dest[len] = file[len];
    }
    dest[len] = '\0';
    if (len == 0) {
        return FALSE;
    }
    for (i = 0; i < len; i++) {
        if ((u8)dest[i] < ' ' || (u8)dest[i] > '~') {
            return FALSE;
        }
    }
    if (len < (int)sizeof(((HeapDebugInfo *)NULL)->file)) {
        for (i = 0; i < len; i++) {
            if ((u8)dest[i] == '.') {
                break;
            }
        }
        if (i == len) {
            return FALSE;
        }
    }
    return TRUE;
}

static void GFL_HeapDumpAllocFailure(HeapID heapId, u32 size, const char *file, u16 line) {
    NNSFndHeapHandle heap = GFL_HeapGetValidHeapBase(heapId);

    NNS_FndGetTotalFreeSizeForExpHeap(heap);
    HeapBase_GetHighestAllocatableSize(heap, 4);
    GFL_HeapDumpOnFailure(heapId);
}

static void GFL_HeapDebugNotifyTrackedAlloc(void *ptr, u32 size) {
    char file[0x14];
    HeapDebugInfo *info = GFL_HeapGetBlockDebugInfoPtr(ptr);

    GFL_HeapGetAllocationCount(GFL_HeapGetBlockHeapID(ptr));
    GFL_HeapDebugCopyFileName(file, info->file);
}

static void GFL_HeapDebugNotifyTrackedFree(void *ptr) {
    char file[0x14];
    HeapDebugInfo *info = GFL_HeapGetBlockDebugInfoPtr(ptr);
    HeapID heapId = GFL_HeapGetBlockHeapID(ptr);

    GFL_HeapGetAllocationCount(heapId);
    GFL_HeapDebugCopyFileName(file, info->file);
    NNS_FndGetSizeForMBlockExpHeap((HeapBlockHeader *)ptr - 1);
    GFL_HeapGetFreeSizeCore(heapId);
}

static void GFL_HeapDebugTraceAlloc(HeapID heapId, void *ptr, u32 size) {
    if (sTrackHeapId >= 0 && heapId == sTrackHeapId) {
        GFL_HeapDebugNotifyTrackedAlloc(ptr, size);
    }
}

static void GFL_HeapDebugTraceFree(void *ptr) {
    HeapID heapId = GFL_HeapGetBlockHeapID(ptr);

    if (sTrackHeapId >= 0 && heapId == sTrackHeapId) {
        GFL_HeapDebugNotifyTrackedFree(ptr);
    }
}

// Reports a heap deleted with blocks still allocated
static void GFL_HeapDeleteCleanCheck(HeapID heapId) {
    if (GFL_HeapGetAllocationCount(heapId) != 0) {
        GFL_HeapDumpOnFailure(heapId);
    }
}

static void GFL_HeapDumpProc(void *block, NNSFndHeapHandle heap, u32 param) {
    char file[0x14];
    void *ptr = (HeapBlockHeader *)block + 1;
    HeapDebugInfo *info = GFL_HeapGetBlockDebugInfoPtr(ptr);
    u16 line;
    int pad;
    int i;

    sHeapDumpState.size = NNS_FndGetSizeForMBlockExpHeap(block);
    sHeapDumpState.ptr = ptr;
    if (!GFL_HeapDebugCopyFileName(file, info->file)) {
        MI_CpuCopy8("SYSTEM ALLOC", file, 13);
        line = 0;
    } else {
        line = info->line;
    }
    MI_CpuCopy8(file, sHeapDumpFile, GFL_STD_StrLen(file) + 1);
    sHeapDumpState.line = line;
    pad = 0x12 - GFL_STD_StrLen(file);
    for (i = 0; i < pad; i++) {
        file[i] = ' ';
    }
    file[i] = '\0';
}

// Prints a heap's sizes and its allocated blocks
void GFL_HeapDumpOnFailure(HeapID heapId) {
    GFL_HeapGetSize(heapId);
    GFL_HeapGetFreeSizeCore(heapId);
    GFL_HeapGetHighestAllocatableSize(heapId);
    GFL_HeapGetAllocationCount(heapId);
    exit(1);
    HeapBase_DumpMemory(GFL_HeapGetValidHeapBase(heapId), GFL_HeapDumpProc, 0);
    exit(0);
}

void GFL_HeapDTCMInit(u32 size) {
    if (!GFL_HeapDTCMInitCore(size)) {
        sys_exit();
    }
}

void *GFL_HeapDTCMAllocate(u32 size) {
    return GFL_HeapDTCMAllocateCore(size);
}

void _freeBlkFromDTCM(void *ptr) {
    freeBlkFromDTCM(ptr);
}
