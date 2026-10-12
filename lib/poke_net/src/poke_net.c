// The request engine of the GDS client: it sends a request and receives its answer in a thread of its own, over an SSL
// connection, and the platform layer under it (the allocator, the mutex, the threads and the sockets). The ROM names
// nothing here; the file's name and its functions' are those of the Generation 4 library, or guesses after them. The
// two halves may be two files (poke_net.c and a platform file), which nothing in the ROM tells apart.

#include "types.h"
#include "poke_net_internal.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"

#define POKE_NET_THREAD_STACK_SIZE 0x800
#define POKE_NET_THREAD_PRIORITY 16
#define POKE_NET_SERVER_URL_LENGTH 0x100

typedef struct {
    POKE_NET_REQUESTCOMMON_AUTH Auth;
    // Whether Auth has its token
    BOOL HasSvlToken;
    int Status;
    char ServerURL[POKE_NET_SERVER_URL_LENGTH];
    u16 ServerPort;
    // The time each step may take, in seconds
    s32 ConnectTimeOut;
    s32 SendTimeOut;
    s32 RecvTimeOut;
    int LastErrorCode;
    s32 SendSize;
    u8 *SendBuffer;
    s32 RecvSize;
    u8 *RecvBuffer;
    // The server's address, in network order
    u32 ServerAddr;
    BOOL AbortFlag;
    int Socket;
    u32 ThreadPriority;
    OSThread Thread;
    OSMutex Mutex;
    // The thread that receives, which blocks in the socket while the request's thread watches the time
    OSThread RecvThread;
    s32 RecvResult;
    s32 RecvOffset;
    BOOL RecvDone;
} POKE_NET_CONTROL;

// The compiler lays these out from the last to the first: the stacks come first in the ROM
static POKE_NET_SSL_CONNECTION sPOKE_NET_SslConnection;
static POKE_NET_CONTROL gPOKE_NET_Control;
static u8 sPOKE_NET_RecvThreadStack[POKE_NET_THREAD_STACK_SIZE];
static u8 sPOKE_NET_ThreadStack[POKE_NET_THREAD_STACK_SIZE];

// The seconds since the system timer started, of which a tick is 64 cycles of the system clock
#define POKE_NET_TicksToSeconds(tick) ((tick) / (OS_SYSTEM_CLOCK / 64))
#define POKE_NET_GetTime() POKE_NET_TicksToSeconds(clock())

void POKE_NET_Release(void) {
    POKE_NET_Clear();
    memset(&gPOKE_NET_Control.Auth, 0, sizeof(POKE_NET_REQUESTCOMMON_AUTH));
    gPOKE_NET_Control.HasSvlToken = FALSE;
    gPOKE_NET_Control.ServerURL[0] = '\0';
    gPOKE_NET_Control.ServerPort = 0;
    POKE_NET_ReleaseMutex();
    POKE_NET_ExRelease();
}

BOOL POKE_NET_Initialize(const char *url, u16 port, POKE_NET_REQUESTCOMMON_AUTH *auth) {
    if (auth != NULL) {
        gPOKE_NET_Control.Auth = *auth;
        gPOKE_NET_Control.HasSvlToken = FALSE;
    }
    POKE_NET_SslInitialize();
    if (POKE_NET_ExInitialize() == FALSE) {
        return FALSE;
    }
    POKE_NET_CreateMutex();
    strcpy(gPOKE_NET_Control.ServerURL, url);
    gPOKE_NET_Control.ServerPort = port;
    gPOKE_NET_Control.SendBuffer = NULL;
    gPOKE_NET_Control.Socket = -1;
    POKE_NET_Clear();
    return TRUE;
}

void POKE_NET_Clear(void) {
    POKE_NET_CloseSocket();
    if (gPOKE_NET_Control.SendBuffer != NULL) {
        POKE_NET_FreeFunc(gPOKE_NET_Control.SendBuffer);
        gPOKE_NET_Control.SendBuffer = NULL;
    }
    gPOKE_NET_Control.ConnectTimeOut = 15;
    gPOKE_NET_Control.SendTimeOut = 30;
    gPOKE_NET_Control.RecvTimeOut = 30;
    gPOKE_NET_Control.SendSize = 0;
    gPOKE_NET_Control.RecvSize = 0;
    gPOKE_NET_Control.RecvBuffer = NULL;
    gPOKE_NET_Control.ServerAddr = 0;
    gPOKE_NET_Control.AbortFlag = FALSE;
    gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_INITIALIZED;
    gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_NONE;
    POKE_NET_SslClear();
}

