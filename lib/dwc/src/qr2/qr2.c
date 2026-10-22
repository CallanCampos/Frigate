#include <string.h>
// Before dwc/ext.h: MWCC builds the copies and the zeroing of the IP verify table in place, as the ROM does, only when
// <string.h> declares memcpy and memset
#include "dwc/ext.h"
#include "dwc/natneg.h"
#include "dwc/qr2.h"
#include "nitro/std.h"

// GameSpy's query and reporting (qr2.c). The ROM names neither the file nor its functions: the file, qr2_init_socketA,
// the qr2_register_*_callback functions, qr2_think, qr2_check_queries, qr2_check_send_heartbeat,
// qr2_send_statechanged, qr2_shutdown, qr2_keybuffer_add, qr2_buffer_add_int, qr2_buffer_addA, gs_encode, gs_encrypt,
// qr_build_partial_query_reply, qr_build_split_query_reply, qr_process_query, qr_process_client_message,
// qr2_parse_queryA, send_keepalive and send_heartbeat are GameSpy's, as pret's pokerevo Wii build has them (qr2.s), and
// the other functions, the types and the data are named after the later SDK (get_sockaddrin and indata are
// also statics of gsAvailable.c and natneg.c)

// Where a reply that takes several packets has got to
typedef struct QRSplitQueryProgress {
    qr2_key_type mCurKeyType;
    int mCurPacketNum;
    int mCurKeyIndex;
    int mCurSubCount; // the players or the teams
    int mCurSubIndex;
    struct qr2_keybuffer_s mKeyBuffer;
} QRSplitQueryProgress;

static void enum_local_ips(void);
static gsi_bool get_sockaddrin(const char *host, int port, SOCKADDR_IN *saddr, HOSTENT **savehent);
static void swap_byte(unsigned char *a, unsigned char *b);
static unsigned char encode_ct(unsigned char c);
static void gs_encode(const unsigned char *ins, int size, unsigned char *result);
static void gs_encrypt(const unsigned char *key, int keyLen, unsigned char *bufferPtr, int bufferLen);
static void NatNegProgressCallback(NegotiateState state, void *userData);
static void NatNegCompletedCallback(NegotiateResult result, SOCKET gameSocket, SOCKADDR_IN *remoteAddr, void *userData);
static void qr_add_packet_header(qr2_buffer_t buf, char ptype, char *reqkey);
static void compute_challenge_response(qr2_t qrec, qr2_buffer_t buf, char *challenge, int challengeLen);
static void handle_public_address(qr2_t qrec, char *buffer);
static void qr_build_partial_query_reply(qr2_t qrec, qr2_buffer_t buf, qr2_key_type keyType, int keyCount,
                                         const unsigned char *keys);
static void qr_build_query_reply(qr2_t qrec, qr2_buffer_t buf, int serverKeyCount, const unsigned char *serverKeys,
                                 int playerKeyCount, const unsigned char *playerKeys, int teamKeyCount,
                                 const unsigned char *teamKeys);
static gsi_bool qr_build_split_query_reply(qr2_t qrec, qr2_buffer_t buf, QRSplitQueryProgress *progress);
static void qr_process_query(qr2_t qrec, qr2_buffer_t buf, unsigned char *qdata, int len, SOCKADDR *sender);
static void qr_process_client_message(qr2_t qrec, char *buf, int len);
static int qr_got_recent_message(qr2_t qrec, int msgKey);
static gsi_bool qr2_process_ip_verify(qr2_t qrec, struct qr2_buffer_s *buf, SOCKADDR_IN *sender);
static gsi_bool qr2_check_ip_verify(qr2_t qrec, SOCKADDR_IN *sender, gsi_u32 ipVerify);
static void qr2_expire_ip_verify(qr2_t qrec);
static void send_keepalive(qr2_t qrec);
static void send_heartbeat(qr2_t qrec, int stateChanged);

struct qr2_implementation_s static_qr2_rec = { -1 };
static qr2_t current_rec = &static_qr2_rec;
// MWCC lays out the .bss in the reverse order of the definitions, and the ROM has num_local_ips first and the buffer
// that the queries are read into last
static char indata[256];
char qr2_hostname[64];
static SOCInAddr local_ip_list[5];
static int num_local_ips;

