#ifndef POKEBW2_DWC_AUTH_H
#define POKEBW2_DWC_AUTH_H

#include "types.h"
#include "dwc/soap.h"

// GameSpy's authentication service in overlay 260 (dwc_auth.c): the Wi-Fi library gives it a token and a challenge, it
// posts them to the service and gets back a certificate of the player, signed by the service, with the key pair of the
// player's session. The competition service takes that certificate and a proof made from the key. The texts of the
// service's messages name the certificate's fields

// The player's certificate, as the service writes it. `valid` tells that the signature is right (it is made by the
// check, not read); the signature is over the fields in between and the server data
typedef struct AuthCertificate {
    int valid;
    int length;
    int version;
    int partnerCode;
    int namespaceId;
    int userId;
    int profileId;
    int expireTime;
    char profileNick[0x1f];
    char uniqueNick[0x15];
    char cdkeyHash[0x21];
    BigNum peerKeyModulus;
    BigNum peerKeyExponent;
    u8 signature[0x80];
    u8 serverData[0x80];
} AuthCertificate;

// The key pair of the session: the modulus and the private key, and the hash of the private key that is the proof
typedef struct AuthKeys {
    BigNum modulus;
    BigNum privateKey;
    u8 proof[0x10];
} AuthKeys;

// What the login brings: the error (0, or one of the codes below), the answer's response code, the certificate and
// the keys
typedef struct AuthResult {
    int error;
    int responseCode;
    AuthCertificate certificate;
    AuthKeys keys;
    int unk_5a0;
} AuthResult;

// The errors of the login that are the library's own: 0x64 for an answer that could not be taken, up to 0x6b for a
// canceled request
typedef enum AuthError {
    AUTH_ERROR_NONE = 0,
    AUTH_ERROR_BAD_ANSWER = 0x64,
    AUTH_ERROR_BAD_RESPONSE = 0x65,
    AUTH_ERROR_BAD_CERTIFICATE = 0x66,
    AUTH_ERROR_NO_MEMORY = 0x68,
    AUTH_ERROR_NOT_AVAILABLE = 0x6a,
    AUTH_ERROR_CANCELED = 0x6b,
} AuthError;

// Called when the login has ended, with the status of the request, the result and the user data
typedef void (*AuthResultFunc)(int status, AuthResult *result, void *userData);

// The URL of the authentication service, which ScClient_SetServers also sets
extern char gAuthUrl[128];
// The URL of the authentication service, or 0 when the library is not running
char *Auth_GetUrl(void);
// Logs in with the token and the challenge that the Wi-Fi library gave, as a partner and a namespace, returning 0 or an
// error code
int wsLoginRemoteAuth(int partnerCode, int namespaceId, const char *authToken, const char *challenge,
                      AuthResultFunc func, void *userData);
// Checks the signature of a certificate, which sets `valid`, and returns it
BOOL Auth_CheckCertificate(AuthCertificate *certificate);
// Writes the certificate's fields into a message, with the prefix of the namespace they are in
BOOL Auth_WriteCertificate(const AuthCertificate *certificate, const char *prefix, SoapWriter *writer);

#endif // POKEBW2_DWC_AUTH_H
