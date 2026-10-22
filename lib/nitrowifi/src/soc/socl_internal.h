#ifndef POKEBW2_NITROWIFI_SOCL_INTERNAL_H
#define POKEBW2_NITROWIFI_SOCL_INTERNAL_H

#include "nitrowifi/soc.h"

// SOCL's state, shared by its files

// NitroWiFi's signature, "[SDK+NINTENDO:WiFi3.3.30052.201006221129]", in the main module
extern const char SOCLi_Signature[];

// socl_startup.c
extern u32 SOCLiDefaultLanBufferSize; // The LAN buffer's size when the config gives none
extern CPSInAddr SOCLiRequestedIP;    // The address that DHCP asks for
extern int SOCLiDhcpState;            // SOCL_DHCP_*
extern CPSConfig SOCLiCPSConfig;

// SOCLiDhcpState: DHCP is in use, and it has finished (or the address is static)
#define SOCL_DHCP_REQUEST 1
#define SOCL_DHCP_CALLBACK 2

// A command pipe: the queue of the packets that its thread runs (the socket's fields at 0xa0 and 0xa4 point to its
// receive and send pipes, and a socket without a receive pipe uses the send pipe for control). Its messages and the
// thread's stack follow the pipe in the socket's memory.
typedef struct SOCLiSocketCommandPipe {
    OSMessageQueue queue;
    OSThread thread;
    OSMutex mutex;
} SOCLiSocketCommandPipe;

// A datagram that SOCLi_UdpRecvCallback queued, followed by its data
typedef struct SOCLiSocketUdpData {
    struct SOCLiSocketUdpData *next;
    u16 len;
    u16 port;
    CPSInAddr ip;
    u8 data[];
} SOCLiSocketUdpData;

// The receive pipe, 0x114 bytes
typedef struct SOCLiSocketRecvCommandPipe {
    SOCLiSocketCommandPipe base;
    // What the reads have taken from CPS's receive buffer and not yet consumed. volatile: the pipe's thread and the
    // readers both change it, and SOCLi_ReadCallBack's load of it is not moved (ntrtwl's NitroWiFi has it volatile too)
    vs32 cps_read_len;
    u16 rcvbuf_consume_min; // From SOCLSocketParam: how much to take before CPS_SocConsume
    s8 peek;                // Suppresses UDP dequeue/CPS offset advances; SSL command reads ignore it
    u8 unk_ff;
    // The datagrams that came in, oldest first. volatile: CPS's callback appends while readers take them;
    // SOCLi_UdpRecvCallback loads the tail again for its store, and SOCLi_GetReadBufferOccpiedSize reads the head once
    // into a local (ntrtwl's NitroWiFi has both ends and the byte count volatile)
    SOCLiSocketUdpData *volatile udp_tail;
    SOCLiSocketUdpData *volatile udp_head;
    vu16 udp_queued;        // The bytes of datagram data they hold (volatile like the list ends)
    u16 udp_queued_max;     // From SOCLSocketParam's rcvpipe_unk_e
    OSThreadQueue udp_wait; // Where a blocking UDP read sleeps
} SOCLiSocketRecvCommandPipe;

struct SOCLSocket;

// The send pipe, 0x110 bytes
typedef struct SOCLiSocketSendCommandPipe {
    SOCLiSocketCommandPipe base;
    CPSSocBuf buffer;
    // Where the next write goes in the buffer (a ring). volatile: the writers move it while the pipe's thread reads it
    volatile u16 write_pos;
    vu16 read_pos;      // How far the pipe's thread has taken data out of it; volatile: the writers read it meanwhile
    OSThreadQueue wait; // Where a blocking write sleeps until the buffer has room
    struct SOCLSocket *socket; // The socket that owns the pipe (a UDP socket shares the UDP send socket's)
} SOCLiSocketSendCommandPipe;

// A socket, 0xbc bytes, followed in memory by its pipes and buffers
typedef struct SOCLSocket {
    CPSSoc cps_socket;                     // Its buffers are in the socket's memory, after the pipes
    SOCLiSocketRecvCommandPipe *recv_pipe; // 0xa0, NULL for a socket that does not read
    SOCLiSocketSendCommandPipe *send_pipe; // A UDP socket's is the UDP send socket's
    vs32 result; // What the last command packet returned; volatile: the pipe's thread sets it while callers wait
    // SOCL_STATUS_*. volatile: the command pipes' threads change it while callers test it, and the original reloads it
    // for each test, with no call or store in between (SOCL_GetStatus loads it twice in a row)
    volatile s16 state;
    s8 flag_block;
    s8 flag;        // The mode (SOCLSocketParam's flag_mode), copied into each command packet
    u16 local_port; // 0xb0
    u16 remote_port;
    CPSInAddr remote_ip;
    struct SOCLSocket *next; // The next socket of the list it is in
} SOCLSocket;

