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

// The stack's state that the socket layer reads. CPS_Startup sets the config-derived ones and DHCP (or the static
// configuration) the address ones
extern u32 CPSMode; // CPSConfig.mode (the CPS_MODE_ flags)
extern CPSInAddr CPSNetMask;
extern void *(*CPSiAlloc)(u32 size); // CPSConfig.alloc, or a no-op when alloc or free is missing
extern CPSInAddr CPSGatewayIp;
extern void (*CPSiFree)(void *ptr);
extern u32 CPSNoIpReason; // Why there is no address: the CPS_NOIP_REASON_ value
extern CPSInAddr CPSMyIp; // 0 until DHCP gives an address
extern CPSInAddr CPSDnsIp[2];
extern MATHRandContext32 CPSiRand; // Seeded by CPS_Startup; the SSL layer draws from it too

// CPS's signature, "[SDK+UBIQUITOUS:CPS]", which CPS_Startup refers to; the string sits in the main module at
// 0x020050ac
extern const char CPSi_Signature[];

// Called with a DHCP-given address; FALSE rejects it
void CPS_SetAddressCheckCallback(BOOL (*callback)(void));
// Starts the receive, scavenger and ACK threads over the config's LAN buffer
void CPS_Startup(CPSConfig *config);
// Asks the scavenger (DHCP) thread to end; TRUE once it has
BOOL CPS_CalmDown(void);
// The function the scavenger thread calls once a second
void CPS_SetScavengerCallback(void (*callback)(void));
// Ends CPS's threads and forgets the address and the LAN buffer
void CPS_Cleanup(void);
// Sets the priority of the receive and scavenger threads
void CPS_SetThreadPriority(u32 priority);
// The wireless driver's receive callback, which puts a frame in the LAN buffer
void CPSi_RecvCallback(u8 *src, u8 *dst, u8 *data, u32 len);
// A free local port
u16 CPSi_GetNewPort(void);

// The socket calls act on the socket that the calling thread registered. Without one they do nothing
// Registers a socket for the calling thread, and unregisters it
void CPS_SocRegister(CPSSoc *soc);
void CPS_SocUnRegister(void);
// Makes the registered socket a UDP one
void CPS_SocDatagramMode(void);
// Sets the ports and the remote address of the registered socket; a local port of 0 takes a new one
void CPS_SocBind(u16 localPort, u16 remotePort, CPSInAddr remoteIp);
// Takes the registered socket for the calling thread, reset to closed, and gives it back
void CPS_SocUse(void);
void CPS_SocRelease(void);
// Gives another thread the caller's socket
void CPS_SocDup(OSThread *thread);
// The function that gets the datagrams of the registered socket; it returns TRUE if it took the datagram
void CPS_SetUdpCallback(BOOL (*callback)(u8 *data, u32 len, CPSSoc *soc));
// TCP's connect, 0 once connected; CPS_TcpConnect does it for the registered socket
int CPSi_TcpConnect(CPSSoc *soc);
int CPS_TcpConnect(void);
void CPS_TcpShutdown(void);
// Waits up to about five seconds for the connection to close
void CPS_TcpClose(void);
// The received data and its length; consuming drops data from the front of the receive buffer
u8 *CPSi_TcpRead(u32 *len, CPSSoc *soc);
u8 *CPS_SocRead(u32 *len);
void CPSi_TcpConsume(u32 len, CPSSoc *soc);
void CPS_SocConsume(u32 len);
// Sends both pieces of data and returns how much was acknowledged
u32 CPSi_TcpWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc);
u32 CPS_SocWrite(u8 *data, u32 len);
// How much can be read, or -1 once the connection is gone
s32 CPS_SocGetLength(void);
// Sends what CPS_SocWrite held back
void CPS_SocFlush(void);
// The address of a name or of a dotted address, or 0
CPSInAddr CPS_Resolve(const char *name);

// CPS's SSL layer, which follows in the overlay: a socket's calls go there when its ssl is set
// The priority applied during selected crypto operations, or -1 when none; values of 32 and above disable it
u32 CPS_SslGetMaxPriority(void);
void CPS_SslSetMaxPriority(u32 priority);
// Registers the caller's named params (pinned certificates) on the current thread's connection
void CPS_SslSetNamedParams(void *const *params, u32 count);
// Stirs entropy into the pool that SSLi_GetRandomBytes draws from
void CPS_SslAddRandomSeed(const void *seed, u32 len);
int CPS_SslConnect(CPSSoc *soc);
u8 *CPS_SslRead(u32 *len, CPSSoc *soc);
void CPS_SslReadRelease(u32 len, CPSSoc *soc);
s32 CPS_SslCanRead(CPSSoc *soc);
u32 CPS_SslWrite(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc);
void CPS_SslShutdown(CPSSoc *soc);
void CPS_SslClose(CPSSoc *soc);
// Called once a second by CPS's scavenger thread: expires old session cache entries and wakes stalled handshakes
void CPS_SslTick(s32 now);
// Clears the session cache, whenever CPS drops its address
void CPS_SslInit(void);
// Turns the SSL layer on, or off, for the caller's socket
void CPS_SetSsl(BOOL enable);

#endif // POKEBW2_NITROWIFI_CPS_H
