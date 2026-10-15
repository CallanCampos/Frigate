#include "nitrowifi/cps.h"
#include "cps_internal.h"
#include "dwc/wcm.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "nitro/os.h"

// NitroWiFi's CPS, the TCP/IP stack under the socket layer (SOCL) in overlay 11: ARP, IP with its fragments, ICMP's
// echo, UDP, TCP, the DHCP client and DNS lookups. The ROM names neither the file nor its
// functions: cps.c is a guess, and the names follow NitroWiFi's CPS where the code shows the role (see cps.h).
static void CPSi_Nop(void);
static BOOL CPSi_LinkIsOn(void);
static u32 CPSi_CalcSum(void *data, u32 len, u32 sum);
static u16 CPSi_FinishSum(u16 sum);
static u16 CPS_CalcCksum(void *data, u32 len);
static BOOL CPSi_BadPseudoSum(void *data, u32 len, CPSIpHeader *ip, u32 proto);
static BOOL CPSi_IsOnLink(CPSInAddr ip);
static CPSInAddr CPSi_NextHop(CPSInAddr ip);
static BOOL CPSi_IsBroadcast(CPSInAddr ip);
static BOOL CPSi_IsMulticast(CPSInAddr ip);
static BOOL CPSi_IsForMe(CPSInAddr ip);
static BOOL CPSi_MacDiffers(u16 *a, u16 *b);
static void CPSi_SendFrame(u8 *frame, u32 len, u8 *data, u32 dataLen);
static void CPSi_PutFrame(u8 *src, u8 *dst, u8 *header, u32 headerLen, u8 *data, u32 dataLen);
static CPSEtherHeader *CPSi_GetFrame(u32 *len);
static void CPSi_ConsumeFrame(void);
static u8 *CPSi_LookupMac(CPSInAddr ip);
static void CPSi_SendArpRequest(CPSInAddr ip);
static u8 *CPSi_ResolveMac(CPSInAddr ip);
static void CPSi_UpdateArp(u8 *mac, CPSInAddr ip, BOOL add);
static void CPSi_SendEther(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u16 type);
static void CPSi_SendIpPacket(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u32 frag);
static void CPSi_SendIp(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u8 proto);
static void CPSi_SendPing(u8 *data, u32 len, CPSSoc *soc);
static void CPSi_SendUdp(u8 *data, u32 len, CPSSoc *soc);
static u32 CPSi_SendTcp(u8 *data, u32 len, CPSSoc *soc, u32 flags);
static void CPSi_ReplyArp(CPSArpPacket *arp);
static void CPSi_ReceiveArp(CPSArpPacket *arp, u32 len);
static void CPSi_ReceiveEchoRequest(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len);
static void CPSi_ReceiveEchoReply(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len);
static BOOL CPSi_IsUnicast(CPSInAddr src, CPSInAddr dst);
static void CPSi_ReceiveIcmp(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len);
static CPSSoc *CPSi_FindListener(CPSIpHeader *ip, CPSTcpHeader *tcp);
static BOOL CPSi_IsTcpOf(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc);
static CPSSoc *CPSi_FindTcpSoc(CPSIpHeader *ip, CPSTcpHeader *tcp);
static void CPSi_ParseTcpOptions(CPSTcpHeader *tcp, CPSSoc *soc);
static BOOL CPSi_KnowsNextHop(CPSInAddr ip);
static void CPSi_SendTcpControl(CPSSoc *soc, u32 flags);
static void CPSi_SendAck(CPSSoc *soc, int mode);
static void CPSi_SendFin(CPSSoc *soc);
static void CPSi_SendReset(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_AcceptSyn(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc);
static BOOL CPSi_ReceiveSynAgain(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_ReceiveSyn(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_ReceiveSynAck(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_MarkOoo(CPSSoc *soc, int index);
static void CPSi_MergeOoo(CPSSoc *soc);
static void CPSi_PackOoo(CPSSoc *soc);
static void CPSi_InsertOoo(CPSSoc *soc, int index);
static void CPSi_ReceiveTcpData(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_ReceiveFin(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_ReceiveRst(CPSIpHeader *ip, CPSTcpHeader *tcp);
static void CPSi_ReceiveTcp(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_ReceiveUdp(CPSIpHeader *ip, CPSUdpHeader *udp, u32 len);
static CPSIpHeader *CPSi_Reassemble(CPSIpHeader *ip, BOOL *allocated);
static void CPSi_ReceiveIp(CPSIpHeader *ip, u32 len);
static void CPSi_RecvThread(void *arg);
static u32 CPSi_Rand(void);
static void CPSi_TcpShutdown(CPSSoc *soc);
static u8 *CPSi_UdpRead(u32 *len, CPSSoc *soc);
static u32 CPSi_TcpSendSegments(u8 *data, u32 len, CPSSoc *soc, BOOL probe);
static u32 CPSi_TcpSendBoth(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc, BOOL probe);
static u32 CPSi_SocWrite(u8 *data, u32 len, u8 *data2, u32 len2);
static BOOL CPSi_CheckCollision(void);
static void CPSi_DhcpGiveUp(void);
static void CPSi_ScavengerThread(void *arg);
static void CPSi_SendDelayedAcks(void);
static void CPSi_AckThread(void *arg);
static void CPSi_AckAlarm(void *arg);
static u8 *CPSi_DhcpMakeMessage(u8 *buf, u8 type);
static u8 *CPSi_Pad(u32 value, u32 size, u8 *p, u32 used);
static u32 CPSi_DhcpSendDiscover(void);
static u32 CPSi_DhcpSendRequest(int mode);
static int CPSi_DhcpWait(u32 xid, int try);
static BOOL CPSi_DhcpDiscover(void);
static BOOL CPSi_DhcpRequest(u32 *timer, int mode);
static void CPSi_DhcpRelease(void);
static u8 *CPSi_DnsSkipName(u8 *p);
static s32 CPSi_DnsQuery(const u8 *name, u32 type, u16 id, u8 *result, u32 resultSize);
static u32 CPSi_ParseNumber(const u8 *text, const u8 **end);
static BOOL CPSi_ParseIp(const u8 *text, CPSInAddr *addr);
static s32 CPSi_DnsLookup(const u8 *name, CPSInAddr server, u16 id);

#define CPS_STACK_SIZE 0x800
#define CPS_ARP_CACHE_SIZE 8
#define CPS_REASSEMBLY_COUNT 8
// The most a reassembled packet carries
#define CPS_REASSEMBLY_SIZE 0x1000

// DHCP's ports, messages and options, and the length BOOTP pads its messages to
#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67
#define DHCP_DISCOVER 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACK 5
#define DHCP_RELEASE 7
#define DHCP_MESSAGE_SIZE 300

// What waiting for a DHCP answer ends with
enum {
    DHCP_WAIT_NONE,
    DHCP_WAIT_OFFER,
    DHCP_WAIT_ACK,
    DHCP_WAIT_OTHER,
};

// What the DHCP thread is at
enum {
    DHCP_STATE_INIT,
    DHCP_STATE_BOUND,
    DHCP_STATE_REBINDING,
    DHCP_STATE_DONE,
};

#define DNS_PORT 53
#define DNS_TYPE_A 1
#define DNS_TYPE_PTR 12
#define DNS_TYPE_NB 32

static u32 sThreadPriority = 16;
// How long each try waits for an answer to DHCP, in units of 7 * 65536 ticks
static u8 sDhcpWaits[5] = { 2, 2, 3, 5, 8 };
static u8 sBroadcastMac[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
// How long each ARP request waits for an answer, in milliseconds
static u32 sArpWaits[6] = { 1000, 1500, 2000, 2500, 3000, 3500 };
// The LLC/SNAP header that turns an Ethernet frame into an 802.11 one, with IP's type. It is a string literal, which
// is laid out after the file's .data objects
#define CPS_SNAP_HEADER ((u8 *)"\xaa\xaa\x03\x00\x00\x00\x08\x00")

// MWCC sorts the file's objects by size before it lays them out, and the order the declarations below are in gives
// the ROM's layout. The LAN buffer's indices and length and the receiving thread's wait are volatile: the wireless
// driver's receive callback reads and changes them at interrupt time
static u8 sSendFailed;
static u8 sIpCollision;
static u16 sMyMss;
static u16 sLastPort;
static u16 sPingSeq;
static u16 sIpId;
static void (*sDhcpCallback)(void);
u32 CPSMode;
u32 CPSNoIpReason;
static u32 sRenewTime;
static void (*sScavengerCallback)(void);
static u32 sLeaseTime;
static u8 *sLanBuf;
static u32 sYieldWait;
static volatile u32 sLanRead;
static BOOL (*sAddressCheck)(void);
void *(*CPSiAlloc)(u32 size);
static volatile u32 sLanWrite;
static u32 sDhcpXid;
CPSInAddr CPSGatewayIp;
static CPSInAddr sRequestedIp;
static BOOL (*sLinkIsOn)(void);
void (*CPSiFree)(void *ptr);
CPSInAddr CPSNetMask;
static CPSInAddr sDhcpServerIp;
CPSInAddr CPSMyIp;
static OSThread *volatile sRecvWaiter;
static BOOL sCalmDown;
static volatile u32 sLanBufLen;
static u8 sMyMac[6];
CPSInAddr CPSDnsIp[2];
MATHRandContext32 CPSiRand;
static OSAlarm sAckAlarm;
// Where the receiving thread builds the TCP segments it answers with
static u8 sReplyBuf[0x5c];
static CPSArpEntry sArpCache[CPS_ARP_CACHE_SIZE];
static CPSSoc sReplySoc;
static CPSSoc sDhcpSoc;
static OSThread sRecvThread;
static OSThread sScavengerThread;
static OSThread sAckThread;
static u8 sDhcpRcvBuf[0x180];
static u8 sDhcpSndBuf[0x180];
static CPSReassembly sReassembly[CPS_REASSEMBLY_COUNT];
static u8 sRecvStack[CPS_STACK_SIZE] ATTRIBUTE_ALIGN(32);
static u8 sScavengerStack[CPS_STACK_SIZE] ATTRIBUTE_ALIGN(32);
static u8 sAckStack[CPS_STACK_SIZE] ATTRIBUTE_ALIGN(32);

#define CPS_CURRENT_SOC() ((CPSSoc *)OS_GetCurrentThread()->specific[0])

// CPS_SetAddressCheckCallback
void CPS_SetAddressCheckCallback(BOOL (*callback)(void)) {
    sAddressCheck = callback;
}

// CPSi_SetNoIp: drops the address, and with it the ARP cache, the connections and the fragments
static void CPSi_SetNoIp(u32 reason) {
    BOOL hadIp = CPSMyIp != 0;
    OSThread *thread;
    int i;

    CPSNoIpReason = reason;
    CPSMyIp = 0;
    CPSNetMask = 0;
    CPSGatewayIp = 0;
    CPSDnsIp[0] = 0;
    CPSDnsIp[1] = 0;
    sDhcpServerIp = 0;
    if (hadIp) {
        sys_memset(sArpCache, 0, sizeof(sArpCache));
        for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
            CPSSoc *soc = thread->specific[0];

            if (soc != NULL && soc->thread != NULL) {
                if (soc->state != CPS_STT_DATAGRAM && soc->state != CPS_STT_PING) {
                    soc->state = CPS_STT_CLOSED;
                }
                if (soc->block_type != CPS_BLOCK_NONE) {
                    soc->block_type = CPS_BLOCK_NONE;
                    scheduler_start_thread(soc->thread);
                }
            }
        }
        for (i = 0; i < CPS_REASSEMBLY_COUNT; i++) {
            CPSReassembly *reassembly = &sReassembly[i];

            if (reassembly->count != 0) {
                CPSiFree(reassembly->buf);
                reassembly->count = 0;
            }
        }
        CPS_SslInit();
    }
}

// CPSi_Yield: lets the other threads run while waiting
static void CPSi_Yield(void) {
    if (sYieldWait == 0) {
        func_0207a92c();
    } else {
        func_0207aa04(sYieldWait);
    }
}

// CPSi_Nop: the default of the callbacks and of the allocator
static void CPSi_Nop(void) {
}

// CPSi_LinkIsOn: the default link check
static BOOL CPSi_LinkIsOn(void) {
    return TRUE;
}

// CPS_Startup
void CPS_Startup(CPSConfig *config) {
    u64 seed;

    func_02004eb0(CPSi_Signature);
    if (config->random_seed) {
        seed = config->random_seed;
    } else {
        seed = clock();
    }
    MATH_InitRand32(&CPSiRand, seed);
    if (config->alloc && config->free) {
        CPSiAlloc = config->alloc;
        CPSiFree = config->free;
    } else {
        CPSiAlloc = (void *)CPSi_Nop;
        CPSiFree = (void *)CPSi_Nop;
    }
    CPSMode = config->mode;
    if (config->mymss != 0) {
        sMyMss = config->mymss;
    } else {
        sMyMss = 1460;
    }
    sRequestedIp = config->requested_ip;
    sYieldWait = config->yield_wait;
    if (config->dhcp_callback) {
        sDhcpCallback = config->dhcp_callback;
    } else {
        sDhcpCallback = CPSi_Nop;
    }
    if (config->link_is_on) {
        sLinkIsOn = config->link_is_on;
    } else {
        sLinkIsOn = CPSi_LinkIsOn;
    }
    sLanBuf = config->lan_buf;
    sLanBufLen = config->lan_buflen;
    sLanRead = 0;
    sLanWrite = 0;
    sLastPort = 1024 + MATH_Rand32(&CPSiRand, 5000 - 1024);
    WCM_GetMacAddress(sMyMac);
    sIpCollision = FALSE;
    scheduler_init_thread(&sRecvThread, CPSi_RecvThread, NULL, sRecvStack + CPS_STACK_SIZE, CPS_STACK_SIZE,
                          sThreadPriority);
    scheduler_init_thread(&sScavengerThread, CPSi_ScavengerThread, NULL, sScavengerStack + CPS_STACK_SIZE,
                          CPS_STACK_SIZE, sThreadPriority);
    scheduler_start_thread(&sRecvThread);
    scheduler_start_thread(&sScavengerThread);
    func_0207bc24(&sAckAlarm);
    scheduler_init_thread(&sAckThread, CPSi_AckThread, NULL, sAckStack + CPS_STACK_SIZE, CPS_STACK_SIZE,
                          sThreadPriority - 1);
}

// CPS_CalmDown
BOOL CPS_CalmDown(void) {
    u32 enabled = CPU_IRQDisable();
    BOOL ended = scheduler_is_thread_ended(&sScavengerThread);

    if (!ended && !sCalmDown) {
        sCalmDown = TRUE;
        scheduler_start_thread(&sScavengerThread);
    }
    CPU_SetIRQMask(enabled);
    return ended;
}

// CPS_SetScavengerCallback
void CPS_SetScavengerCallback(void (*callback)(void)) {
    sScavengerCallback = callback;
}

// CPS_Cleanup
void CPS_Cleanup(void) {
    u32 enabled;

    CPS_CalmDown();
    func_0207a800(&sScavengerThread);
    func_0207a710(&sRecvThread);
    func_0207bd3c(&sAckAlarm);
    enabled = CPU_IRQDisable();
    if (!scheduler_is_thread_ended(&sAckThread)) {
        scheduler_start_thread(&sAckThread);
    }
    CPU_SetIRQMask(enabled);
    func_0207a800(&sAckThread);
    sRecvWaiter = NULL;
    CPSi_SetNoIp(CPS_NOIP_REASON_NONE);
    sLanBuf = NULL;
    sLanBufLen = 0;
}

// CPS_SetThreadPriority
void CPS_SetThreadPriority(u32 priority) {
    sThreadPriority = priority;
    func_0207a998(&sRecvThread, priority);
    func_0207a998(&sScavengerThread, priority);
}

// CPSi_CalcSum: adds the 16-bit words of data to sum, in ones' complement
static u32 CPSi_CalcSum(void *data, u32 len, u32 sum) {
    u32 *p = data;
    u32 align = (u32)data & 3;
    u32 acc = 0;
    u32 word;

    if (align != 0) {
        p = (u32 *)((u32)p & ~3);
        word = *p;
        p++;
        switch (align) {
        case 1:
            word &= ~0xff;
            if (len < 3) {
                len += 1;
                goto last;
            }
            len -= 3;
            break;
        case 2:
            word &= 0xffff0000;
            if (len < 2) {
                len += 2;
                goto last;
            }
            len -= 2;
            break;
        default:
            word &= 0xff000000;
            len -= 1;
            break;
        }
        acc = word;
    }
    while (len >= 4) {
        word = *p;
        acc += word;
        if (acc < word) {
            acc++;
        }
        len -= 4;
        p++;
    }
    if (len == 0) {
        goto fold;
    }
    word = *p;
last:
    if (len == 1) {
        word = (u8)word;
    } else if (len == 2) {
        word = (u16)word;
    } else {
        word &= 0xffffff;
    }
    acc += word;
    if (acc < word) {
        acc++;
    }
fold:
    acc = (u16)acc + (acc >> 16);
    acc = (u16)acc + (acc >> 16);
    if (!(align & 1)) {
        acc = (acc >> 8) + ((acc << 8) & 0xff00);
    }
    acc += sum;
    return (u16)acc + (acc >> 16);
}

// CPSi_FinishSum
static u16 CPSi_FinishSum(u16 sum) {
    sum ^= 0xffff;
    if (sum == 0) {
        sum = 0xffff;
    }
    return sum;
}

// CPS_CalcCksum
static u16 CPS_CalcCksum(void *data, u32 len) {
    return CPSi_FinishSum(CPSi_CalcSum(data, len, 0));
}

// CPSi_BadPseudoSum: checks a UDP or TCP packet's sum, with the pseudo-header of its addresses, protocol and length
static BOOL CPSi_BadPseudoSum(void *data, u32 len, CPSIpHeader *ip, u32 proto) {
    u32 sum = CPSi_CalcSum(ip->src, 8, CPSi_CalcSum(data, len, proto));

    sum += len;

    if (sum & 0x10000) {
        sum = (u16)(sum + 1);
    }
    return sum != 0xffff;
}

// CPSi_IsOnLink: whether an address is reached without the gateway
static BOOL CPSi_IsOnLink(CPSInAddr ip) {
    BOOL onLink = TRUE;

    if (ip != CPS_BROADCAST_ADDR && ip != CPS_LOOPBACK_ADDR && (ip & CPSNetMask) != (CPSMyIp & CPSNetMask)) {
        onLink = FALSE;
    }
    return onLink;
}

// CPSi_NextHop
static CPSInAddr CPSi_NextHop(CPSInAddr ip) {
    if (!CPSi_IsOnLink(ip)) {
        ip = CPSGatewayIp;
    }
    return ip;
}

// CPSi_IsBroadcast
static BOOL CPSi_IsBroadcast(CPSInAddr ip) {
    BOOL broadcast = FALSE;

    if (CPSi_IsOnLink(ip)) {
        u32 host = ~CPSNetMask;

        if (host == (ip & host)) {
            broadcast = TRUE;
        }
    }
    return broadcast;
}

// CPSi_IsMulticast
static BOOL CPSi_IsMulticast(CPSInAddr ip) {
    if ((ip & 0xf0000000) == 0xe0000000) {
        return TRUE;
    }
    return FALSE;
}

// CPSi_IsForMe
static BOOL CPSi_IsForMe(CPSInAddr ip) {
    return CPSMyIp == 0 || ip == CPSMyIp || ip == CPS_LOOPBACK_ADDR || CPSi_IsBroadcast(ip) || CPSi_IsMulticast(ip);
}

// CPSi_MacDiffers
static BOOL CPSi_MacDiffers(u16 *a, u16 *b) {
    int i;

    for (i = 0; i < 3; i++) {
        if (*a++ != *b++) {
            return TRUE;
        }
    }
    return FALSE;
}

// CPSi_SendFrame: sends an Ethernet frame as an 802.11 one, with the SNAP header over its source address
static void CPSi_SendFrame(u8 *frame, u32 len, u8 *data, u32 dataLen) {
    sys_memcpy_fast(CPS_SNAP_HEADER, frame + 6, 6);
    sSendFailed = WCM_SendDCFDataEx(frame, frame + 6, len - 6, data, dataLen) < 0;
}

// CPSi_PutFrame: puts a received frame in the LAN buffer, as its size and the Ethernet frame that it was
static void CPSi_PutFrame(u8 *src, u8 *dst, u8 *header, u32 headerLen, u8 *data, u32 dataLen) {
    u8 *buf = sLanBuf;
    u32 len;
    u16 size;
    u32 next;

    if (buf == NULL || sLanBufLen == 0) {
        return;
    }
    len = headerLen + dataLen;
    if (len < 8 || len > 0x5e4) {
        return;
    }
    if (header[0] != CPS_SNAP_HEADER[0] || header[1] != CPS_SNAP_HEADER[1] || header[2] != CPS_SNAP_HEADER[2]) {
        return;
    }
    if (header[6] != 8 || (header[7] != 0 && header[7] != 6)) {
        return;
    }
    size = (len + 9) & ~1;
    next = sLanWrite + size;
    if (sLanWrite < sLanRead && sLanRead <= next) {
        return;
    }
    if (next == sLanBufLen) {
        next = 0;
        if (sLanRead == 0) {
            return;
        }
    } else if (next > sLanBufLen) {
        next = size;
        if (sLanRead <= size) {
            return;
        }
    }
    if (sLanWrite + size > sLanBufLen) {
        if (sLanBufLen - sLanWrite >= 2) {
            *(u16 *)(buf + sLanWrite) = 0;
        }
        sLanWrite = 0;
    }
    *(u16 *)(sLanBuf + sLanWrite) = size;
    sys_memcpy_fast(dst, sLanBuf + sLanWrite + 2, 6);
    sys_memcpy_fast(src, sLanBuf + sLanWrite + 8, 6);
    sys_memcpy_fast(header + 6, sLanBuf + sLanWrite + 14, headerLen - 6);
    if (data != NULL && dataLen != 0) {
        sys_memcpy_fast(data, sLanBuf + sLanWrite + 8 + headerLen, dataLen);
    }
    sLanWrite = next;
}

// CPSi_RecvCallback
void CPSi_RecvCallback(u8 *src, u8 *dst, u8 *data, u32 len) {
    CPSi_PutFrame(src, dst, data, len, NULL, 0);
    if (sRecvWaiter != NULL && !scheduler_is_thread_ended(sRecvWaiter)) {
        scheduler_start_thread(sRecvWaiter);
    }
}

// CPSi_GetFrame: waits for a frame in the LAN buffer
static CPSEtherHeader *CPSi_GetFrame(u32 *len) {
    u32 enabled = CPU_IRQDisable();
    u8 *buf;
    u16 size;

    while (sLanRead == sLanWrite) {
        sRecvWaiter = OS_GetCurrentThread();
        scheduler_yield(NULL);
        sRecvWaiter = NULL;
    }
    CPU_SetIRQMask(enabled);
    buf = sLanBuf;
    do {
        if (sLanBufLen - sLanRead < 2) {
            sLanRead = 0;
        }
        size = *(u16 *)(buf + sLanRead);
        if (size == 0) {
            sLanRead = 0;
        }
    } while (size == 0);
    *len = size - 2;
    return (CPSEtherHeader *)(sLanBuf + sLanRead + 2);
}

// CPSi_ConsumeFrame
static void CPSi_ConsumeFrame(void) {
    u32 enabled = CPU_IRQDisable();

    sLanRead += *(u16 *)(sLanBuf + sLanRead);
    if (sLanRead >= sLanBufLen) {
        sLanRead = 0;
    }
    CPU_SetIRQMask(enabled);
}

// CPSi_LookupMac: the MAC address of an address in the cache, or NULL
static u8 *CPSi_LookupMac(CPSInAddr ip) {
    u32 enabled = CPU_IRQDisable();
    u8 *mac = NULL;
    u32 i;

    if (ip == CPS_LOOPBACK_ADDR || ip == CPSMyIp) {
        mac = sMyMac;
    } else if (CPSi_IsBroadcast(ip) || CPSi_IsMulticast(ip)) {
        mac = sBroadcastMac;
    } else {
        for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
            CPSArpEntry *entry = &sArpCache[i];

            if (ip == entry->ip) {
                sArpCache[i].time = CPS_NOW();
                mac = entry->mac;
                break;
            }
        }
    }
    CPU_SetIRQMask(enabled);
    return mac;
}

// CPSi_SendArpRequest
static void CPSi_SendArpRequest(CPSInAddr ip) {
    u8 frame[42];

    sys_memset(frame, 0, 42);
    sys_memset(frame, 0xff, 6);
    sys_memcpy_fast(sMyMac, frame + 6, 6);
    *(u16 *)(frame + 12) = 0x608;
    frame[15] = 1;
    frame[16] = 8;
    *(u16 *)(frame + 18) = 0x406;
    frame[21] = 1;
    sys_memcpy_fast(sMyMac, frame + 22, 6);
    CPS_PUT32(frame + 28, CPSMyIp);
    CPS_PUT32(frame + 38, ip);
    CPSi_SendFrame(frame, 42, NULL, 0);
}

// CPSi_ResolveMac: asks for an address's MAC address until it comes, or until the waits run out
static u8 *CPSi_ResolveMac(CPSInAddr ip) {
    u32 try;
    u32 i;
    u8 *mac;

    for (try = 0; try < 6; try++) {
        CPSi_SendArpRequest(ip);
        for (i = 0; i < sArpWaits[try] / 100; i++) {
            if (CPSMyIp == 0) {
                return NULL;
            }
            func_0207aa04(100);
            mac = CPSi_LookupMac(ip);
            if (mac != NULL) {
                return mac;
            }
        }
    }
    return NULL;
}

// CPSi_UpdateArp: refreshes an address's entry, or takes a free or the oldest one for it when add is set
static void CPSi_UpdateArp(u8 *mac, CPSInAddr ip, BOOL add) {
    u16 now;
    u32 i;

    if (ip == CPS_LOOPBACK_ADDR || ip == CPSMyIp || !CPSi_IsOnLink(ip) || CPSi_IsMulticast(ip)) {
        return;
    }
    now = CPS_NOW();
    for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
        if (sArpCache[i].ip == ip) {
            sArpCache[i].time = now;
            sys_memcpy_fast(mac, sArpCache[i].mac, 6);
            return;
        }
    }
    if (add) {
        u32 victim = 0;
        u16 oldest = 0;

        for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
            if (sArpCache[i].ip == 0) {
                victim = i;
                break;
            }
            if ((s16)(now - sArpCache[i].time) > oldest) {
                oldest = now - sArpCache[i].time;
                victim = i;
            }
        }
        sArpCache[victim].ip = ip;
        sys_memcpy_fast(mac, sArpCache[victim].mac, 6);
        sArpCache[victim].time = now;
    }
}

// CPSi_SendEther: puts the Ethernet header in front of buf and sends the frame to the next hop
static void CPSi_SendEther(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u16 type) {
    CPS_PUT16(buf - 2, type);
    if (!CPSi_IsMulticast(ip)) {
        u8 *mac;

        ip = CPSi_NextHop(ip);
        if (ip == 0) {
            return;
        }
        mac = CPSi_LookupMac(ip);
        if (mac == NULL) {
            mac = CPSi_ResolveMac(ip);
        }
        if (mac == NULL) {
            return;
        }
        sys_memcpy_fast(mac, buf - 14, 6);
    } else {
        buf[-14] = 0x01;
        buf[-13] = 0x00;
        buf[-12] = 0x5e;
        buf[-11] = (ip >> 16) & 0x7f;
        buf[-10] = ip >> 8;
        buf[-9] = ip;
    }
    sys_memcpy_fast(sMyMac, buf - 8, 6);
    CPSi_SendFrame(buf - 14, len + 14, data, dataLen);
}

// CPSi_SendIpPacket: finishes the IP header in front of buf and sends one packet, to itself through the LAN buffer
static void CPSi_SendIpPacket(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u32 frag) {
    CPS_PUT16(buf - 18, (u16)(len + 20 + dataLen));
    CPS_PUT16(buf - 14, (u16)frag);
    *(u16 *)(buf - 10) = 0;
    CPS_PUT16(buf - 10, CPS_CalcCksum(buf - 20, 20));
    if (ip != CPS_LOOPBACK_ADDR && ip != CPSMyIp) {
        CPSi_SendEther(buf - 20, len + 20, data, dataLen, ip, CPS_ETHERTYPE_IP);
    }
    if (ip == CPS_LOOPBACK_ADDR || ip == CPSMyIp || CPSi_IsMulticast(ip)) {
        u32 enabled;

        sys_memcpy_fast(CPS_SNAP_HEADER, buf - 28, 8);
        enabled = CPU_IRQDisable();
        CPSi_PutFrame(sMyMac, sMyMac, buf - 28, len + 28, data, dataLen);
        CPU_SetIRQMask(enabled);
    }
}

// CPSi_SendIp: puts the IP header in front of buf and sends the packet, in fragments when it is too long
static void CPSi_SendIp(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u8 proto) {
    u16 offset;

    buf[-20] = 0x45;
    buf[-19] = 0;
    sIpId++;
    CPS_PUT16(buf - 16, sIpId);
    buf[-12] = 0x80;
    buf[-11] = proto;
    CPS_PUT32(buf - 8, CPSMyIp);
    CPS_PUT32(buf - 4, ip);
    offset = 0;
    if (len > CPS_IP_MTU) {
        u8 *p = buf;

        while (len > CPS_IP_MTU) {
            CPSi_SendIpPacket(buf, 0, p, CPS_IP_MTU, ip, 0x2000 | offset);
            offset += CPS_IP_MTU / 8;
            p += CPS_IP_MTU;
            len -= CPS_IP_MTU;
        }
        if (len != 0) {
            CPSi_SendIpPacket(buf, 0, p, len, ip, dataLen != 0 ? (0x2000 | offset) : offset);
            offset += len / 8;
            len = 0;
        }
    }
    while (len + dataLen > CPS_IP_MTU) {
        u32 chunk = CPS_IP_MTU - len;

        CPSi_SendIpPacket(buf, len, data, chunk, ip, 0x2000 | offset);
        offset += CPS_IP_MTU / 8;
        data += chunk;
        dataLen -= chunk;
        len = 0;
    }
    if (len + dataLen != 0) {
        CPSi_SendIpPacket(buf, len, data, dataLen, ip, offset);
    }
}

// CPSi_SendPing: sends an echo request
static void CPSi_SendPing(u8 *data, u32 len, CPSSoc *soc) {
    u16 *icmp = (u16 *)(soc->sndbuf + CPS_SNDBUF_HEADER);

    icmp[0] = 8;
    icmp[2] = (u32)OS_GetCurrentThread();
    icmp[1] = 0;
    icmp[3] = soc->local_port = sPingSeq++;
    CPS_PUT16(&icmp[1], CPSi_FinishSum(CPSi_CalcSum(data, len, CPSi_CalcSum(icmp, 8, 0))));
    CPSi_SendIp((u8 *)icmp, 8, data, len, soc->remote_ip, CPS_PROTO_ICMP);
}

// CPSi_SendUdp
static void CPSi_SendUdp(u8 *data, u32 len, CPSSoc *soc) {
    u8 *buf = soc->sndbuf;
    u8 *udp = buf + CPS_SNDBUF_HEADER;

    CPS_PUT32(udp - 12, CPSMyIp);
    CPS_PUT32(udp - 8, soc->remote_ip);
    *(u16 *)(udp - 4) = CPS_PROTO_UDP << 8;
    CPS_PUT16(udp + 4, (u16)(len + 8));
    *(u16 *)(udp - 2) = *(u16 *)(udp + 4);
    CPS_PUT16(udp + 2, soc->remote_port);
    CPS_PUT16(buf + CPS_SNDBUF_HEADER, soc->local_port);
    *(u16 *)(udp + 6) = 0;
    CPS_PUT16(udp + 6, CPSi_FinishSum(CPSi_CalcSum(data, len, CPSi_CalcSum(udp - 12, 20, 0))));
    CPSi_SendIp(udp, 8, data, len, soc->remote_ip, CPS_PROTO_UDP);
}

// CPSi_SendTcp: sends a segment with as much of the data as fits, returning how much that was
static u32 CPSi_SendTcp(u8 *data, u32 len, CPSSoc *soc, u32 flags) {
    u8 *tcp;
    u32 headerLen;
    int blocks;
    u32 syn;

    if (!(flags & (CPS_TCP_FIN | CPS_TCP_RST)) && soc->state == CPS_STT_CLOSED) {
        return len;
    }
    if (OS_GetCurrentThread() == &sRecvThread) {
        tcp = sReplyBuf + CPS_SNDBUF_HEADER;
    } else {
        tcp = soc->sndbuf + CPS_SNDBUF_HEADER;
    }
    syn = flags & CPS_TCP_SYN;
    if (syn) {
        headerLen = 28;
    } else if ((flags & CPS_TCP_ACK) && soc->sack_permitted && soc->ooo[0].start != 0) {
        int last = -1;
        int best = -1;
        u32 enabled;

        blocks = 0;
        enabled = CPU_IRQDisable();
        u32 mss;
        u32 max;

        // The SACK blocks, the most recent run first
        for (;;) {
            int i;

            for (i = 0; i < 4 && soc->ooo[i].start != 0; i++) {
                if ((best < 0 || soc->ooo[best].order < soc->ooo[i].order) &&
                    (last < 0 || soc->ooo[i].order < soc->ooo[last].order)) {
                    best = i;
                }
            }
            if (best < 0) {
                break;
            }
            CPS_PUT32(tcp + blocks * 8 + 24, soc->ackno + soc->ooo[best].start - soc->rcvbuf_len);
            CPS_PUT32(tcp + blocks * 8 + 28, soc->ackno + soc->ooo[best].start - soc->rcvbuf_len + soc->ooo[best].len);
            last = best;
            best = -1;
            blocks++;
        }
        CPU_SetIRQMask(enabled);
        CPS_PUT32(tcp + 20, 0x01010500 | (blocks * 8 + 2));
        headerLen = blocks * 8 + 24;
        mss = soc->remote_mss < sMyMss ? soc->remote_mss : sMyMss;
        max = mss - 4 - blocks * 8;
        if (len > max) {
            len = max & ~1;
        }
    } else {
        headerLen = 20;
    }
    CPS_PUT32(tcp - 12, CPSMyIp);
    CPS_PUT32(tcp - 8, soc->remote_ip);
    *(u16 *)(tcp - 4) = CPS_PROTO_TCP << 8;
    CPS_PUT16(tcp - 2, (u16)(headerLen + len));
    CPS_PUT16(tcp, soc->local_port);
    CPS_PUT16(tcp + 2, soc->remote_port);
    CPS_PUT32(tcp + 4, soc->seqno);
    CPS_PUT32(tcp + 8, soc->acked = soc->ackno);
    tcp[12] = headerLen << 2;
    tcp[13] = flags;
    soc->acked_window = soc->rcvbuf_size - soc->rcvbuf_len;
    CPS_PUT16(tcp + 14, soc->acked_window);
    *(u16 *)(tcp + 16) = 0;
    *(u16 *)(tcp + 18) = 0;
    if (syn) {
        CPS_PUT32(tcp + 20, 0x02040000 + sMyMss);
        *(u16 *)(tcp + 24) = 0x0101;
        *(u16 *)(tcp + 26) = 0x0204;
    }
    if (flags & CPS_TCP_ACK) {
        soc->delayed_ack = 0;
        soc->ack_time = clock();
    }
    CPS_PUT16(tcp + 16, CPSi_FinishSum(CPSi_CalcSum(data, len, CPSi_CalcSum(tcp - 12, headerLen + 12, 0))));
    soc->seqno += len;
    if (flags & (CPS_TCP_SYN | CPS_TCP_FIN)) {
        soc->seqno++;
    }
    CPSi_SendIp(tcp, headerLen, data, len, soc->remote_ip, CPS_PROTO_TCP);
    return len;
}

// CPSi_ReplyArp: turns a request into its reply and sends it back
static void CPSi_ReplyArp(CPSArpPacket *arp) {
    arp->op = 0x200;
    sys_memcpy_fast(arp->src_mac, arp->dst_mac, 10);
    sys_memcpy_fast(sMyMac, arp->src_mac, 6);
    CPS_PUT32(arp->src_ip, CPSMyIp);
    sys_memcpy_fast(arp->dst_mac, (u8 *)arp - 14, 6);
    sys_memcpy_fast(sMyMac, (u8 *)arp - 8, 6);
    CPSi_SendFrame((u8 *)arp - 14, 42, NULL, 0);
}

// CPSi_ReceiveArp
static void CPSi_ReceiveArp(CPSArpPacket *arp, u32 len) {
    u16 op;
    BOOL fromMe;
    BOOL toMe;
    CPSInAddr src;

    if (len < sizeof(CPSArpPacket)) {
        return;
    }
    if (!CPSi_MacDiffers((u16 *)arp->src_mac, (u16 *)sMyMac)) {
        return;
    }
    if (CPSMyIp == 0) {
        return;
    }
    if (arp->hardware != 0x100 || arp->protocol != 8 || arp->lengths != 0x406) {
        return;
    }
    op = CPS_GET16(&arp->op);
    if (op != 1 && op != 2) {
        return;
    }
    src = CPS_GET32(arp->src_ip);
    fromMe = src == CPSMyIp;
    toMe = CPSMyIp == CPS_GET32(arp->dst_ip);
    if (!fromMe) {
        CPSi_UpdateArp(arp->src_mac, src, toMe);
    }
    if (op == 1 && toMe) {
        CPSi_ReplyArp(arp);
        return;
    }
    if (op == 2 && toMe && fromMe) {
        sIpCollision = TRUE;
    }
}

// CPSi_ReceiveEchoRequest: turns the request into its reply and sends it back
static void CPSi_ReceiveEchoRequest(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len) {
    CPSInAddr hop = CPSi_NextHop(CPS_GET32(ip->src));
    u32 sum;

    if (hop == 0) {
        return;
    }
    if (CPSi_LookupMac(hop) == NULL) {
        CPSi_SendArpRequest(hop);
        return;
    }
    icmp->type = 0;
    sum = icmp->sum + 8;
    icmp->sum = sum + (sum >> 16);
    CPSi_SendIp((u8 *)icmp, len, NULL, 0, CPS_GET32(ip->src), CPS_PROTO_ICMP);
}

// CPSi_ReceiveEchoReply: gives the reply to the socket that pinged
static void CPSi_ReceiveEchoReply(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len) {
    u32 enabled = CPU_IRQDisable();
    OSThread *thread;

    for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
        CPSSoc *soc = thread->specific[0];

        if (soc != NULL && soc->thread != NULL && soc->state == CPS_STT_PING && (u16)(u32)soc->thread == icmp->id &&
            soc->local_port == icmp->seq && soc->rcvbuf_len == 0 && soc->remote_ip == CPS_GET32(ip->src)) {
            len -= sizeof(CPSIcmpHeader);
            if (len > soc->rcvbuf_size) {
                soc->rcvbuf_len = soc->rcvbuf_size;
            } else {
                soc->rcvbuf_len = len;
            }
            sys_memcpy_fast(icmp + 1, soc->rcvbuf, soc->rcvbuf_len);
            if (soc->block_type == CPS_BLOCK_UDPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                scheduler_start_thread(soc->thread);
            }
            break;
        }
    }
    CPU_SetIRQMask(enabled);
}

// CPSi_IsUnicast: whether both addresses are of single hosts
static BOOL CPSi_IsUnicast(CPSInAddr src, CPSInAddr dst) {
    if (src != 0 && !CPSi_IsBroadcast(src) && !CPSi_IsMulticast(src) && dst != 0 && !CPSi_IsBroadcast(dst) &&
        !CPSi_IsMulticast(dst)) {
        return TRUE;
    }
    return FALSE;
}

// CPSi_ReceiveIcmp
static void CPSi_ReceiveIcmp(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len) {
    if (len < sizeof(CPSIcmpHeader)) {
        return;
    }
    if (CPS_CalcCksum(icmp, len) != 0xffff) {
        return;
    }
    if (!CPSi_IsUnicast(CPS_GET32(ip->src), CPS_GET32(ip->dst))) {
        return;
    }
    switch (icmp->type) {
    case 0:
        CPSi_ReceiveEchoReply(ip, icmp, len);
        break;
    case 8:
        CPSi_ReceiveEchoRequest(ip, icmp, len);
        break;
    }
}

// CPSi_FindListener: the socket that listens for a SYN
static CPSSoc *CPSi_FindListener(CPSIpHeader *ip, CPSTcpHeader *tcp) {
    CPSSoc *soc;
    OSThread *thread;

    for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
        soc = thread->specific[0];
        if (soc != NULL && soc->thread != NULL && soc->state == CPS_STT_LISTEN &&
            soc->local_port == CPS_GET16(&tcp->dst_port) &&
            (soc->remote_port == 0 || soc->remote_port == CPS_GET16(&tcp->src_port)) &&
            (soc->remote_ip == 0 || soc->remote_ip == CPS_GET32(ip->src))) {
            return soc;
        }
    }
    return NULL;
}

// CPSi_IsTcpOf: whether a segment belongs to a socket's connection
static BOOL CPSi_IsTcpOf(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc) {
    return soc->state != CPS_STT_DATAGRAM && soc->state != CPS_STT_PING &&
           soc->local_port == CPS_GET16(&tcp->dst_port) && soc->remote_port == CPS_GET16(&tcp->src_port) &&
           soc->remote_ip == CPS_GET32(ip->src);
}

// CPSi_FindTcpSoc
static CPSSoc *CPSi_FindTcpSoc(CPSIpHeader *ip, CPSTcpHeader *tcp) {
    CPSSoc *soc;
    OSThread *thread;

    for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
        soc = thread->specific[0];
        if (soc != NULL && soc->thread != NULL && CPSi_IsTcpOf(ip, tcp, soc)) {
            return soc;
        }
    }
    return NULL;
}

// CPSi_ParseTcpOptions: the peer's MSS and whether it takes SACK
static void CPSi_ParseTcpOptions(CPSTcpHeader *tcp, CPSSoc *soc) {
    u8 *option;
    int len;

    soc->remote_mss = 536;
    len = (tcp->offset & 0xf0) / 4 - (int)sizeof(CPSTcpHeader);
    option = (u8 *)(tcp + 1);
    while (len-- > 0) {
        switch (*option++) {
        case 0:
            return;
        case 1:
            break;
        case 2:
            if (len <= 2 || *option != 4) {
                return;
            }
            soc->remote_mss = (option[1] << 8) | option[2];
            option += 3;
            len -= 3;
            break;
        case 4:
            if (len <= 0 || *option != 2) {
                return;
            }
            soc->sack_permitted = TRUE;
            option++;
            len--;
            break;
        default:
            if (len <= 0 || *option < 2) {
                return;
            }
            len -= *option - 1;
            option += *option - 1;
            break;
        }
    }
}

// CPSi_KnowsNextHop: whether the MAC address of an address's next hop is known (or there is none to know)
static BOOL CPSi_KnowsNextHop(CPSInAddr ip) {
    ip = CPSi_NextHop(ip);
    if (ip == 0) {
        return TRUE;
    }
    return (BOOL)CPSi_LookupMac(ip);
}

// CPSi_SendTcpControl: sends a segment without data. The receiving thread can't wait for ARP, so it asks instead
static void CPSi_SendTcpControl(CPSSoc *soc, u32 flags) {
    if (CPSi_KnowsNextHop(soc->remote_ip) || OS_GetCurrentThread() != &sRecvThread) {
        CPSi_SendTcp(NULL, 0, soc, flags);
        return;
    }
    CPSi_SendArpRequest(CPSi_NextHop(soc->remote_ip));
}

// CPSi_SendAck: acknowledges what came in, now (mode 2), or when there is something to tell (mode 1), or after a
// second segment or the ACK delay
static void CPSi_SendAck(CPSSoc *soc, int mode) {
    if (mode != 2) {
        if (soc->acked == soc->ackno && soc->acked_window == (u16)(soc->rcvbuf_size - soc->rcvbuf_len)) {
            return;
        }
        if (mode != 1) {
            soc->delayed_ack++;
            if (soc->delayed_ack < 2 && (u32)clock() - soc->ack_time < CPS_ACK_DELAY) {
                CPSi_SendDelayedAcks();
                return;
            }
        }
    }
    CPSi_SendTcpControl(soc, CPS_TCP_ACK);
}

// CPSi_SendFin
static void CPSi_SendFin(CPSSoc *soc) {
    CPSi_SendTcpControl(soc, CPS_TCP_FIN | CPS_TCP_ACK);
}

// CPSi_SendReset: answers a segment that belongs to no connection
static void CPSi_SendReset(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = &sReplySoc;
    u8 flags;

    sys_memset(soc, 0, sizeof(CPSSoc));
    soc->local_port = CPS_GET16(&tcp->dst_port);
    soc->remote_port = CPS_GET16(&tcp->src_port);
    soc->remote_ip = CPS_GET32(ip->src);
    flags = tcp->flags;
    if (flags & CPS_TCP_ACK) {
        soc->seqno = CPS_GET32(tcp->ack);
        CPSi_SendTcpControl(soc, CPS_TCP_RST);
        return;
    }
    soc->seqno = 0;
    soc->ackno = len + CPS_GET32(tcp->seq);
    if (flags & (CPS_TCP_SYN | CPS_TCP_FIN)) {
        soc->ackno++;
    }
    CPSi_SendTcpControl(soc, CPS_TCP_RST | CPS_TCP_ACK);
}

// CPSi_AcceptSyn: answers a SYN with a SYN-ACK
static void CPSi_AcceptSyn(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc) {
    soc->state = CPS_STT_SYN_RCVD;
    soc->when = CPS_NOW();
    soc->local_ip_real = CPS_GET32(ip->dst);
    soc->remote_port = CPS_GET16(&tcp->src_port);
    soc->remote_ip = CPS_GET32(ip->src);
    soc->ackno = CPS_GET32(tcp->seq) + 1;
    CPSi_ParseTcpOptions(tcp, soc);
    CPSi_SendTcpControl(soc, CPS_TCP_SYN | CPS_TCP_ACK);
}

// CPSi_ReceiveSynAgain: a SYN for a connection that exists
static BOOL CPSi_ReceiveSynAgain(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = CPSi_FindTcpSoc(ip, tcp);

    if (soc != NULL) {
        if (soc->state == CPS_STT_LISTEN) {
            CPSi_AcceptSyn(ip, tcp, soc);
        } else if (soc->state == CPS_STT_SYN_RCVD) {
            soc->seqno--;
            CPSi_AcceptSyn(ip, tcp, soc);
        } else {
            CPSi_SendReset(ip, tcp, len);
        }
        return TRUE;
    }
    return FALSE;
}

// CPSi_ReceiveSyn
static void CPSi_ReceiveSyn(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc;

    if (!CPSi_IsUnicast(CPS_GET32(ip->src), CPS_GET32(ip->dst))) {
        return;
    }
    if (CPSi_ReceiveSynAgain(ip, tcp, len)) {
        return;
    }
    soc = CPSi_FindListener(ip, tcp);
    if (soc != NULL) {
        CPSi_AcceptSyn(ip, tcp, soc);
        return;
    }
    // Let a thread that is about to listen get to it
    func_0207a92c();
    soc = CPSi_FindListener(ip, tcp);
    if (soc != NULL) {
        CPSi_AcceptSyn(ip, tcp, soc);
    }
}

// CPSi_ReceiveSynAck
static void CPSi_ReceiveSynAck(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = CPSi_FindTcpSoc(ip, tcp);

    if (soc == NULL || soc->state != CPS_STT_SYN_SENT) {
        CPSi_SendReset(ip, tcp, len);
        return;
    }
    func_0207a92c();
    soc->ackno = CPS_GET32(tcp->seq) + 1;
    soc->remote_ackno = CPS_GET32(tcp->ack);
    soc->remote_win = CPS_GET16(&tcp->window);
    CPSi_ParseTcpOptions(tcp, soc);
    CPSi_SendAck(soc, 2);
    soc->state = CPS_STT_ESTABLISHED;
    if (soc->block_type == CPS_BLOCK_TCPCON) {
        soc->block_type = CPS_BLOCK_NONE;
        scheduler_start_thread(soc->thread);
    }
}

// CPSi_MarkOoo: makes a run the most recent
static void CPSi_MarkOoo(CPSSoc *soc, int index) {
    int i;
    u32 max = 0;

    for (i = 0; i < 3 && soc->ooo[i].start != 0; i++) {
        if (max < soc->ooo[i].order) {
            max = soc->ooo[i].order;
        }
    }
    soc->ooo[index].order = max + 1;
}

// CPSi_MergeOoo: takes the runs that the received data reached into it, and joins the runs that touch
static void CPSi_MergeOoo(CPSSoc *soc) {
    u32 start;
    int i;

again:
    start = soc->ooo[0].start;
    if (start != 0) {
        u32 received = soc->rcvbuf_len;

        if (received >= start) {
            u32 end = start + soc->ooo[0].len;

            soc->ackno += end - received;
            soc->rcvbuf_len = end;
            soc->ooo[0].start = 0;
            CPSi_PackOoo(soc);
        }
    }
    for (i = 0; i < 3; i++) {
        if (soc->ooo[i].start != 0 && soc->ooo[i + 1].start != 0 &&
            soc->ooo[i].start + soc->ooo[i].len >= soc->ooo[i + 1].start) {
            soc->ooo[i].len = soc->ooo[i + 1].start + soc->ooo[i + 1].len - soc->ooo[i].start;
            soc->ooo[i + 1].start = 0;
            CPSi_PackOoo(soc);
            goto again;
        }
    }
}

// CPSi_PackOoo: closes the gaps of the runs
static void CPSi_PackOoo(CPSSoc *soc) {
    int i;

    for (i = 0; i < 3; i++) {
        if (soc->ooo[i].start == 0) {
            soc->ooo[i] = soc->ooo[i + 1];
            soc->ooo[i + 1].start = 0;
        }
    }
}

// CPSi_InsertOoo: makes room for a run at index
static void CPSi_InsertOoo(CPSSoc *soc, int index) {
    int i;

    for (i = 2; i >= index; i--) {
        soc->ooo[i + 1] = soc->ooo[i];
    }
    soc->ooo[index].start = 0;
}

// CPSi_ReceiveTcpData: a segment with ACK
static void CPSi_ReceiveTcpData(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    u32 offset = 0;
    CPSSoc *soc = CPSi_FindTcpSoc(ip, tcp);
    u8 flags;
    u32 ack;
    u32 seq;
    u32 enabled;

    if (soc == NULL) {
        CPSi_SendReset(ip, tcp, len);
        return;
    }
    flags = tcp->flags;
    ack = CPS_GET32(tcp->ack);
    if ((s32)(ack - soc->remote_ackno) > 0) {
        soc->remote_ackno = ack;
    }
    seq = CPS_GET32(tcp->seq);
    enabled = CPU_IRQDisable();
    if (soc->state == CPS_STT_ESTABLISHED && soc->ackno != seq) {
        if ((s32)(seq - soc->ackno) < 0) {
            // Partly old: take the rest
            offset = soc->ackno - seq;
        } else {
            // Ahead of what is expected: keep it as a run
            if (!(flags & CPS_TCP_FIN)) {
                offset = soc->rcvbuf_len + seq - soc->ackno;
                if (soc->rcvbuf_len < offset && offset < soc->rcvbuf_size) {
                    if (len > soc->rcvbuf_size - offset) {
                        len = soc->rcvbuf_size - offset;
                    }
                    if (len != 0) {
                        int i;
                        u32 end = offset + len;

                        for (i = 0; i < 4; i++) {
                            u32 runEnd;

                        retry:
                            if (soc->ooo[i].start == 0) {
                                sys_memcpy_fast(CPS_TCP_DATA(tcp), soc->rcvbuf + offset, len);
                                soc->ooo[i].start = offset;
                                soc->ooo[i].len = len;
                                CPSi_MarkOoo(soc, i);
                                CPSi_MergeOoo(soc);
                                break;
                            }
                            runEnd = soc->ooo[i].start + soc->ooo[i].len;
                            if (offset < soc->ooo[i].start && soc->ooo[i].start <= end) {
                                sys_memcpy_fast(CPS_TCP_DATA(tcp), soc->rcvbuf + offset, soc->ooo[i].start - offset);
                                soc->ooo[i].len += soc->ooo[i].start - offset;
                                soc->ooo[i].start = offset;
                                CPSi_MarkOoo(soc, i);
                                CPSi_MergeOoo(soc);
                                break;
                            }
                            if (offset <= runEnd && runEnd < end) {
                                u32 more = end - runEnd;

                                sys_memcpy_fast(CPS_TCP_DATA(tcp) + runEnd - offset, soc->rcvbuf + runEnd, more);
                                soc->ooo[i].len += more;
                                CPSi_MergeOoo(soc);
                                CPSi_MarkOoo(soc, i);
                                break;
                            }
                            if (soc->ooo[i].start <= offset && end <= runEnd) {
                                break;
                            }
                            if (offset < soc->ooo[i].start) {
                                CPSi_InsertOoo(soc, i);
                                goto retry;
                            }
                        }
                    }
                }
            }
            CPU_SetIRQMask(enabled);
            CPSi_SendAck(soc, 2);
            return;
        }
    }
    if (offset > len) {
        // BUG: Returns with the interrupts still disabled
#ifdef BUGFIX
        CPU_SetIRQMask(enabled);
#endif
        return;
    }
    len -= offset;
    soc->remote_win = CPS_GET16(&tcp->window);
    switch (soc->state) {
    case CPS_STT_CLOSED:
    case CPS_STT_SYN_SENT:
        CPU_SetIRQMask(enabled);
        CPSi_SendReset(ip, tcp, len);
        break;
    case CPS_STT_SYN_RCVD:
        soc->state = CPS_STT_ESTABLISHED;
        if (soc->block_type == CPS_BLOCK_TCPCON) {
            soc->block_type = CPS_BLOCK_NONE;
            scheduler_start_thread(soc->thread);
        }
        if (len == 0) {
            CPU_SetIRQMask(enabled);
            break;
        }
        // fallthrough
    case CPS_STT_ESTABLISHED: {
        u32 rcvLen;
        BOOL whole;

        soc->segments++;
        rcvLen = soc->rcvbuf_len;
        if (len > soc->rcvbuf_size - rcvLen) {
            len = soc->rcvbuf_size - rcvLen;
            whole = FALSE;
        } else {
            whole = TRUE;
        }
        if (len != 0) {
            if (soc->ooo[0].start != 0 && soc->ooo[0].start <= rcvLen + len) {
                if (!(flags & CPS_TCP_FIN)) {
                    sys_memcpy_fast(CPS_TCP_DATA(tcp) + offset, soc->rcvbuf + rcvLen, soc->ooo[0].start - rcvLen);
                    len = soc->ooo[0].start + soc->ooo[0].len - soc->rcvbuf_len;
                } else {
                    sys_memcpy_fast(CPS_TCP_DATA(tcp) + offset, soc->rcvbuf + rcvLen, len);
                }
                soc->ooo[0].start = 0;
                CPSi_PackOoo(soc);
            } else {
                sys_memcpy_fast(CPS_TCP_DATA(tcp) + offset, soc->rcvbuf + rcvLen, len);
            }
            soc->rcvbuf_len += len;
            soc->ackno += len;
            CPSi_MergeOoo(soc);
            if (soc->block_type == CPS_BLOCK_TCPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                scheduler_start_thread(soc->thread);
            }
        }
        if (whole && (flags & CPS_TCP_FIN)) {
            soc->ackno++;
            soc->state = CPS_STT_LAST_ACK;
            CPU_SetIRQMask(enabled);
            CPSi_SendFin(soc);
            if (len == 0 && soc->block_type == CPS_BLOCK_TCPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                scheduler_start_thread(soc->thread);
            }
            break;
        }
        CPU_SetIRQMask(enabled);
        if (len != 0) {
            CPSi_SendAck(soc, 0);
        }
        break;
    }
    case CPS_STT_FIN_WAIT1:
    case CPS_STT_FIN_WAIT2:
        if (flags & CPS_TCP_FIN) {
            soc->ackno += len + 1;
            CPU_SetIRQMask(enabled);
            CPSi_SendAck(soc, 2);
            soc->state = CPS_STT_CLOSED;
            if (soc->block_type == CPS_BLOCK_TCPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                scheduler_start_thread(soc->thread);
            }
        } else {
            soc->state = CPS_STT_FIN_WAIT2;
            CPU_SetIRQMask(enabled);
            if (len != 0) {
                soc->ackno += len;
                CPSi_SendAck(soc, 2);
            }
        }
        break;
    case CPS_STT_LAST_ACK:
    case CPS_STT_CLOSING:
        soc->state = CPS_STT_CLOSED;
        CPU_SetIRQMask(enabled);
        if (soc->block_type == CPS_BLOCK_TCPREAD) {
            soc->block_type = CPS_BLOCK_NONE;
            scheduler_start_thread(soc->thread);
        }
        break;
    default:
        CPU_SetIRQMask(enabled);
        if (flags & CPS_TCP_FIN) {
            soc->ackno++;
        }
        CPSi_SendAck(soc, 2);
        break;
    }
    func_0207a92c();
}

// CPSi_ReceiveFin: a FIN without ACK
static void CPSi_ReceiveFin(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = CPSi_FindTcpSoc(ip, tcp);
    u32 enabled;

    if (soc == NULL) {
        return;
    }
    enabled = CPU_IRQDisable();
    switch (soc->state) {
    case CPS_STT_FIN_WAIT1:
        soc->ackno++;
        soc->state = CPS_STT_CLOSING;
        CPU_SetIRQMask(enabled);
        CPSi_SendAck(soc, 2);
        break;
    case CPS_STT_FIN_WAIT2:
        soc->ackno++;
        CPU_SetIRQMask(enabled);
        CPSi_SendAck(soc, 2);
        soc->state = CPS_STT_CLOSED;
        if (soc->block_type == CPS_BLOCK_TCPREAD) {
            soc->block_type = CPS_BLOCK_NONE;
            scheduler_start_thread(soc->thread);
        }
        break;
    case CPS_STT_ESTABLISHED:
        soc->ackno++;
        soc->state = CPS_STT_LAST_ACK;
        CPU_SetIRQMask(enabled);
        CPSi_SendFin(soc);
        break;
    default:
        CPU_SetIRQMask(enabled);
        CPSi_SendReset(ip, tcp, len);
        break;
    }
}

// CPSi_ReceiveRst
static void CPSi_ReceiveRst(CPSIpHeader *ip, CPSTcpHeader *tcp) {
    CPSSoc *soc = CPSi_FindTcpSoc(ip, tcp);

    if (soc == NULL) {
        return;
    }
    func_0207a92c();
    soc->state = CPS_STT_CLOSED;
    if (soc->block_type == CPS_BLOCK_TCPCON || soc->block_type == CPS_BLOCK_TCPREAD) {
        soc->block_type = CPS_BLOCK_NONE;
        scheduler_start_thread(soc->thread);
    }
}

// CPSi_ReceiveTcp
static void CPSi_ReceiveTcp(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    u8 flags;

    if (len < sizeof(CPSTcpHeader)) {
        return;
    }
    if ((tcp->offset & 0xf0) < 0x50 || len < CPS_TCP_HEADER_LEN(tcp)) {
        return;
    }
    if (CPSi_BadPseudoSum(tcp, len, ip, CPS_PROTO_TCP)) {
        return;
    }
    len -= CPS_TCP_HEADER_LEN(tcp);
    flags = tcp->flags;
    switch (flags & (CPS_TCP_FIN | CPS_TCP_SYN | CPS_TCP_RST | CPS_TCP_ACK)) {
    case CPS_TCP_SYN:
        if (!(flags & (CPS_TCP_URG | CPS_TCP_PSH))) {
            CPSi_ReceiveSyn(ip, tcp, len);
        }
        break;
    case CPS_TCP_SYN | CPS_TCP_ACK:
        if (!(flags & (CPS_TCP_URG | CPS_TCP_PSH))) {
            CPSi_ReceiveSynAck(ip, tcp, len);
        }
        break;
    case CPS_TCP_ACK:
    case CPS_TCP_ACK | CPS_TCP_FIN:
        CPSi_ReceiveTcpData(ip, tcp, len);
        break;
    case CPS_TCP_FIN:
        CPSi_ReceiveFin(ip, tcp, len);
        break;
    default:
        if (flags & CPS_TCP_RST) {
            CPSi_ReceiveRst(ip, tcp);
        } else {
            CPSi_SendReset(ip, tcp, len);
        }
        break;
    }
}

// CPSi_ReceiveUdp: gives a datagram to the socket bound to its port, when that has taken the last one
static void CPSi_ReceiveUdp(CPSIpHeader *ip, CPSUdpHeader *udp, u32 len) {
    u32 enabled;
    OSThread *thread;

    if (len < sizeof(CPSUdpHeader)) {
        return;
    }
    if (udp->sum != 0 && CPSi_BadPseudoSum(udp, len, ip, CPS_PROTO_UDP)) {
        return;
    }
    enabled = CPU_IRQDisable();
    for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
        CPSSoc *soc = thread->specific[0];

        if (soc != NULL && soc->thread != NULL && soc->state == CPS_STT_DATAGRAM &&
            soc->local_port == CPS_GET16(&udp->dst_port) &&
            (soc->remote_port == 0 || soc->remote_port == CPS_GET16(&udp->src_port)) &&
            (soc->remote_ip == 0 || soc->remote_ip == CPS_BROADCAST_ADDR || soc->remote_ip == CPS_GET32(ip->src))) {
            soc->local_ip_real = CPS_GET32(ip->dst);
            if (soc->remote_ip == 0) {
                soc->remote_ip = CPS_GET32(ip->src);
                soc->remote_port = CPS_GET16(&udp->src_port);
            }
            if (soc->rcvbuf_len == 0) {
                len -= sizeof(CPSUdpHeader);
                if (len > soc->rcvbuf_size) {
                    soc->rcvbuf_len = soc->rcvbuf_size;
                } else {
                    soc->rcvbuf_len = len;
                }
                sys_memcpy_fast(udp + 1, soc->rcvbuf, soc->rcvbuf_len);
                if (soc->block_type == CPS_BLOCK_UDPREAD) {
                    soc->block_type = CPS_BLOCK_NONE;
                    scheduler_start_thread(soc->thread);
                } else if (soc->udp_callback != NULL && soc->udp_callback(soc->rcvbuf, soc->rcvbuf_len, soc)) {
                    soc->rcvbuf_len = 0;
                }
            }
            break;
        }
    }
    CPU_SetIRQMask(enabled);
}

