#include "nitrowifi/cps.h"
#include "cps_internal.h"
#include "dwc/wcm.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// NitroWiFi's CPS, the TCP/IP stack under the socket layer (SOCL) in overlay 11: ARP, IP with its fragments, ICMP's
// echo, UDP, TCP, the DHCP client and DNS lookups. The ROM names neither the file nor its
// functions: cps.c is a guess, and the names are NitroWiFi's own where the code shows the role (see cps.h).
static void empty_func(void);
static BOOL default_link_is_on(void);
static u32 calc_checksum_do(void *data, u32 len, u32 sum);
static u16 invert_checksum(u16 sum);
static u16 calc_checksum(void *data, u32 len);
static BOOL check_tcpudpsum(void *data, u32 len, CPSIpHeader *ip, u32 proto);
static BOOL ip_islocal(CPSInAddr ip);
static CPSInAddr get_targetip(CPSInAddr ip);
static BOOL is_broadcast(CPSInAddr ip);
static BOOL is_multicast(CPSInAddr ip);
static BOOL ip_isme(CPSInAddr ip);
static BOOL maccmp(u16 *a, u16 *b);
static void send_packet(u8 *frame, u32 len, u8 *data, u32 dataLen);
static void put_in_buffer(const u8 *src, const u8 *dst, const u8 *header, u32 headerLen, const u8 *data, u32 dataLen);
static CPSEtherHeader *receive_packet(u32 *len);
static void throw_packet(void);
static u8 *inq_arpcache(CPSInAddr ip);
static void send_arprequest(CPSInAddr ip);
static u8 *arprequest(CPSInAddr ip);
static void reg_arpcache(u8 *mac, CPSInAddr ip, BOOL add);
static void send_ether(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u16 type);
static void send_ip_frag(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u32 frag);
static void send_ip(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u8 proto);
static void send_ping(u8 *data, u32 len, CPSSoc *soc);
static void send_udp(u8 *data, u32 len, CPSSoc *soc);
static u32 send_tcp(u8 *data, u32 len, CPSSoc *soc, u8 flags);
static void reply_arp(CPSArpPacket *arp);
static void dispatch_arp(CPSArpPacket *arp, u32 len);
static void reply_icmp(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len);
static void process_icmp_reply(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len);
static BOOL valid_IP(CPSInAddr src, CPSInAddr dst);
static void dispatch_icmp(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len);
static CPSSoc *check_listener(CPSIpHeader *ip, CPSTcpHeader *tcp);
static BOOL check_socket(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc);
static CPSSoc *find_socket(CPSIpHeader *ip, CPSTcpHeader *tcp);
static void parse_mss(CPSTcpHeader *tcp, CPSSoc *soc);
static BOOL no_need_inq(CPSInAddr ip);
static void tcp_send_handshake(CPSSoc *soc, u8 flags);
static void tcp_send_ack(CPSSoc *soc, int mode);
static void tcp_send_finack(CPSSoc *soc);
static void tcp_send_rst(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void dt_syn_listen(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc);
static BOOL find_specific_socket(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void dt_syn(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void dt_synack(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void CPSi_MarkOoo(CPSSoc *soc, int index);
static void CPSi_MergeOoo(CPSSoc *soc);
static void CPSi_PackOoo(CPSSoc *soc);
static void CPSi_InsertOoo(CPSSoc *soc, int index);
static void dt_ack(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void dt_fin(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void dt_rst(CPSIpHeader *ip, CPSTcpHeader *tcp);
static void dispatch_tcp(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len);
static void dispatch_udp(CPSIpHeader *ip, CPSUdpHeader *udp, u32 len);
static CPSIpHeader *check_frag(CPSIpHeader *ip, BOOL *allocated);
static void dispatch_ip(CPSIpHeader *ip, u32 len);
static void tcpip(void *arg);
static u32 get_seqno(void);
static void CPSi_TcpShutdownRaw(CPSSoc *soc);
static u8 *udp_read_raw(u32 *len, CPSSoc *soc);
static u32 tcp_write_do(u8 *data, u32 len, CPSSoc *soc, BOOL probe);
static u32 tcp_write_do2(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc, BOOL probe);
static u32 CPSi_SocWrite2(u8 *data, u32 len, u8 *data2, u32 len2);
static BOOL CPSi_CheckCollision(void);
static void set_fixed_ip(void);
static void scavenger(void *arg);
static void CPSi_SendDelayedAcks(void);
static void CPSi_AckThread(void *arg);
static void CPSi_AckAlarm(void *arg);
static u8 *dhcp_setcommon(u8 *buf, u8 type);
static u8 *pad_mem(u8 value, u32 size, u8 *p, u32 used);
static u32 dhcp_send_discover(void);
static u32 dhcp_send_request(int mode);
static int dhcp_analyze_response(u32 xid, int try);
static BOOL dhcp_discover_server(void);
static BOOL dhcp_request_server(u32 *timer, int mode);
static void dhcp_release_server(void);
static u8 *dns_skipname(u8 *p);
static s32 resolve_common(const u8 *name, u32 type, u16 id, u8 *result, u32 resultSize);
static u32 strtol10(const u8 *text, const u8 **end);
static BOOL rawip(const u8 *text, CPSInAddr *addr);
static s32 resolve_sub(const u8 *name, CPSInAddr server, u16 id);

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
// the ROM's layout
static u8 sSendFailed;
static u8 sIpCollision;
static u16 sMyMss;
static u16 sLastPort;
static u16 sPingSeq;
static u16 sIpId;
static void (*sDhcpCallback)(void);
u32 CPSMode;
int CPSNoIpReason;
static u32 sRenewTime;
static void (*sScavengerCallback)(void);
static u32 sLeaseTime;
static u8 *sLanBuf;
static u32 sYieldWait;
static volatile u32 sLanRead; // the receiving thread advances it while the receive callback reads it, at interrupt time
static BOOL (*sAddressCheck)(void);
void *(*CPSiAlloc)(u32 size);
static volatile u32 sLanWrite; // advanced by the wireless driver's receive callback, at interrupt time
static u32 sDhcpXid;
CPSInAddr CPSGatewayIp;
static CPSInAddr sRequestedIp;
static BOOL (*sLinkIsOn)(void);
void (*CPSiFree)(void *ptr);
CPSInAddr CPSNetMask;
static CPSInAddr sDhcpServerIp;
CPSInAddr CPSMyIp;
static OSThread *volatile sRecvWaiter; // read by the receive callback, at interrupt time, to wake the thread
static BOOL sCalmDown;
static volatile u32 sLanBufLen; // read by the receive callback, at interrupt time, while CPS_Cleanup clears it
static u8 sMyMac[6];
CPSInAddr CPSDnsIp[2];
MATHRandContext32 CPSiRand32ctx;
static OSAlarm sAckAlarm;
// Where the receiving thread builds the TCP segments it answers with
static u8 sReplyBuf[0x5c];
static CPSArpCache sArpCache[CPS_ARP_CACHE_SIZE];
static CPSSoc sReplySoc;
static CPSSoc sDhcpSoc;
static OSThread sRecvThread;
static OSThread sScavengerThread;
static OSThread sAckThread;
static u8 sDhcpRcvBuf[0x180];
static u8 sDhcpSndBuf[0x180];
static CPSFragTable sReassembly[CPS_REASSEMBLY_COUNT];
static u8 sRecvStack[CPS_STACK_SIZE] ATTRIBUTE_ALIGN(32);
static u8 sScavengerStack[CPS_STACK_SIZE] ATTRIBUTE_ALIGN(32);
static u8 sAckStack[CPS_STACK_SIZE] ATTRIBUTE_ALIGN(32);

#define CPS_CURRENT_SOC() ((CPSSoc *)OS_GetCurrentThread()->specific[0])

// CPS_SetAddressCheckCallback
void CPS_SetAddressCheckCallback(BOOL (*callback)(void)) {
    sAddressCheck = callback;
}

// reset_network_vars: drops the address, and with it the ARP cache, the connections and the fragments
static void reset_network_vars(int reason) {
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
        MI_CpuFill8(sArpCache, 0, sizeof(sArpCache));
        for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
            CPSSoc *soc = thread->specific[0];

            if (soc != NULL && soc->thread != NULL) {
                if (soc->state != CPS_STT_DATAGRAM && soc->state != CPS_STT_PING) {
                    soc->state = CPS_STT_CLOSED;
                }
                if (soc->block_type != CPS_BLOCK_NONE) {
                    soc->block_type = CPS_BLOCK_NONE;
                    OS_WakeupThreadDirect(soc->thread);
                }
            }
        }
        for (i = 0; i < CPS_REASSEMBLY_COUNT; i++) {
            CPSFragTable *reassembly = &sReassembly[i];

            if (reassembly->count != 0) {
                CPSiFree(reassembly->buf);
                reassembly->count = 0;
            }
        }
        CPSi_SslCleanup();
    }
}

// yield_thread: lets the other threads run while waiting
static void yield_thread(void) {
    if (sYieldWait == 0) {
        OS_YieldThread();
    } else {
        OS_Sleep(sYieldWait);
    }
}

// empty_func: the default of the callbacks and of the allocator
static void empty_func(void) {
}

// default_link_is_on: the default link check
static BOOL default_link_is_on(void) {
    return TRUE;
}

// CPS_Startup
void CPS_Startup(CPSConfig *config) {
    SDK_USING_MIDDLEWARE(CPSi_Signature);
    if (config->random_seed != 0) {
        MATH_InitRand32(&CPSiRand32ctx, config->random_seed);
    } else {
        MATH_InitRand32(&CPSiRand32ctx, OS_GetTick());
    }
    if (config->alloc && config->free) {
        CPSiAlloc = config->alloc;
        CPSiFree = config->free;
    } else {
        CPSiAlloc = (void *)empty_func;
        CPSiFree = (void *)empty_func;
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
        sDhcpCallback = empty_func;
    }
    if (config->link_is_on) {
        sLinkIsOn = config->link_is_on;
    } else {
        sLinkIsOn = default_link_is_on;
    }
    sLanBuf = config->lan_buf;
    sLanBufLen = config->lan_buflen;
    sLanRead = 0;
    sLanWrite = 0;
    sLastPort = 1024 + MATH_Rand32(&CPSiRand32ctx, 5000 - 1024);
    WCM_GetMacAddress(sMyMac);
    sIpCollision = FALSE;
    OS_CreateThread(&sRecvThread, tcpip, NULL, sRecvStack + CPS_STACK_SIZE, CPS_STACK_SIZE, sThreadPriority);
    OS_CreateThread(&sScavengerThread, scavenger, NULL, sScavengerStack + CPS_STACK_SIZE, CPS_STACK_SIZE,
                    sThreadPriority);
    OS_WakeupThreadDirect(&sRecvThread);
    OS_WakeupThreadDirect(&sScavengerThread);
    OS_CreateAlarm(&sAckAlarm);
    OS_CreateThread(&sAckThread, CPSi_AckThread, NULL, sAckStack + CPS_STACK_SIZE, CPS_STACK_SIZE, sThreadPriority - 1);
}

// CPS_CalmDown
int CPS_CalmDown(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    BOOL ended = OS_IsThreadTerminated(&sScavengerThread);

    if (!ended && !sCalmDown) {
        sCalmDown = TRUE;
        OS_WakeupThreadDirect(&sScavengerThread);
    }
    OS_RestoreInterrupts(enabled);
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
    OS_JoinThread(&sScavengerThread);
    OS_DestroyThread(&sRecvThread);
    OS_CancelAlarm(&sAckAlarm);
    enabled = OS_DisableInterrupts();
    if (!OS_IsThreadTerminated(&sAckThread)) {
        OS_WakeupThreadDirect(&sAckThread);
    }
    OS_RestoreInterrupts(enabled);
    OS_JoinThread(&sAckThread);
    sRecvWaiter = NULL;
    reset_network_vars(CPS_NOIP_REASON_NONE);
    sLanBuf = NULL;
    sLanBufLen = 0;
}

// CPS_SetThreadPriority
void CPS_SetThreadPriority(u32 priority) {
    sThreadPriority = priority;
    OS_SetThreadPriority(&sRecvThread, priority);
    OS_SetThreadPriority(&sScavengerThread, priority);
}

// calc_checksum_do: adds the 16-bit words of data to sum, in ones' complement
static u32 calc_checksum_do(void *data, u32 len, u32 sum) {
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

// invert_checksum
static u16 invert_checksum(u16 sum) {
    sum ^= 0xffff;
    if (sum == 0) {
        sum = 0xffff;
    }
    return sum;
}

// calc_checksum
static u16 calc_checksum(void *data, u32 len) {
    return invert_checksum(calc_checksum_do(data, len, 0));
}

// check_tcpudpsum: checks a UDP or TCP packet's sum, with the pseudo-header of its addresses, protocol and length
static BOOL check_tcpudpsum(void *data, u32 len, CPSIpHeader *ip, u32 proto) {
    u32 sum = calc_checksum_do(ip->src, 8, calc_checksum_do(data, len, proto));

    sum += len;

    if (sum & 0x10000) {
        sum = (u16)(sum + 1);
    }
    return sum != 0xffff;
}

// ip_islocal: whether an address is reached without the gateway
static BOOL ip_islocal(CPSInAddr ip) {
    BOOL onLink = TRUE;

    if (ip != CPS_BROADCAST_ADDR && ip != CPS_LOOPBACK_ADDR && (ip & CPSNetMask) != (CPSMyIp & CPSNetMask)) {
        onLink = FALSE;
    }
    return onLink;
}

// get_targetip
static CPSInAddr get_targetip(CPSInAddr ip) {
    if (!ip_islocal(ip)) {
        ip = CPSGatewayIp;
    }
    return ip;
}

// is_broadcast
static BOOL is_broadcast(CPSInAddr ip) {
    BOOL broadcast = FALSE;

    if (ip_islocal(ip)) {
        u32 host = ~CPSNetMask;

        if (host == (ip & host)) {
            broadcast = TRUE;
        }
    }
    return broadcast;
}

// is_multicast
static BOOL is_multicast(CPSInAddr ip) {
    if ((ip & 0xf0000000) == 0xe0000000) {
        return TRUE;
    }
    return FALSE;
}

// ip_isme
static BOOL ip_isme(CPSInAddr ip) {
    return CPSMyIp == 0 || ip == CPSMyIp || ip == CPS_LOOPBACK_ADDR || is_broadcast(ip) || is_multicast(ip);
}

// maccmp
static BOOL maccmp(u16 *a, u16 *b) {
    int i;

    for (i = 0; i < 3; i++) {
        if (*a++ != *b++) {
            return TRUE;
        }
    }
    return FALSE;
}

// send_packet: sends an Ethernet frame as an 802.11 one, with the SNAP header over its source address
static void send_packet(u8 *frame, u32 len, u8 *data, u32 dataLen) {
    MI_CpuCopy(CPS_SNAP_HEADER, frame + 6, 6);
    sSendFailed = WCM_SendDCFDataEx(frame, frame + 6, len - 6, data, dataLen) < 0;
}

// put_in_buffer: puts a received frame in the LAN buffer, as its size and the Ethernet frame that it was
static void put_in_buffer(const u8 *src, const u8 *dst, const u8 *header, u32 headerLen, const u8 *data, u32 dataLen) {
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
    MI_CpuCopy(dst, sLanBuf + sLanWrite + 2, 6);
    MI_CpuCopy(src, sLanBuf + sLanWrite + 8, 6);
    MI_CpuCopy(header + 6, sLanBuf + sLanWrite + 14, headerLen - 6);
    if (data != NULL && dataLen != 0) {
        MI_CpuCopy(data, sLanBuf + sLanWrite + 8 + headerLen, dataLen);
    }
    sLanWrite = next;
}

// CPSi_RecvCallbackFunc
void CPSi_RecvCallbackFunc(const u8 *src, const u8 *dst, const u8 *data, s32 len) {
    put_in_buffer(src, dst, data, len, NULL, 0);
    if (sRecvWaiter != NULL && !OS_IsThreadTerminated(sRecvWaiter)) {
        OS_WakeupThreadDirect(sRecvWaiter);
    }
}

// receive_packet: waits for a frame in the LAN buffer
static CPSEtherHeader *receive_packet(u32 *len) {
    u32 enabled = OS_DisableInterrupts();
    u8 *buf;
    u16 size;

    while (sLanRead == sLanWrite) {
        sRecvWaiter = OS_GetCurrentThread();
        OS_SleepThread(NULL);
        sRecvWaiter = NULL;
    }
    OS_RestoreInterrupts(enabled);
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

// throw_packet
static void throw_packet(void) {
    u32 enabled = OS_DisableInterrupts();

    sLanRead += *(u16 *)(sLanBuf + sLanRead);
    if (sLanRead >= sLanBufLen) {
        sLanRead = 0;
    }
    OS_RestoreInterrupts(enabled);
}

// inq_arpcache: the MAC address of an address in the cache, or NULL
static u8 *inq_arpcache(CPSInAddr ip) {
    u32 enabled = OS_DisableInterrupts();
    u8 *mac = NULL;
    u32 i;

    if (ip == CPS_LOOPBACK_ADDR || ip == CPSMyIp) {
        mac = sMyMac;
    } else if (is_broadcast(ip) || is_multicast(ip)) {
        mac = sBroadcastMac;
    } else {
        for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
            CPSArpCache *entry = &sArpCache[i];

            if (ip == entry->ip) {
                sArpCache[i].time = CPSi_GetTick();
                mac = entry->mac;
                break;
            }
        }
    }
    OS_RestoreInterrupts(enabled);
    return mac;
}

// send_arprequest
static void send_arprequest(CPSInAddr ip) {
    u8 frame[42];

    MI_CpuFill8(frame, 0, 42);
    MI_CpuFill8(frame, 0xff, 6);
    MI_CpuCopy(sMyMac, frame + 6, 6);
    *(u16 *)(frame + 12) = 0x608;
    frame[15] = 1;
    frame[16] = 8;
    *(u16 *)(frame + 18) = 0x406;
    frame[21] = 1;
    MI_CpuCopy(sMyMac, frame + 22, 6);
    CPS_PUT32(frame + 28, CPSMyIp);
    CPS_PUT32(frame + 38, ip);
    send_packet(frame, 42, NULL, 0);
}

// arprequest: asks for an address's MAC address until it comes, or until the waits run out
static u8 *arprequest(CPSInAddr ip) {
    u32 try;
    u32 i;
    u8 *mac;

    for (try = 0; try < 6; try++) {
        send_arprequest(ip);
        for (i = 0; i < sArpWaits[try] / 100; i++) {
            if (CPSMyIp == 0) {
                return NULL;
            }
            OS_Sleep(100);
            mac = inq_arpcache(ip);
            if (mac != NULL) {
                return mac;
            }
        }
    }
    return NULL;
}

// reg_arpcache: refreshes an address's entry, or takes a free or the oldest one for it when add is set
static void reg_arpcache(u8 *mac, CPSInAddr ip, BOOL add) {
    u32 i;
    u32 victim;
    u16 now;
    u16 oldest;

    if (ip == CPS_LOOPBACK_ADDR || ip == CPSMyIp || !ip_islocal(ip) || is_multicast(ip)) {
        return;
    }
    now = CPSi_GetTick();
    for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
        if (ip == sArpCache[i].ip) {
            sArpCache[i].time = now;
            MI_CpuCopy(mac, sArpCache[i].mac, 6);
            return;
        }
    }
    if (!add) {
        return;
    }
    oldest = 0;
    victim = 0;
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
    MI_CpuCopy(mac, sArpCache[victim].mac, 6);
    sArpCache[victim].time = now;
}

// send_ether: puts the Ethernet header in front of buf and sends the frame to the next hop
static void send_ether(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u16 type) {
    CPS_PUT16(buf - 2, type);
    if (!is_multicast(ip)) {
        u8 *mac;

        ip = get_targetip(ip);
        if (ip == 0) {
            return;
        }
        mac = inq_arpcache(ip);
        if (mac == NULL) {
            mac = arprequest(ip);
        }
        if (mac == NULL) {
            return;
        }
        MI_CpuCopy(mac, buf - 14, 6);
    } else {
        buf[-14] = 0x01;
        buf[-13] = 0x00;
        buf[-12] = 0x5e;
        buf[-11] = (ip >> 16) & 0x7f;
        buf[-10] = ip >> 8;
        buf[-9] = ip;
    }
    MI_CpuCopy(sMyMac, buf - 8, 6);
    send_packet(buf - 14, len + 14, data, dataLen);
}

// send_ip_frag: finishes the IP header in front of buf and sends one packet, to itself through the LAN buffer
static void send_ip_frag(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u32 frag) {
    CPS_PUT16(buf - 18, (u16)(len + 20 + dataLen));
    CPS_PUT16(buf - 14, (u16)frag);
    *(u16 *)(buf - 10) = 0;
    CPS_PUT16(buf - 10, calc_checksum(buf - 20, 20));
    if (ip != CPS_LOOPBACK_ADDR && ip != CPSMyIp) {
        send_ether(buf - 20, len + 20, data, dataLen, ip, CPS_ETHERTYPE_IP);
    }
    if (ip == CPS_LOOPBACK_ADDR || ip == CPSMyIp || is_multicast(ip)) {
        u32 enabled;

        MI_CpuCopy(CPS_SNAP_HEADER, buf - 28, 8);
        enabled = OS_DisableInterrupts();
        put_in_buffer(sMyMac, sMyMac, buf - 28, len + 28, data, dataLen);
        OS_RestoreInterrupts(enabled);
    }
}

// send_ip: puts the IP header in front of buf and sends the packet, in fragments when it is too long
static void send_ip(u8 *buf, u32 len, u8 *data, u32 dataLen, CPSInAddr ip, u8 proto) {
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
            send_ip_frag(buf, 0, p, CPS_IP_MTU, ip, 0x2000 | offset);
            p += CPS_IP_MTU;
            len -= CPS_IP_MTU;
            offset += CPS_IP_MTU / 8;
        }
        if (len != 0) {
            if (dataLen != 0) {
                send_ip_frag(buf, 0, p, len, ip, 0x2000 | offset);
            } else {
                send_ip_frag(buf, 0, p, len, ip, offset);
            }
            offset += len / 8;
            len = 0;
        }
    }
    while (len + dataLen > CPS_IP_MTU) {
        u32 chunk = CPS_IP_MTU - len;

        send_ip_frag(buf, len, data, chunk, ip, 0x2000 | offset);
        data += chunk;
        dataLen -= chunk;
        offset += CPS_IP_MTU / 8;
        len = 0;
    }
    if (len + dataLen != 0) {
        send_ip_frag(buf, len, data, dataLen, ip, offset);
    }
}

// send_ping: sends an echo request
static void send_ping(u8 *data, u32 len, CPSSoc *soc) {
    u16 *icmp = (u16 *)(soc->sndbuf.data + CPS_SNDBUF_HEADER);
    u16 sequence;

    icmp[0] = 8;
    icmp[2] = (u32)OS_GetCurrentThread();
    icmp[1] = 0;
    sequence = sPingSeq;
    soc->local_port = sequence;
    sPingSeq = sequence + 1;
    icmp[3] = sequence;
    CPS_PUT16(&icmp[1], invert_checksum(calc_checksum_do(data, len, calc_checksum_do(icmp, 8, 0))));
    send_ip((u8 *)icmp, 8, data, len, soc->remote_ip, CPS_PROTO_ICMP);
}

// send_udp
static void send_udp(u8 *data, u32 len, CPSSoc *soc) {
    u8 *buf = soc->sndbuf.data;
    u8 *udp = buf + CPS_SNDBUF_HEADER;

    CPS_PUT32(udp - 12, CPSMyIp);
    CPS_PUT32(udp - 8, soc->remote_ip);
    *(u16 *)(udp - 4) = CPS_PROTO_UDP << 8;
    CPS_PUT16(udp + 4, (u16)(len + 8));
    *(u16 *)(udp - 2) = *(u16 *)(udp + 4);
    CPS_PUT16(udp + 2, soc->remote_port);
    CPS_PUT16(buf + CPS_SNDBUF_HEADER, soc->local_port);
    *(u16 *)(udp + 6) = 0;
    CPS_PUT16(udp + 6, invert_checksum(calc_checksum_do(data, len, calc_checksum_do(udp - 12, 20, 0))));
    send_ip(udp, 8, data, len, soc->remote_ip, CPS_PROTO_UDP);
}

// send_tcp: sends a segment with as much of the data as fits, returning how much that was
static u32 send_tcp(u8 *data, u32 len, CPSSoc *soc, u8 flags) {
    u8 *tcp;
    u32 headerLen;

    if (!(flags & (CPS_TCP_FIN | CPS_TCP_RST)) && soc->state == CPS_STT_CLOSED) {
        return len;
    }
    if (OS_GetCurrentThread() == &sRecvThread) {
        tcp = sReplyBuf + CPS_SNDBUF_HEADER;
    } else {
        tcp = soc->sndbuf.data + CPS_SNDBUF_HEADER;
    }
    if (flags & CPS_TCP_SYN) {
        headerLen = 28;
    } else if ((flags & CPS_TCP_ACK) && soc->sack_permitted && soc->ooo[0].start != 0) {
        s32 last, best, i;
        u32 blocks;
        u32 enabled;
        u32 mss;

        last = -1;
        best = -1;
        blocks = 0;
        enabled = OS_DisableInterrupts();
        // The SACK blocks, the most recent run first
        for (;;) {
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
            blocks++;
            last = best;
            best = -1;
        }
        OS_RestoreInterrupts(enabled);
        CPS_PUT32(tcp + 20, 0x01010500 | (blocks * 8 + 2));
        headerLen = 24 + blocks * 8;
        mss = soc->remote_mss;
        if (mss >= sMyMss) {
            mss = sMyMss;
        }
        if (len > mss - 4 - blocks * 8) {
            len = (mss - 4 - blocks * 8) & ~1;
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
    soc->acked_window = soc->rcvbuf.size - soc->rcvbuf_len;
    CPS_PUT16(tcp + 14, soc->acked_window);
    *(u16 *)(tcp + 16) = 0;
    *(u16 *)(tcp + 18) = 0;
    if (flags & CPS_TCP_SYN) {
        CPS_PUT32(tcp + 20, 0x02040000 + sMyMss);
        *(u16 *)(tcp + 24) = 0x0101;
        *(u16 *)(tcp + 26) = 0x0204;
    }
    if (flags & CPS_TCP_ACK) {
        soc->delayed_ack = 0;
        soc->ack_time = OS_GetTick();
    }
    CPS_PUT16(tcp + 16, invert_checksum(calc_checksum_do(data, len, calc_checksum_do(tcp - 12, headerLen + 12, 0))));
    soc->seqno += len;
    if (flags & (CPS_TCP_SYN | CPS_TCP_FIN)) {
        soc->seqno++;
    }
    send_ip(tcp, headerLen, data, len, soc->remote_ip, CPS_PROTO_TCP);
    return len;
}

// reply_arp: turns a request into its reply and sends it back
static void reply_arp(CPSArpPacket *arp) {
    arp->op = 0x200;
    MI_CpuCopy(arp->src_mac, arp->dst_mac, 10);
    MI_CpuCopy(sMyMac, arp->src_mac, 6);
    CPS_PUT32(arp->src_ip, CPSMyIp);
    MI_CpuCopy(arp->dst_mac, (u8 *)arp - 14, 6);
    MI_CpuCopy(sMyMac, (u8 *)arp - 8, 6);
    send_packet((u8 *)arp - 14, 42, NULL, 0);
}

// dispatch_arp
static void dispatch_arp(CPSArpPacket *arp, u32 len) {
    u16 op;
    BOOL fromMe;
    BOOL toMe;
    CPSInAddr src;

    if (len < sizeof(CPSArpPacket)) {
        return;
    }
    if (!maccmp((u16 *)arp->src_mac, (u16 *)sMyMac)) {
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
        reg_arpcache(arp->src_mac, src, toMe);
    }
    if (op == 1 && toMe) {
        reply_arp(arp);
        return;
    }
    if (op == 2 && toMe && fromMe) {
        sIpCollision = TRUE;
    }
}

// reply_icmp: turns the request into its reply and sends it back
static void reply_icmp(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len) {
    CPSInAddr hop = get_targetip(CPS_GET32(ip->src));
    u32 sum;

    if (hop == 0) {
        return;
    }
    if (inq_arpcache(hop) == NULL) {
        send_arprequest(hop);
        return;
    }
    icmp->type = 0;
    sum = icmp->sum + 8;
    icmp->sum = sum + (sum >> 16);
    send_ip((u8 *)icmp, len, NULL, 0, CPS_GET32(ip->src), CPS_PROTO_ICMP);
}

// process_icmp_reply: gives the reply to the socket that pinged
static void process_icmp_reply(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len) {
    u32 enabled = OS_DisableInterrupts();
    OSThread *thread;

    for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
        CPSSoc *soc = thread->specific[0];

        if (soc != NULL && soc->thread != NULL && soc->state == CPS_STT_PING && (u16)(u32)soc->thread == icmp->id &&
            soc->local_port == icmp->seq && soc->rcvbuf_len == 0 && soc->remote_ip == CPS_GET32(ip->src)) {
            len -= sizeof(CPSIcmpHeader);
            if (len > soc->rcvbuf.size) {
                soc->rcvbuf_len = soc->rcvbuf.size;
            } else {
                soc->rcvbuf_len = len;
            }
            MI_CpuCopy(icmp + 1, soc->rcvbuf.data, soc->rcvbuf_len);
            if (soc->block_type == CPS_BLOCK_UDPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                OS_WakeupThreadDirect(soc->thread);
            }
            break;
        }
    }
    OS_RestoreInterrupts(enabled);
}

// valid_IP: whether both addresses are of single hosts
static BOOL valid_IP(CPSInAddr src, CPSInAddr dst) {
    if (src != 0 && !is_broadcast(src) && !is_multicast(src) && dst != 0 && !is_broadcast(dst) && !is_multicast(dst)) {
        return TRUE;
    }
    return FALSE;
}

// dispatch_icmp
static void dispatch_icmp(CPSIpHeader *ip, CPSIcmpHeader *icmp, u32 len) {
    if (len < sizeof(CPSIcmpHeader)) {
        return;
    }
    if (calc_checksum(icmp, len) != 0xffff) {
        return;
    }
    if (!valid_IP(CPS_GET32(ip->src), CPS_GET32(ip->dst))) {
        return;
    }
    switch (icmp->type) {
    case 0:
        process_icmp_reply(ip, icmp, len);
        break;
    case 8:
        reply_icmp(ip, icmp, len);
        break;
    }
}

// check_listener: the socket that listens for a SYN
static CPSSoc *check_listener(CPSIpHeader *ip, CPSTcpHeader *tcp) {
    CPSSoc *soc;
    OSThread *thread;

    for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
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

// check_socket: whether a segment belongs to a socket's connection
static BOOL check_socket(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc) {
    return soc->state != CPS_STT_DATAGRAM && soc->state != CPS_STT_PING &&
           soc->local_port == CPS_GET16(&tcp->dst_port) && soc->remote_port == CPS_GET16(&tcp->src_port) &&
           soc->remote_ip == CPS_GET32(ip->src);
}

// find_socket
static CPSSoc *find_socket(CPSIpHeader *ip, CPSTcpHeader *tcp) {
    CPSSoc *soc;
    OSThread *thread;

    for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
        soc = thread->specific[0];
        if (soc != NULL && soc->thread != NULL && check_socket(ip, tcp, soc)) {
            return soc;
        }
    }
    return NULL;
}

// parse_mss: the peer's MSS and whether it takes SACK
static void parse_mss(CPSTcpHeader *tcp, CPSSoc *soc) {
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

// no_need_inq: whether the MAC address of an address's next hop is known (or there is none to know)
static BOOL no_need_inq(CPSInAddr ip) {
    ip = get_targetip(ip);
    if (ip == 0) {
        return TRUE;
    }
    return (BOOL)inq_arpcache(ip);
}

// tcp_send_handshake: sends a segment without data. The receiving thread can't wait for ARP, so it asks instead
static void tcp_send_handshake(CPSSoc *soc, u8 flags) {
    if (no_need_inq(soc->remote_ip) || OS_GetCurrentThread() != &sRecvThread) {
        send_tcp(NULL, 0, soc, flags);
        return;
    }
    send_arprequest(get_targetip(soc->remote_ip));
}

// tcp_send_ack: acknowledges what came in, now (mode 2), or when there is something to tell (mode 1), or after a
// second segment or the ACK delay
static void tcp_send_ack(CPSSoc *soc, int mode) {
    if (mode != 2) {
        if (soc->acked == soc->ackno && soc->acked_window == (u16)(soc->rcvbuf.size - soc->rcvbuf_len)) {
            return;
        }
        if (mode != 1) {
            soc->delayed_ack++;
            if (soc->delayed_ack < 2 && (u32)OS_GetTick() - soc->ack_time < CPS_ACK_DELAY) {
                CPSi_SendDelayedAcks();
                return;
            }
        }
    }
    tcp_send_handshake(soc, CPS_TCP_ACK);
}

// tcp_send_finack
static void tcp_send_finack(CPSSoc *soc) {
    tcp_send_handshake(soc, CPS_TCP_FIN | CPS_TCP_ACK);
}

// tcp_send_rst: answers a segment that belongs to no connection
static void tcp_send_rst(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = &sReplySoc;
    u8 flags;

    MI_CpuFill8(soc, 0, sizeof(CPSSoc));
    soc->local_port = CPS_GET16(&tcp->dst_port);
    soc->remote_port = CPS_GET16(&tcp->src_port);
    soc->remote_ip = CPS_GET32(ip->src);
    flags = tcp->flags;
    if (flags & CPS_TCP_ACK) {
        soc->seqno = CPS_GET32(tcp->ack);
        tcp_send_handshake(soc, CPS_TCP_RST);
        return;
    }
    soc->seqno = 0;
    soc->ackno = len + CPS_GET32(tcp->seq);
    if (flags & (CPS_TCP_SYN | CPS_TCP_FIN)) {
        soc->ackno++;
    }
    tcp_send_handshake(soc, CPS_TCP_RST | CPS_TCP_ACK);
}

// dt_syn_listen: answers a SYN with a SYN-ACK
static void dt_syn_listen(CPSIpHeader *ip, CPSTcpHeader *tcp, CPSSoc *soc) {
    soc->state = CPS_STT_SYN_RCVD;
    soc->when = CPSi_GetTick();
    soc->local_ip_real = CPS_GET32(ip->dst);
    soc->remote_port = CPS_GET16(&tcp->src_port);
    soc->remote_ip = CPS_GET32(ip->src);
    soc->ackno = CPS_GET32(tcp->seq) + 1;
    parse_mss(tcp, soc);
    tcp_send_handshake(soc, CPS_TCP_SYN | CPS_TCP_ACK);
}

// find_specific_socket: a SYN for a connection that exists
static BOOL find_specific_socket(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = find_socket(ip, tcp);

    if (soc != NULL) {
        if (soc->state == CPS_STT_LISTEN) {
            dt_syn_listen(ip, tcp, soc);
        } else if (soc->state == CPS_STT_SYN_RCVD) {
            soc->seqno--;
            dt_syn_listen(ip, tcp, soc);
        } else {
            tcp_send_rst(ip, tcp, len);
        }
        return TRUE;
    }
    return FALSE;
}

// dt_syn
static void dt_syn(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc;

    if (!valid_IP(CPS_GET32(ip->src), CPS_GET32(ip->dst))) {
        return;
    }
    if (find_specific_socket(ip, tcp, len)) {
        return;
    }
    soc = check_listener(ip, tcp);
    if (soc != NULL) {
        dt_syn_listen(ip, tcp, soc);
        return;
    }
    // Let a thread that is about to listen get to it
    OS_YieldThread();
    soc = check_listener(ip, tcp);
    if (soc != NULL) {
        dt_syn_listen(ip, tcp, soc);
    }
}

// dt_synack
static void dt_synack(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = find_socket(ip, tcp);

    if (soc == NULL || soc->state != CPS_STT_SYN_SENT) {
        tcp_send_rst(ip, tcp, len);
        return;
    }
    OS_YieldThread();
    soc->ackno = CPS_GET32(tcp->seq) + 1;
    soc->remote_ackno = CPS_GET32(tcp->ack);
    soc->remote_win = CPS_GET16(&tcp->window);
    parse_mss(tcp, soc);
    tcp_send_ack(soc, 2);
    soc->state = CPS_STT_ESTABLISHED;
    if (soc->block_type == CPS_BLOCK_TCPCON) {
        soc->block_type = CPS_BLOCK_NONE;
        OS_WakeupThreadDirect(soc->thread);
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
    s32 i;

again:
    if (soc->ooo[0].start != 0 && soc->rcvbuf_len >= soc->ooo[0].start) {
        soc->ackno += soc->ooo[0].start + soc->ooo[0].len - soc->rcvbuf_len;
        soc->rcvbuf_len = soc->ooo[0].start + soc->ooo[0].len;
        soc->ooo[0].start = 0;
        CPSi_PackOoo(soc);
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

// dt_ack: a segment with ACK
static void dt_ack(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc;
    u8 flags;
    u32 seq;
    BOOL whole;
    u32 ack;
    u32 enabled;
    u32 offset;
    u32 start;
    u32 runEnd;
    u32 end;
    s32 i;

    offset = 0;
    soc = find_socket(ip, tcp);
    if (soc == NULL) {
        tcp_send_rst(ip, tcp, len);
        return;
    }
    flags = tcp->flags;
    ack = CPS_GET32(tcp->ack);
    if ((s32)(ack - soc->remote_ackno) > 0) {
        soc->remote_ackno = ack;
    }
    seq = CPS_GET32(tcp->seq);
    enabled = OS_DisableInterrupts();
    if (soc->state == CPS_STT_ESTABLISHED && soc->ackno != seq) {
        if ((s32)(seq - soc->ackno) < 0) {
            // Partly old: take the rest
            offset = soc->ackno - seq;
        } else {
            // Ahead of what is expected: keep it as a run
            if (!(flags & CPS_TCP_FIN)) {
                start = soc->rcvbuf_len + seq - soc->ackno;
                if (soc->rcvbuf_len < start && start < soc->rcvbuf.size) {
                    if (len > soc->rcvbuf.size - start) {
                        len = soc->rcvbuf.size - start;
                    }
                    if (len != 0) {
                        for (i = 0; i < 4; i++) {
                            end = start + len;
                            for (;;) {
                                if (soc->ooo[i].start == 0) {
                                    MI_CpuCopy(CPS_TCP_DATA(tcp), soc->rcvbuf.data + start, len);
                                    soc->ooo[i].start = start;
                                    soc->ooo[i].len = len;
                                    CPSi_MarkOoo(soc, i);
                                    CPSi_MergeOoo(soc);
                                    goto done;
                                }
                                runEnd = soc->ooo[i].start + soc->ooo[i].len;
                                if (start < soc->ooo[i].start && soc->ooo[i].start <= end) {
                                    MI_CpuCopy(CPS_TCP_DATA(tcp), soc->rcvbuf.data + start, soc->ooo[i].start - start);
                                    soc->ooo[i].len += soc->ooo[i].start - start;
                                    soc->ooo[i].start = start;
                                    CPSi_MarkOoo(soc, i);
                                    CPSi_MergeOoo(soc);
                                    goto done;
                                }
                                if (start <= runEnd && runEnd < end) {
                                    MI_CpuCopy(CPS_TCP_DATA(tcp) + runEnd - start, soc->rcvbuf.data + runEnd,
                                               end - runEnd);
                                    soc->ooo[i].len += end - runEnd;
                                    CPSi_MergeOoo(soc);
                                    CPSi_MarkOoo(soc, i);
                                    goto done;
                                }
                                if (soc->ooo[i].start <= start && end <= runEnd) {
                                    goto done;
                                }
                                if (start >= soc->ooo[i].start) {
                                    break;
                                }
                                CPSi_InsertOoo(soc, i);
                            }
                        }
                    }
                }
            }
        done:
            OS_RestoreInterrupts(enabled);
            tcp_send_ack(soc, 2);
            return;
        }
    }
    if (offset > len) {
        // BUG: Returns with the interrupts still disabled
#ifdef BUGFIX
        OS_RestoreInterrupts(enabled);
#endif
        return;
    }
    len -= offset;
    soc->remote_win = CPS_GET16(&tcp->window);
    switch (soc->state) {
    case CPS_STT_CLOSED:
    case CPS_STT_SYN_SENT:
        OS_RestoreInterrupts(enabled);
        tcp_send_rst(ip, tcp, len);
        break;
    case CPS_STT_SYN_RCVD:
        soc->state = CPS_STT_ESTABLISHED;
        if (soc->block_type == CPS_BLOCK_TCPCON) {
            soc->block_type = CPS_BLOCK_NONE;
            OS_WakeupThreadDirect(soc->thread);
        }
        if (len == 0) {
            OS_RestoreInterrupts(enabled);
            break;
        }
        // fallthrough
    case CPS_STT_ESTABLISHED:
        soc->segments++;
        if (len > soc->rcvbuf.size - soc->rcvbuf_len) {
            len = soc->rcvbuf.size - soc->rcvbuf_len;
            whole = FALSE;
        } else {
            whole = TRUE;
        }
        if (len != 0) {
            if (soc->ooo[0].start != 0 && soc->ooo[0].start <= soc->rcvbuf_len + len) {
                if (!(flags & CPS_TCP_FIN)) {
                    MI_CpuCopy(CPS_TCP_DATA(tcp) + offset, soc->rcvbuf.data + soc->rcvbuf_len,
                               soc->ooo[0].start - soc->rcvbuf_len);
                    len = soc->ooo[0].start + soc->ooo[0].len - soc->rcvbuf_len;
                } else {
                    MI_CpuCopy(CPS_TCP_DATA(tcp) + offset, soc->rcvbuf.data + soc->rcvbuf_len, len);
                }
                soc->ooo[0].start = 0;
                CPSi_PackOoo(soc);
            } else {
                MI_CpuCopy(CPS_TCP_DATA(tcp) + offset, soc->rcvbuf.data + soc->rcvbuf_len, len);
            }
            soc->rcvbuf_len += len;
            soc->ackno += len;
            CPSi_MergeOoo(soc);
            if (soc->block_type == CPS_BLOCK_TCPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                OS_WakeupThreadDirect(soc->thread);
            }
        }
        if (whole && (flags & CPS_TCP_FIN)) {
            soc->ackno++;
            soc->state = CPS_STT_LAST_ACK;
            OS_RestoreInterrupts(enabled);
            tcp_send_finack(soc);
            if (len == 0 && soc->block_type == CPS_BLOCK_TCPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                OS_WakeupThreadDirect(soc->thread);
            }
        } else {
            OS_RestoreInterrupts(enabled);
            if (len != 0) {
                tcp_send_ack(soc, 0);
            }
        }
        break;
    case CPS_STT_FIN_WAIT1:
    case CPS_STT_FIN_WAIT2:
        if (flags & CPS_TCP_FIN) {
            soc->ackno += len + 1;
            OS_RestoreInterrupts(enabled);
            tcp_send_ack(soc, 2);
            soc->state = CPS_STT_CLOSED;
            if (soc->block_type == CPS_BLOCK_TCPREAD) {
                soc->block_type = CPS_BLOCK_NONE;
                OS_WakeupThreadDirect(soc->thread);
            }
        } else {
            soc->state = CPS_STT_FIN_WAIT2;
            OS_RestoreInterrupts(enabled);
            if (len != 0) {
                soc->ackno += len;
                tcp_send_ack(soc, 2);
            }
        }
        break;
    case CPS_STT_LAST_ACK:
    case CPS_STT_CLOSING:
        soc->state = CPS_STT_CLOSED;
        OS_RestoreInterrupts(enabled);
        if (soc->block_type == CPS_BLOCK_TCPREAD) {
            soc->block_type = CPS_BLOCK_NONE;
            OS_WakeupThreadDirect(soc->thread);
        }
        break;
    default:
        OS_RestoreInterrupts(enabled);
        if (flags & CPS_TCP_FIN) {
            soc->ackno++;
        }
        tcp_send_ack(soc, 2);
        break;
    }
    OS_YieldThread();
}

// dt_fin: a FIN without ACK
static void dt_fin(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    CPSSoc *soc = find_socket(ip, tcp);
    u32 enabled;

    if (soc == NULL) {
        return;
    }
    enabled = OS_DisableInterrupts();
    switch (soc->state) {
    case CPS_STT_FIN_WAIT1:
        soc->ackno++;
        soc->state = CPS_STT_CLOSING;
        OS_RestoreInterrupts(enabled);
        tcp_send_ack(soc, 2);
        break;
    case CPS_STT_FIN_WAIT2:
        soc->ackno++;
        OS_RestoreInterrupts(enabled);
        tcp_send_ack(soc, 2);
        soc->state = CPS_STT_CLOSED;
        if (soc->block_type == CPS_BLOCK_TCPREAD) {
            soc->block_type = CPS_BLOCK_NONE;
            OS_WakeupThreadDirect(soc->thread);
        }
        break;
    case CPS_STT_ESTABLISHED:
        soc->ackno++;
        soc->state = CPS_STT_LAST_ACK;
        OS_RestoreInterrupts(enabled);
        tcp_send_finack(soc);
        break;
    default:
        OS_RestoreInterrupts(enabled);
        tcp_send_rst(ip, tcp, len);
        break;
    }
}

// dt_rst
static void dt_rst(CPSIpHeader *ip, CPSTcpHeader *tcp) {
    CPSSoc *soc = find_socket(ip, tcp);

    if (soc == NULL) {
        return;
    }
    OS_YieldThread();
    soc->state = CPS_STT_CLOSED;
    if (soc->block_type == CPS_BLOCK_TCPCON || soc->block_type == CPS_BLOCK_TCPREAD) {
        soc->block_type = CPS_BLOCK_NONE;
        OS_WakeupThreadDirect(soc->thread);
    }
}

// dispatch_tcp
static void dispatch_tcp(CPSIpHeader *ip, CPSTcpHeader *tcp, u32 len) {
    u8 flags;

    if (len < sizeof(CPSTcpHeader)) {
        return;
    }
    if ((tcp->offset & 0xf0) < 0x50 || len < CPS_TCP_HEADER_LEN(tcp)) {
        return;
    }
    if (check_tcpudpsum(tcp, len, ip, CPS_PROTO_TCP)) {
        return;
    }
    len -= CPS_TCP_HEADER_LEN(tcp);
    flags = tcp->flags;
    switch (flags & (CPS_TCP_FIN | CPS_TCP_SYN | CPS_TCP_RST | CPS_TCP_ACK)) {
    case CPS_TCP_SYN:
        if (!(flags & (CPS_TCP_URG | CPS_TCP_PSH))) {
            dt_syn(ip, tcp, len);
        }
        break;
    case CPS_TCP_SYN | CPS_TCP_ACK:
        if (!(flags & (CPS_TCP_URG | CPS_TCP_PSH))) {
            dt_synack(ip, tcp, len);
        }
        break;
    case CPS_TCP_ACK:
    case CPS_TCP_ACK | CPS_TCP_FIN:
        dt_ack(ip, tcp, len);
        break;
    case CPS_TCP_FIN:
        dt_fin(ip, tcp, len);
        break;
    default:
        if (flags & CPS_TCP_RST) {
            dt_rst(ip, tcp);
        } else {
            tcp_send_rst(ip, tcp, len);
        }
        break;
    }
}

// dispatch_udp: gives a datagram to the socket bound to its port, when that has taken the last one
static void dispatch_udp(CPSIpHeader *ip, CPSUdpHeader *udp, u32 len) {
    u32 enabled;
    OSThread *thread;

    if (len < sizeof(CPSUdpHeader)) {
        return;
    }
    if (udp->sum != 0 && check_tcpudpsum(udp, len, ip, CPS_PROTO_UDP)) {
        return;
    }
    enabled = OS_DisableInterrupts();
    for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
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
                if (len > soc->rcvbuf.size) {
                    soc->rcvbuf_len = soc->rcvbuf.size;
                } else {
                    soc->rcvbuf_len = len;
                }
                MI_CpuCopy(udp + 1, soc->rcvbuf.data, soc->rcvbuf_len);
                if (soc->block_type == CPS_BLOCK_UDPREAD) {
                    soc->block_type = CPS_BLOCK_NONE;
                    OS_WakeupThreadDirect(soc->thread);
                } else if (soc->udp_callback != NULL && soc->udp_callback(soc->rcvbuf.data, soc->rcvbuf_len, soc)) {
                    soc->rcvbuf_len = 0;
                }
            }
            break;
        }
    }
    OS_RestoreInterrupts(enabled);
}

// check_frag: the whole packet once its fragments are all in, which allocated says to free, or NULL
static CPSIpHeader *check_frag(CPSIpHeader *ip, BOOL *allocated) {
    CPSFragTable *free = NULL;
    CPSFragTable *reassembly;
    u32 frag;
    u32 len;
    u32 offset;
    u32 headerLen;
    u32 total;
    u16 id;
    CPSInAddr src;
    u32 i;
    u32 end;
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
        free->buf = CPSiAlloc(CPS_REASSEMBLY_SIZE + sizeof(CPSEtherHeader) + headerLen);
        if (free->buf == NULL) {
            return NULL;
        }
        free->src = src;
        free->id = id;
        free->last = 0;
        free->time = CPSi_GetTick();
        free->data = free->buf + sizeof(CPSEtherHeader) + headerLen;
        MI_CpuCopy(ip, free->buf + sizeof(CPSEtherHeader), headerLen);
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
    MI_CpuCopy((u8 *)ip + headerLen, reassembly->data + (offset << 3), len);
    if (reassembly->last == 0) {
        return NULL;
    }
    offset = 0;
    for (i = 0; i < reassembly->count;) {
        if (reassembly->start[i] <= offset && offset < reassembly->end[i]) {
            offset = reassembly->end[i];
            i = 0;
        } else {
            i++;
        }
    }
    if (offset < reassembly->last) {
        return NULL;
    }
    whole = (CPSIpHeader *)(reassembly->buf + sizeof(CPSEtherHeader));
    CPS_PUT16(&whole->len, (u16)(reassembly->len + ((whole->ver_len & 0xf) << 2)));
    reassembly->count = 0;
    *allocated = TRUE;
    return whole;
}

// dispatch_ip
static void dispatch_ip(CPSIpHeader *ip, u32 len) {
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
        if (!ip_isme(CPS_GET32(ip->dst))) {
            return;
        }
        if (calc_checksum(ip, (ip->ver_len & 0xf) << 2) != 0xffff) {
            return;
        }
        if (CPSMyIp == CPS_GET32(ip->dst)) {
            reg_arpcache((u8 *)ip - 8, CPS_GET32(ip->src), TRUE);
        }
    }
    ip = check_frag(ip, &allocated);
    if (ip == NULL) {
        return;
    }
    headerLen = (ip->ver_len & 0xf) << 2;
    dataLen = CPS_GET16(&ip->len) - headerLen;
    proto = ip->proto;
    if (proto == CPS_PROTO_UDP) {
        dispatch_udp(ip, (CPSUdpHeader *)((u8 *)ip + headerLen), dataLen);
    } else if (CPSMyIp != 0) {
        if (proto == CPS_PROTO_ICMP) {
            dispatch_icmp(ip, (CPSIcmpHeader *)((u8 *)ip + headerLen), dataLen);
        } else if (proto == CPS_PROTO_TCP) {
            dispatch_tcp(ip, (CPSTcpHeader *)((u8 *)ip + headerLen), dataLen);
        }
    }
    if (allocated) {
        CPSiFree((u8 *)ip - sizeof(CPSEtherHeader));
    }
}

// tcpip
static void tcpip(void *arg) {
    for (;;) {
        u32 len;
        CPSEtherHeader *frame = receive_packet(&len);

        if (len > CPS_SNDBUF_HEADER) {
            switch (CPS_GET16(&frame->type)) {
            case CPS_ETHERTYPE_IP:
                dispatch_ip((CPSIpHeader *)(frame + 1), len - sizeof(CPSEtherHeader));
                break;
            case CPS_ETHERTYPE_ARP:
                dispatch_arp((CPSArpPacket *)(frame + 1), len - sizeof(CPSEtherHeader));
                break;
            }
        }
        throw_packet();
    }
}

// CPS_SocGetEport
u16 CPS_SocGetEport(void) {
    BOOL used;

    do {
        OSThread *thread;

        used = FALSE;
        sLastPort++;
        if (sLastPort < 1024 || sLastPort >= 5000) {
            sLastPort = 1024;
        }
        for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
            CPSSoc *soc = thread->specific[0];

            if (soc != NULL && soc->thread != NULL && soc->local_port == sLastPort) {
                used = TRUE;
                break;
            }
        }
    } while (used);
    return sLastPort;
}

// get_seqno
static u32 get_seqno(void) {
    return MATH_Rand32(&CPSiRand32ctx, 0);
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
            soc->local_port = CPS_SocGetEport();
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
        MI_CpuFill8(soc->ooo, 0, sizeof(soc->ooo));
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

// CPSi_TcpConnectRaw: sends the SYN up to three times, returning 0 once connected
u32 CPSi_TcpConnectRaw(CPSSoc *soc) {
    u32 seqno = get_seqno();
    u32 i;

    for (i = 0; i < 3; i++) {
        u32 enabled;

        soc->seqno = seqno;
        soc->state = CPS_STT_SYN_SENT;
        soc->when = CPSi_GetTick();
        tcp_send_handshake(soc, CPS_TCP_SYN);
        enabled = OS_DisableInterrupts();
        if (soc->state == CPS_STT_SYN_SENT && CPSMyIp != 0) {
            soc->block_type = CPS_BLOCK_TCPCON;
            OS_SleepThread(NULL);
        }
        OS_RestoreInterrupts(enabled);
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
u32 CPS_TcpConnect(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->ssl) {
            return CPSi_SslConnect(soc);
        }
        return CPSi_TcpConnectRaw(soc);
    }
    return 1;
}

// CPSi_TcpShutdownRaw
static void CPSi_TcpShutdownRaw(CPSSoc *soc) {
    u32 enabled;

    OS_YieldThread();
    enabled = OS_DisableInterrupts();
    if (soc->state == CPS_STT_SYN_RCVD || soc->state == CPS_STT_ESTABLISHED) {
        soc->state = CPS_STT_FIN_WAIT1;
        OS_RestoreInterrupts(enabled);
        tcp_send_finack(soc);
        return;
    }
    if (soc->state != CPS_STT_CLOSED) {
        OS_RestoreInterrupts(enabled);
        tcp_send_ack(soc, 2);
        return;
    }
    OS_RestoreInterrupts(enabled);
}

// CPS_TcpShutdown
void CPS_TcpShutdown(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->ssl) {
            CPSi_SslShutdown(soc);
        }
        CPSi_TcpShutdownRaw(soc);
    }
}

// CPS_TcpClose: waits up to about five seconds for the connection to close
void CPS_TcpClose(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();
    u32 start;

    if (soc == NULL) {
        return;
    }
    if (soc->ssl) {
        CPSi_SslClose(soc);
    }
    start = CPSi_GetTick();
    while (sLinkIsOn() && soc->state != CPS_STT_CLOSED && (int)(CPSi_GetTick() - start) < 39) {
        yield_thread();
    }
    soc->state = CPS_STT_CLOSED;
}

// udp_read_raw
static u8 *udp_read_raw(u32 *len, CPSSoc *soc) {
    u32 enabled = OS_DisableInterrupts();
    u32 received = soc->rcvbuf_len;

    while (received == 0) {
        soc->block_type = CPS_BLOCK_UDPREAD;
        OS_SleepThread(NULL);
        received = soc->rcvbuf_len;
    }
    OS_RestoreInterrupts(enabled);
    *len = received;
    return soc->rcvbuf.data;
}

// CPSi_TcpReadRaw
u8 *CPSi_TcpReadRaw(u32 *len, CPSSoc *soc) {
    if (soc->state == CPS_STT_ESTABLISHED) {
        tcp_send_ack(soc, 1);
    }
    if (soc->rcvbuf_len == 0 && soc->state == CPS_STT_ESTABLISHED) {
        u32 enabled = OS_DisableInterrupts();

        while (soc->rcvbuf_len == 0 && soc->state == CPS_STT_ESTABLISHED) {
            soc->block_type = CPS_BLOCK_TCPREAD;
            OS_SleepThread(NULL);
        }
        OS_RestoreInterrupts(enabled);
    } else {
        OS_YieldThread();
    }
    if ((*len = soc->rcvbuf_len) != 0) {
        return soc->rcvbuf.data;
    }
    return NULL;
}

// CPS_SocRead
u8 *CPS_SocRead(u32 *len) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->state == CPS_STT_DATAGRAM || soc->state == CPS_STT_PING) {
            return udp_read_raw(len, soc);
        }
        if (soc->ssl) {
            return CPSi_SslRead(len, soc);
        }
        return CPSi_TcpReadRaw(len, soc);
    }
    *len = 0;
    return NULL;
}

