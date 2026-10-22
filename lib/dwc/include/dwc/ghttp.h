#ifndef POKEBW2_DWC_GHTTP_H
#define POKEBW2_DWC_GHTTP_H

#include "dwc/gsXML.h"
#include "dwc/nonport.h"

// GameSpy's HTTP client (ghttp.h), in overlay 189 after the NHTTP client: the types and calls that GameSpy's SOAP
// requests (dwc/gsSoap.h) and DWC use. The client's own files share src/ghttp/ghttp_internal.h

typedef enum { GHTTPFalse, GHTTPTrue } GHTTPBool;

typedef gsi_i32 GHTTPByteCount;

typedef enum {
    GHTTPSocketInit,
    GHTTPHostLookup,
    GHTTPLookupPending,
    GHTTPConnecting,
    GHTTPSecuringSession,
    GHTTPSendingRequest,
    GHTTPPosting,
    GHTTPWaiting,
    GHTTPReceivingStatus,
    GHTTPReceivingHeaders,
    GHTTPReceivingFile
} GHTTPState;

typedef enum {
    GHTTPSuccess,
    GHTTPOutOfMemory,
    GHTTPBufferOverflow,
    GHTTPParseURLFailed,
    GHTTPHostLookupFailed,
    GHTTPSocketFailed,
    GHTTPConnectFailed,
    GHTTPBadResponse,
    GHTTPRequestRejected,
    GHTTPUnauthorized,
    GHTTPForbidden,
    GHTTPFileNotFound,
    GHTTPServerError,
    GHTTPFileWriteFailed,
    GHTTPFileReadFailed,
    GHTTPFileIncomplete,
    GHTTPFileToBig,
    GHTTPEncryptionError,
    GHTTPRequestCancelled
} GHTTPResult;

typedef enum {
    GHTTPEncryptionEngine_None,
    GHTTPEncryptionEngine_GameSpy,
    GHTTPEncryptionEngine_MatrixSsl,
    GHTTPEncryptionEngine_RevoEx,
    GHTTPEncryptionEngine_Default
} GHTTPEncryptionEngine;

typedef int GHTTPRequest;

// The errors that a request gives instead of its number, as DWC builds the client (with GHTTP_EXTENDEDERROR)
typedef enum {
    GHTTPErrorStart = -8,
    GHTTPFailedToOpenFile,
    GHTTPInvalidPost,
    GHTTPInsufficientMemory,
    GHTTPInvalidFileName,
    GHTTPInvalidBufferSize,
    GHTTPInvalidURL,
    GHTTPUnspecifiedError = -1
} GHTTPRequestError;

typedef struct GHIPost *GHTTPPost;

// The callbacks of a request: when it has ended (returns GHTTPFalse to keep the buffer), while it goes on, and while a
// post is sent
typedef GHTTPBool (*ghttpCompletedCallback)(GHTTPRequest request, GHTTPResult result, char *buffer,
                                            GHTTPByteCount bufferLen, void *param);
typedef void (*ghttpProgressCallback)(GHTTPRequest request, GHTTPState state, const char *buffer,
                                      GHTTPByteCount bufferLen, GHTTPByteCount bytesReceived, GHTTPByteCount totalSize,
                                      void *param);
typedef void (*ghttpPostCallback)(GHTTPRequest request, int bytesPosted, int totalBytes, int objectsPosted,
                                  int totalObjects, void *param);

void ghttpStartup(void);
void ghttpCleanup(void);
GHTTPRequest ghttpGetExA(const char *url, const char *headers, char *buffer, int bufferSize, GHTTPPost post,
                         GHTTPBool throttle, GHTTPBool blocking, ghttpProgressCallback progress,
                         ghttpCompletedCallback completed, void *param);
void ghttpThink(void);
GHTTPBool ghttpRequestThink(GHTTPRequest request);
void ghttpCancelRequest(GHTTPRequest request);
GHTTPState ghttpGetState(GHTTPRequest request);
void ghttpSetMaxRecvTime(GHTTPRequest request, gsi_time time);
GHTTPPost ghttpNewPost(void);
void ghttpPostSetAutoFree(GHTTPPost post, GHTTPBool autoFree);
void ghttpFreePost(GHTTPRequest request, GHTTPPost post);
GHTTPBool ghttpPostAddXml(GHTTPPost post, GSXmlStreamWriter writer);
// Adds a file made of bytes in memory to a post (a form), with the name of its field, the name of the file and its type
GHTTPBool ghiPostAddFileFromMemory(GHTTPPost post, const gsi_char *name, const char *buffer, int bufferLen,
                                   const gsi_char *reportFilename, const gsi_char *contentType);
GHTTPBool ghttpSetRequestEncryptionEngine(GHTTPRequest request, GHTTPEncryptionEngine engine);

#endif // POKEBW2_DWC_GHTTP_H
