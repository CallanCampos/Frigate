#ifndef POKEBW2_DWC_ERROR_H
#define POKEBW2_DWC_ERROR_H

#include "types.h"

// The library's one error (TWL-DWC 5.3's dwc_error.c, in ARM9 main): the error that the library last set, and the
// code that the server or the part that failed gave with it. The ROM names none of it; the calls that are the SDK's
// carry its names in the comments, where the code shows them.

// The error that none has set, and the numbers that the part that failed gives: the SDK's DWCError, in its order
// (recalled from the SDK, which the types that DWC_GetLastErrorEx gives them fit). The numbers from 19 to 22 are left
// bare. From ND_ANY on, the ROM's numbers sit one below the reference's, whose MO_SC_CONNECT_BLOCK comes before ND_ANY
typedef enum DWCError {
    DWC_ERROR_NONE = 0,
    DWC_ERROR_DS_MEMORY_ANY = 1,
    DWC_ERROR_AUTH_ANY = 2,
    DWC_ERROR_AUTH_OUT_OF_SERVICE = 3,
    DWC_ERROR_AUTH_STOP_SERVICE = 4,
    DWC_ERROR_AC_ANY = 5,
    DWC_ERROR_NETWORK = 6,
    DWC_ERROR_GHTTP_ANY = 7,
    DWC_ERROR_DISCONNECTED = 8,
    // The one error that stays: nothing replaces it, and clearing leaves it
    DWC_ERROR_FATAL = 9,
    DWC_ERROR_FRIENDS_SHORTAGE = 10,
    DWC_ERROR_NOT_FRIEND_SERVER = 11,
    DWC_ERROR_SERVER_FULL = 12,
    DWC_ERROR_ND_ANY = 13,
    // The download library's HTTP failure
    DWC_ERROR_ND_HTTP = 14,
    // The service locator's answer and server errors, and its HTTP failure
    DWC_ERROR_SVL_ANY = 15,
    DWC_ERROR_SVL_HTTP = 16,
    // The profanity check's answer errors, and its HTTP failure
    DWC_ERROR_PROF_ANY = 17,
    DWC_ERROR_PROF_HTTP = 18,
} DWCError;

// What a caller does about an error, which DWC_GetLastErrorEx gives from the error: the SDK's DWCErrorType, in its
// order. The callers (DWCRap_HandleError) confirm 1 (they clear it and go on), 3 (they end the friends match), 5 (they
// end the download of files) and treat 6 like 3; 2, 4 and 7 are named by the SDK's order
typedef enum DWCErrorType {
    DWC_ETYPE_NO_ERROR = 0,
    // Ends by itself, once the caller has cleared it
    DWC_ETYPE_LIGHT = 1,
    // Is shown to the player
    DWC_ETYPE_SHOW_ERROR = 2,
    // Ends the friends match
    DWC_ETYPE_SHUTDOWN_FM = 3,
    // Ends the GameSpy request (the callers shut the stats part down for the codes of its range, as for a type 2)
    DWC_ETYPE_SHUTDOWN_GHTTP = 4,
    // Ends the download of the server's files
    DWC_ETYPE_SHUTDOWN_ND = 5,
    // Ends the connection
    DWC_ETYPE_DISCONNECT = 6,
    // Cannot be recovered from
    DWC_ETYPE_FATAL = 7,
} DWCErrorType;

// Returns the last error, 0 if there is none, and its code through code if that is not NULL: DWC_GetLastError
DWCError DWC_GetLastError(int *code);
// The same with the type of the error through type, if that is not NULL: DWC_GetLastErrorEx
DWCError DWC_GetLastErrorEx(int *code, DWCErrorType *type);
// Clears the error, unless it is the fatal one: DWC_ClearError
void DWC_ClearError(void);
// Whether there is an error: DWCi_IsError
BOOL DWCi_IsError(void);
// Sets the error and its code, unless the fatal one is set: DWCi_SetError
void DWCi_SetError(DWCError error, int code);

#endif // POKEBW2_DWC_ERROR_H
