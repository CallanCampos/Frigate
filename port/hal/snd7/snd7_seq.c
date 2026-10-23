// The sequencer (NitroSDK's snd_seq.c and the ARM7's side of snd_bank.c, 0x03801560-0x03803140): 16 players, each of
// up to 16 of the driver's 32 tracks, which read a sequence (SSEQ) and play its notes with the instruments of a bank
// (SBNK) on the driver's channels. A player moves on a tick each time its tempo, scaled by its tempo ratio, adds up to
// 240 at a periodic update; at each tick every track waits or reads commands until one makes it wait.
//
// The commands (a byte; with 0x80 clear, a note of that key):
//   note: velocity, length (variable-length)        0x80 wait (var.)        0x81 program (var.)
//   0x93 open track: number, offset (3 bytes)       0x94 jump (3)           0x95 call (3)
//   0xa0 a random argument (two s16) for the next command, 0xa1 a variable's value, 0xa2 run the next command only
//   when the last comparison was true
//   0xb0-0xbd on a variable (number, s16): set, add, subtract, multiply, divide, shift, random, (0xb7 nothing), and
//   the comparisons ==, >=, >, <=, <, !=
//   0xc0-0xd7 (one byte): pan, volume, player volume, transpose, pitch bend, bend range, priority, note wait, tie,
//   portamento from a key, modulation depth, speed, type and range, portamento on, portamento time, attack, decay,
//   sustain, release, loop start (count), expression (volume2), print variable (a debug no-op), mute
//   0xe0 (s16) modulation delay, 0xe1 tempo, 0xe3 sweep pitch; 0xfc loop end, 0xfd return, 0xfe the tracks a
//   sequence allocates (read where it starts), 0xff end
// A track whose arguments come from 0xa0 or 0xa1 reads them in that form instead of its own.
//
// The host reads a sequence and its bank only inside the memory they are in (the ARM9's noted pointers, snd7.c): a
// track that reads past its sequence's memory ends there, and an instrument outside its bank isn't found. The DS reads
// whatever is there. The DS also reads a sequence through a cache of 16 bytes, refilled at each SND_PrepareSeq, which
// gives the same bytes unless the ARM9 changes a sequence while it plays; the host reads the memory.
#include "hal/pxi/pxi_bus.h"
#include "hal/snd7/snd7_internal.h"
#include <stdio.h>
#include <string.h>

// How a command's argument is read: its own type, or as a 0xa0 or 0xa1 before it says
enum {
    ARG_U8,
    ARG_S16,
    ARG_VLV,
    ARG_RANDOM,
    ARG_VARIABLE,
};

// The host's bound on the commands a track reads in one tick without waiting: a sequence that never waits would stop
// the DS's sound thread; here the track ends
#define TRACK_COMMAND_BUDGET 65536

// SNDInstData as the bank keeps it: the type, then the wave (index and wave archive, or the PSG duty), the original
// key, attack, decay, sustain, release and pan
typedef struct SndInst {
    u8 type;
    u16 wave[2];
    u8 originalKey;
    u8 attack;
    u8 decay;
    u8 sustain;
    u8 release;
    u8 pan;
} SndInst;

#define INST_SIZE 12

static void ChannelCallback(Snd7ExChannel *channel, int status, void *data);

static Snd7Player *Player(int playerNo) {
    return &snd7_work.player[playerNo];
}

// The sequence's bytes

// The track's position, which is past its sequence's memory for a jump or a track that leads out of it
static const u8 *Address(const u8 *base, u32 offset) {
    return (const u8 *)((uintptr_t)base + offset);
}

// SeqReadByte (0x038018dc): the next byte of the track, or 0 past its memory, which ends the track
static u8 TrackReadU8(Snd7Track *track) {
    if (track->overrun || (uintptr_t)track->cur < (uintptr_t)track->base ||
        (uintptr_t)track->cur >= (uintptr_t)track->end) {
        track->overrun = TRUE;
        return 0;
    }
    return *track->cur++;
}

static u16 TrackReadU16(Snd7Track *track) {
    u32 low = TrackReadU8(track);

    return (u16)(low | (u32)TrackReadU8(track) << 8);
}

static u32 TrackReadU24(Snd7Track *track) {
    u32 value = TrackReadU8(track);

    value |= (u32)TrackReadU8(track) << 8;
    return value | (u32)TrackReadU8(track) << 16;
}

// The variables (0x03802d78): 0-15 the player's own, 16-31 the global ones, in the shared work. The DS reads past the
// global ones for a number of 32 or more; the host has none there
vs16 *snd7_variable(const Snd7Player *player, int varNo) {
    if (snd7_shared == NULL || varNo >= 2 * SND_PLAYER_VARIABLE_MAX) {
        return NULL;
    }
    if (varNo < SND_PLAYER_VARIABLE_MAX) {
        return &snd7_shared->player[player->myNo].variable[varNo];
    }
    return &snd7_shared->globalVariable[varNo - SND_PLAYER_VARIABLE_MAX];
}

// TrackGetArgument (0x03801e60). A variable that doesn't exist leaves the DS's register with what it held, which at
// every call is the argument type: 4
static s32 TrackGetArgument(Snd7Track *track, Snd7Player *player, int argType) {
    s32 value = 0;

    switch (argType) {
    case ARG_U8:
        value = TrackReadU8(track);
        break;
    case ARG_S16:
        value = TrackReadU16(track);
        break;
    case ARG_VLV: {
        u8 byte;

        do {
            byte = TrackReadU8(track);
            value = (s32)(((u32)value << 7) | (byte & 0x7f));
        } while (byte & 0x80);
        break;
    }
    case ARG_RANDOM: {
        s32 low = (s16)TrackReadU16(track);
        s32 high = (s16)TrackReadU16(track);
        u16 random = snd7_calc_random();

        value = ((s32)((u32)random * (u32)(high - low + 1)) >> 16) + low;
        break;
    }
    case ARG_VARIABLE: {
        vs16 *variable = snd7_variable(player, TrackReadU8(track));

        value = variable != NULL ? *variable : ARG_VARIABLE;
        break;
    }
    }
    return value;
}

