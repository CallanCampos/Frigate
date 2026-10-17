// Tests of the host ARM7's sound driver and sound unit (port/hal/snd7) and of the host's audio output (port/audio).
//
//   port_snd_tests NAME
//
// One scenario a process, each run as the game's main() would be, on the OS core (port_os_run), since the ARM9's
// sound libraries start once a process. Most scenarios drive the ARM7's driver directly, on a clock of their own
// (port_snd7_boot_at, port_snd7_advance): synthetic banks, wave archives and sequences, with the channels' state
// checked update by update against values worked out by hand from the driver's arithmetic (port/hal/snd7) and
// GBATEK's sound unit. The pxi_ scenarios go through the ARM9's SND library and the PXI bus. The rom_ scenarios play
// sequences of the player's ROM (POKEBW2_ROM, else the repository's orig/baserom_<version>.nds) and compare hashes of
// the sound; they skip (77) without it. PORT_SND_VERBOSE prints each scenario's log; PORT_SND_WAV=DIR writes the rom_
// scenarios' sound as WAV files there.
#include "audio/audio.h"
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/rom/port_rom.h"
#include "hal/snd7/snd7_internal.h"
#include "hal/snd7/snd7_output.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/snd.h"
#include <execinfo.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SKIP_STATUS 77

#define LOG_MAX 512
#define LOG_LINE 120

// The driver's clock: its first periodic update after 0x10000 OS ticks, then every 2728
#define FIRST_UPDATE ((u64)SND7_FIRST_UPDATE_TICKS * SND7_OS_TICK_CYCLES)
#define UPDATE_CYCLES ((u64)SND7_UPDATE_TICKS * SND7_OS_TICK_CYCLES)

// The scenarios' main memory arena, as the pxi tests have it
#define TEST_ARENA_SIZE (16 * 1024 * 1024)
#define TEST_ARENA_ALIGNMENT 32

static char sLog[LOG_MAX][LOG_LINE];
static int sLogCount;
static int sFailed;
static int sSkipped;

static void Log(const char *format, ...) {
    va_list args;

    if (sLogCount == LOG_MAX) {
        return;
    }
    va_start(args, format);
    vsnprintf(sLog[sLogCount++], LOG_LINE, format, args);
    va_end(args);
}

static void CheckAt(int ok, const char *what, int line) {
    if (!ok) {
        printf("  check failed (line %d): %s\n", line, what);
        sFailed = 1;
    }
}

#define CHECK(condition) CheckAt((condition) != 0, #condition, __LINE__)

static void CheckEqualAt(long long got, long long expected, const char *what, int line) {
    if (got != expected) {
        printf("  check failed (line %d): %s is %lld, not %lld\n", line, what, got, expected);
        sFailed = 1;
    }
}

#define CHECK_EQ(value, expected) CheckEqualAt((long long)(value), (long long)(expected), #value, __LINE__)

// The OS, as the game's main() starts it (the pxi tests' Boot): the FIFO first, where the ARM7 boots, then the arenas,
// the interrupts, the threads, the tick and alarms, and the VBlank interrupt that WaitFrames waits for

static BOOL sOsBooted;

static void VBlankHandler(void) {
    port_os_set_irq_check_flag(OS_IE_V_BLANK);
}

static void PlaceArena(void) {
    static u8 *sBlock;
    u8 *start;

    if (sBlock == NULL) {
        sBlock = port_pxi_alloc_arena(TEST_ARENA_SIZE + TEST_ARENA_ALIGNMENT);
        if (sBlock == NULL) {
            printf("no memory for the arena\n");
            exit(2);
        }
    }
    start = (u8 *)(((uintptr_t)sBlock + TEST_ARENA_ALIGNMENT - 1) & ~(uintptr_t)(TEST_ARENA_ALIGNMENT - 1));
    OS_SetArenaLo(OS_ARENA_MAIN, start);
    OS_SetArenaHi(OS_ARENA_MAIN, start + TEST_ARENA_SIZE);
}

static void BootOs(void) {
    if (sOsBooted) {
        return;
    }
    sOsBooted = TRUE;
    PXI_Init();
    OS_InitArena();
    PlaceArena();
    OS_InitIrqTable();
    OS_InitThread();
    OS_InitTick();
    OS_InitAlarm();
    OS_SetIrqFunction(OS_IE_V_BLANK, VBlankHandler);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    (void)OS_EnableIrq();
    GX_VBlankIntr(TRUE);
}

static void WaitFrames(int count) {
    int i;

    for (i = 0; i < count; i++) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
}

// The sound the driver makes: its frames' FNV-1a hash, count, energy and peak, and the frames themselves when asked

typedef struct Sound {
    u64 hash;
    u64 frames;
    double energy;
    int peak;
    BOOL keep;
    s16 *kept;
    u64 keptFrames;
    u64 keptCapacity;
} Sound;

static Sound sSound;

static void ResetSound(BOOL keep) {
    free(sSound.kept);
    memset(&sSound, 0, sizeof(sSound));
    sSound.hash = 0xcbf29ce484222325ull;
    sSound.keep = keep;
}

static void Capture(const int16_t *frames, uint32_t count, void *user) {
    u32 i;

    for (i = 0; i < count * 2; i++) {
        u16 sample = (u16)frames[i];
        int magnitude = frames[i] < 0 ? -frames[i] : frames[i];

        sSound.hash = (sSound.hash ^ (sample & 0xff)) * 0x100000001b3ull;
        sSound.hash = (sSound.hash ^ (sample >> 8)) * 0x100000001b3ull;
        sSound.energy += (double)frames[i] * frames[i];
        if (magnitude > sSound.peak) {
            sSound.peak = magnitude;
        }
    }
    if (sSound.keep) {
        if (sSound.keptFrames + count > sSound.keptCapacity) {
            sSound.keptCapacity = (sSound.keptFrames + count) * 2;
            sSound.kept = realloc(sSound.kept, sSound.keptCapacity * 2 * sizeof(s16));
            if (sSound.kept == NULL) {
                printf("no memory for the sound\n");
                exit(2);
            }
        }
        memcpy(&sSound.kept[sSound.keptFrames * 2], frames, count * 2 * sizeof(s16));
        sSound.keptFrames += count;
    }
    sSound.frames += count;
}

// The driver on the scenario's own clock: started at 0, and the periodic updates one by one (update n, from 0, at
// FIRST_UPDATE + n * UPDATE_CYCLES)
static u32 sUpdates;
static SNDSharedWork sShared;

static void BootDriver(void) {
    int i;
    int j;

    BootOs();
    port_snd7_set_output(Capture, NULL);
    port_snd7_boot_at(0);
    ResetSound(FALSE);
    sUpdates = 0;
    memset(&sShared, 0, sizeof(sShared));
    for (i = 0; i < SND_PLAYER_MAX; i++) {
        for (j = 0; j < SND_PLAYER_VARIABLE_MAX; j++) {
            sShared.player[i].variable[j] = SND_VARIABLE_UNSET;
        }
    }
    for (i = 0; i < SND_PLAYER_VARIABLE_MAX; i++) {
        sShared.globalVariable[i] = SND_VARIABLE_UNSET;
    }
    snd7_shared = &sShared;
}

static void RunUpdates(u32 count) {
    while (count-- != 0) {
        port_snd7_advance(FIRST_UPDATE + sUpdates * UPDATE_CYCLES);
        sUpdates++;
    }
}

// The cycle just before the next update: the samples up to it
static void RunToNextUpdate(void) {
    port_snd7_advance(FIRST_UPDATE + sUpdates * UPDATE_CYCLES - 1);
}

// Little-endian fields of the synthetic files
static void Put16(u8 *at, u32 value) {
    at[0] = (u8)value;
    at[1] = (u8)(value >> 8);
}

static void Put32(u8 *at, u32 value) {
    Put16(at, value & 0xffff);
    Put16(at + 2, value >> 16);
}

// A synthetic wave archive (SWAR): wave 0 is PCM8, 4 words, looping from word 1; wave 1 IMA-ADPCM, its header and 3
// words, one-shot. 4 words are the fewest a sound plays with. Both play a sample per mixer sample (a timer of 512)
#define WAVE_TIMER 512
static const s8 sPcm8Samples[16] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 125, 115, 105, 95 };
// The ADPCM nibbles, low first: 7, b, 0, 2, f, 4, 9, 1. From a value of 0 at index 0 (step 7) they decode (GBATEK's
// decoder, step / 8 plus step / 4, / 2 and / 1 for the bits 0 to 2, bit 3 the sign, the index moving by -1, -1, -1,
// -1, 2, 4, 6, 8) to:
//   7: 0 + 1 + 3 + 7 = 11, index 8 (step 16)     b: -(2 + 4 + 8) = -3, index 7 (14)   0: +1 = -2, index 6 (13)
//   2: +(1 + 6) = 5, index 5 (12)                f: -(1 + 3 + 6 + 12) = -17, 13 (25)  4: +(3 + 25) = 11, 15 (31)
//   9: -(3 + 7) = 1, index 14 (28)                1: +(3 + 7) = 11, index 13
static const u8 sAdpcmData[12] = { 0xb7, 0x20, 0x4f, 0x19 };
static const s32 sAdpcmExpected[8] = { 11, -3, -2, 5, -17, 11, 1, 11 };
static u8 sWaveArc[0x80];

static void PutWaveParam(u8 *at, int format, int loop, int timer, int loopStart, u32 loopLen) {
    at[0] = (u8)format;
    at[1] = (u8)loop;
    Put16(at + 2, 32728);
    Put16(at + 4, (u32)timer);
    Put16(at + 6, (u32)loopStart);
    Put32(at + 8, loopLen);
}

static void BuildWaveArc(void) {
    memset(sWaveArc, 0, sizeof(sWaveArc));
    memcpy(sWaveArc, "SWAR", 4);
    Put16(sWaveArc + 4, 0xfeff);
    Put16(sWaveArc + 6, 0x100);
    Put32(sWaveArc + 8, 0x7c);
    Put16(sWaveArc + 12, 0x10);
    Put16(sWaveArc + 14, 1);
    memcpy(sWaveArc + 0x10, "DATA", 4);
    Put32(sWaveArc + 0x14, 0x6c);
    Put32(sWaveArc + 0x38, 2);
    Put32(sWaveArc + 0x3c, 0x44);
    Put32(sWaveArc + 0x40, 0x60);
    PutWaveParam(sWaveArc + 0x44, SND7_FORMAT_PCM8, 1, WAVE_TIMER, 1, 3);
    memcpy(sWaveArc + 0x50, sPcm8Samples, sizeof(sPcm8Samples));
    PutWaveParam(sWaveArc + 0x60, SND7_FORMAT_ADPCM, 0, WAVE_TIMER, 1, 3);
    Put32(sWaveArc + 0x6c, 0);
    memcpy(sWaveArc + 0x70, sAdpcmData, sizeof(sAdpcmData));
}

