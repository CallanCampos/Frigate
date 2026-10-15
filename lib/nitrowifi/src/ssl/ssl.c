#include "nitrowifi/cps.h"
#include "nitro/mi.h"
#include "ssl_internal.h"
#include "system/rtc.h"

// NitroWiFi's SSL layer (SSLv3-like: RC4 with MD5 or SHA1 MACs, RSA key exchange), in overlay 11. No swan name for the
// object or any of its functions; named for their role, after pokediamond's SSL_ssl.o lists the same ones (structure
// reference only, not copied). A connection's own records sit after the socket's TCP ones at soc->con.

// A cache entry remembers a session's id and secret by the peer that offered it, so a later connection can skip the
// full handshake. 4 entries, cleared by CPS_SslInit
typedef struct SSLSessionCacheEntry {
    u8 session_id[0x20];
    u8 secret[0x30];
    s32 time; // clock() >> 16 when the entry was last used
    CPSInAddr ip;
    u16 port;
    u8 valid;
} SSLSessionCacheEntry;

// A certificate field, or another named value a caller registered on the current thread's connection with
// CPS_SslSetNamedParams, found again by name with SSLi_FindNamedParam
typedef struct SSLNamedParam {
    const char *name;
} SSLNamedParam;

// A connection, DER-parsed certificate fields included. 0x830 bytes
typedef struct SSLContext {
    u8 secret[0x30]; // The pre-master secret, overwritten with the master secret once derived
    u8 resumed;       // TRUE once a cached session's secret was restored
    u8 unk_031;
    u16 cipher_suite;
    u8 client_random[0x20];
    u8 server_random[0x20];
    u8 session_id[0x20];
    u8 unk_094[0x28];
    u8 *write_mac_secret;
    u8 *write_key;
    u8 unk_0c4[4];
    CPSRc4Context write_rc4;
    u8 unk_1ca[2];
    u8 write_seq_num[8]; // The 8-byte big-endian sequence number SSLi_ComputeMac folds into a received record's MAC
    u8 *read_mac_secret;
    u8 *read_key;
    u8 unk_1dc[4];
    CPSRc4Context read_rc4;
    u8 unk_2e2[2];
    u8 read_seq_num[8]; // Same, for a sent record's MAC; both start at 0 and SSLi_IncrementCounter advances them
    SHA1Context handshake_sha1;          // The running hash of every handshake message so far
    SHA1Context handshake_sha1_snapshot; // A copy taken before Finished, for a later quick-resume check
    MD5Context handshake_md5;
    MD5Context handshake_md5_snapshot;
    u8 unk_454;
    u8 state;     // SSLI_STATE_*, read back by SSLi_ReceiveRecord's caller
    u8 buf_owned; // TRUE once a handshake buffer was allocated for the current message
    u8 pad_457;
    u8 unk_458[0x3bc]; // The server's certificate fields, parsed by SSLi_ParseAsn1Field: RSA modulus and exponent,
                        // the quick-resume key material and signature check, the subject's common name, validity
    SSLNamedParam *const *named_params; // The caller's registered list, set with CPS_SslSetNamedParams
    u32 named_param_count;
    void *key_param; // Private key consumed by SSLi_FinishKeyExchange; its writer is not recovered
    u8 unk_820[4];
    u8 *read_buf; // Application data the handshake has read but the caller hasn't taken yet
    u32 read_buf_len;
    u32 read_buf_pos;
} SSLContext;

// SSLContext.state
enum {
    SSLI_STATE_WANT_CLIENT_HELLO = 0,
    SSLI_STATE_SERVER_HELLO_RANDOM,
    SSLI_STATE_CERTIFICATE_REQUEST,
    SSLI_STATE_SERVER_HELLO_DONE,
    SSLI_STATE_HANDSHAKE_DONE = 4,
    SSLI_STATE_RESUMED_HELLO = 5,
    SSLI_STATE_CHANGE_CIPHER_SPEC = 6,
    SSLI_STATE_FINISHED = 7,
    SSLI_STATE_ALERT = 8,
    SSLI_STATE_ERROR = 9,
};

// 4 entries, cleared by CPS_SslInit
static SSLSessionCacheEntry sSessionCache[4];

// SSLi_GetRandomBytes's pool: a running SHA1 digest that CPS_SslAddRandomSeed stirs entropy into
static u8 sPoolSeeded;
static u8 sPool[0x14];

// The record CPS_SslWrite sends before ClientKeyExchange when the server asked for a certificate and the caller has
// none to offer: alert(fatal, no_certificate)
static const u8 sNoCertificateAlert[7] = { 0x15, 0x03, 0x00, 0x00, 0x02, 0x01, 0x29 };

// X.509 attribute and algorithm OIDs SSLi_ParseAsn1Field recognizes while walking a certificate, and one sentinel
// that never matches
static const u8 sOid0[4] = { 0xff, 0xff, 0xff, 0 };
static const u8 sOidRsaEncryption[10] = { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01, 0 };
static const u8 sOid2[5] = { 0x55, 0x08, 0x01, 0x01, 0 };
static const u8 sOidMd5WithRsa[10] = { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04, 0 };
static const u8 sOidSha1WithRsa[10] = { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0 };
static const u8 sOidCommonName[4] = { 0x55, 0x04, 0x03, 0 };

static const u8 *const sOids[6] = { sOid0, sOidRsaEncryption, sOid2, sOidMd5WithRsa, sOidSha1WithRsa, sOidCommonName };

// The cipher suites CPS_Sslconnect offers, and the maximum priority a connection's thread may run at while it works
// (CPS_SslSetMaxPriority's role), -1 until set
static struct {
    u16 cipher_suites[2]; // SSL_RSA_WITH_RC4_128_MD5, SSL_RSA_WITH_RC4_128_SHA
    u32 max_priority;
} sConfig = { { 4, 5 }, (u32)-1 };

// The 3 labels generate_key_material concatenates into the master secret, and the 2 that pick a side's Finished MAC
static const u8 sLabelA[4] = "A";
static const u8 sLabelBB[4] = "BB";
static const u8 sLabelCCC[4] = "CCC";
static const u8 sLabelServer[4] = "SRVR";
static const u8 sLabelClient[4] = "CLNT";

static void SSLi_RestoreSessionById(SSLContext *ctx);
static void SSLi_FindSessionByPeer(SSLContext *ctx, CPSInAddr ip, u16 port);
static void SSLi_SaveSession(SSLContext *ctx, CPSInAddr ip, u16 port);
static void SSLi_InvalidateSession(SSLContext *ctx);
static u32 SSLi_GetUnixTime(void);
static u32 SSLi_RaisePriority(void);
static void SSLi_RestorePriority(u32 priority);
static const SSLNamedParam *SSLi_FindNamedParam(const SSLContext *ctx, const char *name);
static s32 SSLi_DecodeLength(const u8 **cursor);
static void SSLi_BuildHostPattern(char *dest, const char *suffix, int repeat);
static s32 SSLi_ParseCertTime(const u8 *digits, BOOL generalized);
static int SSLi_ParseAsn1Field(SSLContext *ctx, const u8 **cursor, int depth, int index, int extra);
static int SSLi_CheckQuickResume(SSLContext *ctx);
static int SSLi_LabelLength(const char *host);
static BOOL SSLi_MatchHostname(const char *pattern, const char *host);
static int SSLi_ParseMessage(SSLContext *ctx);
static void SSLi_ParseServerHelloRandom2(SSLContext *ctx, u8 *msg);
static BOOL SSLi_ContainsU16(const u8 *data, int count, int width, u32 value);
static u16 SSLi_PickCipherSuite(const u8 *data, int count, int width);
static BOOL SSLi_IsSsl3Version(int major);
static void SSLi_ParseServerKeyExchange(SSLContext *ctx, u8 *msg);
static void SSLi_ParseServerHello(SSLContext *ctx, u8 *msg);
static void SSLi_DecryptPreMasterSecret(SSLContext *ctx, u8 *encrypted, void *params);
static void SSLi_HashKeyMaterialBlock(u8 *out, const u8 *label, SSLContext *ctx);
static void SSLi_GenerateMasterSecret(SSLContext *ctx);
static void SSLi_GenerateKeyBlock(SSLContext *ctx);
static void SSLi_FinishKeyExchange(SSLContext *ctx, u8 *msg);
static void SSLi_ComputeFinishedMd5(SSLContext *ctx, u8 *out, int sender);
static void SSLi_ComputeFinishedSha1(SSLContext *ctx, u8 *out, int sender);
static void SSLi_VerifyFinished(SSLContext *ctx, u8 *msg, int sender);
static void SSLi_IncrementCounter(u8 *iv);
static u32 SSLi_Rc4Crypt(SSLContext *ctx, void *buf, u32 len);
static void SSLi_CheckRecordMac(SSLContext *ctx, u8 *hdr);
static u32 SSLi_ComputeMac(SSLContext *ctx, u8 *hdr);
static int SSLi_ReadExact(u8 *dest, u32 len, CPSSoc *soc);
static void SSLi_UpdateHandshakeHash(SSLContext *ctx, const u8 *data, u32 len);
static void SSLi_ProcessHandshakeMessage(SSLContext *ctx, u8 *msg, u32 msgLen);
static int SSLi_ReceiveRecord(CPSSoc *soc);
static void SSLi_GetRandomBytes(u8 *buf, u32 len);
static void SSLi_SendChangeCipherSpecAndFinished(CPSSoc *soc);
static void SSLi_SendClientHello(CPSSoc *soc);
static void SSLi_SendClientKeyExchange(CPSSoc *soc);
static BOOL SSLi_WaitForChangeCipherSpecAndFinished(CPSSoc *soc);
static BOOL SSLi_DoHandshake(CPSSoc *soc);
static void SSLi_FillReadBuffer(CPSSoc *soc);

