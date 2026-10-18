// The sound unit of the host's ARM7 (port/hal/snd7): its registers, as the driver's functions of snd_channel.c,
// snd_capture.c and snd_global.c write them (the ARM7's 0x037ffd60-0x0380036c, 0x038007b4-0x03800818), and the unit
// itself, which the DS has in hardware: 16 channels that play PCM8, PCM16 and IMA-ADPCM samples from memory, PSG
// square waves (channels 8-13) and noise (14 and 15), and the mixer that sums them into the left and right outputs.
// Every sound the game makes, its music, sound effects and cries, comes out of this unit; the oracle comparison holds
// its output to melonDS's sample by sample (port/tools/oracle/README.md, "Sound").
//
// The unit as GBATEK describes it:
//   - a channel's timer counts the sound clock (half the ARM7's bus clock) up from SOUNDxTMR, and each time it
//     overflows the channel moves on to its next sample and the timer goes on from SOUNDxTMR, so a sample lasts
//     0x10000 - TMR ticks. A write to SOUNDxTMR leaves the running count alone: the new value counts from the next
//     overflow, so a channel's pitch changes (an LFO's, a sweep's, a bend's) take effect at its next step;
//   - SOUNDxCNT: volume (bits 0-6), the data shift (bits 8-9: 0, 1, 2 or 4 bits to the right), hold (15), pan (16-22,
//     0 left, 64 center, 127 right), the PSG duty (24-26), the repeat mode (27-28: manual, loop, one-shot), the format
//     (29-30: PCM8, PCM16, IMA-ADPCM, PSG/noise) and start (31), which the channel clears when a one-shot sound ends.
//     A channel that isn't playing outputs its last sample with hold set, else 0;
//   - SOUNDxSAD, SOUNDxPNT and SOUNDxLEN: where the samples are, and the loop's start and length in words. The sound is
//     (PNT + LEN) words; a loop goes back to PNT. IMA-ADPCM data starts with a header word (the first PCM16 value and
//     table index) and has 8 samples a word, low nibble first; the decoder's state at the loop's start is kept the
//     first time it gets there and set again at each loop;
//   - a PCM or IMA-ADPCM sound of fewer than 4 words (PNT + LEN) hangs: start stays set and the channel outputs nothing
//     until the driver clears it;
//   - a sound starts late: a PCM sound's first sample comes on the channel's third timer step, an IMA-ADPCM sound's on
//     its eleventh (three, then the header word's eight nibbles), a square wave's or noise's on its first, and the
//     channel outputs 0 until then;
//   - a PCM or IMA-ADPCM channel reads its sound ahead of what it plays, through a FIFO of 8 words: 8 when it starts,
//     then 4 more each time it has 4 or fewer left, wrapping to the loop's start, so that memory the ARM9 rewrites
//     just ahead of the channel (a stream's buffer) plays as it was. Those two are melonDS's model of the DS
//     (SPUChannel::Start and its FIFO), which the port's oracle comparison holds the unit to;
//   - a PSG channel plays 8 steps per period: for a duty of 0 to 6, low for 7 - duty steps and then high for duty + 1,
//     from the start of the low part; at duty 7 (0%), low throughout. A noise channel shifts a 15-bit register (0x7fff
//     at the start) right, and outputs low and xors 0x6000 in when a 1 drops out, high when a 0 does. High and low are
//     +0x7fff and -0x7fff;
//   - the mixer takes a sample of every channel each 512 sound clock ticks, after the channels' timers have counted
//     those ticks: the sample (16 bits) divided by the data shift (16.4 bits), times the volume (16.11), times 128 -
//     pan to the left and pan to the right with the fraction's low 10 bits dropped (16.8); the channels' sum (20.8), or
//     channel 1 and/or 3 alone as SOUNDCNT selects, times the master volume (SOUNDCNT bits 0-6) over 128, and without
//     its fraction. GBATEK gives those widths, and melonDS, the port's oracle, has them, with a volume, pan or master
//     volume of 127 counting as 128. SOUNDCNT bit 15 enables the output and the channels: while it is clear they don't
//     move on.
// The DS turns the result into 10 bits for its PWM output, clipped. This unit outputs the 16 bits that melonDS outputs
// by default: on the scale of the DS's 10 bits times 64, so that a channel at full volume and center pan is about
// half the range, and clipped at the same level (-0x8000 to 0x7fff), but with the 6 low bits kept, where the DS's
// output has none (melonDS drops them only when told to, and the oracle doesn't tell it). The capture units'
// registers are kept (SND_SetupCapture, their start and status) but they capture nothing: Black 2 and White 2 never
// start them.
//
// A channel's samples are read through host pointers: SOUNDxSAD is a host pointer and the end of the memory it is in,
// and a channel reads 0 past that end.
#include "hal/snd7/snd7_internal.h"
#include <string.h>