// The tracks

// TrackInit (0x03801f18): every parameter at its default; the active bit is the caller's
static void TrackInit(Snd7Track *track) {
    track->flags = (u8)((track->flags & SND7_TRACK_ACTIVE) | SND7_TRACK_NOTE_WAIT | SND7_TRACK_CMP);
    track->base = NULL;
    track->cur = NULL;
    track->end = NULL;
    track->overrun = FALSE;
    track->callStackDepth = 0;
    track->prgNo = 0;
    track->prio = 64;
    track->volume = 127;
    track->volume2 = 127;
    track->extFader = 0;
    track->pan = 0;
    track->extPan = 0;
    track->pitchBend = 0;
    track->extPitch = 0;
    track->attack = 0xff;
    track->decay = 0xff;
    track->sustain = 0xff;
    track->release = 0xff;
    track->panRange = 127;
    track->bendRange = 2;
    track->portaKey = 60;
    track->portaTime = 0;
    track->sweepPitch = 0;
    track->transpose = 0;
    track->channelMask = 0xffff;
    snd7_init_lfo_param(&track->mod);
    track->wait = 0;
    track->channelList = NULL;
}

// TrackStart (0x03801fe4). A sequence whose memory's end isn't known is read without a bound; one the host couldn't
// find has nothing to read
static void TrackStart(Snd7Track *track, const u8 *base, const u8 *end, u32 offset) {
    track->base = base;
    if (base == NULL) {
        track->end = NULL;
    } else {
        track->end = end != NULL ? end : (const u8 *)UINTPTR_MAX;
    }
    track->cur = Address(base, offset);
    track->overrun = FALSE;
}

// AllocTrack (0x03802db8): the first free track, or -1
static int AllocTrack(void) {
    int i;

    for (i = 0; i < SND7_TRACK_NUM; i++) {
        Snd7Track *track = &snd7_work.track[i];

        if (!(track->flags & SND7_TRACK_ACTIVE)) {
            track->flags |= SND7_TRACK_ACTIVE;
            return i;
        }
    }
    return -1;
}

// PlayerGetTrack (0x03802090)
static Snd7Track *PlayerGetTrack(Snd7Player *player, int trackIdx) {
    if (trackIdx > SND7_PLAYER_TRACK_NUM - 1 || player->tracks[trackIdx] == SND7_TRACK_NONE) {
        return NULL;
    }
    return &snd7_work.track[player->tracks[trackIdx]];
}

// TrackUpdateChannel (0x038021ac): the track's and the player's volumes, fader, pitch, pan and modulation to each of
// the track's channels that isn't releasing; with release, a channel whose note has no length left releases
static void TrackUpdateChannel(Snd7Track *track, Snd7Player *player, BOOL release) {
    s32 volume = snd7_decibel_square_table[track->volume] + snd7_decibel_square_table[track->volume2] +
                 snd7_decibel_square_table[player->volume];
    s32 fader = track->extFader + player->extFader;
    s32 pitch = ((track->pitchBend * (track->bendRange << 6)) >> 7) + track->extPitch;
    s32 pan = track->pan;
    Snd7ExChannel *channel;

    if (track->panRange != 127) {
        pan = (pan * track->panRange + 64) >> 7;
    }
    pan += track->extPan;
    if (volume < -0x8000) {
        volume = -0x8000;
    }
    if (fader < -0x8000) {
        fader = -0x8000;
    }
    if (pan < -128) {
        pan = -128;
    } else if (pan > 127) {
        pan = 127;
    }
    for (channel = track->channelList; channel != NULL; channel = channel->nextLink) {
        channel->userDecay2 = (s16)fader;
        if (channel->envStatus == SND7_ENV_RELEASE) {
            continue;
        }
        channel->userDecay = (s16)volume;
        channel->userPitch = (s16)pitch;
        channel->userPan = (s8)pan;
        channel->panRange = track->panRange;
        channel->lfo.param = track->mod;
        if (channel->length == 0 && release) {
            channel->prio = 1;
            snd7_release_exchannel(channel);
        }
    }
}

// TrackReleaseChannels (0x03801ff4): the track's channels release, at the rate release unless it is negative, with
// the lowest priority
static void TrackReleaseChannels(Snd7Track *track, Snd7Player *player, int release) {
    Snd7ExChannel *channel;

    TrackUpdateChannel(track, player, FALSE);
    for (channel = track->channelList; channel != NULL; channel = channel->nextLink) {
        if (snd7_is_exchannel_active(channel)) {
            if (release >= 0) {
                snd7_set_exchannel_release(channel, release & 0xff);
            }
            channel->prio = 1;
            snd7_release_exchannel(channel);
        }
    }
}

// TrackFreeChannels (0x0380205c): the channels play on, but are no longer the track's
static void TrackFreeChannels(Snd7Track *track) {
    Snd7ExChannel *channel;

    for (channel = track->channelList; channel != NULL; channel = channel->nextLink) {
        snd7_free_exchannel(channel);
    }
    track->channelList = NULL;
}

// TrackMute (0x03802df8): off, on (notes stop starting), on with the channels released, or on with them released at
// once and freed
static void TrackMute(Snd7Track *track, Snd7Player *player, int muteType) {
    switch (muteType) {
    case 0:
        track->flags &= ~SND7_TRACK_MUTE;
        break;
    case 1:
        track->flags |= SND7_TRACK_MUTE;
        break;
    case 2:
        track->flags |= SND7_TRACK_MUTE;
        TrackReleaseChannels(track, player, -1);
        break;
    case 3:
        track->flags |= SND7_TRACK_MUTE;
        TrackReleaseChannels(track, player, 127);
        TrackFreeChannels(track);
        break;
    }
}