// CPSi_Reassemble: the whole packet once its fragments are all in, which allocated says to free, or NULL
static CPSIpHeader *CPSi_Reassemble(CPSIpHeader *ip, BOOL *allocated) {
    CPSReassembly *free = NULL;
    CPSReassembly *reassembly;
    u32 frag;
    u32 len;
    u32 offset;
    u32 headerLen;
    u32 total;
    u16 id;
    CPSInAddr src;
    u32 i;
    u32 end;
    u16 last;
    u32 j;
    u16 covered;
    CPSIpHeader *whole;

    *allocated = FALSE;
    frag = CPS_GET16(&ip->frag);
    if (!(frag & 0x3fff)) {
        return ip;
    }
    headerLen = (ip->ver_len & 0xf) << 2;
    reassembly = sReassembly;
    id = ip->id;
    src = CPS_GET32(ip->src);
    for (i = 0; i < CPS_REASSEMBLY_COUNT; i++, reassembly++) {
        if (reassembly->count != 0 && reassembly->src == src && reassembly->id == id) {
            break;
        }
        if (reassembly->count == 0 && free == NULL) {
            free = reassembly;
        }
    }
    len = CPS_GET16(&ip->len) - headerLen;
    offset = frag & 0x1fff;
    total = len + (offset << 3);
    if (i == CPS_REASSEMBLY_COUNT) {
        if (free == NULL || total > CPS_REASSEMBLY_SIZE) {
            return NULL;
        }
        reassembly = free;
        free->buf = CPSiAlloc(headerLen + CPS_REASSEMBLY_SIZE + sizeof(CPSEtherHeader));
        if (free->buf == NULL) {
            return NULL;
        }
        free->src = src;
        free->id = id;
        free->last = 0;
        free->time = CPS_NOW();
        free->data = free->buf + sizeof(CPSEtherHeader) + headerLen;
        sys_memcpy_fast(ip, free->buf + sizeof(CPSEtherHeader), headerLen);
    }
    if (reassembly->count == 8 || total > CPS_REASSEMBLY_SIZE) {
        reassembly->count = 0;
        CPSiFree(reassembly->buf);
        return NULL;
    }
    end = offset + ((len + 7) >> 3);
    if (!(frag & 0x2000)) {
        reassembly->len = total;
        reassembly->last = end;
    }
    reassembly->start[reassembly->count] = offset;
    reassembly->end[reassembly->count] = end;
    reassembly->count++;
    sys_memcpy_fast((u8 *)ip + headerLen, reassembly->data + (offset << 3), len);
    last = reassembly->last;
    if (last == 0) {
        return NULL;
    }
    covered = 0;
    for (j = 0; j < reassembly->count;) {
        if (reassembly->start[j] <= covered && covered < reassembly->end[j]) {
            covered = reassembly->end[j];
            j = 0;
        } else {
            j++;
        }
    }
    if (covered < last) {
        return NULL;
    }
    whole = (CPSIpHeader *)(reassembly->buf + sizeof(CPSEtherHeader));
    CPS_PUT16(&whole->len, (u16)(reassembly->len + ((whole->ver_len & 0xf) << 2)));
    reassembly->count = 0;
    *allocated = TRUE;
    return whole;
}

