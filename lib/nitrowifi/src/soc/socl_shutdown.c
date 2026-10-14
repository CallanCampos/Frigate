#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL_Shutdown: ends the sending half of a TCP connection. It marks the socket closing and has the send pipe's
// thread shut the connection down in CPS (SOCLi_ShutdownCallBack). pokediamond's NitroWiFi names the object
// socl_shutdown.o and both functions.

static int SOCLi_ShutdownCallBack(SOCLCommandPacket *packet);

int SOCL_Shutdown(SOCLSocket *socket) {
    SOCLSendPipe *pipe;
    SOCLCommandPacket *packet;

    if (SOCL_SocketIsInvalid(socket)) {
        return SOCL_EINVAL;
    }
    if (!SOCLi_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (!(socket->state & SOCLI_STATE_CONNECTED) || (socket->state & SOCLI_STATE_CLOSING)) {
        return SOCL_ENOTCONN;
    }
    socket->state |= SOCLI_STATE_CLOSING;
    pipe = SOCLi_GetSendPipe(socket);
    if (pipe != NULL && pipe->socket != NULL) {
        packet = SOCLi_CreateCommandPacket(SOCLi_ShutdownCallBack, pipe->socket, socket->flag_block);
        if (packet != NULL) {
            return SOCLi_ExecCommandPacketInSendPipe(pipe->socket, packet);
        }
        return SOCL_EMFILE;
    }
    return 0;
}

static int SOCLi_ShutdownCallBack(SOCLCommandPacket *packet) {
    SOCLSocket *socket = packet->socket;

    if (SOCLi_SocketIsTCP(socket)) {
        CPS_TcpShutdown();
    }
    return 0;
}
