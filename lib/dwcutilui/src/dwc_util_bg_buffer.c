#include "dwc_util_bg_buffer.h"
#include "dwc_util_heap.h"
#include "dwc_util_task.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"

static void func_ov182_021b6fe0(DWCUtilTask *task, void *arg);
static void func_ov182_021b7040(DWCUtilTask *task, void *arg);
static void func_ov182_021b707c(DWCUtilTask *task, void *arg);

typedef struct DWCUtilBgBuffer {
    // The VBlank task that loads the screen
    DWCUtilTask *task;
    u16 screen[32 * 24];
    u8 dirty;
} DWCUtilBgBuffer;

static struct {
    DWCUtilBgBuffer *buffer;
    // What the palette's load copies, from where to where
    const void *paletteSrc;
    void *paletteDest;
} sBgBuffer;

void func_ov182_021b6f38(const void *screen) {
    sBgBuffer.buffer = func_ov182_021bc4d8(sizeof(DWCUtilBgBuffer), 4);
    MIi_CpuCopyFast(screen, sBgBuffer.buffer->screen, sizeof(sBgBuffer.buffer->screen));
    sBgBuffer.buffer->task = func_ov182_021bd200(1, func_ov182_021b6fe0, NULL, 120);
}

void func_ov182_021b6f78(void) {
    func_ov182_021bd280(1, sBgBuffer.buffer->task);
    func_ov182_021bc4f0(&sBgBuffer.buffer);
}

void func_ov182_021b6f98(void) {
    sBgBuffer.buffer->dirty = TRUE;
}

void func_ov182_021b6fac(const u16 *src, int offset, int width, int rows) {
    int i;
    u16 *dest = sBgBuffer.buffer->screen + offset;

    for (i = 0; i < rows; i++) {
        MI_CpuCopy16(src, dest, width * 2);
        src += 32;
        dest += 32;
    }
}

static void func_ov182_021b6fe0(DWCUtilTask *task, void *arg) {
    if (sBgBuffer.buffer->dirty) {
        DC_FlushRange(sBgBuffer.buffer->screen, sizeof(sBgBuffer.buffer->screen));
        GX_LoadBG2Scr(sBgBuffer.buffer->screen, 0, sizeof(sBgBuffer.buffer->screen));
        sBgBuffer.buffer->dirty = FALSE;
    }
}

void func_ov182_021b7014(const void *palette, int src, int dest) {
    sBgBuffer.paletteSrc = (const u8 *)palette + src * 32;
    sBgBuffer.paletteDest = (void *)(HW_BG_PLTT + dest * 32);
    func_ov182_021bd200(1, func_ov182_021b7040, NULL, 120);
}

static void func_ov182_021b7040(DWCUtilTask *task, void *arg) {
    MI_CpuCopy16(sBgBuffer.paletteSrc, sBgBuffer.paletteDest, 32);
    func_ov182_021bd268(1, task);
}

void func_ov182_021b7060(const void *palette) {
    sBgBuffer.paletteSrc = palette;
    func_ov182_021bd200(1, func_ov182_021b707c, NULL, 120);
}

static void func_ov182_021b707c(DWCUtilTask *task, void *arg) {
    MI_CpuCopy16(sBgBuffer.paletteSrc, (void *)HW_BG_PLTT, 0x200);
    func_ov182_021bd268(1, task);
}
