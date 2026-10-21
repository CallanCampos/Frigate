#ifndef POKEBW2_DWC_GHTTP_INTERNAL_H
#define POKEBW2_DWC_GHTTP_INTERNAL_H

#include "dwc/ext.h"
#include "dwc/memfunc.h"
#include "dwc/nonport.h"

// What the files of the GameSpy HTTP client share, in overlay 189 after the NHTTP client. The ROM names one of the
// files, ghttpEncryption.c, so the others are named after the files of GameSpy's ghttp that pret/pokerevo's Wii build
// has (asm/SDK/DWC/gamespy/ghttp*.s), in the same order: Buffer, Callbacks, Common, Connection, Encryption, Main, Post
// and Process. The functions have the names they have there (and in the ROM's debug texts)

typedef struct GHIConnection GHIConnection;
typedef struct GHIEncryptor GHIEncryptor;

// What the encryption keeps of a connection: its SSL connection (ghttpEncryption.c) and whether that has started
typedef struct GHIEncryptorState {
    struct GHISslConnection *ssl;
    gsi_bool started;
} GHIEncryptorState;

// The callbacks of a request: when it has ended (returns gsi_false to keep the buffer), while it goes on, and while a
// post is sent
typedef gsi_bool (*GHICompletedFunc)(int request, int result, char *buffer, int bufferLen, void *param);
typedef void (*GHIProgressFunc)(int request, int state, const char *buffer, int bufferLen, int bytesReceived,
                                int totalSize, void *param);
typedef void (*GHIPostFunc)(int request, int bytesPosted, int totalBytes, int index, int count, void *param);

struct GHIPost {
    void *items;
    GHIPostFunc callback;
    int unk_08;
    gsi_bool hasFiles;
    gsi_bool hasXml;
    gsi_bool mixed;
    gsi_bool autoFree;
};

// A growing (or fixed) text that a connection builds or receives into
typedef struct GHIBuffer {
    GHIConnection *connection;
    char *data;
    int size;
    int length;
    int pos;
    int sizeIncrement;
    gsi_bool fixed;
    gsi_bool dontFree;
    gsi_bool readOnly;
} GHIBuffer;

// Swaps the two bytes of a number to or from the network order (a macro, so its argument is read twice)
#define GHI_SWAP16(x) ((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))

// Clears a few bytes of what was just allocated; the size is always known, and a short one is cleared without a loop
static inline void ghiClear(void *dest, gsi_u32 size) {
    char *p = dest;

    while (size) {
        *p++ = 0;
        size--;
    }
}

// The same for a copy of a few bytes
static inline void ghiCopy(void *dest, const void *src, gsi_u32 size) {
    gsi_u8 *d = dest;
    const gsi_u8 *s = src;

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
typedef int (*GHIDecryptFunc)(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int *inSize,
                              char *out, int *outSize);
struct GHIEncryptor {
    GHIEncryptorState *state;
    int engine;
    gsi_bool open;
    int unk_0C;
    gsi_bool active;
    int mode;
    gsi_bool established;
    int unk_1C;
    int (*init)(GHIConnection *connection, GHIEncryptor *encryptor);
    gsi_bool (*cleanup)(GHIConnection *connection, GHIEncryptor *encryptor);
    int (*start)(GHIConnection *connection, GHIEncryptor *encryptor);
    GHIEncryptFunc encrypt;
    GHIDecryptFunc decrypt;
    gsi_bool secure;
};

// The progress of a post and the callback that tells the game of it
typedef struct GHIPostingState {
    void *states;
    int index;
    int bytesPosted;
    int totalBytes;
    GHIPostFunc callback;
    int unk_14;
    gsi_bool isXml;
    gsi_bool finished;
} GHIPostingState;

// A request that is in progress: the slot of the connections' list it has, what it asked for and the state it is in
// (the state machine of ghttpProcess.c)
struct GHIConnection {
    gsi_bool inUse;
    int request;
    int uniqueId;
    int type;
    int state;
    char *url;
    char *serverAddress;
    gsi_u32 serverIp;
    gsi_u16 serverPort;
    gsi_u8 pad_22[2];
    char *requestPath;
    gsi_bool secure;
    char *headers;
    int unk_30;
    char *unk_34;
    gsi_bool blocking;
    gsi_bool keepAlive;
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
    gsi_bool userBuffer;
    int httpMajor;
    int httpMinor;
    int statusCode;
    int statusTextPos;
    int headersPos;
    gsi_bool completed;
    int bytesReceived;
    int totalSize;
    char *redirectUrl;
    int redirectCount;
    gsi_bool chunked;
    char chunkHeader[12];
    int chunkHeaderLength;
    int chunkRemaining;
    int chunkState;
    gsi_bool processing;
    gsi_bool closing;
    int throttle;
    gsi_u32 lastThrottleTime;
    GHIPost *post;
    GHIPostingState postingState;
    int maxRecvTime;
    char *proxyAddress;
    gsi_u16 proxyPort;
    gsi_u8 pad_192[2];
    GHIEncryptor encryptor;
    void *hostLookup;
    gsi_u32 lastRecvTime;
};

// The allocator that DWC's code gets its memory from (a heap of the game), and its free, are in dwc/memfunc.h

// ghttpBuffer
gsi_bool ghiResizeBuffer(GHIBuffer *buffer, int count);
gsi_bool ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement);
gsi_bool ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *data, int size);
void ghiFreeBuffer(GHIBuffer *buffer);
gsi_bool ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int size);
gsi_bool ghiEncryptDataToBuffer(GHIBuffer *buffer, const char *data, int size);
gsi_bool ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value);
gsi_bool ghiAppendCharToBuffer(GHIBuffer *buffer, int c);
gsi_bool ghiAppendIntToBuffer(GHIBuffer *buffer, int value);
void ghiResetBuffer(GHIBuffer *buffer);
gsi_bool ghiSendBufferedData(GHIConnection *connection);

