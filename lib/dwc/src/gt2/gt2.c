#include "dwc/gt2.h"
#include "dwc/ext.h"
#include "dwc/gt2Main.h"
#include "dwc/gt2Utility.h"

// GameSpy's UDP engine (gsUdpEngine.c): see dwc/gt2.h. The engine, its connections and its message handlers are
// GameSpy's GSUdpEngineObject, GSUdpRemotePeer and GSUdpMsgHandler; the field names are guesses

// The one engine this build creates (sEngine); every function below reaches it through Gt2_GetEngine
typedef struct {
    GT2Socket socket;     // the lower layer's socket
    DArray connections;   // GSUdpRemotePeer, the established connections, keyed by ip and port
    DArray pending;       // GSUdpMsgHandler, keyed by its 16-byte header
    gsi_bool initialized; // whether Gt2_CreateSocket completed successfully
    gsUdpConnConnectedCallback onAttemptResult;
    gsUdpConnClosedCallback onClosed;
    gsUdpConnPingCallback onPing;
    gsUdpConnReceivedDataCallback onReceived;
    gsUdpAppConnectAttemptCallback onUnhandledAttempt;
    gsUdpErrorCallback onSocketError;
    gsUdpUnknownMsgCallback onUnknownMessage;
    void *userData;
    int pendingCount;
    unsigned int localIp;
    unsigned short localPort;
} GSUdpEngineObject;

// An established connection, keyed by its peer's address
typedef struct {
    unsigned int ip;
    unsigned short port;
    GT2Connection handle; // the lower layer's connection
} GSUdpRemotePeer;

// A message handler, with a fixed 16-byte init message and header
typedef struct {
    unsigned char initMessage[GS_UDP_MSG_HEADER_LEN];
    unsigned char header[GS_UDP_MSG_HEADER_LEN];
    // The original uses four-byte entries, but appends a record word or a record pointer on different paths.
    // Completion searches use the connection-address comparator; the inconsistent representation is unresolved.
    DArray attempts;
    gsUdpConnClosedCallback onClosed;
    gsUdpConnReceivedDataCallback onReceived;
    gsUdpConnConnectedCallback onAttemptResult;
    gsUdpConnPingCallback onPing;
    gsUdpErrorCallback onEvent;
    void *userData;
} GSUdpMsgHandler;

static GSUdpEngineObject *Gt2_GetEngine(void);
static void Gt2_FreePendingConnect(void *element);
static int Gt2_CompareInitMessage(const void *a, const void *b);
static int Gt2_ComparePendingHeader(const void *a, const void *b);
static int Gt2_CompareConnectionAddr(const void *a, const void *b);
static int Gt2_CompareConnectionHandle(const void *a, const void *b);
static void Gt2_OnSocketCreateError(GT2Socket unused);
static void Gt2_OnClosed(GT2Connection addr, GT2CloseReason reason);
static void Gt2_OnConnectAttemptDone(GT2Connection addr, GT2Result rawResult, GT2Byte *message, int length);
static void Gt2_OnPing(GT2Connection addr, int latencyMs);
static void Gt2_OnReceivedData(GT2Connection addr, GT2Byte *data, int len, GT2Bool reliable);
static GT2Bool Gt2_OnUnknownMessage(GT2Socket unused, unsigned int ip, unsigned short port, GT2Byte *data, int len);
static void Gt2_OnIncomingAttempt(GT2Socket unused, GT2Connection handle, unsigned int ip, unsigned short port,
                                  int elapsedMilliseconds, GT2Byte *data, int len);

// The one engine this build creates
static GSUdpEngineObject sEngine;

// Returns the one engine
static GSUdpEngineObject *Gt2_GetEngine(void) {
    return &sEngine;
}

// The freeFunc of the pending-connect array: frees the attempts it was still racing
static void Gt2_FreePendingConnect(void *element) {
    GSUdpMsgHandler *pending = (GSUdpMsgHandler *)element;

    func_ov011_02171558(pending->attempts);
}

// Compares the 16-byte init messages at offset 0 of two GSUdpMsgHandler
static int Gt2_CompareInitMessage(const void *a, const void *b) {
    return memcmp(a, b, 0x10);
}