// CPSi_ReceiveIp
static void CPSi_ReceiveIp(CPSIpHeader *ip, u32 len) {
    BOOL allocated;
    u8 proto;
    u32 headerLen;
    u32 dataLen;

    if (CPS_GET32(ip->dst) != CPS_GET32(ip->src)) {
        u16 total;

        if (len < sizeof(CPSIpHeader)) {
            return;
        }
        if ((ip->ver_len & 0xf) < 5) {
            return;
        }
        total = CPS_GET16(&ip->len);
        if (total < (ip->ver_len & 0xf) << 2 || len < total) {
            return;
        }
        if (!CPSi_IsForMe(CPS_GET32(ip->dst))) {
            return;
        }
        if (CPS_CalcCksum(ip, (ip->ver_len & 0xf) << 2) != 0xffff) {
            return;
        }
        if (CPSMyIp == CPS_GET32(ip->dst)) {
            CPSi_UpdateArp((u8 *)ip - 8, CPS_GET32(ip->src), TRUE);
        }
    }
    ip = CPSi_Reassemble(ip, &allocated);
    if (ip == NULL) {
        return;
    }
    headerLen = (ip->ver_len & 0xf) << 2;
    dataLen = CPS_GET16(&ip->len) - headerLen;
    proto = ip->proto;
    if (proto == CPS_PROTO_UDP) {
        CPSi_ReceiveUdp(ip, (CPSUdpHeader *)((u8 *)ip + headerLen), dataLen);
    } else if (CPSMyIp != 0) {
        if (proto == CPS_PROTO_ICMP) {
            CPSi_ReceiveIcmp(ip, (CPSIcmpHeader *)((u8 *)ip + headerLen), dataLen);
        } else if (proto == CPS_PROTO_TCP) {
            CPSi_ReceiveTcp(ip, (CPSTcpHeader *)((u8 *)ip + headerLen), dataLen);
        }
    }
    if (allocated) {
        CPSiFree((u8 *)ip - sizeof(CPSEtherHeader));
    }
}