// A synthetic bank (SBNK) of six programs, its wave archive in slot 0:
//   0 PCM (wave 0), key 60, attack 127, decay 127, sustain 127, release 127, pan 64
//   1 PSG duty 3, key 60       2 noise, key 60
//   3 a drum set of keys 36 to 38: PCM wave 0 panned left, PSG duty 1 panned right, ADPCM wave 1 at key 72
//   4 a key split: up to key 59 PSG duty 7, up to 127 PCM wave 0
//   5 PCM (wave 0) with attack 100, decay 100, sustain 64, release 100
static u8 sBank[0xd0];

static void PutInstParam(u8 *at, u32 wave0, u32 wave1, int key, int attack, int decay, int sustain, int release,
                         int pan) {
    Put16(at, wave0);
    Put16(at + 2, wave1);
    at[4] = (u8)key;
    at[5] = (u8)attack;
    at[6] = (u8)decay;
    at[7] = (u8)sustain;
    at[8] = (u8)release;
    at[9] = (u8)pan;
}

static void PutInstData(u8 *at, int type, u32 wave0, u32 wave1, int key, int pan) {
    at[0] = (u8)type;
    at[1] = 0;
    PutInstParam(at + 2, wave0, wave1, key, 127, 127, 127, 127, pan);
}

static void BuildBank(void) {
    memset(sBank, 0, sizeof(sBank));
    memcpy(sBank, "SBNK", 4);
    Put16(sBank + 4, 0xfeff);
    Put16(sBank + 6, 0x100);
    Put32(sBank + 8, 0xc4);
    Put16(sBank + 12, 0x10);
    Put16(sBank + 14, 1);
    memcpy(sBank + 0x10, "DATA", 4);
    Put32(sBank + 0x14, 0xb4);
    Put32(sBank + 0x38, 6);
    Put32(sBank + 0x3c, SND_INST_PCM | 0x54 << 8);
    PutInstParam(sBank + 0x54, 0, 0, 60, 127, 127, 127, 127, 64);
    Put32(sBank + 0x40, SND_INST_PSG | 0x60 << 8);
    PutInstParam(sBank + 0x60, 3, 0, 60, 127, 127, 127, 127, 64);
    Put32(sBank + 0x44, SND_INST_NOISE | 0x6a << 8);
    PutInstParam(sBank + 0x6a, 0, 0, 60, 127, 127, 127, 127, 64);
    Put32(sBank + 0x48, SND_INST_DRUM_SET | 0x74 << 8);
    sBank[0x74] = 36;
    sBank[0x75] = 38;
    PutInstData(sBank + 0x76, SND_INST_PCM, 0, 0, 60, 0);
    PutInstData(sBank + 0x82, SND_INST_PSG, 1, 0, 60, 127);
    PutInstData(sBank + 0x8e, SND_INST_PCM, 1, 0, 72, 64);
    Put32(sBank + 0x4c, SND_INST_KEY_SPLIT | 0x9a << 8);
    sBank[0x9a] = 59;
    sBank[0x9b] = 127;
    PutInstData(sBank + 0xa2, SND_INST_PSG, 7, 0, 60, 64);
    PutInstData(sBank + 0xae, SND_INST_PCM, 0, 0, 60, 64);
    Put32(sBank + 0x50, SND_INST_PCM | 0xba << 8);
    PutInstParam(sBank + 0xba, 0, 0, 60, 100, 100, 64, 100, 64);
}

static void BuildData(void) {
    BuildWaveArc();
    BuildBank();
    SND_AssignWaveArc((SNDBankData *)sBank, 0, (SNDWaveArc *)sWaveArc);
}

static void PlaySequence(int playerNo, const u8 *sequence, u32 size) {
    snd7_prepare_seq(playerNo, sequence, sequence + size, 0, sBank, sBank + sizeof(sBank));
    snd7_start_prepared_seq(playerNo);
}

static const Snd7Track *Track(int playerNo, int trackIdx) {
    u8 trackNo = snd7_work.player[playerNo].tracks[trackIdx];

    return trackNo == SND7_TRACK_NONE ? NULL : &snd7_work.track[trackNo];
}

static const Snd7ExChannel *TrackChannel(int playerNo, int trackIdx) {
    const Snd7Track *track = Track(playerNo, trackIdx);

    return track != NULL ? track->channelList : NULL;
}

// The sound unit

static void TestAdpcmDecoder(void) {
    s32 value = 0;
    s32 index = 0;
    int i;

    for (i = 0; i < 8; i++) {
        u8 byte = sAdpcmData[i / 2];

        snd7_adpcm_step(i & 1 ? byte >> 4 : byte & 0xf, &value, &index);
        CHECK_EQ(value, sAdpcmExpected[i]);
    }
    CHECK_EQ(index, 13);
    // The value stops at +0x7fff and -0x7fff, the index at 88 and 0
    value = 0x7ff0;
    index = 88;
    snd7_adpcm_step(7, &value, &index);
    CHECK_EQ(value, 0x7fff);
    CHECK_EQ(index, 88);
    // 0x7fff - (0xfff + 0x1fff + 0x3fff + 0x7fff), then lower still
    snd7_adpcm_step(0xf, &value, &index);
    CHECK_EQ(value, -28669);
    snd7_adpcm_step(0xf, &value, &index);
    CHECK_EQ(value, -0x7fff);
    index = 0;
    snd7_adpcm_step(0, &value, &index);
    CHECK_EQ(index, 0);
}

// A sound on a channel, as the driver's register functions set it, its samples one by one as the mixer plays them
static void TestAdpcmChannel(void) {
    Snd7HwPeek peek;
    s16 out[2];
    int i;

    BootDriver();
    BuildWaveArc();
    snd7_setup_channel_pcm(2, sWaveArc + 0x6c, sWaveArc + sizeof(sWaveArc), SND7_FORMAT_ADPCM, SND7_REPEAT_ONESHOT, 1,
                           3, 127, 0, WAVE_TIMER, 64);
    snd7_start_channel(2);
    for (i = 0; i < 24; i++) {
        snd7_hw_peek(2, &peek);
        if (i < 8) {
            CHECK_EQ(peek.sample, sAdpcmExpected[i]);
        }
        CHECK(peek.cnt & SND7_CNT_START);
        snd7_hw_mix(out);
    }
    // 24 samples (three words after the header), then a one-shot sound ends, and outputs 0 without hold
    snd7_hw_peek(2, &peek);
    CHECK(!(peek.cnt & SND7_CNT_START));
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 0);
    CHECK_EQ(out[1], 0);
}

// PCM8 with a loop: 16 samples, then from sample 4 (word 1) again
static void TestPcm8Loop(void) {
    static const int sExpected[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 125, 115, 105, 95, 50, 60, 70 };
    Snd7HwPeek peek;
    s16 out[2];
    size_t i;

    BootDriver();
    BuildWaveArc();
    snd7_setup_channel_pcm(0, sWaveArc + 0x50, NULL, SND7_FORMAT_PCM8, SND7_REPEAT_LOOP, 1, 3, 127, 0, WAVE_TIMER, 0);
    snd7_start_channel(0);
    for (i = 0; i < NELEMS(sExpected); i++) {
        snd7_hw_peek(0, &peek);
        CHECK_EQ(peek.sample, sExpected[i] * 256);
        snd7_hw_mix(out);
    }
    // At half the rate (a timer of 1024) each sample plays twice
    snd7_setup_channel_pcm(0, sWaveArc + 0x50, NULL, SND7_FORMAT_PCM8, SND7_REPEAT_LOOP, 1, 3, 127, 0, 1024, 0);
    snd7_start_channel(0);
    for (i = 0; i < 6; i++) {
        snd7_hw_peek(0, &peek);
        CHECK_EQ(peek.sample, sExpected[i / 2] * 256);
        snd7_hw_mix(out);
    }
}

// A PCM or IMA-ADPCM sound of fewer than 4 words (PNT + LEN) hangs (GBATEK, SOUNDxLEN): the channel stays busy, at its
// first sample, and outputs nothing, until the driver stops it. Then the same channel plays a sound of 4 words
static void TestShortSound(void) {
    static const u8 sPcm16[4] = { 0x00, 0x40, 0x00, 0x40 };
    Snd7HwPeek peek;
    s16 out[2];
    int i;

    BootDriver();
    BuildWaveArc();
    // PCM8 of 3 words, PCM16 of 1, IMA-ADPCM of 3 (its header and 2), PCM8 of none
    snd7_setup_channel_pcm(0, sWaveArc + 0x50, NULL, SND7_FORMAT_PCM8, SND7_REPEAT_ONESHOT, 1, 2, 127, 0, WAVE_TIMER,
                           64);
    snd7_setup_channel_pcm(1, sPcm16, NULL, SND7_FORMAT_PCM16, SND7_REPEAT_LOOP, 0, 1, 127, 0, WAVE_TIMER, 64);
    snd7_setup_channel_pcm(2, sWaveArc + 0x6c, NULL, SND7_FORMAT_ADPCM, SND7_REPEAT_ONESHOT, 1, 2, 127, 0, WAVE_TIMER,
                           64);
    snd7_setup_channel_pcm(3, sWaveArc + 0x50, NULL, SND7_FORMAT_PCM8, SND7_REPEAT_ONESHOT, 0, 0, 127, 0, WAVE_TIMER,
                           64);
    for (i = 0; i < 4; i++) {
        snd7_start_channel(i);
    }
    for (i = 0; i < 64 * 4; i++) {
        int chNo = i % 4;

        snd7_hw_peek(chNo, &peek);
        CHECK(peek.cnt & SND7_CNT_START);
        CHECK_EQ(peek.index, 0);
        CHECK_EQ(peek.sample, 0);
        if (chNo == 3) {
            snd7_hw_mix(out);
            CHECK_EQ(out[0], 0);
            CHECK_EQ(out[1], 0);
        }
    }
    CHECK(snd7_is_channel_active(0));
    // The driver's stop ends the hang; held, the channel still outputs nothing
    snd7_stop_channel(0, SND7_CHANNEL_STOP_HOLD);
    CHECK(!snd7_is_channel_active(0));
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 0);
    // 4 words play
    snd7_setup_channel_pcm(0, sWaveArc + 0x50, NULL, SND7_FORMAT_PCM8, SND7_REPEAT_ONESHOT, 1, 3, 127, 0, WAVE_TIMER,
                           64);
    snd7_start_channel(0);
    for (i = 0; i < 16; i++) {
        snd7_hw_peek(0, &peek);
        CHECK(peek.cnt & SND7_CNT_START);
        CHECK_EQ(peek.sample, sPcm8Samples[i] * 256);
        snd7_hw_mix(out);
    }
    snd7_hw_peek(0, &peek);
    CHECK(!(peek.cnt & SND7_CNT_START));
}

