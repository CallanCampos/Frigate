#ifndef POKEBW2_DWC_GT2CALLBACK_H
#define POKEBW2_DWC_GT2CALLBACK_H

#include "dwc/gt2Main.h"
#include "dwc/nonport.h"

// The calls of GameSpy's GT2 into the app's callbacks (gt2Callback.c), in overlay 11. Each calls the callback, if
// there is one, with NULL and 0 for an empty message, and frees the socket if a callback closed it; it returns
// gsi_false when it did. The file and its functions are named after GameSpy's, as pret/pokerevo's Wii build has them
// (asm/SDK/DWC/gamespy/gt2Callback.s) and GameSpy's gt2Callback.c names them

GT2Bool gti2SocketErrorCallback(GT2Socket socket);
GT2Bool gti2ConnectAttemptCallback(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port,
                                   int latency, GT2Byte *message, int length);
// gti2ConnectedCallback also keeps the result
GT2Bool gti2ConnectedCallback(GT2Connection connection, GT2Result result, GT2Byte *message, int length);
GT2Bool gti2ReceivedCallback(GT2Connection connection, GT2Byte *message, int length, GT2Bool reliable);
GT2Bool gti2ClosedCallback(GT2Connection connection, GT2CloseReason reason);
GT2Bool gti2PingCallback(GT2Connection connection, int latency);
// Call the filter with the index
GT2Bool gti2SendFilterCallback(GT2Connection connection, int filterId, const GT2Byte *message, int length,
                               GT2Bool reliable);
GT2Bool gti2ReceiveFilterCallback(GT2Connection connection, int filterId, GT2Byte *message, int length,
                                  GT2Bool reliable);
// gti2DumpCallback calls the socket's send or receive dump, and gti2UnrecognizedMessageCallback writes whether the app
// handled the message
GT2Bool gti2DumpCallback(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port,
                         GT2Bool reset, const GT2Byte *message, int length, GT2Bool send);
GT2Bool gti2UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip, unsigned short port, GT2Byte *message,
                                        int length, GT2Bool *handled);

#endif // POKEBW2_DWC_GT2CALLBACK_H