// CPSi_RecvThread
static void CPSi_RecvThread(void *arg) {
    for (;;) {
        u32 len;
        CPSEtherHeader *frame = CPSi_GetFrame(&len);

        if (len > CPS_SNDBUF_HEADER) {
            switch (CPS_GET16(&frame->type)) {
            case CPS_ETHERTYPE_IP:
                CPSi_ReceiveIp((CPSIpHeader *)(frame + 1), len - sizeof(CPSEtherHeader));
                break;
            case CPS_ETHERTYPE_ARP:
                CPSi_ReceiveArp((CPSArpPacket *)(frame + 1), len - sizeof(CPSEtherHeader));
                break;
            }
        }
        CPSi_ConsumeFrame();
    }
}

// CPSi_GetNewPort
u16 CPSi_GetNewPort(void) {
    BOOL used;

    do {
        OSThread *thread;

        used = FALSE;
        sLastPort++;
        if (sLastPort < 1024 || sLastPort >= 5000) {
            sLastPort = 1024;
        }
        for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
            CPSSoc *soc = thread->specific[0];

            if (soc != NULL && soc->thread != NULL && soc->local_port == sLastPort) {
                used = TRUE;
                break;
            }
        }
    } while (used);
    return sLastPort;
}

// CPSi_Rand
static u32 CPSi_Rand(void) {
    return MATH_Rand32(&CPSiRand, 0);
}

