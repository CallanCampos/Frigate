#ifndef POKEBW2_DWC_GPIPEER_H
#define POKEBW2_DWC_GPIPEER_H

#include "dwc/gpi.h"
#include "dwc/gt2.h"
#include "dwc/nonport.h"

// The peers of GameSpy's GP (gpiPeer.c), in overlay 11: the buddies that messages go to directly, through the Udp
// Engine (dwc/gt2.h), once they have authorized each other with the signatures the server gives (see dwc/gp.h for the
// names)

void gpiDestroyPeer(GPConnection *connection, GPIPeer *peer);
GPResult gpiProcessPeers(GPConnection *connection);
GPIPeer *gpiGetPeerByProfile(const GPConnection *connection, int profileid);
GPIPeer *gpiAddPeer(GPConnection *connection, int profileid, GPIBool initiate);
GPResult gpiPeerGetSig(GPConnection *connection, GPIPeer *peer);
GPResult gpiPeerStartConnect(GPConnection *connection, GPIPeer *peer);
GPResult gpiPeerAddMessage(GPConnection *connection, GPIPeer *peer, int type, const char *message);
GPResult gpiPeerStartTransferMessage(GPConnection *connection, GPIPeer *peer, int type,
                                     const GPITransferID *transferID);
GPResult gpiPeerFinishTransferMessage(GPConnection *connection, GPIPeer *peer, const char *message, int len);
// The Udp Engine's callbacks (dwc/gt2.h), with the connection as their user data
void gpiPeerLeftCallback(unsigned int ip, unsigned short port, GSUdpCloseReason reason, void *userData);
void gpiPeerMessageCallback(unsigned int ip, unsigned short port, unsigned char *data, int len, gsi_bool reliable,
                            void *userData);
void gpiPeerAcceptedCallback(unsigned int ip, unsigned short port, GSUdpErrorCode error, gsi_bool rejected,
                             void *userData);
void gpiPeerPingReplyCallback(unsigned int ip, unsigned short port, unsigned int latency, void *userData);
void gpiPeerAddOp(GPIPeer *peer, GPIPeerOp *operation);
void gpiPeerRemoveOp(GPIPeer *peer, GPIPeerOp *operation);

#endif // POKEBW2_DWC_GPIPEER_H
