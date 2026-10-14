#ifndef POKEBW2_NITROWIFI_CPS_H
#define POKEBW2_NITROWIFI_CPS_H

#include "types.h"
#include "nitro/math.h"
#include "nitro/os.h"

// NitroWiFi's CPS, Ubiquitous's TCP/IP stack ("[SDK+UBIQUITOUS:CPS]"), in overlay 11 (cps.c). The ROM names none of
// it: the public calls and variables carry the names of NitroWiFi's CPS API where the code shows their role (the
// socket layer, SOCL, which follows in the overlay, calls them), and the internal ones descriptive CPSi_ names.
//
// A socket (CPSSoc) belongs to a thread: CPS_SocRegister puts it in the thread's first specific word, and every
// CPS_Soc call works on the socket of the thread that makes it. Three threads of the stack's own run under it: one
// receives the frames that the wireless driver puts in the LAN buffer, one runs DHCP and the timeouts once a second,
// and one sends the delayed TCP ACKs when an alarm wakes it.

typedef u32 CPSInAddr;

// What CPS_Startup takes
typedef struct CPSConfig {
    u32 mode;                 // CPS_MODE_*
    void *(*alloc)(u32 size); // Both or neither
    void (*free)(void *ptr);
    void (*dhcp_callback)(void); // Called when DHCP gives up, or when the address is static
    BOOL (*link_is_on)(void);
    u64 random_seed; // 0 to seed with the tick count
    u8 *lan_buf;     // Where the received frames wait for the receiving thread
    u32 lan_buflen;
    u32 mymss;              // 0 for 1460
    CPSInAddr requested_ip; // The address DHCP asks for, or 0
    u32 yield_wait;         // Milliseconds that a wait sleeps for, or 0 to only yield
} CPSConfig;

// mode: an address of the config's own instead of DHCP's, and DNS servers of the config's own
#define CPS_MODE_STATIC_IP 1
#define CPS_MODE_STATIC_DNS 2

// Why there is no address (CPSNoIpReason)
enum {
    CPS_NOIP_REASON_NONE,
    CPS_NOIP_REASON_LINKOFF,
    CPS_NOIP_REASON_DHCPDISCOVERY,
    CPS_NOIP_REASON_LEASETIMEOUT,
    CPS_NOIP_REASON_COLLISION,
    CPS_NOIP_REASON_REJECTED,
};

// A socket's state
enum {
    CPS_STT_CLOSED,
    CPS_STT_LISTEN,
    CPS_STT_SYN_SENT,
    CPS_STT_SYN_RCVD,
    CPS_STT_ESTABLISHED,
    CPS_STT_UNUSED,
    CPS_STT_LAST_ACK, // The peer closed first and the FIN was answered
    CPS_STT_FIN_WAIT1,
    CPS_STT_FIN_WAIT2,
    CPS_STT_CLOSING,
    CPS_STT_DATAGRAM, // UDP
    CPS_STT_PING,     // ICMP echo
};

// What a socket's thread sleeps for
enum {
    CPS_BLOCK_NONE,
    CPS_BLOCK_TCPCON,
    CPS_BLOCK_TCPREAD,
    CPS_BLOCK_UDPREAD,
};

// A run of TCP data that arrived ahead of what the socket expects, at an offset in the receive buffer. order tells
// the most recent run
typedef struct CPSSocOoo {
    u32 start;
    u32 len;
    u32 order;
} CPSSocOoo;

typedef struct CPSSoc {
    OSThread *thread;
    u32 block_type;
    u8 state;
    u8 ssl;
    u16 local_port;
    void *con;               // The SSL connection
    s32 when;                // When the connection began, in units of 65536 ticks
    CPSInAddr local_ip_real; // The address the last datagram or SYN was sent to
    u16 remote_port;
    u16 remote_port_bound;
    CPSInAddr remote_ip;
    CPSInAddr remote_ip_bound;
    u32 ackno;        // The next sequence number expected
    u32 acked;        // The one the last ACK sent
    u32 ack_time;     // When that ACK went, in ticks
    u16 acked_window; // The window the last segment sent
    u8 delayed_ack;   // Segments that came in since
    u8 sack_permitted;
    u32 seqno; // The next sequence number to send
    u16 remote_win;
    u16 remote_mss;
    u32 remote_ackno;
    u32 segments;                                                // Segments received with data
    BOOL (*udp_callback)(u8 *data, u32 len, struct CPSSoc *soc); // TRUE when it took the datagram
    u32 rcvbuf_size;
    u8 *rcvbuf;
    u32 rcvbuf_len;
    u32 sndbuf_size;
    u8 *sndbuf;
    u8 unk_5c[0xc];
    u8 *outbuf; // Data that CPS_SocWrite sends ahead of the next write
    u32 outbuf_len;
    CPSSocOoo ooo[4];
} CPSSoc;