qr2_error_t qr2_init_socketA(qr2_t *qrec, SOCKET sock, int qport, const char *gamename, const char *secretKey,
                             int isPublic, int natNegotiate, qr2_serverkeycallback_t serverKeyCallback,
                             qr2_playerteamkeycallback_t playerKeyCallback, qr2_playerteamkeycallback_t teamKeyCallback,
                             qr2_keylistcallback_t keyListCallback, qr2_countcallback_t playerTeamCountCallback,
                             qr2_adderrorcallback_t addErrorCallback, void *userData) {
    char hostname[64];
    int ret;
    int i;
    qr2_t cr;

    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_StackTrace, "qr2_init_socket()\r\n");
    if (qrec == NULL) {
        cr = &static_qr2_rec;
    } else {
        cr = gsimalloc(sizeof(struct qr2_implementation_s));
        *qrec = cr;
    }
    srand(current_time());
    gsiSafeStrcpyA(cr->gamename, gamename, sizeof(cr->gamename));
    gsiSafeStrcpyA(cr->secretKey, secretKey, sizeof(cr->secretKey));
    cr->qport = qport;
    cr->lastHeartbeat = 0;
    cr->lastKa = 0;
    cr->hbSock = sock;
    cr->listedState = 1;
    cr->userData = userData;
    cr->serverKeyCallback = serverKeyCallback;
    cr->playerKeyCallback = playerKeyCallback;
    cr->teamKeyCallback = teamKeyCallback;
    cr->keyListCallback = keyListCallback;
    cr->playerTeamCountCallback = playerTeamCountCallback;
    cr->addErrorCallback = addErrorCallback;
    cr->nnCallback = NULL;
    cr->cmCallback = NULL;
    cr->cdkeyProcess = NULL;
    cr->isPublic = isPublic;
    cr->readSocket = gsi_false;
    cr->natNegotiate = natNegotiate;
    cr->publicIp = 0;
    cr->publicPort = 0;
    cr->paCallback = NULL;
    cr->crCallback = NULL;
    cr->ccCallback = NULL;
    cr->userStateChangeRequested = gsi_false;
    cr->backendOptions = 0;
    for (i = 0; i < REQUEST_KEY_LEN; i++) {
        cr->instanceKey[i] = rand() % 0xff;
    }
    for (i = 0; i < RECENT_CLIENT_MESSAGES_TO_TRACK; i++) {
        cr->clientMessageKeys[i] = -1;
    }
    cr->curMessageKey = 0;
    memset(cr->ipVerify, 0, sizeof(cr->ipVerify));
    enum_local_ips();
    if (isPublic) {
        int override = qr2_hostname[0];

        if (!override) {
            OS_SPrintf(hostname, "%s.master.gs.nintendowifi.net", gamename);
        }
        ret = get_sockaddrin(override ? qr2_hostname : hostname, 27900, &cr->hbAddr, NULL);
        if (ret == 1) {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment, "%s resolved to %s\r\n",
                          override ? qr2_hostname : hostname, SOC_InetNtoA(cr->hbAddr.addr));
        } else {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_HotError,
                          "Failed on DNS lookup for %s \r\n", override ? qr2_hostname : hostname);
        }
    } else {
        ret = 1;
    }
    if (!ret) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_HotError,
                      "qr2_init_socket() returned failed (DNS error)\r\n");
        return 3;
    }
    return 0;
}

void qr2_register_natneg_callback(qr2_t qrec, qr2_natnegcallback_t callback) {
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_StackTrace, "qr2_register_natneg_callback()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    qrec->nnCallback = callback;
}

void qr2_register_clientmessage_callback(qr2_t qrec, qr2_clientmessagecallback_t callback) {
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_StackTrace,
                  "qr2_register_clientmessage_callback()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    qrec->cmCallback = callback;
}

void qr2_register_publicaddress_callback(qr2_t qrec, qr2_publicaddresscallback_t callback) {
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_StackTrace,
                  "qr2_register_publicaddress_callback()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    qrec->paCallback = callback;
}

void qr2_think(qr2_t qrec) {
    if (qrec == NULL) {
        qrec = current_rec;
    }
    if (qrec->isPublic) {
        qr2_check_send_heartbeat(qrec);
    }
    qr2_check_queries(qrec);
    qr2_expire_ip_verify(qrec);
    NNThink();
}

void qr2_check_queries(qr2_t qrec) {
    SOCSockAddrIn saddr;
    int saddrlen = sizeof(SOCSockAddrIn);
    int error;

    if (!qrec->readSocket) {
        return;
    }
    while (CanReceiveOnSocket(qrec->hbSock)) {
        error = recvfrom(qrec->hbSock, indata, sizeof(indata) - 1, 0, (SOCKADDR *)&saddr, &saddrlen);
        if (error != -1) {
            indata[error] = '\0';
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "Received %d bytes on query socket\r\n", error);
            gsDebugBinary(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_RawDump, indata, error);
            qr2_parse_queryA(qrec, indata, error, (SOCKADDR *)&saddr);
        } else if (error == 0) {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "CanReceiveOnSocket() returned true, but recvfrom return 0, socket closed!\r\n", error);
        } else {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                          "CanReceiveOnSocket() returned true, but recvfrom failed, error: %d!\r\n",
                          GOAGetLastError(qrec->hbSock));
        }
    }
}

void qr2_check_send_heartbeat(qr2_t qrec) {
    gsi_u32 tc = current_time();

    if (qrec->hbSock == -1) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError, "HBSock is invalid\r\n");
        return;
    }
    if (qrec->listedState > 0 && tc - qrec->lastHeartbeat > 10000) {
        if (qrec->listedState >= 4) {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_HotError,
                          "No response from master, generating NoChallengeResponse error\r\n");
            qrec->listedState = 0;
            qrec->addErrorCallback(5, "No challenge value was received from the master server.", qrec->userData);
            return;
        }
        send_heartbeat(qrec, 3);
        qrec->listedState++;
    } else if (qrec->userStateChangeRequested && tc - qrec->lastHeartbeat > 10000) {
        send_heartbeat(qrec, 1);
    } else if (tc - qrec->lastHeartbeat > 60000 || qrec->lastHeartbeat == 0 || tc < qrec->lastHeartbeat) {
        send_heartbeat(qrec, 0);
    }
    if (current_time() - qrec->lastKa > 20000) {
        send_keepalive(qrec);
    }
}

