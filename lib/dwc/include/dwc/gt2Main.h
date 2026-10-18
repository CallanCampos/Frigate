#ifndef POKEBW2_DWC_GT2MAIN_H
#define POKEBW2_DWC_GT2MAIN_H

#include "types.h"
#include "dwc/darray.h"
#include "dwc/gt2Buffer.h"
#include "dwc/hashtable.h"

// GameSpy's GT2 (gt2Main.c), in overlay 11: the API of its UDP transport, which gt2.c's "Udp Engine" and DWC's
// matchmaking and transport talk through, with the layouts of a GT2 socket and connection. A connection's messages
// start with the bytes 0xfe 0xfe and a type: 0 is the app's reliable data, 1 to 7 the reliable handshake (the client's
// challenge, the server's challenge, the client's response, accept, reject, close and keep-alive), and 100 to 104
// unreliable acks, nacks, pings, pongs and the note that a connection is closed. The ROM has no names for the files or
// their functions: the files are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Main.s and the other gt2*.s), and so are the functions in the comments beside them, while
// the symbols keep their addresses. The types take the gti2 prefix of the functions inside GT2.
//
// A connection's state goes 0 (waiting for the server's challenge) and 1 (waiting to be accepted) on the side that
// connects, 2 (waiting for the client's challenge), 3 (waiting for the client's response) and 4 (waiting for the app to
// accept or reject it) on the side that listens, then 5 (connected), 6 (closing) and 7 (closed). The results the
// functions return are 0 (success), 1 (out of memory), 2 (rejected), 3 (a network error), 4 (a bad address), 5 (the
// address already has a connection), 6 (timed out), 7 (the handshake failed), 8 (not connected), 9 (a bad message) and
// 10 (sending failed); a connection closes for 0 (closed here), 1 (closed by the peer), 2 (a protocol error), 3 (a
// socket error) or 4 (out of memory).

// The byte order of the network, which the sockets take ports and addresses in
#define GTI2_SWAP16(x) ((u16)((u8)((x) >> 8) | ((x) << 8 & 0xff00)))
#define GTI2_SWAP32(x) ((u8)((x) >> 24) | ((x) >> 8 & 0xff00) | ((x) << 8 & 0xff0000) | ((x) << 24 & 0xff000000))

typedef struct GTI2Socket GTI2Socket;
typedef struct GTI2Connection GTI2Connection;

// A socket's callbacks: an error of the socket, a connection attempt from a peer (with the milliseconds the handshake
// took and the attempt's message), every datagram sent or received (reset when the peer's port was unreachable),
// and a datagram that isn't GT2's, which returns whether the app handled it
typedef void (*GTI2SocketErrorCallback)(GTI2Socket *socket);
typedef void (*GTI2ConnectAttemptCallback)(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port,
                                           u32 latency, const void *message, int length);
typedef void (*GTI2DumpCallback)(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, BOOL reset,
                                 const void *message, int length);
typedef BOOL (*GTI2UnrecognizedMessageCallback)(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length);

// A connection's callbacks: the result of its connect attempt (with the peer's message for a rejection), a message
// received, the connection closed (with the reason), and the latency of a ping in milliseconds
typedef struct GTI2ConnectionCallbacks {
    void (*onConnected)(GTI2Connection *connection, int result, const void *message, int length);
    void (*onReceived)(GTI2Connection *connection, const void *message, int length, BOOL reliable);
    void (*onClosed)(GTI2Connection *connection, int reason);
    void (*onPing)(GTI2Connection *connection, u32 latency);
} GTI2ConnectionCallbacks;

// The filters a connection passes the messages it sends and receives through
typedef void (*GTI2FilterCallback)(GTI2Connection *connection, int filterId, const void *message, int length,
                                   BOOL reliable);

struct GTI2Socket {
    int handle; // the socket's descriptor
    u32 ip;
    u16 port;
    HashTable *connections;    // of GTI2Connection *, by address
    DArray *closedConnections; // the GTI2Connection * that closed and wait to be freed
    BOOL close;                // closed inside a callback, so it is freed once the callbacks return
    BOOL error;
    int callbackLevel; // how many callbacks are running
    GTI2ConnectAttemptCallback onConnectAttempt;
    GTI2SocketErrorCallback onSocketError;
    GTI2DumpCallback onSendDump;
    GTI2DumpCallback onReceiveDump;
    GTI2UnrecognizedMessageCallback onUnrecognizedMessage;
    u8 unk_34[4];
    int outgoingBufferSize;
    int incomingBufferSize;
    int protocol;       // 2 puts a 2-byte length before every message, 3 isn't bound to the local address
    int protocolOffset; // the bytes before GT2's own header: the protocol, or 0 for protocol 3
    u8 unk_48[4];
};

