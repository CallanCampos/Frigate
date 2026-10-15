#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "nitro/os.h"

// SOCL_Close: marks the socket closing and queues SOCLi_CloseCallBack in its control pipe, which shuts the connection
// down and moves the socket to the trash list. SOCLi_TrashSocket (CPS's scavenger callback) cleans the trash up:
// SOCLi_CleanupSocket frees the socket's pipes (SOCLi_FreeCommandPipe, which answers the commands still queued with
// an error) and then the socket. pokediamond's NitroWiFi names the object socl_close.o and its functions, except the
// two that ask about the socket's state, which this version added.

static int SOCLi_CloseCallBack(SOCLCommandPacket *packet);
static void SOCLi_CleanupSocket(SOCLSocket *socket);
static void SOCLi_FreeCommandPipe(SOCLCommandPipe *pipe);

BOOL SOCL_IsClosed(int socket) {
    if (socket >= 0 && SOCL_SocketIsInvalid((SOCLSocket *)socket) && !SOCL_SocketIsInTrash((SOCLSocket *)socket)) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov011_02157314(int socket) {
    SOCLSocket *s = (SOCLSocket *)socket;
    u32 irq;

    if (socket <= 0) {
        return FALSE;
    }
    irq = CPU_IRQDisable();
    if (SOCL_SocketIsInTrash(s)) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    if (SOCL_SocketIsInvalid(s)) {
        CPU_SetIRQMask(irq);
        return TRUE;
    }
    if (s->state & SOCLI_STATE_CLOSE_CALLED) {
        CPU_SetIRQMask(irq);
        return FALSE;
    }
    CPU_SetIRQMask(irq);
    return TRUE;
}

int SOCL_Close(int socket) {
    SOCLSocket *s = (SOCLSocket *)socket;
    SOCLCommandPacket *packet;
    u32 irq;

    if (socket <= 0) {
        return SOCL_EINVAL;
    }
    irq = CPU_IRQDisable();
    if (SOCL_SocketIsInTrash(s)) {
        CPU_SetIRQMask(irq);
        return SOCL_EINPROGRESS;
    }
    if (SOCL_SocketIsInvalid(s)) {
        CPU_SetIRQMask(irq);
        return 0;
    }
    if (!SOCLi_SocketIsCreated(s)) {
        CPU_SetIRQMask(irq);
        return SOCL_ENETRESET;
    }
    if (s->state & SOCLI_STATE_CLOSE_CALLED) {
        CPU_SetIRQMask(irq);
        return SOCL_EINPROGRESS;
    }
    CPU_SetIRQMask(irq);
    s->state |= SOCLI_STATE_CLOSING | SOCLI_STATE_CLOSE_CALLED;
    if (SOCLi_SocketIsTCP(s)) {
        SOCLi_SendCommandPacket(s->send_pipe, NULL);
        SOCLiSocketCount[0]--;
    } else {
        SOCLiSocketCount[1]--;
    }
    packet = SOCLi_CreateCommandPacket(SOCLi_CloseCallBack, s, 1);
    packet->reply_queue = NULL;
    SOCLi_SendCommandPacketToCtrlPipe(s, packet);
    return 0;
}

static int SOCLi_CloseCallBack(SOCLCommandPacket *packet) {
    SOCLSocket *socket = packet->socket;
    u32 irq;

    if (SOCLi_SocketIsTCP(socket)) {
        func_0207a800(&SOCLi_GetSendPipe(socket)->base.thread);
        CPS_TcpShutdown();
        CPS_TcpClose();
        CPS_SocRelease();
    }
    CPS_SocUnRegister();
    socket->state &= ~(SOCLI_STATE_BINDING | SOCLI_STATE_CONNECTED);
    SOCLi_SendCommandPacket(socket->flag == 2 ? socket->send_pipe : socket->recv_pipe, NULL);
    irq = CPU_IRQDisable();
    SOCLi_SocketUnregister(socket);
    SOCLi_SocketRegisterTrash(socket);
    CPU_SetIRQMask(irq);
    socket->state |= SOCLI_STATE_CLOSED;
    return 0;
}

static void SOCLi_CleanupSocket(SOCLSocket *socket) {
    u32 irq;
    s8 flag;

    if (socket == NULL) {
        return;
    }
    socket->state = 0;
    flag = socket->flag;
    if (SOCLi_ModeIsTCP(flag)) {
        SOCLi_FreeCommandPipe(socket->send_pipe);
        SOCLi_FreeCommandPipe(socket->recv_pipe);
    } else if (flag == 1) {
        SOCLUdpPacket *udp = SOCLi_GetRecvPipe(socket)->udp_head;

        while (udp != NULL) {
            SOCLUdpPacket *next = udp->next;

            SOCLiConfigPtr->free(udp);
            udp = next;
        }
        SOCLi_GetRecvPipe(socket)->udp_queued = 0;
        SOCLi_GetRecvPipe(socket)->udp_tail = NULL;
        SOCLi_GetRecvPipe(socket)->udp_head = NULL;
        OS_WakeupThread(&SOCLi_GetRecvPipe(socket)->udp_wait);
        SOCLi_FreeCommandPipe(socket->recv_pipe);
    } else if (flag == 2) {
        SOCLi_FreeCommandPipe(socket->send_pipe);
    }
    irq = CPU_IRQDisable();
    SOCLi_SocketUnregister(socket);
    SOCLi_SocketUnregisterTrash(socket);
    SOCLiConfigPtr->free(socket);
    CPU_SetIRQMask(irq);
}

// Waits for the pipe's thread to end, and answers the commands that it never ran with an error
static void SOCLi_FreeCommandPipe(SOCLCommandPipe *pipe) {
    OSMessage message;
    u32 irq;

    if (pipe == NULL) {
        return;
    }
    func_0207a800(&pipe->thread);
    irq = CPU_IRQDisable();
    func_0207aab4();
    if (OS_ReceiveMessage(&pipe->queue, &message, OS_MESSAGE_NOBLOCK)) {
        do {
            if (message != NULL) {
                if (((SOCLCommandPacket *)message)->reply_queue != NULL) {
                    OS_SendMessage(((SOCLCommandPacket *)message)->reply_queue, (OSMessage)SOCL_ECANCELED,
                                   OS_MESSAGE_NOBLOCK);
                }
                SOCLi_FreeCommandPacket(message);
            }
        } while (OS_ReceiveMessage(&pipe->queue, &message, OS_MESSAGE_NOBLOCK));
    }
    func_0207aad8();
    func_0207a918();
    CPU_SetIRQMask(irq);
}

void SOCLi_TrashSocket(void) {
    u32 irq = CPU_IRQDisable();

    while (SOCLiSocketListTrash != NULL) {
        SOCLi_CleanupSocket(SOCLiSocketListTrash);
    }
    CPU_SetIRQMask(irq);
}
