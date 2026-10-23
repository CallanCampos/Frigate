#include "dwcutil/dwc_aoss.h"
#include "dwc/ext.h"
#include "dwc_aoss_ap.h"
#include "dwc_wcm.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nitro/wm.h"

// The AOSS client of DWC's settings (see dwc_aoss.h): Buffalo's one-touch setup. It looks for the access point named
// ESSID-AOSS, joins it with a fixed WEP key, takes an address in its subnet and talks to it in UDP packets on port
// 22416 in three stages, each answered by the access point: the hello (0x1000), the security settings that it offers
// (0x2000) and the end (0x3000). The packets are encrypted with RC4 under the id of the access point. It ends with the
// keys of the network. The file also has the small network helpers that it works with; the Wi-Fi scan and connect are
// in dwc_aoss_ap.c. The ROM names none of it; the names are
// the DWC utility library's own, as that library's symbols and debug information have them, where the code shows them,
// and ours elsewhere.
//
// MWCC 2.0 lays the statics out in the reverse of their definition order

#define AOSS_SSID "ESSID-AOSS"
#define AOSS_KEY "MELCO"
#define AOSS_PORT 22416
#define AOSS_CLIENT_ADDR 0xc0a80b65 // 192.168.11.101
#define AOSS_ROUTER_ADDR 0xc0a80b01 // 192.168.11.1
#define AOSS_PACKET_SIZE 0x5dc
#define AOSS_BUFFER_SIZE 0x5f8 // of a packet, with the three words that the receiver keeps before the message
#define AOSS_PROTOCOL 0x11

// A record of a message: 4 bytes of header (the length is the payload's), and the payload
typedef struct AossItem {
    u8 type;
    u8 reserved;
    u16 length; // big endian
    u16 next;   // big endian, from the first item
    u8 data[1];
} AossItem;

typedef struct AossRecord {
    u8 type;
    u8 reserved;
    u16 length; // big endian
    union {
        s32 status; // type 7: big endian
        AossItem items[1];
        struct {
            u16 flags;
            AossItem items[1];
        } secure;
        u8 bytes[1];
    } payload;
} AossRecord;

// The header of a message
typedef struct tagASTSpktHeader {
    u16 PktVersion; // 0x00: 1
    u16 unk_02;
    u16 unk_04;
    u16 cmd; // 0x06: 0x1010 + 0x1000 for each stage
    u16 type;
    u16 size;      // 0x0a: of the body
    u16 CryptType; // 0x0c: bit 4 says it is encrypted
    s8 crc8;       // 0x0e
    u8 Fragment;   // 0x0f
    u8 SeqID[8];   // 0x10: of a client, 6 bytes of address and 2 of a number
} tagASTSpktHeader;

// A record with a status
typedef struct tagASTSpktResult {
    u8 type;
    u8 reserved;
    u16 length; // big endian
    s32 Result; // big endian
} tagASTSpktResult;

// The body of an encrypted message
typedef struct tagASTSpktCryptData {
    u16 size; // big endian, of the data
    u8 iv[2];
    u8 Data[1];
} tagASTSpktCryptData;

// A received message: its header (tagASTSpktHeader) and its records, as bytes
typedef struct AossPacket {
    int sock;
    int length;
    int unk_08;
    u8 data[AOSS_PACKET_SIZE];
} AossPacket;

// What the hello has: two names and a number
typedef struct AossHello {
    u8 name[0x80];
    u8 info[0x80];
    u8 extra[4];
} AossHello;

// The hello's fields (see AossHello) take the first three buffers, and the keys of the stages the next ones
typedef struct PRODUCT_INFO_AOSS {
    u8 ProductInfo[5][0x80];
} PRODUCT_INFO_AOSS;

typedef struct ASTS_DATA_WEP {
    u32 Enable;
    u32 KeyLen;
    char SSID[0x28];
    u8 Data[4][0x40];
} ASTS_DATA_WEP;

typedef struct ASTS_DATA_AES {
    u32 Enable;
    u32 KeyLen;
    char SSID[0x28];
    u8 Data[0x40];
} ASTS_DATA_AES;

typedef struct ASTS_DATA_INFO {
    u32 ReNew;
    u32 EncryptLevel;
    ASTS_DATA_WEP WepData_40;
    ASTS_DATA_WEP WepData_104;
    ASTS_DATA_AES TkipData;
    ASTS_DATA_AES AesData;
    u32 BSSID[2];
} ASTS_DATA_INFO;

// The request that is run
typedef struct AossRequest {
    const u8 *data; // 0x24
    int length;     // 0x28
    u32 wanted;     // 0x2c: the types of security that were asked for
    u32 found;      // 0x30: the types that the access point has
    u32 addr;       // 0x34: of the access point
    u32 netmask;    // 0x38
    s8 reconnected; // 0x3c
    s8 version;     // 0x3d
} AossRequest;

typedef struct AossWork {
    int encrypt;         // 0x00
    void *scanList;      // 0x04
    int randReady;       // 0x08
    void *apInfo;        // 0x0c
    int error;           // 0x10
    AossPacket *packet;  // 0x14
    u32 randState;       // 0x18
    u32 randMul;         // 0x1c
    u32 randAdd;         // 0x20
    AossRequest request; // 0x24
} AossWork;

typedef struct RETRY_VAL {
    s16 count;
    s16 time;
} RETRY_VAL;

typedef struct AossEndpoints {
    u32 router;
    u32 unk_04[3];
    u32 client;
} AossEndpoints;

typedef struct RC4Context {
    u32 i;
    u32 j;
    u8 *s;
    u32 n;
} RC4Context;

typedef struct AOSS_timeval {
    int tv_sec;
    int tv_usec;
} AOSS_timeval;

static void *AOSS_MALLOC(u32 size);
static void AOSS_FREE(void *ptr);
static int AOSS_Init_old(AOSS_INFO *param);
static u32 aoss_IP_sta(u32 addr, u32 netmask);
static void aoss_release(void);
static void aoss_data_init(AOSS_INFO *param);
static void aoss_set_error(int error);
static int aoss_get_error(void);
static int AOSS_seq_data_copy(AOSS_INFO *param);
static int aoss_check_str(const u8 *text, int length);
static int checkAP(const WLLA_BSS_LIST *list);
static int AOSS_set_APInfo(WLLA_CONNECT_PARAM *target);
static void MakeSendSeqID(int count, u8 *ids, const u8 *mac);
static int AOSS_StatusExclusion(int mode);
static int RecvMessage(int state, AossPacket *packet, int *errors, const u8 *ids, int sock);
static int AstsSetInitRes(int state, AossPacket *packet, int *errors, const u8 *ids);
static int AstsSetSecureRes(int state, AossPacket *packet, int *errors, const u8 *ids);
static int AstsRestartRes(int state, AossPacket *packet, int *errors, const u8 *ids);
static int DecodePacket(u8 *data);
static int CheckSeqID(int type, const u8 *id);
static int CheckRecvSeqID(const u8 *expected, u8 *received);
static int GetProductInfo(const AossItem *items, AossHello *hello);
static u32 aoss_get_bin(const u8 *data, int length);
static int GetWEPData(const AossRecord *record, ASTS_DATA_WEP *info);
static int GetWPAData(const AossRecord *record, ASTS_DATA_AES *info);
static int GetBssidData(const AossRecord *record, u8 *key);
static int GetSecurityData(int slot, const u8 *data, int length, ASTS_DATA_INFO *slots, PRODUCT_INFO_AOSS *session);
static int GetSecurityType(int flags);
static int SendMessage(int state, const AossEndpoints *endpoints, const u8 *ids, int sock);
static int AstsSetInitReq(const AossEndpoints *endpoints, const u8 *ids, int sock);
static int AstsSetSecureReq(const AossEndpoints *endpoints, const u8 *ids, int sock);
static int AstsRestartReq(const AossEndpoints *endpoints, const u8 *ids, int sock);
static s16 SetStaData(u8 *buffer);
static void PacketDataSet(int encrypt, u8 *dest, const u8 *src, s16 *length, s16 *flags, s8 *checksum);
static void SetPacketHeader(tagASTSpktHeader *header, int type, int length, int flags, s8 checksum, s8 protocol,
                            const u8 *id);