// The request's thread: a step of the request each time round, until it ends
void POKE_NET_ThreadFunction(void) {
    for (;;) {
        BOOL end = FALSE;
        int status;

        switch (gPOKE_NET_Control.Status) {
        case POKE_NET_GDS_STATUS_INACTIVE:
        case POKE_NET_GDS_STATUS_INITIALIZED:
        case POKE_NET_GDS_STATUS_ABORTED:
        case POKE_NET_GDS_STATUS_FINISHED:
        case POKE_NET_GDS_STATUS_ERROR:
            end = TRUE;
            break;
        case POKE_NET_GDS_STATUS_REQUEST:
            if (POKE_NET_GetSvlToken() == FALSE) {
                if (gPOKE_NET_Control.LastErrorCode == POKE_NET_GDS_LASTERROR_ABORTED) {
                    status = POKE_NET_GDS_STATUS_ABORTED;
                } else {
                    status = POKE_NET_GDS_STATUS_ERROR;
                }
                end = TRUE;
            } else {
                status = POKE_NET_GDS_STATUS_NETSETTING;
            }
            POKE_NET_LockMutex();
            gPOKE_NET_Control.Status = status;
            break;
        case POKE_NET_GDS_STATUS_NETSETTING:
            if (POKE_NET_Setup() == FALSE) {
                if (gPOKE_NET_Control.LastErrorCode == POKE_NET_GDS_LASTERROR_ABORTED) {
                    status = POKE_NET_GDS_STATUS_ABORTED;
                } else {
                    status = POKE_NET_GDS_STATUS_ERROR;
                }
                end = TRUE;
            } else {
                status = POKE_NET_GDS_STATUS_CONNECTING;
            }
            POKE_NET_LockMutex();
            gPOKE_NET_Control.Status = status;
            break;
        case POKE_NET_GDS_STATUS_CONNECTING:
            if (POKE_NET_Connecting() == FALSE) {
                if (gPOKE_NET_Control.LastErrorCode == POKE_NET_GDS_LASTERROR_ABORTED) {
                    status = POKE_NET_GDS_STATUS_ABORTED;
                } else {
                    status = POKE_NET_GDS_STATUS_ERROR;
                }
                end = TRUE;
            } else {
                status = POKE_NET_GDS_STATUS_SENDING;
            }
            POKE_NET_LockMutex();
            gPOKE_NET_Control.Status = status;
            break;
        case POKE_NET_GDS_STATUS_SENDING:
            if (POKE_NET_Sending() == FALSE) {
                if (gPOKE_NET_Control.LastErrorCode == POKE_NET_GDS_LASTERROR_ABORTED) {
                    status = POKE_NET_GDS_STATUS_ABORTED;
                } else {
                    status = POKE_NET_GDS_STATUS_ERROR;
                }
                end = TRUE;
            } else {
                status = POKE_NET_GDS_STATUS_RECEIVING;
            }
            if (gPOKE_NET_Control.SendBuffer != NULL) {
                POKE_NET_FreeFunc(gPOKE_NET_Control.SendBuffer);
                gPOKE_NET_Control.SendBuffer = NULL;
            }
            POKE_NET_LockMutex();
            gPOKE_NET_Control.Status = status;
            break;
        case POKE_NET_GDS_STATUS_RECEIVING:
            if (POKE_NET_Receiving() == FALSE) {
                if (gPOKE_NET_Control.LastErrorCode == POKE_NET_GDS_LASTERROR_ABORTED) {
                    status = POKE_NET_GDS_STATUS_ABORTED;
                } else {
                    status = POKE_NET_GDS_STATUS_ERROR;
                }
            } else {
                status = POKE_NET_GDS_STATUS_FINISHED;
            }
            end = TRUE;
            POKE_NET_LockMutex();
            gPOKE_NET_Control.Status = status;
            break;
        }

        if (end) {
            break;
        }
        if (gPOKE_NET_Control.AbortFlag == TRUE) {
            gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_ABORTED;
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ABORTED;
            break;
        }
        POKE_NET_UnlockMutex();
        POKE_NET_Sleep(1);
    }
    POKE_NET_UnlockMutex();
}