void qr2_send_statechanged(qr2_t qrec) {
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_StackTrace, "qr2_send_statechanged()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    if (!qrec->isPublic) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_Warning,
                      "Requested send statechange for LAN game, discarding\r\n");
        return;
    }
    if (current_time() - qrec->lastHeartbeat < 10000) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_Notice,
                      "Queing statechange for later send (too soon)\r\n");
        qrec->userStateChangeRequested = gsi_true;
        return;
    }
    send_heartbeat(qrec, 1);
    qrec->userStateChangeRequested = gsi_false;
}

void qr2_shutdown(qr2_t qrec) {
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_StackTrace, "qr2_shutdown()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    if (qrec->isPublic) {
        send_heartbeat(qrec, 2);
    }
    if (qrec->hbSock != -1 && qrec->readSocket) {
        closesocket(qrec->hbSock);
    }
    qrec->hbSock = -1;
    qrec->lastHeartbeat = 0;
    if (qrec->readSocket) {
        SocketShutDown();
    }
    if (qrec->ccCallback != NULL) {
        NNFreeNegotiateList();
    }
    if (qrec != &static_qr2_rec) {
        gsifree(qrec);
    }
}

gsi_bool qr2_keybuffer_add(qr2_keybuffer_t keyBuffer, int keyId) {
    if (keyBuffer->numkeys >= MAX_REGISTERED_KEYS) {
        return gsi_false;
    }
    if (keyId < 1 || keyId > MAX_REGISTERED_KEYS) {
        return gsi_false;
    }
    keyBuffer->keys[keyBuffer->numkeys++] = keyId;
    return gsi_true;
}

gsi_bool qr2_buffer_add_int(qr2_buffer_t outBuf, int value) {
    char temp[20];

    OS_SPrintf(temp, "%d", value);
    return qr2_buffer_addA(outBuf, temp);
}

gsi_bool qr2_buffer_addA(qr2_buffer_t outBuf, const char *value) {
    int copylen = strlen(value) + 1;

    if (copylen > MAX_DATA_SIZE - outBuf->len) {
        copylen = MAX_DATA_SIZE - outBuf->len;
    }
    if (copylen <= 0) {
        return gsi_false;
    }
    memcpy(outBuf->buffer + outBuf->len, value, copylen);
    outBuf->len += copylen;
    outBuf->buffer[outBuf->len - 1] = '\0';
    return gsi_true;
}

static void enum_local_ips(void) {
    SOCHostEnt *phost = getlocalhost();

    if (phost == NULL) {
        return;
    }
    for (num_local_ips = 0; num_local_ips < 5; num_local_ips++) {
        if (phost->addrList[num_local_ips] == NULL) {
            break;
        }
        memcpy(&local_ip_list[num_local_ips], phost->addrList[num_local_ips], sizeof(SOCInAddr));
    }
}

// The address of a host (four numbers or a name to look up) at a port, and the host's entry in savehent
static gsi_bool get_sockaddrin(const char *host, int port, SOCKADDR_IN *saddr, HOSTENT **savehent) {
    SOCHostEnt *hent = NULL;

    memset(saddr, 0, sizeof(SOCSockAddrIn));
    saddr->family = 2;
    saddr->port = QR2_SWAP16((gsi_u16)port);
    if (host == NULL) {
        saddr->addr.addr = 0;
    } else {
        saddr->addr.addr = inet_addr(host);
    }
    if (saddr->addr.addr == -1 && strcmp(host, "255.255.255.255") != 0) {
        hent = SOC_GetHostByName(host);
        if (hent == NULL) {
            return gsi_false;
        }
        saddr->addr.addr = *(gsi_u32 *)hent->addrList[0];
    }
    if (savehent != NULL) {
        *savehent = hent;
    }
    return gsi_true;
}

static void swap_byte(unsigned char *a, unsigned char *b) {
    gsi_u8 swapByte = *a;

    *a = *b;
    *b = swapByte;
}

// The character of a base 64 digit
static unsigned char encode_ct(unsigned char c) {
    if (c < 26) {
        return 'A' + c;
    }
    if (c < 52) {
        return 'a' + c - 26;
    }
    if (c < 62) {
        return '0' + c - 52;
    }
    if (c == 62) {
        return '+';
    }
    if (c == 63) {
        return '/';
    }
    return 0;
}

// The bytes as base 64 digits
static void gs_encode(const unsigned char *ins, int size, unsigned char *result) {
    int i;
    int pos;
    gsi_u8 trip[3];
    gsi_u8 kwart[4];

    i = 0;
    while (i < size) {
        for (pos = 0; pos <= 2; pos++, i++) {
            if (i < size) {
                trip[pos] = *ins++;
            } else {
                trip[pos] = '\0';
            }
        }
        kwart[0] = trip[0] >> 2;
        kwart[1] = ((trip[0] & 3) << 4) + (trip[1] >> 4);
        kwart[2] = ((trip[1] & 15) << 2) + (trip[2] >> 6);
        kwart[3] = trip[2] & 63;
        for (pos = 0; pos <= 3; pos++) {
            *result++ = encode_ct(kwart[pos]);
        }
    }
    *result = '\0';
}

