#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL_GetStatus: what a socket can do now (data to read, room to write, an error, not connected). pokediamond's
// NitroWiFi names the object socl_poll.o and its functions.

static s32 SOCLi_GetReadBufferOccpiedSize(const SOCLSocket *socket);

int SOCL_GetStatus(int s) {
    SOCLSocket *socket = (SOCLSocket *)s;
    int status = 0;
    OSIntrMode irq;

    if (SOCL_SocketIsInvalid(socket)) {
        status |= SOCL_POLLNVAL;
    } else {
        if (socket->state & SOCL_STATUS_ERROR) {
            status |= SOCL_POLLERR;
        }
        if (socket->flag == 1 || (socket->state & SOCL_STATUS_CONNECTED)) {
            irq = OS_DisableInterrupts();
            if (SOCLi_GetReadBufferOccpiedSize(socket) > 0) {
                status |= SOCL_POLLRDNORM;
            }
            if (SOCLi_GetWriteBufferFreeSize(socket) > 0) {
                status |= SOCL_POLLWRNORM;
            }
            OS_RestoreInterrupts(irq);
        }
        if (SOCL_SocketIsTCP(socket)) {
            if ((socket->state & SOCL_STATUS_CONNECTED) && socket->cps_socket.state != CPS_STT_ESTABLISHED &&
                !(status & SOCL_POLLRDNORM)) {
                socket->state &= ~(SOCL_STATUS_CONNECTING | SOCL_STATUS_CONNECTED);
            }
            if (!(socket->state & SOCL_STATUS_CONNECTING) && !(socket->state & SOCL_STATUS_CONNECTED)) {
                status |= SOCL_POLLHUP;
            }
        }
    }
    return status;
}

// How much a read could take now: the next datagram's size for UDP, the buffered data for TCP
static s32 SOCLi_GetReadBufferOccpiedSize(const SOCLSocket *socket) {
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    s32 size = 0;

    if (pipe != NULL) {
        s8 mode = socket->flag;

        if (mode == 1) {
            SOCLiSocketUdpData *head = pipe->udp_head;

            size = head != NULL ? head->len : 0;
        } else if (mode == 0 || mode == 4) {
            size = socket->cps_socket.rcvbuf_len - pipe->cps_read_len;
        }
    }
    return size;
}
