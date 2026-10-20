#include <string.h>
// Before dwc/ext.h: MWCC builds Auth_OnResponse's memset of the result in place, as the ROM does, only when
// <string.h> declares memset first
#include "dwc/auth.h"
#include "dwc/ext.h"

// GameSpy's authentication service, as overlay 260's Wi-Fi library uses it: it logs in with the token and the challenge
// that the library has and gets the player's certificate, and the competition service asks it to write the certificate
// and check its signature. The signature is of an MD5-like hash of the certificate's fields over the service's RSA
// public key, which is in the file below.

// What a login is waiting for: the function to call and its user data
typedef struct AuthRequest {
    AuthResultFunc func;
    void *userData;
} AuthRequest;

static BOOL Auth_PrepareUrl(void);
static void Auth_OnResponse(int status, int unused, SoapReader *reader, void *userData);
static u32 Auth_ToWire(u32 value);
static BOOL Auth_ReadCertificate(AuthCertificate *certificate, SoapReader *reader);

// The service's RSA public key: the exponent and the modulus, as hexadecimal numbers
static const char sPublicKeyExponent[] = "010001";
static const char sPublicKeyModulus[] =
    "BF05D63E93751AD4A59A4A7389CF0BE8A22CCDEEA1E7F12C062D6E194472EFDA5184CCECEB4FBADF5EB1D7ABFE91181453972AA971F624AF9B"
    "A8F0F82E2869FB7D44BDE8D56EE50977898F3FEE75869622C4981F07506248BD3D092E8EA05C12B2FA37881176084C8F8B8756C4722CDC57D2"
    "AD28ACD3AD85934FB48D6B2D2027";

// The declaration of the messages' namespace
static const char *sAuthNamespaces[] = { "ns1=\"http://gamespy.net/AuthService/\"" };

// The URL of the authentication service, set by ScClient_SetServers or once it is asked for
char gAuthUrl[128];

static BOOL Auth_PrepareUrl(void) {
    if (__GSIACResult == 1) {
        if (gAuthUrl[0] == '\0') {
            OS_SNPrintf(gAuthUrl, 128, "https://%s.auth.pubsvs.gs.nintendowifi.net/AuthService/AuthService.asmx",
                        __GSIACGamename);
        }
        return TRUE;
    }
    return FALSE;
}

char *Auth_GetUrl(void) {
    if (__GSIACResult == 1) {
        if (gAuthUrl[0] == '\0') {
            OS_SNPrintf(gAuthUrl, 128, "https://%s.auth.pubsvs.gs.nintendowifi.net/AuthService/AuthService.asmx",
                        __GSIACGamename);
        }
        return gAuthUrl;
    }
    gsDebugFormat(0, 3, 1, "No availability\r\n");
    return NULL;
}

static void Auth_OnResponse(int status, int unused, SoapReader *reader, void *userData) {
    AuthRequest *request = userData;
    AuthResult result;
    AuthCertificate *certificate = &result.certificate;
    HashContext context;

    // As in GameSpy's login callbacks, the certificate is marked invalid before the whole result is cleared
    certificate->valid = FALSE;
    memset(&result, 0, sizeof(result));
    if (status == 0) {
        if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, "LoginRemoteAuthResult")) {
            result.error = AUTH_ERROR_BAD_RESPONSE;
        } else if (!gsXmlReadChildAsInt(reader, "responseCode", &result.responseCode)) {
            result.error = AUTH_ERROR_BAD_RESPONSE;
        } else if (result.responseCode != 0) {
            result.error = 7;
        } else if (!gsXmlMoveToChild(reader, "certificate") || !Auth_ReadCertificate(certificate, reader) ||
                   !gsXmlMoveToParent(reader) ||
                   !gsXmlReadChildAsLargeInt(reader, "peerkeyprivate", &result.keys.privateKey)) {
            result.error = AUTH_ERROR_BAD_RESPONSE;
        } else {
            memcpy(&result.keys.modulus, &certificate->peerKeyModulus, sizeof(BigNum));
            func_ov011_021719fc(&context);
            gsLargeIntAddToMD5(&result.keys.privateKey, &context);
            func_ov011_02171a9c(result.keys.proof, &context);
            certificate->valid = Auth_CheckCertificate(certificate);
            if (!certificate->valid) {
                result.error = AUTH_ERROR_BAD_CERTIFICATE;
            }
        }
    } else if (status == 0x12) {
        result.error = AUTH_ERROR_CANCELED;
    } else {
        result.error = AUTH_ERROR_BAD_ANSWER;
    }
    if (request->func != NULL) {
        request->func(status, &result, request->userData);
    }
    gsifree(request);
}

