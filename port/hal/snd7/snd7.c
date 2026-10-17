// The host ARM7's sound driver for phase 1: a null driver (PXI tag SOUND), which takes every command of the decomp's
// NitroSDK SND (lib/nitro/src/snd) and plays no sound. The real driver, written from the ARM7's code, comes in phase 2
// (PLAN.md §4.7); until then this one keeps the ARM9's side going as the game expects:
//   - every packet is done as soon as the ARM9 sends it: the shared work's count of finished packets
//     (finishedCommandTag) moves on before PXI_SendWordByFifo returns, so SND_RecvCommandReply, SND_WaitForCommandProc
//     and SND_AllocCommand never wait;
//   - a sequence plays from the command that prepares it until it ends or is stopped: its player's bit is set in the
//     shared work's playerStatus, and its tick counter runs. When it ends comes from its data: a timing-only reading of
//     the sequence (waits, notes that wait, tempo, jumps, calls, loops, variables, tracks it opens), at the DS's rate
//     of a driver update every 2728 * 64 cycles and a tick each time the tempo adds up to 240. A sequence whose tracks
//     all reach their end (0xff) ends there, so a jingle that the game waits for ends when it would; one that loops
//     plays until it is stopped, as the background music does. A sequence whose data the host can't find (see below)
//     plays for a second;
//   - channels never play, so channelStatus stays 0: a wave the game plays (cries, streams) has ended as soon as the
//     game looks, and NNS_SndWaveOutWaitForChannelStop's busy loop ends at once;
//   - the alarms (SND_SetupAlarm, which the stream player uses) go off on the virtual clock, each a word to the ARM9
//     with the alarm's number and ID, as the DS's driver sends them;
//   - SND_ReadDriverInfo gets a driver state with no tracks.
// The driver's work at each VBlank (port_snd7_frame) runs the updates and alarms of the time since the last one.
//
// Pointers: the packet's word carries the low 26 bits of the address of its first command, and the commands carry
// pointers in 32-bit arguments, which a 64-bit host cuts. The packet is always the ARM9's queue's ready list
// (SNDi_Queue.readyHead) when it is sent, and the shared work the one SNDi_SharedWork points to, so those two are
// taken from the ARM9's variables after a check of the bits. A sequence's data, the driver state's buffer and the
// start of a range of data that goes are found with port_pxi_noted_pointer, from the pointers that the host's
// snd_interface.c noted under the sound's tag when it queued the commands. A sequence is read only inside the memory
// it was found in: a jump, call or track whose offset leads out of it ends the track there.
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/pxi.h"
#include "nitro/snd.h"
#include <stdio.h>
#include <string.h>

// The driver's update: every 2728 ticks of 64 cycles (NitroSDK's SND_PROC_INTERVAL), 191.95 times a second
#define SND7_UPDATE_CYCLES (2728u * 64u)
// The sequencer's tempo: a tick each time the tempo, scaled by the player's ratio (256 is 1), adds up to 240; a
// sequence starts at 120
#define SND7_TICK_THRESHOLD 240
#define SND7_DEFAULT_TEMPO 120
#define SND7_TEMPO_RATIO_ONE 256
// The alarms' ticks are the ARM7's OS ticks, 64 cycles each
#define SND7_ALARM_TICK_CYCLES 64u
// How long a sequence whose data can't be read plays: a second of updates
#define SND7_UNKNOWN_SEQUENCE_UPDATES 192
// A track's calls and loops nest 3 deep; a track that runs this many commands without a wait ends
#define SND7_TRACK_STACK_DEPTH 3
#define SND7_TRACK_COMMAND_BUDGET 4096
// A player's parameter that the driver keeps: the tempo ratio (SND_SetPlayerTempoRatio's offset)
#define SND7_PLAYER_PARAM_TEMPO_RATIO 0x1a
// At most this many words an alarm sends in one VBlank, so that a short period can't flood the FIFO
#define SND7_ALARM_FIRES_MAX 64

typedef struct Snd7Track {
    BOOL active;
    const u8 *pos;
    s32 wait;
    BOOL noteWait;
    BOOL compare;
    u32 depth;
    const u8 *stack[SND7_TRACK_STACK_DEPTH];
    u8 loops[SND7_TRACK_STACK_DEPTH];
} Snd7Track;

