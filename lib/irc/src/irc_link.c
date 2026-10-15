// Overlay 31, the third of its four files (see net_devirc.c): the link of frames between the two machines. One pings,
// the other answers, and then they send each other frames that carry a checksum, over the SPI bus of irc_spi.c.
// Built at -O2,p as part of lib/irc (see its library.toml), whose name is ours.
#include "irc/irc_link.h"
#include "irc_spi.h"
#include "nitro/os.h"

// How long a ping may go unanswered, in milliseconds
#define IRC_PING_TIMEOUT 100

// Where the link is on the way to a connection: the machine that pings is the parent and the one that answers the child
enum {
    IRC_STATE_IDLE,
    IRC_STATE_PING_SENT,
    IRC_STATE_REPLIED,
    IRC_STATE_PARENT_LINKED,
    IRC_STATE_CHILD_LINKED,
};

// The state of the link of frames
typedef struct {
    u8 unk00;
    u32 errorCount;
    // Called with each frame of data, with the number of bytes, its command and its ID
    void (*recvCallback)(u8 *data, u8 size, u8 cmd, u8 id);
    // Called when the link ends, and when the other side accepts it
    void (*closeCallback)(int a0);
    // Never set. The call keeps the peer's ID in r0 and a zero in r1, so it takes both
    void (*connectCallback)(u8 id, int a1);
    BOOL resend;
    // Whether this machine pinged and got the reply
    BOOL isParent;
    BOOL pinging;
    int state;
    BOOL linked;
    u64 tick;
} IrcLinkData;

// The head of a frame: the command, the ID of the link and the checksum
typedef struct {
    u8 cmd;
    u8 id;
    u8 sumLo;
    u8 sumHi;
} IrcFrameHeader;

// A frame as it goes over the infrared: its head and the data
typedef struct {
    IrcFrameHeader header;
    u8 data[0xb4];
} IrcFrameData;

// The IDs the two machines of the link use for its frames
typedef struct {
    u8 local;
    u8 peer;
} IrcLinkIdData;

static u16 IrcLink_Checksum(const u8 *data, u32 size);
static void IrcLink_OnError(void);
static void IrcLink_ClearState(void);
static void IrcLink_SendFrame(const u8 *data, u8 size, u8 cmd, u8 id);
static void IrcLink_Finish(int a0);
static void IrcLink_Nop(void);
static BOOL IrcLink_IsRecvIgnored(u8 size);
static BOOL IrcLink_CheckPingTimeout(void);
static BOOL IrcLink_ValidateFrame(u8 size);
static void IrcLink_SetLocalId(u8 id);

static IrcLinkIdData IrcLinkIds = { 0xff, 0xff };
static IrcLinkData IrcLink;
static IrcFrameData IrcFrame;

