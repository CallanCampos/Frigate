#include <string.h>
// Before dwc/ext.h: MWCC copies the 16-byte headers in place, as the ROM does, only when <string.h> declares memcpy
// first
#include "dwc/ext.h"
#include "dwc/gsUdpEngine.h"
#include "dwc/gt2Main.h"
#include "dwc/gt2Utility.h"

// GameSpy's UDP engine (gsUdpEngine.c): see dwc/gsUdpEngine.h. The engine, its connections and its message handlers are
// GameSpy's GSUdpEngineObject, GSUdpRemotePeer and GSUdpMsgHandler, with GameSpy's field names where the code shows
// them and ours elsewhere

// The one engine this build creates (aUdpObject); every function below reaches it through gsUdpEngineGetEngine
typedef struct {
    GT2Socket mSocket;     // the lower layer's socket
    DArray mRemotePeers;   // GSUdpRemotePeer, the established connections, keyed by ip and port
    DArray mMsgHandlers;   // GSUdpMsgHandler, keyed by its 16-byte header
    gsi_bool mInitialized; // whether gsUdpEngineInitialize completed successfully
    gsUdpConnConnectedCallback mAppConnected;
    gsUdpConnClosedCallback mAppClosed;
    gsUdpConnPingCallback mAppPingReply;
    gsUdpConnReceivedDataCallback mAppRecvData;
    gsUdpAppConnectAttemptCallback mAppConnAttempt;
    gsUdpErrorCallback mAppNetworkError;
    gsUdpUnknownMsgCallback mAppUnknownMessage;
    void *mAppUserData;
    int mAppPendingConnections;
    unsigned int mLocalAddr;
    unsigned short mLocalPort;
} GSUdpEngineObject;

// An established connection, keyed by its peer's address
typedef struct {
    unsigned int mAddr;
    unsigned short mPort;
    GT2Connection mConnection; // the lower layer's connection
} GSUdpRemotePeer;

// A message handler, with a fixed 16-byte init message and header
typedef struct {
    unsigned char mInitialMsg[GS_UDP_MSG_HEADER_LEN];
    unsigned char mHeader[GS_UDP_MSG_HEADER_LEN];
    // The original uses four-byte entries, but appends a record word or a record pointer on different paths.
    // Completion searches use the connection-address comparator; the inconsistent representation is unresolved.
    DArray mPendingConnections;
    gsUdpConnClosedCallback mClosed;
    gsUdpConnReceivedDataCallback mReceived;
    gsUdpConnConnectedCallback mConnected;
    gsUdpConnPingCallback mPingReply;
    gsUdpErrorCallback mNetworkError;
    void *mUserData;
} GSUdpMsgHandler;

static GSUdpEngineObject *gsUdpEngineGetEngine(void);
static void gsUdpMsgHandlerFree(void *element);
static int gsUdpMsgHandlerCompare(const void *a, const void *b);
static int gsUdpMsgHandlerCompare2(const void *a, const void *b);
static int gsUdpRemotePeerCompare(const void *a, const void *b);
static int gsUdpRemotePeerCompare2(const void *a, const void *b);
static void gsUdpSocketError(GT2Socket unused);
static void gsUdpClosedRoutingCB(GT2Connection addr, GT2CloseReason reason);
static void gsUdpConnectedRoutingCB(GT2Connection addr, GT2Result rawResult, GT2Byte *message, int length);
static void gsUdpPingRoutingCB(GT2Connection addr, int latencyMs);
static void gsUdpReceivedRoutingCB(GT2Connection addr, GT2Byte *data, int len, GT2Bool reliable);
static GT2Bool gsUdpUnrecognizedMsgCB(GT2Socket unused, unsigned int ip, unsigned short port, GT2Byte *data, int len);
static void gsUdpConnAttemptCB(GT2Socket unused, GT2Connection handle, unsigned int ip, unsigned short port,
                               int elapsedMilliseconds, GT2Byte *data, int len);

// The one engine this build creates
static GSUdpEngineObject aUdpObject;

// Returns the one engine
static GSUdpEngineObject *gsUdpEngineGetEngine(void) {
    return &aUdpObject;
}

// The freeFunc of the pending-connect array: frees the attempts it was still racing
static void gsUdpMsgHandlerFree(void *element) {
    GSUdpMsgHandler *pending = (GSUdpMsgHandler *)element;

    ArrayFree(pending->mPendingConnections);
}

