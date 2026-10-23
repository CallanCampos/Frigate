#ifndef POKEBW2_DWC_VCT_H
#define POKEBW2_DWC_VCT_H

#include "types.h"
#include "dwc/vct_audio.h"
#include "dwc/vct_dsp.h"
#include "dwc/vct_session.h"
#include "struct_decls.h"

// Nintendo's voice chat library libVCT (the ROM carries the version string "[SDK+Nintendo:libVCT 1.4.1]"), which
// sends the microphone's sound to the machines of a Wi-Fi match and plays what they send. It sits in overlay 29 beside
// the game's own code for it. The ROM names none of its functions: the names are libVCT's own, as ntrtwl's
// reconstruction of libVCT has them, where the code shows them, and ours elsewhere; the one name the ROM gives is the
// field `aid` of the configuration, from an assertion.

// The events of a session: 1, 4 and 8 for a request, 7 for the voice starting, and 2, 3, 9, 11 and 12 for a refusal, a
// close or the end of it. The SDK's VCTEvent (ntrtwl's libvct, include/vct.h)
typedef enum VCTEvent {
    VCT_EVENT_NONE,
    VCT_EVENT_INCOMING,
    VCT_EVENT_REJECT,
    VCT_EVENT_BUSY,
    VCT_EVENT_CANCEL,
    VCT_EVENT_NOTIFY_FREE,
    VCT_EVENT_NOTIFY_BUSY,
    VCT_EVENT_CONNECTED,
    VCT_EVENT_RESPONDBYE,
    VCT_EVENT_DISCONNECTED,
    VCT_EVENT_CONTACT,
    VCT_EVENT_TIMEOUT,
    VCT_EVENT_ABORT,
} VCTEvent;

// Called with an event for a session
typedef void (*VCTEventCallback)(u8 aid, VCTEvent event, VCTSession *session, void *userData);

struct VCTConfig {
    // Memory for the sessions, and how many there are, at most 8
    VCTSession *session;
    u32 numSession;
    // 1, 2 or 3. In the first the application chooses to ask and to answer for each machine
    VCTMode mode;
    // The ID of this machine in the match, below 32
    u8 aid;
    u8 pad[3];
    // Memory for the packets that come in, aligned to 32 bytes
    void *audioBuffer;
    u32 audioBufferSize;
    VCTEventCallback callback;
    void *userData;
};

// Starts the library, returning FALSE for a configuration it cannot work with
BOOL VCT_Init(VCTConfig *config);
void VCT_Cleanup(void);
// Runs the library a tick
void VCT_Main(void);
// Takes in a packet that another machine has sent, returning FALSE for one that is not meant for the library
BOOL VCT_HandleData(u8 aid, u8 *data, int size);

#endif // POKEBW2_DWC_VCT_H
