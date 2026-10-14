#include "dwc/gt2.h"
#include "dwc/ext.h"

// Guessed as "gt2.c": Nintendo's "Udp Engine" wrapper around GameSpy's GT2, named only from the error string this
// file's data pool carries, "[Udp Engine] error creating gt2 socket, error code: %d". No existing decomp has
// recovered the real GT2 SDK (gt2.c/gt2callback.c/gt2aux.c), and the lower socket layer this file builds on
// (func_ov011_0217d144 and the other func_ov011_0217dNNN addresses declared in ext.h) isn't decompiled either, so
// every name and struct field here is a guess read off this file's own behavior, never swan's or the SDK's.

// The one engine this build creates (data_ov011_02190928); every function below reaches it through Gt2_GetEngine
typedef struct GT2Engine {
    void *socket;                       // the lower layer's socket handle
    void *connections;                  // GT2Connection array, the established connections, keyed by ip and port
    void *pending;                      // GT2PendingConnect array, one per Gt2_Connect call, keyed by its address
    int maxConnections;                 // 0 until Gt2_CreateSocket runs; doubles as the "is created" flag
    Gt2AttemptResultCallback onAttemptResult;
    Gt2ClosedCallback onClosed;
    Gt2PingCallback onPing;
    Gt2ReceivedCallback onReceived;
    Gt2ReceivedCallback onUnroutable;
    Gt2EventCallback onSocketError;
    Gt2UnknownMessageCallback onUnknownMessage;
    void *userData;
    int pendingCount;
    u32 localIp;
    u16 localPort;
} GT2Engine;

// An established connection, keyed by its peer's address
typedef struct GT2Connection {
    u32 ip;
    u16 port;
    void *handle;                       // the lower layer's per-connection handle
} GT2Connection;

// A connect attempt started by Gt2_Connect, keyed by its local and remote address text
typedef struct GT2PendingConnect {
    u8 localAddr[0x10];
    u8 remoteAddr[0x10];
    void *attempts;                     // the low-level connections raced for this one Gt2_Connect call
    Gt2ClosedCallback onClosed;
    Gt2PendingDataCallback onReceived;
    Gt2AttemptResultCallback onAttemptResult;
    Gt2PingCallback onPing;
    Gt2EventCallback onEvent;
    void *userData;
} GT2PendingConnect;

// The callback table a newly auto-accepted connection gets, read off func_ov011_0217d1a0 and func_ov011_0217d190's
// one call site each
typedef struct GT2CallbackTable {
    void *onClosed;
    void *onReceived;
    void *onPing;
    void *onSocketError;
} GT2CallbackTable;

static GT2Engine *Gt2_GetEngine(void);
static void Gt2_FreePendingConnect(void *element);
static int Gt2_CompareLocalAddr(const void *a, const void *b);
static int Gt2_CompareRemoteAddr(const void *a, const void *b);
static int Gt2_CompareConnectionAddr(const void *a, const void *b);
static int Gt2_CompareConnectionHandle(const void *a, const void *b);
static void Gt2_OnSocketCreateError(void);
static void Gt2_OnClosed(void *addr, int reason);
static void Gt2_OnConnectAttemptDone(void *addr, int rawResult);
static void Gt2_OnPing(void *addr, int len);
static void Gt2_OnReceivedData(void *addr, const void *data, int len, void *context);
static BOOL Gt2_OnUnknownMessage(void *unused, u32 ip, u16 port, int len, const void *data);
static void Gt2_OnReceivedRaw(void *unused, void *handle, u32 ip, u16 port, void *context, const void *data,
                              int len);

// The one engine this build creates
static GT2Engine sEngine;

// Returns the one engine
static GT2Engine *Gt2_GetEngine(void) {
    return &sEngine;
}

// The freeFunc of the pending-connect array: frees the attempts it was still racing
static void Gt2_FreePendingConnect(void *element) {
    GT2PendingConnect *pending = (GT2PendingConnect *)element;

    func_ov011_02171558(pending->attempts);
}