// A PSG channel at duty 3: 4 low steps then 4 high; the output at volume 127, center pan, master volume 127: the
// sample (0x7fff high, -0x7fff low) times 127, times 64 of 128 to each side, times 127 of 128: 16128.5, rounded down
static void TestPsgDuty(void) {
    static const int sHigh[16] = { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1 };
    Snd7HwPeek peek;
    s16 out[2];
    int i;

    BootDriver();
    snd7_setup_channel_psg(8, 3, 127, 0, WAVE_TIMER, 64);
    snd7_start_channel(8);
    for (i = 0; i < 16; i++) {
        snd7_hw_peek(8, &peek);
        CHECK_EQ(peek.sample, sHigh[i] ? 0x7fff : -0x7fff);
        snd7_hw_mix(out);
        CHECK_EQ(out[0], sHigh[i] ? 16128 : -16129);
        CHECK_EQ(out[1], out[0]);
    }
    // Duty 7 (0%) is low throughout (GBATEK, "PSG Sound"); duty 0 high one step in 8
    snd7_setup_channel_psg(9, 7, 127, 0, WAVE_TIMER, 64);
    snd7_setup_channel_psg(10, 0, 127, 0, WAVE_TIMER, 64);
    snd7_start_channel(9);
    snd7_start_channel(10);
    for (i = 0; i < 8; i++) {
        snd7_hw_peek(9, &peek);
        CHECK_EQ(peek.sample, -0x7fff);
        snd7_hw_peek(10, &peek);
        CHECK_EQ(peek.sample, i == 7 ? 0x7fff : -0x7fff);
        snd7_hw_mix(out);
    }
    // A square wave on a channel that has none (0-7) is silent
    snd7_setup_channel_psg(3, 3, 127, 0, WAVE_TIMER, 64);
    snd7_start_channel(3);
    snd7_hw_peek(3, &peek);
    CHECK_EQ(peek.sample, 0);
}

// The noise channel's 15-bit register from 0x7fff: 14 ones drop out (low, with 0x6000 in), then 14 zeros (high), then a
// one; and the sequence repeats every 32767 steps
static void TestNoise(void) {
    static s32 sSamples[32767 + 64];
    Snd7HwPeek peek;
    s16 out[2];
    int i;

    BootDriver();
    snd7_setup_channel_noise(14, 127, 0, WAVE_TIMER, 64);
    snd7_start_channel(14);
    for (i = 0; i < (int)NELEMS(sSamples); i++) {
        snd7_hw_peek(14, &peek);
        sSamples[i] = peek.sample;
        snd7_hw_mix(out);
    }
    for (i = 0; i < 29; i++) {
        CHECK_EQ(sSamples[i], i >= 14 && i < 28 ? 0x7fff : -0x7fff);
    }
    for (i = 0; i < 64; i++) {
        CHECK_EQ(sSamples[32767 + i], sSamples[i]);
    }
    // Only channels 14 and 15 make noise
    snd7_setup_channel_noise(13, 127, 0, WAVE_TIMER, 64);
    snd7_start_channel(13);
    snd7_hw_peek(13, &peek);
    CHECK_EQ(peek.sample, -0x7fff);
}

// The mixer's volume, data shift, pan, master volume and output selector, on a PCM16 sample of 0x4000
static void TestMixer(void) {
    static const u8 sPcm16[16] = { 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40,
                                   0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40 };
    s16 out[2];

    BootDriver();
    snd7_setup_channel_pcm(0, sPcm16, NULL, SND7_FORMAT_PCM16, SND7_REPEAT_LOOP, 0, 4, 64, 1, WAVE_TIMER, 0);
    snd7_start_channel(0);
    // 0x4000 * 64 >> 1 = 0x80000; left 128 of 128, right 0; times 127 of 128 at 21 bits of fraction: 4064 and 0
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 4064);
    CHECK_EQ(out[1], 0);
    snd7_set_channel_pan(0, 127);
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 31);
    CHECK_EQ(out[1], 4032);
    // A data shift of 3 divides by 16; the master volume 64 halves
    snd7_set_channel_volume(0, 64, 3);
    snd7_set_master_volume(64);
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 0x4000 * 64 / 16 * 1 * 64 >> 21);
    CHECK_EQ(out[1], 0x4000 * 64 / 16 * 127 * 64 >> 21);
    // The master pan puts every channel at its value, and gives each its own back at -1
    snd7_set_master_volume(127);
    snd7_set_channel_volume(0, 64, 1);
    snd7_set_master_pan(0);
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 4064);
    snd7_set_master_pan(-1);
    snd7_hw_mix(out);
    CHECK_EQ(out[1], 4032);
    // The left output from channel 1 alone, the right from channel 3 alone, and channel 1 out of the mixer
    snd7_setup_channel_pcm(1, sPcm16, NULL, SND7_FORMAT_PCM16, SND7_REPEAT_LOOP, 0, 4, 127, 0, WAVE_TIMER, 0);
    snd7_start_channel(1);
    snd7_set_output_selector(1, 2, 1, 0);
    snd7_hw_mix(out);
    CHECK_EQ(out[0], (s64)0x4000 * 127 * 128 * 127 >> 21);
    CHECK_EQ(out[1], 0);
    snd7_set_output_selector(0, 0, 1, 0);
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 31);
    snd7_disable();
    snd7_hw_mix(out);
    CHECK_EQ(out[0], 0);
}

// SND_CalcTimer: a pitch is 64ths of a semitone; an octave doubles or halves the timer. The results are the BIOS
// table's (2^(i / 768) - 1) * 0x10000 worked out apart
static void TestCalcTimer(void) {
    CHECK_EQ(snd7_calc_timer(8006, 0), 8006);
    CHECK_EQ(snd7_calc_timer(8006, 768), 4003);
    CHECK_EQ(snd7_calc_timer(8006, -768), 16012);
    CHECK_EQ(snd7_calc_timer(8006, 64), 7556);
    CHECK_EQ(snd7_calc_timer(0x2000, 64), 7732);
    CHECK_EQ(snd7_calc_timer(512, 512), 322);
    CHECK_EQ(snd7_calc_timer(512, -1), 512);
    CHECK_EQ(snd7_calc_timer(8006, -100), 8762);
    // Clamped to 0x10 and 0xffff, and 0xffff when it would pass 32 bits
    CHECK_EQ(snd7_calc_timer(0x10, 768 * 3), 0x10);
    CHECK_EQ(snd7_calc_timer(0xffff, -768), 0xffff);
    CHECK_EQ(snd7_calc_timer(0x1000, -768 * 17), 0xffff);
    CHECK_EQ(snd7_pitch_table[1], 59);
    CHECK_EQ(snd7_pitch_table[704], 58179);
    CHECK_EQ(snd7_pitch_table[767], 65418);
    // SND_SinIdx's quarter turns, and SND_CalcRandom's generator from its seed
    CHECK_EQ(snd7_sin_idx(0), 0);
    CHECK_EQ(snd7_sin_idx(32), 127);
    CHECK_EQ(snd7_sin_idx(48), 90);
    CHECK_EQ(snd7_sin_idx(64), 0);
    CHECK_EQ(snd7_sin_idx(96), -127);
    CHECK_EQ(snd7_sin_idx(112), -90);
    snd7_set_random(0x12345678);
    CHECK_EQ(snd7_calc_random(), (u16)((0x12345678u * 1664525u + 1013904223u) >> 16));
}

// The channels

static int sDrops[SND7_CHANNEL_NUM];

static void DropCallback(Snd7ExChannel *channel, int status, void *data) {
    if (status == SND7_EXCHANNEL_CALLBACK_DROP) {
        sDrops[channel->myNo]++;
    }
}

// SND_AllocExChannel's order (4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13), priority and loudness, and the
// locks
static void TestAlloc(void) {
    static const int sOrder[16] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
    Snd7ExChannel *channel;
    int i;

    BootDriver();
    memset(sDrops, 0, sizeof(sDrops));
    for (i = 0; i < 16; i++) {
        channel = snd7_alloc_exchannel(0xffff, 64, FALSE, DropCallback, NULL);
        CHECK(channel != NULL && channel->myNo == sOrder[i]);
    }
    // All at priority 64: a request below it fails; at it, the first in the order of the quietest goes, and is told
    CHECK(snd7_alloc_exchannel(0xffff, 63, FALSE, DropCallback, NULL) == NULL);
    snd7_work.channel[0].volume = 0x0140; // volume 0x40 shifted right by 1: quieter than 127 unshifted
    channel = snd7_alloc_exchannel(0xffff, 64, FALSE, DropCallback, NULL);
    CHECK(channel != NULL && channel->myNo == 0);
    CHECK_EQ(sDrops[0], 1);
    CHECK_EQ(snd7_sync(channel), SND7_SYNC_STOP);
    CHECK_EQ(channel->volume, 127);
    channel = snd7_alloc_exchannel(0xffff, 64, FALSE, DropCallback, NULL);
    CHECK(channel != NULL && channel->myNo == 4);
    // A lower priority goes first
    snd7_work.channel[11].prio = 10;
    channel = snd7_alloc_exchannel(0xffff, 64, FALSE, DropCallback, NULL);
    CHECK(channel != NULL && channel->myNo == 11);
    // The PSG and noise channels' masks
    for (i = 0; i < 16; i++) {
        snd7_work.channel[i].prio = 0;
    }
    channel = snd7_alloc_exchannel(0x3f00, 1, FALSE, NULL, NULL);
    CHECK(channel != NULL && channel->myNo == 8);
    channel = snd7_alloc_exchannel(0xc000, 1, FALSE, NULL, NULL);
    CHECK(channel != NULL && channel->myNo == 14);
    // A locked channel is never given; a weakly locked one only to a strong request
    snd7_lock_channel(1u << 4, 0);
    snd7_lock_channel(1u << 5, 1);
    CHECK_EQ(snd7_get_locked_channel(0), 1u << 4);
    CHECK_EQ(snd7_get_locked_channel(1), 1u << 5);
    CHECK(snd7_alloc_exchannel(1u << 4, 127, TRUE, NULL, NULL) == NULL);
    CHECK(snd7_alloc_exchannel(1u << 5, 127, FALSE, NULL, NULL) == NULL);
    channel = snd7_alloc_exchannel(1u << 5, 127, TRUE, NULL, NULL);
    CHECK(channel != NULL && channel->myNo == 5);
    snd7_unlock_channel(1u << 4, 0);
    channel = snd7_alloc_exchannel(1u << 4, 127, FALSE, NULL, NULL);
    CHECK(channel != NULL && channel->myNo == 4);
}

