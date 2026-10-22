#ifndef POKEBW2_DWC_DWC_GHTTP_H
#define POKEBW2_DWC_DWC_GHTTP_H

#include "types.h"
#include "dwc/ghttp.h"

// dwc_ghttp: DWC's use of the GameSpy HTTP client (dwc/ghttp.h), in overlay 189: the root certificate of its hosts, the
// starting and ending of the client, and a request with the callbacks of the game and the error that a result turns to

// DWC's states and results of its requests, which are the client's (dwc/ghttp.h)
typedef enum {
    DWCGHTTPFalse = -1,
    DWCGHTTPHostLookup = GHTTPHostLookup,
    DWCGHTTPConnecting = GHTTPConnecting,
    DWCGHTTPSecuringSession = GHTTPSecuringSession,
    DWCGHTTPSendingRequest = GHTTPSendingRequest,
    DWCGHTTPPosting = GHTTPPosting,
    DWCGHTTPWaiting = GHTTPWaiting,
    DWCGHTTPReceivingStatus = GHTTPReceivingStatus,
    DWCGHTTPReceivingHeaders = GHTTPReceivingHeaders,
    DWCGHTTPReceivingFile = GHTTPReceivingFile
} DWCGHTTPState;

// BW2's client adds GHTTPRequestCancelled (18) after GHTTPEncryptionError, so DWC's count and memory error, which come
// after the client's results, are 19 and 20
typedef enum {
    DWCGHTTPSuccess = GHTTPSuccess,
    DWCGHTTPOutOfMemory = GHTTPOutOfMemory,
    DWCGHTTPBufferOverflow = GHTTPBufferOverflow,
    DWCGHTTPParseURLFailed = GHTTPParseURLFailed,
    DWCGHTTPHostLookupFailed = GHTTPHostLookupFailed,
    DWCGHTTPSocketFailed = GHTTPSocketFailed,
    DWCGHTTPConnectFailed = GHTTPConnectFailed,
    DWCGHTTPBadResponse = GHTTPBadResponse,
    DWCGHTTPRequestRejected = GHTTPRequestRejected,
    DWCGHTTPUnauthorized = GHTTPUnauthorized,
    DWCGHTTPForbidden = GHTTPForbidden,
    DWCGHTTPFileNotFound = GHTTPFileNotFound,
    DWCGHTTPServerError = GHTTPServerError,
    DWCGHTTPFileWriteFailed = GHTTPFileWriteFailed,
    DWCGHTTPFileReadFailed = GHTTPFileReadFailed,
    DWCGHTTPFileIncomplete = GHTTPFileIncomplete,
    DWCGHTTPFileToBig = GHTTPFileToBig,
    DWCGHTTPEncryptionError = GHTTPEncryptionError,
    DWCGHTTPNum = GHTTPRequestCancelled + 1,
    DWCGHTTPMemoryError = DWCGHTTPNum + 1
} DWCGHTTPResult;

// What the game gives to be done with the result of a request
typedef void (*DWCGHTTPCompletedCallback)(const char *buf, int len, DWCGHTTPResult result, void *param);
typedef void (*DWCGHTTPProgressCallback)(DWCGHTTPState state, const char *buf, int len, int bytesReceived,
                                         int totalSize, void *param);

// Starts and ends the module (the first returns a BOOL), and runs it for a frame
BOOL DWC_InitGHTTP(const char *gamename);
BOOL DWC_ShutdownGHTTP(void);
BOOL DWC_ProcessGHTTP(void);
// Called with a result or an error, to set the Wi-Fi library's error from it
DWCGHTTPResult DWCi_HandleGHTTPError(DWCGHTTPResult result);
// Overlay 189's, which keep the list of the hosts that may be reached: adds the host of a URL, or removes it
BOOL DWCi_SetGHTTPRootCA(const char *url);
BOOL DWCi_RemoveGHTTPRootCA(const char *url);
// Starts a request for a URL, with a buffer of this size or one the client grows, and a post or none; returns the
// request, or an error below 0
int DWC_GetGHTTPDataEx(const char *url, int bufferSize, BOOL freeBuffer, GHTTPPost *post,
                       DWCGHTTPProgressCallback progress, DWCGHTTPCompletedCallback completed, void *param);
int DWC_GetGHTTPData(const char *url, int bufferSize, BOOL freeBuffer, DWCGHTTPProgressCallback progress,
                     DWCGHTTPCompletedCallback completed, void *param);
void DWC_CancelGHTTPRequest(int request);

#endif // POKEBW2_DWC_DWC_GHTTP_H
