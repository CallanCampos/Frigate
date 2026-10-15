#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "nitro/os.h"

// SOCL_GetStatus: what a socket can do now (data to read, room to write, an error, not connected). pokediamond's
// NitroWiFi names the object socl_poll.o and its functions.

static int SOCLi_GetReadBufferOccpiedSize(const SOCLSocket *socket);

int SOCL_GetStatus(SOCLSocket *socket) {
    int status = 0;
    u32 irq;

    if (SOCL_SocketIsInvalid(socket)) {
        status |= SOCL_STATUS_INVALID;
    } else {
        if (socket->state & SOCLI_STATE_CONNECT_FAILED) {
            status |= SOCL_STATUS_ERROR;
        }
        if (socket->flag == 1 || (socket->state & SOCLI_STATE_CONNECTED)) {
            irq = CPU_IRQDisable();
            if (SOCLi_GetReadBufferOccpiedSize(socket) > 0) {
                status |= SOCL_STATUS_READABLE;
            }
            if (SOCLi_GetWriteBufferFreeSize(socket) > 0) {
                status |= SOCL_STATUS_WRITABLE;
            }
            CPU_SetIRQMask(irq);
        }
        if (SOCLi_SocketIsTCP(socket)) {
            if ((socket->state & SOCLI_STATE_CONNECTED) && socket->cps_state != CPS_STT_ESTABLISHED &&
                !(status & SOCL_STATUS_READABLE)) {
                socket->state &= ~(SOCLI_STATE_BINDING | SOCLI_STATE_CONNECTED);
            }
            if (!(socket->state & SOCLI_STATE_BINDING) && !(socket->state & SOCLI_STATE_CONNECTED)) {
                status |= SOCL_STATUS_NOTCONN;
            }
        }
    }
    return status;
}

// How much a read could take now: the next datagram's size for UDP, the buffered data for TCP
static int SOCLi_GetReadBufferOccpiedSize(const SOCLSocket *socket) {
    SOCLRecvPipe *pipe = SOCLi_GetRecvPipe(socket);
    int size = 0;

    if (pipe != NULL) {
        s8 mode = socket->flag;

        if (mode == 1) {
            if (pipe->udp_head != NULL) {
                size = pipe->udp_head->len;
            }
        } else if (mode == 0 || mode == 4) {
            size = socket->rcvbuf_len - pipe->cps_read_len;
        }
    }
    return size;
}