// Locking a channel stops what plays on it and tells its owner; a square wave only plays on channels 8 to 13
static void TestLockStops(void) {
    Snd7WaveParam wave = { SND7_FORMAT_PCM8, 1, 32728, WAVE_TIMER, 1, 3 };
    Snd7ExChannel *channel;

    BootDriver();
    BuildWaveArc();
    memset(sDrops, 0, sizeof(sDrops));
    channel = snd7_alloc_exchannel(1u << 6, 64, FALSE, DropCallback, NULL);
    CHECK(channel != NULL);
    CHECK(!snd7_start_exchannel_psg(channel, 3, -1));
    CHECK(snd7_start_exchannel_pcm(channel, &wave, sWaveArc + 0x50, NULL, -1));
    CHECK(snd7_is_exchannel_active(channel));
    snd7_lock_channel(1u << 6, 0);
    CHECK_EQ(sDrops[6], 1);
    CHECK(!snd7_is_exchannel_active(channel));
    CHECK_EQ(channel->prio, 0);
}

// Sequences: a note, update by update

// A note of key 60 at velocity 100 that lasts 48 ticks, then the end. At tempo 120 a tick comes every other update
// (the tempo counter starts at 240), so tick n is at update 2(n - 1)
static const u8 sSeqNote[] = { 0x3c, 100, 48, 0xff };

static void TestSeqNote(void) {
    const Snd7ExChannel *channel;
    Snd7HwPeek peek;
    u16 volume = SND_CalcChannelVolume(snd7_decibel_square_table[100]);
    int u;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqNote, sizeof(sSeqNote));
    CHECK_EQ(sShared.playerStatus, 1);
    CHECK(TrackChannel(0, 0) == NULL);

    // Update 0, tick 1: the note takes channel 4, the first in the allocation order, with the instrument's envelope;
    // the attack of 127 reaches 0 dB at once and the decay starts. The channel's stop (of what it played before),
    // start, volume, timer and pan wait for the next update's register writes
    RunUpdates(1);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL && channel->myNo == 4);
    CHECK_EQ(channel->key, 60);
    CHECK_EQ(channel->velocity, 100);
    CHECK_EQ(channel->originalKey, 60);
    CHECK_EQ(channel->length, 48);
    CHECK_EQ(channel->prio, 64 + 64);
    CHECK_EQ(channel->envStatus, SND7_ENV_DECAY);
    CHECK_EQ(channel->envDecay, 0);
    CHECK_EQ(channel->volume, volume);
    CHECK_EQ(channel->timer, WAVE_TIMER);
    CHECK_EQ(channel->pan, 64);
    CHECK_EQ(snd7_sync(channel), SND7_SYNC_STOP | SND7_SYNC_START | SND7_SYNC_VOLUME | SND7_SYNC_TIMER | SND7_SYNC_PAN);
    CHECK_EQ(Track(0, 0)->wait, 48);
    CHECK_EQ(sShared.player[0].tickCounter, 1);
    snd7_hw_peek(4, &peek);
    CHECK(!(peek.cnt & SND7_CNT_START));

    // Update 1: the sound unit's channel 4 starts, looping, at the channel's volume, pan and timer. The decay of 127
    // falls to the sustain of 127, 0 dB
    RunUpdates(1);
    snd7_hw_peek(4, &peek);
    CHECK_EQ(peek.cnt, SND7_CNT_START | (u32)SND7_REPEAT_LOOP << 27 | (u32)SND7_FORMAT_PCM8 << 29 | 64u << 16 |
                           (u32)(volume >> 8) << 8 | (volume & 0xff));
    CHECK_EQ(peek.tmr, 0x10000 - WAVE_TIMER);
    CHECK_EQ(peek.pnt, 1);
    CHECK_EQ(peek.len, 3);
    CHECK(peek.sad == sWaveArc + 0x50);
    CHECK_EQ(channel->envStatus, SND7_ENV_SUSTAIN);
    CHECK_EQ(snd7_sync(channel), 0);
    CHECK_EQ(sShared.channelStatus, 1u << 4);
    RunToNextUpdate();
    snd7_hw_peek(4, &peek);
    CHECK(peek.sample != 0);

    // Each tick shortens the note; the track waits out its 48 ticks
    RunUpdates(1);
    CHECK_EQ(channel->length, 47);
    CHECK_EQ(Track(0, 0)->wait, 47);
    for (u = 3; u < 96; u++) {
        RunUpdates(1);
    }
    CHECK_EQ(channel->length, 1);
    CHECK_EQ(sShared.player[0].tickCounter, 48);
    CHECK_EQ(sShared.playerStatus, 1);

    // Update 96, tick 49: the note's length runs out and the track reads its end; the player stops, releasing the
    // channel (release 127, a rate of 0xffff: -512 dB tenths at once), and isn't counted the tick it ended on
    RunUpdates(1);
    CHECK_EQ(sShared.playerStatus, 0);
    CHECK_EQ(sShared.player[0].tickCounter, 48);
    CHECK_EQ(snd7_work.player[0].flags & SND7_PLAYER_ACTIVE, 0);
    CHECK_EQ(channel->length, 0);
    CHECK_EQ(channel->envStatus, SND7_ENV_RELEASE);
    CHECK_EQ(channel->prio, 1);
    CHECK(channel->callback == NULL);
    CHECK_EQ(channel->envDecay, -65535);
    CHECK_EQ(channel->volume, SND_CalcChannelVolume(snd7_decibel_square_table[100] + (-65535 >> 7)));
    CHECK(snd7_is_exchannel_active(channel));

    // Update 97: below -72.3 dB the channel ends, its stop owed to the sound unit; update 98 stops it there
    RunUpdates(1);
    CHECK(!snd7_is_exchannel_active(channel));
    CHECK_EQ(snd7_sync(channel), SND7_SYNC_STOP);
    CHECK_EQ(channel->volume, 0);
    CHECK_EQ(channel->prio, 0);
    snd7_hw_peek(4, &peek);
    CHECK(peek.cnt & SND7_CNT_START);
    RunUpdates(1);
    snd7_hw_peek(4, &peek);
    CHECK(!(peek.cnt & SND7_CNT_START));
    CHECK_EQ(sShared.channelStatus, 0);
}

// The track's and player's parameters on a note: volume 64, expression 100, the player's volume 110, pan 32, transpose
// 2, a bend range of 12 and a bend of 64 (half the range up: 64 * 12 * 64 >> 7 = 384), then key 60 (played as 62)
static const u8 sSeqParams[] = {
    0xc1, 64, 0xd5, 100, 0xc2, 110, 0xc0, 32, 0xc3, 2, 0xc5, 12, 0xc4, 64, 0x3c, 127, 10, 0xff,
};

static void TestSeqParams(void) {
    const Snd7ExChannel *channel;
    s32 decibels = snd7_decibel_square_table[64] + snd7_decibel_square_table[100] + snd7_decibel_square_table[110];

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqParams, sizeof(sSeqParams));
    RunUpdates(1);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    CHECK_EQ(channel->key, 62);
    CHECK_EQ(channel->userDecay, decibels);
    CHECK_EQ(channel->userPitch, 384);
    CHECK_EQ(channel->userPan, -32);
    CHECK_EQ(channel->volume, SND_CalcChannelVolume(decibels));
    CHECK_EQ(channel->timer, snd7_calc_timer(WAVE_TIMER, 2 * 64 + 384));
    CHECK_EQ(channel->pan, 32);
    // The ARM9's track parameters (SNDi_SetTrackParam) and player volume (the fader, SNDi_SetPlayerParam) at the next
    // update: the track's pan 20 more, its fader -100, the player's -50
    snd7_set_track_param(0, 1, 9, 20, 1);
    snd7_set_track_param(0, 1, 0xa, (u32)-100, 2);
    snd7_set_player_param(0, 6, (u32)-50, 2);
    RunUpdates(1);
    CHECK_EQ(channel->userPan, -12);
    CHECK_EQ(channel->userDecay2, -150);
    CHECK_EQ(channel->volume, SND_CalcChannelVolume(decibels - 150));
    CHECK_EQ(channel->pan, 52);
}

// Programs 1 (PSG duty 3) and 2 (noise), the drum set (3) and the key split (4). Notes of 1 tick
static const u8 sSeqPrograms[] = {
    0xc7, 0,                                              // no note wait: the notes start together
    0x81, 1, 0x3c, 127, 1, 0x81, 2,   0x3c, 127,  1,      // PSG, noise
    0x81, 3, 0x24, 127, 1, 0x25, 127, 1,    0x27, 127, 1, // drums 36, 37 and 39 (none)
    0x81, 4, 0x3b, 127, 1, 0x3c, 127, 1,                  // the key split's 59 and 60
    0x80, 4, 0xff,
};

static void TestSeqPrograms(void) {
    Snd7HwPeek peek;
    const Snd7ExChannel *channel;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqPrograms, sizeof(sSeqPrograms));
    RunUpdates(2);
    // The PSG notes take 8, 9, 10 in turn; the noise 14; the PCM notes 4, 5
    snd7_hw_peek(8, &peek);
    CHECK_EQ(peek.cnt >> 29 & 3, SND7_FORMAT_PSG);
    CHECK_EQ(peek.cnt >> 24 & 7, 3);
    CHECK_EQ(peek.tmr, 0x10000 - (snd7_calc_timer(0x1f46, 0) & 0xfffc));
    snd7_hw_peek(14, &peek);
    CHECK_EQ(peek.cnt >> 29 & 3, SND7_FORMAT_PSG);
    CHECK(peek.cnt & SND7_CNT_START);
    // Drum 36: wave 0 panned hard left (the instrument's pan 0 is -64)
    channel = &snd7_work.channel[4];
    CHECK(snd7_is_exchannel_active(channel));
    CHECK_EQ(channel->initPan, -64);
    CHECK_EQ(channel->pan, 0);
    CHECK_EQ(channel->type, SND7_EXCHANNEL_PCM);
    // Drum 37: PSG duty 1 panned right, on channel 9
    snd7_hw_peek(9, &peek);
    CHECK_EQ(peek.cnt >> 24 & 7, 1);
    CHECK_EQ(peek.cnt >> 16 & 0x7f, 127);
    // Key split 59: PSG duty 7 on channel 10; 60: PCM on channel 5. Drum 39 had no instrument
    snd7_hw_peek(10, &peek);
    CHECK_EQ(peek.cnt >> 24 & 7, 7);
    CHECK(snd7_is_exchannel_active(&snd7_work.channel[5]));
    CHECK(!snd7_is_exchannel_active(&snd7_work.channel[6]));
    CHECK(!snd7_is_exchannel_active(&snd7_work.channel[11]));
}

// The envelope of program 5: attack 100 (a rate of 255 - 100), decay 100 (0x1e00 / (126 - 100) = 295 a update), sustain
// 64 (-119 dB tenths), release 100; a note of 60 ticks. Its level, in 128ths of a tenth of a decibel, from -92544
static const u8 sSeqEnvelope[] = { 0x81, 5, 0x3c, 127, 60, 0xff };