// The bytes encrypted in place with the key (RC4 with a twist)
static void gs_encrypt(const unsigned char *key, int keyLen, unsigned char *bufferPtr, int bufferLen) {
    short counter;
    gsi_u8 x;
    gsi_u8 y;
    gsi_u8 xorIndex;
    gsi_u8 state[256];

    for (counter = 0; counter < 256; counter++) {
        state[counter] = counter;
    }
    x = 0;
    y = 0;
    for (counter = 0; counter < 256; counter++) {
        y = (key[x] + state[counter] + y) % 256;
        x = (x + 1) % keyLen;
        swap_byte(&state[counter], &state[y]);
    }
    x = 0;
    y = 0;
    for (counter = 0; counter < bufferLen; counter++) {
        x = (x + bufferPtr[counter] + 1) % 256;
        y = (state[x] + y) % 256;
        swap_byte(&state[x], &state[y]);
        xorIndex = (state[x] + state[y]) % 256;
        bufferPtr[counter] ^= state[xorIndex];
    }
}

static void NatNegProgressCallback(NegotiateState state, void *userData) {
}

static void NatNegCompletedCallback(NegotiateResult result, SOCKET gameSocket, SOCKADDR_IN *remoteAddr,
                                    void *userData) {
    qr2_t qrec = userData;

    if (qrec->ccCallback != NULL && result == 0) {
        qrec->ccCallback(gameSocket, remoteAddr, qrec->userData);
    }
}

static void qr_add_packet_header(qr2_buffer_t buf, char ptype, char *reqkey) {
    buf->buffer[0] = ptype;
    memcpy(buf->buffer + 1, reqkey, REQUEST_KEY_LEN);
    buf->len = REQUEST_KEY_LEN + 1;
}

static void compute_challenge_response(qr2_t qrec, qr2_buffer_t buf, char *challenge, int challengeLen) {
    char encryptedVal[65];

    if (challengeLen < 1) {
        return;
    }
    if (challengeLen > 65) {
        return;
    }
    if (challenge[challengeLen - 1] != '\0') {
        return;
    }
    gsiSafeStrcpyA(encryptedVal, challenge, sizeof(encryptedVal));
    gs_encrypt((const gsi_u8 *)qrec->secretKey, strlen(qrec->secretKey), (gsi_u8 *)encryptedVal, challengeLen - 1);
    gs_encode((const gsi_u8 *)encryptedVal, challengeLen - 1, buf->buffer + buf->len);
    buf->len += strlen(buf->buffer + buf->len) + 1;
}

// Takes the public address that the master server saw this side at, in 12 hexadecimal digits
static void handle_public_address(qr2_t qrec, char *buffer) {
    gsi_u32 ip;
    gsi_u32 portTemp;
    gsi_u16 port;

    STD_TSScanf(buffer, "%08X%04X", &ip, &portTemp);
    port = portTemp;
    ip = QR2_SWAP32(ip);
    if (ip != 0 && port != 0) {
        SOCInAddr addr;

        addr.addr = ip;
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "Received public address (%s:%d)\r\n", SOC_InetNtoA(addr), port);
        if (qrec->publicIp != ip || qrec->publicPort != port) {
            qrec->publicIp = ip;
            qrec->publicPort = port;
            qrec->paCallback(ip, port, qrec->userData);
        }
    }
}

static void qr_build_partial_query_reply(qr2_t qrec, qr2_buffer_t buf, qr2_key_type keyType, int keyCount,
                                         const unsigned char *keys) {
    struct qr2_keybuffer_s kb;
    int playerTeamCount;
    gsi_u16 ctTemp;
    int i;
    int pindex;
    const char *k;
    int len;

    kb.numkeys = 0;
    if (keyCount == 0) {
        return;
    }
    if (keyType == 1 || keyType == 2) {
        if (MAX_DATA_SIZE - buf->len < 2) {
            return;
        }
        playerTeamCount = qrec->playerTeamCountCallback(keyType, qrec->userData);
        ctTemp = QR2_SWAP16((gsi_u16)playerTeamCount);
        memcpy(buf->buffer + buf->len, &ctTemp, sizeof(ctTemp));
        buf->len += sizeof(ctTemp);
    } else {
        playerTeamCount = 1;
    }
    if (keyCount == 0xff) {
        qrec->keyListCallback(keyType, &kb, qrec->userData);
        for (i = 0; i < kb.numkeys; i++) {
            k = qr2_registered_key_list[kb.keys[i]];
            if (k == NULL) {
                k = "unknown";
            }
            qr2_buffer_addA(buf, k);
            if (keyType == 0) {
                len = buf->len;
                qrec->serverKeyCallback(kb.keys[i], buf, qrec->userData);
                if (len == buf->len) {
                    qr2_buffer_addA(buf, "");
                }
            }
        }
        if (MAX_DATA_SIZE - buf->len < 1) {
            return;
        }
        buf->buffer[buf->len++] = '\0';
        keyCount = kb.numkeys;
        keys = kb.keys;
        if (keyType == 0) {
            return;
        }
    }
    for (pindex = 0; pindex < playerTeamCount; pindex++) {
        for (i = 0; i < keyCount; i++) {
            len = buf->len;
            if (keyType == 0) {
                qrec->serverKeyCallback(keys[i], buf, qrec->userData);
            } else if (keyType == 1) {
                qrec->playerKeyCallback(keys[i], pindex, buf, qrec->userData);
            } else if (keyType == 2) {
                qrec->teamKeyCallback(keys[i], pindex, buf, qrec->userData);
            }
            if (len == buf->len) {
                qr2_buffer_addA(buf, "");
            }
        }
    }
}