// Compares the local addresses of two GT2PendingConnect
static int Gt2_CompareLocalAddr(const void *a, const void *b) {
    return memcmp(a, b, 0x10);
}

// Compares the remote addresses of two GT2PendingConnect
static int Gt2_CompareRemoteAddr(const void *a, const void *b) {
    return memcmp((const u8 *)a + 0x10, (const u8 *)b + 0x10, 0x10);
}

// Compares the ip and port of two GT2Connection
static int Gt2_CompareConnectionAddr(const void *a, const void *b) {
    const GT2Connection *ca = (const GT2Connection *)a;
    const GT2Connection *cb = (const GT2Connection *)b;

    if (ca->ip != cb->ip) {
        return 1;
    }
    if (ca->port != cb->port) {
        return 1;
    }
    return 0;
}

// Compares the lower-layer handle of two GT2Connection
static int Gt2_CompareConnectionHandle(const void *a, const void *b) {
    const GT2Connection *ca = (const GT2Connection *)a;
    const GT2Connection *cb = (const GT2Connection *)b;

    return ca->handle != cb->handle;
}

// The callback the lower socket calls on a socket-wide error; notifies the app and every pending connect still
// racing
static void Gt2_OnSocketCreateError(void) {
    int i;
    GT2Engine *engine = Gt2_GetEngine();
    int count;

    func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Socket error, passing to app and message handlers\n");
    if (engine->onSocketError != NULL) {
        engine->onSocketError(3, engine->userData);
    }

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GT2PendingConnect *pending = (GT2PendingConnect *)func_ov011_0217158c(engine->pending, i);
        if (pending->onEvent != NULL) {
            pending->onEvent(3, pending->userData);
        }
    }
}

// Notifies that a connection closed, with its mapped reason, then drops it
static void Gt2_OnClosed(void *addr, int reason) {
    GT2Engine *engine = Gt2_GetEngine();
    int i;
    int count;
    int index;
    GT2Connection key;
    char buf[0x18];

    if (reason == 2 || reason == 3) {
        reason = 2;
    } else if (reason == 4) {
        reason = 3;
    }

    func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connection closed to %s\n",
                        func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GT2PendingConnect *pending = (GT2PendingConnect *)func_ov011_0217158c(engine->pending, i);
        if (pending->onClosed != NULL) {
            func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connection closed: passed to message handler\n");
            pending->onClosed(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, pending->userData);
        }
    }

    if (engine->onClosed != NULL) {
        func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connection closed: passed to app\n");
        engine->onClosed(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, engine->userData);
    }

    key.handle = addr;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionHandle, 0, FALSE);
    if (index != -1) {
        func_ov011_02171668(engine->connections, index);
    }
}

// The callback the lower socket calls when one low-level attempt of a Gt2_Connect call finishes; notifies that
// attempt's own handler and the app, and drops the attempt
static void Gt2_OnConnectAttemptDone(void *addr, int rawResult) {
    GT2Engine *engine = Gt2_GetEngine();
    char buf[0x18];
    int count;
    int reason;
    int i;
    int index;
    GT2Connection connKey;
    GT2Connection attemptKey;

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
        connKey.port = (u16)func_ov011_0217d384(addr);
        index = func_ov011_021716ac(engine->connections, &connKey, Gt2_CompareConnectionAddr, 0, FALSE);
        if (index != -1) {
            func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connect rejected by %s\n",
                                func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));
            func_ov011_02171668(engine->connections, index);
        }
    }

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GT2PendingConnect *pending = (GT2PendingConnect *)func_ov011_0217158c(engine->pending, i);

        attemptKey.ip = func_ov011_0217d380(addr);
        attemptKey.port = (u16)func_ov011_0217d384(addr);
        index = func_ov011_021716ac(pending->attempts, &attemptKey, Gt2_CompareConnectionAddr, 0, FALSE);
        if (index != -1) {
            if (pending->onAttemptResult != NULL) {
                func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Passing connect result to message handler\n");
                pending->onAttemptResult(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason,
                                         rawResult == 2, pending->userData);
            }
            func_ov011_02171668(pending->attempts, index);
            return;
        }
    }

    if (engine->pendingCount > 0) {
        if (engine->onAttemptResult != NULL) {
            func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Passing connect result to app\n");
            engine->onAttemptResult(func_ov011_0217d380(addr), func_ov011_0217d384(addr), reason, rawResult == 2,
                                    engine->userData);
        }
        engine->pendingCount--;
    }
}

