#ifndef POKEBW2_DWC_GPIBUFFER_H
#define POKEBW2_DWC_GPIBUFFER_H

#include "dwc/gpi.h"
#include "dwc/nonport.h"

// The texts of GameSpy's GP that grow as they are written (gpiBuffer.c), in overlay 11, and the sending and receiving
// of them over the connection manager's socket and to the peers (see dwc/gp.h for the names)

int gpiAppendCharToBuffer(GPConnection *connection, GPIBuffer *outputBuffer, int c);
int gpiAppendStringToBufferLen(GPConnection *connection, GPIBuffer *outputBuffer, const char *string, int stringLen);
int gpiAppendStringToBuffer(GPConnection *connection, GPIBuffer *outputBuffer, const char *buffer);
int gpiAppendIntToBuffer(GPConnection *connection, GPIBuffer *outputBuffer, int num);
// The ones to a peer, which go straight out when nothing waits before them and the Udp Engine has room
int gpiSendOrBufferChar(GPConnection *connection, GPIPeer *peer, int c);
int gpiSendOrBufferStringLen(GPConnection *connection, GPIPeer *peer, const char *string, int stringLen);
int gpiSendOrBufferString(GPConnection *connection, GPIPeer *peer, char *string);
// id names the socket in the debug texts ("CM" the connection manager, "SM" the search manager)
int gpiRecvToBuffer(GPConnection *connection, int sock, GPIBuffer *inputBuffer, int *bytesRead, gsi_bool *connClosed,
                    const char *id);
int gpiSendFromBuffer(GPConnection *connection, int sock, GPIBuffer *outputBuffer, gsi_bool *connClosed,
                      gsi_bool clipSentData, const char *id);
int gpiSendBufferToPeer(GPConnection *connection, gsi_u32 ip, gsi_u16 port, GPIBuffer *outputBuffer,
                        gsi_bool *connClosed, gsi_bool clipSentData);
// Reads a peer's message ("\\m\\<type>\\len\\<len>\\msg\\" and the text): message is NULL until a whole one is there
int gpiReadMessageFromBuffer(GPConnection *connection, GPIBuffer *inputBuffer, char **message, int *type, int *len);
int gpiClipBufferToPosition(GPConnection *connection, GPIBuffer *buffer);

#endif // POKEBW2_DWC_GPIBUFFER_H