int wsLoginRemoteAuth(int partnerCode, int namespaceId, const char *authToken, const char *challenge,
                      AuthResultFunc func, void *userData) {
    AuthRequest *request;
    SoapWriter *writer;

    if (!Auth_PrepareUrl()) {
        return AUTH_ERROR_NOT_AVAILABLE;
    }
    request = gsimalloc(sizeof(AuthRequest));
    if (request == NULL) {
        return AUTH_ERROR_NO_MEMORY;
    }
    request->func = func;
    request->userData = userData;
    writer = gsXmlCreateStreamWriter(sAuthNamespaces, 1);
    if (writer == NULL) {
        return AUTH_ERROR_NO_MEMORY;
    }
    if (!gsXmlWriteOpenTag(writer, "ns1", "LoginRemoteAuth") || !gsXmlWriteIntElement(writer, "ns1", "version", 1) ||
        !gsXmlWriteIntElement(writer, "ns1", "partnercode", partnerCode) ||
        !gsXmlWriteIntElement(writer, "ns1", "namespaceid", namespaceId) ||
        !gsXmlWriteStringElement(writer, "ns1", "authtoken", authToken) ||
        !gsXmlWriteStringElement(writer, "ns1", "challenge", challenge) ||
        !gsXmlWriteCloseTag(writer, "ns1", "LoginRemoteAuth") || !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return AUTH_ERROR_NO_MEMORY;
    }
    if (gsiExecuteSoap(gAuthUrl, "SOAPAction: \"http://gamespy.net/AuthService/LoginRemoteAuth\"", writer,
                       Auth_OnResponse, request) == NULL) {
        gsXmlFreeWriter(writer);
        gsifree(request);
        return AUTH_ERROR_NO_MEMORY;
    }
    return 0;
}

static u32 Auth_ToWire(u32 value) {
    return value;
}

