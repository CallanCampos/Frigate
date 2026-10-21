#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TEST_START_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TEST_START_H

#include "types.h"

// The connection utility's start of the connection test (dwc_util_connection.h): it starts the test, waits for its
// first step and goes on to the wait for the second (dwc_util_test_progress.h). A failure shows a dialog, and B,
// closing the lid or the button end the test and go back to the choice of the setup of a connection.
// The ROM does not embed this source file's name; dwc_util_test_start.c is descriptive.
void DWCUtilTestStart_Init(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TEST_START_H
