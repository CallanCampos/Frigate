#ifndef POKEBW2_GFL_NET_DEVWIRELESS_H
#define POKEBW2_GFL_NET_DEVWIRELESS_H

#include "types.h"
#include "gfl/net.h"

// net_devwireless.c, in overlay 30: the GFL net device for local wireless, a table of functions over net_whpipe.c

// The device table that func_020116c0 gives for the wireless net types
const GFLNetDevTable *NetDevWireless_GetTable(void);

#endif // POKEBW2_GFL_NET_DEVWIRELESS_H
