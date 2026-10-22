#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_internal.h"
#include "vct_session_internal.h"

// The machines the voice chat talks to. Machines ask each other for voice with small control packets, and a table of
// what each packet does to a machine's state decides whether to carry on, to refuse, or to give the application an
// event.

// The sessions that are free and in use, the clock and the random number generator's constants
struct SessionInfo {
    VCTSession *free;
    VCTSession *use;
    u64 (*clock)(void);
    u32 randomMultiplier;
    u32 randomIncrement;
    // A session to answer from for a machine that cannot be given one, and the one session of the second mode
    VCTSession tmp;
    VCTSession trans;
};

struct VCTSessionState {
    // How many machines are in the voice chat, and which
    int count;
    u32 mask;
};

static VCTSession *vct_create_session_impl(u8 aid);
static int vct_request_impl(VCTSession *session, int command);
static int vct_response_impl(VCTSession *session, int command);
static void vct_add_notify_aid_list(VCTSession *session, SSPHeader *packet);
static int vct_send_notify_impl(VCTSession *session, SSPHeader *packet);
static void vct_send_notify(int flag);
static void vct_init_session(VCTSession *session, u8 aid);
static BOOL vct_send_ssp(VCTSession *session, SSPHeader *packet);
static int vct_dispatch_session(SSPHeader *packet, VCTSession *session);
static int vct_build_ssp_header(VCTSession *session, SSPHeader *packet, u8 command);
static int vct_check_recv_state(VCTSession *session, SSPHeader *packet);
static BOOL vct_check_multisession(VCTSession *session);
static int vct_check_send_state(VCTSession *session, SSPHeader *packet);
static VCTSession *vct_find_session(int aid);
static int vct_build_request_data(VCTSession *session, u32 command, SSPHeader *packet, u32 size);
static int vct_build_response_data(VCTSession *session, u32 command, SSPHeader *packet, u32 size);
static BOOL vct_handle_ssp_client(SSPHeader *packet, VCTResult *result);
static BOOL vct_handle_ssp_server(SSPHeader *packet, VCTResult *result);
static BOOL vct_response_reject(SSPHeader *packet, VCTResult *result);
static BOOL vct_handle_notify(SSPHeader *packet, VCTResult *result);

// The events that a packet gives the application, by the command of the broadcast and of the direct packet
static const u8 sBroadcastEvents[5] = { 1, 8, 4, 10, 0 };
static const u8 sDirectEvents[7] = { 7, 12, 2, 3, 9, 2, 0 };

// What a control packet does to a machine in each state (a row each), by the command (a column each: those of a
// broadcast packet, then those of a direct one): the next state, -1 to take no notice, or -2 to refuse. The first two
// tables are for when the packet is a reply, the others for when it is a request, one of each for the first mode.
static const s8 sRecvState[6][11] = {
    { 4, -2, -2, -2, -1, -1, -1, -1, -1, -1, -1 }, { -2, -2, -2, -2, -1, 2, 0, 0, 0, -1, 0 },
    { -2, 5, 5, -2, -1, -1, -1, -1, -1, -1, -1 },  { -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1 },
    { -2, -2, 5, -2, -1, -1, -1, -1, -1, -1, -1 }, { -2, -2, -2, -2, -1, 0, 0, -1, -1, 0, -1 },
};
static const s8 sTransRecvState[6][11] = {
    { -2, -2, -2, 3, -1, -1, 0, -1, -1, -1, -1 },   { -2, -2, -2, -3, -1, 2, 0, 0, 0, -1, 0 },
    { -2, -2, -2, -3, -1, -1, -1, -1, -1, -1, -1 }, { -2, 0, -2, -3, -1, -1, -1, -1, -1, -1, -1 },
    { -2, -2, -2, -3, -1, -1, -1, -1, -1, -1, -1 }, { -2, -2, -2, -3, -1, 0, 0, -1, -1, 0, -1 },
};
static const s8 sSendState[6][11] = {
    { 1, -2, -2, -2, -2, -2, 0, 0, 0, -2, -2 },   { -2, -2, 5, -2, -2, -2, 0, -2, -2, -2, -2 },
    { -2, 5, -2, -2, -2, -2, 0, -2, -2, -2, -2 }, { -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2 },
    { -2, -2, -2, -2, -2, 2, 0, 0, 0, -2, 0 },    { -2, -2, -2, -2, -2, 0, 0, -2, -2, 0, -2 },
};
static const s8 sTransSendState[6][11] = {
    { -2, -2, -2, 1, -1, -2, 0, 0, 0, -2, -2 },   { -2, 5, 5, -2, -1, -2, 0, -2, -2, -2, -2 },
    { -2, 5, -2, -2, -1, -2, 0, -2, -2, -2, -2 }, { -2, -2, -2, -2, -1, -2, 0, -2, -2, -2, -2 },
    { -2, -2, -2, -2, -1, 3, 0, 0, 0, -2, 0 },    { -2, -2, -2, -2, -1, 0, 0, -2, -2, 0, -2 },
};

