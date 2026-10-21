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
// the other functions, the types and the data are named after the later SDK; qr2_get_sockaddrin is the SDK's
// get_sockaddrin, whose name gsAvailable.c's has

// Where a reply that takes several packets has got to
typedef struct QRSplitQueryProgress {
    QR2KeyType curKeyType;
    int curPacketNum;
    int curKeyIndex;
    int curSubCount; // the players or the teams
    int curSubIndex;
    QR2KeyBuffer keyBuffer;
} QRSplitQueryProgress;

static void enum_local_ips(void);
static BOOL qr2_get_sockaddrin(const char *host, int port, SocAddrIn *saddr, SocHostEnt **savehent);
static void swap_byte(u8 *a, u8 *b);
static u8 encode_ct(u8 c);
static void gs_encode(const u8 *ins, int size, char *result);
static void gs_encrypt(const u8 *key, int keyLen, u8 *bufferPtr, int bufferLen);
static void NatNegProgressCallback(int state, void *userData);
static void NatNegCompletedCallback(int result, int gameSocket, SocAddrIn *remoteAddr, void *userData);
static void qr_add_packet_header(QR2Buffer *buf, char ptype, const char *reqkey);
static void compute_challenge_response(QR2Implementation *qrec, QR2Buffer *buf, const char *challenge,
                                       int challengeLen);
static void handle_public_address(QR2Implementation *qrec, const char *buffer);
static void qr_build_partial_query_reply(QR2Implementation *qrec, QR2Buffer *buf, int keyType, int keyCount,
                                         const u8 *keys);
static void qr_build_query_reply(QR2Implementation *qrec, QR2Buffer *buf, int serverKeyCount, const u8 *serverKeys,
                                 int playerKeyCount, const u8 *playerKeys, int teamKeyCount, const u8 *teamKeys);
static BOOL qr_build_split_query_reply(QR2Implementation *qrec, QR2Buffer *buf, QRSplitQueryProgress *progress);
static void qr_process_query(QR2Implementation *qrec, QR2Buffer *buf, const u8 *qdata, int len, SocAddrIn *sender);
static void qr_process_client_message(QR2Implementation *qrec, const char *buf, int len);
static BOOL qr_got_recent_message(QR2Implementation *qrec, int msgKey);
static BOOL qr2_process_ip_verify(QR2Implementation *qrec, QR2Buffer *buf, SocAddrIn *sender);
static BOOL qr2_check_ip_verify(QR2Implementation *qrec, SocAddrIn *sender, u32 ipVerify);
static void qr2_expire_ip_verify(QR2Implementation *qrec);
static void send_keepalive(QR2Implementation *qrec);
static void send_heartbeat(QR2Implementation *qrec, int stateChanged);

QR2Implementation static_qr2_rec = { -1 };
static QR2Implementation *current_rec = &static_qr2_rec;
// MWCC lays out the .bss in the reverse order of the definitions, and the ROM has num_local_ips first and the buffer
// that the queries are read into last
static char qr2_indata[256];
char qr2_hostname[64];
static SocInAddr local_ip_list[5];
static int num_local_ips;

