// The host ARM7's sound driver (port/hal/snd7), which plays the game's music, sound effects and cries: what its files
// share. Nothing outside port/hal/snd7 and its tests (port/tests/snd) includes it.
//
// The DS's ARM7 runs TwlSDK 5.3's stock SND driver, which the decomp doesn't have. These files are its C, written from
// the ARM7's own code (extract/<version>/arm7/arm7.bin; its WRAM autoload at 0x037f8000) and GBATEK's sound chapter.
// The driver's files and the ARM7 addresses of their functions, for the reader who wants to compare:
//   snd7.c          the thread (SndThread 0x038006ac), the commands (SND_CommandProc 0x03803450), the alarms
//                   (0x03803208-0x03803404), the shared work (SND_UpdateSharedWork 0x03803180) and the driver state
//   snd7_exchannel.c the driver's channels (SND_ExChannelInit 0x0380081c to SND_ExChannelStart 0x03801528), the
//                   timer and volume arithmetic (0x03800378-0x038005a4) and the channel locks
//   snd7_seq.c      the players and tracks (SND_SeqInit 0x03801560 to SND_NoteOn 0x03802ffc)
//   snd7_hw.c       the sound unit's registers, as the driver's register functions (0x037ffd60-0x0380036c) write them,
//                   and the sound unit itself: the 16 channels and the mixer, which the DS does in hardware
//   snd7_tables.c   the decibel table (0x03808e48) and the BIOS's pitch table, which the ARM7 reads by SWI
// Beside them, the ARM9's side that the host replaces: snd_interface_host.c (the commands' pointers),
// snd_command_host.c (the wake-up of SND_WaitForCommandProc) and snd_bank_host.c (banks and wave archives, and the
// handles they keep for pointers). The volume table the ARM7 reads
// from its BIOS (SWI 0x1c) is the one the ARM9's SND_CalcChannelVolume has (lib/nitro/src/snd/snd_util.c), which the
// driver calls.
// The ARM7's work (SNDi_Work at 0x03809fbc, 0x1180 bytes) holds the channels, players, tracks and alarms; here it is
// Snd7Work, with host pointers where the DS has ARM7 addresses. A player's and a track's fields that the ARM9 writes by
// their offset (SNDi_SetPlayerParam and SNDi_SetTrackParam) keep the DS's layout.
#ifndef PORT_HAL_SND7_SND7_INTERNAL_H
#define PORT_HAL_SND7_SND7_INTERNAL_H

#include "types.h"
#include "nitro/snd.h"
#include <stddef.h>
#include <stdint.h>

#define SND7_CHANNEL_NUM 16
#define SND7_PLAYER_NUM 16
#define SND7_TRACK_NUM 32
#define SND7_ALARM_NUM 8
#define SND7_PLAYER_TRACK_NUM 16
#define SND7_CALL_STACK_DEPTH 3
#define SND7_CAPTURE_NUM 2

// The DS's clocks: the ARM7's bus clock is the virtual clock's (port_os_cycles); the sound unit runs at half of it, and
// the mixer takes a sample of the channels every 512 of its ticks, so every 1024 cycles
#define SND7_CYCLES_PER_SAMPLE 1024u
#define SND7_SOUND_TICKS_PER_SAMPLE 512u
// The OS's tick, which the alarms count, is 64 cycles
#define SND7_OS_TICK_CYCLES 64u
// The driver's update: a periodic OS alarm of 2728 ticks (SND_PROC_INTERVAL), the first 0x10000 ticks after the
// driver starts (SndThread)
#define SND7_UPDATE_TICKS 2728u
#define SND7_FIRST_UPDATE_TICKS 0x10000u

// The decibel volumes the driver works in: SND_VOLUME_DB_MIN (-72.3 dB) is silence
#define SND7_VOLUME_DB_MIN (-723)

// The driver's channels (NitroSDK's SNDExChannel, 0x54 bytes on the DS)