// The callback for a ping datagram; notifies the pending connect that is still racing this address, or the app
static void Gt2_OnPing(void *addr, int len) {
    GT2Engine *engine = Gt2_GetEngine();
    char buf[0x18];
    int i;
    int count;

    func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Received ping from %s\n",
                        func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));

    count = func_ov011_02171588(engine->pending);
    for (i = 0; i < count; i++) {
        GT2PendingConnect *pending = (GT2PendingConnect *)func_ov011_0217158c(engine->pending, i);
        if (pending->onPing != NULL) {
            func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Passing to message handler\n");
            pending->onPing(func_ov011_0217d380(addr), func_ov011_0217d384(addr), len, pending->userData);
            return;
        }
    }

    if (engine->onPing != NULL) {
        func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Passing to app\n");
        engine->onPing(func_ov011_0217d380(addr), func_ov011_0217d384(addr), len, engine->userData);
    }
}

// The callback for a received datagram; if its first 16 bytes match a pending connect's remote address, notifies
// that connect's own handler, otherwise the app
static void Gt2_OnReceivedData(void *addr, const void *data, int len, void *context) {
    GT2Engine *engine = Gt2_GetEngine();
    char buf[0x18];
    u8 key[0x20];
    int index;

    func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Received data from %s\n",
                        func_ov011_0217ed14(func_ov011_0217d380(addr), func_ov011_0217d384(addr), buf));

    if (len >= 0x10) {
        memcpy(key, data, 0x10);
        index = func_ov011_021716ac(engine->pending, key, Gt2_CompareRemoteAddr, 0, FALSE);
        if (index != -1) {
            GT2PendingConnect *pending = (GT2PendingConnect *)func_ov011_0217158c(engine->pending, index);
            if (pending->onReceived != NULL) {
                func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Passed to message handler\n");
                pending->onReceived(func_ov011_0217d380(addr), func_ov011_0217d384(addr), (const u8 *)data + 0x10,
                                    len - 0x10, context);
                return;
            }
        }
    }

    if (engine->onReceived != NULL) {
        func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Passed to app\n");
        engine->onReceived(func_ov011_0217d380(addr), func_ov011_0217d384(addr), data, len, context,
                           engine->userData);
    }
}

// The lower socket's callback for a datagram with no matching 16-byte header; passes it straight to the app
static BOOL Gt2_OnUnknownMessage(void *unused, u32 ip, u16 port, int len, const void *data) {
    GT2Engine *engine = Gt2_GetEngine();
    char buf[0x18];
    BOOL handled;

    (void)unused;
    if (engine->onUnknownMessage == NULL) {
        return FALSE;
    }

    handled = FALSE;
    func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Unknown message from %s, passing to app\n",
                        func_ov011_0217ed14(ip, port, buf));
    if (engine->onUnknownMessage(ip, port, len, data, engine->userData) != 0) {
        handled = TRUE;
    }
    return handled;
}