static u16 IrcLink_Checksum(const u8 *data, u32 size) {
    u32 sum = 0x80;
    u32 i = 0;

    // The loop is tested before its first pass and then at its end, which at -O2,p takes a guarded do/while: a for
    // loop is only tested at its end
    if (i < size) {
        do {
            u8 byte = *data++;

            if (i & 1) {
                sum += byte;
            } else {
                sum += byte << 8;
            }
        } while (++i < size);
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return sum;
}

static void IrcLink_OnError(void) {
    IrcLink.resend = TRUE;
    IrcLink.errorCount++;
}

static void IrcLink_ClearState(void) {
    IrcLink.unk00 = 0;
    IrcLink.state = IRC_STATE_IDLE;
    IrcLink.linked = FALSE;
    IrcLink.resend = FALSE;
    IrcLink.pinging = FALSE;
    IrcLink.isParent = FALSE;
    IrcLink.errorCount = 0;
}

void IrcLink_Reset(u8 id) {
    IrcSpi_Init();
    IrcSpi_Recv((u8 *)&IrcFrame);
    IrcLink_ClearState();
    IrcLink_SetLocalId(id);
    IrcLinkIds.peer = 0xff;
    IrcLink.connectCallback = NULL;
    IrcLink.recvCallback = NULL;
    IrcLink.closeCallback = NULL;
}

void IrcLink_Ping(void) {
    if (!IrcLink.linked && !IrcLink.pinging) {
        u8 ping;

        IrcLink.pinging = TRUE;
        ping = IRC_CMD_PING ^ IRC_XOR;
        IrcSpi_Send(&ping, 1);
        IrcLink.state = IRC_STATE_PING_SENT;
        IrcLink.tick = clock();
    }
}

BOOL IrcLink_IsParent(void) {
    return IrcLink.isParent;
}

BOOL IrcLink_IsLinked(void) {
    return IrcLink.linked;
}

static void IrcLink_SendFrame(const u8 *data, u8 size, u8 cmd, u8 id) {
    IrcFrameHeader *header = &IrcFrame.header;
    u8 *dst;
    u8 *raw;
    int i;
    u16 sum;

    header->id = id;
    header->cmd = cmd;
    header->sumLo = 0;
    header->sumHi = 0;
    dst = IrcFrame.data;
    for (i = 0; i < size; i++) {
        *dst++ = *data++;
    }
    size += sizeof(IrcFrameHeader);
    raw = (u8 *)&IrcFrame;
    sum = IrcLink_Checksum(raw, size);
    header->sumLo = sum;
    header->sumHi = sum >> 8;
    for (i = 0; i < size; i++) {
        raw[i] ^= IRC_XOR;
    }
    // The original loads IrcFrame's address again for the call rather than passing raw
    IrcSpi_Send((u8 *)&IrcFrame, size);
}

static void IrcLink_Finish(int a0) {
    if (IrcLink.linked && !IrcLink.isParent && a0 == 0) {
        IrcLink_SendFrame(NULL, 0, IRC_CMD_CLOSE, IrcLinkIds.local);
    }
    if (IrcLink.closeCallback != NULL) {
        IrcLink.closeCallback(a0);
    }
    IrcLink_ClearState();
}

void IrcLink_Close(void) {
    IrcLink_Finish(0);
    IrcSpi_Release();
}

void IrcLink_Send(const u8 *data, u8 size, u8 cmd) {
    if (IrcLink.linked) {
        IrcLink_SendFrame(data, size, cmd, IrcLinkIds.local);
    }
}

static void IrcLink_Nop(void) {
}

static BOOL IrcLink_IsRecvIgnored(u8 size) {
    u32 i;

    if (size == 1) {
        if (IrcLink.linked) {
            return TRUE;
        }
        if (IrcFrame.header.cmd != IRC_CMD_PING) {
            return TRUE;
        }
        return FALSE;
    }
    switch (size) {
    case 2:
    case 3:
        // Empty loops, probably of debug output compiled out; -O2,p keeps them where -O4,p drops them
        for (i = 0; i < 4; i++) {
        }
        for (i = 0; i < size; i++) {
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL IrcLink_CheckPingTimeout(void) {
    if (!IrcLink.linked && !IrcLink.pinging) {
        return FALSE;
    }
    if (OS_TicksToMilliSeconds(clock() - IrcLink.tick) < IRC_PING_TIMEOUT) {
        return FALSE;
    }
    IrcLink_Finish(1);
    IrcLink.pinging = FALSE;
    return TRUE;
}

static BOOL IrcLink_ValidateFrame(u8 size) {
    IrcFrameData *frame = &IrcFrame;
    u8 *raw = (u8 *)&IrcFrame;
    u32 i;
    u16 stored;
    u16 sum;

    if (size == 1) {
        if (raw[0] == IRC_CMD_PING) {
            return TRUE;
        }
        return FALSE;
    }
    if (size < sizeof(IrcFrameHeader)) {
        return FALSE;
    }
    stored = frame->header.sumLo + (frame->header.sumHi << 8);
    frame->header.sumLo = 0;
    frame->header.sumHi = 0;
    sum = IrcLink_Checksum(raw, size);
    frame->header.sumLo = stored;
    frame->header.sumHi = stored >> 8;
    if (stored == sum) {
        IrcLink.unk00 = 0;
        return TRUE;
    }
    if (IrcLink.linked) {
        for (i = 0; i < size; i++) {
        }
        IrcLink_OnError();
    }
    return FALSE;
}

BOOL IrcLink_Poll(void) {
    IrcFrameData *frame;
    u8 *raw;
    u8 size = IrcSpi_Recv((u8 *)&IrcFrame);

    if (IrcLink_IsRecvIgnored(size)) {
        size = 0;
    }
    if (size == 0) {
        if (IrcLink_CheckPingTimeout()) {
            return TRUE;
        }
        return FALSE;
    }
    if (!IrcLink_ValidateFrame(size)) {
        return FALSE;
    }
    IrcLink.tick = clock();
    frame = &IrcFrame;
    raw = (u8 *)&IrcFrame;
    if (raw[0] < IRC_CMD_MIN) {
        if (!IrcLink.linked) {
            return FALSE;
        }
        if (raw[1] != IrcLinkIds.local) {
            return FALSE;
        }
    }
    switch (frame->header.cmd) {
    case IRC_CMD_PING:
        switch (IrcLink.state) {
        case IRC_STATE_IDLE:
            break;
        case IRC_STATE_PING_SENT:
            IrcLink.state = IRC_STATE_REPLIED;
            IrcLink_SendFrame(NULL, 0, IRC_CMD_REPLY, IrcLinkIds.local);
            break;
        case IRC_STATE_REPLIED:
        case IRC_STATE_PARENT_LINKED:
        case IRC_STATE_CHILD_LINKED:
            break;
        }
        if (!IrcLink.pinging) {
            IrcLink.pinging = TRUE;
            IrcLink.state = IRC_STATE_REPLIED;
            IrcLink.isParent = FALSE;
            if (size <= 1 || IrcLink.linked) {
                IrcLink_SendFrame(NULL, 0, IRC_CMD_REPLY, IrcLinkIds.local);
                if (IrcLink.linked) {
                    IrcLink_OnError();
                }
                IrcLink_Nop();
            }
        }
        break;
    case IRC_CMD_REPLY:
        if (IrcLink.state == IRC_STATE_PING_SENT) {
            IrcLink.state = IRC_STATE_PARENT_LINKED;
            IrcLink.isParent = TRUE;
            IrcLinkIds.peer = frame->header.id;
            if (frame->header.id == IrcLinkIds.local) {
                IrcLink_SendFrame(NULL, 0, IRC_CMD_ACCEPT, IrcLinkIds.local);
                IrcLink.linked = TRUE;
                IrcLink.resend = FALSE;
                IrcLink.pinging = FALSE;
            }
        }
        break;
    case IRC_CMD_ACCEPT:
        if (IrcLink.state == IRC_STATE_REPLIED) {
            IrcLink.state = IRC_STATE_CHILD_LINKED;
            IrcLinkIds.peer = frame->header.id;
            IrcLink.linked = TRUE;
            IrcLink.resend = FALSE;
            IrcLink.pinging = FALSE;
            if (IrcLink.connectCallback != NULL) {
                IrcLink.connectCallback(frame->header.id, 0);
            }
        }
        break;
    case IRC_CMD_RESEND:
        IrcLink_OnError();
        break;
    default:
        if (IrcLink.resend != TRUE && IrcLink.linked) {
            IrcLink.recvCallback(IrcFrame.data, size - sizeof(IrcFrameHeader), IrcFrame.header.cmd, IrcFrame.header.id);
        }
        break;
    }
    return FALSE;
}

void IrcLink_SetRecvCallback(void (*callback)(u8 *data, u8 size, u8 cmd, u8 id)) {
    IrcLink.recvCallback = callback;
}

static void IrcLink_SetLocalId(u8 id) {
    IrcLinkIds.local = id;
}

u8 IrcLink_GetPeerId(void) {
    return IrcLinkIds.peer;
}
