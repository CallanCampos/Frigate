#include "dwc_aoss.h"
#include "dwc/ext.h"
#include "dwc_aoss_ap.h"
#include "dwc_wcm.h"
#include "gfl/std.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nitro/wm.h"

// The AOSS client of DWC's settings (see dwc_aoss.h): Buffalo's one-touch setup. It looks for the access point named
// ESSID-AOSS, joins it with a fixed WEP key, takes an address in its subnet and talks to it in UDP packets on port
// 22416 in three stages, each answered by the access point: the hello (0x1000), the security settings that it offers
// (0x2000) and the end (0x3000). The packets are encrypted with RC4 under the id of the access point. It ends with the
// keys of the network. The file also has the small network helpers that it works with; the Wi-Fi scan and connect are
// in dwc_aoss_ap.c. The ROM names none of it; the names are ours.
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
typedef struct AossHeader {
    u16 version; // 0x00: 1
    u16 unk_02;
    u16 unk_04;
    u16 type; // 0x06: 0x1010 + 0x1000 for each stage
    u16 unk_08;
    u16 length;  // 0x0a: of the body
    u16 flags;   // 0x0c: bit 4 says it is encrypted
    s8 checksum; // 0x0e
    u8 protocol; // 0x0f
    u8 id[8];    // 0x10: of a client, 6 bytes of address and 2 of a number
} AossHeader;

// A record with a status
typedef struct AossStatusRecord {
    u8 type;
    u8 reserved;
    u16 length; // big endian
    s32 status; // big endian
} AossStatusRecord;

// The body of an encrypted message
typedef struct AossEncrypted {
    u16 length; // big endian, of the data
    u8 iv[2];
    u8 data[1];
} AossEncrypted;

typedef struct AossPacket {
    int sock;
    int length;
    int unk_08;
    AossHeader header;
    u8 body[AOSS_PACKET_SIZE - sizeof(AossHeader)];
} AossPacket;

// What the hello has: two names and a number
typedef struct AossHello {
    u8 name[0x80];
    u8 info[0x80];
    u8 extra[4];
} AossHello;

// The hello's fields (see AossHello) take the first three buffers, and the keys of the stages the next ones
typedef struct AossSession {
    u8 buffer[5][0x80];
} AossSession;

typedef struct AossWepInfo {
    u32 unk_00;
    u32 keyLength;
    char ssid[0x28];
    u8 key[4][0x40];
} AossWepInfo;

typedef struct AossPskInfo {
    u32 unk_00;
    u32 keyLength;
    char ssid[0x28];
    u8 key[0x40];
} AossPskInfo;

typedef struct AossApInfo {
    u32 unk_00;
    u32 unk_04;
    AossWepInfo wep40;
    AossWepInfo wep104;
    AossPskInfo tkip;
    AossPskInfo aes;
    u32 unk_348[2];
} AossApInfo;

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

typedef struct AossRetry {
    s16 count;
    s16 delay;
} AossRetry;

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

typedef struct SocTimeval {
    int sec;
    int usec;
} SocTimeval;

static void *Aoss_Alloc(u32 size);
static void Aoss_Free(void *ptr);
static int Aoss_Run(AossParam *param);
static u32 Aoss_NextAddress(u32 addr, u32 netmask);
static void Aoss_ReleaseBuffers(void);
static void Aoss_Begin(AossParam *param);
static void Aoss_SetError(int error);
static int Aoss_GetError(void);
static int Aoss_StoreResult(AossParam *param);
static int Aoss_IsPrintable(const u8 *text, int length);
static int Aoss_CheckScan(const WifiScanList *list);
static int Aoss_InitTarget(WifiConnectParam *target);
static void Aoss_MakeIds(int count, u8 *ids, const u8 *mac);
static int Aoss_SetMode(int mode);
static int Aoss_ProcessPacket(int state, AossPacket *packet, int *errors, const u8 *ids, int sock);
static int Aoss_OnStage0(int state, AossPacket *packet, int *errors, const u8 *ids);
static int Aoss_OnStage1(int state, AossPacket *packet, int *errors, const u8 *ids);
static int Aoss_OnStage2(int state, AossPacket *packet, int *errors, const u8 *ids);
static int Aoss_DecodePacket(AossHeader *header);
static int Aoss_CheckPeer(int type, const u8 *id);
static int Aoss_CheckIds(const u8 *expected, u8 *received);
static int Aoss_ParseHello(const AossItem *items, AossHello *hello);
static u32 Aoss_ReadLe(const u8 *data, int length);
static int Aoss_ParseWep(const AossRecord *record, AossWepInfo *info);
static int Aoss_ParsePsk(const AossRecord *record, AossPskInfo *info);
static int Aoss_ParseKey(const AossRecord *record, u8 *key);
static int Aoss_ParseSecurity(int slot, const AossRecord *record, int length, AossApInfo *slots, AossSession *session);
static int Aoss_IsEncrypted(int flags);
static int Aoss_Send(int state, const AossEndpoints *endpoints, const u8 *ids, int sock);
static int Aoss_SendStage0(const AossEndpoints *endpoints, const u8 *ids, int sock);
static int Aoss_SendStage1(const AossEndpoints *endpoints, const u8 *ids, int sock);
static int Aoss_SendStage2(const AossEndpoints *endpoints, const u8 *ids, int sock);
static s16 Aoss_BuildHello(u8 *buffer);
static void Aoss_BuildPayload(int encrypt, u8 *dest, const u8 *src, s16 *length, s16 *flags, s8 *checksum);
static void Aoss_BuildHeader(AossHeader *header, int type, int length, int flags, s8 checksum, s8 protocol,
                             const u8 *id);
