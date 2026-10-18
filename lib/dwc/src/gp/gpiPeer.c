#include "dwc/gpiPeer.h"
#include "dwc/darray.h"
#include "dwc/ext.h"
#include "dwc/gpiBuddy.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiInfo.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiTransfer.h"
#include "dwc/gpiUtility.h"
#include "dwc/gt2.h"
#include "dwc/md5.h"
#include "nitrowifi/soc.h"

// GameSpy GP's peers (gpiPeer.c). The functions' names, from pret/pokerevo's Wii build and GameSpy's later SDK, are
// in dwc/gp.h. A peer that we connect to gets the signature of its profile from the server, connects through the Udp
// Engine and sends "\auth\" with it; the other side checks it against our password and answers "\aack\" or "\anack\".
// Once connected, the peers send each other messages of the form "\m\<type>\len\<len>\msg\" and the text

static int gpiProcessPeerInitiatingConnection(GPConnection *connection, GPIPeer *peer);
static int gpiProcessPeerAcceptingConnection(GPConnection *connection, GPIPeer *peer);
static int gpiPeerSendMessages(GPConnection *connection, GPIPeer *peer);
static int gpiProcessPeerConnected(GPConnection *connection, GPIPeer *peer);
static void gpiCheckTimedOutPeerOperations(GPConnection *connection, GPIPeer *peer);
static int gpiProcessPeer(GPConnection *connection, GPIPeer *peer);
static void gpiRemovePeer(GPConnection *connection, GPIPeer *peer);
static GPIPeer *gpiGetPeerByAddr(const GPConnection *connection, u32 ip, u16 port);
static void gpiFreeMessage(void *element);

static int gpiProcessPeerInitiatingConnection(GPConnection *connection, GPIPeer *peer) {
    GPIConnection *iconnection = *connection;
    char *footer = NULL;
    BOOL connClosed;
    int result;
    BOOL freeSig;
    GPIProfile *profile;
    int state;
    GPIPeer *other;
    char *message;

    if (peer == NULL) {
        return 3;
    }
    if (peer->state == GPI_PEER_DISCONNECTED || peer->state == GPI_PEER_NOT_CONNECTED) {
        return 3;
    }
    switch (peer->state) {
    case GPI_PEER_GETTING_SIG:
        break;
    case GPI_PEER_GOT_SIG:
        func_ov011_0216cca8(1, 3, 0x1f, "Got the peer signature for profileid: %d\n", peer->profile);
        result = gpiPeerStartConnect(connection, peer);
        if (result != 0) {
            return result;
        }
        break;
    case GPI_PEER_CONNECTING:
        Gt2_GetConnectionState(peer->ip, peer->port, &state);
        if (state == 1) {
            freeSig = TRUE;
            if (!gpiGetProfile(connection, peer->profile, &profile)) {
                gpiSetErrorString(connection, "Error connecting to a peer.");
                return 3;
            }
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\auth\\");
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\pid\\");
            gpiAppendIntToBuffer(connection, &peer->outputBuffer, iconnection->profileid);
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\nick\\");
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, iconnection->nick);
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\sig\\");
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, profile->peerSig);
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\final\\");
            // The signature stays while another peer of the profile still needs it
            for (other = iconnection->peerList; other != NULL; other = other->pnext) {
                if (other->profile == peer->profile && other != peer && other->state <= GPI_PEER_CONNECTING) {
                    freeSig = FALSE;
                }
            }
            if (freeSig) {
                func_ov011_0216ddfc(profile->peerSig);
                profile->peerSig = NULL;
                if (gpiCanFreeProfile(profile)) {
                    gpiRemoveProfile(connection, profile);
                }
            }
            peer->state = GPI_PEER_WAITING;
        }
        break;
    case GPI_PEER_WAITING:
        if (peer->inputBuffer.buffer != NULL) {
            footer = strstr(peer->inputBuffer.buffer, "\\final\\");
        }
        if (footer != NULL) {
            *footer = '\0';
            message = peer->inputBuffer.buffer;
            if (strncmp(message, "\\anack\\", 7) == 0) {
                // A refused signature is asked for again, once
                if (++peer->nackCount > 1) {
                    gpiSetErrorString(connection, "Error getting buddy authorization.");
                    return 3;
                }
                result = gpiPeerGetSig(connection, peer);
                if (result != 0) {
                    return result;
                }
            } else if (strncmp(message, "\\aack\\", 6) != 0) {
                gpiSetErrorString(connection, "Error parsing buddy message.");
                return 3;
            }
            peer->state = GPI_PEER_CONNECTED;
            peer->inputBuffer.len = 0;
        }
        break;
    }
    if (peer->outputBuffer.len > 0) {
        result = gpiSendBufferToPeer(connection, peer->ip, peer->port, &peer->outputBuffer, &connClosed, TRUE);
        if (connClosed || result != 0) {
            peer->state = GPI_PEER_DISCONNECTED;
        }
    }
    return 0;
}

