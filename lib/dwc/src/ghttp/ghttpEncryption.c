#include "ghttp_internal.h"

// ghttpEncryption: the secure connections of the requests, through the SSL of the Wi-Fi library's sockets (TWL's), and
// the root certificates that they trust. The debug texts say the name of this file, which is the name of this one

#define GHI_FILE "ghttpEncryption.c"

// How a certificate that the server sent is checked (the SSL library tells of what it found, and the callback says
// which of the faults to ignore by clearing their bits)
typedef struct GHISslCertInfo {
    u8 unk_000[0x5b0];
    char issuer[0x100];
    char subject[0x100];
    char commonName[0x100];
} GHISslCertInfo;

typedef int (*GHISslAuthFunc)(u32 result, GHISslCertInfo *info, int length);

// What the SSL library's socket takes to make a connection secure
typedef struct GHISslConnection {
    u8 unk_000[0x800];
    int unk_800;
    u8 unk_804[0xc];
    GHISslAuthFunc authCallback;
    void **certs;
    int certCount;
    u8 unk_81C[0x14];
} GHISslConnection;

// The root certificates of a host that a request may be secured with, and how many have asked for them
typedef struct GHIRootCAEntry {
    char *url;
    GHICertList *certList;
    int unk_08;
    int refCount;
} GHIRootCAEntry;

static u32 sSslEntropy[8];
static void *sRootCAList;

// What the connection has: 0 for no encryption, or the engine. 5 is taken as 4, the one that exists
BOOL ghttpSetRequestEncryptionEngine(int request, int engine) {
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection == NULL) {
        return FALSE;
    }
    if (engine == 5) {
        engine = 4;
    }
    if (connection->encryptor.engine == engine) {
        return TRUE;
    }
    if (connection->encryptor.state != NULL && connection->encryptor.engine != engine) {
        return FALSE;
    }
    if (engine == 0 && strncmp(connection->url, "https://", 8) == 0) {
        return FALSE;
    }

    connection->encryptor.engine = engine;
    if (engine == 0) {
        connection->encryptor.state = NULL;
        return TRUE;
    }
    connection->encryptor.state = NULL;
    connection->encryptor.init = ghiEncryptorSslInitFunc;
    connection->encryptor.start = ghiEncryptorSslStartFunc;
    connection->encryptor.cleanup = ghiEncryptorSslCleanupFunc;
    connection->encryptor.encrypt = ghiEncryptorSslEncryptFunc;
    connection->encryptor.decrypt = ghiEncryptorSslDecryptFunc;
    connection->encryptor.open = FALSE;
    connection->encryptor.unk_0C = 0;
    connection->encryptor.active = FALSE;
    connection->encryptor.mode = 1;
    connection->encryptor.established = FALSE;
    connection->encryptor.unk_1C = 0;
    connection->encryptor.secure = 1;
    return TRUE;
}

// Sets the root certificates that a host's secure connections are checked against
BOOL ghiEncyptorSetRootCAList(const char *url, GHICertList *rootCA) {
    if (rootCA == NULL || url == NULL || strcmp(url, "") == 0) {
        func_ov011_0216cca8(8, 4, 1,
                            "%s(@%s:%d):SSL - Null pointer or empty url string; url=0x%X, theRootCAList=0x%X\n",
                            GHI_FILE, __FUNCTION__, 116, url, rootCA);
        return FALSE;
    }
    return ghiEncryptorSetTwlRootCA(url, rootCA);
}

BOOL ghiEncryptorRemoveRootCAList(const char *url) {
    return ghiEncryptorCleanupTwlRootCA(url);
}

// Orders the entries of the list of roots by their host
static int ghiCompareRootCAEntries(const void *a, const void *b) {
    return strcmp(((const GHIRootCAEntry *)a)->url, ((const GHIRootCAEntry *)b)->url);
}

