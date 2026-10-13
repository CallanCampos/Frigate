// dwc_rap_vchat.c: the game's side of the Wi-Fi voice chat, between the connection (dwc_rap.c in overlay 11), the
// microphone and the speaker and Nintendo's voice chat library libVCT, which follows it in overlay 29. The ROM names
// no file, so the name is a guess. The names are ours

#include "gfl/dwc_vchat.h"
#include "types.h"
#include "dwc/dwc.h"
#include "dwc/vct.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nnsys/snd.h"
#include "system/dsi.h"

#define VCT_SAMPLE_BYTES 0x880

typedef struct DWCRapVChatWork {
    // What the microphone samples into, a ring that the stream callback reads
    u8 micRing[VCT_SAMPLE_BYTES];
    // Memory for the library's packets
    void *packetMemory;
    // The speaker's stream buffer
    u8 streamBuffer[VCT_SAMPLE_BYTES];
    // What is sent in place of the microphone's samples when it is muted
    u8 silence[VCT_SAMPLE_BYTES];
    void (*onEnd)(void);
    VCTSession session;
    // Whether the machine of each net ID is being heard
    int heard[2];
    int callbackKind;
    // 0 before a request, 1 while it waits, 2 once the voice flows
    int phase;
    u16 stalled;
    u16 muted;
    u8 unk19b4[4];
    u8 stream[0x64];
    VCTSession *requestSession;
    MICAutoParam micParam;
    u64 lastTick;
    int tickBudget;
    u16 waitFrames;
    u8 micState;
    u8 micRunning;
} DWCRapVChatWork;

static void InitHardware(void);
static void MicCallback(void);
static void StreamCallback(NNSSndStrmCallbackStatus status, int numChannels, void *buffer[], u32 len,
                           NNSSndStrmFormat format, void *arg);
static BOOL RequestVoice(u8 aid);
static BOOL AcceptVoice(u8 aid);
static void ManualCallback(u8 aid, int event, VCTSession *session, void *userData);
static void SessionCallback(u8 aid, int event, VCTSession *session, void *userData);
static void DropSession(VCTSession *session);
static void StartStream(void);

static DWCRapVChatWork *sVChatWork;

static void InitHardware(void) {
    func_0207e75c();
    func_0207ebb8();
    func_0207efc4(PM_AMP_ON);
    func_0207eff4(3);
    VCT_SetVad(TRUE);
    NNS_SndInit();
    NNS_SndStrmInit((NNSSndStrm *)sVChatWork->stream);
    sys_memset32_fast(0, sVChatWork->silence, VCT_SAMPLE_BYTES);
    sVChatWork->tickBudget = 0;
    VCT_SetEchoCancel(TRUE);
}

static void MicCallback(void) {
}

static void StreamCallback(NNSSndStrmCallbackStatus status, int numChannels, void *buffer[], u32 len,
                           NNSSndStrmFormat format, void *arg) {
    u8 *samples = arg;
    u32 i;

    if (status == NNS_SND_STRM_CALLBACK_SETUP) {
        for (i = 0; i < numChannels; i++) {
            sys_memset(buffer[i], 0, len);
        }
        return;
    }
    if (!isRunningOnDSi()) {
        if (sVChatWork->micRunning) {
            func_0207e7a0(&sVChatWork->micParam, MicCallback, 0);
            sVChatWork->micRunning = 0;
        }
    } else {
        if (sVChatWork->micRunning) {
            func_0207e904(&sVChatWork->micParam, MicCallback, 0);
            sVChatWork->micRunning = 0;
        }
    }
    if ((u8 *)func_0207e8f8() - samples < len) {
        samples += len;
    }
    if (PAD_DetectFold() || sVChatWork->muted != 0) {
        samples = sVChatWork->silence;
    }
    for (i = 0; i < numChannels; i++) {
        VCT_ReadSamples(buffer[i], len, NULL);
    }
    if (sVChatWork->stalled == 0) {
        VCT_SubmitSamples(samples, len);
    }
    sVChatWork->micState = 2;
}

static BOOL RequestVoice(u8 aid) {
    VCTSession *session;
    int ret;

    if (sVChatWork->requestSession == NULL) {
        session = VCT_AllocSession(aid);
        if (session == NULL) {
            return FALSE;
        }
        ret = VCT_Request(session, 0);
    } else {
        session = sVChatWork->requestSession;
        ret = VCT_Request(session, 0);
    }
    if (ret) {
        VCT_ReleaseSession(session);
        sVChatWork->requestSession = NULL;
        return FALSE;
    }
    sVChatWork->requestSession = session;
    return TRUE;
}