// CPSi_SocConsumeRaw: drops data from the front of the receive buffer, moving the runs after it along
void CPSi_SocConsumeRaw(u32 len, CPSSoc *soc) {
    u32 enabled = OS_DisableInterrupts();
    u32 received = soc->rcvbuf_len;
    BOOL wasFull = FALSE;
    u32 start;

    if (received == soc->rcvbuf.size && len != 0) {
        wasFull = TRUE;
    }
    if (len >= received) {
        soc->rcvbuf_len = 0;
    } else {
        soc->rcvbuf_len = received - len;
        MI_CpuMove(soc->rcvbuf.data + len, soc->rcvbuf.data, received - len);
    }
    start = soc->ooo[0].start;
    if (start != 0) {
        u32 consumed = received - soc->rcvbuf_len;

        if (consumed != 0) {
            int i;

            for (i = 0; i < 4 && soc->ooo[i].start != 0; i++) {
            }
            if (start > consumed) {
                u8 *runs = soc->rcvbuf.data + start;

                MI_CpuMove(runs, runs - consumed, soc->ooo[i - 1].start + soc->ooo[i - 1].len - start);
                for (i = 0; i < 4 && soc->ooo[i].start != 0; i++) {
                    soc->ooo[i].start -= consumed;
                }
            } else {
                MI_CpuFill8(soc->ooo, 0, sizeof(soc->ooo));
                OS_Terminate();
            }
        }
    }
    OS_RestoreInterrupts(enabled);
    if (soc->state != CPS_STT_DATAGRAM && soc->state != CPS_STT_PING) {
        if (soc->rcvbuf_len == 0 || wasFull) {
            tcp_send_ack(soc, 0);
        }
    }
}

