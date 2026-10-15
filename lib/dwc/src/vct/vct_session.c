#include "vct_internal.h"

#include "nitro/mi.h"
#include "nitro/os.h"

// The machines the voice chat talks to. Machines ask each other for voice with small control packets, and a table of
// what each packet does to a machine's state decides whether to carry on, to refuse, or to give the application an
// event.

// The sessions that are free and in use, the clock and the random number generator's constants
typedef struct VCTSessionPool {
    VCTSession *freeList;
    VCTSession *usedList;
    u64 (*clock)(void);
    u32 randomMultiplier;
    u32 randomIncrement;
    // A session to answer from for a machine that cannot be given one, and the one session of the second mode
    VCTSession pending;
    VCTSession host;
} VCTSessionPool;

typedef struct VCTSessionState {
    // How many machines are in the voice chat, and which
    int count;
    u32 mask;
    // When the last request was sent, or 0
    u64 requestTime;
} VCTSessionState;

static VCTSession *AllocFromPool(u8 aid);
static int SendRequest(VCTSession *session, int command);
static int SendResponse(VCTSession *session, int command);
static void FillBroadcastFields(VCTSession *session, VCTControlPacket *packet);
static int SendBroadcast(VCTSession *session, VCTControlPacket *packet);
static void BroadcastCommand(int flag);
static void InitSession(VCTSession *session, u8 aid);
static BOOL SendControl(VCTSession *session, VCTControlPacket *packet);
static int GetEvent(VCTControlPacket *packet, VCTSession *session);
static void InitControlPacket(VCTSession *session, VCTControlPacket *packet, u8 command);
static int ReplyTransition(VCTSession *session, VCTControlPacket *packet);
static BOOL CanStart(VCTSession *session);
static int CheckTransition(VCTSession *session, VCTControlPacket *packet);
static VCTSession *FindSession(int aid);
static int BuildRequest(VCTSession *session, u32 command, VCTControlPacket *packet, u32 size);
static int BuildResponse(VCTSession *session, u32 command, VCTControlPacket *packet, u32 size);
static BOOL HandlePeerPacket(VCTControlPacket *packet, VCTEventResult *result);
static BOOL HandleHostPacket(VCTControlPacket *packet, VCTEventResult *result);
static BOOL HandleClosePacket(VCTControlPacket *packet, VCTEventResult *result);
static BOOL HandleCommand4(VCTControlPacket *packet, VCTEventResult *result);

// The events that a packet gives the application, by the command of the broadcast and of the direct packet
static const u8 sBroadcastEvents[5] = {1, 8, 4, 10, 0};
static const u8 sDirectEvents[7] = {7, 12, 2, 3, 9, 2, 0};

// What a control packet does to a machine in each state (a row each), by the command (a column each: those of a
// broadcast packet, then those of a direct one): the next state, -1 to take no notice, or -2 to refuse. The first two
// tables are for when the packet is a reply, the others for when it is a request, one of each for the first mode.
static const s8 sReplyTableOther[6][11] = {
    {-2, -2, -2, 3, -1, -1, 0, -1, -1, -1, -1}, {-2, -2, -2, -3, -1, 2, 0, 0, 0, -1, 0},
    {-2, -2, -2, -3, -1, -1, -1, -1, -1, -1, -1}, {-2, 0, -2, -3, -1, -1, -1, -1, -1, -1, -1},
    {-2, -2, -2, -3, -1, -1, -1, -1, -1, -1, -1}, {-2, -2, -2, -3, -1, 0, 0, -1, -1, 0, -1},
};
static const s8 sCheckTableMode1[6][11] = {
    {1, -2, -2, -2, -2, -2, 0, 0, 0, -2, -2}, {-2, -2, 5, -2, -2, -2, 0, -2, -2, -2, -2},
    {-2, 5, -2, -2, -2, -2, 0, -2, -2, -2, -2}, {-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2},
    {-2, -2, -2, -2, -2, 2, 0, 0, 0, -2, 0}, {-2, -2, -2, -2, -2, 0, 0, -2, -2, 0, -2},
};
static const s8 sCheckTableOther[6][11] = {
    {-2, -2, -2, 1, -1, -2, 0, 0, 0, -2, -2}, {-2, 5, 5, -2, -1, -2, 0, -2, -2, -2, -2},
    {-2, 5, -2, -2, -1, -2, 0, -2, -2, -2, -2}, {-2, -2, -2, -2, -1, -2, 0, -2, -2, -2, -2},
    {-2, -2, -2, -2, -1, 3, 0, 0, 0, -2, 0}, {-2, -2, -2, -2, -1, 0, 0, -2, -2, 0, -2},
};
static const s8 sReplyTableMode1[6][11] = {
    {4, -2, -2, -2, -1, -1, -1, -1, -1, -1, -1}, {-2, -2, -2, -2, -1, 2, 0, 0, 0, -1, 0},
    {-2, 5, 5, -2, -1, -1, -1, -1, -1, -1, -1}, {-2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1},
    {-2, -2, 5, -2, -1, -1, -1, -1, -1, -1, -1}, {-2, -2, -2, -2, -1, 0, 0, -1, -1, 0, -1},
};

