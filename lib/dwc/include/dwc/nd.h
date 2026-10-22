#ifndef POKEBW2_DWC_ND_H
#define POKEBW2_DWC_ND_H

#include "types.h"

// NitroDWC's download library (DWC_Nd), in overlay 189, which Mystery Gift receives Wi-Fi gifts with. The ROM names
// none of it. The names of the calls, of DWCNdFileInfo and of most values of the enums come from pret's symbols
// (pokeheartgold's NitroDWC and pokeplatinum's use of them), matched to these functions by what they do; the names of
// DWCNdFileInfo's fields, the errors and the work are NitroDWC's own, as its libraries' symbols and ntrtwl's
// reconstruction of NitroDWC have them, and the value that neither shows (ND_CBREASON_CLEANED_UP) is ours

// A file the server offers, 0xb0 bytes
typedef struct DWCNdFileInfo {
    // The name that the file is asked for with, and a text about it in UTF-16
    char name[33];
    short explain[51];
    char param1[11];
    // Mystery Gift's gifts give the games they are for here, as a hexadecimal mask of 1 << GAME_VERSION
    char param2[11];
    char param3[11];
    unsigned int size;
} DWCNdFileInfo;

// What a callback reports on: the first call, the file counts, a list and a file. The library's end is ours
typedef enum DWCNdCallbackReason {
    DWC_ND_CBREASON_INITIALIZE,
    DWC_ND_CBREASON_GETFILELISTNUM,
    DWC_ND_CBREASON_GETFILELIST,
    DWC_ND_CBREASON_GETFILE,
    ND_CBREASON_CLEANED_UP,
} DWCNdCallbackReason;

// The errors of the download library, among them DWC_ND_ERROR_PARAM (a text that is too long) and DWC_ND_ERROR_FATAL
// (the library could not be set up): the SDK's DWCNdError
typedef enum DWCNdError {
    DWC_ND_ERROR_NONE,
    DWC_ND_ERROR_ALLOC,
    DWC_ND_ERROR_BUSY,
    DWC_ND_ERROR_HTTP,
    DWC_ND_ERROR_BUFFULL,
    DWC_ND_ERROR_PARAM,
    DWC_ND_ERROR_CANCELED,
    DWC_ND_ERROR_DLSERVER,
    DWC_ND_ERROR_FATAL,
} DWCNdError;

// The callback of every asynchronous call, with its result and the code of a failure of the server (-1 when there is
// none)
typedef void (*DWCNdCallback)(DWCNdCallbackReason reason, DWCNdError error, int serverError);

BOOL DWC_NdInitAsync(DWCNdCallback callback, char *gameCode, char *password);
// Gives the game the callback of what has ended since the last call; not a name of pret's
void NdReportResults(void);
BOOL DWC_NdCleanupAsync(void);
BOOL DWC_NdSetAttr(const char *attr1, const char *attr2, const char *attr3);
BOOL DWC_NdGetFileListAsync(DWCNdFileInfo *files, unsigned int offset, unsigned int count);
BOOL DWC_NdGetFileAsync(DWCNdFileInfo *file, char *buffer, unsigned int size);
BOOL DWC_NdCancelAsync(void);
BOOL DWC_NdGetProgress(u32 *received, u32 *contentLength);

#endif // POKEBW2_DWC_ND_H