// Starts a request with its data, of the given size or of the request's own if size is -1. response is the buffer for
// the answer
BOOL POKE_NET_Request(u16 reqno, void *param, void *response, s32 size) {
    s32 reqsize;
    POKE_NET_REQUESTCOMMON *request;

    POKE_NET_LockMutex();
    switch (gPOKE_NET_Control.Status) {
    case POKE_NET_GDS_STATUS_INACTIVE:
    case POKE_NET_GDS_STATUS_REQUEST:
    case POKE_NET_GDS_STATUS_NETSETTING:
    case POKE_NET_GDS_STATUS_CONNECTING:
    case POKE_NET_GDS_STATUS_SENDING:
    case POKE_NET_GDS_STATUS_RECEIVING:
        POKE_NET_UnlockMutex();
        gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_ERROR;
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_NOTINITIALIZED;
        return FALSE;
    case POKE_NET_GDS_STATUS_ABORTED:
    case POKE_NET_GDS_STATUS_FINISHED:
    case POKE_NET_GDS_STATUS_ERROR:
        POKE_NET_Clear();
        break;
    }
    POKE_NET_UnlockMutex();

    gPOKE_NET_Control.RecvSize = POKE_NET_GetResponseMaxSize(reqno);
    if (gPOKE_NET_Control.RecvSize == -1) {
        gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_ERROR;
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ILLEGALREQUEST;
        return FALSE;
    }
    gPOKE_NET_Control.RecvBuffer = response;

    if (size != -1) {
        reqsize = size + POKE_NET_REQUEST_HEADER_SIZE;
    } else {
        reqsize = POKE_NET_GetRequestSize(reqno);
        if (reqsize == -1) {
            gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_ERROR;
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ILLEGALREQUEST;
            return FALSE;
        }
    }

    gPOKE_NET_Control.SendBuffer = POKE_NET_AllocFunc(reqsize);
    if (gPOKE_NET_Control.SendBuffer == NULL) {
        return FALSE;
    }
    gPOKE_NET_Control.SendSize = reqsize;
    POKE_NET_Memcpy(gPOKE_NET_Control.SendBuffer, &reqsize, POKE_NET_SIZE_LENGTH);
    request = (POKE_NET_REQUESTCOMMON *)(gPOKE_NET_Control.SendBuffer + POKE_NET_SIZE_LENGTH);
    request->ReqCode = reqno;
    request->Auth = gPOKE_NET_Control.Auth;
    request->Option = 0;
    memcpy(request->Param, param, reqsize - POKE_NET_REQUEST_HEADER_SIZE);

    gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_REQUEST;
    POKE_NET_LockMutex();
    if (POKE_NET_CreateThread() == FALSE) {
        gPOKE_NET_Control.Status = POKE_NET_GDS_STATUS_ERROR;
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_CREATETHREAD;
        POKE_NET_UnlockMutex();
        return FALSE;
    }
    POKE_NET_UnlockMutex();
    return TRUE;
}

int POKE_NET_GetStatus(void) {
    return gPOKE_NET_Control.Status;
}

void *POKE_NET_GetResponse(void) {
    POKE_NET_LockMutex();
    if (POKE_NET_GetStatus() != POKE_NET_GDS_STATUS_FINISHED) {
        POKE_NET_UnlockMutex();
        return NULL;
    }
    POKE_NET_UnlockMutex();
    return gPOKE_NET_Control.RecvBuffer + POKE_NET_SIZE_LENGTH;
}

s32 POKE_NET_GetResponseSize(void) {
    POKE_NET_LockMutex();
    if (POKE_NET_GetStatus() != POKE_NET_GDS_STATUS_FINISHED) {
        POKE_NET_UnlockMutex();
        return -1;
    }
    POKE_NET_UnlockMutex();
    return gPOKE_NET_Control.RecvSize;
}

int POKE_NET_GetLastErrorCode(void) {
    POKE_NET_LockMutex();
    if (POKE_NET_GetStatus() != POKE_NET_GDS_STATUS_ERROR) {
        POKE_NET_UnlockMutex();
        return POKE_NET_GDS_LASTERROR_NONE;
    }
    POKE_NET_UnlockMutex();
    return gPOKE_NET_Control.LastErrorCode;
}

