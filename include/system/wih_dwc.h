#ifndef POKEBW2_SYSTEM_WIH_DWC_H
#define POKEBW2_SYSTEM_WIH_DWC_H

#include "types.h"
#include "gfl/heap.h"

// The wireless scan's look at the access points and games around the player (wih_dwc.c). The rest of the file's
// functions are in gfl/net.h and app/comm_tvt_field.h

// Stop and start the beacon timers' countdown, as the save does while it writes
void func_020129e0(void);
void func_020129f8(void);
// Allocates the copy of the console's Wi-Fi connection settings, and reads them
void setupDownloadPlayConfig(HeapID heapId);
void downloadPlayConfigForWifi(void);

#endif // POKEBW2_SYSTEM_WIH_DWC_H