static int SendPacketUDP(void *buffer, int length, int toAll, int sock);
static int EncodeRC4(const u8 *in, u8 *out, u32 length, u8 *checksum, u8 *iv, const u8 *key, u32 keyLength);
static int DecodeRC4(const u8 *in, u8 *out, u32 length, u8 checksum, const u8 *iv, const u8 *key, u32 keyLength);
static void RC4init(RC4Context *context, const u8 *key, u32 keyLength, u32 n);
static void RC4encrypt(RC4Context *context, u8 *out, const u8 *in, u32 length);
static u32 arcfour_byte(RC4Context *context);
static u8 crc(const u8 *data, int length);
static u32 update_crc(u32 crc, const u8 *data, int length, int ready, u32 *table);
static void make_crc_table(int ready, u32 *table);
static int CryptSeqNo(u8 *block, int length, const char *key, int keyLength);
static void MakeKey(int round, char *out, int length, const char *key, int keyLength);
static void AddKey(const char *mask, u8 *block, int length);
static void Rotate(u8 *block, int length, u8 *work);
static void *Alloc(u32 name, s32 size);
static void Free(u32 name, void *ptr, s32 size);
static int AOSS_SetIPAddr(u32 addr, u32 netmask, u32 gateway);
static int AOSS_Disconnect(void);
static int AOSS_Memcmp(const u8 *a, const u8 *b, int size);
static void AOSS_Memcpy(void *dest, const void *src, u32 size);
static void AOSS_Memset(void *dest, int value, u32 size);
static int AOSS_Recvfrom(int sock, void *buffer, int size, int flags, SOCSockAddrIn *from, int *fromLength);
static int AOSS_Select(int count, SOCPollFD *readFds, void *writeFds, void *exceptFds, const AOSS_timeval *timeout);
static int AOSS_Sendto(int sock, const void *buffer, int size, int flags, SOCSockAddrIn *to, int toLength);
static int AOSS_Setsockopt(int sock, int level, int name, const void *value, int length);
static int AOSS_Socket(int domain, int type, int protocol);
static int AOSS_Bind(int sock, SOCSockAddrIn *address, int length);
static int AOSS_Close(int sock);
static u32 AOSS_Htonl(u32 value);
static u16 AOSS_Htons(u16 value);
static u32 AOSS_Ntohl(u32 value);
static u16 AOSS_Ntohs(u16 value);
static u32 AOSS_Strlen(const char *text);
static u16 AOSS_Rand(void);
static void AOSS_FD_ZERO(SOCPollFD *fd);
static void AOSS_FD_SET(int sock, SOCPollFD *fd);

// The scan's retry settings before the caller's are read: unset
static const RETRY_VAL sDefaultRetry = { -1, -1 };
static u8 sRecordType[3] = { 9, 8, 0 };
static struct {
    int socket;
    int mode;
} sNet = { -1, -1 };
static SOCConfig sSocConfig = {
    SOC_VENDOR_NINTENDO, SOC_VERSION, Alloc, Free, 0, { 0 }, { 0 }, { 0 }, { 0 }, { 0 }, 0x1000, 0x1000,
};

static u8 sCipherKey[0x78];
static u8 sPeerMac[8];
static ASTS_DATA_INFO sApInfo[2];
static u32 sCrcTable[256];
static PRODUCT_INFO_AOSS sSession;
static AossWork sAoss;

static void *AOSS_MALLOC(u32 size) {
    return gAossApAlloc(size);
}

static void AOSS_FREE(void *ptr) {
    gAossApFree(ptr);
}

int AOSSi_Init(AOSS_INFO *param) {
    int result;

    if (param->retryCount_Scan == 0 || param->retryCount_Scan < -1 || param->retryInterval_Scan < -1 ||
        param->retryCount_Comu == 0 || param->retryCount_Comu < -1 || param->retryInterval_Comu < -1 ||
        param->selectInterval < -1 || param->productInfo.dataLen == 0 || param->productInfo.dataLen > 0x100 ||
        param->productInfo.productName[param->productInfo.dataLen - 1] != 0) {
        result = -1;
    } else {
        result = 0;
    }
    if (gAossApAlloc == NULL || gAossApFree == NULL) {
        result = -1;
    }
    if (result == -1) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    sAoss.packet = AOSS_MALLOC(AOSS_BUFFER_SIZE);
    if (sAoss.packet == NULL) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    AOSS_StatusExclusion(-1);
    result = AOSS_Init_old(param);
    AOSS_FREE(sAoss.packet);
    aoss_release();
    if (sNet.socket != -1) {
        AOSS_Close(sNet.socket);
    }
    return result;
}

