#ifndef POKEBW2_DWC_NASFUNC_H
#define POKEBW2_DWC_NASFUNC_H

#include "types.h"
#include "nitro/os.h"
#include "nitrowifi/cps.h"

// DWC's login to the Nintendo authentication server (overlay 11): the login's thread (dwc_auth.c), the HTTP client
// that it and the other requests to the server use (dwc_http_client.c), the connection test (dwc_conntest.c), the
// profanity check (dwc_profanity.c), the service locator (dwc_svcloc.c) and the root certificates (dwc_rootcert_*.c).
// The ROM names none of it: the file names are ours, from what each file does and the strings it sends, and so are the
// names of the globals that the files share (DWCi_ and what they hold). The type names follow the SDK's (DWCHttp,
// DWCAuthParam and the others, recalled from it), and so do the functions that NitroDWC's headers (auth/dwc_auth.h,
// dwc_http.h, dwc_netcheck.h, dwc_prof.h and dwc_svl.h) and the Wii DWC's dwc_auth_interface.c declare; the others'
// names are ours, in their style. The functions that the game calls are in dwc/dwc.h, and the connection test's in
// dwc/ac.h.

// The allocator and the deallocator of a request, with a name (0 here) first, as DWC_Alloc and DWC_Free take them
#ifdef PLATFORM_PC
// The SDK uses 32-bit longs; native allocation callbacks must match DWCACAlloc/DWCACFree exactly.
typedef void *(*DWCAuthAlloc)(u32 name, s32 size);
typedef void (*DWCAuthFree)(u32 name, void *ptr, s32 size);
#else
typedef void *(*DWCAuthAlloc)(unsigned long name, long size);
typedef void (*DWCAuthFree)(unsigned long name, void *ptr, long size);
#endif

// A text of the HTTP client: where it starts, where the next byte goes, where it ends, and its size
typedef struct DWCHttpBuffer {
    char *buffer;
    char *write_index;
    char *buffer_tail;
    unsigned long length;
} DWCHttpBuffer;

// A name and value of an answer's headers and form fields, as copies
typedef struct DWCHttpLabelValue {
    char *label;
    char *value;
} DWCHttpLabelValue;

// Whether a request is a POST or a GET: the SDK's DWCHttpAction
typedef enum DWCHttpAction {
    DWCHTTP_POST = 0,
    DWCHTTP_GET = 1,
} DWCHttpAction;

