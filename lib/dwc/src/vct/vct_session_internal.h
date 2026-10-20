#ifndef POKEBW2_DWC_VCT_SESSION_INTERNAL_H
#define POKEBW2_DWC_VCT_SESSION_INTERNAL_H

#include "types.h"
#include "dwc/vct_session.h"
#include "struct_decls.h"

// A control packet: a machine asking another to start or stop hearing it
struct VCTControlPacket {
    u32 magic;
    u8 type;
    u8 length;
    u8 command;
    u8 pad7;
    u8 from;
    u8 to;
    u8 flag;
    u8 origin;
    u32 argument;
};

// What a control packet gave the application: an event, and the session it is for
struct VCTEventResult {
    int event;
    VCTSession *session;
};

// vct_session.c: the machines
void VCT_ClearSessions(void);
void VCT_CheckTimeout(void);
BOOL VCT_HandleControlPacket(int aid, VCTControlPacket *packet, int size, VCTEventResult *result);
BOOL VCT_InitSessions(VCTConfig *config);

#endif // POKEBW2_DWC_VCT_SESSION_INTERNAL_H
