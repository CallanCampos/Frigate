#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's reads: SOCL_ReadFrom takes data from a socket, under its receive pipe's mutex. A TCP socket reads from
// CPS's receive buffer (SOCLi_CopyCPSBuffer) or, when it has to wait for data, through a command in its receive pipe
// (SOCLi_ReadCallBack); a UDP socket reads the datagrams that SOCLi_UdpRecvCallback queued in its pipe
// (SOCLi_ReadUdpBuffer). What was read from CPS's buffer is consumed (CPS_SocConsume) once it passes the socket's
// minimum. pokediamond's NitroWiFi names the object socl_read.o and every function in it, except the callback that
// reads without waiting, which this version added. Its write half is socl_write.c here.
static int SOCLi_ReadAndConsumeBuffer(SOCLSocket *socket, u8 *buf, int len, u16 *remote_port, CPSInAddr *remote_ip,
                                      int block, int flags);
static int SOCLi_ReadBuffer(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip, s32 block);
static int SOCLi_CopyCPSBuffer(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip);
static u8 *SOCLi_ReadCPSBuffer(SOCLSocket *socket, int *avail, u16 *local_port, u16 *remote_port, CPSInAddr *remote_ip);
static int SOCLi_ExecReadCommand(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip,
                                 int block);
static int SOCLi_ReadCallBack(void *arg);
static int SOCLi_ReadNoWaitCallBack(void *arg);
static int SOCLi_ConsumeBuffer(SOCLSocket *socket);
static int SOCLi_ConsumeCallBack(void *arg);
static int SOCLi_ConsumeCPSBuffer(SOCLSocket *socket);
static int SOCLi_ReadUdpBuffer(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip,
                               s32 block);

// How often the UDP queue had no memory for a datagram, and how often it was full
static u32 SOCLi_CheckCount[2];

