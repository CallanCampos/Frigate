#ifndef POKEBW2_DWC_VCT_SESSION_INTERNAL_H
#define POKEBW2_DWC_VCT_SESSION_INTERNAL_H

#include "types.h"
#include "dwc/vct_session.h"
#include "struct_decls.h"

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
