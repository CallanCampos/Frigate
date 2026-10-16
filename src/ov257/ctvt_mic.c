#include "app/comm_tvt/ctvt_mic.h"
#include "types.h"
#include "app/comm_tvt/ima_adpcm.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nnsys/snd.h"
#include "twl/sndex.h"

// The Xtransceiver's microphone: the voice is recorded into a buffer, sent packed by ctvt_talk.c, and the other
// side's voice played back on a wave channel

// The recording buffer, a little over three seconds at 8180 Hz
#define CTVT_MIC_BUFFER_SIZE 0xc800
// The start of a recording that is left out, the click of the microphone starting
#define CTVT_MIC_SKIPPED_SIZE 0x800
#define CTVT_MIC_RATE 8180
#define CTVT_MIC_WAVE_CHANNEL 7
// The microphone is only used once this many frames have passed
#define CTVT_MIC_WARMUP_FRAMES 180

struct CtvtMic {
    u32 recordedSize;
    void *buffer;
    BOOL recording;
    BOOL playing;
    u8 frames;
    u32 playFrames;
    u32 playSize;
    int playSpeed;
    NNSSndWaveOutHandle channel;
};

static void CtvtMic_VBlank(void *data);
static void CtvtMic_OnBufferFull(MICResult result, void *arg);
static void CtvtMic_AllocChannel(CtvtMic *work, HeapID heapId);
static void CtvtMic_FreeChannel(CtvtMic *work);

static u16 sFilter[6] = { 0x7e46, 0x81ba, 0x7e46, 0x7e43, 0x836e, 0 };

static CtvtMic *sCtvtMic;

CtvtMic *CtvtMic_Create(HeapID heapId) {
    CtvtMic *work = GFL_HeapAllocate(heapId, sizeof(CtvtMic), TRUE, "ctvt_mic.c", 87);

    func_0207e75c();
    func_0207ebb8();
    func_0207efc4(PM_AMP_ON);
    func_0207f008(80);
    if (hw_isDSi() == TRUE) {
        func_027047c0(0, sFilter);
    }
    CtvtMic_AllocChannel(work, heapId);
    work->buffer = allocConfigDSSoftwareFeature(heapId, CTVT_MIC_BUFFER_SIZE, "ctvt_mic.c", 134);
    work->recordedSize = 0;
    work->recording = FALSE;
    work->frames = 0;
    sCtvtMic = work;
    GFL_VBlankSetCallback(CtvtMic_VBlank, NULL);
    return work;
}

void CtvtMic_Delete(CtvtMic *work) {
    GFL_VBlankResetCallback();
    sCtvtMic = NULL;
    CtvtMic_StopRecording(work);
    CtvtMic_FreeChannel(work);
    func_0207efc4(PM_AMP_OFF);
    func_02042ed0(work->buffer);
    GFL_HeapFree(work);
}

void CtvtMic_Update(CtvtMic *work) {
    if (work->recording == TRUE) {
        void *last = func_0207e8f8();

        if (last == NULL) {
            work->recordedSize = 0;
        } else {
            work->recordedSize = (u8 *)last - (u8 *)work->buffer + 4;
            if (work->recordedSize > CTVT_MIC_BUFFER_SIZE) {
                work->recordedSize = CTVT_MIC_BUFFER_SIZE;
            }
        }
    }
    if (work->playing == TRUE && !NNS_SndWaveOutIsPlaying(work->channel)) {
        work->playing = FALSE;
    }
    CtvtMic_Debug(work);
}

void CtvtMic_Draw(CtvtMic *work) {
}

static void CtvtMic_VBlank(void *data) {
    if (sCtvtMic->playing == TRUE) {
        sCtvtMic->playFrames++;
    }
    if (sCtvtMic->frames < CTVT_MIC_WARMUP_FRAMES) {
        sCtvtMic->frames++;
    }
}

BOOL CtvtMic_StartRecording(CtvtMic *work) {
    MICAutoParam param;

    param.type = MIC_SAMPLING_TYPE_SIGNED_12BIT;
    param.size = CTVT_MIC_BUFFER_SIZE;
    param.buffer = work->buffer;
    sys_memset32(0, param.buffer, param.size);
    if (param.size & 0x1f) {
        param.size &= ~0x1f;
    }
    param.fullCallback = CtvtMic_OnBufferFull;
    param.loop = FALSE;
    param.rate = MIC_SAMPLING_RATE_8180;
    param.fullArg = work;
    if (MIC_StartAutoSampling(&param) == MIC_RESULT_SUCCESS) {
        work->recording = TRUE;
        work->recordedSize = 0;
        return TRUE;
    }
    return FALSE;
}