void *POKE_NET_AllocFunc(s32 size) {
    return func_020586e4(0, size);
}

void POKE_NET_FreeFunc(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    func_02058728(0, ptr, 0);
}

void POKE_NET_LockMutex(void) {
    OS_LockMutex(&gPOKE_NET_Control.Mutex);
}

void POKE_NET_UnlockMutex(void) {
    OS_UnlockMutex(&gPOKE_NET_Control.Mutex);
}

BOOL POKE_NET_CreateMutex(void) {
    OS_InitMutex(&gPOKE_NET_Control.Mutex);
    return TRUE;
}

BOOL POKE_NET_ReleaseMutex(void) {
    return TRUE;
}

BOOL POKE_NET_ExInitialize(void) {
    if (!clock_is_inited()) {
        clock_init();
    }
    gPOKE_NET_Control.ThreadPriority = POKE_NET_THREAD_PRIORITY;
    return TRUE;
}

void POKE_NET_ExRelease(void) {
}

void POKE_NET_ThreadEntry(void *arg) {
    POKE_NET_ThreadFunction();
}

BOOL POKE_NET_CreateThread(void) {
    scheduler_init_thread(&gPOKE_NET_Control.Thread, POKE_NET_ThreadEntry, NULL,
                          sPOKE_NET_ThreadStack + POKE_NET_THREAD_STACK_SIZE, POKE_NET_THREAD_STACK_SIZE,
                          gPOKE_NET_Control.ThreadPriority);
    scheduler_start_thread(&gPOKE_NET_Control.Thread);
    return TRUE;
}

// Whether a text is an address in numbers, four groups of up to three digits
BOOL POKE_NET_IsIPAddress(const char *text) {
    int digits = 0;
    int dots = 0;

    while (*text != '\0') {
        if (*text >= '0' && *text <= '9') {
            digits++;
            if (digits >= 4) {
                return FALSE;
            }
        } else if (*text == '.') {
            if (digits == 0) {
                return FALSE;
            }
            dots++;
            digits = 0;
            if (dots >= 4) {
                return FALSE;
            }
        } else {
            return FALSE;
        }
        text++;
    }
    if (dots == 3 && digits != 0) {
        return TRUE;
    }
    return FALSE;
}

// Gives a host's address as text, resolving its name if it is one
BOOL POKE_NET_GetHostAddress(const char *host, char *address) {
    if (POKE_NET_IsIPAddress(host) == FALSE) {
        SocHostEnt *entry = func_ov011_02157d40(host);

        if (entry != NULL) {
            strcpy(address, func_ov011_02158014(*(SocInAddr *)entry->addrList[0]));
        } else {
            return FALSE;
        }
    } else {
        strcpy(address, host);
    }
    return TRUE;
}

#define POKE_NET_Swap16(x) ((u16)((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00)))
#define POKE_NET_Swap8x2(x) ((((x) >> 8) & 0x00ff00ff) | (((x) << 8) & 0xff00ff00))

static inline u32 POKE_NET_Swap32(u32 x) {
    x = POKE_NET_Swap8x2(x);
    return (x >> 16) | (x << 16);
}

// Opens the socket and readies its SSL connection to the server
BOOL POKE_NET_Setup(void) {
    char address[16];

    gPOKE_NET_Control.Socket = func_ov011_02157bd0(POKE_NET_SOC_AF_INET, POKE_NET_SOC_SOCK_STREAM, 0);
    if (gPOKE_NET_Control.Socket == -1) {
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_CREATESOCKET;
        return FALSE;
    }
    if (POKE_NET_GetHostAddress(gPOKE_NET_Control.ServerURL, address) == FALSE) {
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_NETWORKSETTING;
        return FALSE;
    }
    gPOKE_NET_Control.ServerAddr = POKE_NET_Swap32(func_ov011_02155d40(address));
    POKE_NET_SSL_StartConnection(gPOKE_NET_Control.ServerURL, &sPOKE_NET_SslConnection);
    if (func_ov011_02158174(gPOKE_NET_Control.Socket, &sPOKE_NET_SslConnection) < 0) {
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_SSL;
        return FALSE;
    }
    return TRUE;
}