// Compares the 16-byte headers at offset 0x10 of two GSUdpMsgHandler
static int Gt2_ComparePendingHeader(const void *a, const void *b) {
    const GSUdpMsgHandler *pendingA = a;
    const GSUdpMsgHandler *pendingB = b;

    return memcmp(pendingA->header, pendingB->header, sizeof(pendingA->header));
}

// Compares the ip and port of two GSUdpRemotePeer
static int Gt2_CompareConnectionAddr(const void *a, const void *b) {
    const GSUdpRemotePeer *ca = (const GSUdpRemotePeer *)a;
    const GSUdpRemotePeer *cb = (const GSUdpRemotePeer *)b;

    if (ca->ip != cb->ip) {
        return 1;
    }
    if (ca->port != cb->port) {
        return 1;
    }
    return 0;
}

// Compares the lower-layer handle of two GSUdpRemotePeer
static int Gt2_CompareConnectionHandle(const void *a, const void *b) {
    const GSUdpRemotePeer *ca = (const GSUdpRemotePeer *)a;
    const GSUdpRemotePeer *cb = (const GSUdpRemotePeer *)b;

    return ca->handle != cb->handle;
}

// The lower socket's error callback; notifies the app and every pending connect.
// Its socket argument is unused because this wrapper has one global engine.
static void Gt2_OnSocketCreateError(GT2Socket unused) {
    int i;
    GSUdpEngineObject *engine = Gt2_GetEngine();
    int count;

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Socket error, passing to app and message handlers\n");
    if (engine->onSocketError != NULL) {
        engine->onSocketError(3, engine->userData);
    }

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, i);
        if (pending->onEvent != NULL) {
            pending->onEvent(3, pending->userData);
        }
    }
}

// Notifies that a connection closed, with its mapped reason, then drops it
static void Gt2_OnClosed(GT2Connection addr, GT2CloseReason reason) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
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
                  func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, i);
        if (pending->onClosed != NULL) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Connection closed: passed to message handler\n");
            pending->onClosed(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, pending->userData);
        }
    }

    if (engine->onClosed != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "[Udp Engine] Connection closed: passed to app\n");
        engine->onClosed(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, engine->userData);
    }

    key.handle = addr;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionHandle, 0, gsi_false);
    if (index != -1) {
        func_ov011_02171668(engine->connections, index);
    }
}

// The callback the lower socket calls when one low-level attempt of a Gt2_Connect call finishes; notifies that
// attempt's own handler and the app, and drops the attempt
static void Gt2_OnConnectAttemptDone(GT2Connection addr, GT2Result rawResult, GT2Byte *message, int length) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
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
    case 0:
        reason = 0;
        break;
    case 2:
        reason = 2;
        break;
    case 6:
        reason = 6;
        break;
    case 7:
        reason = 7;
        break;
    default:
        reason = 0xd;
        break;
    }

    if (reason == 2) {
        connKey.ip = func_ov011_0217d380(addr);
        connKey.port = (gsi_u16)func_ov011_0217d384(addr);
        index = func_ov011_021716ac(engine->connections, &connKey, Gt2_CompareConnectionAddr, 0, gsi_false);
        if (index != -1) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Connect rejected by %s\n",
                          func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));
            func_ov011_02171668(engine->connections, index);
        }
    }

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, i);

        attemptKey.ip = func_ov011_0217d380(addr);
        attemptKey.port = (gsi_u16)func_ov011_0217d384(addr);
        index = func_ov011_021716ac(pending->attempts, &attemptKey, Gt2_CompareConnectionAddr, 0, gsi_false);
        if (index != -1) {
            if (pending->onAttemptResult != NULL) {
                gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                              "[Udp Engine] Passing connect result to message handler\n");
                pending->onAttemptResult(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, rawResult == 2,
                                         pending->userData);
            }
            func_ov011_02171668(pending->attempts, index);
            return;
        }
    }

    if (engine->pendingCount > 0) {
        if (engine->onAttemptResult != NULL) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Passing connect result to app\n");
            engine->onAttemptResult(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, rawResult == 2,
                                    engine->userData);
        }
        engine->pendingCount--;
    }
}