// Compares the 16-byte init messages at offset 0 of two GSUdpMsgHandler
static int gsUdpMsgHandlerCompare(const void *a, const void *b) {
    return memcmp(a, b, 0x10);
}

// Compares the 16-byte headers at offset 0x10 of two GSUdpMsgHandler
static int gsUdpMsgHandlerCompare2(const void *a, const void *b) {
    const GSUdpMsgHandler *pendingA = a;
    const GSUdpMsgHandler *pendingB = b;

    return memcmp(pendingA->mHeader, pendingB->mHeader, sizeof(pendingA->mHeader));
}

// Compares the ip and port of two GSUdpRemotePeer
static int gsUdpRemotePeerCompare(const void *a, const void *b) {
    const GSUdpRemotePeer *ca = (const GSUdpRemotePeer *)a;
    const GSUdpRemotePeer *cb = (const GSUdpRemotePeer *)b;

    if (ca->mAddr != cb->mAddr) {
        return 1;
    }
    if (ca->mPort != cb->mPort) {
        return 1;
    }
    return 0;
}

// Compares the lower-layer handle of two GSUdpRemotePeer
static int gsUdpRemotePeerCompare2(const void *a, const void *b) {
    const GSUdpRemotePeer *ca = (const GSUdpRemotePeer *)a;
    const GSUdpRemotePeer *cb = (const GSUdpRemotePeer *)b;

    return ca->mConnection != cb->mConnection;
}

// The lower socket's error callback; notifies the app and every pending connect.
// Its socket argument is unused because this wrapper has one global engine.
static void gsUdpSocketError(GT2Socket unused) {
    int i;
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    int count;

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Socket error, passing to app and message handlers\n");
    if (engine->mAppNetworkError != NULL) {
        engine->mAppNetworkError(3, engine->mAppUserData);
    }

    count = ArrayLength(engine->mMsgHandlers);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, i);
        if (pending->mNetworkError != NULL) {
            pending->mNetworkError(3, pending->mUserData);
        }
    }
}

// Notifies that a connection closed, with its mapped reason, then drops it
static void gsUdpClosedRoutingCB(GT2Connection addr, GT2CloseReason reason) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    int i;
    int count;
    int index;
    GSUdpRemotePeer key;
    char buf[0x18];

    if (reason == 2 || reason == 3) {
        reason = 2;
    } else if (reason == 4) {
        reason = 3;
    }

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Connection closed to %s\n",
                  gt2AddressToString(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), buf));

    count = ArrayLength(engine->mMsgHandlers);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, i);
        if (pending->mClosed != NULL) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Connection closed: passed to message handler\n");
            pending->mClosed(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), reason, pending->mUserData);
        }
    }

    if (engine->mAppClosed != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "[Udp Engine] Connection closed: passed to app\n");
        engine->mAppClosed(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), reason, engine->mAppUserData);
    }

    key.mConnection = addr;
    index = ArraySearch(engine->mRemotePeers, &key, gsUdpRemotePeerCompare2, 0, gsi_false);
    if (index != -1) {
        ArrayDeleteAt(engine->mRemotePeers, index);
    }
}

// The callback the lower socket calls when one low-level attempt of a gsUdpEngineAddMsgHandler call finishes; notifies
// that attempt's own handler and the app, and drops the attempt
static void gsUdpConnectedRoutingCB(GT2Connection addr, GT2Result rawResult, GT2Byte *message, int length) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    char buf[0x18];
    int count;
    int reason;
    int i;
    int index;
    GSUdpRemotePeer connKey;
    GSUdpRemotePeer attemptKey;

    (void)message;
    (void)length;
    switch (rawResult) {
    case 7:
        reason = 7;
        break;
    case 2:
        reason = 2;
        break;
    case 6:
        reason = 6;
        break;
    case 0:
        reason = 0;
        break;
    default:
        reason = 0xd;
        break;
    }

    if (rawResult == 2) {
        connKey.mAddr = gt2GetRemoteIP(addr);
        connKey.mPort = (gsi_u16)gt2GetRemotePort(addr);
        index = ArraySearch(engine->mRemotePeers, &connKey, gsUdpRemotePeerCompare, 0, gsi_false);
        if (index != -1) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Connect rejected by %s\n",
                          gt2AddressToString(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), buf));
            ArrayDeleteAt(engine->mRemotePeers, index);
        }
    }

    count = ArrayLength(engine->mMsgHandlers);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, i);

        attemptKey.mAddr = gt2GetRemoteIP(addr);
        attemptKey.mPort = (gsi_u16)gt2GetRemotePort(addr);
        index = ArraySearch(pending->mPendingConnections, &attemptKey, gsUdpRemotePeerCompare, 0, gsi_false);
        if (index != -1) {
            if (pending->mConnected != NULL) {
                gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                              "[Udp Engine] Passing connect result to message handler\n");
                pending->mConnected(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), reason, (rawResult == 2) ? 1 : 0,
                                    pending->mUserData);
            }
            ArrayDeleteAt(pending->mPendingConnections, index);
            return;
        }
    }

    if (engine->mAppPendingConnections > 0) {
        if (engine->mAppConnected != NULL) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Passing connect result to app\n");
            engine->mAppConnected(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), reason, (rawResult == 2) ? 1 : 0,
                                  engine->mAppUserData);
        }
        engine->mAppPendingConnections--;
    }
}

