#ifndef POKEBW2_DWC_TRANSPORT_H
#define POKEBW2_DWC_TRANSPORT_H

#include "types.h"
#include "dwc/dwc.h"
#include "dwc/gt2Main.h"

// DWC's transport between the machines of a match (dwc_transport.c, in overlay 11): it sends the game's data to a
// machine over its GT2 connection, in pieces no larger than the connection takes, keeps what GT2 could not take to send
// again, and puts the pieces it receives back together in the game's buffer. The ROM names none of it; the types take
// the names that TWL-DWC gives them (DWCTransportInfo, recalled from the SDK), and the functions keep their addresses.
// The game's functions (sending, the receive buffers, the callbacks and the receive timeouts) are in dwc/dwc.h.

// The function called with the latency of a ping to a machine, in milliseconds
typedef void (*DWCPingFunc)(int latency, u8 aid, void *param);

// What the transport keeps for each machine of the match: the data being sent and how much of it is sent, the game's
// receive buffer, how much of the data being received has come and its size, whether data is being sent, the state of
// the receiving (0 none, 1 waiting for a header, 2 receiving into the buffer, 3 receiving the match's own data, 4
// skipping data that does not fit), the state before the match's own data, the kind of the last header, and the
// times the receive timeout counts from (with the timeout in milliseconds, 0 for none) and of the last data received
typedef struct DWCTransportConnection {
    const u8 *sendBuffer;
    u8 *recvBuffer;
    int recvBufferSize;
    int sendingSize;
    int recvingSize;
    int requestSendSize;
    int requestRecvSize;
    u8 sendState;
    u8 recvState;
    u8 lastRecvState;
    u8 pad;
    u16 unk_20;
    u16 lastRecvType;
    u64 recvTimeoutTime;
    u32 recvTimeout;
    u64 lastRecvTime;
} DWCTransportConnection;

// The transport's state, which the match gives memory for: a connection for each aid, the game's callbacks with their
// parameters, the largest piece that is sent at once and the time after which a silent machine is dropped, in
// milliseconds
typedef struct DWCTransportInfo {
    DWCTransportConnection connections[32];
    DWCSendFunc sendCallback;
    DWCRecvFunc recvCallback;
    DWCTimeoutFunc recvTimeoutCallback;
    DWCPingFunc pingCallback;
    void *sendParam;
    void *recvParam;
    void *recvTimeoutParam;
    void *pingParam;
    u16 sendSplitMax;
    u32 keepAliveTimeout;
} DWCTransportInfo;

// Sends the game's data to a machine, in pieces, returning FALSE if it can't be sent now; and sends data of the voice
// chat unreliably, at most 0x5b6 bytes. lib/dwc/src/vct/vct_internal.h declares both for libVCT too
BOOL func_ov011_02168a08(u8 aid, const void *data, int size);
BOOL func_ov011_02168b18(u8 aid, const void *data, int size);
// Sends data of a kind (1 the game's, 2 to 4 the match's own) to a machine, returning FALSE if it can't be sent now
BOOL func_ov011_02168a1c(int type, u8 aid, const void *data, int size);
// Starts the transport in the memory the match gives it, and ends it
void func_ov011_02168d0c(DWCTransportInfo *info);
void func_ov011_02168fec(void);
// GT2's callbacks of a connection: a message received, and the latency of a ping
void func_ov011_02168d40(GTI2Connection *connection, const void *message, int length, BOOL reliable);
void func_ov011_02168d80(GTI2Connection *connection, u32 latency);
// Runs the transport for a frame: drops the machines that sent nothing for too long, calls the receive timeouts and
// sends the next piece of the data being sent
void func_ov011_02168db0(void);
// Clears the sending and receiving of a machine, and notes that data has just come from it
void func_ov011_02168fa4(u8 aid);
void func_ov011_02169000(u8 aid);
// Sends data from a machine (from) to another unreliably, through the server when there is no connection to it,
// marked as relayed when relay is TRUE
void func_ov011_021690a4(u8 from, u8 aid, const void *data, int size, BOOL relay);

#endif // POKEBW2_DWC_TRANSPORT_H
