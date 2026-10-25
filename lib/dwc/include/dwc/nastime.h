#ifndef POKEBW2_DWC_NASTIME_H
#define POKEBW2_DWC_NASTIME_H

#include "types.h"

// The authentication server's date (overlay 11's dwc_nastime.c), which DWC_GetDateTime (dwc/dwc.h) gives the game.
// The globals' names are ours.

// Whether the server's date was read, the console's clock in seconds when it was, and how far the console's clock was
// from the server's
extern BOOL DWCi_NasTimeValid;
extern s64 DWCi_NasTimeLocalSeconds;
extern s64 DWCi_NasTimeOffset;

// Keeps the server's date and time from the "Date" header of the authentication server's answer, such as
// "Fri, 03 Mar 2006 01:28:13 GMT" (FALSE if the header can't be read); the SDK's name
BOOL DWC_Auth_SetNasTimeDiff(const char *dateHeader);

#endif // POKEBW2_DWC_NASTIME_H
