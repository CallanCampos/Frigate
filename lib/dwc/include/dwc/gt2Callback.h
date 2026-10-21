#ifndef POKEBW2_DWC_GT2CALLBACK_H
#define POKEBW2_DWC_GT2CALLBACK_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The calls of GameSpy's GT2 into the app's callbacks (gt2Callback.c), in overlay 11. Each calls the callback, if
// there is one, with NULL and 0 for an empty message, and frees the socket if a callback closed it; it returns
// gsi_false when it did. The file and its functions are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Callback.s) and GameSpy's gt2Callback.c names them

gsi_bool gti2SocketErrorCallback(GTI2Socket *socket);
gsi_bool gti2ConnectAttemptCallback(GTI2Socket *socket, GTI2Connection *connection, gsi_u32 ip, gsi_u16 port,
                                    int latency, const void *message, int length);
// gti2ConnectedCallback also keeps the result
gsi_bool gti2ConnectedCallback(GTI2Connection *connection, int result, const void *message, int length);
gsi_bool gti2ReceivedCallback(GTI2Connection *connection, const void *message, int length, gsi_bool reliable);
gsi_bool gti2ClosedCallback(GTI2Connection *connection, int reason);
gsi_bool gti2PingCallback(GTI2Connection *connection, int latency);
// Call the filter with the index
gsi_bool gti2SendFilterCallback(GTI2Connection *connection, int filterId, const void *message, int length,
                                gsi_bool reliable);
gsi_bool gti2ReceiveFilterCallback(GTI2Connection *connection, int filterId, const void *message, int length,
                                   gsi_bool reliable);
// gti2DumpCallback calls the socket's send or receive dump, and gti2UnrecognizedMessageCallback writes whether the app
// handled the message
gsi_bool gti2DumpCallback(GTI2Socket *socket, GTI2Connection *connection, gsi_u32 ip, gsi_u16 port, gsi_bool reset,
                          const void *message, int length, gsi_bool send);
gsi_bool gti2UnrecognizedMessageCallback(GTI2Socket *socket, gsi_u32 ip, gsi_u16 port, const void *message, int length,
                                         gsi_bool *handled);

#endif // POKEBW2_DWC_GT2CALLBACK_H