static void TestSeqEnvelope(void) {
    static const s32 sAttack[] = { -56032, -33925, -20540, -12436 };
    const Snd7ExChannel *channel;
    s32 level;
    int i;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqEnvelope, sizeof(sSeqEnvelope));
    RunUpdates(1);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    CHECK_EQ(channel->attack, 155);
    CHECK_EQ(channel->decay, 295);
    CHECK_EQ(channel->sustain, 64);
    CHECK_EQ(channel->release, 295);
    CHECK_EQ(channel->envDecay, sAttack[0]);
    for (i = 1; i < 4; i++) {
        RunUpdates(1);
        CHECK_EQ(channel->envDecay, sAttack[i]);
        CHECK_EQ(channel->envStatus, SND7_ENV_ATTACK);
    }
    // The attack ends at 0, where the decay starts
    for (i = 0; i < 64 && channel->envStatus == SND7_ENV_ATTACK; i++) {
        RunUpdates(1);
    }
    CHECK_EQ(channel->envStatus, SND7_ENV_DECAY);
    CHECK_EQ(channel->envDecay, 0);
    RunUpdates(1);
    CHECK_EQ(channel->envDecay, -295);
    for (i = 0; i < 64 && channel->envStatus == SND7_ENV_DECAY; i++) {
        RunUpdates(1);
    }
    CHECK_EQ(channel->envStatus, SND7_ENV_SUSTAIN);
    CHECK_EQ(channel->envDecay, -119 * 128);
    // Tick 61 (update 120) ends the note: it releases at 295 an update
    while (sUpdates < 121) {
        RunUpdates(1);
    }
    CHECK_EQ(channel->envStatus, SND7_ENV_RELEASE);
    level = channel->envDecay;
    RunUpdates(1);
    CHECK_EQ(channel->envDecay, level - 295);
}

// Overrides of the envelope by the track (0xd0-0xd3) and the tie: two notes on one channel
static const u8 sSeqTie[] = {
    0xd0, 120, 0xd1, 10,  0xd2, 100,  0xd3, 50, // attack 120 (the table's 63), decay 10 (21), sustain 100, release 50
    0xc8, 1,   0x3c, 100, 4,    0x40, 90,   4,  0xc8, 0, 0x80, 2, 0xff,
};

static void TestSeqTie(void) {
    const Snd7ExChannel *channel;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqTie, sizeof(sSeqTie));
    RunUpdates(1);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    CHECK_EQ(channel->attack, 63);
    CHECK_EQ(channel->decay, 21);
    CHECK_EQ(channel->sustain, 100);
    CHECK_EQ(channel->release, 0x1e00 / (126 - 50));
    // A tied note doesn't end by itself
    CHECK_EQ(channel->length, -1);
    // Tick 5 (update 8): the second note takes the same channel, with its key and velocity
    RunUpdates(8);
    CHECK(TrackChannel(0, 0) == channel);
    CHECK_EQ(channel->key, 64);
    CHECK_EQ(channel->velocity, 90);
    CHECK(channel->nextLink == NULL);
    // Tick 9 (update 16): tie off releases and frees the track's channels
    RunUpdates(8);
    CHECK(TrackChannel(0, 0) == NULL);
    CHECK_EQ(channel->envStatus, SND7_ENV_RELEASE);
    CHECK(channel->callback == NULL);
}

// Portamento and sweep: a sweep of -256 (four semitones down) over the note; then portamento from key 55 with a time
// of 4: a sweep of (55 - 60) * 64 = -320 over (4 * 4 * 320) >> 11 = 2 updates
static const u8 sSeqSweep[] = {
    0xe3, 0x00, 0xff,                // sweep pitch -256
    0x3c, 127,  8,    0xe3, 0,    0, // a note of 8 ticks, then no sweep
    0xc9, 55,   0xcf, 4,    0x3c, 127, 8, 0xff,
};

static void TestSeqSweep(void) {
    const Snd7ExChannel *channel;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqSweep, sizeof(sSeqSweep));
    RunUpdates(1);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    // Without portamento time the sweep lasts the note, counted by ticks: -256 * (8 - 0) / 8
    CHECK_EQ(channel->sweepPitch, -256);
    CHECK_EQ(channel->sweepLength, 8);
    CHECK_EQ(channel->flags & SND7_EXCHANNEL_AUTO_SWEEP, 0);
    CHECK_EQ(channel->timer, snd7_calc_timer(WAVE_TIMER, -256));
    RunUpdates(2);
    // A tick on (sweep counter 1): -256 * 7 / 8 = -224
    CHECK_EQ(channel->sweepCounter, 1);
    CHECK_EQ(channel->timer, snd7_calc_timer(WAVE_TIMER, -224));
    // The second note, at tick 9 (update 16): portamento from 55, automatic over 2 updates
    while (sUpdates < 17) {
        RunUpdates(1);
    }
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    CHECK_EQ(channel->sweepPitch, -320);
    CHECK_EQ(channel->sweepLength, 2);
    CHECK(channel->flags & SND7_EXCHANNEL_AUTO_SWEEP);
    CHECK_EQ(channel->sweepCounter, 1);
    CHECK_EQ(channel->timer, snd7_calc_timer(WAVE_TIMER, -320));
    RunUpdates(1);
    CHECK_EQ(channel->timer, snd7_calc_timer(WAVE_TIMER, -160));
    RunUpdates(1);
    CHECK_EQ(channel->timer, WAVE_TIMER);
}

// Modulation: depth 10, speed 32, range 1, on the pitch, after a delay of 2 updates. The LFO's counter moves 32 * 64 =
// 2048 a update, its high byte a step of the 128-step sine every eighth of 256: steps 8, 16, 24... after the delay; the
// pitch moves by sin * 10 * 1 * 64 >> 14
static const u8 sSeqModulation[] = {
    0xca, 10, 0xcb, 32, 0xcd, 1, 0xcc, 0, 0xe0, 2, 0, 0x3c, 127, 0x7f, 0xff,
};

static void TestSeqModulation(void) {
    const Snd7ExChannel *channel;
    int i;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqModulation, sizeof(sSeqModulation));
    RunUpdates(1);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    // The delay: the LFO's value is 0 and its counter waits
    CHECK_EQ(channel->lfo.param.depth, 10);
    CHECK_EQ(channel->lfo.delayCounter, 1);
    CHECK_EQ(channel->timer, WAVE_TIMER);
    RunUpdates(1);
    CHECK_EQ(channel->lfo.delayCounter, 2);
    CHECK_EQ(channel->lfo.counter, 0);
    for (i = 1; i <= 6; i++) {
        s32 lfo = snd7_sin_idx(8 * (i - 1)) * 10 * 64 >> 14;

        RunUpdates(1);
        CHECK_EQ(channel->timer, snd7_calc_timer(WAVE_TIMER, lfo));
        CHECK_EQ(channel->lfo.counter >> 8, 8 * i);
    }
    // The ARM9's depth (SND_SetTrackModDepth) reaches the channel at the next update
    snd7_set_track_param(0, 1, 0x1a, 0, 1);
    RunUpdates(1);
    CHECK_EQ(channel->lfo.param.depth, 0);
    CHECK_EQ(channel->timer, WAVE_TIMER);
}

// Flow: two tracks that the first allocates (0xfe) and opens (0x93); calls, loops and a jump. Track 1 counts its
// notes in variable 0: a call to a note, a loop of 3 notes, then waits; track 0 sets the tempo to 240 (a tick every
// update) and jumps over a note
static const u8 sSeqFlow[] = {
    0xfe, 0x03, 0x00,             // 0: tracks 0 and 1
    0x93, 1,    22,   0, 0,       // 3: track 1 at 22
    0xe1, 240,  0,                // 8: tempo 240
    0x94, 16,   0,    0,          // 11: jump to 16
    0x3c,                         // 15: (skipped)
    0x80, 10,   0xff,             // 16: wait 10, the end
    0xc7, 0,    0xff,             // 19: (unused)
    0x95, 38,   0,    0,          // 22: call 38
    0xd4, 3,                      // 26: a loop of 3
    0xb1, 0,    1,    0, 0x80, 1, // 28: variable 0 += 1, wait 1
    0xfc,                         // 34: the loop's end
    0x80, 2,    0xff,             // 35: wait 2, the end
    0xb0, 0,    0x10, 0,          // 38: variable 0 = 16
    0xfd,                         // 42: return
};

static void TestSeqFlow(void) {
    BootDriver();
    BuildData();
    PlaySequence(0, sSeqFlow, sizeof(sSeqFlow));
    CHECK(Track(0, 1) != NULL);
    CHECK(Track(0, 2) == NULL);
    RunUpdates(1);
    // Tick 1: track 0 opens track 1, which runs after it in the same tick: its call sets 16 and the loop's first pass
    // adds 1
    CHECK_EQ(snd7_work.player[0].tempo, 240);
    CHECK_EQ(Track(0, 0)->wait, 10);
    CHECK_EQ(sShared.player[0].variable[0], 17);
    RunUpdates(1);
    CHECK_EQ(sShared.player[0].variable[0], 18);
    RunUpdates(1);
    CHECK_EQ(sShared.player[0].variable[0], 19);
    CHECK_EQ(Track(0, 1)->callStackDepth, 1);
    RunUpdates(1);
    CHECK_EQ(Track(0, 1)->callStackDepth, 0);
    RunUpdates(2);
    CHECK(Track(0, 1) == NULL);
    CHECK(Track(0, 0) != NULL);
    CHECK_EQ(sShared.player[0].variable[0], 19);
    // Track 0's wait of 10 ends it at tick 11; the player stops with it
    RunUpdates(4);
    CHECK_EQ(sShared.playerStatus, 1);
    RunUpdates(1);
    CHECK_EQ(sShared.playerStatus, 0);
    CHECK_EQ(sShared.player[0].tickCounter, 10);
}

// Variables: random, a variable as an argument, arithmetic, comparisons and the conditional
static const u8 sSeqVariables[] = {
    0xb0, 1,    0x05, 0x00,           // v1 = 5
    0xa1, 0xb0, 2,    1,              // v2 = v1
    0xb3, 2,    0xfd, 0xff,           // v2 *= -3: -15
    0xb4, 2,    2,    0,              // v2 /= 2: -7 (toward 0)
    0xb5, 1,    3,    0,              // v1 <<= 3: 40
    0xb5, 1,    0xfe, 0xff,           // v1 >>= 2: 10
    0xa0, 0xb0, 3,    10,   0, 20, 0, // v3 = random 10..20
    0xb6, 4,    100,  0,              // v4 = random 0..100
    0xbb, 3,    20,   0,              // v3 <= 20
    0xa2, 0xb0, 5,    1,    0,        // if so v5 = 1
    0xb8, 3,    21,   0,              // v3 == 21: false
    0xa2, 0xb0, 6,    1,    0,        // (skipped)
    0xb0, 16,   0x34, 0x12,           // global 0 = 0x1234
    0x80, 1,    0xff,
};

