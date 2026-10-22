#include <string.h>
// Before dwc/ext.h: MWCC builds Auth_OnResponse's memset of the result in place, as the ROM does, only when
// <string.h> declares memset first
#include "dwc/AuthService.h"
#include "dwc/ext.h"

// GameSpy's authentication service, as overlay 260's Wi-Fi library uses it: it logs in with the token and the challenge
// that the library has and gets the player's certificate, and the competition service asks it to write the certificate
// and check its signature. The signature is of an MD5-like hash of the certificate's fields over the service's RSA
// public key, which is in the file below. The file and the names are GameSpy's own, as its webservices/AuthService.c
// has them, where the code shows them, and ours elsewhere.

// What a login is waiting for: the function to call and its user data
typedef struct WSIRequestData {
    WSLoginCallback mUserCallback;
    void *mUserData;
} WSIRequestData;

static gsi_bool Auth_PrepareUrl(void);
static void Auth_OnResponse(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData);
static gsi_u32 Auth_ToWire(gsi_u32 value);
static gsi_bool Auth_ReadCertificate(GSLoginCertificate *certificate, GSXmlStreamReader reader);

// The service's RSA public key: the exponent and the modulus, as hexadecimal numbers
static const char sPublicKeyExponent[] = "010001";
static const char WS_AUTHSERVICE_SIGNATURE_KEY[] =
    "BF05D63E93751AD4A59A4A7389CF0BE8A22CCDEEA1E7F12C062D6E194472EFDA5184CCECEB4FBADF5EB1D7ABFE91181453972AA971F624AF9B"
    "A8F0F82E2869FB7D44BDE8D56EE50977898F3FEE75869622C4981F07506248BD3D092E8EA05C12B2FA37881176084C8F8B8756C4722CDC57D2"
    "AD28ACD3AD85934FB48D6B2D2027";

// The declaration of the messages' namespace
static const char *WS_AUTHSERVICE_NAMESPACES[] = { "ns1=\"http://gamespy.net/AuthService/\"" };

// The URL of the authentication service, set by ScClient_SetServers or once it is asked for
char wsAuthServiceURL[128];

static gsi_bool Auth_PrepareUrl(void) {
    if (__GSIACResult == GSIACAvailable) {
        if (wsAuthServiceURL[0] == '\0') {
            OS_SNPrintf(wsAuthServiceURL, 128,
                        "https://%s.auth.pubsvs.gs.nintendowifi.net/AuthService/AuthService.asmx", __GSIACGamename);
        }
        return gsi_true;
    }
    return gsi_false;
}

char *Auth_GetUrl(void) {
    if (__GSIACResult == GSIACAvailable) {
        if (wsAuthServiceURL[0] == '\0') {
            OS_SNPrintf(wsAuthServiceURL, 128,
                        "https://%s.auth.pubsvs.gs.nintendowifi.net/AuthService/AuthService.asmx", __GSIACGamename);
        }
        return wsAuthServiceURL;
    }
    gsDebugFormat(GSIDebugCat_App, GSIDebugType_State, GSIDebugLevel_HotError, "No availability\r\n");
    return NULL;
}

static void Auth_OnResponse(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData) {
    WSIRequestData *request = userData;
    WSLoginResponse result;
    GSLoginCertificate *certificate = &result.mCertificate;
    MD5_CTX context;

    // As in GameSpy's login callbacks, the certificate is marked invalid before the whole result is cleared
    certificate->mIsValid = gsi_false;
    memset(&result, 0, sizeof(result));
    if (status == 0) {
        if (!gsXmlMoveToStart(reader) || !gsXmlMoveToNext(reader, "LoginRemoteAuthResult")) {
            result.mLoginResult = WSLogin_ParseError;
        } else if (!gsXmlReadChildAsInt(reader, "responseCode", (int *)&result.mResponseCode)) {
            result.mLoginResult = WSLogin_ParseError;
        } else if (result.mResponseCode != 0) {
            result.mLoginResult = 7;
        } else if (!gsXmlMoveToChild(reader, "certificate") || !Auth_ReadCertificate(certificate, reader) ||
                   !gsXmlMoveToParent(reader) ||
                   !gsXmlReadChildAsLargeInt(reader, "peerkeyprivate", &result.mPrivateData.mPeerPrivateKey.exponent)) {
            result.mLoginResult = WSLogin_ParseError;
        } else {
            memcpy(&result.mPrivateData.mPeerPrivateKey.modulus, &certificate->mPeerPublicKey.modulus,
                   sizeof(gsLargeInt_t));
            MD5Init(&context);
            gsLargeIntAddToMD5(&result.mPrivateData.mPeerPrivateKey.exponent, &context);
            MD5Final(result.mPrivateData.mKeyHash, &context);
            certificate->mIsValid = Auth_CheckCertificate(certificate);
            if (!certificate->mIsValid) {
                result.mLoginResult = WSLogin_InvalidCertificate;
            }
        }
    } else if (status == 0x12) {
        result.mLoginResult = WSLogin_Cancelled;
    } else {
        result.mLoginResult = WSLogin_HttpError;
    }
    if (request->mUserCallback != NULL) {
        request->mUserCallback(status, &result, request->mUserData);
    }
    gsifree(request);
}

