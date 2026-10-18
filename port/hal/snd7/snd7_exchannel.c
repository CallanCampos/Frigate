// The driver's channels (NitroSDK's snd_exchannel.c and snd_util.c on the ARM7, 0x03800378-0x03801558): what the
// sequencer and the ARM9 play on the sound unit's 16 channels, the game's music and sound effects through the
// sequencer, its streams and cries from the ARM9's own commands. A channel is allocated by priority, plays a PCM wave,
// a PSG square wave or noise, has an envelope (attack, decay, sustain, release), a sweep and an LFO, and takes its
// volume, pitch and pan from them and from its track at each update. SND_UpdateExChannel writes what changed to the
// sound unit's registers at the start of the next update.
#include "hal/snd7/snd7_internal.h"

// The order in which SND_AllocExChannel looks at the channels (0x03808f4c), and the data shifts by which it compares
// two channels' loudness (0x03808f48)
static const u8 sChannelOrder[SND7_CHANNEL_NUM] = { 4, 5, 6, 7, 2, 0, 3, 1, 8, 9, 10, 11, 14, 12, 15, 13 };
static const u8 sVolumeShift[4] = { 0, 1, 2, 4 };

// The attack rates of the attacks from 109 to 127 (0x03808f5c), from the fastest; below 109 it is 255 - attack
static const u8 sAttackTable[19] = { 0, 1, 5, 14, 26, 38, 51, 63, 73, 84, 92, 100, 109, 116, 123, 127, 132, 137, 143 };

// A quarter of a sine, 0 to 127 (0x03808e24)
static const s8 sSinTable[33] = { 0,  6,  12,  19,  25,  31,  37,  43,  49,  54,  60,  65,  71,  76,  81,  85, 90,
                                  94, 98, 102, 106, 109, 112, 115, 117, 120, 122, 123, 125, 126, 126, 127, 127 };

// The channel locks (0x03809f98 and 0x03809f94): a channel in sLockChannel is the ARM9's alone, one in
// sWeakLockChannel can still be allocated by a strong request (a track that has its own allocatable channels)
static u32 sLockChannel;
static u32 sWeakLockChannel;

// SND_CalcRandom's state (0x03809360)
static u32 sRandomSeed = 0x12345678;

Snd7Work snd7_work;

// snd_util.c

// SND_CalcTimer (0x03800378): a channel's timer for a pitch in 64ths of a semitone. The BIOS's pitch table gives the
// fraction of an octave, and the octaves are shifts
u16 snd7_calc_timer(int timer, int pitch) {
    int octave = 0;
    int pitchIndex = -pitch;
    u64 result;
    int shift;

    while (pitchIndex < 0) {
        octave--;
        pitchIndex += 768;
    }
    while (pitchIndex >= 768) {
        octave++;
        pitchIndex -= 768;
    }
    result = (u64)((s64)timer * (s64)(snd7_pitch_table[pitchIndex] + 0x10000));
    shift = octave - 16;
    if (shift <= 0) {
        shift = -shift;
        result = shift < 64 ? result >> shift : 0;
    } else if (shift < 32) {
        // The bits that the shift would take past 32
        if (result & (~0ull << (32 - shift))) {
            return 0xffff;
        }
        result <<= shift;
    } else {
        return 0xffff;
    }
    if (result < 0x10) {
        result = 0x10;
    } else if (result > 0xffff) {
        result = 0xffff;
    }
    return (u16)result;
}

// SND_SinIdx (0x0380051c): the sine of index / 128 of a turn
int snd7_sin_idx(int index) {
    if (index < 32) {
        return sSinTable[index];
    }
    if (index < 64) {
        return sSinTable[64 - index];
    }
    if (index < 96) {
        return -sSinTable[index - 64];
    }
    return -sSinTable[128 - index];
}

// SND_CalcRandom (0x03800574): a linear congruential generator, its high half
u16 snd7_calc_random(void) {
    sRandomSeed = sRandomSeed * 1664525 + 1013904223;
    return (u16)(sRandomSeed >> 16);
}

u32 snd7_random_state(void) {
    return sRandomSeed;
}

void snd7_set_random(u32 seed) {
    sRandomSeed = seed;
}

// The LFO (0x038013f4-0x038014d0)

// SND_InitLfoParam: no depth, a range of 1, a speed of 16, no delay, on the pitch
void snd7_init_lfo_param(Snd7LfoParam *param) {
    param->target = SND7_LFO_PITCH;
    param->depth = 0;
    param->range = 1;
    param->speed = 16;
    param->delay = 0;
}