// The channels whose volume the surround decay changes: not 1 and 3, which the effects use
#define SURROUND_DECAY_CHANNELS 0xfff5

// SOUNDCNT's high byte (0x04000501)
#define CONTROL_LEFT_MASK 0x03
#define CONTROL_RIGHT_SHIFT 2
#define CONTROL_CHANNEL1_BYPASS 0x10
#define CONTROL_CHANNEL3_BYPASS 0x20
#define CONTROL_ENABLE 0x80

// The capture units' SNDCAPxCNT
#define CAPTURE_START 0x80

#define PSG_HIGH 0x7fff
#define PSG_LOW (-0x7fff)
#define NOISE_SEED 0x7fff
#define NOISE_XOR 0x6000

// The writable bits of SOUNDxCNT
#define CNT_WRITABLE 0xff7f837fu

// The fewest words (PNT + LEN) of a PCM or IMA-ADPCM sound that plays
#define SOUND_MIN_WORDS 4

// The channels' FIFO: 8 words, read 4 at a time from memory once 4 or fewer are left
#define FIFO_SIZE 32
#define FIFO_BURST 16

typedef struct HwChannel {
    u32 cnt;
    const u8 *sad;
    const u8 *sadEnd;
    u16 tmr;
    u16 pnt;
    u32 len;
    // The sound being played: its samples, the loop's first sample, the current one, the channel's timer (from
    // SOUNDxTMR up, a step at 0x10000), and the current sample's value
    u32 total;
    u32 loopStart;
    u32 index;
    u32 timer;
    s32 sample;
    // A sound too short to play: the channel stays busy, silent and still
    BOOL hung;
    // The timer steps before the sound's first sample, which the channel spends at 0
    u32 delay;
    // The FIFO (8 words, as bytes): where the channel reads and writes in it, the bytes it holds, the sound's next byte
    // to read into it, and whether it was filled at the start (at the first mixer period after it)
    u8 fifo[FIFO_SIZE];
    u32 fifoRead;
    u32 fifoWrite;
    u32 fifoLevel;
    u32 fetch;
    BOOL primed;
    // IMA-ADPCM: the byte whose two nibbles are being decoded
    u8 adpcmByte;
    // The mixer sample whose mixing takes the start first, counted from the unit's reset
    u32 startMix;
    // IMA-ADPCM: the decoder, and its state at the loop's start
    s32 adpcmValue;
    s32 adpcmIndex;
    s32 loopValue;
    s32 loopIndex;
    BOOL loopSaved;
    // PSG and noise
    u32 psgStep;
    u32 lfsr;
} HwChannel;

static struct {
    HwChannel channel[SND7_CHANNEL_NUM];
    u8 masterVolume;
    u8 control;
    u8 captureCnt[SND7_CAPTURE_NUM];
    u32 captureDest[SND7_CAPTURE_NUM];
    u16 captureLength[SND7_CAPTURE_NUM];
    // The driver's statics of snd_channel.c (0x0380935c and 0x03809a5c-0x03809a7f): the master pan (-1 for none), the
    // surround decay, and the pan and volume each channel was given, which those two change
    s32 masterPan;
    s32 surroundDecay;
    u8 orgPan[SND7_CHANNEL_NUM];
    u8 orgVolume[SND7_CHANNEL_NUM];
    // The mixer's samples since the reset
    u32 mixes;
} sHw;

