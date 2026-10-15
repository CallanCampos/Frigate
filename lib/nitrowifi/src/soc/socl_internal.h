#ifndef POKEBW2_NITROWIFI_SOCL_INTERNAL_H
#define POKEBW2_NITROWIFI_SOCL_INTERNAL_H

#include "nitrowifi/soc.h"

// SOCL's state, shared by its files

// socl_startup.c
extern SOCLConfig *SOCLiConfigPtr;     // The config SOCL_Startup was given, NULL while SOCL is down
extern u32 SOCLiDefaultLanBufferSize;  // The LAN buffer's size when the config gives none
extern CPSInAddr SOCLiRequestedIP;     // The address that DHCP asks for
extern u32 SOCLiDhcpState;             // SOCLI_DHCP_*
extern CPSConfig SOCLiCPSConfig;

// SOCLiDhcpState: DHCP is in use, and it has finished (or the address is static)
#define SOCLI_DHCP_REQUESTED 1
#define SOCLI_DHCP_DONE 2

// A command pipe: the queue of the packets that its thread runs (the socket's fields at 0xa0 and 0xa4 point to its
// receive and send pipes, and a socket without a receive pipe uses the send pipe for control). Its messages and the
// thread's stack follow the pipe in the socket's memory.
typedef struct SOCLCommandPipe {
    OSMessageQueue queue;
    OSThread thread;
    OSMutex mutex;
} SOCLCommandPipe;

// A datagram that SOCLi_UdpRecvCallback queued, followed by its data
typedef struct SOCLUdpPacket {
    struct SOCLUdpPacket *next;
    u16 len;
    u16 port;
    CPSInAddr ip;
    u8 data[];
} SOCLUdpPacket;

// The receive pipe, 0x114 bytes
typedef struct SOCLRecvPipe {
    SOCLCommandPipe base;
    int cps_read_len;       // What the reads have taken from CPS's receive buffer and not yet consumed
    u16 rcvbuf_consume_min; // From SOCLSocketParam: how much to take before CPS_SocConsume
    s8 peek;               // Suppresses UDP dequeue/CPS offset advances; SSL command reads ignore it
    u8 unk_ff;
    SOCLUdpPacket *udp_tail;  // The datagrams that came in, oldest first
    SOCLUdpPacket *udp_head;
    u16 udp_queued;         // The bytes of datagram data they hold
    u16 udp_queued_max;     // From SOCLSocketParam's rcvpipe_unk_e
    OSThreadQueue udp_wait; // Where a blocking UDP read sleeps
} SOCLRecvPipe;

// A buffer in the socket's memory
typedef struct SOCLSocketBuffer {
    u32 size;
    u8 *data; // NULL when size is 0
} SOCLSocketBuffer;

struct SOCLSocket;

// The send pipe, 0x110 bytes
typedef struct SOCLSendPipe {
    SOCLCommandPipe base;
    SOCLSocketBuffer buffer;
    u16 write_pos;           // Where the next write goes in the buffer (a ring)
    u16 read_pos;            // How far the pipe's thread has taken data out of it
    OSThreadQueue wait;      // Where a blocking write sleeps until the buffer has room
    struct SOCLSocket *socket; // The socket that owns the pipe (a UDP socket shares the UDP send socket's)
} SOCLSendPipe;

