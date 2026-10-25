#ifndef POKEBW2_DWCUTIL_SCENEINIT_H
#define POKEBW2_DWCUTIL_SCENEINIT_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's first screen: it opens the language's message file and loads the two screens' graphics,
// then goes on to the first choice. The file is the DWC utility library's sceneinit.o (libdwcutil.TWL.HYB.a), as
// aletheia's map of Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it.

// The message file of the utility's language, which this screen opens
extern DWCUtilMsg *gDWCUtilMsg;

void DWCUtilMainMenu_InitUtility(void);

#endif // POKEBW2_DWCUTIL_SCENEINIT_H