// Looks the ctx's session id up in the cache; restores its secret and marks the connection resumed on a hit
static void SSLi_RestoreSessionById(SSLContext *ctx) {
    u32 enabled = CPU_IRQDisable();
    int i;

    ctx->resumed = FALSE;
    for (i = 0; i < 4; i++) {
        SSLSessionCacheEntry *entry = &sSessionCache[i];
        if (entry->valid && entry->ip == 0 && entry->port == 0 && memcmp(entry, ctx->session_id, 0x20) == 0) {
            sys_memcpy(entry->secret, ctx->secret, 0x30);
            ctx->resumed = TRUE;
            break;
        }
    }
    CPU_SetIRQMask(enabled);
}

// Looks a cached session up by the peer it was made with, offering it (ctx->session_id, ctx->resumed) for the next
// ClientHello
static void SSLi_FindSessionByPeer(SSLContext *ctx, CPSInAddr ip, u16 port) {
    u32 enabled = CPU_IRQDisable();
    int i;

    ctx->resumed = FALSE;
    for (i = 0; i < 4; i++) {
        SSLSessionCacheEntry *entry = &sSessionCache[i];
        if (entry->valid && entry->ip == ip && entry->port == port) {
            sys_memcpy(entry->session_id, ctx->session_id, 0x20);
            sys_memcpy(entry->secret, ctx->secret, 0x30);
            entry->time = (u32)(clock() >> 16);
            ctx->resumed = TRUE;
            break;
        }
    }
    CPU_SetIRQMask(enabled);
}

// Stores the connection's current session (id and secret) in the cache, reusing a peer hit, an invalid entry, or the
// oldest entry
static void SSLi_SaveSession(SSLContext *ctx, CPSInAddr ip, u16 port) {
    u32 enabled = CPU_IRQDisable();
    u32 now = (u32)(clock() >> 16);
    SSLSessionCacheEntry *target = &sSessionCache[0];
    u32 bestAge = 0;
    int i;

    for (i = 0; i < 4; i++) {
        SSLSessionCacheEntry *entry = &sSessionCache[i];
        if (entry->valid && ip != 0 && ip == entry->ip && port != 0 && port == entry->port) {
            target = entry;
            break;
        }
        if (bestAge != (u32)-1) {
            if (!entry->valid) {
                bestAge = (u32)-1;
                target = entry;
            } else {
                u32 age = now - entry->time;
                if (age > bestAge) {
                    bestAge = age;
                    target = entry;
                }
            }
        }
    }

    sys_memcpy(ctx->session_id, target->session_id, 0x20);
    sys_memcpy(ctx->secret, target->secret, 0x30);
    target->time = now;
    target->valid = TRUE;
    target->ip = ip;
    target->port = port;
    CPU_SetIRQMask(enabled);
}

// Marks the cache entry that offered this session id as no longer valid, once the handshake using it has failed
static void SSLi_InvalidateSession(SSLContext *ctx) {
    u32 enabled = CPU_IRQDisable();
    int i;

    for (i = 0; i < 4; i++) {
        SSLSessionCacheEntry *entry = &sSessionCache[i];
        if (entry->valid && memcmp(entry, ctx->session_id, 0x20) == 0) {
            entry->valid = FALSE;
            break;
        }
    }
    CPU_SetIRQMask(enabled);
}

// A Unix-ish timestamp for the first 4 bytes of a client random
static u32 SSLi_GetUnixTime(void) {
    RTCDate date;
    RTCTime time;

    func_0207cc10(&date);
    func_0207cc80(&time);
    return (u32)func_0207d12c(&date, &time) + 0x386d4380;
}

// CPS_SslGetMaxPriority's role: the priority CPS_SslSetMaxPriority last gave, or -1
u32 CPS_SslGetMaxPriority(void) {
    return sConfig.max_priority;
}

// CPS_SslSetMaxPriority's role: the priority a connection's thread runs at while SSLi_RaisePriority holds it up for
// a crypto operation
void CPS_SslSetMaxPriority(u32 priority) {
    sConfig.max_priority = priority;
}

// Raises the current thread's priority to the configured maximum (CPS_SslSetMaxPriority's role) for a crypto
// operation, returning the priority it had so SSLi_RestorePriority can put it back; -1 once the slot (0-31) runs out
static u32 SSLi_RaisePriority(void) {
    if (sConfig.max_priority >= 0x20) {
        return (u32)-1;
    }
    {
        OSThread *thread = OS_GetCurrentThread();
        u32 old = func_0207aa00(thread);
        func_0207a998(thread, sConfig.max_priority);
        return old;
    }
}

static void SSLi_RestorePriority(u32 priority) {
    if (priority < 0x20) {
        func_0207a998(OS_GetCurrentThread(), priority);
    }
}

// CPS_SslSetNamedParams's role: registers the caller's named params (certificates to pin) on the current thread's
// connection, if it has one
void CPS_SslSetNamedParams(void *const *params, u32 count) {
    OSThread *thread = OS_GetCurrentThread();
    CPSSoc *soc = (CPSSoc *)thread->specific[0];

    if (soc != NULL && soc->con != NULL) {
        SSLContext *ctx = (SSLContext *)soc->con;
        ctx->named_params = (SSLNamedParam *const *)params;
        ctx->named_param_count = count;
    }
}

// Finds the registered param whose name matches, or NULL
static const SSLNamedParam *SSLi_FindNamedParam(const SSLContext *ctx, const char *name) {
    int count = ctx->named_param_count;
    int i;

    for (i = 0; i < count; i++) {
        const SSLNamedParam *param = ctx->named_params[i];
        if (strcmp(param->name, name) == 0) {
            return param;
        }
    }
    return NULL;
}

// Reads a BER-style length: a byte with the high bit clear is the value itself; one with it set gives the count of
// following big-endian bytes to read it from. -1 if that count would need the high bits of the result
static s32 SSLi_DecodeLength(const u8 **cursor) {
    const u8 *p = *cursor;
    u32 value = *p++;

    if (value & 0x80) {
        u32 count = value & 0x7f;
        value = 0;
        while (count-- != 0) {
            if (value & 0xff000000) {
                return -1;
            }
            value = (value << 8) + *p++;
        }
    }
    *cursor = p;
    return value;
}

// Appends ", *<suffix>" to a hostname pattern (truncating the whole thing to 0xff bytes), or just <suffix> once if
// repeat is 0
static void SSLi_BuildHostPattern(char *dest, const char *suffix, int repeat) {
    char *p = dest;

    if (*p != '\0') {
        while (*++p != '\0') {
        }
        if (p - dest < 0xff) {
            *p++ = ',';
            *p++ = ' ';
        }
    }
    while (TRUE) {
        if (repeat == 0) {
            break;
        }
        repeat--;
        if (p - dest >= 0xff) {
            break;
        }
        *p++ = *suffix++;
    }
    *p = '\0';
}

// Parses a certificate's UTCTime (YYMMDDHHMMSSZ) or GeneralizedTime (CCYYMMDDHHMMSSZ) into the same kind of rough
// day-and-time value SSLi_GetUnixTime's sibling (func_0207d12c) would give, for a validity check
static s32 SSLi_ParseCertTime(const u8 *digits, BOOL generalized) {
    s32 value;

    if (!generalized) {
        int year = digits[0] * 10 + digits[1];
        value = (year >= 50 ? 1900 : 2000) + year;
        digits += 2;
    } else {
        value = (digits[0] * 10 + digits[1]) * 100 + digits[2] * 10 + digits[3];
        digits += 4;
    }
    value = value * 100 + digits[0] * 10 + digits[1];
    value = value * 100 + digits[2] * 10 + digits[3];
    value = value * 100 + digits[4] * 10 + digits[5];
    value = value * 100 + digits[6] * 10 + digits[7];
    return value;
}