// How long to wait for an answer, in clock ticks
static u64 sRequestTimeout = 15709687;

static VCTSessionPool sSessionPool;
static VCTSessionState sSessionState;

VCTSession *VCT_AllocSession(u8 aid)
{
    if (gVCTSettings.mode == 0 || gVCTSettings.mode == 2) {
        return NULL;
    }
    return AllocFromPool(aid);
}

BOOL VCT_ReleaseSession(VCTSession *session)
{
    VCTSession *previous = NULL;
    VCTSession *current = sSessionPool.usedList;

    if (session == NULL) {
        return FALSE;
    }
    if (gVCTSettings.mode == 2) {
        if (session == &sSessionPool.host) {
            sSessionPool.host.active = 0;
        }
        return TRUE;
    }
    while (current != NULL) {
        if (current == session) {
            session->active = 0;
            if (previous != NULL) {
                previous->next = session->next;
            } else if (current->next != NULL) {
                sSessionPool.usedList = current->next;
            } else {
                sSessionPool.usedList = NULL;
            }
            session->next = sSessionPool.freeList;
            sSessionPool.freeList = session;
            return TRUE;
        }
        previous = current;
        current = current->next;
    }
    return FALSE;
}

// Makes a control packet to send as a request, returning 0 or what is wrong with it
static int BuildRequest(VCTSession *session, u32 command, VCTControlPacket *packet, u32 size)
{
    if (session == NULL) {
        return -1;
    }
    if (size < 0x10) {
        return -1;
    }
    if (gVCTSettings.unkC == 1) {
        return -3;
    }
    if (command >= 5) {
        return -3;
    }
    packet->type = VCT_TYPE_BROADCAST;
    InitControlPacket(session, packet, command);
    if (!CheckTransition(session, packet)) {
        return -3;
    }
    if (gVCTSettings.mode == 1 && command == 0) {
        if (!CanStart(session)) {
            return -3;
        }
    }
    return 0;
}

int VCT_Request(VCTSession *session, int command)
{
    if (gVCTSettings.mode == 2) {
        return -3;
    }
    return SendRequest(session, command);
}

// Makes a control packet to send as a reply
static int BuildResponse(VCTSession *session, u32 command, VCTControlPacket *packet, u32 size)
{
    if (session == NULL) {
        return -1;
    }
    if (size < 0x10) {
        return -1;
    }
    if (command >= 6) {
        return -3;
    }
    packet->type = VCT_TYPE_CONTROL;
    InitControlPacket(session, packet, command);
    if (command == 0) {
        if (gVCTSettings.mode == 1 && session->state == 4) {
            if (!CanStart(session)) {
                return -3;
            }
        }
        session->mask = session->aid != 0 ? 1 << session->aid : 1;
    }
    if (!CheckTransition(session, packet)) {
        return -3;
    }
    return 0;
}

int VCT_Respond(VCTSession *session, int command)
{
    if (gVCTSettings.mode == 2) {
        return -3;
    }
    return SendResponse(session, command);
}

int VCT_AddSession(u8 aid)
{
    VCTSession *session;

    if (gVCTSettings.mode != 3) {
        return -2;
    }
    if (sSessionState.count == 3) {
        return -6;
    }
    if (aid == gVCTSettings.aid) {
        return -1;
    }
    if (FindSession(aid) != NULL) {
        return 0;
    }
    session = VCT_AllocSession(aid);
    if (session == NULL) {
        return -1;
    }
    session->aid = aid;
    session->origin = aid;
    session->state = 2;
    session->mask = 1 << aid;
    gVCTSettings.callback(aid, 7, session, gVCTSettings.userData);
    sSessionState.count++;
    return 0;
}

