#include "nitro/os.h"
#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCL's bind and connect: SOCL_Bind and SOCL_Connect set the socket's ports and address, and for a socket that
// blocks they have its receive pipe's thread bind it in CPS, and for TCP connect it (SOCLi_ExecBindCommand and
// SOCLi_BindCallBack). pokediamond's NitroWiFi names the object socl_bind.o and every function in it (SOCL_Bind and
// SOCL_Connect; its other calls from here, such as SOCL_Listen, this version lacks).

static int SOCLi_ExecBindCommand(SOCLSocket *socket);
static int SOCLi_BindCallBack(void *arg);

// What SOCL_Connect returns for a connect that is still running and has no error yet
static int SOCLiResultCodeInConnecting = SOCL_EINPROGRESS;

// Whether an address is a broadcast one: the all-ones address, or the network's with every host bit set
static inline BOOL SOCLi_IsBroadcast(CPSInAddr ip) {
    if (((CPSMyIp ^ ip) & CPSNetMask) != 0) {
        return ip == 0xffffffff;
    } else {
        u32 host = ~CPSNetMask;

        return host == (ip & host);
    }
}

int SOCL_Bind(int s, u16 local_port) {
    SOCLSocket *socket = (SOCLSocket *)s;

    if (SOCL_SocketIsInvalid(socket)) {
        return SOCL_EINVAL;
    }
    if (!SOCLi_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (socket->state & SOCLI_STATE_BINDING) {
        return SOCL_EALREADY;
    }
    socket->local_port = local_port;
    if (socket->flag == 1) {
        return SOCLi_ExecBindCommand(socket);
    }
    return 0;
}

int SOCL_Connect(int s, u16 remote_port, CPSInAddr remote_ip) {
    SOCLSocket *socket = (SOCLSocket *)s;
    int result;

    if (SOCL_SocketIsInvalid(socket) || (socket->state & SOCLI_STATE_CLOSING)) {
        return SOCL_EINVAL;
    }
    if (!SOCLi_SocketIsCreated(socket)) {
        return SOCL_ENETRESET;
    }
    if (SOCLi_SocketIsTCP(socket)) {
        if ((remote_ip & 0xf0000000) == 0xe0000000 || SOCLi_IsBroadcast(remote_ip)) {
            return SOCL_EINVAL;
        }
        if (socket->state & SOCLI_STATE_CONNECTED) {
            if (socket->flag_block == 1) {
                return SOCL_EISCONN;
            }
            return 0;
        }
        if (socket->state & SOCLI_STATE_BINDING) {
            if (socket->state & SOCLI_STATE_CONNECT_FAILED) {
                return socket->result;
            }
            return SOCLiResultCodeInConnecting;
        }
        socket->remote_port = remote_port;
        socket->remote_ip = remote_ip;
        result = SOCLi_ExecBindCommand(socket);
        return socket->flag_block == 1 || result < 0 ? result : SOCL_EINPROGRESS;
    }
    socket->remote_port = remote_port;
    socket->remote_ip = remote_ip;
    return 0;
}

static int SOCLi_ExecBindCommand(SOCLSocket *socket) {
    SOCLCommandPacket *packet = SOCLi_CreateCommandPacket(SOCLi_BindCallBack, socket, socket->flag_block);

    if (packet == NULL) {
        return SOCL_EMFILE;
    }
    packet->bind.local_port = socket->local_port;
    packet->bind.remote_port = socket->remote_port;
    packet->bind.remote_ip = socket->remote_ip;
    socket->state |= SOCLI_STATE_BINDING;
    return SOCLi_ExecCommandPacketInRecvPipe(socket, packet);
}

static int SOCLi_BindCallBack(void *arg) {
    SOCLiCommandBind *cpacket = (SOCLiCommandBind *)arg;
    SOCLSocket *socket = cpacket->h.socket;
    int result = 0;
    SOCLRecvPipe *pipe = socket->recv_pipe;

    OS_LockMutex(&pipe->base.mutex);
    CPS_SocBind(cpacket->local_port, cpacket->remote_port, cpacket->remote_ip);
    pipe->cps_read_len = 0;
    if (cpacket->h.socket_flag == 0 || cpacket->h.socket_flag == 4) {
        result = CPS_TcpConnect();
    }
    OS_UnlockMutex(&pipe->base.mutex);
    if (result != 0) {
        socket->state |= SOCLI_STATE_CONNECT_FAILED;
        return SOCL_ETIMEDOUT;
    }
    socket->state |= SOCLI_STATE_CONNECTED;
    return 0;
}