// Reports ping latency to the first pending connect with a ping callback, otherwise to the app
static void gsUdpPingRoutingCB(GT2Connection addr, int latencyMs) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    char buf[0x18];
    int i;
    int count;

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Received ping from %s\n",
                  gt2AddressToString(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), buf));

    count = ArrayLength(engine->mMsgHandlers);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, i);
        if (pending->mPingReply != NULL) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Passed to message handler\n");
            pending->mPingReply(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), latencyMs, pending->mUserData);
            return;
        }
    }

    if (engine->mAppPingReply != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, "[Udp Engine] Passed to app\n");
        engine->mAppPingReply(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), latencyMs, engine->mAppUserData);
    }
}

// The callback for a received datagram; if its first 16 bytes match a pending connect's header, notifies
// that connect's own handler, otherwise the app
static void gsUdpReceivedRoutingCB(GT2Connection addr, GT2Byte *data, int len, GT2Bool reliable) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    GSUdpMsgHandler key;
    char buf[0x18];
    int index;

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Received data from %s\n",
                  gt2AddressToString(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), buf));

    if (len >= 0x10) {
        memcpy(key.mHeader, data, GS_UDP_MSG_HEADER_LEN);
        index = ArraySearch(engine->mMsgHandlers, &key, gsUdpMsgHandlerCompare2, 0, gsi_false);
        if (index != -1) {
            GSUdpMsgHandler *pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, index);
            if (pending->mReceived != NULL) {
                gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                              "[Udp Engine] Passed to message handler\n");
                pending->mReceived(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), data + 0x10, len - 0x10, reliable,
                                   pending->mUserData);
                return;
            }
        }
    }

    if (engine->mAppRecvData != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, "[Udp Engine] Passed to app\n");
        engine->mAppRecvData(gt2GetRemoteIP(addr), gt2GetRemotePort(addr), data, len, reliable, engine->mAppUserData);
    }
}

// Handles a datagram offered by the lower socket's unknown-message path; returns whether the app handled it
static GT2Bool gsUdpUnrecognizedMsgCB(GT2Socket unused, unsigned int ip, unsigned short port, GT2Byte *data, int len) {
    char buf[0x18];
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    (void)unused;
    if (engine->mAppUnknownMessage != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "[Udp Engine] Unknown message from %s, passing to app\n", gt2AddressToString(ip, port, buf));
        if (engine->mAppUnknownMessage(ip, port, data, len, engine->mAppUserData) != 0) {
            return gsi_true;
        }
        return gsi_false;
    }
    return gsi_false;
}

// The lower socket's incoming-attempt callback; auto-accepts when the first 16 data bytes match a pending
// connect's init message, otherwise asks the app whether to accept it
static void gsUdpConnAttemptCB(GT2Socket unused, GT2Connection handle, unsigned int ip, unsigned short port,
                               int elapsedMilliseconds, GT2Byte *data, int len) {
    int index;
    GSUdpMsgHandler key;
    GSUdpRemotePeer newConnection;
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    char buf[0x18];
    GT2ConnectionCallbacks table;

    (void)unused;
    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Connection attempt from %s\n", gt2AddressToString(ip, port, buf));
    if (len >= 0x10) {
        memcpy(key.mInitialMsg, data, GS_UDP_MSG_HEADER_LEN);
        newConnection.mAddr = ip;
        newConnection.mPort = port;
        newConnection.mConnection = handle;
        ArrayAppend(engine->mRemotePeers, &newConnection);
        index = ArraySearch(engine->mMsgHandlers, &key, gsUdpMsgHandlerCompare, 0, gsi_false);
        if (index != -1) {
            table.closed = gsUdpClosedRoutingCB;
            table.connected = gsUdpConnectedRoutingCB;
            table.ping = gsUdpPingRoutingCB;
            table.received = gsUdpReceivedRoutingCB;
            gt2Accept(newConnection.mConnection, &table);
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Connection attempt auto-accepted for message handler\n");
            return;
        }
    }
    if (engine->mAppConnAttempt != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "[Udp Engine] Connection attempt from %s, asking app to accept/reject\n",
                      gt2AddressToString(ip, port, buf));
        engine->mAppConnAttempt(ip, port, elapsedMilliseconds, data, len, engine->mAppUserData);
        return;
    }
    gt2Reject(handle, 0, 0);
    ArrayRemoveAt(engine->mRemotePeers, ArrayLength(engine->mRemotePeers) - 1);
}

