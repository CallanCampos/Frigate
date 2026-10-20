// DWC's auto-connect, its teardown (TWL-DWC 5.3, ac). The file name is a guess: the three functions form one state
// machine on a state byte, and may belong to the file before or after, with no data to tell. dwc_ac.c's destroy and
// dwc_ac_result.c call it.

#include "dwc/ac.h"

static int DWCAc_StepWcmTeardown(void);
static BOOL DWCAc_StepSocTeardown(void);

// Steps the teardown whose state is *state. Up to 10 is the wireless manager's, brought down to idle by
// DWCAc_StepWcmTeardown; from 0xb to 0x11 the sockets are brought down first, by DWCAc_StepSocTeardown, and 0xe lets go
// of CPS before that. TRUE when it is over (the state is then 0), or when it failed (the state is then 0x12)
BOOL DWCAc_StepTeardown(u8 *state) {
    if (*state <= 10) {
        int result = DWCAc_StepWcmTeardown();

        if (result == 1) {
            *state = 0;
            return TRUE;
        }
        if (result == -1) {
            *state = 0x12;
            return TRUE;
        }
    } else if (*state == 14) {
        DWC_Netcheck_Abort();
        DWC_Netcheck_Destroy();
        *state = 12;
    } else if (*state < 18) {
        if (DWCAc_StepSocTeardown() == TRUE) {
            *state = 10;
        }
    }
    return FALSE;
}

// Asks the wireless manager for the next step down from the state it is in; 1 once it is down, -1 if it cannot be
static int DWCAc_StepWcmTeardown(void) {
    switch (WCM_GetState()) {
    case 0:
        return 1;
    case 1:
        WCM_Finish();
        break;
    case 3:
        WCM_CleanupAsync();
        break;
    case 6:
        WCM_StopSearchAsync();
        break;
    case 9:
        WCM_DisconnectAsync();
        break;
    case 12:
        WCM_RestoreIdleAsync();
        break;
    case 11:
        DWCAc_SetError(0);
        return -1;
    }
    return 0;
}

// Brings the sockets down: TRUE once SOCL has closed them and SOC is cleaned up (or was never started)
static BOOL DWCAc_StepSocTeardown(void) {
    int result;

    if (SOCLiConfigPtr == NULL) {
        return TRUE;
    }
    if (SOCL_CalmDown() != 0) {
        return FALSE;
    }
    result = SOC_Cleanup();
    return result == 0 || result == SOCL_ENETRESET;
}