void POKE_NET_CloseSocket(void) {
    if (gPOKE_NET_Control.Socket == -1) {
        return;
    }
    func_ov011_02157d30(gPOKE_NET_Control.Socket, POKE_NET_SOC_SHUT_RDWR);
    func_ov011_02157d38(gPOKE_NET_Control.Socket);
    gPOKE_NET_Control.Socket = -1;
    POKE_NET_SSL_EndConnection(&sPOKE_NET_SslConnection);
}

BOOL POKE_NET_Connecting(void) {
    SocAddrIn address;
    s32 start;
    int result;

    address.len = sizeof(SocAddrIn);
    address.family = POKE_NET_SOC_AF_INET;
    address.port = POKE_NET_Swap16(gPOKE_NET_Control.ServerPort);
    address.addr = gPOKE_NET_Control.ServerAddr;
    start = POKE_NET_GetTime();

    for (;;) {
        result = func_ov011_02157c10(gPOKE_NET_Control.Socket, &address);
        if (result >= 0) {
            break;
        }
        if (result == POKE_NET_SOC_EINPROGRESS) {
            if (gPOKE_NET_Control.ConnectTimeOut != 0) {
                if ((s32)(POKE_NET_GetTime() - start) >= gPOKE_NET_Control.ConnectTimeOut) {
                    gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_CONNECTTIMEOUT;
                    return FALSE;
                }
            }
        } else if (result == POKE_NET_SOC_EISCONN) {
            break;
        } else {
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_CONNECT;
            return FALSE;
        }

        POKE_NET_LockMutex();
        if (gPOKE_NET_Control.AbortFlag == TRUE) {
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ABORTED;
            POKE_NET_UnlockMutex();
            return FALSE;
        }
        POKE_NET_UnlockMutex();
        POKE_NET_Sleep(1);
    }
    return TRUE;
}

BOOL POKE_NET_Sending(void) {
    s32 offset = 0;
    s32 start = POKE_NET_GetTime();

    for (;;) {
        s32 size = gPOKE_NET_Control.SendSize - offset;
        int result;

        if (size > gPOKE_NET_Control.SendSize) {
            size = gPOKE_NET_Control.SendSize;
        }
        result = func_ov011_02157cbc(gPOKE_NET_Control.Socket, gPOKE_NET_Control.SendBuffer + offset, size, 0);
        if (result != POKE_NET_SOC_EWOULDBLOCK) {
            if (result < 0) {
                gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_SENDTIMEOUT;
                return FALSE;
            }
            offset += result;
            if (offset == gPOKE_NET_Control.SendSize) {
                break;
            }
        }
        if (gPOKE_NET_Control.SendTimeOut != 0) {
            if ((s32)(POKE_NET_GetTime() - start) >= gPOKE_NET_Control.SendTimeOut) {
                gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_SENDTIMEOUT;
                return FALSE;
            }
        }

        POKE_NET_LockMutex();
        if (gPOKE_NET_Control.AbortFlag == TRUE) {
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ABORTED;
            POKE_NET_UnlockMutex();
            return FALSE;
        }
        POKE_NET_UnlockMutex();
        POKE_NET_Sleep(1);
    }
    return TRUE;
}

// The receiving thread: reads what the socket has whenever the request's thread has taken the last read
void POKE_NET_RecvThreadEntry(void *arg) {
    for (;;) {
        s32 offset = gPOKE_NET_Control.RecvOffset;

        POKE_NET_LockMutex();
        if (!gPOKE_NET_Control.RecvDone) {
            gPOKE_NET_Control.RecvResult =
                func_ov011_02157c50(gPOKE_NET_Control.Socket, gPOKE_NET_Control.RecvBuffer + offset,
                                    gPOKE_NET_Control.RecvSize - offset, 0);
            gPOKE_NET_Control.RecvDone = TRUE;
        }
        POKE_NET_UnlockMutex();
        POKE_NET_Sleep(1);
    }
}