static int gpiProcessPeerAcceptingConnection(GPConnection *connection, GPIPeer *peer) {
    GPIConnection *iconnection = *connection;
    char *footer;
    char *message;
    int state;
    BOOL connClosed;
    int pid;
    char intValue[16];
    char nick[31];
    char sig[33];
    char sigCheck[33];
    char buffer[259];

    if (peer->state != GPI_PEER_WAITING) {
        return 3;
    }
    Gt2_GetConnectionState(peer->ip, peer->port, &state);
    if (state == 3) {
        peer->state = GPI_PEER_DISCONNECTED;
        return 0;
    }
    footer = strstr(peer->inputBuffer.buffer, "\\final\\");
    if (footer != NULL) {
        *footer = '\0';
        message = peer->inputBuffer.buffer;
        if (strncmp(message, "\\auth\\", 6) == 0) {
            if (!gpiValueForKey(message, "\\pid\\", intValue, 16)) {
                peer->state = GPI_PEER_DISCONNECTED;
                return 0;
            }
            pid = atol(intValue);
            if (!gpiValueForKey(peer->inputBuffer.buffer, "\\nick\\", nick, 31)) {
                peer->state = GPI_PEER_DISCONNECTED;
                return 0;
            }
            if (!gpiValueForKey(peer->inputBuffer.buffer, "\\sig\\", sig, 33)) {
                peer->state = GPI_PEER_DISCONNECTED;
                return 0;
            }
            // The signature is the digest of our password with our profile and the peer's
            OS_SPrintf(buffer, "%s%d%d", iconnection->password, iconnection->profileid, pid);
            func_ov011_021723e8((const u8 *)buffer, strlen(buffer), sigCheck);
            if (strcmp(sig, sigCheck) != 0) {
                gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\anack\\");
                gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\final\\");
                gpiSendBufferToPeer(connection, peer->ip, peer->port, &peer->outputBuffer, &connClosed, TRUE);
                peer->state = GPI_PEER_DISCONNECTED;
                return 0;
            }
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\aack\\");
            gpiAppendStringToBuffer(connection, &peer->outputBuffer, "\\final\\");
            peer->state = GPI_PEER_CONNECTED;
            peer->profile = pid;
        } else {
            peer->state = GPI_PEER_DISCONNECTED;
            return 0;
        }
        peer->inputBuffer.len = 0;
    }
    return 0;
}

static int gpiPeerSendMessages(GPConnection *connection, GPIPeer *peer) {
    GPIMessage *message;
    BOOL connClosed;
    int result;

    if (peer == NULL) {
        return 3;
    }
    if (peer->outputBuffer.len != 0) {
        return 0;
    }
    while (func_ov011_02171588(peer->messages) != 0) {
        message = func_ov011_0217158c(peer->messages, 0);
        result = gpiSendBufferToPeer(connection, peer->ip, peer->port, &message->buffer, &connClosed, FALSE);
        if (connClosed || result != 0) {
            peer->state = GPI_PEER_DISCONNECTED;
            return 0;
        }
        // A message that the Udp Engine took only part of goes on next time
        if (message->buffer.pos != message->buffer.len) {
            break;
        }
        func_ov011_02171668(peer->messages, 0);
    }
    return 0;
}