// CPS_SocRegister
void CPS_SocRegister(CPSSoc *soc) {
    OS_GetCurrentThread()->specific[0] = soc;
}

// CPS_SocUnRegister
void CPS_SocUnRegister(void) {
    OS_GetCurrentThread()->specific[0] = NULL;
}

// CPS_SocDatagramMode
void CPS_SocDatagramMode(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        soc->state = CPS_STT_DATAGRAM;
        soc->rcvbuf_len = 0;
    }
}

// CPS_SocBind
void CPS_SocBind(u16 localPort, u16 remotePort, CPSInAddr remoteIp) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (remoteIp == CPS_LOOPBACK_ADDR) {
            remoteIp = CPSMyIp;
        }
        soc->remote_port = soc->remote_port_bound = remotePort;
        soc->remote_ip = soc->remote_ip_bound = remoteIp;
        if (localPort == 0) {
            soc->local_port = CPSi_GetNewPort();
        } else {
            soc->local_port = localPort;
        }
    }
}

// CPS_SocUse
void CPS_SocUse(void) {
    OSThread *thread = OS_GetCurrentThread();
    CPSSoc *soc = thread->specific[0];

    if (soc != NULL) {
        soc->thread = thread;
        soc->state = CPS_STT_CLOSED;
        soc->rcvbuf_len = 0;
        sys_memset(soc->ooo, 0, sizeof(soc->ooo));
        soc->outbuf_len = 0;
        soc->udp_callback = NULL;
        soc->sack_permitted = FALSE;
    }
}