// Whether the engine was initialized
gsi_bool gsUdpEngineIsInitialized(void) {
    return gsUdpEngineGetEngine()->mInitialized;
}

// Creates the one engine, binding its socket to localPort
GSUdpErrorCode gsUdpEngineInitialize(unsigned short localPort, int incomingBufferSize, int outgoingBufferSize,
                                     gsUdpErrorCallback onSocketError, gsUdpConnConnectedCallback onAttemptResult,
                                     gsUdpConnClosedCallback onClosed, gsUdpConnPingCallback onPing,
                                     gsUdpConnReceivedDataCallback onReceived, gsUdpUnknownMsgCallback onUnknownMessage,
                                     gsUdpAppConnectAttemptCallback onUnhandledAttempt, void *userData) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    char addrBuf[0x18];
    int error;

    if (incomingBufferSize == 0) {
        incomingBufferSize = 0x5dc;
    }
    if (outgoingBufferSize == 0) {
        outgoingBufferSize = 0x5b4;
    }

    engine->mAppNetworkError = onSocketError;
    engine->mAppUnknownMessage = onUnknownMessage;
    engine->mAppConnected = onAttemptResult;
    engine->mAppClosed = onClosed;
    engine->mAppPingReply = onPing;
    engine->mAppRecvData = onReceived;
    engine->mAppConnAttempt = onUnhandledAttempt;

    gt2AddressToString(0, localPort, addrBuf);
    error = gt2CreateSocket(&engine->mSocket, addrBuf, outgoingBufferSize, incomingBufferSize, gsUdpSocketError);
    if (error != 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] error creating gt2 socket, error code: %d\n", error);
        return 3;
    }

    engine->mRemotePeers = ArrayNew(0xc, 1, NULL);
    if (engine->mRemotePeers == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    engine->mMsgHandlers = ArrayNew(0x3c, 1, gsUdpMsgHandlerFree);
    if (engine->mMsgHandlers == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    gt2SetUnrecognizedMessageCallback(engine->mSocket, gsUdpUnrecognizedMsgCB);
    gt2Listen(engine->mSocket, gsUdpConnAttemptCB);
    engine->mLocalAddr = gt2GetLocalIP(engine->mSocket);
    engine->mLocalPort = (gsi_u16)gt2GetLocalPort(engine->mSocket);
    engine->mAppPendingConnections = 0;
    engine->mInitialized = gsi_true;
    if (userData != NULL) {
        engine->mAppUserData = userData;
    } else {
        engine->mAppUserData = NULL;
    }
    return 0;
}

// The lower-layer state of the connection to an address, written to *state
GSUdpErrorCode gsUdpEngineGetPeerState(unsigned int ip, unsigned short port, GSUdpPeerState *state) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    GSUdpRemotePeer key;
    GSUdpRemotePeer *conn;
    int index;

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_State, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        *state = 3;
        return 0xb;
    }

    key.mAddr = ip;
    key.mPort = port;
    index = ArraySearch(engine->mRemotePeers, &key, gsUdpRemotePeerCompare, 0, gsi_false);
    if (index == -1) {
        *state = 3;
        return 0;
    }
    conn = (GSUdpRemotePeer *)ArrayNth(engine->mRemotePeers, index);
    *state = gt2GetConnectionState(conn->mConnection);
    return 0;
}

