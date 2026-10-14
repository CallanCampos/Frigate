#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "gfl/std.h"
#include "nitro/os.h"

// SOCL's socket creation: SOCL_CreateSocket gets the memory for a socket and its pipes and buffers (their sizes come
// from a SOCLSocketParam), then has the socket's control pipe register it with CPS. pokediamond's NitroWiFi names the
// object socl_create.o and every function in it (and has the listen and accept calls here, which this version lacks).
static int SOCLi_CreateSocketCallBack(SOCLCommandPacket *packet);
static SOCLSocket *SOCLi_StartupSocket(const SOCLSocketParam *param);
static u32 SOCLi_GetSizeSocket(const SOCLSocketParam *param);
static u32 SOCLi_GetSizeCommandPipe(const SOCLPipeParam *param);
static u8 *SOCLi_InitSocket(SOCLSocket *socket, const SOCLSocketParam *param);
static u8 *SOCLi_InitSocketBuffer(u8 *data, SOCLSocketBuffer *buffer, u32 size);
static u8 *SOCLi_InitCommandPipe(u8 *data, SOCLCommandPipe *pipe, const SOCLPipeParam *param);

int SOCL_CreateSocket(const SOCLSocketParam *param) {
    SOCLSocket *socket;

    if (SOCLiConfigPtr == NULL) {
        return SOCL_ENETRESET;
    }
    socket = SOCLi_StartupSocket(param);
    if (socket == NULL) {
        return SOCL_ENOMEM;
    }
    SOCLi_ExecCommandPacketInCtrlPipe(socket, SOCLi_CreateCommandPacket(SOCLi_CreateSocketCallBack, socket, 1));
    if (SOCLi_SocketIsCreated(socket)) {
        if (SOCLi_SocketIsTCP(socket)) {
            SOCLiSocketCount[0]++;
        } else {
            SOCLiSocketCount[1]++;
        }
    }
    return (int)socket;
}

static int SOCLi_CreateSocketCallBack(SOCLCommandPacket *packet) {
    SOCLSocket *socket = packet->socket;
    SOCLCommandPipe *send_pipe;

    CPS_SocRegister((CPSSoc *)socket);
    send_pipe = socket->send_pipe;
    switch (socket->flag) {
    case 0:
    case 4:
        CPS_SocDup(&send_pipe->thread);
        CPS_SocUse();
        break;
    case 1:
        CPS_SocUse();
        CPS_SocDatagramMode();
        CPS_SetUdpCallback(SOCLi_UdpRecvCallback);
        break;
    case 2:
        CPS_SocDatagramMode();
        break;
    }
    socket->state = SOCLI_STATE_CREATED;
    return 0;
}

static SOCLSocket *SOCLi_StartupSocket(const SOCLSocketParam *param) {
    u32 size = SOCLi_GetSizeSocket(param);
    u32 irq = CPU_IRQDisable();
    SOCLSocket *socket = (SOCLSocket *)SOCLiConfigPtr->alloc(size);

    if (socket != NULL) {
        sys_memset(socket, 0, size);
        SOCLi_InitSocket(socket, param);
        SOCLi_SocketRegister(socket);
    }
    CPU_SetIRQMask(irq);
    return socket;
}

static u32 SOCLi_GetSizeSocket(const SOCLSocketParam *param) {
    u32 size = sizeof(SOCLSocket);

    if (param->buffer.rcvbuf_size != 0) {
        size += sizeof(SOCLRecvPipe);
        size += SOCLi_RoundUp4(param->buffer.rcvbuf_size);
        size += SOCLi_RoundUp4(param->buffer.rcvbuf_aux_size);
        size += SOCLi_GetSizeCommandPipe(&param->rcv_pipe);
    }
    if (param->buffer.sndbuf_size != 0) {
        size += sizeof(SOCLSendPipe);
        size += SOCLi_RoundUp4(param->buffer.sndbuf_size);
        size += SOCLi_RoundUp4(param->buffer.sndbuf_aux_size);
        size += SOCLi_RoundUp4(param->buffer.sndpipe_buf_size);
        size += SOCLi_GetSizeCommandPipe(&param->snd_pipe);
    }
    return size;
}

static u32 SOCLi_GetSizeCommandPipe(const SOCLPipeParam *param) {
    return SOCLi_RoundUp4(param->queue_length * sizeof(OSMessage)) + SOCLi_RoundUp4(param->stack_size);
}

static u8 *SOCLi_InitSocket(SOCLSocket *socket, const SOCLSocketParam *param) {
    u8 *data = (u8 *)socket + sizeof(SOCLSocket);

    socket->flag = param->flag_mode;
    socket->flag_block = param->flag_block;
    if (param->buffer.rcvbuf_size != 0) {
        SOCLRecvPipe *pipe = (SOCLRecvPipe *)data;

        socket->recv_pipe = &pipe->base;
        pipe->rcvbuf_consume_min = param->buffer.rcvbuf_consume_min;
        data = SOCLi_InitCommandPipe(data + sizeof(SOCLRecvPipe), &pipe->base, &param->rcv_pipe);
        data = SOCLi_InitSocketBuffer(data, &socket->rcvbuf, param->buffer.rcvbuf_size);
        data = SOCLi_InitSocketBuffer(data, &socket->rcvbuf_aux, param->buffer.rcvbuf_aux_size);
        pipe->udp_queued_max = param->buffer.rcvpipe_unk_e;
        pipe->udp_wait.head = pipe->udp_wait.tail = NULL;
    }
    if (param->buffer.sndbuf_size != 0) {
        SOCLSendPipe *pipe = (SOCLSendPipe *)data;

        socket->send_pipe = &pipe->base;
        pipe->socket = socket;
        data = SOCLi_InitCommandPipe(data + sizeof(SOCLSendPipe), &pipe->base, &param->snd_pipe);
        data = SOCLi_InitSocketBuffer(data, &socket->sndbuf, param->buffer.sndbuf_size);
        data = SOCLi_InitSocketBuffer(data, &socket->sndbuf_aux, param->buffer.sndbuf_aux_size);
        data = SOCLi_InitSocketBuffer(data, &pipe->buffer, param->buffer.sndpipe_buf_size);
        pipe->wait.head = pipe->wait.tail = NULL;
    } else {
        socket->send_pipe = SOCLiUDPSendSocket->send_pipe;
    }
    return data;
}

static u8 *SOCLi_InitSocketBuffer(u8 *data, SOCLSocketBuffer *buffer, u32 size) {
    u8 *end = data;

    if (size == 0) {
        data = NULL;
    }
    buffer->data = data;
    buffer->size = size;
    return end + SOCLi_RoundUp4(size);
}

static u8 *SOCLi_InitCommandPipe(u8 *data, SOCLCommandPipe *pipe, const SOCLPipeParam *param) {
    u32 size = SOCLi_GetSizeCommandPipe(param);

    func_0207acc4(&pipe->queue, (OSMessage *)data, param->queue_length);
    OS_InitMutex(&pipe->mutex);
    scheduler_init_thread(&pipe->thread, (void (*)(void *))SOCLi_CommandPacketHandler, pipe, data + size, param->stack_size, param->priority);
    scheduler_start_thread(&pipe->thread);
    return data + size;
}