static int gpiProcessPeerConnected(GPConnection *connection, GPIPeer *peer) {
    GPIConnection *iconnection = *connection;
    GPRecvBuddyMessageArg *messageArg;
    GPRecvBuddyUTMArg *utmArg;
    int result;
    int state;
    BOOL connClosed;
    GPICallback callback;
    char *message;
    int type;
    int len;

    if (peer == NULL) {
        return 3;
    }
    if (peer->outputBuffer.len != 0) {
        result = gpiSendBufferToPeer(connection, peer->ip, peer->port, &peer->outputBuffer, &connClosed, TRUE);
        if (connClosed || result != 0) {
            peer->state = GPI_PEER_DISCONNECTED;
            return 0;
        }
    }
    if (peer->outputBuffer.len == 0) {
        result = gpiPeerSendMessages(connection, peer);
        if (result != 0) {
            return result;
        }
        if (peer->state == GPI_PEER_DISCONNECTED) {
            return 0;
        }
    }
    if (peer->inputBuffer.len > 0) {
        peer->timeout = func_ov011_021714a8(NULL) + 10000;
    }
    while (1) {
        result = gpiReadMessageFromBuffer(connection, &peer->inputBuffer, &message, &type, &len);
        if (result != 0) {
            return result;
        }
        if (message != NULL) {
            switch (type) {
            case GPI_BM_MESSAGE:
                callback = iconnection->callbacks[3];
                if (callback.callback != NULL) {
                    messageArg = func_ov011_0216dddc(sizeof(GPRecvBuddyMessageArg));
                    if (messageArg == NULL) {
                        gpiSetErrorString(connection, "Out of memory.");
                        return 1;
                    }
                    messageArg->profile = peer->profile;
                    messageArg->message = func_ov011_0216e2c8(message);
                    messageArg->date = func_ov011_021714a8(NULL);
                    result = gpiAddCallback(connection, callback, messageArg, NULL, GPI_ADD_MESSAGE);
                    if (result != 0) {
                        return result;
                    }
                }
                break;
            case GPI_BM_UTM:
                callback = iconnection->callbacks[4];
                if (callback.callback != NULL) {
                    utmArg = func_ov011_0216dddc(sizeof(GPRecvBuddyUTMArg));
                    if (utmArg == NULL) {
                        gpiSetErrorString(connection, "Out of memory.");
                        return 1;
                    }
                    utmArg->profile = peer->profile;
                    utmArg->message = func_ov011_0216e2c8(message);
                    utmArg->date = func_ov011_021714a8(NULL);
                    result = gpiAddCallback(connection, callback, utmArg, NULL, GPI_ADD_MESSAGE);
                    if (result != 0) {
                        return result;
                    }
                }
                break;
            case GPI_BM_PING:
                gpiSendBuddyMessage(connection, peer->profile, GPI_BM_PONG, "1", 0, NULL);
                break;
            case GPI_BM_KEYS_REQUEST:
                result = gpiBuddyHandleKeyRequest(connection, peer);
                if (result != 0) {
                    return result;
                }
                break;
            case GPI_BM_KEYS_REPLY:
                result = gpiBuddyHandleKeyReply(connection, peer, message);
                if (result != 0) {
                    return result;
                }
                break;
            case GPI_BM_FILE_SEND_REQUEST:
            case GPI_BM_FILE_SEND_REPLY:
            case 202:
            case 203:
            case 204:
            case 205:
            case 206:
            case 207:
            case 208:
                gpiHandleTransferMessage(connection, peer, type, peer->inputBuffer.buffer, message, len);
                break;
            }
            gpiClipBufferToPosition(connection, &peer->inputBuffer);
        }
        if (message == NULL) {
            Gt2_GetConnectionState(peer->ip, peer->port, &state);
            if (state == 3) {
                peer->state = GPI_PEER_DISCONNECTED;
            }
            return 0;
        }
    }
}