static BOOL AcceptVoice(u8 aid) {
    BOOL result;

    if (sVChatWork->requestSession != NULL && sVChatWork->requestSession->state == 4) {
        result = FALSE;
        if (VCT_Respond(sVChatWork->requestSession, 0)) {
            return result;
        }
        if (VCT_AttachSession(sVChatWork->requestSession)) {
            result = TRUE;
        }
        return result;
    }
    return FALSE;
}

static void ManualCallback(u8 aid, int event, VCTSession *session, void *userData) {
    switch (event) {
    case 9:
        DropSession(session);
        break;
    case 7:
        VCT_AttachSession(session);
        break;
    case 12:
        DropSession(session);
        break;
    }
}

static void SessionCallback(u8 aid, int event, VCTSession *session, void *userData) {
    switch (event) {
    case 1:
        if (sVChatWork->requestSession != NULL) {
            VCT_Respond(session, 3);
            VCT_ReleaseSession(session);
        } else {
            sVChatWork->requestSession = session;
        }
        break;
    case 8:
        VCT_Respond(session, 0);
        DropSession(session);
        break;
    case 9:
        DropSession(session);
        break;
    case 4:
        VCT_Respond(session, 4);
        DropSession(session);
        break;
    case 7:
        if (session->active != sVChatWork->callbackKind) {
            DropSession(session);
        } else if (VCT_AttachSession(session)) {
            sVChatWork->phase = 2;
        }
        break;
    case 2:
        DropSession(session);
        break;
    case 3:
    case 11:
    case 12:
        DropSession(session);
        break;
    }
}

static void DropSession(VCTSession *session) {
    VCT_DetachSession(session);
    VCT_ReleaseSession(session);
    sVChatWork->requestSession = NULL;
    DWCRap_ResetVoiceChat();
}

// Runs the voice chat a frame: the library once for each part of its tick that the time since the last frame holds,
// and, for the machine that asks, the request
void DWCRapVChat_Update(BOOL stalled) {
    u64 now;
    u32 elapsed;

    sVChatWork->stalled = stalled;
    now = clock();
    elapsed = (u32)now - (u32)sVChatWork->lastTick;
    sVChatWork->tickBudget += elapsed * 0xfa00 / 0x82ea - 0x411a;
    if (sVChatWork->tickBudget < -10000) {
        sVChatWork->tickBudget = 0;
    }
    sVChatWork->lastTick = now;
    VCT_Tick();
    if (sVChatWork->tickBudget >= 0x411a) {
        do {
            VCT_Tick();
            sVChatWork->tickBudget -= 0x411a;
        } while (sVChatWork->tickBudget >= 0x411a);
    }
    if (sVChatWork->callbackKind == 3) {
        return;
    }
    switch (sVChatWork->phase) {
    case 0:
        if (func_ov011_021602c0() == 0) {
            if (RequestVoice(1)) {
                sVChatWork->phase = 1;
                sVChatWork->waitFrames = 60;
            }
        } else if (func_ov011_021602c0() == 1) {
            if (AcceptVoice(0)) {
                sVChatWork->phase = 2;
            }
        }
        break;
    case 1:
        sVChatWork->waitFrames--;
        if (sVChatWork->waitFrames == 0) {
            sVChatWork->phase = 0;
        }
        break;
    case 2:
        break;
    }
}

BOOL DWCRapVChat_ReceivePacket(int aid, u8 *data, int size) {
    if (sVChatWork == NULL) {
        return FALSE;
    }
    if (VCT_ReceivePacket(aid, data, size)) {
        return TRUE;
    }
    return FALSE;
}