static void TestSeqVariables(void) {
    BootDriver();
    BuildData();
    snd7_set_random(1);
    PlaySequence(2, sSeqVariables, sizeof(sSeqVariables));
    RunUpdates(1);
    CHECK_EQ(sShared.player[2].variable[1], 10);
    CHECK_EQ(sShared.player[2].variable[2], -7);
    CHECK(sShared.player[2].variable[3] >= 10 && sShared.player[2].variable[3] <= 20);
    CHECK(sShared.player[2].variable[4] >= 0 && sShared.player[2].variable[4] <= 100);
    CHECK_EQ(sShared.player[2].variable[5], 1);
    CHECK_EQ(sShared.player[2].variable[6], SND_VARIABLE_UNSET);
    CHECK_EQ(sShared.globalVariable[0], 0x1234);
    // The random values are SND_CalcRandom's, from the seed: the generator's next two
    snd7_set_random(1);
    {
        u16 first = snd7_calc_random();
        u16 second = snd7_calc_random();

        CHECK_EQ(sShared.player[2].variable[3], ((s32)first * 11 >> 16) + 10);
        CHECK_EQ(sShared.player[2].variable[4], (s32)second * 101 >> 16);
    }
}

// Mute: a muted track starts no notes; the third type releases and frees its channels; the ARM9's mute too
static const u8 sSeqMute[] = { 0xd7, 1, 0x3c, 127, 2, 0xd7, 0, 0x3c, 127, 8, 0xff };

static void TestSeqMute(void) {
    const Snd7ExChannel *channel;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqMute, sizeof(sSeqMute));
    RunUpdates(1);
    CHECK(TrackChannel(0, 0) == NULL);
    RunUpdates(4);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL);
    snd7_set_track_mute(0, 1, 3);
    CHECK(TrackChannel(0, 0) == NULL);
    CHECK_EQ(channel->envStatus, SND7_ENV_RELEASE);
    CHECK_EQ(channel->release, 0xffff);
    CHECK(Track(0, 0)->flags & SND7_TRACK_MUTE);
}

// The player's controls: the tempo ratio (SND_SetPlayerTempoRatio, 512: twice as fast), pause (which releases the
// notes), the channels a track may take (only 2), priority, stop
static const u8 sSeqLong[] = { 0x3c, 127, 0x83, 0x60, 0xff }; // a note of 480 ticks

static void TestPlayerControls(void) {
    const Snd7ExChannel *channel;

    BootDriver();
    BuildData();
    PlaySequence(0, sSeqLong, sizeof(sSeqLong));
    snd7_set_track_allocatable_channel(0, 0xffff, 1u << 2);
    snd7_set_player_param(0, 0x1a, 512, 2);
    snd7_set_player_param(0, 4, 100, 1);
    RunUpdates(4);
    // Ticks at every update: 4
    CHECK_EQ(sShared.player[0].tickCounter, 4);
    channel = TrackChannel(0, 0);
    CHECK(channel != NULL && channel->myNo == 2);
    CHECK_EQ(channel->prio, 100 + 64);
    snd7_pause_seq(0, TRUE);
    CHECK(TrackChannel(0, 0) == NULL);
    CHECK_EQ(channel->envStatus, SND7_ENV_RELEASE);
    RunUpdates(4);
    CHECK_EQ(sShared.player[0].tickCounter, 4);
    snd7_pause_seq(0, FALSE);
    RunUpdates(1);
    CHECK_EQ(sShared.player[0].tickCounter, 5);
    snd7_stop_seq(0);
    CHECK_EQ(sShared.playerStatus, 0);
}

// A track whose data leads out of its sequence's memory ends there (a host bound; the DS reads on)
static const u8 sSeqOutside[] = { 0x80, 2, 0x94, 0xff, 0xff, 0xff };

static void TestSeqOutside(void) {
    BootDriver();
    BuildData();
    PlaySequence(0, sSeqOutside, sizeof(sSeqOutside));
    RunUpdates(6);
    CHECK_EQ(sShared.playerStatus, 0);
    CHECK_EQ(sShared.player[0].tickCounter, 2);
}

// A sequence with no byte at its start: one the host couldn't find (no memory), and one whose offset is its end. The
// track stays where it was put, without reading the byte before, and the player ends at its first tick
static const u8 sSeqShort[] = { 0x3c, 100, 1, 0xff };

static void TestSeqNothing(void) {
    BootDriver();
    BuildData();
    snd7_start_seq(0, NULL, NULL, 0, sBank, sBank + sizeof(sBank));
    CHECK(Track(0, 0) != NULL && Track(0, 0)->cur == NULL);
    CHECK(!Track(0, 0)->overrun);
    snd7_start_seq(1, sSeqShort, sSeqShort + sizeof(sSeqShort), sizeof(sSeqShort), sBank, sBank + sizeof(sBank));
    CHECK(Track(1, 0) != NULL && Track(1, 0)->cur == sSeqShort + sizeof(sSeqShort));
    CHECK(!Track(1, 0)->overrun);
    CHECK_EQ(sShared.playerStatus, 3);
    RunUpdates(4);
    CHECK_EQ(sShared.playerStatus, 0);
    CHECK_EQ(sShared.channelStatus, 0);
}

// SND_ReadDriverInfo's state, as SND_ReadTrackInfo reads it
static SNDDriverInfo sInfo;

static void TestDriverInfo(void) {
    SNDTrackInfo trackInfo;
    u8 seq[] = { 0xc0, 80, 0x3c, 127, 0x10, 0xff };

    BootDriver();
    BuildData();
    PlaySequence(3, seq, sizeof(seq));
    RunUpdates(1);
    // A player that never played has the zeros its work starts with, track 0 every one, as on the DS; one with none
    snd7_work.player[0].tracks[0] = SND7_TRACK_NONE;
    snd7_read_driver_info(&sInfo);
    CHECK(SND_ReadTrackInfo(&sInfo, 3, 0, &trackInfo));
    CHECK_EQ(trackInfo.prgNo, 0);
    CHECK_EQ(trackInfo.volume, 127);
    CHECK_EQ(trackInfo.pan, 80);
    CHECK_EQ(trackInfo.chCount, 1);
    CHECK_EQ(trackInfo.channel[0], 4);
    CHECK(!SND_ReadTrackInfo(&sInfo, 0, 0, &trackInfo));
    CHECK(!SND_ReadTrackInfo(&sInfo, 3, 1, &trackInfo));
}

// Through the ARM9's SND library and the PXI bus

static u8 sWave16[64];

// A wave played on a locked channel, as NNS's wave output does it: SND_SetupChannelPcm, then SND_StartTimer; the
// channel's bit in the shared work goes up at once and down at the driver's first run after the wave's 32 samples
static void TestPxiWaveOut(void) {
    u32 tag;
    int i;

    BootOs();
    SND_Init();
    port_snd7_set_output(Capture, NULL);
    ResetSound(TRUE);
    for (i = 0; i < 32; i++) {
        sWave16[i] = (u8)(i * 4);
    }
    SND_LockChannel(1u << 2, 0);
    SND_SetupChannelPcm(2, SND_WAVE_FORMAT_PCM8, sWave16, SND_CHANNEL_LOOP_1SHOT, 0, 8, 127, 0, 512, 64);
    SND_StartTimer(1u << 2, 0, 0, 0);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
    CHECK_EQ(SND_GetChannelStatus(), 1u << 2);
    WaitFrames(12);
    CHECK_EQ(SND_GetChannelStatus(), 0);
    // The wave's samples reached the output: sample i * 4 at volume 127, center: i * 1024 * 127 * 64 * 127 >> 21
    {
        BOOL found = FALSE;
        u64 frame;

        for (frame = 0; frame + 1 < sSound.keptFrames; frame++) {
            if (sSound.kept[frame * 2] == (s16)((s64)4 * 256 * 127 * 64 * 127 >> 21) &&
                sSound.kept[frame * 2 + 2] == (s16)((s64)8 * 256 * 127 * 64 * 127 >> 21)) {
                found = TRUE;
                break;
            }
        }
        CHECK(found);
    }
    Log("%llu frames of sound in 12 frames", (unsigned long long)sSound.frames);
    CHECK(sSound.frames >= 12 * 547 - 547 && sSound.frames <= 13 * 547);
}

// A sequence through the ARM9: SND_PrepareSeq with a bank, SND_StartPreparedSeq; the driver finds both from the
// pointers noted for the commands
static void TestPxiSequence(void) {
    u32 tag;

    BootOs();
    SND_Init();
    BuildData();
    SND_PrepareSeq(1, sSeqNote, 0, (const SNDBankData *)sBank);
    SND_StartPreparedSeq(1);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK | SND_COMMAND_IMMEDIATE);
    SND_WaitForCommandProc(tag);
    CHECK_EQ(SND_GetPlayerStatus(), 1u << 1);
    WaitFrames(10);
    CHECK(SND_GetChannelStatus() & (1u << 4));
    CHECK(SND_GetPlayerTickCounter(1) > 0);
    // SND_InvalidateBankData stops the player whose bank is in the range, at once
    SND_InvalidateBankData(sBank, sBank + sizeof(sBank));
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK | SND_COMMAND_IMMEDIATE);
    SND_WaitForCommandProc(tag);
    CHECK_EQ(SND_GetPlayerStatus(), 0);
}

// SND_CalcRandom's generator, n draws on from the seed
static u16 RandomAfter(u32 seed, int draws) {
    while (draws-- > 0) {
        seed = seed * 1664525 + 1013904223;
    }
    return (u16)(seed >> 16);
}

// The runs that the DS's driver makes for the ARM9's waits (snd_command_host.c), each drawing from SND_CalcRandom's
// generator once: SND_WaitForCommandProc right after a flush wakes the driver for a run that isn't periodic; a second
// wait, a flush with SND_COMMAND_IMMEDIATE (which wakes it itself) and a wait after a periodic run don't
static void TestPxiWakeUps(void) {
    u32 tag;

    BootOs();
    SND_Init();
    snd7_set_random(1);
    SND_SetMasterVolume(100);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    CHECK(snd7_holds_packet(tag));
    SND_WaitForCommandProc(tag);
    CHECK(!snd7_holds_packet(tag));
    SND_WaitForCommandProc(tag);
    CHECK_EQ(snd7_calc_random(), RandomAfter(1, 2));
    CHECK_EQ(snd7_hw_master_volume(), 100);

    snd7_set_random(2);
    SND_SetMasterVolume(110);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK | SND_COMMAND_IMMEDIATE);
    SND_WaitForCommandProc(tag);
    CHECK_EQ(snd7_calc_random(), RandomAfter(2, 2));

    // Nothing waiting: the tag of the last packet taken back
    snd7_set_random(3);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    SND_WaitForCommandProc(tag);
    CHECK_EQ(snd7_calc_random(), RandomAfter(3, 1));

    // The driver's periodic runs start 0x10000 ticks after it does, then come every 2728: 10 frames hold some
    SND_SetMasterVolume(120);
    tag = SND_GetCurrentCommandTag();
    SND_FlushCommand(SND_COMMAND_BLOCK);
    WaitFrames(10);
    CHECK(!snd7_holds_packet(tag));
    snd7_set_random(4);
    SND_WaitForCommandProc(tag);
    CHECK_EQ(snd7_calc_random(), RandomAfter(4, 1));
    CHECK_EQ(snd7_hw_master_volume(), 120);
}