// A socket, 0xbc bytes, followed in memory by its pipes and buffers
typedef struct SOCLSocket {
    u8 unk_00[8];              // The socket starts with its CPSSoc (0xa0 bytes: SOCL_Resolve makes one on its own)
    u8 cps_state;              // 0x08, CPSSoc.state
    u8 unk_09[0x3f];
    SOCLSocketBuffer rcvbuf;   // 0x48
    u32 rcvbuf_len;            // CPSSoc.rcvbuf_len: how much of the receive buffer holds data
    SOCLSocketBuffer sndbuf;   // 0x54
    SOCLSocketBuffer rcvbuf_aux;
    SOCLSocketBuffer sndbuf_aux;
    u8 unk_6c[0x34];
    SOCLCommandPipe *recv_pipe;
    SOCLCommandPipe *send_pipe;
    int result; // What the last command packet returned
    // SOCLI_STATE_*. volatile: the command pipes' threads change it while callers test it, and the original reloads it
    // for each test (a plain s16 gets one load shared by the tests, and SOCL_GetStatus, SOCL_Connect, SOCLi_ReadFrom and
    // SOCL_WriteTo change)
    // TODO: volatile was added to match SOCL_GetStatus, SOCLi_ReadFrom and SOCL_WriteTo (and to bring SOCL_Connect and
    // SOCL_Shutdown to size), on the reasoning that the command pipes' threads change it; it is not known whether
    // NitroWiFi's own header declares it volatile. Revisit if another spelling of the tests turns up.
    volatile s16 state;
    s8 flag_block;
    s8 flag;    // The mode (SOCLSocketParam's flag_mode), copied into each command packet
    u16 local_port;  // 0xb0
    u16 remote_port;
    CPSInAddr remote_ip;
    struct SOCLSocket *next; // The next socket of the list it is in
} SOCLSocket;

// The receive pipe of a socket that has one (every socket that reads)
static inline SOCLRecvPipe *SOCLi_GetRecvPipe(const SOCLSocket *socket) {
    return (SOCLRecvPipe *)socket->recv_pipe;
}

// The send pipe of a socket that has one (a UDP socket's is the UDP send socket's)
static inline SOCLSendPipe *SOCLi_GetSendPipe(const SOCLSocket *socket) {
    return (SOCLSendPipe *)socket->send_pipe;
}

// socl_startup.c: the socket that sends every UDP socket's datagrams
extern SOCLSocket *SOCLiUDPSendSocket;

// SOCLi_ReadFrom's flags (BSD's MSG_PEEK and MSG_DONTWAIT)
#define SOCLI_READ_PEEK 2     // Retain UDP datagrams/plain-CPS read position; SSL reads still consume copied bytes
#define SOCLI_READ_NOBLOCK 4  // Do not wait for data

// SOCL_WriteTo's flags
#define SOCLI_WRITE_NOBLOCK 4 // Do not wait for room in the send buffer

// SOCLSocket.state
#define SOCLI_STATE_CREATED 1
#define SOCLI_STATE_BINDING 2        // SOCLi_ExecBindCommand has queued the bind (and connect)
#define SOCLI_STATE_CONNECTED 4      // The bind (and for TCP the connect) succeeded
#define SOCLI_STATE_CLOSING 8
#define SOCLI_STATE_CLOSE_CALLED 0x10   // SOCL_Close has run (it queued the close command)
#define SOCLI_STATE_CLOSED 0x20         // The close command has run: the socket waits in the trash list
#define SOCLI_STATE_CONNECT_FAILED 0x40 // The connect failed, with the error in the socket's result
#define SOCLI_STATE_RESET 0x80       // The connection was reset

static inline BOOL SOCLi_SocketIsCreated(const SOCLSocket *socket) {
    BOOL created = FALSE;
    if (socket != NULL) {
        if (socket->state & SOCLI_STATE_CREATED) {
            created = TRUE;
        } else {
            created = FALSE;
        }
    }
    return created;
}

// SOCLSocket.flag: 0 and 4 are TCP. SOCLi_ModeIsTCP tests a mode that was read already
static inline BOOL SOCLi_ModeIsTCP(s8 mode) {
    BOOL tcp = TRUE;
    if (mode != 0 && mode != 4) {
        tcp = FALSE;
    }
    return tcp;
}

static inline BOOL SOCLi_SocketIsTCP(const SOCLSocket *socket) {
    BOOL tcp = TRUE;
    if (socket->flag != 0 && socket->flag != 4) {
        tcp = FALSE;
    }
    return tcp;
}

