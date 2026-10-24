#include "types.h"
#include "gfl/dwc_vchat.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "save/chatter.h"
#include "twl/sndex.h"

// The microphone, which records a Chatot's cry. The name is the ROM's string.

// A second of samples at 8180 Hz, which is what a Chatot's recording holds
#define SND_MIC_SAMPLES PERAP_VOICE_DECODED_SIZE
// The buffer has room to start at a 32-byte boundary
#define SND_MIC_BUFFER_SIZE (SND_MIC_SAMPLES + 0x20)
// How far the count has to go for SndMic_IsOverLimit
#define SND_MIC_COUNT_MAX 180

typedef struct {
    MICAutoParam param;
    BOOL sampling;
    u32 enabled : 1;
    u32 count : 31;
    void *buffer;
} SndMic;

static MICResult SndMic_StartSampling(MICAutoParam *param);
static void *SndMic_GetAlignedBuffer(void);

// The microphone filter's coefficients, an SNDEXIirFilterParam, and a sixth halfword that is not sent
static u16 sMicFilter[6] = { 0x7e46, 0x81ba, 0x7e46, 0x7e43, 0x836e, 0 };

static SndMic *sSndMic;

void setupMic(HeapID heapId) {
    SndMic *mic;
    void *buffer;

    mic = GFL_HeapAllocate(heapId, sizeof(SndMic), FALSE, "snd_mic.c", 89);
    MI_CpuFill8(mic, 0, sizeof(SndMic));
    buffer = GFL_HeapAllocate(heapId, SND_MIC_BUFFER_SIZE, FALSE, "snd_mic.c", 95);
    MI_CpuFill8(buffer, 0, SND_MIC_BUFFER_SIZE);
    mic->buffer = buffer;
    PM_SetAmp(PM_AMP_ON);
    PM_SetAmpGain(PM_AMPGAIN_80);
    mic->enabled = TRUE;
    MIC_Init();
    SNDEX_SetIirFilter(SNDEX_IIR_FILTER_ADC_1, (const SNDEXIirFilterParam *)sMicFilter);
    sSndMic = mic;
}

void ampOffFreeBlocks(void) {
    PM_SetAmp(PM_AMP_OFF);
    GFL_HeapFree(sSndMic->buffer);
    GFL_HeapFree(sSndMic);
}

void SndMic_AddCount(u32 a0) {
    SndMic *mic = sSndMic;

    if (mic->enabled && !SndMic_IsOverLimit()) {
        mic->count += a0;
    }
}

BOOL SndMic_IsOverLimit(void) {
    return sSndMic->count > SND_MIC_COUNT_MAX;
}

static MICResult SndMic_StartSampling(MICAutoParam *param) {
    MICResult result = MIC_StartLimitedSampling(param);

    sSndMic->sampling = TRUE;
    sSndMic->param = *param;
    return result;
}

u32 SndMic_StartRecording(void (*callback)(u32 result, u32 *done), u32 *done) {
    MICAutoParam param;

    param.type = 2;
    param.buffer = SndMic_GetAlignedBuffer();
    param.size = SND_MIC_SAMPLES;
    if (param.size & 0x1f) {
        param.size &= ~0x1f;
    }
    param.rate = MIC_SAMPLING_RATE_8180;
    param.loop_enable = FALSE;
    param.full_callback = (MICCallback)callback;
    param.full_arg = done;
    return SndMic_StartSampling(&param);
}

void SndMic_EncodeChatter(PerapVoice *chatter) {
    PerapVoice_Encode(chatter, SndMic_GetAlignedBuffer());
}

// The buffer from its first 32-byte boundary
static void *SndMic_GetAlignedBuffer(void) {
    return (void *)(((uptr)sSndMic->buffer + 0x1f) & ~0x1f);
}