typedef struct Snd7Player {
    BOOL active;  // prepared or started, until it ends or is stopped: its bit in playerStatus
    BOOL started; // SND_StartPreparedSeq came
    BOOL paused;
    u32 number;
    // The sequence's data: its base (NULL when it can't be read), the end of the memory it is in, and the low 32 bits
    // of its base as the ARM9 sent it, for SND_InvalidateSeqData
    const u8 *base;
    const u8 *end;
    u32 base32;
    u32 updatesLeft; // for a sequence that can't be read
    u32 tempo;
    u32 tempoRatio;
    u32 tempoCounter;
    u32 ticks;
    Snd7Track tracks[SND_PLAYER_TRACK_MAX];
} Snd7Player;

typedef struct Snd7Alarm {
    BOOL running;
    u32 tick;
    u32 period;
    u8 id;
    u64 due;
} Snd7Alarm;

static struct {
    SNDSharedWork *work;
    Snd7Player players[SND_PLAYER_MAX];
    Snd7Alarm alarms[SND_ALARM_MAX];
    u64 lastUpdate;
    // The things reported once a run: a packet or shared work that isn't the ARM9's, data that can't be found
    BOOL reportedPacket;
    BOOL reportedSequence;
    BOOL reportedInfo;
} sSnd;

// The shared work's view of a player

static void WriteStatus(void) {
    u32 status = 0;
    int i;

    if (sSnd.work == NULL) {
        return;
    }
    for (i = 0; i < SND_PLAYER_MAX; i++) {
        if (sSnd.players[i].active) {
            status |= 1u << i;
        }
    }
    sSnd.work->playerStatus = status;
    sSnd.work->channelStatus = 0;
    sSnd.work->captureStatus = 0;
}

static void EndPlayer(Snd7Player *player) {
    player->active = FALSE;
    player->started = FALSE;
    WriteStatus();
}

// A sequence variable: 0-15 the player's own, 16-31 the global ones, all in the shared work as the ARM9 sees them
static vs16 *Variable(const Snd7Player *player, u32 number) {
    static vs16 sScratch;

    if (sSnd.work == NULL || number >= 2 * SND_PLAYER_VARIABLE_MAX) {
        sScratch = SND_VARIABLE_UNSET;
        return &sScratch;
    }
    if (number < SND_PLAYER_VARIABLE_MAX) {
        return &sSnd.work->player[player->number].variable[number];
    }
    return &sSnd.work->globalVariable[number - SND_PLAYER_VARIABLE_MAX];
}

// The timing-only sequencer

// How a command's last argument is read: its own type, or as the random or variable prefix before it says
enum {
    ARG_U8,
    ARG_S16,
    ARG_VARIABLE_LENGTH,
    ARG_RANDOM,
    ARG_FROM_VARIABLE,
};

// The sequence's data at offset from its base, or NULL when that is outside the memory it is in
static const u8 *DataAt(const Snd7Player *player, u32 offset) {
    return offset < (uintptr_t)player->end - (uintptr_t)player->base ? player->base + offset : NULL;
}

static BOOL Fetch(const Snd7Player *player, Snd7Track *track, u32 bytes, u32 *value) {
    u32 i;

    *value = 0;
    if (track->pos < player->base || track->pos > player->end || (uintptr_t)(player->end - track->pos) < bytes) {
        return FALSE;
    }
    for (i = 0; i < bytes; i++) {
        *value |= (u32)*track->pos++ << (8 * i);
    }
    return TRUE;
}

static BOOL FetchArgument(const Snd7Player *player, Snd7Track *track, int type, s32 *value) {
    u32 raw;
    u32 high;
    int i;

    switch (type) {
    case ARG_U8:
        if (!Fetch(player, track, 1, &raw)) {
            return FALSE;
        }
        *value = (s32)raw;
        return TRUE;
    case ARG_S16:
        if (!Fetch(player, track, 2, &raw)) {
            return FALSE;
        }
        *value = (s16)raw;
        return TRUE;
    case ARG_VARIABLE_LENGTH:
        *value = 0;
        for (i = 0; i < 4; i++) {
            if (!Fetch(player, track, 1, &raw)) {
                return FALSE;
            }
            *value = (*value << 7) | (s32)(raw & 0x7f);
            if (!(raw & 0x80)) {
                break;
            }
        }
        return TRUE;
    case ARG_RANDOM:
        // The driver's random value between the two; the middle, which is deterministic and as likely a length
        if (!Fetch(player, track, 2, &raw) || !Fetch(player, track, 2, &high)) {
            return FALSE;
        }
        *value = ((s16)raw + (s16)high) / 2;
        return TRUE;
    default:
        if (!Fetch(player, track, 1, &raw)) {
            return FALSE;
        }
        *value = *Variable(player, raw);
        return TRUE;
    }
}