// CPS_SocRelease
void CPS_SocRelease(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        soc->thread = NULL;
    }
}

// CPS_SocDup
void CPS_SocDup(OSThread *thread) {
    thread->specific[0] = OS_GetCurrentThread()->specific[0];
}

// CPS_SetUdpCallback
void CPS_SetUdpCallback(BOOL (*callback)(u8 *data, u32 len, CPSSoc *soc)) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        soc->udp_callback = callback;
    }
}

// CPSi_TcpConnect: sends the SYN up to three times, returning 0 once connected
int CPSi_TcpConnect(CPSSoc *soc) {
    u32 seqno = CPSi_Rand();
    u32 i;

    for (i = 0; i < 3; i++) {
        u32 enabled;

        soc->seqno = seqno;
        soc->state = CPS_STT_SYN_SENT;
        soc->when = CPS_NOW();
        CPSi_SendTcpControl(soc, CPS_TCP_SYN);
        enabled = CPU_IRQDisable();
        if (soc->state == CPS_STT_SYN_SENT && CPSMyIp != 0) {
            soc->block_type = CPS_BLOCK_TCPCON;
            scheduler_yield(NULL);
        }
        CPU_SetIRQMask(enabled);
        if (soc->state == CPS_STT_ESTABLISHED) {
            return 0;
        }
        if (CPSMyIp == 0) {
            break;
        }
    }
    return 1;
}

// CPS_TcpConnect
int CPS_TcpConnect(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->ssl) {
            return CPS_SslConnect(soc);
        }
        return CPSi_TcpConnect(soc);
    }
    return 1;
}

// CPSi_TcpShutdown
static void CPSi_TcpShutdown(CPSSoc *soc) {
    u32 enabled;

    func_0207a92c();
    enabled = CPU_IRQDisable();
    if (soc->state == CPS_STT_SYN_RCVD || soc->state == CPS_STT_ESTABLISHED) {
        soc->state = CPS_STT_FIN_WAIT1;
        CPU_SetIRQMask(enabled);
        CPSi_SendFin(soc);
        return;
    }
    if (soc->state != CPS_STT_CLOSED) {
        CPU_SetIRQMask(enabled);
        CPSi_SendAck(soc, 2);
        return;
    }
    CPU_SetIRQMask(enabled);
}

// CPS_TcpShutdown
void CPS_TcpShutdown(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->ssl) {
            CPS_SslShutdown(soc);
        }
        CPSi_TcpShutdown(soc);
    }
}

// CPS_TcpClose: waits up to about five seconds for the connection to close
void CPS_TcpClose(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();
    s32 start;

    if (soc == NULL) {
        return;
    }
    if (soc->ssl) {
        CPS_SslClose(soc);
    }
    start = CPS_NOW();
    while (sLinkIsOn() && soc->state != CPS_STT_CLOSED && CPS_NOW() - start < 39) {
        CPSi_Yield();
    }
    soc->state = CPS_STT_CLOSED;
}

// CPSi_UdpRead
static u8 *CPSi_UdpRead(u32 *len, CPSSoc *soc) {
    u32 enabled = CPU_IRQDisable();
    u32 received = soc->rcvbuf_len;

    while (received == 0) {
        soc->block_type = CPS_BLOCK_UDPREAD;
        scheduler_yield(NULL);
        received = soc->rcvbuf_len;
    }
    CPU_SetIRQMask(enabled);
    *len = received;
    return soc->rcvbuf;
}

// CPSi_TcpRead
u8 *CPSi_TcpRead(u32 *len, CPSSoc *soc) {
    if (soc->state == CPS_STT_ESTABLISHED) {
        CPSi_SendAck(soc, 1);
    }
    if (soc->rcvbuf_len == 0 && soc->state == CPS_STT_ESTABLISHED) {
        u32 enabled = CPU_IRQDisable();

        while (soc->rcvbuf_len == 0 && soc->state == CPS_STT_ESTABLISHED) {
            soc->block_type = CPS_BLOCK_TCPREAD;
            scheduler_yield(NULL);
        }
        CPU_SetIRQMask(enabled);
    } else {
        func_0207a92c();
    }
    if ((*len = soc->rcvbuf_len) != 0) {
        return soc->rcvbuf;
    }
    return NULL;
}

// CPS_SocRead
u8 *CPS_SocRead(u32 *len) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->state == CPS_STT_DATAGRAM || soc->state == CPS_STT_PING) {
            return CPSi_UdpRead(len, soc);
        }
        if (soc->ssl) {
            return CPS_SslRead(len, soc);
        }
        return CPSi_TcpRead(len, soc);
    }
    *len = 0;
    return NULL;
}

// CPSi_TcpConsume: drops data from the front of the receive buffer, moving the runs after it along
void CPSi_TcpConsume(u32 len, CPSSoc *soc) {
    u32 enabled = CPU_IRQDisable();
    u32 received = soc->rcvbuf_len;
    BOOL wasFull = FALSE;
    u32 start;

    if (received == soc->rcvbuf_size && len != 0) {
        wasFull = TRUE;
    }
    if (len >= received) {
        soc->rcvbuf_len = 0;
    } else {
        soc->rcvbuf_len = received - len;
        sys_memcpy_ex(soc->rcvbuf + len, soc->rcvbuf, received - len);
    }
    start = soc->ooo[0].start;
    if (start != 0) {
        u32 consumed = received - soc->rcvbuf_len;

        if (consumed != 0) {
            int i;

            for (i = 0; i < 4 && soc->ooo[i].start != 0; i++) {
            }
            if (start > consumed) {
                u8 *runs = soc->rcvbuf + start;

                sys_memcpy_ex(runs, runs - consumed, soc->ooo[i - 1].start + soc->ooo[i - 1].len - start);
                for (i = 0; i < 4 && soc->ooo[i].start != 0; i++) {
                    soc->ooo[i].start -= consumed;
                }
            } else {
                sys_memset(soc->ooo, 0, sizeof(soc->ooo));
                sys_exit();
            }
        }
    }
    CPU_SetIRQMask(enabled);
    if (soc->state != CPS_STT_DATAGRAM && soc->state != CPS_STT_PING) {
        if (soc->rcvbuf_len == 0 || wasFull) {
            CPSi_SendAck(soc, 0);
        }
    }
}

// CPS_SocConsume
void CPS_SocConsume(u32 len) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->ssl) {
            CPS_SslReadRelease(len, soc);
            return;
        }
        CPSi_TcpConsume(len, soc);
    }
}

// CPSi_TcpSendSegments: sends data in segments while the window lasts, or a byte to probe a closed window
static u32 CPSi_TcpSendSegments(u8 *data, u32 len, CPSSoc *soc, BOOL probe) {
    u32 window = 1;
    u32 sent;
    u32 segments;
    u32 budget;

    if (!probe) {
        window = soc->remote_win;
    }
    segments = soc->segments;
    budget = segments * 2 + 4;
    while (len != 0 && soc->state == CPS_STT_ESTABLISHED) {
        sent = soc->remote_mss < window ? soc->remote_mss : window;
        if (sMyMss < sent) {
            sent = sMyMss;
        }
        if (!probe) {
            sent &= ~1;
        }
        if (len < sent) {
            sent = len;
        }
        budget += soc->segments - segments;
        segments = soc->segments;
        if (budget-- == 0) {
            sent = 0;
        }
        if (sent == 0) {
            break;
        }
        sent = CPSi_SendTcp(data, sent, soc, CPS_TCP_PSH | CPS_TCP_ACK);
        func_0207a92c();
        window -= sent;
        data += sent;
        len -= sent;
    }
    return sent;
}

// CPSi_TcpSendBoth
static u32 CPSi_TcpSendBoth(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc, BOOL probe) {
    u32 sent = CPSi_TcpSendSegments(data, len, soc, probe);

    if (sent != 0 && len2 != 0) {
        sent = CPSi_TcpSendSegments(data2, len2, soc, FALSE);
    }
    return sent;
}