// The stack's state that the socket layer reads
extern u32 CPSMode;                  // CPSMode: the config's mode
extern CPSInAddr CPSNetMask;         // CPSNetMask
extern void *(*CPSiAlloc)(u32 size); // CPSiAlloc
extern CPSInAddr CPSGatewayIp;       // CPSGatewayIp
extern void (*CPSiFree)(void *ptr);  // CPSiFree
extern u32 CPSNoIpReason;            // CPSNoIpReason
extern CPSInAddr CPSMyIp;            // CPSMyIp, 0 until DHCP gives an address
extern CPSInAddr CPSDnsIp[2];        // CPSDnsIp
extern MATHRandContext32 CPSiRand;   // CPSiRand, which the SSL layer draws from too

// CPS's signature, "[SDK+UBIQUITOUS:CPS]", which CPS_Startup refers to; the string sits in the main module at
// 0x020050ac
extern const char CPSi_Signature[];

void CPS_SetAddressCheckCallback(BOOL (*callback)(void)); // CPS_SetAddressCheckCallback: may reject a DHCP address
void CPS_Startup(CPSConfig *config);                      // CPS_Startup
BOOL CPS_CalmDown(void);                                  // CPS_CalmDown: asks the DHCP thread to end, TRUE once it has
void CPS_SetScavengerCallback(void (*callback)(void));    // CPS_SetScavengerCallback: called once a second
void CPS_Cleanup(void);                                   // CPS_Cleanup
void CPS_SetThreadPriority(u32 priority);                 // CPS_SetThreadPriority
// The wireless driver's receive callback, which puts a frame in the LAN buffer
void CPSi_RecvCallback(u8 *src, u8 *dst, u8 *data, u32 len);
u16 CPSi_GetNewPort(void);                                           // A free local port
void CPS_SocRegister(CPSSoc *soc);                                   // CPS_SocRegister
void CPS_SocUnRegister(void);                                        // CPS_SocUnRegister
void CPS_SocDatagramMode(void);                                      // CPS_SocDatagramMode
void CPS_SocBind(u16 localPort, u16 remotePort, CPSInAddr remoteIp); // CPS_SocBind
void CPS_SocUse(void);                                               // CPS_SocUse
void CPS_SocRelease(void);                                           // CPS_SocRelease
void CPS_SocDup(OSThread *thread);                                   // CPS_SocDup: gives a thread the caller's socket
void CPS_SetUdpCallback(BOOL (*callback)(u8 *data, u32 len, CPSSoc *soc)); // CPS_SetUdpCallback
int CPSi_TcpConnect(CPSSoc *soc);                                          // TCP's connect, 0 once connected
int CPS_TcpConnect(void);                                                  // CPS_TcpConnect
void CPS_TcpShutdown(void);                                                // CPS_TcpShutdown
void CPS_TcpClose(void);                                                   // CPS_TcpClose
u8 *CPSi_TcpRead(u32 *len, CPSSoc *soc);                                   // TCP's read
u8 *CPS_SocRead(u32 *len);                                                 // CPS_SocRead
void CPSi_TcpConsume(u32 len, CPSSoc *soc);                                // TCP's consume
void CPS_SocConsume(u32 len);                                              // CPS_SocConsume
u32 CPSi_TcpWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc);    // TCP's write
u32 CPS_SocWrite(u8 *data, u32 len);                                       // CPS_SocWrite
s32 CPS_SocGetLength(void);              // CPS_SocGetLength, -1 once the connection is gone
void CPS_SocFlush(void);                 // CPS_SocFlush
CPSInAddr CPS_Resolve(const char *name); // CPS_Resolve

// CPS's SSL layer, which follows in the overlay: a socket's calls go there when its ssl is set
u32 CPS_SslGetMaxPriority(void);                       // CPS_SslGetMaxPriority's role
void CPS_SslSetMaxPriority(u32 priority);               // CPS_SslSetMaxPriority's role
void CPS_SslSetNamedParams(void *const *params, u32 count); // Registers the caller's named params (pinned
                                                            // certificates) on the current thread's connection
void CPS_SslAddRandomSeed(const void *seed, u32 len);  // CPS_SslAddRandomSeed's role: stirs entropy into SSLi_GetRandomBytes's pool
int CPS_SslConnect(CPSSoc *soc);
u8 *CPS_SslRead(u32 *len, CPSSoc *soc);
void CPS_SslReadRelease(u32 len, CPSSoc *soc);
s32 CPS_SslCanRead(CPSSoc *soc);
u32 CPS_SslWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc);
void CPS_SslShutdown(CPSSoc *soc);
void CPS_SslClose(CPSSoc *soc);
void CPS_SslTick(s32 now);
void CPS_SslInit(void);
void CPS_SetSsl(BOOL enable); // CPS_SetSsl's role: turns the SSL layer on for the caller's socket

#endif // POKEBW2_NITROWIFI_CPS_H
