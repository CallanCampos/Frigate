// The SSL settings of the GDS client's connection: the certificate authority that the server's certificate must come
// from, and the settings of a connection, which the socket library reads. The ROM names nothing here: the file's name
// and its functions' are guesses.

#include "types.h"
#include "poke_net_internal.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"

static int POKE_NET_SSL_AuthCallback(int result, POKE_NET_SSL_CONNECTION *connection, int level);

// The public key of Nintendo of America's certificate authority: the exponent 65537 and the modulus
static u8 sPOKE_NET_SSL_CaExponent[] = {0x01, 0x00, 0x01};

static u8 sPOKE_NET_SSL_CaModulo[] = {
    0xb4, 0xa8, 0x0f, 0xe3, 0x80, 0x02, 0xa7, 0xad, 0xfb, 0x59, 0x9d, 0xe1, 0x92, 0x9d, 0x19, 0x7b,
    0x62, 0x00, 0x96, 0xce, 0x13, 0x2e, 0x5b, 0x83, 0xc0, 0x33, 0x26, 0xa6, 0x39, 0x34, 0xac, 0x8e,
    0xe7, 0x7f, 0x69, 0x7e, 0xd4, 0xc0, 0x7c, 0xb7, 0x60, 0x86, 0xaf, 0xb6, 0x45, 0x06, 0x36, 0xe4,
    0x5d, 0xea, 0xed, 0x15, 0x54, 0x36, 0x1d, 0x57, 0xb6, 0x9e, 0x0e, 0x07, 0x43, 0x62, 0x10, 0xee,
    0x35, 0xc2, 0x41, 0x34, 0x38, 0xd0, 0xdd, 0x52, 0xae, 0x86, 0xfd, 0x0c, 0x9d, 0x29, 0xfa, 0x3e,
    0xb6, 0xbb, 0xd9, 0x51, 0x38, 0x26, 0x24, 0x87, 0xa2, 0x20, 0x37, 0x4e, 0x8e, 0xbc, 0xd1, 0x04,
    0x5b, 0x98, 0x18, 0x7e, 0xa8, 0x54, 0xfd, 0xd0, 0xbf, 0xa6, 0x1b, 0x48, 0x0e, 0xe6, 0x19, 0xb4,
    0xdf, 0xc4, 0x3f, 0x8a, 0x19, 0xce, 0xa0, 0x14, 0x23, 0xb1, 0x47, 0x12, 0x2c, 0x1e, 0x8c, 0xa7,
};

static char sPOKE_NET_SSL_CaDn[] =
    "US, Washington, Nintendo of America Inc., NOA, Nintendo Class 2 CA, ca@noa.nintendo.com";

static POKE_NET_SSL_CA_INFO sPOKE_NET_SSL_CaInfo = {
    sPOKE_NET_SSL_CaDn,
    sizeof(sPOKE_NET_SSL_CaModulo),
    sPOKE_NET_SSL_CaModulo,
    sizeof(sPOKE_NET_SSL_CaExponent),
    sPOKE_NET_SSL_CaExponent,
};

static POKE_NET_SSL_CA_INFO *sPOKE_NET_SSL_CaInfoList[] = {
    &sPOKE_NET_SSL_CaInfo,
};

// The settings that connections start from: the default ones unless others were set
static POKE_NET_SSL_CONNECTION sPOKE_NET_SSL_DefaultConnection;
static u32 sPOKE_NET_SSL_Entropy[8];
static POKE_NET_SSL_CONNECTION *sPOKE_NET_SSL_Connection;

// Lets every certificate that the library's own check let through pass
static int POKE_NET_SSL_AuthCallback(int result, POKE_NET_SSL_CONNECTION *connection, int level) {
    return 0;
}

// Seeds the random numbers of the secure connections, and makes the default settings the first time
void POKE_NET_SSL_Setup(void) {
    OS_GetLowEntropyData(sPOKE_NET_SSL_Entropy);
    func_ov011_02159d64(sPOKE_NET_SSL_Entropy, sizeof(sPOKE_NET_SSL_Entropy));
    if (sPOKE_NET_SSL_Connection != NULL) {
        return;
    }
    sys_memset(&sPOKE_NET_SSL_DefaultConnection, 0, sizeof(POKE_NET_SSL_CONNECTION));
    sPOKE_NET_SSL_DefaultConnection.ServerName = NULL;
    POKE_NET_SSL_SetConnection(&sPOKE_NET_SSL_DefaultConnection);
    POKE_NET_SSL_SetCaInfo(sPOKE_NET_SSL_CaInfoList, NELEMS(sPOKE_NET_SSL_CaInfoList));
    POKE_NET_SSL_SetAuthCallback(POKE_NET_SSL_AuthCallback);
}

// Sets the settings that connections start from, NULL for the default ones
void POKE_NET_SSL_SetConnection(POKE_NET_SSL_CONNECTION *connection) {
    if (connection != NULL) {
        sPOKE_NET_SSL_Connection = connection;
    } else {
        sPOKE_NET_SSL_Connection = &sPOKE_NET_SSL_DefaultConnection;
    }
}

void POKE_NET_SSL_SetAuthCallback(POKE_NET_SSL_AUTH_CALLBACK callback) {
    sPOKE_NET_SSL_Connection->AuthCallback = callback;
}

void POKE_NET_SSL_SetCaInfo(POKE_NET_SSL_CA_INFO **caInfo, int count) {
    sPOKE_NET_SSL_Connection->CaInfo = caInfo;
    sPOKE_NET_SSL_Connection->CaInfoCount = count;
}

// Makes the settings of a connection to a server, with a copy of the server's name
void POKE_NET_SSL_StartConnection(const char *serverName, POKE_NET_SSL_CONNECTION *connection) {
    sys_memcpy(sPOKE_NET_SSL_Connection, connection, sizeof(POKE_NET_SSL_CONNECTION));
    if (serverName == NULL) {
        return;
    }
    connection->ServerName = POKE_NET_AllocFunc(NNS_STD_StrLen(serverName) + 1);
    func_0207f7a4(connection->ServerName, serverName);
}

void POKE_NET_SSL_EndConnection(POKE_NET_SSL_CONNECTION *connection) {
    if (connection->ServerName == NULL) {
        return;
    }
    POKE_NET_FreeFunc(connection->ServerName);
    connection->ServerName = NULL;
}
