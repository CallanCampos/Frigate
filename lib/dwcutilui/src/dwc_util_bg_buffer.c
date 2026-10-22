#include "dwc_util_bg_buffer.h"
#include "dwc_util_heap.h"
#include "dwc_util_task.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"

static void DWCUtilBgBuffer_UploadTask(DWCUtilTask *task, void *arg);
static void DWCUtilBgBuffer_LoadPaletteRowTask(DWCUtilTask *task, void *arg);
static void DWCUtilBgBuffer_LoadPaletteTask(DWCUtilTask *task, void *arg);

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

void DWCi_SCREENlInit(const void *screen) {
    sBgBuffer.buffer = DWCi_HEAPlAllocEx(sizeof(DWCUtilBgBuffer), 4);
    MIi_CpuCopyFast(screen, sBgBuffer.buffer->screen, sizeof(sBgBuffer.buffer->screen));
    sBgBuffer.buffer->task = DWCi_TSKlForm(1, DWCUtilBgBuffer_UploadTask, NULL, 120);
}

void DWCi_SCREENlEnd(void) {
    DWCi_TSKlDeleteEx(1, sBgBuffer.buffer->task);
    DWCi_HEAPlFree_(&sBgBuffer.buffer);
}

void DWCi_SCREENlRenew(void) {
    sBgBuffer.buffer->dirty = TRUE;
}

void DWCi_SCREENlCopy(const u16 *src, int offset, int width, int rows) {
    int i;
    u16 *dest = sBgBuffer.buffer->screen + offset;

    for (i = 0; i < rows; i++) {
        MI_CpuCopy16(src, dest, width * 2);
        src += 32;
        dest += 32;
    }
}

static void DWCUtilBgBuffer_UploadTask(DWCUtilTask *task, void *arg) {
    if (sBgBuffer.buffer->dirty) {
        DC_FlushRange(sBgBuffer.buffer->screen, sizeof(sBgBuffer.buffer->screen));
        GX_LoadBG2Scr(sBgBuffer.buffer->screen, 0, sizeof(sBgBuffer.buffer->screen));
        sBgBuffer.buffer->dirty = FALSE;
    }
}

void DWCi_SCREENlCopyPalette(const void *palette, int src, int dest) {
    sBgBuffer.paletteSrc = (const u8 *)palette + src * 32;
    sBgBuffer.paletteDest = (void *)(HW_BG_PLTT + dest * 32);
    DWCi_TSKlForm(1, DWCUtilBgBuffer_LoadPaletteRowTask, NULL, 120);
}

static void DWCUtilBgBuffer_LoadPaletteRowTask(DWCUtilTask *task, void *arg) {
    MI_CpuCopy16(sBgBuffer.paletteSrc, sBgBuffer.paletteDest, 32);
    DWCi_TSKlDelete(1, task);
}

void DWCUtilBgBuffer_LoadPalette(const void *palette) {
    sBgBuffer.paletteSrc = palette;
    DWCi_TSKlForm(1, DWCUtilBgBuffer_LoadPaletteTask, NULL, 120);
}

static void DWCUtilBgBuffer_LoadPaletteTask(DWCUtilTask *task, void *arg) {
    MI_CpuCopy16(sBgBuffer.paletteSrc, (void *)HW_BG_PLTT, 0x200);
    DWCi_TSKlDelete(1, task);
}