// A byte offset into the certificate fields past ctx->unk_458, whose exact layout isn't recovered; kept as an
// explicit offset rather than named members until it is (see docs/nonmatching-functions.md for this function)
#define SSLI_CERT_FIELD(ctx, off) (*(u32 *)((u8 *)(ctx) + (off)))
#define SSLI_CERT_BYTE(ctx, off) (*((u8 *)(ctx) + (off)))

// Walks one DER tag starting at *cursor (SEQUENCE and SET recurse into their members), picking out an RSA public
// key's modulus and exponent (INTEGER), its signature (BIT STRING), the algorithm it was signed with (OBJECT
// IDENTIFIER, matched against sOids), the subject's common name (a string following the commonName OID) and the
// validity dates (UTCTime, GeneralizedTime). index counts which INTEGER or OID this is within the current SEQUENCE;
// extra carries the class/constructed byte along from the caller for the context-specific [0] version tag
static int SSLi_ParseAsn1Field(SSLContext *ctx, const u8 **cursor, int depth, int index, int extra) {
    const u8 *p = *cursor;
    u8 tag = *p;
    const u8 *valueStart = p + 1;
    s32 len = SSLi_DecodeLength(&valueStart);

    if (len < 0 || len > 0x7d0) {
        return TRUE;
    }

    switch (tag & 0x1f) {
    case 2: // INTEGER: the first one in a SubjectPublicKeyInfo's RSAPublicKey SEQUENCE is the modulus, the second
            // the exponent
        if (index == 0 && len <= 0x100) {
            SSLI_CERT_FIELD(ctx, 0x458) = (u32)valueStart;
            SSLI_CERT_FIELD(ctx, 0x45c) = len;
        } else if (index == 2 && len <= 0x100) {
            SSLI_CERT_FIELD(ctx, 0x484) = (u32)valueStart;
            SSLI_CERT_FIELD(ctx, 0x48c) = len;
        }
        break;
    case 3: // BIT STRING (the signature, with a leading unused-bits byte)
        SSLI_CERT_FIELD(ctx, 0x5a4) = (u32)(valueStart + 1);
        SSLI_CERT_FIELD(ctx, 0x5a8) = len - 1;
        break;
    case 6: { // OBJECT IDENTIFIER
        int i;
        for (i = 0; i < 6; i++) {
            if (memcmp(valueStart, sOids[i], strlen((const char *)sOids[i])) == 0) {
                switch (i) {
                case 1:
                    SSLI_CERT_BYTE(ctx, 0x5ac) = i;
                    break;
                case 3:
                case 4:
                    SSLI_CERT_BYTE(ctx, 0x454) = (u8)i;
                    break;
                case 5:
                    SSLI_CERT_BYTE(ctx, 0x5af) = 1;
                    break;
                default:
                    break;
                }
                break;
            }
        }
        break;
    }
    case 12: // UTF8String
    case 19: // PrintableString
    case 20: // T61String
    case 22: // IA5String
        if (SSLI_CERT_BYTE(ctx, 0x5ac) != 0) {
            if (SSLI_CERT_BYTE(ctx, 0x5ac + 2) == 5 && len <= 0x4f) {
                u8 *dest = (u8 *)ctx + 0x594;
                sys_memcpy(valueStart, dest, len);
                dest[len] = '\0';
            } else {
                SSLi_BuildHostPattern((char *)ctx + 0x6b0, (const char *)valueStart, len);
            }
        }
        SSLI_CERT_BYTE(ctx, 0x5ae) = 0;
        break;
    case 16: // SEQUENCE
    case 17: // SET
    {
        const u8 *end = valueStart + len;
        int i = 0;
        while (valueStart < end) {
            if (SSLi_ParseAsn1Field(ctx, &valueStart, depth + 1, i, 0)) {
                return TRUE;
            }
            i++;
        }
        break;
    }
    case 23: // UTCTime
    case 24: // GeneralizedTime
        if (extra != 2) {
            SSLi_ParseCertTime(valueStart, (tag & 0x1f) == 24);
        }
        break;
    default:
        if ((tag & 0xe0) == 0xa0 && len > 0) {
            const u8 *end = valueStart + len;
            while (valueStart < end) {
                if (SSLi_ParseAsn1Field(ctx, &valueStart, depth + 1, 0, 0)) {
                    return TRUE;
                }
            }
        }
        break;
    }

    *cursor = valueStart + len;
    return FALSE;
}

#undef SSLI_CERT_FIELD
#undef SSLI_CERT_BYTE

// Registered public-key record. The length/pointer pairs and name prefix come from the original loads.
// This private layout is distinct from the RSA-CRT key consumed by SSLi_DecryptPreMasterSecret.
typedef struct SSLRsaPublicKey {
    const char *name;
    s32 modulus_len;
    const u8 *modulus;
    s32 exponent_len;
    const u8 *exponent;
} SSLRsaPublicKey;

// Provisional certificate-key view used only by the still-incomplete SSLi_SendClientKeyExchange.
// Keep its existing field interpretation separate from the recovered registered public-key layout.
typedef struct SSLCertificateKey {
    const u8 *exponent;
    u32 exponent_len;
    const u8 *modulus;
    u32 modulus_len;
} SSLCertificateKey;

// Checks the signature SSLi_ParseAsn1Field found (ctx's certificate's own, for the quick-resume ticket) against the
// snapshot handshake digest at ctx+0x468, PKCS#1 v1.5 style: decrypted = signature^e mod n, then 0x00 0x01 0xff...
// 0x00 <digest>
static int SSLi_VerifyQuickResumeSignature(SSLContext *ctx, const SSLRsaPublicKey *key) {
#define SIG_PTR (*(const u8 **)((u8 *)ctx + 0x5a4))
#define SIG_LEN (*(u32 *)((u8 *)ctx + 0x5a8))
#define DIGEST_LEN (*(u32 *)((u8 *)ctx + 0x47c))
    int n;
    u16 *digits;
    u16 *sigDigits, *expDigits, *modDigits;
    u8 *decrypted;
    u32 priority;
    int status = 0;

    if (SIG_PTR == NULL || SIG_LEN == 0 || key->exponent == NULL || key->exponent_len == 0 || key->modulus == NULL ||
        key->modulus_len == 0) {
        return 2;
    }

    n = (key->modulus_len * 2) / 2;
    digits = (u16 *)CPSiAlloc(n * 8);
    if (digits == NULL) {
        return 2;
    }

    sigDigits = digits + n;
    expDigits = sigDigits + n;
    modDigits = expDigits + n;
    decrypted = (u8 *)sigDigits;

    BigInt_BytesToDigits(sigDigits, SIG_PTR, SIG_LEN, n);
    BigInt_BytesToDigits(expDigits, key->exponent, key->exponent_len, n);
    BigInt_BytesToDigits(modDigits, key->modulus, key->modulus_len, n);
    priority = SSLi_RaisePriority();
    BigInt_ModExp(digits, sigDigits, expDigits, n, modDigits);
    SSLi_RestorePriority(priority);
    BigInt_DigitsToBytes(decrypted, digits, key->modulus_len, n);

    if (decrypted[0] == 0 && decrypted[1] == 1) {
        int pos = 2;
        while (pos < key->modulus_len && decrypted[pos] == 0xff) {
            pos++;
        }
        if (pos + 1 >= key->modulus_len || decrypted[pos] != 0 || decrypted[pos + 1] != 0x30 ||
            memcmp((u8 *)ctx + 0x468, decrypted + key->modulus_len - DIGEST_LEN, DIGEST_LEN) != 0) {
            status = 2;
        }
    } else {
        status = 2;
    }
    CPSiFree(digits);
    return status;
#undef SIG_PTR
#undef SIG_LEN
#undef DIGEST_LEN
}

// Decides whether the certificate SSLi_ParseAsn1Field just finished with lets the connection skip the handshake:
// a registered param whose name matches the certificate's common name, with a signature over the handshake so far
// that checks out against it. The exact fields this reads past ctx+0x454 aren't recovered (see
// docs/nonmatching-functions.md)
static int SSLi_CheckQuickResume(SSLContext *ctx) {
#define CF32(off) (*(u32 *)((u8 *)ctx + (off)))
#define CF8(off) (*((u8 *)ctx + (off)))
    int flags = CF8(0x5af) ? 0 : (2 << 14);
    u32 digestLen = CF32(0x45c);
    const SSLNamedParam *param;

    if (digestLen == (u32)-1) {
        return 4 | flags;
    }
    {
        u32 type = CF32(0x458);
        const u8 *start = (const u8 *)CF32(0x460);
        const u8 *end = (const u8 *)CF32(0x464);

        if (type == 3) {
            MD5Init(&ctx->handshake_md5_snapshot);
            MD5Update(&ctx->handshake_md5_snapshot, start, end - start);
            MD5Final(&ctx->handshake_md5_snapshot, (u8 *)ctx + 0x468);
            CF32(0x47c) = 0x10;
        } else if (type == 4) {
            SHA1Init(&ctx->handshake_sha1_snapshot);
            SHA1Update(&ctx->handshake_sha1_snapshot, start, end - start);
            SHA1Final(&ctx->handshake_sha1_snapshot, (u8 *)ctx + 0x468);
            CF32(0x47c) = 0x14;
        } else {
            return 3 | flags;
        }
    }

    param = SSLi_FindNamedParam(ctx, (const char *)ctx + 0x5b0);
    if (param == NULL) {
        return 1 | flags;
    }
    return SSLi_VerifyQuickResumeSignature(ctx, (const SSLRsaPublicKey *)param) | flags;
#undef CF32
#undef CF8
}