enum {
    SND7_EXCHANNEL_PCM,
    SND7_EXCHANNEL_PSG,
    SND7_EXCHANNEL_NOISE,
};

// Where the envelope is
enum {
    SND7_ENV_ATTACK,
    SND7_ENV_DECAY,
    SND7_ENV_SUSTAIN,
    SND7_ENV_RELEASE,
};

// What a channel's callback is told: the channel was taken for another sound (DROP), or its sound ended (FINISH)
enum {
    SND7_EXCHANNEL_CALLBACK_DROP,
    SND7_EXCHANNEL_CALLBACK_FINISH,
};

// The byte of flags at offset 3: the bit fields active_flag, start_flag and auto_sweep, then sync_flag, the five
// register writes that SND_UpdateExChannel owes the channel
#define SND7_EXCHANNEL_ACTIVE 0x01
#define SND7_EXCHANNEL_START 0x02
#define SND7_EXCHANNEL_AUTO_SWEEP 0x04
#define SND7_EXCHANNEL_SYNC_SHIFT 3
#define SND7_SYNC_START 0x01
#define SND7_SYNC_STOP 0x02
#define SND7_SYNC_TIMER 0x04
#define SND7_SYNC_VOLUME 0x08
#define SND7_SYNC_PAN 0x10

// What an LFO moves
enum {
    SND7_LFO_PITCH,
    SND7_LFO_VOLUME,
    SND7_LFO_PAN,
};

// SNDLfoParam (6 bytes) and SNDLfo
typedef struct Snd7LfoParam {
    u8 target;
    u8 speed;
    u8 depth;
    u8 range;
    u16 delay;
} Snd7LfoParam;

typedef struct Snd7Lfo {
    Snd7LfoParam param;
    u16 delayCounter;
    u16 counter;
} Snd7Lfo;

// SNDWaveParam: the 12 bytes in front of a wave's samples. The loop's start and length are in words
typedef struct Snd7WaveParam {
    u8 format;
    u8 loopFlag;
    u16 rate;
    u16 timer;
    u16 loopStart;
    u32 loopLen;
} Snd7WaveParam;

#define SND7_WAVE_PARAM_SIZE 12

typedef struct Snd7ExChannel Snd7ExChannel;
typedef void (*Snd7ExChannelCallback)(Snd7ExChannel *channel, int status, void *data);

struct Snd7ExChannel {
    u8 myNo;
    u8 type;
    u8 envStatus;
    u8 flags;
    u8 panRange;
    u8 originalKey;
    s16 userDecay2;
    u8 key;
    u8 velocity;
    s8 initPan;
    s8 userPan;
    s16 userDecay;
    s16 userPitch;
    s32 envDecay;
    s32 sweepCounter;
    s32 sweepLength;
    u8 attack;
    u8 sustain;
    u16 decay;
    u16 release;
    u8 prio;
    u8 pan;
    u16 volume; // the channel volume in the low byte and the data shift in the high one, as SND_CalcChannelVolume
    u16 timer;
    Snd7Lfo lfo;
    s16 sweepPitch;
    s32 length;
    Snd7WaveParam wave;
    // A PCM channel's samples, and where the memory they are in ends (NULL when unknown); a PSG channel's duty, which
    // the DS keeps in the same field
    const u8 *data;
    const u8 *dataEnd;
    u32 duty;
    Snd7ExChannelCallback callback;
    void *callbackData;
    Snd7ExChannel *nextLink;
};

static inline u32 snd7_sync(const Snd7ExChannel *channel) {
    return (u32)channel->flags >> SND7_EXCHANNEL_SYNC_SHIFT;
}

static inline void snd7_set_sync(Snd7ExChannel *channel, u32 sync) {
    channel->flags = (u8)((channel->flags & 7) | (sync << SND7_EXCHANNEL_SYNC_SHIFT));
}