// Reports ping latency to the first pending connect with a ping callback, otherwise to the app
static void Gt2_OnPing(GT2Connection addr, int latencyMs) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    char buf[0x18];
    int i;
    int count;

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Received ping from %s\n",
                  func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GSUdpMsgHandler *pending = (GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, i);
        if (pending->onPing != NULL) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Passing to message handler\n");
            pending->onPing(func_ov011_0217d380(addr), func_ov011_0217d384(addr), latencyMs, pending->userData);
            return;
        }
    }

    if (engine->onPing != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, "[Udp Engine] Passing to app\n");
        engine->onPing(func_ov011_0217d380(addr), func_ov011_0217d384(addr), latencyMs, engine->userData);
    }
}

// The callback for a received datagram; if its first 16 bytes match a pending connect's header, notifies
// that connect's own handler, otherwise the app
static void Gt2_OnReceivedData(GT2Connection addr, GT2Byte *data, int len, GT2Bool reliable) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    char buf[0x18];
    GSUdpMsgHandler key;
    int index;
    const gsi_u8 *src;
    gsi_u8 *dest;
    gsi_u32 n;

    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Received data from %s\n",
                  func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));

    if (len >= 0x10) {
        src = (const gsi_u8 *)data;
        dest = key.header;
        n = 0x10;
        do {
            *dest++ = *src++;
        } while (--n != 0);
        index = func_ov011_021716ac(engine->pending, &key, Gt2_ComparePendingHeader, 0, gsi_false);
        if (index != -1) {
            GSUdpMsgHandler *pending = (GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, index);
            if (pending->onReceived != NULL) {
                gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                              "[Udp Engine] Passed to message handler\n");
                pending->onReceived(func_ov011_0217d380(addr), func_ov011_0217d384(addr), data + 0x10, len - 0x10,
                                    reliable, pending->userData);
                return;
            }
        }
    }

    if (engine->onReceived != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment, "[Udp Engine] Passed to app\n");
        engine->onReceived(func_ov011_0217d380(addr), func_ov011_0217d384(addr), data, len, reliable, engine->userData);
    }
}

// Handles a datagram offered by the lower socket's unknown-message path; returns whether the app handled it
static GT2Bool Gt2_OnUnknownMessage(GT2Socket unused, unsigned int ip, unsigned short port, GT2Byte *data, int len) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    char buf[0x18];
    gsi_bool handled;

    (void)unused;
    if (engine->onUnknownMessage == NULL) {
        return gsi_false;
    }

    handled = gsi_false;
    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Unknown message from %s, passing to app\n", func_ov011_0217ed14(ip, port, buf));
    if (engine->onUnknownMessage(ip, port, data, len, engine->userData) != 0) {
        handled = gsi_true;
    }
    return handled;
}

// The lower socket's incoming-attempt callback; auto-accepts when the first 16 data bytes match a pending
// connect's init message, otherwise asks the app whether to accept it
static void Gt2_OnIncomingAttempt(GT2Socket unused, GT2Connection handle, unsigned int ip, unsigned short port,
                                  int elapsedMilliseconds, GT2Byte *data, int len) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    char buf[0x18];
    GSUdpRemotePeer newConnection;
    GT2ConnectionCallbacks table;
    int index;
    GSUdpMsgHandler key;
    const gsi_u8 *src;
    gsi_u8 *dest;
    gsi_u32 n;

    (void)unused;
    gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "[Udp Engine] Connection attempt from %s\n", func_ov011_0217ed14(ip, port, buf));

    if (len >= 0x10) {
        src = (const gsi_u8 *)data;
        dest = key.initMessage;
        n = 0x10;
        do {
            *dest++ = *src++;
        } while (--n != 0);

        newConnection.ip = ip;
        newConnection.port = port;
        newConnection.handle = handle;
        func_ov011_021715a4(engine->connections, &newConnection);
        index = func_ov011_021716ac(engine->pending, &key, Gt2_CompareInitMessage, 0, gsi_false);
        if (index != -1) {
            table.closed = Gt2_OnClosed;
            table.received = Gt2_OnReceivedData;
            table.ping = Gt2_OnPing;
            table.connected = Gt2_OnConnectAttemptDone;
            func_ov011_0217d190(handle, &table);

            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "[Udp Engine] Connection attempt auto-accepted for message handler\n");
            return;
        }
    }

    if (engine->onUnhandledAttempt != NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "[Udp Engine] Connection attempt from %s, asking app to accept/reject\n",
                      func_ov011_0217ed14(ip, port, buf));
        engine->onUnhandledAttempt(ip, port, elapsedMilliseconds, data, len, engine->userData);
        return;
    }

    func_ov011_0217d198(handle, 0, 0);
    func_ov011_02171634(engine->connections, func_ov011_02171588(engine->connections) - 1);
}

