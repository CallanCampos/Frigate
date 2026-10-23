#ifndef POKEBW2_DWCUTIL_DWC_WCM_H
#define POKEBW2_DWCUTIL_DWC_WCM_H

#include "types.h"

// The wrapper that DWC's settings in overlay 182 put around the wireless connection manager (WCM): it starts the WCM,
// searches for access points, connects to one and cleans up, one operation at a time, and reports to a callback. The
// ROM names none of it; the names are the DWC utility library's own, as that library's symbols and debug information
// have them (its apc.c), where the code shows them, and ours elsewhere

// What the callback is called with
enum {
    DWC_WCM_EVENT_UNKNOWN = 1,
    DWC_WCM_EVENT_FAILED,
    // These two only from the copy of the wrapper in dwcutil_ac.c: the WCM ended, and a search went once through the
    // channels
    DWC_WCM_EVENT_TERMINATED,
    DWC_WCM_EVENT_SEARCH_AROUND,
    DWC_WCM_EVENT_FOUND_AP,
    DWC_WCM_EVENT_STARTED,
    DWC_WCM_EVENT_SEARCHING = 8,
    DWC_WCM_EVENT_SEARCH_FAILED,
    DWC_WCM_EVENT_SEARCH_ENDED,
    DWC_WCM_EVENT_END_SEARCH_FAILED,
    DWC_WCM_EVENT_CONNECTED,
    DWC_WCM_EVENT_CONNECT_FAILED,
    DWC_WCM_EVENT_DISCONNECTED,
    DWC_WCM_EVENT_DISCONNECT_FAILED,
    DWC_WCM_EVENT_CLEANED_UP = 0x14,
};

// Told each event with a second argument that is always 0
typedef void (*DWCWcmCallback)(int event, int param);

// Starts the WCM in a work the caller gives (a description of the access point to leave room for, the WCM's own work
// and the buffers of its access point list); TRUE when the start is taken. Needs at least 0x2700 bytes
BOOL APC_Init(DWCWcmCallback callback, void *work, u32 size);
// Searches for the access point with the given BSSID (or any) and ESSID of the given length (or any)
BOOL APC_SearchStart(const u8 *bssid, const u8 *essid, int essidLength, u32 option);
// Copies up to max descriptions (0xc0 bytes each) of the access points found to dest and counts all of them
int APC_GetAPList(void *dest, int max);
// Stops the search
BOOL APC_SearchEnd(void);
// Disconnects
BOOL APC_Disconnect(void);
// Cleans the WCM up
BOOL APC_End(void);
// Connects to the access point with the description and the WEP key
BOOL APC_Connect(const void *bssDesc, const void *wepDesc, u32 option);

#endif // POKEBW2_DWCUTIL_DWC_WCM_H