int qr2_init_socketA(QR2Implementation **qrec, int sock, int qport, const char *gamename, const char *secretKey,
                     BOOL isPublic, BOOL natNegotiate, QR2ServerKeyCallback serverKeyCallback,
                     QR2PlayerTeamKeyCallback playerKeyCallback, QR2PlayerTeamKeyCallback teamKeyCallback,
                     QR2KeyListCallback keyListCallback, QR2CountCallback playerTeamCountCallback,
                     QR2AddErrorCallback addErrorCallback, void *userData) {
    char hostname[64];
    int ret;
    int i;
    QR2Implementation *cr;

    gsDebugFormat(3, 4, 0x40, "qr2_init_socket()\r\n");
    if (qrec == NULL) {
        cr = &static_qr2_rec;
    } else {
        cr = gsimalloc(sizeof(QR2Implementation));
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
    cr->readSocket = FALSE;
    cr->natNegotiate = natNegotiate;
    cr->publicIp = 0;
    cr->publicPort = 0;
    cr->paCallback = NULL;
    cr->crCallback = NULL;
    cr->ccCallback = NULL;
    cr->userStateChangeRequested = FALSE;
    cr->backendOptions = 0;
    for (i = 0; i < QR2_REQUEST_KEY_LEN; i++) {
        cr->instanceKey[i] = rand() % 0xff;
    }
    for (i = 0; i < QR2_RECENT_CLIENT_MESSAGES_TO_TRACK; i++) {
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
        ret = qr2_get_sockaddrin(override ? qr2_hostname : hostname, 27900, &cr->hbAddr, NULL);
        if (ret == 1) {
            gsDebugFormat(3, 0, 0x10, "%s resolved to %s\r\n", override ? qr2_hostname : hostname,
                          SOC_InetNtoA(*(SocInAddr *)&cr->hbAddr.addr));
        } else {
            gsDebugFormat(3, 0, 1, "Failed on DNS lookup for %s \r\n", override ? qr2_hostname : hostname);
        }
    } else {
        ret = 1;
    }
    if (!ret) {
        gsDebugFormat(3, 4, 1, "qr2_init_socket() returned failed (DNS error)\r\n");
        return 3;
    }
    return 0;
}

void qr2_register_natneg_callback(QR2Implementation *qrec, QR2NatNegCallback callback) {
    gsDebugFormat(3, 4, 0x40, "qr2_register_natneg_callback()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    qrec->nnCallback = callback;
}

void qr2_register_clientmessage_callback(QR2Implementation *qrec, QR2ClientMessageCallback callback) {
    gsDebugFormat(3, 4, 0x40, "qr2_register_clientmessage_callback()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    qrec->cmCallback = callback;
}

void qr2_register_publicaddress_callback(QR2Implementation *qrec, QR2PublicAddressCallback callback) {
    gsDebugFormat(3, 4, 0x40, "qr2_register_publicaddress_callback()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    qrec->paCallback = callback;
}

void qr2_think(QR2Implementation *qrec) {
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

void qr2_check_queries(QR2Implementation *qrec) {
    SocAddrIn saddr;
    int saddrlen = sizeof(SocAddrIn);
    int error;

    if (!qrec->readSocket) {
        return;
    }
    while (CanReceiveOnSocket(qrec->hbSock)) {
        error = recvfrom(qrec->hbSock, qr2_indata, sizeof(qr2_indata) - 1, 0, &saddr, &saddrlen);
        if (error != -1) {
            qr2_indata[error] = '\0';
            gsDebugFormat(3, 0, 0x10, "Received %d bytes on query socket\r\n", error);
            gsDebugBinary(3, 0, 0x20, (const u8 *)qr2_indata, error);
            qr2_parse_queryA(qrec, qr2_indata, error, &saddr);
        } else if (error == 0) {
            gsDebugFormat(3, 0, 0x10, "CanReceiveOnSocket() returned true, but recvfrom return 0, socket closed!\r\n",
                          error);
        } else {
            gsDebugFormat(3, 0, 0x10, "CanReceiveOnSocket() returned true, but recvfrom failed, error: %d!\r\n",
                          GOAGetLastError(qrec->hbSock));
        }
    }
}

void qr2_check_send_heartbeat(QR2Implementation *qrec) {
    u32 tc = current_time();

    if (qrec->hbSock == -1) {
        gsDebugFormat(3, 0, 2, "HBSock is invalid\r\n");
        return;
    }
    if (qrec->listedState > 0 && tc - qrec->lastHeartbeat > 10000) {
        if (qrec->listedState >= 4) {
            gsDebugFormat(3, 0, 1, "No response from master, generating NoChallengeResponse error\r\n");
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

void qr2_send_statechanged(QR2Implementation *qrec) {
    gsDebugFormat(3, 4, 0x40, "qr2_send_statechanged()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    if (!qrec->isPublic) {
        gsDebugFormat(3, 4, 4, "Requested send statechange for LAN game, discarding\r\n");
        return;
    }
    if (current_time() - qrec->lastHeartbeat < 10000) {
        gsDebugFormat(3, 4, 8, "Queing statechange for later send (too soon)\r\n");
        qrec->userStateChangeRequested = TRUE;
        return;
    }
    send_heartbeat(qrec, 1);
    qrec->userStateChangeRequested = FALSE;
}

void qr2_shutdown(QR2Implementation *qrec) {
    gsDebugFormat(3, 0, 0x40, "qr2_shutdown()\r\n");
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

BOOL qr2_keybuffer_add(QR2KeyBuffer *keyBuffer, int keyId) {
    if (keyBuffer->numKeys >= MAX_REGISTERED_KEYS) {
        return FALSE;
    }
    if (keyId < 1 || keyId > MAX_REGISTERED_KEYS) {
        return FALSE;
    }
    keyBuffer->keys[keyBuffer->numKeys++] = keyId;
    return TRUE;
}

BOOL qr2_buffer_add_int(QR2Buffer *outBuf, int value) {
    char temp[20];

    OS_SPrintf(temp, "%d", value);
    return qr2_buffer_addA(outBuf, temp);
}

BOOL qr2_buffer_addA(QR2Buffer *outBuf, const char *value) {
    int copylen = strlen(value) + 1;

    if (copylen > QR2_MAX_DATA_SIZE - outBuf->len) {
        copylen = QR2_MAX_DATA_SIZE - outBuf->len;
    }
    if (copylen <= 0) {
        return FALSE;
    }
    memcpy(outBuf->buffer + outBuf->len, value, copylen);
    outBuf->len += copylen;
    outBuf->buffer[outBuf->len - 1] = '\0';
    return TRUE;
}

static void enum_local_ips(void) {
    SocHostEnt *phost = getlocalhost();

    if (phost == NULL) {
        return;
    }
    for (num_local_ips = 0; num_local_ips < 5; num_local_ips++) {
        if (phost->addrList[num_local_ips] == NULL) {
            break;
        }
        memcpy(&local_ip_list[num_local_ips], phost->addrList[num_local_ips], sizeof(SocInAddr));
    }
}

// The address of a host (four numbers or a name to look up) at a port, and the host's entry in savehent
static BOOL qr2_get_sockaddrin(const char *host, int port, SocAddrIn *saddr, SocHostEnt **savehent) {
    SocHostEnt *hent = NULL;

    memset(saddr, 0, sizeof(SocAddrIn));
    saddr->family = 2;
    saddr->port = QR2_SWAP16((u16)port);
    if (host == NULL) {
        saddr->addr = 0;
    } else {
        saddr->addr = inet_addr(host);
    }
    if (saddr->addr == -1 && strcmp(host, "255.255.255.255") != 0) {
        hent = SOC_GetHostByName(host);
        if (hent == NULL) {
            return FALSE;
        }
        saddr->addr = *(u32 *)hent->addrList[0];
    }
    if (savehent != NULL) {
        *savehent = hent;
    }
    return TRUE;
}

static void swap_byte(u8 *a, u8 *b) {
    u8 swapByte = *a;

    *a = *b;
    *b = swapByte;
}

// The character of a base 64 digit
static u8 encode_ct(u8 c) {
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
static void gs_encode(const u8 *ins, int size, char *result) {
    int i;
    int pos;
    u8 trip[3];
    u8 kwart[4];

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
static void gs_encrypt(const u8 *key, int keyLen, u8 *bufferPtr, int bufferLen) {
    short counter;
    u8 x;
    u8 y;
    u8 xorIndex;
    u8 state[256];

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

static void NatNegProgressCallback(int state, void *userData) {
}

static void NatNegCompletedCallback(int result, int gameSocket, SocAddrIn *remoteAddr, void *userData) {
    QR2Implementation *qrec = userData;

    if (qrec->ccCallback != NULL && result == 0) {
        qrec->ccCallback(gameSocket, remoteAddr, qrec->userData);
    }
}

static void qr_add_packet_header(QR2Buffer *buf, char ptype, const char *reqkey) {
    buf->buffer[0] = ptype;
    memcpy(buf->buffer + 1, reqkey, QR2_REQUEST_KEY_LEN);
    buf->len = QR2_REQUEST_KEY_LEN + 1;
}

static void compute_challenge_response(QR2Implementation *qrec, QR2Buffer *buf, const char *challenge,
                                       int challengeLen) {
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
    gs_encrypt((const u8 *)qrec->secretKey, strlen(qrec->secretKey), (u8 *)encryptedVal, challengeLen - 1);
    gs_encode((const u8 *)encryptedVal, challengeLen - 1, buf->buffer + buf->len);
    buf->len += strlen(buf->buffer + buf->len) + 1;
}

// Takes the public address that the master server saw this side at, in 12 hexadecimal digits
static void handle_public_address(QR2Implementation *qrec, const char *buffer) {
    u32 ip;
    u32 portTemp;
    u16 port;

    STD_TSScanf(buffer, "%08X%04X", &ip, &portTemp);
    port = portTemp;
    ip = QR2_SWAP32(ip);
    if (ip != 0 && port != 0) {
        SocInAddr addr;

        addr.addr = ip;
        gsDebugFormat(3, 0, 8, "Received public address (%s:%d)\r\n", SOC_InetNtoA(addr), port);
        if (qrec->publicIp != ip || qrec->publicPort != port) {
            qrec->publicIp = ip;
            qrec->publicPort = port;
            qrec->paCallback(ip, port, qrec->userData);
        }
    }
}

static void qr_build_partial_query_reply(QR2Implementation *qrec, QR2Buffer *buf, int keyType, int keyCount,
                                         const u8 *keys) {
    QR2KeyBuffer kb;
    int playerTeamCount;
    u16 ctTemp;
    int i;
    int pindex;
    const char *k;
    int len;

    kb.numKeys = 0;
    if (keyCount == 0) {
        return;
    }
    if (keyType == 1 || keyType == 2) {
        if (QR2_MAX_DATA_SIZE - buf->len < 2) {
            return;
        }
        playerTeamCount = qrec->playerTeamCountCallback(keyType, qrec->userData);
        ctTemp = QR2_SWAP16((u16)playerTeamCount);
        memcpy(buf->buffer + buf->len, &ctTemp, sizeof(ctTemp));
        buf->len += sizeof(ctTemp);
    } else {
        playerTeamCount = 1;
    }
    if (keyCount == 0xff) {
        qrec->keyListCallback(keyType, &kb, qrec->userData);
        for (i = 0; i < kb.numKeys; i++) {
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
        if (QR2_MAX_DATA_SIZE - buf->len < 1) {
            return;
        }
        buf->buffer[buf->len++] = '\0';
        keyCount = kb.numKeys;
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

static void qr_build_query_reply(QR2Implementation *qrec, QR2Buffer *buf, int serverKeyCount, const u8 *serverKeys,
                                 int playerKeyCount, const u8 *playerKeys, int teamKeyCount, const u8 *teamKeys) {
    qr_build_partial_query_reply(qrec, buf, 0, serverKeyCount, serverKeys);
    qr_build_partial_query_reply(qrec, buf, 1, playerKeyCount, playerKeys);
    qr_build_partial_query_reply(qrec, buf, 2, teamKeyCount, teamKeys);
}

// Adds the next part of a reply that takes several packets: TRUE when there is a packet to send
static BOOL qr_build_split_query_reply(QR2Implementation *qrec, QR2Buffer *buf, QRSplitQueryProgress *progress) {
    u8 *packetNumPos = NULL;

    if (progress->curKeyType >= key_type_count) {
        return FALSE;
    }
    if (QR2_MAX_DATA_SIZE - buf->len < 32) {
        return FALSE;
    }
    qr2_buffer_addA(buf, "splitnum");
    packetNumPos = (u8 *)&buf->buffer[buf->len++];
    *packetNumPos = progress->curPacketNum++;
    while (progress->curKeyType < key_type_count) {
        if (progress->keyBuffer.numKeys == 0) {
            qrec->keyListCallback(progress->curKeyType, &progress->keyBuffer, qrec->userData);
        }
        if (progress->curSubCount == 0 && progress->curKeyType != key_server) {
            progress->curSubCount = qrec->playerTeamCountCallback(progress->curKeyType, qrec->userData);
        }
        if (QR2_MAX_DATA_SIZE - buf->len < 100) {
            return TRUE;
        }
        buf->buffer[buf->len++] = progress->curKeyType;
        while (progress->curKeyIndex < progress->keyBuffer.numKeys) {
            int registeredKeyIndex = progress->keyBuffer.keys[progress->curKeyIndex];
            const char *keyName = qr2_registered_key_list[registeredKeyIndex];

            if (!qr2_buffer_addA(buf, keyName)) {
                return TRUE;
            }
            if (progress->curKeyType == key_server) {
                qrec->serverKeyCallback(registeredKeyIndex, buf, qrec->userData);
                if (QR2_MAX_DATA_SIZE - buf->len < 1) {
                    return TRUE;
                }
            } else {
                if (QR2_MAX_DATA_SIZE - buf->len < 1) {
                    return TRUE;
                }
                buf->buffer[buf->len++] = progress->curSubIndex;
                while (progress->curSubIndex < progress->curSubCount) {
                    if (progress->curKeyType == key_player) {
                        qrec->playerKeyCallback(registeredKeyIndex, progress->curSubIndex, buf, qrec->userData);
                    } else if (progress->curKeyType == key_team) {
                        qrec->teamKeyCallback(registeredKeyIndex, progress->curSubIndex, buf, qrec->userData);
                    }
                    if (QR2_MAX_DATA_SIZE - buf->len < 1) {
                        return TRUE;
                    }
                    progress->curSubIndex++;
                }
                if (QR2_MAX_DATA_SIZE - buf->len > 0) {
                    buf->buffer[buf->len++] = '\0';
                }
            }
            progress->curKeyIndex++;
            progress->curSubIndex = 0;
        }
        if (QR2_MAX_DATA_SIZE - buf->len > 0) {
            buf->buffer[buf->len++] = '\0';
        }
        progress->curKeyType++;
        progress->curKeyIndex = 0;
        progress->curSubCount = 0;
        progress->curSubIndex = 0;
        progress->keyBuffer.numKeys = 0;
    }
    *packetNumPos |= 0x80;
    return TRUE;
}

static void qr_process_query(QR2Implementation *qrec, QR2Buffer *buf, const u8 *qdata, int len, SocAddrIn *sender) {
    u8 serverKeyCount;
    u8 playerKeyCount;
    u8 teamKeyCount;
    u8 exFlags = 0;
    const u8 *serverKeys = NULL;
    const u8 *playerKeys = NULL;
    const u8 *teamKeys = NULL;

    if (len < 3) {
        gsDebugFormat(3, 0, 2, "Discarding invalid query (too short#1: %d bytes total)\r\n", len);
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
        gsDebugFormat(3, 0, 2, "Discarding invalid query (too short#2: %d bytes remain)\r\n", len);
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
        gsDebugFormat(3, 0, 2, "Discarding invalid query (too short#3: %d bytes remain)\r\n", len);
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
        gsDebugFormat(3, 0, 2, "Discarding invalid query (too short#4: %d bytes remain)\r\n", len);
        return;
    }
    if (len > 0) {
        exFlags = qdata[0];
        len--;
    }
    if ((exFlags & 1) == 1) {
        QRSplitQueryProgress progress;

        progress.curPacketNum = 0;
        progress.curKeyType = key_server;
        progress.curKeyIndex = 0;
        progress.curSubCount = 0;
        progress.curSubIndex = 0;
        progress.keyBuffer.numKeys = 0;
        gsDebugFormat(3, 0, 0x10, "Building query reply (split packet supported)\r\n");
        while (qr_build_split_query_reply(qrec, buf, &progress) == TRUE) {
            sendto(qrec->hbSock, buf->buffer, buf->len, 0, sender, sizeof(SocAddrIn));
            buf->len = 5;
            if (progress.curPacketNum > 7) {
                return;
            }
        }
        gsDebugFormat(3, 0, 0x10, "Finished split query reply (%d packets)\r\n", progress.curPacketNum);
    } else {
        gsDebugFormat(3, 0, 0x10, "Building query reply (single packet)\r\n");
        qr_build_query_reply(qrec, buf, serverKeyCount, serverKeys, playerKeyCount, playerKeys, teamKeyCount, teamKeys);
        sendto(qrec->hbSock, buf->buffer, buf->len, 0, sender, sizeof(SocAddrIn));
    }
}

// A message from a client through the master server: a NAT negotiation's cookie after the magic bytes, or the game's
static void qr_process_client_message(QR2Implementation *qrec, const char *buf, int len) {
    u8 natNegBytes[NATNEG_MAGIC_LEN] = { 0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2 };
    int i;
    BOOL isNatNeg = TRUE;

    gsDebugFormat(3, 4, 8, "Processing client message\r\n");
    if (len >= NATNEG_MAGIC_LEN + 4) {
        for (i = 0; i < NATNEG_MAGIC_LEN; i++) {
            if ((u8)buf[i] != natNegBytes[i]) {
                isNatNeg = FALSE;
                break;
            }
        }
    } else {
        isNatNeg = FALSE;
    }
    if (isNatNeg) {
        int cookie;

        memcpy(&cookie, buf + NATNEG_MAGIC_LEN, 4);
        cookie = QR2_SWAP32(cookie);
        if (qrec->ccCallback != NULL) {
            if (__GSIACResult != 1) {
                __GSIACResult = 1;
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
static BOOL qr_got_recent_message(QR2Implementation *qrec, int msgKey) {
    int i;

    for (i = 0; i < QR2_RECENT_CLIENT_MESSAGES_TO_TRACK; i++) {
        if (qrec->clientMessageKeys[i] == msgKey) {
            return TRUE;
        }
    }
    qrec->curMessageKey = (qrec->curMessageKey + 1) % QR2_RECENT_CLIENT_MESSAGES_TO_TRACK;
    qrec->clientMessageKeys[qrec->curMessageKey] = msgKey;
    return FALSE;
}

// Adds a random challenge for a host to the reply, which its query has to give back
static BOOL qr2_process_ip_verify(QR2Implementation *qrec, QR2Buffer *buf, SocAddrIn *sender) {
    int i = 0;
    u32 now = current_time();
    int firstFreeIndex = -1;
    int numDuplicates = 0;

    if ((qrec->backendOptions & 0x80) == 0) {
        qr2_buffer_add_int(buf, 0);
        return TRUE;
    }
    for (; i < QR2_IPVERIFY_ARRAY_SIZE; i++) {
        if (firstFreeIndex == -1 && qrec->ipVerify[i].addr.addr == 0) {
            firstFreeIndex = i;
        }
        if (qrec->ipVerify[i].addr.addr == sender->addr && qrec->ipVerify[i].addr.port == sender->port) {
            numDuplicates++;
        }
    }
    if (numDuplicates > 5) {
        return FALSE;
    }
    if (firstFreeIndex == -1) {
        return FALSE;
    }
    qrec->ipVerify[firstFreeIndex].addr = *sender;
    qrec->ipVerify[firstFreeIndex].challenge = QR2_SWAP32((rand() << 16) | rand());
    qrec->ipVerify[firstFreeIndex].createTime = now;
    qr2_buffer_add_int(buf, qrec->ipVerify[firstFreeIndex].challenge);
    return TRUE;
}

// Whether a host gave back the challenge it was sent, which is then forgotten
static BOOL qr2_check_ip_verify(QR2Implementation *qrec, SocAddrIn *sender, u32 ipVerify) {
    int i = 0;

    for (; i < QR2_IPVERIFY_ARRAY_SIZE; i++) {
        if (qrec->ipVerify[i].addr.addr == sender->addr && qrec->ipVerify[i].addr.port == sender->port) {
            if (qrec->ipVerify[i].challenge == ipVerify) {
                qrec->ipVerify[i].addr.addr = 0;
                qrec->ipVerify[i].addr.port = 0;
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Forgets the challenges that weren't given back within 4 seconds
static void qr2_expire_ip_verify(QR2Implementation *qrec) {
    int i = 0;
    u32 now = current_time();

    for (; i < QR2_IPVERIFY_ARRAY_SIZE; i++) {
        if (qrec->ipVerify[i].addr.addr != 0 && now - qrec->ipVerify[i].createTime > 4000) {
            qrec->ipVerify[i].addr.addr = 0;
        }
    }
}

void qr2_parse_queryA(QR2Implementation *qrec, char *query, int len, SocAddrIn *sender) {
    QR2Buffer buf;
    char ptype;
    char *reqkey;
    char *pos;
    u32 ipVerify = 0;
    int i;

    buf.len = 0;
    gsDebugFormat(3, 0, 0x40, "qr2_parse_queryA()\r\n");
    if (qrec == NULL) {
        qrec = current_rec;
    }
    if (query[0] == ';') {
        if (qrec->cdkeyProcess != NULL) {
            gsDebugFormat(3, 0, 8, "Forwarding cdkey query onto cdkey sdk\r\n");
            qrec->cdkeyProcess(query, len, sender);
        } else {
            gsDebugFormat(3, 0, 2, "Received cdkey query but not using qr2-cdkey integration!\r\n");
        }
        return;
    }
    if (len >= 6 && memcmp(query, NNMagicData, NATNEG_MAGIC_LEN) == 0) {
        gsDebugFormat(3, 0, 8, "Forwarding natneg query onto natneg sdk\r\n");
        NNProcessData(query, len, sender);
        return;
    }
    if (len < 7) {
        return;
    }
    if ((u8)query[0] != 0xfe || (u8)query[1] != 0xfd) {
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
        gsDebugFormat(3, 0, 8, "Received IP verify challenge request\r\n");
        if (qr2_process_ip_verify(qrec, &buf, sender)) {
            break;
        }
        return;
    case 0:
        gsDebugFormat(3, 0, 8, "Processing query packet\r\n");
        if ((qrec->backendOptions & 0x80) == 0x80) {
            if (len < 4) {
                return;
            }
            ipVerify = QR2_SWAP32(*(u32 *)pos);
            pos += 4;
            len -= 4;
            if (!qr2_check_ip_verify(qrec, sender, ipVerify)) {
                return;
            }
        }
        qr_process_query(qrec, &buf, (const u8 *)pos, len, sender);
        return;
    case 1:
        gsDebugFormat(3, 0, 8, "Processing challenge packet\r\n");
        for (i = 0; i < QR2_REQUEST_KEY_LEN; i++) {
            if (reqkey[i] != qrec->instanceKey[i]) {
                return;
            }
        }
        if (len >= 15) {
            u32 backendOptions;

            STD_TSScanf(pos + len - 15, "%02x", &backendOptions);
            qrec->backendOptions = backendOptions;
            gsDebugFormat(3, 4, 8, "Received setting options: %d\r\n", qrec->backendOptions);
            if (qrec->paCallback != NULL) {
                handle_public_address(qrec, pos + len - 13);
            } else {
                gsDebugFormat(3, 0, 8, "Discarding public address (no callback set)\r\n");
            }
        }
        compute_challenge_response(qrec, &buf, pos, len);
        break;
    case 2:
        gsDebugFormat(3, 0, 8, "Processing echo packet\r\n");
        if (len > 32) {
            len = 32;
        }
        buf.buffer[0] = 5;
        memcpy(buf.buffer + buf.len, pos, len);
        buf.len += len;
        break;
    case 4:
        gsDebugFormat(3, 0, 2, "Processing adderror packet\r\n");
        if (qrec->listedState == -1) {
            return;
        }
        for (i = 0; i < QR2_REQUEST_KEY_LEN; i++) {
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
        gsDebugFormat(3, 0, 8, "Processing clientmessage packet\r\n");
        for (i = 0; i < QR2_REQUEST_KEY_LEN; i++) {
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
        gsDebugFormat(3, 0, 0x10, "Processing keepalive packet\r\n");
        return;
    case 10:
        gsDebugFormat(3, 0, 0x10, "Processing client packet\r\n");
        if (qrec->crCallback != NULL) {
            qrec->crCallback(qrec->userData);
        }
        return;
    default:
        return;
    }
    sendto(qrec->hbSock, buf.buffer, buf.len, 0, sender, sizeof(SocAddrIn));
    gsDebugFormat(3, 0, 0x10, "Sent %d bytes as QR2 query response\r\n", buf.len);
    gsDebugBinary(3, 0, 0x20, (const u8 *)buf.buffer, buf.len);
}

static void send_keepalive(QR2Implementation *qrec) {
    QR2Buffer buf;

    buf.len = 0;
    qr_add_packet_header(&buf, 8, qrec->instanceKey);
    sendto(qrec->hbSock, buf.buffer, buf.len, 0, &qrec->hbAddr, sizeof(SocAddrIn));
    qrec->lastKa = current_time();
    gsDebugFormat(3, 0, 0x10, "Sent keepalive to master\r\n", buf);
}

// state changed is 0 for the periodic heartbeat, 1 for a change of the game's state, 2 when it shuts down and 3 while
// it waits for the master server's challenge
static void send_heartbeat(QR2Implementation *qrec, int stateChanged) {
    QR2Buffer buf;
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
        QR2Buffer temp;

        memcpy(temp.buffer, buf.buffer, buf.len);
        temp.len = buf.len;
        qr_build_query_reply(qrec, &temp, 0xff, NULL, 0xff, NULL, 0xff, NULL);
        if (QR2_MAX_DATA_SIZE - temp.len < 1) {
            temp.len = buf.len;
            qr_build_query_reply(qrec, &temp, 0xff, NULL, 0, NULL, 0, NULL);
        }
        memcpy(buf.buffer, temp.buffer, temp.len);
        buf.len = temp.len;
    } else if (QR2_MAX_DATA_SIZE - buf.len >= 1) {
        buf.buffer[buf.len++] = '\0';
    }
    sendto(qrec->hbSock, buf.buffer, buf.len, 0, &qrec->hbAddr, sizeof(SocAddrIn));
    qrec->lastKa = qrec->lastHeartbeat = current_time();
    if (stateChanged != 0) {
        qrec->userStateChangeRequested = FALSE;
    }
    gsDebugFormat(3, 0, 0x10, "Sent heartbeat to master (size %d)\r\n", buf.len);
    gsDebugBinary(3, 0, 0x20, (const u8 *)buf.buffer, buf.len);
}
