#include "vct_internal.h"

#include "nitro/mi.h"
#include "nitro/os.h"

// The memory the packets come in, the microphone's levels, a filter on its samples, and a canceller of the echo of
// the speaker. The divider and the square root unit of the hardware do the sums that need them.

// The packets that are free, and the memory they are made of
typedef struct VCTPacketPool {
    VCTPacket *freeList;
    VCTPacket *base;
} VCTPacketPool;

// What the library has made of the microphone's loudness
typedef struct VCTCapture {
    // Whether somebody is speaking
    u32 speaking;
    u32 level;
    // How many quiet frames end the speaking
    int quietFrameLimit;
    int unkC;
} VCTCapture;

// How the loudness of the frames before has set the level that a voice has to rise above
typedef struct VCTNoiseFloor {
    u32 threshold;
    u32 index;
    int quietFrames;
} VCTNoiseFloor;

typedef struct VCTEcho {
    s16 attenuation;
    int highpassMinusInput;
    int readIndex;
    int writeIndex;
    int highpassOutput;
    s32 coefficients[6];
    s32 taps[6];
} VCTEcho;

static VCTCapture sCapture = {0, 0, 15, 0x3fc};
static s16 sInitialCoefficients[6] = {-5637, 16445, -14404, 11473, -12246, 892};

static s16 sEchoHistory[0x800];
static s16 sFarEnd[VCT_FRAME_SAMPLES];
static VCTEcho sEcho;
static u32 sLoudnessHistory[4];
static VCTNoiseFloor sNoiseFloor;
static VCTPacketPool sPacketPool;

BOOL VCT_InitPacketPool(void *buffer, u32 size)
{
    u32 count = size / sizeof(VCTPacket);
    VCTPacket *packets;
    u32 i;

    sPacketPool.base = buffer;
    if (count < 4 || count > 0x48) {
        return FALSE;
    }
    if (buffer == NULL) {
        return FALSE;
    }
    if ((u32)buffer & 0x1f) {
        sPacketPool.base = NULL;
        return FALSE;
    }
    MI_CpuClearFast(buffer, size);
    packets = sPacketPool.base;
    for (i = 0; i < count - 1; i++) {
        packets[i].nextFree = &packets[i + 1];
    }
    packets[count - 1].nextFree = NULL;
    sPacketPool.freeList = sPacketPool.base;
    return TRUE;
}

void VCT_ClearPacketPool(void)
{
    sPacketPool.freeList = NULL;
    sPacketPool.base = NULL;
}

VCTPacket *VCT_AllocPacket(void)
{
    VCTPacket *packet = NULL;
    u32 irq = CPU_IRQDisable();
    VCTPacket *head = sPacketPool.freeList;

    if (head != NULL) {
        packet = head;
        sPacketPool.freeList = head->nextFree;
    }
    CPU_SetIRQMask(irq);
    return packet;
}

void VCT_FreePacket(VCTPacket *packet)
{
    u32 irq = CPU_IRQDisable();

    packet->nextFree = sPacketPool.freeList;
    sPacketPool.freeList = packet;
    packet->prev = NULL;
    packet->next = NULL;
    CPU_SetIRQMask(irq);
}

void VCT_ResetCapture(void)
{
    int i;

    for (i = 0; i < 4; i++) {
        sLoudnessHistory[i] = 0x1000000;
    }
    sNoiseFloor.index = 0;
    sNoiseFloor.threshold = 0;
    sCapture.speaking = 0;
    sCapture.level = 0;
    sNoiseFloor.quietFrames = 0;
}

void VCT_GetCaptureLevels(s32 *out)
{
    if (out != NULL) {
        sys_memcpy(&sCapture, out, sizeof(VCTCapture));
    }
}

