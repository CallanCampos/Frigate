#ifndef POKEBW2_DWC_AUTH_H
#define POKEBW2_DWC_AUTH_H

#include "dwc/ghttp.h"
#include "dwc/soap.h"

// GameSpy's authentication service in overlay 260 (dwc_auth.c): the Wi-Fi library gives it a token and a challenge, it
// posts them to the service and gets back a certificate of the player, signed by the service, with the key pair of the
// player's session. The competition service takes that certificate and a proof made from the key. The texts of the
// service's messages name the certificate's fields

// The player's certificate, as the service writes it. `valid` tells that the signature is right (it is made by the
// check, not read); the signature is over the fields in between and the server data
typedef struct GSLoginCertificate {
    gsi_bool valid;
    gsi_u32 length;
    gsi_u32 version;
    gsi_u32 partnerCode;
    gsi_u32 namespaceId;
    gsi_u32 userId;
    gsi_u32 profileId;
    gsi_u32 expireTime;
    gsi_char profileNick[0x1f];
    gsi_char uniqueNick[0x15];
    gsi_char cdkeyHash[0x21];
    gsCryptRSAKey peerPublicKey;
    gsi_u8 signature[0x80];
    gsi_u8 serverData[0x80];
} GSLoginCertificate;

// The key pair of the session (its modulus and private exponent), and the hash of the private key that is the proof
typedef struct GSLoginCertificatePrivate {
    gsCryptRSAKey peerPrivateKey;
    char proof[0x10];
} GSLoginPrivateData;

// The login's results: the server's (0 to WSLogin_ServerError) and the library's own (from WSLogin_HttpError, 0x64)
typedef enum WSLoginValue {
    WSLogin_Success = 0,
    WSLogin_ServerInitFailed,
    WSLogin_UserNotFound,
    WSLogin_InvalidPassword,
    WSLogin_InvalidProfile,
    WSLogin_UniqueNickExpired,
    WSLogin_DBError,
    WSLogin_ServerError,
    WSLogin_FailureMax,
    WSLogin_HttpError = 100,
    WSLogin_ParseError,
    WSLogin_InvalidCertificate,
    WSLogin_LoginFailed,
    WSLogin_OutOfMemory,
    WSLogin_InvalidParameters,
    WSLogin_NoAvailabilityCheck,
    WSLogin_Cancelled,
    WSLogin_UnknownError
} WSLoginValue;

// What the login brings: its result, the answer's response code, the certificate and the keys
typedef struct WSLoginResponse {
    WSLoginValue error;
    WSLoginValue responseCode;
    GSLoginCertificate certificate;
    GSLoginPrivateData keys;
    void *userData;
} WSLoginResponse;

// Called when the login has ended, with the status of the request, the result and the user data
typedef void (*WSLoginCallback)(GHTTPResult status, WSLoginResponse *result, void *userData);

// The URL of the authentication service, which ScClient_SetServers also sets
extern char gAuthUrl[128];
// The URL of the authentication service, or 0 when the library is not running
char *Auth_GetUrl(void);
// Logs in with the token and the challenge that the Wi-Fi library gave, as a partner and a namespace, returning 0 or an
// error code
gsi_u32 wsLoginRemoteAuth(int partnerCode, int namespaceId, const gsi_char *authToken, const gsi_char *challenge,
                          WSLoginCallback func, void *userData);
// Checks the signature of a certificate, which sets `valid`, and returns it
gsi_bool Auth_CheckCertificate(const GSLoginCertificate *certificate);
// Writes the certificate's fields into a message, with the prefix of the namespace they are in
gsi_bool Auth_WriteCertificate(const GSLoginCertificate *certificate, const char *prefix, GSXmlStreamWriter writer);

#endif // POKEBW2_DWC_AUTH_H