// IMA-ADPCM's tables
static const s16 sAdpcmStepTable[89] = {
    0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x0010, 0x0011, 0x0013, 0x0015, 0x0017,
    0x0019, 0x001c, 0x001f, 0x0022, 0x0025, 0x0029, 0x002d, 0x0032, 0x0037, 0x003c, 0x0042, 0x0049, 0x0050,
    0x0058, 0x0061, 0x006b, 0x0076, 0x0082, 0x008f, 0x009d, 0x00ad, 0x00be, 0x00d1, 0x00e6, 0x00fd, 0x0117,
    0x0133, 0x0151, 0x0173, 0x0198, 0x01c1, 0x01ee, 0x0220, 0x0256, 0x0292, 0x02d4, 0x031c, 0x036c, 0x03c3,
    0x0424, 0x048e, 0x0502, 0x0583, 0x0610, 0x06ab, 0x0756, 0x0812, 0x08e0, 0x09c3, 0x0abd, 0x0bd0, 0x0cff,
    0x0e4c, 0x0fba, 0x114c, 0x1307, 0x14ee, 0x1706, 0x1954, 0x1bdc, 0x1ea5, 0x21b6, 0x2515, 0x28ca, 0x2cdf,
    0x315b, 0x364b, 0x3bb9, 0x41b2, 0x4844, 0x4f7e, 0x5771, 0x602f, 0x69ce, 0x7462, 0x7fff,
};

