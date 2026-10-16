#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/gfd.h"

// NitroSystem's gfd_VramTransferManager.c: a queue of transfers to VRAM, registered at any time and done in the next
// V-blank. The function and static names are the SDK's, from pokediamond's disassembly, except the queue helpers and
// GfdClearVramTransferQueue, named here. swan calls NNS_GfdInitVramTransferManager gfxUploadQueueBind,
// GfdClearVramTransferQueue gfxUploadQueueReset, NNS_GfdDoVramTransfer gfxExecUploadRequests,
// NNS_GfdRegisterNewVramTransferTask gfxUploadAsync,
// the queue functions Push, GetFront, GetEnd and Pop gfxNotifyUploadRequested, gfxUploadQueueGetCurrentRequest,
// gfxAllocUploadRequest and gfxUploadQueueAdvance, DoTransfer_ gfxUpload, the DoTransfer functions gfxUploadFunc_*,
// the queue s_VramTransferManager g_GfxUploadQueue, NextQueueIndex gfxUploadQueueGetNextIndex, IsQueueFull and
// IsQueueEmpty gfxUploadQueueIsFull and gfxUploadQueueIsEmpty, ResetTaskQueue_ gfxUploadQueueResetCore, and the
// transfer table transFunc GFX_UPLOAD_FUNCS

typedef void (*GfdTransferFunc)(const void *pSrc, u32 dstAddr, u32 szByte);

static GfdVramTransferQueue s_VramTransferManager;

static u16 NextQueueIndex(const GfdVramTransferQueue *queue, u16 index);
static BOOL IsQueueFull(const GfdVramTransferQueue *queue);
static BOOL IsQueueEmpty(const GfdVramTransferQueue *queue);
static void DoTransfer3dTex(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer3dTexPltt(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer3dClearImageColor(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer3dClearImageDepth(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG0CharMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG1CharMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG2CharMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG3CharMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG0ScrMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG1ScrMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG2ScrMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG3ScrMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG2BmpMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG3BmpMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjPlttMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBGPlttMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjExtPlttMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBGExtPlttMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjOamMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjCharMain(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG0CharSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG1CharSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG2CharSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG3CharSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG0ScrSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG1ScrSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG2ScrSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG3ScrSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG2BmpSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBG3BmpSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjPlttSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBGPlttSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjExtPlttSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dBGExtPlttSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjOamSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer2dObjCharSub(const void *pSrc, u32 dstAddr, u32 szByte);
static void DoTransfer_(const NNSGfdVramTransferTask *task);
static void ResetTaskQueue_(GfdVramTransferQueue *queue);

static u16 NextQueueIndex(const GfdVramTransferQueue *queue, u16 index) {
    return (u16)((index + 1) % queue->capacity);
}

static BOOL IsQueueFull(const GfdVramTransferQueue *queue) {
    return queue->count == queue->capacity ? TRUE : FALSE;
}

static BOOL IsQueueEmpty(const GfdVramTransferQueue *queue) {
    return queue->count == 0 ? TRUE : FALSE;
}

static void DoTransfer3dTex(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_BeginLoadTex();
    GX_LoadTex(pSrc, dstAddr, szByte);
    GX_EndLoadTex();
}

static void DoTransfer3dTexPltt(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_BeginLoadTexPltt();
    GX_LoadTexPltt(pSrc, dstAddr, szByte);
    GX_EndLoadTexPltt();
}

static void DoTransfer3dClearImageColor(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_BeginLoadClearImage();
    GX_LoadClearImageColor(pSrc, szByte);
    GX_EndLoadClearImage();
}

static void DoTransfer3dClearImageDepth(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_BeginLoadClearImage();
    GX_LoadClearImageDepth(pSrc, szByte);
    GX_EndLoadClearImage();
}

static void DoTransfer2dBG0CharMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG0Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG1CharMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG1Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG2CharMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG2Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG3CharMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG3Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG0ScrMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG0Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG1ScrMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG1Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG2ScrMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG2Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG3ScrMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG3Scr(pSrc, dstAddr, szByte);
}

// A bitmap BG's image is where its screen would be
static void DoTransfer2dBG2BmpMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG2Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG3BmpMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBG3Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dObjPlttMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadOBJPltt(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBGPlttMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadBGPltt(pSrc, dstAddr, szByte);
}

static void DoTransfer2dObjExtPlttMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_BeginLoadOBJExtPltt();
    GX_LoadOBJExtPltt(pSrc, dstAddr, szByte);
    GX_EndLoadOBJExtPltt();
}

static void DoTransfer2dBGExtPlttMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_BeginLoadBGExtPltt();
    GX_LoadBGExtPltt(pSrc, dstAddr, szByte);
    GX_EndLoadBGExtPltt();
}

static void DoTransfer2dObjOamMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadOAM(pSrc, dstAddr, szByte);
}

static void DoTransfer2dObjCharMain(const void *pSrc, u32 dstAddr, u32 szByte) {
    GX_LoadOBJ(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG0CharSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG0Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG1CharSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG1Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG2CharSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG2Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG3CharSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG3Char(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG0ScrSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG0Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG1ScrSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG1Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG2ScrSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG2Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG3ScrSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG3Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG2BmpSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG2Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBG3BmpSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBG3Scr(pSrc, dstAddr, szByte);
}

static void DoTransfer2dObjPlttSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadOBJPltt(pSrc, dstAddr, szByte);
}

static void DoTransfer2dBGPlttSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadBGPltt(pSrc, dstAddr, szByte);
}

static void DoTransfer2dObjExtPlttSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_BeginLoadOBJExtPltt();
    GXS_LoadOBJExtPltt(pSrc, dstAddr, szByte);
    GXS_EndLoadOBJExtPltt();
}

