#ifndef POKEBW2_DWC_VCT_H
#define POKEBW2_DWC_VCT_H

#include "types.h"
#include "dwc/vct_audio.h"
#include "dwc/vct_dsp.h"
#include "dwc/vct_session.h"
#include "struct_decls.h"

// Nintendo's voice chat library libVCT (the ROM carries the version string "[SDK+Nintendo:libVCT 1.4.1]"), which
// sends the microphone's sound to the machines of a Wi-Fi match and plays what they send. It sits in overlay 29 beside
// the game's own code for it. The ROM names none of its functions, so the names say what the code does; the one name
// the ROM gives is the field `aid` of the configuration, from an assertion.

// Called with an event for a session. The events are numbers: 1, 4 and 8 for a request, 7 for the voice starting, and
// 2, 3, 9, 11 and 12 for a refusal, a close or the end of it
typedef void (*VCTCallback)(u8 aid, int event, VCTSession *session, void *userData);

struct VCTConfig {
    // Memory for the sessions, and how many there are, at most 8
    VCTSession *sessions;
    u32 sessionCount;
    // 1, 2 or 3. In the first the application chooses to ask and to answer for each machine
    int mode;
    // The ID of this machine in the match, below 32
    u8 aid;
    u8 pad[3];
    // Memory for the packets that come in, aligned to 32 bytes
    void *buffer;
    u32 bufferSize;
    VCTCallback callback;
    void *userData;
};

// Starts the library, returning FALSE for a configuration it cannot work with
BOOL VCT_Init(VCTConfig *config);
void VCT_Shutdown(void);
// Runs the library a tick
void VCT_Tick(void);
// Takes in a packet that another machine has sent, returning FALSE for one that is not meant for the library
BOOL VCT_ReceivePacket(u8 aid, void *data, int size);

#endif // POKEBW2_DWC_VCT_H