// Reuses a known connection or starts one through the lower layer, then associates it with a pending init message
GSUdpErrorCode gsUdpEngineStartTalkingToPeer(unsigned int ip, unsigned short port,
                                             char initMessage[GS_UDP_MSG_HEADER_LEN], int timeoutMs) {
    char buf[0x18];
    GSUdpRemotePeer key;
    GSUdpMsgHandler pendingKey;
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    GT2ConnectionCallbacks table;
    int index;

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (ip == 0 || port == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Invalid parameter(s), check ip, port");
        return 0xa;
    }
    key.mAddr = ip;
    key.mPort = port;
    index = ArraySearch(engine->mRemotePeers, &key, gsUdpRemotePeerCompare, 0, gsi_false);
    if (index != -1) {
        GSUdpRemotePeer *conn = (GSUdpRemotePeer *)ArrayNth(engine->mRemotePeers, index);
        int state = gt2GetConnectionState(conn->mConnection);

        if (state == 1) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                          "[Udp Engine] Engine is already talking to remote address\n");
            return 5;
        }
        if (state == 0) {
            GSUdpMsgHandler *pending;

            memcpy(pendingKey.mInitialMsg, initMessage, GS_UDP_MSG_HEADER_LEN);
            index = ArraySearch(engine->mMsgHandlers, &pendingKey, gsUdpMsgHandlerCompare, 0, gsi_false);
            if (index != -1) {
                pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, index);
                ArrayAppend(pending->mPendingConnections, conn);
            }
        }
    } else {
        GSUdpMsgHandler *pending;
        GSUdpRemotePeer *conn;

        gt2AddressToString(ip, port, buf);
        table.closed = gsUdpClosedRoutingCB;
        table.connected = gsUdpConnectedRoutingCB;
        table.ping = gsUdpPingRoutingCB;
        table.received = gsUdpReceivedRoutingCB;
        gt2Connect(engine->mSocket, &key.mConnection, buf, initMessage, 0x10, timeoutMs, &table, 0);
        ArrayAppend(engine->mRemotePeers, &key);
        memcpy(pendingKey.mInitialMsg, initMessage, GS_UDP_MSG_HEADER_LEN);
        index = ArraySearch(engine->mMsgHandlers, &pendingKey, gsUdpMsgHandlerCompare, 0, gsi_false);
        if (index != -1) {
            conn = (GSUdpRemotePeer *)ArrayNth(engine->mRemotePeers, ArrayLength(engine->mRemotePeers) - 1);
            pending = (GSUdpMsgHandler *)ArrayNth(engine->mMsgHandlers, index);
            ArrayAppend(pending->mPendingConnections, &conn);
        } else {
            engine->mAppPendingConnections++;
        }
    }
    return 0;
}

// Sends a message to the connection of an address, with a 16-byte header prepended
GSUdpErrorCode gsUdpEngineSendMessage(unsigned int ip, unsigned short port, char header[GS_UDP_MSG_HEADER_LEN],
                                      unsigned char *message, unsigned int size, gsi_bool reliable) {
    GSUdpEngineObject *engine;
    int total;
    int index;
    GSUdpRemotePeer key;
    GSUdpRemotePeer *conn;
    unsigned char *packet;
    int result;
    char buf[22];

    engine = gsUdpEngineGetEngine();
    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (header[0]) {
        total = size + 0x10;
    } else {
        total = size;
    }
    key.mAddr = ip;
    key.mPort = port;
    index = ArraySearch(engine->mRemotePeers, &key, gsUdpRemotePeerCompare, 0, gsi_false);
    if (index == -1) {
        gt2AddressToString(ip, port, buf);
        // BUG: The formatted address is passed but the format string has no %s to print it
#ifdef BUGFIX
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] address not found for sending message: %s\n", buf);
#else
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] address not found for sending message\n", buf);
#endif
        return 4;
    }
    conn = (GSUdpRemotePeer *)ArrayNth(engine->mRemotePeers, index);
    if (total > gt2GetOutgoingBufferSize(conn->mConnection) && reliable) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] Message Size too large, dropping message\n");
        return 0xc;
    }
    // BUG: An empty header allocates only size bytes, but the copies always write size + 0x10 bytes.
#ifdef BUGFIX
    // Reserve capacity for both copies while preserving the original transmitted length.
    packet = gsimalloc(size + 0x10);
#else
    packet = gsimalloc(total);
#endif
    memcpy(packet, header, GS_UDP_MSG_HEADER_LEN);
    memcpy(packet + 0x10, message, size);
    result = gt2Send(conn->mConnection, packet, total, reliable);
    gsifree(packet);
    if (result != 0) {
        return 8;
    }
    return 0;
}

