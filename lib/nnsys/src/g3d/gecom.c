#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "twl/mi.h"

// NitroSystem's G3D geometry command buffer: every geometry command that G3D sends goes through NNS_G3dGeBufferOP_N,
// and a display list through NNS_G3dGeSendDL. A large display list goes to the FIFO by DMA when GX has a channel, and
// the commands that follow it wait in the buffer, if there is one, until the DMA is done. The file is named after
// NitroSystem's own gecom.c, and the names are NitroSystem's, as ntrtwl's reconstruction of NitroSystem has them,
// except those of the DSi's NDMA path (ToDmaNo, ToNDmaNo, SendDisplayList and SendDisplayListFast), which is newer than
// that reconstruction and named here. NNS_G3dGeWaitSendDL and simpleUnlock_ are static because only this file refers to
// them

static void NNS_G3dGeWaitSendDL(void);
static void simpleUnlock_(void *arg);

// With -ipa file, MWCC lays these out in the order they are declared but with the first one last: NNS_G3dGeBuffer,
// NNS_G3dFlagGXDmaAsync, then NNS_G3dFlagUseFastDma
u32 NNS_G3dFlagUseFastDma;
static NNSG3dGeBuffer *NNS_G3dGeBuffer;
// Whether a display list is on its way by DMA; the DMA's callback (simpleUnlock_) clears it from the interrupt
static volatile BOOL NNS_G3dFlagGXDmaAsync;

BOOL NNS_G3dGeIsBufferExist(void) {
    return NNS_G3dGeBuffer != NULL;
}

void NNS_G3dGeSetBuffer(NNSG3dGeBuffer *buf) {
    if (NNS_G3dGeBuffer == NULL) {
        buf->idx = 0;
        NNS_G3dGeBuffer = buf;
    }
}

NNSG3dGeBuffer *NNS_G3dGeReleaseBuffer(void) {
    NNSG3dGeBuffer *buf;

    NNS_G3dGeFlushBuffer();
    buf = NNS_G3dGeBuffer;
    NNS_G3dGeBuffer = NULL;
    return buf;
}

// Waits for the DMA to be done, and sends the commands that waited for it
void NNS_G3dGeFlushBuffer(void) {
    if (NNS_G3dFlagGXDmaAsync) {
        NNS_G3dGeWaitSendDL();
    }
    if (NNS_G3dGeBuffer != NULL && NNS_G3dGeBuffer->idx != 0) {
        MIi_CpuSend32(NNS_G3dGeBuffer->data, &reg_G3X_GXFIFO, NNS_G3dGeBuffer->idx * sizeof(u32));
        NNS_G3dGeBuffer->idx = 0;
    }
}

static void NNS_G3dGeWaitSendDL(void) {
    while (NNS_G3dFlagGXDmaAsync) {
    }
}

// The DMA's callback, run from its interrupt: arg is NNS_G3dFlagGXDmaAsync, which NNS_G3dGeWaitSendDL spins on
static void simpleUnlock_(void *arg) {
    *(volatile BOOL *)arg = FALSE;
}

// GX's DMA channel as the SDK's DMA functions take it, GX_DMA_NOT_USE for the DSi's NDMA channels above 3, and as the
// NDMA functions take it, the channel minus 4, or GX_DMA_NOT_USE for the SDK's own
static inline u32 ToDmaNo(u32 id) {
    return id > MI_DMA_MAX_NUM ? GX_DMA_NOT_USE : id;
}

static inline u32 ToNDmaNo(u32 id) {
    return id > MI_DMA_MAX_NUM ? id - 4 : GX_DMA_NOT_USE;
}

// Send the display list by DMA on GX's channel: the SDK's DMA on a DS, and on a DSi the NDMA for the channels above 3.
// The fast one is the SDK's variant that doesn't wait for the FIFO to be half empty
static inline void SendDisplayListFast(const u32 *dl, u32 size) {
    if (OS_IsRunOnTwl()) {
        u32 id = GXi_DmaId;
        u32 dmaNo = ToDmaNo(id);

        if (dmaNo == GX_DMA_NOT_USE) {
            MI_SendNDmaGXCommandAsyncFast(ToNDmaNo(id), dl, size, simpleUnlock_, (void *)&NNS_G3dFlagGXDmaAsync);
        } else {
            MI_SendGXCommandAsyncFast(dmaNo, dl, size, simpleUnlock_, (void *)&NNS_G3dFlagGXDmaAsync);
        }
    } else {
        MI_SendGXCommandAsyncFast(ToDmaNo(GXi_DmaId), dl, size, simpleUnlock_, (void *)&NNS_G3dFlagGXDmaAsync);
    }
}

static inline void SendDisplayList(const u32 *dl, u32 size) {
    if (OS_IsRunOnTwl()) {
        u32 id = GXi_DmaId;
        u32 dmaNo = ToDmaNo(id);

        if (dmaNo == GX_DMA_NOT_USE) {
            MI_SendNDmaGXCommandAsync(ToNDmaNo(id), dl, size, simpleUnlock_, (void *)&NNS_G3dFlagGXDmaAsync);
        } else {
            MI_SendGXCommandAsync(dmaNo, dl, size, simpleUnlock_, (void *)&NNS_G3dFlagGXDmaAsync);
        }
    } else {
        MI_SendGXCommandAsync(ToDmaNo(GXi_DmaId), dl, size, simpleUnlock_, (void *)&NNS_G3dFlagGXDmaAsync);
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
    NNS_G3dFlagGXDmaAsync = TRUE;
    if (NNS_G3dFlagUseFastDma) {
        SendDisplayListFast(dl, size);
    } else {
        SendDisplayList(dl, size);
    }
}

// Sends a geometry command and its parameters: into the buffer if the DMA is going and they fit, or else to the FIFO
// after what is before them
void NNS_G3dGeBufferOP_N(u32 op, const u32 *args, u32 num) {
    if (NNS_G3dGeBuffer != NULL) {
        if (NNS_G3dFlagGXDmaAsync && NNS_G3dGeBuffer->idx + 1 + num <= NNS_G3D_SIZE_COMBUFFER) {
            NNS_G3dGeBuffer->data[NNS_G3dGeBuffer->idx++] = op;
            if (num != 0) {
                MIi_CpuCopyFast(args, &NNS_G3dGeBuffer->data[NNS_G3dGeBuffer->idx], num * sizeof(u32));
                NNS_G3dGeBuffer->idx += num;
            }
            return;
        }
        if (NNS_G3dGeBuffer->idx != 0) {
            NNS_G3dGeFlushBuffer();
        } else if (NNS_G3dFlagGXDmaAsync) {
            NNS_G3dGeWaitSendDL();
        }
    } else if (NNS_G3dFlagGXDmaAsync) {
        NNS_G3dGeWaitSendDL();
    }
    reg_G3X_GXFIFO = op;
    MIi_CpuSend32(args, &reg_G3X_GXFIFO, num * sizeof(u32));
}