// CPSi_TcpWrite: sends both pieces of data and waits for their ACKs, returning how much of it was acknowledged
u32 CPSi_TcpWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc) {
    u32 total = 0;
    BOOL probe;
    u32 base;
    s32 lastProgress;

    soc->segments = 0;
    probe = FALSE;
    lastProgress = CPS_NOW();
    while (sLinkIsOn() && len != 0 && soc->state == CPS_STT_ESTABLISHED && CPS_NOW() - lastProgress < 159) {
        s32 start;
        u32 acked;

        base = soc->seqno;
        CPSi_TcpSendBoth(data, len, data2, len2, soc, probe);
        start = CPS_NOW();
        do {
            CPSi_Yield();
        } while (sLinkIsOn() && soc->state == CPS_STT_ESTABLISHED && soc->seqno != soc->remote_ackno &&
                 CPS_NOW() - start < 15 && (!probe || soc->remote_win == 0));
        acked = soc->remote_ackno - base;
        if (acked > soc->seqno - base) {
            acked = 0;
        }
        total += acked;
        if (acked != 0) {
            lastProgress = CPS_NOW();
        }
        soc->seqno = soc->remote_ackno;
        if (soc->state == CPS_STT_ESTABLISHED && soc->remote_win == 0 && acked == 0) {
            if (!probe) {
                start = CPS_NOW();
                while (sLinkIsOn() && CPS_NOW() - start < 15) {
                    CPSi_Yield();
                    if (soc->remote_win != 0) {
                        break;
                    }
                }
                if (soc->remote_win == 0) {
                    probe = TRUE;
                }
            }
        } else {
            probe = FALSE;
        }
        if (acked >= len) {
            u32 over = acked - len;

            data = data2 + over;
            len = len2 - over;
            data2 = NULL;
            len2 = 0;
        } else {
            data += acked;
            len -= acked;
        }
    }
    return total;
}

// CPSi_SocWrite: sends both pieces of data the way the socket sends, returning how much went, or 0 on an error
static u32 CPSi_SocWrite(u8 *data, u32 len, u8 *data2, u32 len2) {
    CPSSoc *soc = CPS_CURRENT_SOC();
    u32 sent;

    if (soc != NULL) {
        if (soc->state == CPS_STT_DATAGRAM) {
            if (len != 0) {
                CPSi_SendUdp(data, len, soc);
            }
            if (len2 != 0) {
                CPSi_SendUdp(data2, len2, soc);
            }
            sent = len + len2;
        } else if (soc->state == CPS_STT_PING) {
            if (len != 0) {
                CPSi_SendPing(data, len, soc);
            }
            if (len2 != 0) {
                CPSi_SendPing(data2, len2, soc);
            }
            sent = len + len2;
        } else if (soc->ssl) {
            sent = CPS_SslWrite(data, len, data2, len2, soc);
        } else {
            sent = CPSi_TcpWrite(data, len, data2, len2, soc);
        }
        if (!sSendFailed) {
            return sent;
        }
    }
    return 0;
}

// CPS_SocWrite
u32 CPS_SocWrite(u8 *data, u32 len) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        u32 pending = soc->outbuf_len;

        if (pending != 0) {
            u32 sent = CPSi_SocWrite(soc->outbuf, pending, data, len);

            if (sent < soc->outbuf_len) {
                sys_memcpy_ex(soc->outbuf + sent, soc->outbuf, soc->outbuf_len - sent);
                soc->outbuf_len -= sent;
                return 0;
            }
            sent -= soc->outbuf_len;
            soc->outbuf_len = 0;
            return sent;
        }
        return CPSi_SocWrite(data, len, NULL, 0);
    }
    return 0;
}

// CPS_SocGetLength
s32 CPS_SocGetLength(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->state == CPS_STT_ESTABLISHED) {
            CPSi_SendAck(soc, 1);
        }
        if (soc->ssl) {
            return CPS_SslCanRead(soc);
        }
        if (soc->rcvbuf_len != 0 || soc->state == CPS_STT_ESTABLISHED || soc->state == CPS_STT_DATAGRAM ||
            soc->state == CPS_STT_PING) {
            return soc->rcvbuf_len;
        }
        return -1;
    }
    return 0;
}

// CPS_SocFlush
void CPS_SocFlush(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL && soc->outbuf_len != 0) {
        CPSi_SocWrite(soc->outbuf, soc->outbuf_len, NULL, 0);
        soc->outbuf_len = 0;
    }
}

// CPSi_CheckCollision: asks twice who has the address and waits for an answer, FALSE when another host has it
static BOOL CPSi_CheckCollision(void) {
    s32 start;

    CPSi_SendArpRequest(CPSMyIp);
    func_0207aa04(100);
    CPSi_SendArpRequest(CPSMyIp);
    start = CPS_NOW();
    while (sLinkIsOn() && CPS_NOW() - start < 23) {
        if (sIpCollision) {
            return FALSE;
        }
        func_0207aa04(100);
    }
    return TRUE;
}

// CPSi_DhcpGiveUp: tells the DHCP callback, and checks a static address
static void CPSi_DhcpGiveUp(void) {
    sDhcpCallback();
    if (CPSMyIp != 0 && !CPSi_CheckCollision()) {
        CPSi_SetNoIp(CPS_NOIP_REASON_COLLISION);
    }
}

// Whether the address check callback rejects the address DHCP gave
static inline BOOL CPSi_AddressRejected(void) {
    return sAddressCheck != NULL && !sAddressCheck();
}

// CPSi_ScavengerThread: once a second, runs DHCP and drops what timed out
static void CPSi_ScavengerThread(void *arg) {
    int state;
    u32 arpTimer;
    u32 leaseTimer;
    BOOL first;
    s32 now;
    int i;
    OSThread *thread;

    sCalmDown = FALSE;
    state = DHCP_STATE_INIT;
    sys_memset(&sDhcpSoc, 0, sizeof(CPSSoc));
    sDhcpSoc.rcvbuf_size = sizeof(sDhcpRcvBuf);
    sDhcpSoc.rcvbuf = sDhcpRcvBuf;
    sDhcpSoc.sndbuf_size = sizeof(sDhcpSndBuf);
    sDhcpSoc.sndbuf = sDhcpSndBuf;
    CPS_SocRegister(&sDhcpSoc);
    arpTimer = 1;
    leaseTimer = 1;
    CPSNoIpReason = CPS_NOIP_REASON_LINKOFF;
    first = TRUE;
    for (;;) {
        func_0207aa04(1000);
        if (sCalmDown) {
            break;
        }
        now = CPS_NOW();
        if (sLinkIsOn()) {
            if (--leaseTimer == 0) {
                if (CPSMode & CPS_MODE_STATIC_IP) {
                    if (state == DHCP_STATE_INIT) {
                        CPSi_DhcpGiveUp();
                        state = DHCP_STATE_BOUND;
                    }
                } else {
                    switch (state) {
                    case DHCP_STATE_INIT:
                        if (first) {
                            CPSNoIpReason = CPS_NOIP_REASON_DHCPDISCOVERY;
                            first = FALSE;
                        }
                        if (CPSi_DhcpDiscover() && CPSi_DhcpRequest(&leaseTimer, 0)) {
                            if (CPSi_AddressRejected()) {
                                CPSi_SetNoIp(CPS_NOIP_REASON_REJECTED);
                                CPSi_DhcpGiveUp();
                                state = DHCP_STATE_DONE;
                            } else {
                                state = DHCP_STATE_BOUND;
                                if (!CPSi_CheckCollision()) {
                                    CPSi_SetNoIp(CPS_NOIP_REASON_COLLISION);
                                    state = DHCP_STATE_DONE;
                                }
                            }
                        } else {
                            CPSi_DhcpGiveUp();
                            state = DHCP_STATE_DONE;
                        }
                        break;
                    case DHCP_STATE_BOUND:
                        if (!CPSi_DhcpRequest(&leaseTimer, 1) && leaseTimer < 60) {
                            state = DHCP_STATE_REBINDING;
                        } else if (CPSi_AddressRejected()) {
                            CPSi_SetNoIp(CPS_NOIP_REASON_REJECTED);
                            CPSi_DhcpGiveUp();
                            state = DHCP_STATE_DONE;
                        } else if (!CPSi_CheckCollision()) {
                            CPSi_SetNoIp(CPS_NOIP_REASON_COLLISION);
                            state = DHCP_STATE_DONE;
                        }
                        break;
                    case DHCP_STATE_REBINDING:
                        if (CPSi_DhcpRequest(&leaseTimer, 2)) {
                            if (CPSi_AddressRejected()) {
                                CPSi_SetNoIp(CPS_NOIP_REASON_REJECTED);
                                CPSi_DhcpGiveUp();
                                state = DHCP_STATE_DONE;
                            } else {
                                state = DHCP_STATE_BOUND;
                                if (!CPSi_CheckCollision()) {
                                    CPSi_SetNoIp(CPS_NOIP_REASON_COLLISION);
                                    state = DHCP_STATE_DONE;
                                }
                            }
                        } else if (leaseTimer < 60) {
                            CPSi_SetNoIp(CPS_NOIP_REASON_LEASETIMEOUT);
                            state = DHCP_STATE_INIT;
                            arpTimer = 1;
                            leaseTimer = 1;
                        }
                        break;
                    case DHCP_STATE_DONE:
                        break;
                    }
                }
            }
        } else {
            arpTimer = 1;
            CPSi_SetNoIp(CPS_NOIP_REASON_LINKOFF);
            state = DHCP_STATE_INIT;
            leaseTimer = arpTimer;
        }
        for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
            if (sArpCache[i].ip != 0 && (s16)(now - sArpCache[i].time) > 957) {
                sArpCache[i].ip = 0;
            }
        }
        if (CPSGatewayIp != 0 && --arpTimer == 0) {
            CPSi_SendArpRequest(CPSGatewayIp);
            arpTimer = 105;
        }
        for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
            CPSSoc *soc = thread->specific[0];

            if (soc != NULL && soc->thread != NULL) {
                if (soc->state == CPS_STT_SYN_RCVD && now - soc->when > 39) {
                    soc->state = CPS_STT_LISTEN;
                    soc->remote_port = soc->remote_port_bound;
                    soc->remote_ip = soc->remote_ip_bound;
                } else if (soc->state == CPS_STT_SYN_SENT && now - soc->when > 39) {
                    if (soc->block_type == CPS_BLOCK_TCPCON) {
                        soc->state = CPS_STT_CLOSED;
                        soc->block_type = CPS_BLOCK_NONE;
                        scheduler_start_thread(soc->thread);
                    }
                } else if (soc->state != CPS_STT_ESTABLISHED && soc->block_type == CPS_BLOCK_TCPREAD) {
                    soc->block_type = CPS_BLOCK_NONE;
                    scheduler_start_thread(soc->thread);
                }
            }
        }
        for (i = 0; i < CPS_REASSEMBLY_COUNT; i++) {
            CPSReassembly *reassembly = &sReassembly[i];

            if (reassembly->count != 0 && now - reassembly->time > 239) {
                CPSiFree(reassembly->buf);
                reassembly->count = 0;
            }
        }
        CPS_SslTick(now);
        if (sScavengerCallback != NULL) {
            sScavengerCallback();
        }
    }
    if (!(CPSMode & CPS_MODE_STATIC_IP) && state != DHCP_STATE_DONE) {
        CPSi_DhcpRelease();
    }
    CPS_SocUnRegister();
}

// CPSi_SendDelayedAcks: sends the ACKs that waited long enough, and sets the alarm for the next one
static void CPSi_SendDelayedAcks(void) {
    u32 wait = 0xffffffff;
    u32 enabled = CPU_IRQDisable();
    u32 now = clock();
    OSThread *thread;

    for (thread = g_IsThreadSwitchRequested.list; thread != NULL; thread = thread->next) {
        CPSSoc *soc = thread->specific[0];

        if (soc != NULL && soc->thread != NULL && soc->state == CPS_STT_ESTABLISHED && soc->delayed_ack != 0) {
            u32 waited = now - soc->ack_time;

            if (waited >= CPS_ACK_DELAY) {
                CPSi_SendAck(soc, 1);
            } else if (wait > CPS_ACK_DELAY - waited) {
                wait = CPS_ACK_DELAY - waited;
            }
        }
    }
    if (wait != 0xffffffff) {
        func_0207bd3c(&sAckAlarm);
        func_0207bcf4(&sAckAlarm, wait, CPSi_AckAlarm, NULL);
    }
    CPU_SetIRQMask(enabled);
}

// CPSi_AckThread
static void CPSi_AckThread(void *arg) {
    while (!scheduler_is_thread_ended(&sScavengerThread)) {
        CPSi_SendDelayedAcks();
        scheduler_yield(NULL);
    }
}

// CPSi_AckAlarm
static void CPSi_AckAlarm(void *arg) {
    scheduler_start_thread(&sAckThread);
}

// CPSi_DhcpMakeMessage: the BOOTP header and the options every message has, returning where the rest go
static u8 *CPSi_DhcpMakeMessage(u8 *buf, u8 type) {
    u8 *option;

    sys_memset(buf, 0, 0xec);
    *(u16 *)buf = 0x101;
    buf[2] = 6;
    CPS_PUT32(buf + 4, sDhcpXid);
    CPS_PUT32(buf + 12, CPSMyIp);
    sys_memcpy_fast(sMyMac, buf + 28, 6);
    *(u16 *)(buf + 0xec) = 0x8263;
    *(u16 *)(buf + 0xee) = 0x6353;
    *(u16 *)(buf + 0xf0) = 0x135;
    buf[0xf2] = type;
    buf[0xf3] = 61;
    buf[0xf4] = 7;
    buf[0xf5] = 1;
    sys_memcpy_fast(sMyMac, buf + 0xf6, 6);
    option = buf + 0xfc;
    if (type != DHCP_RELEASE) {
        option[0] = 12;
        option[1] = 10;
        sys_memcpy_fast("NintendoDS", option + 2, 10);
        option[12] = 55;
        option[13] = 3;
        option[14] = 1;
        option[15] = 3;
        option[16] = 6;
        option += 17;
    }
    return option;
}

