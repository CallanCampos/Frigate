#ifndef POKEBW2_DWC_GT2MAIN_H
#define POKEBW2_DWC_GT2MAIN_H

#include "dwc/darray.h"
#include "dwc/gsPlatformSocket.h"
#include "dwc/hashtable.h"
#include "dwc/nonport.h"

// GameSpy's GT2 (gt2Main.c), in overlay 11: the API of its UDP transport, which gsUdpEngine.c's "Udp Engine" and DWC's
// matchmaking and transport talk through, with the layouts of a GT2 socket and connection. A connection's messages
// start with the bytes 0xfe 0xfe and a type: 0 is the app's reliable data, 1 to 7 the reliable handshake (the client's
// challenge, the server's challenge, the client's response, accept, reject, close and keep-alive), and 100 to 104
// unreliable acks, nacks, pings, pongs and the note that a connection is closed. The ROM has no names for the files or
// their functions: the files are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Main.s and the other gt2*.s), and so are the functions, after GameSpy's source. The types
// take the gti2 prefix of the functions inside GT2.
//
// A connection's state goes 0 (waiting for the server's challenge) and 1 (waiting to be accepted) on the side that
// connects, 2 (waiting for the client's challenge), 3 (waiting for the client's response) and 4 (waiting for the app to
// accept or reject it) on the side that listens, then 5 (connected), 6 (closing) and 7 (closed). The results the
// functions return are 0 (success), 1 (out of memory), 2 (rejected), 3 (a network error), 4 (a bad address), 5 (the
// address already has a connection), 6 (timed out), 7 (the handshake failed), 8 (not connected), 9 (a bad message) and
// 10 (sending failed); a connection closes for 0 (closed here), 1 (closed by the peer), 2 (a protocol error), 3 (a
// socket error) or 4 (out of memory).

// The byte order of the network, which the sockets take ports and addresses in
#define GTI2_SWAP16(x) ((gsi_u16)((gsi_u8)((x) >> 8) | ((x) << 8 & 0xff00)))
#define GTI2_SWAP32(x) ((gsi_u8)((x) >> 24) | ((x) >> 8 & 0xff00) | ((x) << 8 & 0xff0000) | ((x) << 24 & 0xff000000))

// GameSpy's public types of GT2 (gt2.h)
typedef int GT2Bool;
#define GT2False 0
#define GT2True 1

typedef unsigned char GT2Byte;

typedef struct GTI2Socket *GT2Socket;
typedef struct GTI2Connection *GT2Connection;

typedef unsigned short GT2MessageID;

typedef enum {
    GT2Success,
    GT2OutOfMemory,
    GT2Rejected,
    GT2NetworkError,
    GT2AddressError,
    GT2DuplicateAddress,
    GT2TimedOut,
    GT2NegotiationError,
    GT2InvalidConnection,
    GT2InvalidMessage,
    GT2SendFailed
} GT2Result;

typedef enum { GT2Connecting, GT2Connected, GT2Closing, GT2Closed } GT2ConnectionState;

typedef enum {
    GT2LocalClose,
    GT2RemoteClose,
    GT2CommunicationError,
    GT2SocketError,
    GT2NotEnoughMemory
} GT2CloseReason;

// A socket's callbacks: an error of the socket, a connection attempt from a peer (with the milliseconds the handshake
// took and the attempt's message), every datagram sent or received (reset when the peer's port was unreachable),
// and a datagram that isn't GT2's, which returns whether the app handled it
typedef void (*gt2SocketErrorCallback)(GT2Socket socket);
typedef void (*gt2ConnectAttemptCallback)(GT2Socket socket, GT2Connection connection, unsigned int ip,
                                          unsigned short port, int latency, GT2Byte *message, int len);
typedef void (*gt2DumpCallback)(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port,
                                GT2Bool reset, const GT2Byte *message, int len);
typedef GT2Bool (*gt2UnrecognizedMessageCallback)(GT2Socket socket, unsigned int ip, unsigned short port,
                                                  GT2Byte *message, int len);