// The players (NitroSDK's SNDPlayer, 0x24 bytes on the DS). The first 0x20 bytes keep the DS's layout, since the ARM9
// writes them by offset (SND_SetPlayerTempoRatio and the like); the bank, an ARM7 address at 0x20, is a host pointer
#define SND7_PLAYER_ACTIVE 0x01
#define SND7_PLAYER_PREPARED 0x02
#define SND7_PLAYER_PAUSE 0x04
#define SND7_TRACK_NONE 0xff

typedef struct Snd7Player {
    u8 flags;
    u8 myNo;
    u8 padding_02[2];
    u8 prio;
    u8 volume;
    s16 extFader;
    u8 tracks[SND7_PLAYER_TRACK_NUM];
    u16 tempo;
    u16 tempoRatio;
    u16 tempoCounter;
    u16 padding_1e;
    const u8 *bank;
    const u8 *bankEnd;
} Snd7Player;

#define SND7_PLAYER_PARAM_SIZE 0x20

// The tracks (NitroSDK's SNDTrack, 0x40 bytes on the DS). The first 0x24 bytes keep the DS's layout, for
// SNDi_SetTrackParam; from 0x24 on the DS has ARM7 addresses, which are host pointers here
#define SND7_TRACK_ACTIVE 0x01
#define SND7_TRACK_NOTE_WAIT 0x02
#define SND7_TRACK_MUTE 0x04
#define SND7_TRACK_TIE 0x08
#define SND7_TRACK_NOTE_FINISH_WAIT 0x10
#define SND7_TRACK_PORTA 0x20
#define SND7_TRACK_CMP 0x40
#define SND7_TRACK_CHANNEL_MASK 0x80

typedef struct Snd7Track {
    u8 flags;
    u8 panRange;
    u16 prgNo;
    u8 volume;
    u8 volume2;
    s8 pitchBend;
    u8 bendRange;
    s8 pan;
    s8 extPan;
    s16 extFader;
    s16 extPitch;
    u8 attack;
    u8 decay;
    u8 sustain;
    u8 release;
    u8 prio;
    s8 transpose;
    u8 portaKey;
    u8 portaTime;
    s16 sweepPitch;
    Snd7LfoParam mod;
    u16 channelMask;
    s32 wait;
    // The sequence's data: its base, where the track reads, and where the memory it is in ends (the host's bound)
    const u8 *base;
    const u8 *cur;
    const u8 *end;
    const u8 *callStack[SND7_CALL_STACK_DEPTH];
    u8 loopCount[SND7_CALL_STACK_DEPTH];
    u8 callStackDepth;
    Snd7ExChannel *channelList;
    // The host's: the track read past the end of its sequence's memory, which ends it
    BOOL overrun;
} Snd7Track;

#define SND7_TRACK_PARAM_SIZE 0x24

// The alarms (NitroSDK's SNDAlarm, 0x40 bytes on the DS with an OSAlarm): enable, the ID the ARM9 gave it, the words
// still to send, the first tick and the period, and the host's state of the OS alarm: when it goes off next, in cycles
// of the virtual clock, and whether it repeats
typedef struct Snd7Alarm {
    u8 enable;
    u8 id;
    u8 count;
    u64 tick;
    u64 period;
    BOOL armed;
    BOOL periodic;
    u64 due;
    u32 handlerArg; // the alarm's number and its ID when it started, which its handler sends
} Snd7Alarm;

typedef struct Snd7Work {
    Snd7ExChannel channel[SND7_CHANNEL_NUM];
    Snd7Player player[SND7_PLAYER_NUM];
    Snd7Track track[SND7_TRACK_NUM];
    Snd7Alarm alarm[SND7_ALARM_NUM];
} Snd7Work;

extern Snd7Work snd7_work;
// The work that the ARM7 and the ARM9 share (SND_COMMAND_SHARED_WORK); NULL until the ARM9 sends it
extern SNDSharedWork *snd7_shared;

