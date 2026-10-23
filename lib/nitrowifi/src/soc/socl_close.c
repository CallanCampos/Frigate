#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL_Close: marks the socket closing and queues SOCLi_CloseCallBack in its control pipe, which shuts the connection
// down and moves the socket to the trash list. SOCLi_TrashSocket (CPS's scavenger callback) cleans the trash up:
// SOCLi_CleanupSocket frees the socket's pipes (SOCLi_FreeCommandPipe, which answers the commands still queued with
// an error) and then the socket. pokediamond's NitroWiFi names the object socl_close.o and its functions, except the
// two that ask about the socket's state, which this version added.

static int SOCLi_CloseCallBack(void *arg);
static void SOCLi_CleanupSocket(SOCLSocket *socket);
static void SOCLi_FreeCommandPipe(SOCLiSocketCommandPipe *pipe);

BOOL SOCL_IsClosed(int socket) {
    if (socket >= 0 && SOCL_SocketIsInvalid((SOCLSocket *)socket) && !SOCL_SocketIsInTrash((SOCLSocket *)socket)) {
        return TRUE;
    }
    return FALSE;
}

BOOL SOCL_IsNotClosing(int socket) {
    SOCLSocket *s = (SOCLSocket *)socket;
    OSIntrMode irq;

    if (socket <= 0) {
        return FALSE;
    }
    irq = OS_DisableInterrupts();
    if (SOCL_SocketIsInTrash(s)) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    if (SOCL_SocketIsInvalid(s)) {
        OS_RestoreInterrupts(irq);
        return TRUE;
    }
    if (s->state & SOCL_STATUS_WAIT_CLOSE) {
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    OS_RestoreInterrupts(irq);
    return TRUE;
}

int SOCL_Close(int socket) {
    SOCLSocket *s = (SOCLSocket *)socket;
    SOCLiCommandPacket *packet;
    OSIntrMode irq;

    if (socket <= 0) {
        return SOCL_EINVAL;
    }
    irq = OS_DisableInterrupts();
    if (SOCL_SocketIsInTrash(s)) {
        OS_RestoreInterrupts(irq);
        return SOCL_EINPROGRESS;
    }
    if (SOCL_SocketIsInvalid(s)) {
        OS_RestoreInterrupts(irq);
        return 0;
    }
    if (!SOCL_SocketIsCreated(s)) {
        OS_RestoreInterrupts(irq);
        return SOCL_ENETRESET;
    }
    if (s->state & SOCL_STATUS_WAIT_CLOSE) {
        OS_RestoreInterrupts(irq);
        return SOCL_EINPROGRESS;
    }
    OS_RestoreInterrupts(irq);
    s->state |= SOCL_STATUS_CLOSING | SOCL_STATUS_WAIT_CLOSE;
    if (SOCL_SocketIsTCP(s)) {
        SOCLi_SendCommandPacket(&s->send_pipe->base, NULL);
        SOCLiSocketCount[0]--;
    } else {
        SOCLiSocketCount[1]--;
    }
    packet = SOCLi_CreateCommandPacket(SOCLi_CloseCallBack, s, 1);
    packet->h.reply_queue = NULL;
    SOCLi_SendCommandPacketToCtrlPipe(s, packet);
    return 0;
}

static int SOCLi_CloseCallBack(void *arg) {
    SOCLiCommandClose *cpacket = (SOCLiCommandClose *)arg;
    SOCLSocket *socket = cpacket->h.socket;
    OSIntrMode irq;

    if (SOCL_SocketIsTCP(socket)) {
        OS_JoinThread(&socket->send_pipe->base.thread);
        CPS_TcpShutdown();
        CPS_TcpClose();
        CPS_SocRelease();
    }
    CPS_SocUnRegister();
    socket->state &= ~(SOCL_STATUS_CONNECTING | SOCL_STATUS_CONNECTED);
    SOCLi_SendCommandPacket(socket->flag == 2 ? &socket->send_pipe->base : &socket->recv_pipe->base, NULL);
    irq = OS_DisableInterrupts();
    SOCLi_SocketUnregister(socket);
    SOCLi_SocketRegisterTrash(socket);
    OS_RestoreInterrupts(irq);
    socket->state |= SOCL_STATUS_WAIT_RELEASE;
    return 0;
}

static void SOCLi_CleanupSocket(SOCLSocket *socket) {
    OSIntrMode irq;
    s8 flag;

    if (socket == NULL) {
        return;
    }
    socket->state = 0;
    flag = socket->flag;
    if (SOCLi_ModeIsTCP(flag)) {
        SOCLi_FreeCommandPipe(&socket->send_pipe->base);
        SOCLi_FreeCommandPipe(&socket->recv_pipe->base);
    } else if (flag == 1) {
        SOCLiSocketUdpData *udp = socket->recv_pipe->udp_head;

        while (udp != NULL) {
            SOCLiSocketUdpData *next = udp->next;

            SOCLiConfigPtr->free(udp);
            udp = next;
        }
        socket->recv_pipe->udp_queued = 0;
        socket->recv_pipe->udp_tail = NULL;
        socket->recv_pipe->udp_head = NULL;
        OS_WakeupThread(&socket->recv_pipe->udp_wait);
        SOCLi_FreeCommandPipe(&socket->recv_pipe->base);
    } else if (flag == 2) {
        SOCLi_FreeCommandPipe(&socket->send_pipe->base);
    }
    irq = OS_DisableInterrupts();
    SOCLi_SocketUnregister(socket);
    SOCLi_SocketUnregisterTrash(socket);
    SOCLiConfigPtr->free(socket);
    OS_RestoreInterrupts(irq);
}

// Waits for the pipe's thread to end, and answers the commands that it never ran with an error
static void SOCLi_FreeCommandPipe(SOCLiSocketCommandPipe *pipe) {
    OSMessage message;
    OSIntrMode irq;

    if (pipe == NULL) {
        return;
    }
    OS_JoinThread(&pipe->thread);
    irq = OS_DisableInterrupts();
    OS_DisableScheduler();
    if (OS_ReceiveMessage(&pipe->queue, &message, OS_MESSAGE_NOBLOCK)) {
        do {
            if (message != NULL) {
                if (((SOCLiCommandPacket *)message)->h.reply_queue != NULL) {
                    OS_SendMessage(((SOCLiCommandPacket *)message)->h.reply_queue, (OSMessage)SOCL_ECANCELED,
                                   OS_MESSAGE_NOBLOCK);
                }
                SOCLi_FreeCommandPacket(message);
            }
        } while (OS_ReceiveMessage(&pipe->queue, &message, OS_MESSAGE_NOBLOCK));
    }
    OS_EnableScheduler();
    OS_RescheduleThread();
    OS_RestoreInterrupts(irq);
}

void SOCLi_TrashSocket(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    while (SOCLiSocketListTrash != NULL) {
        SOCLi_CleanupSocket(SOCLiSocketListTrash);
    }
    OS_RestoreInterrupts(irq);
}
