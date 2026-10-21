#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TEST_PROGRESS_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TEST_PROGRESS_H

#include "types.h"

// The connection utility's connection test after its first step (dwc_util_connection.h): it waits for the second
// step and goes on to the wait for the connection (dwc_util_test_connect.h). A failure shows a dialog, and B, closing
// the lid or the button end the test and go back to the choice of the setup of a connection.
// The ROM does not embed this source file's name; dwc_util_test_progress.c is descriptive.
void DWCUtilTestProgress_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TEST_PROGRESS_H
