#ifndef POKEBW2_DWC_UTILITY_H
#define POKEBW2_DWC_UTILITY_H

#include "types.h"

// The Nintendo Wi-Fi Connection settings of NitroDWC (dwc_utility.h), in overlay 182 with the game's wrapper. The
// ROM names none of it; the names are the SDK's, chosen from what the functions do

// The work that DWC_StartUtility needs, as the caller allocates it
#define DWC_UTILITY_WORK_SIZE 0x40000

// Languages and starting screens of the settings
#define DWC_LANGUAGE_ENGLISH 1
#define DWC_UTILITY_TOP_MENU_FOR_USA 0

// Runs the settings until the player leaves them; negative when the game runs on a DSi or the parameters are wrong
int DWC_StartUtility(void *work, int language, int param);

#endif // POKEBW2_DWC_UTILITY_H