gsi_u32 wsLoginRemoteAuth(int partnerCode, int namespaceId, const gsi_char *authToken, const gsi_char *challenge,
                          WSLoginCallback func, void *userData) {
    WSIRequestData *request;
    GSXmlStreamWriter writer;

    if (!Auth_PrepareUrl()) {
        return WSLogin_NoAvailabilityCheck;
    }
    request = gsimalloc(sizeof(WSIRequestData));
    if (request == NULL) {
        return WSLogin_OutOfMemory;
    }
    request->mUserCallback = func;
    request->mUserData = userData;
    writer = gsXmlCreateStreamWriter(WS_AUTHSERVICE_NAMESPACES, 1);
    if (writer == NULL) {
        return WSLogin_OutOfMemory;
    }
    if (!gsXmlWriteOpenTag(writer, "ns1", "LoginRemoteAuth") || !gsXmlWriteIntElement(writer, "ns1", "version", 1) ||
        !gsXmlWriteIntElement(writer, "ns1", "partnercode", partnerCode) ||
        !gsXmlWriteIntElement(writer, "ns1", "namespaceid", namespaceId) ||
        !gsXmlWriteStringElement(writer, "ns1", "authtoken", authToken) ||
        !gsXmlWriteStringElement(writer, "ns1", "challenge", challenge) ||
        !gsXmlWriteCloseTag(writer, "ns1", "LoginRemoteAuth") || !gsXmlCloseWriter(writer)) {
        gsXmlFreeWriter(writer);
        return WSLogin_OutOfMemory;
    }
    if (gsiExecuteSoap(wsAuthServiceURL, "SOAPAction: \"http://gamespy.net/AuthService/LoginRemoteAuth\"", writer,
                       Auth_OnResponse, request) == NULL) {
        gsXmlFreeWriter(writer);
        gsifree(request);
        return WSLogin_OutOfMemory;
    }
    return 0;
}

static gsi_u32 Auth_ToWire(gsi_u32 value) {
    return value;
}