// How long to wait for an answer, in clock ticks
static u64 sRequestTimeout = 15709687;

static SessionInfo sSession;
// When the last request was sent, or 0
static u64 sRequestTime;
static VCTSessionState sSessionState;

VCTSession *VCT_CreateSession(u8 aid) {
    if (vct.SSPMode == 0 || vct.SSPMode == 2) {
        return NULL;
    }
    return vct_create_session_impl(aid);
}

BOOL VCT_DeleteSession(VCTSession *session) {
    VCTSession *previous = NULL;
    VCTSession *current = sSession.use;

    if (session == NULL) {
        return FALSE;
    }
    if (vct.SSPMode == 2) {
        if (session == &sSession.trans) {
            sSession.trans.mode = 0;
        }
        return TRUE;
    }
    while (current != NULL) {
        if (current == session) {
            session->mode = 0;
            if (previous != NULL) {
                previous->next = session->next;
            } else if (current->next != NULL) {
                sSession.use = current->next;
            } else {
                sSession.use = NULL;
            }
            session->next = sSession.free;
            sSession.free = session;
            return TRUE;
        }
        previous = current;
        current = current->next;
    }
    return FALSE;
}

// Makes a control packet to send as a request, returning 0 or what is wrong with it
static int vct_build_request_data(VCTSession *session, u32 command, SSPHeader *packet, u32 size) {
    if (session == NULL) {
        return -1;
    }
    if (size < 0x10) {
        return -1;
    }
    if (vct.listenMode == 1) {
        return -3;
    }
    if (command >= 5) {
        return -3;
    }
    packet->method = VCT_METHOD_REQUEST;
    vct_build_ssp_header(session, packet, command);
    if (!vct_check_send_state(session, packet)) {
        return -3;
    }
    if (vct.SSPMode == 1 && command == 0) {
        if (!vct_check_multisession(session)) {
            return -3;
        }
    }
    return 0;
}

int VCT_Request(VCTSession *session, VCTRequestCode command) {
    if (vct.SSPMode == 2) {
        return -3;
    }
    return vct_request_impl(session, command);
}

// Makes a control packet to send as a reply
static int vct_build_response_data(VCTSession *session, u32 command, SSPHeader *packet, u32 size) {
    if (session == NULL) {
        return -1;
    }
    if (size < 0x10) {
        return -1;
    }
    if (command >= 6) {
        return -3;
    }
    packet->method = VCT_METHOD_RESPONSE;
    vct_build_ssp_header(session, packet, command);
    if (command == 0) {
        if (vct.SSPMode == 1 && session->state == 4) {
            if (!vct_check_multisession(session)) {
                return -3;
            }
        }
        session->aidBitmap = session->aid != 0 ? 1 << session->aid : 1;
    }
    if (!vct_check_send_state(session, packet)) {
        return -3;
    }
    return 0;
}

int VCT_Response(VCTSession *session, VCTResponseCode command) {
    if (vct.SSPMode == 2) {
        return -3;
    }
    return vct_response_impl(session, command);
}

int VCT_AddConferenceClient(u8 aid) {
    VCTSession *session;

    if (vct.SSPMode != 3) {
        return -2;
    }
    if (sSessionState.count == 3) {
        return -6;
    }
    if (aid == vct.myAID) {
        return -1;
    }
    if (vct_find_session(aid) != NULL) {
        return 0;
    }
    session = VCT_CreateSession(aid);
    if (session == NULL) {
        return -1;
    }
    session->aid = aid;
    session->talking = aid;
    session->state = 2;
    session->aidBitmap = 1 << aid;
    vct.callback(aid, 7, session, vct.userData);
    sSessionState.count++;
    return 0;
}

