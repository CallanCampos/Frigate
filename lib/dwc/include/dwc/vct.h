#ifndef POKEBW2_DWC_VCT_H
#define POKEBW2_DWC_VCT_H

#include "types.h"

// Nintendo's voice chat library libVCT (the ROM carries the version string "[SDK+Nintendo:libVCT 1.4.1]"), which
// sends the microphone's sound to the machines of a Wi-Fi match and plays what they send. It sits in overlay 29 beside
// the game's own code for it. The ROM names none of its functions, so the names say what the code does; the one name
// the ROM gives is the field `aid` of the configuration, from an assertion.

// A machine that the voice chat is talking to. A machine is asked to take part with a request, answers with a reply,
// and the application hears of what comes of it in an event
typedef struct VCTSession VCTSession;
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

// Called with an event for a session. The events are numbers: 1, 4 and 8 for a request, 7 for the voice starting, and
// 2, 3, 9, 11 and 12 for a refusal, a close or the end of it
typedef void (*VCTCallback)(u8 aid, int event, VCTSession *session, void *userData);

typedef struct VCTConfig {
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
} VCTConfig;

// Starts the library, returning FALSE for a configuration it cannot work with
BOOL VCT_Init(VCTConfig *config);
void VCT_Shutdown(void);
// Runs the library a tick
void VCT_Tick(void);
// Takes in a packet that another machine has sent, returning FALSE for one that is not meant for the library
BOOL VCT_ReceivePacket(u8 aid, void *data, int size);

// Hands a frame of 0x440 bytes of the microphone's samples over to be sent
BOOL VCT_SubmitSamples(void *samples, int size);
// Fills a frame with the voices that are due, returning whether there was any, and gives the machines heard as bits
BOOL VCT_ReadSamples(void *dst, int size, u32 *heardMask);
// The five ways to code a frame, from 0: 8-bit samples, mu-law and ADPCM at 2, 3 and 4 bits. Not all are allowed
// in every mode
BOOL VCT_SetCodec(int codec);
// Whether frames with no voice in them are left out
void VCT_SetVad(BOOL enabled);
// Whether to keep the microphone from picking up what the speaker plays
void VCT_SetEchoCancel(BOOL enabled);

// Copies the four words of how loud the microphone is: whether somebody is speaking, the level, and two limits
void VCT_GetCaptureLevels(s32 *out);

// The sessions: one for a machine, and its voice's start and end
VCTSession *VCT_AllocSession(u8 aid);
BOOL VCT_ReleaseSession(VCTSession *session);
BOOL VCT_AttachSession(VCTSession *session);
void VCT_DetachSession(VCTSession *session);
int VCT_Request(VCTSession *session, int command);
int VCT_Respond(VCTSession *session, int command);
int VCT_AddSession(u8 aid);
int VCT_RemoveSession(u8 aid);

#endif // POKEBW2_DWC_VCT_H