// A connection's callbacks: the result of its connect attempt (with the peer's message for a rejection), a message
// received, the connection closed (with the reason), and the latency of a ping in milliseconds
typedef void (*gt2ConnectedCallback)(GT2Connection connection, GT2Result result, GT2Byte *message, int len);
typedef void (*gt2ReceivedCallback)(GT2Connection connection, GT2Byte *message, int len, GT2Bool reliable);
typedef void (*gt2ClosedCallback)(GT2Connection connection, GT2CloseReason reason);
typedef void (*gt2PingCallback)(GT2Connection connection, int latency);

typedef struct {
    gt2ConnectedCallback connected;
    gt2ReceivedCallback received;
    gt2ClosedCallback closed;
    gt2PingCallback ping;
} GT2ConnectionCallbacks;

// The filters a connection passes the messages it sends and receives through
typedef void (*gt2SendFilterCallback)(GT2Connection connection, int filterID, const GT2Byte *message, int len,
                                      GT2Bool reliable);
typedef void (*gt2ReceiveFilterCallback)(GT2Connection connection, int filterID, GT2Byte *message, int len,
                                         GT2Bool reliable);

// A buffer of bytes: length bytes of capacity are used
typedef struct GTI2Buffer {
    GT2Byte *buffer;
    int size;
    int len;
} GTI2Buffer;

// A connection's state inside GT2, and the types of its messages
typedef enum {
    GTI2AwaitingServerChallenge,
    GTI2AwaitingAcceptance,
    GTI2AwaitingClientChallenge,
    GTI2AwaitingClientResponse,
    GTI2AwaitingAcceptReject,
    GTI2Connected,
    GTI2Closing,
    GTI2Closed
} GTI2ConnectionState;

typedef enum {
    GTI2MsgAppReliable,
    GTI2MsgClientChallenge,
    GTI2MsgServerChallenge,
    GTI2MsgClientResponse,
    GTI2MsgAccept,
    GTI2MsgReject,
    GTI2MsgClose,
    GTI2MsgKeepAlive,
    GTI2NumReliableMessages,
    GTI2MsgAck = 100,
    GTI2MsgNack,
    GTI2MsgPing,
    GTI2MsgPong,
    GTI2MsgClosed
} GTI2MessageType;

// What a socket sends over: UDP, or UDP with a length before each message (VDP), or ad hoc
typedef enum { GTI2UdpProtocol, GTI2VdpProtocol = 2, GTI2AdHocProtocol = 3 } GTI2ProtocolType;

typedef struct GTI2Socket {
    SOCKET socket; // the socket's descriptor
    unsigned int ip;
    unsigned short port;
    HashTable connections;    // of GT2Connection, by address
    DArray closedConnections; // the GT2Connection that closed and wait to be freed
    GT2Bool close;            // closed inside a callback, so it is freed once the callbacks return
    GT2Bool error;
    int callbackLevel; // how many callbacks are running
    gt2ConnectAttemptCallback connectAttemptCallback;
    gt2SocketErrorCallback socketErrorCallback;
    gt2DumpCallback sendDumpCallback;
    gt2DumpCallback receiveDumpCallback;
    gt2UnrecognizedMessageCallback unrecognizedMessageCallback;
    void *data; // the app's data
    int outgoingBufferSize;
    int incomingBufferSize;
    GTI2ProtocolType
        protocolType;   // VDP puts a 2-byte length before every message, ad hoc isn't bound to the local address
    int protocolOffset; // the bytes before GT2's own header: the protocol, or 0 for ad hoc
    GT2Bool broadcastEnabled;
} GTI2Socket;

// A reliable message that arrived ahead of its turn and waits in the incoming buffer
typedef struct GTI2IncomingBufferMessage {
    int start;
    int len;
    GTI2MessageType type;
    unsigned short serialNumber;
} GTI2IncomingBufferMessage;

// A reliable message that was sent and waits in the outgoing buffer for the peer's ack
typedef struct GTI2OutgoingBufferMessage {
    int start;
    int len;
    unsigned short serialNumber;
    gsi_time lastSend;
} GTI2OutgoingBufferMessage;

