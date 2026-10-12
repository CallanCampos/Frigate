#include "types.h"
#include "gfl/sound.h"
#include "gfl/dwc_vchat.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "save/chatter.h"
#include "twl/mic.h"

// The microphone, which records a Chatot's cry. The name is the ROM's string.

// A second of samples at 8180 Hz, which is what a Chatot's recording holds
#define SND_MIC_SAMPLES PERAP_VOICE_DECODED_SIZE
// The buffer has room to start at a 32-byte boundary
#define SND_MIC_BUFFER_SIZE (SND_MIC_SAMPLES + 0x20)
// How far the count has to go for func_02006e3c
#define SND_MIC_COUNT_MAX 180

typedef struct {
    MICAutoParam param;
    BOOL sampling;
    u32 enabled : 1;
    u32 count : 31;
    void *buffer;
} SndMic;

static MICResult func_02006e54(MICAutoParam *param);
static void *func_02006ed4(void);

static u16 sMicFilter[6] = { 0x7e46, 0x81ba, 0x7e46, 0x7e43, 0x836e, 0 };

static SndMic *sSndMic;

void setupMic(HeapID heapId) {
    SndMic *mic;
    void *buffer;

    mic = GFL_HeapAllocate(heapId, sizeof(SndMic), FALSE, "snd_mic.c", 89);
    sys_memset(mic, 0, sizeof(SndMic));
    buffer = GFL_HeapAllocate(heapId, SND_MIC_BUFFER_SIZE, FALSE, "snd_mic.c", 95);
    sys_memset(buffer, 0, SND_MIC_BUFFER_SIZE);
    mic->buffer = buffer;
    func_0207efc4(PM_AMP_ON);
    func_0207eff4(2);
    mic->enabled = TRUE;
    func_0207e75c();
    if (hw_isDSi() == TRUE) {
        func_027047c0(0, sMicFilter);
    }
    sSndMic = mic;
}

void ampOffFreeBlocks(void) {
    func_0207efc4(PM_AMP_OFF);
    GFL_HeapFree(sSndMic->buffer);
    GFL_HeapFree(sSndMic);
}

void func_02006e0c(u32 a0) {
    SndMic *mic = sSndMic;

    if (mic->enabled && !func_02006e3c()) {
        mic->count += a0;
    }
}

BOOL func_02006e3c(void) {
    return sSndMic->count > SND_MIC_COUNT_MAX;
}

static MICResult func_02006e54(MICAutoParam *param) {
    MICResult result = MIC_StartAutoSampling(param);

    sSndMic->sampling = TRUE;
    sSndMic->param = *param;
    return result;
}

u32 func_02006e80(void (*callback)(u32 result, u32 *done), u32 *done) {
    MICAutoParam param;

    param.type = 2;
    param.buffer = func_02006ed4();
    param.size = SND_MIC_SAMPLES;
    if (param.size & 0x1f) {
        param.size &= ~0x1f;
    }
    param.rate = MIC_SAMPLING_RATE_8180;
    param.loop = FALSE;
    param.fullCallback = (MICCallback)callback;
    param.fullArg = done;
    return func_02006e54(&param);
}

void func_02006ec0(void *chatter) {
    PerapVoice_Encode(chatter, func_02006ed4());
}

// The buffer from its first 32-byte boundary
static void *func_02006ed4(void) {
    return (void *)(((u32)sSndMic->buffer + 0x1f) & ~0x1f);
}
