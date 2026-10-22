#ifndef POKEBW2_DWC_GSAVAILABLE_H
#define POKEBW2_DWC_GSAVAILABLE_H

#include "types.h"

// GameSpy's availability check (gsAvailable.c), in overlay 11: asks the server "<game>.available.gs.nintendowifi.net"
// whether the game's services are up, with one UDP packet that it sends again once after two seconds

// The result of the check; the services count as available when no answer came
typedef enum { GSIACWaiting, GSIACAvailable, GSIACUnavailable, GSIACTemporarilyUnavailable } GSIACResult;

extern GSIACResult __GSIACResult;
// The game's name, which the check copies, and the host to ask instead, unless it is empty
extern char __GSIACGamename[0x40];
extern char GSIACHostname[0x40];

void GSIStartAvailableCheckA(const char *gamename);
// Waits for the answer; returns the result once the check is done, and GSIACWaiting before
GSIACResult GSIAvailableCheckThink(void);

#endif // POKEBW2_DWC_GSAVAILABLE_H