// PlayerStopTrack (0x038020bc)
static void PlayerStopTrack(Snd7Player *player, int trackIdx) {
    Snd7Track *track = PlayerGetTrack(player, trackIdx);

    if (track == NULL) {
        return;
    }
    TrackReleaseChannels(track, player, -1);
    TrackFreeChannels(track);
    snd7_work.track[player->tracks[trackIdx]].flags &= ~SND7_TRACK_ACTIVE;
    player->tracks[trackIdx] = SND7_TRACK_NONE;
}

// PlayerStop (0x03802114)
static void PlayerStop(Snd7Player *player) {
    int i;

    for (i = 0; i < SND7_PLAYER_TRACK_NUM; i++) {
        PlayerStopTrack(player, i);
    }
    player->flags &= ~SND7_PLAYER_ACTIVE;
}

// The bank (the ARM7's side of snd_bank.c)

// SND_ReadInstData (0x03802e74): the instrument of a program for a key; a drum set's or key split's holds the
// instrument for the key. FALSE when there is none. The host has no instrument for a bank it can't find
static BOOL ReadInstData(const u8 *bank, const u8 *bankEnd, int prgNo, int key, SndInst *inst) {
    u8 bytes[INST_SIZE];
    BOOL ok = TRUE;
    u32 offset;
    u32 data;

    if (prgNo < 0 || bank == NULL) {
        return FALSE;
    }
    if ((u32)prgNo >= snd7_read32(bank, bankEnd, 0x38, &ok) || !ok) {
        return FALSE;
    }
    offset = snd7_read32(bank, bankEnd, 0x3c + 4 * (u32)prgNo, &ok);
    if (!ok) {
        return FALSE;
    }
    data = offset >> 8;
    bytes[0] = (u8)offset;
    switch (bytes[0]) {
    case SND_INST_PCM:
    case SND_INST_PSG:
    case SND_INST_NOISE:
    case SND_INST_DIRECTPCM:
    case SND_INST_NULL:
        if (!snd7_read_bytes(bank, bankEnd, data, &bytes[2], INST_SIZE - 2)) {
            return FALSE;
        }
        break;
    case SND_INST_DRUM_SET: {
        u8 range[2];

        if (!snd7_read_bytes(bank, bankEnd, data, range, 2) || key < range[0] || key > range[1] ||
            !snd7_read_bytes(bank, bankEnd, data + 2 + (u32)(key - range[0]) * INST_SIZE, bytes, INST_SIZE)) {
            return FALSE;
        }
        break;
    }
    case SND_INST_KEY_SPLIT: {
        u8 keys[8];
        int i;

        if (!snd7_read_bytes(bank, bankEnd, data, keys, 8)) {
            return FALSE;
        }
        for (i = 0; i < 8; i++) {
            if (key <= keys[i]) {
                break;
            }
        }
        if (i == 8 || !snd7_read_bytes(bank, bankEnd, data + 8 + (u32)i * INST_SIZE, bytes, INST_SIZE)) {
            return FALSE;
        }
        break;
    }
    default:
        return FALSE;
    }
    inst->type = bytes[0];
    inst->wave[0] = (u16)(bytes[2] | bytes[3] << 8);
    inst->wave[1] = (u16)(bytes[4] | bytes[5] << 8);
    inst->originalKey = bytes[6];
    inst->attack = bytes[7];
    inst->decay = bytes[8];
    inst->sustain = bytes[9];
    inst->release = bytes[10];
    inst->pan = bytes[11];
    return TRUE;
}

// SND_GetWaveDataAddress (0x03802fbc): an entry of a wave archive's table is an offset in the archive, an address of
// a wave loaded by itself (a handle of the host's SND_SetWaveDataAddress, snd_bank_host.c) or 0 for none
static const u8 *GetWaveDataAddress(const u8 *waveArc, const u8 *waveArcEnd, u32 index, const u8 **end) {
    BOOL ok = TRUE;
    u32 entry = snd7_read32(waveArc, waveArcEnd, 0x3c + 4 * index, &ok);

    *end = NULL;
    if (!ok || entry == 0) {
        return NULL;
    }
    if (entry < 0x02000000) {
        *end = waveArcEnd;
        return waveArc + entry;
    }
    return snd7_resolve(entry, end);
}