// CPSi_Pad: fills the rest of the first size bytes, of which used are taken
static u8 *CPSi_Pad(u32 value, u32 size, u8 *p, u32 used) {
    if (used < size) {
        u32 rest = size - used;

        sys_memset(p, value, rest);
        p += rest;
    }
    return p;
}

#define DHCP_BUF (sDhcpSndBuf + CPS_SNDBUF_HEADER + sizeof(CPSUdpHeader))

// CPSi_DhcpSendDiscover
static u32 CPSi_DhcpSendDiscover(void) {
    u8 *buf = DHCP_BUF;
    u8 *p;

    sDhcpXid = MATH_Rand32(&CPSiRand, 0);
    p = CPSi_DhcpMakeMessage(buf, DHCP_DISCOVER);
    if (sRequestedIp != 0) {
        p[0] = 50;
        p[1] = 4;
        CPS_PUTBE32(p + 2, sRequestedIp);
        p += 6;
    }
    *p++ = 0xff;
    CPS_SocWrite(buf, CPSi_Pad(0, DHCP_MESSAGE_SIZE, p, p - buf) - buf);
    return sDhcpXid;
}

// CPSi_DhcpSendRequest: after an offer (mode 0), or to renew (1) or rebind (2) the lease
static u32 CPSi_DhcpSendRequest(int mode) {
    u8 *buf = DHCP_BUF;
    u8 *p = CPSi_DhcpMakeMessage(buf, DHCP_REQUEST);

    if (mode == 0) {
        p[0] = 50;
        p[1] = 4;
        CPS_PUTBE32(p + 2, sRequestedIp);
        p[6] = 54;
        p[7] = 4;
        CPS_PUTBE32(p + 8, sDhcpServerIp);
        p += 12;
    }
    *p++ = 0xff;
    CPS_SocWrite(buf, CPSi_Pad(0, DHCP_MESSAGE_SIZE, p, p - buf) - buf);
    return sDhcpXid;
}

// CPSi_DhcpWait: waits for the answer to a message and takes its options
static int CPSi_DhcpWait(u32 xid, int try) {
    s32 start;
    s32 timeout;
    int result;

    timeout = sDhcpWaits[try] * 7;
    start = CPS_NOW();
    result = DHCP_WAIT_NONE;

    while (sLinkIsOn() && result == DHCP_WAIT_NONE && CPS_NOW() - start < timeout) {
        u32 len;
        u8 *msg;

        if (CPS_SocGetLength() == 0) {
            CPSi_Yield();
            continue;
        }
        msg = CPS_SocRead(&len);
        if (len > 0xf0 && msg[0] == 2 && xid == CPS_GET32(msg + 4) &&
            !CPSi_MacDiffers((u16 *)(msg + 28), (u16 *)sMyMac)) {
            CPSInAddr yourIp;
            u8 *end;

            result = DHCP_WAIT_OTHER;
            yourIp = CPS_BE32(msg + 16);
            end = msg + len;
            if (msg[0xec] == 0x63 && msg[0xed] == 0x82 && msg[0xee] == 0x53 && msg[0xef] == 0x63) {
                u8 *option = msg + 0xf0;
                u32 staticDns = CPSMode & CPS_MODE_STATIC_DNS;
                u8 code;

                while (option < end && (code = *option++) != 0xff) {
                    if (code == 0) {
                        continue;
                    }
                    switch (code) {
                    case 1:
                        CPSNetMask = CPS_BE32(option + 1);
                        break;
                    case 3:
                        CPSGatewayIp = CPS_BE32(option + 1);
                        break;
                    case 6:
                        if (!staticDns) {
                            CPSDnsIp[1] = option[0] < 8 ? 0 : CPS_BE32(option + 5);
                            CPSDnsIp[0] = CPS_BE32(option + 1);
                        }
                        break;
                    case 51:
                        sLeaseTime = CPS_BE32(option + 1);
                        break;
                    case 53:
                        if (option[1] == DHCP_OFFER) {
                            sRequestedIp = yourIp;
                            result = DHCP_WAIT_OFFER;
                        } else if (option[1] == DHCP_ACK) {
                            CPSMyIp = yourIp;
                            result = DHCP_WAIT_ACK;
                        }
                        break;
                    case 54:
                        sDhcpServerIp = CPS_BE32(option + 1);
                        break;
                    }
                    option += *option + 1;
                }
            }
        }
        CPS_SocConsume(len);
    }
    return result;
}

// CPSi_DhcpDiscover: TRUE once a server offered an address
static BOOL CPSi_DhcpDiscover(void) {
    int result;
    int i;

    CPS_SocUse();
    CPS_SocDatagramMode();
    CPS_SocBind(DHCP_CLIENT_PORT, DHCP_SERVER_PORT, CPS_BROADCAST_ADDR);
    for (i = 0; i < 5; i++) {
        result = CPSi_DhcpWait(CPSi_DhcpSendDiscover(), i);
        if (result == DHCP_WAIT_OFFER) {
            break;
        }
    }
    CPS_SocRelease();
    return result == DHCP_WAIT_OFFER;
}

// CPSi_DhcpRequest: TRUE once the server acknowledged; timer gets the seconds until the next request
static BOOL CPSi_DhcpRequest(u32 *timer, int mode) {
    int result;
    int i;

    CPS_SocUse();
    CPS_SocDatagramMode();
    if (mode == 1) {
        CPS_SocBind(DHCP_CLIENT_PORT, DHCP_SERVER_PORT, sDhcpServerIp);
    } else {
        CPS_SocBind(DHCP_CLIENT_PORT, DHCP_SERVER_PORT, CPS_BROADCAST_ADDR);
    }
    for (i = 0; i < 5; i++) {
        result = CPSi_DhcpWait(CPSi_DhcpSendRequest(mode), i);
        if (result != DHCP_WAIT_NONE && result != DHCP_WAIT_OFFER) {
            break;
        }
    }
    CPS_SocRelease();
    if (result == DHCP_WAIT_ACK) {
        *timer = sLeaseTime / 2;
        sRenewTime = sLeaseTime * 3 / 8;
        return TRUE;
    }
    sRenewTime /= 2;
    *timer = sRenewTime;
    switch (mode) {
    case 1:
        if (*timer < 60) {
            *timer = 1;
            sRenewTime = sLeaseTime / 8;
        }
        break;
    case 2:
        if (*timer < 60) {
            *timer = 1;
        }
        break;
    }
    return FALSE;
}

// CPSi_DhcpRelease
static void CPSi_DhcpRelease(void) {
    u8 *buf;
    u8 *p;

    CPS_SocUse();
    CPS_SocDatagramMode();
    CPS_SocBind(DHCP_CLIENT_PORT, DHCP_SERVER_PORT, sDhcpServerIp);
    buf = DHCP_BUF;
    p = CPSi_DhcpMakeMessage(buf, DHCP_RELEASE);
    p[0] = 54;
    p[1] = 4;
    CPS_PUTBE32(p + 2, sDhcpServerIp);
    p[6] = 0xff;
    p += 7;
    CPS_SocWrite(buf, CPSi_Pad(0, DHCP_MESSAGE_SIZE, p, p - buf) - buf);
    CPS_SocRelease();
}

// CPSi_DnsSkipName
static u8 *CPSi_DnsSkipName(u8 *p) {
    u8 len = *p++;

    while (len != 0) {
        if ((len & 0xc0) == 0xc0) {
            return p + 1;
        }
        p += len;
        len = *p++;
    }
    return p;
}

// CPSi_DnsQuery: asks the bound server about a name, returning an address, 1 or 2 for a name that did or didn't fit
// in result, -1 for no such name, or 0 when no answer came
static s32 CPSi_DnsQuery(const u8 *name, u32 type, u16 id, u8 *result, u32 resultSize) {
    u32 len;
    u8 query[0x44];
    s32 start;
    u8 *labelLen;
    u8 *p;
    s32 answer;
    u8 c;

    *(u16 *)(query + 0) = CPS_SWAP16(id);
    if (type != DNS_TYPE_NB) {
        *(u16 *)(query + 2) = 0x0001;
    } else {
        *(u16 *)(query + 2) = 0x1001;
    }
    *(u16 *)(query + 4) = 0x100;
    *(u16 *)(query + 6) = 0;
    *(u16 *)(query + 8) = 0;
    *(u16 *)(query + 10) = 0;
    len = 0;
    labelLen = query + 12;
    p = query + 13;
    while ((c = *name++) != 0) {
        if (p - query > 0x3c) {
            return -1;
        }
        if (c != '.') {
            len++;
            *p++ = c;
        } else if (len != 0) {
            *labelLen = len;
            labelLen = p++;
            len = 0;
        }
    }
    *labelLen = len;
    p[0] = 0;
    p[1] = type >> 8;
    p[2] = type;
    p[3] = 0;
    p[4] = 1;
    answer = 0;
    CPS_SocWrite(query, p + 5 - query);
    start = CPS_NOW();
    while (sLinkIsOn() && answer == 0 && CPS_NOW() - start < 15) {
        u8 *reply;

        if (CPS_SocGetLength() == 0) {
            func_0207aa04(10);
            continue;
        }
        reply = CPS_SocRead(&len);
        if (len > 12 && CPS_GET16(reply) == id) {
            if ((reply[3] & 0xf) == 3) {
                answer = -1;
            } else if ((reply[3] & 0xf) == 0) {
                u8 *end = reply + len;
                u32 questions = CPS_BE16(reply + 4);

                p = reply + 12;
                while (questions-- != 0) {
                    p = CPSi_DnsSkipName(p) + 4;
                }
                while (p < end) {
                    u16 dataLen;

                    p = CPSi_DnsSkipName(p);
                    dataLen = CPS_BE16(p + 8);
                    if (CPS_BE16(p) == type) {
                        if (type != DNS_TYPE_PTR) {
                            answer = CPS_BE32(p + 10 + dataLen - 4);
                        } else if (dataLen > resultSize) {
                            answer = 2;
                        } else {
                            sys_memcpy_fast(p + 10, result, dataLen);
                            answer = 1;
                        }
                        break;
                    }
                    p += 10 + dataLen;
                }
            }
        }
        CPS_SocConsume(len);
    }
    return answer;
}

// CPSi_ParseNumber
static u32 CPSi_ParseNumber(const u8 *text, const u8 **end) {
    u32 n;

    *end = text;
    n = 0;
    for (;;) {
        u8 digit = *text - '0';

        if (digit > 9) {
            break;
        }
        n = digit + n * 10;
        text++;
        *end = text;
    }
    return n;
}

// CPSi_ParseIp: a dotted address
static BOOL CPSi_ParseIp(const u8 *text, CPSInAddr *addr) {
    CPSInAddr ip = 0;
    int i;

    for (i = 0; i < 4; i++) {
        const u8 *end;
        u32 n;

        ip <<= 8;
        n = CPSi_ParseNumber(text, &end);
        if (text == end) {
            return FALSE;
        }
        text = end;
        if (n > 255 || (i != 3 && *text++ != '.') || (i == 3 && *text != '\0')) {
            return FALSE;
        }
        ip |= n;
    }
    *addr = ip;
    return TRUE;
}

// CPSi_DnsLookup: asks a server for a name's address
static s32 CPSi_DnsLookup(const u8 *name, CPSInAddr server, u16 id) {
    s32 result;

    if (server == 0) {
        return -1;
    }
    CPS_SocUse();
    CPS_SocDatagramMode();
    CPS_SocBind(0, DNS_PORT, server);
    result = CPSi_DnsQuery(name, DNS_TYPE_A, id, NULL, 0);
    CPS_SocRelease();
    return result;
}

// CPS_Resolve: the address of a name, or of a dotted address, or 0
CPSInAddr CPS_Resolve(const char *name) {
    s32 result = 0;
    struct {
        u8 usable[2];
        u16 id[2];
    } servers;
    int round = 0;
    int i;

    servers.id[0] = MATH_Rand32(&CPSiRand, 0x10000);
    servers.id[1] = MATH_Rand32(&CPSiRand, 0x10000);
    if (CPSi_ParseIp((const u8 *)name, (CPSInAddr *)&result)) {
        return result;
    }
    servers.usable[0] = TRUE;
    servers.usable[1] = TRUE;
    for (; round < 3; round++) {
        for (i = 0; i < 2; i++) {
            CPSInAddr hop;

            if (!servers.usable[i]) {
                continue;
            }
            if (CPSDnsIp[i] == 0 || (hop = CPSi_NextHop(CPSDnsIp[i])) == 0 ||
                (CPSi_LookupMac(hop) == NULL && CPSi_ResolveMac(hop) == NULL)) {
                servers.usable[i] = FALSE;
                continue;
            }
            result = CPSi_DnsLookup((const u8 *)name, CPSDnsIp[i], servers.id[i]);
            if (result != 0 && result != -1) {
                goto done;
            }
            if (result == -1) {
                servers.usable[i] = FALSE;
            }
        }
    }
done:
    if (result == -1) {
        result = 0;
    }
    return result;
}
