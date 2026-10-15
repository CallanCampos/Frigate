#ifndef POKEBW2_NITROWIFI_SOC_H
#define POKEBW2_NITROWIFI_SOC_H

#include "types.h"
#include "nitrowifi/cps.h"

// NitroWiFi's socket layer, SOCL, over CPS, in overlay 11, and the BSD-style SOC calls over it. The ROM names none of
// it. Its files and names follow NitroWiFi's objects as pret's pokediamond links them (socl_startup.o, socl_command.o
// and so on, with every function named), matched to this newer version (NitroWiFi 3.3, by its signature in the main
// module) by what each function does.

// What SOCL_Startup takes
typedef struct SOCLConfig {
    BOOL use_dhcp; // Ask DHCP for an address, or use host_ip
    struct {
        CPSInAddr my_ip;
        CPSInAddr net_mask;
        CPSInAddr gateway_ip;
        CPSInAddr dns_ip[2];
    } host_ip;
    void *(*alloc)(u32 size);
    void (*free)(void *ptr);
    u32 cmd_packet_max;   // How many command packets the sockets can have waiting
    u32 lan_buffer_size;  // 0 for SOCLiDefaultLanBufferSize
    void *lan_buffer;     // NULL to allocate lan_buffer_size bytes
    s32 cps_thread_prio;  // 0 for 11
    s32 mtu;              // 0 for 576, and kept between 576 and 1500
    s32 rwin;             // TCP's receive window, 0 for 0x10c0
} SOCLConfig;

// The thread of a socket's command pipe, and how many packets it queues
typedef struct SOCLPipeParam {
    u16 stack_size; // Bytes
    u8 priority;
    u8 queue_length; // Message count
} SOCLPipeParam;

// How a socket is created: SOCLSocketParamTCP, SOCLSocketParamUDP and SOCLSocketParamUDPSend (soc.c). A socket has a
// receive pipe when rcvbuf_size is not 0 and a send pipe when sndbuf_size is not 0; without one it uses the UDP send
// socket's.
typedef struct SOCLSocketParam {
    s8 flag_mode;
    s8 flag_block;
    // Buffer sizes and the consume threshold count bytes.
    struct {
        u16 rcvbuf_size;
        u16 rcvbuf_consume_min;
        u16 sndbuf_size;
        u16 rcvbuf_aux_size;
        u16 sndbuf_aux_size;
        u16 sndpipe_buf_size;
        u16 rcvpipe_unk_e;
    } buffer;
    SOCLPipeParam rcv_pipe;
    SOCLPipeParam snd_pipe;
} SOCLSocketParam;

extern SOCLSocketParam SOCLSocketParamUDP;
extern SOCLSocketParam SOCLSocketParamTCP;
extern SOCLSocketParam SOCLSocketParamUDPSend;

// SOCL's errors, as negative return values (BSD's numbers)
#define SOCL_EAGAIN (-6)
#define SOCL_EALREADY (-7)
#define SOCL_ECANCELED (-11) // What the commands still queued in a pipe that is freed get
#define SOCL_ECONNRESET (-15)
#define SOCL_EINPROGRESS (-26)
#define SOCL_EINVAL (-28)
#define SOCL_EMFILE (-33)
#define SOCL_EMSGSIZE (-35) // A datagram that is more than a packet holds
#define SOCL_EISCONN (-30)
#define SOCL_ENETRESET (-39) // SOCL has not been started
#define SOCL_ENOMEM (-49)
#define SOCL_ENOTCONN (-56)
#define SOCL_ETIMEDOUT (-76)

// The BSD-style calls over SOCL (soc.c). A socket is the handle that SOC_Socket returns (SOCL's socket, which the
// calls take as an int), and they return negative SOCL_E* codes. The address and port of a socket address are in
// network order, and its first byte is its size
#define POKE_NET_SOC_AF_INET 2
#define POKE_NET_SOC_SOCK_STREAM 1
#define POKE_NET_SOC_SHUT_RDWR 2
#define POKE_NET_SOC_EWOULDBLOCK (-6)
#define POKE_NET_SOC_EINPROGRESS (-26)
#define POKE_NET_SOC_EISCONN (-30)

typedef struct SocAddrIn {
    u8 len;
    u8 family;
    u16 port;
    u32 addr;
} SocAddrIn;
typedef struct SocHostEnt {
    char *name;
    char **aliases;
    s16 addrType;
    s16 addrLength;
    u8 **addrList;
} SocHostEnt;
// An address (in network order) passed by value
typedef struct SocInAddr {
    u32 addr;
} SocInAddr;
// A poll descriptor, and the configuration that SOC_Startup takes (its addresses are in network order)
typedef struct SocPollFd {
    int fd;
    s16 events;
    s16 revents;
} SocPollFd;
typedef void *(*SocAllocFunc)(u32 name, s32 size);
typedef void (*SocFreeFunc)(u32 name, void *ptr, s32 size);
typedef struct SocConfig {
    u32 flag;           // 0x00
    SocAllocFunc alloc; // 0x04
    SocFreeFunc free;   // 0x08
    int useDhcp;        // 0x0c
    u32 addr;           // 0x10
    u32 netmask;        // 0x14
    u32 gateway;        // 0x18
    u32 dns1;           // 0x1c
    u32 dns2;           // 0x20
    u32 unk_24;         // 0x24
    u32 unk_28;         // 0x28
    u32 mtu;            // 0x2c: SOCLConfig's mtu
    u32 rwin;           // 0x30: SOCLConfig's rwin
    u32 unk_34[9];      // 0x34
} SocConfig;