static void DoTransfer2dBGExtPlttSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_BeginLoadBGExtPltt();
    GXS_LoadBGExtPltt(pSrc, dstAddr, szByte);
    GXS_EndLoadBGExtPltt();
}

static void DoTransfer2dObjOamSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadOAM(pSrc, dstAddr, szByte);
}

static void DoTransfer2dObjCharSub(const void *pSrc, u32 dstAddr, u32 szByte) {
    GXS_LoadOBJ(pSrc, dstAddr, szByte);
}

static void DoTransfer_(const NNSGfdVramTransferTask *task) {
    // Indexed by NNS_GFD_DST_TYPE
    static const GfdTransferFunc transFunc[NNS_GFD_DST_MAX] = {
        DoTransfer3dTex,
        DoTransfer3dTexPltt,
        DoTransfer3dClearImageColor,
        DoTransfer3dClearImageDepth,
        DoTransfer2dBG0CharMain,
        DoTransfer2dBG1CharMain,
        DoTransfer2dBG2CharMain,
        DoTransfer2dBG3CharMain,
        DoTransfer2dBG0ScrMain,
        DoTransfer2dBG1ScrMain,
        DoTransfer2dBG2ScrMain,
        DoTransfer2dBG3ScrMain,
        DoTransfer2dBG2BmpMain,
        DoTransfer2dBG3BmpMain,
        DoTransfer2dObjPlttMain,
        DoTransfer2dBGPlttMain,
        DoTransfer2dObjExtPlttMain,
        DoTransfer2dBGExtPlttMain,
        DoTransfer2dObjOamMain,
        DoTransfer2dObjCharMain,
        DoTransfer2dBG0CharSub,
        DoTransfer2dBG1CharSub,
        DoTransfer2dBG2CharSub,
        DoTransfer2dBG3CharSub,
        DoTransfer2dBG0ScrSub,
        DoTransfer2dBG1ScrSub,
        DoTransfer2dBG2ScrSub,
        DoTransfer2dBG3ScrSub,
        DoTransfer2dBG2BmpSub,
        DoTransfer2dBG3BmpSub,
        DoTransfer2dObjPlttSub,
        DoTransfer2dBGPlttSub,
        DoTransfer2dObjExtPlttSub,
        DoTransfer2dBGExtPlttSub,
        DoTransfer2dObjOamSub,
        DoTransfer2dObjCharSub,
    };
    GfdTransferFunc fn = transFunc[task->type];

    DC_FlushRange(task->pSrc, task->szByte);
    fn(task->pSrc, task->dstAddr, task->szByte);
}

static void ResetTaskQueue_(GfdVramTransferQueue *queue) {
    queue->head = queue->tail = 0;
    queue->count = 0;
    queue->pendingBytes = 0;
}

BOOL NNSi_GfdPushVramTransferTaskQueue(GfdVramTransferQueue *queue) {
    if (!IsQueueFull(queue)) {
        queue->tail = NextQueueIndex(queue, queue->tail);
        queue->count++;
        return TRUE;
    }
    return FALSE;
}

NNSGfdVramTransferTask *NNSi_GfdGetFrontVramTransferTaskQueue(GfdVramTransferQueue *queue) {
    return &queue->tasks[queue->head];
}

NNSGfdVramTransferTask *NNSi_GfdGetEndVramTransferTaskQueue(GfdVramTransferQueue *queue) {
    return &queue->tasks[queue->tail];
}

BOOL NNSi_GfdPopVramTransferTaskQueue(GfdVramTransferQueue *queue) {
    if (!IsQueueEmpty(queue)) {
        queue->head = NextQueueIndex(queue, queue->head);
        queue->count--;
        return TRUE;
    }
    return FALSE;
}

void NNS_GfdInitVramTransferManager(NNSGfdVramTransferTask *pTaskArray, u32 lengthOfArray) {
    s_VramTransferManager.tasks = pTaskArray;
    s_VramTransferManager.capacity = lengthOfArray;
    ResetTaskQueue_(&s_VramTransferManager);
}

void GfdClearVramTransferQueue(void) {
    ResetTaskQueue_(&s_VramTransferManager);
}

void NNS_GfdDoVramTransfer(void) {
    GfdVramTransferQueue *queue = &s_VramTransferManager;
    NNSGfdVramTransferTask *task = NNSi_GfdGetFrontVramTransferTaskQueue(queue);

    while (NNSi_GfdPopVramTransferTaskQueue(queue)) {
        DoTransfer_(task);
        queue->pendingBytes -= task->szByte;
        task = NNSi_GfdGetFrontVramTransferTaskQueue(queue);
    }
}

BOOL NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_TYPE type, u32 dstAddr, const void *pSrc, u32 szByte) {
    GfdVramTransferQueue *queue = &s_VramTransferManager;

    if (!IsQueueFull(queue)) {
        NNSGfdVramTransferTask *task = NNSi_GfdGetEndVramTransferTaskQueue(queue);

        task->type = type;
        task->pSrc = pSrc;
        task->dstAddr = dstAddr;
        task->szByte = szByte;
        NNSi_GfdPushVramTransferTaskQueue(queue);
        queue->pendingBytes += task->szByte;
        return TRUE;
    }
    return FALSE;
}