int VCT_RemoveSession(u8 aid)
{
    VCTSession *session;

    if (gVCTSettings.mode != 3) {
        return -2;
    }
    session = FindSession(aid);
    if (session == NULL) {
        return -1;
    }
    gVCTSettings.callback(aid, 9, session, gVCTSettings.userData);
    sSessionState.count--;
    return 0;
}

BOOL VCT_InitSessions(VCTConfig *config)
{
    u32 i;

    if (config->sessions == NULL || config->sessionCount == 0 || config->sessionCount > 8) {
        return FALSE;
    }
    sSessionPool.freeList = config->sessions;
    sys_memset(config->sessions, 0, config->sessionCount * sizeof(VCTSession));
    sys_memset(&sSessionPool.host, 0, sizeof(VCTSession));
    sys_memset(&sSessionPool.pending, 0, sizeof(VCTSession));
    for (i = 0; i < config->sessionCount - 1; i++) {
        sSessionPool.freeList[i].next = &sSessionPool.freeList[i + 1];
    }
    sSessionPool.freeList[config->sessionCount - 1].next = NULL;
    sSessionPool.usedList = NULL;
    sSessionState.mask = 0;
    sSessionState.count = 0;
    gVCTSettings.unk14 = 0;
    sSessionPool.clock = clock;
    sSessionPool.randomMultiplier = 0x5d588b65;
    sSessionPool.randomIncrement = 0x269ec3;
    return TRUE;
}

void VCT_ClearSessions(void)
{
    sSessionPool.usedList = NULL;
    sSessionPool.freeList = NULL;
}

// Gives up on a request that has gone unanswered for too long
void VCT_CheckTimeout(void)
{
    u64 now = clock();

    if (gVCTSettings.mode != 2) {
        return;
    }
    if (gVCTSettings.unkC == 0) {
        return;
    }
    if (sSessionState.requestTime == 0) {
        return;
    }
    if (now - sSessionState.requestTime <= sRequestTimeout) {
        return;
    }
    sSessionState.requestTime = 0;
    sSessionPool.host.active = 0;
    sSessionPool.host.state = 0;
    BroadcastCommand(0);
    gVCTSettings.callback(sSessionPool.host.aid, 9, &sSessionPool.host, gVCTSettings.userData);
}

// Takes in a control packet, returning whether it has given the application an event
BOOL VCT_HandleControlPacket(int aid, VCTControlPacket *packet, int size, VCTEventResult *result)
{
    if (size != sizeof(VCTControlPacket)) {
        return FALSE;
    }
    if (packet->from != aid || packet->to != gVCTSettings.aid) {
        return FALSE;
    }
    if (packet->length != sizeof(VCTControlPacket)) {
        return HandleClosePacket(packet, result);
    }
    if (gVCTSettings.unkC == 0) {
        return HandlePeerPacket(packet, result);
    }
    return HandleHostPacket(packet, result);
}

static VCTSession *AllocFromPool(u8 aid)
{
    VCTSession *session;

    if (aid >= VCT_AID_MAX) {
        return NULL;
    }
    if (aid == gVCTSettings.aid) {
        return NULL;
    }
    session = sSessionPool.freeList;
    if (session == NULL) {
        return NULL;
    }
    sSessionPool.freeList = session->next;
    InitSession(session, aid);
    session->next = sSessionPool.usedList;
    sSessionPool.usedList = session;
    return session;
}

static int SendRequest(VCTSession *session, int command)
{
    VCTControlPacket packet;
    int result = BuildRequest(session, command, &packet, sizeof(packet));

    if (result != 0) {
        return result;
    }
    if (!SendControl(session, &packet)) {
        return -4;
    }
    return 0;
}

static int SendResponse(VCTSession *session, int command)
{
    VCTControlPacket packet;
    int result = BuildResponse(session, command, &packet, sizeof(packet));

    if (result != 0) {
        return result;
    }
    if (!SendControl(session, &packet)) {
        return -4;
    }
    return 0;
}

static void FillBroadcastFields(VCTSession *session, VCTControlPacket *packet)
{
    if (packet->flag == 1) {
        packet->origin = session->aid;
        packet->argument = sSessionState.mask;
    }
}

