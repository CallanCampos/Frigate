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
    u16 stack_size;
    u8 priority;
    u8 queue_length;
} SOCLPipeParam;

// How a socket is created: SOCLSocketParamTCP, SOCLSocketParamUDP and SOCLSocketParamUDPSend (soc.c). A socket has a
// receive pipe when rcvbuf_size is not 0 and a send pipe when sndbuf_size is not 0; without one it uses the UDP send
// socket's.
typedef struct SOCLSocketParam {
    s8 flag_mode;
    s8 flag_block;
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

extern SOCLSocketParam SOCLSocketParamTCP;
extern SOCLSocketParam SOCLSocketParamUDPSend;

// SOCL's errors, as negative return values (BSD's numbers)
#define SOCL_EAGAIN (-6)
#define SOCL_EALREADY (-7)
#define SOCL_ECONNRESET (-15)
#define SOCL_EINPROGRESS (-26)
#define SOCL_EINVAL (-28)
#define SOCL_EMFILE (-33)
#define SOCL_EISCONN (-30)
#define SOCL_ENETRESET (-39) // SOCL has not been started
#define SOCL_ENOMEM (-49)
#define SOCL_ENOTCONN (-56)
#define SOCL_ETIMEDOUT (-76)

int SOCL_Startup(SOCLConfig *config);
int SOCL_CreateSocket(const SOCLSocketParam *param);

#endif // POKEBW2_NITROWIFI_SOC_H
