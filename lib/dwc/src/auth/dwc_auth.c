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
    "BF05D63E93751AD4A59A4A7389CF0BE8A22CCDEEA1E7F12C062D6E194472EFDA5184CCECEB4FBADF5EB1D7ABFE91181453972AA971F624AF9BA8F0F82E2869FB7D44BDE8D56EE50977898F3FEE75869622C4981F07506248BD3D092E8EA05C12B2FA37881176084C8F8B8756C4722CDC57D2AD28ACD3AD85934FB48D6B2D2027";

// The declaration of the messages' namespace
static const char *sAuthNamespaces[] = {"ns1=\"http://gamespy.net/AuthService/\""};

// The URL of the authentication service, set by ScClient_SetServers or once it is asked for
char gAuthUrl[128];

static BOOL Auth_PrepareUrl(void) {
    if (data_ov011_02190698 == 1) {
        if (gAuthUrl[0] == '\0') {
            func_0207a2c0(gAuthUrl, 128, "https://%s.auth.pubsvs.gs.nintendowifi.net/AuthService/AuthService.asmx",
                          data_ov011_0219069c);
        }
        return TRUE;
    }
    return FALSE;
}

char *Auth_GetUrl(void) {
    if (data_ov011_02190698 == 1) {
        if (gAuthUrl[0] == '\0') {
            func_0207a2c0(gAuthUrl, 128, "https://%s.auth.pubsvs.gs.nintendowifi.net/AuthService/AuthService.asmx",
                          data_ov011_0219069c);
        }
        return gAuthUrl;
    }
    func_ov011_0216cca8(0, 3, 1, "No availability\r\n");
    return NULL;
}