// Whether the engine was initialized
gsi_bool Gt2_IsInitialized(void) {
    return Gt2_GetEngine()->initialized;
}

// Creates the one engine, binding its socket to localPort
GSUdpErrorCode Gt2_CreateSocket(unsigned short localPort, int incomingBufferSize, int outgoingBufferSize,
                                gsUdpErrorCallback onSocketError, gsUdpConnConnectedCallback onAttemptResult,
                                gsUdpConnClosedCallback onClosed, gsUdpConnPingCallback onPing,
                                gsUdpConnReceivedDataCallback onReceived, gsUdpUnknownMsgCallback onUnknownMessage,
                                gsUdpAppConnectAttemptCallback onUnhandledAttempt, void *userData) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    char addrBuf[0x18];
    int error;

    if (incomingBufferSize == 0) {
        incomingBufferSize = 0x5dc;
    }
    if (outgoingBufferSize == 0) {
        outgoingBufferSize = 0x5b4;
    }

    engine->onSocketError = onSocketError;
    engine->onUnknownMessage = onUnknownMessage;
    engine->onAttemptResult = onAttemptResult;
    engine->onClosed = onClosed;
    engine->onPing = onPing;
    engine->onReceived = onReceived;
    engine->onUnhandledAttempt = onUnhandledAttempt;

    func_ov011_0217ed14(0, localPort, addrBuf);
    error =
        func_ov011_0217d144(&engine->socket, addrBuf, outgoingBufferSize, incomingBufferSize, Gt2_OnSocketCreateError);
    if (error != 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] error creating gt2 socket, error code: %d\n", error);
        return 3;
    }

    engine->connections = func_ov011_02171514(0xc, 1, NULL);
    if (engine->connections == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    engine->pending = func_ov011_02171514(0x3c, 1, Gt2_FreePendingConnect);
    if (engine->pending == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    func_ov011_0217d3a0(engine->socket, Gt2_OnUnknownMessage);
    func_ov011_0217d188(engine->socket, Gt2_OnIncomingAttempt);
    engine->localIp = func_ov011_0217d388(engine->socket);
    engine->localPort = (gsi_u16)func_ov011_0217d38c(engine->socket);
    engine->pendingCount = 0;
    engine->initialized = gsi_true;
    if (userData != NULL) {
        engine->userData = userData;
    } else {
        engine->userData = NULL;
    }
    return 0;
}

// The lower-layer state of the connection to an address, written to *state
GSUdpErrorCode Gt2_GetConnectionState(unsigned int ip, unsigned short port, GSUdpPeerState *state) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    GSUdpRemotePeer key;
    GSUdpRemotePeer *conn;
    int index;

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_State, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        *state = 3;
        return 0xb;
    }

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, gsi_false);
    if (index == -1) {
        *state = 3;
        return 0;
    }
    conn = (GSUdpRemotePeer *)func_ov011_0217158c(engine->connections, index);
    *state = func_ov011_0217d364(conn->handle);
    return 0;
}

