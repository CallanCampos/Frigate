#ifndef POKEBW2_GFL_NET_DEVWIFI_H
#define POKEBW2_GFL_NET_DEVWIFI_H

#include "types.h"
#include "gfl/net.h"

// net_devwifi.c, in overlay 11: the GFL net device for Wi-Fi, a table of functions over dwc_rap.c

// The device table for Wi-Fi, which main's func_020116c0 returns for the Wi-Fi connection types
const GFLNetDevTable *NetDevWifi_GetTable(void);
// An ARM function of main past the network library: gets the last error of DWC and its type, returning its result code
int func_020583b0(int *code, int *type);

#endif // POKEBW2_GFL_NET_DEVWIFI_H
