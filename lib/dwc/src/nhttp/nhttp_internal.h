#ifndef POKEBW2_DWC_NHTTP_INTERNAL_H
#define POKEBW2_DWC_NHTTP_INTERNAL_H

#include "types.h"
#include "dwc/ext.h"
#include "dwc/nhttp.h"
#include "nitro/os.h"

// What the files of Nintendo's HTTP client share. The client is one library in the ROM (overlay 189, in front of the
// GameSpy HTTP code), made of the files of Nintendo's NHTTP SDK, which pret/pokerevo's asm/SDK/NHTTP names for the Wii
// build: NHTTP_os, _socket, _stdlib, _bgnend, _control, _list, _recvbuf, _request, _response and _thread, and the
// d_nhttp files on top. The ROM has none of the file names, so they are guesses from the order and the function
// names of the Wii files, which this code follows (NHTTPi_ is the library's prefix there). A request to a server is
// run by one thread, which takes the requests in the order they were sent.

#define NHTTP_SOCKET_ERROR (-1001)
#define NHTTP_SOCKET_CANCELED (-1002)
#define NHTTP_BUFFER_FULL (-1003)

// What the thread's send buffer and the receive buffer of a connection hold
#define NHTTP_SEND_BUFFER_SIZE 0x4000
#define NHTTP_RECV_BUFFER_SIZE 0x8000
// The size of the first part of a response's header, which is inside the response, and the size of each part that
// follows
#define NHTTP_HEADER_SIZE 0x400
#define NHTTP_HEADER_BLOCK_SIZE 0x200
#define NHTTP_BOUNDARY_SIZE 0x14
// Where the first header field starts in a response's header, after the status line
#define NHTTP_FIRST_FIELD 12

// The methods of a request
#define NHTTP_METHOD_GET 0
#define NHTTP_METHOD_POST 1
#define NHTTP_METHOD_HEAD 2

// How the data of a POST is sent: NHTTP_POST_AUTO uses multipart as soon as one part is binary
#define NHTTP_POST_AUTO 0
#define NHTTP_POST_URLENCODED 1
#define NHTTP_POST_MULTIPART 2

// How a post data part is written
#define NHTTP_PART_ASCII 0
#define NHTTP_PART_ASCII_ENCODED 1
#define NHTTP_PART_RAW 2

// A connection's stage, which the thread moves it through
enum {
    NHTTP_STAGE_NONE,
    NHTTP_STAGE_QUEUED,
    NHTTP_STAGE_SENDING,
    NHTTP_STAGE_RECV_HEADER,
    NHTTP_STAGE_RECV_BODY,
    NHTTP_STAGE_DONE,
};

typedef struct NHttpState NHttpState;
typedef struct NHttpRequest NHttpRequest;
typedef struct NHttpResponse NHttpResponse;
typedef struct NHttpConnection NHttpConnection;
typedef struct NHttpThreadInfo NHttpThreadInfo;
typedef struct NHttpMutexInfo NHttpMutexInfo;

// NHTTP_bgnend's: the allocator, the error and what the thread needs to run
struct NHttpState {
    NHttpAllocFunc alloc;
    NHttpFreeFunc free;
    BOOL started;
    int socket;
    int sslError;
    int error;
    BOOL quit;
    void *stack;
    int unk_20;
    int unk_24;
};

// A part of the request's header or post data: its name and value, and for a post the size of the value
typedef struct NHttpHeader {
    struct NHttpHeader *prev;
    struct NHttpHeader *next;
    char *name;
    char *value;
    int length;
    int binary;
} NHttpHeader;

// A request that was sent and waits for the thread, or that the thread runs
typedef struct NHttpQueueNode {
    struct NHttpQueueNode *prev;
    struct NHttpQueueNode *next;
    int id;
    NHttpRequest *request;
    int socket;
} NHttpQueueNode;

typedef struct NHttpListInfo {
    NHttpQueueNode *head;
    int count;
} NHttpListInfo;