// Stops the engine's socket without freeing it
GSUdpErrorCode gsUdpEngineThink(void) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    gt2Think(engine->mSocket);
    return 0;
}

// Tears the engine down entirely
GSUdpErrorCode gsUdpEngineShutdown(void) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    gt2CloseSocket(engine->mSocket);
    ArrayFree(engine->mMsgHandlers);
    ArrayFree(engine->mRemotePeers);
    engine->mInitialized = gsi_false;
    return 0;
}

// The engine's own bound local port
unsigned short gsUdpEngineGetLocalPort(void) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_HotError,
                      "[Udp Engine] Engine not initialized\n");
        return 0;
    }
    return engine->mLocalPort;
}

// Registers a pending connect; initMessage and header each point to 16 readable bytes with a nonzero first byte
GSUdpErrorCode gsUdpEngineAddMsgHandler(char initMessage[GS_UDP_MSG_HEADER_LEN], char header[GS_UDP_MSG_HEADER_LEN],
                                        gsUdpErrorCallback onEvent, gsUdpConnConnectedCallback onAttemptResult,
                                        gsUdpConnClosedCallback onClosed, gsUdpConnPingCallback onPing,
                                        gsUdpConnReceivedDataCallback onReceived, void *userData) {
    GSUdpMsgHandler pending;
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (initMessage[0] == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Invalid init message\n");
        return 0xa;
    }
    if (header[0] == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug, "[Udp Engine] Invalid header\n");
        return 0xa;
    }
    pending.mClosed = onClosed;
    pending.mConnected = onAttemptResult;
    pending.mPingReply = onPing;
    pending.mReceived = onReceived;
    pending.mNetworkError = onEvent;
    memcpy(pending.mInitialMsg, initMessage, GS_UDP_MSG_HEADER_LEN);
    memcpy(pending.mHeader, header, GS_UDP_MSG_HEADER_LEN);
    pending.mPendingConnections = ArrayNew(4, 1, NULL);
    if (pending.mPendingConnections == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "[Udp Engine] No more memory!!!\n");
        return 1;
    }
    pending.mUserData = userData;
    ArrayAppend(engine->mMsgHandlers, &pending);
    return 0;
}

// Removes the pending connect with a matching 16-byte header
GSUdpErrorCode gsUdpEngineRemoveMsgHandler(char header[GS_UDP_MSG_HEADER_LEN]) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    GSUdpMsgHandler key;
    int index;

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (header == NULL || header[0] == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] invalid or empty header\n");
        return 0xa;
    }

    memcpy(key.mHeader, header, GS_UDP_MSG_HEADER_LEN);

    index = ArraySearch(engine->mMsgHandlers, &key, gsUdpMsgHandlerCompare2, 0, gsi_false);
    if (index != -1) {
        ArrayDeleteAt(engine->mMsgHandlers, index);
    }
    return 0;
}

// Whether there are no pending connects left
gsi_bool gsUdpEngineNoMoreMsgHandlers(void) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return gsi_true;
    }
    return ArrayLength(engine->mMsgHandlers) == 0;
}

// Whether gsUdpEngineInitialize was given none of its app-wide callbacks
gsi_bool gsUdpEngineNoApp(void) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return gsi_true;
    }
    if (engine->mAppClosed != NULL || engine->mAppConnAttempt != NULL || engine->mAppConnected != NULL ||
        engine->mAppNetworkError != NULL || engine->mAppPingReply != NULL || engine->mAppRecvData != NULL ||
        engine->mAppUnknownMessage != NULL) {
        return gsi_false;
    }
    return gsi_true;
}

// Closes an established connection
int gsUdpEngineGetPeerOutBufferFreeSpace(unsigned int ip, unsigned short port) {
    GSUdpEngineObject *engine = gsUdpEngineGetEngine();
    GSUdpRemotePeer key;
    GSUdpRemotePeer *conn;
    int index;

    if (engine->mInitialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 0;
    }

    key.mAddr = ip;
    key.mPort = port;
    index = ArraySearch(engine->mRemotePeers, &key, gsUdpRemotePeerCompare, 0, gsi_false);
    if (index != -1) {
        conn = (GSUdpRemotePeer *)ArrayNth(engine->mRemotePeers, index);
        return gt2GetOutgoingBufferFreeSpace(conn->mConnection);
    }
    return 0;
}