// Reuses a known connection or starts one through the lower layer, then associates it with a pending init message
GSUdpErrorCode Gt2_OpenConnection(unsigned int ip, unsigned short port, char initMessage[GS_UDP_MSG_HEADER_LEN],
                                  int timeoutMs) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    char buf[0x18];
    GSUdpRemotePeer key;
    GSUdpRemotePeer *conn;
    GT2ConnectionCallbacks table;
    GSUdpMsgHandler pendingKey;
    const gsi_u8 *src;
    gsi_u8 *dest;
    gsi_u32 n;
    int index;
    int state;

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (ip == 0 || port == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Invalid parameter(s), check ip, port");
        return 0xa;
    }

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, gsi_false);
    if (index != -1) {
        conn = (GSUdpRemotePeer *)func_ov011_0217158c(engine->connections, index);
        state = func_ov011_0217d364(conn->handle);
        if (state == 1) {
            gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                          "[Udp Engine] Engine is already talking to remote address\n");
            return 5;
        }
        if (state != 0) {
            return 0;
        }

        src = (const gsi_u8 *)initMessage;
        dest = pendingKey.initMessage;
        n = 0x10;
        do {
            *dest++ = *src++;
        } while (--n != 0);
        index = func_ov011_021716ac(engine->pending, &pendingKey, Gt2_CompareInitMessage, 0, gsi_false);
        if (index == -1) {
            return 0;
        }
        func_ov011_021715a4(((GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, index))->attempts, conn);
        return 0;
    }

    func_ov011_0217ed14(ip, port, buf);
    table.closed = Gt2_OnClosed;
    table.received = Gt2_OnReceivedData;
    table.ping = Gt2_OnPing;
    table.connected = Gt2_OnConnectAttemptDone;
    func_ov011_0217d1a0(engine->socket, &key.handle, buf, initMessage, 0x10, timeoutMs, &table, 0);

    func_ov011_021715a4(engine->connections, &key);
    src = (const gsi_u8 *)initMessage;
    dest = pendingKey.initMessage;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);
    index = func_ov011_021716ac(engine->pending, &pendingKey, Gt2_CompareInitMessage, 0, gsi_false);
    if (index != -1) {
        conn =
            (GSUdpRemotePeer *)func_ov011_0217158c(engine->connections, func_ov011_02171588(engine->connections) - 1);
        func_ov011_021715a4(((GSUdpMsgHandler *)func_ov011_0217158c(engine->pending, index))->attempts, &conn);
    } else {
        engine->pendingCount++;
    }
    return 0;
}

// Sends a message to the connection of an address, with a 16-byte header prepended
GSUdpErrorCode Gt2_Send(unsigned int ip, unsigned short port, char header[GS_UDP_MSG_HEADER_LEN],
                        unsigned char *message, unsigned int size, gsi_bool reliable) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    GSUdpRemotePeer key;
    char buf[0x18];
    GSUdpRemotePeer *conn;
    void *packet;
    int index;
    int maxSize;
    int total;
    const gsi_u8 *src;
    gsi_u8 *dest;
    int n;

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }

    total = (((const char *)header)[0] != 0) ? size + 0x10 : size;

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, gsi_false);
    if (index == -1) {
        // BUG: The formatted address is computed but the format string has no %s to print it
#ifdef BUGFIX
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] address not found for sending message: %s\n", func_ov011_0217ed14(ip, port, buf));
#else
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "[Udp Engine] address not found for sending message\n", func_ov011_0217ed14(ip, port, buf));
#endif
        return 4;
    }

    conn = (GSUdpRemotePeer *)func_ov011_0217158c(engine->connections, index);
    maxSize = func_ov011_0217d390(conn->handle);
    if (total > maxSize && reliable != 0) {
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
    src = (const gsi_u8 *)header;
    dest = (gsi_u8 *)packet;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);
    memcpy((gsi_u8 *)packet + 0x10, message, size);
    index = func_ov011_0217d290(conn->handle, packet, total, reliable);
    gsifree(packet);
    return (index != 0) ? 8 : 0;
}

// Stops the engine's socket without freeing it
GSUdpErrorCode Gt2_StopSocket(void) {
    GSUdpEngineObject *engine = Gt2_GetEngine();

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    func_ov011_0217d168(engine->socket);
    return 0;
}

