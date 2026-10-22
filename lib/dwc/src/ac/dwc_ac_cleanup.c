// DWC's auto-connect, its teardown (TWL-DWC 5.3, ac). The file name is a guess: the three functions form one state
// machine on a state byte, and may belong to the file before or after, with no data to tell. dwc_ac.c's destroy and
// dwc_ac_result.c call it.

#include "dwc/ac.h"

static int DisConnectAP(void);
static BOOL CloseSocket(void);

// Steps the teardown whose state is *state. Up to 10 is the wireless manager's, brought down to idle by
// DisConnectAP; from 0xb to 0x11 the sockets are brought down first, by CloseSocket, and 0xe lets go
// of CPS before that. TRUE when it is over (the state is then 0), or when it failed (the state is then 0x12)
BOOL DWCi_AC_CloseNetwork(u8 *state) {
    if (*state <= 10) {
        int result = DisConnectAP();

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
        if (CloseSocket() == TRUE) {
            *state = 10;
        }
    }
    return FALSE;
}

// Asks the wireless manager for the next step down from the state it is in; 1 once it is down, -1 if it cannot be
static int DisConnectAP(void) {
    switch (WCM_GetPhase()) {
    case 0:
        return 1;
    case 1:
        WCM_Finish();
        break;
    case 3:
        WCM_CleanupAsync();
        break;
    case 6:
        WCM_EndSearchAsync();
        break;
    case 9:
        WCM_DisconnectAsync();
        break;
    case 12:
        WCM_TerminateAsync();
        break;
    case 11:
        DWCi_AC_SetError(0);
        return -1;
    }
    return 0;
}

// Brings the sockets down: TRUE once SOCL has closed them and SOC is cleaned up (or was never started)
static BOOL CloseSocket(void) {
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
