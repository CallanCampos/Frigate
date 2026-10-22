#ifndef POKEBW2_DWC_VCT_SESSION_H
#define POKEBW2_DWC_VCT_SESSION_H

#include "types.h"
#include "struct_decls.h"

// A machine that the voice chat is talking to. A machine is asked to take part with a request, answers with a reply,
// and the application hears of what comes of it in an event
// The library's modes, a session's states, and the requests and replies that sessions exchange: the SDK's VCTMode,
// VCTState, VCTRequestCode and VCTResponseCode (ntrtwl's libvct, include/vct.h)
typedef enum VCTMode {
    VCT_MODE_NULL,
    VCT_MODE_PHONE,
    VCT_MODE_TRANSCEIVER,
    VCT_MODE_CONFERENCE,
} VCTMode;

typedef enum VCTState {
    VCT_STATE_INIT,
    VCT_STATE_OUTGOING,
    VCT_STATE_TALKING,
    VCT_STATE_CONNECTED,
    VCT_STATE_INCOMING,
    VCT_STATE_DISCONNECTING,
} VCTState;

typedef enum VCTRequestCode {
    VCT_REQUEST_INVITE = 0,
    VCT_REQUEST_BYE,
    VCT_REQUEST_CANCEL,
    VCT_REQUEST_CONTACT,
    VCT_REQUEST_NOTIFY,
    VCT_REQUEST_END,
} VCTRequestCode;

typedef enum VCTResponseCode {
    VCT_RESPONSE_OK,
    VCT_RESPONSE_BAD_REQUEST,
    VCT_RESPONSE_NOT_ACCEPTABLE,
    VCT_RESPONSE_BUSY_HERE,
    VCT_RESPONSE_TERMINATED,
    VCT_RESPONSE_DECLINE,
    VCT_RESPONSE_END,
} VCTResponseCode;

struct VCTSession {
    // Not 0 while the session is in use: the library's mode
    VCTMode active;
    u8 aid;
    // The machine that asked for the machine's voice, when it is not the machine itself
    u8 origin;
    u8 pad6[2];
    // The machines whose voice is heard, a bit each
    u32 mask;
    // 2 once the voice flows both ways
    VCTState state;
    void *userData;
    VCTSession *next;
};

// The sessions: one for a machine, and its voice's start and end
VCTSession *VCT_AllocSession(u8 aid);
BOOL VCT_ReleaseSession(VCTSession *session);
int VCT_Request(VCTSession *session, VCTRequestCode command);
int VCT_Respond(VCTSession *session, VCTResponseCode command);
int VCT_AddSession(u8 aid);
int VCT_RemoveSession(u8 aid);

#endif // POKEBW2_DWC_VCT_SESSION_H
