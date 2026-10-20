#ifndef POKEBW2_DWC_NASFUNC_H
#define POKEBW2_DWC_NASFUNC_H

#include "types.h"
#include "nitro/os.h"
#include "nitrowifi/cps.h"

// DWC's login to the Nintendo authentication server (dwc_nasfunc.c, overlay 11): the login's thread, the HTTP client
// that it and the other requests to the server use, the connection test, the profanity check and the service locator.
// The ROM names none of it. The type names follow the SDK's (DWCHttp, DWCAuthParam and the others, recalled from it),
// and so do the functions that NitroDWC's headers (auth/dwc_auth.h, dwc_http.h, dwc_netcheck.h, dwc_prof.h and
// dwc_svl.h) and the Wii DWC's dwc_auth_interface.c declare; the others' names are ours, in their style. The functions
// that the game calls are in dwc/dwc.h, and the connection test's in dwc/ac.h.

// The allocator and the deallocator of a request, with a name (0 here) first, as DWC_Alloc and DWC_Free take them
typedef void *(*DWCHttpAlloc)(u32 name, u32 size);
typedef void (*DWCHttpFree)(u32 name, void *ptr, u32 size);

// A text of the HTTP client: where it starts, where the next byte goes, where it ends, and its size
typedef struct DWCHttpBuffer {
    char *start;
    char *cur;
    char *end;
    int capacity;
} DWCHttpBuffer;

// A name and value of an answer's headers and form fields, as copies
typedef struct DWCHttpLabelValue {
    char *label;
    char *value;
} DWCHttpLabelValue;

// What a request is made with: the URL, whether it is a GET (else a POST), the size of the buffer of the answer, the
// memory functions, whether the server's certificate goes unchecked (for a URL other than the retail server's) and
// the timeout in milliseconds
typedef struct DWCHttpParam {
    const char *url;
    BOOL get;
    int bufferSize;
    DWCHttpAlloc alloc;
    DWCHttpFree free;
    BOOL ignoreCa;
    int timeout;
} DWCHttpParam;

// NitroWiFi's SSL connection (0x830 bytes) as the HTTP client fills it in: the host that the server's certificate must
// name, and the function that is asked about the certificate check's result
typedef struct DWCHttpSslConnection {
    u8 unk_000[0x800];
    const char *hostname;
    u8 unk_804[0xc];
    int (*validateCertificate)(int result, struct DWCHttpSslConnection *con, int index);
    u8 unk_814[0x1c];
} DWCHttpSslConnection;

// The states of a request (DWCHttp.result): ready, out of memory, the host not found, the connection refused, the send
// failed, the connection lost, timed out, cancelled, and done
#define DWC_HTTP_READY 0
#define DWC_HTTP_NO_MEMORY 1
#define DWC_HTTP_DNS_ERROR 2
#define DWC_HTTP_CONNECT_ERROR 3
#define DWC_HTTP_SEND_ERROR 5
#define DWC_HTTP_TIMEOUT 6
#define DWC_HTTP_CANCELLED 7
#define DWC_HTTP_DONE 8

// An HTTP request and its thread, whose stack is the first 0x1000 bytes (0x1c50 bytes)
typedef struct DWCHttp {
    u8 stack[0x1000];
    u8 started; // 0xff once the request is ready to run
    DWCHttpParam param;
    int result;
    char url[0x100];
    const char *host;
    const char *path;
    CPSInAddr ip;
    BOOL secure;
    u16 port;
    CPSSoc soc;
    DWCHttpSslConnection con;
    void *rcvBuffer;
    void *sndBuffer;
    u32 entropy[8];
    int fieldCount;
    DWCHttpBuffer request;
    DWCHttpBuffer response;
    OSMutex lock;
    int contentLength;
    int bodyLength;
    DWCHttpLabelValue fields[32];
    OSThread thread;
    OSMutex cancelLock;
    BOOL cancelled;
} DWCHttp;

// What the login is started with: the player's name in the game, the game's code for the server (gsbrcd) and the memory
// functions
typedef struct DWCAuthParam {
    u16 ingamesn[0x1a];
    char gsbrcd[0xc];
    DWCHttpAlloc alloc;
    DWCHttpFree free;
} DWCAuthParam;

// What the server answered the login: the result (20000 and the return code, or the error), the return code's text,
// the server's time, the address of the service locator, the token and the challenge that the player logs in to the
// other servers with, and the cookie
typedef struct DWCAuthResult {
    int returnCode;
    char retcode[4];
    char datetime[0xf];
    char locator[0x33];
    char token[0x12d];
    char challenge[9];
    char cookie[0x41];
} DWCAuthResult;

// What the service locator answers: whether the service is up, its host and its token
typedef struct DWCSvlResult {
    BOOL status;
    char svlhost[0x41];
    char svltoken[0x12d];
} DWCSvlResult;

// The steps of the login (DWC_Auth_GetError): 0 while it runs, 0x14 once cancelled, 0x15 once done, and the others
// for its errors

// Starts the login with its parameters and an HTTP client, returning 0 if it started
int DWC_Auth_Create(const DWCAuthParam *param, DWCHttp *http);
// Lets go of the login's memory
void DWC_Auth_Destroy(void);
// The login's step, and the server's answer, with a negative error for a failure
int DWC_Auth_GetError(void);
void DWC_Auth_GetResult(DWCAuthResult *result);

// The login that the friends' match runs: starts it with the player's name, the game's code and the user ID, and runs
// it; whether it is over, whether it worked and its error; the token and the challenge that it got; and the end of it
BOOL DWCi_Auth_StartAuthentication(const u16 *ingamesn, const char *gsbrcd, u64 userId, DWCHttpAlloc alloc,
                                   DWCHttpFree free);
void DWCi_Auth_AbortAuthentication(void);
void DWCi_Auth_ProcessAuthentication(void);
BOOL DWCi_Auth_IsFinished(void);
BOOL DWCi_Auth_IsSucceeded(void);
int DWCi_Auth_GetErrorCode(void);
void DWCi_Auth_GetGameSpyToken(char *token, char *challenge);
void DWCi_Auth_EndAuthentication(void);
// The console's user ID, or its pending one while none is confirmed; and whether it has one
u64 DWCi_Auth_GetConsoleUserId(void);
BOOL DWCi_Auth_HasConsoleUserId(void);

// The service locator: makes its memory, lets go of it, asks for a service and runs the request, which answers 0 when
// there is none, 1 when it did not start, 2 while it runs, 3 once done, 4 for an error and 5 once cancelled
BOOL DWC_Svl_Init(DWCHttpAlloc alloc, DWCHttpFree free);
void DWC_Svl_Cleanup(void);
BOOL DWC_Svl_GetTokenAsync(const char *service, DWCSvlResult *result);
int DWC_Svl_Process(void);

#endif // POKEBW2_DWC_NASFUNC_H