// The shift command's value: to the left by a positive argument, to the right by a negative one, as the ARM's shifts
// by a register do it, which take the amount from the register's low byte and give 0 to the left and the sign to the
// right for an amount of 32 or more
static s32 ShiftVariable(s32 value, s32 argument) {
    u32 amount;

    if (argument >= 0) {
        amount = (u32)argument & 0xff;
        return amount >= 32 ? 0 : (s32)((u32)value << amount);
    }
    amount = (0u - (u32)argument) & 0xff;
    if (amount >= 32) {
        return value < 0 ? -1 : 0;
    }
    return value >> amount;
}

static void VariableCommand(const Snd7Player *player, Snd7Track *track, u32 command, u32 number, s32 argument) {
    vs16 *variable = Variable(player, number);
    s32 value = *variable;

    switch (command) {
    case 0xb0:
        *variable = (s16)argument;
        break;
    case 0xb1:
        *variable = (s16)(value + argument);
        break;
    case 0xb2:
        *variable = (s16)(value - argument);
        break;
    case 0xb3:
        *variable = (s16)(value * argument);
        break;
    case 0xb4:
        if (argument != 0) {
            *variable = (s16)(value / argument);
        }
        break;
    case 0xb5:
        *variable = (s16)ShiftVariable(value, argument);
        break;
    case 0xb6:
        *variable = (s16)(argument / 2);
        break;
    case 0xb8:
        track->compare = value == argument;
        break;
    case 0xb9:
        track->compare = value >= argument;
        break;
    case 0xba:
        track->compare = value > argument;
        break;
    case 0xbb:
        track->compare = value <= argument;
        break;
    case 0xbc:
        track->compare = value < argument;
        break;
    case 0xbd:
        track->compare = value != argument;
        break;
    }
}

// Opens a track at offset; one whose start is outside the sequence's memory stays closed
static void StartTrack(Snd7Player *player, u32 number, u32 offset) {
    Snd7Track *track = &player->tracks[number];
    const u8 *start = DataAt(player, offset);

    memset(track, 0, sizeof(*track));
    if (start == NULL) {
        return;
    }
    track->active = TRUE;
    track->pos = start;
    track->noteWait = TRUE;
    track->compare = TRUE;
}