static void gpiCheckTimedOutPeerOperations(GPConnection *connection, GPIPeer *peer) {
    GPIPeerOp *op = peer->peerOpQueue.opList;
    GPICallback callback;
    GPGetBuddyStatusInfoKeysArg *arg;

    if (peer != NULL) {
        for (; op != NULL && op != peer->peerOpQueue.last; op = op->next) {
            if (op->type != 2 && func_ov011_0216e324() > op->timeout && op->callback != NULL) {
                // A request for the keys of the buddy's status gets an answer with none
                if (op->state == GPI_BM_KEYS_REQUEST) {
                    arg = func_ov011_0216dddc(sizeof(GPGetBuddyStatusInfoKeysArg));
                    callback.callback = op->callback;
                    callback.param = op->userData;
                    arg->keys = NULL;
                    arg->numKeys = 0;
                    arg->values = NULL;
                    arg->profile = peer->profile;
                    gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_NORMAL);
                }
                func_ov011_0216cca8(1, 4, 8, "Peer operation timed out");
                gpiPeerRemoveOp(peer, op);
            }
        }
    }
}

static int gpiProcessPeer(GPConnection *connection, GPIPeer *peer) {
    int result = 0;

    if (peer->state == GPI_PEER_NOT_CONNECTED) {
        return 3;
    }
    if (peer->state != GPI_PEER_CONNECTED) {
        if (peer->initiated) {
            result = gpiProcessPeerInitiatingConnection(connection, peer);
        } else {
            result = gpiProcessPeerAcceptingConnection(connection, peer);
        }
    }
    if (result == 0 && peer->state == GPI_PEER_CONNECTED) {
        result = gpiProcessPeerConnected(connection, peer);
        gpiCheckTimedOutPeerOperations(connection, peer);
    }
    return result;
}

void gpiDestroyPeer(GPConnection *connection, GPIPeer *peer) {
    func_ov011_0216ddfc(peer->inputBuffer.buffer);
    peer->inputBuffer.buffer = NULL;
    func_ov011_0216ddfc(peer->outputBuffer.buffer);
    peer->outputBuffer.buffer = NULL;
    if (peer->messages != NULL) {
        func_ov011_02171558(peer->messages);
        peer->messages = NULL;
    }
    func_ov011_0216ddfc(peer);
}

static void gpiRemovePeer(GPConnection *connection, GPIPeer *peer) {
    GPIConnection *iconnection = *connection;
    GPIPeer *previous;
    GPIPeer *current;
    GPIMessage *message;

    if (peer != NULL && iconnection->peerList != NULL) {
        previous = iconnection->peerList;
        if (previous == peer) {
            iconnection->peerList = peer->pnext;
        } else {
            for (current = previous->pnext; current != peer; current = current->pnext) {
                if (current == NULL) {
                    func_ov011_0216cca8(1, 4, 1, "Tried to remove peer not in list.");
                    return;
                }
                previous = current;
            }
            previous->pnext = peer->pnext;
        }
        // The buddy messages that the peer didn't send go through the server
        while (func_ov011_02171588(peer->messages) != 0) {
            message = func_ov011_0217158c(peer->messages, 0);
            if (message->type < GPI_BM_STATUS) {
                gpiSendServerBuddyMessage(connection, peer->profile, message->type,
                                          message->buffer.buffer + message->start);
            }
            func_ov011_02171668(peer->messages, 0);
        }
        gpiDestroyPeer(connection, peer);
    }
}

int gpiProcessPeers(GPConnection *connection) {
    GPIConnection *iconnection = *connection;
    GPIPeer *peer;
    GPIPeer *next;
    int result;

    // Gt2_StopSocket runs the Udp Engine's socket for a frame
    Gt2_StopSocket();
    for (peer = iconnection->peerList; peer != NULL; peer = next) {
        next = peer->pnext;
        if (peer->state == GPI_PEER_DISCONNECTED) {
            func_ov011_0216cca8(1, 4, 8, "Peer disconnected, pid: %d", peer->profile);
            gpiRemovePeer(connection, peer);
        } else {
            result = gpiProcessPeer(connection, peer);
            if (peer->state == GPI_PEER_DISCONNECTED || result != 0 || func_ov011_021714a8(NULL) > peer->timeout) {
                func_ov011_0216cca8(1, 4, 8, "Peer disconnected, pid: %d", peer->profile);
                gpiRemovePeer(connection, peer);
            }
        }
    }
    return 0;
}

