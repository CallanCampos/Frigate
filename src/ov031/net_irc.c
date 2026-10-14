// Overlay 31, the second of its four files (see net_devirc.c): NetIrcSys, which makes the packets of the infrared
// device and takes turns with the other machine, over the link of frames in irc_link.c. It also holds the timeout and
// peer filter that the games set through net_devirc.h.
#include "field/net_irc.h"
#include "field/net_devirc.h"
#include "gfl/net_handle.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "irc/irc_link.h"
#include "nitro/wm.h"

// The most data one packet carries, and the size of a whole one
#define _IRC_SEND_SIZE 0xa6

// What a machine sends the other: a packet of the system, which the frames of the link carry
typedef struct {
    u16 crc;
    u8 cmd;
    // The turn this machine sends in the low four bits, the one it last got from the other in the high four
    u8 turn;
    // Both machines make a random ID. The packet has its own and the one it saw in the other's
    u32 senderId;
    u32 receiverId;
    u8 gameCode;
    u8 pad;
    u8 data[_IRC_SEND_SIZE];
} _NETIRC_DATA;

typedef struct {
    // Called with each packet that came in
    GFLNetRecvFunc recvCallback;
    // The packet that was last sent
    _NETIRC_DATA packet;
    // The ID of the machine this one is linking with, 0 until it is known
    u32 peerId;
    // How many frames the link may be down before it is lost (0 for no limit), and how many have passed since it went
    // down
    u32 timeout;
    u32 elapsed;
    // The last data sent, which is sent again when the other missed it
    const u8 *sendData;
    BOOL (*peerFilter)(int gameCode, int peerGameCode);
    u8 initialize;
    u8 unkCD;
    // Whether this machine may send now: the machines take turns
    u8 send_turn;
    u8 unkCF_0 : 1;
    u8 peerSeen : 1;
    u8 unkD0[0xc];
    u8 isParent;
    u8 running;
    u8 lastSize;
    u8 lastCmd;
    // The turn that was last sent, whether its answer is still to come, the turn to send next and the turn that was
    // last received
    u8 lastTurn;
    u8 sendPending;
    u8 sendTurn;
    u8 recvTurn;
    u8 resend;
    u8 timedOut;
    u8 tgid;
    u8 peerSession;
} NetIrcSysData;

static void NetIrc_SendPacket(const u8 *data, u8 size, u8 cmd, u8 turn);
static void NetIrc_OnFrame(u8 *data, u8 size, u8 cmd, u8 id);
static BOOL NetIrc_GetSenderNetId(void);
static void NetIrc_RefreshRole(void);
static u8 NetIrc_GetIsParent(void);

static NetIrcSysData NetIrcSys;

static void NetIrc_SendPacket(const u8 *data, u8 size, u8 cmd, u8 turn) {
    GFL_ASSERT(size < _IRC_SEND_SIZE);
    NetIrcSys.packet.cmd = cmd;
    NetIrcSys.packet.turn = turn;
    if (data != NULL) {
        sys_memcpy(data, NetIrcSys.packet.data, _IRC_SEND_SIZE);
    }
    NetIrcSys.packet.crc = getCRC16(&NetIrcSys.packet.cmd, size + 0xc);
    IrcLink_Send((u8 *)&NetIrcSys.packet, size + 0xe, cmd);
}

void NetIrc_Init(u32 timeout, u8 gameCode) {
    sys_memset(&NetIrcSys, 0, sizeof(NetIrcSys));
    NetIrcSys.timeout = timeout;
    NetIrcSys.tgid = func_020812b8();
    IrcLink_Reset(3);
    IrcLink_SetRecvCallback(NetIrc_OnFrame);
    NetIrcSys.packet.senderId = GFL_RandomLC(0);
    if (NetIrcSys.packet.senderId == 0) {
        NetIrcSys.packet.senderId = 1;
    }
    NetIrcSys.packet.gameCode = gameCode;
}

void NetDevIrc_SetTimeout(u32 frames) {
    NetIrcSys.timeout = frames;
}

void NetIrc_Exit(void) {
    if (NetIrcSys.initialize == TRUE) {
        IrcLink_Close();
    }
    NetIrcSys.initialize = FALSE;
}

void NetIrc_SetRecvCallback(GFLNetRecvFunc callback) {
    NetIrcSys.recvCallback = callback;
}