// The lower socket's callback for a received datagram; auto-accepts it into a connection when its header matches a
// pending connect's local address, otherwise asks the app whether to accept it
static void Gt2_OnReceivedRaw(void *unused, void *handle, u32 ip, u16 port, void *context, const void *data,
                              int len) {
    GT2Engine *engine = Gt2_GetEngine();
    char buf[0x18];
    GT2Connection newConnection;
    GT2CallbackTable table;
    int index;

    (void)unused;
    func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connection attempt from %s\n", func_ov011_0217ed14(ip, port, buf));

    if (len >= 0x10) {
        index = func_ov011_021716ac(engine->pending, data, Gt2_CompareLocalAddr, 0, FALSE);
        if (index != -1) {
            table.onClosed = (void *)Gt2_OnClosed;
            table.onReceived = (void *)Gt2_OnReceivedData;
            table.onPing = (void *)Gt2_OnPing;
            table.onSocketError = (void *)Gt2_OnConnectAttemptDone;
            func_ov011_0217d190(handle, &table);

            newConnection.ip = ip;
            newConnection.port = port;
            newConnection.handle = handle;
            func_ov011_021715a4(engine->connections, &newConnection);

            func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connection attempt auto-accepted for message handler\n");
            return;
        }
    }

    if (engine->onUnroutable != NULL) {
        func_ov011_0216cca8(0xc, 0, 0x10, "[Udp Engine] Connection attempt from %s, asking app to accept/reject\n",
                            func_ov011_0217ed14(ip, port, buf));
        engine->onUnroutable(ip, port, data, len, context, engine->userData);
        return;
    }

    func_ov011_0217d198(context, 0, 0);
    func_ov011_02171634(engine->connections, func_ov011_02171588(engine->connections) - 1);
}

// The engine's own maxConnections, which doubles as whether it was created
int Gt2_GetMaxConnections(void) {
    return Gt2_GetEngine()->maxConnections;
}

// Creates the one engine, binding its socket to localAddr
int Gt2_CreateSocket(const char *localAddr, int maxConnections, int connectionsGrowBy,
                     Gt2EventCallback onSocketError, Gt2AttemptResultCallback onAttemptResult,
                     Gt2ClosedCallback onClosed, Gt2PingCallback onPing, Gt2ReceivedCallback onReceived,
                     Gt2UnknownMessageCallback onUnknownMessage, Gt2ReceivedCallback onUnroutable, void *userData) {
    GT2Engine *engine = Gt2_GetEngine();
    char addrBuf[0x18];
    int error;

    if (maxConnections == 0) {
        maxConnections = 0x5dc;
    }
    if (connectionsGrowBy == 0) {
        connectionsGrowBy = 0x5b4;
    }

    engine->onSocketError = onSocketError;
    engine->onUnknownMessage = onUnknownMessage;
    engine->onAttemptResult = onAttemptResult;
    engine->onClosed = onClosed;
    engine->onPing = onPing;
    engine->onReceived = onReceived;
    engine->onUnroutable = onUnroutable;

    func_ov011_0217ed14(0, (u32)localAddr, addrBuf);
    error = func_ov011_0217d144(engine, addrBuf, connectionsGrowBy, maxConnections, Gt2_OnSocketCreateError);
    if (error != 0) {
        func_ov011_0216cca8(0xc, 0, 2, "[Udp Engine] error creating gt2 socket, error code: %d\n", error);
        return 3;
    }

    engine->connections = func_ov011_02171514(0xc, 1, NULL);
    if (engine->connections == NULL) {
        func_ov011_0216cca8(0xc, 2, 1, "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    engine->pending = func_ov011_02171514(0x3c, 1, Gt2_FreePendingConnect);
    if (engine->pending == NULL) {
        func_ov011_0216cca8(0xc, 2, 1, "[Udp Engine] No more memory!!!\n");
        return 1;
    }

    func_ov011_0217d3a0(engine->socket, Gt2_OnUnknownMessage);
    func_ov011_0217d188(engine->socket, Gt2_OnReceivedRaw);
    engine->localIp = func_ov011_0217d388(engine->socket);
    engine->localPort = (u16)func_ov011_0217d38c(engine->socket);
    engine->pendingCount = 0;
    engine->maxConnections = maxConnections;
    engine->userData = userData;
    return 0;
}

// The lower-layer state of the connection to an address, written to *state
int Gt2_GetConnectionState(u32 ip, u16 port, int *state) {
    GT2Engine *engine = Gt2_GetEngine();
    GT2Connection key;
    GT2Connection *conn;
    int index;

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 3, 0xf, "[Udp Engine] Engine not initialized\n");
        *state = 3;
        return 0xb;
    }

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, FALSE);
    if (index == -1) {
        *state = 3;
        return 0;
    }
    conn = (GT2Connection *)func_ov011_0217158c(engine->connections, index);
    *state = func_ov011_0217d364(conn->handle);
    return 0;
}

