#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TEST_CONNECT_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TEST_CONNECT_H

#include "types.h"

// The connection utility's connection test after its second step (dwc_util_connection.h): it waits for the connection,
// takes what the test found into the connection being edited and goes on to the message of the success
// (dwc_util_test_done.h). A failure, or a connection that is not with a WEP key or none, shows a dialog, and B,
// closing the lid or the button end the test and go back to the choice of the setup of a connection.
// The ROM does not embed this source file's name; dwc_util_test_connect.c is descriptive.
void func_ov182_021b4880(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TEST_CONNECT_H