// socl_startup.c: the socket that sends every UDP socket's datagrams
extern int SOCLiUDPSendSocket;

// SOCL_ReadFrom's and SOCL_WriteTo's flags (BSD's MSG_PEEK and MSG_DONTWAIT)
#define SOCL_MSG_PEEK 2     // Retain UDP datagrams/plain-CPS read position; SSL reads still consume copied bytes
#define SOCL_MSG_DONTWAIT 4 // Do not wait for data to read or for room in the send buffer

// SOCLSocket.state
#define SOCL_STATUS_CREATE 1
#define SOCL_STATUS_CONNECTING 2 // SOCLi_ExecBindCommand has queued the bind (and connect)
#define SOCL_STATUS_CONNECTED 4  // The bind (and for TCP the connect) succeeded
#define SOCL_STATUS_CLOSING 8
#define SOCL_STATUS_WAIT_CLOSE 0x10   // SOCL_Close has run (it queued the close command)
#define SOCL_STATUS_WAIT_RELEASE 0x20 // The close command has run: the socket waits in the trash list
#define SOCL_STATUS_ERROR 0x40        // The connect failed, with the error in the socket's result
#define SOCL_STATUS_RESET 0x80        // The connection was reset

static inline BOOL SOCL_SocketIsCreated(const SOCLSocket *socket) {
    BOOL created = FALSE;
    if (socket != NULL) {
        if (socket->state & SOCL_STATUS_CREATE) {
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

static inline BOOL SOCL_SocketIsTCP(const SOCLSocket *socket) {
    BOOL tcp = TRUE;
    if (socket->flag != 0 && socket->flag != 4) {
        tcp = FALSE;
    }
    return tcp;
}

// A command's handler, run by the pipe's thread: it takes the packet and reads it as its own command's struct
typedef int (*SOCLiCommandHandler)(void *arg);

// The command packets, in the shape of ntrtwl's NitroWiFi: every command starts with this 0x10-byte header and its
// arguments follow it. The type names are ntrtwl's (SOCLiCommand*); the fields keep the names this code gave them.
typedef struct SOCLiCommandHeader {
    SOCLiCommandHandler handler;
    SOCLSocket *socket;
    OSMessageQueue *reply_queue; // Borrowed caller-stack result queue for mode 1, NULL otherwise
    s8 socket_flag;
    s8 flag_block; // Exactly 1 waits for the result; bit 0 blocks queue admission (writes use 3 or 0)
    u8 padding[2];
} SOCLiCommandHeader;

typedef struct SOCLiCommandCreateSocket {
    SOCLiCommandHeader h;
} SOCLiCommandCreateSocket;

typedef struct SOCLiCommandBind {
    SOCLiCommandHeader h;
    u16 local_port;
    u16 remote_port;
    CPSInAddr remote_ip;
} SOCLiCommandBind;

typedef struct SOCLiCommandRead {
    SOCLiCommandHeader h;
    u8 *buf;
    s32 len;
    u16 *remote_port; // Optional peer outputs, filled according to the selected receive path
    CPSInAddr *remote_ip;
} SOCLiCommandRead;

typedef struct SOCLiCommandConsume {
    SOCLiCommandHeader h;
} SOCLiCommandConsume;

typedef struct SOCLiCommandWrite {
    SOCLiCommandHeader h;
    u8 *buf1; // Owned send-ring slices used by the queued callback, not application storage
    s32 len1; // Slice byte counts; buf2 is the wrapped part, not an application-buffer pointer
    u8 *buf2;
    s32 len2;
    u16 write_end; // The pipe's write position after this data
    u8 padding[2];
    u16 local_port; // For a UDP packet, the datagram's ports and address
    u16 remote_port;
    CPSInAddr remote_ip; // 0 for TCP
} SOCLiCommandWrite;

typedef struct SOCLiCommandShutdown {
    SOCLiCommandHeader h;
} SOCLiCommandShutdown;

typedef struct SOCLiCommandClose {
    SOCLiCommandHeader h;
} SOCLiCommandClose;

typedef struct SOCLiCommandEnableSsl {
    SOCLiCommandHeader h;
    void *info; // Borrowed connection retained by CPS; NULL detaches without freeing/resetting it
} SOCLiCommandEnableSsl;

// A command for a socket's pipe: the pool's packets are all the size of the largest command, 0x2c bytes (the write)
typedef union SOCLiCommandPacket {
    SOCLiCommandHeader h;
    SOCLiCommandCreateSocket create_socket;
    SOCLiCommandBind bind;
    SOCLiCommandRead read;
    SOCLiCommandConsume consume;
    SOCLiCommandWrite write;
    SOCLiCommandShutdown shutdown;
    SOCLiCommandClose close;
    SOCLiCommandEnableSsl enable_ssl;
} SOCLiCommandPacket;

// socl_command.c
int SOCLi_StartupCommandPacketQueue(s32 max);
int SOCLi_CleanupCommandPacketQueue(void);
SOCLiCommandPacket *SOCLi_AllocCommandPacket(s32 flags);
SOCLiCommandPacket *SOCLi_CreateCommandPacket(SOCLiCommandHandler handler, SOCLSocket *socket, s32 flag_block);
void SOCLi_FreeCommandPacket(SOCLiCommandPacket *packet);
SOCLiSocketCommandPipe *SOCLi_GetCtrlPipe(SOCLSocket *socket);
int SOCLi_SendCommandPacket(SOCLiSocketCommandPipe *pipe, SOCLiCommandPacket *packet);
int SOCLi_SendCommandPacketToCtrlPipe(SOCLSocket *socket, SOCLiCommandPacket *packet);
int SOCLi_ExecCommandPacketInRecvPipe(SOCLSocket *socket, SOCLiCommandPacket *packet);
int SOCLi_ExecCommandPacketInSendPipe(SOCLSocket *socket, SOCLiCommandPacket *packet);
int SOCLi_ExecCommandPacketInCtrlPipe(SOCLSocket *socket, SOCLiCommandPacket *packet);
void SOCLi_CommandPacketHandler(void *arg);

// socl_bind.c
// SOCL ports/IP scalars are in host order; SOC wrappers convert their network-order address fields.
int SOCL_Bind(int s, u16 local_port);
int SOCL_Connect(int s, u16 remote_port, CPSInAddr remote_ip);

// socl_write.c
// Copies caller payload into the send ring before queue admission; caller storage is not retained after return.
// Returns admitted byte count, not callback/transmission completion. Nonblocking writes may return a prefix;
// a later blocking-loop admission failure returns EAGAIN even if an earlier prefix was queued.
// UDP copies destination scalars, using the stored peer only when remote_ip is zero and the stored IP is nonzero.
// flags is SOCL_MSG_*; both command modes used by writes (3/0) return without waiting for callback completion.
int SOCL_WriteTo(int s, const void *buf, int len, u16 remote_port, CPSInAddr remote_ip, int flags);
// How much room the socket's send pipe has in its buffer
s32 SOCLi_GetWriteBufferFreeSize(SOCLSocket *socket);

// socl_shutdown.c
// Retains the allocation. Exact flag_block==1 waits for the callback; other modes report admission, not completion.
int SOCL_Shutdown(int s);

// socl_close.c
// Tests nonnegative handle absence from live/trash lists, not historical allocation ownership or generation.
BOOL SOCL_IsClosed(int socket);
// Returns without awaiting callback/reclamation; zero also covers an already absent positive handle.
int SOCL_Close(int socket);

// socl_cleanup.c
int SOCL_Cleanup(void);
CPSInAddr SOCL_Resolve(const char *name);
CPSInAddr SOCL_InetAtoH(const char *name);

// The socket's local port, or 0 for no socket
static inline u16 SOCL_GetHostPort(int sock) {
    SOCLSocket *socket = (SOCLSocket *)sock;

    return sock ? socket->local_port : 0;
}

int SOCL_SetResolver(const CPSInAddr dns1, const CPSInAddr dns2);
// Shuts CPS down after a failed SOCL_Startup, and in SOCL_Cleanup (no counterpart in pokediamond's NitroWiFi, whose
// SOCL_Cleanup does it inline; name a guess)
void SOCLi_CleanupCPS(void);

// socl_poll.c
// SOCL_GetStatus's bits
#define SOCL_POLLRDNORM 0x01 // Data to read (or a datagram)
#define SOCL_POLLWRNORM 0x08 // Room in the send buffer
#define SOCL_POLLERR 0x20    // The connect failed
#define SOCL_POLLHUP 0x40    // Neither connecting nor connected
#define SOCL_POLLNVAL 0x80   // Not a socket
int SOCL_GetStatus(int s);

// socl_misc.c
u32 SOCLi_RoundUp4(u32 value);

// socl_read.c
// len is a byte capacity; queued UDP returns the full datagram length and discards any uncopied tail unless peeking.
// UDP fills peer outputs independently; ordinary CPS requires both nonnull; SSL commands fill neither.
// flags is SOCL_MSG_*. Peek prevents UDP dequeue/CPS offset advancement, but SSL still consumes copied bytes.
int SOCL_ReadFrom(int s, void *buf, int len, u16 *remote_port, CPSInAddr *remote_ip, int flags);
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