// SND_UpdateLfo: after the delay, the counter's high byte turns through 128 steps of the sine
static void UpdateLfo(Snd7Lfo *lfo) {
    u32 step;
    u32 position;

    if (lfo->delayCounter < lfo->param.delay) {
        lfo->delayCounter++;
        return;
    }
    step = (u32)lfo->param.speed << 6;
    position = (lfo->counter + step) >> 8;
    while (position >= 128) {
        position -= 128;
    }
    lfo->counter = (u16)(lfo->counter + (u16)step);
    lfo->counter &= 0xff;
    lfo->counter |= (u16)((position & 0xff) << 8);
}

// SND_GetLfoValue
static s32 GetLfoValue(const Snd7Lfo *lfo) {
    if (lfo->param.depth == 0) {
        return 0;
    }
    if (lfo->delayCounter < lfo->param.delay) {
        return 0;
    }
    return snd7_sin_idx((lfo->counter >> 8) & 0x7f) * lfo->param.depth * lfo->param.range;
}

// The decay and release rates of the values 0 to 127 (0x038014d4)
static u16 CalcDecayRate(int value) {
    if (value == 127) {
        return 0xffff;
    }
    if (value == 126) {
        return 0x3c00;
    }
    if (value < 50) {
        return (u16)(value * 2 + 1);
    }
    return (u16)(0x1e00 / (126 - value));
}

// snd_exchannel.c

// SND_ExChannelInit (0x0380081c)
void snd7_exchannel_init(void) {
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        Snd7ExChannel *channel = &snd7_work.channel[i];

        channel->myNo = (u8)i;
        channel->flags &= SND7_EXCHANNEL_START | SND7_EXCHANNEL_AUTO_SWEEP;
    }
    sWeakLockChannel = 0;
    sLockChannel = 0;
}

// SND_UpdateExChannel (0x03800870): the register writes that the last update owes the channels. The channels that
// start are set up first and started together after
void snd7_update_exchannel(void) {
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        Snd7ExChannel *channel = &snd7_work.channel[i];
        u32 sync = snd7_sync(channel);

        if (sync == 0) {
            continue;
        }
        if (sync & SND7_SYNC_STOP) {
            snd7_stop_channel(i, 0);
        }
        if (sync & SND7_SYNC_START) {
            switch (channel->type) {
            case SND7_EXCHANNEL_PCM:
                snd7_setup_channel_pcm(i, channel->data, channel->dataEnd, channel->wave.format,
                                       channel->wave.loopFlag ? SND7_REPEAT_LOOP : SND7_REPEAT_ONESHOT,
                                       channel->wave.loopStart, (int)channel->wave.loopLen, channel->volume & 0xff,
                                       channel->volume >> 8, channel->timer, channel->pan);
                break;
            case SND7_EXCHANNEL_PSG:
                snd7_setup_channel_psg(i, (int)channel->duty, channel->volume & 0xff, channel->volume >> 8,
                                       channel->timer, channel->pan);
                break;
            case SND7_EXCHANNEL_NOISE:
                snd7_setup_channel_noise(i, channel->volume & 0xff, channel->volume >> 8, channel->timer, channel->pan);
                break;
            }
            continue;
        }
        if (sync & SND7_SYNC_TIMER) {
            snd7_set_channel_timer(i, channel->timer);
        }
        if (sync & SND7_SYNC_VOLUME) {
            snd7_set_channel_volume(i, channel->volume & 0xff, channel->volume >> 8);
        }
        if (sync & SND7_SYNC_PAN) {
            snd7_set_channel_pan(i, channel->pan);
        }
    }
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        Snd7ExChannel *channel = &snd7_work.channel[i];
        u32 sync = snd7_sync(channel);

        if (sync == 0) {
            continue;
        }
        if (sync & SND7_SYNC_START) {
            snd7_start_channel(i);
        }
        snd7_set_sync(channel, 0);
    }
}

// SND_UpdateExChannelEnvelope (0x03800ea0): at a periodic update the envelope moves on; its level in decibels
static s32 UpdateEnvelope(Snd7ExChannel *channel, BOOL doPeriodicProc) {
    if (doPeriodicProc) {
        switch (channel->envStatus) {
        case SND7_ENV_ATTACK:
            channel->envDecay = -((-channel->envDecay * channel->attack) >> 8);
            if (channel->envDecay == 0) {
                channel->envStatus = SND7_ENV_DECAY;
            }
            break;
        case SND7_ENV_DECAY: {
            s32 sustain = snd7_decibel_square_table[channel->sustain] << 7;

            channel->envDecay -= channel->decay;
            if (channel->envDecay <= sustain) {
                channel->envDecay = sustain;
                channel->envStatus = SND7_ENV_SUSTAIN;
            }
            break;
        }
        case SND7_ENV_SUSTAIN:
            break;
        case SND7_ENV_RELEASE:
            channel->envDecay -= channel->release;
            break;
        }
    }
    return channel->envDecay >> 7;
}