static int AOSS_Init_old(AOSS_INFO *param) {
    RETRY_VAL scan = sDefaultRetry;
    RETRY_VAL send = { 0, 0 };
    s16 timeout;
    int ret;
    int option = 1;
    int sends;
    int state = 0;
    int newState;
    int result;
    SOCSockAddrIn from;
    int fromLength;
    SOCPollFD pollFd;
    AOSS_timeval timeval;
    s16 i;
    u8 ids[3][8];
    int scanResult;
    WLLA_CONNECT_PARAM target;
    SOCSockAddrIn addr;
    u32 ip;
    AossEndpoints endpoints;
    AossPacket *packet;

    sends = 0;
    ip = 0;
    AOSS_Memset(ids, 0, sizeof(ids));
    scan.count = param->retryCount_Scan;
    if (scan.count == -1) {
        scan.count = 10;
    }
    send.count = param->retryCount_Comu;
    if (send.count == -1) {
        send.count = 10;
    }
    scan.time = param->retryInterval_Scan;
    if (scan.time == -1) {
        scan.time = 100;
    }
    send.time = param->retryInterval_Comu;
    if (send.time == -1) {
        send.time = 100;
    }
    timeout = param->selectInterval;
    if (timeout == -1) {
        timeout = 2000;
    }
    aoss_data_init(param);
    if ((sAoss.request.wanted & AOSS_TYPE_WEP40) != 1) {
        aoss_set_error(0x13);
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }

    i = 0;
    AOSS_StatusExclusion(0);
    while (1) {
        if (sAoss.scanList != NULL) {
            AOSS_FREE(sAoss.scanList);
            sAoss.scanList = NULL;
        }
        if (AOSSi_WLANGetBSSList((WLLA_BSS_LIST **)&sAoss.scanList) == -1) {
            param->aossResult = 0xf;
            aoss_release();
            return -1;
        }
        ret = checkAP(sAoss.scanList);
        if (ret == 4) {
            param->aossResult = 2;
            aoss_release();
            return -1;
        }
        if (ret == 0) {
            break;
        }
        if (i >= scan.count) {
            param->aossResult = 1;
            aoss_release();
            return -1;
        }
        AOSSi_Sleep(scan.time);
        i++;
    }

    AOSS_StatusExclusion(1);
    AOSS_Memset(&target, 0, sizeof(target));
    if (AOSS_set_APInfo(&target) != 0) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    sAoss.apInfo = AOSS_MALLOC(sizeof(WLLA_LINK_STAT));
    if (sAoss.apInfo == NULL) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    AOSS_Memset(sAoss.apInfo, 0, sizeof(WLLA_LINK_STAT));
    for (i = 0; i < scan.count; i++) {
        ret = AOSSi_WLANConnect(&target, sAoss.apInfo);
        if (ret == -1) {
            param->aossResult = 0xf;
            aoss_release();
            return -1;
        }
        if (ret == 0) {
            if (ret != 0 || ((WLLA_LINK_STAT *)sAoss.apInfo)->LinkStat == 1) {
                break;
            }
        }
        AOSSi_Sleep(scan.time);
    }
    if (i == scan.count) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    if (AOSS_SetIPAddr(AOSS_CLIENT_ADDR, ~0xff, AOSS_CLIENT_ADDR) != 0) {
        aoss_set_error(0xc);
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    aoss_release();
    MakeSendSeqID(3, ids[0], param->macAddress);
    sNet.socket = AOSS_Socket(2, 2, 0);
    if (sNet.socket < 0) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    if (AOSS_Setsockopt(sNet.socket, 0xffff, 1, &option, 4) < 0) {
        aoss_set_error(0xb);
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    AOSS_Memset(&addr, 0, sizeof(addr));
    addr.family = 2;
    addr.addr.addr = AOSS_Htonl(AOSS_CLIENT_ADDR);
    addr.port = AOSS_Htons(AOSS_PORT);
    if (AOSS_Bind(sNet.socket, &addr, 8) < 0) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }

    while (1) {
        packet = sAoss.packet;
        AOSS_Memset(&endpoints, 0, sizeof(endpoints));
        endpoints.client = AOSS_CLIENT_ADDR;
        endpoints.router = AOSS_ROUTER_ADDR;
        while (1) {
            if (state == 1 && sAoss.request.reconnected != 1) {
                if (sNet.socket != -1) {
                    AOSS_Close(sNet.socket);
                }
                sNet.socket = -1;
                if (AOSS_Disconnect() != 0) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                sAoss.scanList = AOSS_MALLOC(sizeof(WLLA_LINK_STAT));
                if (sAoss.scanList == NULL) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                while (1) {
                    if (sAoss.scanList != NULL) {
                        AOSS_FREE(sAoss.scanList);
                        sAoss.scanList = NULL;
                    }
                    scanResult = AOSSi_WLANGetBSSList((WLLA_BSS_LIST **)&sAoss.scanList);
                    if (scanResult == -1) {
                        param->aossResult = 0xf;
                        aoss_release();
                        return -1;
                    }
                    ret = checkAP(sAoss.scanList);
                    if (ret == 4) {
                        param->aossResult = 2;
                        aoss_release();
                        return -1;
                    }
                    if (ret == 0) {
                        break;
                    }
                    if (i >= scan.count) {
                        param->aossResult = 1;
                        aoss_release();
                        return -1;
                    }
                    AOSSi_Sleep(scan.time);
                    i++;
                }
                if (scanResult == -1) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                sAoss.apInfo = AOSS_MALLOC(sizeof(WLLA_LINK_STAT));
                if (sAoss.apInfo == NULL) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                AOSS_Memset(sAoss.apInfo, 0, sizeof(WLLA_LINK_STAT));
                for (i = 0; i < scan.count; i++) {
                    ret = AOSSi_WLANConnect(&target, sAoss.apInfo);
                    if (ret == -1) {
                        param->aossResult = 0xf;
                        aoss_release();
                        return -1;
                    }
                    if (ret == 0) {
                        if (ret != 0 || ((WLLA_LINK_STAT *)sAoss.apInfo)->LinkStat == 1) {
                            break;
                        }
                    }
                    AOSSi_Sleep(scan.time);
                }
                if (i == scan.count) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                ip = aoss_IP_sta(sAoss.request.addr, sAoss.request.netmask);
                if (AOSS_SetIPAddr(ip, sAoss.request.netmask, ip) != 0) {
                    aoss_set_error(0xc);
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                sAoss.request.reconnected = 1;
                aoss_release();
                sNet.socket = AOSS_Socket(2, 2, 0);
                if (sNet.socket < 0) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                if (AOSS_Setsockopt(sNet.socket, 0xffff, 1, &option, 4) < 0) {
                    aoss_set_error(0xb);
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                AOSS_Memset(&addr, 0, sizeof(addr));
                addr.family = 2;
                addr.addr.addr = AOSS_Htonl(ip);
                addr.port = AOSS_Htons(AOSS_PORT);
                if (AOSS_Bind(sNet.socket, &addr, 8) < 0) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
            }

            if (SendMessage(state, &endpoints, ids[0], sNet.socket) == -1) {
                aoss_set_error(0x1000 + state);
                param->aossResult = 0xf;
                aoss_release();
                return -1;
            }
            AOSS_Memset(packet, 0, AOSS_BUFFER_SIZE);
            AOSS_FD_ZERO(&pollFd);
            AOSS_FD_SET(sNet.socket, &pollFd);
            timeval.tv_sec = timeout / 1000;
            timeval.tv_usec = timeout % 1000 * 1000;
            if (AOSS_Select(sNet.socket + 1, &pollFd, NULL, NULL, &timeval) > 0) {
                break;
            }
            sends++;
            if (sends > send.count) {
                if (state == 0) {
                    aoss_set_error(0xf);
                } else if (state == 1) {
                    aoss_set_error(0x10);
                } else {
                    aoss_set_error(0x11);
                }
                result = -1;
                goto done;
            }
            AOSSi_Sleep(send.time);
        }
        fromLength = 8;
        ret = AOSS_Recvfrom(sNet.socket, packet->data, AOSS_PACKET_SIZE, 0, &from, &fromLength);
        packet->sock = sNet.socket;
        packet->length = AOSS_Ntohs(ret);
        newState = RecvMessage(state, packet, &sends, ids[0], sNet.socket);
        if (newState == 100) {
            result = 0;
            break;
        }
        if (newState == -1) {
            result = -1;
            break;
        }
        if (state != newState) {
            if (newState == 2) {
                if (sNet.socket != -1) {
                    AOSS_Close(sNet.socket);
                }
                sNet.socket = -1;
                if (AOSS_Disconnect() != 0) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                i = 0;
                AOSS_StatusExclusion(4);
                while (1) {
                    if (sAoss.scanList != NULL) {
                        AOSS_FREE(sAoss.scanList);
                        sAoss.scanList = NULL;
                    }
                    scanResult = AOSSi_WLANGetBSSList((WLLA_BSS_LIST **)&sAoss.scanList);
                    if (scanResult == -1) {
                        param->aossResult = 0xf;
                        aoss_release();
                        return -1;
                    }
                    ret = checkAP(sAoss.scanList);
                    if (ret == 4) {
                        param->aossResult = 2;
                        aoss_release();
                        return -1;
                    }
                    if (ret == 0) {
                        break;
                    }
                    if (i >= scan.count) {
                        param->aossResult = 1;
                        aoss_release();
                        return -1;
                    }
                    AOSSi_Sleep(scan.time);
                    i++;
                }
                sAoss.apInfo = AOSS_MALLOC(sizeof(WLLA_LINK_STAT));
                if (sAoss.apInfo == NULL) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                AOSS_Memset(sAoss.apInfo, 0, sizeof(WLLA_LINK_STAT));
                for (i = 0; i < scan.count; i++) {
                    ret = AOSSi_WLANConnect(&target, sAoss.apInfo);
                    if (ret == -1) {
                        param->aossResult = 0xf;
                        aoss_release();
                        return -1;
                    }
                    if (ret == 0) {
                        if (ret != 0 || ((WLLA_LINK_STAT *)sAoss.apInfo)->LinkStat == 1) {
                            break;
                        }
                    }
                    AOSSi_Sleep(scan.time);
                }
                if (i == scan.count) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                if (AOSS_SetIPAddr(ip, sAoss.request.netmask, ip) != 0) {
                    aoss_set_error(0xc);
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                aoss_release();
                sNet.socket = AOSS_Socket(2, 2, 0);
                if (sNet.socket < 0) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                if (AOSS_Setsockopt(sNet.socket, 0xffff, 1, &option, 4) < 0) {
                    aoss_set_error(0xb);
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
                AOSS_Memset(&addr, 0, sizeof(addr));
                addr.family = 2;
                addr.addr.addr = AOSS_Htonl(ip);
                addr.port = AOSS_Htons(AOSS_PORT);
                if (AOSS_Bind(sNet.socket, &addr, 8) < 0) {
                    param->aossResult = 0xf;
                    aoss_release();
                    return -1;
                }
            }
            state = newState;
            continue;
        }
        state = newState;
        if (sends > send.count) {
            if (state == 0) {
                aoss_set_error(0xf);
            } else if (state == 1) {
                aoss_set_error(0x10);
            } else {
                aoss_set_error(0x11);
            }
            result = -1;
            break;
        }
        AOSSi_Sleep(send.time);
    }

done:
    if (sNet.socket != -1) {
        AOSS_Close(sNet.socket);
    }
    sNet.socket = -1;
    if (AOSS_Disconnect() != 0) {
        param->aossResult = 0xf;
        aoss_release();
        return -1;
    }
    if (result != 0) {
        u8 error;

        switch (aoss_get_error()) {
        case 0xf:
            error = 3;
            break;
        case 0x10:
            error = 4;
            break;
        case 0x11:
            error = 5;
            break;
        case 0x14:
            error = 7;
            break;
        case 0x15:
            error = 8;
            break;
        default:
            error = 0xf;
            break;
        }
        param->aossResult = error;
        aoss_release();
        return -1;
    }
    if (AOSS_seq_data_copy(param) != 0) {
        param->aossResult = 6;
        aoss_release();
        return -1;
    }
    return 0;
}

// The first address after addr in its subnet, or the first one if that is the last
static u32 aoss_IP_sta(u32 addr, u32 netmask) {
    u32 network = addr & netmask;
    u32 next = ((addr & ~netmask) + 1) | network;

    if (next >= (~netmask | network)) {
        return network | 1;
    }
    return next;
}

static void aoss_release(void) {
    if (sAoss.apInfo != NULL) {
        AOSS_FREE(sAoss.apInfo);
        sAoss.apInfo = NULL;
    }
    if (sAoss.scanList != NULL) {
        AOSS_FREE(sAoss.scanList);
        sAoss.scanList = NULL;
    }
}

static void aoss_data_init(AOSS_INFO *param) {
    AOSS_Memset(sPeerMac, 0, 8);
    sAoss.error = 1;
    AOSS_Memset(&sAoss.request, 0, sizeof(AossRequest));
    sAoss.request.data = param->productInfo.productName;
    sAoss.request.length = param->productInfo.dataLen;
    sAoss.request.wanted = param->supportSecurity & 0xf;
    sAoss.request.version = param->productInfo.dataType;
    sAoss.request.found = 0;
    sAoss.request.addr = AOSS_ROUTER_ADDR;
    sAoss.request.reconnected = 0;
}

static void aoss_set_error(int error) {
    sAoss.error = error;
}

static int aoss_get_error(void) {
    return sAoss.error;
}

// Copies the keys that the access point sent to the answer, if they are fit
static int AOSS_seq_data_copy(AOSS_INFO *param) {
    AOSS_SECURE_INFO *result;
    ASTS_DATA_WEP *wep40;
    ASTS_DATA_WEP *wep104;
    ASTS_DATA_AES *tkip;
    ASTS_DATA_AES *aes;

    result = &param->secureInfo;
    wep40 = &sApInfo[0].WepData_40;
    wep104 = &sApInfo[0].WepData_104;
    tkip = &sApInfo[0].TkipData;
    aes = &sApInfo[0].AesData;
    if (result == NULL) {
        return -1;
    }
    param->supportSecurity = sAoss.request.wanted & sAoss.request.found;
    AOSS_Memset(result, 0, sizeof(AOSS_SECURE_INFO));
    if (param->supportSecurity & AOSS_TYPE_WEP40) {
        AOSS_Memcpy(result->wep40Key[0], wep40->Data[0], wep40->KeyLen);
        AOSS_Memcpy(result->wep40Key[1], wep40->Data[1], wep40->KeyLen);
        AOSS_Memcpy(result->wep40Key[2], wep40->Data[2], wep40->KeyLen);
        AOSS_Memcpy(result->wep40Key[3], wep40->Data[3], wep40->KeyLen);
        if (aoss_check_str((u8 *)wep40->SSID, AOSS_Strlen(wep40->SSID)) != 0) {
            goto fail;
        }
        AOSS_Memcpy(result->WEP40_SSID, wep40->SSID, AOSS_Strlen(wep40->SSID));
    }
    if (param->supportSecurity & AOSS_TYPE_WEP104) {
        AOSS_Memcpy(result->wep104Key[0], wep104->Data[0], wep104->KeyLen);
        AOSS_Memcpy(result->wep104Key[1], wep104->Data[1], wep104->KeyLen);
        AOSS_Memcpy(result->wep104Key[2], wep104->Data[2], wep104->KeyLen);
        AOSS_Memcpy(result->wep104Key[3], wep104->Data[3], wep104->KeyLen);
        if (aoss_check_str((u8 *)wep104->SSID, AOSS_Strlen(wep104->SSID)) != 0) {
            goto fail;
        }
        AOSS_Memcpy(result->WEP104_SSID, wep104->SSID, AOSS_Strlen(wep104->SSID));
    }
    if (param->supportSecurity & AOSS_TYPE_TKIP) {
        if (aoss_check_str(tkip->Data, tkip->KeyLen - 1) != 0) {
            goto fail;
        }
        AOSS_Memcpy(result->WPA_PSK_TKIP, tkip->Data, tkip->KeyLen);
        if (aoss_check_str((u8 *)tkip->SSID, AOSS_Strlen(tkip->SSID)) != 0) {
            goto fail;
        }
        AOSS_Memcpy(result->WPA_PSK_TKIP_SSID, tkip->SSID, AOSS_Strlen(tkip->SSID));
    }
    if (param->supportSecurity & AOSS_TYPE_AES) {
        if (aoss_check_str(aes->Data, aes->KeyLen - 1) != 0) {
            goto fail;
        }
        AOSS_Memcpy(result->WPA_PSK_AES, aes->Data, aes->KeyLen);
        if (aoss_check_str((u8 *)aes->SSID, AOSS_Strlen(aes->SSID)) != 0) {
            goto fail;
        }
        AOSS_Memcpy(result->WPA_PSK_AES_SSID, aes->SSID, AOSS_Strlen(aes->SSID));
    }
    param->aossResult = 0;
    return 0;

fail:
    AOSS_Memset(result, 0, sizeof(AOSS_SECURE_INFO));
    return -1;
}

static int aoss_check_str(const u8 *text, int length) {
    int i;

    for (i = 0; i < length; i++) {
        u8 c = *text++;

        if (c < 0x20 || c > 0x7f) {
            return -1;
        }
    }
    return 0;
}

// Whether the scan has exactly one access point named ESSID-AOSS: 0 if so, 4 if there are several, 5 if none
static int checkAP(const WLLA_BSS_LIST *list) {
    int result = 0;
    int found = 0;
    int count = list->Num;
    int i;

    if (count == 0) {
        return 5;
    }
    if (list->Num > 0x40) {
        count = 0x40;
    }
    for (i = 0; i < count; i++) {
        if ((list->BssInfo[i].Capability & 1) && list->BssInfo[i].ssidLength == AOSS_Strlen(AOSS_SSID) &&
            AOSS_Memcmp(list->BssInfo[i].ssid, (u8 *)AOSS_SSID, AOSS_Strlen(AOSS_SSID)) == 0) {
            found++;
        }
    }
    if (found > 1) {
        result = 4;
    }
    if (found == 0) {
        result = 5;
    }
    return result;
}

// The access point to connect to: ESSID-AOSS with the WEP key MELCO
static int AOSS_set_APInfo(WLLA_CONNECT_PARAM *target) {
    target->ssidLength = AOSS_Strlen(AOSS_SSID);
    AOSS_Memcpy(target->ssid, AOSS_SSID, target->ssidLength);
    target->AuthAlgo = 1;
    target->keyLength = AOSS_Strlen(AOSS_KEY);
    if (target->keyLength > 13) {
        return -1;
    }
    AOSS_Memcpy(target->key, AOSS_KEY, target->keyLength);
    return 0;
}

static void MakeSendSeqID(int count, u8 *ids, const u8 *mac) {
    typedef struct {
        u8 mac[6];
        u16 number;
    } Id;
    Id *id = (Id *)ids;
    int i;

    for (i = 0; i < count; i++) {
        AOSS_Memcpy(id->mac, mac, 6);
        id->number = AOSS_Rand();
        id->number = AOSS_Htons(id->number);
        id++;
    }
}

static int AOSS_StatusExclusion(int mode) {
    if (mode == -1) {
        sNet.mode = mode;
        return 0;
    }
    if (sNet.mode != mode) {
        sNet.mode = mode;
        return AOSSi_Status(mode);
    }
    return 0;
}

static int RecvMessage(int state, AossPacket *packet, int *errors, const u8 *ids, int sock) {
    tagASTSpktHeader *header = (tagASTSpktHeader *)packet->data;

    if (AOSS_Ntohs(header->PktVersion) < 1) {
        (*errors)++;
        return state;
    }
    if (header->Fragment != AOSS_PROTOCOL) {
        (*errors)++;
        return state;
    }
    if (DecodePacket(packet->data) > 0) {
        (*errors)++;
        return state;
    }
    switch (AOSS_Ntohs(header->cmd)) {
    case 0x1010:
        state = AstsSetInitRes(state, packet, errors, ids);
        break;
    case 0x2010:
        state = AstsSetSecureRes(state, packet, errors, ids);
        break;
    case 0x3010:
        state = AstsRestartRes(state, packet, errors, ids);
        break;
    }
    return state;
}

static int AstsSetInitRes(int state, AossPacket *packet, int *errors, const u8 *ids) {
    tagASTSpktHeader *header;
    AossRecord *record;
    int result;

    if (state != 0) {
        (*errors)++;
        return state;
    }
    header = (tagASTSpktHeader *)packet->data;
    record = (AossRecord *)(packet->data + sizeof(tagASTSpktHeader));
    if (CheckRecvSeqID(ids, header->SeqID) < 0) {
        (*errors)++;
        return state;
    }
    if (AOSS_Ntohs(record->length) == 0) {
        (*errors)++;
        return state;
    }
    if (record->type == 7) {
        if (AOSS_Ntohl(record->payload.status) == -2) {
            aoss_set_error(0x14);
        } else if (AOSS_Ntohl(record->payload.status) == -3) {
            aoss_set_error(0x15);
        } else {
            aoss_set_error(0x18);
        }
        return -1;
    }
    if (record->type != 1) {
        (*errors)++;
        return state;
    }
    result = GetProductInfo(record->payload.items, (AossHello *)sSession.ProductInfo[0]);
    if (result < 0) {
        if (result == -2) {
            aoss_set_error(0x16);
            return -1;
        }
        (*errors)++;
        return state;
    }
    sAoss.encrypt = GetSecurityType(AOSS_Ntohs(header->CryptType));
    *errors = 0;
    return 1;
}

static int AstsSetSecureRes(int state, AossPacket *packet, int *errors, const u8 *ids) {
    AossRecord *record;
    u8 *data;
    tagASTSpktHeader *header;

    if (state != 1) {
        (*errors)++;
        return state;
    }
    header = (tagASTSpktHeader *)packet->data;
    data = packet->data + sizeof(tagASTSpktHeader);
    if (CheckRecvSeqID(ids + 8, header->SeqID) < 0) {
        (*errors)++;
        return state;
    }
    record = (AossRecord *)data;
    if (AOSS_Ntohs(record->length) == 0) {
        (*errors)++;
        return state;
    }
    if (record->type == 7) {
        if (AOSS_Ntohl(record->payload.status) == -2) {
            aoss_set_error(0x14);
        } else if (AOSS_Ntohl(record->payload.status) == -3) {
            aoss_set_error(0x15);
        } else {
            aoss_set_error(0x18);
        }
        return -1;
    }
    AOSS_Memset(sApInfo, 0, sizeof(sApInfo));
    if (GetSecurityData(0, data, AOSS_Ntohs(header->size), sApInfo, &sSession) < 0) {
        (*errors)++;
        return state;
    }
    if ((sAoss.request.found & sAoss.request.wanted) == 0) {
        return state;
    }
    *errors = 0;
    return 2;
}

static int AstsRestartRes(int state, AossPacket *packet, int *errors, const u8 *ids) {
    AossRecord *record;

    if (state != 2) {
        (*errors)++;
        return state;
    }
    record = (AossRecord *)(packet->data + sizeof(tagASTSpktHeader));
    if (CheckRecvSeqID(ids + 16, ((tagASTSpktHeader *)packet->data)->SeqID) < 0) {
        (*errors)++;
        return state;
    }
    if (record->type != 7) {
        (*errors)++;
        return state;
    }
    if (AOSS_Ntohs(record->length) == 0) {
        (*errors)++;
        return state;
    }
    if (AOSS_Ntohl(record->payload.status) == 0) {
        return 100;
    }
    if (AOSS_Ntohl(record->payload.status) == -2) {
        aoss_set_error(0x14);
        return -1;
    }
    if (AOSS_Ntohl(record->payload.status) == -3) {
        aoss_set_error(0x15);
        return -1;
    }
    aoss_set_error(0x18);
    return -1;
}

// Checks the id of a message and decrypts its body; 0 if it is fit
static int DecodePacket(u8 *data) {
    tagASTSpktHeader *header = (tagASTSpktHeader *)data;
    tagASTSpktCryptData *body = (tagASTSpktCryptData *)(data + sizeof(tagASTSpktHeader));
    u8 id[8];
    u32 length;
    u8 *buffer;
    int result;

    AOSS_Memcpy(id, header->SeqID, 8);
    if (CryptSeqNo(id, 8, AOSS_KEY, AOSS_Strlen(AOSS_KEY)) == -1) {
        aoss_set_error(2);
        return -100;
    }
    result = CheckSeqID(AOSS_Ntohs(header->cmd), id);
    if (result != 0) {
        return result;
    }
    if (AOSS_Ntohs(header->cmd) == 0x1000) {
        AOSS_Memcpy(sPeerMac, id, 8);
    }
    if ((AOSS_Ntohs(header->CryptType) & 0xf) == 0) {
        return 0;
    }
    length = AOSS_Ntohs(body->size);
    buffer = AOSS_MALLOC(length);
    if (buffer == NULL) {
        aoss_set_error(2);
        return 100;
    }
    if (DecodeRC4(body->Data, buffer, length, header->crc8, body->iv, sPeerMac, 8) < 0) {
        AOSS_FREE(buffer);
        if (aoss_get_error() == 2) {
            return 100;
        }
        return 200;
    }
    AOSS_Memcpy(body, buffer, length);
    header->size = AOSS_Htons(length);
    AOSS_FREE(buffer);
    return 0;
}

// Whether a message may come from the access point we talk to: 0 if so, 1 if it is another, 2 if there is none yet
static int CheckSeqID(int type, const u8 *id) {
    int result = 0;
    int i;
    int known = 0;

    for (i = 0; i < 6; i++) {
        if (sPeerMac[i] != 0) {
            known = 1;
            break;
        }
    }
    if (known) {
        if (AOSS_Memcmp(sPeerMac, id, 6) != 0) {
            result = 1;
        }
    } else if (type != 0x1000) {
        result = 2;
    }
    return result;
}

// Whether the id that came back is the one that was sent, with its number one higher: 0 if so, -1 or -2
static int CheckRecvSeqID(const u8 *expected, u8 *received) {
    int result = 0;

    CryptSeqNo(received, 8, AOSS_KEY, AOSS_Strlen(AOSS_KEY));
    if (AOSS_Memcmp(expected, received, 6) != 0) {
        result--;
    } else if (AOSS_Ntohs(*(u16 *)(expected + 6)) + 1 != AOSS_Ntohs(*(u16 *)(received + 6))) {
        result -= 2;
    }
    return result;
}

static int GetProductInfo(const AossItem *items, AossHello *hello) {
    const AossItem *item;

    AOSS_Memset(hello, 0, 0x104);
    item = items;
    while (1) {
        int length = AOSS_Ntohs(item->length);

        if (length <= 0) {
            return -1;
        }
        switch (item->type) {
        case 0:
            AOSS_Memcpy(hello->name, item->data, length);
            break;
        case 1:
            AOSS_Memcpy(hello->info, item->data, length);
            break;
        case 2:
            AOSS_Memcpy(hello->extra, item->data, length);
            break;
        case 3:
        case 4: {
            int value = AOSS_Ntohs(item->data[0]);

            if (value <= 0) {
                return -2;
            }
        } break;
        case 5:
            sAoss.request.addr = AOSS_Ntohl(aoss_get_bin(item->data, length));
            break;
        case 6:
            sAoss.request.netmask = AOSS_Ntohl(aoss_get_bin(item->data, length));
            break;
        default:
            return -1;
        }
        if (item->next == 0) {
            break;
        }
        item = (const AossItem *)((const u8 *)items + AOSS_Ntohs(item->next));
    }
    return 0;
}

static u32 aoss_get_bin(const u8 *data, int length) {
    u32 value = 0;
    int i;
    const u8 *p = data + (length - 1);

    for (i = 0; i < length; i++) {
        value = (value << 8) + *p--;
    }
    return value;
}

static int GetWEPData(const AossRecord *record, ASTS_DATA_WEP *info) {
    const AossItem *item = record->payload.secure.items;
    const AossItem *items;
    char *ssid = info->SSID;
    u8 *key3 = info->Data[3];
    u8 *key2 = info->Data[2];
    u8 *key1 = info->Data[1];
    u8 *key0 = info->Data[0];

    items = item;

    while (1) {
        u32 length = AOSS_Ntohs(item->length);

        switch (item->type) {
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            if (length > 5) {
                return -1;
            }
            break;
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
            if (length > 13) {
                return -1;
            }
            break;
        case 0x15:
        case 0x25:
            if (length > 33) {
                return -1;
            }
            break;
        }
        switch (item->type) {
        case 0x10:
        case 0x20:
            AOSS_Memcpy(key0, item->data, length);
            info->KeyLen = length;
            break;
        case 0x11:
        case 0x21:
            AOSS_Memcpy(key1, item->data, length);
            info->KeyLen = length;
            break;
        case 0x12:
        case 0x22:
            AOSS_Memcpy(key2, item->data, length);
            info->KeyLen = length;
            break;
        case 0x13:
        case 0x23:
            AOSS_Memcpy(key3, item->data, length);
            info->KeyLen = length;
            break;
        case 0x15:
        case 0x25:
            if (length != 0 && item->data[length - 1] != 0) {
                return -1;
            }
            AOSS_Memcpy(ssid, item->data, length);
            break;
        default:
            return -1;
        }
        if (item->next == 0) {
            break;
        }
        item = (const AossItem *)((const u8 *)items + AOSS_Ntohs(item->next));
    }
    return 0;
}

static int GetWPAData(const AossRecord *record, ASTS_DATA_AES *info) {
    const AossItem *item = record->payload.secure.items;
    const AossItem *items;
    char *ssid = info->SSID;
    u8 *key = info->Data;

    items = item;

    while (1) {
        u32 length = AOSS_Ntohs(item->length);

        switch (item->type) {
        case 0x30:
        case 0x40:
            if (length > 0x40) {
                return -1;
            }
            break;
        case 0x35:
        case 0x45:
            if (length > 0x21) {
                return -1;
            }
            break;
        }
        switch (item->type) {
        case 0x30:
        case 0x40:
            AOSS_Memcpy(key, item->data, length);
            info->KeyLen = length;
            break;
        case 0x35:
        case 0x45:
            if (length != 0 && item->data[length - 1] != 0) {
                return -1;
            }
            AOSS_Memcpy(ssid, item->data, length);
            break;
        default:
            return -1;
        }
        if (item->next != 0) {
            item = (const AossItem *)((const u8 *)items + AOSS_Ntohs(item->next));
        } else {
            break;
        }
    }
    return 0;
}

static int GetBssidData(const AossRecord *record, u8 *key) {
    const AossItem *item = record->payload.secure.items;
    int length = AOSS_Ntohs(item->length);

    if (length <= 0) {
        return -1;
    }
    if (item->type != 0x70) {
        return -1;
    }
    AOSS_Memcpy(key, item->data, length);
    return 0;
}

// Reads the security settings that the reply of a stage has into slot of slots; 0 if they are fit
static int GetSecurityData(int slot, const u8 *data, int length, ASTS_DATA_INFO *slots, PRODUCT_INFO_AOSS *session) {
    const AossRecord *entry;
    int step;
    int next;
    int result;
    u32 found;
    int size;

    found = 0;
    if (length <= 0) {
        return -2;
    }
    while (1) {
        entry = (const AossRecord *)data;
        if (entry->type == sRecordType[slot]) {
            break;
        }
        step = AOSS_Ntohs(entry->length) + 4;
        length -= step;
        data += step;
        if (length <= 0) {
            return -4;
        }
    }
    data += 4;
    size = AOSS_Ntohs(entry->length);
    do {
        switch (*data) {
        case 3:
            result = GetWEPData((const AossRecord *)data, &slots[slot].WepData_40);
            found |= AOSS_TYPE_WEP40;
            break;
        case 4:
            result = GetWEPData((const AossRecord *)data, &slots[slot].WepData_104);
            found |= AOSS_TYPE_WEP104;
            break;
        case 5:
            result = GetWPAData((const AossRecord *)data, &slots[slot].TkipData);
            found |= AOSS_TYPE_TKIP;
            break;
        case 6:
            result = GetWPAData((const AossRecord *)data, &slots[slot].AesData);
            found |= AOSS_TYPE_AES;
            break;
        case 10:
            result = GetBssidData((const AossRecord *)data, session->ProductInfo[slot + 3]);
            break;
        default:
            result = -3;
            break;
        }
        if (result != 0) {
            return result;
        }
        next = AOSS_Ntohs(((const AossRecord *)data)->length) + 4;
        size -= next;
        data += next;
    } while (size > 0);
    sAoss.request.found |= found;
    return 0;
}

static int GetSecurityType(int flags) {
    int result = 0;

    if (flags & 0x10) {
        result = 1;
    }
    return result;
}

static int SendMessage(int state, const AossEndpoints *endpoints, const u8 *ids, int sock) {
    switch (state) {
    case 0:
        AOSS_StatusExclusion(2);
        return AstsSetInitReq(endpoints, ids, sock);
    case 1:
        AOSS_StatusExclusion(3);
        return AstsSetSecureReq(endpoints, ids, sock);
    case 2:
        AOSS_StatusExclusion(5);
        return AstsRestartReq(endpoints, ids, sock);
    }
    return -1;
}

static int AstsSetInitReq(const AossEndpoints *endpoints, const u8 *ids, int sock) {
    s8 checksum = 0;
    s16 length = 0;
    s16 flags = 0;
    u8 id[8];
    tagASTSpktHeader *header = (tagASTSpktHeader *)sAoss.packet;
    u8 *body;
    u8 *payload;

    AOSS_Memset(header, 0, AOSS_PACKET_SIZE);
    body = AOSS_MALLOC(0x210);
    if (body == NULL) {
        aoss_set_error(2);
        return -1;
    }
    AOSS_Memset(body, 0, 0x210);
    payload = (u8 *)(header + 1);
    AOSS_Memcpy(sPeerMac, ids, 8);
    AOSS_Memcpy(id, sPeerMac, 8);
    length = SetStaData(body + 4);
    if (length < 0) {
        aoss_set_error(3);
        if (body != NULL) {
            AOSS_FREE(body);
        }
        return -1;
    }
    body[0] = 0;
    *(u16 *)(body + 2) = AOSS_Htons(length);
    length += 4;
    PacketDataSet(0, payload, body, &length, &flags, &checksum);
    flags |= 0x10;
    if (CryptSeqNo(id, 8, AOSS_KEY, sizeof(AOSS_KEY)) != 0) {
        aoss_set_error(2);
        if (body != NULL) {
            AOSS_FREE(body);
        }
        return -1;
    }
    SetPacketHeader(header, 0x1000, length, flags, checksum, AOSS_PROTOCOL, id);
    length += sizeof(tagASTSpktHeader);
    SendPacketUDP(header, length, 0xff, sock);
    if (body != NULL) {
        AOSS_FREE(body);
    }
    return 0;
}

static int AstsSetSecureReq(const AossEndpoints *endpoints, const u8 *ids, int sock) {
    s8 checksum = 0;
    s16 length = 0;
    s16 flags = 0;
    u8 id[8];
    tagASTSpktResult record;
    tagASTSpktHeader *header = (tagASTSpktHeader *)sAoss.packet;

    AOSS_Memset(&record, 0, 8);
    AOSS_Memset(header, 0, AOSS_PACKET_SIZE);
    record.type = 2;
    record.reserved = 0;
    record.length = AOSS_Htons(4);
    record.Result = sAoss.request.wanted;
    record.Result = AOSS_Htonl(record.Result);
    length = 8;
    PacketDataSet(sAoss.encrypt, (u8 *)(header + 1), (u8 *)&record, &length, &flags, &checksum);
    AOSS_Memcpy(id, ids + 8, 8);
    if (CryptSeqNo(id, 8, AOSS_KEY, sizeof(AOSS_KEY)) != 0) {
        aoss_set_error(2);
        return -1;
    }
    SetPacketHeader(header, 0x2000, length, flags, checksum, AOSS_PROTOCOL, id);
    length += sizeof(tagASTSpktHeader);
    SendPacketUDP(header, length, 0, sock);
    return 0;
}

static int AstsRestartReq(const AossEndpoints *endpoints, const u8 *ids, int sock) {
    u8 id[8];
    tagASTSpktHeader *header = (tagASTSpktHeader *)sAoss.packet;

    AOSS_Memset(header, 0, AOSS_PACKET_SIZE);
    AOSS_Memcpy(id, ids + 16, 8);
    CryptSeqNo(id, 8, AOSS_KEY, AOSS_Strlen(AOSS_KEY));
    SetPacketHeader(header, 0x3000, 0, 0, 0, AOSS_PROTOCOL, id);
    SendPacketUDP(header, sizeof(tagASTSpktHeader), 0, sock);
    return 0;
}

// The body of the hello: the request's data and the item 0x60 with 14; returns its length
static s16 SetStaData(u8 *buffer) {
    AossItem *first = (AossItem *)buffer;
    s16 length;
    s16 total = 0;
    AossItem *second;
    s16 offset;
    u32 value;

    first->type = sAoss.request.version;
    first->reserved = 1;
    length = sAoss.request.length;
    AOSS_Memcpy(first->data, sAoss.request.data, length);
    first->length = AOSS_Htons(length);
    offset = ((s16)(length + 6) + 1) / 2 * 2;
    first->next = AOSS_Htons(offset);
    total += offset;
    second = (AossItem *)(buffer + offset);
    second->type = 0x60;
    second->reserved = 0;
    second->next = AOSS_Htons(0);
    value = AOSS_Htonl(0xe);
    AOSS_Memcpy(second->data, &value, 4);
    second->length = AOSS_Htons(4);
    total += 10;
    return total;
}

// Puts the body in the packet, encrypted when the client wants it so
static void PacketDataSet(int encrypt, u8 *dest, const u8 *src, s16 *length, s16 *flags, s8 *checksum) {
    if (encrypt == 1) {
        *flags = 1;
        EncodeRC4(src, dest + 4, *length, (u8 *)checksum, dest + 2, sPeerMac, 8);
        *(u16 *)dest = AOSS_Htons(*length);
        *length += 4;
    } else {
        AOSS_Memcpy(dest, src, *length);
    }
}

static void SetPacketHeader(tagASTSpktHeader *header, int type, int length, int flags, s8 checksum, s8 protocol,
                            const u8 *id) {
    header->PktVersion = AOSS_Htons(1);
    header->unk_02 = 0;
    header->unk_04 = 0;
    header->cmd = AOSS_Htons(type);
    header->type = 0;
    header->size = AOSS_Htons(length);
    header->CryptType = AOSS_Htons(flags);
    header->crc8 = checksum;
    header->Fragment = protocol;
    AOSS_Memcpy(header->SeqID, id, 8);
}

static int SendPacketUDP(void *buffer, int length, int toAll, int sock) {
    SOCSockAddrIn to;

    AOSS_Memset(&to, 0, sizeof(to));
    to.family = 2;
    to.port = AOSS_Htons(AOSS_PORT);
    to.addr.addr = AOSS_Htonl(sAoss.request.addr);
    if (toAll == 0xff || sAoss.request.reconnected == 0) {
        to.addr.addr = -1;
    }
    return AOSS_Sendto(sock, buffer, length, 0, &to, 8);
}

static int EncodeRC4(const u8 *in, u8 *out, u32 length, u8 *checksum, u8 *iv, const u8 *key, u32 keyLength) {
    RC4Context context;
    u16 number;

    *checksum = crc(in, length);
    context.s = AOSS_MALLOC(length);
    if (context.s == NULL) {
        return -1;
    }
    number = AOSS_Rand();
    AOSS_Memcpy(iv, &number, 2);
    AOSS_Memcpy(sCipherKey, iv, 2);
    AOSS_Memcpy(sCipherKey + 2, key, keyLength);
    RC4init(&context, sCipherKey, keyLength + 2, length);
    RC4encrypt(&context, out, in, length);
    AOSS_FREE(context.s);
    return 0;
}

static int DecodeRC4(const u8 *in, u8 *out, u32 length, u8 checksum, const u8 *iv, const u8 *key, u32 keyLength) {
    RC4Context context;

    context.s = AOSS_MALLOC(length);
    if (context.s == NULL) {
        aoss_set_error(2);
        return -1;
    }
    AOSS_Memcpy(sCipherKey, iv, 2);
    AOSS_Memcpy(sCipherKey + 2, key, keyLength);
    RC4init(&context, sCipherKey, keyLength + 2, length);
    RC4encrypt(&context, out, in, length);
    u8 actual = crc(out, length);
    if (actual != checksum) {
        aoss_set_error(0x12);
        AOSS_FREE(context.s);
        return -1;
    }
    AOSS_FREE(context.s);
    return 0;
}

// RC4 with a state of n bytes instead of 256
static void RC4init(RC4Context *context, const u8 *key, u32 keyLength, u32 n) {
    u8 *s = context->s;
    u32 i;
    u32 j;
    u32 k;

    context->i = context->j = 0;
    context->n = n;
    for (i = 0; i < n; i++) {
        s[i] = i;
    }
    j = 0;
    k = 0;
    for (i = 0; i < n; i++) {
        u8 value = s[i];
        u8 other;

        j = (j + key[k] + value) % context->n;
        other = s[j];
        s[j] = value;
        s[i] = other;
        k++;
        if (k >= keyLength) {
            k = 0;
        }
    }
}

static void RC4encrypt(RC4Context *context, u8 *out, const u8 *in, u32 length) {
    u32 i;

    for (i = 0; i < length; i++) {
        u8 key = arcfour_byte(context);

        out[i] = in[i] ^ key;
    }
}

static u32 arcfour_byte(RC4Context *context) {
    u8 *s = context->s;
    u32 n = context->n;
    u8 i = (context->i + 1) % n;
    u8 si = s[i];
    u8 j = (si + context->j) % n;
    u8 sj;

    sj = s[j];
    context->i = i;
    context->j = j;
    s[j] = si;
    s[i] = sj;
    return s[(si + sj) % context->n];
}

// The low byte of the complement of the CRC-32
static u8 crc(const u8 *data, int length) {
    return update_crc(-1, data, length, 0, sCrcTable) ^ 0xffffffff;
}

static u32 update_crc(u32 crc, const u8 *data, int length, int ready, u32 *table) {
    int i;

    if (ready == 0) {
        make_crc_table(ready, table);
    }
    for (i = 0; i < length; i++) {
        crc = (crc >> 8) ^ table[(u8)(data[i] ^ crc)];
    }
    return crc;
}

// Fills the table of the CRC-32; the caller passes its flag of a ready table, which isn't read
static void make_crc_table(int ready, u32 *table) {
    u32 value;
    int i;
    int bit;

    for (i = 0; i < 256; i++) {
        value = i;
        for (bit = 0; bit < 8; bit++) {
            if (value & 1) {
                value = (value >> 1) ^ 0xedb88320;
            } else {
                value >>= 1;
            }
        }
        *table++ = value;
    }
}

// Scrambles a block with two rounds of a Feistel network keyed by the text; 0 if it had the memory
static int CryptSeqNo(u8 *block, int length, const char *key, int keyLength) {
    char *mask;
    u8 *work;
    int round;

    mask = AOSS_MALLOC(length / 2);
    if (mask == NULL) {
        return -1;
    }
    work = AOSS_MALLOC(length);
    if (work == NULL) {
        AOSS_FREE(mask);
        return -1;
    }
    for (round = 0; round < 2; round++) {
        MakeKey(round, mask, length, key, keyLength);
        AddKey(mask, block, length);
        Rotate(block, length, work);
    }
    AOSS_FREE(mask);
    AOSS_FREE(work);
    return 0;
}

static void MakeKey(int round, char *out, int length, const char *key, int keyLength) {
    int half = length / 2;
    int k = round % keyLength;
    int i;

    for (i = 0; i < half; i++) {
        out[i] = i;
        out[i] ^= key[k];
        k++;
        if (k >= keyLength) {
            k = 0;
        }
    }
}

static void AddKey(const char *mask, u8 *block, int length) {
    int i;
    int half = length / 2;

    for (i = 0; i < half; i++) {
        block[half + i] ^= mask[i];
    }
}

static void Rotate(u8 *block, int length, u8 *work) {
    int half = length / 2;

    AOSS_Memcpy(work, block + half, half);
    AOSS_Memcpy(work + half, block, half);
    AOSS_Memcpy(block, work, length);
}

static void *Alloc(u32 name, s32 size) {
    if (size > 0) {
        return gAossApAlloc(size);
    }
    return NULL;
}

static void Free(u32 name, void *ptr, s32 size) {
    gAossApFree(ptr);
}

// Starts the socket library with a fixed address
static int AOSS_SetIPAddr(u32 addr, u32 netmask, u32 gateway) {
    sSocConfig.addr.addr = AOSS_Htonl(addr);
    sSocConfig.netmask.addr = AOSS_Htonl(netmask);
    sSocConfig.router.addr = AOSS_Htonl(gateway);
    if (SOC_Startup(&sSocConfig) < 0) {
        return -1;
    }
    if (CPSMyIp == 0) {
        do {
            OS_Sleep(100);
        } while (CPSMyIp == 0);
    }
    return 0;
}

static int AOSS_Disconnect(void) {
    if (SOC_Cleanup() < 0) {
        return -1;
    }
    return -(AOSSi_Disconnect() != 0);
}

static int AOSS_Memcmp(const u8 *a, const u8 *b, int size) {
    int difference = 0;

    while (size-- > 0 && (difference = *a - *b) == 0) {
        a++;
        b++;
    }
    return difference;
}

static void AOSS_Memcpy(void *dest, const void *src, u32 size) {
    MI_CpuCopy8(src, dest, size);
}

static void AOSS_Memset(void *dest, int value, u32 size) {
    MI_CpuFill8(dest, (u8)value, size);
}

static int AOSS_Recvfrom(int sock, void *buffer, int size, int flags, SOCSockAddrIn *from, int *fromLength) {
    from->len = *fromLength;
    return SOC_RecvFrom(sock, buffer, size, flags, from);
}

// Waits for the descriptor to be ready for at most the time
static int AOSS_Select(int count, SOCPollFD *readFds, void *writeFds, void *exceptFds, const AOSS_timeval *timeout) {
    SOCPollFD fd = *readFds;
    u64 ticks = 0;

    ticks += ((u64)(s64)timeout->tv_sec * 33514000) >> 6;
    ticks += ((u64)(s64)timeout->tv_usec * 33514000) >> 6;
    return SOC_Poll(&fd, 1, ticks);
}

static int AOSS_Sendto(int sock, const void *buffer, int size, int flags, SOCSockAddrIn *to, int toLength) {
    to->len = toLength;
    return SOC_SendTo(sock, buffer, size, flags, to);
}

// The socket library takes no options
static int AOSS_Setsockopt(int sock, int level, int name, const void *value, int length) {
}

static int AOSS_Socket(int domain, int type, int protocol) {
    return SOC_Socket(domain, type, protocol);
}

static int AOSS_Bind(int sock, SOCSockAddrIn *address, int length) {
    address->len = length;
    return SOC_Bind(sock, address);
}

static int AOSS_Close(int sock) {
    return SOC_Close(sock);
}

static u32 AOSS_Htonl(u32 value) {
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

static u16 AOSS_Htons(u16 value) {
    return ((value >> 8) & 0xff) | ((value << 8) & 0xff00);
}

static u32 AOSS_Ntohl(u32 value) {
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

static u16 AOSS_Ntohs(u16 value) {
    return ((value >> 8) & 0xff) | ((value << 8) & 0xff00);
}

static u32 AOSS_Strlen(const char *text) {
    u32 length = 0;

    while (text[length] != '\0') {
        length++;
    }
    return length;
}

// A linear congruential generator, seeded with the time
static u16 AOSS_Rand(void) {
    if (sAoss.randReady == 0) {
        RTCTime time;
        u32 seed = 0;

        AOSS_Memset(&time, 0, sizeof(time));
        if (RTC_GetTime(&time) == 0) {
            seed += time.hour << 10;
            seed += time.minute << 3;
            seed += time.second;
        }
        sAoss.randState = seed;
        sAoss.randMul = 0x5d588b65;
        sAoss.randAdd = 0x269ec3;
        sAoss.randReady = 1;
    }
    sAoss.randState = sAoss.randMul * sAoss.randState + sAoss.randAdd;
    return ((sAoss.randState >> 16) * 0x7fff) >> 16;
}

static void AOSS_FD_ZERO(SOCPollFD *fd) {
    fd->fd = 0;
    fd->events = 0;
    fd->revents = 0;
}

static void AOSS_FD_SET(int sock, SOCPollFD *fd) {
    fd->fd = sock;
    fd->events = 1;
}