// Runs the track's commands until one makes it wait. FALSE when it ended
static BOOL RunTrack(Snd7Player *player, Snd7Track *track) {
    int budget = SND7_TRACK_COMMAND_BUDGET;

    while (track->wait == 0) {
        u32 command;
        u32 value;
        s32 argument;
        BOOL execute = TRUE;
        int type = -1;

        if (--budget == 0 || !Fetch(player, track, 1, &command)) {
            return FALSE;
        }
        if (command == 0xa2) {
            execute = track->compare;
            if (!Fetch(player, track, 1, &command)) {
                return FALSE;
            }
        }
        if (command == 0xa0 || command == 0xa1) {
            type = command == 0xa0 ? ARG_RANDOM : ARG_FROM_VARIABLE;
            if (!Fetch(player, track, 1, &command)) {
                return FALSE;
            }
        }
        if (command < 0x80) {
            // A note: its velocity, then its length, which it waits for in the note-wait mode
            if (!Fetch(player, track, 1, &value) ||
                !FetchArgument(player, track, type < 0 ? ARG_VARIABLE_LENGTH : type, &argument)) {
                return FALSE;
            }
            if (execute && track->noteWait) {
                track->wait = argument > 0 ? argument : 0;
            }
            continue;
        }
        switch (command & 0xf0) {
        case 0x80:
            // A wait, or a program change
            if (!FetchArgument(player, track, type < 0 ? ARG_VARIABLE_LENGTH : type, &argument)) {
                return FALSE;
            }
            if (execute && command == 0x80) {
                track->wait = argument > 0 ? argument : 0;
            }
            break;
        case 0x90:
            if (command == 0x93) {
                // Opens a track: its number, then where it starts
                u32 offset;

                if (!Fetch(player, track, 1, &value) || !Fetch(player, track, 3, &offset)) {
                    return FALSE;
                }
                if (execute && value < SND_PLAYER_TRACK_MAX && &player->tracks[value] != track) {
                    StartTrack(player, value, offset);
                }
            } else if (command == 0x94 || command == 0x95) {
                // A jump or a call: where to, which must be inside the sequence's memory
                const u8 *target;

                if (!Fetch(player, track, 3, &value)) {
                    return FALSE;
                }
                if (!execute) {
                    break;
                }
                target = DataAt(player, value);
                if (target == NULL) {
                    return FALSE;
                }
                if (command == 0x95) {
                    if (track->depth == SND7_TRACK_STACK_DEPTH) {
                        break;
                    }
                    track->stack[track->depth] = track->pos;
                    track->loops[track->depth] = 0;
                    track->depth++;
                }
                track->pos = target;
            } else {
                return FALSE;
            }
            break;
        case 0xb0:
            if (!Fetch(player, track, 1, &value) ||
                !FetchArgument(player, track, type < 0 ? ARG_S16 : type, &argument)) {
                return FALSE;
            }
            if (execute) {
                VariableCommand(player, track, command, value, argument);
            }
            break;
        case 0xc0:
        case 0xd0:
            if (!FetchArgument(player, track, type < 0 ? ARG_U8 : type, &argument)) {
                return FALSE;
            }
            if (!execute) {
                break;
            }
            if (command == 0xc7) {
                track->noteWait = argument != 0;
            } else if (command == 0xd4 && track->depth < SND7_TRACK_STACK_DEPTH) {
                // A loop's start, with its count (0 loops for good)
                track->stack[track->depth] = track->pos;
                track->loops[track->depth] = (u8)argument;
                track->depth++;
            }
            break;
        case 0xe0:
            if (!FetchArgument(player, track, type < 0 ? ARG_S16 : type, &argument)) {
                return FALSE;
            }
            if (execute && command == 0xe1) {
                player->tempo = argument > 0 ? (u32)argument : 0;
            }
            break;
        case 0xf0:
            if (command == 0xfc) {
                // A loop's end: back to its start until its count runs out
                if (execute && track->depth != 0) {
                    u8 count = track->loops[track->depth - 1];

                    if (count != 0 && --count == 0) {
                        track->depth--;
                        break;
                    }
                    track->loops[track->depth - 1] = count;
                    track->pos = track->stack[track->depth - 1];
                }
            } else if (command == 0xfd) {
                if (execute && track->depth != 0) {
                    track->depth--;
                    track->pos = track->stack[track->depth];
                }
            } else if (command == 0xfe) {
                if (!Fetch(player, track, 2, &value)) {
                    return FALSE;
                }
            } else if (command == 0xff) {
                if (execute) {
                    return FALSE;
                }
            } else {
                return FALSE;
            }
            break;
        default:
            return FALSE;
        }
    }
    return TRUE;
}

// A tick of the track: its wait runs down, then it runs on. FALSE when it ended
static BOOL TickTrack(Snd7Player *player, Snd7Track *track) {
    if (track->wait > 0) {
        track->wait--;
        if (track->wait > 0) {
            return TRUE;
        }
    }
    return RunTrack(player, track);
}

static void UpdatePlayer(Snd7Player *player) {
    int i;

    if (!player->active || !player->started || player->paused) {
        return;
    }
    if (player->base == NULL) {
        if (player->updatesLeft == 0 || --player->updatesLeft == 0) {
            EndPlayer(player);
        }
        return;
    }
    while (player->tempoCounter >= SND7_TICK_THRESHOLD) {
        BOOL playing = FALSE;

        player->tempoCounter -= SND7_TICK_THRESHOLD;
        for (i = 0; i < SND_PLAYER_TRACK_MAX; i++) {
            Snd7Track *track = &player->tracks[i];

            if (track->active && !TickTrack(player, track)) {
                track->active = FALSE;
            }
            playing |= track->active;
        }
        player->ticks++;
        if (sSnd.work != NULL) {
            sSnd.work->player[player->number].tickCounter = player->ticks;
        }
        if (!playing) {
            EndPlayer(player);
            return;
        }
    }
    player->tempoCounter += player->tempo * player->tempoRatio / SND7_TEMPO_RATIO_ONE;
}

// The commands