// How many bytes until the next '.' or the end of the string
static int SSLi_LabelLength(const char *host) {
    int i = 0;
    while (host[i] != '.' && host[i] != '\0') {
        i++;
    }
    return i;
}

// Matches a hostname against a pattern that may have a leading "*" wildcard label (the subject common name a
// certificate offers, against the name the caller connected to)
static BOOL SSLi_MatchHostname(const char *pattern, const char *host) {
    while (TRUE) {
        char p = *pattern++;
        char h = *host++;

        if (h != p) {
            int hostLen, patLen;

            if (p != '*') {
                return FALSE;
            }
            hostLen = SSLi_LabelLength(host - 1);
            patLen = SSLi_LabelLength(pattern);
            if (patLen > hostLen) {
                return FALSE;
            }
            host += hostLen - patLen;
            continue;
        }
        if (p == '\0') {
            return TRUE;
        }
    }
}

// A field SSLi_ParseMessage walks a certificate's bytes with, giving SSLi_ParseAsn1Field a default class/constructed
// value and a callback for after it runs
typedef struct SSLAsn1Step {
    int defaultExtra;
    void (*after)(SSLContext *ctx, const u8 *valueStart);
} SSLAsn1Step;

// Parses the server's certificate (an X.509 Certificate, DER-encoded) as it arrives, a field at a time, retrying
// until the field SSLi_ParseAsn1Field needs is fully buffered. The exact bookkeeping fields this touches past
// ctx+0x454 aren't recovered (see docs/nonmatching-functions.md)
static int SSLi_ParseMessage(SSLContext *ctx) {
    (void)ctx;
    return SSLI_STATE_ERROR;
}

// Parses a ServerHello that carries the server's random after the session id, rather than before it (a second
// shape this code's generic handshake peer can send; see SSLi_ParseServerHello for the usual one)
static void SSLi_ParseServerHelloRandom2(SSLContext *ctx, u8 *msg) {
    int sidLen;
    BOOL wasResumed = ctx->resumed;
    BOOL resumed;

    sys_memcpy(msg + 2, ctx->server_random, 0x20);
    sidLen = msg[0x22];
    msg += 0x23;

    if (wasResumed && sidLen == 0x20 && memcmp(ctx->session_id, msg, 0x20) == 0) {
        ctx->unk_031 = 1;
    } else {
        if (wasResumed) {
            SSLi_InvalidateSession(ctx);
        }
        resumed = (sidLen != 0);
        if (resumed) {
            sys_memcpy(msg, ctx->session_id, 0x20);
        }
        ctx->resumed = resumed;
        ctx->unk_031 = 0;

        ctx->cipher_suite = (u16)((msg[sidLen] << 8) | msg[sidLen + 1]);
        if (ctx->cipher_suite != 4 && ctx->cipher_suite != 5) {
            ctx->state = SSLI_STATE_ERROR;
            return;
        }
    }
    ctx->state = SSLI_STATE_CERTIFICATE_REQUEST;
}

// Whether value is among the count big-endian values of width bytes each (2 or 3) in data
static BOOL SSLi_ContainsU16(const u8 *data, int count, int width, u32 value) {
    int i;

    for (i = 0; i < count; i++, data += width) {
        u32 entry = (data[0] << 8) | data[1];
        if (width == 3) {
            entry = (entry << 8) | data[2];
        }
        if (entry == value) {
            return TRUE;
        }
    }
    return FALSE;
}

// Picks the first of this connection's offered cipher suites that the count entries of width bytes each in data
// also list, or 0
static u16 SSLi_PickCipherSuite(const u8 *data, int count, int width) {
    int i;

    for (i = 0; i < 2; i++) {
        if (SSLi_ContainsU16(data, count, width, sConfig.cipher_suites[i])) {
            return sConfig.cipher_suites[i];
        }
    }
    return 0;
}

// Whether major is SSLv3's (3); only version 3.x handshakes are understood
static BOOL SSLi_IsSsl3Version(int major) {
    return major == 3;
}

// Parses a ServerKeyExchange for an export cipher suite: a version, a 3-byte cipher suite list, two length-prefixed
// fields and the server's temporary key value, stored (zero-padded on the left, if short) where SSLi_ParseServerHello
// put the client random, since this shape of handshake has no further use for it
static void SSLi_ParseServerKeyExchange(SSLContext *ctx, u8 *msg) {
    u16 cipher;
    u32 len1, len2, offset;

    if (!SSLi_IsSsl3Version(msg[0])) {
        return;
    }

    cipher = SSLi_PickCipherSuite(msg + 8, ((msg[2] << 8) | msg[3]) / 3, 3);
    if (cipher == 0) {
        return;
    }
    ctx->cipher_suite = cipher;

    len1 = (msg[4] << 8) | msg[5];
    len2 = (msg[6] << 8) | msg[7];
    ctx->resumed = FALSE;
    offset = ((msg[2] << 8) | msg[3]) + 8 + len1;

    if (len2 >= 0x20) {
        sys_memcpy(msg + offset, ctx->client_random, 0x20);
    } else {
        sys_memset(ctx->client_random, 0, 0x20 - len2);
        sys_memcpy(msg + offset, ctx->server_random - len2, len2);
    }
    ctx->state = SSLI_STATE_HANDSHAKE_DONE;
}

// Parses a ServerHello: a version, the server's random, a session id, a cipher suite and the compression methods
// (unused: SSLv3's only one, none, is all this offers). Restores the cached session if the server echoed its id
static void SSLi_ParseServerHello(SSLContext *ctx, u8 *msg) {
    int sidLen;
    u16 cipher;

    if (!SSLi_IsSsl3Version(msg[0])) {
        return;
    }
    sys_memcpy(msg + 2, ctx->client_random, 0x20);

    sidLen = msg[0x22];
    msg += 0x23;
    if (sidLen == 0x20) {
        sys_memcpy(msg, ctx->session_id, 0x20);
        SSLi_RestoreSessionById(ctx);
    } else {
        ctx->resumed = FALSE;
    }

    cipher = SSLi_PickCipherSuite(msg + sidLen + 2, ((msg[sidLen] << 8) | msg[sidLen + 1]) / 2, 2);
    ctx->cipher_suite = cipher;
    if (cipher == 0) {
        return;
    }
    ctx->state = SSLI_STATE_SERVER_HELLO_RANDOM;
}

// RSA-CRT private key: six alternating length/pointer pairs observed in the original function.
// First/second names avoid assuming a particular p/q ordering for the caller's key material.
typedef struct SSLRsaPrivateKey {
    s32 modulus_len;
    const u8 *modulus;
    s32 first_prime_len;
    const u8 *first_prime;
    s32 second_prime_len;
    const u8 *second_prime;
    s32 first_exponent_len;
    const u8 *first_exponent;
    s32 second_exponent_len;
    const u8 *second_exponent;
    s32 coefficient_len;
    const u8 *coefficient;
} SSLRsaPrivateKey;

