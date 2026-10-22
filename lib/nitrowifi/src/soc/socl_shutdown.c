#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL_Shutdown: ends the sending half of a TCP connection. It marks the socket closing and has the send pipe's
// thread shut the connection down in CPS (SOCLi_ShutdownCallBack). pokediamond's NitroWiFi names the object
// socl_shutdown.o and both functions.

static int SOCLi_ShutdownCallBack(void *arg);

int SOCL_Shutdown(int s) {
    SOCLSocket *socket = (SOCLSocket *)s;
    SOCLiSocketSendCommandPipe *pipe;
    SOCLiCommandPacket *packet;
    int result;

    if (SOCL_SocketIsInvalid(socket)) {
        return SOCL_EINVAL;
    }
    if (!SOCL_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (!(socket->state & SOCL_STATUS_CONNECTED) || (socket->state & SOCL_STATUS_CLOSING)) {
        return SOCL_ENOTCONN;
    }
    socket->state |= SOCL_STATUS_CLOSING;
    pipe = socket->send_pipe;
    if (pipe != NULL && pipe->socket != NULL) {
        packet = SOCLi_CreateCommandPacket(SOCLi_ShutdownCallBack, pipe->socket, socket->flag_block);
        if (packet == NULL) {
            return SOCL_EMFILE;
        }
        result = SOCLi_ExecCommandPacketInSendPipe(pipe->socket, packet);
    } else {
        result = 0;
    }
    return result;
}

static int SOCLi_ShutdownCallBack(void *arg) {
    SOCLiCommandShutdown *cpacket = (SOCLiCommandShutdown *)arg;
    SOCLSocket *socket = cpacket->h.socket;

    if (SOCL_SocketIsTCP(socket)) {
        CPS_TcpShutdown();
    }
    return 0;
}