static void PrepareSequence(u32 number, u32 base32, u32 offset) {
    Snd7Player *player = &sSnd.players[number];
    const u8 *end = NULL;
    const u8 *base = port_pxi_noted_pointer(PXI_FIFO_TAG_SOUND, base32, &end);

    memset(player, 0, sizeof(*player));
    player->active = TRUE;
    player->number = number;
    player->base32 = base32;
    player->tempo = SND7_DEFAULT_TEMPO;
    player->tempoRatio = SND7_TEMPO_RATIO_ONE;
    player->tempoCounter = SND7_TICK_THRESHOLD;
    // A new sequence starts with its tick count at 0 and its own variables unset
    if (sSnd.work != NULL) {
        int i;

        sSnd.work->player[number].tickCounter = 0;
        for (i = 0; i < SND_PLAYER_VARIABLE_MAX; i++) {
            sSnd.work->player[number].variable[i] = SND_VARIABLE_UNSET;
        }
    }
    if (base == NULL || offset >= (u64)(end - base)) {
        if (!sSnd.reportedSequence) {
            fprintf(stderr,
                    "port: sound: a sequence's data at %#x can't be found in host memory; it plays for a "
                    "second\n",
                    base32);
            sSnd.reportedSequence = TRUE;
        }
        player->updatesLeft = SND7_UNKNOWN_SEQUENCE_UPDATES;
    } else {
        player->base = base;
        player->end = end;
        // A sequence of several tracks starts with the ones it allocates (0xfe, a mask), which it opens itself
        StartTrack(player, 0, offset);
        if (base[offset] == 0xfe) {
            u32 allocated;

            Fetch(player, &player->tracks[0], 3, &allocated);
        }
    }
    WriteStatus();
}

// SND_ReadDriverInfo's state: no player has a track
static void ReadDriverInfo(u32 address) {
    const u8 *end = NULL;
    SNDDriverInfo *info = port_pxi_noted_pointer(PXI_FIFO_TAG_SOUND, address, &end);
    int i;

    if (info == NULL || (u64)(end - (const u8 *)info) < sizeof(*info)) {
        if (!sSnd.reportedInfo) {
            fprintf(stderr, "port: sound: the buffer of SND_ReadDriverInfo at %#x can't be found in host memory\n",
                    address);
            sSnd.reportedInfo = TRUE;
        }
        return;
    }
    memset(info, 0, sizeof(*info));
    for (i = 0; i < SND_PLAYER_MAX; i++) {
        memset(info->player[i].trackIndex, SND_TRACK_INDEX_NONE, sizeof(info->player[i].trackIndex));
    }
}

// SND_InvalidateSeqData: the players whose sequence starts in the range stop, as the data goes. The range's start is
// the host pointer that snd_interface_host.c noted, and the players' bases are compared with it as host pointers; a
// player whose data wasn't found, or a start that wasn't noted, is compared by the low 32 bits, across a range that may
// wrap around at 4 GiB as a host's can
static void InvalidateSequences(u32 start32, u32 end32) {
    const u8 *start = port_pxi_noted_pointer(PXI_FIFO_TAG_SOUND, start32, NULL);
    u32 size = end32 - start32;
    int i;

    for (i = 0; i < SND_PLAYER_MAX; i++) {
        Snd7Player *player = &sSnd.players[i];
        BOOL inRange;

        if (!player->active) {
            continue;
        }
        if (start != NULL && player->base != NULL) {
            inRange = (uintptr_t)player->base - (uintptr_t)start < size;
        } else {
            inRange = player->base32 - start32 < size;
        }
        if (inRange) {
            EndPlayer(player);
        }
    }
}

static void StartAlarms(u32 mask) {
    u64 now = port_os_cycles();
    int i;

    for (i = 0; i < SND_ALARM_MAX; i++) {
        if (mask & (1u << i)) {
            Snd7Alarm *alarm = &sSnd.alarms[i];

            alarm->running = TRUE;
            alarm->due = now + (u64)(alarm->tick != 0 ? alarm->tick : 1) * SND7_ALARM_TICK_CYCLES;
        }
    }
}

static void StopAlarms(u32 mask) {
    int i;

    for (i = 0; i < SND_ALARM_MAX; i++) {
        if (mask & (1u << i)) {
            sSnd.alarms[i].running = FALSE;
        }
    }
}