int VCT_RemoveConferenceClient(u8 aid) {
    VCTSession *session;

    if (vct.SSPMode != 3) {
        return -2;
    }
    session = vct_find_session(aid);
    if (session == NULL) {
        return -1;
    }
    vct.callback(aid, 9, session, vct.userData);
    sSessionState.count--;
    return 0;
}

BOOL vct_init_ssp(VCTConfig *config) {
    unsigned int i;

    if (config->session == NULL || config->numSession == 0 || config->numSession > 8) {
        return FALSE;
    }
    sSession.free = config->session;
    MI_CpuFill8(config->session, 0, config->numSession * sizeof(VCTSession));
    MI_CpuFill8(&sSession.trans, 0, sizeof(VCTSession));
    MI_CpuFill8(&sSession.tmp, 0, sizeof(VCTSession));
    for (i = 0; i < config->numSession - 1; i++) {
        sSession.free[i].next = &sSession.free[i + 1];
    }
    (sSession.free + config->numSession - 1)->next = NULL;
    sSession.use = NULL;
    sSessionState.mask = 0;
    sSessionState.count = 0;
    vct.autoResponse = 0;
    sSession.clock = OS_GetTick;
    sSession.randomMultiplier = 0x5d588b65;
    sSession.randomIncrement = 0x269ec3;
    return TRUE;
}

void vct_finish_ssp(void) {
    sSession.use = NULL;
    sSession.free = NULL;
}

// Gives up on a request that has gone unanswered for too long
void vct_check_timeout(void) {
    u64 now = OS_GetTick();

    if (vct.SSPMode != 2) {
        return;
    }
    if (vct.listenMode == 0) {
        return;
    }
    if (sRequestTime == 0) {
        return;
    }
    if (now - sRequestTime <= sRequestTimeout) {
        return;
    }
    sRequestTime = 0;
    sSession.trans.mode = 0;
    sSession.trans.state = 0;
    vct_send_notify(0);
    vct.callback(sSession.trans.aid, 9, &sSession.trans, vct.userData);
}

// Takes in a control packet, returning whether it has given the application an event
BOOL vct_handle_ssp(int aid, SSPHeader *packet, int size, VCTResult *result) {
    if (size != sizeof(SSPHeader)) {
        return FALSE;
    }
    if (packet->from != aid || packet->to != vct.myAID) {
        return FALSE;
    }
    if (packet->version != sizeof(SSPHeader)) {
        return vct_response_reject(packet, result);
    }
    if (vct.listenMode == 0) {
        return vct_handle_ssp_client(packet, result);
    }
    return vct_handle_ssp_server(packet, result);
}

static VCTSession *vct_create_session_impl(u8 aid) {
    VCTSession *session;

    if (aid >= VCT_AID_MAX) {
        return NULL;
    }
    if (aid == vct.myAID) {
        return NULL;
    }
    session = sSession.free;
    if (session == NULL) {
        return NULL;
    }
    sSession.free = session->next;
    vct_init_session(session, aid);
    session->next = sSession.use;
    sSession.use = session;
    return session;
}

static int vct_request_impl(VCTSession *session, int command) {
    SSPHeader packet;
    int result = vct_build_request_data(session, command, &packet, sizeof(packet));

    if (result != 0) {
        return result;
    }
    if (!vct_send_ssp(session, &packet)) {
        return -4;
    }
    return 0;
}

static int vct_response_impl(VCTSession *session, int command) {
    SSPHeader packet;
    int result = vct_build_response_data(session, command, &packet, sizeof(packet));

    if (result != 0) {
        return result;
    }
    if (!vct_send_ssp(session, &packet)) {
        return -4;
    }
    return 0;
}

static void vct_add_notify_aid_list(VCTSession *session, SSPHeader *packet) {
    if (packet->info == 1) {
        packet->talking = session->aid;
        packet->aidBitmap = sSessionState.mask;
    }
}

// Sends a control packet to every machine of the voice chat, returning 0 if there was one it reached
static int vct_send_notify_impl(VCTSession *session, SSPHeader *packet) {
    int sent = 0;
    u8 aid;

    vct_add_notify_aid_list(session, packet);
    for (aid = 0; aid < VCT_AID_MAX; aid++) {
        if ((sSessionState.mask & (aid != 0 ? 1 << aid : 1)) && aid != vct.myAID) {
            packet->to = aid;
            if (DWC_SendReliable(aid, packet, sizeof(SSPHeader))) {
                sent++;
            }
        }
    }
    if (packet->info == 1) {
        session->aidBitmap = sSessionState.mask;
    } else if (packet->info == 0) {
        session->aidBitmap = 0;
    }
    return sent == 0 ? -4 : 0;
}

