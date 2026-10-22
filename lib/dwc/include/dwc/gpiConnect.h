#ifndef POKEBW2_DWC_GPICONNECT_H
#define POKEBW2_DWC_GPICONNECT_H

#include "dwc/gpi.h"
#include "dwc/nonport.h"

// The login of GameSpy's GP to the connection manager (gpiConnect.c), in overlay 11 (see dwc/gp.h for the names)

GPResult gpiConnect(GPConnection *connection, const char nick[GP_NICK_LEN], const char uniquenick[GP_UNIQUENICK_LEN],
                    const char email[GP_EMAIL_LEN], const char password[GP_PASSWORD_LEN],
                    const char authtoken[GP_AUTHTOKEN_LEN], const char partnerchallenge[GP_PARTNERCHALLENGE_LEN],
                    const char cdkey[GP_CDKEY_LEN], GPEnum firewall, GPIBool newuser, GPEnum blocking,
                    GPCallback callback, void *param);
GPResult gpiProcessConnect(GPConnection *connection, GPIOperation *operation, const char *input);
GPResult gpiCheckConnect(GPConnection *connection);
void gpiDisconnect(GPConnection *connection, GPIBool tellServer);
// Handles the server's "\\rar\\", which gives the namespace and the partner of the login (the name is ours)
int gpiProcessRemoteAuthReply(GPConnection *connection, const char *input);

#endif // POKEBW2_DWC_GPICONNECT_H
