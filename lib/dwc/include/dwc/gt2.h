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
// Notified with the ip and port of a ping, and its latency in milliseconds
typedef void (*Gt2PingCallback)(u32 ip, u16 port, u32 latencyMs, void *userData);
// Notified with the ip and port of a received datagram, its data and length, whether it is reliable, and userData
typedef void (*Gt2ReceivedCallback)(u32 ip, u16 port, const void *data, int len, BOOL reliable, void *userData);
// An incoming attempt not claimed by a pending connect, with elapsed milliseconds, init data and userData
typedef void (*Gt2IncomingAttemptCallback)(u32 ip, u16 port, u32 elapsedMilliseconds, const void *data, int len,
                                           void *userData);
// A pending connect's own received callback, including the userData supplied to Gt2_Connect
typedef void (*Gt2PendingDataCallback)(u32 ip, u16 port, const void *data, int len, BOOL reliable, void *userData);
// Handles a datagram offered by the lower socket's unknown-message path, independently of pending headers;
// its result tells the lower socket whether the app handled it
typedef BOOL (*Gt2UnknownMessageCallback)(u32 ip, u16 port, int len, const void *data, void *userData);

// Creates the one engine with its app-wide callbacks; pending connects can supply their own event handlers.
// Buffer sizes are bytes; zero selects 1500 incoming or 1460 outgoing bytes.
// Returns 0 (success), 1 (out of memory), or 3 (the lower socket could not be created)
int Gt2_CreateSocket(u16 localPort, int incomingBufferSize, int outgoingBufferSize, Gt2EventCallback onSocketError,
                     Gt2AttemptResultCallback onAttemptResult, Gt2ClosedCallback onClosed, Gt2PingCallback onPing,
                     Gt2ReceivedCallback onReceived, Gt2UnknownMessageCallback onUnknownMessage,
                     Gt2IncomingAttemptCallback onUnhandledAttempt, void *userData);
// Stops the engine's socket without freeing it, and the full teardown
int Gt2_StopSocket(void);
int Gt2_CloseSocket(void);

// Whether the engine was initialized, and its bound local port
BOOL Gt2_IsInitialized(void);
u16 Gt2_GetLocalPort(void);
// Whether there are no pending connects left
BOOL Gt2_IsIdle(void);
// Whether Gt2_CreateSocket was given none of its app-wide callbacks
BOOL Gt2_HasNoAppCallbacks(void);

// Registers a pending connect; initMessage and header each point to 16 readable bytes with a nonzero first byte.
// Its callbacks are this pending connect's own.
int Gt2_Connect(const char *initMessage, const char *header, Gt2EventCallback onEvent,
                Gt2AttemptResultCallback onAttemptResult, Gt2ClosedCallback onClosed, Gt2PingCallback onPing,
                Gt2PendingDataCallback onReceived, void *userData);
// Removes the pending connect matching the fixed 16-byte header, whose first byte must be nonzero
int Gt2_CancelConnect(const char *header);
// Opens a connection using a 16-byte init message; timeoutMs is milliseconds, or zero for no timeout
int Gt2_OpenConnection(u32 ip, u16 port, const void *initMessage, u32 timeoutMs);
// Closes an established connection
int Gt2_Close(u32 ip, u16 port);
// Sends a message of size bytes, with a 16-byte header, to the connection of an address
int Gt2_Send(u32 ip, u16 port, const void *header, const void *message, int size, BOOL reliable);
// The state of the connection to an address (its lower-layer handle's own state), written to *state
int Gt2_GetConnectionState(u32 ip, u16 port, int *state);

// A wide-character string's length, unrelated to GT2 itself but public and placed in this file by the original
int Gt2_WStrLen(const u16 *str);

#endif // POKEBW2_DWC_GT2_H