// Only type selects the transport: 1 creates TCP, every other value UDP. domain/protocol are ignored.
int SOC_Socket(int domain, int type, int protocol);
// Address fields are read during the call, not retained: Bind uses port, Connect uses port/addr; len/family ignored.
// TCP Bind records the local port for a later connect; UDP may queue a bind. Errors need not roll back endpoint/state.
int SOC_Bind(int sock, const SocAddrIn *address);
// A fresh TCP attempt waits for the callback with flag_block==1; other modes map admission to EINPROGRESS.
// UDP only stores the peer; repeated TCP attempts report existing connection/pending state.
int SOC_Connect(int sock, const SocAddrIn *address);
// Receive sizes are byte capacities. UDP returns the full datagram length even when fewer bytes fit; ordinary
// TCP/SSL reads return the copied count. Read flag 2 retains UDP/plain-CPS data, but SSL reads still consume it.
int SOC_Read(int sock, void *buffer, int size, int flags);
// On a nonnegative result, only from->port/addr are written when from is nonnull; len/family are unchanged.
// SSL command reads do not supply peer outputs, so this wrapper does not establish a valid peer on that path.
int SOC_RecvFrom(int sock, void *buffer, int size, int flags, SocAddrIn *from);
// Copies caller data into the send ring before return; SendTo also copies destination scalars without retaining to.
// Positive results count admitted payload bytes, not transmission completion. Nonblocking sends can return a prefix;
// a later admission failure can return an error after an earlier prefix was queued.
int SOC_Send(int sock, const void *buffer, int size, int flags);
int SOC_SendTo(int sock, const void *buffer, int size, int flags, const SocAddrIn *to);
// Ignores how; the TCP callback requests send-half shutdown without freeing the socket. flag_block==1 waits for it;
// other modes report queue admission. A missing send pipe can return zero without issuing a command.
int SOC_Shutdown(int sock, int how);
// Zero does not establish close-callback completion or allocation release; an absent positive handle also returns zero.
// A socket with a close request already set, or still in trash, returns EINPROGRESS. Scavenging reclaims its storage.
int SOC_Close(int sock);
// Returns shared static result/name/address storage, overwritten by later successful lookups; do not free it.
SocHostEnt *SOC_GetHostByName(const char *host);
int SOC_GetSockName(int sock, SocAddrIn *address);
u32 SOC_GetHostID(void);
// Sets the DNS servers, despite the name (the SDK's; pokediamond's is the same)
int SOC_GetResolver(const u32 *dns1, const u32 *dns2);
// Command 3 returns zero for flag_block==1, otherwise 4. Command 4 stores mode 0 for flags&4, otherwise mode 1.
// Other commands return zero without changing mode.
// A zero handle returns -1.
int SOC_Fcntl(int sock, int command, int flags);
// Copies selected configuration fields and allocator callbacks into static storage; does not retain config itself.
int SOC_Startup(const SocConfig *config);
int SOC_Cleanup(void);
// Returns a shared 16-byte static formatting buffer, overwritten by the next call; do not free it.
char *SOC_InetNtoA(SocInAddr addr);
int SOC_InetAtoN(const char *text, u32 *address);
// AF_INET only: reads four address bytes and needs at least 16 writable buffer bytes; returns buffer or NULL.
char *SOC_InetNtoP(int family, const void *address, char *buffer, u32 size);
void SOC_U32to4U8(u32 value, u8 *bytes);
// count counts descriptors; each scan replaces revents and always checks status bits 0xe0 as well as requested events.
// -1 disables the timeout budget; other nonpositive values scan once. Positive ticks decrease by 523 per delay;
// this loop does not measure an absolute deadline. Returns the number of descriptors with nonzero revents.
int SOC_Poll(SocPollFd *fds, u32 count, s64 ticks);
// Borrows mutable connection storage on a created TCP-mode socket whose binding-state bit is clear. Keep nonnull
// sslInfo alive while installed and through SSL operations/close using it. Success confirms the change, not a handshake.
// NULL detaches/selects plain TCP; it does not reset/free the old context or perform SSL shutdown. The state gate
// also applies to detachment, so this is not an unrestricted cleanup operation for an already binding socket.
int SOCL_EnableSsl(int sock, void *sslInfo);
void SOCLi_TrashSocket(void);

// Successful initial startup retains config for later callbacks and cleanup. Keep it alive until cleanup succeeds;
// an already active layer ignores the new pointer. SOC_Startup instead supplies its own static SOCLConfig.
int SOCL_Startup(SOCLConfig *config);
// Reads param synchronously without retaining its pointer. Owned buffer/pipe storage is in the socket allocation;
// a UDP receive socket instead borrows its shared send pipe.
// The final command result is ignored, so a returned allocation handle alone does not prove CREATED/registration.
int SOCL_CreateSocket(const SOCLSocketParam *param);

#endif // POKEBW2_NITROWIFI_SOC_H