// Tears the engine down entirely
GSUdpErrorCode Gt2_CloseSocket(void) {
    GSUdpEngineObject *engine = Gt2_GetEngine();

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    func_ov011_0217d158(engine->socket);
    func_ov011_02171558(engine->pending);
    func_ov011_02171558(engine->connections);
    engine->initialized = gsi_false;
    return 0;
}

// The engine's own bound local port
unsigned short Gt2_GetLocalPort(void) {
    GSUdpEngineObject *engine = Gt2_GetEngine();

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_HotError,
                      "[Udp Engine] Engine not initialized\n");
        return 0;
    }
    return engine->localPort;
}

// Registers a pending connect; initMessage and header each point to 16 readable bytes with a nonzero first byte
GSUdpErrorCode Gt2_Connect(char initMessage[GS_UDP_MSG_HEADER_LEN], char header[GS_UDP_MSG_HEADER_LEN],
                           gsUdpErrorCallback onEvent, gsUdpConnConnectedCallback onAttemptResult,
                           gsUdpConnClosedCallback onClosed, gsUdpConnPingCallback onPing,
                           gsUdpConnReceivedDataCallback onReceived, void *userData) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    GSUdpMsgHandler pending;
    void *attempts;
    const gsi_u8 *src;
    gsi_u8 *dest;
    int n;

    if (engine->initialized == 0) {
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

    src = (const gsi_u8 *)initMessage;
    dest = pending.initMessage;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);
    src = (const gsi_u8 *)header;
    dest = pending.header;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);

    attempts = func_ov011_02171514(4, 1, NULL);
    if (attempts == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    pending.attempts = attempts;
    pending.onClosed = onClosed;
    pending.onPing = onPing;
    pending.onReceived = onReceived;
    pending.onAttemptResult = onAttemptResult;
    pending.onEvent = onEvent;
    pending.userData = userData;
    func_ov011_021715a4(engine->pending, &pending);
    return 0;
}

// Removes the pending connect with a matching 16-byte header
GSUdpErrorCode Gt2_CancelConnect(char header[GS_UDP_MSG_HEADER_LEN]) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    GSUdpMsgHandler key;
    const gsi_u8 *src;
    gsi_u8 *dest;
    gsi_u32 n;
    int index;

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (header == NULL || header[0] == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Invalid or empty header\n");
        return 0xa;
    }

    src = (const gsi_u8 *)header;
    dest = key.header;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);

    index = func_ov011_021716ac(engine->pending, &key, Gt2_ComparePendingHeader, 0, gsi_false);
    if (index != -1) {
        func_ov011_02171668(engine->pending, index);
    }
    return 0;
}

// Whether there are no pending connects left
gsi_bool Gt2_IsIdle(void) {
    GSUdpEngineObject *engine = Gt2_GetEngine();

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return gsi_true;
    }
    return func_ov011_02171588(engine->pending) == 0;
}

// Whether Gt2_CreateSocket was given none of its app-wide callbacks
gsi_bool Gt2_HasNoAppCallbacks(void) {
    GSUdpEngineObject *engine = Gt2_GetEngine();

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return gsi_true;
    }
    if (engine->onClosed != NULL || engine->onUnhandledAttempt != NULL || engine->onAttemptResult != NULL ||
        engine->onSocketError != NULL || engine->onPing != NULL || engine->onReceived != NULL ||
        engine->onUnknownMessage != NULL) {
        return gsi_false;
    }
    return gsi_true;
}

// Closes an established connection
int Gt2_Close(unsigned int ip, unsigned short port) {
    GSUdpEngineObject *engine = Gt2_GetEngine();
    GSUdpRemotePeer key;
    GSUdpRemotePeer *conn;
    int index;

    if (engine->initialized == 0) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Network, GSIDebugLevel_Debug,
                      "[Udp Engine] Engine not initialized\n");
        return 0;
    }

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, gsi_false);
    if (index != -1) {
        conn = (GSUdpRemotePeer *)func_ov011_0217158c(engine->connections, index);
        return func_ov011_0217d394(conn->handle);
    }
    return 0;
}

// A wide-character string's length
int Gt2_WStrLen(const gsi_u16 *str) {
    const gsi_u16 *p = str;

    while (*p++ != 0) {
    }
    return p - str - 1;
}