static const s8 sAdpcmIndexTable[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

// The data shift as the mixer applies it: how far a channel's samples go to the left before the volume, from 16
// bits to 16.4 (a shift of 0, 1, 2 or 4 bits to the right of that)
static const u8 sDataShift[4] = { 4, 3, 2, 0 };

// The timer steps before a sound's first sample: PCM, IMA-ADPCM (its header word's 8 nibbles after PCM's 3), and a
// square wave or noise. IMA-ADPCM's header comes from the FIFO on the step after PCM's delay
#define START_DELAY_PCM 3
#define START_DELAY_ADPCM 11
#define START_DELAY_PSG 1
#define ADPCM_HEADER_DELAY (START_DELAY_ADPCM - START_DELAY_PCM - 1)

void snd7_adpcm_step(int nibble, s32 *value, s32 *index) {
    s32 step = sAdpcmStepTable[*index];
    s32 diff = step >> 3;

    if (nibble & 1) {
        diff += step >> 2;
    }
    if (nibble & 2) {
        diff += step >> 1;
    }
    if (nibble & 4) {
        diff += step;
    }
    if (nibble & 8) {
        *value -= diff;
        if (*value < -0x7fff) {
            *value = -0x7fff;
        }
    } else {
        *value += diff;
        if (*value > 0x7fff) {
            *value = 0x7fff;
        }
    }
    *index += sAdpcmIndexTable[nibble & 7];
    if (*index < 0) {
        *index = 0;
    } else if (*index > 88) {
        *index = 88;
    }
}

// The sound unit

static u32 Format(const HwChannel *channel) {
    return (channel->cnt >> 29) & 3;
}

// A byte of the channel's memory, or 0 past its end
static u8 ReadByte(const HwChannel *channel, u32 offset) {
    if (channel->sad == NULL || (channel->sadEnd != NULL && offset >= (uintptr_t)(channel->sadEnd - channel->sad))) {
        return 0;
    }
    return channel->sad[offset];
}

// 4 words of the sound into the FIFO, from where it has read up to: after the sound's end, from its loop's start, or
// nothing for a one-shot sound
static void FifoBurst(HwChannel *channel) {
    u32 total = (channel->pnt + channel->len) * 4;
    u32 burst = FIFO_BURST;
    u32 i;

    if (channel->fetch >= total) {
        if (((channel->cnt >> 27) & 3) != SND7_REPEAT_LOOP) {
            return;
        }
        channel->fetch = channel->pnt * 4u;
    }
    if (channel->fetch + burst > total) {
        burst = total - channel->fetch;
    }
    for (i = 0; i < burst; i++) {
        channel->fifo[channel->fifoWrite] = ReadByte(channel, channel->fetch + i);
        channel->fifoWrite = (channel->fifoWrite + 1) % FIFO_SIZE;
    }
    channel->fetch += burst;
    channel->fifoLevel += burst;
}

// The FIFO's next bytes, little-endian; it reads 4 more words when 4 or fewer are left
static u32 FifoRead(HwChannel *channel, u32 size) {
    u32 value = 0;
    u32 i;

    for (i = 0; i < size; i++) {
        value |= (u32)channel->fifo[channel->fifoRead] << (8 * i);
        channel->fifoRead = (channel->fifoRead + 1) % FIFO_SIZE;
    }
    channel->fifoLevel = channel->fifoLevel >= size ? channel->fifoLevel - size : 0;
    if (channel->fifoLevel <= FIFO_BURST) {
        FifoBurst(channel);
    }
    return value;
}

// The FIFO as the channel starts its sound: 8 words
static void FifoPrime(HwChannel *channel) {
    channel->fifoRead = 0;
    channel->fifoWrite = 0;
    channel->fifoLevel = 0;
    channel->fetch = 0;
    FifoBurst(channel);
    FifoBurst(channel);
    channel->primed = TRUE;
}

// IMA-ADPCM's header word, the first PCM16 value and the table's index, from the FIFO
static void ReadAdpcmHeader(HwChannel *channel) {
    u32 header = FifoRead(channel, 4);

    channel->adpcmValue = (s16)header;
    channel->adpcmIndex = (header >> 16) & 0x7f;
    if (channel->adpcmIndex > 88) {
        channel->adpcmIndex = 88;
    }
}

// The sample at the channel's index, from the FIFO, which gives the sound's bytes in the order the samples come. An
// IMA-ADPCM sample is the decoder's value after its nibble, a byte's low one first
static void FetchSample(HwChannel *channel, int chNo) {
    u32 index = channel->index;

    switch (Format(channel)) {
    case SND7_FORMAT_PCM8:
        channel->sample = (s32)(s8)FifoRead(channel, 1) << 8;
        break;
    case SND7_FORMAT_PCM16:
        channel->sample = (s16)FifoRead(channel, 2);
        break;
    case SND7_FORMAT_ADPCM: {
        if (!(index & 1)) {
            channel->adpcmByte = (u8)FifoRead(channel, 1);
        }
        if (index == channel->loopStart && !channel->loopSaved) {
            channel->loopValue = channel->adpcmValue;
            channel->loopIndex = channel->adpcmIndex;
            channel->loopSaved = TRUE;
        }
        snd7_adpcm_step(index & 1 ? channel->adpcmByte >> 4 : channel->adpcmByte & 0xf, &channel->adpcmValue,
                        &channel->adpcmIndex);
        channel->sample = channel->adpcmValue;
        break;
    }
    default:
        if (chNo >= 8 && chNo <= 13) {
            u32 duty = (channel->cnt >> 24) & 7;

            channel->sample = duty != 7 && channel->psgStep >= 7 - duty ? PSG_HIGH : PSG_LOW;
        } else if (chNo >= 14) {
            BOOL carry = channel->lfsr & 1;

            channel->lfsr >>= 1;
            if (carry) {
                channel->lfsr ^= NOISE_XOR;
                channel->sample = PSG_LOW;
            } else {
                channel->sample = PSG_HIGH;
            }
        } else {
            channel->sample = 0;
        }
        break;
    }
}

// The start bit went from 0 to 1: the sound starts, its first sample after its delay, or hangs when it is too short to
// play. The FIFO fills at the next mixer period
static void StartSound(HwChannel *channel) {
    u32 words = channel->pnt + channel->len;

    channel->index = 0;
    channel->loopSaved = FALSE;
    channel->hung = FALSE;
    channel->sample = 0;
    channel->primed = FALSE;
    channel->startMix = sHw.mixes;
    if (Format(channel) != SND7_FORMAT_PSG && words < SOUND_MIN_WORDS) {
        channel->hung = TRUE;
        return;
    }
    switch (Format(channel)) {
    case SND7_FORMAT_PCM8:
        channel->total = words * 4;
        channel->loopStart = channel->pnt * 4u;
        channel->delay = START_DELAY_PCM;
        break;
    case SND7_FORMAT_PCM16:
        channel->total = words * 2;
        channel->loopStart = channel->pnt * 2u;
        channel->delay = START_DELAY_PCM;
        break;
    case SND7_FORMAT_ADPCM:
        channel->total = (words - 1) * 8;
        channel->loopStart = channel->pnt != 0 ? (channel->pnt - 1u) * 8 : 0;
        channel->delay = START_DELAY_ADPCM;
        break;
    default:
        channel->psgStep = 0;
        channel->lfsr = NOISE_SEED;
        channel->total = 0;
        channel->delay = START_DELAY_PSG;
        break;
    }
}

// One step of the channel's timer: the next sample, a loop, or the end of a one-shot sound; the first sample at the
// end of the start's delay
static void NextSample(HwChannel *channel, int chNo) {
    if (channel->delay != 0) {
        channel->delay--;
        if (channel->delay == ADPCM_HEADER_DELAY && Format(channel) == SND7_FORMAT_ADPCM) {
            ReadAdpcmHeader(channel);
        } else if (channel->delay == 0) {
            FetchSample(channel, chNo);
        }
        return;
    }
    if (Format(channel) == SND7_FORMAT_PSG) {
        channel->psgStep = (channel->psgStep + 1) & 7;
        FetchSample(channel, chNo);
        return;
    }
    channel->index++;
    if (channel->index >= channel->total) {
        if (((channel->cnt >> 27) & 3) != SND7_REPEAT_LOOP || channel->loopStart >= channel->total) {
            channel->cnt &= ~SND7_CNT_START;
            return;
        }
        channel->index = channel->loopStart;
        if (Format(channel) == SND7_FORMAT_ADPCM && channel->loopSaved) {
            channel->adpcmValue = channel->loopValue;
            channel->adpcmIndex = channel->loopIndex;
            // The loop's start is decoded again from the state kept there
            channel->loopSaved = FALSE;
        }
    }
    FetchSample(channel, chNo);
}

static void WriteCnt(int chNo, u32 value) {
    HwChannel *channel = &sHw.channel[chNo];
    u32 old = channel->cnt;

    channel->cnt = value & CNT_WRITABLE;
    if (!(old & SND7_CNT_START) && (channel->cnt & SND7_CNT_START)) {
        StartSound(channel);
    }
}

// A volume, pan or master volume of 127 counts as 128
static s32 Factor(u32 value) {
    return value == 127 ? 128 : (s32)value;
}

void snd7_hw_mix(s16 out[2]) {
    s64 mixLeft = 0;
    s64 mixRight = 0;
    s64 left[SND7_CHANNEL_NUM];
    s64 right[SND7_CHANNEL_NUM];
    s64 outLeft;
    s64 outRight;
    int i;

    sHw.mixes++;
    if (!(sHw.control & CONTROL_ENABLE)) {
        out[0] = 0;
        out[1] = 0;
        return;
    }
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        HwChannel *channel = &sHw.channel[i];
        BOOL busy;
        s32 sample;
        s32 value;
        s32 pan;

        // The channel moves on by one mixer period, and the mixer takes the sample it is on then. A sound that just
        // started fills its FIFO and loads its timer from SOUNDxTMR first. Each overflow reloads the timer from
        // SOUNDxTMR as it is then
        if ((channel->cnt & SND7_CNT_START) && !channel->hung) {
            if (!channel->primed) {
                if (Format(channel) != SND7_FORMAT_PSG) {
                    FifoPrime(channel);
                }
                channel->timer = channel->tmr;
                channel->primed = TRUE;
            }

            channel->timer += SND7_SOUND_TICKS_PER_SAMPLE;
            while ((channel->timer >> 16) != 0 && (channel->cnt & SND7_CNT_START)) {
                channel->timer = channel->tmr + (channel->timer - 0x10000u);
                NextSample(channel, i);
            }
        }
        busy = (channel->cnt & SND7_CNT_START) != 0;
        sample = busy || (channel->cnt & SND7_CNT_HOLD) ? channel->sample : 0;
        value = sample * (1 << sDataShift[(channel->cnt >> 8) & 3]) * Factor(channel->cnt & 0x7f);
        pan = Factor((channel->cnt >> 16) & 0x7f);
        left[i] = (s64)value * (128 - pan) >> 10;
        right[i] = (s64)value * pan >> 10;
        if (!((i == 1 && (sHw.control & CONTROL_CHANNEL1_BYPASS)) ||
              (i == 3 && (sHw.control & CONTROL_CHANNEL3_BYPASS)))) {
            mixLeft += left[i];
            mixRight += right[i];
        }
    }
    switch (sHw.control & CONTROL_LEFT_MASK) {
    case 0:
        outLeft = mixLeft;
        break;
    case 1:
        outLeft = left[1];
        break;
    case 2:
        outLeft = left[3];
        break;
    default:
        outLeft = left[1] + left[3];
        break;
    }
    switch ((sHw.control >> CONTROL_RIGHT_SHIFT) & 3) {
    case 0:
        outRight = mixRight;
        break;
    case 1:
        outRight = right[1];
        break;
    case 2:
        outRight = right[3];
        break;
    default:
        outRight = right[1] + right[3];
        break;
    }
    // The master volume, then the fraction's 8 bits off
    outLeft = (outLeft * Factor(sHw.masterVolume & 0x7f) >> 7) >> 8;
    outRight = (outRight * Factor(sHw.masterVolume & 0x7f) >> 7) >> 8;
    out[0] = (s16)(outLeft > 0x7fff ? 0x7fff : outLeft < -0x8000 ? -0x8000 : outLeft);
    out[1] = (s16)(outRight > 0x7fff ? 0x7fff : outRight < -0x8000 ? -0x8000 : outRight);
}