struct SOCLCommandPacket;
typedef int (*SOCLCommandHandler)(struct SOCLCommandPacket *packet);

// A command for a socket's pipe, 0x2c bytes: the handler, run by the pipe's thread, with its arguments after the
// header
typedef struct SOCLCommandPacket {
    SOCLCommandHandler handler;
    SOCLSocket *socket;
    OSMessageQueue *reply_queue; // Borrowed caller-stack result queue for mode 1, NULL otherwise
    s8 socket_flag;
    s8 flag_block;               // Exactly 1 waits for the result; bit 0 blocks queue admission (writes use 3 or 0)
    u8 unk_0e[2];
    union {
        struct {
            u16 local_port;
            u16 remote_port;
            CPSInAddr remote_ip;
        } bind;
        struct {
            u8 *buf;
            int len;
            u16 *remote_port; // Optional peer outputs, filled according to the selected receive path
            CPSInAddr *remote_ip;
        } read;
        struct {
            u8 *buf1;        // Owned send-ring slices used by the queued callback, not application storage
            int len1;        // Slice byte counts; buf2 is the wrapped part, not an application-buffer pointer
            u8 *buf2;
            int len2;
            u16 write_end;   // The pipe's write position after this data
            u8 unk_22[2];
            u16 local_port;  // For a UDP packet, the datagram's ports and address
            u16 remote_port;
            CPSInAddr remote_ip; // 0 for TCP
        } write;
        struct {
            void *info; // Borrowed connection retained by CPS; NULL detaches without freeing/resetting it
        } ssl;
        u8 raw[0x1c];
    } args;
} SOCLCommandPacket;

// socl_command.c
int SOCLi_StartupCommandPacketQueue(u32 max);
int SOCLi_CleanupCommandPacketQueue(void);
SOCLCommandPacket *SOCLi_AllocCommandPacket(s32 flags);
SOCLCommandPacket *SOCLi_CreateCommandPacket(SOCLCommandHandler handler, SOCLSocket *socket, s32 flag_block);
void SOCLi_FreeCommandPacket(SOCLCommandPacket *packet);
SOCLCommandPipe *SOCLi_GetCtrlPipe(SOCLSocket *socket);
int SOCLi_SendCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet);
int SOCLi_SendCommandPacketToCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
int SOCLi_ExecCommandPacketInRecvPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
int SOCLi_ExecCommandPacketInSendPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
int SOCLi_ExecCommandPacketInCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
void SOCLi_CommandPacketHandler(OSMessageQueue *queue);

// socl_bind.c
// SOCL ports/IP scalars are in host order; SOC wrappers convert their network-order address fields.
int SOCL_Bind(SOCLSocket *socket, u16 local_port);
int SOCL_Connect(SOCLSocket *socket, u16 remote_port, CPSInAddr remote_ip);

// socl_write.c
// Copies caller payload into the send ring before queue admission; caller storage is not retained after return.
// Returns admitted byte count, not callback/transmission completion. Nonblocking writes may return a prefix;
// a later blocking-loop admission failure returns EAGAIN even if an earlier prefix was queued.
// UDP copies destination scalars, using the stored peer only when remote_ip is zero and the stored IP is nonzero.
// flags is SOCLI_WRITE_*; both command modes used by writes (3/0) return without waiting for callback completion.
int SOCL_WriteTo(SOCLSocket *socket, const u8 *buf, int len, u16 remote_port, CPSInAddr remote_ip, int flags);
// How much room the socket's send pipe has in its buffer
int SOCLi_GetWriteBufferFreeSize(const SOCLSocket *socket);

// socl_shutdown.c
// Retains the allocation. Exact flag_block==1 waits for the callback; other modes report admission, not completion.
int SOCL_Shutdown(SOCLSocket *socket);