// The channel's callback: a track's, or none, in which case the channel goes back to the lowest priority
static void Finish(Snd7ExChannel *channel) {
    if (channel->callback != NULL) {
        channel->callback(channel, SND7_EXCHANNEL_CALLBACK_FINISH, channel->callbackData);
    } else {
        channel->prio = 0;
    }
    channel->volume = 0;
    channel->flags &= ~SND7_EXCHANNEL_ACTIVE;
}

// SND_ExChannelMain (0x03800a5c): each playing channel's volume, timer and pan from its velocity, key, envelope,
// sweep, LFO and track; what changed is written at the next SND_UpdateExChannel. A channel whose sound ended, or whose
// release fell silent, ends
void snd7_exchannel_main(BOOL doPeriodicProc) {
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        Snd7ExChannel *channel = &snd7_work.channel[i];
        s32 volume = 0;
        s32 pitch = 0;
        s32 pan = 0;
        s32 sweep;
        s32 lfo;
        u16 timer;
        u16 channelVolume;

        if (!(channel->flags & SND7_EXCHANNEL_ACTIVE)) {
            continue;
        }
        if (channel->flags & SND7_EXCHANNEL_START) {
            snd7_set_sync(channel, snd7_sync(channel) | SND7_SYNC_START);
            channel->flags &= ~SND7_EXCHANNEL_START;
        } else if (!snd7_is_channel_active(i)) {
            Finish(channel);
            continue;
        }

        volume += snd7_decibel_square_table[channel->velocity];
        pitch += (channel->key - channel->originalKey) * 64;
        volume += UpdateEnvelope(channel, doPeriodicProc);

        // The sweep: from its pitch at the start to none at its length
        if (channel->sweepPitch == 0 || channel->sweepCounter >= channel->sweepLength) {
            sweep = 0;
        } else {
            sweep = (s32)((s64)channel->sweepPitch * ((s64)channel->sweepLength - channel->sweepCounter) /
                          channel->sweepLength);
            if (doPeriodicProc && (channel->flags & SND7_EXCHANNEL_AUTO_SWEEP)) {
                channel->sweepCounter++;
            }
        }
        pitch += sweep;
        volume += channel->userDecay + channel->userDecay2;
        pitch += channel->userPitch;

        // The LFO: 64ths of a semitone for the pitch and the pan, 60ths of a decibel for the volume, at 14 bits of
        // fraction
        lfo = GetLfoValue(&channel->lfo);
        if (lfo != 0) {
            s64 value = lfo;

            switch (channel->lfo.param.target) {
            case SND7_LFO_VOLUME:
                value *= 60;
                break;
            case SND7_LFO_PITCH:
            case SND7_LFO_PAN:
                value *= 64;
                break;
            }
            lfo = (s32)(value >> 14);
        }
        if (doPeriodicProc) {
            UpdateLfo(&channel->lfo);
        }
        switch (channel->lfo.param.target) {
        case SND7_LFO_PITCH:
            pitch += lfo;
            break;
        case SND7_LFO_VOLUME:
            if (volume > -0x8000) {
                volume += lfo;
            }
            break;
        case SND7_LFO_PAN:
            pan += lfo;
            break;
        }

        pan += channel->initPan;
        if (channel->panRange != 127) {
            pan = (pan * channel->panRange + 64) >> 7;
        }
        pan += channel->userPan;

        if (channel->envStatus == SND7_ENV_RELEASE && volume <= SND7_VOLUME_DB_MIN) {
            snd7_set_sync(channel, SND7_SYNC_STOP);
            Finish(channel);
            continue;
        }

        channelVolume = SND_CalcChannelVolume(volume);
        timer = snd7_calc_timer(channel->wave.timer, pitch);
        if (channel->type == SND7_EXCHANNEL_PSG) {
            timer &= 0xfffc;
        }
        pan += 64;
        if (pan < 0) {
            pan = 0;
        } else if (pan > 127) {
            pan = 127;
        }
        if (channelVolume != channel->volume) {
            channel->volume = channelVolume;
            snd7_set_sync(channel, snd7_sync(channel) | SND7_SYNC_VOLUME);
        }
        if (timer != channel->timer) {
            channel->timer = timer;
            snd7_set_sync(channel, snd7_sync(channel) | SND7_SYNC_TIMER);
        }
        if (pan != channel->pan) {
            channel->pan = (u8)pan;
            snd7_set_sync(channel, snd7_sync(channel) | SND7_SYNC_PAN);
        }
    }
}

