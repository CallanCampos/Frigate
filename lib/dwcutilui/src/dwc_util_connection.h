#ifndef POKEBW2_DWCUTIL_DWC_UTIL_CONNECTION_H
#define POKEBW2_DWCUTIL_DWC_UTIL_CONNECTION_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's connection test (dwcutil/dwcutil_ac.h), and what it found, saved into the connection
// being edited.
// The ROM does not embed this source file's name; dwc_util_connection.c is descriptive.

// The message file of the utility's language, which the first screen opens (dwc_util_main_menu.c)
extern DWCUtilMsg *gDWCUtilMsg;

void DWCi_RAKUlInit(void);
void DWCi_RAKUlEnd(void);
// 0 while the test works, 1 and 2 at its two steps, 3 once it connected with a WEP key or none, 4 when it failed, 5
// when it connected otherwise
int DWCi_RAKUlGetState(void);
void DWCi_RAKUlSetResult(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_CONNECTION_H
