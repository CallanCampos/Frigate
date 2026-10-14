#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL_EnableSsl: turns SSL on for a TCP socket that has not been bound, or off (with a NULL connection). It has the
// receive pipe's thread, which owns CPS's socket, switch CPS's SSL layer (SOCLi_EnableSslCallBack). pokediamond's
// NitroWiFi names the object socl_ssl.o and the three functions.

static int SOCLi_ExecEnableSslCommand(SOCLSocket *socket, void *ssl);
static int SOCLi_EnableSslCallBack(SOCLCommandPacket *packet);

int SOCL_EnableSsl(int sock, void *sslInfo) {
    SOCLSocket *socket = (SOCLSocket *)sock;

    if (SOCL_SocketIsInvalid(socket)) {
        return SOCL_EINVAL;
    }
    if (!SOCLi_ModeIsTCP(socket->flag)) {
        return SOCL_EINVAL;
    }
    if (!SOCLi_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (socket->state & SOCLI_STATE_BINDING) {
        return SOCL_EINVAL;
    }
    return SOCLi_ExecEnableSslCommand(socket, sslInfo);
}

static int SOCLi_ExecEnableSslCommand(SOCLSocket *socket, void *ssl) {
    SOCLCommandPacket *packet = SOCLi_CreateCommandPacket(SOCLi_EnableSslCallBack, socket, TRUE);

    if (packet == NULL) {
        return SOCL_EMFILE;
    }
    packet->args.ssl.info = ssl;
    return SOCLi_ExecCommandPacketInRecvPipe(socket, packet);
}

static int SOCLi_EnableSslCallBack(SOCLCommandPacket *packet) {
    SOCLSocket *socket = packet->socket;
    SOCLRecvPipe *pipe = SOCLi_GetRecvPipe(socket);
    CPSSoc *soc = (CPSSoc *)pipe->base.thread.specific[0];

    OS_LockMutex(&pipe->base.mutex);
    if (packet->args.ssl.info != NULL) {
        soc->con = packet->args.ssl.info;
        func_ov011_0215a5e0(TRUE);
        socket->flag = 4;
    } else {
        socket->flag = 0;
        func_ov011_0215a5e0(FALSE);
        soc->con = NULL;
    }
    OS_UnlockMutex(&pipe->base.mutex);
    return 0;
}