static void vct_send_notify(int flag) {
    SSPHeader packet;

    packet.method = VCT_METHOD_REQUEST;
    vct_build_ssp_header(&sSession.trans, &packet, 4);
    packet.info = flag;
    vct_send_notify_impl(&sSession.trans, &packet);
}

static void vct_init_session(VCTSession *session, u8 aid) {
    memset(session, 0, sizeof(VCTSession));
    session->mode = vct.SSPMode;
    session->state = 0;
    session->aid = aid;
}

static BOOL vct_send_ssp(VCTSession *session, SSPHeader *packet) {
    if (packet->method == VCT_METHOD_REQUEST && packet->code == 4) {
        vct_send_notify_impl((VCTSession *)session, packet);
    } else {
        if (!DWC_SendReliable(session->aid, packet, sizeof(SSPHeader))) {
            return FALSE;
        }
    }
    return TRUE;
}

// The event that a control packet gives the application, if any
static int vct_dispatch_session(SSPHeader *packet, VCTSession *session) {
    session->talking = packet->talking;
    if (packet->method == VCT_METHOD_REQUEST) {
        if (packet->code == 0) {
            session->aidBitmap = session->aid != 0 ? 1 << session->aid : 1;
        }
        return sBroadcastEvents[packet->code];
    } else if (packet->method == VCT_METHOD_RESPONSE) {
        if (packet->code == 0) {
            if (session->state == 1) {
                session->aidBitmap = session->aid != 0 ? 1 << session->aid : 1;
                return 7;
            }
            return session->state == 5 ? 9 : 12;
        }
        return sDirectEvents[packet->code];
    }
    return 12;
}

// Fills in a control packet from this machine to the session's. It returns 0, which no caller reads
static int vct_build_ssp_header(VCTSession *session, SSPHeader *packet, u8 command) {
    packet->magic = VCT_PACKET_MAGIC;
    packet->version = sizeof(SSPHeader);
    packet->code = command;
    packet->from = vct.myAID;
    packet->to = session->aid;
    packet->info = 0;
    packet->talking = vct.myAID;
    packet->aidBitmap = 0;
    return 0;
}

// Moves a session on by a reply, returning what the table says
static int vct_check_recv_state(VCTSession *session, SSPHeader *packet) {
    int column = packet->code;
    int next;

    if (packet->method == VCT_METHOD_RESPONSE) {
        column += 5;
    }
    if ((u32)column >= 11) {
        column = -1;
    }
    if (column < 0) {
        return -1;
    }
    if (vct.SSPMode == 1) {
        next = sRecvState[session->state][column];
    } else {
        next = sTransRecvState[session->state][column];
    }
    switch (next) {
    case -1:
        return next;
    case -2:
        vct_response_impl(session, 1);
        session->state = 0;
        session->mode = 0;
        return next;
    case -3:
        vct_response_impl(session, 3);
        session->state = 0;
        session->mode = 0;
        return next;
    default:
        return next;
    }
}

static BOOL vct_check_multisession(VCTSession *session) {
    VCTSession *other;

    for (other = sSession.use; other != NULL; other = other->next) {
        if (other->mode != 0 && other->state == 2 && session != other) {
            return FALSE;
        }
    }
    return TRUE;
}

// Moves a session on by a request, returning 0 if it is refused
static int vct_check_send_state(VCTSession *session, SSPHeader *packet) {
    int column = packet->code;
    int next;

    if (packet->method == VCT_METHOD_RESPONSE) {
        column += 5;
    }
    if ((u32)column >= 11) {
        column = -1;
    }
    if (column < 0) {
        return -1;
    }
    if (vct.SSPMode == 1) {
        next = sSendState[session->state][column];
    } else {
        next = sTransSendState[session->state][column];
    }
    if (next == -1) {
        return TRUE;
    }
    if (next == -2) {
        return FALSE;
    }
    session->state = next;
    return TRUE;
}

static VCTSession *vct_find_session(int aid) {
    VCTSession *session = sSession.use;

    if (vct.SSPMode == 2) {
        if (sSession.trans.mode != 0 && sSession.trans.aid == aid) {
            return &sSession.trans;
        }
        return NULL;
    }
    for (; session != NULL; session = session->next) {
        if (session->mode != 0 && session->aid == aid) {
            return session;
        }
    }
    return NULL;
}