typedef struct NHttpReqInfo {
    NHttpQueueNode *current;
} NHttpReqInfo;

// The mutex that guards the connections, and whether it is made
struct NHttpMutexInfo {
    OSMutex mutex;
    BOOL ready;
};

// The thread that runs the requests, with its message queue (which wakes it when a request was sent) and the buffer
// it sends from
struct NHttpThreadInfo {
    OSMessageQueue queue;
    OSMessage messages[3];
    OSThread thread;
    BOOL queueCreated;
    u8 unk_F0[0x10];
    u8 sendBuffer[NHTTP_SEND_BUFFER_SIZE];
};

// A part of a response's header beyond the first NHTTP_HEADER_SIZE bytes
typedef struct NHttpHeaderBlock {
    struct NHttpHeaderBlock *next;
    char data[NHTTP_HEADER_BLOCK_SIZE];
} NHttpHeaderBlock;

// Called when a response is destroyed with its buffer, the function to free it with and the user's parameter
typedef void (*NHttpReleaseFunc)(void *buffer, NHttpFreeFunc free, void *param);

// What the server answered: the raw header (the first NHTTP_HEADER_SIZE bytes here, the rest in blocks) and the body
// in the user's buffer
struct NHttpResponse {
    int headerSize;
    int bufferUsed;
    int bodyReceived;
    int contentLength;
    BOOL succeeded;
    BOOL headerDone;
    int statusCode;
    int bufferSize;
    char *headerCopy;
    char *valueCopy;
    char *buffer;
    int unk_2C;
    NHttpReleaseFunc release;
    NHttpHeaderBlock *blocks;
    char header[NHTTP_HEADER_SIZE];
    void *userParam;
};

// The settings that NHttpSslConnect gives the socket library to make a connection secure
typedef struct NHttpSslInfo {
    u8 unk_000[0x800];
    char *hostName;
    u8 unk_804[0xc];
    u32 (*filter)(u32);
    void *rootCerts;
    int rootCertCount;
    u8 unk_81C[0x14];
} NHttpSslInfo;

// What a request is: its url and the parts of it, the headers and post data it carries, and the settings that
// NHTTP_SetRootCA and the like give
struct NHttpRequest {
    BOOL canceled;
    BOOL sent;
    BOOL https;
    BOOL useProxy;
    BOOL rawPost;
    int hostEnd;
    int pathStart;
    int method;
    int port;
    char *url;
    char *host;
    NHttpResponse *response;
    NHttpHeader *headers;
    NHttpHeader *postData;
    char boundary[NHTTP_BOUNDARY_SIZE];
    char authorization[0x5c];
    int authorizationSize;
    NHttpSslInfo *ssl;
    int unk_B0;
    int unk_B4;
    int unk_B8;
    int unk_BC;
    void *rootCerts;
    int rootCertCount;
    int unk_C8;
    int unk_CC;
    int verifyOption;
    int postMode;
    int skipVerify;
    int unk_DC;
    u8 unk_E0[4];
    char proxyHost[0x100];
    int proxyPort;
    char proxyAuthorization[0x5c];
    int proxyAuthorizationSize;
    int unk_248;
    const void *rawData;
    int rawSize;
    int unk_254;
};

// What the caller of NHTTP_CreateRequest has: the request and its response, the stage the thread has brought it to and
// the buffer the thread reads from the socket into. Connections are chained in a list
struct NHttpConnection {
    int stage;
    int status;
    int unk_08;
    BOOL running;
    NHttpRequest *request;
    NHttpResponse *response;
    int id;
    NHttpCallback callback;
    NHttpConnection *next;
    void *userBuffer;
    int userBufferSize;
    int virtualContentLength;
    int unk_30;
    u8 unk_34[0xc];
    u8 recvBuffer[NHTTP_RECV_BUFFER_SIZE];
    u32 recvPos;
    u32 recvSize;
    int waiters;
};

