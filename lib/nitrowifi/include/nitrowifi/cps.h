#ifndef POKEBW2_NITROWIFI_CPS_H
#define POKEBW2_NITROWIFI_CPS_H

#include "types.h"
#include "nitro/math.h"
#include "nitro/os.h"

// NitroWiFi's CPS, Ubiquitous's TCP/IP stack ("[SDK+UBIQUITOUS:CPS]"), in overlay 11 (cps.c). The ROM names none of
// it: the public calls and variables carry the names of NitroWiFi's CPS API where the code shows their role (the
// socket layer, SOCL, which follows in the overlay, calls them), and the internal ones the names of NitroWiFi's own
// cps.c, as ntrtwl's reconstruction of NitroWiFi has them.
//
// A socket (CPSSoc) belongs to a thread: CPS_SocRegister puts it in the thread's first specific word, and every
// CPS_Soc call works on the socket of the thread that makes it. Three threads of the stack's own run under it: one
// receives the frames that the wireless driver puts in the LAN buffer, one runs DHCP and the timeouts once a second,
// and one sends the delayed TCP ACKs when an alarm wakes it.

typedef u32 CPSInAddr;

// A certificate authority that the SSL layer accepts (NitroWiFi's ssl.h): its distinguished name, the RSA modulus of
// its public key and the key's public exponent
typedef struct CPSCaInfo {
    char *dn;
    int modulus_len;
    u8 *modulus;
    int exponent_len;
    u8 *exponent;
} CPSCaInfo;

// The states of RC4, MD5 and SHA-1 that an SSL connection keeps. RC4's has pokediamond's field layout (CPSi_rc4_init
// writes s right after i and j, with no padding)
typedef struct CPSRc4Ctx {
    u8 i;
    u8 j;
    u8 s[256];
} CPSRc4Ctx;

typedef struct CPSMd5Ctx {
    u32 state[4];
    u32 count[2]; // The message's length so far, in bits, as a 64-bit little-endian pair
    u8 buffer[64];
} CPSMd5Ctx;

typedef struct CPSSha1Ctx {
    u32 state[5];
    u32 count[2]; // The total bits hashed so far: count[0] is the high word, count[1] the low one CPSi_sha1_calc adds
                  // to
    u8 buffer[64];
} CPSSha1Ctx;

// A certificate's RSA public key, as the SSL layer parses it. A descriptive name of ours; no SDK name for it is known
typedef struct SSLCertificatePublicKey {
    u8 modulus[0x100];
    s32 modulus_len;
    u8 exponent[8];
    s32 exponent_len;
} SSLCertificatePublicKey;

// An SSL connection, DER-parsed certificate fields included. 0x830 bytes. Its user (SOCL_EnableSsl's caller) fills in
// the server's name, the certificate callback and the root CAs, and the SSL layer the rest
typedef struct CPSSslConnection {
    u8 secret[0x30]; // The pre-master secret, overwritten with the master secret once derived
    u8 resumed;      // TRUE once a cached session's secret was restored
    u8 unk_031;
    u16 cipher_suite;
    u8 client_random[0x20];
    u8 server_random[0x20];
    // Session storage becomes the embedded key block after session-cache operations.
    // The final generated block overlaps pointer slots, assigned only after generation.
    union {
        u8 key_block[0x50];
        struct {
            u8 session_id[0x20];
            u8 unk_094[0x28];
            u8 *write_mac_secret;
            u8 *write_key;
        } fields;
    } key_storage; // Descriptive reconstruction: 0x74..0xc3, not an established SDK type.
    u8 unk_0c4[4];
    CPSRc4Ctx write_rc4;
    u8 unk_1ca[2];
    u8 write_seq_num[8]; // The 8-byte big-endian sequence number make_ciphertext folds into an outgoing record's MAC
    u8 *read_mac_secret;
    u8 *read_key;
    u8 unk_1dc[4];
    CPSRc4Ctx read_rc4;
    u8 unk_2e2[2];
    u8 read_seq_num[8];                 // Same, for a received record's MAC; both start at 0 and add1_be8 advances them
    CPSSha1Ctx handshake_sha1;          // The running hash of every handshake message so far
    CPSSha1Ctx handshake_sha1_snapshot; // Copied for Finished; the snapshot contexts also serve as certificate-hash
                                        // scratch storage
    CPSMd5Ctx handshake_md5;
    CPSMd5Ctx handshake_md5_snapshot;
    u8 is_server;                // Descriptive role name: nonzero selects the server handshake path
    u8 state;                    // SSLI_STATE_*, read back by parse_record's caller
    u8 application_record_ready; // TRUE when read_buf holds a complete application record retained by dispatch
    u8 pad_457;
    s32 signature_algorithm; // 0x458: OID index 3 (MD5) or 4 (SHA1)
    s32 public_key_algorithm;
    const u8 *signed_start; // Borrowed DER span, valid while the input certificate buffer is owned by the caller
    const u8 *signed_end;
    u8 certificate_digest[0x14];
    u32 certificate_digest_len;
    CPSCaInfo chain_key;                     // 0x480; borrowed next-certificate key spans, with the dn slot unused
    SSLCertificatePublicKey certificate_key; // 0x494, ends at 0x5a4
    const u8 *certificate_signature;
    s32 certificate_signature_len;
    // One byte reused across phases, not two independent flags.
    union {
        u8 parsing_subject;
        u8 certificate_requested;
    } certificate_phase; // 0x5ac
    u8 parsing_rsa_key;
    u8 attribute_oid;
    u8 certificate_date_valid;
    char issuer[0x100];
    char subject[0x100];
    char common_name[0x50];
    char *hostname;        // 0x800: the server's name, checked against the certificate's, or NULL
    const u8 *certificate; // 0x804
    s32 certificate_len;
    u32 current_date;
    int (*validate_certificate)(int result, struct CPSSslConnection *ctx, int index);
    CPSCaInfo **named_params; // The caller's root CAs, set with CPS_SetRootCa
    u32 named_param_count;
    void *key_param; // Private key consumed by rcv_client_key_exchange; its writer is not recovered
    u8 unk_820[4];
    u8 *read_buf; // Owned incomplete record, or a dispatched application record awaiting consumption
    u32 read_buf_len;
    u32 read_buf_pos;
} CPSSslConnection;

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
#define CPS_DONOTUSE_DHCP 1
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
    CPS_STT_CLOSE_WAIT,
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