// snd7_exchannel.c
void snd7_exchannel_init(void);
void snd7_update_exchannel(void);
void snd7_exchannel_main(BOOL doPeriodicProc);
BOOL snd7_start_exchannel_pcm(Snd7ExChannel *channel, const Snd7WaveParam *wave, const u8 *data, const u8 *dataEnd,
                              s32 length);
BOOL snd7_start_exchannel_psg(Snd7ExChannel *channel, int duty, s32 length);
BOOL snd7_start_exchannel_noise(Snd7ExChannel *channel, s32 length);
void snd7_set_exchannel_attack(Snd7ExChannel *channel, int attack);
void snd7_set_exchannel_decay(Snd7ExChannel *channel, int decay);
void snd7_set_exchannel_sustain(Snd7ExChannel *channel, int sustain);
void snd7_set_exchannel_release(Snd7ExChannel *channel, int release);
void snd7_release_exchannel(Snd7ExChannel *channel);
BOOL snd7_is_exchannel_active(const Snd7ExChannel *channel);
Snd7ExChannel *snd7_alloc_exchannel(u32 chBitMask, int prio, BOOL strongRequest, Snd7ExChannelCallback callback,
                                    void *callbackData);
void snd7_free_exchannel(Snd7ExChannel *channel);
void snd7_stop_unlocked_channel(u32 chBitMask, u32 flags);
void snd7_lock_channel(u32 chBitMask, u32 flags);
void snd7_unlock_channel(u32 chBitMask, u32 flags);
u32 snd7_get_locked_channel(u32 flags);
void snd7_invalidate_wave(const u8 *start, const u8 *end);
void snd7_init_lfo_param(Snd7LfoParam *param);
u16 snd7_calc_timer(int timer, int pitch);
int snd7_sin_idx(int index);
u16 snd7_calc_random(void);
void snd7_set_random(u32 seed);
u32 snd7_random_state(void);
extern const s16 snd7_decibel_square_table[128];
extern const u16 snd7_pitch_table[768];

// snd7_seq.c
void snd7_seq_init(void);
void snd7_seq_main(BOOL doPeriodicProc);
void snd7_prepare_seq(int playerNo, const u8 *seqBase, const u8 *seqEnd, u32 seqOffset, const u8 *bank,
                      const u8 *bankEnd);
void snd7_start_seq(int playerNo, const u8 *seqBase, const u8 *seqEnd, u32 seqOffset, const u8 *bank,
                    const u8 *bankEnd);
void snd7_start_prepared_seq(int playerNo);
void snd7_stop_seq(int playerNo);
void snd7_pause_seq(int playerNo, BOOL flag);
void snd7_skip_seq(int playerNo, u32 tick);
void snd7_set_player_param(int playerNo, u32 offset, u32 data, int size);
void snd7_set_track_param(int playerNo, u32 trackBitMask, u32 offset, u32 data, int size);
void snd7_set_track_mute(int playerNo, u32 trackBitMask, int muteType);
void snd7_set_track_allocatable_channel(int playerNo, u32 trackBitMask, u32 chBitFlag);
void snd7_invalidate_seq(const u8 *start, const u8 *end, u32 start32, u32 end32);
void snd7_invalidate_bank(const u8 *start, const u8 *end, u32 start32, u32 end32);
vs16 *snd7_variable(const Snd7Player *player, int varNo);

// snd7.c: the interval timer, which SND_SkipSeq stops and starts again; SND_ReadDriverInfo's copy of the state;
// whether the DS's driver would still hold a packet that the host ran (for snd_command_host.c)
void snd7_stop_interval_timer(void);
void snd7_start_interval_timer(void);
void snd7_read_driver_info(SNDDriverInfo *info);
BOOL snd7_holds_packet(u32 tag);