// SND_ExChannelStart (0x03801528): the envelope from silence, the LFO from its start
static void ExChannelStart(Snd7ExChannel *channel, s32 length) {
    channel->envDecay = SND7_VOLUME_DB_MIN * 128;
    channel->envStatus = SND7_ENV_ATTACK;
    channel->length = length;
    channel->lfo.counter = 0;
    channel->lfo.delayCounter = 0;
    channel->flags |= SND7_EXCHANNEL_START | SND7_EXCHANNEL_ACTIVE;
}

// SND_StartExChannelPcm (0x03800dd4)
BOOL snd7_start_exchannel_pcm(Snd7ExChannel *channel, const Snd7WaveParam *wave, const u8 *data, const u8 *dataEnd,
                              s32 length) {
    channel->type = SND7_EXCHANNEL_PCM;
    channel->wave = *wave;
    channel->data = data;
    channel->dataEnd = dataEnd;
    ExChannelStart(channel, length);
    return TRUE;
}

// SND_StartExChannelPsg (0x03800e10): only channels 8 to 13 play square waves; the timer is that of key 60, C4: 8 steps
// a period at 261.6 Hz
BOOL snd7_start_exchannel_psg(Snd7ExChannel *channel, int duty, s32 length) {
    if (channel->myNo < 8 || channel->myNo > 13) {
        return FALSE;
    }
    channel->duty = (u32)duty;
    channel->type = SND7_EXCHANNEL_PSG;
    channel->wave.timer = 0x1f46;
    ExChannelStart(channel, length);
    return TRUE;
}

// SND_StartExChannelNoise (0x03800e5c): only channels 14 and 15 play noise
BOOL snd7_start_exchannel_noise(Snd7ExChannel *channel, s32 length) {
    if (channel->myNo < 14 || channel->myNo > 15) {
        return FALSE;
    }
    channel->type = SND7_EXCHANNEL_NOISE;
    channel->wave.timer = 0x1f46;
    ExChannelStart(channel, length);
    return TRUE;
}

// SND_SetExChannelAttack (0x03800f48)
void snd7_set_exchannel_attack(Snd7ExChannel *channel, int attack) {
    if (attack < 109) {
        channel->attack = (u8)(255 - attack);
    } else {
        channel->attack = sAttackTable[127 - attack];
    }
}

// SND_SetExChannelDecay (0x03800f68)
void snd7_set_exchannel_decay(Snd7ExChannel *channel, int decay) {
    channel->decay = CalcDecayRate(decay);
}

// SND_SetExChannelSustain (0x03800f84)
void snd7_set_exchannel_sustain(Snd7ExChannel *channel, int sustain) {
    channel->sustain = (u8)sustain;
}

// SND_SetExChannelRelease (0x03800f8c)
void snd7_set_exchannel_release(Snd7ExChannel *channel, int release) {
    channel->release = CalcDecayRate(release);
}

// SND_ReleaseExChannel (0x03800fa8)
void snd7_release_exchannel(Snd7ExChannel *channel) {
    channel->envStatus = SND7_ENV_RELEASE;
}

// SND_IsExChannelActive (0x03800fb4)
BOOL snd7_is_exchannel_active(const Snd7ExChannel *channel) {
    return (channel->flags & SND7_EXCHANNEL_ACTIVE) != 0;
}

// The loudness by which SND_AllocExChannel compares channels of the same priority: the channel volume without its
// data shift
static int Loudness(const Snd7ExChannel *channel) {
    return ((channel->volume & 0xff) << 4) >> sVolumeShift[(channel->volume >> 8) & 3];
}