// The host's banks and wave archives (snd_bank_host.c): links as handles, the walk over the instruments, the waves
// loaded by themselves
static void TestBankHost(void) {
    static u8 sBank2[sizeof(sBank)];
    static u8 sWaveArc2[sizeof(sWaveArc)];
    SNDInstPos pos;
    SNDInstData inst;
    int count = 0;
    u32 link;

    BootOs();
    BuildData();
    memcpy(sBank2, sBank, sizeof(sBank));
    memset(sBank2 + 0x18, 0, 0x20);
    memcpy(sWaveArc2, sWaveArc, sizeof(sWaveArc));
    Put32(sWaveArc2 + 0x18, 0);
    // Two banks on one archive, then the first on another
    SND_AssignWaveArc((SNDBankData *)sBank2, 0, (SNDWaveArc *)sWaveArc);
    memcpy(&link, sWaveArc + 0x18, 4);
    CHECK(snd7_handle_pointer(link) == sBank2 + 0x18);
    memcpy(&link, sBank2 + 0x1c, 4);
    CHECK(snd7_handle_pointer(link) == sBank + 0x18);
    SND_AssignWaveArc((SNDBankData *)sBank, 0, (SNDWaveArc *)sWaveArc2);
    memcpy(&link, sBank2 + 0x1c, 4);
    CHECK_EQ(link, 0);
    memcpy(&link, sWaveArc2 + 0x18, 4);
    CHECK(snd7_handle_pointer(link) == sBank + 0x18);
    SND_DestroyWaveArc((SNDWaveArc *)sWaveArc2);
    memcpy(&link, sBank + 0x18, 4);
    CHECK_EQ(link, 0);
    // The instruments, one by one: 3 plain, 3 drums, 2 of the split, 1 plain
    pos = SND_GetFirstInstDataPos((const SNDBankData *)sBank);
    while (SND_GetNextInstData((const SNDBankData *)sBank, &inst, &pos)) {
        count++;
    }
    CHECK_EQ(count, 9);
    // A wave loaded by itself, and back to none
    CHECK_EQ(SND_GetWaveDataCount((const SNDWaveArc *)sWaveArc), 2);
    CHECK(SND_GetWaveDataAddress((const SNDWaveArc *)sWaveArc, 1) == (const SNDWaveData *)(sWaveArc + 0x60));
    SND_SetWaveDataAddress((SNDWaveArc *)sWaveArc, 1, (const SNDWaveData *)(sWaveArc2 + 0x44));
    CHECK(SND_GetWaveDataAddress((const SNDWaveArc *)sWaveArc, 1) == (const SNDWaveData *)(sWaveArc2 + 0x44));
    SND_SetWaveDataAddress((SNDWaveArc *)sWaveArc, 1, NULL);
    CHECK(SND_GetWaveDataAddress((const SNDWaveArc *)sWaveArc, 1) == NULL);
}

// The host's WAV dump (port/audio): its head, and every frame the driver made
static void TestAudioDump(void) {
    char path[512];
    u8 header[44];
    FILE *file;
    long size;
    u32 dataBytes;
    u64 frames;
    const char *dir = getenv("PORT_SND_TMP");

    snprintf(path, sizeof(path), "%s/port_snd_dump_%d.wav", dir != NULL ? dir : ".", (int)getpid());
    BootOs();
    port_snd7_boot_at(0);
    CHECK(port_audio_dump_open(path));
    snd7_setup_channel_psg(8, 3, 127, 0, 0x1f46, 64);
    snd7_start_channel(8);
    port_snd7_advance(PORT_SND7_CYCLES_PER_SAMPLE * 1000 - 1);
    frames = port_snd7_samples();
    port_audio_close();
    CHECK_EQ(frames, 1000);
    file = fopen(path, "rb");
    CHECK(file != NULL);
    if (file == NULL) {
        return;
    }
    CHECK_EQ(fread(header, 1, sizeof(header), file), sizeof(header));
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fclose(file);
    remove(path);
    memcpy(&dataBytes, header + 40, 4);
    CHECK(memcmp(header, "RIFF", 4) == 0 && memcmp(header + 8, "WAVEfmt ", 8) == 0);
    CHECK_EQ(header[22] | header[23] << 8, 2);
    CHECK_EQ(header[24] | header[25] << 8 | header[26] << 16, PORT_SND7_SAMPLE_RATE);
    CHECK_EQ(dataBytes, frames * 4);
    CHECK_EQ(size, 44 + (long)frames * 4);
}

// The ROM's sequences

typedef struct Sdat {
    const u8 *data;
    u32 size;
    u32 info;
    u32 symbols;
    u32 fat;
} Sdat;

static u32 Get32(const u8 *at) {
    return (u32)at[0] | (u32)at[1] << 8 | (u32)at[2] << 16 | (u32)at[3] << 24;
}

static u16 Get16(const u8 *at) {
    return (u16)(at[0] | at[1] << 8);
}

// A file of the ROM's NitroFS root by name
static const u8 *RootFile(const u8 *rom, size_t romSize, const char *name, u32 *size) {
    u32 fnt = Get32(rom + 0x40);
    u32 fatOffset = Get32(rom + 0x48);
    const u8 *entry = rom + fnt + Get32(rom + fnt);
    u32 fileId = Get16(rom + fnt + 4);

    while (*entry != 0) {
        u32 length = *entry & 0x7f;
        BOOL directory = (*entry & 0x80) != 0;

        if (!directory && length == strlen(name) && memcmp(entry + 1, name, length) == 0) {
            u32 start = Get32(rom + fatOffset + fileId * 8);
            u32 end = Get32(rom + fatOffset + fileId * 8 + 4);

            if (end > romSize || start > end) {
                return NULL;
            }
            *size = end - start;
            return rom + start;
        }
        entry += 1 + length + (directory ? 2 : 0);
        if (!directory) {
            fileId++;
        }
    }
    return NULL;
}

// An entry of the SDAT's info block (0 sequences, 2 banks, 3 wave archives, 4 players), or NULL
static const u8 *SdatInfo(const Sdat *sdat, int type, u32 index) {
    const u8 *record = sdat->data + sdat->info + Get32(sdat->data + sdat->info + 8 + 4 * (u32)type);
    u32 offset;

    if (index >= Get32(record)) {
        return NULL;
    }
    offset = Get32(record + 4 + 4 * index);
    return offset != 0 ? sdat->data + sdat->info + offset : NULL;
}

static const u8 *SdatFile(const Sdat *sdat, u32 fileId, u32 *size) {
    const u8 *entry = sdat->data + sdat->fat + 12 + 16 * fileId;

    *size = Get32(entry + 4);
    return sdat->data + Get32(entry);
}