// socl_close.c
// Tests nonnegative handle absence from live/trash lists, not historical allocation ownership or generation.
BOOL SOCL_IsClosed(int socket);
// TRUE for a handle that is no socket of the list and not in the trash, or that is a socket SOCL_Close has not run on
// (SOC_Close's caller in DWC asks it after a failed close; no counterpart in pokediamond's NitroWiFi)
BOOL func_ov011_02157314(int socket);
// Returns without awaiting callback/reclamation; zero also covers an already absent positive handle.
int SOCL_Close(int socket);
void SOCLi_TrashSocket(void);

// socl_cleanup.c
int SOCL_Cleanup(void);
int SOCL_CalmDown(void);
CPSInAddr SOCL_Resolve(const char *name);
CPSInAddr SOCL_InetAtoH(const char *name);
CPSInAddr SOCL_GetHostID(void);
int SOCL_SetResolver(CPSInAddr dns1, CPSInAddr dns2);
// Shuts CPS down after a failed SOCL_Startup, and in SOCL_Cleanup (no counterpart in pokediamond's NitroWiFi, whose
// SOCL_Cleanup does it inline; name a guess)
void SOCLi_CleanupCPS(void);

// socl_poll.c
// SOCL_GetStatus's bits
#define SOCL_STATUS_READABLE 0x01  // Data to read (or a datagram)
#define SOCL_STATUS_WRITABLE 0x08  // Room in the send buffer
#define SOCL_STATUS_ERROR 0x20     // The connect failed
#define SOCL_STATUS_NOTCONN 0x40   // Neither connecting nor connected
#define SOCL_STATUS_INVALID 0x80   // Not a socket
int SOCL_GetStatus(SOCLSocket *socket);

// socl_misc.c
u32 SOCLi_RoundUp4(u32 value);

// socl_read.c
// len is a byte capacity; queued UDP returns the full datagram length and discards any uncopied tail unless peeking.
// UDP fills peer outputs independently; ordinary CPS requires both nonnull; SSL commands fill neither.
// flags is SOCLI_READ_*. Peek prevents UDP dequeue/CPS offset advancement, but SSL still consumes copied bytes.
int SOCLi_ReadFrom(SOCLSocket *socket, u8 *buf, int len, u16 *remote_port, CPSInAddr *remote_ip, int flags);
BOOL SOCLi_UdpRecvCallback(u8 *data, u32 len, CPSSoc *soc);

// socl_list.c
// Two counters that SOCL_CreateSocket raises and SOCL_Close lowers, for the TCP sockets and for the others (the name
// and the split are guesses)
extern s32 SOCLiSocketCount[2];
// Puts a new socket in the list of sockets (SOCLi_SocketRegister in pokediamond's NitroWiFi)
void SOCLi_SocketRegister(SOCLSocket *socket);
BOOL SOCL_SocketIsInvalid(const SOCLSocket *socket); // TRUE for a handle that is not a socket of the list
BOOL SOCL_SocketIsInTrash(const SOCLSocket *socket); // TRUE for a socket that SOCLi_CloseCallBack put in the trash
void SOCLi_SocketUnregister(SOCLSocket *socket);
void SOCLi_SocketRegisterTrash(SOCLSocket *socket);
void SOCLi_SocketUnregisterTrash(SOCLSocket *socket);
// The sockets that were closed and wait for SOCLi_TrashSocket to free them
extern SOCLSocket *SOCLiSocketList;
extern SOCLSocket *SOCLiSocketListTrash;
// Zeroes the socket counters before SOCL starts (no counterpart in pokediamond's NitroWiFi; name a guess)
void SOCLi_ResetSocketCount(void);
// CPS's address check callback: drops a gateway or DNS server that the net mask and address rule out (no counterpart in
// pokediamond's NitroWiFi; name a guess)
BOOL SOCLi_AddressCheckCallback(void);

#endif // POKEBW2_NITROWIFI_SOCL_INTERNAL_H
