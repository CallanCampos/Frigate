#ifndef POKEBW2_DWC_VCT_SESSION_H
#define POKEBW2_DWC_VCT_SESSION_H

#include "types.h"
#include "struct_decls.h"

// A machine that the voice chat is talking to. A machine is asked to take part with a request, answers with a reply,
// and the application hears of what comes of it in an event
struct VCTSession {
    // Not 0 while the session is in use: the library's mode
    int active;
    u8 aid;
    // The machine that asked for the machine's voice, when it is not the machine itself
    u8 origin;
    u8 pad6[2];
    // The machines whose voice is heard, a bit each
    u32 mask;
    // 2 once the voice flows both ways
    int state;
    u32 unk10;
    VCTSession *next;
};

// The sessions: one for a machine, and its voice's start and end
VCTSession *VCT_AllocSession(u8 aid);
BOOL VCT_ReleaseSession(VCTSession *session);
int VCT_Request(VCTSession *session, int command);
int VCT_Respond(VCTSession *session, int command);
int VCT_AddSession(u8 aid);
int VCT_RemoveSession(u8 aid);

#endif // POKEBW2_DWC_VCT_SESSION_H