typedef struct GTI2Connection {
    unsigned int ip;
    unsigned short port;
    GTI2Socket *socket;
    GTI2ConnectionState state;
    GT2Bool initiated;          // opened from this side
    GT2Bool freeAtAcceptReject; // closed while the app decides, so it is freed once it has
    GT2Result connectionResult; // the result of the connect attempt
    gsi_time startTime;
    gsi_time timeout; // in milliseconds, 0 for none
    int callbackLevel;
    GT2ConnectionCallbacks callbacks;
    char *initialMessage; // the message the connect attempt sends
    int initialMessageLen;
    void *data;
    GTI2Buffer incomingBuffer;
    GTI2Buffer outgoingBuffer;
    DArray incomingBufferMessages;       // GTI2IncomingBufferMessage
    DArray outgoingBufferMessages;       // GTI2OutgoingBufferMessage
    unsigned short serialNumber;         // of the next reliable message sent
    unsigned short expectedSerialNumber; // of the next reliable message to receive
    char response[0x20];                 // the response the peer must give to this side's challenge
    gsi_time lastSend;
    gsi_time challengeTime; // when the server's challenge was sent, which the attempt's latency counts from
    GT2Bool pendingAck;
    gsi_time pendingAckTime;
    DArray sendFilters;    // gt2SendFilterCallback
    DArray receiveFilters; // gt2ReceiveFilterCallback
} GTI2Connection;

// gt2CreateSocket, bound to a local address ("ip:port", or ":port"), with the buffer sizes of its connections (0 for
// 64 KB); gt2CloseSocket and gt2Think, which receives, runs every connection's timers and frees the closed ones
GT2Result gt2CreateSocket(GT2Socket *socket, const char *localAddress, int outgoingBufferSize, int incomingBufferSize,
                          gt2SocketErrorCallback onSocketError);
void gt2CloseSocket(GT2Socket socket);
void gt2Think(GT2Socket socket);
// gt2Listen, which takes connection attempts with the callback (none without it), and gt2Accept and gt2Reject, for a
// connection waiting for the app; gt2Accept returns gsi_false unless it was
void gt2Listen(GT2Socket socket, gt2ConnectAttemptCallback callback);
GT2Bool gt2Accept(GT2Connection connection, GT2ConnectionCallbacks *callbacks);
void gt2Reject(GT2Connection connection, const GT2Byte *message, int length);
// gt2Connect, to "host:port" with a message for the peer: blocking, it runs the socket until the attempt ends and
// returns its result, otherwise it returns once the attempt started
GT2Result gt2Connect(GT2Socket socket, GT2Connection *connection, const char *remoteAddress, const GT2Byte *message,
                     int length, int timeout, GT2ConnectionCallbacks *callbacks, GT2Bool blocking);
// gt2Send; a length of -1 sends a text with its end
GT2Result gt2Send(GT2Connection connection, const GT2Byte *message, int length, GT2Bool reliable);
// Close at once without waiting for the peer
void gt2CloseConnectionHard(GT2Connection connection);
void gt2CloseAllConnectionsHard(GT2Socket socket);
// The connection's state as the app sees it: 0 connecting, 1 connected, 2 closing, 3 closed
GT2ConnectionState gt2GetConnectionState(GT2Connection connection);
// The peer's ip and port, the socket's own ip and port (gt2GetLocalPort), and the size and free space of the
// connection's outgoing buffer (gt2GetOutgoingBufferFreeSpace)
unsigned int gt2GetRemoteIP(GT2Connection connection);
unsigned short gt2GetRemotePort(GT2Connection connection);
unsigned int gt2GetLocalIP(GT2Socket socket);
unsigned short gt2GetLocalPort(GT2Socket socket);
int gt2GetOutgoingBufferSize(GT2Connection connection);
int gt2GetOutgoingBufferFreeSpace(GT2Connection connection);
SOCKET gt2GetSocketSOCKET(GT2Socket socket);
// gt2SetUnrecognizedMessageCallback
void gt2SetUnrecognizedMessageCallback(GT2Socket socket, gt2UnrecognizedMessageCallback callback);
void gt2SetConnectionData(GT2Connection connection, void *userData);
void *gt2GetConnectionData(GT2Connection connection);

#endif // POKEBW2_DWC_GT2MAIN_H