void snd7_hw_reset(void) {
    memset(&sHw, 0, sizeof(sHw));
    sHw.masterPan = -1;
}

void snd7_hw_peek(int chNo, Snd7HwPeek *peek) {
    const HwChannel *channel = &sHw.channel[chNo];

    peek->cnt = channel->cnt;
    peek->sad = channel->sad;
    peek->tmr = channel->tmr;
    peek->pnt = channel->pnt;
    peek->len = channel->len;
    peek->index = channel->index;
    peek->sample = channel->sample;
    peek->startMix = channel->startMix;
}

int snd7_hw_master_volume(void) {
    return sHw.masterVolume;
}

// snd_global.c

// SND_Enable (0x037ffd60)
void snd7_enable(void) {
    sHw.control |= CONTROL_ENABLE;
}

// SND_Disable (0x037ffd78): the output and every channel stop, holding their samples, and the capture units
void snd7_disable(void) {
    int i;

    sHw.control &= ~CONTROL_ENABLE;
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        snd7_stop_channel(i, SND7_CHANNEL_STOP_HOLD);
    }
    sHw.captureCnt[0] = 0;
    sHw.captureCnt[1] = 0;
}

// SND_SetMasterVolume (0x037ffe88)
void snd7_set_master_volume(int volume) {
    sHw.masterVolume = (u8)volume;
}

