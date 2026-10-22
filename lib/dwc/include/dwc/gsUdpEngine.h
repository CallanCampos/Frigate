#ifndef POKEBW2_DWC_GSUDPENGINE_H
#define POKEBW2_DWC_GSUDPENGINE_H

#include "dwc/nonport.h"

// GameSpy's UDP engine (gsUdpEngine.c, a file of GameSpy's common code), which GP's peers talk through over GameSpy's
// GT2 (dwc/gt2Main.h): its debug texts start with "[Udp Engine]" as GameSpy's do, and its types are GameSpy's
// gsUdpEngine.h's. The file and its functions are GameSpy's own, as its common/gsUdpEngine.c has them, and so are
// the fields' names where the code shows them (ours elsewhere). There is one global engine, which is why no function
// below takes a socket handle.

// The length of a message handler's init message and header
#define GS_UDP_MSG_HEADER_LEN 16

typedef enum _GSUdpErrorCode {
    GS_UDP_NO_ERROR,
    GS_UDP_NO_MEMORY,
    GS_UDP_REJECTED,
    GS_UDP_NETWORK_ERROR,
    GS_UDP_ADDRESS_ERROR,
    GS_UDP_ADDRESS_ALREADY_IN_USE,
    GS_UDP_TIMED_OUT,
    GS_UDP_REMOTE_ERROR,
    GS_UDP_SEND_FAILED,
    GS_UDP_INVALID_MESSAGE,
    GS_UDP_PARAMETER_ERROR,
    GS_UDP_NOT_INITIALIZED,
    GS_UDP_MSG_TOO_BIG,
    GS_UDP_UNKNOWN_ERROR,
    GS_UDP_NUM_ERROR_CODES
} GSUdpErrorCode;

typedef enum _GSUdpPeerState {
    GS_UDP_PEER_CONNECTING,
    GS_UDP_PEER_CONNECTED,
    GS_UDP_PEER_CLOSING,
    GS_UDP_PEER_CLOSED,
    GS_UDP_PEER_STATE_NUM
} GSUdpPeerState;

typedef enum _GSUdpCloseReason {
    GS_UDP_CLOSED_LOCALLY,
    GS_UDP_CLOSED_REMOTELY,
    GS_UDP_CLOSED_BY_COMM_ERROR,
    GS_UDP_CLOSED_BY_LOW_MEM,
    GS_UDP_CLOSED_NUM
} GSUdpCloseReason;

// Notified of an error of the socket, with the engine's or a message handler's user data
typedef void (*gsUdpErrorCallback)(GSUdpErrorCode code, void *userData);
// An incoming attempt not claimed by a message handler, with its latency, init message and user data
typedef void (*gsUdpAppConnectAttemptCallback)(unsigned int ip, unsigned short port, int latency,
                                               unsigned char *initMsg, unsigned int initMsgLen, void *userData);
// A connection that closed, and why
typedef void (*gsUdpConnClosedCallback)(unsigned int ip, unsigned short port, GSUdpCloseReason reason, void *userData);
// A received message, whether it is reliable, and the user data. The length is an int in BW2's engine, where the later
// SDK's is unsigned: gpiPeerMessageCallback compares it signed
typedef void (*gsUdpConnReceivedDataCallback)(unsigned int ip, unsigned short port, unsigned char *message,
                                              int messageLength, gsi_bool reliable, void *userData);
// A connect attempt that just finished, its error and whether the peer rejected it
typedef void (*gsUdpConnConnectedCallback)(unsigned int ip, unsigned short port, GSUdpErrorCode error,
                                           gsi_bool rejected, void *userData);
// A ping's latency in milliseconds
typedef void (*gsUdpConnPingCallback)(unsigned int ip, unsigned short port, unsigned int latency, void *userData);
// A datagram that isn't GT2's; returns whether the app handled it
typedef gsi_bool (*gsUdpUnknownMsgCallback)(unsigned int ip, unsigned short port, unsigned char *message,
                                            unsigned int messageLength, void *userData);

// Creates the one engine with its app-wide callbacks; message handlers can supply their own.
// Buffer sizes are bytes; zero selects 1500 incoming or 1460 outgoing bytes
GSUdpErrorCode gsUdpEngineInitialize(unsigned short localPort, int incomingBufferSize, int outgoingBufferSize,
                                     gsUdpErrorCallback onSocketError, gsUdpConnConnectedCallback onAttemptResult,
                                     gsUdpConnClosedCallback onClosed, gsUdpConnPingCallback onPing,
                                     gsUdpConnReceivedDataCallback onReceived, gsUdpUnknownMsgCallback onUnknownMessage,
                                     gsUdpAppConnectAttemptCallback onUnhandledAttempt, void *userData);
// gsUdpEngineThink, and gsUdpEngineShutdown, the full teardown
GSUdpErrorCode gsUdpEngineThink(void);
GSUdpErrorCode gsUdpEngineShutdown(void);

// gsUdpEngineIsInitialized, and gsUdpEngineGetLocalPort
gsi_bool gsUdpEngineIsInitialized(void);
unsigned short gsUdpEngineGetLocalPort(void);
// Whether there are no message handlers left
gsi_bool gsUdpEngineNoMoreMsgHandlers(void);
// Whether gsUdpEngineInitialize was given none of its app-wide callbacks
gsi_bool gsUdpEngineNoApp(void);

// Registers a message handler; initMessage and header each point to 16 readable bytes with
// a nonzero first byte
GSUdpErrorCode gsUdpEngineAddMsgHandler(char initMessage[GS_UDP_MSG_HEADER_LEN], char header[GS_UDP_MSG_HEADER_LEN],
                                        gsUdpErrorCallback onEvent, gsUdpConnConnectedCallback onAttemptResult,
                                        gsUdpConnClosedCallback onClosed, gsUdpConnPingCallback onPing,
                                        gsUdpConnReceivedDataCallback onReceived, void *userData);
// Removes the message handler of the fixed 16-byte header, whose first byte must be
// nonzero
GSUdpErrorCode gsUdpEngineRemoveMsgHandler(char header[GS_UDP_MSG_HEADER_LEN]);
// Opens a connection with a 16-byte init message; timeoutMs is milliseconds, or zero
// for no timeout
GSUdpErrorCode gsUdpEngineStartTalkingToPeer(unsigned int ip, unsigned short port,
                                             char initMessage[GS_UDP_MSG_HEADER_LEN], int timeoutMs);
// The free space of the outgoing buffer of the connection to an address
int gsUdpEngineGetPeerOutBufferFreeSpace(unsigned int ip, unsigned short port);
// Sends a message of size bytes, with a 16-byte header, to the connection of an address
GSUdpErrorCode gsUdpEngineSendMessage(unsigned int ip, unsigned short port, char header[GS_UDP_MSG_HEADER_LEN],
                                      unsigned char *message, unsigned int size, gsi_bool reliable);
// The state of the connection to an address, written to *state
GSUdpErrorCode gsUdpEngineGetPeerState(unsigned int ip, unsigned short port, GSUdpPeerState *state);

#endif // POKEBW2_DWC_GSUDPENGINE_H