// SND_NoteOn (0x03802ffc): starts the instrument's wave, square wave or noise on the channel, with its envelope and
// pan
static BOOL NoteOn(Snd7ExChannel *channel, int key, int velocity, s32 length, const Snd7Player *player,
                   const SndInst *inst) {
    int release = inst->release;
    BOOL result;

    if (release == 0xff) {
        length = -1;
        release = 0;
    }
    switch (inst->type) {
    case SND_INST_PCM:
    case SND_INST_DIRECTPCM: {
        const u8 *wave = NULL;
        const u8 *waveEnd = NULL;
        Snd7WaveParam param;
        u8 header[SND7_WAVE_PARAM_SIZE];

        if (inst->type == SND_INST_PCM) {
            BOOL ok = TRUE;
            u32 link = snd7_read32(player->bank, player->bankEnd, 0x18 + 8 * (u32)inst->wave[1], &ok);
            const u8 *waveArcEnd = NULL;
            const u8 *waveArc = ok && link != 0 ? snd7_resolve(link, &waveArcEnd) : NULL;

            if (waveArc != NULL && inst->wave[0] < snd7_read32(waveArc, waveArcEnd, 0x38, &ok) && ok) {
                wave = GetWaveDataAddress(waveArc, waveArcEnd, inst->wave[0], &waveEnd);
            }
        } else {
            wave = snd7_resolve((u32)inst->wave[0] | (u32)inst->wave[1] << 16, &waveEnd);
        }
        if (wave == NULL || !snd7_read_bytes(wave, waveEnd, 0, header, sizeof(header))) {
            return FALSE;
        }
        param.format = header[0];
        param.loopFlag = header[1];
        param.rate = (u16)(header[2] | header[3] << 8);
        param.timer = (u16)(header[4] | header[5] << 8);
        param.loopStart = (u16)(header[6] | header[7] << 8);
        param.loopLen = (u32)header[8] | (u32)header[9] << 8 | (u32)header[10] << 16 | (u32)header[11] << 24;
        result = snd7_start_exchannel_pcm(channel, &param, wave + SND7_WAVE_PARAM_SIZE, waveEnd, length);
        break;
    }
    case SND_INST_PSG:
        result = snd7_start_exchannel_psg(channel, inst->wave[0], length);
        break;
    case SND_INST_NOISE:
        result = snd7_start_exchannel_noise(channel, length);
        break;
    default:
        result = FALSE;
        break;
    }
    if (!result) {
        return FALSE;
    }
    channel->key = (u8)key;
    channel->originalKey = inst->originalKey;
    channel->velocity = (u8)velocity;
    snd7_set_exchannel_attack(channel, inst->attack);
    snd7_set_exchannel_decay(channel, inst->decay);
    snd7_set_exchannel_sustain(channel, inst->sustain);
    snd7_set_exchannel_release(channel, release);
    channel->initPan = (s8)(inst->pan - 64);
    return TRUE;
}

// A note on the track (PlayerStepTick's note, 0x03802488-0x038026c4): unless the track is muted or the notes don't
// play (SND_SkipSeq), a channel of the instrument's kind that the track may use, with the track's envelope, sweep and
// portamento; with tie, the note the track already plays takes the new key and velocity. Then the note's wait
static void TrackNote(Snd7Track *track, Snd7Player *player, int key, int velocity, s32 length, BOOL playNote) {
    Snd7ExChannel *channel = NULL;

    if (key < 0) {
        key = 0;
    } else if (key > 127) {
        key = 127;
    }
    if (!(track->flags & SND7_TRACK_MUTE) && playNote) {
        if ((track->flags & SND7_TRACK_TIE) && track->channelList != NULL) {
            channel = track->channelList;
            channel->key = (u8)key;
            channel->velocity = (u8)velocity;
        }
        if (channel == NULL) {
            SndInst inst;
            u32 allowed;

            if (!ReadInstData(player->bank, player->bankEnd, track->prgNo, key, &inst)) {
                goto noteDone;
            }
            switch (inst.type) {
            case SND_INST_PCM:
            case SND_INST_DIRECTPCM:
                allowed = 0xffff;
                break;
            case SND_INST_PSG:
                allowed = 0x3f00;
                break;
            case SND_INST_NOISE:
                allowed = 0xc000;
                break;
            default:
                goto noteDone;
            }
            channel = snd7_alloc_exchannel(allowed & track->channelMask, player->prio + track->prio,
                                           (track->flags & SND7_TRACK_CHANNEL_MASK) != 0, ChannelCallback, track);
            if (channel == NULL) {
                goto noteDone;
            }
            if (!NoteOn(channel, key, velocity, (track->flags & SND7_TRACK_TIE) || length <= 0 ? -1 : length, player,
                        &inst)) {
                channel->prio = 0;
                snd7_free_exchannel(channel);
                goto noteDone;
            }
            channel->nextLink = track->channelList;
            track->channelList = channel;
        }
        if (track->attack != 0xff) {
            snd7_set_exchannel_attack(channel, track->attack);
        }
        if (track->decay != 0xff) {
            snd7_set_exchannel_decay(channel, track->decay);
        }
        if (track->sustain != 0xff) {
            snd7_set_exchannel_sustain(channel, track->sustain);
        }
        if (track->release != 0xff) {
            snd7_set_exchannel_release(channel, track->release);
        }
        channel->sweepPitch = track->sweepPitch;
        if (track->flags & SND7_TRACK_PORTA) {
            channel->sweepPitch = (s16)(channel->sweepPitch + (s16)((u32)(track->portaKey - key) << 6));
        }
        if (track->portaTime == 0) {
            channel->sweepLength = length > 0 ? length : -1;
            channel->flags &= ~SND7_EXCHANNEL_AUTO_SWEEP;
        } else {
            s32 time = track->portaTime * track->portaTime;
            s32 sweep = channel->sweepPitch < 0 ? -channel->sweepPitch : channel->sweepPitch;

            channel->sweepLength = (s32)((u32)time * (u32)sweep) >> 11;
        }
        channel->sweepCounter = 0;
    }
noteDone:
    track->portaKey = (u8)key;
    if (track->flags & SND7_TRACK_NOTE_WAIT) {
        track->wait = length;
        if (length == 0) {
            track->flags |= SND7_TRACK_NOTE_FINISH_WAIT;
        }
    }
}

// A variable's shift (0xb5): to the left by a positive amount, to the right by a negative one, as the ARM's shifts by a
// register do it, which take the amount's low byte and give 0 to the left and the sign to the right from 32 on
static s32 ShiftVariable(s32 value, s32 amount) {
    u32 count;

    if (amount >= 0) {
        count = (u32)amount & 0xff;
        return count >= 32 ? 0 : (s32)((u32)value << count);
    }
    count = (0u - (u32)amount) & 0xff;
    if (count >= 32) {
        return value < 0 ? -1 : 0;
    }
    return value >> count;
}