static void Execute(const SNDCommand *command) {
    u32 number = command->arg[0];

    switch (command->id) {
    case SND_COMMAND_SHARED_WORK:
        if (SNDi_SharedWork != NULL && (u32)(uintptr_t)SNDi_SharedWork == command->arg[0]) {
            sSnd.work = SNDi_SharedWork;
            WriteStatus();
        }
        break;
    case SND_COMMAND_PREPARE_SEQ:
        if (number < SND_PLAYER_MAX) {
            PrepareSequence(number, command->arg[1], command->arg[2]);
        }
        break;
    case SND_COMMAND_START_PREPARED_SEQ:
        if (number < SND_PLAYER_MAX && sSnd.players[number].active) {
            sSnd.players[number].started = TRUE;
        }
        break;
    case SND_COMMAND_STOP_SEQ:
        if (number < SND_PLAYER_MAX) {
            EndPlayer(&sSnd.players[number]);
        }
        break;
    case SND_COMMAND_PAUSE_SEQ:
        if (number < SND_PLAYER_MAX) {
            sSnd.players[number].paused = command->arg[1] != 0;
        }
        break;
    case SND_COMMAND_PLAYER_PARAM:
        if (number < SND_PLAYER_MAX && command->arg[1] == SND7_PLAYER_PARAM_TEMPO_RATIO) {
            sSnd.players[number].tempoRatio = command->arg[2] & 0xffff;
        }
        break;
    case SND_COMMAND_INVALIDATE_SEQ:
        InvalidateSequences(command->arg[0], command->arg[1]);
        break;
    case SND_COMMAND_SETUP_ALARM:
        if (number < SND_ALARM_MAX) {
            Snd7Alarm *alarm = &sSnd.alarms[number];

            alarm->running = FALSE;
            alarm->tick = command->arg[1];
            alarm->period = command->arg[2];
            alarm->id = (u8)command->arg[3];
        }
        break;
    case SND_COMMAND_START_TIMER:
        StartAlarms(command->arg[2]);
        break;
    case SND_COMMAND_STOP_TIMER:
        StopAlarms(command->arg[2]);
        break;
    case SND_COMMAND_READ_DRIVER_INFO:
        ReadDriverInfo(command->arg[0]);
        break;
    default:
        // The tracks' and channels' settings, the master volume and pan, the banks and waves: nothing plays
        break;
    }
}

// A word of the ARM9: 0 asks the driver to look at its packets, which it has done already; any other is a packet
static void SoundService(u32 tag, u32 data, BOOL err) {
    const SNDCommand *command = SNDi_Queue.readyHead;

    if (err || data == 0) {
        return;
    }
    if (command == NULL || ((u32)(uintptr_t)command & PORT_PXI_DATA_MASK) != data) {
        if (!sSnd.reportedPacket) {
            fprintf(stderr, "port: sound: a packet at %#x that isn't the ARM9's ready list; its commands are skipped\n",
                    data);
            sSnd.reportedPacket = TRUE;
        }
        command = NULL;
    }
    for (; command != NULL; command = command->next) {
        Execute(command);
    }
    if (sSnd.work != NULL) {
        sSnd.work->finishedCommandTag++;
    }
}

void port_snd7_start(void) {
    memset(&sSnd, 0, sizeof(sSnd));
    sSnd.lastUpdate = port_os_cycles();
    port_pxi_register_service(PXI_FIFO_TAG_SOUND, SoundService);
}

void port_snd7_frame(u64 cycles) {
    int i;

    if (cycles < sSnd.lastUpdate) {
        sSnd.lastUpdate = cycles;
    }
    while (cycles - sSnd.lastUpdate >= SND7_UPDATE_CYCLES) {
        sSnd.lastUpdate += SND7_UPDATE_CYCLES;
        for (i = 0; i < SND_PLAYER_MAX; i++) {
            UpdatePlayer(&sSnd.players[i]);
        }
    }
    for (i = 0; i < SND_ALARM_MAX; i++) {
        Snd7Alarm *alarm = &sSnd.alarms[i];
        int fires = 0;

        while (alarm->running && alarm->due <= cycles && fires < SND7_ALARM_FIRES_MAX) {
            port_pxi_reply(PXI_FIFO_TAG_SOUND, (u32)i | ((u32)alarm->id << 8), FALSE);
            fires++;
            if (alarm->period == 0) {
                alarm->running = FALSE;
            } else {
                alarm->due += (u64)alarm->period * SND7_ALARM_TICK_CYCLES;
            }
        }
        if (alarm->running && alarm->due <= cycles) {
            // The rest of a period too short for a frame's words
            alarm->due = cycles + (u64)alarm->period * SND7_ALARM_TICK_CYCLES;
        }
    }
}