static void qr_build_query_reply(qr2_t qrec, qr2_buffer_t buf, int serverKeyCount, const unsigned char *serverKeys,
                                 int playerKeyCount, const unsigned char *playerKeys, int teamKeyCount,
                                 const unsigned char *teamKeys) {
    qr_build_partial_query_reply(qrec, buf, key_server, serverKeyCount, serverKeys);
    qr_build_partial_query_reply(qrec, buf, key_player, playerKeyCount, playerKeys);
    qr_build_partial_query_reply(qrec, buf, key_team, teamKeyCount, teamKeys);
}

// Adds the next part of a reply that takes several packets: gsi_true when there is a packet to send
static gsi_bool qr_build_split_query_reply(qr2_t qrec, qr2_buffer_t buf, QRSplitQueryProgress *progress) {
    gsi_u8 *packetNumPos = NULL;

    if (progress->mCurKeyType >= key_type_count) {
        return gsi_false;
    }
    if (MAX_DATA_SIZE - buf->len < 32) {
        return gsi_false;
    }
    qr2_buffer_addA(buf, "splitnum");
    packetNumPos = (gsi_u8 *)&buf->buffer[buf->len++];
    *packetNumPos = progress->mCurPacketNum++;
    while (progress->mCurKeyType < key_type_count) {
        if (progress->mKeyBuffer.numkeys == 0) {
            qrec->keyListCallback(progress->mCurKeyType, &progress->mKeyBuffer, qrec->userData);
        }
        if (progress->mCurSubCount == 0 && progress->mCurKeyType != key_server) {
            progress->mCurSubCount = qrec->playerTeamCountCallback(progress->mCurKeyType, qrec->userData);
        }
        if (MAX_DATA_SIZE - buf->len < 100) {
            return gsi_true;
        }
        buf->buffer[buf->len++] = progress->mCurKeyType;
        while (progress->mCurKeyIndex < progress->mKeyBuffer.numkeys) {
            int registeredKeyIndex = progress->mKeyBuffer.keys[progress->mCurKeyIndex];
            const char *keyName = qr2_registered_key_list[registeredKeyIndex];

            if (!qr2_buffer_addA(buf, keyName)) {
                return gsi_true;
            }
            if (progress->mCurKeyType == key_server) {
                qrec->serverKeyCallback(registeredKeyIndex, buf, qrec->userData);
                if (MAX_DATA_SIZE - buf->len < 1) {
                    return gsi_true;
                }
            } else {
                if (MAX_DATA_SIZE - buf->len < 1) {
                    return gsi_true;
                }
                buf->buffer[buf->len++] = progress->mCurSubIndex;
                while (progress->mCurSubIndex < progress->mCurSubCount) {
                    if (progress->mCurKeyType == key_player) {
                        qrec->playerKeyCallback(registeredKeyIndex, progress->mCurSubIndex, buf, qrec->userData);
                    } else if (progress->mCurKeyType == key_team) {
                        qrec->teamKeyCallback(registeredKeyIndex, progress->mCurSubIndex, buf, qrec->userData);
                    }
                    if (MAX_DATA_SIZE - buf->len < 1) {
                        return gsi_true;
                    }
                    progress->mCurSubIndex++;
                }
                if (MAX_DATA_SIZE - buf->len > 0) {
                    buf->buffer[buf->len++] = '\0';
                }
            }
            progress->mCurKeyIndex++;
            progress->mCurSubIndex = 0;
        }
        if (MAX_DATA_SIZE - buf->len > 0) {
            buf->buffer[buf->len++] = '\0';
        }
        progress->mCurKeyType++;
        progress->mCurKeyIndex = 0;
        progress->mCurSubCount = 0;
        progress->mCurSubIndex = 0;
        progress->mKeyBuffer.numkeys = 0;
    }
    *packetNumPos |= 0x80;
    return gsi_true;
}