// Opens a connection to an address the app already knows is valid, racing it against any existing one, or creates
// one through the lower layer when none exists yet
int Gt2_OpenConnection(u32 ip, u16 port, const void *header, void *context) {
    GT2Engine *engine = Gt2_GetEngine();
    char buf[0x40];
    GT2Connection key;
    GT2Connection *conn;
    GT2CallbackTable table;
    void *outHandle;
    int index;
    int state;

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (ip == 0 || port == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Invalid parameter(s), check ip, port");
        return 0xa;
    }

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, FALSE);
    if (index != -1) {
        conn = (GT2Connection *)func_ov011_0217158c(engine->connections, index);
        state = func_ov011_0217d364(conn->handle);
        if (state == 1) {
            func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine is already talking to remote address\n");
            return 5;
        }
        if (state != 0) {
            return 0;
        }

        index = func_ov011_021716ac(engine->pending, header, Gt2_CompareLocalAddr, 0, FALSE);
        if (index == -1) {
            return 0;
        }
        func_ov011_021716ac(((GT2PendingConnect *)func_ov011_0217158c(engine->pending, index))->attempts, &key,
                            Gt2_CompareConnectionAddr, 0, FALSE);
        return 0;
    }

    func_ov011_0217ed14(ip, port, buf);
    table.onClosed = (void *)Gt2_OnClosed;
    table.onReceived = (void *)Gt2_OnReceivedData;
    table.onPing = (void *)Gt2_OnPing;
    table.onSocketError = (void *)Gt2_OnConnectAttemptDone;
    func_ov011_0217d1a0(engine->socket, &outHandle, buf, header, 0x10, context, &table, 0);

    key.handle = outHandle;
    func_ov011_021715a4(engine->connections, &key);
    return 0;
}

// Sends a message to the connection of an address, with a 16-byte header prepended
int Gt2_Send(u32 ip, u16 port, const void *header, const void *message, int size, u32 flags) {
    GT2Engine *engine = Gt2_GetEngine();
    GT2Connection key;
    char buf[0x18];
    GT2Connection *conn;
    void *packet;
    int index;
    int maxSize;
    int total;
    const u8 *src;
    u8 *dest;
    int n;

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 3;
    }

    total = (((const char *)header)[0] != 0) ? size + 0x10 : size;

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, FALSE);
    if (index == -1) {
        // BUG: The formatted address is computed but the format string has no %s to print it
#ifdef BUGFIX
        func_ov011_0216cca8(0xc, 0, 2, "[Udp Engine] address not found for sending message: %s\n",
                            func_ov011_0217ed14(ip, port, buf));
#else
        func_ov011_0216cca8(0xc, 0, 2, "[Udp Engine] address not found for sending message\n",
                            func_ov011_0217ed14(ip, port, buf));
#endif
        return 4;
    }

    conn = (GT2Connection *)func_ov011_0217158c(engine->connections, index);
    maxSize = func_ov011_0217d390(conn->handle);
    if (total > maxSize && flags != 0) {
        func_ov011_0216cca8(0xc, 0, 2, "[Udp Engine] Message Size too large, dropping message\n");
        return 0xc;
    }

    packet = func_ov011_0216dddc(total);
    src = (const u8 *)header;
    dest = (u8 *)packet;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);
    memcpy((u8 *)packet + 0x10, message, size);
    index = func_ov011_0217d290(conn->handle, packet, total, flags);
    func_ov011_0216ddfc(packet);
    return (index != 0) ? 8 : 0;
}

// Stops the engine's socket without freeing it
int Gt2_StopSocket(void) {
    GT2Engine *engine = Gt2_GetEngine();

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    func_ov011_0217d168(engine->socket);
    return 0;
}