// The commands 0xb0-0xbd on a variable
static void VariableCommand(Snd7Track *track, vs16 *variable, int command, s32 argument) {
    s32 value = *variable;
    BOOL compare;

    switch (command) {
    case 0xb0:
        *variable = (s16)argument;
        return;
    case 0xb1:
        *variable = (s16)(value + argument);
        return;
    case 0xb2:
        *variable = (s16)(value - argument);
        return;
    case 0xb3:
        *variable = (s16)(value * argument);
        return;
    case 0xb4:
        if (argument != 0) {
            *variable = (s16)(value / argument);
        }
        return;
    case 0xb5:
        *variable = (s16)ShiftVariable(value, argument);
        return;
    case 0xb6: {
        BOOL negative = FALSE;
        s32 random;

        if (argument < 0) {
            argument = (s16)-argument;
            negative = TRUE;
        }
        random = (s32)snd7_calc_random() * (argument + 1) >> 16;
        *variable = (s16)(negative ? -random : random);
        return;
    }
    case 0xb8:
        compare = value == argument;
        break;
    case 0xb9:
        compare = value >= argument;
        break;
    case 0xba:
        compare = value > argument;
        break;
    case 0xbb:
        compare = value <= argument;
        break;
    case 0xbc:
        compare = value < argument;
        break;
    case 0xbd:
        compare = value != argument;
        break;
    default:
        return;
    }
    if (compare) {
        track->flags |= SND7_TRACK_CMP;
    } else {
        track->flags &= ~SND7_TRACK_CMP;
    }
}

// The commands 0xc0-0xd7, of one byte
static void ByteCommand(Snd7Track *track, Snd7Player *player, int command, u8 argument) {
    switch (command) {
    case 0xc0:
        track->pan = (s8)(argument - 64);
        break;
    case 0xc1:
        track->volume = argument;
        break;
    case 0xc2:
        player->volume = argument;
        break;
    case 0xc3:
        track->transpose = (s8)argument;
        break;
    case 0xc4:
        track->pitchBend = (s8)argument;
        break;
    case 0xc5:
        track->bendRange = argument;
        break;
    case 0xc6:
        track->prio = argument;
        break;
    case 0xc7:
        track->flags = (u8)((track->flags & ~SND7_TRACK_NOTE_WAIT) | (argument & 1 ? SND7_TRACK_NOTE_WAIT : 0));
        break;
    case 0xc8:
        track->flags = (u8)((track->flags & ~SND7_TRACK_TIE) | (argument & 1 ? SND7_TRACK_TIE : 0));
        TrackReleaseChannels(track, player, -1);
        TrackFreeChannels(track);
        break;
    case 0xc9:
        track->portaKey = (u8)(argument + track->transpose);
        track->flags |= SND7_TRACK_PORTA;
        break;
    case 0xca:
        track->mod.depth = argument;
        break;
    case 0xcb:
        track->mod.speed = argument;
        break;
    case 0xcc:
        track->mod.target = argument;
        break;
    case 0xcd:
        track->mod.range = argument;
        break;
    case 0xce:
        track->flags = (u8)((track->flags & ~SND7_TRACK_PORTA) | (argument & 1 ? SND7_TRACK_PORTA : 0));
        break;
    case 0xcf:
        track->portaTime = argument;
        break;
    case 0xd0:
        track->attack = argument;
        break;
    case 0xd1:
        track->decay = argument;
        break;
    case 0xd2:
        track->sustain = argument;
        break;
    case 0xd3:
        track->release = argument;
        break;
    case 0xd4:
        if (track->callStackDepth < SND7_CALL_STACK_DEPTH) {
            track->callStack[track->callStackDepth] = track->cur;
            track->loopCount[track->callStackDepth] = argument;
            track->callStackDepth++;
        }
        break;
    case 0xd5:
        track->volume2 = argument;
        break;
    case 0xd6:
        // Prints a variable when the driver's debug output is on, which it never is
        break;
    case 0xd7:
        TrackMute(track, player, argument);
        break;
    }
}

