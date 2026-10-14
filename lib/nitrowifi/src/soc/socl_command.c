#include "nitrowifi/soc.h"
#include "socl_internal.h"
#include "nitro/os.h"

// SOCL's command packets: the socket calls do not run in the caller's thread but as packets that a command pipe's
// thread takes from its queue and runs, one at a time. This file keeps the pool of packets (a queue of the free ones)
// and sends the packets to the pipes. pokediamond's NitroWiFi names the object socl_command.o and every function in it.

static int SOCLi_ExecCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet);

static SOCLCommandPacket *SOCLiCommandPackets;
static OSMessageQueue SOCLiCommandPacketQueue;

int SOCLi_StartupCommandPacketQueue(u32 max) {
    int packets_offset = (max * sizeof(OSMessage) + 3) & ~3;
    int packets_size = (max * sizeof(SOCLCommandPacket) + 3) & ~3;
    SOCLCommandPacket *packets = (SOCLCommandPacket *)SOCLiConfigPtr->alloc(packets_size + packets_offset);
    u8 *packet;

    if (packets == NULL) {
        return -1;
    }
    func_0207acc4(&SOCLiCommandPacketQueue, (OSMessage *)packets, max);
    packet = (u8 *)packets + packets_offset;
    while ((s32)max > 0) {
        SOCLi_FreeCommandPacket((SOCLCommandPacket *)packet);
        max--;
        packet += sizeof(SOCLCommandPacket);
    }
    SOCLiCommandPackets = packets;
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
        packet->handler = handler;
        packet->socket = socket;
        packet->reply_queue = NULL;
        packet->socket_flag = socket->flag;
        packet->flag_block = flag_block;
    }
    return packet;
}

void SOCLi_FreeCommandPacket(SOCLCommandPacket *packet) {
    if (packet != NULL) {
        OS_SendMessage(&SOCLiCommandPacketQueue, packet, OS_MESSAGE_NOBLOCK);
    }
}

SOCLCommandPipe *SOCLi_GetCtrlPipe(SOCLSocket *socket) {
    SOCLCommandPipe *pipe = socket->recv_pipe;

    if (pipe == NULL) {
        pipe = socket->send_pipe;
    }
    return pipe;
}

int SOCLi_SendCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet) {
    BOOL sent = OS_SendMessage(&pipe->queue, packet, packet == NULL || (packet->flag_block & 1) ? OS_MESSAGE_BLOCK : OS_MESSAGE_NOBLOCK);

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

    if (packet->flag_block == 1) {
        func_0207acc4(&queue, message, 1);
        packet->reply_queue = &queue;
        SOCLi_SendCommandPacket(pipe, packet);
        OS_ReceiveMessage(&queue, &result, OS_MESSAGE_BLOCK);
    } else {
        packet->reply_queue = NULL;
        result = (OSMessage)SOCLi_SendCommandPacket(pipe, packet);
    }
    return (int)result;
}

int SOCLi_ExecCommandPacketInRecvPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_ExecCommandPacket(socket->recv_pipe, packet);
}

int SOCLi_ExecCommandPacketInSendPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_ExecCommandPacket(socket->send_pipe, packet);
}

int SOCLi_ExecCommandPacketInCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet) {
    return SOCLi_ExecCommandPacket(SOCLi_GetCtrlPipe(socket), packet);
}

void SOCLi_CommandPacketHandler(OSMessageQueue *queue) {
    SOCLCommandPacket *packet;

    while (1) {
        int result;
        u32 irq;

        OS_ReadMessage(queue, (OSMessage *)&packet, OS_MESSAGE_BLOCK);
        if (packet == NULL) {
            return;
        }
        result = packet->handler(packet);
        irq = CPU_IRQDisable();
        func_0207aab4();
        OS_ReceiveMessage(queue, NULL, OS_MESSAGE_NOBLOCK);
        if (packet->socket != NULL) {
            packet->socket->result = result;
        }
        if (packet->reply_queue != NULL) {
            OS_SendMessage(packet->reply_queue, (OSMessage)result, OS_MESSAGE_NOBLOCK);
        }
        SOCLi_FreeCommandPacket(packet);
        func_0207aad8();
        CPU_SetIRQMask(irq);
    }
}
