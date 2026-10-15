#ifndef POKEBW2_DWC_GHTTP_INTERNAL_H
#define POKEBW2_DWC_GHTTP_INTERNAL_H

#include "types.h"
#include "dwc/ext.h"

// What the files of the GameSpy HTTP client share, in overlay 189 after the NHTTP client. The ROM names one of the
// files, ghttpEncryption.c, so the others are named after the files of GameSpy's ghttp that pret/pokerevo's Wii build
// has (asm/SDK/DWC/gamespy/ghttp*.s), in the same order: Buffer, Callbacks, Common, Connection, Encryption, Main, Post and
// Process. The functions have the names they have there (and in the ROM's debug texts)

typedef struct GHIConnection GHIConnection;
typedef struct GHIEncryptor GHIEncryptor;

// What the encryption keeps of a connection: its SSL connection (ghttpEncryption.c) and whether that has started
typedef struct GHIEncryptorState {
    struct GHISslConnection *ssl;
    BOOL started;
} GHIEncryptorState;

// The callbacks of a request: when it has ended (returns FALSE to keep the buffer), while it goes on, and while a post
// is sent
typedef BOOL (*GHICompletedFunc)(int request, int result, char *buffer, int bufferLen, void *param);
typedef void (*GHIProgressFunc)(int request, int state, const char *buffer, int bufferLen, int bytesReceived,
                                int totalSize, void *param);
typedef void (*GHIPostFunc)(int request, int bytesPosted, int totalBytes, int index, int count, void *param);

struct GHIPost {
    void *items;
    GHIPostFunc callback;
    int unk_08;
    BOOL hasFiles;
    BOOL hasXml;
    BOOL mixed;
    BOOL autoFree;
};

// A growing (or fixed) text that a connection builds or receives into
typedef struct GHIBuffer {
    GHIConnection *connection;
    char *data;
    int size;
    int length;
    int pos;
    int sizeIncrement;
    BOOL fixed;
    BOOL dontFree;
    BOOL readOnly;
} GHIBuffer;

// Swaps the two bytes of a number to or from the network order (a macro, so its argument is read twice)
#define GHI_SWAP16(x) ((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))

// Clears a few bytes of what was just allocated; the size is always known, and a short one is cleared without a loop
static inline void ghiClear(void *dest, u32 size) {
    char *p = dest;

    while (size) {
        *p++ = 0;
        size--;
    }
}

// The same for a copy of a few bytes
static inline void ghiCopy(void *dest, const void *src, u32 size) {
    char *d = dest;
    const char *s = src;

    while (size) {
        *d++ = *s++;
        size--;
    }
}

// The certificates a root of trust is made of: the array and its length
typedef struct GHICertList {
    void **certs;
    int count;
} GHICertList;

// The size of the pieces that the data of a buffer is encrypted in
#define GHI_ENCRYPT_CHUNK_SIZE 0x3f01

// The encryption of a connection (ghttpEncryption.c): its engine, and the functions that start it, end it and
// (en|de)crypt the data
typedef int (*GHIEncryptFunc)(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int inSize, char *out,
                               int *outSize);
typedef int (*GHIDecryptFunc)(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int *inSize, char *out,
                               int *outSize);
struct GHIEncryptor {
    GHIEncryptorState *state;
    int engine;
    BOOL open;
    int unk_0C;
    BOOL active;
    int mode;
    BOOL established;
    int unk_1C;
    int (*init)(GHIConnection *connection, GHIEncryptor *encryptor);
    BOOL (*cleanup)(GHIConnection *connection, GHIEncryptor *encryptor);
    int (*start)(GHIConnection *connection, GHIEncryptor *encryptor);
    GHIEncryptFunc encrypt;
    GHIDecryptFunc decrypt;
    BOOL secure;
};

// The progress of a post and the callback that tells the game of it
typedef struct GHIPostingState {
    void *states;
    int index;
    int bytesPosted;
    int totalBytes;
    GHIPostFunc callback;
    int unk_14;
    BOOL isXml;
    BOOL finished;
} GHIPostingState;