GPIPeer *gpiGetPeerByProfile(const GPConnection *connection, int profileid) {
    GPIConnection *iconnection = *connection;
    GPIPeer *peer;

    for (peer = iconnection->peerList; peer != NULL; peer = peer->pnext) {
        if (peer->profile == profileid) {
            return peer;
        }
    }
    return NULL;
}

static GPIPeer *gpiGetPeerByAddr(const GPConnection *connection, u32 ip, u16 port) {
    GPIConnection *iconnection = *connection;
    GPIPeer *peer;

    if (ip == 0 && port == 0) {
        return NULL;
    }
    for (peer = iconnection->peerList; peer != NULL; peer = peer->pnext) {
        if (peer->ip == ip && peer->port == port) {
            return peer;
        }
    }
    return NULL;
}

static void gpiFreeMessage(void *element) {
    GPIMessage *message = element;

    func_ov011_0216ddfc(message->buffer.buffer);
    message->buffer.buffer = NULL;
}

GPIPeer *gpiAddPeer(GPConnection *connection, int profileid, BOOL initiate) {
    GPIConnection *iconnection = *connection;
    GPIPeer *peer = func_ov011_0216dddc(sizeof(GPIPeer));

    if (peer == NULL) {
        return NULL;
    }
    memset(peer, 0, sizeof(GPIPeer));
    peer->state = GPI_PEER_NOT_CONNECTED;
    peer->initiated = initiate;
    peer->profile = profileid;
    peer->timeout = func_ov011_021714a8(NULL) + 10000;
    peer->pnext = iconnection->peerList;
    peer->messages = func_ov011_02171514(sizeof(GPIMessage), 0, gpiFreeMessage);
    iconnection->peerList = peer;
    peer->peerOpQueue.opList = NULL;
    peer->peerOpQueue.last = NULL;
    peer->peerOpQueue.first = NULL;
    return peer;
}

int gpiPeerGetSig(GPConnection *connection, GPIPeer *peer) {
    GPIOperation *operation;
    int result;

    result = gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, FALSE, NULL, NULL);
    if (result == 0 && (result = gpiSendGetInfo(connection, peer->profile, operation->id)) == 0) {
        peer->state = GPI_PEER_GETTING_SIG;
        result = 0;
    }
    return result;
}

int gpiPeerStartConnect(GPConnection *connection, GPIPeer *peer) {
    GPIConnection *iconnection = *connection;
    GPIProfile *profile;
    int state;
    int result;

    if (!gpiGetProfile(connection, peer->profile, &profile)) {
        gpiSetErrorString(connection, "Error connecting to a peer.");
        return 3;
    }
    if (profile->buddyStatusInfo != NULL) {
        Gt2_GetConnectionState(profile->buddyStatusInfo->buddyIp, profile->buddyStatusInfo->buddyPort, &state);
        // BUG: The test is always true, so a connection that is already being made, or made, is opened again
#ifdef BUGFIX
        if (state != 1 && state != 0)
#else
        if (state != 1 || state != 0)
#endif
        {
            result = Gt2_OpenConnection(profile->buddyStatusInfo->buddyIp, profile->buddyStatusInfo->buddyPort,
                                        iconnection->mHeader, 10000);
            if (result != 5) {
                gpiSetError(connection, 5, "There was an error starting communication with a peer.");
                gpiCallErrorCallback(connection, 3, 0);
                return 3;
            }
        }
        peer->ip = profile->buddyStatusInfo->buddyIp;
        peer->port = profile->buddyStatusInfo->buddyPort;
    }
    peer->state = GPI_PEER_CONNECTING;
    return 0;
}