GHICertList *ghiEncryptorGetRootCAList(const char *url) {
    GHIRootCAEntry key;
    int index;

    if (url == NULL || sRootCAList == NULL || strcmp(url, "") == 0) {
        func_ov011_0216cca8(8, 4, 1,
                            "%s(@%s:%d):SSL - Null pointer or empty URL string; url=0x%X, ghttpCertList=0x%X\n",
                            GHI_FILE, __FUNCTION__, 786, url, sRootCAList);
        return NULL;
    }
    key.url = ghiCopyHost((char *)url);
    index = func_ov011_021716ac(sRootCAList, &key, ghiCompareRootCAEntries, 0, TRUE);
    func_ov011_0216ddfc(key.url);
    if (index == -1) {
        func_ov011_0216cca8(8, 4, 1, "%s(@%s:%d):SSL - Could not find Url in the Cert List. url=%s\n", GHI_FILE,
                            __FUNCTION__, 803, url);
        return NULL;
    }
    return ((GHIRootCAEntry *)func_ov011_0217158c(sRootCAList, index))->certList;
}

static void ghiFreeRootCAEntry(void *element) {
    GHIRootCAEntry *entry = element;

    entry->certList = NULL;
    func_ov011_0216ddfc(entry->url);
    entry->url = NULL;
}

BOOL ghiEncryptorSetTwlRootCA(const char *url, GHICertList *rootCA) {
    GHIRootCAEntry entry;
    int index;

    if (url == NULL || rootCA == NULL || strcmp(url, "") == 0) {
        func_ov011_0216cca8(8, 4, 1,
                            "%s(@%s:%d):SSL - Null pointer or empty url string; url=0x%X, theRootCAList=0x%X\n",
                            GHI_FILE, __FUNCTION__, 837, url, rootCA);
        return FALSE;
    }
    if (sRootCAList == NULL) {
        sRootCAList = func_ov011_02171514(sizeof(GHIRootCAEntry), 1, ghiFreeRootCAEntry);
        if (sRootCAList == NULL) {
            func_ov011_0216cca8(8, 2, 1, "%s(@%s:%d):SSL - Out of memory. Cannot allocate memory for ghttpCertList\n",
                                GHI_FILE, __FUNCTION__, 852);
            return FALSE;
        }
    }
    entry.url = ghiCopyHost((char *)url);
    entry.certList = rootCA;
    entry.refCount = 1;
    index = func_ov011_021716ac(sRootCAList, &entry, ghiCompareRootCAEntries, 0, TRUE);
    if (index == -1) {
        func_ov011_02171600(sRootCAList, &entry, ghiCompareRootCAEntries);
    } else {
        func_ov011_0216ddfc(entry.url);
        ((GHIRootCAEntry *)func_ov011_0217158c(sRootCAList, index))->refCount++;
    }
    return TRUE;
}

BOOL ghiEncryptorCleanupTwlRootCA(const char *url) {
    BOOL result = TRUE;
    GHIRootCAEntry key;
    int index;

    if (url == NULL || sRootCAList == NULL || strcmp(url, "") == 0) {
        func_ov011_0216cca8(8, 4, 1,
                            "%s(@%s:%d):SSL - Null pointer or empty url string; url=0x%X, ghttpCertList=0x%X\n",
                            GHI_FILE, __FUNCTION__, 891, url, sRootCAList);
        return FALSE;
    }
    key.url = ghiCopyHost((char *)url);
    index = func_ov011_021716ac(sRootCAList, &key, ghiCompareRootCAEntries, 0, TRUE);
    if (index == -1) {
        func_ov011_0216cca8(8, 4, result, "%s(@%s:%d):SSL - Could not find Url in the Cert List. url=%s\n", GHI_FILE,
                            __FUNCTION__, 906, url);
        result = FALSE;
    } else {
        GHIRootCAEntry *entry = func_ov011_0217158c(sRootCAList, index);

        entry->refCount--;
        if (entry->refCount <= 0) {
            func_ov011_02171668(sRootCAList, index);
            if (func_ov011_02171588(sRootCAList) == 0) {
                func_ov011_02171558(sRootCAList);
                sRootCAList = NULL;
            }
        }
    }
    func_ov011_0216ddfc(key.url);
    return result;
}

