#include "dwc/error.h"

// DWC's error (TWL-DWC 5.3's dwc_error.c, a guessed name; the ROM names none of it, and the file is ARM code in ARM9
// main like the rest of the library's base): the one error that the library keeps, with its code and the type that
// says what to do about it. The numbers of the errors are the callers', who set them with func_020584cc.

// The last error and its code
typedef struct DWCErrorInfo {
    int error;
    int code;
} DWCErrorInfo;

static DWCErrorInfo sErrorInfo;

int func_02058390(int *code) {
    if (code != NULL) {
        *code = sErrorInfo.code;
    }
    return sErrorInfo.error;
}

int func_020583b0(int *code, int *type) {
    if (code != NULL) {
        *code = sErrorInfo.code;
    }
    if (type != NULL) {
        // The type that each error gets. The errors that end the connection and those that are shown are met twice,
        // in two groups of the numbers
        switch (sErrorInfo.error) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 8:
            *type = DWC_ETYPE_DISCONNECT;
            break;
        case 6:
            *type = DWC_ETYPE_SHUTDOWN_FM;
            break;
        case 22:
            *type = DWC_ETYPE_SHOW_ERROR;
            break;
        case 7:
            *type = DWC_ETYPE_SHUTDOWN_GHTTP;
            break;
        case 10:
        case 11:
        case 12:
        case 21:
            *type = DWC_ETYPE_LIGHT;
            break;
        case 1:
        case DWC_ERROR_FATAL:
            *type = DWC_ETYPE_FATAL;
            break;
        case 13:
            *type = DWC_ETYPE_SHUTDOWN_ND;
            break;
        case 14:
        case 16:
        case 18:
            *type = DWC_ETYPE_DISCONNECT;
            break;
        case 15:
        case 17:
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

void func_02058490(void) {
    if (sErrorInfo.error != DWC_ERROR_FATAL) {
        sErrorInfo.error = DWC_ERROR_NONE;
        sErrorInfo.code = 0;
    }
}

BOOL func_020584b0(void) {
    return sErrorInfo.error != DWC_ERROR_NONE;
}

void func_020584cc(int error, int code) {
    if (sErrorInfo.error != DWC_ERROR_FATAL) {
        sErrorInfo.error = error;
        sErrorInfo.code = code;
    }
}