// Decrypts with both prime/exponent pairs, recombines their results, then reduces by the full modulus.
// Seven n-digit regions and BigInt_Div's three-n-digit scratch occupy exactly 20*n allocated bytes.
static void SSLi_DecryptPreMasterSecret(SSLContext *ctx, u8 *encrypted, void *keyParam) {
    const SSLRsaPrivateKey *key = (const SSLRsaPrivateKey *)keyParam;
    int n;
    u16 *buf;
    u16 *operand, *intermediate, *firstResult, *secondResult, *prime, *result, *work;
    u32 priority;

    if (key == NULL || key->modulus_len == 0) {
        return;
    }

    n = (key->modulus_len * 2) / 2 + 1;
    buf = (u16 *)CPSiAlloc(20 * n);
    if (buf == NULL) {
        return;
    }

    operand = buf + n;
    intermediate = operand + n;
    firstResult = intermediate + n;
    secondResult = firstResult + n;
    prime = secondResult + n;
    result = prime + n;
    work = result + n;

    BigInt_BytesToDigits(buf, encrypted, key->modulus_len, n);
    BigInt_BytesToDigits(operand, key->first_exponent, key->first_exponent_len, n);
    BigInt_BytesToDigits(prime, key->first_prime, key->first_prime_len, n);
    priority = SSLi_RaisePriority();
    BigInt_ModExpFast(firstResult, buf, operand, n, prime);
    BigInt_BytesToDigits(operand, key->second_exponent, key->second_exponent_len, n);
    BigInt_BytesToDigits(prime, key->second_prime, key->second_prime_len, n);
    BigInt_ModExpFast(secondResult, buf, operand, n, prime);
    SSLi_RestorePriority(priority);

    BigInt_Sub(buf, firstResult, secondResult, n);
    BigInt_BytesToDigits(operand, key->coefficient, key->coefficient_len, n);
    BigInt_Mul(intermediate, buf, operand, n);
    BigInt_BytesToDigits(operand, key->second_prime, key->second_prime_len, n);
    BigInt_Mul(buf, intermediate, operand, n);
    BigInt_Add(intermediate, buf, secondResult, n);
    BigInt_BytesToDigits(operand, key->modulus, key->modulus_len, n);

    if (BigInt_GetSign(intermediate, n) < 0) {
        BigInt_Negate(intermediate, n);
        BigInt_Div(NULL, intermediate, operand, result, n, work);
        BigInt_Sub(result, operand, result, n);
    } else {
        BigInt_Div(NULL, intermediate, operand, result, n, work);
    }
    BigInt_DigitsToBytes((u8 *)ctx, result, 0x30, n);
    CPSiFree(buf);
}

// One block of SSLv3's key material generation: MD5(secret + SHA1(label + secret + client_random + server_random)),
// reusing the handshake hash snapshot slots as scratch since they aren't needed yet this early
static void SSLi_HashKeyMaterialBlock(u8 *out, const u8 *label, SSLContext *ctx) {
    u8 digest[20];

    SHA1Init(&ctx->handshake_sha1_snapshot);
    SHA1Update(&ctx->handshake_sha1_snapshot, label, strlen((const char *)label));
    SHA1Update(&ctx->handshake_sha1_snapshot, ctx->secret, 0x30);
    SHA1Update(&ctx->handshake_sha1_snapshot, ctx->client_random, 0x40);
    SHA1Final(&ctx->handshake_sha1_snapshot, digest);

    MD5Init(&ctx->handshake_md5_snapshot);
    MD5Update(&ctx->handshake_md5_snapshot, ctx->secret, 0x30);
    MD5Update(&ctx->handshake_md5_snapshot, digest, 0x14);
    MD5Final(&ctx->handshake_md5_snapshot, out);
}

// Turns the pre-master secret (ctx->secret) into the 0x30-byte master secret, overwriting it in place:
// MD5(pms+SHA1("A"+pms+randoms)) || MD5(pms+SHA1("BB"+pms+randoms)) || MD5(pms+SHA1("CCC"+pms+randoms))
static void SSLi_GenerateMasterSecret(SSLContext *ctx) {
    u8 block[0x30];

    SSLi_HashKeyMaterialBlock(block, sLabelA, ctx);
    SSLi_HashKeyMaterialBlock(block + 0x10, sLabelBB, ctx);
    SSLi_HashKeyMaterialBlock(block + 0x20, sLabelCCC, ctx);
    sys_memcpy(block, ctx->secret, 0x30);
}

// Generates the key_block (SSLv3's generate_key_material, repeating "A", "BB", "CCC", ... for each round) into
// ctx->session_id and the scratch past it, which the handshake has no further use for, then splits it into this
// side's write and read MAC secrets and RC4 keys depending on which half of each pair ctx->unk_454 (the role) picks,
// and sets both RC4 contexts up (there is no IV: this cipher suite is always RC4)
static void SSLi_GenerateKeyBlock(SSLContext *ctx) {
    int macLen = ctx->cipher_suite == 4 ? 0x10 : ctx->cipher_suite == 5 ? 0x14 : 0;
    const int keyLen = 0x10;
    int total = (macLen + keyLen) * 2;
    u8 *block = ctx->session_id;
    int round;

    for (round = 0; (round + 1) * 0x10 < total; round++) {
        u8 seed[0x15];
        u8 digest[0x14];
        int i;

        for (i = 0; i <= round; i++) {
            seed[i] = (u8)(0x41 + round);
        }

        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, seed, round + 1);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->secret, 0x30);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->server_random, 0x20);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->client_random, 0x20);
        SHA1Final(&ctx->handshake_sha1_snapshot, digest);

        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->secret, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, digest, 0x14);
        MD5Final(&ctx->handshake_md5_snapshot, block + round * 0x10);
    }

    if (ctx->unk_454) {
        ctx->read_mac_secret = block;
        ctx->read_key = block + macLen * 2;
        ctx->write_mac_secret = block + macLen;
        ctx->write_key = block + macLen * 2 + keyLen;
    } else {
        ctx->write_mac_secret = block;
        ctx->write_key = block + macLen * 2;
        ctx->read_mac_secret = block + macLen;
        ctx->read_key = block + macLen * 2 + keyLen;
    }

    CPSi_rc4_init(&ctx->read_rc4, ctx->read_key, 0x10);
    CPSi_rc4_init(&ctx->write_rc4, ctx->write_key, 0x10);
}

// Finishes a ClientKeyExchange that carried an RSA-encrypted pre-master secret with this connection's own key
// (for the quick-resume ticket's key exchange; see SSLi_DecryptPreMasterSecret), derives the master secret and
// key block from it, remembers the session and sets this side's role for the key block's halves
static void SSLi_FinishKeyExchange(SSLContext *ctx, u8 *msg) {
    SSLi_DecryptPreMasterSecret(ctx, msg, ctx->key_param);
    SSLi_GenerateMasterSecret(ctx);
    SSLi_SaveSession(ctx, 0, 0);
    SSLi_GenerateKeyBlock(ctx);
    ctx->state = SSLI_STATE_RESUMED_HELLO;
}

// Half of a Finished message's MAC (SHA1 is SSLi_ComputeFinishedSha1): HMAC-SSLv3 style, with the running MD5
// handshake hash's digest (not continued afterward: the snapshot is for a later quick resume, not this) as the
// inner hash's last input, under the given sender's 4-byte label ("CLNT" or "SRVR")
static void SSLi_ComputeFinishedMd5(SSLContext *ctx, u8 *out, int sender) {
    const u8 *label = (ctx->unk_454 == sender) ? sLabelClient : sLabelServer;
    u8 pad[0x30];

    // Finishes the running handshake hash (it has no further use: Finished is the last message it covers)
    MD5Update(&ctx->handshake_md5, label, 4);
    MD5Update(&ctx->handshake_md5, ctx->secret, 0x30);
    sys_memset(pad, 0x36, 0x30);
    MD5Update(&ctx->handshake_md5, pad, 0x30);
    MD5Final(&ctx->handshake_md5, out);

    MD5Init(&ctx->handshake_md5);
    MD5Update(&ctx->handshake_md5, ctx->secret, 0x30);
    sys_memset(pad, 0x5c, 0x30);
    MD5Update(&ctx->handshake_md5, pad, 0x30);
    MD5Update(&ctx->handshake_md5, out, 0x10);
    MD5Final(&ctx->handshake_md5, out);
}

static void SSLi_ComputeFinishedSha1(SSLContext *ctx, u8 *out, int sender) {
    const u8 *label = (ctx->unk_454 == sender) ? sLabelClient : sLabelServer;
    u8 pad[0x28];

    SHA1Update(&ctx->handshake_sha1, label, 4);
    SHA1Update(&ctx->handshake_sha1, ctx->secret, 0x30);
    sys_memset(pad, 0x36, 0x28);
    SHA1Update(&ctx->handshake_sha1, pad, 0x28);
    SHA1Final(&ctx->handshake_sha1, out);

    SHA1Init(&ctx->handshake_sha1);
    SHA1Update(&ctx->handshake_sha1, ctx->secret, 0x30);
    sys_memset(pad, 0x5c, 0x28);
    SHA1Update(&ctx->handshake_sha1, pad, 0x28);
    SHA1Update(&ctx->handshake_sha1, out, 0x14);
    SHA1Final(&ctx->handshake_sha1, out);
}

// Checks a received Finished message against what this side computes for the sender it names, restoring the
// running handshake hash after each half so it still covers this message for whichever Finished comes next
static void SSLi_VerifyFinished(SSLContext *ctx, u8 *msg, int sender) {
    u8 computed[0x14];

    ctx->handshake_md5_snapshot = ctx->handshake_md5;
    SSLi_ComputeFinishedMd5(ctx, computed, sender);
    ctx->handshake_md5 = ctx->handshake_md5_snapshot;
    if (memcmp(msg, computed, 0x10) != 0) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }

    ctx->handshake_sha1_snapshot = ctx->handshake_sha1;
    SSLi_ComputeFinishedSha1(ctx, computed, sender);
    ctx->handshake_sha1 = ctx->handshake_sha1_snapshot;
    if (memcmp(msg + 0x10, computed, 0x14) != 0) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    ctx->state = SSLI_STATE_CHANGE_CIPHER_SPEC;
}

