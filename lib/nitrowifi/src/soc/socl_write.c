#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's writes: SOCL_WriteTo puts the data in the send pipe's ring buffer (SOCLi_WriteBuffer, which waits for room
// when the socket blocks) and queues a command (SOCLi_ExecWriteCommand) for the pipe's thread, which copies it to
// the socket's send buffer in packets and has CPS send it (SOCLi_WriteCallBack). pokediamond's NitroWiFi has this in
// the object socl_read.o, where this version has the write half in an object of its own.

static int SOCLi_WriteBuffer(SOCLSocket *socket, const u8 *buf, s32 len, u16 remote_port, CPSInAddr remote_ip,
                             s32 block);
static s32 SOCLi_AllocWriteBuffer(SOCLSocket *socket, s32 len, s32 chunk, s32 *write_pos, s32 block);
static int SOCLi_ExecWriteCommand(SOCLSocket *socket, const u8 *buf, s32 len, s32 pos, u16 remote_port,
                                  CPSInAddr remote_ip, s32 block);
static int SOCLi_WriteCallBack(void *arg);
static int SOCLi_GetOptimumSendBufLen(int len);
static int SOCLi_MemCpy(u8 *dst, int len, SOCLiCommandWrite *cpacket);

int SOCL_WriteTo(int s, const void *buf, int len, u16 remote_port, CPSInAddr remote_ip, int flags) {
    SOCLSocket *socket = (SOCLSocket *)s;
    SOCLSendPipe *pipe;
    int block;
    int result;

    if (SOCL_SocketIsInvalid(socket)) {
        return SOCL_EINVAL;
    }
    if (!SOCLi_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (SOCLi_SocketIsTCP(socket)) {
        if (!(socket->state & SOCLI_STATE_CONNECTED) || (socket->state & SOCLI_STATE_CLOSING)) {
            return SOCL_ENOTCONN;
        }
    }
    pipe = socket->send_pipe;
    if ((flags & SOCLI_WRITE_NOBLOCK) || socket->flag_block == 0) {
        if (!OS_TryLockMutex(&pipe->base.mutex)) {
            return SOCL_EAGAIN;
        }
        block = 0;
    } else {
        OS_LockMutex(&pipe->base.mutex);
        block = 1;
    }
    result = SOCLi_WriteBuffer(socket, buf, len, remote_port, remote_ip, block);
    OS_UnlockMutex(&pipe->base.mutex);
    return result;
}

static int SOCLi_WriteBuffer(SOCLSocket *socket, const u8 *buf, s32 len, u16 remote_port, CPSInAddr remote_ip,
                             s32 block) {
    SOCLSendPipe *pipe = socket->send_pipe;
    s32 sndbuf_len;
    s32 write_pos;
    s32 alloc_len;
    s32 chunk_max;
    s32 written = 0;

    sndbuf_len = pipe->socket->cps_socket.sndbuf.size;
    if (socket->flag == 1) {
        sndbuf_len -= 0x2a;
        if (len > sndbuf_len) {
            return SOCL_EMSGSIZE;
        }
        chunk_max = len;
    } else {
        sndbuf_len -= 0x36;
        chunk_max = MATH_MIN(len, sndbuf_len);
    }
    while (len > 0) {
        alloc_len = SOCLi_AllocWriteBuffer(socket, len, chunk_max, &write_pos, block);
        if (alloc_len > 0) {
            if (SOCLi_ExecWriteCommand(socket, buf, alloc_len, write_pos, remote_port, remote_ip, block) <= 0) {
                return SOCL_EAGAIN;
            }
            buf += alloc_len;
            len -= alloc_len;
            written += alloc_len;
        }
        if (!(block & 1)) {
            if (alloc_len <= 0) {
                return SOCL_EAGAIN;
            }
            break;
        }
    }
    return written;
}

// Finds room for up to len bytes (at least chunk) in the pipe's buffer, sleeping for it when the socket blocks, and
// gives the position to write at
static s32 SOCLi_AllocWriteBuffer(SOCLSocket *socket, s32 len, s32 chunk, s32 *write_pos, s32 block) {
    SOCLSendPipe *pipe = socket->send_pipe;
    s32 free;
    u32 irq;

    if (chunk > len) {
        chunk = len;
    }
    irq = OS_DisableInterrupts();
    while (1) {
        free = SOCLi_GetWriteBufferFreeSize(socket);
        if (free >= chunk) {
            if (free >= len) {
                free = len;
            }
            *write_pos = pipe->write_pos;
            break;
        }
        if (!(block & 1)) {
            free = 0;
            break;
        }
        OS_SleepThread(&pipe->wait);
    }
    OS_RestoreInterrupts(irq);
    return free;
}

s32 SOCLi_GetWriteBufferFreeSize(SOCLSocket *socket) {
    SOCLSendPipe *pipe = socket->send_pipe;
    s32 size = pipe->buffer.size;
    s32 write_pos = pipe->write_pos;
    s32 read_pos = pipe->read_pos;
    s32 free = read_pos - write_pos - 1;

    if (free < 0) {
        free += size;
    }
    return free;
}

static int SOCLi_ExecWriteCommand(SOCLSocket *socket, const u8 *buf, s32 len, s32 pos, u16 remote_port,
                                  CPSInAddr remote_ip, s32 block) {
    SOCLSendPipe *pipe = socket->send_pipe;
    SOCLCommandPacket *packet = SOCLi_CreateCommandPacket(SOCLi_WriteCallBack, pipe->socket, block);
    u16 old_write_pos;
    u8 *data;
    int size;

    if (packet == NULL) {
        return SOCL_EMFILE;
    }
    if ((block & 1) && socket->flag != 1) {
        packet->h.flag_block = 3;
    } else {
        packet->h.flag_block = 0;
    }
    data = pipe->buffer.data;
    size = pipe->buffer.size;
    if (pos + len < size) {
        packet->write.buf1 = data + pos;
        packet->write.len1 = len;
        packet->write.buf2 = NULL;
        packet->write.len2 = 0;
        pos += len;
    } else {
        packet->write.buf1 = data + pos;
        packet->write.len1 = size - pos;
        packet->write.buf2 = data;
        packet->write.len2 = len - packet->write.len1;
        pos = packet->write.len2;
        MI_CpuCopy(buf + packet->write.len1, packet->write.buf2, pos);
    }
    MI_CpuCopy(buf, packet->write.buf1, packet->write.len1);
    old_write_pos = pipe->write_pos;
    packet->write.write_end = pos;
    pipe->write_pos = packet->write.write_end;
    if (socket->flag == 1) {
        if (socket->local_port == 0) {
            socket->local_port = CPSi_GetNewPort();
            socket->cps_socket.local_port = socket->local_port;
        }
        packet->write.local_port = socket->local_port;
        if (socket->remote_ip == 0 || remote_ip != 0) {
            packet->write.remote_ip = remote_ip;
            packet->write.remote_port = remote_port;
        } else {
            packet->write.remote_ip = socket->remote_ip;
            packet->write.remote_port = socket->remote_port;
        }
    } else {
        packet->write.remote_ip = 0;
    }
    if (SOCLi_ExecCommandPacketInSendPipe(pipe->socket, packet) != 0) {
        pipe->write_pos = old_write_pos;
        len = 0;
    }
    return len;
}

static int SOCLi_WriteCallBack(void *arg) {
    SOCLiCommandWrite *cpacket = (SOCLiCommandWrite *)arg;
    SOCLSocket *socket = cpacket->h.socket;
    int result = 0;
    SOCLSendPipe *pipe = socket->send_pipe;
    u8 *data;
    int overhead;
    s8 mode;
    int chunk;
    int len;
    int written;

    if (socket->flag == 2 && (socket->state & SOCLI_STATE_CLOSING)) {
        return SOCL_EINVAL;
    }
    if (socket->flag != 0 && socket->flag != 4 || (socket->state & SOCLI_STATE_CONNECTED)) {
        if (cpacket->remote_ip != 0) {
            CPS_SocBind(cpacket->local_port, cpacket->remote_port, cpacket->remote_ip);
        }
        mode = socket->flag;
        overhead = SOCLi_ModeIsTCP(mode) ? 0x36 : 0x2a;
        data = socket->cps_socket.sndbuf.data;
        if (mode == 0 || mode == 4) {
            chunk = SOCLi_GetOptimumSendBufLen(socket->cps_socket.sndbuf.size - overhead);
        } else {
            chunk = socket->cps_socket.sndbuf.size - overhead;
        }
        while (1) {
            len = SOCLi_MemCpy(data + overhead, chunk, cpacket);
            if (len <= 0) {
                break;
            }
            written = CPS_SocWrite(data + overhead, len);
            if (written <= 0) {
                if (SOCLi_SocketIsTCP(socket)) {
                    socket->state &= ~(SOCLI_STATE_BINDING | SOCLI_STATE_CONNECTED | SOCLI_STATE_CLOSING);
                    socket->state |= SOCLI_STATE_RESET;
                    CPS_TcpClose();
                }
                result = SOCL_ETIMEDOUT;
                break;
            }
            result += written;
        }
    } else {
        result = SOCL_ETIMEDOUT;
    }
    pipe->read_pos = cpacket->write_end;
    OS_WakeupThread(&pipe->wait);
    return result;
}

// The largest size that fits a whole number of the connection's segments, for TCP
static int SOCLi_GetOptimumSendBufLen(int len) {
    CPSSoc *soc = (CPSSoc *)OS_GetCurrentThread()->specific[0];
    u32 irq = OS_DisableInterrupts();
    int unit;
    int mss;
    int win;
    int mymss;
    int count;

    if (soc != NULL) {
        mss = soc->remote_mss;
        if (mss != 0 && (win = soc->remote_win) != 0) {
            mymss = SOCLiCPSConfig.mymss;
            if (mss > win) {
                mss = win;
            }
            if (mss > mymss) {
                mss = mymss;
            }
            unit = mss * 2;
        } else {
            unit = SOCLiCPSConfig.mymss * 2;
        }
    } else {
        unit = SOCLiCPSConfig.mymss * 2;
    }
    OS_RestoreInterrupts(irq);
    if (unit > 0) {
        count = len / unit;
        if (count > 0) {
            return unit * count;
        }
    }
    return len;
}

// Copies up to len bytes of the command's data (in two parts when it wraps around the pipe's buffer) to dst, and
// takes them off the command
static int SOCLi_MemCpy(u8 *dst, int len, SOCLiCommandWrite *cpacket) {
    int len1 = cpacket->len1;
    int len2 = cpacket->len2;

    if (len1 > len) {
        len1 = len;
        len2 = 0;
    } else if (len2 > len - len1) {
        len2 = len - len1;
    }
    if (len1 > 0) {
        MI_CpuCopy(cpacket->buf1, dst, len1);
        cpacket->buf1 += len1;
        cpacket->len1 -= len1;
    }
    if (len2 > 0) {
        MI_CpuCopy(cpacket->buf2, dst + len1, len2);
        cpacket->buf2 += len2;
        cpacket->len2 -= len2;
    }
    return len1 + len2;
}