// SND_SetOutputSelector (0x037ffe98): SOUNDCNT's high byte, keeping its enable bit
void snd7_set_output_selector(int left, int right, int channel1, int channel3) {
    u32 enable = (sHw.control & CONTROL_ENABLE) ? 1 : 0;

    sHw.control = (u8)(left | right << 2 | channel1 << 4 | channel3 << 5 | enable << 7);
}

// snd_channel.c

// The volume of a channel that the surround decay lowers (0x0380030c): channels panned far to a side lose some of it.
// The products wrap at 32 bits, as the ARM's multiplications do
static int CalcSurroundDecay(int volume, int pan) {
    u32 decay = (u32)sHw.surroundDecay;
    u32 factor;

    if (pan < 24) {
        factor = decay * (u32)(pan + 40) + ((0x7fffu - decay) << 6);
    } else if (pan <= 104) {
        return volume;
    } else {
        factor = (0u - decay) * (u32)(pan - 40) + ((decay + 0x7fffu) << 6);
    }
    return (s32)((u32)volume * factor) >> 21;
}

static BOOL HasSurroundDecay(int chNo) {
    return sHw.surroundDecay > 0 && (SURROUND_DECAY_CHANNELS & (1 << chNo));
}

// SND_SetupChannelPcm (0x037ffed0): every register but start, which stops the channel
void snd7_setup_channel_pcm(int chNo, const u8 *data, const u8 *dataEnd, int format, int loop, int loopStart,
                            int loopLen, int volume, int shift, int timer, int pan) {
    HwChannel *channel = &sHw.channel[chNo];

    sHw.orgPan[chNo] = (u8)pan;
    if (sHw.masterPan >= 0) {
        pan = sHw.masterPan;
    }
    sHw.orgVolume[chNo] = (u8)volume;
    if (HasSurroundDecay(chNo)) {
        volume = CalcSurroundDecay(volume, pan);
    }
    WriteCnt(chNo, (u32)volume | (u32)shift << 8 | (u32)pan << 16 | (u32)loop << 27 | (u32)format << 29);
    channel->tmr = (u16)(0x10000 - timer);
    channel->pnt = (u16)loopStart;
    channel->len = (u32)loopLen & 0x3fffff;
    channel->sad = data;
    channel->sadEnd = dataEnd;
}