// Adds 1 to an 8-byte big-endian sequence number, carrying leftward
static void SSLi_IncrementCounter(u8 seq[8]) {
    int i = 8;
    do {
        i--;
        if (++seq[i] != 0) {
            break;
        }
    } while (i != 0);
}

// RC4-crypts buf with this connection's write key, handing len back unchanged for the callers that only crypt as
// a step partway through building a record
static u32 SSLi_Rc4Crypt(SSLContext *ctx, void *buf, u32 len) {
    CPSi_rc4_crypt(&ctx->read_rc4, buf, len);
    return len;
}

// Appends this record's MAC to its content (SSLv3's: hash(mac_secret + pad1 + seq_num + type + length + content),
// then hash(mac_secret + pad2 + that)) and RC4-crypts the whole thing, advancing the sequence number. hdr is the
// record header (type, version, length) with the content right after it; returns the header's own size (5)
// plus the content and MAC's combined length
static u32 SSLi_ComputeMac(SSLContext *ctx, u8 *hdr) {
    u32 len = (hdr[3] << 8) + hdr[4];
    u8 *content = hdr + 5;
    u8 pad[0x30];

    if (ctx->cipher_suite == 4) {
        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->write_mac_secret, 0x10);
        sys_memset(pad, 0x36, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, pad, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->write_seq_num, 8);
        MD5Update(&ctx->handshake_md5_snapshot, hdr, 1);
        MD5Update(&ctx->handshake_md5_snapshot, hdr + 3, len + 2);
        MD5Final(&ctx->handshake_md5_snapshot, content + len);

        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->write_mac_secret, 0x10);
        sys_memset(pad, 0x5c, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, pad, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, content + len, 0x10);
        MD5Final(&ctx->handshake_md5_snapshot, content + len);
        len += 0x10;
    } else if (ctx->cipher_suite == 5) {
        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->write_mac_secret, 0x14);
        sys_memset(pad, 0x36, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, pad, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->write_seq_num, 8);
        SHA1Update(&ctx->handshake_sha1_snapshot, hdr, 1);
        SHA1Update(&ctx->handshake_sha1_snapshot, hdr + 3, len + 2);
        SHA1Final(&ctx->handshake_sha1_snapshot, content + len);

        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->write_mac_secret, 0x14);
        sys_memset(pad, 0x5c, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, pad, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, content + len, 0x14);
        SHA1Final(&ctx->handshake_sha1_snapshot, content + len);
        len += 0x14;
    }

    hdr[3] = (u8)(len >> 8);
    hdr[4] = (u8)len;
    CPSi_rc4_crypt(&ctx->write_rc4, content, len);
    SSLi_IncrementCounter(ctx->write_seq_num);
    return len + 5;
}

// RC4-decrypts a received record and checks its MAC, the mirror of SSLi_ComputeMac: hdr's length still covers the
// MAC, which this strips off after recomputing it over the shorter content and comparing. Sets ctx->state to an
// error on a mismatch either way, so the caller doesn't need to check this call's return for that
static void SSLi_CheckRecordMac(SSLContext *ctx, u8 *hdr) {
    u32 len = (u32)SSLi_Rc4Crypt(ctx, hdr + 5, (hdr[3] << 8) + hdr[4]);
    u8 pad[0x30];
    u8 mac[0x14];
    int macLen;

    if (ctx->cipher_suite == 4) {
        macLen = 0x10;
        len -= macLen;
        hdr[3] = (u8)(len >> 8);
        hdr[4] = (u8)len;

        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->read_mac_secret, 0x10);
        sys_memset(pad, 0x36, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, pad, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->read_seq_num, 8);
        MD5Update(&ctx->handshake_md5_snapshot, hdr, 1);
        MD5Update(&ctx->handshake_md5_snapshot, hdr + 3, len + 2);
        MD5Final(&ctx->handshake_md5_snapshot, mac);

        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->read_mac_secret, 0x10);
        sys_memset(pad, 0x5c, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, pad, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, mac, 0x10);
        MD5Final(&ctx->handshake_md5_snapshot, mac);
    } else {
        macLen = 0x14;
        len -= macLen;
        hdr[3] = (u8)(len >> 8);
        hdr[4] = (u8)len;

        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->read_mac_secret, 0x14);
        sys_memset(pad, 0x36, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, pad, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->read_seq_num, 8);
        SHA1Update(&ctx->handshake_sha1_snapshot, hdr, 1);
        SHA1Update(&ctx->handshake_sha1_snapshot, hdr + 3, len + 2);
        SHA1Final(&ctx->handshake_sha1_snapshot, mac);

        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->read_mac_secret, 0x14);
        sys_memset(pad, 0x5c, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, pad, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, mac, 0x14);
        SHA1Final(&ctx->handshake_sha1_snapshot, mac);
    }

    if (memcmp(hdr + 5 + len, mac, macLen) != 0) {
        ctx->state = SSLI_STATE_ERROR;
    }
    SSLi_IncrementCounter(ctx->read_seq_num);
}

// Reads exactly len bytes from the socket's TCP stream into dest, waiting across as many reads as it takes; -1 if
// the connection ends first
static int SSLi_ReadExact(u8 *dest, u32 len, CPSSoc *soc) {
    while (len > 0) {
        u32 avail;
        u8 *data = CPSi_TcpRead(&avail, soc);

        if (avail == 0) {
            return -1;
        }
        if (avail > len) {
            avail = len;
        }
        sys_memcpy(data, dest, avail);
        CPSi_TcpConsume(avail, soc);
        dest += avail;
        len -= avail;
    }
    return 0;
}

// Feeds data into both running handshake hashes, so Finished covers every handshake message once it's sent
static void SSLi_UpdateHandshakeHash(SSLContext *ctx, const u8 *data, u32 len) {
    SHA1Update(&ctx->handshake_sha1, data, len);
    MD5Update(&ctx->handshake_md5, data, len);
}

// Draws len bytes from sPool, a running SHA1 digest CPS_SslAddRandomSeed stirs seed into, reseeding first from
// CPSiRand (the stack's own generator) if nothing ever has. sPool is updated (a carry-add of a fresh SHA1 digest of
// itself) a 20-byte block at a time, and a zero byte never comes out since that cipher suite byte would be
// mistaken for a string terminator downstream
static void SSLi_GetRandomBytes(u8 *dest, u32 len) {
    u32 produced = 0;
    int pos = 0x14;
    u8 digest[0x14];

    if (!sPoolSeeded) {
        u32 seed = MATH_Rand32(&CPSiRand, 0);
        CPS_SslAddRandomSeed(&seed, 4);
    }

    while (produced < len) {
        if (pos == 0x14) {
            SHA1Context sha1;
            int i, carry;

            SHA1Init(&sha1);
            SHA1Update(&sha1, sPool, 0x14);
            SHA1FinalNoLength(&sha1, digest);

            carry = 1;
            for (i = 0x13; i >= 0; i--) {
                int sum = sPool[i] + digest[i] + carry;
                sPool[i] = (u8)sum;
                carry = sum >> 8;
            }
            pos = 0;
        }
        if (digest[pos] != 0) {
            dest[produced++] = digest[pos];
        }
        pos++;
    }
}

// CPS_SslAddRandomSeed's role: stirs more entropy into SSLi_GetRandomBytes's pool (a plain SHA1 of the old pool and
// the new seed) and marks it seeded, so the next draw no longer reseeds from CPSiRand on its own
void CPS_SslAddRandomSeed(const void *seed, u32 len) {
    SHA1Context sha1;
    u32 enabled;

    SHA1Init(&sha1);
    enabled = CPU_IRQDisable();
    SHA1Update(&sha1, sPool, 0x14);
    SHA1Update(&sha1, seed, len);
    SHA1Final(&sha1, sPool);
    CPU_SetIRQMask(enabled);
    sPoolSeeded = TRUE;
}

// Parses a ClientKeyExchange-shaped message's RSA-encrypted content, or dispatches to the handshake message
// SSLi_ParseAsn1Field's certificate fields and ctx->state pick out: ServerHello, Certificate, ServerKeyExchange,
// CertificateRequest, ServerHelloDone and ClientKeyExchange (should this side ever receive one, acting as a
// server). msgLen counts only the message body, after the 4-byte handshake header msg already points past.
// The precise dispatch this uses past ctx->state isn't recovered (see docs/nonmatching-functions.md)
static void SSLi_ProcessHandshakeMessage(SSLContext *ctx, u8 *msg, u32 msgLen) {
    (void)msg;
    (void)msgLen;
    ctx->state = SSLI_STATE_ERROR;
}

