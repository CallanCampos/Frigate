#ifndef POKEBW2_DWCUTIL_SCENESETERROR_H
#define POKEBW2_DWCUTIL_SCENESETERROR_H

#include "types.h"

// The connection utility's error screen: the DWC error code that DWCi_SetErrorCode set, and the message of its kind.
// A then goes back to the first screen of the connection settings or to the settings of the connection.
// The file is the DWC utility library's sceneseterror.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code
// to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetError(void);
void DWCi_SetErrorCode(int code);

#endif // POKEBW2_DWCUTIL_SCENESETERROR_H
