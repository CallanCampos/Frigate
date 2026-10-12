#ifndef POKEBW2_APP_WIFI_P2PMATCH_NET_H
#define POKEBW2_APP_WIFI_P2PMATCH_NET_H

// The commands the machines of the Wi-Fi Club's match board (overlay 173) send each other. The ROM has no name for
// the file: wifi_p2pmatch_net.c is our guess

#include "types.h"

// The commands' high byte
#define WIFIP2PMATCH_NET_CMD_BASE 0xa00

// work is the match board's work, which the handlers get
void WifiP2PMatchNet_CommandInit(void *work);
void WifiP2PMatchNet_CommandExit(void *work);

#endif // POKEBW2_APP_WIFI_P2PMATCH_NET_H