// CPS_SocConsume
void CPS_SocConsume(u32 len) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->ssl) {
            CPSi_SslConsume(len, soc);
            return;
        }
        CPSi_SocConsumeRaw(len, soc);
    }
}

// tcp_write_do: sends data in segments while the window lasts, or a byte to probe a closed window
static u32 tcp_write_do(u8 *data, u32 len, CPSSoc *soc, BOOL probe) {
    u32 sent;
    u32 window;
    u32 segments;
    u32 budget;

    if (probe) {
        window = 1;
    } else {
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
        sent = send_tcp(data, sent, soc, CPS_TCP_PSH | CPS_TCP_ACK);
        OS_YieldThread();
        window -= sent;
        data += sent;
        len -= sent;
    }
    return sent;
}

// tcp_write_do2
static u32 tcp_write_do2(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc, BOOL probe) {
    u32 sent = tcp_write_do(data, len, soc, probe);

    if (sent != 0 && len2 != 0) {
        sent = tcp_write_do(data2, len2, soc, FALSE);
    }
    return sent;
}

// CPSi_TcpWrite2Raw: sends both pieces of data and waits for their ACKs, returning how much of it was acknowledged
u32 CPSi_TcpWrite2Raw(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc) {
    u32 total = 0;
    BOOL probe;
    u32 base;
    u32 lastProgress;

    soc->segments = 0;
    probe = FALSE;
    lastProgress = CPSi_GetTick();
    while (sLinkIsOn() && len != 0 && soc->state == CPS_STT_ESTABLISHED && (int)(CPSi_GetTick() - lastProgress) < 159) {
        u32 start;
        u32 acked;

        base = soc->seqno;
        tcp_write_do2(data, len, data2, len2, soc, probe);
        start = CPSi_GetTick();
        do {
            yield_thread();
        } while (sLinkIsOn() && soc->state == CPS_STT_ESTABLISHED && soc->seqno != soc->remote_ackno &&
                 (int)(CPSi_GetTick() - start) < 15 && (!probe || soc->remote_win == 0));
        acked = soc->remote_ackno - base;
        if (acked > soc->seqno - base) {
            acked = 0;
        }
        total += acked;
        if (acked != 0) {
            lastProgress = CPSi_GetTick();
        }
        soc->seqno = soc->remote_ackno;
        if (soc->state == CPS_STT_ESTABLISHED && soc->remote_win == 0 && acked == 0) {
            if (!probe) {
                start = CPSi_GetTick();
                while (sLinkIsOn() && (int)(CPSi_GetTick() - start) < 15) {
                    yield_thread();
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

// CPSi_SocWrite2: sends both pieces of data the way the socket sends, returning how much went, or 0 on an error
static u32 CPSi_SocWrite2(u8 *data, u32 len, u8 *data2, u32 len2) {
    CPSSoc *soc = CPS_CURRENT_SOC();
    u32 sent;

    if (soc != NULL) {
        if (soc->state == CPS_STT_DATAGRAM) {
            if (len != 0) {
                send_udp(data, len, soc);
            }
            if (len2 != 0) {
                send_udp(data2, len2, soc);
            }
            sent = len + len2;
        } else if (soc->state == CPS_STT_PING) {
            if (len != 0) {
                send_ping(data, len, soc);
            }
            if (len2 != 0) {
                send_ping(data2, len2, soc);
            }
            sent = len + len2;
        } else if (soc->ssl) {
            sent = CPSi_SslWrite2(data, len, data2, len2, soc);
        } else {
            sent = CPSi_TcpWrite2Raw(data, len, data2, len2, soc);
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
            u32 sent = CPSi_SocWrite2(soc->outbuf.data, pending, data, len);

            if (sent < soc->outbuf_len) {
                MI_CpuMove(soc->outbuf.data + sent, soc->outbuf.data, soc->outbuf_len - sent);
                soc->outbuf_len -= sent;
                return 0;
            }
            sent -= soc->outbuf_len;
            soc->outbuf_len = 0;
            return sent;
        }
        return CPSi_SocWrite2(data, len, NULL, 0);
    }
    return 0;
}

// CPS_SocGetLength
s32 CPS_SocGetLength(void) {
    CPSSoc *soc = CPS_CURRENT_SOC();

    if (soc != NULL) {
        if (soc->state == CPS_STT_ESTABLISHED) {
            tcp_send_ack(soc, 1);
        }
        if (soc->ssl) {
            return CPSi_SslGetLength(soc);
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
        CPSi_SocWrite2(soc->outbuf.data, soc->outbuf_len, NULL, 0);
        soc->outbuf_len = 0;
    }
}

// CPSi_CheckCollision: asks twice who has the address and waits for an answer, FALSE when another host has it
static BOOL CPSi_CheckCollision(void) {
    u32 start;

    send_arprequest(CPSMyIp);
    OS_Sleep(100);
    send_arprequest(CPSMyIp);
    start = CPSi_GetTick();
    while (sLinkIsOn() && (int)(CPSi_GetTick() - start) < 23) {
        if (sIpCollision) {
            return FALSE;
        }
        OS_Sleep(100);
    }
    return TRUE;
}

// set_fixed_ip: tells the DHCP callback, and checks a static address
static void set_fixed_ip(void) {
    sDhcpCallback();
    if (CPSMyIp != 0 && !CPSi_CheckCollision()) {
        reset_network_vars(CPS_NOIP_REASON_COLLISION);
    }
}

// Whether the address check callback rejects the address DHCP gave
static inline BOOL CPSi_AddressRejected(void) {
    return sAddressCheck != NULL ? sAddressCheck() == 0 : FALSE;
}

// scavenger: once a second, runs DHCP and drops what timed out
static void scavenger(void *arg) {
    int state;
    u32 arpTimer;
    u32 leaseTimer;
    BOOL first;
    u32 now;
    int i;
    OSThread *thread;

    sCalmDown = FALSE;
    state = DHCP_STATE_INIT;
    MI_CpuFill8(&sDhcpSoc, 0, sizeof(CPSSoc));
    sDhcpSoc.rcvbuf.size = sizeof(sDhcpRcvBuf);
    sDhcpSoc.rcvbuf.data = sDhcpRcvBuf;
    sDhcpSoc.sndbuf.size = sizeof(sDhcpSndBuf);
    sDhcpSoc.sndbuf.data = sDhcpSndBuf;
    CPS_SocRegister(&sDhcpSoc);
    arpTimer = 1;
    leaseTimer = 1;
    CPSNoIpReason = CPS_NOIP_REASON_LINKOFF;
    first = TRUE;
    for (;;) {
        OS_Sleep(1000);
        if (sCalmDown) {
            break;
        }
        now = CPSi_GetTick();
        if (sLinkIsOn()) {
            if (--leaseTimer == 0) {
                if (CPSMode & CPS_DONOTUSE_DHCP) {
                    if (state == DHCP_STATE_INIT) {
                        set_fixed_ip();
                        state = DHCP_STATE_BOUND;
                    }
                } else {
                    switch (state) {
                    case DHCP_STATE_INIT:
                        if (first) {
                            CPSNoIpReason = CPS_NOIP_REASON_DHCPDISCOVERY;
                            first = FALSE;
                        }
                        if (!dhcp_discover_server() || !dhcp_request_server(&leaseTimer, 0)) {
                            set_fixed_ip();
                            state = DHCP_STATE_DONE;
                        } else {
                            if (CPSi_AddressRejected()) {
                                reset_network_vars(CPS_NOIP_REASON_REJECTED);
                                set_fixed_ip();
                                state = DHCP_STATE_DONE;
                            } else {
                                state = DHCP_STATE_BOUND;
                                if (!CPSi_CheckCollision()) {
                                    reset_network_vars(CPS_NOIP_REASON_COLLISION);
                                    state = DHCP_STATE_DONE;
                                }
                            }
                        }
                        break;
                    case DHCP_STATE_BOUND:
                        if (!dhcp_request_server(&leaseTimer, 1) && leaseTimer < 60) {
                            state = DHCP_STATE_REBINDING;
                        } else if (CPSi_AddressRejected()) {
                            reset_network_vars(CPS_NOIP_REASON_REJECTED);
                            set_fixed_ip();
                            state = DHCP_STATE_DONE;
                        } else if (!CPSi_CheckCollision()) {
                            reset_network_vars(CPS_NOIP_REASON_COLLISION);
                            state = DHCP_STATE_DONE;
                        }
                        break;
                    case DHCP_STATE_REBINDING:
                        if (dhcp_request_server(&leaseTimer, 2)) {
                            if (CPSi_AddressRejected()) {
                                reset_network_vars(CPS_NOIP_REASON_REJECTED);
                                set_fixed_ip();
                                state = DHCP_STATE_DONE;
                            } else {
                                state = DHCP_STATE_BOUND;
                                if (!CPSi_CheckCollision()) {
                                    reset_network_vars(CPS_NOIP_REASON_COLLISION);
                                    state = DHCP_STATE_DONE;
                                }
                            }
                        } else if (leaseTimer < 60) {
                            reset_network_vars(CPS_NOIP_REASON_LEASETIMEOUT);
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
            reset_network_vars(CPS_NOIP_REASON_LINKOFF);
            state = DHCP_STATE_INIT;
            leaseTimer = arpTimer;
        }
        for (i = 0; i < CPS_ARP_CACHE_SIZE; i++) {
            if (sArpCache[i].ip != 0 && (s16)(now - sArpCache[i].time) > 957) {
                sArpCache[i].ip = 0;
            }
        }
        if (CPSGatewayIp != 0 && --arpTimer == 0) {
            send_arprequest(CPSGatewayIp);
            arpTimer = 105;
        }
        for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
            CPSSoc *soc = thread->specific[0];

            if (soc != NULL && soc->thread != NULL) {
                if (soc->state == CPS_STT_SYN_RCVD && (int)(now - soc->when) > 39) {
                    soc->state = CPS_STT_LISTEN;
                    soc->remote_port = soc->remote_port_bound;
                    soc->remote_ip = soc->remote_ip_bound;
                } else if (soc->state == CPS_STT_SYN_SENT && (int)(now - soc->when) > 39) {
                    if (soc->block_type == CPS_BLOCK_TCPCON) {
                        soc->state = CPS_STT_CLOSED;
                        soc->block_type = CPS_BLOCK_NONE;
                        OS_WakeupThreadDirect(soc->thread);
                    }
                } else if (soc->state != CPS_STT_ESTABLISHED && soc->block_type == CPS_BLOCK_TCPREAD) {
                    soc->block_type = CPS_BLOCK_NONE;
                    OS_WakeupThreadDirect(soc->thread);
                }
            }
        }
        for (i = 0; i < CPS_REASSEMBLY_COUNT; i++) {
            CPSFragTable *reassembly = &sReassembly[i];

            if (reassembly->count != 0 && (int)(now - reassembly->time) > 239) {
                CPSiFree(reassembly->buf);
                reassembly->count = 0;
            }
        }
        CPSi_SslPeriodical(now);
        if (sScavengerCallback != NULL) {
            sScavengerCallback();
        }
    }
    if (!(CPSMode & CPS_DONOTUSE_DHCP) && state != DHCP_STATE_DONE) {
        dhcp_release_server();
    }
    CPS_SocUnRegister();
}

// CPSi_SendDelayedAcks: sends the ACKs that waited long enough, and sets the alarm for the next one
static void CPSi_SendDelayedAcks(void) {
    u32 wait = 0xffffffff;
    u32 enabled = OS_DisableInterrupts();
    u32 now = OS_GetTick();
    OSThread *thread;

    for (thread = OSi_ThreadInfo.list; thread != NULL; thread = thread->next) {
        CPSSoc *soc = thread->specific[0];

        if (soc != NULL && soc->thread != NULL && soc->state == CPS_STT_ESTABLISHED && soc->delayed_ack != 0) {
            u32 waited = now - soc->ack_time;

            if (waited >= CPS_ACK_DELAY) {
                tcp_send_ack(soc, 1);
            } else if (wait > CPS_ACK_DELAY - waited) {
                wait = (u32)CPS_ACK_DELAY - waited;
            }
        }
    }
    if (wait != 0xffffffff) {
        OS_CancelAlarm(&sAckAlarm);
        OS_SetAlarm(&sAckAlarm, wait, CPSi_AckAlarm, NULL);
    }
    OS_RestoreInterrupts(enabled);
}

// CPSi_AckThread
static void CPSi_AckThread(void *arg) {
    while (!OS_IsThreadTerminated(&sScavengerThread)) {
        CPSi_SendDelayedAcks();
        OS_SleepThread(NULL);
    }
}

// CPSi_AckAlarm
static void CPSi_AckAlarm(void *arg) {
    OS_WakeupThreadDirect(&sAckThread);
}

// dhcp_setcommon: the BOOTP header and the options every message has, returning where the rest go
static u8 *dhcp_setcommon(u8 *buf, u8 type) {
    MI_CpuFill8(buf, 0, 0xec);
    *(u16 *)buf = 0x101;
    buf[2] = 6;
    CPS_PUT32(buf + 4, sDhcpXid);
    CPS_PUT32(buf + 12, CPSMyIp);
    MI_CpuCopy(sMyMac, buf + 28, 6);
    *(u16 *)(buf + 0xec) = 0x8263;
    *(u16 *)(buf + 0xee) = 0x6353;
    *(u16 *)(buf + 0xf0) = 0x135;
    buf += 0xf2;
    *buf++ = type;
    *buf++ = 61;
    *buf++ = 7;
    *buf++ = 1;
    MI_CpuCopy(sMyMac, buf, 6);
    buf += 6;
    if (type != DHCP_RELEASE) {
        *buf++ = 12;
        *buf++ = 10;
        MI_CpuCopy("NintendoDS", buf, 10);
        buf += 10;
        *buf++ = 55;
        *buf++ = 3;
        *buf++ = 1;
        *buf++ = 3;
        *buf++ = 6;
    }
    return buf;
}

// pad_mem: fills the rest of the first size bytes, of which used are taken
static u8 *pad_mem(u8 value, u32 size, u8 *p, u32 used) {
    if (used < size) {
        u32 rest = size - used;

        MI_CpuFill8(p, value, rest);
        p += rest;
    }
    return p;
}

#define DHCP_BUF (sDhcpSndBuf + CPS_SNDBUF_HEADER + sizeof(CPSUdpHeader))

// dhcp_send_discover
static u32 dhcp_send_discover(void) {
    u8 *buf = DHCP_BUF;
    u8 *p;

    sDhcpXid = MATH_Rand32(&CPSiRand32ctx, 0);
    p = dhcp_setcommon(buf, DHCP_DISCOVER);
    if (sRequestedIp != 0) {
        p[0] = 50;
        p[1] = 4;
        CPS_PUTBE32(p + 2, sRequestedIp);
        p += 6;
    }
    *p++ = 0xff;
    CPS_SocWrite(buf, pad_mem(0, DHCP_MESSAGE_SIZE, p, p - buf) - buf);
    return sDhcpXid;
}

// dhcp_send_request: after an offer (mode 0), or to renew (1) or rebind (2) the lease
static u32 dhcp_send_request(int mode) {
    u8 *buf = DHCP_BUF;
    u8 *p = dhcp_setcommon(buf, DHCP_REQUEST);

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
    CPS_SocWrite(buf, pad_mem(0, DHCP_MESSAGE_SIZE, p, p - buf) - buf);
    return sDhcpXid;
}

// dhcp_analyze_response: waits for the answer to a message and takes its options
static int dhcp_analyze_response(u32 xid, int try) {
    u8 *msg;
    u8 *end;
    u32 len;
    u8 code;
    u32 start;
    s32 timeout = sDhcpWaits[try] * 7;
    int result;

    start = CPSi_GetTick();
    result = DHCP_WAIT_NONE;

    while (sLinkIsOn() && result == DHCP_WAIT_NONE && (int)(CPSi_GetTick() - start) < timeout) {
        if (CPS_SocGetLength() == 0) {
            yield_thread();
        } else {
            msg = CPS_SocRead(&len);
            if (len > 0xf0 && msg[0] == 2 && xid == CPS_GET32(msg + 4) && !maccmp((u16 *)(msg + 28), (u16 *)sMyMac)) {
                CPSInAddr yourIp;

                result = DHCP_WAIT_OTHER;
                yourIp = CPS_BE32(msg + 16);
                end = msg + len;
                msg += 0xec;
                if (*msg++ == 0x63 && *msg++ == 0x82 && *msg++ == 0x53 && *msg++ == 0x63) {

                    while (msg < end && (code = *msg++) != 0xff) {
                        if (code != 0) {
                            switch (code) {
                            case 1:
                                CPSNetMask = CPS_BE32(msg + 1);
                                break;
                            case 3:
                                CPSGatewayIp = CPS_BE32(msg + 1);
                                break;
                            case 6:
                                if (!(CPSMode & CPS_MODE_STATIC_DNS)) {
                                    if (msg[0] < 8) {
                                        CPSDnsIp[1] = 0;
                                    } else {
                                        CPSDnsIp[1] = CPS_BE32(msg + 5);
                                    }
                                    CPSDnsIp[0] = CPS_BE32(msg + 1);
                                }
                                break;
                            case 51:
                                sLeaseTime = CPS_BE32(msg + 1);
                                break;
                            case 53:
                                switch (msg[1]) {
                                case DHCP_OFFER:
                                    sRequestedIp = yourIp;
                                    result = DHCP_WAIT_OFFER;
                                    break;
                                case DHCP_ACK:
                                    CPSMyIp = yourIp;
                                    result = DHCP_WAIT_ACK;
                                    break;
                                }
                                break;
                            case 54:
                                sDhcpServerIp = CPS_BE32(msg + 1);
                                break;
                            }
                            msg += msg[0] + 1;
                        }
                    }
                }
            }
            CPS_SocConsume(len);
        }
    }
    return result;
}

// dhcp_discover_server: TRUE once a server offered an address
static BOOL dhcp_discover_server(void) {
    int result;
    int i;

    CPS_SocUse();
    CPS_SocDatagramMode();
    CPS_SocBind(DHCP_CLIENT_PORT, DHCP_SERVER_PORT, CPS_BROADCAST_ADDR);
    for (i = 0; i < 5; i++) {
        result = dhcp_analyze_response(dhcp_send_discover(), i);
        if (result == DHCP_WAIT_OFFER) {
            break;
        }
    }
    CPS_SocRelease();
    return result == DHCP_WAIT_OFFER;
}

// dhcp_request_server: TRUE once the server acknowledged; timer gets the seconds until the next request
static BOOL dhcp_request_server(u32 *timer, int mode) {
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
        result = dhcp_analyze_response(dhcp_send_request(mode), i);
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

// dhcp_release_server
static void dhcp_release_server(void) {
    u8 *buf;
    u8 *p;

    CPS_SocUse();
    CPS_SocDatagramMode();
    CPS_SocBind(DHCP_CLIENT_PORT, DHCP_SERVER_PORT, sDhcpServerIp);
    buf = DHCP_BUF;
    p = dhcp_setcommon(buf, DHCP_RELEASE);
    p[0] = 54;
    p[1] = 4;
    CPS_PUTBE32(p + 2, sDhcpServerIp);
    p[6] = 0xff;
    p += 7;
    CPS_SocWrite(buf, pad_mem(0, DHCP_MESSAGE_SIZE, p, p - buf) - buf);
    CPS_SocRelease();
}

// dns_skipname
static u8 *dns_skipname(u8 *p) {
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

// resolve_common: asks the bound server about a name, returning an address, 1 or 2 for a name that did or didn't fit
// in result, -1 for no such name, or 0 when no answer came
static s32 resolve_common(const u8 *name, u32 type, u16 id, u8 *result, u32 resultSize) {
    u8 query[0x44];
    u8 *reply;
    u8 *p;
    u8 *labelLen;
    u32 len;
    u32 count;
    u8 c;
    s32 answer;
    u32 start;
    u8 *end;

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
    labelLen = query + 12;
    p = labelLen + 1;
    len = 0;
    while ((c = *name++) != 0) {
        if (p - query > 0x3c) {
            return -1;
        }
        if (c != '.') {
            *p++ = c;
            len++;
        } else if (len != 0) {
            *labelLen = len;
            labelLen = p;
            p++;
            len = 0;
        }
    }
    *labelLen = len;
    answer = 0;
    p[0] = 0;
    p[1] = type >> 8;
    p[2] = type;
    p[3] = 0;
    p[4] = 1;
    CPS_SocWrite(query, p + 5 - query);
    start = CPSi_GetTick();
    while (sLinkIsOn() && answer == 0 && (int)(CPSi_GetTick() - start) < 15) {
        if (CPS_SocGetLength() == 0) {
            OS_Sleep(10);
            continue;
        }
        reply = CPS_SocRead(&len);
        if (len > 12 && CPS_GET16(reply) == id) {
            if ((reply[3] & 0xf) == 3) {
                answer = -1;
            } else if ((reply[3] & 0xf) == 0) {
                end = reply + len;
                count = (u16)(reply[5] | (reply[4] << 8));
                p = reply + 12;
                while (count-- != 0) {
                    p = dns_skipname(p) + 4;
                }
                while (p < end) {
                    p = dns_skipname(p);
                    count = CPS_BE16(p + 8);
                    if (CPS_BE16(p) == type) {
                        if (type != DNS_TYPE_PTR) {
                            answer = ((u32)CPS_BE16(p + 6 + count) << 16) | CPS_BE16(p + 8 + count);
                        } else if (count <= resultSize) {
                            MI_CpuCopy(p + 10, result, count);
                            answer = 1;
                        } else {
                            answer = 2;
                        }
                        break;
                    }
                    p += count + 10;
                }
            }
        }
        CPS_SocConsume(len);
    }
    return answer;
}

// strtol10
static u32 strtol10(const u8 *text, const u8 **end) {
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

// rawip: a dotted address
static BOOL rawip(const u8 *text, CPSInAddr *addr) {
    CPSInAddr ip = 0;
    int i;

    for (i = 0; i < 4; i++) {
        const u8 *end;
        u32 n;

        ip <<= 8;
        n = strtol10(text, &end);
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

// resolve_sub: asks a server for a name's address
static s32 resolve_sub(const u8 *name, CPSInAddr server, u16 id) {
    s32 result;

    if (server == 0) {
        return -1;
    }
    CPS_SocUse();
    CPS_SocDatagramMode();
    CPS_SocBind(0, DNS_PORT, server);
    result = resolve_common(name, DNS_TYPE_A, id, NULL, 0);
    CPS_SocRelease();
    return result;
}

// CPS_Resolve: the address of a name, or of a dotted address, or 0
CPSInAddr CPS_Resolve(const char *name) {
    s32 result = 0;
    u16 id[2];
    u8 usable[2];
    int round = 0;
    int i;

    id[0] = MATH_Rand32(&CPSiRand32ctx, 0x10000);
    id[1] = MATH_Rand32(&CPSiRand32ctx, 0x10000);
    if (rawip((const u8 *)name, (CPSInAddr *)&result)) {
        return result;
    }
    usable[0] = TRUE;
    usable[1] = TRUE;
    for (; round < 3; round++) {
        for (i = 0; i < 2; i++) {
            CPSInAddr hop;

            if (!usable[i]) {
                continue;
            }
            if (CPSDnsIp[i] == 0 || (hop = get_targetip(CPSDnsIp[i])) == 0) {
                usable[i] = FALSE;
                continue;
            }
            if (inq_arpcache(hop) == NULL && arprequest(hop) == NULL) {
                usable[i] = FALSE;
                continue;
            }
            result = resolve_sub((const u8 *)name, CPSDnsIp[i], id[i]);
            if (result != 0 && result != -1) {
                goto done;
            }
            if (result == -1) {
                usable[i] = FALSE;
            }
        }
    }
done:
    if (result == -1) {
        result = 0;
    }
    return result;
}