static void qr_process_query(qr2_t qrec, qr2_buffer_t buf, unsigned char *qdata, int len, SOCKADDR *sender) {
    gsi_u8 serverKeyCount;
    gsi_u8 playerKeyCount;
    gsi_u8 teamKeyCount;
    gsi_u8 exFlags = 0;
    const gsi_u8 *serverKeys = NULL;
    const gsi_u8 *playerKeys = NULL;
    const gsi_u8 *teamKeys = NULL;

    if (len < 3) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "Discarding invalid query (too short#1: %d bytes total)\r\n", len);
        return;
    }
    serverKeyCount = qdata[0];
    qdata++;
    len--;
    if (serverKeyCount != 0 && serverKeyCount != 0xff) {
        serverKeys = qdata;
        qdata += serverKeyCount;
        len -= serverKeyCount;
    }
    if (len < 2) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "Discarding invalid query (too short#2: %d bytes remain)\r\n", len);
        return;
    }
    playerKeyCount = qdata[0];
    qdata++;
    len--;
    if (playerKeyCount != 0 && playerKeyCount != 0xff) {
        playerKeys = qdata;
        qdata += playerKeyCount;
        len -= playerKeyCount;
    }
    if (len < 1) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "Discarding invalid query (too short#3: %d bytes remain)\r\n", len);
        return;
    }
    teamKeyCount = qdata[0];
    qdata++;
    len--;
    if (teamKeyCount != 0 && teamKeyCount != 0xff) {
        teamKeys = qdata;
        qdata += teamKeyCount;
        len -= teamKeyCount;
    }
    if (len < 0) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError,
                      "Discarding invalid query (too short#4: %d bytes remain)\r\n", len);
        return;
    }
    if (len > 0) {
        exFlags = qdata[0];
        len--;
    }
    if ((exFlags & 1) == 1) {
        QRSplitQueryProgress progress;

        progress.mCurPacketNum = 0;
        progress.mCurKeyType = key_server;
        progress.mCurKeyIndex = 0;
        progress.mCurSubCount = 0;
        progress.mCurSubIndex = 0;
        progress.mKeyBuffer.numkeys = 0;
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "Building query reply (split packet supported)\r\n");
        while (qr_build_split_query_reply(qrec, buf, &progress) == gsi_true) {
            sendto(qrec->hbSock, buf->buffer, buf->len, 0, (SOCKADDR *)sender, sizeof(SOCSockAddrIn));
            buf->len = 5;
            if (progress.mCurPacketNum > 7) {
                return;
            }
        }
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "Finished split query reply (%d packets)\r\n", progress.mCurPacketNum);
    } else {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                      "Building query reply (single packet)\r\n");
        qr_build_query_reply(qrec, buf, serverKeyCount, serverKeys, playerKeyCount, playerKeys, teamKeyCount, teamKeys);
        sendto(qrec->hbSock, buf->buffer, buf->len, 0, (SOCKADDR *)sender, sizeof(SOCSockAddrIn));
    }
}

// A message from a client through the master server: a NAT negotiation's cookie after the magic bytes, or the game's
static void qr_process_client_message(qr2_t qrec, char *buf, int len) {
    gsi_u8 natNegBytes[NATNEG_MAGIC_LEN] = { 0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2 };
    int i;
    gsi_bool isNatNeg = gsi_true;

    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_Notice, "Processing client message\r\n");
    if (len >= NATNEG_MAGIC_LEN + 4) {
        for (i = 0; i < NATNEG_MAGIC_LEN; i++) {
            if ((gsi_u8)buf[i] != natNegBytes[i]) {
                isNatNeg = gsi_false;
                break;
            }
        }
    } else {
        isNatNeg = gsi_false;
    }
    if (isNatNeg) {
        int cookie;

        memcpy(&cookie, buf + NATNEG_MAGIC_LEN, 4);
        cookie = QR2_SWAP32(cookie);
        if (qrec->ccCallback != NULL) {
            if (__GSIACResult != GSIACAvailable) {
                __GSIACResult = GSIACAvailable;
                gsiSafeStrcpyA(__GSIACGamename, qrec->gamename, sizeof(__GSIACGamename));
            }
            NNBeginNegotiationWithSocket(qrec->hbSock, cookie, 1, NatNegProgressCallback, NatNegCompletedCallback,
                                         qrec);
        } else if (qrec->nnCallback != NULL) {
            qrec->nnCallback(cookie, qrec->userData);
        }
    } else if (qrec->cmCallback != NULL) {
        qrec->cmCallback(buf, len, qrec->userData);
    }
}

// Whether a client message's key was seen lately; if not, keeps it
static int qr_got_recent_message(qr2_t qrec, int msgKey) {
    int i;

    for (i = 0; i < RECENT_CLIENT_MESSAGES_TO_TRACK; i++) {
        if (qrec->clientMessageKeys[i] == msgKey) {
            return gsi_true;
        }
    }
    qrec->curMessageKey = (qrec->curMessageKey + 1) % RECENT_CLIENT_MESSAGES_TO_TRACK;
    qrec->clientMessageKeys[qrec->curMessageKey] = msgKey;
    return gsi_false;
}