// SND_AllocExChannel (0x03800fc4): of the channels the mask allows, in sChannelOrder's order, the one of the lowest
// priority, and of those the quietest. The request fails when that one's priority is above prio; else whatever it
// played stops, its callback is told, and it is set up afresh for the caller
Snd7ExChannel *snd7_alloc_exchannel(u32 chBitMask, int prio, BOOL strongRequest, Snd7ExChannelCallback callback,
                                    void *callbackData) {
    Snd7ExChannel *best = NULL;
    int i;

    chBitMask &= ~sLockChannel;
    if (!strongRequest) {
        chBitMask &= ~sWeakLockChannel;
    }
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        int chNo = sChannelOrder[i];
        Snd7ExChannel *channel;

        if (!(chBitMask & (1u << chNo))) {
            continue;
        }
        channel = &snd7_work.channel[chNo];
        if (best == NULL) {
            best = channel;
            continue;
        }
        if (channel->prio > best->prio) {
            continue;
        }
        if (channel->prio == best->prio && Loudness(best) <= Loudness(channel)) {
            continue;
        }
        best = channel;
    }
    if (best == NULL || prio < best->prio) {
        return NULL;
    }
    if (best->callback != NULL) {
        best->callback(best, SND7_EXCHANNEL_CALLBACK_DROP, best->callbackData);
    }
    snd7_set_sync(best, SND7_SYNC_STOP);
    best->flags &= ~SND7_EXCHANNEL_ACTIVE;
    best->nextLink = NULL;
    best->callback = callback;
    best->callbackData = callbackData;
    best->length = 0;
    best->prio = (u8)prio;
    best->volume = 127;
    best->flags &= ~SND7_EXCHANNEL_START;
    best->flags |= SND7_EXCHANNEL_AUTO_SWEEP;
    best->key = 60;
    best->originalKey = 60;
    best->velocity = 127;
    best->initPan = 0;
    best->userDecay = 0;
    best->userDecay2 = 0;
    best->userPitch = 0;
    best->userPan = 0;
    best->panRange = 127;
    best->sweepPitch = 0;
    best->sweepLength = 0;
    best->sweepCounter = 0;
    best->attack = 0;
    best->decay = 0xffff;
    best->sustain = 127;
    best->release = 0xffff;
    snd7_init_lfo_param(&best->lfo.param);
    return best;
}

// SND_FreeExChannel (0x03801190): the channel keeps playing, but no longer tells anyone
void snd7_free_exchannel(Snd7ExChannel *channel) {
    if (channel != NULL) {
        channel->callback = NULL;
        channel->callbackData = NULL;
    }
}

// What SND_StopUnlockedChannel and SND_LockChannel do to a channel: it stops at once and is free
static void DropChannel(int chNo) {
    Snd7ExChannel *channel = &snd7_work.channel[chNo];

    if (channel->callback != NULL) {
        channel->callback(channel, SND7_EXCHANNEL_CALLBACK_DROP, channel->callbackData);
    }
    snd7_stop_channel(chNo, 0);
    channel->prio = 0;
    snd7_free_exchannel(channel);
    snd7_set_sync(channel, 0);
    channel->flags &= ~SND7_EXCHANNEL_ACTIVE;
}

// SND_StopUnlockedChannel (0x038011a4)
void snd7_stop_unlocked_channel(u32 chBitMask, u32 flags) {
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM && chBitMask != 0; i++, chBitMask >>= 1) {
        if ((chBitMask & 1) && !(sLockChannel & (1u << i))) {
            DropChannel(i);
        }
    }
}

// SND_LockChannel (0x03801254): the channels stop and become the ARM9's. SND_LOCK_IMPLIED_ALLOC_CHANNEL (flag 1)
// leaves them to strong requests
void snd7_lock_channel(u32 chBitMask, u32 flags) {
    u32 mask = chBitMask;
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM && mask != 0; i++, mask >>= 1) {
        if ((mask & 1) && !(sLockChannel & (1u << i))) {
            DropChannel(i);
        }
    }
    if (flags & 1) {
        sWeakLockChannel |= chBitMask;
    } else {
        sLockChannel |= chBitMask;
    }
}

// SND_UnlockChannel (0x03801328)
void snd7_unlock_channel(u32 chBitMask, u32 flags) {
    if (flags & 1) {
        sWeakLockChannel &= ~chBitMask;
    } else {
        sLockChannel &= ~chBitMask;
    }
}

// SND_GetLockedChannel (0x0380135c)
u32 snd7_get_locked_channel(u32 flags) {
    return flags & 1 ? sWeakLockChannel : sLockChannel;
}

// SND_InvalidateWave (0x03801378): the PCM channels that play from the range stop; they end at the next update
void snd7_invalidate_wave(const u8 *start, const u8 *end) {
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        Snd7ExChannel *channel = &snd7_work.channel[i];

        if ((channel->flags & SND7_EXCHANNEL_ACTIVE) && channel->type == SND7_EXCHANNEL_PCM &&
            (uintptr_t)start <= (uintptr_t)channel->data && (uintptr_t)channel->data <= (uintptr_t)end) {
            channel->flags &= ~SND7_EXCHANNEL_START;
            snd7_stop_channel(i, 0);
        }
    }
}