// One tick of a track (in PlayerStepTick, 0x0380233c-0x03802d24): its channels' note lengths and manual sweeps move
// on; it waits for its notes to end (note-finish wait) or its wait to run out, then reads commands until one makes it
// wait. -1 when it ended
static int TrackStepTick(Snd7Track *track, Snd7Player *player, BOOL playNote) {
    Snd7ExChannel *channel;
    int budget = TRACK_COMMAND_BUDGET;

    for (channel = track->channelList; channel != NULL; channel = channel->nextLink) {
        if (channel->length > 0) {
            channel->length--;
        }
        if (!(channel->flags & SND7_EXCHANNEL_AUTO_SWEEP) && channel->sweepCounter < channel->sweepLength) {
            channel->sweepCounter++;
        }
    }
    if (track->flags & SND7_TRACK_NOTE_FINISH_WAIT) {
        if (track->channelList != NULL) {
            return 0;
        }
        track->flags &= ~SND7_TRACK_NOTE_FINISH_WAIT;
    }
    if (track->wait > 0) {
        track->wait--;
        if (track->wait > 0) {
            return 0;
        }
    }
    while (track->wait == 0 && !(track->flags & SND7_TRACK_NOTE_FINISH_WAIT)) {
        BOOL specialArg = FALSE;
        BOOL execute = TRUE;
        int argType = ARG_U8;
        int command;

        if (--budget == 0) {
            return -1;
        }
        command = TrackReadU8(track);
        if (command == 0xa2) {
            command = TrackReadU8(track);
            execute = (track->flags & SND7_TRACK_CMP) != 0;
        }
        if (command == 0xa0) {
            command = TrackReadU8(track);
            argType = ARG_RANDOM;
            specialArg = TRUE;
        }
        if (command == 0xa1) {
            command = TrackReadU8(track);
            argType = ARG_VARIABLE;
            specialArg = TRUE;
        }
        if (!(command & 0x80)) {
            int velocity = TrackReadU8(track);
            s32 length = TrackGetArgument(track, player, specialArg ? argType : ARG_VLV);
            int key = command + track->transpose;

            if (track->overrun) {
                return -1;
            }
            if (execute) {
                TrackNote(track, player, key, velocity, length, playNote);
            }
            continue;
        }
        switch (command & 0xf0) {
        case 0x80: {
            s32 argument = TrackGetArgument(track, player, specialArg ? argType : ARG_VLV);

            if (!execute) {
                break;
            }
            if (command == 0x80) {
                track->wait = argument;
            } else if (command == 0x81) {
                if (argument < 0x10000) {
                    track->prgNo = (u16)argument;
                }
            }
            break;
        }
        case 0x90:
            if (command == 0x93) {
                int trackNo = TrackReadU8(track);
                u32 offset = TrackReadU24(track);
                Snd7Track *opened;

                if (!execute || track->overrun) {
                    break;
                }
                opened = PlayerGetTrack(player, trackNo);
                if (opened != NULL && opened != track) {
                    TrackReleaseChannels(opened, player, -1);
                    TrackFreeChannels(opened);
                    TrackStart(opened, track->base, track->end, offset);
                }
            } else if (command == 0x94) {
                u32 offset = TrackReadU24(track);

                if (execute && !track->overrun) {
                    track->cur = Address(track->base, offset);
                }
            } else if (command == 0x95) {
                u32 offset = TrackReadU24(track);

                if (!execute || track->overrun || track->callStackDepth >= SND7_CALL_STACK_DEPTH) {
                    break;
                }
                track->callStack[track->callStackDepth] = track->cur;
                track->callStackDepth++;
                track->cur = Address(track->base, offset);
            }
            break;
        case 0xc0:
        case 0xd0: {
            u8 argument = (u8)TrackGetArgument(track, player, specialArg ? argType : ARG_U8);

            if (execute) {
                ByteCommand(track, player, command, argument);
            }
            break;
        }
        case 0xe0: {
            s16 argument = (s16)TrackGetArgument(track, player, specialArg ? argType : ARG_S16);

            if (!execute) {
                break;
            }
            if (command == 0xe0) {
                track->mod.delay = (u16)argument;
            } else if (command == 0xe1) {
                player->tempo = (u16)argument;
            } else if (command == 0xe3) {
                track->sweepPitch = argument;
            }
            break;
        }
        case 0xb0: {
            int varNo = TrackReadU8(track);
            s16 argument = (s16)TrackGetArgument(track, player, specialArg ? argType : ARG_S16);
            vs16 *variable = snd7_variable(player, varNo);

            if (execute && variable != NULL) {
                VariableCommand(track, variable, command, argument);
            }
            break;
        }
        case 0xf0:
            if (!execute) {
                break;
            }
            switch (command) {
            case 0xfc: {
                // The end of a loop: back to its start until its count runs out (0 loops for good)
                u8 count;

                if (track->callStackDepth == 0) {
                    break;
                }
                count = track->loopCount[track->callStackDepth - 1];
                if (count != 0) {
                    count--;
                    if (count == 0) {
                        track->callStackDepth--;
                        break;
                    }
                }
                track->loopCount[track->callStackDepth - 1] = count;
                track->cur = track->callStack[track->callStackDepth - 1];
                break;
            }
            case 0xfd:
                if (track->callStackDepth != 0) {
                    track->callStackDepth--;
                    track->cur = track->callStack[track->callStackDepth];
                }
                break;
            case 0xff:
                return -1;
            }
            break;
        }
        if (track->overrun) {
            return -1;
        }
    }
    return 0;
}

// PlayerStepTick (0x03802304): a tick of every track of the player; a track that ends stops. TRUE when none plays
static BOOL PlayerStepTick(Snd7Player *player, BOOL playNote) {
    BOOL playing = FALSE;
    int i;

    for (i = 0; i < SND7_PLAYER_TRACK_NUM; i++) {
        Snd7Track *track = PlayerGetTrack(player, i);

        if (track == NULL || track->cur == NULL) {
            continue;
        }
        if (TrackStepTick(track, player, playNote) == 0) {
            playing = TRUE;
        } else {
            PlayerStopTrack(player, i);
        }
    }
    return !playing;
}

// The channels' callback for a track (0x0380214c): a channel whose sound ended is free; either way it leaves the
// track's list
static void ChannelCallback(Snd7ExChannel *channel, int status, void *data) {
    Snd7Track *track = data;
    Snd7ExChannel *link;

    if (status == SND7_EXCHANNEL_CALLBACK_FINISH) {
        channel->prio = 0;
        snd7_free_exchannel(channel);
    }
    if (track->channelList == channel) {
        track->channelList = channel->nextLink;
        return;
    }
    for (link = track->channelList; link != NULL && link->nextLink != NULL; link = link->nextLink) {
        if (link->nextLink == channel) {
            link->nextLink = channel->nextLink;
            return;
        }
    }
}

// The players

// SND_SeqInit (0x03801560)
void snd7_seq_init(void) {
    int i;

    for (i = 0; i < SND7_PLAYER_NUM; i++) {
        snd7_work.player[i].flags &= ~SND7_PLAYER_ACTIVE;
        snd7_work.player[i].myNo = (u8)i;
    }
    for (i = 0; i < SND7_TRACK_NUM; i++) {
        snd7_work.track[i].flags &= ~SND7_TRACK_ACTIVE;
    }
}