// A sequence's number by its name in the symbol block, or -1
static int SdatSequence(const Sdat *sdat, const char *name) {
    const u8 *symbols = sdat->data + sdat->symbols;
    const u8 *record = symbols + Get32(symbols + 8);
    u32 count = Get32(record);
    u32 i;

    for (i = 0; i < count; i++) {
        u32 offset = Get32(record + 4 + 4 * i);

        if (offset != 0 && strcmp((const char *)symbols + offset, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static BOOL OpenSdat(Sdat *sdat) {
    char message[512];
    const char *path = port_rom_default_path();

    if (path == NULL || port_rom_open(path, message, sizeof(message)) == PORT_ROM_REFUSED) {
        printf("no ROM: skipped\n");
        return FALSE;
    }
    sdat->data = RootFile(port_rom_data(), port_rom_size(), "swan_sound_data.sdat", &sdat->size);
    if (sdat->data == NULL || memcmp(sdat->data, "SDAT", 4) != 0) {
        printf("no swan_sound_data.sdat in the ROM\n");
        return FALSE;
    }
    sdat->symbols = Get32(sdat->data + 0x10);
    sdat->info = Get32(sdat->data + 0x18);
    sdat->fat = Get32(sdat->data + 0x20);
    return TRUE;
}

// A sequence's files, loaded as NitroSystem's archive player loads them (a copy each, its bank linked to its wave
// archives), and its player set up from the archive's info: its volume, priority and channels
typedef struct Loaded {
    u8 *sequence;
    u32 sequenceSize;
    u8 *bank;
    u32 bankSize;
    u8 *waveArcs[4];
    int playerNo;
    int volume;
    int channelPrio;
    u32 allocatable;
} Loaded;

static u8 *Copy(const u8 *data, u32 size) {
    u8 *copy = malloc(size != 0 ? size : 1);

    if (copy == NULL) {
        printf("no memory\n");
        exit(2);
    }
    memcpy(copy, data, size);
    return copy;
}

static BOOL LoadSequence(const Sdat *sdat, int seqNo, Loaded *loaded) {
    const u8 *info = SdatInfo(sdat, 0, (u32)seqNo);
    const u8 *bankInfo;
    const u8 *playerInfo;
    const u8 *file;
    u32 size;
    int i;

    memset(loaded, 0, sizeof(*loaded));
    if (info == NULL || (bankInfo = SdatInfo(sdat, 2, Get16(info + 4))) == NULL) {
        return FALSE;
    }
    file = SdatFile(sdat, Get32(info) & 0xffffff, &size);
    loaded->sequence = Copy(file, size);
    loaded->sequenceSize = size;
    file = SdatFile(sdat, Get32(bankInfo) & 0xffffff, &size);
    loaded->bank = Copy(file, size);
    loaded->bankSize = size;
    for (i = 0; i < 4; i++) {
        u16 waveArcNo = Get16(bankInfo + 4 + 2 * i);
        const u8 *waveArcInfo = waveArcNo != 0xffff ? SdatInfo(sdat, 3, waveArcNo) : NULL;

        if (waveArcInfo != NULL) {
            file = SdatFile(sdat, Get32(waveArcInfo) & 0xffffff, &size);
            loaded->waveArcs[i] = Copy(file, size);
            SND_AssignWaveArc((SNDBankData *)loaded->bank, i, (SNDWaveArc *)loaded->waveArcs[i]);
        }
    }
    loaded->volume = info[6];
    loaded->channelPrio = info[7];
    loaded->playerNo = info[9];
    playerInfo = SdatInfo(sdat, 4, (u32)loaded->playerNo);
    loaded->allocatable = playerInfo != NULL ? Get16(playerInfo + 2) : 0;
    return TRUE;
}

static void FreeLoaded(Loaded *loaded) {
    int i;

    for (i = 0; i < 4; i++) {
        if (loaded->waveArcs[i] != NULL) {
            SND_DestroyWaveArc((SNDWaveArc *)loaded->waveArcs[i]);
            free(loaded->waveArcs[i]);
        }
    }
    if (loaded->bank != NULL) {
        SND_DestroyBank((SNDBankData *)loaded->bank);
    }
    free(loaded->bank);
    free(loaded->sequence);
}

// Starts the sequence on its player as NitroSystem's player does: SND_PrepareSeq at the data's base, the allocatable
// channels, the player's priority and volume (the decibels of the info's volume), SND_StartPreparedSeq
static void StartLoaded(const Loaded *loaded) {
    int playerNo = loaded->playerNo & 0xf;

    snd7_prepare_seq(playerNo, loaded->sequence + Get32(loaded->sequence + 0x18),
                     loaded->sequence + loaded->sequenceSize, 0, loaded->bank, loaded->bank + loaded->bankSize);
    if (loaded->allocatable != 0) {
        snd7_set_track_allocatable_channel(playerNo, 0xffff, loaded->allocatable);
    }
    snd7_set_player_param(playerNo, 4, (u32)loaded->channelPrio, 1);
    snd7_set_player_param(playerNo, 6, (u32)SNDi_DecibelTable[loaded->volume & 0x7f], 2);
    snd7_start_prepared_seq(playerNo);
}

static void WriteWav(const char *name) {
    const char *dir = getenv("PORT_SND_WAV");
    char path[512];
    FILE *file;
    u8 header[44];
    u32 bytes = (u32)(sSound.keptFrames * 4);

    if (dir == NULL || sSound.kept == NULL) {
        return;
    }
    snprintf(path, sizeof(path), "%s/%s.wav", dir, name);
    file = fopen(path, "wb");
    if (file == NULL) {
        return;
    }
    memcpy(header, "RIFF", 4);
    Put32(header + 4, 36 + bytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    Put32(header + 16, 16);
    Put16(header + 20, 1);
    Put16(header + 22, 2);
    Put32(header + 24, PORT_SND7_SAMPLE_RATE);
    Put32(header + 28, PORT_SND7_SAMPLE_RATE * 4);
    Put16(header + 32, 4);
    Put16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    Put32(header + 40, bytes);
    fwrite(header, 1, sizeof(header), file);
    fwrite(sSound.kept, 4, sSound.keptFrames, file);
    fclose(file);
}

// Plays a sequence for seconds (or until it ends, then a second more), and returns its sound's hash
static u64 PlayRomSequence(const Sdat *sdat, const char *name, u32 seconds, BOOL *ended) {
    Loaded loaded;
    int seqNo = SdatSequence(sdat, name);
    u32 updates = (u32)((u64)seconds * PORT_OS_CYCLES_PER_SECOND / UPDATE_CYCLES);
    u32 after = 0;
    u32 u;

    *ended = FALSE;
    if (seqNo < 0 || !LoadSequence(sdat, seqNo, &loaded)) {
        printf("  no sequence %s\n", name);
        sFailed = 1;
        return 0;
    }
    BootDriver();
    ResetSound(getenv("PORT_SND_WAV") != NULL);
    StartLoaded(&loaded);
    for (u = 0; u < updates; u++) {
        RunUpdates(1);
        if (sShared.playerStatus == 0) {
            *ended = TRUE;
            if (++after == 192) {
                break;
            }
        }
    }
    snd7_stop_seq(loaded.playerNo & 0xf);
    RunUpdates(2);
    WriteWav(name);
    Log("%s: %llu frames, %s, peak %d, rms %.0f, hash %016llx", name, (unsigned long long)sSound.frames,
        *ended ? "ended" : "plays on", sSound.peak,
        sSound.frames != 0 ? sqrt(sSound.energy / (double)(sSound.frames * 2)) : 0.0, (unsigned long long)sSound.hash);
    FreeLoaded(&loaded);
    return sSound.hash;
}

// The sequences and their sound's hashes over their first seconds: the driver's output, which must not change unless
// the driver does (then the hashes are made again, PORT_SND_VERBOSE printing them, with the change's reason)
typedef struct RomCase {
    const char *name;
    u32 seconds;
    u64 hash;
} RomCase;

static const RomCase sRomCases[] = {
    { "SEQ_BGM_TITLE", 20, 0x624427502c4ea7d2ull }, { "SEQ_BGM_POKECEN", 20, 0xa6884ffd9f7a676bull },
    { "SEQ_BGM_WIN1", 10, 0x332d611b70f1e41bull },  { "SEQ_ME_ITEM", 5, 0xec11d0a94fdd0cb2ull },
    { "SEQ_SE_DECIDE1", 2, 0x783fc13ec2cfb949ull }, { "SEQ_SE_SELECT1", 2, 0x81d2d21896629139ull },
};

static void TestRomSequences(void) {
    Sdat sdat;
    size_t i;

    BootOs();
    if (!OpenSdat(&sdat)) {
        sSkipped = 1;
        return;
    }
    for (i = 0; i < NELEMS(sRomCases); i++) {
        BOOL ended;
        u64 hash = PlayRomSequence(&sdat, sRomCases[i].name, sRomCases[i].seconds, &ended);
        u64 again = PlayRomSequence(&sdat, sRomCases[i].name, sRomCases[i].seconds, &ended);

        // The same sound each time, and the one recorded
        CHECK_EQ(again, hash);
        CHECK(sSound.peak > 0);
        if (sRomCases[i].hash != 0 && hash != sRomCases[i].hash) {
            printf("  %s: hash %016llx, not %016llx\n", sRomCases[i].name, (unsigned long long)hash,
                   (unsigned long long)sRomCases[i].hash);
            sFailed = 1;
        }
    }
}

// Every sequence of the archive for 4 seconds: none reads outside its data or stops the driver, and few are silent
static void TestRomEverySequence(void) {
    Sdat sdat;
    u32 count;
    u32 seqNo;
    u32 played = 0;
    u32 silent = 0;

    BootOs();
    if (!OpenSdat(&sdat)) {
        sSkipped = 1;
        return;
    }
    count = Get32(sdat.data + sdat.info + Get32(sdat.data + sdat.info + 8));
    for (seqNo = 0; seqNo < count; seqNo++) {
        Loaded loaded;
        u32 u;

        if (!LoadSequence(&sdat, (int)seqNo, &loaded)) {
            continue;
        }
        BootDriver();
        StartLoaded(&loaded);
        for (u = 0; u < 4 * 192; u++) {
            RunUpdates(1);
        }
        if (sSound.peak == 0) {
            silent++;
            Log("silent: %u", seqNo);
        }
        FreeLoaded(&loaded);
        played++;
    }
    Log("%u sequences played, %u silent", played, silent);
    CHECK(played > 1000);
    CHECK(silent < played / 20);
}

// For a look at one sequence: PORT_SND_SEQ=NAME for PORT_SND_SECONDS (10 by default), its stats in the log and with
// PORT_SND_WAV its sound. Not a test of its own (no ctest entry)
static void TestRomPlay(void) {
    Sdat sdat;
    const char *name = getenv("PORT_SND_SEQ");
    const char *seconds = getenv("PORT_SND_SECONDS");
    BOOL ended;

    BootOs();
    if (name == NULL || !OpenSdat(&sdat)) {
        sSkipped = 1;
        return;
    }
    PlayRomSequence(&sdat, name, seconds != NULL ? (u32)atoi(seconds) : 10, &ended);
}

typedef struct Test {
    const char *name;
    void (*main)(void);
} Test;

static const Test sTests[] = {
    { "adpcm_decoder", TestAdpcmDecoder },
    { "adpcm_channel", TestAdpcmChannel },
    { "pcm8_loop", TestPcm8Loop },
    { "short_sound", TestShortSound },
    { "psg_duty", TestPsgDuty },
    { "noise", TestNoise },
    { "mixer", TestMixer },
    { "calc_timer", TestCalcTimer },
    { "alloc", TestAlloc },
    { "lock_stops", TestLockStops },
    { "seq_note", TestSeqNote },
    { "seq_params", TestSeqParams },
    { "seq_programs", TestSeqPrograms },
    { "seq_envelope", TestSeqEnvelope },
    { "seq_tie", TestSeqTie },
    { "seq_sweep", TestSeqSweep },
    { "seq_modulation", TestSeqModulation },
    { "seq_flow", TestSeqFlow },
    { "seq_variables", TestSeqVariables },
    { "seq_mute", TestSeqMute },
    { "seq_outside", TestSeqOutside },
    { "seq_nothing", TestSeqNothing },
    { "player_controls", TestPlayerControls },
    { "driver_info", TestDriverInfo },
    { "bank_host", TestBankHost },
    { "pxi_wave_out", TestPxiWaveOut },
    { "pxi_sequence", TestPxiSequence },
    { "pxi_wake_ups", TestPxiWakeUps },
    { "audio_dump", TestAudioDump },
    { "rom_sequences", TestRomSequences },
    { "rom_every_sequence", TestRomEverySequence },
    { "rom_play", TestRomPlay },
};

static void PrintLog(void) {
    int i;

    for (i = 0; i < sLogCount; i++) {
        printf("  | %s\n", sLog[i]);
    }
}

static void OnHang(int signal) {
    static const char message[] = "hang: the scenario didn't end\n";
    void *frames[64];
    int count;

    (void)signal;
    (void)!write(STDERR_FILENO, message, sizeof(message) - 1);
    count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    _Exit(3);
}

int main(int argc, char **argv) {
    const char *name = argc > 1 ? argv[1] : "";
    size_t i;
    PortOsResult result;

    setvbuf(stdout, NULL, _IONBF, 0);
    signal(SIGALRM, OnHang);
    alarm(strncmp(name, "rom_", 4) == 0 ? 1200 : 50);
    for (i = 0; i < NELEMS(sTests); i++) {
        if (strcmp(name, sTests[i].name) == 0) {
            result = port_os_run(sTests[i].main);
            CHECK(result == PORT_OS_RETURNED);
            if (sSkipped && !sFailed) {
                return SKIP_STATUS;
            }
            printf("%s: %s\n", name, sFailed ? "FAIL" : "ok");
            if (sFailed || getenv("PORT_SND_VERBOSE") != NULL) {
                PrintLog();
            }
            return sFailed;
        }
    }
    printf("usage: %s NAME\n", argv[0]);
    return 2;
}