int gpiPeerAddMessage(GPConnection *connection, GPIPeer *peer, int type, const char *message) {
    int result;

    if (peer == NULL) {
        return 3;
    }
    if (message == NULL) {
        return 3;
    }
    {
        int len = strlen(message);
        GPIMessage gpiMessage = { 0 };

        gpiMessage.type = type;
        result = gpiAppendStringToBuffer(connection, &gpiMessage.buffer, "\\m\\");
        if (result == 0 && (result = gpiAppendIntToBuffer(connection, &gpiMessage.buffer, type)) == 0 &&
            (result = gpiAppendStringToBuffer(connection, &gpiMessage.buffer, "\\len\\")) == 0 &&
            (result = gpiAppendIntToBuffer(connection, &gpiMessage.buffer, len)) == 0 &&
            (result = gpiAppendStringToBuffer(connection, &gpiMessage.buffer, "\\msg\\\n")) == 0) {
            gpiMessage.start = gpiMessage.buffer.len;
            result = gpiAppendStringToBufferLen(connection, &gpiMessage.buffer, message, len);
            if (result == 0 && (result = gpiAppendCharToBuffer(connection, &gpiMessage.buffer, '\0')) == 0) {
                func_ov011_021715a4(peer->messages, &gpiMessage);
                peer->timeout = func_ov011_021714a8(NULL) + 10000;
                result = 0;
            }
        }
    }
    return result;
}

int gpiPeerStartTransferMessage(GPConnection *connection, GPIPeer *peer, int type, const GPITransferID *transferID) {
    char buffer[64];
    u32 count = transferID->count;
    int profileid = transferID->profileid;
    u32 time = transferID->time;

    if (transferID == NULL) {
        return 3;
    }
    OS_SPrintf(buffer, "\\m\\%d\\xfer\\%d %u %u", type, profileid, count, time);
    return gpiSendOrBufferString(connection, peer, buffer);
}

int gpiPeerFinishTransferMessage(GPConnection *connection, GPIPeer *peer, const char *message, int len) {
    char buffer[32];
    int result;

    if (peer == NULL) {
        return 3;
    }
    if (message == NULL) {
        message = "";
    }
    if (len == -1) {
        len = strlen(message);
    }
    OS_SPrintf(buffer, "\\len\\%d\\msg\\\n", len);
    result = gpiSendOrBufferString(connection, peer, buffer);
    if (result == 0 && (result = gpiSendOrBufferStringLen(connection, peer, message, len)) == 0 &&
        (result = gpiSendOrBufferChar(connection, peer, '\0')) == 0) {
        peer->timeout = func_ov011_021714a8(NULL) + 10000;
        result = 0;
    }
    return result;
}

void gpiPeerLeftCallback(u32 ip, u16 port, int reason, void *userData) {
    GPIPeer *peer = gpiGetPeerByAddr(userData, ip, port);

    if (peer != NULL) {
        SocInAddr addr;

        addr.addr = ip;
        func_ov011_0216cca8(1, 0, 8, "Peer left: addr: %s:%d, profile: %d\n", SOC_InetNtoA(addr), port, peer->profile);
        peer->state = GPI_PEER_DISCONNECTED;
    }
}