// Starts the voice chat in heapId, with the library's codec and the number of machines to talk to
void DWCRapVChat_Start(HeapID heapId, int codec, int count) {
    VCTConfig config;
    u8 channels[3] = {0, 0, 0};
    BOOL ret;

    if (sVChatWork == NULL) {
        sVChatWork = DWCRapCommon_Alloc(heapId, sizeof(DWCRapVChatWork), 32);
        sys_memset(sVChatWork, 0, sizeof(DWCRapVChatWork));
        sVChatWork->packetMemory = DWCRapCommon_Alloc(heapId, 0x23c0 * count + 32, 32);
        sVChatWork->onEnd = NULL;
        InitHardware();
    }
    sVChatWork->micParam.type = MIC_SAMPLING_TYPE_SIGNED_12BIT;
    sVChatWork->micParam.buffer = sVChatWork->micRing;
    sVChatWork->micParam.size = VCT_SAMPLE_BYTES;
    if (!isRunningOnDSi()) {
        sVChatWork->micParam.rate = 0x1040;
    } else {
        sVChatWork->micParam.rate = 0x1001;
    }
    sVChatWork->micParam.loop = TRUE;
    sVChatWork->micParam.fullCallback = NULL;
    sVChatWork->micParam.fullArg = NULL;
    sVChatWork->micRunning = TRUE;
    NNS_SndStrmAllocChannel((NNSSndStrm *)sVChatWork->stream, 1, channels);
    NNS_SndStrmSetVolume((NNSSndStrm *)sVChatWork->stream, 0);
    ret = NNS_SndStrmSetup((NNSSndStrm *)sVChatWork->stream, NNS_SND_STRM_FORMAT_PCM16, sVChatWork->streamBuffer,
                           VCT_SAMPLE_BYTES, isRunningOnDSi() ? 0x40 : 0x41, 2, StreamCallback, sVChatWork);
    GFL_ASSERT(ret);
    sVChatWork->phase = 0;
    sVChatWork->requestSession = NULL;
    config.mode = 1;
    sVChatWork->callbackKind = 1;
    config.sessions = &sVChatWork->session;
    config.sessionCount = count;
    config.aid = func_ov011_021602c0();
    GFL_ASSERT((config.aid != -1));
    if (sVChatWork->callbackKind == 3) {
        config.callback = ManualCallback;
    } else {
        config.callback = SessionCallback;
    }
    config.userData = NULL;
    config.buffer = sVChatWork->packetMemory;
    config.bufferSize = 0x23c0 * count + 32;
    VCT_Init(&config);
    sVChatWork->stalled = 0;
    VCT_SetCodec(codec);
    StartStream();
    VCT_SetEchoCancel(TRUE);
}

static void StartStream(void) {
    NNS_SndStrmStart((NNSSndStrm *)sVChatWork->stream);
}

// Ends the voice chat of the machine that asked
void DWCRapVChat_RequestEnd(void) {
    if (sVChatWork->requestSession == NULL || sVChatWork->phase == 0) {
        DWCRap_ResetVoiceChat();
        return;
    }
    if (sVChatWork->phase == 1) {
        if (VCT_Request(sVChatWork->requestSession, 2)) {
            DWCRap_ResetVoiceChat();
            return;
        }
    }
    if (VCT_Request(sVChatWork->requestSession, 1)) {
        DWCRap_ResetVoiceChat();
    }
}

void DWCRapVChat_SetEndCallback(void (*callback)(void)) {
    sVChatWork->onEnd = callback;
}

void DWCRapVChat_End(void) {
    void (*onEnd)(void);

    if (sVChatWork != NULL) {
        onEnd = sVChatWork->onEnd;
        if (!isRunningOnDSi()) {
            func_0207e8d4();
        } else {
            MIC_StopAutoSampling();
        }
        NNS_SndStrmStop((NNSSndStrm *)sVChatWork->stream);
        NNS_SndStrmFreeChannel((NNSSndStrm *)sVChatWork->stream);
        VCT_Shutdown();
        func_0207efc4(PM_AMP_OFF);
        DWCRapCommon_Free(0, sVChatWork->packetMemory, 0);
        DWCRapCommon_Free(0, sVChatWork, 0);
        sVChatWork = NULL;
        if (onEnd != NULL) {
            onEnd();
        }
    }
}

// The machine that is speaking, if the voice is loud enough
BOOL DWCRapVChat_GetSpeaking(void) {
    s32 levels[4];

    if (sVChatWork != NULL) {
        VCT_GetCaptureLevels(levels);
        if (levels[1] > 2) {
            return levels[0];
        }
    }
    return FALSE;
}

BOOL DWCRapVChat_AddHeard(u32 netIds, int myNetId) {
    int i;

    if (sVChatWork == NULL || sVChatWork->callbackKind != 3) {
        return FALSE;
    }
    for (i = 0; i < 2; i++) {
        if (i != myNetId && (netIds & (1 << i)) && sVChatWork->heard[i] != 1) {
            if (VCT_AddSession(i)) {
                return FALSE;
            }
            sVChatWork->heard[i] = 1;
        }
    }
    return TRUE;
}

BOOL DWCRapVChat_RemoveHeard(int myNetId) {
    int i;

    if (sVChatWork == NULL || sVChatWork->callbackKind != 3) {
        return FALSE;
    }
    for (i = 0; i < 2; i++) {
        if (i != myNetId && sVChatWork->heard[i] == 1) {
            if (VCT_RemoveSession(i)) {
                return FALSE;
            }
            sVChatWork->heard[i] = 0;
        }
    }
    return TRUE;
}

void DWCRapVChat_SetMuted(BOOL muted) {
    if (sVChatWork != NULL) {
        sVChatWork->muted = muted;
        if (sVChatWork->requestSession != NULL) {
            if (muted) {
                VCT_DetachSession(sVChatWork->requestSession);
            } else {
                VCT_AttachSession(sVChatWork->requestSession);
            }
        }
    }
}