// The library: its state, the requests that wait, the request that the thread runs, the mutex and the thread
typedef struct NHttpRoot {
    NHttpState state;
    NHttpListInfo list;
    NHttpReqInfo req;
    NHttpMutexInfo mutexInfo;
    NHttpThreadInfo threadInfo;
} NHttpRoot;

// What the thread keeps on its stack while it runs requests: the request that it takes from the list, the host of the
// last request and its address (kept to connect to the same one again), and how far each step of the request got
typedef struct NHttpThreadWork {
    int id;
    char hostName[0x100];
    char unk_104[0x200];
    char statusLine[0xe];
    u8 unk_312[2];
    u32 address;
    u32 prevAddress;
    int port;
    int prevPort;
    int sendCount;
    int recvCount;
    int contentLength;
    int error;
    int retry;
    BOOL keepAlive;
    BOOL chunked;
} NHttpThreadWork;

// What the callback of a connection gets for NHTTP_EVENT_POST: the name of the part that is sent, and the buffer
// that the callback sets (with its size) to give the part's data. The offset is how much was given before
typedef struct NHttpPostEvent {
    const char *name;
    void *buffer;
    int bufferSize;
    int offset;
} NHttpPostEvent;

// What the callback of a connection gets for the events of the buffer being full and of the body being received: the
// buffer of the answer, its size, and how much of it is filled. The callback may set them to give another buffer
typedef struct NHttpBufferEvent {
    void *buffer;
    int bufferSize;
    int used;
} NHttpBufferEvent;

// Where the thread that waits for a connection to end sleeps
typedef struct NHttpWaitInfo {
    BOOL ready;
    OSMutex mutex;
    OSThreadQueue waiters;
} NHttpWaitInfo;

// What a part of the post data is sent as
#define NHTTP_SEND_RAW 0
#define NHTTP_SEND_PART 1
#define NHTTP_SEND_ENCODED 2

// Whether a character is an upper case letter, for the functions that compare text without caring for the case
static inline BOOL NHTTPi_isUpper(int c) {
    return (c >= 'A') & (c <= 'Z');
}

static inline char NHTTPi_toLower(int c) {
    if (NHTTPi_isUpper(c)) {
        c += 'a' - 'A';
    }
    return c;
}

// NHTTP_os
void NHTTPi_InitMutex(OSMutex *mutex);
void NHTTPi_LockMutex(OSMutex *mutex);
BOOL NHTTPi_createCommThread(NHttpThreadInfo *info, int priority, void *stack);
void NHTTPi_destroyCommThread(NHttpThreadInfo *info, NHttpState *state);
BOOL NHTTPi_idleCommThread(OSMessageQueue *queue);
void NHTTPi_kickCommThread(OSMessageQueue *queue);
void NHTTPi_CheckCurrentThread(NHttpThreadInfo *info, BOOL outside);

// NHTTP_socket
int NHTTPi_SocCheck(int sock);
int NHTTPi_SocOpen(NHttpRequest *request);
int NHTTPi_SocClose(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock);
int NHTTPi_SocConnect(NHttpState *state, NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, u32 addr, int port);
int NHTTPi_SocSSLConnect(NHttpState *state, NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock);
int NHTTPi_SocRecv_sub(NHttpConnection *connection, int sock, void *buffer, int size, int flags);
int NHTTPi_SocRecv(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, void *buffer, int size, int flags);
int NHTTPi_SocSend_sub(int sock, const void *buffer, int size, int flags);
int NHTTPi_SocSend(NHttpRequest *request, int sock, const void *buffer, int size, int flags);
void NHTTPi_SocCancel(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock);
u32 NHTTPi_resolveHostname(NHttpRequest *request, const char *host);