// Tells what the SSL library found in the server's certificate and which of the faults it may ignore. Returns the
// faults that remain
static int SslAuthCallback(u32 result, GHISslCertInfo *info, int length) {
    func_ov011_0216cca8(8, 4, 8, "%s(@%s:%d):SSL - RECEIVED CERTFICATE\n", GHI_FILE, __FUNCTION__, 938);
    func_ov011_0216cca8(8, 4, 8, "l:%d\n\ts:%s\n\tCN=%s\n\ti:%s\n", length, info->subject, info->commonName,
                        info->issuer);
    if (result & 0x8000) {
        func_ov011_0216cca8(8, 4, 2, "%s(@%s:%d): Certificate is out-of-date - Ignoring\r\n", GHI_FILE, __FUNCTION__,
                            946);
        result &= ~0x8000;
    }
    if (result & 0x4000) {
        func_ov011_0216cca8(8, 4, 2, "%s(@%s:%d): SSL: Server name does not match - Ignoring\r\n", GHI_FILE,
                            __FUNCTION__, 954);
        result &= ~0x4000;
    }
    switch ((u8)result) {
    case 1:
        func_ov011_0216cca8(8, 4, 2, "%s(@%s:%d): SSL: No root CA installed.(DO NOT IGNORE)\r\n", GHI_FILE,
                            __FUNCTION__, 963);
        break;
    case 2:
        func_ov011_0216cca8(8, 4, 2, "%s(@%s:%d): SSL: Bad signature.(DO NOT IGNORE)\n", GHI_FILE, __FUNCTION__, 969);
        break;
    case 3:
        func_ov011_0216cca8(8, 4, 2, "%s(@%s:%d):SSL: Unknown signature algorithm\n", GHI_FILE, __FUNCTION__, 975);
        break;
    case 4:
        func_ov011_0216cca8(8, 4, 2, "%s(@%s:%d):SSL: Unknown public key alrorithm\n", GHI_FILE, __FUNCTION__, 981);
        break;
    }
    return result;
}

// Makes the SSL connection of a host, with the certificates that it is to trust
static GHISslConnection *ghiTwlSSLInit(const char *url) {
    GHISslConnection *ssl = NULL;
    GHICertList *rootCA = ghiEncryptorGetRootCAList(url);

    if (rootCA != NULL) {
        if (rootCA->certs != NULL && rootCA->count > 0) {
            OS_GetLowEntropyData(sSslEntropy);
            CPS_SslAddRandomSeed(sSslEntropy, sizeof(sSslEntropy));
            ssl = func_ov011_0216dddc(sizeof(GHISslConnection));
            if (ssl != NULL) {
                memset(ssl, 0, sizeof(GHISslConnection));
                ssl->unk_800 = 0;
                ssl->certs = rootCA->certs;
                ssl->certCount = rootCA->count;
                ssl->authCallback = SslAuthCallback;
            } else {
                func_ov011_0216cca8(8, 2, 2, "%s(@%s:%d): TWL Failed to allocate memory for ssl connection\r\n",
                                    GHI_FILE, __FUNCTION__, 1020);
            }
        } else {
            func_ov011_0216cca8(8, 2, 2, "%s(@%s:%d): TWL Uninitialized certificate list\r\n", GHI_FILE, __FUNCTION__,
                                1027);
        }
    } else {
        func_ov011_0216cca8(8, 2, 2, "%s(@%s:%d): TWL Uninitialized certificate list\r\n", GHI_FILE, __FUNCTION__,
                            1034);
    }
    return ssl;
}

int ghiEncryptorSslInitFunc(GHIConnection *connection, GHIEncryptor *encryptor) {
    GHIEncryptorState *state;

    func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d)\r\n", GHI_FILE, __FUNCTION__, 1047);
    encryptor->state = func_ov011_0216dddc(sizeof(GHIEncryptorState));
    if (encryptor->state == NULL) {
        func_ov011_0216cca8(8, 2, 2, "%s(@%s:%d):Failed to allocate SSL interface (out of memory: %d bytes)\r\n",
                            GHI_FILE, __FUNCTION__, 1061, 8);
        return 3;
    }
    memset(encryptor->state, 0, sizeof(GHIEncryptorState));
    state = encryptor->state;
    state->ssl = ghiTwlSSLInit(connection->url);
    state->started = FALSE;
    if (state->ssl == NULL) {
        func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d):GameSpy SSL (TWL) certificate is uninitialized\r\n", GHI_FILE,
                            __FUNCTION__, 1073);
        return 3;
    }
    encryptor->unk_0C = 0;
    encryptor->active = FALSE;
    encryptor->mode = 0;
    encryptor->open = TRUE;
    encryptor->established = TRUE;
    encryptor->unk_1C = 1;
    func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d):GameSpy SSL (TWL engine) initialized\r\n", GHI_FILE, __FUNCTION__, 1085);
    return 1;
}