// Reads one TLS record's header and body and acts on it: a handshake message goes to
// SSLi_ProcessHandshakeMessage, a ChangeCipherSpec or an application data record update ctx->state so the caller
// (SSLi_DoHandshake or CPS_SslRead) can tell what arrived, and any record past the handshake is decrypted and its
// MAC checked first (SSLi_CheckRecordMac). Returns ctx->state, or SSLI_STATE_ERROR if the connection ended
static int SSLi_ReceiveRecord(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u8 hdr[5];

    if (SSLi_ReadExact(hdr, 5, soc) != 0) {
        ctx->state = SSLI_STATE_ERROR;
        return ctx->state;
    }

    if (hdr[0] != 0x16) {
        ctx->state = SSLI_STATE_ERROR;
        return ctx->state;
    }

    {
        u32 len = (hdr[3] << 8) + hdr[4] + 5;
        u8 *buf = (u8 *)CPSiAlloc(len);
        if (buf == NULL) {
            ctx->state = SSLI_STATE_ERROR;
            return ctx->state;
        }
        sys_memcpy(hdr, buf, 5);
        if (SSLi_ReadExact(buf + 5, len - 5, soc) != 0) {
            CPSiFree(buf);
            ctx->state = SSLI_STATE_ERROR;
            return ctx->state;
        }
        SSLi_ProcessHandshakeMessage(ctx, buf, len);
    }
    return ctx->state;
}

// Sends ClientHello: version, a fresh client random (a Unix-ish timestamp, then 28 more random bytes), the cached
// session id for this peer if there is one, and the 2 cipher suites this connection offers
static void SSLi_SendClientHello(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u8 msg[0x80];
    u8 *p = msg + 9;
    u32 time;

    time = SSLi_GetUnixTime();
    ctx->client_random[0] = (u8)(time >> 24);
    ctx->client_random[1] = (u8)(time >> 16);
    ctx->client_random[2] = (u8)(time >> 8);
    ctx->client_random[3] = (u8)time;
    SSLi_GetRandomBytes(ctx->client_random + 4, 0x1c);

    sys_memcpy(ctx->client_random, p + 2, 0x20);
    SSLi_FindSessionByPeer(ctx, soc->remote_ip, soc->remote_port);

    if (ctx->resumed) {
        p[0x22] = 0x20;
        sys_memcpy(ctx->session_id, p + 0x23, 0x20);
        p += 0x43;
    } else {
        p[0x22] = 0;
        p += 0x23;
    }
    p[0] = 0;
    p[1] = 4;
    sys_memcpy(sConfig.cipher_suites, p + 2, 4);
    p[6] = 1;
    p[7] = 0;

    msg[0] = 0x16;
    msg[1] = 3;
    msg[2] = 0;
    {
        u32 len = (u32)(p + 8 - (msg + 5));
        msg[3] = (u8)(len >> 8);
        msg[4] = (u8)len;
        msg[5] = 1;
        msg[6] = 0;
        msg[7] = (u8)(len >> 16);
        msg[8] = (u8)(len >> 8);
        CPSi_TcpWrite(msg, (u32)(p + 8 - msg), NULL, 0, soc);
        SSLi_UpdateHandshakeHash(ctx, msg + 5, len);
    }
}

// Sends ClientKeyExchange: the pre-master secret (a fresh random value, with SSLv3's version in its first 2 bytes)
// RSA-encrypted under the server's public key, in up to 0xb4f-byte chunks (CPSiAlloc can't promise more at once).
// Sends a "no certificate" alert first if the server asked for one and the caller has none to offer
static void SSLi_SendClientKeyExchange(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (*((u8 *)ctx + 0x5ac)) { // set while parsing a CertificateRequest; see docs/nonmatching-functions.md
        CPSi_TcpWrite((u8 *)sNoCertificateAlert, 7, NULL, 0, soc);
    }

    ctx->secret[0] = 3;
    ctx->secret[1] = 0;
    SSLi_GetRandomBytes(ctx->secret + 2, 0x2e);

    {
        const SSLCertificateKey *key = (const SSLCertificateKey *)ctx->key_param;
        u32 n = key->modulus_len;
        u8 *encrypted = (u8 *)CPSiAlloc(n);
        u32 priority;

        if (encrypted == NULL) {
            ctx->state = SSLI_STATE_ERROR;
            return;
        }
        sys_memset(encrypted, 0, 2);
        encrypted[1] = 2;
        SSLi_GetRandomBytes(encrypted + 2, n - 0x23);
        encrypted[n - 0x21] = 0;
        sys_memcpy(ctx->secret, encrypted + n - 0x30, 0x30);

        priority = SSLi_RaisePriority();
        {
            int digitCount = ((n << 1 | (n >> 31)) >> 1) + 1;
            u16 *digits = (u16 *)CPSiAlloc(digitCount * 6);
            u16 *c = digits;
            u16 *e = c + digitCount;
            u16 *mod = e + digitCount;

            BigInt_BytesToDigits(c, encrypted, n, digitCount);
            BigInt_BytesToDigits(e, key->exponent, key->exponent_len, digitCount);
            BigInt_BytesToDigits(mod, key->modulus, n, digitCount);
            BigInt_ModExpFast(c, c, e, digitCount, mod);
            BigInt_DigitsToBytes(encrypted, c, n, digitCount);
            CPSiFree(digits);
        }
        SSLi_RestorePriority(priority);

        {
            u8 hdr[9];
            hdr[0] = 0x16;
            hdr[1] = 3;
            hdr[2] = 0;
            hdr[3] = (u8)((n + 4) >> 8);
            hdr[4] = (u8)(n + 4);
            hdr[5] = 0x10;
            hdr[6] = (u8)(n >> 16);
            hdr[7] = (u8)(n >> 8);
            hdr[8] = (u8)n;
            CPSi_TcpWrite(hdr, 9, encrypted, n, soc);
            SSLi_UpdateHandshakeHash(ctx, hdr + 5, n + 4);
        }
        CPSiFree(encrypted);
    }
}

// Sends ChangeCipherSpec (switching this side to the keys SSLi_GenerateKeyBlock made) and Finished (the two
// Finished MAC halves, encrypted and MACed with the new write keys since ChangeCipherSpec already took effect)
static void SSLi_SendChangeCipherSpecAndFinished(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u8 ccs[6] = { 0x14, 3, 0, 0, 1, 1 };
    u8 msg[0x60];

    CPSi_TcpWrite(ccs, 6, NULL, 0, soc);

    msg[0] = 0x16;
    msg[1] = 3;
    msg[2] = 0;
    msg[3] = 0;
    msg[4] = 0x24;
    msg[5] = 0x14;
    msg[6] = 0;
    msg[7] = 0;
    msg[8] = 0;
    msg[9] = 0;

    SSLi_ComputeFinishedMd5(ctx, msg + 9, 0);
    SSLi_ComputeFinishedSha1(ctx, msg + 0x19, 0);
    SSLi_UpdateHandshakeHash(ctx, msg + 5, 0x24);

    {
        u32 len = SSLi_ComputeMac(ctx, msg);
        CPSi_TcpWrite(msg, len, NULL, 0, soc);
    }
}

// Waits for the server's ChangeCipherSpec and Finished, in that order; TRUE if the connection ended first (the
// Finished check itself sets ctx->state to an error rather than failing this)
static BOOL SSLi_WaitForChangeCipherSpecAndFinished(CPSSoc *soc) {
    if (SSLi_ReceiveRecord(soc) == SSLI_STATE_ERROR) {
        return TRUE;
    }
    if (SSLi_ReceiveRecord(soc) == SSLI_STATE_ERROR) {
        return TRUE;
    }
    return FALSE;
}

// Drives a full or abbreviated handshake: ClientHello, then everything up to ServerHelloDone (full) or straight to
// ChangeCipherSpec (a session the server resumed); then ClientKeyExchange, or not, and both sides' ChangeCipherSpec
// and Finished. ctx->state ends up SSLI_STATE_ALERT (and the connection unusable) if anything along the way failed
static BOOL SSLi_DoHandshake(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    SSLi_SendClientHello(soc);
    while (TRUE) {
        if (SSLi_ReceiveRecord(soc) == SSLI_STATE_ERROR) {
            goto fail;
        }
        if (ctx->state == SSLI_STATE_HANDSHAKE_DONE || ctx->unk_031) {
            break;
        }
    }

    if (ctx->unk_031) {
        SSLi_GenerateKeyBlock(ctx);
        if (SSLi_WaitForChangeCipherSpecAndFinished(soc)) {
            return TRUE;
        }
        SSLi_SendChangeCipherSpecAndFinished(soc);
    } else {
        SSLi_SendClientKeyExchange(soc);
        SSLi_GenerateMasterSecret(ctx);
        if (ctx->resumed) {
            SSLi_SaveSession(ctx, soc->remote_ip, soc->remote_port);
        }
        SSLi_GenerateKeyBlock(ctx);
        SSLi_SendChangeCipherSpecAndFinished(soc);
        if (SSLi_WaitForChangeCipherSpecAndFinished(soc)) {
            return TRUE;
        }
    }
    goto fail; // SSLv3's own flow leaves ctx->state SSLI_STATE_ALERT either way; see docs/nonmatching-functions.md

fail:
    ctx->state = SSLI_STATE_ALERT;
    return FALSE;
}