// Adds a random challenge for a host to the reply, which its query has to give back
static gsi_bool qr2_process_ip_verify(qr2_t qrec, struct qr2_buffer_s *buf, SOCKADDR_IN *sender) {
    int i = 0;
    gsi_u32 now = current_time();
    int firstFreeIndex = -1;
    int numDuplicates = 0;

    if ((qrec->backendOptions & 0x80) == 0) {
        qr2_buffer_add_int(buf, 0);
        return gsi_true;
    }
    for (; i < QR2_IPVERIFY_ARRAY_SIZE; i++) {
        if (firstFreeIndex == -1 && qrec->ipVerify[i].addr.addr.addr == 0) {
            firstFreeIndex = i;
        }
        if (qrec->ipVerify[i].addr.addr.addr == sender->addr.addr && qrec->ipVerify[i].addr.port == sender->port) {
            numDuplicates++;
        }
    }
    if (numDuplicates > 5) {
        return gsi_false;
    }
    if (firstFreeIndex == -1) {
        return gsi_false;
    }
    qrec->ipVerify[firstFreeIndex].addr = *sender;
    qrec->ipVerify[firstFreeIndex].challenge = QR2_SWAP32((rand() << 16) | rand());
    qrec->ipVerify[firstFreeIndex].createtime = now;
    qr2_buffer_add_int(buf, qrec->ipVerify[firstFreeIndex].challenge);
    return gsi_true;
}

// Whether a host gave back the challenge it was sent, which is then forgotten
static gsi_bool qr2_check_ip_verify(qr2_t qrec, SOCKADDR_IN *sender, gsi_u32 ipVerify) {
    int i = 0;

    for (; i < QR2_IPVERIFY_ARRAY_SIZE; i++) {
        if (qrec->ipVerify[i].addr.addr.addr == sender->addr.addr && qrec->ipVerify[i].addr.port == sender->port) {
            if (qrec->ipVerify[i].challenge == ipVerify) {
                qrec->ipVerify[i].addr.addr.addr = 0;
                qrec->ipVerify[i].addr.port = 0;
                return gsi_true;
            }
        }
    }
    return gsi_false;
}

// Forgets the challenges that weren't given back within 4 seconds
static void qr2_expire_ip_verify(qr2_t qrec) {
    int i = 0;
    gsi_u32 now = current_time();

    for (; i < QR2_IPVERIFY_ARRAY_SIZE; i++) {
        if (qrec->ipVerify[i].addr.addr.addr != 0 && now - qrec->ipVerify[i].createtime > 4000) {
            qrec->ipVerify[i].addr.addr.addr = 0;
        }
    }
}

void qr2_parse_queryA(qr2_t qrec, char *query, int len, SOCKADDR *sender) {
    struct qr2_buffer_s buf;
    char ptype;
    char *reqkey;
    char *pos;
    gsi_u32 ipVerify = 0;
    int i;

    buf.len = 0;
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_StackTrace, "qr2_parse_queryA()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    if (query[0] == ';') {
        if (qrec->cdkeyProcess != NULL) {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice,
                          "Forwarding cdkey query onto cdkey sdk\r\n");
            qrec->cdkeyProcess(query, len, sender);
        } else {
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError,
                          "Received cdkey query but not using qr2-cdkey integration!\r\n");
        }
        return;
    }
    if (len >= 6 && memcmp(query, NNMagicData, NATNEG_MAGIC_LEN) == 0) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "Forwarding natneg query onto natneg sdk\r\n");
        NNProcessData(query, len, (SOCKADDR_IN *)sender);
        return;
    }
    if (len < 7) {
        return;
    }
    if ((gsi_u8)query[0] != 0xfe || (gsi_u8)query[1] != 0xfd) {
        return;
    }
    if (qrec->listedState > 0) {
        qrec->listedState = 0;
    }
    ptype = query[2];
    reqkey = &query[3];
    pos = query + 7;
    len -= 7;
    qr_add_packet_header(&buf, ptype, reqkey);
    switch (ptype) {
    case 9:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "Received IP verify challenge request\r\n");
        if (qr2_process_ip_verify(qrec, &buf, (SOCKADDR_IN *)sender)) {
            break;
        }
        return;
    case 0:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice, "Processing query packet\r\n");
        if ((qrec->backendOptions & 0x80) == 0x80) {
            if (len < 4) {
                return;
            }
            ipVerify = QR2_SWAP32(*(gsi_u32 *)pos);
            pos += 4;
            len -= 4;
            if (!qr2_check_ip_verify(qrec, (SOCKADDR_IN *)sender, ipVerify)) {
                return;
            }
        }
        qr_process_query(qrec, &buf, (gsi_u8 *)pos, len, sender);
        return;
    case 1:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice, "Processing challenge packet\r\n");
        for (i = 0; i < REQUEST_KEY_LEN; i++) {
            if (reqkey[i] != qrec->instanceKey[i]) {
                return;
            }
        }
        if (len >= 15) {
            gsi_u32 backendOptions;

            STD_TSScanf(pos + len - 15, "%02x", &backendOptions);
            qrec->backendOptions = backendOptions;
            gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_Notice, "Received setting options: %d\r\n",
                          qrec->backendOptions);
            if (qrec->paCallback != NULL) {
                handle_public_address(qrec, pos + len - 13);
            } else {
                gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice,
                              "Discarding public address (no callback set)\r\n");
            }
        }
        compute_challenge_response(qrec, &buf, pos, len);
        break;
    case 2:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice, "Processing echo packet\r\n");
        if (len > 32) {
            len = 32;
        }
        buf.buffer[0] = 5;
        memcpy(buf.buffer + buf.len, pos, len);
        buf.len += len;
        break;
    case 4:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_WarmError, "Processing adderror packet\r\n");
        if (qrec->listedState == -1) {
            return;
        }
        for (i = 0; i < REQUEST_KEY_LEN; i++) {
            if (reqkey[i] != qrec->instanceKey[i]) {
                return;
            }
        }
        if (len < 2) {
            return;
        }
        qrec->listedState = -1;
        qrec->addErrorCallback(*pos, pos + 1, qrec->userData);
        return;
    case 6:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Notice,
                      "Processing clientmessage packet\r\n");
        for (i = 0; i < REQUEST_KEY_LEN; i++) {
            if (reqkey[i] != qrec->instanceKey[i]) {
                return;
            }
        }
        if (len < 4) {
            return;
        }
        buf.buffer[0] = 7;
        memcpy(buf.buffer + buf.len, pos, 4);
        buf.len += 4;
        memcpy(&i, pos, 4);
        if (!qr_got_recent_message(qrec, i)) {
            qr_process_client_message(qrec, pos + 4, len - 4);
        }
        break;
    case 8:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment, "Processing keepalive packet\r\n");
        return;
    case 10:
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment, "Processing client packet\r\n");
        if (qrec->crCallback != NULL) {
            qrec->crCallback(qrec->userData);
        }
        return;
    default:
        return;
    }
    sendto(qrec->hbSock, buf.buffer, buf.len, 0, (SOCKADDR *)sender, sizeof(SOCSockAddrIn));
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "Sent %d bytes as QR2 query response\r\n", buf.len);
    gsDebugBinary(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_RawDump, buf.buffer, buf.len);
}

