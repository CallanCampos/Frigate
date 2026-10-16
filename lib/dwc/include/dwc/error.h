#ifndef POKEBW2_DWC_ERROR_H
#define POKEBW2_DWC_ERROR_H

#include "types.h"

// The library's one error (TWL-DWC 5.3's dwc_error.c, in ARM9 main): the error that the library last set, and the
// code that the server or the part that failed gave with it. The ROM names none of it; the calls that are the SDK's
// carry its names in the comments, where the code shows them.

// The error that none has set. A non-zero error is a number that the part that failed gives, 1 to 22 so far; the
// parts of DWC pass it as a bare number (13 for the download of files, 6, 8, 9 and 20 for the stats and score parts)
#define DWC_ERROR_NONE 0
// The one error that stays: nothing replaces it, and clearing leaves it
#define DWC_ERROR_FATAL 9

// What a caller does about an error, which func_020583b0 gives from the error: the SDK's DWCErrorType, in its order.
// The callers (DWCRap_HandleError) confirm 1 (they clear it and go on), 3 (they end the friends match), 5 (they end the
// download of files) and treat 6 like 3; 2, 4 and 7 are named by the SDK's order
#define DWC_ETYPE_NO_ERROR 0
// Ends by itself, once the caller has cleared it
#define DWC_ETYPE_LIGHT 1
// Is shown to the player
#define DWC_ETYPE_SHOW_ERROR 2
// Ends the friends match
#define DWC_ETYPE_SHUTDOWN_FM 3
// Ends the GameSpy request (the callers shut the stats part down for the codes of its range, as for a type 2)
#define DWC_ETYPE_SHUTDOWN_GHTTP 4
// Ends the download of the server's files
#define DWC_ETYPE_SHUTDOWN_ND 5
// Ends the connection
#define DWC_ETYPE_DISCONNECT 6
// Cannot be recovered from
#define DWC_ETYPE_FATAL 7

// Returns the last error, 0 if there is none, and its code through code if that is not NULL: DWC_GetLastError
int func_02058390(int *code);
// The same with the type of the error through type, if that is not NULL: DWC_GetLastErrorEx
int func_020583b0(int *code, int *type);
// Clears the error, unless it is the fatal one: DWC_ClearError
void func_02058490(void);
// Whether there is an error: DWCi_IsError
BOOL func_020584b0(void);
// Sets the error and its code, unless the fatal one is set: DWCi_SetError
void func_020584cc(int error, int code);

#endif // POKEBW2_DWC_ERROR_H
