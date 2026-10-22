#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's command packets: the socket calls do not run in the caller's thread but as packets that a command pipe's
// thread takes from its queue and runs, one at a time. This file keeps the pool of packets (a queue of the free ones)
// and sends the packets to the pipes. pokediamond's NitroWiFi names the object socl_command.o and every function in it.

static int SOCLi_ExecCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet);

// With their explicit zeros the code reaches both from one base, as the original does
static OSMessage *SOCLiCommandPackets = NULL;
static OSMessageQueue SOCLiCommandPacketQueue = { 0 };

int SOCLi_StartupCommandPacketQueue(s32 max) {
    int packets_offset = (max * sizeof(OSMessage) + 3) & ~3;
    int packets_size = (max * sizeof(SOCLCommandPacket) + 3) & ~3;
    OSMessage *messages = (OSMessage *)SOCLiConfigPtr->alloc(packets_size + packets_offset);
    u8 *packet;

    if (messages == NULL) {
        return -1;
    }
    OS_InitMessageQueue(&SOCLiCommandPacketQueue, messages, max);
    packet = (u8 *)messages + packets_offset;
    while (max > 0) {
        SOCLi_FreeCommandPacket((SOCLCommandPacket *)packet);
        max--;
        packet += sizeof(SOCLCommandPacket);
    }
    SOCLiCommandPackets = messages;
    return 0;
}

int SOCLi_CleanupCommandPacketQueue(void) {
    if (SOCLiCommandPacketQueue.usedCount < SOCLiCommandPacketQueue.msgCount) {
        return -1;
    }
    SOCLiConfigPtr->free(SOCLiCommandPackets);
    SOCLiCommandPackets = NULL;
    return 0;
}

SOCLCommandPacket *SOCLi_AllocCommandPacket(s32 flags) {
    SOCLCommandPacket *packet;

    if (OS_ReceiveMessage(&SOCLiCommandPacketQueue, (OSMessage *)&packet, flags)) {
        return packet;
    }
    return NULL;
}

SOCLCommandPacket *SOCLi_CreateCommandPacket(SOCLCommandHandler handler, SOCLSocket *socket, s32 flag_block) {
    SOCLCommandPacket *packet = SOCLi_AllocCommandPacket(flag_block);

    if (packet != NULL) {
        packet->h.handler = handler;
        packet->h.socket = socket;
        packet->h.reply_queue = NULL;
        packet->h.socket_flag = socket->flag;
        packet->h.flag_block = flag_block;
    }
    return packet;
}

void SOCLi_FreeCommandPacket(SOCLCommandPacket *packet) {
    if (packet != NULL) {
        OS_SendMessage(&SOCLiCommandPacketQueue, packet, OS_MESSAGE_NOBLOCK);
    }
}

SOCLCommandPipe *SOCLi_GetCtrlPipe(SOCLSocket *socket) {
    return socket->recv_pipe != NULL ? &socket->recv_pipe->base : &socket->send_pipe->base;
}

int SOCLi_SendCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet) {
    BOOL sent = OS_SendMessage(&pipe->queue, packet,
                               packet == NULL || (packet->h.flag_block & 1) ? OS_MESSAGE_BLOCK : OS_MESSAGE_NOBLOCK);

    if (!sent) {
        SOCLi_FreeCommandPacket(packet);
    }
    return sent ? 0 : -42;
}

int SOCLi_SendCommandPacketToCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_SendCommandPacket(SOCLi_GetCtrlPipe(socket), packet);
}

static int SOCLi_ExecCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet) {
    OSMessage result;
    OSMessage message[1];
    OSMessageQueue queue;

    if (packet->h.flag_block == 1) {
        OS_InitMessageQueue(&queue, message, 1);
        packet->h.reply_queue = &queue;
        SOCLi_SendCommandPacket(pipe, packet);
        OS_ReceiveMessage(&queue, &result, OS_MESSAGE_BLOCK);
    } else {
        packet->h.reply_queue = NULL;
        result = (OSMessage)SOCLi_SendCommandPacket(pipe, packet);
    }
    return (int)result;
}

int SOCLi_ExecCommandPacketInRecvPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_ExecCommandPacket(&socket->recv_pipe->base, packet);
}

int SOCLi_ExecCommandPacketInSendPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_ExecCommandPacket(&socket->send_pipe->base, packet);
}

int SOCLi_ExecCommandPacketInCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_ExecCommandPacket(SOCLi_GetCtrlPipe(socket), packet);
}

void SOCLi_CommandPacketHandler(void *arg) {
    SOCLCommandPipe *pipe = (SOCLCommandPipe *)arg;
    SOCLCommandPacket *packet;

    while (1) {
        int result;
        u32 irq;

        OS_ReadMessage(&pipe->queue, (OSMessage *)&packet, OS_MESSAGE_BLOCK);
        if (packet == NULL) {
            return;
        }
        result = packet->h.handler(packet);
        irq = OS_DisableInterrupts();
        OS_DisableScheduler();
        OS_ReceiveMessage(&pipe->queue, NULL, OS_MESSAGE_NOBLOCK);
        if (packet->h.socket != NULL) {
            packet->h.socket->result = result;
        }
        if (packet->h.reply_queue != NULL) {
            OS_SendMessage(packet->h.reply_queue, (OSMessage)result, OS_MESSAGE_NOBLOCK);
        }
        SOCLi_FreeCommandPacket(packet);
        OS_EnableScheduler();
        OS_RestoreInterrupts(irq);
    }
}
