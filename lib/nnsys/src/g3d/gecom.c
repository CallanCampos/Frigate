#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "twl/mi.h"

// NitroSystem's G3D geometry command buffer: every geometry command that G3D sends goes through NNS_G3dGeBufferOP_N,
// and a display list through NNS_G3dGeSendDL. A large display list goes to the FIFO by DMA when GX has a channel, and
// the commands that follow it wait in the buffer, if there is one, until the DMA is done. The file is named after
// NitroSystem's own gecom.c. WaitDma and OnDmaDone are static because only this file calls them

#define GEBUFFER_ARRAY_SIZE 192

// The commands that wait for the DMA: the number of words, then the words as the FIFO takes them (each command, then
// its parameters)
typedef struct {
    u32 idx;
    u32 data[GEBUFFER_ARRAY_SIZE];
} GeBuffer;

static void WaitDma(void);

u32 NNS_G3dFlagUseFastDma;
static GeBuffer *sGeBuffer;
// Whether a display list is on its way by DMA; the DMA's callback (OnDmaDone) clears it from the interrupt
static volatile BOOL sDmaBusy;

BOOL NNS_G3dGeIsBufferExist(void) {
    return sGeBuffer != NULL;
}

void NNS_G3dGeSetBuffer(void *buffer) {
    GeBuffer *buf = buffer;

    if (sGeBuffer == NULL) {
        buf->idx = 0;
        sGeBuffer = buf;
    }
}

void *NNS_G3dGeReleaseBuffer(void) {
    GeBuffer *buf;

    NNS_G3dGeFlushBuffer();
    buf = sGeBuffer;
    sGeBuffer = NULL;
    return buf;
}

// Waits for the DMA to be done, and sends the commands that waited for it
void NNS_G3dGeFlushBuffer(void) {
    if (sDmaBusy) {
        WaitDma();
    }
    if (sGeBuffer != NULL && sGeBuffer->idx != 0) {
        MIi_CpuSend32(sGeBuffer->data, &reg_G3X_GXFIFO, sGeBuffer->idx * sizeof(u32));
        sGeBuffer->idx = 0;
    }
}

static void WaitDma(void) {
    while (sDmaBusy) {
    }
}

static void OnDmaDone(void *arg) {
    *(BOOL *)arg = FALSE;
}

// Send the display list by DMA on GX's channel: the SDK's DMA on a DS, and on a DSi also the NDMA for the channels
// above 3 (GX's channel minus 4), where the SDK's functions get GX_DMA_NOT_USE. The fast one is the SDK's variant that
// doesn't wait for the FIFO to be half empty
static inline void SendDisplayListFast(const u32 *dl, u32 size) {
    if (OS_IsRunOnTwl()) {
        u32 dmaNo = GXi_DmaId;
        u32 sdkDmaNo = dmaNo > MI_DMA_MAX_NUM ? GX_DMA_NOT_USE : dmaNo;

        if (sdkDmaNo == GX_DMA_NOT_USE) {
            u32 ndmaNo = GX_DMA_NOT_USE;

            if (dmaNo > MI_DMA_MAX_NUM) {
                ndmaNo = dmaNo - 4;
            }
            MI_SendNDmaGXCommandAsyncFast(ndmaNo, dl, size, OnDmaDone, (void *)&sDmaBusy);
        } else {
            MI_SendGXCommandAsyncFast(sdkDmaNo, dl, size, OnDmaDone, (void *)&sDmaBusy);
        }
    } else {
        u32 sdkDmaNo = GXi_DmaId > MI_DMA_MAX_NUM ? GX_DMA_NOT_USE : GXi_DmaId;

        MI_SendGXCommandAsyncFast(sdkDmaNo, dl, size, OnDmaDone, (void *)&sDmaBusy);
    }
}

static inline void SendDisplayList(const u32 *dl, u32 size) {
    if (OS_IsRunOnTwl()) {
        u32 dmaNo = GXi_DmaId;
        u32 sdkDmaNo = dmaNo > MI_DMA_MAX_NUM ? GX_DMA_NOT_USE : dmaNo;

        if (sdkDmaNo == GX_DMA_NOT_USE) {
            u32 ndmaNo = GX_DMA_NOT_USE;

            if (dmaNo > MI_DMA_MAX_NUM) {
                ndmaNo = dmaNo - 4;
            }
            MI_SendNDmaGXCommandAsync(ndmaNo, dl, size, OnDmaDone, (void *)&sDmaBusy);
        } else {
            MI_SendGXCommandAsync(sdkDmaNo, dl, size, OnDmaDone, (void *)&sDmaBusy);
        }
    } else {
        u32 sdkDmaNo = GXi_DmaId > MI_DMA_MAX_NUM ? GX_DMA_NOT_USE : GXi_DmaId;

        MI_SendGXCommandAsync(sdkDmaNo, dl, size, OnDmaDone, (void *)&sDmaBusy);
    }
}

// Sends a display list, which starts with a parameterless first command word, by DMA if it is large enough and GX has a
// channel for it, or else as commands. Fast DMA is chosen by NNS_G3dFlagUseFastDma
void NNS_G3dGeSendDL(const void *list, u32 size) {
    const u32 *dl = list;

    if (size < 256 || GXi_DmaId == GX_DMA_NOT_USE) {
        NNS_G3dGeBufferOP_N(dl[0], dl + 1, size / 4 - 1);
        return;
    }

    NNS_G3dGeFlushBuffer();
    sDmaBusy = TRUE;
    if (NNS_G3dFlagUseFastDma) {
        SendDisplayListFast(dl, size);
    } else {
        SendDisplayList(dl, size);
    }
}

// Sends a geometry command and its parameters: into the buffer if the DMA is going and they fit, or else to the FIFO
// after what is before them
void NNS_G3dGeBufferOP_N(u32 op, const u32 *args, u32 num) {
    if (sGeBuffer != NULL) {
        if (sDmaBusy && sGeBuffer->idx + 1 + num <= GEBUFFER_ARRAY_SIZE) {
            sGeBuffer->data[sGeBuffer->idx++] = op;
            if (num != 0) {
                MIi_CpuCopyFast(args, &sGeBuffer->data[sGeBuffer->idx], num * sizeof(u32));
                sGeBuffer->idx += num;
            }
            return;
        }
        if (sGeBuffer->idx != 0) {
            NNS_G3dGeFlushBuffer();
        } else if (sDmaBusy) {
            WaitDma();
        }
    } else if (sDmaBusy) {
        WaitDma();
    }
    reg_G3X_GXFIFO = op;
    MIi_CpuSend32(args, &reg_G3X_GXFIFO, num * sizeof(u32));
}
