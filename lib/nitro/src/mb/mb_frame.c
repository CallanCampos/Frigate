// The frames that the multiboot parent and its children send each other, and the putting together of the requests
// that a child sends in pieces (libmb). The names are descriptive
#include "mb_private.h"

// How many bytes of a request the children's slots hold
#define MB_FRAME_SLOT_SIZE 0x1e

// The pieces of the requests so far, the size of a piece and how many a request has
static struct {
    u32 unk_00;
    MBFrameBuffer *buf;
    int payloadSize;
    int fragmentCount;
    int slotSize;
    u32 unk_14;
} sMBFrame;

static BOOL MBFrame_IsComplete(u16 aid);
static const void *MBFrame_Store(const MBChildFrame *frame, u16 aid);

// Sets the size of the frames of the children, which tells the size of a piece of a request
void MBFrame_SetPayloadSize(int size) {
    sMBFrame.payloadSize = size - 2;
    sMBFrame.fragmentCount = MB_FRAME_SLOT_SIZE / sMBFrame.payloadSize;
    sMBFrame.slotSize = MB_FRAME_SLOT_SIZE;
}

void MBFrame_SetBuffer(MBFrameBuffer *buf) {
    sMBFrame.buf = buf;
    sys_memset(buf, 0, sizeof(MBFrameBuffer));
}

// Forgets the pieces of a child's request
void MBFrame_ClearChild(u16 aid) {
    if (sMBFrame.buf != NULL) {
        int index = aid - 1;

        sys_memset(sMBFrame.buf->slot[index], 0, MB_FRAME_SLOT_SIZE);
        sMBFrame.buf->received[index] = 0;
    }
}

// Writes a frame of the parent to a buffer, and gives where the data that goes with it starts, or NULL for a type
// that is not known
u8 *MBFrame_Encode(const MBFrame *frame, u8 *dst) {
    u8 *p = dst;

    *p++ = frame->type;
    switch (frame->type) {
    case MB_FRAME_CONNECTED:
    case MB_FRAME_KICK:
    case MB_FRAME_FILE_INFO:
    case MB_FRAME_BOOT:
    case MB_FRAME_MEMBER_FULL:
        break;
    case MB_FRAME_BLOCK:
        *p++ = frame->value & 0xff;
        *p++ = (frame->value & 0xff00) >> 8;
        *p++ = frame->value2 & 0xff;
        *p++ = (frame->value2 & 0xff00) >> 8;
        break;
    default:
        return NULL;
    }
    return p;
}

// Reads a frame of a child. A piece of a request is stored; the request is given once it is whole
const void *MBFrame_Decode(const u8 *src, MBChildFrame *out, u16 aid) {
    const u8 *next;

    out->type = src[0];
    switch (out->type) {
    case MB_FRAME_REQUEST:
        if (MBFrame_IsComplete(aid)) {
            return sMBFrame.buf->slot[aid - 1];
        }
        out->fragment = src[1];
        if (out->fragment > sMBFrame.fragmentCount) {
            return NULL;
        }
        sys_memcpy(src + 2, out->pieceData, sMBFrame.payloadSize);
        next = MBFrame_Store(out, aid);
        break;
    case MB_FRAME_DOWNLOAD_REQUEST:
        next = src + 3;
        out->value = src[1];
        out->value |= (src[2] << 8) & 0xff00;
        break;
    case MB_FRAME_BLOCK_REQUEST:
        next = src + 3;
        out->value = src[1];
        out->value |= (src[2] << 8) & 0xff00;
        sys_memcpy(next, out->data, sMBFrame.payloadSize);
        next += sMBFrame.payloadSize;
        break;
    default:
        return NULL;
    }
    return next;
}

// Puts a piece of a request in its place in the child's slot
static const void *MBFrame_Store(const MBChildFrame *frame, u16 aid) {
    u8 fragment;

    if (sMBFrame.buf == NULL) {
        return NULL;
    }
    fragment = frame->fragment;
    if (fragment > sMBFrame.fragmentCount) {
        return NULL;
    }
    sys_memcpy(frame->pieceData, &sMBFrame.buf->slot[aid - 1][fragment * sMBFrame.payloadSize], sMBFrame.payloadSize);
    sMBFrame.buf->received[aid - 1] |= 1 << fragment;
    if (MBFrame_IsComplete(aid)) {
        return sMBFrame.buf->slot[aid - 1];
    }
    return NULL;
}

static BOOL MBFrame_IsComplete(u16 aid) {
    u16 i;

    for (i = 0; i < sMBFrame.fragmentCount; i++) {
        if ((sMBFrame.buf->received[aid - 1] & (1 << i)) == 0) {
            return FALSE;
        }
    }
    return TRUE;
}
