#include "dwc/error.h"

// DWC's error (TWL-DWC 5.3's dwc_error.c, a guessed name; the ROM names none of it, and the file is ARM code in ARM9
// main like the rest of the library's base): the one error that the library keeps, with its code and the type that
// says what to do about it. The numbers of the errors are the callers', who set them with DWCi_SetError.

// The last error and its code
typedef struct DWCErrorInfo {
    DWCError error;
    int code;
} DWCErrorInfo;

static DWCErrorInfo sErrorInfo;

DWCError DWC_GetLastError(int *code) {
    if (code != NULL) {
        *code = sErrorInfo.code;
    }
    return sErrorInfo.error;
}

DWCError DWC_GetLastErrorEx(int *code, DWCErrorType *type) {
    if (code != NULL) {
        *code = sErrorInfo.code;
    }
    if (type != NULL) {
        // The type that each error gets. The errors that end the connection and those that are shown are met twice,
        // in two groups of the numbers
        switch (sErrorInfo.error) {
        case DWC_ERROR_AUTH_ANY:
        case DWC_ERROR_AUTH_OUT_OF_SERVICE:
        case DWC_ERROR_AUTH_STOP_SERVICE:
        case DWC_ERROR_AC_ANY:
        case DWC_ERROR_DISCONNECTED:
            *type = DWC_ETYPE_DISCONNECT;
            break;
        case DWC_ERROR_NETWORK:
            *type = DWC_ETYPE_SHUTDOWN_FM;
            break;
        case 22:
            *type = DWC_ETYPE_SHOW_ERROR;
            break;
        case DWC_ERROR_GHTTP_ANY:
            *type = DWC_ETYPE_SHUTDOWN_GHTTP;
            break;
        case DWC_ERROR_FRIENDS_SHORTAGE:
        case DWC_ERROR_NOT_FRIEND_SERVER:
        case DWC_ERROR_SERVER_FULL:
        case 21:
            *type = DWC_ETYPE_LIGHT;
            break;
        case DWC_ERROR_DS_MEMORY_ANY:
        case DWC_ERROR_FATAL:
            *type = DWC_ETYPE_FATAL;
            break;
        case DWC_ERROR_ND_ANY:
            *type = DWC_ETYPE_SHUTDOWN_ND;
            break;
        case DWC_ERROR_ND_HTTP:
        case DWC_ERROR_SVL_HTTP:
        case DWC_ERROR_PROF_HTTP:
            *type = DWC_ETYPE_DISCONNECT;
            break;
        case DWC_ERROR_SVL_ANY:
        case DWC_ERROR_PROF_ANY:
        case 19:
        case 20:
            *type = DWC_ETYPE_SHOW_ERROR;
            break;
        default:
            *type = DWC_ETYPE_NO_ERROR;
            break;
        }
    }
    return sErrorInfo.error;
}

void DWC_ClearError(void) {
    if (sErrorInfo.error != DWC_ERROR_FATAL) {
        sErrorInfo.error = DWC_ERROR_NONE;
        sErrorInfo.code = 0;
    }
}

BOOL DWCi_IsError(void) {
    return sErrorInfo.error != DWC_ERROR_NONE;
}

void DWCi_SetError(DWCError error, int code) {
    if (sErrorInfo.error != DWC_ERROR_FATAL) {
        sErrorInfo.error = error;
        sErrorInfo.code = code;
    }
}