// SND_SetupChannelPsg (0x037fff9c)
void snd7_setup_channel_psg(int chNo, int duty, int volume, int shift, int timer, int pan) {
    sHw.orgPan[chNo] = (u8)pan;
    if (sHw.masterPan >= 0) {
        pan = sHw.masterPan;
    }
    sHw.orgVolume[chNo] = (u8)volume;
    if (HasSurroundDecay(chNo)) {
        volume = CalcSurroundDecay(volume, pan);
    }
    WriteCnt(chNo, (u32)volume | (u32)shift << 8 | (u32)pan << 16 | (u32)duty << 24 | 0x60000000u);
    sHw.channel[chNo].tmr = (u16)(0x10000 - timer);
}

// SND_SetupChannelNoise (0x03800048)
void snd7_setup_channel_noise(int chNo, int volume, int shift, int timer, int pan) {
    sHw.orgPan[chNo] = (u8)pan;
    if (sHw.masterPan >= 0) {
        pan = sHw.masterPan;
    }
    sHw.orgVolume[chNo] = (u8)volume;
    if (HasSurroundDecay(chNo)) {
        volume = CalcSurroundDecay(volume, pan);
    }
    WriteCnt(chNo, (u32)volume | (u32)shift << 8 | (u32)pan << 16 | 0x60000000u);
    sHw.channel[chNo].tmr = (u16)(0x10000 - timer);
}

// SND_StopChannel (0x038000ec): with SND_CHANNEL_STOP_HOLD the channel keeps its last sample
void snd7_stop_channel(int chNo, s32 flags) {
    u32 cnt = sHw.channel[chNo].cnt & ~SND7_CNT_START;

    if (flags & SND7_CHANNEL_STOP_HOLD) {
        cnt |= SND7_CNT_HOLD;
    }
    WriteCnt(chNo, cnt);
}

// SND_SetChannelVolume (0x03800114): a halfword store to SOUNDxCNT's volume and shift (which clears hold)
void snd7_set_channel_volume(int chNo, int volume, int shift) {
    HwChannel *channel = &sHw.channel[chNo];

    sHw.orgVolume[chNo] = (u8)volume;
    if (HasSurroundDecay(chNo)) {
        volume = CalcSurroundDecay(volume, (channel->cnt >> 16) & 0xff);
    }
    WriteCnt(chNo, (channel->cnt & 0xffff0000u) | (((u32)volume | (u32)shift << 8) & 0xffff));
}

