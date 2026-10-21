#ifndef POKEBW2_DWC_GPICONNECT_H
#define POKEBW2_DWC_GPICONNECT_H

#include "dwc/gpi.h"
#include "dwc/nonport.h"

// The login of GameSpy's GP to the connection manager (gpiConnect.c), in overlay 11 (see dwc/gp.h for the names)

int gpiConnect(GPConnection *connection, const char *nick, const char *uniquenick, const char *email,
               const char *password, const char *authtoken, const char *partnerchallenge, const char *cdkey,
               gsi_bool firewall, gsi_bool newuser, int blocking, GPCallback callback, void *param);
int gpiProcessConnect(GPConnection *connection, GPIOperation *operation, const char *input);
int gpiCheckConnect(GPConnection *connection);
void gpiDisconnect(GPConnection *connection, gsi_bool tellServer);
// Handles the server's "\\rar\\", which gives the namespace and the partner of the login (the name is ours)
int gpiProcessRemoteAuthReply(GPConnection *connection, const char *input);

#endif // POKEBW2_DWC_GPICONNECT_H