BOOL Auth_CheckCertificate(AuthCertificate *certificate) {
    RsaPublicKey key;
    HashContext context;
    u8 digest[0x10];
    u32 word;

    func_ov011_021719fc(&context);
    word = Auth_ToWire(certificate->length);
    func_ov011_02171a24(&context, &word, sizeof(word));
    word = Auth_ToWire(certificate->version);
    func_ov011_02171a24(&context, &word, sizeof(word));
    word = Auth_ToWire(certificate->partnerCode);
    func_ov011_02171a24(&context, &word, sizeof(word));
    word = Auth_ToWire(certificate->namespaceId);
    func_ov011_02171a24(&context, &word, sizeof(word));
    word = Auth_ToWire(certificate->userId);
    func_ov011_02171a24(&context, &word, sizeof(word));
    word = Auth_ToWire(certificate->profileId);
    func_ov011_02171a24(&context, &word, sizeof(word));
    word = Auth_ToWire(certificate->expireTime);
    func_ov011_02171a24(&context, &word, sizeof(word));
    func_ov011_02171a24(&context, certificate->profileNick, strlen(certificate->profileNick));
    func_ov011_02171a24(&context, certificate->uniqueNick, strlen(certificate->uniqueNick));
    func_ov011_02171a24(&context, certificate->cdkeyHash, strlen(certificate->cdkeyHash));
    gsLargeIntAddToMD5(&certificate->peerKeyModulus, &context);
    gsLargeIntAddToMD5(&certificate->peerKeyExponent, &context);
    func_ov011_02171a24(&context, certificate->serverData, sizeof(certificate->serverData));
    func_ov011_02171a9c(digest, &context);
    gsLargeIntSetFromHexString(&key.modulus, sPublicKeyModulus);
    gsLargeIntSetFromHexString(&key.exponent, sPublicKeyExponent);
    if (gsCryptRSAVerifySignedHash(&key, digest, sizeof(digest), certificate->signature,
                                   sizeof(certificate->signature)) != 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL Auth_WriteCertificate(const AuthCertificate *certificate, const char *prefix, SoapWriter *writer) {
    if (!gsXmlWriteIntElement(writer, prefix, "length", certificate->length) ||
        !gsXmlWriteIntElement(writer, prefix, "version", certificate->version) ||
        !gsXmlWriteIntElement(writer, prefix, "partnercode", certificate->partnerCode) ||
        !gsXmlWriteIntElement(writer, prefix, "namespaceid", certificate->namespaceId) ||
        !gsXmlWriteIntElement(writer, prefix, "userid", certificate->userId) ||
        !gsXmlWriteIntElement(writer, prefix, "profileid", certificate->profileId) ||
        !gsXmlWriteIntElement(writer, prefix, "expiretime", certificate->expireTime) ||
        !gsXmlWriteAsciiStringElement(writer, prefix, "profilenick", certificate->profileNick) ||
        !gsXmlWriteAsciiStringElement(writer, prefix, "uniquenick", certificate->uniqueNick) ||
        !gsXmlWriteAsciiStringElement(writer, prefix, "cdkeyhash", certificate->cdkeyHash) ||
        !gsXmlWriteLargeIntElement(writer, prefix, "peerkeymodulus", &certificate->peerKeyModulus) ||
        !gsXmlWriteLargeIntElement(writer, prefix, "peerkeyexponent", &certificate->peerKeyExponent) ||
        !gsXmlWriteHexBinaryElement(writer, prefix, "serverdata", certificate->serverData,
                                    sizeof(certificate->serverData)) ||
        !gsXmlWriteHexBinaryElement(writer, prefix, "signature", certificate->signature,
                                    sizeof(certificate->signature))) {
        return FALSE;
    }
    return TRUE;
}

static BOOL Auth_ReadCertificate(AuthCertificate *certificate, SoapReader *reader) {
    int size;
    char text[0x101];

    if (!gsXmlReadChildAsInt(reader, "length", &certificate->length) ||
        !gsXmlReadChildAsInt(reader, "version", &certificate->version) ||
        !gsXmlReadChildAsInt(reader, "partnercode", &certificate->partnerCode) ||
        !gsXmlReadChildAsInt(reader, "namespaceid", &certificate->namespaceId) ||
        !gsXmlReadChildAsInt(reader, "userid", &certificate->userId) ||
        !gsXmlReadChildAsInt(reader, "profileid", &certificate->profileId) ||
        !gsXmlReadChildAsInt(reader, "expiretime", &certificate->expireTime) ||
        !gsXmlReadChildAsStringNT(reader, "profilenick", certificate->profileNick, sizeof(certificate->profileNick)) ||
        !gsXmlReadChildAsStringNT(reader, "uniquenick", certificate->uniqueNick, sizeof(certificate->uniqueNick)) ||
        !gsXmlReadChildAsStringNT(reader, "cdkeyhash", certificate->cdkeyHash, sizeof(certificate->cdkeyHash)) ||
        !gsXmlReadChildAsStringNT(reader, "peerkeymodulus", text, sizeof(text)) ||
        !gsLargeIntSetFromHexString(&certificate->peerKeyModulus, text) ||
        !gsXmlReadChildAsStringNT(reader, "peerkeyexponent", text, sizeof(text)) ||
        !gsLargeIntSetFromHexString(&certificate->peerKeyExponent, text) ||
        !gsXmlReadChildAsHexBinary(reader, "serverdata", certificate->serverData, sizeof(certificate->serverData),
                                   &size) ||
        !gsXmlReadChildAsHexBinary(reader, "signature", certificate->signature, sizeof(certificate->signature),
                                   &size)) {
        return FALSE;
    }
    return TRUE;
}