// A buffer a socket gives CPS
typedef struct CPSSocBuf {
    u32 size;
    u8 *data;
} CPSSocBuf;

typedef struct CPSSoc {
    OSThread *thread;
    u32 block_type;
    u8 state;
    u8 ssl;
    u16 local_port;
    void *con;               // The SSL connection
    u32 when;                // When the connection began, in units of 65536 ticks
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
    CPSSocBuf rcvbuf;
    u32 rcvbuf_len; // How much of the receive buffer holds data
    CPSSocBuf sndbuf;
    CPSSocBuf linbuf;
    CPSSocBuf outbuf; // Data that CPS_SocWrite sends ahead of the next write
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
extern int CPSNoIpReason; // Why there is no address: the CPS_NOIP_REASON_ value
extern CPSInAddr CPSMyIp; // 0 until DHCP gives an address
extern CPSInAddr CPSDnsIp[2];
extern MATHRandContext32 CPSiRand32ctx; // Seeded by CPS_Startup; the SSL layer draws from it too

// CPS's signature, "[SDK+UBIQUITOUS:CPS]", which CPS_Startup refers to; the string sits in the main module at
// 0x020050ac
extern const char CPSi_Signature[];

// Called with a DHCP-given address; FALSE rejects it
void CPS_SetAddressCheckCallback(BOOL (*callback)(void));
// Starts the receive, scavenger and ACK threads over the config's LAN buffer
void CPS_Startup(CPSConfig *config);
// Asks the scavenger (DHCP) thread to end; TRUE once it has
int CPS_CalmDown(void);
// The function the scavenger thread calls once a second
void CPS_SetScavengerCallback(void (*callback)(void));
// Ends CPS's threads and forgets the address and the LAN buffer
void CPS_Cleanup(void);
// Sets the priority of the receive and scavenger threads
void CPS_SetThreadPriority(u32 priority);
// The wireless driver's receive callback, which puts a frame in the LAN buffer
void CPSi_RecvCallbackFunc(const u8 *src, const u8 *dst, const u8 *data, s32 len);
// A free local port
u16 CPS_SocGetEport(void);

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
u32 CPSi_TcpConnectRaw(CPSSoc *soc);
u32 CPS_TcpConnect(void);
void CPS_TcpShutdown(void);
// Waits up to about five seconds for the connection to close
void CPS_TcpClose(void);
// The received data and its length; consuming drops data from the front of the receive buffer
u8 *CPSi_TcpReadRaw(u32 *len, CPSSoc *soc);
u8 *CPS_SocRead(u32 *len);
void CPSi_SocConsumeRaw(u32 len, CPSSoc *soc);
void CPS_SocConsume(u32 len);
// Sends both pieces of data and returns how much was acknowledged
u32 CPSi_TcpWrite2Raw(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc);
u32 CPS_SocWrite(u8 *data, u32 len);
// How much can be read, or -1 once the connection is gone
s32 CPS_SocGetLength(void);
// Sends what CPS_SocWrite held back
void CPS_SocFlush(void);
// The address of a name or of a dotted address, or 0
CPSInAddr CPS_Resolve(const char *name);

// CPS's SSL layer, which follows in the overlay: a socket's calls go there when its ssl is set
// The priority applied during selected crypto operations, or -1 when none; values of 32 and above disable it
u32 CPS_GetSslHandshakePriority(void);
void CPS_SetSslHandshakePriority(u32 priority);
// Registers the caller's root certificate authorities on the current thread's connection
void CPS_SetRootCa(CPSCaInfo **ca, int count);
// Stirs entropy into the pool that set_random draws from
void CPS_SslAddRandomSeed(void *seed, u32 len);
u32 CPSi_SslConnect(CPSSoc *soc);
u8 *CPSi_SslRead(u32 *len, CPSSoc *soc);
void CPSi_SslConsume(u32 len, CPSSoc *soc);
s32 CPSi_SslGetLength(CPSSoc *soc);
u32 CPSi_SslWrite2(u8 *data, u32 len, u8 *data2, u32 len2, CPSSoc *soc);
void CPSi_SslShutdown(CPSSoc *soc);
void CPSi_SslClose(CPSSoc *soc);
// Called once a second by CPS's scavenger thread: expires old session cache entries and wakes stalled handshakes
void CPSi_SslPeriodical(u32 now);
// Clears the session cache, whenever CPS drops its address
void CPSi_SslCleanup(void);
// Turns the SSL layer on, or off, for the caller's socket
void CPS_SetSsl(int enable);

#endif // POKEBW2_NITROWIFI_CPS_H