BOOL POKE_NET_Receiving(void) {
    s32 offset = 0;
    s32 size = -1;
    s32 start;
    u64 startTick = clock();

    scheduler_init_thread(&gPOKE_NET_Control.RecvThread, POKE_NET_RecvThreadEntry, NULL,
                          sPOKE_NET_RecvThreadStack + POKE_NET_THREAD_STACK_SIZE, POKE_NET_THREAD_STACK_SIZE,
                          gPOKE_NET_Control.ThreadPriority);
    gPOKE_NET_Control.RecvResult = POKE_NET_SOC_EWOULDBLOCK;
    gPOKE_NET_Control.RecvOffset = 0;
    gPOKE_NET_Control.RecvDone = FALSE;
    scheduler_start_thread(&gPOKE_NET_Control.RecvThread);
    start = POKE_NET_TicksToSeconds(startTick);

    for (;;) {
        if (gPOKE_NET_Control.RecvTimeOut != 0) {
            if ((s32)(POKE_NET_GetTime() - start) >= gPOKE_NET_Control.RecvTimeOut) {
                gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_RECVTIMEOUT;
                func_0207a710(&gPOKE_NET_Control.RecvThread);
                return FALSE;
            }
        }
        if (!gPOKE_NET_Control.RecvDone) {
            continue;
        }
        if (gPOKE_NET_Control.RecvResult != POKE_NET_SOC_EWOULDBLOCK) {
            if (gPOKE_NET_Control.RecvResult <= 0) {
                gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_RECVRESPONSE;
                func_0207a710(&gPOKE_NET_Control.RecvThread);
                return FALSE;
            }
            offset += gPOKE_NET_Control.RecvResult;
            if (offset >= POKE_NET_SIZE_LENGTH && size == -1) {
                POKE_NET_Memcpy(&size, gPOKE_NET_Control.RecvBuffer, POKE_NET_SIZE_LENGTH);
            }
            if (offset == size && size != -1) {
                gPOKE_NET_Control.RecvSize = size - POKE_NET_SIZE_LENGTH;
                break;
            }
            gPOKE_NET_Control.RecvOffset = offset;
        }

        POKE_NET_LockMutex();
        if (gPOKE_NET_Control.AbortFlag == TRUE) {
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ABORTED;
            POKE_NET_UnlockMutex();
            func_0207a710(&gPOKE_NET_Control.RecvThread);
            return FALSE;
        }
        POKE_NET_UnlockMutex();
        gPOKE_NET_Control.RecvDone = FALSE;
        POKE_NET_Sleep(1);
    }
    func_0207a710(&gPOKE_NET_Control.RecvThread);
    return TRUE;
}

void POKE_NET_Sleep(s32 msec) {
    func_0207aa04(msec);
}

// Fetches the token of the Nintendo Wi-Fi Connection's service locator, once, into the request that waits to be sent
BOOL POKE_NET_GetSvlToken(void) {
    DWCSvlResult result;
    int type;
    int code;

    if (gPOKE_NET_Control.HasSvlToken == TRUE) {
        return TRUE;
    }
    if (!func_ov011_0215de44("", &result)) {
        gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_GETSVL;
        return FALSE;
    }

    for (;;) {
        int state = func_ov011_0215dea8();

        if (state == DWC_SVL_STATE_SUCCESS) {
            POKE_NET_Memcpy(gPOKE_NET_Control.Auth.SvlToken, result.svltoken, sizeof(result.svltoken));
            ((POKE_NET_REQUESTCOMMON *)(gPOKE_NET_Control.SendBuffer + POKE_NET_SIZE_LENGTH))->Auth =
                gPOKE_NET_Control.Auth;
            gPOKE_NET_Control.HasSvlToken = TRUE;
            return TRUE;
        } else if (state == DWC_SVL_STATE_ERROR) {
            func_020583b0(&code, &type);
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_GETSVL;
            return FALSE;
        } else if (state == DWC_SVL_STATE_CANCELED) {
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_GETSVL;
            return FALSE;
        }

        POKE_NET_LockMutex();
        if (gPOKE_NET_Control.AbortFlag == TRUE) {
            gPOKE_NET_Control.LastErrorCode = POKE_NET_GDS_LASTERROR_ABORTED;
            POKE_NET_UnlockMutex();
            return FALSE;
        }
        POKE_NET_UnlockMutex();
        POKE_NET_Sleep(1);
    }
    return TRUE;
}

void POKE_NET_SslInitialize(void) {
}

void POKE_NET_SslClear(void) {
    POKE_NET_SSL_Setup();
}