// Sends a control packet to every machine of the voice chat, returning 0 if there was one it reached
static int SendBroadcast(VCTSession *session, VCTControlPacket *packet)
{
    int sent = 0;
    u8 aid;

    FillBroadcastFields(session, packet);
    for (aid = 0; aid < VCT_AID_MAX; aid++) {
        if ((sSessionState.mask & (aid != 0 ? 1 << aid : 1)) && aid != gVCTSettings.aid) {
            packet->to = aid;
            if (func_ov011_02168a08(aid, packet, sizeof(VCTControlPacket))) {
                sent++;
            }
        }
    }
    if (packet->flag == 1) {
        session->mask = sSessionState.mask;
    } else if (packet->flag == 0) {
        session->mask = 0;
    }
    return sent == 0 ? -4 : 0;
}

static void BroadcastCommand(int flag)
{
    VCTControlPacket packet;

    packet.type = VCT_TYPE_BROADCAST;
    InitControlPacket(&sSessionPool.host, &packet, 4);
    packet.flag = flag;
    SendBroadcast(&sSessionPool.host, &packet);
}

static void InitSession(VCTSession *session, u8 aid)
{
    memset(session, 0, sizeof(VCTSession));
    session->active = gVCTSettings.mode;
    session->state = 0;
    session->aid = aid;
}

static BOOL SendControl(VCTSession *session, VCTControlPacket *packet)
{
    if (packet->type == VCT_TYPE_BROADCAST && packet->command == 4) {
        SendBroadcast((VCTSession *)session, packet);
    } else {
        if (!func_ov011_02168a08(session->aid, packet, sizeof(VCTControlPacket))) {
            return FALSE;
        }
    }
    return TRUE;
}

// The event that a control packet gives the application, if any
static int GetEvent(VCTControlPacket *packet, VCTSession *session)
{
    session->origin = packet->origin;
    if (packet->type == VCT_TYPE_BROADCAST) {
        if (packet->command == 0) {
            session->mask = session->aid != 0 ? 1 << session->aid : 1;
        }
        return sBroadcastEvents[packet->command];
    } else if (packet->type == VCT_TYPE_CONTROL) {
        if (packet->command == 0) {
            if (session->state == 1) {
                session->mask = session->aid != 0 ? 1 << session->aid : 1;
                return 7;
            }
            return session->state == 5 ? 9 : 12;
        }
        return sDirectEvents[packet->command];
    }
    return 12;
}

static void InitControlPacket(VCTSession *session, VCTControlPacket *packet, u8 command)
{
    packet->magic = VCT_PACKET_MAGIC;
    packet->length = sizeof(VCTControlPacket);
    packet->command = command;
    packet->from = gVCTSettings.aid;
    packet->to = session->aid;
    packet->flag = 0;
    packet->origin = gVCTSettings.aid;
    packet->argument = 0;
}

// Moves a session on by a reply, returning what the table says
static int ReplyTransition(VCTSession *session, VCTControlPacket *packet)
{
    int column = packet->command;
    const s8(*table)[11];
    int next;

    if (packet->type == VCT_TYPE_CONTROL) {
        column += 5;
    }
    if ((u32)column >= 11) {
        column = -1;
    }
    if (column < 0) {
        return -1;
    }
    table = sReplyTableMode1;
    if (gVCTSettings.mode != 1) {
        table = sReplyTableOther;
    }
    next = table[session->state][column];
    switch (next) {
    case -1:
        return next;
    case -2:
        SendResponse(session, 1);
        session->state = 0;
        session->active = 0;
        return next;
    case -3:
        SendResponse(session, 3);
        session->state = 0;
        session->active = 0;
        return next;
    default:
        return next;
    }
}

static BOOL CanStart(VCTSession *session)
{
    VCTSession *other;

    for (other = sSessionPool.usedList; other != NULL; other = other->next) {
        if (other->active != 0 && other->state == 2 && session != other) {
            return FALSE;
        }
    }
    return TRUE;
}

// Moves a session on by a request, returning 0 if it is refused
static int CheckTransition(VCTSession *session, VCTControlPacket *packet)
{
    int column = packet->command;
    const s8(*table)[11];
    int next;

    if (packet->type == VCT_TYPE_CONTROL) {
        column += 5;
    }
    if ((u32)column >= 11) {
        column = -1;
    }
    if (column < 0) {
        return -1;
    }
    table = gVCTSettings.mode == 1 ? sCheckTableMode1 : sCheckTableOther;
    next = table[session->state][column];
    if (next == -1) {
        return TRUE;
    }
    if (next == -2) {
        return FALSE;
    }
    session->state = next;
    return TRUE;
}