// Turns the socket of the connection into an SSL one, once. Returns 1 for done and 3 for an error
int ghiEncryptorSslStartFunc(GHIConnection *connection, GHIEncryptor *encryptor) {
    GHIEncryptorState *state = encryptor->state;
    int error;

    func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d)\r\n", GHI_FILE, __FUNCTION__, 1100);
    if (!state->started) {
        error = SOCL_EnableSsl(connection->socket, state->ssl);
        if (error < 0) {
            switch (error) {
            case -28:
                func_ov011_0216cca8(8, 4, 15,
                                    "%s(@%s:%d):GameSpy SSL (TWL) Invalid processing. Socket config is invalid\r\n",
                                    GHI_FILE, __FUNCTION__, 1113);
                break;
            case -33:
                // BUG: the text ends with the file name, as if its arguments were one short, so the format is given
                // the function name and the line for its first two conversions
#ifdef BUGFIX
                func_ov011_0216cca8(8, 4, 15,
                                    "%s(@%s:%d):GameSpy SSL (TWL) Cannot create any more socket descriptors.\r\n",
                                    GHI_FILE, __FUNCTION__, 1118);
#else
                func_ov011_0216cca8(
                    8, 4, 15, "%s(@%s:%d):GameSpy SSL (TWL) Cannot create any more socket descriptors.\r\n" GHI_FILE,
                    __FUNCTION__, 1118);
#endif
                break;
            case -39:
                func_ov011_0216cca8(8, 4, 15,
                                    "%s(@%s:%d):GameSpy SSL (TWL) Socket library is not initialized.\r\n", GHI_FILE,
                                    __FUNCTION__, 1124);
                break;
            case -42:
                func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d):GameSpy SSL (TWL)Insufficient resources.\r\n", GHI_FILE,
                                    __FUNCTION__, 1129);
                break;
            default:
                func_ov011_0216cca8(8, 4, 15,
                                    "%s(@%s:%d):GameSpy SSL (TWL) SOC_EnableSsl failed (Unknown Error = %d)\r\n",
                                    GHI_FILE, __FUNCTION__, 1135, error);
                break;
            }
            connection->completed = TRUE;
            connection->result = 6;
            connection->socketError = error;
            return 3;
        }
        state->started = TRUE;
    }
    encryptor->unk_0C = 1;
    encryptor->active = TRUE;
    func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d)return success\r\n", GHI_FILE, __FUNCTION__, 1154);
    return 1;
}

// What the common code calls to send and receive through the SSL, and the functions to encrypt and decrypt: they do
// nothing here, since the SSL library does all of it in the socket
int ghiEncryptorSend(GHIConnection *connection, GHIEncryptor *encryptor, const char *data, int size, int *sent) {
    return 1;
}

int ghiEncryptorRecv(GHIConnection *connection, GHIEncryptor *encryptor, char *buffer, int *size) {
    return 1;
}

int ghiEncryptorSslEncryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int inSize,
                               char *out, int *outSize) {
    return 3;
}

int ghiEncryptorSslDecryptFunc(GHIConnection *connection, GHIEncryptor *encryptor, const char *in, int *inSize,
                               char *out, int *outSize) {
    return 3;
}

BOOL ghiEncryptorSslCleanupFunc(GHIConnection *connection, GHIEncryptor *encryptor) {
    GHIEncryptorState *state;

    func_ov011_0216cca8(8, 4, 15, "%s(@%s:%d):GameSpy SSL (Twl) engine cleanup called\r\n", GHI_FILE, __FUNCTION__,
                        1237);
    if (encryptor != NULL) {
        state = encryptor->state;
        if (state != NULL) {
            if (state->ssl != NULL) {
                func_ov011_0216ddfc(state->ssl);
            }
            func_ov011_0216ddfc(state);
            encryptor->state = NULL;
        }
        encryptor->open = FALSE;
        encryptor->unk_0C = 0;
        encryptor->active = FALSE;
    }
    return TRUE;
}