// SND_SeqMain (0x038015c4): at a periodic update each started player that isn't paused moves on the ticks its tempo
// gives, counting them in the shared work; then every player's channels take its tracks' parameters. The shared work's
// playerStatus gets the players that are active
void snd7_seq_main(BOOL doPeriodicProc) {
    u32 status = 0;
    int i;

    for (i = 0; i < SND7_PLAYER_NUM; i++) {
        Snd7Player *player = &snd7_work.player[i];
        int j;

        if (!(player->flags & SND7_PLAYER_ACTIVE)) {
            continue;
        }
        if (player->flags & SND7_PLAYER_PREPARED) {
            if (doPeriodicProc && !(player->flags & SND7_PLAYER_PAUSE)) {
                u32 ticks = 0;
                u32 done;

                while (player->tempoCounter >= 240) {
                    player->tempoCounter -= 240;
                    ticks++;
                }
                for (done = 0; done < ticks; done++) {
                    if (PlayerStepTick(player, TRUE)) {
                        PlayerStop(player);
                        break;
                    }
                }
                if (snd7_shared != NULL) {
                    snd7_shared->player[player->myNo].tickCounter += done;
                }
                player->tempoCounter = (u16)(player->tempoCounter + ((u32)player->tempo * player->tempoRatio >> 8));
            }
            for (j = 0; j < SND7_PLAYER_TRACK_NUM; j++) {
                Snd7Track *track = PlayerGetTrack(player, j);

                if (track != NULL) {
                    TrackUpdateChannel(track, player, TRUE);
                }
            }
        }
        if (player->flags & SND7_PLAYER_ACTIVE) {
            status |= 1u << i;
        }
    }
    if (snd7_shared != NULL) {
        snd7_shared->playerStatus = status;
    }
}

// SND_PrepareSeq (0x0380171c): the player starts afresh with its first track at the sequence's offset, and the tracks
// a 0xfe there allocates; the shared work's tick count and the player's variables start again. It plays from
// SND_StartPreparedSeq
void snd7_prepare_seq(int playerNo, const u8 *seqBase, const u8 *seqEnd, u32 seqOffset, const u8 *bank,
                      const u8 *bankEnd) {
    Snd7Player *player = Player(playerNo);
    Snd7Track *track;
    int trackNo;
    int i;

    if (player->flags & SND7_PLAYER_ACTIVE) {
        PlayerStop(player);
    }
    player->flags &= ~SND7_PLAYER_PAUSE;
    player->bank = bank;
    player->bankEnd = bankEnd;
    player->tempo = 120;
    player->tempoRatio = 256;
    player->tempoCounter = 240;
    player->volume = 127;
    player->extFader = 0;
    player->prio = 64;
    for (i = 0; i < SND7_PLAYER_TRACK_NUM; i++) {
        player->tracks[i] = SND7_TRACK_NONE;
    }
    if (snd7_shared != NULL) {
        snd7_shared->player[player->myNo].tickCounter = 0;
        for (i = 0; i < SND_PLAYER_VARIABLE_MAX; i++) {
            snd7_shared->player[player->myNo].variable[i] = SND_DEFAULT_VARIABLE;
        }
    }
    trackNo = AllocTrack();
    if (trackNo < 0) {
        return;
    }
    track = &snd7_work.track[trackNo];
    TrackInit(track);
    TrackStart(track, seqBase, seqEnd, seqOffset);
    player->tracks[0] = (u8)trackNo;
    if (TrackReadU8(track) != 0xfe) {
        // No header of tracks: the track starts at the byte it read. A sequence with no byte there (one the host
        // couldn't find, or an offset at its end) read none, so the track stays where it is and ends at its first tick
        if (!track->overrun) {
            track->cur--;
        }
        track->overrun = FALSE;
    } else {
        u32 mask = TrackReadU16(track) >> 1;

        for (i = 1; mask != 0; i++, mask >>= 1) {
            if (mask & 1) {
                trackNo = AllocTrack();
                if (trackNo < 0) {
                    break;
                }
                TrackInit(&snd7_work.track[trackNo]);
                player->tracks[i] = (u8)trackNo;
            }
        }
    }
    player->flags = (u8)((player->flags & ~(SND7_PLAYER_ACTIVE | SND7_PLAYER_PREPARED)) | SND7_PLAYER_ACTIVE);
    if (snd7_shared != NULL) {
        snd7_shared->playerStatus |= 1u << playerNo;
    }
}

// SND_StartSeq (0x0380195c)
void snd7_start_seq(int playerNo, const u8 *seqBase, const u8 *seqEnd, u32 seqOffset, const u8 *bank,
                    const u8 *bankEnd) {
    snd7_prepare_seq(playerNo, seqBase, seqEnd, seqOffset, bank, bankEnd);
    Player(playerNo)->flags |= SND7_PLAYER_PREPARED;
}

// SND_StartPreparedSeq (0x0380193c)
void snd7_start_prepared_seq(int playerNo) {
    Player(playerNo)->flags |= SND7_PLAYER_PREPARED;
}

// SND_StopSeq (0x0380198c)
void snd7_stop_seq(int playerNo) {
    Snd7Player *player = Player(playerNo);

    if (!(player->flags & SND7_PLAYER_ACTIVE)) {
        return;
    }
    PlayerStop(player);
    if (snd7_shared != NULL) {
        snd7_shared->playerStatus &= ~(1u << playerNo);
    }
}

// SND_PauseSeq (0x038019e8): a pause releases the tracks' notes
void snd7_pause_seq(int playerNo, BOOL flag) {
    Snd7Player *player = Player(playerNo);
    int i;

    player->flags = (u8)((player->flags & ~SND7_PLAYER_PAUSE) | ((flag & 1) ? SND7_PLAYER_PAUSE : 0));
    if (!flag) {
        return;
    }
    for (i = 0; i < SND7_PLAYER_TRACK_NUM; i++) {
        Snd7Track *track = PlayerGetTrack(player, i);

        if (track != NULL) {
            TrackReleaseChannels(track, player, 127);
            TrackFreeChannels(track);
        }
    }
}