// What a request is made with: the URL, whether it is a GET or a POST, the size of the buffer of the answer, the
// memory functions, whether the server's certificate goes unchecked (for a URL other than the retail server's) and
// the timeout in milliseconds
typedef struct DWCHttpParam {
    const char *url;
    DWCHttpAction action;
    unsigned long len_recvbuf;
    DWCAuthAlloc alloc;
    DWCAuthFree free;
    BOOL ignoreca;
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

// The states of a request (DWCHttp.result), the SDK's DWCHttpError: ready, out of memory, the host not found, the
// connection refused, the send timed out (4, which the code doesn't set), the send failed, the receive timed out,
// cancelled, and done
typedef enum DWCHttpError {
    DWCHTTP_E_NOERR = 0,
    DWCHTTP_E_MEMERR = 1,
    DWCHTTP_E_DNSERR = 2,
    DWCHTTP_E_CONERR = 3,
    DWCHTTP_E_SENDERR = 5,
    DWCHTTP_E_RECVTOUT = 6,
    DWCHTTP_E_ABORT = 7,
    DWCHTTP_E_FINISH = 8,
} DWCHttpError;

// An HTTP request and its thread, whose stack is the first 0x1000 bytes (0x1c50 bytes)
typedef struct DWCHttp {
    u8 stack[0x1000];
    u8 initflag; // 0xff once the request is ready to run
    DWCHttpParam param;
    DWCHttpError error;
    char url[0x100];
    char *hostname;
    char *filepath;
    CPSInAddr hostip;
    BOOL ssl_enabled;
    u16 port;
    CPSSoc soc;
    DWCHttpSslConnection con;
    u8 *lowrecvbuf;
    u8 *lowsendbuf;
    u32 lowentropydata[8];
    int num_postitem;
    DWCHttpBuffer req;
    DWCHttpBuffer rep;
    OSMutex content_len_mutex;
    int content_len;
    int receivedbody_len;
    DWCHttpLabelValue labelvalue[32];
    OSThread thread;
    OSMutex mutex;
    BOOL abort;
} DWCHttp;

// What the login is started with: the player's name in the game, the game's code for the server (gsbrcd) and the memory
// functions
typedef struct DWCAuthParam {
    u16 ingamesn[0x1a];
    char gsbrcd[0xc];
    DWCAuthAlloc alloc;
    DWCAuthFree free;
} DWCAuthParam;

// What the server answered the login: the result (20000 and the return code, or the error), the return code's text,
// the server's time, the address of the service locator, the token and the challenge that the player logs in to the
// other servers with, and the cookie
typedef struct DWCAuthResult {
    int returncode;
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
// for its errors. The SDK's DWCAuthError, with its names (NitroDWC's auth/dwc_auth.h)
typedef enum DWCAuthError {
    DWCAUTH_E_NOERR,
    DWCAUTH_E_BMINITERR,
    DWCAUTH_E_MEMERR,
    DWCAUTH_E_ACERR,
    DWCAUTH_E_HTTPINITERR,
    DWCAUTH_E_CALERR,
    DWCAUTH_E_INVALIDPARAM,
    DWCAUTH_E_CANTADDHTTPHEADER,
    DWCAUTH_E_CANTADDPOSTITEM,
    DWCAUTH_E_DNSERR,
    DWCAUTH_E_CANTSTARTHTTPTHREAD,
    DWCAUTH_E_HTTPCONERR,
    DWCAUTH_E_HTTPPARSEERR,
    DWCAUTH_E_HTTPERR,
    DWCAUTH_E_NASPARSEERR,
    DWCAUTH_E_BMERR,
    DWCAUTH_E_NASALREADYEXIST,
    DWCAUTH_E_NASACCTEXPIRE,
    DWCAUTH_E_NASERR,
    DWCAUTH_E_TOOMANYRETRY,
    DWCAUTH_E_ABORT,
    DWCAUTH_E_FINISH,
    DWCAUTH_E_NOTINITIALIZED,
    DWCAUTH_E_MAX,
} DWCAuthError;

// Starts the login with its parameters and an HTTP client, returning 0 if it started
DWCAuthError DWC_Auth_Create(DWCAuthParam *param, void *http);
// Lets go of the login's memory
void DWC_Auth_Destroy(void);
// The login's step, and the server's answer, with a negative error for a failure
DWCAuthError DWC_Auth_GetError(void);
void DWC_Auth_GetResult(DWCAuthResult *result);

// The login that the friends' match runs: starts it with the player's name, the game's code and the user ID, and runs
// it; whether it is over, whether it worked and its error; the token and the challenge that it got; and the end of it
BOOL DWCi_Auth_StartAuthentication(const u16 *ingamesn, const char *gsbrcd, u64 userId, DWCAuthAlloc alloc,
                                   DWCAuthFree free);
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

// The states of the service locator's request: the SDK's DWCSvlState, with its names (NitroDWC's auth/dwc_svl.h)
typedef enum DWCSvlState {
    DWC_SVL_STATE_DIRTY = 0,
    DWC_SVL_STATE_IDLE,
    DWC_SVL_STATE_HTTP,
    DWC_SVL_STATE_SUCCESS,
    DWC_SVL_STATE_ERROR,
    DWC_SVL_STATE_CANCELED,
    DWC_SVL_STATE_MAX,
} DWCSvlState;

// The service locator: makes its memory, lets go of it, asks for a service and runs the request, which answers 0 when
// there is none, 1 when it did not start, 2 while it runs, 3 once done, 4 for an error and 5 once cancelled
BOOL DWC_Svl_Init(DWCAuthAlloc alloc, DWCAuthFree free);
void DWC_Svl_Cleanup(void);
BOOL DWC_Svl_GetTokenAsync(char *service, DWCSvlResult *result);
DWCSvlState DWC_Svl_Process(void);

// The login's HTTP client and helpers that the connection test (dwc_conntest.c), the profanity check
// (dwc_profanity.c) and the service locator (dwc_svcloc.c) share with dwc_auth.c
// What the requests to the server tell about the player and the console, as the form fields are made from them
typedef struct DWCCalInfo {
    char userid[0xe];
    char passwd[7];
    char gamecd[5];
    char makercd[3];
    char unitcd[2];
    char macadr[0xd];
    char lang[3];
    char birth[5];
    char devtime[0xd];
    char bssid[0xd];
    char ssid[0x20];
    char unk_6e;
    char apinfo[0xe];
    char unk_7d;
    u16 devname[10];
    u8 unk_92[2];
} DWCCalInfo;

// The login's request: the server's URL and the rest of what it is made with (the URL that DWC_Auth_SetCustomNas sets)
extern DWCHttpParam DWCauthhttpparam;

// Cancels the login and waits for its thread
void DWC_Auth_Abort(void);
// Waits for the login's thread
void DWC_Auth_Join(void);
// The player's and the console's details for a request, from the current Wi-Fi ID, and the form fields made from them
BOOL DWC_Auth_GetCalInfoFromNVRAM(DWCCalInfo *info);
BOOL DWC_Auth_SetCalInfoToHttp(DWCHttp *http, DWCCalInfo *info, BOOL sendSsid);

// Makes a request, ends its headers, runs it in a thread at a priority, and cancels it
DWCHttpError DWC_Http_Create(DWCHttp *http, DWCHttpParam *param);
DWCHttpError DWC_Http_FinishHeader(DWCHttp *http);
void DWC_Http_StartThread(DWCHttp *http, u32 priority);
void DWC_Http_Abort(DWCHttp *http);
// Lets go of a request's memory
void DWC_Http_Destroy(DWCHttp *http);
// Adds a header, a form field whose value is encoded in Base64, or text, to the request
DWCHttpError DWC_Http_Add_HeaderItem(DWCHttp *http, char *name, char *value);
DWCHttpError DWC_Http_Add_PostBase64Item(DWCHttp *http, const char *name, const char *value, unsigned long size);
DWCHttpError DWC_Http_Add_Body(DWCHttp *http, const char *text);
// Parses the answer (its headers only, or its form fields too), and reads a header, a field's Base64-decoded value
// or a field's raw value
BOOL DWC_Http_ParseResult(DWCHttp *http, BOOL headersOnly);
char *DWC_Http_GetResult(DWCHttp *http, char *label);
int DWC_Http_GetBase64DecodedResult(DWCHttp *http, char *label, char *dest, unsigned long size);
BOOL DWC_Http_GetRawResult(DWCHttp *http, char *label, char *dest, int size);

// dwc_conntest.c: whether the requests to the server send the access point's SSID, which the connection test sets
// once it has been redirected
extern BOOL DWCishotspot;

// The connection test (dwc_conntest.c), which the states 13 and 14 of dwc_ac_soc.c run, by the 52xxx errors it leads
// to in dwc_ac_result.c. Its states, the SDK's DWCNetcheckError (NitroDWC's auth/dwc_netcheck.h): 0 while the test is
// not over, 4 if it has no memory, 11 if it went well
typedef enum DWCNetcheckError {
    DWCNETCHECK_E_NOERR,
    DWCNETCHECK_E_HTTPINITERR,
    DWCNETCHECK_E_HTTPPARSEERR,
    DWCNETCHECK_E_HTTPERR,
    DWCNETCHECK_E_MEMERR,
    DWCNETCHECK_E_AUTHINITERR,
    DWCNETCHECK_E_AUTHERR,
    DWCNETCHECK_E_302TWICE,
    DWCNETCHECK_E_CANTADDPOSTITEM,
    DWCNETCHECK_E_NASPARSEERR,
    DWCNETCHECK_E_NETUNAVAIL,
    DWCNETCHECK_E_NETAVAIL,
    DWCNETCHECK_E_MAX,
} DWCNetcheckError;

// What the test is started with, the SDK's DWCNetcheckParam: the memory functions and the work of the backup
// manager. Auto-connect keeps it in its block DWC_AC_WORK_MEMFUNCS
typedef struct DWCNetcheckParam {
    DWCAuthAlloc alloc;
    DWCAuthFree free;
    void *bmworkarea;
} DWCNetcheckParam;

// Starts the test from the memory functions (copied), answering 0 once it is going; DWC_Netcheck_Abort ends CPS's
// threads and DWC_Netcheck_Destroy then lets go of its memory; DWC_Netcheck_GetError gives the test's state, and
// DWC_Netcheck_GetReturnCode its result
DWCNetcheckError DWC_Netcheck_Create(DWCNetcheckParam *param);
void DWC_Netcheck_Destroy(void);
void DWC_Netcheck_Abort(void);
DWCNetcheckError DWC_Netcheck_GetError(void);
int DWC_Netcheck_GetReturnCode(void);

// A root certificate that the HTTPS connection accepts: its name, its public key and the exponent of the key. Each is a
// file of its own (dwc_rootcert_*.c) that holds only the certificate: built with -ipa file, its name and key of 64
// bytes or more and its global get 4-aligned sections of their own, as the ROM has them, which one file of all 13
// can't give. The files and globals are named after each certificate's own name. DWCi_Http_Thread accepts all but
// DWCi_RootCertNintendoClass2, which DWC's GameSpy HTTP and the RAP's NHTTP requests use. Each is NitroWiFi's CPSCaInfo

extern CPSCaInfo DWCi_RootCertBaltimoreCyberTrust;
extern CPSCaInfo DWCi_RootCertGlobalSign;
extern CPSCaInfo DWCi_RootCertGteCyberTrustGlobal;
extern CPSCaInfo DWCi_RootCertGteCyberTrust;
extern CPSCaInfo DWCi_RootCertGlobalSignR2;
extern CPSCaInfo DWCi_RootCertNintendo;
extern CPSCaInfo DWCi_RootCertNintendoClass2;
extern CPSCaInfo DWCi_RootCertThawtePremiumServer;
extern CPSCaInfo DWCi_RootCertThawteServer;
extern CPSCaInfo DWCi_RootCertVeriSignClass3G2;
extern CPSCaInfo DWCi_RootCertVeriSignClass3G3;
extern CPSCaInfo DWCi_RootCertVeriSignClass3;
extern CPSCaInfo DWCi_RootCertRsaSecureServer;

#endif // POKEBW2_DWC_NASFUNC_H
