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

// File-local descriptive names; no original SDK type names have been established.
typedef struct SSLCertificatePublicKey {
    u8 modulus[0x100];
    s32 modulus_len;
    u8 exponent[8];
    s32 exponent_len;
} SSLCertificatePublicKey;

typedef struct SSLRsaPublicKey {
    const char *name;
    s32 modulus_len;
    const u8 *modulus;
    s32 exponent_len;
    const u8 *exponent;
} SSLRsaPublicKey;

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

// A connection, DER-parsed certificate fields included. 0x830 bytes
typedef struct SSLContext {
    u8 secret[0x30]; // The pre-master secret, overwritten with the master secret once derived
    u8 resumed;       // TRUE once a cached session's secret was restored
    u8 unk_031;
    u16 cipher_suite;
    u8 client_random[0x20];
    u8 server_random[0x20];
    // Session storage becomes the embedded key block after session-cache operations.
    // The final generated block overlaps pointer slots, assigned only after generation.
    union {
        u8 key_block[0x50];
        struct {
            u8 session_id[0x20];
            u8 unk_094[0x28];
            u8 *write_mac_secret;
            u8 *write_key;
        } fields;
    } key_storage; // Descriptive reconstruction: 0x74..0xc3, not an established SDK type.
    u8 unk_0c4[4];
    CPSRc4Context write_rc4;
    u8 unk_1ca[2];
    u8 write_seq_num[8]; // The 8-byte big-endian sequence number SSLi_ComputeMac folds into an outgoing record's MAC
    u8 *read_mac_secret;
    u8 *read_key;
    u8 unk_1dc[4];
    CPSRc4Context read_rc4;
    u8 unk_2e2[2];
    u8 read_seq_num[8]; // Same, for a received record's MAC; both start at 0 and SSLi_IncrementCounter advances them
    SHA1Context handshake_sha1;          // The running hash of every handshake message so far
    SHA1Context handshake_sha1_snapshot; // Copied for Finished; the snapshot contexts also serve as certificate-hash
                                         // scratch storage
    MD5Context handshake_md5;
    MD5Context handshake_md5_snapshot;
    u8 is_server; // Descriptive role name: nonzero selects the server handshake path
    u8 state;     // SSLI_STATE_*, read back by SSLi_ReceiveRecord's caller
    u8 application_record_ready; // TRUE when read_buf holds a complete application record retained by dispatch
    u8 pad_457;
    s32 signature_algorithm; // 0x458: OID index 3 (MD5) or 4 (SHA1)
    s32 public_key_algorithm;
    const u8 *signed_start; // Borrowed DER span, valid while the input certificate buffer is owned by the caller
    const u8 *signed_end;
    u8 certificate_digest[0x14];
    u32 certificate_digest_len;
    SSLRsaPublicKey chain_key;               // 0x480; borrowed next-certificate key spans, with the name slot unused
    SSLCertificatePublicKey certificate_key; // 0x494, ends at 0x5a4
    const u8 *certificate_signature;
    s32 certificate_signature_len;
    // One byte reused across phases, not two independent flags.
    union {
        u8 parsing_subject;
        u8 certificate_requested;
    } certificate_phase; // 0x5ac
    u8 parsing_rsa_key;
    u8 attribute_oid;
    u8 certificate_date_valid;
    char issuer[0x100];
    char subject[0x100];
    char common_name[0x50];
    const char *hostname;  // 0x800
    const u8 *certificate; // 0x804
    s32 certificate_len;
    u32 current_date;
    int (*validate_certificate)(int result, struct SSLContext *ctx, int index);
    SSLNamedParam *const *named_params; // The caller's registered list, set with CPS_SslSetNamedParams
    u32 named_param_count;
    void *key_param; // Private key consumed by SSLi_FinishKeyExchange; its writer is not recovered
    u8 unk_820[4];
    u8 *read_buf; // Owned incomplete record, or a dispatched application record awaiting consumption
    u32 read_buf_len;
    u32 read_buf_pos;
} SSLContext;

// SSLContext.state
enum {
    SSLI_STATE_INITIAL = 0,
    SSLI_STATE_CLIENT_HELLO_PARSED,
    SSLI_STATE_SERVER_HELLO_PARSED,
    SSLI_STATE_CERTIFICATE_ACCEPTED,
    SSLI_STATE_SERVER_HELLO_DONE = 4,
    SSLI_STATE_MASTER_SECRET_READY = 5,
    SSLI_STATE_FINISHED_VERIFIED = 6,
    SSLI_STATE_EXPECT_FINISHED = 7,
    SSLI_STATE_APPLICATION_DATA = 8,
    SSLI_STATE_ERROR = 9,
};

// Certificate validation combines a low-byte result with these independent flags.
enum {
    SSLI_CERT_RESULT_MASK = 0xff,
    SSLI_CERT_HOSTNAME_MISMATCH = 0x4000,
    SSLI_CERT_DATE_INVALID = 0x8000,
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

// The cipher suites CPS_Sslconnect offers, and the priority applied during selected crypto operations.
// CPS_SslSetMaxPriority configures it; -1 disables priority changes until set.
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
static u32 SSLi_ApplyConfiguredPriority(void);
static void SSLi_RestorePriority(u32 priority);
static const SSLNamedParam *SSLi_FindNamedParam(const SSLContext *ctx, const char *name);
static s32 SSLi_DecodeLength(const u8 **cursor);
static void SSLi_AppendCertificateNameValue(char *dest, const char *value, int byteCount);
static u32 SSLi_ParseCertTime(const u8 *digits, int tag);
static int SSLi_ParseAsn1Field(SSLContext *ctx, const u8 **cursor, int depth, int index, int extra);
static int SSLi_CheckCertificate(SSLContext *ctx);
static int SSLi_LabelLength(const char *host);
static int SSLi_CompareHostname(const char *host, const char *pattern);
static void SSLi_ParseCertificateChain(SSLContext *ctx, const u8 *body);
static int SSLi_VerifyCertificateSignature(SSLContext *ctx, const SSLRsaPublicKey *key);
static void SSLi_ParseServerHello(SSLContext *ctx, u8 *msg);
static BOOL SSLi_ContainsU16(const u8 *data, int count, int width, u32 value);
static u16 SSLi_PickCipherSuite(const u8 *data, int count, int width);
static BOOL SSLi_IsSsl3Version(int major, int minor);
static void SSLi_ParseSsl2ClientHello(SSLContext *ctx, u8 *msg);
static void SSLi_ParseClientHello(SSLContext *ctx, u8 *msg);
static void SSLi_DecryptPreMasterSecret(SSLContext *ctx, u8 *encrypted, void *params);
static void SSLi_HashKeyMaterialBlock(u8 *out, const u8 *label, SSLContext *ctx);
static void SSLi_GenerateMasterSecret(SSLContext *ctx);
static void SSLi_GenerateKeyBlock(SSLContext *ctx);
static void SSLi_FinishKeyExchange(SSLContext *ctx, u8 *msg);
static void SSLi_ComputeFinishedMd5(SSLContext *ctx, u8 *out, int sender);
static void SSLi_ComputeFinishedSha1(SSLContext *ctx, u8 *out, int sender);
static void SSLi_VerifyFinished(SSLContext *ctx, u8 *msg);
static void SSLi_IncrementCounter(u8 *counterEnd);
static u32 SSLi_Rc4Crypt(SSLContext *ctx, void *buf, u32 len);
static u32 SSLi_CheckRecordMac(SSLContext *ctx, u8 *hdr);
static u32 SSLi_ComputeMac(SSLContext *ctx, u8 *hdr);
static int SSLi_ReadExact(u8 *dest, u32 len, CPSSoc *soc);
static void SSLi_UpdateHandshakeHash(SSLContext *ctx, const u8 *data, u32 len);
static void SSLi_ProcessRecord(SSLContext *ctx, u8 *record);
static int SSLi_ReceiveRecord(CPSSoc *soc);
static void SSLi_GetRandomBytes(u8 *buf, int len);
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
        if (entry->valid && entry->ip == 0 && entry->port == 0 &&
            memcmp(entry, ctx->key_storage.fields.session_id, 0x20) == 0) {
            sys_memcpy(entry->secret, ctx->secret, 0x30);
            ctx->resumed = TRUE;
            break;
        }
    }
    CPU_SetIRQMask(enabled);
}

