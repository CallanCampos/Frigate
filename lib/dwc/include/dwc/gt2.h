#ifndef POKEBW2_DWC_GT2_H
#define POKEBW2_DWC_GT2_H

#include "types.h"

// Nintendo's wrapper around GameSpy's GT2 (the UDP transport that DWC's matchmaking talks through), named from the
// only string this module's data pool carries about it: "[Udp Engine] error creating gt2 socket, error code: %d".
// The real GameSpy GT2 SDK is public (gt2.c/gt2callback.c/gt2aux.c), but no existing decomp has recovered it, and
// the lower socket layer this file calls (func_ov011_0217d144 and the other func_ov011_0217dNNN addresses declared
// in dwc/ext.h) is not decompiled either, so every name here is a guess read off this file's own behavior, not the
// SDK's or swan's; this file's name ("gt2.c") is a guess too. There is one global engine, which is why every public
// function below takes no socket handle.

// Notified once with a code (3 for a socket-wide error) and the engine's or a pending connect's userData
typedef void (*Gt2EventCallback)(int code, void *userData);
// Notified with the ip and port of a connect attempt that just finished, and its raw result
typedef void (*Gt2AttemptResultCallback)(u32 ip, u16 port, int reason, BOOL wasRejected, void *userData);
// Notified with the ip and port of a connection that closed, and why
typedef void (*Gt2ClosedCallback)(u32 ip, u16 port, int reason, void *userData);
// Notified with the ip and port of a ping, and the datagram's length
typedef void (*Gt2PingCallback)(u32 ip, u16 port, int len, void *userData);
// Notified with the ip and port of a received datagram, its data and length, a per-call context, and userData
typedef void (*Gt2ReceivedCallback)(u32 ip, u16 port, const void *data, int len, void *context, void *userData);
// A pending connect's own "received" callback (set by Gt2_Connect), without the trailing userData
typedef void (*Gt2PendingDataCallback)(u32 ip, u16 port, const void *data, int len, void *context);
// Notified with the ip and port of a datagram with no 16-byte header match, its length, data and userData; its
// result becomes Gt2_CreateSocket's own "was it handled" result
typedef BOOL (*Gt2UnknownMessageCallback)(u32 ip, u16 port, int len, const void *data, void *userData);

// Creates the one engine; its callbacks are the app-wide fallbacks, used when no pending connect claims an event.
// Returns 0 (success), 1 (out of memory), 3 (the lower socket could not be created) or 0xa (a bad parameter)
int Gt2_CreateSocket(const char *localAddr, int maxConnections, int connectionsGrowBy,
                     Gt2EventCallback onSocketError, Gt2AttemptResultCallback onAttemptResult,
                     Gt2ClosedCallback onClosed, Gt2PingCallback onPing, Gt2ReceivedCallback onReceived,
                     Gt2UnknownMessageCallback onUnknownMessage, Gt2ReceivedCallback onUnroutable, void *userData);
// Stops the engine's socket without freeing it, and the full teardown
int Gt2_StopSocket(void);
int Gt2_CloseSocket(void);

// The engine's own state once created
int Gt2_GetMaxConnections(void);
u16 Gt2_GetLocalPort(void);
// Whether there are no pending connects left
BOOL Gt2_IsIdle(void);
// Whether Gt2_CreateSocket was given none of its app-wide callbacks
BOOL Gt2_HasNoAppCallbacks(void);

// Starts a connection attempt, racing a primary and a fallback address; its callbacks are this one attempt's own
int Gt2_Connect(const char *localAddr, const char *remoteAddr, Gt2EventCallback onEvent,
                Gt2AttemptResultCallback onAttemptResult, Gt2ClosedCallback onClosed, Gt2PingCallback onPing,
                Gt2PendingDataCallback onReceived, void *userData);
// Cancels the connect attempts still pending to a remote address
int Gt2_CancelConnect(const char *remoteAddr);
// Opens a connection to an address the app already knows is valid, without racing it
int Gt2_OpenConnection(u32 ip, u16 port, const void *header, void *context);
// Closes an established connection
int Gt2_Close(u32 ip, u16 port);
// Sends a message of size bytes, with a 16-byte header, to the connection of an address
int Gt2_Send(u32 ip, u16 port, const void *header, const void *message, int size, u32 flags);
// The state of the connection to an address (its lower-layer handle's own state), written to *state
int Gt2_GetConnectionState(u32 ip, u16 port, int *state);

// A wide-character string's length, unrelated to GT2 itself but public and placed in this file by the original
int Gt2_WStrLen(const u16 *str);

#endif // POKEBW2_DWC_GT2_H