static BOOL vct_handle_ssp_client(SSPHeader *packet, VCTResult *result) {
    VCTSession *session;
    int next;

    if (packet->method == VCT_METHOD_REQUEST) {
        if (packet->code == 4) {
            return vct_handle_notify(packet, result);
        }
        if (packet->code == 3) {
            return FALSE;
        }
    }
    session = vct_find_session(packet->from);
    if (session != NULL) {
        next = vct_check_recv_state(session, packet);
        switch (next) {
        case -1:
            return FALSE;
        case -2:
            result->event = 12;
            session->mode = 0;
            result->session = session;
            return TRUE;
        }
        result->event = vct_dispatch_session(packet, session);
        result->session = session;
        session->state = next;
        return result->event != 0;
    }
    session = vct_create_session_impl(packet->from);
    if (session == NULL) {
        vct_init_session(&sSession.tmp, packet->from);
        vct_response_impl(&sSession.tmp, 3);
        return FALSE;
    }
    next = vct_check_recv_state(session, packet);
    if (next == -2 || next == -1) {
        VCT_DeleteSession(session);
        return FALSE;
    }
    result->event = vct_dispatch_session(packet, session);
    result->session = session;
    session->state = next;
    return result->event != 0;
}

static BOOL vct_handle_ssp_server(SSPHeader *packet, VCTResult *result) {
    VCTSession *host;
    int next;

    if (sSession.trans.mode == 2) {
        if (sSession.trans.aid == packet->from) {
            host = &sSession.trans;
            next = vct_check_recv_state(host, packet);
            switch (next) {
            case -2:
                result->event = 12;
                result->session = NULL;
                return TRUE;
            case -3:
            case -1:
                return FALSE;
            }
            result->event = vct_dispatch_session(packet, host);
            result->session = host;
            host->state = next;
        } else {
            vct_send_notify(1);
            return FALSE;
        }
    } else {
        if (!(sSessionState.mask & (packet->from != 0 ? 1 << packet->from : 1))) {
            return FALSE;
        }
        sSession.trans.aid = packet->from;
        sSession.trans.mode = 2;
        sSession.trans.state = 0;
        host = &sSession.trans;
        next = vct_check_recv_state(host, packet);
        switch (next) {
        case -3:
        case -2:
        case -1:
            sSession.trans.mode = 0;
            return FALSE;
        }
        result->event = vct_dispatch_session(packet, host);
        result->session = host;
        host->state = next;
    }
    if (result->event == 10) {
        vct_send_notify(1);
        sRequestTime = OS_GetTick();
    } else {
        vct_send_notify(0);
        sRequestTime = 0;
    }
    return TRUE;
}

static BOOL vct_response_reject(SSPHeader *packet, VCTResult *result) {
    VCTSession *session;

    if (packet->method == VCT_METHOD_REQUEST && packet->code == 4) {
        return FALSE;
    }
    session = vct_find_session(packet->from);
    if (session != NULL) {
        result->event = 2;
        result->session = session;
        session->mode = 0;
        vct_response_impl(session, 2);
        return TRUE;
    }
    vct_init_session(&sSession.tmp, packet->from);
    vct_response_impl(&sSession.tmp, 2);
    return FALSE;
}

static BOOL vct_handle_notify(SSPHeader *packet, VCTResult *result) {
    if (vct.SSPMode != 2) {
        return FALSE;
    }
    if (vct.serverAID != packet->from) {
        return FALSE;
    }
    switch (packet->info) {
    case 0:
        if (sSession.trans.mode == 0) {
            return FALSE;
        }
        if (sSession.trans.state == 5 || sSession.trans.state == 2) {
            result->event = 9;
        } else {
            result->event = 5;
        }
        sSession.trans.mode = 0;
        sSession.trans.state = 0;
        result->session = &sSession.trans;
        break;
    case 1:
        sSession.trans.mode = 2;
        if (packet->talking == vct.myAID) {
            sSession.trans.state = 2;
            result->event = 7;
            sSession.trans.aidBitmap = packet->aidBitmap;
        } else {
            sSession.trans.state = 3;
            result->event = 6;
            sSession.trans.aidBitmap = packet->talking != 0 ? 1 << packet->talking : 1;
        }
        sSession.trans.talking = packet->talking;
        result->session = &sSession.trans;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}