void gpiPeerMessageCallback(u32 ip, u16 port, const void *data, int len, BOOL reliable, void *userData) {
    GPConnection *connection = userData;
    GPIPeer *peer = gpiGetPeerByAddr(connection, ip, port);
    SocInAddr addr;
    int length;
    int size;
    char *buffer;
    char *newBuffer;

    addr.addr = ip;
    // A message from an address that has no peer is one connecting to us
    if (peer == NULL) {
        peer = gpiAddPeer(connection, -1, FALSE);
        if (peer != NULL) {
            peer->state = GPI_PEER_WAITING;
            peer->ip = ip;
            peer->port = port;
        } else {
            func_ov011_0216cca8(1, 2, 1, "gpiPeerMessageCallback: out of memory when allocating peer, addr: %s:%d",
                                SOC_InetNtoA(addr), port);
            return;
        }
    }
    buffer = peer->inputBuffer.buffer;
    length = peer->inputBuffer.len;
    size = peer->inputBuffer.size;
    if (len > size - length) {
        size = length + (len >= 0x4000 ? len : 0x4000);
        newBuffer = func_ov011_0216ddec(buffer, size + 1);
        if (newBuffer == NULL) {
            func_ov011_0216cca8(1, 2, 1, "gpiPeerMessageCallback: out of memory when reallocating buffer, addr: %s:%d",
                                SOC_InetNtoA(addr), port);
            func_ov011_0216ddfc(buffer);
            gpiSetErrorString(connection, "Out of memory.");
            gpiCallErrorCallback(connection, 1, 0);
            return;
        }
        buffer = newBuffer;
    }
    memcpy(buffer + length, data, len);
    peer->inputBuffer.buffer = buffer;
    peer->inputBuffer.len += len;
    peer->inputBuffer.size = size;
    buffer[peer->inputBuffer.len] = '\0';
}

void gpiPeerAcceptedCallback(u32 ip, u16 port, int reason, BOOL rejected, void *userData) {
    GPIPeer *peer = gpiGetPeerByAddr(userData, ip, port);
    SocInAddr addr;

    addr.addr = ip;
    if (peer == NULL) {
        func_ov011_0216cca8(1, 0, 1, "Peer does not exist: ip-port: %s:%d\n", SOC_InetNtoA(addr), port);
    } else if (rejected) {
        peer->state = GPI_PEER_DISCONNECTED;
        func_ov011_0216cca8(1, 0, 8, "Peer Connection rejected: ip-port: %s:%d\n", SOC_InetNtoA(addr), port);
        return;
    }
    func_ov011_0216cca8(1, 0, 8, "Peer Connection accepted: ip-port: %s:%d\n", SOC_InetNtoA(addr), port);
}

void gpiPeerPingReplyCallback(u32 ip, u16 port, u32 latency, void *userData) {
}

void gpiPeerAddOp(GPIPeer *peer, GPIPeerOp *operation) {
    if (peer == NULL || operation == NULL) {
        func_ov011_0216cca8(1, 4, 2, "Peer operation not added");
        return;
    }
    if (peer->peerOpQueue.first == NULL) {
        peer->peerOpQueue.opList = operation;
        peer->peerOpQueue.last = operation;
        peer->peerOpQueue.first = operation;
    } else {
        if (peer->peerOpQueue.opList == peer->peerOpQueue.last) {
            peer->peerOpQueue.opList->next = operation;
        } else {
            peer->peerOpQueue.last->next = operation;
        }
        peer->peerOpQueue.last = operation;
    }
    func_ov011_0216cca8(1, 4, 8, "Peer Operation Added");
}

void gpiPeerRemoveOp(GPIPeer *peer, GPIPeerOp *operation) {
    GPIPeerOp *previous;
    GPIPeerOp *current;

    if (peer == NULL || operation == NULL) {
        func_ov011_0216cca8(1, 4, 2, "Peer operation not removed");
        return;
    }
    if (peer->peerOpQueue.first == NULL) {
        func_ov011_0216cca8(1, 4, 2, "Peer operation not removed");
        return;
    }
    previous = peer->peerOpQueue.opList;
    if (previous == peer->peerOpQueue.last && previous == operation) {
        peer->peerOpQueue.first = peer->peerOpQueue.opList = peer->peerOpQueue.last = operation->next;
    } else if (previous == operation) {
        peer->peerOpQueue.first = peer->peerOpQueue.opList = previous->next;
    } else {
        for (current = previous->next; current != operation; current = current->next) {
            if (current == NULL) {
                func_ov011_0216cca8(1, 4, 1, "Tried to remove peer operation not in list.");
                return;
            }
            previous = current;
        }
        previous->next = operation->next;
    }
    func_ov011_0216cca8(1, 4, 8, "Peer operation removed");
    func_ov011_0216ddfc(operation);
}
