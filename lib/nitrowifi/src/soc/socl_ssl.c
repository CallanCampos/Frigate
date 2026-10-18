#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL_EnableSsl switches SSL on/off for a created TCP-mode socket while its BINDING state bit is clear. The
// receive pipe's thread, which owns CPS's socket, installs the borrowed connection pointer or detaches it for NULL
// (SOCLi_EnableSslCallBack); this does not free/reset the old context. pokediamond's NitroWiFi names the object
// socl_ssl.o and the three functions.

static int SOCLi_ExecEnableSslCommand(SOCLSocket *socket, void *ssl);
static int SOCLi_EnableSslCallBack(void *arg);

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
    packet->enable_ssl.info = ssl;
    return SOCLi_ExecCommandPacketInRecvPipe(socket, packet);
}

static int SOCLi_EnableSslCallBack(void *arg) {
    SOCLiCommandEnableSsl *cpacket = (SOCLiCommandEnableSsl *)arg;
    SOCLSocket *socket = cpacket->h.socket;
    SOCLRecvPipe *pipe = socket->recv_pipe;
    CPSSoc *soc = (CPSSoc *)pipe->base.thread.specific[0];

    OS_LockMutex(&pipe->base.mutex);
    if (cpacket->info != NULL) {
        soc->con = cpacket->info;
        CPS_SetSsl(TRUE);
        socket->flag = 4;
    } else {
        socket->flag = 0;
        CPS_SetSsl(FALSE);
        soc->con = NULL;
    }
    OS_UnlockMutex(&pipe->base.mutex);
    return 0;
}