// ghttpCallbacks
void ghiCallCompletedCallback(GHIConnection *connection);
void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, int length);
void ghiCallPostCallback(GHIConnection *connection);

// ghttpCommon
extern int ghiThrottleInterval;
extern int ghiThrottleBytes;
extern gsi_u16 ghiProxyPort;
extern char *ghiProxyAddress;
void ghiCreateLock(void);
void ghiFreeLock(void);
void ghiLock(void);
void ghiUnlock(void);
gsi_bool ghiDecryptReceivedData(GHIConnection *connection);
int ghiDoReceive(GHIConnection *connection, char *buffer, int *size);
int ghiDoSend(GHIConnection *connection, const char *data, int size);
int ghiTrySendThenBuffer(GHIConnection *connection, const char *data, int size);

// ghttpConnection: the list of the connections that are in use or free
typedef gsi_bool (*GHIEnumFunc)(GHIConnection *connection);
int ghiFindFreeSlot(void);
GHIConnection *ghiNewConnection(void);
gsi_bool ghiCloseSocket(GHIConnection *connection);
gsi_bool ghiFreeConnectionData(GHIConnection *connection);
gsi_bool ghiFreeConnection(GHIConnection *connection);
GHIConnection *ghiRequestToConnection(int request);
void ghiEnumConnections(GHIEnumFunc func);
void ghiRedirectConnection(GHIConnection *connection);
void ghiCleanupConnections(void);

char *ghiCopyHost(char *url);

// ghttpMain: the calls of the game (and DWC) to the client
void ghiHandleStatus(GHIConnection *connection);
gsi_bool ghiProcessConnection(GHIConnection *connection);
void ghttpStartup(void);
void ghttpCleanup(void);
int ghttpGetExA(const char *url, const char *headers, char *buffer, int bufferSize, GHIPost *post, gsi_bool throttle,
                gsi_bool blocking, GHIProgressFunc progress, GHICompletedFunc completed, void *param);
void ghttpThink(void);
gsi_bool ghttpThinkRequest(int request);
void ghttpCancelRequest(int request);
int ghttpGetState(int request);
void ghttpSetMaxRecvTime(int request, int time);
GHIPost *ghttpNewPost(void);
void ghttpPostSetAutoFree(GHIPost *post, gsi_bool autoFree);
void ghttpFreePost(int request, GHIPost *post);
gsi_bool ghttpPostAddXml(GHIPost *post, SoapWriter *writer);
gsi_bool ghttpSetRootCAList(const char *url, GHICertList *rootCA);
gsi_bool ghttpRemoveRootCAList(const char *url);

// ghttpPost: a post is a list of items; the one of a connection is made by the game and the connection keeps its state
GHIPost *ghiNewPost(void);
void ghiPostSetAutoFree(GHIPost *post, gsi_bool autoFree);
void ghiFreePost(GHIPost *post);
void ghiFreePostOfRequest(int request, GHIPost *post);
gsi_bool ghiPostAddXml(GHIPost *post, SoapWriter *writer);
const char *ghiPostGetContentType(GHIConnection *connection);
int ghiPostGetLength(GHIConnection *connection);
gsi_bool ghiPostInitState(GHIConnection *connection);
int ghiPostDoPosting(GHIConnection *connection);
void ghiPostCleanupState(GHIConnection *connection);

// ghttpProcess: what each state of a connection does
gsi_bool ghiParseURL(GHIConnection *connection);
gsi_bool ghiProcessIncomingFileData(GHIConnection *connection, const char *data, int size);
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
gsi_bool ghttpSetRequestEncryptionEngine(int request, int engine);
gsi_bool ghiEncyptorSetRootCAList(const char *url, GHICertList *rootCA);
gsi_bool ghiEncryptorRemoveRootCAList(const char *url);
GHICertList *ghiEncryptorGetRootCAList(const char *url);
gsi_bool ghiEncryptorSetTwlRootCA(const char *url, GHICertList *rootCA);
gsi_bool ghiEncryptorCleanupTwlRootCA(const char *url);
int ghiEncryptorSslInitFunc(GHIConnection *connection, GHIEncryptor *encryptor);
int ghiEncryptorSslStartFunc(GHIConnection *connection, GHIEncryptor *encryptor);
gsi_bool ghiEncryptorSslCleanupFunc(GHIConnection *connection, GHIEncryptor *encryptor);
int ghiEncryptorSslEncryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int inSize,
                               char *out, int *outSize);
int ghiEncryptorSslDecryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int *inSize,
                               char *out, int *outSize);
int ghiEncryptorSend(GHIConnection *connection, GHIEncryptor *encryptor, const char *data, int size, int *sent);
int ghiEncryptorRecv(GHIConnection *connection, GHIEncryptor *encryptor, char *buffer, int *size);

#endif // POKEBW2_DWC_GHTTP_INTERNAL_H
