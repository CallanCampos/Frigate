#ifndef POKEBW2_DWC_GPIPEER_H
#define POKEBW2_DWC_GPIPEER_H

#include "types.h"
#include "dwc/gpi.h"

// The peers of GameSpy's GP (gpiPeer.c), in overlay 11: the buddies that messages go to directly, through the Udp
// Engine (dwc/gt2.h), once they have authorized each other with the signatures the server gives (see dwc/gp.h for the
// names)

void gpiDestroyPeer(GPConnection *connection, GPIPeer *peer);
int gpiProcessPeers(GPConnection *connection);
GPIPeer *gpiGetPeerByProfile(const GPConnection *connection, int profileid);
GPIPeer *gpiAddPeer(GPConnection *connection, int profileid, BOOL initiate);
int gpiPeerGetSig(GPConnection *connection, GPIPeer *peer);
int gpiPeerStartConnect(GPConnection *connection, GPIPeer *peer);
int gpiPeerAddMessage(GPConnection *connection, GPIPeer *peer, int type, const char *message);
int gpiPeerStartTransferMessage(GPConnection *connection, GPIPeer *peer, int type, const GPITransferID *transferID);
int gpiPeerFinishTransferMessage(GPConnection *connection, GPIPeer *peer, const char *message, int len);
// The Udp Engine's callbacks (dwc/gt2.h), with the connection as their user data
void gpiPeerLeftCallback(u32 ip, u16 port, int reason, void *userData);
void gpiPeerMessageCallback(u32 ip, u16 port, const void *data, int len, BOOL reliable, void *userData);
void gpiPeerAcceptedCallback(u32 ip, u16 port, int reason, BOOL rejected, void *userData);
void gpiPeerPingReplyCallback(u32 ip, u16 port, u32 latency, void *userData);
void gpiPeerAddOp(GPIPeer *peer, GPIPeerOp *operation);
void gpiPeerRemoveOp(GPIPeer *peer, GPIPeerOp *operation);

#endif // POKEBW2_DWC_GPIPEER_H
