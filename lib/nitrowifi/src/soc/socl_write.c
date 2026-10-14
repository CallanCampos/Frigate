#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "gfl/std.h"
#include "nitro/os.h"

// SOCL's writes: SOCL_WriteTo puts the data in the send pipe's ring buffer (SOCLi_WriteBuffer, which waits for room
// when the socket blocks) and queues a command (SOCLi_ExecWriteCommand) for the pipe's thread, which copies it to
// the socket's send buffer in packets and has CPS send it (SOCLi_WriteCallBack). pokediamond's NitroWiFi has this in
// the object socl_read.o, where this version has the write half in an object of its own.

static int SOCLi_WriteBuffer(SOCLSocket *socket, const u8 *buf, int len, u16 remote_port, CPSInAddr remote_ip,
                             int block);
static int SOCLi_AllocWriteBuffer(SOCLSocket *socket, int len, int chunk, int *write_pos, int block);
static int SOCLi_ExecWriteCommand(SOCLSocket *socket, const u8 *buf, int len, int pos, u16 remote_port,
                                  CPSInAddr remote_ip, int block);
static int SOCLi_WriteCallBack(SOCLCommandPacket *packet);
static int SOCLi_GetOptimumSendBufLen(int len);
static int SOCLi_MemCpy(u8 *dst, int len, SOCLCommandPacket *packet);