// Tears the engine down entirely
int Gt2_CloseSocket(void) {
    GT2Engine *engine = Gt2_GetEngine();

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    func_ov011_0217d158(engine->socket);
    func_ov011_02171558(engine->pending);
    func_ov011_02171558(engine->connections);
    engine->maxConnections = 0;
    return 0;
}

// The engine's own bound local port
u16 Gt2_GetLocalPort(void) {
    GT2Engine *engine = Gt2_GetEngine();

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 1, "[Udp Engine] Engine not initialized\n");
        return 0;
    }
    return engine->localPort;
}

// Starts a connection attempt, racing a primary and a fallback address
int Gt2_Connect(const char *localAddr, const char *remoteAddr, Gt2EventCallback onEvent,
                Gt2AttemptResultCallback onAttemptResult, Gt2ClosedCallback onClosed, Gt2PingCallback onPing,
                Gt2PendingDataCallback onReceived, void *userData) {
    GT2Engine *engine = Gt2_GetEngine();
    GT2PendingConnect pending;
    void *attempts;
    const u8 *src;
    u8 *dest;
    int n;

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (localAddr[0] == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Invalid init message\n");
        return 0xa;
    }
    if (remoteAddr[0] == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Invalid header\n");
        return 0xa;
    }

    src = (const u8 *)localAddr;
    dest = pending.localAddr;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);
    src = (const u8 *)remoteAddr;
    dest = pending.remoteAddr;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);

    attempts = func_ov011_02171514(4, 1, NULL);
    if (attempts == NULL) {
        func_ov011_0216cca8(0xc, 2, 1, "[Udp Engine] No more memory!!!\n");
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

// Cancels the connect attempts still pending to a remote address
int Gt2_CancelConnect(const char *remoteAddr) {
    GT2Engine *engine = Gt2_GetEngine();
    u8 key[0x10];
    const u8 *src;
    u8 *dest;
    int n;
    int index;

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 3;
    }
    if (remoteAddr == NULL || remoteAddr[0] == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Invalid or empty header\n");
        return 0xa;
    }

    src = (const u8 *)remoteAddr;
    dest = key;
    n = 0x10;
    do {
        *dest++ = *src++;
    } while (--n != 0);

    index = func_ov011_021716ac(engine->pending, key, Gt2_CompareRemoteAddr, 0, FALSE);
    if (index != -1) {
        func_ov011_02171668(engine->pending, index);
    }
    return 0;
}

// Whether there are no pending connects left
BOOL Gt2_IsIdle(void) {
    GT2Engine *engine = Gt2_GetEngine();

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return TRUE;
    }
    return func_ov011_02171588(engine->pending) == 0;
}

// Whether Gt2_CreateSocket was given none of its app-wide callbacks
BOOL Gt2_HasNoAppCallbacks(void) {
    GT2Engine *engine = Gt2_GetEngine();

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return TRUE;
    }
    if (engine->onClosed != NULL || engine->onUnroutable != NULL || engine->onAttemptResult != NULL ||
        engine->onSocketError != NULL || engine->onPing != NULL || engine->onReceived != NULL ||
        engine->onUnknownMessage != NULL) {
        return FALSE;
    }
    return TRUE;
}

// Closes an established connection
int Gt2_Close(u32 ip, u16 port) {
    GT2Engine *engine = Gt2_GetEngine();
    GT2Connection key;
    GT2Connection *conn;
    int index;

    if (engine->maxConnections == 0) {
        func_ov011_0216cca8(0xc, 0, 0xf, "[Udp Engine] Engine not initialized\n");
        return 0;
    }

    key.ip = ip;
    key.port = port;
    index = func_ov011_021716ac(engine->connections, &key, Gt2_CompareConnectionAddr, 0, FALSE);
    if (index != -1) {
        conn = (GT2Connection *)func_ov011_0217158c(engine->connections, index);
        return func_ov011_0217d394(conn->handle);
    }
    return 0;
}

// A wide-character string's length
int Gt2_WStrLen(const u16 *str) {
    const u16 *p = str;

    while (*p++ != 0) {
    }
    return p - str - 1;
}