// snd7_hw.c: the sound unit's registers, as the driver's functions of snd_channel.c, snd_capture.c and snd_global.c
// write them. A channel's source is a host pointer and the end of the memory it is in
#define SND7_CNT_START 0x80000000u
#define SND7_CNT_HOLD 0x8000u
#define SND7_CHANNEL_STOP_HOLD 1

enum {
    SND7_FORMAT_PCM8,
    SND7_FORMAT_PCM16,
    SND7_FORMAT_ADPCM,
    SND7_FORMAT_PSG,
};

enum {
    SND7_REPEAT_MANUAL,
    SND7_REPEAT_LOOP,
    SND7_REPEAT_ONESHOT,
};

void snd7_hw_reset(void);
void snd7_enable(void);
void snd7_disable(void);
void snd7_set_master_volume(int volume);
void snd7_set_output_selector(int left, int right, int channel1, int channel3);
void snd7_setup_channel_pcm(int chNo, const u8 *data, const u8 *dataEnd, int format, int loop, int loopStart,
                            int loopLen, int volume, int shift, int timer, int pan);
void snd7_setup_channel_psg(int chNo, int duty, int volume, int shift, int timer, int pan);
void snd7_setup_channel_noise(int chNo, int volume, int shift, int timer, int pan);
void snd7_stop_channel(int chNo, s32 flags);
void snd7_set_channel_volume(int chNo, int volume, int shift);
void snd7_set_channel_timer(int chNo, int timer);
void snd7_set_channel_pan(int chNo, int pan);
BOOL snd7_is_channel_active(int chNo);
void snd7_start_channel(int chNo);
void snd7_set_master_pan(int pan);
u32 snd7_get_channel_control(int chNo);
void snd7_set_surround_decay(int decay);
void snd7_setup_capture(int capture, int format, u32 dest, int length, BOOL repeat, int addSrc, int outSrc);
BOOL snd7_is_capture_active(int capture);
void snd7_start_capture(u32 capBitMask);
void snd7_stop_capture(u32 capBitMask);
// The mixer: the sound unit's next sample, left and right, after its channels move on by one mixer period
void snd7_hw_mix(s16 out[2]);

// A channel's registers and what it plays now, for tests and the trace: the sample's index in the sound and its value,
// and the mixer sample (counted from the unit's reset) whose mixing took the channel's last start first
typedef struct Snd7HwPeek {
    u32 cnt;
    const u8 *sad;
    u16 tmr;
    u16 pnt;
    u32 len;
    u32 index;
    s32 sample;
    u32 startMix;
} Snd7HwPeek;

void snd7_hw_peek(int chNo, Snd7HwPeek *peek);
int snd7_hw_master_volume(void);

// The IMA-ADPCM decoder's step, as the sound unit does it (GBATEK): the next value and table index after a nibble
void snd7_adpcm_step(int nibble, s32 *value, s32 *index);

// snd_bank_host.c: the 32-bit handles that the host's banks and wave archives keep in place of ARM9 addresses (their
// links, and the addresses of waves loaded one by one), which are what the ARM7 finds there
#define SND7_HANDLE_FLAG 0x80000000u
u32 snd7_handle(const void *pointer);
void *snd7_handle_pointer(u32 handle);

// snd7.c: an address the ARM9 put in 32 bits (a handle, or the low bits of a pointer into its static data or main
// memory arena) as a host pointer, with the end of the memory it is in (NULL when unknown); NULL when there is none.
// snd7_memory_end gives the end of the memory a host pointer is in, NULL when unknown
const u8 *snd7_resolve(u32 address, const u8 **end);
const u8 *snd7_memory_end(const void *pointer);
// The little-endian words of data the ARM9 loaded, read only inside [start, end) (end NULL: no bound). FALSE outside
BOOL snd7_read_bytes(const u8 *start, const u8 *end, u32 offset, void *dst, u32 size);
u32 snd7_read32(const u8 *start, const u8 *end, u32 offset, BOOL *ok);

#endif // PORT_HAL_SND7_SND7_INTERNAL_H
