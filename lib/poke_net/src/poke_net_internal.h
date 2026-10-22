#ifndef POKEBW2_POKE_NET_INTERNAL_H
#define POKEBW2_POKE_NET_INTERNAL_H

#include "types.h"
#include "dwc/dwc.h"
#include "dwc/error.h"
#include "dwc/ext.h"
#include "dwc/memfunc.h"
#include "nitro/os.h"
#include "nitrowifi/cps.h"
#include "nitrowifi/soc.h"
#include "poke_net/poke_net_gds.h"

// What the files of the GDS client share: the request engine and its platform layer (poke_net.c), the SSL settings
// (poke_net_ssl.c), and what they call outside the library, none of it with a name in the ROM

// A request as it is sent, after the four bytes of its size
typedef struct {
    u16 ReqCode;
    u16 Option;
    POKE_NET_REQUESTCOMMON_AUTH Auth;
    u8 Param[0];
} POKE_NET_REQUESTCOMMON;

// The bytes before a request's and an answer's data: the size, then the header above or a POKE_NET_RESPONSE
#define POKE_NET_SIZE_LENGTH sizeof(s32)
#define POKE_NET_REQUEST_HEADER_SIZE (POKE_NET_SIZE_LENGTH + sizeof(POKE_NET_REQUESTCOMMON))
#define POKE_NET_RESPONSE_HEADER_SIZE (POKE_NET_SIZE_LENGTH + sizeof(POKE_NET_RESPONSE))

// The SSL settings take NitroWiFi's types: a connection (CPSSslConnection) whose server's name, certificate callback
// and certificate authorities (CPSCaInfo) the client fills in. The callback judges a certificate: it returns the error
// to fail with, 0 for none
typedef int (*POKE_NET_SSL_AUTH_CALLBACK)(int result, CPSSslConnection *connection, int level);

// poke_net.c: the request engine
BOOL POKE_NET_Initialize(const char *url, u16 port, POKE_NET_REQUESTCOMMON_AUTH *auth);
void POKE_NET_Release(void);
void POKE_NET_Clear(void);
void POKE_NET_ThreadFunction(void);
BOOL POKE_NET_Request(u16 reqno, void *param, void *response, s32 size);
int POKE_NET_GetStatus(void);
void *POKE_NET_GetResponse(void);
s32 POKE_NET_GetResponseSize(void);
int POKE_NET_GetLastErrorCode(void);
// poke_net.c: the platform layer
void *POKE_NET_AllocFunc(s32 size);
void POKE_NET_FreeFunc(void *ptr);
void POKE_NET_LockMutex(void);
void POKE_NET_UnlockMutex(void);
BOOL POKE_NET_CreateMutex(void);
BOOL POKE_NET_ReleaseMutex(void);
BOOL POKE_NET_ExInitialize(void);
void POKE_NET_ExRelease(void);
void POKE_NET_ThreadEntry(void *arg);
BOOL POKE_NET_CreateThread(void);
BOOL POKE_NET_IsIPAddress(const char *text);
BOOL POKE_NET_GetHostAddress(const char *host, char *address);
BOOL POKE_NET_Setup(void);
void POKE_NET_CloseSocket(void);
BOOL POKE_NET_Connecting(void);
BOOL POKE_NET_Sending(void);
void POKE_NET_RecvThreadEntry(void *arg);
BOOL POKE_NET_Receiving(void);
void POKE_NET_Sleep(s32 msec);
BOOL POKE_NET_GetSvlToken(void);
void POKE_NET_SslInitialize(void);
void POKE_NET_SslClear(void);

// poke_net_gds.c: the sizes of a request and of the largest answer to it, with their headers, or -1
s32 POKE_NET_GetRequestSize(s32 reqno);
s32 POKE_NET_GetResponseMaxSize(s32 reqno);

// poke_net_ssl.c
void POKE_NET_SSL_Setup(void);
void POKE_NET_SSL_SetConnection(CPSSslConnection *connection);
void POKE_NET_SSL_SetAuthCallback(POKE_NET_SSL_AUTH_CALLBACK callback);
void POKE_NET_SSL_SetCaInfo(CPSCaInfo **caInfo, int count);
void POKE_NET_SSL_StartConnection(const char *serverName, CPSSslConnection *connection);
void POKE_NET_SSL_EndConnection(CPSSslConnection *connection);

// The C library's memcpy as the compiler builds it in: a copy of a constant size is done in place
#define POKE_NET_Memcpy __memcpy

// The socket library of overlay 11: resolving a host's name to an address in host order (0 if it fails), and an
// address as text
u32 CPS_Resolve(const char *host);

#endif // POKEBW2_POKE_NET_INTERNAL_H