static void send_keepalive(qr2_t qrec) {
    struct qr2_buffer_s buf;

    buf.len = 0;
    qr_add_packet_header(&buf, 8, qrec->instanceKey);
    sendto(qrec->hbSock, buf.buffer, buf.len, 0, (SOCKADDR *)&qrec->hbAddr, sizeof(SOCSockAddrIn));
    qrec->lastKa = current_time();
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment, "Sent keepalive to master\r\n", buf);
}

// state changed is 0 for the periodic heartbeat, 1 for a change of the game's state, 2 when it shuts down and 3 while
// it waits for the master server's challenge
static void send_heartbeat(qr2_t qrec, int stateChanged) {
    struct qr2_buffer_s buf;
    int i;
    char ipkey[20];

    buf.len = 0;
    qr_add_packet_header(&buf, 3, qrec->instanceKey);
    for (i = 0; i < num_local_ips; i++) {
        OS_SPrintf(ipkey, "localip%d", i);
        qr2_buffer_addA(&buf, ipkey);
        qr2_buffer_addA(&buf, SOC_InetNtoA(local_ip_list[i]));
    }
    qr2_buffer_addA(&buf, "localport");
    qr2_buffer_add_int(&buf, qrec->qport);
    qr2_buffer_addA(&buf, "natneg");
    qr2_buffer_addA(&buf, qrec->natNegotiate ? "1" : "0");
    if (stateChanged) {
        qr2_buffer_addA(&buf, "statechanged");
        qr2_buffer_add_int(&buf, stateChanged);
    }
    qr2_buffer_addA(&buf, "gamename");
    qr2_buffer_addA(&buf, qrec->gamename);
    if (qrec->paCallback != NULL) {
        qr2_buffer_addA(&buf, "publicip");
        qr2_buffer_add_int(&buf, qrec->publicIp);
        qr2_buffer_addA(&buf, "publicport");
        qr2_buffer_add_int(&buf, qrec->publicPort);
    }
    if (stateChanged != 2) {
        // The keys go into a copy first, so that a reply too big for the packet can be built again without the
        // players' and the teams' keys
        struct qr2_buffer_s temp;

        memcpy(temp.buffer, buf.buffer, buf.len);
        temp.len = buf.len;
        qr_build_query_reply(qrec, &temp, 0xff, NULL, 0xff, NULL, 0xff, NULL);
        if (MAX_DATA_SIZE - temp.len < 1) {
            temp.len = buf.len;
            qr_build_query_reply(qrec, &temp, 0xff, NULL, 0, NULL, 0, NULL);
        }
        memcpy(buf.buffer, temp.buffer, temp.len);
        buf.len = temp.len;
    } else if (MAX_DATA_SIZE - buf.len >= 1) {
        buf.buffer[buf.len++] = '\0';
    }
    sendto(qrec->hbSock, buf.buffer, buf.len, 0, (SOCKADDR *)&qrec->hbAddr, sizeof(SOCSockAddrIn));
    qrec->lastKa = qrec->lastHeartbeat = current_time();
    if (stateChanged != 0) {
        qrec->userStateChangeRequested = gsi_false;
    }
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_Comment,
                  "Sent heartbeat to master (size %d)\r\n", buf.len);
    gsDebugBinary(GSIDebugCat_QR2, GSIDebugType_Network, GSIDebugLevel_RawDump, buf.buffer, buf.len);
}