// SND_SetChannelTimer (0x0380018c)
void snd7_set_channel_timer(int chNo, int timer) {
    sHw.channel[chNo].tmr = (u16)(0x10000 - timer);
}

// SND_SetChannelPan (0x038001a4): a byte store to the pan, and to the volume when the surround decay changes it
void snd7_set_channel_pan(int chNo, int pan) {
    HwChannel *channel = &sHw.channel[chNo];

    sHw.orgPan[chNo] = (u8)pan;
    if (sHw.masterPan >= 0) {
        pan = sHw.masterPan;
    }
    WriteCnt(chNo, (channel->cnt & 0xff00ffffu) | ((u32)pan & 0xff) << 16);
    if (HasSurroundDecay(chNo)) {
        WriteCnt(chNo, (channel->cnt & 0xffffff00u) | ((u32)CalcSurroundDecay(sHw.orgVolume[chNo], pan) & 0xff));
    }
}

// SND_IsChannelActive (0x0380021c)
BOOL snd7_is_channel_active(int chNo) {
    return (sHw.channel[chNo].cnt & SND7_CNT_START) != 0;
}

// The start bit, as SND_StartTimer and SND_UpdateExChannel set it with a byte store
void snd7_start_channel(int chNo) {
    WriteCnt(chNo, sHw.channel[chNo].cnt | SND7_CNT_START);
}

// SND_SetMasterPan (0x03800238): every channel at that pan, or back at its own for a negative one
void snd7_set_master_pan(int pan) {
    int i;

    sHw.masterPan = pan;
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        u32 value = pan >= 0 ? (u32)pan & 0xff : sHw.orgPan[i];

        WriteCnt(i, (sHw.channel[i].cnt & 0xff00ffffu) | value << 16);
    }
}

// SND_GetChannelControl (0x0380029c), for SND_ReadDriverInfo
u32 snd7_get_channel_control(int chNo) {
    return sHw.channel[chNo].cnt;
}

// SNDi_SetSurroundDecay (0x038002ac)
void snd7_set_surround_decay(int decay) {
    int i;

    sHw.surroundDecay = decay;
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        if (SURROUND_DECAY_CHANNELS & (1 << i)) {
            HwChannel *channel = &sHw.channel[i];
            int volume = CalcSurroundDecay(sHw.orgVolume[i], (channel->cnt >> 16) & 0xff);

            WriteCnt(i, (channel->cnt & 0xffffff00u) | ((u32)volume & 0xff));
        }
    }
}

// snd_capture.c

// SND_SetupCapture (0x038007b4): SNDCAPxCNT (adding to the channel, the source, one-shot or repeat, the format), the
// destination and the length
void snd7_setup_capture(int capture, int format, u32 dest, int length, BOOL repeat, int addSrc, int outSrc) {
    sHw.captureCnt[capture] = (u8)(outSrc | addSrc << 1 | (repeat ? 0 : 1) << 2 | format << 3);
    sHw.captureDest[capture] = dest;
    sHw.captureLength[capture] = (u16)length;
}

// SND_IsCaptureActive (0x03800804)
BOOL snd7_is_capture_active(int capture) {
    return (sHw.captureCnt[capture] & CAPTURE_START) != 0;
}

// SND_StartTimer's and SND_StopTimer's capture units: both start with one halfword store when both are asked
void snd7_start_capture(u32 capBitMask) {
    if (capBitMask & 1) {
        sHw.captureCnt[0] |= CAPTURE_START;
    }
    if (capBitMask & 2) {
        sHw.captureCnt[1] |= CAPTURE_START;
    }
}

void snd7_stop_capture(u32 capBitMask) {
    if (capBitMask & 1) {
        sHw.captureCnt[0] = 0;
    }
    if (capBitMask & 2) {
        sHw.captureCnt[1] = 0;
    }
}
