#ifndef POKEBW2_DWCUTIL_SCENESETTESTCOMP_H
#define POKEBW2_DWCUTIL_SCENESETTESTCOMP_H

#include "types.h"

// The connection utility's message after a successful connection test with auto-connect (scenesettest.h),
// which stays for 180 frames before the settings of the connection or the menu that the utility was started from.
// The file is the DWC utility library's object that aletheia's map of Black 1's SDK code to that library's objects
// (layout/symbols_sdk.txt) shows as scenesettestcomp (libdwcutil.TWL.HYB.a), its name cut to 16 characters; its scene
// is DWCi_SceneSetTestCompletet, as the SDK spells it, so the rest of the name is not certain and the file keeps the
// part the map shows.
void DWCUtilAutoConnectOk_Init(void);

#endif // POKEBW2_DWCUTIL_SCENESETTESTCOMP_H