int SOCL_WriteTo(SOCLSocket *socket, const u8 *buf, int len, u16 remote_port, CPSInAddr remote_ip, int flags) {
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
    pipe = SOCLi_GetSendPipe(socket);
    if ((flags & SOCLI_WRITE_NOBLOCK) || socket->flag_block == 0) {
        if (!func_0207aeec(&pipe->base.mutex)) {
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

static int SOCLi_WriteBuffer(SOCLSocket *socket, const u8 *buf, int len, u16 remote_port, CPSInAddr remote_ip,
                             int block) {
    int written = 0;
    int chunk_max = SOCLi_GetSendPipe(socket)->socket->sndbuf.size;
    int alloc_len;
    int write_pos;

    if (socket->flag == 1) {
        chunk_max -= 0x2a;
        if (len > chunk_max) {
            return SOCL_EMSGSIZE;
        }
        chunk_max = len;
    } else {
        chunk_max -= 0x36;
        if (len <= chunk_max) {
            chunk_max = len;
        }
    }
    while (len > 0) {
        alloc_len = SOCLi_AllocWriteBuffer(socket, len, chunk_max, &write_pos, block);
        if (alloc_len > 0) {
            if (SOCLi_ExecWriteCommand(socket, buf, alloc_len, write_pos, remote_port, remote_ip, block) <= 0) {
                return SOCL_EAGAIN;
            }
            buf += alloc_len;
            written += alloc_len;
            len -= alloc_len;
        }
        if (!(block & 1)) {
            if (alloc_len > 0) {
                break;
            }
            return SOCL_EAGAIN;
        }
    }
    return written;
}

// Finds room for up to len bytes (at least chunk) in the pipe's buffer, sleeping for it when the socket blocks, and
// gives the position to write at
static int SOCLi_AllocWriteBuffer(SOCLSocket *socket, int len, int chunk, int *write_pos, int block) {
    SOCLSendPipe *pipe = SOCLi_GetSendPipe(socket);
    int free;
    u32 irq;

    if (chunk > len) {
        chunk = len;
    }
    irq = CPU_IRQDisable();
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
        scheduler_yield(&pipe->wait);
    }
    CPU_SetIRQMask(irq);
    return free;
}

int SOCLi_GetWriteBufferFreeSize(const SOCLSocket *socket) {
    SOCLSendPipe *pipe = SOCLi_GetSendPipe(socket);
    int size = pipe->buffer.size;
    int write_pos = pipe->write_pos;
    int read_pos = pipe->read_pos;
    int free = read_pos - write_pos - 1;

    if (free < 0) {
        free += size;
    }
    return free;
}

static int SOCLi_ExecWriteCommand(SOCLSocket *socket, const u8 *buf, int len, int pos, u16 remote_port,
                                  CPSInAddr remote_ip, int block) {
    SOCLSendPipe *pipe = SOCLi_GetSendPipe(socket);
    SOCLCommandPacket *packet = SOCLi_CreateCommandPacket(SOCLi_WriteCallBack, pipe->socket, block);
    u16 old_write_pos;
    u8 *data;
    int size;

    if (packet == NULL) {
        return SOCL_EMFILE;
    }
    if ((block & 1) && socket->flag != 1) {
        packet->flag_block = 3;
    } else {
        packet->flag_block = 0;
    }
    data = pipe->buffer.data;
    size = pipe->buffer.size;
    if (pos + len < size) {
        packet->args.write.buf1 = data + pos;
        packet->args.write.len1 = len;
        packet->args.write.buf2 = NULL;
        packet->args.write.len2 = 0;
        pos += len;
    } else {
        packet->args.write.buf1 = data + pos;
        packet->args.write.len1 = size - pos;
        packet->args.write.buf2 = data;
        packet->args.write.len2 = len - packet->args.write.len1;
        pos = packet->args.write.len2;
        sys_memcpy_fast(buf + packet->args.write.len1, packet->args.write.buf2, pos);
    }
    sys_memcpy_fast(buf, packet->args.write.buf1, packet->args.write.len1);
    old_write_pos = pipe->write_pos;
    packet->args.write.write_end = pos;
    pipe->write_pos = packet->args.write.write_end;
    if (socket->flag == 1) {
        if (socket->local_port == 0) {
            socket->local_port = CPSi_GetNewPort();
            ((CPSSoc *)socket)->local_port = socket->local_port;
        }
        packet->args.write.local_port = socket->local_port;
        if (socket->remote_ip == 0 || remote_ip != 0) {
            packet->args.write.remote_ip = remote_ip;
            packet->args.write.remote_port = remote_port;
        } else {
            packet->args.write.remote_ip = socket->remote_ip;
            packet->args.write.remote_port = socket->remote_port;
        }
    } else {
        packet->args.write.remote_ip = 0;
    }
    if (SOCLi_ExecCommandPacketInSendPipe(pipe->socket, packet) != 0) {
        pipe->write_pos = old_write_pos;
        len = 0;
    }
    return len;
}

static int SOCLi_WriteCallBack(SOCLCommandPacket *packet) {
    SOCLSocket *socket = packet->socket;
    int result = 0;
    SOCLSendPipe *pipe = SOCLi_GetSendPipe(socket);
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
        if (packet->args.write.remote_ip != 0) {
            CPS_SocBind(packet->args.write.local_port, packet->args.write.remote_port, packet->args.write.remote_ip);
        }
        mode = socket->flag;
        overhead = SOCLi_ModeIsTCP(mode) ? 0x36 : 0x2a;
        data = socket->sndbuf.data;
        if (mode == 0 || mode == 4) {
            chunk = SOCLi_GetOptimumSendBufLen(socket->sndbuf.size - overhead);
        } else {
            chunk = socket->sndbuf.size - overhead;
        }
        while (1) {
            len = SOCLi_MemCpy(data + overhead, chunk, packet);
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
    pipe->read_pos = packet->args.write.write_end;
    OS_WakeupThread(&pipe->wait);
    return result;
}

// The largest size that fits a whole number of the connection's segments, for TCP
static int SOCLi_GetOptimumSendBufLen(int len) {
    CPSSoc *soc = (CPSSoc *)OS_GetCurrentThread()->specific[0];
    u32 irq = CPU_IRQDisable();
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
    CPU_SetIRQMask(irq);
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
static int SOCLi_MemCpy(u8 *dst, int len, SOCLCommandPacket *packet) {
    int len1 = packet->args.write.len1;
    int len2 = packet->args.write.len2;

    if (len1 > len) {
        len1 = len;
        len2 = 0;
    } else if (len2 > len - len1) {
        len2 = len - len1;
    }
    if (len1 > 0) {
        sys_memcpy_fast(packet->args.write.buf1, dst, len1);
        packet->args.write.buf1 += len1;
        packet->args.write.len1 -= len1;
    }
    if (len2 > 0) {
        sys_memcpy_fast(packet->args.write.buf2, dst + len1, len2);
        packet->args.write.buf2 += len2;
        packet->args.write.len2 -= len2;
    }
    return len1 + len2;
}