void NetDevIrc_SetPeerFilter(BOOL (*filter)(int gameCode, int peerGameCode)) {
    NetIrcSys.peerFilter = filter;
}

void NetIrc_SetInitialized(void) {
    GFL_ASSERT(NetIrcSys.initialize == FALSE);
    NetIrcSys.initialize = TRUE;
}

BOOL NetIrc_IsInitialized(void) {
    return NetIrcSys.initialize;
}

u8 NetIrc_GetSendTurn(void) {
    return NetIrcSys.send_turn;
}

u8 NetIrc_GetUnkCF0(void) {
    return NetIrcSys.unkCF_0;
}

BOOL NetIrc_CanSend(void) {
    if (NetIrcSys.send_turn == TRUE && !NetIrcSys.unkCF_0 && NetIrcSys.elapsed == 0 && !NetIrcSys.sendPending) {
        return TRUE;
    }
    return FALSE;
}

void NetIrc_Send(const u8 *data, u8 size, u8 cmd) {
    GFL_ASSERT(NetIrcSys.send_turn == TRUE);
    NetIrc_SendPacket(data, size, cmd, NetIrcSys.sendTurn | (NetIrcSys.recvTurn << 4));
    NetIrcSys.sendData = data;
    NetIrcSys.lastSize = size;
    NetIrcSys.lastCmd = cmd;
    NetIrcSys.lastTurn = NetIrcSys.sendTurn;
    NetIrcSys.sendPending = TRUE;
    NetIrcSys.send_turn = FALSE;
}

void NetIrc_SendPending(void) {
    NetIrcSysData *sys = &NetIrcSys;

    if (NetIrcSys.send_turn == TRUE) {
        if (sys->resend == TRUE) {
            NetIrc_SendPacket(sys->sendData, sys->lastSize, sys->lastCmd, sys->lastTurn | (NetIrcSys.recvTurn << 4));
            sys->resend = FALSE;
        } else {
            NetIrc_SendPacket(NULL, 0, IRC_CMD_EMPTY, sys->lastTurn | (NetIrcSys.recvTurn << 4));
        }
        sys->send_turn = FALSE;
    }
}

BOOL NetIrc_GetNetId(void) {
    if (NetIrcSys.isParent == TRUE) {
        return FALSE;
    }
    return TRUE;
}

static void NetIrc_OnFrame(u8 *data, u8 size, u8 cmd, u8 id) {
    _NETIRC_DATA *packet = (_NETIRC_DATA *)data;
    u8 zero[6] = { 0, 0, 0, 0, 0, 0 };
    int peerTurn = packet->turn & 0xf;
    int peerAck = packet->turn >> 4;
    BOOL senderNetId;

    NetIrcSys.send_turn = TRUE;
    GFL_ASSERT(size <= sizeof(_NETIRC_DATA));
    if (size == 0 || size > sizeof(_NETIRC_DATA)) {
        return;
    }
    if (packet->crc != getCRC16(&packet->cmd, size - 2)) {
        return;
    }
    if (NetIrcSys.peerFilter != NULL) {
        if (!NetIrcSys.peerFilter(NetIrcSys.packet.gameCode, packet->gameCode)) {
            return;
        }
    } else if (NetIrcSys.packet.gameCode != packet->gameCode) {
        return;
    }
    if (packet->receiverId == NetIrcSys.packet.senderId && packet->senderId == NetIrcSys.peerId) {
        if (!NetIrcSys.peerSeen) {
            NetIrc_RefreshRole();
            NetIrcSys.peerSeen = TRUE;
        }
    } else if (packet->receiverId == 0 && NetIrcSys.peerId == 0) {
        NetIrcSys.packet.receiverId = packet->senderId;
        NetIrcSys.peerId = packet->senderId;
        return;
    } else if (packet->receiverId == NetIrcSys.packet.senderId && NetIrcSys.peerId == 0) {
        NetIrcSys.packet.receiverId = packet->senderId;
        NetIrcSys.peerId = packet->senderId;
    } else {
        if (NetIrcSys.peerSeen) {
            return;
        }
        if (NetIrcSys.peerId == packet->senderId) {
            return;
        }
        NetIrcSys.peerId = 0;
        return;
    }
    if (cmd >= IRC_CMD_MIN) {
        return;
    }
    if (NetIrcSys.sendPending == TRUE && !NetIrcSys.resend) {
        if (peerAck == NetIrcSys.sendTurn) {
            if (NetIrcSys.sendTurn < 0xf) {
                NetIrcSys.sendTurn = NetIrcSys.sendTurn + 1;
            } else {
                NetIrcSys.sendTurn = 0;
            }
            NetIrcSys.sendPending = FALSE;
        } else {
            NetIrcSys.resend = TRUE;
        }
    }
    if (cmd == IRC_CMD_EMPTY) {
        return;
    }
    if (NetIrcSys.recvTurn == peerTurn) {
        return;
    }
    senderNetId = NetIrc_GetSenderNetId();
    if (NetIrcSys.recvCallback != NULL) {
        if (!NetIrcSys.recvCallback(senderNetId, packet->data, size - 0xe)) {
            return;
        }
    }
    NetIrcSys.recvTurn = peerTurn;
}

