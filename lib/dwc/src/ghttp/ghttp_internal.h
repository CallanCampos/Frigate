#ifndef POKEBW2_DWC_GHTTP_INTERNAL_H
#define POKEBW2_DWC_GHTTP_INTERNAL_H

#include "dwc/darray.h"
#include "dwc/ext.h"
#include "dwc/ghttp.h"
#include "dwc/gsPlatformSocket.h"
#include "dwc/gsXML.h"
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

// A post: its items, the callback that tells the game of its progress, and what kinds of items it has
typedef struct GHIPost {
    DArray items;
    ghttpPostCallback callback;
    void *param;
    GHTTPBool hasFiles;
    GHTTPBool hasXml;
    GHTTPBool mixed;
    GHTTPBool autoFree;
} GHIPost;

// A growing (or fixed) text that a connection builds or receives into
typedef struct GHIBuffer {
    GHIConnection *connection;
    char *data;
    int size;
    int length;
    int pos;
    int sizeIncrement;
    GHTTPBool fixed;
    GHTTPBool dontFree;
    GHTTPBool readOnly;
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
typedef enum {
    GHIEncryptionResult_None,
    GHIEncryptionResult_Success,
    GHIEncryptionResult_BufferTooSmall,
    GHIEncryptionResult_Error
} GHIEncryptionResult;

typedef GHIEncryptionResult (*GHTTPEncryptorInitFunc)(GHIConnection *connection, GHIEncryptor *encryptor);
typedef GHIEncryptionResult (*GHTTPEncryptorStartFunc)(GHIConnection *connection, GHIEncryptor *encryptor);
typedef GHIEncryptionResult (*GHTTPEncryptorCleanupFunc)(GHIConnection *connection, GHIEncryptor *encryptor);
typedef GHIEncryptionResult (*GHTTPEncryptorEncryptFunc)(GHIConnection *connection, GHIEncryptor *encryptor,
                                                         const char *in, int inSize, char *out, int *outSize);
typedef GHIEncryptionResult (*GHTTPEncryptorDecryptFunc)(GHIConnection *connection, GHIEncryptor *encryptor,
                                                         const char *in, int *inSize, char *out, int *outSize);
struct GHIEncryptor {
    GHIEncryptorState *state;
    GHTTPEncryptionEngine engine;
    GHTTPBool open;
    GHTTPBool unk_0C;
    GHTTPBool active;
    GHTTPBool mode;
    GHTTPBool established;
    GHTTPBool unk_1C;
    GHTTPEncryptorInitFunc init;
    GHTTPEncryptorCleanupFunc cleanup;
    GHTTPEncryptorStartFunc start;
    GHTTPEncryptorEncryptFunc encrypt;
    GHTTPEncryptorDecryptFunc decrypt;
    gsi_bool secure;
};

// The progress of a post and the callback that tells the game of it
typedef struct GHIPostingState {
    DArray states;
    int index;
    int bytesPosted;
    int totalBytes;
    ghttpPostCallback callback;
    void *param;
    GHTTPBool isXml;
    GHTTPBool finished;
} GHIPostingState;

typedef enum { GHIGET, GHISAVE, GHISTREAM, GHIHEAD, GHIPOST } GHIRequestType;

typedef enum { GHIHttp, GHIHttps } GHIProtocol;

// Where a chunked transfer is: a chunk's header, its data, the line end after it, or the footer
typedef enum { CRHeader, CRChunk, CRCRLF, CRFooter } CRState;

// A request that is in progress: the slot of the connections' list it has, what it asked for and the state it is in
// (the state machine of ghttpProcess.c)
struct GHIConnection {
    GHTTPBool inUse;
    GHTTPRequest request;
    int uniqueId;
    GHIRequestType type;
    GHTTPState state;
    char *url;
    char *serverAddress;
    unsigned int serverIp;
    unsigned short serverPort;
    gsi_u8 pad_22[2];
    char *requestPath;
    GHIProtocol secure;
    char *headers;
    int unk_30;
    char *unk_34;
    GHTTPBool blocking;
    GHTTPBool keepAlive;
    GHTTPResult result;
    ghttpProgressCallback progressCallback;
    ghttpCompletedCallback completedCallback;
    void *callbackParam;
    SOCKET socket;
    int socketError;
    GHIBuffer sendBuffer;
    GHIBuffer encodeBuffer;
    GHIBuffer recvBuffer;
    GHIBuffer decodeBuffer;
    GHIBuffer fileBuffer;
    GHTTPBool userBuffer;
    int httpMajor;
    int httpMinor;
    int statusCode;
    int statusTextPos;
    int headersPos;
    GHTTPBool completed;
    GHTTPByteCount bytesReceived;
    GHTTPByteCount totalSize;
    char *redirectUrl;
    int redirectCount;
    GHTTPBool chunked;
    char chunkHeader[12];
    int chunkHeaderLength;
    int chunkRemaining;
    CRState chunkState;
    GHTTPBool processing;
    GHTTPBool closing;
    GHTTPBool throttle;
    gsi_time lastThrottleTime;
    GHTTPPost post;
    GHIPostingState postingState;
    gsi_time maxRecvTime;
    char *proxyAddress;
    unsigned short proxyPort;
    gsi_u8 pad_192[2];
    GHIEncryptor encryptor;
    GSIResolveHostnameHandle hostLookup;
    gsi_time lastRecvTime;
};

// The allocator that DWC's code gets its memory from (a heap of the game), and its free, are in dwc/memfunc.h

// ghttpBuffer
GHTTPBool ghiResizeBuffer(GHIBuffer *buffer, int count);
GHTTPBool ghiInitBuffer(GHIConnection *connection, GHIBuffer *buffer, int initialSize, int sizeIncrement);
GHTTPBool ghiInitFixedBuffer(GHIConnection *connection, GHIBuffer *buffer, char *data, int size);
void ghiFreeBuffer(GHIBuffer *buffer);
GHTTPBool ghiAppendDataToBuffer(GHIBuffer *buffer, const char *data, int size);
GHTTPBool ghiEncryptDataToBuffer(GHIBuffer *buffer, const char *data, int size);
GHTTPBool ghiAppendHeaderToBuffer(GHIBuffer *buffer, const char *name, const char *value);
GHTTPBool ghiAppendCharToBuffer(GHIBuffer *buffer, int c);
GHTTPBool ghiAppendIntToBuffer(GHIBuffer *buffer, int value);
void ghiResetBuffer(GHIBuffer *buffer);
GHTTPBool ghiSendBufferedData(GHIConnection *connection);

// ghttpCallbacks
void ghiCallCompletedCallback(GHIConnection *connection);
void ghiCallProgressCallback(GHIConnection *connection, const char *buffer, GHTTPByteCount length);
void ghiCallPostCallback(GHIConnection *connection);

// ghttpCommon
typedef enum { GHIRecvData, GHINoData, GHIConnClosed, GHIError } GHIRecvResult;

typedef enum { GHITrySendError, GHITrySendSent, GHITrySendBuffered } GHITrySendResult;

extern int ghiThrottleInterval;
extern int ghiThrottleBytes;
extern unsigned short ghiProxyPort;
extern char *ghiProxyAddress;
void ghiCreateLock(void);
void ghiFreeLock(void);
void ghiLock(void);
void ghiUnlock(void);
GHTTPBool ghiDecryptReceivedData(GHIConnection *connection);
GHIRecvResult ghiDoReceive(GHIConnection *connection, char buffer[], int *size);
int ghiDoSend(GHIConnection *connection, const char *data, int size);
GHITrySendResult ghiTrySendThenBuffer(GHIConnection *connection, const char *data, int size);

// ghttpConnection: the list of the connections that are in use or free

int ghiFindFreeSlot(void);
GHIConnection *ghiNewConnection(void);
GHTTPBool ghiCloseSocket(GHIConnection *connection);
GHTTPBool ghiFreeConnectionData(GHIConnection *connection);
GHTTPBool ghiFreeConnection(GHIConnection *connection);
GHIConnection *ghiRequestToConnection(GHTTPRequest request);
void ghiEnumConnections(GHTTPBool (*func)(GHIConnection *));
void ghiRedirectConnection(GHIConnection *connection);
void ghiCleanupConnections(void);

char *ghiCopyHost(char *url);

// ghttpMain: the calls of the game (and DWC) to the client are in dwc/ghttp.h
void ghiHandleStatus(GHIConnection *connection);
GHTTPBool ghiProcessConnection(GHIConnection *connection);
GHTTPBool ghttpSetRootCAList(const char *url, GHICertList *rootCA);
GHTTPBool ghttpRemoveRootCAList(const char *url);

// ghttpPost: a post is a list of items; the one of a connection is made by the game and the connection keeps its state
typedef enum { GHIPostingError, GHIPostingDone, GHIPostingPosting, GHIPostingWaitForContinue } GHIPostingResult;

GHTTPPost ghiNewPost(void);
void ghiPostSetAutoFree(GHTTPPost post, GHTTPBool autoFree);
void ghiFreePost(GHTTPPost post);
void ghiFreePostOfRequest(GHTTPRequest request, GHTTPPost post);
GHTTPBool ghiPostAddXml(GHTTPPost post, GSXmlStreamWriter writer);
const char *ghiPostGetContentType(GHIConnection *connection);
int ghiPostGetLength(GHIConnection *connection);
GHTTPBool ghiPostInitState(GHIConnection *connection);
GHIPostingResult ghiPostDoPosting(GHIConnection *connection);
void ghiPostCleanupState(GHIConnection *connection);

// ghttpProcess: what each state of a connection does
GHTTPBool ghiParseURL(GHIConnection *connection);
GHTTPBool ghiProcessIncomingFileData(GHIConnection *connection, const char *data, int size);
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
GHTTPBool ghiEncyptorSetRootCAList(const char *url, GHICertList *rootCA);
GHTTPBool ghiEncryptorRemoveRootCAList(const char *url);
GHICertList *ghiEncryptorGetRootCAList(const char *url);
GHTTPBool ghiEncryptorSetTwlRootCA(const char *url, GHICertList *rootCA);
GHTTPBool ghiEncryptorCleanupTwlRootCA(const char *url);
GHIEncryptionResult ghiEncryptorSslInitFunc(GHIConnection *connection, GHIEncryptor *encryptor);
GHIEncryptionResult ghiEncryptorSslStartFunc(GHIConnection *connection, GHIEncryptor *encryptor);
GHIEncryptionResult ghiEncryptorSslCleanupFunc(GHIConnection *connection, GHIEncryptor *encryptor);
GHIEncryptionResult ghiEncryptorSslEncryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in,
                                               int inSize, char *out, int *outSize);
GHIEncryptionResult ghiEncryptorSslDecryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in,
                                               int *inSize, char *out, int *outSize);
GHIEncryptionResult ghiEncryptorSend(GHIConnection *connection, GHIEncryptor *encryptor, const char *data, int size,
                                     int *sent);
GHIEncryptionResult ghiEncryptorRecv(GHIConnection *connection, GHIEncryptor *encryptor, char *buffer, int *size);

#endif // POKEBW2_DWC_GHTTP_INTERNAL_H