static int Aoss_SendPacket(void *buffer, int length, int toAll, int sock);
static int Aoss_Encrypt(const u8 *in, u8 *out, u32 length, u8 *checksum, u8 *iv, const u8 *key, u32 keyLength);
static int Aoss_Decrypt(const u8 *in, u8 *out, u32 length, u8 checksum, const u8 *iv, const u8 *key, u32 keyLength);
static void Rc4_Init(RC4Context *context, const u8 *key, u32 keyLength, u32 n);
static void Rc4_Crypt(RC4Context *context, u8 *out, const u8 *in, u32 length);
static u32 Rc4_Next(RC4Context *context);
static u8 Aoss_Checksum(const u8 *data, int length);
static u32 Crc32_Update(u32 crc, const u8 *data, int length, int ready, u32 *table);
static void Crc32_MakeTable(int i, u32 *table);
static int Aoss_Scramble(u8 *block, int length, const char *key, int keyLength);
static void Aoss_ScrambleKey(int round, char *out, int length, const char *key, int keyLength);
static void Aoss_ScrambleXor(const char *mask, u8 *block, int length);
static void Aoss_ScrambleSwap(u8 *block, int length, u8 *work);
static void *Soc_Alloc(u32 name, s32 size);
static void Soc_Free(u32 name, void *ptr, s32 size);
static int Net_Start(u32 addr, u32 netmask, u32 gateway);
static int Net_Stop(void);
static int Mem_Compare(const u8 *a, const u8 *b, int size);
static void Mem_Copy(void *dest, const void *src, u32 size);
static void Mem_Fill(void *dest, int value, u32 size);
static int Net_RecvFrom(int sock, void *buffer, int size, int flags, SocAddrIn *from, int *fromLength);
static int Net_Poll(int count, SocPollFd *readFds, void *writeFds, void *exceptFds, const SocTimeval *timeout);
static int Net_SendTo(int sock, const void *buffer, int size, int flags, SocAddrIn *to, int toLength);
static int Net_SetSockOpt(int sock, int level, int name, const void *value, int length);
static int Net_Socket(int domain, int type, int protocol);
static int Net_Bind(int sock, SocAddrIn *address, int length);
static int Net_Close(int sock);
static u32 Net_Htonl(u32 value);
static u32 Net_Htons(u16 value);
static u32 Net_Ntohl(u32 value);
static u32 Net_Ntohs(u16 value);
static u32 Str_Length(const char *text);
static u16 Aoss_Rand(void);
static void Net_PollInit(SocPollFd *fd);
static void Net_PollSet(int sock, SocPollFd *fd);

static u8 sRecordType[3] = { 9, 8, 0 };
static struct {
    int socket;
    int mode;
} sNet = { -1, -1 };
static SocConfig sSocConfig = { 0x01000000, Soc_Alloc, Soc_Free, 0, 0, 0, 0, 0, 0, 0x1000, 0x1000 };

static u8 sCipherKey[0x78];
static u8 sPeerMac[8];
static AossApInfo sApInfo[2];
static u32 sCrcTable[256];
static AossSession sSession;
static AossWork sAoss;

static void *Aoss_Alloc(u32 size) {
    return gAossAp.alloc(size);
}

static void Aoss_Free(void *ptr) {
    gAossApFree(ptr);
}

int DWC_Aoss_Start(AossParam *param) {
    int result;

    if (param->scanCount == 0 || param->scanCount < -1 || param->scanDelay < -1 || param->sendCount == 0 ||
        param->sendCount < -1 || param->sendDelay < -1 || param->timeout < -1 || param->length == 0 ||
        param->length > 0x100 || param->data[param->length - 1] != 0) {
        result = -1;
    } else {
        result = 0;
    }
    if (gAossAp.alloc == NULL || gAossApFree == NULL) {
        result = -1;
    }
    if (result == -1) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    sAoss.packet = Aoss_Alloc(AOSS_BUFFER_SIZE);
    if (sAoss.packet == NULL) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    Aoss_SetMode(-1);
    result = Aoss_Run(param);
    Aoss_Free(sAoss.packet);
    Aoss_ReleaseBuffers();
    if (sNet.socket != -1) {
        Net_Close(sNet.socket);
    }
    return result;
}