// A request that is in progress: the slot of the connections' list it has, what it asked for and the state it is in
// (the state machine of ghttpProcess.c)
struct GHIConnection {
    BOOL inUse;
    int request;
    int uniqueId;
    int type;
    int state;
    char *url;
    char *serverAddress;
    u32 serverIp;
    u16 serverPort;
    u8 pad_22[2];
    char *requestPath;
    BOOL secure;
    char *headers;
    int unk_30;
    char *unk_34;
    BOOL blocking;
    BOOL keepAlive;
    int result;
    GHIProgressFunc progressCallback;
    GHICompletedFunc completedCallback;
    void *callbackParam;
    int socket;
    int socketError;
    GHIBuffer sendBuffer;
    GHIBuffer encodeBuffer;
    GHIBuffer recvBuffer;
    GHIBuffer decodeBuffer;
    GHIBuffer fileBuffer;
    BOOL userBuffer;
    int httpMajor;
    int httpMinor;
    int statusCode;
    int statusTextPos;
    int headersPos;
    BOOL completed;
    int bytesReceived;
    int totalSize;
    char *redirectUrl;
    int redirectCount;
    BOOL chunked;
    char chunkHeader[12];
    int chunkHeaderLength;
    int chunkRemaining;
    int chunkState;
    BOOL processing;
    BOOL closing;
    int throttle;
    u32 lastThrottleTime;
    GHIPost *post;
    GHIPostingState postingState;
    int maxRecvTime;
    char *proxyAddress;
    u16 proxyPort;
    u8 pad_192[2];
    GHIEncryptor encryptor;
    void *hostLookup;
    u32 lastRecvTime;
};

// The allocator that DWC's code gets its memory from (a heap of the game), and its free
void *func_020586e4(int heap, u32 size);
void func_02058728(int heap, void *ptr, int unused);

// ghttpBuffer
BOOL ghiResizeBuffer(GHIBuffer *buffer, int count);
BOOL ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement);
BOOL ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *data, int size);
void ghiFreeBuffer(GHIBuffer *buffer);
BOOL ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int size);
BOOL ghiEncryptDataToBuffer(GHIBuffer *buffer, const char *data, int size);
BOOL ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value);
BOOL ghiAppendCharToBuffer(GHIBuffer *buffer, int c);
BOOL ghiAppendIntToBuffer(GHIBuffer *buffer, int value);
void ghiResetBuffer(GHIBuffer *buffer);
BOOL ghiSendBufferedData(GHIConnection *connection);

// ghttpCallbacks
void ghiCallCompletedCallback(GHIConnection *connection);
void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, int length);
void ghiCallPostCallback(GHIConnection *connection);

// ghttpCommon
extern int ghiThrottleInterval;
extern int ghiThrottleBytes;
extern u16 ghiProxyPort;
extern char *ghiProxyAddress;
void ghiCreateLock(void);
void ghiFreeLock(void);
void ghiLock(void);
void ghiUnlock(void);
BOOL ghiDecryptReceivedData(GHIConnection *connection);
int ghiDoReceive(GHIConnection *connection, char *buffer, int *size);
int ghiDoSend(GHIConnection *connection, const char *data, int size);
int ghiTrySendThenBuffer(GHIConnection *connection, const char *data, int size);

// ghttpConnection: the list of the connections that are in use or free
typedef BOOL (*GHIEnumFunc)(GHIConnection *connection);
int ghiFindFreeSlot(void);
GHIConnection *ghiNewConnection(void);
BOOL ghiCloseSocket(GHIConnection *connection);
BOOL ghiFreeConnectionData(GHIConnection *connection);
BOOL ghiFreeConnection(GHIConnection *connection);
GHIConnection *ghiRequestToConnection(int request);
void ghiEnumConnections(GHIEnumFunc func);
void ghiRedirectConnection(GHIConnection *connection);
void ghiCleanupConnections(void);

char *ghiCopyHost(char *url);

