#ifndef POKEBW2_DWC_VCT_SESSION_INTERNAL_H
#define POKEBW2_DWC_VCT_SESSION_INTERNAL_H

#include "types.h"
#include "dwc/vct_session.h"
#include "struct_decls.h"

// A control packet: a machine asking another to start or stop hearing it
struct SSPHeader {
    u32 magic;
    u8 method;
    u8 version;
    u8 code;
    u8 reserved;
    u8 from;
    u8 to;
    u8 info;
    u8 talking;
    u32 aidBitmap;
};

// What a control packet gave the application: an event, and the session it is for
struct VCTResult {
    int event;
    VCTSession *session;
};

// vct_session.c: the machines
void vct_finish_ssp(void);
void vct_check_timeout(void);
BOOL vct_handle_ssp(int aid, SSPHeader *packet, int size, VCTResult *result);
BOOL vct_init_ssp(VCTConfig *config);

#endif // POKEBW2_DWC_VCT_SESSION_INTERNAL_H