// Decides from the loudness of a frame whether a voice has started, goes on or has ended: 0 for none, 1 for the start,
// 2 for the middle and 3 for the end
int VCT_UpdateVad(u32 energy, u32 rms, u8 *level)
{
    u32 scaled = rms >> 8;
    int result;
    int i;

    sCapture.level = scaled;
    if (level != NULL) {
        *level = scaled;
    }
    if (sCapture.speaking == 0) {
        u32 sum = 0;
        u32 floor;

        for (i = 0; i < 4; i++) {
            sum += sLoudnessHistory[i];
        }
        REG_SQRTCNT = 0;
        REG_SQRT_PARAM_LO = sum / 4;
        while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
        }
        floor = REG_SQRT_RESULT;
        if (scaled != 0 && scaled >= floor * 2) {
            sum = 0;
            for (i = 0; i < 4; i++) {
                sum += sLoudnessHistory[i];
            }
            REG_SQRTCNT = 0;
            REG_SQRT_PARAM_LO = sum / 4;
            while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
            }
            sNoiseFloor.threshold = REG_SQRT_RESULT * 3 / 2;
            sNoiseFloor.index = 0;
            sCapture.speaking = 1;
            result = 1;
        } else {
            result = 0;
        }
        sLoudnessHistory[sNoiseFloor.index] = energy;
        sNoiseFloor.index = (sNoiseFloor.index + 1) & 3;
        return result;
    } else {
        u32 sum = 0;

        sLoudnessHistory[sNoiseFloor.index] = energy;
        sNoiseFloor.index = (sNoiseFloor.index + 1) & 3;
        for (i = 0; i < 4; i++) {
            sum += sLoudnessHistory[i];
        }
        REG_SQRTCNT = 0;
        REG_SQRT_PARAM_LO = sum / 4;
        while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
        }
        if (REG_SQRT_RESULT > sNoiseFloor.threshold) {
            sNoiseFloor.quietFrames = 0;
            return 2;
        }
        sNoiseFloor.quietFrames++;
        if (sNoiseFloor.quietFrames <= sCapture.quietFrameLimit) {
            return 2;
        }
        sCapture.speaking = 0;
        sNoiseFloor.quietFrames = 0;
        sNoiseFloor.index = 0;
        return 3;
    }
}

void VCT_InitCapture(void)
{
    s16 frames = 15;
    s16 frameUnits = 0x44;

    sCapture.quietFrameLimit = frames;
    sCapture.unkC = frames * frameUnits;
    VCT_ResetCapture();
}

// Filters a frame in place with a high-pass filter and returns the mean of the squares of the result
u32 VCT_FilterFrame(s16 *samples, int bytes)
{
    s16 input = samples[0];
    int minusInput = -(input * 0xe9f);
    int output = ((input * 0xe9f) + sEcho.highpassMinusInput + sEcho.highpassOutput * 0xd3e) >> 12;
    u64 sum = output * output;
    u32 count = bytes >> 1;
    u32 i;

    sEcho.highpassOutput = output;
    sEcho.highpassMinusInput = minusInput;
    for (i = 1; i < count; i++) {
        int scaled = samples[i] * 0xe9f;
        int next = scaled + minusInput + output * 0xd3e;

        samples[i - 1] = output;
        output = next >> 12;
        sum += output * output;
        minusInput = -scaled;
    }
    sEcho.highpassMinusInput = minusInput;
    sEcho.highpassOutput = output;
    samples[count - 1] = output;
    REG_DIVCNT = 1;
    REG_DIV_NUMER_LO = (u32)sum;
    REG_DIV_NUMER_HI = (u32)(sum >> 32);
    REG_DIV_DENOM_LO = count;
    REG_DIV_DENOM_HI = 0;
    while (REG_DIVCNT & REG_DIVCNT_BUSY) {
    }
    return REG_DIV_RESULT_LO;
}

