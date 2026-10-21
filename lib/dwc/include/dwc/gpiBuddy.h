#ifndef POKEBW2_DWC_GPIBUDDY_H
#define POKEBW2_DWC_GPIBUDDY_H

#include "dwc/gpi.h"
#include "dwc/nonport.h"

// The buddy messages and the buddy list of GameSpy's GP (gpiBuddy.c), in overlay 11 (see dwc/gp.h for the names)

// Handle the server's "\\bm\\" buddy messages, "\\bsi\\" status infos and "\\bdy\\" buddy list
int gpiProcessRecvBuddyMessage(GPConnection *connection, const char *input);
int gpiProcessRecvBuddyStatusInfo(GPConnection *connection, const char *input);
int gpiProcessRecvBuddyList(GPConnection *connection, const char *input);
// Send a buddy message through the server, or to the buddy's peer when it has one (GP_DONT_ROUTE, 0xb00, never
// through the server); peerOp is the operation that waits for the peer's answer, if any
int gpiSendServerBuddyMessage(GPConnection *connection, int profileid, int type, const char *message);
int gpiSendBuddyMessage(GPConnection *connection, int profileid, int type, const char *message, int sendOption,
                        GPIPeerOp *peerOp);
// Answer a peer's request for the keys of our status, and handle its reply to ours
int gpiBuddyHandleKeyRequest(GPConnection *connection, GPIPeer *peer);
int gpiBuddyHandleKeyReply(GPConnection *connection, GPIPeer *peer, char *buffer);
// Authorize a buddy request, and delete a buddy, telling the server if asked to
int gpiAuthBuddyRequest(GPConnection *connection, int profile);
int gpiDeleteBuddy(GPConnection *connection, int profile, gsi_bool sendServerRequest);
// Sets whether a profile's buddy was deleted (the name is ours)
int gpiSetBuddyDeleted(GPConnection *connection, int profile, gsi_bool deleted);

#endif // POKEBW2_DWC_GPIBUDDY_H