// A reliable message that arrived ahead of its turn and waits in the incoming buffer
typedef struct GTI2IncomingMessage {
    int offset;
    int length;
    int type;
    u16 serialNumber;
} GTI2IncomingMessage;

// A reliable message that was sent and waits in the outgoing buffer for the peer's ack
typedef struct GTI2OutgoingMessage {
    int offset;
    int length;
    u16 serialNumber;
    u32 sendTime;
} GTI2OutgoingMessage;

struct GTI2Connection {
    u32 ip;
    u16 port;
    GTI2Socket *socket;
    int state;
    BOOL initiated;          // opened from this side
    BOOL freeAtAcceptReject; // closed while the app decides, so it is freed once it has
    int result;              // the result of the connect attempt
    u32 startTime;
    u32 timeout; // in milliseconds, 0 for none
    int callbackLevel;
    GTI2ConnectionCallbacks callbacks;
    u8 *initialMessage; // the message the connect attempt sends
    int initialMessageLength;
    void *userData;
    GTI2Buffer incoming;
    GTI2Buffer outgoing;
    DArray *incomingMessages; // GTI2IncomingMessage
    DArray *outgoingMessages; // GTI2OutgoingMessage
    u16 serialNumber;         // of the next reliable message sent
    u16 expectedSerialNumber; // of the next reliable message to receive
    u8 response[0x20];        // the response the peer must give to this side's challenge
    u32 lastSendTime;
    u32 challengeTime; // when the server's challenge was sent, which the attempt's latency counts from
    BOOL pendingAck;
    u32 pendingAckTime;
    DArray *sendFilters;    // GTI2FilterCallback
    DArray *receiveFilters; // GTI2FilterCallback
};

// gt2CreateSocket, bound to a local address ("ip:port", or ":port"), with the buffer sizes of its connections (0 for
// 64 KB); gt2CloseSocket and gt2Think, which receives, runs every connection's timers and frees the closed ones
int func_ov011_0217d144(GTI2Socket **socket, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                        GTI2SocketErrorCallback onSocketError);
void func_ov011_0217d158(GTI2Socket *socket);
void func_ov011_0217d168(GTI2Socket *socket);
// gt2Listen, which takes connection attempts with the callback (none without it), and gt2Accept and gt2Reject, for a
// connection waiting for the app; gt2Accept returns FALSE unless it was
void func_ov011_0217d188(GTI2Socket *socket, GTI2ConnectAttemptCallback callback);
BOOL func_ov011_0217d190(GTI2Connection *connection, const GTI2ConnectionCallbacks *callbacks);
void func_ov011_0217d198(GTI2Connection *connection, const void *message, int length);
// gt2Connect, to "host:port" with a message for the peer: blocking, it runs the socket until the attempt ends and
// returns its result, otherwise it returns once the attempt started
int func_ov011_0217d1a0(GTI2Socket *socket, GTI2Connection **connection, const char *remoteAddress, const void *message,
                        int length, u32 timeout, const GTI2ConnectionCallbacks *callbacks, BOOL blocking);
// gt2Send; a length of -1 sends a text with its end
int func_ov011_0217d290(GTI2Connection *connection, const void *message, int length, BOOL reliable);
// gt2CloseConnectionHard and gt2CloseAllConnectionsHard, which close at once without waiting for the peer
void func_ov011_0217d338(GTI2Connection *connection);
void func_ov011_0217d350(GTI2Socket *socket);
// The connection's state as the app sees it: 0 connecting, 1 connected, 2 closing, 3 closed
int func_ov011_0217d364(GTI2Connection *connection);
// The peer's ip and port, the socket's own ip and port (gt2GetLocalPort), and the size and free space of the
// connection's outgoing buffer (gt2GetOutgoingBufferFreeSpace)
u32 func_ov011_0217d380(GTI2Connection *connection);
u16 func_ov011_0217d384(GTI2Connection *connection);
u32 func_ov011_0217d388(GTI2Socket *socket);
u16 func_ov011_0217d38c(GTI2Socket *socket);
int func_ov011_0217d390(GTI2Connection *connection);
int func_ov011_0217d394(GTI2Connection *connection);
// gt2GetSocketSOCKET, gt2SetUnrecognizedMessageCallback, gt2SetConnectionData and gt2GetConnectionData
int func_ov011_0217d39c(GTI2Socket *socket);
void func_ov011_0217d3a0(GTI2Socket *socket, GTI2UnrecognizedMessageCallback callback);
void func_ov011_0217d3a4(GTI2Connection *connection, void *userData);
void *func_ov011_0217d3a8(GTI2Connection *connection);

#endif // POKEBW2_DWC_GT2MAIN_H