static int Aoss_Run(AossParam *param) {
    u8 ids[3][8];
    WifiConnectParam target;
    AossEndpoints endpoints;
    AossRetry scan = { -1, -1 };
    AossRetry send = { 0, 0 };
    int option = 1;
    int sends = 0;
    SocAddrIn from;
    int fromLength;
    SocPollFd pollFd;
    SocTimeval timeval;
    SocAddrIn addr;
    int scanResult;
    int state = 0;
    int newState;
    u32 ip = 0;
    AossPacket *packet;
    int sec;
    int usec;
    int timeout;
    int ret;
    s16 i;

    Mem_Fill(ids, state, sizeof(ids));
    scan.count = param->scanCount;
    if (scan.count == -1) {
        scan.count = 10;
    }
    send.count = param->sendCount;
    if (send.count == -1) {
        send.count = 10;
    }
    scan.delay = param->scanDelay;
    if (scan.delay == -1) {
        scan.delay = 100;
    }
    send.delay = param->sendDelay;
    if (send.delay == -1) {
        send.delay = 100;
    }
    timeout = param->timeout;
    if (timeout == -1) {
        timeout = 2000;
    }
    Aoss_Begin(param);
    if ((sAoss.request.wanted & AOSS_TYPE_WEP40) != 1) {
        Aoss_SetError(0x13);
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }

    i = 0;
    Aoss_SetMode(0);
    while (1) {
        if (sAoss.scanList != NULL) {
            Aoss_Free(sAoss.scanList);
            sAoss.scanList = NULL;
        }
        if (AossAp_Scan((WifiScanList **)&sAoss.scanList) == -1) {
            param->error = 0xf;
            Aoss_ReleaseBuffers();
            return -1;
        }
        ret = Aoss_CheckScan(sAoss.scanList);
        if (ret == 4) {
            param->error = 2;
            Aoss_ReleaseBuffers();
            return -1;
        }
        if (ret == 0) {
            break;
        }
        if (i >= scan.count) {
            param->error = 1;
            Aoss_ReleaseBuffers();
            return -1;
        }
        AossAp_Sleep(scan.delay);
        i++;
    }

    Aoss_SetMode(1);
    Mem_Fill(&target, 0, sizeof(target));
    if (Aoss_InitTarget(&target) != 0) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    sAoss.apInfo = Aoss_Alloc(sizeof(WifiConnectResult));
    if (sAoss.apInfo == NULL) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    Mem_Fill(sAoss.apInfo, 0, sizeof(WifiConnectResult));
    for (i = 0; i < scan.count; i++) {
        ret = AossAp_Connect(&target, sAoss.apInfo);
        if (ret == -1) {
            param->error = 0xf;
            Aoss_ReleaseBuffers();
            return -1;
        }
        if (ret == 0 && ((WifiConnectResult *)sAoss.apInfo)->success == 1) {
            break;
        }
        AossAp_Sleep(scan.delay);
    }
    if (i == scan.count) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    if (Net_Start(AOSS_CLIENT_ADDR, ~0xff, AOSS_CLIENT_ADDR) != 0) {
        Aoss_SetError(0xc);
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    Aoss_ReleaseBuffers();
    Aoss_MakeIds(3, ids[0], param->mac);
    sNet.socket = Net_Socket(2, 2, 0);
    if (sNet.socket < 0) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    if (Net_SetSockOpt(sNet.socket, 0xffff, 1, &option, 4) < 0) {
        Aoss_SetError(0xb);
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    Mem_Fill(&addr, 0, sizeof(addr));
    addr.family = 2;
    addr.addr = Net_Htonl(AOSS_CLIENT_ADDR);
    addr.port = Net_Htons(AOSS_PORT);
    if (Net_Bind(sNet.socket, &addr, 8) < 0) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }

    while (1) {
        packet = sAoss.packet;
        Mem_Fill(&endpoints, 0, sizeof(endpoints));
        endpoints.client = AOSS_CLIENT_ADDR;
        endpoints.router = AOSS_ROUTER_ADDR;
        sec = timeout / 1000;
        usec = timeout % 1000 * 1000;
        while (1) {
            if (state == 1 && sAoss.request.reconnected != 1) {
                if (sNet.socket != -1) {
                    Net_Close(sNet.socket);
                }
                sNet.socket = -1;
                if (Net_Stop() != 0) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                sAoss.scanList = Aoss_Alloc(sizeof(WifiConnectResult));
                if (sAoss.scanList == NULL) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                while (1) {
                    if (sAoss.scanList != NULL) {
                        Aoss_Free(sAoss.scanList);
                        sAoss.scanList = NULL;
                    }
                    scanResult = AossAp_Scan((WifiScanList **)&sAoss.scanList);
                    if (scanResult == -1) {
                        param->error = 0xf;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    ret = Aoss_CheckScan(sAoss.scanList);
                    if (ret == 4) {
                        param->error = 2;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    if (ret == 0) {
                        break;
                    }
                    if (i >= scan.count) {
                        param->error = 1;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    AossAp_Sleep(scan.delay);
                    i++;
                }
                if (scanResult == -1) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                sAoss.apInfo = Aoss_Alloc(sizeof(WifiConnectResult));
                if (sAoss.apInfo == NULL) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                Mem_Fill(sAoss.apInfo, 0, sizeof(WifiConnectResult));
                for (i = 0; i < scan.count; i++) {
                    ret = AossAp_Connect(&target, sAoss.apInfo);
                    if (ret == -1) {
                        param->error = 0xf;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    if (ret == 0 && ((WifiConnectResult *)sAoss.apInfo)->success == 1) {
                        break;
                    }
                    AossAp_Sleep(scan.delay);
                }
                if (i == scan.count) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                ip = Aoss_NextAddress(sAoss.request.addr, sAoss.request.netmask);
                if (Net_Start(ip, sAoss.request.netmask, ip) != 0) {
                    Aoss_SetError(0xc);
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                sAoss.request.reconnected = 1;
                Aoss_ReleaseBuffers();
                sNet.socket = Net_Socket(2, 2, 0);
                if (sNet.socket < 0) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                if (Net_SetSockOpt(sNet.socket, 0xffff, 1, &option, 4) < 0) {
                    Aoss_SetError(0xb);
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                Mem_Fill(&addr, 0, sizeof(addr));
                addr.family = 2;
                addr.addr = Net_Htonl(ip);
                addr.port = Net_Htons(AOSS_PORT);
                if (Net_Bind(sNet.socket, &addr, 8) < 0) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
            }

            if (Aoss_Send(state, &endpoints, ids[0], sNet.socket) == -1) {
                Aoss_SetError(0x1000 + state);
                param->error = 0xf;
                Aoss_ReleaseBuffers();
                return -1;
            }
            Mem_Fill(packet, 0, AOSS_BUFFER_SIZE);
            Net_PollInit(&pollFd);
            Net_PollSet(sNet.socket, &pollFd);
            timeval.sec = sec;
            timeval.usec = usec;
            if (Net_Poll(sNet.socket + 1, &pollFd, NULL, NULL, &timeval) <= 0) {
                sends++;
                if (sends > send.count) {
                    if (state == 0) {
                        Aoss_SetError(0xf);
                    } else if (state == 1) {
                        Aoss_SetError(0x10);
                    } else {
                        Aoss_SetError(0x11);
                    }
                    ret = -1;
                    goto done;
                }
                AossAp_Sleep(send.delay);
                continue;
            }
            fromLength = 8;
            packet->sock = sNet.socket;
            packet->length =
                Net_Ntohs(Net_RecvFrom(sNet.socket, &packet->header, AOSS_PACKET_SIZE, 0, &from, &fromLength));
            newState = Aoss_ProcessPacket(state, packet, &sends, ids[0], sNet.socket);
            if (newState == 100) {
                ret = 0;
                goto done;
            }
            if (newState == -1) {
                ret = -1;
                goto done;
            }
            if (state == newState) {
                if (sends > send.count) {
                    if (newState == 0) {
                        Aoss_SetError(0xf);
                    } else if (newState == 1) {
                        Aoss_SetError(0x10);
                    } else {
                        Aoss_SetError(0x11);
                    }
                    ret = -1;
                    goto done;
                }
                AossAp_Sleep(send.delay);
                break;
            }
            if (newState == 2) {
                if (sNet.socket != -1) {
                    Net_Close(sNet.socket);
                }
                sNet.socket = -1;
                if (Net_Stop() != 0) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                i = 0;
                Aoss_SetMode(4);
                while (1) {
                    if (sAoss.scanList != NULL) {
                        Aoss_Free(sAoss.scanList);
                        sAoss.scanList = NULL;
                    }
                    scanResult = AossAp_Scan((WifiScanList **)&sAoss.scanList);
                    if (scanResult == -1) {
                        param->error = 0xf;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    ret = Aoss_CheckScan(sAoss.scanList);
                    if (ret == 4) {
                        param->error = 2;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    if (ret == 0) {
                        break;
                    }
                    if (i >= scan.count) {
                        param->error = 1;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    AossAp_Sleep(scan.delay);
                    i++;
                }
                sAoss.apInfo = Aoss_Alloc(sizeof(WifiConnectResult));
                if (sAoss.apInfo == NULL) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                Mem_Fill(sAoss.apInfo, 0, sizeof(WifiConnectResult));
                for (i = 0; i < scan.count; i++) {
                    ret = AossAp_Connect(&target, sAoss.apInfo);
                    if (ret == -1) {
                        param->error = 0xf;
                        Aoss_ReleaseBuffers();
                        return -1;
                    }
                    if (ret == 0 && ((WifiConnectResult *)sAoss.apInfo)->success == 1) {
                        break;
                    }
                    AossAp_Sleep(scan.delay);
                }
                if (i == scan.count) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                if (Net_Start(ip, sAoss.request.netmask, ip) != 0) {
                    Aoss_SetError(0xc);
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                Aoss_ReleaseBuffers();
                sNet.socket = Net_Socket(2, 2, 0);
                if (sNet.socket < 0) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                if (Net_SetSockOpt(sNet.socket, 0xffff, 1, &option, 4) < 0) {
                    Aoss_SetError(0xb);
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
                Mem_Fill(&addr, 0, sizeof(addr));
                addr.family = 2;
                addr.addr = Net_Htonl(ip);
                addr.port = Net_Htons(AOSS_PORT);
                if (Net_Bind(sNet.socket, &addr, 8) < 0) {
                    param->error = 0xf;
                    Aoss_ReleaseBuffers();
                    return -1;
                }
            }
            state = newState;
            break;
        }
    }

done:
    if (sNet.socket != -1) {
        Net_Close(sNet.socket);
    }
    sNet.socket = -1;
    if (Net_Stop() != 0) {
        param->error = 0xf;
        Aoss_ReleaseBuffers();
        return -1;
    }
    if (ret != 0) {
        u8 error;

        switch (Aoss_GetError()) {
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
        param->error = error;
        Aoss_ReleaseBuffers();
        return -1;
    }
    if (Aoss_StoreResult(param) != 0) {
        param->error = 6;
        Aoss_ReleaseBuffers();
        return -1;
    }
    return 0;
}

// The first address after addr in its subnet, or the first one if that is the last
static u32 Aoss_NextAddress(u32 addr, u32 netmask) {
    u32 network = addr & netmask;
    u32 next = ((addr & ~netmask) + 1) | network;

    if (next >= (~netmask | network)) {
        return network | 1;
    }
    return next;
}

static void Aoss_ReleaseBuffers(void) {
    if (sAoss.apInfo != NULL) {
        Aoss_Free(sAoss.apInfo);
        sAoss.apInfo = NULL;
    }
    if (sAoss.scanList != NULL) {
        Aoss_Free(sAoss.scanList);
        sAoss.scanList = NULL;
    }
}

static void Aoss_Begin(AossParam *param) {
    Mem_Fill(sPeerMac, 0, 8);
    sAoss.error = 1;
    Mem_Fill(&sAoss.request, 0, sizeof(AossRequest));
    sAoss.request.data = param->data;
    sAoss.request.length = param->length;
    sAoss.request.wanted = param->types & 0xf;
    sAoss.request.version = param->version;
    sAoss.request.found = 0;
    sAoss.request.addr = AOSS_ROUTER_ADDR;
    sAoss.request.reconnected = 0;
}

static void Aoss_SetError(int error) {
    sAoss.error = error;
}

static int Aoss_GetError(void) {
    return sAoss.error;
}

// Copies the keys that the access point sent to the answer, if they are fit
static int Aoss_StoreResult(AossParam *param) {
    AossPskInfo *aes = &sApInfo[0].aes;
    AossPskInfo *tkip = &sApInfo[0].tkip;
    AossWepInfo *wep104 = &sApInfo[0].wep104;
    AossWepInfo *wep40 = &sApInfo[0].wep40;
    AossResult *result = &param->result;

    if (result == NULL) {
        return -1;
    }
    param->types = sAoss.request.wanted & sAoss.request.found;
    Mem_Fill(result, 0, sizeof(AossResult));
    if (param->types & AOSS_TYPE_WEP40) {
        Mem_Copy(result->wep40Key[0], wep40->key[0], wep40->keyLength);
        Mem_Copy(result->wep40Key[1], wep40->key[1], wep40->keyLength);
        Mem_Copy(result->wep40Key[2], wep40->key[2], wep40->keyLength);
        Mem_Copy(result->wep40Key[3], wep40->key[3], wep40->keyLength);
        if (Aoss_IsPrintable((u8 *)wep40->ssid, Str_Length(wep40->ssid)) != 0) {
            goto fail;
        }
        Mem_Copy(result->wep40Ssid, wep40->ssid, Str_Length(wep40->ssid));
    }
    if (param->types & AOSS_TYPE_WEP104) {
        Mem_Copy(result->wep104Key[0], wep104->key[0], wep104->keyLength);
        Mem_Copy(result->wep104Key[1], wep104->key[1], wep104->keyLength);
        Mem_Copy(result->wep104Key[2], wep104->key[2], wep104->keyLength);
        Mem_Copy(result->wep104Key[3], wep104->key[3], wep104->keyLength);
        if (Aoss_IsPrintable((u8 *)wep104->ssid, Str_Length(wep104->ssid)) != 0) {
            goto fail;
        }
        Mem_Copy(result->wep104Ssid, wep104->ssid, Str_Length(wep104->ssid));
    }
    if (param->types & AOSS_TYPE_TKIP) {
        if (Aoss_IsPrintable(tkip->key, tkip->keyLength - 1) != 0) {
            goto fail;
        }
        Mem_Copy(result->tkipKey, tkip->key, tkip->keyLength);
        if (Aoss_IsPrintable((u8 *)tkip->ssid, Str_Length(tkip->ssid)) != 0) {
            goto fail;
        }
        Mem_Copy(result->tkipSsid, tkip->ssid, Str_Length(tkip->ssid));
    }
    if (param->types & AOSS_TYPE_AES) {
        if (Aoss_IsPrintable(aes->key, aes->keyLength - 1) != 0) {
            goto fail;
        }
        Mem_Copy(result->aesKey, aes->key, aes->keyLength);
        if (Aoss_IsPrintable((u8 *)aes->ssid, Str_Length(aes->ssid)) != 0) {
            goto fail;
        }
        Mem_Copy(result->aesSsid, aes->ssid, Str_Length(aes->ssid));
    }
    param->error = 0;
    return 0;

fail:
    Mem_Fill(result, 0, sizeof(AossResult));
    return -1;
}

static int Aoss_IsPrintable(const u8 *text, int length) {
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
static int Aoss_CheckScan(const WifiScanList *list) {
    int result = 0;
    int found = 0;
    int count = list->count;
    int i;

    if (count == 0) {
        return 5;
    }
    if (count > 0x40) {
        count = 0x40;
    }
    for (i = 0; i < count; i++) {
        const WifiApInfo *ap = &list->ap[i];

        if ((ap->type & 1) && ap->ssidLength == Str_Length(AOSS_SSID) &&
            Mem_Compare(ap->ssid, (u8 *)AOSS_SSID, Str_Length(AOSS_SSID)) == 0) {
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
static int Aoss_InitTarget(WifiConnectParam *target) {
    target->ssidLength = Str_Length(AOSS_SSID);
    Mem_Copy(target->ssid, AOSS_SSID, target->ssidLength);
    target->authMode = 1;
    target->keyLength = Str_Length(AOSS_KEY);
    if (target->keyLength > 13) {
        return -1;
    }
    Mem_Copy(target->key, AOSS_KEY, target->keyLength);
    return 0;
}

static void Aoss_MakeIds(int count, u8 *ids, const u8 *mac) {
    typedef struct {
        u8 mac[6];
        u16 number;
    } Id;
    Id *id = (Id *)ids;
    int i;

    for (i = 0; i < count; i++) {
        Mem_Copy(id->mac, mac, 6);
        id->number = Aoss_Rand();
        id->number = Net_Htons(id->number);
        id++;
    }
}

static int Aoss_SetMode(int mode) {
    if (mode == -1) {
        sNet.mode = mode;
        return 0;
    }
    if (sNet.mode != mode) {
        sNet.mode = mode;
        return AossAp_NotifyModeChange(mode);
    }
    return 0;
}

static int Aoss_ProcessPacket(int state, AossPacket *packet, int *errors, const u8 *ids, int sock) {
    AossHeader *header = &packet->header;

    if (Net_Ntohs(header->version) < 1) {
        (*errors)++;
        return state;
    }
    if (header->protocol != AOSS_PROTOCOL) {
        (*errors)++;
        return state;
    }
    if (Aoss_DecodePacket(header) > 0) {
        (*errors)++;
        return state;
    }
    switch (Net_Ntohs(header->type)) {
    case 0x1010:
        state = Aoss_OnStage0(state, packet, errors, ids);
        break;
    case 0x2010:
        state = Aoss_OnStage1(state, packet, errors, ids);
        break;
    case 0x3010:
        state = Aoss_OnStage2(state, packet, errors, ids);
        break;
    }
    return state;
}

static int Aoss_OnStage0(int state, AossPacket *packet, int *errors, const u8 *ids) {
    AossHeader *header;
    AossRecord *record;
    int result;

    if (state != 0) {
        (*errors)++;
        return state;
    }
    header = &packet->header;
    record = (AossRecord *)packet->body;
    if (Aoss_CheckIds(ids, header->id) < 0) {
        (*errors)++;
        return state;
    }
    if (Net_Ntohs(record->length) == 0) {
        (*errors)++;
        return state;
    }
    if (record->type == 7) {
        if (Net_Ntohl(record->payload.status) == -2) {
            Aoss_SetError(0x14);
        } else if (Net_Ntohl(record->payload.status) == -3) {
            Aoss_SetError(0x15);
        } else {
            Aoss_SetError(0x18);
        }
        return -1;
    }
    if (record->type != 1) {
        (*errors)++;
        return state;
    }
    result = Aoss_ParseHello(record->payload.items, (AossHello *)sSession.buffer[0]);
    if (result < 0) {
        if (result == -2) {
            Aoss_SetError(0x16);
            return -1;
        }
        (*errors)++;
        return state;
    }
    sAoss.encrypt = Aoss_IsEncrypted(Net_Ntohs(header->flags));
    *errors = 0;
    return 1;
}

static int Aoss_OnStage1(int state, AossPacket *packet, int *errors, const u8 *ids) {
    AossHeader *header;
    AossRecord *record;

    if (state != 1) {
        (*errors)++;
        return state;
    }
    header = &packet->header;
    record = (AossRecord *)packet->body;
    if (Aoss_CheckIds(ids + 8, header->id) < 0) {
        (*errors)++;
        return state;
    }
    if (Net_Ntohs(record->length) == 0) {
        (*errors)++;
        return state;
    }
    if (record->type == 7) {
        if (Net_Ntohl(record->payload.status) == -2) {
            Aoss_SetError(0x14);
        } else if (Net_Ntohl(record->payload.status) == -3) {
            Aoss_SetError(0x15);
        } else {
            Aoss_SetError(0x18);
        }
        return -1;
    }
    Mem_Fill(sApInfo, 0, sizeof(sApInfo));
    if (Aoss_ParseSecurity(0, record, Net_Ntohs(header->length), sApInfo, &sSession) < 0) {
        (*errors)++;
        return state;
    }
    if ((sAoss.request.found & sAoss.request.wanted) == 0) {
        return state;
    }
    *errors = 0;
    return 2;
}

static int Aoss_OnStage2(int state, AossPacket *packet, int *errors, const u8 *ids) {
    AossHeader *header;
    AossRecord *record;

    if (state != 2) {
        (*errors)++;
        return state;
    }
    record = (AossRecord *)packet->body;
    header = &packet->header;
    if (Aoss_CheckIds(ids + 16, header->id) < 0) {
        (*errors)++;
        return state;
    }
    if (record->type != 7) {
        (*errors)++;
        return state;
    }
    if (Net_Ntohs(record->length) == 0) {
        (*errors)++;
        return state;
    }
    if (Net_Ntohl(record->payload.status) == 0) {
        return 100;
    }
    if (Net_Ntohl(record->payload.status) == -2) {
        Aoss_SetError(0x14);
        return -1;
    }
    if (Net_Ntohl(record->payload.status) == -3) {
        Aoss_SetError(0x15);
        return -1;
    }
    Aoss_SetError(0x18);
    return -1;
}

// Checks the id of a message and decrypts its body; 0 if it is fit
static int Aoss_DecodePacket(AossHeader *header) {
    AossEncrypted *body = (AossEncrypted *)(header + 1);
    u8 id[8];
    u32 length;
    u8 *buffer;
    int result;

    Mem_Copy(id, header->id, 8);
    if (Aoss_Scramble(id, 8, AOSS_KEY, Str_Length(AOSS_KEY)) == -1) {
        Aoss_SetError(2);
        return -100;
    }
    result = Aoss_CheckPeer(Net_Ntohs(header->type), id);
    if (result != 0) {
        return result;
    }
    if (Net_Ntohs(header->type) == 0x1000) {
        Mem_Copy(sPeerMac, id, 8);
    }
    if ((Net_Ntohs(header->flags) & 0xf) == 0) {
        return 0;
    }
    length = Net_Ntohs(body->length);
    buffer = Aoss_Alloc(length);
    if (buffer == NULL) {
        Aoss_SetError(2);
        return 100;
    }
    if (Aoss_Decrypt(body->data, buffer, length, header->checksum, body->iv, sPeerMac, 8) < 0) {
        Aoss_Free(buffer);
        if (Aoss_GetError() == 2) {
            return 100;
        }
        return 200;
    }
    Mem_Copy(body, buffer, length);
    header->length = Net_Htons(length);
    Aoss_Free(buffer);
    return 0;
}

// Whether a message may come from the access point we talk to: 0 if so, 1 if it is another, 2 if there is none yet
static int Aoss_CheckPeer(int type, const u8 *id) {
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
        if (Mem_Compare(sPeerMac, id, 6) != 0) {
            result = 1;
        }
    } else if (type != 0x1000) {
        result = 2;
    }
    return result;
}

// Whether the id that came back is the one that was sent, with its number one higher: 0 if so, -1 or -2
static int Aoss_CheckIds(const u8 *expected, u8 *received) {
    int result = 0;

    Aoss_Scramble(received, 8, AOSS_KEY, Str_Length(AOSS_KEY));
    if (Mem_Compare(expected, received, 6) != 0) {
        result--;
    } else if (Net_Ntohs(*(u16 *)(expected + 6)) + 1 != Net_Ntohs(*(u16 *)(received + 6))) {
        result -= 2;
    }
    return result;
}

static int Aoss_ParseHello(const AossItem *items, AossHello *hello) {
    const AossItem *item;

    Mem_Fill(hello, 0, 0x104);
    item = items;
    while (1) {
        int length = Net_Ntohs(item->length);

        if (length <= 0) {
            return -1;
        }
        switch (item->type) {
        case 0:
            Mem_Copy(hello->name, item->data, length);
            break;
        case 1:
            Mem_Copy(hello->info, item->data, length);
            break;
        case 2:
            Mem_Copy(hello->extra, item->data, length);
            break;
        case 3:
        case 4: {
            int value = Net_Ntohs(item->data[0]);

            if (value <= 0) {
                return -2;
            }
        } break;
        case 5:
            sAoss.request.addr = Net_Ntohl(Aoss_ReadLe(item->data, length));
            break;
        case 6:
            sAoss.request.netmask = Net_Ntohl(Aoss_ReadLe(item->data, length));
            break;
        default:
            return -1;
        }
        if (item->next == 0) {
            break;
        }
        item = (const AossItem *)((const u8 *)items + Net_Ntohs(item->next));
    }
    return 0;
}

static u32 Aoss_ReadLe(const u8 *data, int length) {
    u32 value = 0;
    int i;
    const u8 *p = data + (length - 1);

    for (i = 0; i < length; i++) {
        value = (value << 8) + *p--;
    }
    return value;
}

static int Aoss_ParseWep(const AossRecord *record, AossWepInfo *info) {
    const AossItem *item = record->payload.secure.items;
    const AossItem *items;
    char *ssid = info->ssid;
    u8 *key3 = info->key[3];
    u8 *key2 = info->key[2];
    u8 *key1 = info->key[1];
    u8 *key0 = info->key[0];

    items = item;

    while (1) {
        u32 length = Net_Ntohs(item->length);

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
            Mem_Copy(key0, item->data, length);
            info->keyLength = length;
            break;
        case 0x11:
        case 0x21:
            Mem_Copy(key1, item->data, length);
            info->keyLength = length;
            break;
        case 0x12:
        case 0x22:
            Mem_Copy(key2, item->data, length);
            info->keyLength = length;
            break;
        case 0x13:
        case 0x23:
            Mem_Copy(key3, item->data, length);
            info->keyLength = length;
            break;
        case 0x15:
        case 0x25:
            if (length != 0 && item->data[length - 1] != 0) {
                return -1;
            }
            Mem_Copy(ssid, item->data, length);
            break;
        default:
            return -1;
        }
        if (item->next == 0) {
            break;
        }
        item = (const AossItem *)((const u8 *)items + Net_Ntohs(item->next));
    }
    return 0;
}

static int Aoss_ParsePsk(const AossRecord *record, AossPskInfo *info) {
    const AossItem *item = record->payload.secure.items;
    const AossItem *items;
    char *ssid = info->ssid;
    u8 *key = info->key;

    items = item;

    while (1) {
        u32 length = Net_Ntohs(item->length);

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
            Mem_Copy(key, item->data, length);
            info->keyLength = length;
            break;
        case 0x35:
        case 0x45:
            if (length != 0 && item->data[length - 1] != 0) {
                return -1;
            }
            Mem_Copy(ssid, item->data, length);
            break;
        default:
            return -1;
        }
        if (item->next != 0) {
            item = (const AossItem *)((const u8 *)items + Net_Ntohs(item->next));
        } else {
            break;
        }
    }
    return 0;
}

static int Aoss_ParseKey(const AossRecord *record, u8 *key) {
    const AossItem *item = record->payload.secure.items;
    int length = Net_Ntohs(item->length);

    if (length <= 0) {
        return -1;
    }
    if (item->type != 0x70) {
        return -1;
    }
    Mem_Copy(key, item->data, length);
    return 0;
}

// Reads the security settings that the reply of a stage has into slot of slots; 0 if they are fit
static int Aoss_ParseSecurity(int slot, const AossRecord *record, int length, AossApInfo *slots, AossSession *session) {
    u32 found = 0;
    int result;
    AossApInfo *ap;
    const AossRecord *entry;
    int size;

    if (length <= 0) {
        return -2;
    }
    while (1) {
        int step;

        entry = record;
        if (entry->type == sRecordType[slot]) {
            break;
        }
        step = Net_Ntohs(entry->length) + 4;
        length -= step;
        record = (const AossRecord *)((const u8 *)entry + step);
        if (length <= 0) {
            return -4;
        }
    }
    size = Net_Ntohs(entry->length);
    record = (const AossRecord *)((const u8 *)entry + 4);
    ap = &slots[slot];
    do {
        switch (record->type) {
        case 3:
            result = Aoss_ParseWep(record, &ap->wep40);
            found |= AOSS_TYPE_WEP40;
            break;
        case 4:
            result = Aoss_ParseWep(record, &ap->wep104);
            found |= AOSS_TYPE_WEP104;
            break;
        case 5:
            result = Aoss_ParsePsk(record, &ap->tkip);
            found |= AOSS_TYPE_TKIP;
            break;
        case 6:
            result = Aoss_ParsePsk(record, &ap->aes);
            found |= AOSS_TYPE_AES;
            break;
        case 10:
            result = Aoss_ParseKey(record, session->buffer[slot + 3]);
            break;
        default:
            result = -3;
            break;
        }
        if (result != 0) {
            return result;
        }
        {
            int step = Net_Ntohs(record->length) + 4;

            size -= step;
            record = (const AossRecord *)((const u8 *)record + step);
        }
    } while (size > 0);
    sAoss.request.found |= found;
    return 0;
}

static int Aoss_IsEncrypted(int flags) {
    int result = 0;

    if (flags & 0x10) {
        result = 1;
    }
    return result;
}

static int Aoss_Send(int state, const AossEndpoints *endpoints, const u8 *ids, int sock) {
    switch (state) {
    case 0:
        Aoss_SetMode(2);
        return Aoss_SendStage0(endpoints, ids, sock);
    case 1:
        Aoss_SetMode(3);
        return Aoss_SendStage1(endpoints, ids, sock);
    case 2:
        Aoss_SetMode(5);
        return Aoss_SendStage2(endpoints, ids, sock);
    }
    return -1;
}

static int Aoss_SendStage0(const AossEndpoints *endpoints, const u8 *ids, int sock) {
    s8 checksum = 0;
    s16 length = 0;
    s16 flags = 0;
    u8 id[8];
    AossHeader *header = (AossHeader *)sAoss.packet;
    u8 *body;
    u8 *payload;

    Mem_Fill(header, 0, AOSS_PACKET_SIZE);
    body = Aoss_Alloc(0x210);
    if (body == NULL) {
        Aoss_SetError(2);
        return -1;
    }
    Mem_Fill(body, 0, 0x210);
    payload = (u8 *)(header + 1);
    Mem_Copy(sPeerMac, ids, 8);
    Mem_Copy(id, sPeerMac, 8);
    length = Aoss_BuildHello(body + 4);
    if (length < 0) {
        Aoss_SetError(3);
        if (body != NULL) {
            Aoss_Free(body);
        }
        return -1;
    }
    body[0] = 0;
    *(u16 *)(body + 2) = Net_Htons(length);
    length += 4;
    Aoss_BuildPayload(0, payload, body, &length, &flags, &checksum);
    flags |= 0x10;
    if (Aoss_Scramble(id, 8, AOSS_KEY, sizeof(AOSS_KEY)) != 0) {
        Aoss_SetError(2);
        if (body != NULL) {
            Aoss_Free(body);
        }
        return -1;
    }
    Aoss_BuildHeader(header, 0x1000, length, flags, checksum, AOSS_PROTOCOL, id);
    length += sizeof(AossHeader);
    Aoss_SendPacket(header, length, 0xff, sock);
    if (body != NULL) {
        Aoss_Free(body);
    }
    return 0;
}

static int Aoss_SendStage1(const AossEndpoints *endpoints, const u8 *ids, int sock) {
    s8 checksum = 0;
    s16 length = 0;
    s16 flags = 0;
    u8 id[8];
    AossStatusRecord record;
    AossHeader *header = (AossHeader *)sAoss.packet;

    Mem_Fill(&record, 0, 8);
    Mem_Fill(header, 0, AOSS_PACKET_SIZE);
    record.type = 2;
    record.reserved = 0;
    record.length = Net_Htons(4);
    record.status = sAoss.request.wanted;
    record.status = Net_Htonl(record.status);
    length = 8;
    Aoss_BuildPayload(sAoss.encrypt, (u8 *)(header + 1), (u8 *)&record, &length, &flags, &checksum);
    Mem_Copy(id, ids + 8, 8);
    if (Aoss_Scramble(id, 8, AOSS_KEY, sizeof(AOSS_KEY)) != 0) {
        Aoss_SetError(2);
        return -1;
    }
    Aoss_BuildHeader(header, 0x2000, length, flags, checksum, AOSS_PROTOCOL, id);
    length += sizeof(AossHeader);
    Aoss_SendPacket(header, length, 0, sock);
    return 0;
}

static int Aoss_SendStage2(const AossEndpoints *endpoints, const u8 *ids, int sock) {
    u8 id[8];
    AossHeader *header = (AossHeader *)sAoss.packet;

    Mem_Fill(header, 0, AOSS_PACKET_SIZE);
    Mem_Copy(id, ids + 16, 8);
    Aoss_Scramble(id, 8, AOSS_KEY, Str_Length(AOSS_KEY));
    Aoss_BuildHeader(header, 0x3000, 0, 0, 0, AOSS_PROTOCOL, id);
    Aoss_SendPacket(header, sizeof(AossHeader), 0, sock);
    return 0;
}

// The body of the hello: the request's data and the item 0x60 with 14; returns its length
static s16 Aoss_BuildHello(u8 *buffer) {
    AossItem *first = (AossItem *)buffer;
    AossItem *second;
    s16 length;
    s16 total = 0;
    s16 offset;
    u32 value;

    first->type = sAoss.request.version;
    first->reserved = 1;
    length = sAoss.request.length;
    Mem_Copy(first->data, sAoss.request.data, length);
    first->length = Net_Htons(length);
    offset = ((s16)(length + 6) + 1) / 2 * 2;
    first->next = Net_Htons(offset);
    total += offset;
    second = (AossItem *)(buffer + offset);
    second->type = 0x60;
    second->reserved = 0;
    second->next = Net_Htons(0);
    value = Net_Htonl(0xe);
    Mem_Copy(second->data, &value, 4);
    second->length = Net_Htons(4);
    total += 10;
    return total;
}

// Puts the body in the packet, encrypted when the client wants it so
static void Aoss_BuildPayload(int encrypt, u8 *dest, const u8 *src, s16 *length, s16 *flags, s8 *checksum) {
    if (encrypt == 1) {
        *flags = 1;
        Aoss_Encrypt(src, dest + 4, *length, (u8 *)checksum, dest + 2, sPeerMac, 8);
        *(u16 *)dest = Net_Htons(*length);
        *length += 4;
    } else {
        Mem_Copy(dest, src, *length);
    }
}

static void Aoss_BuildHeader(AossHeader *header, int type, int length, int flags, s8 checksum, s8 protocol,
                             const u8 *id) {
    header->version = Net_Htons(1);
    header->unk_02 = 0;
    header->unk_04 = 0;
    header->type = Net_Htons(type);
    header->unk_08 = 0;
    header->length = Net_Htons(length);
    header->flags = Net_Htons(flags);
    header->checksum = checksum;
    header->protocol = protocol;
    Mem_Copy(header->id, id, 8);
}

static int Aoss_SendPacket(void *buffer, int length, int toAll, int sock) {
    SocAddrIn to;

    Mem_Fill(&to, 0, sizeof(to));
    to.family = 2;
    to.port = Net_Htons(AOSS_PORT);
    to.addr = Net_Htonl(sAoss.request.addr);
    if (toAll == 0xff || sAoss.request.reconnected == 0) {
        to.addr = -1;
    }
    return Net_SendTo(sock, buffer, length, 0, &to, 8);
}

static int Aoss_Encrypt(const u8 *in, u8 *out, u32 length, u8 *checksum, u8 *iv, const u8 *key, u32 keyLength) {
    RC4Context context;
    u16 number;

    *checksum = Aoss_Checksum(in, length);
    context.s = Aoss_Alloc(length);
    if (context.s == NULL) {
        return -1;
    }
    number = Aoss_Rand();
    Mem_Copy(iv, &number, 2);
    Mem_Copy(sCipherKey, iv, 2);
    Mem_Copy(sCipherKey + 2, key, keyLength);
    Rc4_Init(&context, sCipherKey, keyLength + 2, length);
    Rc4_Crypt(&context, out, in, length);
    Aoss_Free(context.s);
    return 0;
}

static int Aoss_Decrypt(const u8 *in, u8 *out, u32 length, u8 checksum, const u8 *iv, const u8 *key, u32 keyLength) {
    RC4Context context;

    context.s = Aoss_Alloc(length);
    if (context.s == NULL) {
        Aoss_SetError(2);
        return -1;
    }
    Mem_Copy(sCipherKey, iv, 2);
    Mem_Copy(sCipherKey + 2, key, keyLength);
    Rc4_Init(&context, sCipherKey, keyLength + 2, length);
    Rc4_Crypt(&context, out, in, length);
    u8 actual = Aoss_Checksum(out, length);
    if (actual != checksum) {
        Aoss_SetError(0x12);
        Aoss_Free(context.s);
        return -1;
    }
    Aoss_Free(context.s);
    return 0;
}

// RC4 with a state of n bytes instead of 256
static void Rc4_Init(RC4Context *context, const u8 *key, u32 keyLength, u32 n) {
    u8 *s = context->s;
    u32 i;
    u32 j = 0;
    u32 k;

    context->i = 0;
    context->j = 0;
    context->n = n;
    for (i = 0; i < n; i++) {
        s[i] = i;
    }
    k = 0;
    for (i = 0; i < n; i++) {
        u8 value = s[i];

        j = (j + value + key[k]) % context->n;
        s[i] = s[j];
        s[j] = value;
        k++;
        if (k >= keyLength) {
            k = 0;
        }
    }
}

static void Rc4_Crypt(RC4Context *context, u8 *out, const u8 *in, u32 length) {
    u32 i;

    for (i = 0; i < length; i++) {
        u8 key = Rc4_Next(context);

        out[i] = in[i] ^ key;
    }
}

static u32 Rc4_Next(RC4Context *context) {
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
static u8 Aoss_Checksum(const u8 *data, int length) {
    return Crc32_Update(-1, data, length, 0, sCrcTable) ^ 0xffffffff;
}

static u32 Crc32_Update(u32 crc, const u8 *data, int length, int ready, u32 *table) {
    int i;

    if (ready == 0) {
        Crc32_MakeTable(ready, table);
    }
    for (i = 0; i < length; i++) {
        crc = (crc >> 8) ^ table[(u8)(data[i] ^ crc)];
    }
    return crc;
}

static void Crc32_MakeTable(int i, u32 *table) {
    for (i = 0; i < 256; i++) {
        u32 value = i;
        int bit;
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
static int Aoss_Scramble(u8 *block, int length, const char *key, int keyLength) {
    char *mask;
    u8 *work;
    int round;

    mask = Aoss_Alloc(length / 2);
    if (mask == NULL) {
        return -1;
    }
    work = Aoss_Alloc(length);
    if (work == NULL) {
        Aoss_Free(mask);
        return -1;
    }
    for (round = 0; round < 2; round++) {
        Aoss_ScrambleKey(round, mask, length, key, keyLength);
        Aoss_ScrambleXor(mask, block, length);
        Aoss_ScrambleSwap(block, length, work);
    }
    Aoss_Free(mask);
    Aoss_Free(work);
    return 0;
}

static void Aoss_ScrambleKey(int round, char *out, int length, const char *key, int keyLength) {
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

static void Aoss_ScrambleXor(const char *mask, u8 *block, int length) {
    int i;
    int half = length / 2;

    for (i = 0; i < half; i++) {
        block[half + i] ^= mask[i];
    }
}

static void Aoss_ScrambleSwap(u8 *block, int length, u8 *work) {
    int half = length / 2;

    Mem_Copy(work, block + half, half);
    Mem_Copy(work + half, block, half);
    Mem_Copy(block, work, length);
}

static void *Soc_Alloc(u32 name, s32 size) {
    if (size > 0) {
        return gAossAp.alloc(size);
    }
    return NULL;
}

static void Soc_Free(u32 name, void *ptr, s32 size) {
    gAossApFree(ptr);
}

// Starts the socket library with a fixed address
static int Net_Start(u32 addr, u32 netmask, u32 gateway) {
    sSocConfig.addr = Net_Htonl(addr);
    sSocConfig.netmask = Net_Htonl(netmask);
    sSocConfig.gateway = Net_Htonl(gateway);
    if (SOC_Startup(&sSocConfig) < 0) {
        return -1;
    }
    if (CPSMyIp == 0) {
        do {
            AossAp_Sleep(100);
        } while (CPSMyIp == 0);
    }
    return 0;
}

static int Net_Stop(void) {
    if (SOC_Cleanup() < 0) {
        return -1;
    }
    return -(AossAp_Disconnect() != 0);
}

static int Mem_Compare(const u8 *a, const u8 *b, int size) {
    int difference = 0;

    while (size-- > 0 && (difference = *a - *b) == 0) {
        a++;
        b++;
    }
    return difference;
}

static void Mem_Copy(void *dest, const void *src, u32 size) {
    sys_memcpy(src, dest, size);
}

static void Mem_Fill(void *dest, int value, u32 size) {
    sys_memset(dest, (u8)value, size);
}

static int Net_RecvFrom(int sock, void *buffer, int size, int flags, SocAddrIn *from, int *fromLength) {
    from->len = *fromLength;
    return SOC_RecvFrom(sock, buffer, size, flags, from);
}

// Waits for the descriptor to be ready for at most the time
static int Net_Poll(int count, SocPollFd *readFds, void *writeFds, void *exceptFds, const SocTimeval *timeout) {
    SocPollFd fd = *readFds;
    u64 ticks = 0;

    ticks += ((u64)(s64)timeout->sec * 33514000) >> 6;
    ticks += ((u64)(s64)timeout->usec * 33514000) >> 6;
    return SOC_Poll(&fd, 1, ticks);
}

static int Net_SendTo(int sock, const void *buffer, int size, int flags, SocAddrIn *to, int toLength) {
    to->len = toLength;
    return SOC_SendTo(sock, buffer, size, flags, to);
}

// The socket library takes no options
static int Net_SetSockOpt(int sock, int level, int name, const void *value, int length) {
}

static int Net_Socket(int domain, int type, int protocol) {
    return SOC_Socket(domain, type, protocol);
}

static int Net_Bind(int sock, SocAddrIn *address, int length) {
    address->len = length;
    return SOC_Bind(sock, address);
}

static int Net_Close(int sock) {
    return SOC_Close(sock);
}

static u32 Net_Htonl(u32 value) {
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

static u32 Net_Htons(u16 value) {
    return (u16)(((value >> 8) & 0xff) | ((value << 8) & 0xff00));
}

static u32 Net_Ntohl(u32 value) {
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

static u32 Net_Ntohs(u16 value) {
    return (u16)(((value >> 8) & 0xff) | ((value << 8) & 0xff00));
}

static u32 Str_Length(const char *text) {
    u32 length = 0;

    while (text[length] != '\0') {
        length++;
    }
    return length;
}

// A linear congruential generator, seeded with the time
static u16 Aoss_Rand(void) {
    if (sAoss.randReady == 0) {
        RTCTime time;
        u32 seed = 0;

        Mem_Fill(&time, 0, sizeof(time));
        if (func_0207cc80(&time) == 0) {
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

static void Net_PollInit(SocPollFd *fd) {
    fd->fd = 0;
    fd->events = 0;
    fd->revents = 0;
}

static void Net_PollSet(int sock, SocPollFd *fd) {
    fd->fd = sock;
    fd->events = 1;
}