gsi_bool Auth_CheckCertificate(const GSLoginCertificate *certificate) {
    gsCryptRSAKey key;
    MD5_CTX context;
    gsi_u8 digest[0x10];
    gsi_u32 word;

    MD5Init(&context);
    word = Auth_ToWire(certificate->mLength);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    word = Auth_ToWire(certificate->mVersion);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    word = Auth_ToWire(certificate->mPartnerCode);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    word = Auth_ToWire(certificate->mNamespaceId);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    word = Auth_ToWire(certificate->mUserId);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    word = Auth_ToWire(certificate->mProfileId);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    word = Auth_ToWire(certificate->mExpireTime);
    MD5Update(&context, (unsigned char *)&word, sizeof(word));
    MD5Update(&context, (unsigned char *)certificate->mProfileNick, strlen(certificate->mProfileNick));
    MD5Update(&context, (unsigned char *)certificate->mUniqueNick, strlen(certificate->mUniqueNick));
    MD5Update(&context, (unsigned char *)certificate->mCdKeyHash, strlen(certificate->mCdKeyHash));
    gsLargeIntAddToMD5(&certificate->mPeerPublicKey.modulus, &context);
    gsLargeIntAddToMD5(&certificate->mPeerPublicKey.exponent, &context);
    MD5Update(&context, (unsigned char *)certificate->mServerData, sizeof(certificate->mServerData));
    MD5Final(digest, &context);
    gsLargeIntSetFromHexString(&key.modulus, WS_AUTHSERVICE_SIGNATURE_KEY);
    gsLargeIntSetFromHexString(&key.exponent, sPublicKeyExponent);
    if (gsCryptRSAVerifySignedHash(&key, digest, sizeof(digest), certificate->mSignature,
                                   sizeof(certificate->mSignature)) != 0) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool Auth_WriteCertificate(const GSLoginCertificate *certificate, const char *prefix, GSXmlStreamWriter writer) {
    if (!gsXmlWriteIntElement(writer, prefix, "length", certificate->mLength) ||
        !gsXmlWriteIntElement(writer, prefix, "version", certificate->mVersion) ||
        !gsXmlWriteIntElement(writer, prefix, "partnercode", certificate->mPartnerCode) ||
        !gsXmlWriteIntElement(writer, prefix, "namespaceid", certificate->mNamespaceId) ||
        !gsXmlWriteIntElement(writer, prefix, "userid", certificate->mUserId) ||
        !gsXmlWriteIntElement(writer, prefix, "profileid", certificate->mProfileId) ||
        !gsXmlWriteIntElement(writer, prefix, "expiretime", certificate->mExpireTime) ||
        !gsXmlWriteAsciiStringElement(writer, prefix, "profilenick", certificate->mProfileNick) ||
        !gsXmlWriteAsciiStringElement(writer, prefix, "uniquenick", certificate->mUniqueNick) ||
        !gsXmlWriteAsciiStringElement(writer, prefix, "cdkeyhash", certificate->mCdKeyHash) ||
        !gsXmlWriteLargeIntElement(writer, prefix, "peerkeymodulus", &certificate->mPeerPublicKey.modulus) ||
        !gsXmlWriteLargeIntElement(writer, prefix, "peerkeyexponent", &certificate->mPeerPublicKey.exponent) ||
        !gsXmlWriteHexBinaryElement(writer, prefix, "serverdata", certificate->mServerData,
                                    sizeof(certificate->mServerData)) ||
        !gsXmlWriteHexBinaryElement(writer, prefix, "signature", certificate->mSignature,
                                    sizeof(certificate->mSignature))) {
        return gsi_false;
    }
    return gsi_true;
}

static gsi_bool Auth_ReadCertificate(GSLoginCertificate *certificate, GSXmlStreamReader reader) {
    int size;
    char text[0x101];

    if (!gsXmlReadChildAsInt(reader, "length", &certificate->mLength) ||
        !gsXmlReadChildAsInt(reader, "version", &certificate->mVersion) ||
        !gsXmlReadChildAsInt(reader, "partnercode", &certificate->mPartnerCode) ||
        !gsXmlReadChildAsInt(reader, "namespaceid", &certificate->mNamespaceId) ||
        !gsXmlReadChildAsInt(reader, "userid", &certificate->mUserId) ||
        !gsXmlReadChildAsInt(reader, "profileid", &certificate->mProfileId) ||
        !gsXmlReadChildAsInt(reader, "expiretime", &certificate->mExpireTime) ||
        !gsXmlReadChildAsStringNT(reader, "profilenick", certificate->mProfileNick,
                                  sizeof(certificate->mProfileNick)) ||
        !gsXmlReadChildAsStringNT(reader, "uniquenick", certificate->mUniqueNick, sizeof(certificate->mUniqueNick)) ||
        !gsXmlReadChildAsStringNT(reader, "cdkeyhash", certificate->mCdKeyHash, sizeof(certificate->mCdKeyHash)) ||
        !gsXmlReadChildAsStringNT(reader, "peerkeymodulus", text, sizeof(text)) ||
        !gsLargeIntSetFromHexString(&certificate->mPeerPublicKey.modulus, text) ||
        !gsXmlReadChildAsStringNT(reader, "peerkeyexponent", text, sizeof(text)) ||
        !gsLargeIntSetFromHexString(&certificate->mPeerPublicKey.exponent, text) ||
        !gsXmlReadChildAsHexBinary(reader, "serverdata", certificate->mServerData, sizeof(certificate->mServerData),
                                   &size) ||
        !gsXmlReadChildAsHexBinary(reader, "signature", certificate->mSignature, sizeof(certificate->mSignature),
                                   &size)) {
        return gsi_false;
    }
    return gsi_true;
}