// NHTTP_stdlib
void NHTTPi_memcpy(void *dest, const void *src, u32 size);
int NHTTPi_strlen(const char *s);
int NHTTPi_strnlen(const char *s, int max);
int NHTTPi_strcmp(const char *a, const char *b);
void NHTTPi_memclr(void *dest, u32 size);
int NHTTPi_strnicmp(const char *a, const char *b, int size);
int NHTTPi_getUrlEncodedSize(const char *s);
int NHTTPi_getUrlEncodedSize2(const char *s, int size);
int NHTTPi_encodeUrlChar(char *dest, char c);
int NHTTPi_strToHex(const char *s, int size);
int NHTTPi_strToInt(const char *s, int size);
int NHTTPi_intToStr(char *dest, u32 value);
int NHTTPi_compareToken(const char *a, const char *b);
int NHTTPi_strtonum(const char *s, int size);
int NHTTPi_memfind(const char *haystack, int haystackSize, const char *needle, int needleSize);
int NHTTPi_Base64Encode(char *dest, const char *src);

// NHTTP_bgnend
void NHTTPi_InitBgnEndInfo(NHttpState *state);
void *NHTTPi_alloc(u32 size, int align);
void NHTTPi_free(void *ptr);
void NHTTPi_SetError(NHttpState *state, int error);
void NHTTPi_SetSSLError(NHttpState *state, int error);
int NHTTPi_GetSSLError(NHttpState *state);
BOOL NHTTPi_Startup(NHttpRoot *root, NHttpAllocFunc alloc, NHttpFreeFunc free, int priority);
void NHTTPi_CleanupAsync(NHttpRoot *root, void (*callback)(void));
int NHTTPi_GetError(NHttpState *state);

// NHTTP_control
extern const char NHTTPi_boundary[];
BOOL NHTTPi_addHdrList(NHttpHeader **list, NHttpState *state, const char *name, const char *value);
NHttpHeader *NHTTPi_getHdrFromList(void *list);
BOOL NHTTP_AddHeaderField(NHttpRequest *request, NHttpState *state, const char *name, const char *value);
BOOL NHTTP_AddPostDataAscii(NHttpRequest *request, NHttpState *state, const char *name, const char *value);

// NHTTP_list
void NHTTPi_InitListInfo(NHttpListInfo *info);
int NHTTPi_setReqQueue(NHttpListInfo *info, NHttpRequest *request);
NHttpQueueNode *NHTTPi_findReqQueue(NHttpListInfo *info, int id);
BOOL NHTTPi_freeReqQueue(NHttpListInfo *info, NHttpMutexInfo *mutexInfo, int id);
void NHTTPi_allFreeReqQueue(NHttpListInfo *info, NHttpMutexInfo *mutexInfo);
NHttpQueueNode *NHTTPi_getReqFromReqQueue(NHttpListInfo *info);

// NHTTP_recvbuf
BOOL NHTTPi_isRecvBufFull(NHttpResponse *response, u32 size);
int NHTTPi_findNextLineHdrRecvBuf(NHttpResponse *response, int start, int end, int *colon, int *lineEnd);
int NHTTPi_skipSpaceHdrRecvBuf(NHttpResponse *response, int start, int end);
int NHTTPi_compareTokenN_HdrRecvBuf(NHttpResponse *response, int start, int end, const char *token, int terminator);
BOOL NHTTPi_loadFromHdrRecvBuf(NHttpResponse *response, char *dest, int start, int size);
int NHTTPi_RecvBuf(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, int offset, int flags);
int NHTTPi_RecvBufN(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, int offset, int size, int flags);

// NHTTP_request
void NHTTPi_InitRequestInfo(NHttpReqInfo *info);
NHttpRequest *NHTTP_CreateRequest(NHttpState *state, const char *url, int method, void *buffer, int bufferSize,
                                  void *param, int unused, NHttpReleaseFunc release);
void NHTTP_DestroyRequest(NHttpRoot *root, NHttpRequest *request);
BOOL NHTTPi_destroyRequestObject(NHttpMutexInfo *mutexInfo, NHttpRequest *request);
int NHTTP_SendRequestAsync(NHttpRoot *root, NHttpRequest *request);
BOOL NHTTP_CancelRequestAsync(NHttpRoot *root, int id);
void NHTTPi_cancelAllRequests(NHttpRoot *root);