void VCT_InitEcho(void)
{
    int i;

    sEcho.writeIndex = 0;
    sEcho.readIndex = 0x19d;
    for (i = 0; i < 0x800; i++) {
        sEchoHistory[i] = 0;
    }
    for (i = 0; i < 6; i++) {
        sEcho.taps[i] = 0;
        sEcho.coefficients[i] = sInitialCoefficients[i];
    }
}

// Keeps the frame that was played, to know what the microphone heard of it
void VCT_SaveFarEnd(s16 *samples, int bytes)
{
    sys_memcpy32_fast(samples, sFarEnd, bytes);
}

// Runs the speaker's frame through a filter of the echo's way back, and turns the microphone down by more the louder
// the echo is
static void SuppressEcho(s16 *mic, s16 *far, int bytes, u32 rms)
{
    u64 energy = 0;
    u32 count = bytes >> 1;
    u32 i;

    for (i = 0; i < count; i++) {
        s32 sum;
        s16 sample;

        sEchoHistory[sEcho.writeIndex] = far[i];
        sEcho.taps[0] = sEchoHistory[sEcho.readIndex];
        sum = sEcho.taps[4] * sEcho.coefficients[4] + sEcho.taps[5] * sEcho.coefficients[5];
        sEcho.taps[5] = sEcho.taps[4];
        sum += sEcho.taps[3] * sEcho.coefficients[3];
        sEcho.taps[4] = sEcho.taps[3];
        sum += sEcho.taps[2] * sEcho.coefficients[2];
        sEcho.taps[3] = sEcho.taps[2];
        sum += sEcho.taps[1] * sEcho.coefficients[1];
        sEcho.taps[2] = sEcho.taps[1];
        sum += sEcho.taps[0] * sEcho.coefficients[0];
        sEcho.taps[1] = sEcho.taps[0];
        if (sum > 0x3fff8000) {
            sum = 0x3fff8000;
        } else if (sum < -0x40000000) {
            sum = -0x40000000;
        }
        sEcho.writeIndex = (sEcho.writeIndex + 1) & 0x7ff;
        sEcho.readIndex = (sEcho.readIndex + 1) & 0x7ff;
        far[i] = sum >> 15;
        sample = far[i];
        far[i] = mic[i];
        energy += sample * sample;
    }
    REG_DIVCNT = 1;
    REG_DIV_NUMER_LO = (u32)energy;
    REG_DIV_NUMER_HI = (u32)(energy >> 32);
    REG_DIV_DENOM_LO = count;
    REG_DIV_DENOM_HI = 0;
    while (REG_DIVCNT & REG_DIVCNT_BUSY) {
    }
    REG_SQRTCNT = 0;
    REG_SQRT_PARAM_LO = REG_DIV_RESULT_LO;
    while (REG_SQRTCNT & REG_SQRTCNT_BUSY) {
    }
    if (REG_SQRT_RESULT * 3 < rms * 2) {
        sEcho.attenuation = 0;
        return;
    }
    if (rms < REG_SQRT_RESULT) {
        sEcho.attenuation = 4;
    }
    if (sEcho.attenuation < 4) {
        sEcho.attenuation++;
    }
    for (i = 0; i < count; i++) {
        mic[i] >>= sEcho.attenuation;
    }
}

void VCT_CancelEcho(s16 *samples, int bytes, u32 rms)
{
    SuppressEcho(samples, sFarEnd, bytes, rms);
}

void VCT_MixSamples(s16 *a, s16 *b, s16 *out, u32 bytes)
{
    u32 count = bytes >> 1;
    u32 i;

    for (i = 0; i < count; i += 4) {
        int sum = a[i] + b[i];

        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i] = sum;
        sum = a[i + 1] + b[i + 1];
        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i + 1] = sum;
        sum = a[i + 2] + b[i + 2];
        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i + 2] = sum;
        sum = a[i + 3] + b[i + 3];
        if (sum > 0x7fff) {
            sum = 0x7fff;
        } else if (sum < -0x8000) {
            sum = -0x8000;
        }
        out[i + 3] = sum;
    }
}