// CPS_SslConnect's role: connects the socket's TCP first if it isn't already, then runs the handshake
int CPS_SslConnect(CPSSoc *soc) {
    u8 state = soc->state;
    SSLContext *ctx = (SSLContext *)soc->con;

    if (state != CPS_STT_ESTABLISHED) {
        if (CPSi_TcpConnect(soc) != 0) {
            return TRUE;
        }
    }

    ctx->state = 0;
    ctx->read_mac_secret = NULL;
    ctx->unk_454 = 0;
    SHA1Init(&ctx->handshake_sha1);
    MD5Init(&ctx->handshake_md5);
    return SSLi_DoHandshake(soc);
}

// CPS_SslWrite's role: sends data as application data records, each up to 0xb4f bytes of content, MACed and
// RC4-crypted by SSLi_ComputeMac. data2/len2 is a second buffer CPS_SocWrite-style callers hand this to send right
// after the first, as one write; returns the total bytes sent, which may be less than len+len2 if CPSiAlloc or the
// connection itself runs out partway
static u32 CPS_SslWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u32 total = len + len2;
    u32 sent = 0;

    while (total > 0) {
        u32 chunk = total > 0xb4f ? 0xb4f : total;
        u8 *buf = (u8 *)CPSiAlloc(chunk + 0x19);
        u32 sentNow;

        if (buf == NULL) {
            break;
        }
        if (len > 0) {
            u32 n = chunk < len ? chunk : len;
            sys_memcpy(data, buf + 5, n);
            data += n;
            len -= n;
            if (n < chunk) {
                sys_memcpy(data2, buf + 5 + n, chunk - n);
                data2 += chunk - n;
                len2 -= chunk - n;
            }
        } else {
            sys_memcpy(data2, buf + 5, chunk);
            data2 += chunk;
            len2 -= chunk;
        }

        buf[0] = 0x17;
        buf[1] = 3;
        buf[2] = 0;
        buf[3] = (u8)(chunk >> 8);
        buf[4] = (u8)chunk;
        sentNow = SSLi_ComputeMac(ctx, buf);

        if (CPSi_TcpWrite(buf, sentNow, NULL, 0, soc) < sentNow) {
            chunk = 0;
        }
        CPSiFree(buf);
        sent += chunk;
        total -= chunk;
        if (chunk == 0) {
            break;
        }
    }
    return sent;
}

// Reads one more application data record into ctx->read_buf if it isn't already there, buffering the whole record
// (CPSi_TcpRead's own chunking aside) since its MAC can't be checked until all of it has arrived. Keeps reading and
// processing other records (handshake messages too, for a renegotiation) in between until one does turn out to be
// application data, or the connection ends. ctx->buf_owned marks a complete, MAC-checked buffer
static void SSLi_FillReadBuffer(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->read_buf != NULL || ctx->state == SSLI_STATE_ERROR) {
        return;
    }
    if (soc->state < CPS_STT_ESTABLISHED) {
        return;
    }

    while (TRUE) {
        if (SSLi_ReceiveRecord(soc) == SSLI_STATE_ERROR) {
            return;
        }
        if (ctx->buf_owned) {
            break;
        }
    }
}

// CPS_SslRead's role: a pointer into the application data the handshake has buffered and not yet given out, and
// *len to how much of it there is; NULL (with *len 0) once the connection has nothing left and has closed
u8 *CPS_SslRead(u32 *len, CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->read_buf != NULL && !ctx->buf_owned) {
        u8 *data = ctx->read_buf + ctx->read_buf_pos;
        u32 remaining = ctx->read_buf_len - ctx->read_buf_pos;

        if (SSLi_ReadExact(data, remaining, soc) != 0) {
            CPSiFree(ctx->read_buf);
            ctx->read_buf = NULL;
            *len = 0;
            return NULL;
        }
        SSLi_ProcessHandshakeMessage(ctx, ctx->read_buf, 0);
        if (!ctx->buf_owned) {
            ctx->read_buf = NULL;
        }
    }

    while (ctx->read_buf == NULL) {
        if (SSLi_ReceiveRecord(soc) == SSLI_STATE_ERROR) {
            *len = 0;
            return NULL;
        }
    }

    *len = ctx->read_buf_len - ctx->read_buf_pos;
    return ctx->read_buf + ctx->read_buf_pos;
}

// CPS_SslReadRelease's role: marks len bytes of what CPS_SslRead last returned as taken, freeing the
// buffer once all of it has been
void CPS_SslReadRelease(u32 len, CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (len >= ctx->read_buf_len - ctx->read_buf_pos) {
        if (ctx->read_buf != NULL) {
            CPSiFree(ctx->read_buf);
        }
        ctx->read_buf = NULL;
        return;
    }
    ctx->read_buf_pos += len;
}

// CPS_SslCanRead's role: how many bytes CPS_SslRead would give out right now, or -1 once the connection has
// closed with nothing left
s32 CPS_SslCanRead(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->read_buf == NULL || !ctx->buf_owned) {
        SSLi_FillReadBuffer(soc);
    }
    if (ctx->read_buf != NULL && ctx->buf_owned) {
        return (s32)(ctx->read_buf_len - ctx->read_buf_pos);
    }
    if (ctx->read_buf == NULL && soc->state == CPS_STT_ESTABLISHED && ctx->state == SSLI_STATE_ERROR) {
        return -1;
    }
    return 0;
}

// This file's signature, which CPS_SetSsl refers to purely to keep it linked in
static const char sSslSignature[] = "[SDK+UBIQUITOUS:SSL]";

// CPS_SslShutdown's role: sends a close_notify alert if the connection ended in one already (SSLv3's own way to
// say goodbye), then resets the state so a later CPS_SslConnect can reuse the connection
void CPS_SslShutdown(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->state == SSLI_STATE_ALERT) {
        u8 msg[7];

        msg[0] = 0x15;
        msg[1] = 3;
        msg[2] = 0;
        msg[3] = 0;
        msg[4] = 2;
        msg[5] = 1;
        msg[6] = 0;
        CPSi_TcpWrite(msg, SSLi_ComputeMac(ctx, msg), NULL, 0, soc);
    }
    ctx->state = 0;
}

// CPS_SslClose's role: frees the connection's read buffer, if it has one, and resets its state
void CPS_SslClose(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    ctx->state = 0;
    if (ctx->read_buf != NULL) {
        CPSiFree(ctx->read_buf);
    }
    ctx->read_buf = NULL;
}

// CPS_SetSsl's role: turns the SSL layer on for the caller's socket (SOCL_EnableSsl's own caller already set
// soc->con before this)
void CPS_SetSsl(BOOL enable) {
    OSThread *thread;
    CPSSoc *soc;

    func_02004eb0(sSslSignature);
    thread = OS_GetCurrentThread();
    soc = (CPSSoc *)thread->specific[0];
    if (soc != NULL) {
        soc->ssl = (u8)enable;
    }
}

// CPS_SslTick's role: called once a second by CPS's own housekeeping thread. Expires session cache entries that
// have sat unused too long, and wakes any thread that has been blocked reading an SSL socket for too long, so its
// own timeout logic can run
void CPS_SslTick(s32 now) {
    u32 enabled = CPU_IRQDisable();
    int i;
    OSThread *thread;

    for (i = 0; i < 4; i++) {
        SSLSessionCacheEntry *entry = &sSessionCache[i];
        if (entry->valid && now - entry->time > 0x3bd) {
            entry->valid = FALSE;
        }
    }
    CPU_SetIRQMask(enabled);

    for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
        CPSSoc *soc = (CPSSoc *)thread->specific[0];
        if (soc != NULL && soc->con != NULL && soc->ssl != 0 && soc->state == CPS_STT_ESTABLISHED &&
            ((SSLContext *)soc->con)->read_buf == NULL && now - soc->when > 0xef && soc->block_type == CPS_BLOCK_TCPREAD) {
            soc->state = 0;
            soc->block_type = CPS_BLOCK_NONE;
            scheduler_start_thread(thread);
        }
    }
}

// CPS_SslInit's role: clears the session cache, once at CPS_Startup
void CPS_SslInit(void) {
    sys_memset(sSessionCache, 0, sizeof(sSessionCache));
}