// NHTTP_response
void NHTTP_DestroyResponse(NHttpMutexInfo *mutexInfo, NHttpResponse *response);
int NHTTPi_getHeaderValue(NHttpResponse *response, const char *name, int *start);
int NHTTP_GetHeaderField(NHttpResponse *response, const char *name, char **value);
int NHTTP_GetHeaderAll(NHttpState *state, NHttpResponse *response, char **header);

// NHTTP_thread
void NHTTPi_InitThreadInfo(NHttpThreadInfo *info);
void NHTTPi_IsCreateCommThreadMessageQueueOn(NHttpThreadInfo *info);
void NHTTPi_IsCreateCommThreadMessageQueueOff(NHttpThreadInfo *info);
BOOL NHTTPi_IsCreateCommThreadMessageQueue(NHttpThreadInfo *info);
BOOL NHTTPi_CheckHeaderEnd(const char *tail, int count);
int NHTTPi_SaveBuf(NHttpRequest *request, u8 *sendBuffer, int sock, int *count, const u8 *data, int size);
void NHTTPi_CommThreadProcMain(void);

// d_nhttp_common
NHttpRoot *NHTTPi_GetSystemInfoP(void);
NHttpState *NHTTPi_GetBgnEndInfoP(NHttpRoot *root);
NHttpListInfo *NHTTPi_GetListInfoP(NHttpRoot *root);
NHttpReqInfo *NHTTPi_GetReqInfoP(NHttpRoot *root);
NHttpThreadInfo *NHTTPi_GetThreadInfoP(NHttpRoot *root);
NHttpMutexInfo *NHTTPi_GetMutexInfoP(NHttpRoot *root);
void NHTTPi_SetVirtualContentLength(NHttpConnection *connection, int length);
void NHTTPi_InitMutexInfo(NHttpMutexInfo *mutexInfo);
void NHTTPi_initLockReqList(NHttpMutexInfo *mutexInfo);
void NHTTPi_exitLockReqList(void);
void NHTTPi_InitConnectionList(void);
int NHTTPi_AddWaiter(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
int NHTTPi_RemoveWaiter(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
void NHTTPi_lockReqList(NHttpMutexInfo *mutexInfo);
void NHTTPi_unlockReqList(NHttpMutexInfo *mutexInfo);
NHttpConnection *NHTTPi_ControlConnectionList(NHttpMutexInfo *mutexInfo, NHttpConnection *key, int mode);
int NHTTPi_CommitConnectionList(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
int NHTTPi_OmitConnectionList(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
NHttpResponse *NHTTPi_Connection2Response(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
NHttpConnection *NHTTPi_Request2Connection(NHttpMutexInfo *mutexInfo, NHttpRequest *request);
NHttpConnection *NHTTPi_Response2Connection(NHttpMutexInfo *mutexInfo, NHttpResponse *response);
NHttpConnection *NHTTPi_GetConnection(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
NHttpRequest *NHTTPi_GetRequest(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
NHttpResponse *NHTTPi_GetResponse(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
int NHTTPi_GetConnectionListLength(void);
int NHTTPi_PostSendCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection, const char *name, int offset);
void NHTTPi_BufferFullCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
void NHTTPi_ReceivedCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
void NHTTPi_CompleteCallback(NHttpMutexInfo *mutexInfo, NHttpConnection *connection);
void NHTTPi_WaitConnection(NHttpConnection *connection);
void NHTTPi_WakeConnection(NHttpConnection *connection);

// d_nhttp_private
int NHTTPi_DestroyConnection(NHttpConnection *connection);
int NHTTPi_CancelConnection(NHttpConnection *connection);

#endif // POKEBW2_DWC_NHTTP_INTERNAL_H