static void CtvtMic_OnBufferFull(MICResult result, void *arg) {
    CtvtMic *work = arg;

    work->recording = FALSE;
    work->recordedSize = CTVT_MIC_BUFFER_SIZE;
}

BOOL CtvtMic_StopRecording(CtvtMic *work) {
    if (work->recording == FALSE) {
        return TRUE;
    }
    if (MIC_StopAutoSampling() == MIC_RESULT_SUCCESS) {
        work->recordedSize = (u8 *)func_0207e8f8() - (u8 *)work->buffer + 4;
        if (work->recordedSize > CTVT_MIC_BUFFER_SIZE) {
            work->recordedSize = CTVT_MIC_BUFFER_SIZE;
        }
        work->recording = FALSE;
        return TRUE;
    }
    return FALSE;
}

BOOL CtvtMic_IsRecording(CtvtMic *work) {
    return work->recording;
}

u32 CtvtMic_GetRecordedSize(CtvtMic *work) {
    if (work->recordedSize > CTVT_MIC_SKIPPED_SIZE) {
        return work->recordedSize - CTVT_MIC_SKIPPED_SIZE;
    }
    return 0;
}

void *CtvtMic_GetBuffer(CtvtMic *work) {
    return work->buffer;
}

u32 CtvtMic_Encode(CtvtMic *work, const s16 *src, u8 *dst, u32 size) {
    sys_memset(dst, 0, size);
    return Adpcm_Encode(src, size, dst);
}

u32 CtvtMic_Decode(CtvtMic *work, const s8 *src, s16 *dst, u32 size) {
    sys_memset32(0, dst, size * 4);
    return Adpcm_Decode(src, size, dst);
}

static void CtvtMic_AllocChannel(CtvtMic *work, HeapID heapId) {
    work->channel = NNS_SndWaveOutAllocChannel(CTVT_MIC_WAVE_CHANNEL);
    // "Failed to get a wave handle!!"
    GFL_ASSERT_MSG(
        work->channel != NULL,
        "Wave\x83\x6e\x83\x93\x83\x68\x83\x8b\x82\xcc\x8a\x6d\x95\xdb\x82\xc9\x8e\xb8\x94\x73\x81\x49\x81\x49\n");
    work->playing = FALSE;
}

static void CtvtMic_FreeChannel(CtvtMic *work) {
    NNS_SndWaveOutFreeChannel(work->channel);
}

void CtvtMic_Debug(CtvtMic *work) {
}

BOOL CtvtMic_Play(CtvtMic *work, const void *data, u32 size, int volume, int speed) {
    if (size > 32) {
        work->playing = NNS_SndWaveOutStart(work->channel, NNS_SND_WAVE_FORMAT_PCM16, (const u8 *)data + 0x800, FALSE,
                                            0, size / 2, CTVT_MIC_RATE, volume, speed, 64);
        work->playFrames = 0;
        work->playSize = size;
        work->playSpeed = speed;
        return work->playing;
    }
    return FALSE;
}

void CtvtMic_StopPlaying(CtvtMic *work) {
    work->playing = FALSE;
    NNS_SndWaveOutStop(work->channel);
}

BOOL CtvtMic_IsPlaying(CtvtMic *work) {
    return work->playing;
}

u32 CtvtMic_GetPlaySize(CtvtMic *work) {
    return work->playSize;
}

u16 CtvtMic_GetPlayFrames(CtvtMic *work) {
    return work->playFrames;
}

// The length of the playback in frames: the samples at 8180 Hz, at the playback's speed
u16 CtvtMic_GetPlayLength(CtvtMic *work) {
    return (work->playSize << 15) / (270 * work->playSpeed);
}

BOOL CtvtMic_IsReady(CtvtMic *work) {
    if (work->frames >= CTVT_MIC_WARMUP_FRAMES) {
        return TRUE;
    }
    return FALSE;
}