// Looks a cached session up by its peer, offering its session ID and secret for the next ClientHello.
static void SSLi_FindSessionByPeer(SSLContext *ctx, CPSInAddr ip, u16 port) {
    u32 enabled = CPU_IRQDisable();
    int i;

    ctx->resumed = FALSE;
    for (i = 0; i < 4; i++) {
        SSLSessionCacheEntry *entry = &sSessionCache[i];
        if (entry->valid && entry->ip == ip && entry->port == port) {
            sys_memcpy(entry->session_id, ctx->key_storage.fields.session_id, 0x20);
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

    sys_memcpy(ctx->key_storage.fields.session_id, target->session_id, 0x20);
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
        if (entry->valid && memcmp(entry, ctx->key_storage.fields.session_id, 0x20) == 0) {
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

// Configures the priority applied unconditionally during selected crypto operations; values >= 32 disable it.
void CPS_SslSetMaxPriority(u32 priority) {
    sConfig.max_priority = priority;
}

// Applies the configured priority without comparing it to the thread's current priority, returning the old value.
// An invalid configured value (>= 32) returns UINT32_MAX without calling the thread APIs.
static u32 SSLi_ApplyConfiguredPriority(void) {
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

// Appends a certificate name value, separating existing values with ", ". The destination has room for 255 bytes
// excluding its terminator. byteCount controls copying; a NUL in value does not stop it.
static void SSLi_AppendCertificateNameValue(char *dest, const char *value, int byteCount) {
    char *p = dest;

    if (*p != '\0') {
        while (*++p != '\0') {
        }
        // BUG: At length 254, the delimiter leaves the terminator one byte beyond the 256-byte destination.
#ifdef BUGFIX
        if (p - dest >= 0xfe) {
#else
        if (p - dest >= 0xff) {
#endif
            return;
        }
        *p++ = ',';
        *p++ = ' ';
    }
    while (byteCount-- != 0 && p - dest < 0xff) {
        *p++ = *value++;
    }
    *p = '\0';
}

// Packs the calendar date as year:16, month:8, day:8. Time of day is ignored.
static u32 SSLi_ParseCertTime(const u8 *digits, int tag) {
    u32 first = digits[0] * 10 + digits[1] - '0' * 11;
    u32 year, month, day;
    digits += 2;
    if (tag == 23) {
        year = first + (first < 50 ? 2000 : 1900);
    } else {
        year = first * 100 + (digits[0] * 10 + digits[1] - '0' * 11);
        digits += 2;
    }
    month = digits[0] * 10 + digits[1] - '0' * 11;
    day = digits[2] * 10 + digits[3] - '0' * 11;
    return (year << 16) + (month << 8) + day;
}

// mode 0 retains the leaf key, mode 1 walks subsequent certificates, and mode 2 extracts
// a following certificate's public key by reference for the preceding signature check.
static int SSLi_ParseAsn1Field(SSLContext *ctx, const u8 **cursor, int depth, int index, int mode) {
    const u8 *value = *cursor + 1;
    int tag = **cursor;
    s32 len = SSLi_DecodeLength(&value);
    const u8 *end;
    int i;
    if (len < 0 || len > 2000) {
        return 1;
    }
    switch (tag & 31) {
    case 2:
        if (ctx->parsing_rsa_key) {
            if (index == 0 || index == 1) {
                while (*value == 0) {
                    value++;
                    len--;
                }
                if (index == 0) {
                    if (mode == 0 && len <= 0x100) {
                        sys_memcpy(value, ctx->certificate_key.modulus, len);
                        ctx->certificate_key.modulus_len = len;
                    } else if (mode == 2) {
                        ctx->chain_key.modulus_len = len;
                        ctx->chain_key.modulus = value;
                    }
                } else {
                    if (mode == 0 && len <= 8) {
                        sys_memcpy(value, ctx->certificate_key.exponent, len);
                        ctx->certificate_key.exponent_len = len;
                    } else if (mode == 2) {
                        ctx->chain_key.exponent_len = len;
                        ctx->chain_key.exponent = value;
                    }
                }
            }
        }
        break;
    case 3:
        if (depth == 1 && mode != 2) {
            ctx->certificate_signature = value + 1;
            ctx->certificate_signature_len = len - 1;
        }
        if (ctx->parsing_rsa_key) {
            value++;
            if (SSLi_ParseAsn1Field(ctx, &value, depth, 0, mode)) {
                return 1;
            }
            ctx->parsing_rsa_key = 0;
            *cursor = value;
            return 0;
        }
        break;
    case 6:
        for (i = 0; i < 6; i++) {
            if (memcmp(value, sOids[i], strlen((const char *)sOids[i])) == 0) {
                switch (i) {
                case 1:
                case 2:
                    if (mode == 0)
                        ctx->public_key_algorithm = i;
                    ctx->parsing_rsa_key = i;
                    break;
                case 3:
                case 4:
                    if (mode != 2)
                        ctx->signature_algorithm = i;
                    break;
                case 5:
                    if (mode != 2)
                        ctx->attribute_oid = i;
                    break;
                }
                break;
            }
        }
        break;
    case 12:
    case 19:
    case 20:
    case 22:
        if (mode != 2) {
            if (ctx->certificate_phase.parsing_subject) {
                SSLi_AppendCertificateNameValue(ctx->subject, (const char *)value, len);
                if (ctx->attribute_oid == 5 && len <= 0x4f) {
                    sys_memcpy(value, ctx->common_name, len);
                    ctx->common_name[len] = 0;
                }
            } else {
                SSLi_AppendCertificateNameValue(ctx->issuer, (const char *)value, len);
            }
        }
        ctx->attribute_oid = 0;
        break;
    case 23:
    case 24:
        if (mode != 2) {
            u32 date = SSLi_ParseCertTime(value, tag & 31);
            if (index == 0) {
                if (ctx->current_date >= date)
                    ctx->certificate_date_valid = 1;
            } else {
                if (ctx->current_date > date)
                    ctx->certificate_date_valid = 0;
            }
        }
        value += len;
        ctx->certificate_phase.parsing_subject = 1;
        *cursor = value;
        return 0;
    case 16:
        if (depth == 0 && index == 0 && mode != 2)
            ctx->signed_start = value;
        end = value + len;
        for (i = 0; value < end; i++) {
            if (SSLi_ParseAsn1Field(ctx, &value, depth + 1, i, mode))
                return 1;
        }
        if (depth == 1 && index == 0 && mode != 2)
            ctx->signed_end = value;
        *cursor = value;
        return 0;
    case 17:
        end = value + len;
        while (value < end) {
            if (SSLi_ParseAsn1Field(ctx, &value, depth + 1, 0, mode))
                return 1;
        }
        *cursor = value;
        return 0;
    default:
        if (tag == 0xa0) {
            end = value + len;
            while (value < end) {
                if (SSLi_ParseAsn1Field(ctx, &value, depth + 1, 0, mode))
                    return 1;
            }
            *cursor = value;
            return 0;
        }
        break;
    }
    *cursor = value + len;
    return 0;
}

// Checks the parsed certificate's signature against its signed DER digest, PKCS#1 v1.5 style:
// decrypted = signature^e mod n, then 0x00 0x01 0xff...
// 0x00 <digest>
static int SSLi_VerifyCertificateSignature(SSLContext *ctx, const SSLRsaPublicKey *key) {
    int n;
    u16 *digits;
    u16 *sigDigits, *expDigits, *modDigits;
    u8 *decrypted;
    u32 priority;
    int status = 0;

    if (ctx->certificate_signature == NULL || ctx->certificate_signature_len == 0 || key->exponent == NULL ||
        key->exponent_len == 0 || key->modulus == NULL || key->modulus_len == 0) {
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

    BigInt_BytesToDigits(sigDigits, ctx->certificate_signature, ctx->certificate_signature_len, n);
    BigInt_BytesToDigits(expDigits, key->exponent, key->exponent_len, n);
    BigInt_BytesToDigits(modDigits, key->modulus, key->modulus_len, n);
    priority = SSLi_ApplyConfiguredPriority();
    BigInt_ModExp(digits, sigDigits, expDigits, n, modDigits);
    SSLi_RestorePriority(priority);
    BigInt_DigitsToBytes(decrypted, digits, key->modulus_len, n);

    if (decrypted[0] == 0 && decrypted[1] == 1) {
        int pos = 2;
        while (pos < key->modulus_len && decrypted[pos] == 0xff) {
            pos++;
        }
        if (pos + 1 >= key->modulus_len || decrypted[pos] != 0 || decrypted[pos + 1] != 0x30 ||
            memcmp(ctx->certificate_digest, decrypted + key->modulus_len - ctx->certificate_digest_len,
                   ctx->certificate_digest_len) != 0) {
            status = 2;
        }
    } else {
        status = 2;
    }
    CPSiFree(digits);
    return status;
}

// Hashes the certificate's signed DER span and checks its signature using the registered issuer key.
// The handshake snapshot contexts serve as temporary digest storage here; this does not restore a session.
static int SSLi_CheckCertificate(SSLContext *ctx) {
    int flags = ctx->certificate_date_valid ? 0 : SSLI_CERT_DATE_INVALID;
    const SSLNamedParam *param;

    if (ctx->public_key_algorithm == -1) {
        return 4 | flags;
    }
    {
        s32 type = ctx->signature_algorithm;
        const u8 *start = ctx->signed_start;
        const u8 *end = ctx->signed_end;

        if (type == 3) {
            MD5Init(&ctx->handshake_md5_snapshot);
            MD5Update(&ctx->handshake_md5_snapshot, start, end - start);
            MD5Final(&ctx->handshake_md5_snapshot, ctx->certificate_digest);
            ctx->certificate_digest_len = 0x10;
        } else if (type == 4) {
            SHA1Init(&ctx->handshake_sha1_snapshot);
            SHA1Update(&ctx->handshake_sha1_snapshot, start, end - start);
            SHA1Final(&ctx->handshake_sha1_snapshot, ctx->certificate_digest);
            ctx->certificate_digest_len = 0x14;
        } else {
            return 3 | flags;
        }
    }

    param = SSLi_FindNamedParam(ctx, ctx->issuer);
    if (param == NULL) {
        return 1 | flags;
    }
    return SSLi_VerifyCertificateSignature(ctx, (const SSLRsaPublicKey *)param) | flags;
}

// How many bytes until the next '.' or the end of the string
static int SSLi_LabelLength(const char *host) {
    int i = 0;
    while (host[i] != '.' && host[i] != '\0') {
        i++;
    }
    return i;
}

// Compares the requested hostname with the certificate pattern (the second
// argument may contain a wildcard). Like strcmp, zero means a match.
static int SSLi_CompareHostname(const char *host, const char *pattern) {
    for (;;) {
        int p = *pattern++;
        int h = *host++;
        if (h == p) {
            if (h == 0)
                return 0;
        } else {
            int hostLen, patternLen;
            if (p != '*')
                return 1;
            host--;
            hostLen = SSLi_LabelLength(host);
            patternLen = SSLi_LabelLength(pattern);
            if (patternLen > hostLen)
                return 1;
            host += hostLen - patternLen;
        }
    }
}

// Processes the Certificate message's BE24-length-prefixed chain. The callback can
// override validation; continuation depends on the original low-byte result as well.
static void SSLi_ParseCertificateChain(SSLContext *ctx, const u8 *body) {
    int remaining = (body[0] << 16) + (body[1] << 8) + body[2];
    int index = 0;
    int mode = 0;
    RTCDate date;
    body += 3;
    ctx->public_key_algorithm = -1;
    func_0207cc10(&date);
    ctx->current_date = ((date.year + 2000) << 16) + (date.month << 8) + date.day;
    ctx->subject[0] = 0;
    ctx->certificate_key.exponent_len = 0;
    ctx->certificate_key.modulus_len = 0;
    for (;;) {
        int len = (body[0] << 16) + (body[1] << 8) + body[2];
        int result, original;
        body += 3;
        remaining -= len + 3;
        ctx->signature_algorithm = -1;
        ctx->parsing_rsa_key = 0;
        ctx->certificate_phase.parsing_subject = 0;
        ctx->certificate_date_valid = 0;
        ctx->subject[0] = 0;
        ctx->issuer[0] = 0;
        ctx->common_name[0] = 0;
        ctx->certificate = body;
        ctx->certificate_len = len;
        if (SSLi_ParseAsn1Field(ctx, &body, 0, 0, mode) || (u32)ctx->certificate_key.modulus_len < 0x33 ||
            ctx->certificate_key.exponent_len == 0) {
            ctx->state = SSLI_STATE_ERROR;
            return;
        }
        result = SSLi_CheckCertificate(ctx);
        if (index == 0 && ctx->hostname && SSLi_CompareHostname(ctx->hostname, ctx->common_name)) {
            result |= SSLI_CERT_HOSTNAME_MISMATCH;
        }
        original = result & SSLI_CERT_RESULT_MASK;
        if (original == 1 && remaining != 0) {
            const u8 *next = body + 3;
            ctx->parsing_rsa_key = 0;
            if (SSLi_ParseAsn1Field(ctx, &next, 0, 0, 2)) {
                ctx->state = SSLI_STATE_ERROR;
                return;
            }
            result = (result & ~SSLI_CERT_RESULT_MASK) | SSLi_VerifyCertificateSignature(ctx, &ctx->chain_key);
        }
        if (ctx->validate_certificate)
            result = ctx->validate_certificate(result, ctx, index);
        index++;
        if (original != 0 && result == 0 && remaining != 0) {
            mode = 1;
            continue;
        }
        ctx->state = result == 0 ? SSLI_STATE_CERTIFICATE_ACCEPTED : SSLI_STATE_ERROR;
        return;
    }
}

// Handles the ServerHello received by the client: peer random, selected session ID and cipher suite.
static void SSLi_ParseServerHello(SSLContext *ctx, u8 *msg) {
    int sidLen;
    BOOL wasResumed;
    BOOL resumed;
    u8 *cipher;

    sys_memcpy(msg + 2, ctx->server_random, 0x20);
    sidLen = msg[0x22];
    wasResumed = ctx->resumed;
    msg += 0x23;
    if (wasResumed && sidLen == 0x20 && memcmp(ctx->key_storage.fields.session_id, msg, 0x20) == 0) {
        ctx->unk_031 = 1;
    } else {
        if (wasResumed)
            SSLi_InvalidateSession(ctx);
        if (sidLen == 0) {
            resumed = FALSE;
        } else {
            sys_memcpy(msg, ctx->key_storage.fields.session_id, 0x20);
            resumed = TRUE;
        }
        ctx->resumed = resumed;
        ctx->unk_031 = 0;
    }
    cipher = msg + sidLen;
    ctx->cipher_suite = (cipher[0] << 8) + cipher[1];
    if (ctx->cipher_suite != 4 && ctx->cipher_suite != 5) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    ctx->state = SSLI_STATE_SERVER_HELLO_PARSED;
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
static BOOL SSLi_IsSsl3Version(int major, int minor) {
    (void)minor;
    return major == 3;
}

// Handles the SSLv2-compatible ClientHello: three-byte cipher entries, a session ID and a challenge
// copied into the client-random storage. The normal SSLv3 ClientHello uses SSLi_ParseClientHello.
static void SSLi_ParseSsl2ClientHello(SSLContext *ctx, u8 *msg) {
    u16 cipher;
    int cipherLen, sessionLen, challengeLen, offset;

    if (!SSLi_IsSsl3Version(msg[0], msg[1]))
        return;
    cipherLen = (msg[2] << 8) + msg[3];
    cipher = SSLi_PickCipherSuite(msg + 8, cipherLen / 3, 3);
    if (cipher == 0)
        return;
    ctx->cipher_suite = cipher;
    sessionLen = (msg[4] << 8) + msg[5];
    challengeLen = (msg[6] << 8) + msg[7];
    ctx->resumed = FALSE;
    offset = cipherLen + 8 + sessionLen;
    if (challengeLen >= 0x20) {
        sys_memcpy(msg + offset, ctx->client_random, 0x20);
    } else {
        sys_memset(ctx->client_random, 0, 0x20 - challengeLen);
        sys_memcpy(msg + offset, ctx->client_random + 0x20 - challengeLen, challengeLen);
    }
    ctx->state = SSLI_STATE_CLIENT_HELLO_PARSED;
}

// Handles the ClientHello received by the server: client random, offered session ID and two-byte cipher entries.
// The offered session ID can restore a cached session.
static void SSLi_ParseClientHello(SSLContext *ctx, u8 *msg) {
    int sidLen;
    u16 cipher;
    u8 *cipherList;

    if (!SSLi_IsSsl3Version(msg[0], msg[1])) {
        return;
    }
    sys_memcpy(msg + 2, ctx->client_random, 0x20);

    sidLen = msg[0x22];
    msg += 0x23;
    if (sidLen != 0x20) {
        ctx->resumed = FALSE;
    } else {
        sys_memcpy(msg, ctx->key_storage.fields.session_id, 0x20);
        SSLi_RestoreSessionById(ctx);
    }

    cipherList = msg + sidLen;
    cipher = SSLi_PickCipherSuite(cipherList + 2, ((cipherList[0] << 8) + cipherList[1]) / 2, 2);
    ctx->cipher_suite = cipher;
    if (cipher == 0) {
        return;
    }
    ctx->state = SSLI_STATE_CLIENT_HELLO_PARSED;
}

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
    priority = SSLi_ApplyConfiguredPriority();
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

// Generates SSLv3 key material in embedded storage after session-cache use is finished.
// Suite 4 needs four MD5 blocks; suite 5 needs five, with eight unused tail bytes.
// MAC/key pointers borrow the needed bytes until the connection is reset. Assign the
// overlapping write-pointer slots only after generation, then initialize both RC4 contexts.
static void SSLi_GenerateKeyBlock(SSLContext *ctx) {
    // Handshake cipher selection guarantees suite 4 or 5 here.
    int macLen = ctx->cipher_suite == 4 ? 0x10 : 0x14;
    const int keyLen = 0x10;
    int total = (macLen + keyLen) * 2;
    u8 *block = ctx->key_storage.key_block;
    int round;

    for (round = 0; round * 0x10 < total; round++) {
        u8 seed;
        u8 digest[0x14];
        int i;

        SHA1Init(&ctx->handshake_sha1_snapshot);
        seed = 0x41 + round;
        for (i = 0; i <= round; i++) {
            SHA1Update(&ctx->handshake_sha1_snapshot, &seed, 1);
        }
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->secret, 0x30);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->server_random, 0x20);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->client_random, 0x20);
        SHA1Final(&ctx->handshake_sha1_snapshot, digest);

        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->secret, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, digest, 0x14);
        MD5Final(&ctx->handshake_md5_snapshot, block + round * 0x10);
    }

    if (ctx->is_server) {
        ctx->read_mac_secret = block;
        ctx->read_key = block + macLen * 2;
        ctx->key_storage.fields.write_mac_secret = block + macLen;
        ctx->key_storage.fields.write_key = block + macLen * 2 + keyLen;
    } else {
        ctx->key_storage.fields.write_mac_secret = block;
        ctx->key_storage.fields.write_key = block + macLen * 2;
        ctx->read_mac_secret = block + macLen;
        ctx->read_key = block + macLen * 2 + keyLen;
    }

    CPSi_rc4_init(&ctx->read_rc4, ctx->read_key, 0x10);
    CPSi_rc4_init(&ctx->write_rc4, ctx->key_storage.fields.write_key, 0x10);
}

// Finishes a ClientKeyExchange that carried an RSA-encrypted pre-master secret with this connection's own key
// (for the quick-resume ticket's key exchange; see SSLi_DecryptPreMasterSecret), derives the master secret and
// key block from it, remembers the session and sets this side's role for the key block's halves
static void SSLi_FinishKeyExchange(SSLContext *ctx, u8 *msg) {
    SSLi_DecryptPreMasterSecret(ctx, msg, ctx->key_param);
    SSLi_GenerateMasterSecret(ctx);
    SSLi_SaveSession(ctx, 0, 0);
    SSLi_GenerateKeyBlock(ctx);
    ctx->state = SSLI_STATE_MASTER_SECRET_READY;
}

// Computes the MD5 half of Finished using the sender label and master secret.
// This destructively finalizes the running transcript; callers save and restore it.
static void SSLi_ComputeFinishedMd5(SSLContext *ctx, u8 *out, int sender) {
    const u8 *label = (ctx->is_server == sender) ? sLabelClient : sLabelServer;
    u8 pad[0x30];

    // The caller restores the transcript after this destructive computation.
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

// SHA1 half of Finished; also destroys the running transcript until the caller restores it.
static void SSLi_ComputeFinishedSha1(SSLContext *ctx, u8 *out, int sender) {
    const u8 *label = (ctx->is_server == sender) ? sLabelClient : sLabelServer;
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

// Checks the received Finished with the original fixed sender selector 1, restoring the running
// handshake hash after each half so processing can subsequently include this message.
static void SSLi_VerifyFinished(SSLContext *ctx, u8 *msg) {
    u8 computed[0x14];

    ctx->handshake_md5_snapshot = ctx->handshake_md5;
    SSLi_ComputeFinishedMd5(ctx, computed, 1);
    ctx->handshake_md5 = ctx->handshake_md5_snapshot;
    if (memcmp(msg, computed, 0x10) != 0) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }

    ctx->handshake_sha1_snapshot = ctx->handshake_sha1;
    SSLi_ComputeFinishedSha1(ctx, computed, 1);
    ctx->handshake_sha1 = ctx->handshake_sha1_snapshot;
    if (memcmp(msg + 0x10, computed, 0x14) != 0) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    ctx->state = SSLI_STATE_FINISHED_VERIFIED;
}

// counterEnd is one past an 8-byte big-endian counter; carry is bounded to those eight bytes.
static void SSLi_IncrementCounter(u8 *counterEnd) {
    int remaining = 8;

    do {
        --counterEnd;
        if (++*counterEnd != 0) {
            break;
        }
    } while (--remaining != 0);
}

// Decrypts received bytes with the read RC4 state and returns len unchanged.
static u32 SSLi_Rc4Crypt(SSLContext *ctx, void *buf, u32 len) {
    CPSi_rc4_crypt(&ctx->read_rc4, buf, len);
    return len;
}

// RC4-decrypts a received record and checks its MAC, the mirror of SSLi_ComputeMac: hdr's length still covers the
// MAC, which this strips off after recomputing it over the shorter content and comparing. Sets ctx->state to an
// error on a mismatch either way, so the caller doesn't need to check this call's return for that
static u32 SSLi_CheckRecordMac(SSLContext *ctx, u8 *hdr) {
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
    SSLi_IncrementCounter(ctx->read_seq_num + sizeof(ctx->read_seq_num));
    return len + 5;
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
        MD5Update(&ctx->handshake_md5_snapshot, ctx->key_storage.fields.write_mac_secret, 0x10);
        sys_memset(pad, 0x36, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, pad, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->write_seq_num, 8);
        MD5Update(&ctx->handshake_md5_snapshot, hdr, 1);
        MD5Update(&ctx->handshake_md5_snapshot, hdr + 3, len + 2);
        MD5Final(&ctx->handshake_md5_snapshot, content + len);

        MD5Init(&ctx->handshake_md5_snapshot);
        MD5Update(&ctx->handshake_md5_snapshot, ctx->key_storage.fields.write_mac_secret, 0x10);
        sys_memset(pad, 0x5c, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, pad, 0x30);
        MD5Update(&ctx->handshake_md5_snapshot, content + len, 0x10);
        MD5Final(&ctx->handshake_md5_snapshot, content + len);
        len += 0x10;
    } else if (ctx->cipher_suite == 5) {
        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->key_storage.fields.write_mac_secret, 0x14);
        sys_memset(pad, 0x36, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, pad, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->write_seq_num, 8);
        SHA1Update(&ctx->handshake_sha1_snapshot, hdr, 1);
        SHA1Update(&ctx->handshake_sha1_snapshot, hdr + 3, len + 2);
        SHA1Final(&ctx->handshake_sha1_snapshot, content + len);

        SHA1Init(&ctx->handshake_sha1_snapshot);
        SHA1Update(&ctx->handshake_sha1_snapshot, ctx->key_storage.fields.write_mac_secret, 0x14);
        sys_memset(pad, 0x5c, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, pad, 0x28);
        SHA1Update(&ctx->handshake_sha1_snapshot, content + len, 0x14);
        SHA1Final(&ctx->handshake_sha1_snapshot, content + len);
        len += 0x14;
    }

    hdr[3] = (u8)(len >> 8);
    hdr[4] = (u8)len;
    CPSi_rc4_crypt(&ctx->write_rc4, content, len);
    SSLi_IncrementCounter(ctx->write_seq_num + sizeof(ctx->write_seq_num));
    return len + 5;
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

// Takes ownership of a complete record. Application data retains the allocation in
// read_buf; every other path frees it after processing its contents.
static void SSLi_ProcessRecord(SSLContext *ctx, u8 *record) {
    int type;
    u32 remaining;
    u8 *body;
    if (ctx->state == SSLI_STATE_ERROR) {
        CPSiFree(record);
        return;
    }
    type = record[0];
    remaining = (record[3] << 8) + record[4] + 5;
    if (ctx->read_mac_secret != NULL &&
        (((ctx->state == SSLI_STATE_EXPECT_FINISHED || ctx->state == SSLI_STATE_APPLICATION_DATA) && type != 0x15) ||
         (type == 0x15 && remaining > 7))) {
        remaining = SSLi_CheckRecordMac(ctx, record);
        if (ctx->state == SSLI_STATE_ERROR) {
            CPSiFree(record);
            return;
        }
    }
    body = record + 5;
    remaining -= 5;
    switch (type) {
    case 0x14:
        if (ctx->read_mac_secret != NULL) {
            sys_memset(ctx->read_seq_num, 0, 8);
            ctx->state = SSLI_STATE_EXPECT_FINISHED;
        } else {
            ctx->state = SSLI_STATE_ERROR;
        }
        break;
    case 0x15:
        if (remaining != 2 || body[0] == 2)
            ctx->state = SSLI_STATE_ERROR;
        break;
    case 0x16:
        if (remaining < 4) {
            ctx->state = SSLI_STATE_ERROR;
            break;
        }
        // BUG: The original compares a body length against bytes including its four-byte header,
        // then subtracts both. Later iterations also read a header without checking that four bytes remain.
        do {
#ifdef BUGFIX
            if (remaining < 4) {
                ctx->state = SSLI_STATE_ERROR;
                break;
            }
#endif
            int messageType = body[0];
            int len = (body[1] << 16) + (body[2] << 8) + body[3];
#ifdef BUGFIX
            if (len > remaining - 4) {
#else
            if (len > remaining) {
#endif
                ctx->state = SSLI_STATE_ERROR;
                break;
            }
            body += 4;
            switch (messageType) {
            case 1:
                if (ctx->is_server && ctx->state == SSLI_STATE_INITIAL)
                    SSLi_ParseClientHello(ctx, body);
                else
                    ctx->state = SSLI_STATE_ERROR;
                break;
            case 2:
                if (len >= 0x26 && !ctx->is_server)
                    SSLi_ParseServerHello(ctx, body);
                else
                    ctx->state = SSLI_STATE_ERROR;
                break;
            case 11:
                if (ctx->state == SSLI_STATE_SERVER_HELLO_PARSED && !ctx->is_server) {
                    SSLi_ParseCertificateChain(ctx, body);
                    ctx->certificate_phase.certificate_requested = 0;
                } else
                    ctx->state = SSLI_STATE_ERROR;
                break;
            case 13:
                if (ctx->state == SSLI_STATE_CERTIFICATE_ACCEPTED && !ctx->is_server)
                    ctx->certificate_phase.certificate_requested = 1;
                else
                    ctx->state = SSLI_STATE_ERROR;
                break;
            case 14:
                if (!ctx->is_server && ctx->state == SSLI_STATE_CERTIFICATE_ACCEPTED)
                    ctx->state = SSLI_STATE_SERVER_HELLO_DONE;
                else
                    ctx->state = SSLI_STATE_ERROR;
                break;
            case 16:
                if (ctx->is_server)
                    SSLi_FinishKeyExchange(ctx, body);
                else
                    ctx->state = SSLI_STATE_ERROR;
                break;
            case 20:
                SSLi_VerifyFinished(ctx, body);
                break;
            default:
                ctx->state = SSLI_STATE_ERROR;
                break;
            }
            if (ctx->state != SSLI_STATE_ERROR) {
                SSLi_UpdateHandshakeHash(ctx, body - 4, len + 4);
                body += len;
                remaining -= len + 4;
            }
        } while (remaining != 0 && ctx->state != SSLI_STATE_ERROR);
        break;
    case 0x17:
        if (ctx->state == SSLI_STATE_APPLICATION_DATA) {
            ctx->read_buf = record;
            ctx->read_buf_pos = 5;
            ctx->read_buf_len = remaining + 5;
            ctx->application_record_ready = 1;
            return;
        }
        ctx->state = SSLI_STATE_ERROR;
        break;
    default:
        ctx->state = SSLI_STATE_ERROR;
        break;
    }
    CPSiFree(record);
}

// Peeks the framing before allocating. The SSLv2-compatible ClientHello form has
// a two-byte header; standard records retain their five-byte header in the buffer.
static int SSLi_ReceiveRecord(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u32 len;
    u8 *peek, *record;
    for (;;) {
        peek = CPSi_TcpRead(&len, soc);
        if (len >= 5)
            break;
        if (len == 0 || soc->state == 0) {
            ctx->state = SSLI_STATE_ERROR;
            return SSLI_STATE_ERROR;
        }
    }
    if (peek[0] == 0x80) {
        if (!ctx->is_server || ctx->state != 0) {
            ctx->state = SSLI_STATE_ERROR;
            return ctx->state;
        }
        len = peek[1];
        // BUG: With a non-NULL zero-byte allocation, the original can read record[0] without a record body.
#ifdef BUGFIX
        if (len == 0) {
            ctx->state = SSLI_STATE_ERROR;
            return SSLI_STATE_ERROR;
        }
#endif
        CPSi_TcpConsume(2, soc);
        record = (u8 *)CPSiAlloc(len);
        if (record == NULL) {
            ctx->state = SSLI_STATE_ERROR;
            return SSLI_STATE_ERROR;
        }
        if (SSLi_ReadExact(record, len, soc) == 0 && record[0] == 1) {
            SSLi_ParseSsl2ClientHello(ctx, record + 1);
        } else {
            ctx->state = SSLI_STATE_ERROR;
        }
        SSLi_UpdateHandshakeHash(ctx, record, len);
        CPSiFree(record);
    } else {
        len = (peek[3] << 8) + peek[4] + 5;
        if (len > 0x4805) {
            ctx->state = SSLI_STATE_ERROR;
            return SSLI_STATE_ERROR;
        }
        record = (u8 *)CPSiAlloc(len);
        if (record == NULL) {
            ctx->state = SSLI_STATE_ERROR;
            return SSLI_STATE_ERROR;
        }
        if (SSLi_ReadExact(record, len, soc) != 0) {
            CPSiFree(record);
            ctx->state = SSLI_STATE_ERROR;
            return SSLI_STATE_ERROR;
        }
        SSLi_ProcessRecord(ctx, record);
    }
    return ctx->state;
}

// Draws len bytes from sPool, a running SHA1 digest CPS_SslAddRandomSeed stirs seed into, reseeding first from
// CPSiRand (the stack's own generator) if nothing ever has. sPool is updated (a carry-add of a fresh SHA1 digest of
// itself) a 20-byte block at a time under the IRQ mask. Only nonzero bytes are returned.
static void SSLi_GetRandomBytes(u8 *dest, int len) {
    int produced = 0;
    int pos = 0x14;
    u8 digest[0x14];

    if (!sPoolSeeded) {
        u32 seed = MATH_Rand32(&CPSiRand, 0);
        CPS_SslAddRandomSeed(&seed, 4);
    }

    while (produced < len) {
        if (pos == 0x14) {
            SHA1Context sha1;
            int i;
            u32 carry;
            u32 enabled;

            SHA1Init(&sha1);
            enabled = CPU_IRQDisable();
            SHA1Update(&sha1, sPool, 0x14);
            SHA1FinalNoLength(&sha1, digest);

            carry = 1;
            for (i = 0x13; i >= 0; i--) {
                u32 sum = sPool[i] + digest[i] + carry;
                sPool[i] = (u8)sum;
                carry = sum >> 8;
            }
            CPU_SetIRQMask(enabled);
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

// Sends ChangeCipherSpec (switching this side to the keys SSLi_GenerateKeyBlock made) and Finished (the two
// Finished MAC halves, encrypted and MACed with the new write keys since ChangeCipherSpec already took effect)
static void SSLi_SendChangeCipherSpecAndFinished(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u8 *packet = (u8 *)CPSiAlloc(0x83);
    u32 len;
    if (packet == NULL) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    packet[0] = 0x14;
    packet[1] = 3;
    packet[2] = 0;
    packet[3] = 0;
    packet[4] = 1;
    packet[5] = 1;
    sys_memset(ctx->write_seq_num, 0, 8);
    packet[6] = 0x16;
    packet[7] = 3;
    packet[8] = 0;
    packet[9] = 0;
    packet[10] = 0x28;
    packet[11] = 0x14;
    packet[12] = 0;
    packet[13] = 0;
    packet[14] = 0x24;
    sys_memcpy(&ctx->handshake_md5, &ctx->handshake_md5_snapshot, sizeof(ctx->handshake_md5));
    SSLi_ComputeFinishedMd5(ctx, packet + 15, 0);
    sys_memcpy(&ctx->handshake_md5_snapshot, &ctx->handshake_md5, sizeof(ctx->handshake_md5));
    sys_memcpy(&ctx->handshake_sha1, &ctx->handshake_sha1_snapshot, sizeof(ctx->handshake_sha1));
    SSLi_ComputeFinishedSha1(ctx, packet + 31, 0);
    sys_memcpy(&ctx->handshake_sha1_snapshot, &ctx->handshake_sha1, sizeof(ctx->handshake_sha1));
    SSLi_UpdateHandshakeHash(ctx, packet + 11, 0x28);
    len = SSLi_ComputeMac(ctx, packet + 6);
    CPSi_TcpWrite(packet, len + 6, NULL, 0, soc);
    CPSiFree(packet);
}

// Sends ClientHello: version, a fresh client random (a Unix-ish timestamp, then 28 more random bytes), the cached
// session id for this peer if there is one, and the 2 cipher suites this connection offers
static void SSLi_SendClientHello(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u8 *packet = (u8 *)CPSiAlloc(0x98);
    u8 *p;
    u32 time;
    int len, i;
    if (packet == NULL) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    p = packet + 9;
    p[0] = 3;
    p[1] = 0;
    time = SSLi_GetUnixTime();
    ctx->client_random[0] = time >> 24;
    ctx->client_random[1] = time >> 16;
    ctx->client_random[2] = time >> 8;
    ctx->client_random[3] = time;
    SSLi_GetRandomBytes(ctx->client_random + 4, 0x1c);
    sys_memcpy(ctx->client_random, p + 2, 0x20);
    SSLi_FindSessionByPeer(ctx, soc->remote_ip, soc->remote_port);
    if (ctx->resumed) {
        p[0x22] = 0x20;
        sys_memcpy(ctx->key_storage.fields.session_id, p + 0x23, 0x20);
        p += 0x43;
    } else {
        p[0x22] = 0;
        p += 0x23;
    }
    p[0] = 0;
    p[1] = 4;
    p += 2;
    for (i = 0; i < 2; i++) {
        p[0] = sConfig.cipher_suites[i] >> 8;
        p[1] = sConfig.cipher_suites[i];
        p += 2;
    }
    p[0] = 1;
    p[1] = 0;
    len = p + 2 - packet - 5;
    packet[0] = 0x16;
    packet[1] = 3;
    packet[2] = 0;
    packet[3] = len >> 8;
    packet[4] = len;
    packet[5] = 1;
    packet[6] = (len - 4) >> 16;
    packet[7] = (len - 4) >> 8;
    packet[8] = len - 4;
    CPSi_TcpWrite(packet, len + 5, NULL, 0, soc);
    SSLi_UpdateHandshakeHash(ctx, packet + 5, len);
    CPSiFree(packet);
}

// RSA-encrypts the pre-master secret using the inline key retained from the leaf
// certificate, then sends and hashes the complete ClientKeyExchange message.
static void SSLi_SendClientKeyExchange(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    int len, n, i;
    u8 *padded, *packet, *out;
    u16 *digits, *base, *exponent, *modulus;
    u32 priority;
    if (ctx->certificate_phase.certificate_requested)
        CPSi_TcpWrite((u8 *)sNoCertificateAlert, 7, NULL, 0, soc);
    ctx->secret[0] = 3;
    ctx->secret[1] = 0;
    SSLi_GetRandomBytes(ctx->secret + 2, 0x2e);
    len = ctx->certificate_key.modulus_len;
    n = (len * 2) / 2;
    padded = (u8 *)CPSiAlloc(len);
    if (padded == NULL) {
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    padded[0] = 0;
    padded[1] = 2;
    SSLi_GetRandomBytes(padded + 2, len - 0x33);
    padded[len - 0x31] = 0;
    sys_memcpy(ctx->secret, padded + len - 0x30, 0x30);
    digits = (u16 *)CPSiAlloc(n * 8);
    if (digits == NULL) {
        CPSiFree(padded);
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    base = digits + n;
    exponent = base + n;
    modulus = exponent + n;
    BigInt_BytesToDigits(base, padded, len, n);
    BigInt_BytesToDigits(exponent, ctx->certificate_key.exponent, ctx->certificate_key.exponent_len, n);
    BigInt_BytesToDigits(modulus, ctx->certificate_key.modulus, len, n);
    priority = SSLi_ApplyConfiguredPriority();
    BigInt_ModExp(digits, base, exponent, n, modulus);
    SSLi_RestorePriority(priority);
    packet = (u8 *)CPSiAlloc(len + 0x49);
    if (packet == NULL) {
        CPSiFree(padded);
        CPSiFree(digits);
        ctx->state = SSLI_STATE_ERROR;
        return;
    }
    packet[0] = 0x16;
    packet[1] = 3;
    packet[2] = 0;
    packet[3] = (len + 4) >> 8;
    packet[4] = len + 4;
    packet[5] = 0x10;
    packet[6] = len >> 16;
    packet[7] = len >> 8;
    packet[8] = len;
    out = packet + 9;
    if (len & 1)
        *out++ = digits[len / 2];
    for (i = len / 2 - 1; i >= 0; i--) {
        *out++ = digits[i] >> 8;
        *out++ = digits[i];
    }
    CPSi_TcpWrite(packet, len + 9, NULL, 0, soc);
    SSLi_UpdateHandshakeHash(ctx, packet + 5, len + 4);
    CPSiFree(packet);
    CPSiFree(digits);
    CPSiFree(padded);
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
// and Finished. State 8 allows application records after the exchange.
static BOOL SSLi_DoHandshake(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    int state;

    SSLi_SendClientHello(soc);
    do {
        state = SSLi_ReceiveRecord(soc);
        if (state == SSLI_STATE_ERROR)
            return TRUE;
    } while (state != SSLI_STATE_SERVER_HELLO_DONE && !ctx->unk_031);

    if (ctx->unk_031) {
        SSLi_GenerateKeyBlock(ctx);
        if (SSLi_WaitForChangeCipherSpecAndFinished(soc))
            return TRUE;
        SSLi_SendChangeCipherSpecAndFinished(soc);
    } else {
        SSLi_SendClientKeyExchange(soc);
        SSLi_GenerateMasterSecret(ctx);
        if (ctx->resumed)
            SSLi_SaveSession(ctx, soc->remote_ip, soc->remote_port);
        SSLi_GenerateKeyBlock(ctx);
        SSLi_SendChangeCipherSpecAndFinished(soc);
        if (SSLi_WaitForChangeCipherSpecAndFinished(soc))
            return TRUE;
    }
    ctx->state = SSLI_STATE_APPLICATION_DATA;
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

    ctx->state = SSLI_STATE_INITIAL;
    ctx->read_mac_secret = NULL;
    ctx->is_server = 0;
    SHA1Init(&ctx->handshake_sha1);
    MD5Init(&ctx->handshake_md5);
    return SSLi_DoHandshake(soc);
}

// CPS_SslRead's role: a pointer into the application data the handshake has buffered and not yet given out, and
// *len to how much of it there is; NULL (with *len 0) once the connection has nothing left and has closed
u8 *CPS_SslRead(u32 *len, CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->read_buf != NULL && !ctx->application_record_ready) {
        u8 *data = ctx->read_buf + ctx->read_buf_pos;
        u32 remaining = ctx->read_buf_len - ctx->read_buf_pos;

        if (SSLi_ReadExact(data, remaining, soc) != 0) {
            CPSiFree(ctx->read_buf);
            ctx->read_buf = NULL;
            *len = 0;
            return NULL;
        }
        SSLi_ProcessRecord(ctx, ctx->read_buf);
        if (!ctx->application_record_ready) {
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

// Copies currently available TCP bytes into an incomplete record without waiting
// for the rest. Once complete, dispatch transfers or frees the allocation.
static void SSLi_FillReadBuffer(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    u32 len, remaining;
    u8 *data;
    BOOL complete;
    if (ctx->read_buf == NULL) {
        if (soc->rcvbuf_len < 5)
            return;
        data = CPSi_TcpRead(&len, soc);
        len = (data[3] << 8) + data[4] + 5;
        if (len > 0x4805) {
            ctx->state = SSLI_STATE_ERROR;
            return;
        }
        ctx->read_buf = (u8 *)CPSiAlloc(len);
        if (ctx->read_buf == NULL) {
            ctx->state = SSLI_STATE_ERROR;
            return;
        }
        ctx->read_buf_len = len;
        ctx->read_buf_pos = 0;
        ctx->application_record_ready = 0;
    } else if (soc->rcvbuf_len == 0) {
        return;
    }
    data = CPSi_TcpRead(&len, soc);
    remaining = ctx->read_buf_len - ctx->read_buf_pos;
    if (len >= remaining) {
        len = remaining;
        complete = TRUE;
    } else {
        complete = FALSE;
    }
    sys_memcpy(data, ctx->read_buf + ctx->read_buf_pos, len);
    CPSi_TcpConsume(len, soc);
    if (complete) {
        SSLi_ProcessRecord(ctx, ctx->read_buf);
        if (!ctx->application_record_ready)
            ctx->read_buf = NULL;
    } else {
        ctx->read_buf_pos += len;
    }
}

// CPS_SslCanRead's role: how many bytes CPS_SslRead would give out right now, or -1 once the connection has
// closed with nothing left
s32 CPS_SslCanRead(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->read_buf == NULL || !ctx->application_record_ready) {
        SSLi_FillReadBuffer(soc);
    }
    if (ctx->read_buf != NULL && ctx->application_record_ready) {
        return (s32)(ctx->read_buf_len - ctx->read_buf_pos);
    }
    if (ctx->read_buf == NULL && soc->state == CPS_STT_ESTABLISHED && ctx->state == SSLI_STATE_ERROR) {
        return -1;
    }
    return 0;
}

// CPS_SslWrite's role: sends data as application data records, each up to 0xb4f bytes of content, MACed and
// RC4-crypted by SSLi_ComputeMac. data2/len2 is a second buffer CPS_SocWrite-style callers hand this to send right
// after the first, as one write; returns the total bytes sent, which may be less than len+len2 if CPSiAlloc or the
// connection itself runs out partway
u32 CPS_SslWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;
    int total = len + len2;
    u32 sent = 0;
    int chunk;
    do {
        u8 *buf;
        u32 first, second, sentNow;
        chunk = total > 0xb4f ? 0xb4f : total;
        buf = (u8 *)CPSiAlloc(chunk + 0x19);
        if (buf == NULL)
            break;
        first = chunk;
        if (len < (u32)chunk)
            first = len;
        second = chunk - first;
        sys_memcpy(data, buf + 5, first);
        data += first;
        len -= first;
        sys_memcpy(data2, buf + 5 + first, second);
        data2 += second;
        buf[0] = 0x17;
        buf[1] = 3;
        buf[2] = 0;
        buf[3] = chunk >> 8;
        buf[4] = chunk;
        sentNow = SSLi_ComputeMac(ctx, buf);
        if (CPSi_TcpWrite(buf, sentNow, NULL, 0, soc) < sentNow)
            chunk = 0;
        CPSiFree(buf);
        total -= chunk;
        sent += chunk;
    } while (total != 0 && chunk != 0);
    return sent;
}

// This file's signature, which CPS_SetSsl refers to purely to keep it linked in
static const char sSslSignature[] = "[SDK+UBIQUITOUS:SSL]";

// Sends close_notify for an established SSL session, then resets the handshake state.
void CPS_SslShutdown(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    if (ctx->state == SSLI_STATE_APPLICATION_DATA) {
        u8 msg[5 + 2 + 20];

        msg[0] = 0x15;
        msg[1] = 3;
        msg[2] = 0;
        msg[3] = 0;
        msg[4] = 2;
        msg[5] = 1;
        msg[6] = 0;
        CPSi_TcpWrite(msg, SSLi_ComputeMac(ctx, msg), NULL, 0, soc);
    }
    ctx->state = SSLI_STATE_INITIAL;
}

// CPS_SslClose's role: frees the connection's read buffer, if it has one, and resets its state
void CPS_SslClose(CPSSoc *soc) {
    SSLContext *ctx = (SSLContext *)soc->con;

    ctx->state = SSLI_STATE_INITIAL;
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
        if (soc != NULL && soc->thread != NULL && soc->ssl != 0 && soc->state == CPS_STT_ESTABLISHED &&
            ((SSLContext *)soc->con)->state < SSLI_STATE_APPLICATION_DATA && now - soc->when > 0xef &&
            soc->block_type == CPS_BLOCK_TCPREAD) {
            soc->state = 0;
            soc->block_type = CPS_BLOCK_NONE;
            scheduler_start_thread(soc->thread);
        }
    }
}

// CPS_SslInit's role: clears the session cache, once at CPS_Startup
void CPS_SslInit(void) {
    sys_memset(sSessionCache, 0, sizeof(sSessionCache));
}