static BOOL NetIrc_GetSenderNetId(void) {
    if (NetIrc_GetIsParent() == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL NetIrc_IsError(void) {
    if (!NetIrcSys.initialize) {
        return FALSE;
    }
    if (NetIrcSys.timedOut) {
        return TRUE;
    }
    if (NetIrcSys.running == TRUE && !NetDevIrc_IsConnected()) {
        return TRUE;
    }
    return FALSE;
}

void NetIrc_StartLink(void) {
    NetIrcSys.isParent = IrcLink_IsParent();
    NetIrcSys.peerSession = IrcLink_GetPeerId();
    NetIrcSys.running = TRUE;
    NetIrcSys.sendTurn = 0;
    NetIrcSys.recvTurn = 0xe;
    if (NetIrcSys.isParent != TRUE) {
        NetIrcSys.send_turn = TRUE;
    }
}

static void NetIrc_RefreshRole(void) {
    NetIrcSys.isParent = IrcLink_IsParent();
    NetIrcSys.peerSession = IrcLink_GetPeerId();
}

static u8 NetIrc_GetIsParent(void) {
    return NetIrcSys.isParent;
}

BOOL NetDevIrc_IsConnected(void) {
    if (IrcLink_IsLinked() == TRUE) {
        return TRUE;
    }
    if (NetIrcSys.elapsed < NetIrcSys.timeout || (NetIrcSys.elapsed == 0 && NetIrcSys.timeout == 0)) {
        return TRUE;
    }
    return FALSE;
}

BOOL NetIrc_GetPeerSeen(void) {
    return NetIrcSys.peerSeen;
}

BOOL NetIrc_IsLinked(void) {
    return IrcLink_IsLinked();
}

void NetIrc_Update(void) {
    NetIrcSysData *sys = &NetIrcSys;

    if (!sys->running && sys->elapsed >= sys->timeout && sys->elapsed != 0) {
        return;
    }
    if (sys->elapsed == 0) {
        IrcLink_Poll();
    }
    if (sys->running == TRUE) {
        if (sys->elapsed != 0) {
            if (IrcLink_IsLinked() == TRUE) {
                sys->elapsed = 0;
                if (IrcLink_IsParent() == TRUE) {
                    sys->send_turn = FALSE;
                } else {
                    sys->send_turn = TRUE;
                }
                if (sys->sendPending == TRUE) {
                    sys->resend = TRUE;
                }
            } else if (sys->elapsed < 5) {
                sys->elapsed++;
            } else if (sys->elapsed == 5) {
                IrcLink_Reset(3);
                IrcLink_SetRecvCallback(NetIrc_OnFrame);
                sys->elapsed++;
            } else {
                IrcLink_Poll();
                if (sys->elapsed < sys->timeout) {
                    if (sys->isParent == TRUE && sys->elapsed % 5 == 0) {
                        IrcLink_Ping();
                    }
                    sys->elapsed++;
                } else {
                    sys->running = FALSE;
                    sys->timedOut = TRUE;
                    func_02040a9c(1 - NetIrc_GetNetId());
                    IrcLink_Close();
                }
            }
        } else if (!IrcLink_IsLinked()) {
            if (sys->timeout != 0) {
                IrcLink_Close();
                sys->elapsed = 1;
            } else {
                sys->running = FALSE;
                sys->timedOut = TRUE;
                sys->elapsed = 0xffff;
                func_02040a9c(1 - NetIrc_GetNetId());
                IrcLink_Close();
            }
        }
    }
}