static VCTSession *FindSession(int aid)
{
    VCTSession *session = sSessionPool.usedList;

    if (gVCTSettings.mode == 2) {
        if (sSessionPool.host.active != 0 && sSessionPool.host.aid == aid) {
            return &sSessionPool.host;
        }
        return NULL;
    }
    for (; session != NULL; session = session->next) {
        if (session->active != 0 && session->aid == aid) {
            return session;
        }
    }
    return NULL;
}

static BOOL HandlePeerPacket(VCTControlPacket *packet, VCTEventResult *result)
{
    VCTSession *session;
    int next;

    if (packet->type == VCT_TYPE_BROADCAST) {
        if (packet->command == 4) {
            return HandleCommand4(packet, result);
        }
        if (packet->command == 3) {
            return FALSE;
        }
    }
    session = FindSession(packet->from);
    if (session != NULL) {
        next = ReplyTransition(session, packet);
        switch (next) {
        case -1:
            return FALSE;
        case -2:
            result->event = 12;
            session->active = 0;
            result->session = session;
            return TRUE;
        }
        result->event = GetEvent(packet, session);
        result->session = session;
        session->state = next;
        return result->event != 0;
    }
    session = AllocFromPool(packet->from);
    if (session == NULL) {
        InitSession(&sSessionPool.pending, packet->from);
        SendResponse(&sSessionPool.pending, 3);
        return FALSE;
    }
    next = ReplyTransition(session, packet);
    if (next == -2 || next == -1) {
        VCT_ReleaseSession(session);
        return FALSE;
    }
    result->event = GetEvent(packet, session);
    result->session = session;
    session->state = next;
    return result->event != 0;
}

static BOOL HandleHostPacket(VCTControlPacket *packet, VCTEventResult *result)
{
    VCTSession *host = &sSessionPool.host;
    int next;

    if (sSessionPool.host.active == 2) {
        if (sSessionPool.host.aid == packet->from) {
            next = ReplyTransition(host, packet);
            switch (next) {
            case -2:
                result->event = 12;
                result->session = NULL;
                return TRUE;
            case -3:
            case -1:
                return FALSE;
            }
        } else {
            BroadcastCommand(1);
            return FALSE;
        }
    } else {
        if (!(sSessionState.mask & (packet->from != 0 ? 1 << packet->from : 1))) {
            return FALSE;
        }
        sSessionPool.host.aid = packet->from;
        sSessionPool.host.active = 2;
        sSessionPool.host.state = 0;
        next = ReplyTransition(host, packet);
        switch (next) {
        case -3:
        case -2:
        case -1:
            sSessionPool.host.active = 0;
            return FALSE;
        }
    }
    result->event = GetEvent(packet, host);
    result->session = host;
    host->state = next;
    if (result->event == 10) {
        BroadcastCommand(1);
        sSessionState.requestTime = clock();
    } else {
        BroadcastCommand(0);
        sSessionState.requestTime = 0;
    }
    return TRUE;
}

static BOOL HandleClosePacket(VCTControlPacket *packet, VCTEventResult *result)
{
    VCTSession *session;

    if (packet->type == VCT_TYPE_BROADCAST && packet->command == 4) {
        return FALSE;
    }
    session = FindSession(packet->from);
    if (session != NULL) {
        result->event = 2;
        result->session = session;
        session->active = 0;
        SendResponse(session, 2);
        return TRUE;
    }
    InitSession(&sSessionPool.pending, packet->from);
    SendResponse(&sSessionPool.pending, 2);
    return FALSE;
}

static BOOL HandleCommand4(VCTControlPacket *packet, VCTEventResult *result)
{
    if (gVCTSettings.mode != 2) {
        return FALSE;
    }
    if (gVCTSettings.hostAid != packet->from) {
        return FALSE;
    }
    switch (packet->flag) {
    case 0:
        if (sSessionPool.host.active == 0) {
            return FALSE;
        }
        if (sSessionPool.host.state == 5 || sSessionPool.host.state == 2) {
            result->event = 9;
        } else {
            result->event = 5;
        }
        sSessionPool.host.active = 0;
        sSessionPool.host.state = 0;
        result->session = &sSessionPool.host;
        break;
    case 1:
        sSessionPool.host.active = 2;
        if (packet->origin == gVCTSettings.aid) {
            sSessionPool.host.state = 2;
            result->event = 7;
            sSessionPool.host.mask = packet->argument;
        } else {
            sSessionPool.host.state = 3;
            result->event = 6;
            sSessionPool.host.mask = packet->origin != 0 ? 1 << packet->origin : 1;
        }
        sSessionPool.host.origin = packet->origin;
        result->session = &sSessionPool.host;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
