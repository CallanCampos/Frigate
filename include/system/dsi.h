#ifndef POKEBW2_SYSTEM_DSI_H
#define POKEBW2_SYSTEM_DSI_H

#include "types.h"

// The DSi's settings, with what a DS answers in their place. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except isRunningOnDSi and isWirelessEnabled

// Whether the game runs on a DSi, through hw_isDSi
BOOL isRunningOnDSi(void);
// TRUE when the DSi's parental controls restrict exchanging photos, despite swan's name
BOOL canPlayerExchangePhotos(void);
// FALSE when the DSi's wireless communications are turned off
BOOL isWirelessEnabled(void);
// Whether the DSi's user agreed to the EULA, TRUE on a DS
BOOL hasLicenseBeenAccepted(void);
// The birthday in the DS's owner settings
void getBirthdayMonthDay(u8 *month, u8 *day);

#endif // POKEBW2_SYSTEM_DSI_H
