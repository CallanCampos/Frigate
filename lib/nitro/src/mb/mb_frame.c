// The frames that the multiboot parent and its children send each other, and the putting together of the requests
// that a child sends in pieces (libmb). The names are descriptive
#include "mb_private.h"

// How many bytes of a request the children's slots hold
#define MB_FRAME_SLOT_SIZE 0x1e

// The pieces of the requests so far, the size of a piece and how many a request has
static struct {
    u32 unk_00;
    MbRequestPieceBuf *buf;
    int size;
    int num;
    int bufSize;
    u32 unk_14;
} sMBFrame;

static BOOL IsGetAllRequestData(u16 aid);
static const void *MBi_ReceiveRequestDataPiece(const MBCommChildBlockHeader *frame, u16 aid);

// Sets the size of the frames of the children, which tells the size of a piece of a request
void MBi_SetChildMPMaxSize(int size) {
    sMBFrame.size = size - 2;
    sMBFrame.num = MB_FRAME_SLOT_SIZE / sMBFrame.size;
    sMBFrame.bufSize = MB_FRAME_SLOT_SIZE;
}

void MBi_SetParentPieceBuffer(MbRequestPieceBuf *buf) {
    sMBFrame.buf = buf;
    MI_CpuFill8(buf, 0, sizeof(MbRequestPieceBuf));
}

// Forgets the pieces of a child's request
void MBi_ClearParentPieceBuffer(u16 aid) {
    if (sMBFrame.buf != NULL) {
        int index = aid - 1;

        MI_CpuFill8(sMBFrame.buf->data_buf[index], 0, MB_FRAME_SLOT_SIZE);
        sMBFrame.buf->data_bmp[index] = 0;
    }
}

// Writes a frame of the parent to a buffer, and gives where the data that goes with it starts, or NULL for a type
// that is not known
u8 *MBi_MakeParentSendBuffer(const MBCommParentBlockHeader *frame, u8 *dst) {
    u8 *p = dst;

    *p++ = frame->type;
    switch (frame->type) {
    case MB_COMM_TYPE_PARENT_SENDSTART:
    case MB_COMM_TYPE_PARENT_KICKREQ:
    case MB_COMM_TYPE_PARENT_DL_FILEINFO:
    case MB_COMM_TYPE_PARENT_BOOTREQ:
    case MB_COMM_TYPE_PARENT_MEMBER_FULL:
        break;
    case MB_COMM_TYPE_PARENT_DATA:
        *p++ = frame->fid & 0xff;
        *p++ = (frame->fid & 0xff00) >> 8;
        *p++ = frame->seqno & 0xff;
        *p++ = (frame->seqno & 0xff00) >> 8;
        break;
    default:
        return NULL;
    }
    return p;
}

// Reads a frame of a child. A piece of a request is stored; the request is given once it is whole
const void *MBi_SetRecvBufferFromChild(const u8 *src, MBCommChildBlockHeader *out, u16 aid) {
    const u8 *next;

    out->type = src[0];
    switch (out->type) {
    case MB_COMM_TYPE_CHILD_FILEREQ:
        if (IsGetAllRequestData(aid)) {
            return sMBFrame.buf->data_buf[aid - 1];
        }
        out->fragment = src[1];
        if (out->fragment > sMBFrame.num) {
            return NULL;
        }
        MI_CpuCopy8(src + 2, out->pieceData, sMBFrame.size);
        next = MBi_ReceiveRequestDataPiece(out, aid);
        break;
    case MB_COMM_TYPE_CHILD_ACCEPT_FILEINFO:
        next = src + 3;
        out->value = src[1];
        out->value |= (src[2] << 8) & 0xff00;
        break;
    case MB_COMM_TYPE_CHILD_CONTINUE:
        next = src + 3;
        out->value = src[1];
        out->value |= (src[2] << 8) & 0xff00;
        MI_CpuCopy8(next, out->data, sMBFrame.size);
        next += sMBFrame.size;
        break;
    default:
        return NULL;
    }
    return next;
}

// Puts a piece of a request in its place in the child's slot
static const void *MBi_ReceiveRequestDataPiece(const MBCommChildBlockHeader *frame, u16 aid) {
    u8 fragment;

    if (sMBFrame.buf == NULL) {
        return NULL;
    }
    fragment = frame->fragment;
    if (fragment > sMBFrame.num) {
        return NULL;
    }
    MI_CpuCopy8(frame->pieceData, &sMBFrame.buf->data_buf[aid - 1][fragment * sMBFrame.size], sMBFrame.size);
    sMBFrame.buf->data_bmp[aid - 1] |= 1 << fragment;
    if (IsGetAllRequestData(aid)) {
        return sMBFrame.buf->data_buf[aid - 1];
    }
    return NULL;
}

static BOOL IsGetAllRequestData(u16 aid) {
    u16 i;

    for (i = 0; i < sMBFrame.num; i++) {
        if ((sMBFrame.buf->data_bmp[aid - 1] & (1 << i)) == 0) {
            return FALSE;
        }
    }
    return TRUE;
}