// SND_SkipSeq (0x03801a60): the player's notes stop and it moves on tick ticks without playing notes, with the
// driver's interval timer stopped meanwhile and started again after, which starts its period afresh
void snd7_skip_seq(int playerNo, u32 tick) {
    Snd7Player *player = Player(playerNo);
    u32 done;
    int i;

    for (i = 0; i < SND7_PLAYER_TRACK_NUM; i++) {
        Snd7Track *track = PlayerGetTrack(player, i);

        if (track != NULL) {
            TrackReleaseChannels(track, player, 127);
            TrackFreeChannels(track);
        }
    }
    snd7_stop_interval_timer();
    for (done = 0; done < tick; done++) {
        if (PlayerStepTick(player, FALSE)) {
            PlayerStop(player);
            break;
        }
    }
    snd7_start_interval_timer();
    if (snd7_shared != NULL) {
        snd7_shared->player[player->myNo].tickCounter += done;
    }
}

// SNDi_SetPlayerParam (0x03801cec) and SNDi_SetTrackParam (0x03801d30): a byte, halfword or word that the ARM9 writes
// at an offset of a player or of tracks. The DS writes anywhere; the host only in the fields that keep its layout
static void SetParam(void *object, u32 limit, u32 offset, u32 data, int size) {
    u8 bytes[4] = { (u8)data, (u8)(data >> 8), (u8)(data >> 16), (u8)(data >> 24) };

    if ((size != 1 && size != 2 && size != 4) || offset > limit || (u32)size > limit - offset) {
        return;
    }
    memcpy((u8 *)object + offset, bytes, (size_t)size);
}

void snd7_set_player_param(int playerNo, u32 offset, u32 data, int size) {
    SetParam(Player(playerNo), SND7_PLAYER_PARAM_SIZE, offset, data, size);
}

void snd7_set_track_param(int playerNo, u32 trackBitMask, u32 offset, u32 data, int size) {
    Snd7Player *player = Player(playerNo);
    int i;

    for (i = 0; i < SND7_PLAYER_TRACK_NUM && trackBitMask != 0; i++, trackBitMask >>= 1) {
        Snd7Track *track;

        if (!(trackBitMask & 1)) {
            continue;
        }
        track = PlayerGetTrack(player, i);
        if (track != NULL) {
            SetParam(track, SND7_TRACK_PARAM_SIZE, offset, data, size);
        }
    }
}

// SND_SetTrackMute (0x03801b28)
void snd7_set_track_mute(int playerNo, u32 trackBitMask, int muteType) {
    Snd7Player *player = Player(playerNo);
    int i;

    for (i = 0; i < SND7_PLAYER_TRACK_NUM && trackBitMask != 0; i++, trackBitMask >>= 1) {
        Snd7Track *track;

        if (!(trackBitMask & 1)) {
            continue;
        }
        track = PlayerGetTrack(player, i);
        if (track != NULL) {
            TrackMute(track, player, muteType);
        }
    }
}

// SND_SetTrackAllocatableChannel (0x03801b94): the channels a track's notes may take, which also makes its requests
// strong
void snd7_set_track_allocatable_channel(int playerNo, u32 trackBitMask, u32 chBitFlag) {
    Snd7Player *player = Player(playerNo);
    int i;

    for (i = 0; i < SND7_PLAYER_TRACK_NUM && trackBitMask != 0; i++, trackBitMask >>= 1) {
        Snd7Track *track;

        if (!(trackBitMask & 1)) {
            continue;
        }
        track = PlayerGetTrack(player, i);
        if (track != NULL) {
            track->channelMask = (u16)chBitFlag;
            track->flags |= SND7_TRACK_CHANNEL_MASK;
        }
    }
}

// Whether a host pointer of the ARM9 lies in the range [start, end], as the DS compares its addresses. A range whose
// start the host couldn't find (start NULL) compares the low 32 bits, across a range that may wrap at 4 GiB as a
// host's can
static BOOL InRange(const void *pointer, const u8 *start, const u8 *end, u32 start32, u32 end32) {
    if (pointer == NULL) {
        return FALSE;
    }
    if (start != NULL) {
        return (uintptr_t)start <= (uintptr_t)pointer && (uintptr_t)pointer <= (uintptr_t)end;
    }
    return (u32)(uintptr_t)pointer - start32 <= end32 - start32;
}

// SND_InvalidateSeq (0x03801c04): the players with a track reading in the range stop
void snd7_invalidate_seq(const u8 *start, const u8 *end, u32 start32, u32 end32) {
    int i;

    for (i = 0; i < SND7_PLAYER_NUM; i++) {
        Snd7Player *player = &snd7_work.player[i];
        int j;

        if (!(player->flags & SND7_PLAYER_ACTIVE)) {
            continue;
        }
        for (j = 0; j < SND7_PLAYER_TRACK_NUM; j++) {
            Snd7Track *track = PlayerGetTrack(player, j);

            if (track != NULL && InRange(track->cur, start, end, start32, end32)) {
                PlayerStop(player);
                break;
            }
        }
    }
}

// SND_InvalidateBank (0x03801c90): the players whose bank is in the range stop
void snd7_invalidate_bank(const u8 *start, const u8 *end, u32 start32, u32 end32) {
    int i;

    for (i = 0; i < SND7_PLAYER_NUM; i++) {
        Snd7Player *player = &snd7_work.player[i];

        if ((player->flags & SND7_PLAYER_ACTIVE) && InRange(player->bank, start, end, start32, end32)) {
            PlayerStop(player);
        }
    }
}
