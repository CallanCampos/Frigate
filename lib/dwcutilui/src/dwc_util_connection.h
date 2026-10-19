#ifndef POKEBW2_DWCUTIL_DWC_UTIL_CONNECTION_H
#define POKEBW2_DWCUTIL_DWC_UTIL_CONNECTION_H

#include "types.h"

// The connection utility's connection test (dwcutil/dwcutil_ac.h), and what it found, saved into the connection
// being edited.
// The ROM does not embed this source file's name; dwc_util_connection.c is descriptive.
void func_ov182_021ae3d8(void);
void func_ov182_021ae424(void);
// 0 while the test works, 1 and 2 at its two steps, 3 once it connected with a WEP key or none, 4 when it failed, 5
// when it connected otherwise
int func_ov182_021ae440(void);
void func_ov182_021ae4b0(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_CONNECTION_H
