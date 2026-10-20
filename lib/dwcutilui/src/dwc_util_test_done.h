#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TEST_DONE_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TEST_DONE_H

#include "types.h"

// The connection utility's message after the connection test of dwc_util_connection.h connected, which stays for 120
// frames, before the message of the connection test with auto-connect (as dwc_util_page_multi4.h's after an AOSS
// setup).
// The ROM does not embed this source file's name; dwc_util_test_done.c is descriptive.
void func_ov182_021b4b68(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TEST_DONE_H