int SOCL_ReadFrom(int s, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip, int flags) {
    SOCLSocket *socket = (SOCLSocket *)s;
    int block;
    int result;
    SOCLiSocketRecvCommandPipe *pipe;

    if (SOCL_SocketIsInvalid(socket)) {
        return SOCL_EINVAL;
    }
    if ((flags & SOCL_MSG_DONTWAIT) || socket->flag_block == 0) {
        block = 0;
    } else {
        if (OS_GetProcMode() == OS_PROCMODE_IRQ) {
            return SOCL_EINVAL;
        }
        block = 1;
    }
    if (!SOCL_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (SOCL_SocketIsTCP(socket)) {
        if (!(socket->state & SOCL_STATUS_CONNECTED) || (socket->state & SOCL_STATUS_CLOSING)) {
            return SOCL_ENOTCONN;
        }
    }
    pipe = socket->recv_pipe;
    if (!(block & 1)) {
        if (!OS_TryLockMutex(&pipe->base.mutex)) {
            return SOCL_EAGAIN;
        }
    } else {
        OS_LockMutex(&pipe->base.mutex);
    }
    result = SOCLi_ReadAndConsumeBuffer(socket, buf, len, remote_port, remote_ip, block, flags);
    OS_UnlockMutex(&pipe->base.mutex);
    return result;
}

static int SOCLi_ReadAndConsumeBuffer(SOCLSocket *socket, u8 *buf, int len, u16 *remote_port, CPSInAddr *remote_ip,
                                      int block, int flags) {
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    BOOL peek = (flags & SOCL_MSG_PEEK) && pipe != NULL;
    s8 was_peeking;
    int result;

    if (peek) {
        was_peeking = pipe->peek;
        pipe->peek = 1;
    }
    if (socket->flag == 1) {
        result = SOCLi_ReadUdpBuffer(socket, buf, len, remote_port, remote_ip, block);
    } else {
        result = SOCLi_ReadBuffer(socket, buf, len, remote_port, remote_ip, block);
        if (result >= 0) {
            SOCLi_ConsumeBuffer(socket);
        }
    }
    if (peek) {
        pipe->peek = was_peeking;
    }
    return result;
}

static int SOCLi_ReadBuffer(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip, s32 block) {
    int result;

    if (socket->flag == 4) {
        return SOCLi_ExecReadCommand(socket, buf, len, remote_port, remote_ip, block);
    }
    result = SOCLi_CopyCPSBuffer(socket, buf, len, remote_port, remote_ip);
    if (result == SOCL_EAGAIN && (block & 1)) {
        result = SOCLi_ExecReadCommand(socket, buf, len, remote_port, remote_ip, 1);
    }
    return result;
}

static int SOCLi_CopyCPSBuffer(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip) {
    u16 local_port;
    u16 sender_port;
    int avail;
    CPSInAddr sender_ip;
    int result;
    u8 *data;
    OSIntrMode irq = OS_DisableInterrupts();

    data = SOCLi_ReadCPSBuffer(socket, &avail, &local_port, &sender_port, &sender_ip);
    if (data != NULL) {
        result = avail;
        if (result == 0) {
            result = SOCL_EAGAIN;
        } else {
            if (len > result) {
                len = result;
            }
            if (SOCL_SocketIsTCP(socket)) {
                result = len;
            }
            MI_CpuCopy(data, buf, len);
            if (socket->recv_pipe->peek == 0) {
                socket->recv_pipe->cps_read_len += result;
            }
        }
    } else {
        if (avail == 0) {
            result = 0;
        } else {
            result = SOCL_EINVAL;
        }
        socket->state &= ~(SOCL_STATUS_CONNECTING | SOCL_STATUS_CONNECTED);
    }
    if (result >= 0) {
        if (remote_port != NULL && remote_ip != NULL) {
            *remote_port = sender_port;
            *remote_ip = sender_ip;
        }
        if (socket->local_port == 0) {
            socket->local_port = local_port;
        }
    }
    OS_RestoreInterrupts(irq);
    return result;
}

// Where CPS's receive buffer stands for the socket: the data not yet read (avail, -1 when CPS has gone ahead of the
// pipe) and the ports and address of its sender. NULL when there is nothing to read or the connection has gone
static u8 *SOCLi_ReadCPSBuffer(SOCLSocket *socket, int *avail, u16 *local_port, u16 *remote_port,
                               CPSInAddr *remote_ip) {
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    CPSSoc *soc = (CPSSoc *)pipe->base.thread.specific[0];
    int read_len = pipe->cps_read_len;
    int left = soc->rcvbuf_len - read_len;

    if (left >= 0) {
        *local_port = soc->local_port;
        *remote_port = soc->remote_port;
        *remote_ip = soc->remote_ip;
        *avail = left;
        if (left == 0 && soc->state != CPS_STT_ESTABLISHED) {
            return NULL;
        }
    } else {
        *avail = -1;
        return NULL;
    }
    return soc->rcvbuf.data + read_len;
}

static int SOCLi_ExecReadCommand(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip,
                                 int block) {
    SOCLiCommandPacket *packet;

    if (block & 1) {
        packet = SOCLi_CreateCommandPacket(SOCLi_ReadCallBack, socket, 1);
    } else {
        packet = SOCLi_CreateCommandPacket(SOCLi_ReadNoWaitCallBack, socket, 1);
    }
    packet->read.buf = buf;
    packet->read.len = len;
    packet->read.remote_port = remote_port;
    packet->read.remote_ip = remote_ip;
    return SOCLi_ExecCommandPacketInRecvPipe(socket, packet);
}

static int SOCLi_ReadCallBack(void *arg) {
    SOCLiCommandRead *cpacket = (SOCLiCommandRead *)arg;
    SOCLSocket *socket = cpacket->h.socket;
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    u8 *buf = cpacket->buf;
    int len = cpacket->len;
    u16 *remote_port = cpacket->remote_port;
    CPSInAddr *remote_ip = cpacket->remote_ip;
    u8 *data;
    u32 cps_len;
    int read_len;
    int result;

    read_len = pipe->cps_read_len;
    while (1) {
        data = CPS_SocRead(&cps_len);
        if (data == NULL || (int)cps_len - read_len > 0) {
            break;
        }
        if (SOCL_SocketIsTCP(socket) && socket->cps_socket.state != CPS_STT_ESTABLISHED) {
            data = NULL;
            break;
        }
        OS_Sleep(10);
    }
    if (socket->state & SOCL_STATUS_RESET) {
        return SOCL_ECONNRESET;
    }
    if (socket->flag == 4) {
        if (data == NULL) {
            return 0;
        }
        if (len > cps_len) {
            len = cps_len;
        }
        MI_CpuCopy(data, buf, len);
        CPS_SocConsume(len);
        return len;
    }
    if (data != NULL) {
        result = SOCLi_CopyCPSBuffer(socket, buf, len, remote_port, remote_ip);
    } else {
        result = 0;
    }
    if (result <= 0) {
        return result;
    }
    if (pipe->cps_read_len >= pipe->rcvbuf_consume_min) {
        SOCLi_ConsumeCPSBuffer(socket);
    }
    return result;
}

// The read command for a call that does not wait: it takes what CPS has now
static int SOCLi_ReadNoWaitCallBack(void *arg) {
    SOCLiCommandRead *cpacket = (SOCLiCommandRead *)arg;
    SOCLSocket *socket = cpacket->h.socket;
    u8 *buf = cpacket->buf;
    int len = cpacket->len;
    int read_len = socket->recv_pipe->cps_read_len;
    s32 length;
    u32 cps_len;
    u8 *data;

    if (socket->state & SOCL_STATUS_RESET) {
        return SOCL_ECONNRESET;
    }
    length = CPS_SocGetLength();
    if (length == 0) {
        return SOCL_EAGAIN;
    }
    if (length == -1) {
        return 0;
    }
    data = CPS_SocRead(&cps_len);
    if (data != NULL && (int)(cps_len - read_len) <= 0) {
        if (SOCL_SocketIsTCP(socket)) {
            if (socket->cps_socket.state != CPS_STT_ESTABLISHED) {
                data = NULL;
            }
        }
    }
    if (socket->state & SOCL_STATUS_RESET) {
        return SOCL_ECONNRESET;
    }
    if (data == NULL) {
        return 0;
    }
    if (len > cps_len) {
        len = cps_len;
    }
    MI_CpuCopy(data, buf, len);
    CPS_SocConsume(len);
    return len;
}

static int SOCLi_ConsumeBuffer(SOCLSocket *socket) {
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    SOCLiCommandPacket *packet;
    int result;

    if (pipe->cps_read_len < pipe->rcvbuf_consume_min) {
        return 0;
    }
    packet = SOCLi_CreateCommandPacket(SOCLi_ConsumeCallBack, socket, 0);
    if (packet == NULL) {
        return SOCL_EMFILE;
    }
    result = SOCLi_ExecCommandPacketInRecvPipe(socket, packet);
    return result;
}

static int SOCLi_ConsumeCallBack(void *arg) {
    SOCLiCommandConsume *cpacket = (SOCLiCommandConsume *)arg;
    return SOCLi_ConsumeCPSBuffer(cpacket->h.socket);
}

static int SOCLi_ConsumeCPSBuffer(SOCLSocket *socket) {
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    OSIntrMode irq = OS_DisableInterrupts();
    int consume_len = pipe->cps_read_len;

    if (consume_len != 0) {
        pipe->cps_read_len = 0;
        CPS_SocConsume(consume_len);
    }
    OS_RestoreInterrupts(irq);
    return consume_len;
}

static int SOCLi_ReadUdpBuffer(SOCLSocket *socket, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip,
                               s32 block) {
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    SOCLiSocketUdpData *packet;
    OSIntrMode irq = OS_DisableInterrupts();
    int result;

    packet = pipe->udp_head;
    if (packet == NULL) {
        BOOL wait = block & 1;

        while (1) {
            if (!wait) {
                result = SOCL_EAGAIN;
                break;
            }
            OS_SleepThread(&pipe->udp_wait);
            if (SOCL_SocketIsInvalid(socket) || !SOCL_SocketIsCreated(socket)) {
                result = SOCL_ENOTCONN;
                break;
            }
            packet = pipe->udp_head;
            if (packet != NULL) {
                break;
            }
        }
    }
    if (packet != NULL) {
        if (len > packet->len) {
            len = packet->len;
        }
        MI_CpuCopy(packet->data, buf, len);
        if (remote_port != NULL) {
            *remote_port = packet->port;
        }
        if (remote_ip != NULL) {
            *remote_ip = packet->ip;
        }
        result = packet->len;
        if (pipe->peek == 0) {
            pipe->udp_head = packet->next;
            if (packet->next == NULL) {
                pipe->udp_tail = NULL;
            }
            SOCLiConfigPtr->free(packet);
            pipe->udp_queued -= result;
        }
    }
    OS_RestoreInterrupts(irq);
    return result;
}

BOOL SOCLi_UdpRecvCallback(u8 *data, u32 len, CPSSoc *soc) {
    SOCLSocket *socket = (SOCLSocket *)soc;
    SOCLiSocketRecvCommandPipe *pipe = socket->recv_pipe;
    SOCLiSocketUdpData *packet;
    OSIntrMode irq = OS_DisableInterrupts();

    if (pipe->udp_queued_max >= pipe->udp_queued + len) {
        packet = SOCLiConfigPtr->alloc(len + sizeof(SOCLiSocketUdpData));
        if (packet != NULL) {
            pipe->udp_queued += len;
            packet->next = NULL;
            packet->len = len;
            packet->port = soc->remote_port;
            packet->ip = soc->remote_ip;
            MI_CpuCopy(data, packet->data, len);
            if (pipe->udp_tail != NULL) {
                pipe->udp_tail->next = packet;
            }
            pipe->udp_tail = packet;
            if (pipe->udp_head == NULL) {
                pipe->udp_head = packet;
            }
        } else {
            SOCLi_CheckCount[0]++;
        }
    } else {
        SOCLi_CheckCount[1]++;
    }
    if (socket->local_port == 0) {
        socket->local_port = soc->local_port;
    }
    soc->remote_port = soc->remote_port_bound;
    soc->remote_ip = soc->remote_ip_bound;
    OS_WakeupThread(&pipe->udp_wait);
    OS_RestoreInterrupts(irq);
    return TRUE;
}
