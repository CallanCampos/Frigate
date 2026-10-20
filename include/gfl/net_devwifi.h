#ifndef POKEBW2_GFL_NET_DEVWIFI_H
#define POKEBW2_GFL_NET_DEVWIFI_H

#include "types.h"
#include "gfl/net.h"

// net_devwifi.c, in overlay 11: the GFL net device for Wi-Fi, a table of functions over dwc_rap.c

// The device table for Wi-Fi, which main's NetDefine_LoadDevice returns for the Wi-Fi connection types
const GFLNetDevTable *NetDevWifi_GetTable(void);

#endif // POKEBW2_GFL_NET_DEVWIFI_H