// ghttpMain: the calls of the game (and DWC) to the client
void ghiHandleStatus(GHIConnection *connection);
BOOL ghiProcessConnection(GHIConnection *connection);
void ghttpStartup(void);
void ghttpCleanup(void);
int ghttpGetExA(const char *url, const char *headers, char *buffer, int bufferSize, GHIPost *post, BOOL throttle,
                BOOL blocking, GHIProgressFunc progress, GHICompletedFunc completed, void *param);
void ghttpThink(void);
BOOL ghttpThinkRequest(int request);
void ghttpCancelRequest(int request);
int ghttpGetState(int request);
void ghttpSetMaxRecvTime(int request, int time);
GHIPost *ghttpNewPost(void);
void ghttpPostSetAutoFree(GHIPost *post, BOOL autoFree);
void ghttpFreePost(int request, GHIPost *post);
BOOL ghttpPostAddXml(GHIPost *post, SoapWriter *writer);
BOOL ghttpSetRootCAList(const char *url, GHICertList *rootCA);
BOOL ghttpRemoveRootCAList(const char *url);

// ghttpPost: a post is a list of items; the one of a connection is made by the game and the connection keeps its state
GHIPost *ghiNewPost(void);
void ghiPostSetAutoFree(GHIPost *post, BOOL autoFree);
void ghiFreePost(GHIPost *post);
void ghiFreePostOfRequest(int request, GHIPost *post);
BOOL ghiPostAddXml(GHIPost *post, SoapWriter *writer);
const char *ghiPostGetContentType(GHIConnection *connection);
int ghiPostGetLength(GHIConnection *connection);
BOOL ghiPostInitState(GHIConnection *connection);
int ghiPostDoPosting(GHIConnection *connection);
void ghiPostCleanupState(GHIConnection *connection);

// ghttpProcess: what each state of a connection does
BOOL ghiParseURL(GHIConnection *connection);
BOOL ghiProcessIncomingFileData(GHIConnection *connection, const char *data, int size);
void ghiDoSocketInit(GHIConnection *connection);
void ghiDoHostLookup(GHIConnection *connection);
void ghiDoLookupPending(GHIConnection *connection);
void ghiDoConnecting(GHIConnection *connection);
void ghiDoSecuringSession(GHIConnection *connection);
void ghiDoSendingRequest(GHIConnection *connection);
void ghiDoPosting(GHIConnection *connection);
void ghiDoWaiting(GHIConnection *connection);
void ghiDoReceivingStatus(GHIConnection *connection);
void ghiDoReceivingHeaders(GHIConnection *connection);
void ghiDoReceivingFile(GHIConnection *connection);

// ghttpEncryption
BOOL ghttpSetRequestEncryptionEngine(int request, int engine);
BOOL ghiEncyptorSetRootCAList(const char *url, GHICertList *rootCA);
BOOL ghiEncryptorRemoveRootCAList(const char *url);
GHICertList *ghiEncryptorGetRootCAList(const char *url);
BOOL ghiEncryptorSetTwlRootCA(const char *url, GHICertList *rootCA);
BOOL ghiEncryptorCleanupTwlRootCA(const char *url);
int ghiEncryptorSslInitFunc(GHIConnection *connection, GHIEncryptor *encryptor);
int ghiEncryptorSslStartFunc(GHIConnection *connection, GHIEncryptor *encryptor);
BOOL ghiEncryptorSslCleanupFunc(GHIConnection *connection, GHIEncryptor *encryptor);
int ghiEncryptorSslEncryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int inSize,
                               char *out, int *outSize);
int ghiEncryptorSslDecryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int *inSize,
                               char *out, int *outSize);
int ghiEncryptorSend(GHIConnection *connection, GHIEncryptor *encryptor, const char *data, int size, int *sent);
int ghiEncryptorRecv(GHIConnection *connection, GHIEncryptor *encryptor, char *buffer, int *size);

#endif // POKEBW2_DWC_GHTTP_INTERNAL_H