static void Auth_OnResponse(int status, int unused, SoapReader *reader, void *userData) {
    AuthRequest *request = userData;
    AuthResult result = {0};
    HashContext context;

    if (status == 0) {
        if (!func_ov011_02170d18(reader) || !func_ov011_02170d28(reader, "LoginRemoteAuthResult")
            || !func_ov011_02171090(reader, "responseCode", &result.responseCode)) {
            result.error = AUTH_ERROR_BAD_RESPONSE;
        } else if (result.responseCode != 0) {
            result.error = 7;
        } else if (!func_ov011_02170db8(reader, "certificate") || !Auth_ReadCertificate(&result.certificate, reader)
                   || !func_ov011_02170d70(reader)
                   || !func_ov011_02171330(reader, "peerkeyprivate", &result.keys.privateKey)) {
            result.error = AUTH_ERROR_BAD_RESPONSE;
        } else {
            memcpy(&result.keys.modulus, &result.certificate.peerKeyModulus, sizeof(BigNum));
            func_ov011_021719fc(&context);
            func_ov011_0216dd04(&result.keys.privateKey, &context);
            func_ov011_02171a9c(result.keys.proof, &context);
            result.certificate.valid = Auth_CheckCertificate(&result.certificate);
            if (!result.certificate.valid) {
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
    func_ov011_0216ddfc(request);
}

int wsLoginRemoteAuth(int partnerCode, int namespaceId, const char *authToken, const char *challenge,
                      AuthResultFunc func, void *userData) {
    AuthRequest *request;
    SoapWriter *writer;

    if (!Auth_PrepareUrl()) {
        return AUTH_ERROR_NOT_AVAILABLE;
    }
    request = func_ov011_0216dddc(sizeof(AuthRequest));
    if (request == NULL) {
        return AUTH_ERROR_NO_MEMORY;
    }
    request->func = func;
    request->userData = userData;
    writer = func_ov011_0216fbf8(sAuthNamespaces, 1);
    if (writer == NULL) {
        return AUTH_ERROR_NO_MEMORY;
    }
    if (!func_ov011_02170614(writer, "ns1", "LoginRemoteAuth") || !func_ov011_02170818(writer, "ns1", "version", 1)
        || !func_ov011_02170818(writer, "ns1", "partnercode", partnerCode)
        || !func_ov011_02170818(writer, "ns1", "namespaceid", namespaceId)
        || !func_ov011_021706b8(writer, "ns1", "authtoken", authToken)
        || !func_ov011_021706b8(writer, "ns1", "challenge", challenge)
        || !func_ov011_02170660(writer, "ns1", "LoginRemoteAuth") || !func_ov011_0216fea8(writer)) {
        func_ov011_0216fe64(writer);
        return AUTH_ERROR_NO_MEMORY;
    }
    if (func_ov011_0216ea4c(gAuthUrl, "SOAPAction: \"http://gamespy.net/AuthService/LoginRemoteAuth\"", writer,
                            Auth_OnResponse, request)
        == NULL) {
        func_ov011_0216fe64(writer);
        func_ov011_0216ddfc(request);
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
    func_ov011_0216dd04(&certificate->peerKeyModulus, &context);
    func_ov011_0216dd04(&certificate->peerKeyExponent, &context);
    func_ov011_02171a24(&context, certificate->serverData, sizeof(certificate->serverData));
    func_ov011_02171a9c(digest, &context);
    func_ov011_0216dc38(&key.modulus, sPublicKeyModulus);
    func_ov011_0216dc38(&key.exponent, sPublicKeyExponent);
    if (func_ov011_0216cac0(&key, digest, sizeof(digest), certificate->signature, sizeof(certificate->signature))
        != 0) {
        return FALSE;
    }
    return TRUE;
}

BOOL Auth_WriteCertificate(const AuthCertificate *certificate, const char *prefix, SoapWriter *writer) {
    if (!func_ov011_02170818(writer, prefix, "length", certificate->length)
        || !func_ov011_02170818(writer, prefix, "version", certificate->version)
        || !func_ov011_02170818(writer, prefix, "partnercode", certificate->partnerCode)
        || !func_ov011_02170818(writer, prefix, "namespaceid", certificate->namespaceId)
        || !func_ov011_02170818(writer, prefix, "userid", certificate->userId)
        || !func_ov011_02170818(writer, prefix, "profileid", certificate->profileId)
        || !func_ov011_02170818(writer, prefix, "expiretime", certificate->expireTime)
        || !func_ov011_0217071c(writer, prefix, "profilenick", certificate->profileNick)
        || !func_ov011_0217071c(writer, prefix, "uniquenick", certificate->uniqueNick)
        || !func_ov011_0217071c(writer, prefix, "cdkeyhash", certificate->cdkeyHash)
        || !func_ov011_02170a80(writer, prefix, "peerkeymodulus", &certificate->peerKeyModulus)
        || !func_ov011_02170a80(writer, prefix, "peerkeyexponent", &certificate->peerKeyExponent)
        || !func_ov011_0217090c(writer, prefix, "serverdata", certificate->serverData, sizeof(certificate->serverData))
        || !func_ov011_0217090c(writer, prefix, "signature", certificate->signature, sizeof(certificate->signature))) {
        return FALSE;
    }
    return TRUE;
}

static BOOL Auth_ReadCertificate(AuthCertificate *certificate, SoapReader *reader) {
    int size;
    char text[0x101];

    if (!func_ov011_02171090(reader, "length", &certificate->length)
        || !func_ov011_02171090(reader, "version", &certificate->version)
        || !func_ov011_02171090(reader, "partnercode", &certificate->partnerCode)
        || !func_ov011_02171090(reader, "namespaceid", &certificate->namespaceId)
        || !func_ov011_02171090(reader, "userid", &certificate->userId)
        || !func_ov011_02171090(reader, "profileid", &certificate->profileId)
        || !func_ov011_02171090(reader, "expiretime", &certificate->expireTime)
        || !func_ov011_02170e90(reader, "profilenick", certificate->profileNick, sizeof(certificate->profileNick))
        || !func_ov011_02170e90(reader, "uniquenick", certificate->uniqueNick, sizeof(certificate->uniqueNick))
        || !func_ov011_02170e90(reader, "cdkeyhash", certificate->cdkeyHash, sizeof(certificate->cdkeyHash))
        || !func_ov011_02170e90(reader, "peerkeymodulus", text, sizeof(text))
        || !func_ov011_0216dc38(&certificate->peerKeyModulus, text)
        || !func_ov011_02170e90(reader, "peerkeyexponent", text, sizeof(text))
        || !func_ov011_0216dc38(&certificate->peerKeyExponent, text)
        || !func_ov011_02170ed8(reader, "serverdata", certificate->serverData, sizeof(certificate->serverData), &size)
        || !func_ov011_02170ed8(reader, "signature", certificate->signature, sizeof(certificate->signature), &size)) {
        return FALSE;
    }
    return TRUE;
}
