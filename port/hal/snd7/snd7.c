// The host ARM7's sound driver (PXI tag SOUND), which plays everything the game plays: its music, sound effects and
// cries (src/system/snd_sys.c, through NitroSystem's sound players and the ARM9's SND library). It is TwlSDK's SND
// driver as the ARM7 runs it, in C written from the ARM7's code (port/hal/snd7/snd7_internal.h lists its files and
// functions), and the sound unit it drives (snd7_hw.c), whose samples go to the host's audio (port_snd7_set_output,
// snd7_output.h). Its state after each run goes to the sound trace that holds it to melonDS's, run by run
// (port_snd7_set_trace, port/app/sound_trace.c, port/tools/oracle/README.md "Sound").
//
// The DS's driver is a thread (SndThread, 0x038006ac) that waits for a message: a periodic OS alarm every 2728 ticks
// of 64 cycles (about 192 a second, the first 0x10000 ticks after the driver starts) or the ARM9's wake-up word (0).
// For each it runs, in order, SND_UpdateExChannel (the register writes the last run owed the channels), SND_CommandProc
// (the packets of commands the ARM9 sent), SND_SeqMain and SND_ExChannelMain (which move the sequences and the
// envelopes on only at a periodic run), SND_UpdateSharedWork (the channels' and capture units' status for the ARM9)
// and SND_CalcRandom. Here:
//   - time is the virtual clock (port_os_cycles). The driver catches up to it (Advance) at each VBlank
//     (port_snd7_frame) and whenever the ARM9 sends a word, running in their order the periodic runs, the alarms and
//     the mixer's samples (one each 1024 cycles, 32,728.5 a second) that fall before it; so the samples and the
//     driver's state are the same on every run, whenever the host looks;
//   - a packet's commands run as soon as the ARM9 sends it, and the shared work's count of finished packets
//     (finishedCommandTag) moves on before PXI_SendWordByFifo returns, as the ARM9's busy waits need
//     (SND_RecvCommandReply, SND_WaitForCommandProc, SND_AllocCommand). The DS runs them at its next run, after that
//     run's SND_UpdateExChannel; here they run before it. A wake-up word runs a run that isn't periodic at once;
//   - so the ARM9 finds a packet done as soon as it has sent it, where the DS's finds it waiting until the driver's
//     next run, and the waits that wake the driver for that run (SND_WaitForCommandProc after a flush, SND_AllocCommand
//     with no command left) would never send the word. Each run draws from SND_CalcRandom's generator, which the
//     whole driver shares, so a run missing would change every random note after it. The host's
//     SND_WaitForCommandProc (snd_command_host.c) sends the word whenever the DS's driver would still hold the packet,
//     which snd7_holds_packet tells from the count of finished packets at the driver's last run; SND_AllocCommand's
//     wake-up, with all 256 commands sent, isn't modelled;
//   - the alarms (SND_SetupAlarm, which the stream player uses) are the ARM7's OS alarms on the virtual clock, each
//     sending the ARM9 its number and ID; their handler's count of words still to send covers a full FIFO (16 words
//     waiting in the ARM9's), as on the DS. The next alarm is the OS core's timed work (port_os_set_timed_work), so
//     that its word reaches the ARM9 when it goes off, as on the DS, and not with the others of the frame at the
//     VBlank: NitroSystem's stream player queues a block to read at each alarm and drops the oldest when a third
//     comes before its thread has read one, which two or three alarms a frame taken together did every 7 frames or
//     so, playing a block of silence and the rest of the stream a block late.
//
// Pointers: the packet's word carries the low 26 bits of the address of its first command, and the commands carry
// pointers in 32-bit arguments, which a 64-bit host cuts. The packet is always the ARM9's queue's ready list
// (SNDi_Queue.readyHead) when it is sent, and the shared work the one SNDi_SharedWork points to, so those two are
// taken from the ARM9's variables after a check of the bits. A sequence and its bank, a channel's wave, the driver
// state's buffer and the start of a range of data that goes are found with port_pxi_noted_pointer, from the pointers
// that the host's snd_interface.c noted under the sound's tag when it queued the commands. The waves and wave archives
// that banks link to are found by the handles of the host's snd_bank.c (snd_bank_host.c).
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "hal/snd7/snd7_internal.h"
#include "hal/snd7/snd7_output.h"
#include "nitro/pxi.h"
#include "tools/oracle/sndtrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The DS's commands that the decomp's snd.h doesn't name
#define COMMAND_START_SEQ 0
#define COMMAND_SKIP_SEQ 5
#define COMMAND_PLAYER_LOCAL_VAR 0xa
#define COMMAND_PLAYER_GLOBAL_VAR 0xb
#define COMMAND_SETUP_CHANNEL_PSG 0xf
#define COMMAND_SETUP_CHANNEL_NOISE 0x10
#define COMMAND_SETUP_CAPTURE 0x11
#define COMMAND_SURROUND_DECAY 0x16
#define COMMAND_STOP_UNLOCKED_CHANNEL 0x1c

// The ARM7's address of its work (SNDi_Work), which SND_ReadDriverInfo's state carries for its links
#define WORK_ADDRESS 0x03809fbc

// The words that the ARM9's receive FIFO holds
#define FIFO_DEPTH 16

// The samples kept for the output until the next VBlank: room for two frames' at first, more when the clock runs
// further between VBlanks
#define OUTPUT_FRAMES_INITIAL 2048

SNDSharedWork *snd7_shared;

static struct {
    BOOL booted;
    // The virtual clock's cycle the driver has run to, and the next mixer sample and periodic run
    u64 now;
    u64 nextSample;
    BOOL timerRunning;
    u64 nextUpdate;
    u64 samples;
    // The output, and the samples made since it was last given them. They go to it only from port_snd7_frame (the
    // host's stack at a VBlank), never from inside the ARM9's call that sent a word, which runs on a thread's fiber
    PortSnd7Output output;
    void *outputUser;
    s16 *buffer;
    u32 bufferFrames;
    u32 buffered;
    // The shared work's count of finished packets at the driver's last run: the packets the DS's driver would have run
    u32 runTag;
    // The mixer sample after the last periodic run, for the trace (PBW_SNDTRACE_RUN_MIX)
    u32 runMix;
    // The things reported once a run: a packet that isn't the ARM9's, data that can't be found
    BOOL reportedPacket;
    BOOL reportedPointer;
    // The trace, called after each run
    PortSnd7Trace trace;
    void *traceUser;
    // An alarm's moment is the OS core's timed work
    BOOL alarmScheduled;
} sSnd;

// Memory the ARM9 gave the driver

const u8 *snd7_memory_end(const void *pointer) {
    const u8 *end = NULL;

    if (pointer == NULL || port_pxi_pointer((u32)(uintptr_t)pointer, &end) != pointer) {
        return NULL;
    }
    return end;
}

const u8 *snd7_resolve(u32 address, const u8 **end) {
    const u8 *pointer;

    *end = NULL;
    if (address == 0) {
        return NULL;
    }
    if (address & SND7_HANDLE_FLAG) {
        pointer = snd7_handle_pointer(address);
        if (pointer != NULL) {
            *end = snd7_memory_end(pointer);
            return pointer;
        }
    }
    return port_pxi_pointer(address, end);
}

BOOL snd7_read_bytes(const u8 *start, const u8 *end, u32 offset, void *dst, u32 size) {
    if (start == NULL) {
        return FALSE;
    }
    if (end != NULL && (u64)offset + size > (u64)((uintptr_t)end - (uintptr_t)start)) {
        return FALSE;
    }
    memcpy(dst, (const u8 *)((uintptr_t)start + offset), size);
    return TRUE;
}

u32 snd7_read32(const u8 *start, const u8 *end, u32 offset, BOOL *ok) {
    u8 bytes[4];

    if (!snd7_read_bytes(start, end, offset, bytes, 4)) {
        *ok = FALSE;
        return 0;
    }
    return (u32)bytes[0] | (u32)bytes[1] << 8 | (u32)bytes[2] << 16 | (u32)bytes[3] << 24;
}

// A pointer a command carries in 32 bits, found among the ARM9's notes (NULL and a report when it isn't)
static const u8 *CommandPointer(u32 address, const u8 **end) {
    const u8 *pointer;

    *end = NULL;
    if (address == 0) {
        return NULL;
    }
    pointer = port_pxi_noted_pointer(PXI_FIFO_TAG_SOUND, address, end);
    if (pointer == NULL) {
        // Noted without a size outside the arena and static data (constant data): no bound, as on the DS
        pointer = port_pxi_noted_pointer(PXI_FIFO_TAG_SOUND, address, NULL);
        *end = NULL;
    }
    if (pointer == NULL && !sSnd.reportedPointer) {
        fprintf(stderr, "port: sound: an address the ARM9 sent, %#x, can't be found in host memory\n", address);
        sSnd.reportedPointer = TRUE;
    }
    return pointer;
}

// The alarms (snd_alarm.c, 0x03803208-0x03803404), on the ARM7's OS alarms

// The ARM7's OS tick now, in cycles of the virtual clock
static u64 TickCycles(u64 cycles) {
    return cycles / SND7_OS_TICK_CYCLES * SND7_OS_TICK_CYCLES;
}

static void AlarmInit(void) {
    int i;

    for (i = 0; i < SND7_ALARM_NUM; i++) {
        snd7_work.alarm[i].enable = 0;
        snd7_work.alarm[i].id = 0;
        snd7_work.alarm[i].armed = FALSE;
    }
}

// SND_SetupAlarm (0x03803234)
static void SetupAlarm(int alarmNo, u32 tick, u32 period, u32 id) {
    Snd7Alarm *alarm = &snd7_work.alarm[alarmNo];

    if (alarm->enable) {
        alarm->armed = FALSE;
        alarm->enable = 0;
    }
    alarm->tick = tick;
    alarm->period = period;
    alarm->id = (u8)id;
    alarm->count = 0;
}

// SND_StartAlarm (0x03803298): at tick from now, then each period if it has one
static void StartAlarm(int alarmNo) {
    Snd7Alarm *alarm = &snd7_work.alarm[alarmNo];

    if (alarm->enable) {
        alarm->armed = FALSE;
        alarm->enable = 0;
    }
    alarm->handlerArg = (u32)alarmNo | (u32)alarm->id << 8;
    alarm->armed = TRUE;
    alarm->periodic = alarm->period != 0;
    alarm->due = TickCycles(sSnd.now) + alarm->tick * SND7_OS_TICK_CYCLES;
    alarm->enable = 1;
}

// SND_StopAlarm (0x0380334c): a new ID, so that the ARM9 ignores a word of the old one on its way
static void StopAlarm(int alarmNo) {
    Snd7Alarm *alarm = &snd7_work.alarm[alarmNo];

    if (alarm->enable) {
        alarm->armed = FALSE;
        alarm->id++;
        alarm->enable = 0;
    }
}

// The alarm's handler (0x0380338c): the alarm's number and ID to the ARM9, with the ones a full FIFO held back
static void AlarmHandler(Snd7Alarm *alarm) {
    u32 arg = alarm->handlerArg;

    if (((arg >> 8) & 0xff) != alarm->id) {
        return;
    }
    if (alarm->count < 0xff) {
        alarm->count++;
    }
    while (alarm->count != 0) {
        if (port_pxi_pending() >= FIFO_DEPTH) {
            return;
        }
        port_pxi_reply(PXI_FIFO_TAG_SOUND, arg, FALSE);
        alarm->count--;
    }
}

static void Advance(u64 cycles);

// The OS core's timed work at the next alarm: the driver runs to it, which sends its word
static void AlarmWork(void) {
    Advance(port_os_cycles());
}

// The next alarm's moment for the OS core, or none
static void ScheduleAlarm(void) {
    u64 due = UINT64_MAX;
    int i;

    for (i = 0; i < SND7_ALARM_NUM; i++) {
        if (snd7_work.alarm[i].armed && snd7_work.alarm[i].due < due) {
            due = snd7_work.alarm[i].due;
        }
    }
    if (sSnd.alarmScheduled || due != UINT64_MAX) {
        port_os_set_timed_work(AlarmWork, due);
        sSnd.alarmScheduled = due != UINT64_MAX;
    }
}

static void FireAlarm(Snd7Alarm *alarm) {
    if (alarm->periodic) {
        alarm->due += alarm->period * SND7_OS_TICK_CYCLES;
    } else {
        alarm->armed = FALSE;
    }
    AlarmHandler(alarm);
}

// The interval timer (SND_StartIntervalTimer 0x03800610 and SND_StopIntervalTimer 0x03800658)
void snd7_start_interval_timer(void) {
    sSnd.timerRunning = TRUE;
    sSnd.nextUpdate = TickCycles(sSnd.now) + (u64)SND7_FIRST_UPDATE_TICKS * SND7_OS_TICK_CYCLES;
}

void snd7_stop_interval_timer(void) {
    sSnd.timerRunning = FALSE;
}

// The thread

// SND_UpdateSharedWork (0x03803180)
static void UpdateSharedWork(void) {
    u32 channelStatus = 0;
    u32 captureStatus = 0;
    int i;

    if (snd7_shared == NULL) {
        return;
    }
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        if (snd7_is_channel_active(i)) {
            channelStatus |= 1u << i;
        }
    }
    if (snd7_is_capture_active(0)) {
        captureStatus |= 1;
    }
    if (snd7_is_capture_active(1)) {
        captureStatus |= 2;
    }
    snd7_shared->channelStatus = (u16)channelStatus;
    snd7_shared->captureStatus = (u16)captureStatus;
}

// One pass of SndThread's loop. Its SND_CommandProc has nothing to do: the packets ran when they came, and count as the
// DS's would from here
static void RunThread(BOOL doPeriodicProc) {
    if (doPeriodicProc) {
        sSnd.runMix = (u32)sSnd.samples;
    }
    if (snd7_shared != NULL) {
        sSnd.runTag = snd7_shared->finishedCommandTag;
    }
    snd7_update_exchannel();
    snd7_seq_main(doPeriodicProc);
    snd7_exchannel_main(doPeriodicProc);
    UpdateSharedWork();
    (void)snd7_calc_random();
    if (sSnd.trace != NULL) {
        sSnd.trace(doPeriodicProc, sSnd.traceUser);
    }
}

static void FlushOutput(void) {
    if (sSnd.buffered != 0 && sSnd.output != NULL) {
        sSnd.output(sSnd.buffer, sSnd.buffered, sSnd.outputUser);
    }
    sSnd.buffered = 0;
}

// The mixer's next sample, kept for the output
static void Mix(void) {
    if (sSnd.buffered == sSnd.bufferFrames) {
        u32 frames = sSnd.bufferFrames != 0 ? sSnd.bufferFrames * 2 : OUTPUT_FRAMES_INITIAL;
        s16 *buffer = realloc(sSnd.buffer, (size_t)frames * 2 * sizeof(*buffer));

        if (buffer == NULL) {
            fprintf(stderr, "port: sound: no memory for the output's samples\n");
            abort();
        }
        sSnd.buffer = buffer;
        sSnd.bufferFrames = frames;
    }
    snd7_hw_mix(&sSnd.buffer[sSnd.buffered * 2]);
    sSnd.buffered++;
    sSnd.samples++;
}

// The driver and the sound unit up to the virtual clock's cycle: the mixer's samples, the periodic runs and the
// alarms, in the order they fall (at the same cycle: the sample, the run, then the alarms by number). At the cycle it
// last ran to, only an alarm started since can be due: one started for no ticks, which goes off at once, as the DS's
// OS alarm does when its moment has passed
static void Advance(u64 cycles) {
    if (!sSnd.booted || cycles < sSnd.now) {
        return;
    }
    for (;;) {
        u64 next = sSnd.nextSample;
        int event = -2;
        int i;

        if (sSnd.timerRunning && sSnd.nextUpdate < next) {
            next = sSnd.nextUpdate;
            event = -1;
        }
        for (i = 0; i < SND7_ALARM_NUM; i++) {
            if (snd7_work.alarm[i].armed && snd7_work.alarm[i].due < next) {
                next = snd7_work.alarm[i].due;
                event = i;
            }
        }
        if (next > cycles) {
            break;
        }
        // An alarm started for no ticks is due at the tick it started in, which may be before now
        if (next > sSnd.now) {
            sSnd.now = next;
        }
        if (event == -2) {
            Mix();
            sSnd.nextSample += SND7_CYCLES_PER_SAMPLE;
        } else if (event == -1) {
            sSnd.nextUpdate += (u64)SND7_UPDATE_TICKS * SND7_OS_TICK_CYCLES;
            RunThread(TRUE);
        } else {
            FireAlarm(&snd7_work.alarm[event]);
        }
    }
    sSnd.now = cycles;
    ScheduleAlarm();
}

// The commands (SND_CommandProc, 0x03803450)

// SND_ReadDriverInfo: the driver's state in the decomp's SNDDriverInfo. The DS copies its work as it is; the host
// fills the fields that the ARM9 reads (SND_ReadTrackInfo): the players' tracks, the tracks' parameters with their
// first 0x24 bytes as the DS has them, and the channels with their links as ARM7 addresses, which SND_ReadTrackInfo
// turns into offsets in the state from arm7Address. The decomp's state has room for 20 tracks of the driver's 32 (a
// guess at its layout): the tracks go into it in the order of the players and their tracks, and those past 20 read as
// none
static const SNDDriverChannel *ChannelLink(const SNDDriverInfo *info, const Snd7ExChannel *channel) {
    if (channel == NULL) {
        return NULL;
    }
    return (const SNDDriverChannel *)(uintptr_t)(info->arm7Address +
                                                 (u32)((uintptr_t)&info->channel[channel->myNo] - (uintptr_t)info));
}

void snd7_read_driver_info(SNDDriverInfo *info) {
    u8 slot[SND7_TRACK_NUM];
    u32 tracks = 0;
    u32 locked = snd7_get_locked_channel(0);
    int i;
    int j;

    memset(info, 0, sizeof(*info));
    memset(slot, SND_TRACK_INDEX_NONE, sizeof(slot));
    info->arm7Address = WORK_ADDRESS;
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        info->channel[i].myNo = (u8)i;
        info->channel[i].next = ChannelLink(info, snd7_work.channel[i].nextLink);
    }
    for (i = 0; i < SND7_PLAYER_NUM; i++) {
        const Snd7Player *player = &snd7_work.player[i];

        memcpy(info->player[i].unk_00, player, sizeof(info->player[i].unk_00));
        memcpy(info->player[i].unk_18, &player->tempo, 8);
        for (j = 0; j < SND7_PLAYER_TRACK_NUM; j++) {
            u8 trackNo = player->tracks[j];

            info->player[i].trackIndex[j] = SND_TRACK_INDEX_NONE;
            if (trackNo >= SND7_TRACK_NUM) {
                continue;
            }
            if (slot[trackNo] == SND_TRACK_INDEX_NONE && tracks < NELEMS(info->track)) {
                const Snd7Track *track = &snd7_work.track[trackNo];
                SNDDriverTrack *infoTrack = &info->track[tracks];

                memcpy(infoTrack, track, SND7_TRACK_PARAM_SIZE);
                infoTrack->channel = ChannelLink(info, track->channelList);
                slot[trackNo] = (u8)tracks++;
            }
            info->player[i].trackIndex[j] = slot[trackNo];
        }
    }
    memcpy(info->unk_11c4, &locked, sizeof(locked));
}

// The trace (snd7_output.h): the driver's work in the DS's layout (port/tools/oracle/sndtrace.h). The fields that the
// host keeps as the DS does are copied as they are, which these offsets, from the ARM7's code, hold to: a channel's up
// to its wave's parameters, a player's up to its bank, a track's up to its sequence
_Static_assert(offsetof(Snd7ExChannel, envDecay) == 0x10 && offsetof(Snd7ExChannel, volume) == 0x24 &&
                   offsetof(Snd7ExChannel, lfo) == 0x28 && offsetof(Snd7ExChannel, length) == 0x34 &&
                   offsetof(Snd7ExChannel, wave) == 0x38 && sizeof(Snd7WaveParam) == SND7_WAVE_PARAM_SIZE &&
                   offsetof(Snd7ExChannel, data) >= 0x44,
               "a channel's fields up to its wave's parameters are the DS's");
_Static_assert(offsetof(Snd7Player, tempo) == 0x18 && offsetof(Snd7Player, bank) >= SND7_PLAYER_PARAM_SIZE,
               "a player's fields up to its bank are the DS's");
_Static_assert(offsetof(Snd7Track, wait) == 0x20 && offsetof(Snd7Track, base) >= SND7_TRACK_PARAM_SIZE,
               "a track's fields up to its sequence are the DS's");
_Static_assert(sizeof(SNDSharedWork) == PBW_SNDTRACE_SHARED_SIZE, "the shared work is the DS's");

// The ARM7's address of the channel's sequencer callback (0x0380214c), the only one the driver gives a channel
#define SEQ_CHANNEL_CALLBACK_ADDRESS 0x0380214cu

// A link from the work into itself as the ARM7's address (a channel or a track; NULL is 0), else the pointer's low
// 32 bits
static u32 TraceLink(const void *pointer) {
    uintptr_t at = (uintptr_t)pointer;
    uintptr_t channels = (uintptr_t)snd7_work.channel;
    uintptr_t tracks = (uintptr_t)snd7_work.track;

    if (pointer == NULL) {
        return 0;
    }
    if (at >= channels && at < channels + sizeof(snd7_work.channel)) {
        return PBW_SNDTRACE_WORK_ADDRESS + (u32)((at - channels) / sizeof(Snd7ExChannel)) * PBW_SNDTRACE_CHANNEL_SIZE;
    }
    if (at >= tracks && at < tracks + sizeof(snd7_work.track)) {
        return PBW_SNDTRACE_WORK_ADDRESS + PBW_SNDTRACE_TRACKS +
               (u32)((at - tracks) / sizeof(Snd7Track)) * PBW_SNDTRACE_TRACK_SIZE;
    }
    return (u32)at;
}

static void TraceWork(u8 *work) {
    int i;
    int j;

    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        const Snd7ExChannel *channel = &snd7_work.channel[i];
        u8 *out = work + i * PBW_SNDTRACE_CHANNEL_SIZE;

        memcpy(out, channel, 0x44);
        // A square wave's duty where a sound's samples are: the DS keeps them in one field
        pbw_sndtrace_put32(out + 0x44, channel->type == SND7_EXCHANNEL_PSG ? channel->duty : TraceLink(channel->data));
        pbw_sndtrace_put32(out + 0x48, channel->callback != NULL ? SEQ_CHANNEL_CALLBACK_ADDRESS : 0);
        pbw_sndtrace_put32(out + 0x4c, TraceLink(channel->callbackData));
        pbw_sndtrace_put32(out + 0x50, TraceLink(channel->nextLink));
    }
    for (i = 0; i < SND7_PLAYER_NUM; i++) {
        const Snd7Player *player = &snd7_work.player[i];
        u8 *out = work + PBW_SNDTRACE_PLAYERS + i * PBW_SNDTRACE_PLAYER_SIZE;

        memcpy(out, player, SND7_PLAYER_PARAM_SIZE);
        pbw_sndtrace_put32(out + 0x20, TraceLink(player->bank));
    }
    for (i = 0; i < SND7_TRACK_NUM; i++) {
        const Snd7Track *track = &snd7_work.track[i];
        u8 *out = work + PBW_SNDTRACE_TRACKS + i * PBW_SNDTRACE_TRACK_SIZE;

        memcpy(out, track, SND7_TRACK_PARAM_SIZE);
        pbw_sndtrace_put32(out + 0x24, TraceLink(track->base));
        pbw_sndtrace_put32(out + 0x28, TraceLink(track->cur));
        for (j = 0; j < SND7_CALL_STACK_DEPTH; j++) {
            pbw_sndtrace_put32(out + 0x2c + j * 4, TraceLink(track->callStack[j]));
            out[0x38 + j] = track->loopCount[j];
        }
        out[0x3b] = track->callStackDepth;
        pbw_sndtrace_put32(out + 0x3c, TraceLink(track->channelList));
    }
    for (i = 0; i < SND7_ALARM_NUM; i++) {
        const Snd7Alarm *alarm = &snd7_work.alarm[i];
        u8 *out = work + PBW_SNDTRACE_ALARMS + i * PBW_SNDTRACE_ALARM_SIZE;

        out[0] = alarm->enable;
        out[1] = alarm->id;
        out[2] = alarm->count;
        pbw_sndtrace_put64(out + 4, alarm->tick);
        pbw_sndtrace_put64(out + 0xc, alarm->period);
    }
}

void port_snd7_set_trace(PortSnd7Trace trace, void *user) {
    sSnd.trace = trace;
    sSnd.traceUser = user;
}

void port_snd7_trace_record(uint8_t *record, int periodic) {
    int i;

    memset(record, 0, PBW_SNDTRACE_RECORD_SIZE);
    pbw_sndtrace_put64(record + PBW_SNDTRACE_CYCLE, sSnd.now);
    pbw_sndtrace_put32(record + PBW_SNDTRACE_SEED, snd7_random_state());
    pbw_sndtrace_put32(record + PBW_SNDTRACE_PERIODS, periodic ? 1 : 0);
    pbw_sndtrace_put32(record + PBW_SNDTRACE_RUN_MIX, sSnd.runMix);
    pbw_sndtrace_put32(record + PBW_SNDTRACE_LOCK, snd7_get_locked_channel(0));
    pbw_sndtrace_put32(record + PBW_SNDTRACE_WEAK_LOCK, snd7_get_locked_channel(1));
    for (i = 0; i < SND7_CHANNEL_NUM; i++) {
        Snd7HwPeek peek;

        snd7_hw_peek(i, &peek);
        pbw_sndtrace_put32(record + PBW_SNDTRACE_CNT + i * 4, peek.cnt);
        pbw_sndtrace_put32(record + PBW_SNDTRACE_START_MIX + i * 4, peek.startMix);
    }
    TraceWork(record + PBW_SNDTRACE_WORK);
    if (snd7_shared != NULL) {
        memcpy(record + PBW_SNDTRACE_SHARED, (const void *)snd7_shared, PBW_SNDTRACE_SHARED_SIZE);
    }
}

static void ForChannels(u32 mask, void (*apply)(int chNo, u32 a, u32 b), u32 a, u32 b) {
    int i;

    for (i = 0; i < SND7_CHANNEL_NUM && mask != 0; i++, mask >>= 1) {
        if (mask & 1) {
            apply(i, a, b);
        }
    }
}

static void ApplyTimer(int chNo, u32 timer, u32 unused) {
    snd7_set_channel_timer(chNo, (int)timer);
}

static void ApplyVolume(int chNo, u32 volume, u32 shift) {
    snd7_set_channel_volume(chNo, (int)volume, (int)shift);
}

static void ApplyPan(int chNo, u32 pan, u32 unused) {
    snd7_set_channel_pan(chNo, (int)pan);
}

static void ApplyStart(int chNo, u32 unused, u32 unused2) {
    snd7_start_channel(chNo);
}

static void ApplyStop(int chNo, u32 flags, u32 unused) {
    snd7_stop_channel(chNo, (s32)flags);
}

// SND_StartTimer and SND_StopTimer: the channels, capture units and alarms of the masks start or stop together
static void StartTimer(u32 chBitMask, u32 capBitMask, u32 alarmBitMask) {
    int i;

    ForChannels(chBitMask, ApplyStart, 0, 0);
    snd7_start_capture(capBitMask);
    for (i = 0; i < SND7_ALARM_NUM && alarmBitMask != 0; i++, alarmBitMask >>= 1) {
        if (alarmBitMask & 1) {
            StartAlarm(i);
        }
    }
    UpdateSharedWork();
}

static void StopTimer(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags) {
    int i;

    for (i = 0; i < SND7_ALARM_NUM && alarmBitMask != 0; i++, alarmBitMask >>= 1) {
        if (alarmBitMask & 1) {
            StopAlarm(i);
        }
    }
    ForChannels(chBitMask, ApplyStop, flags, 0);
    snd7_stop_capture(capBitMask);
    UpdateSharedWork();
}

// A sequence and its bank: the sequence's memory, and the bank's (its file, when the ARM9 noted its size)
static void StartSequence(const SNDCommand *command, BOOL start) {
    const u8 *seqEnd;
    const u8 *bankEnd;
    const u8 *seqBase = CommandPointer(command->arg[1], &seqEnd);
    const u8 *bank = CommandPointer(command->arg[3], &bankEnd);
    int playerNo = (int)command->arg[0];

    if (start) {
        snd7_start_seq(playerNo, seqBase, seqEnd, command->arg[2], bank, bankEnd);
    } else {
        snd7_prepare_seq(playerNo, seqBase, seqEnd, command->arg[2], bank, bankEnd);
    }
}

// A range the ARM9 invalidates, from its noted start
static void Invalidate(const SNDCommand *command) {
    const u8 *end;
    const u8 *start = port_pxi_noted_pointer(PXI_FIFO_TAG_SOUND, command->arg[0], &end);
    u32 size = command->arg[1] - command->arg[0];

    if (start != NULL) {
        end = (const u8 *)((uintptr_t)start + size);
    }
    switch (command->id) {
    case SND_COMMAND_INVALIDATE_SEQ:
        snd7_invalidate_seq(start, end, command->arg[0], command->arg[1]);
        break;
    case SND_COMMAND_INVALIDATE_BANK:
        snd7_invalidate_bank(start, end, command->arg[0], command->arg[1]);
        break;
    default:
        if (start != NULL) {
            snd7_invalidate_wave(start, end);
        }
        break;
    }
}

static BOOL ValidPlayer(u32 playerNo) {
    return playerNo < SND7_PLAYER_NUM;
}

// One command of the ARM9's packet, as SND_CommandProc's switch (0x03803450) runs it: the sequences' (start, prepare,
// stop, pause, skip, and the players' and tracks' parameters, mutes, channels and variables), the channels' the ARM9
// plays itself (a PCM wave, a square wave or noise, their timer, volume and pan, and the timers that start and stop
// channels, captures and alarms together), the capture units and alarms, the master volume, pan and output, the
// channel locks, the shared work's address, the ranges of data the ARM9 frees (Invalidate) and the driver's state for
// SND_ReadDriverInfo. An unknown command, or a player number out of range, does nothing
static void Execute(const SNDCommand *command) {
    u32 arg0 = command->arg[0];
    u32 arg1 = command->arg[1];
    u32 arg2 = command->arg[2];
    u32 arg3 = command->arg[3];

    switch (command->id) {
    case COMMAND_START_SEQ:
    case SND_COMMAND_PREPARE_SEQ:
        if (ValidPlayer(arg0)) {
            StartSequence(command, command->id == COMMAND_START_SEQ);
        }
        break;
    case SND_COMMAND_STOP_SEQ:
        if (ValidPlayer(arg0)) {
            snd7_stop_seq((int)arg0);
        }
        break;
    case SND_COMMAND_START_PREPARED_SEQ:
        if (ValidPlayer(arg0)) {
            snd7_start_prepared_seq((int)arg0);
        }
        break;
    case SND_COMMAND_PAUSE_SEQ:
        if (ValidPlayer(arg0)) {
            snd7_pause_seq((int)arg0, (BOOL)arg1);
        }
        break;
    case COMMAND_SKIP_SEQ:
        if (ValidPlayer(arg0)) {
            snd7_skip_seq((int)arg0, arg1);
        }
        break;
    case SND_COMMAND_PLAYER_PARAM:
        if (ValidPlayer(arg0)) {
            snd7_set_player_param((int)arg0, arg1, arg2, (int)arg3);
        }
        break;
    case SND_COMMAND_TRACK_PARAM:
        if (ValidPlayer(arg0 & 0xffffff)) {
            snd7_set_track_param((int)(arg0 & 0xffffff), arg1, arg2, arg3, (int)(arg0 >> 24));
        }
        break;
    case SND_COMMAND_MUTE_TRACK:
        if (ValidPlayer(arg0)) {
            snd7_set_track_mute((int)arg0, arg1, (int)arg2);
        }
        break;
    case SND_COMMAND_ALLOCATABLE_CHANNEL:
        if (ValidPlayer(arg0)) {
            snd7_set_track_allocatable_channel((int)arg0, arg1, arg2);
        }
        break;
    case COMMAND_PLAYER_LOCAL_VAR:
        if (snd7_shared != NULL && ValidPlayer(arg0) && arg1 < SND_PLAYER_VARIABLE_MAX) {
            snd7_shared->player[arg0].variable[arg1] = (s16)arg2;
        }
        break;
    case COMMAND_PLAYER_GLOBAL_VAR:
        if (snd7_shared != NULL && arg0 < SND_PLAYER_VARIABLE_MAX) {
            snd7_shared->globalVariable[arg0] = (s16)arg1;
        }
        break;
    case SND_COMMAND_START_TIMER:
        StartTimer(arg0, arg1, arg2);
        break;
    case SND_COMMAND_STOP_TIMER:
        StopTimer(arg0, arg1, arg2, arg3);
        break;
    case SND_COMMAND_SETUP_CHANNEL_PCM: {
        // The channel and timer; the address; the loop's length, the volume and shift; the loop's start, the pan, the
        // format and the loop mode. The DS keeps 27 bits of the address; the host looks its 32 up
        const u8 *end;
        const u8 *data = CommandPointer(arg1, &end);

        if ((arg0 & 0xffff) < SND7_CHANNEL_NUM) {
            snd7_setup_channel_pcm((int)(arg0 & 0xffff), data, end, (int)((arg3 >> 24) & 3), (int)((arg3 >> 26) & 3),
                                   (int)(arg3 & 0xffff), (int)(arg2 & 0x3fffff), (int)((arg2 >> 24) & 0x7f),
                                   (int)((arg2 >> 22) & 3), (int)(arg0 >> 16), (int)((arg3 >> 16) & 0x7f));
        }
        break;
    }
    case COMMAND_SETUP_CHANNEL_PSG:
        if (arg0 < SND7_CHANNEL_NUM) {
            snd7_setup_channel_psg((int)arg0, (int)arg3, (int)(arg1 & 0x7f), (int)((arg1 >> 8) & 3),
                                   (int)((arg2 >> 8) & 0xffff), (int)(arg2 & 0x7f));
        }
        break;
    case COMMAND_SETUP_CHANNEL_NOISE:
        if (arg0 < SND7_CHANNEL_NUM) {
            snd7_setup_channel_noise((int)arg0, (int)(arg1 & 0x7f), (int)((arg1 >> 8) & 3), (int)((arg2 >> 8) & 0xffff),
                                     (int)(arg2 & 0x7f));
        }
        break;
    case COMMAND_SETUP_CAPTURE:
        snd7_setup_capture((int)((arg2 >> 31) & 1), (int)((arg2 >> 30) & 1), arg0, (int)arg1, (arg2 >> 29) & 1,
                           (int)((arg2 >> 28) & 1), (int)((arg2 >> 27) & 1));
        break;
    case SND_COMMAND_SETUP_ALARM:
        if (arg0 < SND7_ALARM_NUM) {
            SetupAlarm((int)arg0, arg1, arg2, arg3);
        }
        break;
    case SND_COMMAND_CHANNEL_TIMER:
        ForChannels(arg0, ApplyTimer, arg1, 0);
        break;
    case SND_COMMAND_CHANNEL_VOLUME:
        ForChannels(arg0, ApplyVolume, arg1, arg2);
        break;
    case SND_COMMAND_CHANNEL_PAN:
        ForChannels(arg0, ApplyPan, arg1, 0);
        break;
    case COMMAND_SURROUND_DECAY:
        snd7_set_surround_decay((int)arg0);
        break;
    case SND_COMMAND_MASTER_VOLUME:
        snd7_set_master_volume((int)arg0);
        break;
    case SND_COMMAND_MASTER_PAN:
        snd7_set_master_pan((int)arg0);
        break;
    case SND_COMMAND_OUTPUT_SELECTOR:
        snd7_set_output_selector((int)arg0, (int)arg1, (int)arg2, (int)arg3);
        break;
    case SND_COMMAND_LOCK_CHANNEL:
        snd7_lock_channel(arg0, arg1);
        break;
    case SND_COMMAND_UNLOCK_CHANNEL:
        snd7_unlock_channel(arg0, arg1);
        break;
    case COMMAND_STOP_UNLOCKED_CHANNEL:
        snd7_stop_unlocked_channel(arg0, arg1);
        break;
    case SND_COMMAND_SHARED_WORK:
        if (arg0 == 0) {
            snd7_shared = NULL;
        } else if (SNDi_SharedWork != NULL && (u32)(uintptr_t)SNDi_SharedWork == arg0) {
            snd7_shared = SNDi_SharedWork;
        } else {
            const u8 *end;

            snd7_shared = (SNDSharedWork *)port_pxi_pointer(arg0, &end);
        }
        break;
    case SND_COMMAND_INVALIDATE_SEQ:
    case SND_COMMAND_INVALIDATE_BANK:
    case SND_COMMAND_INVALIDATE_WAVE:
        Invalidate(command);
        break;
    case SND_COMMAND_READ_DRIVER_INFO: {
        const u8 *end;
        SNDDriverInfo *info = (SNDDriverInfo *)CommandPointer(arg0, &end);

        if (info != NULL && (end == NULL || (u64)((uintptr_t)end - (uintptr_t)info) >= sizeof(*info))) {
            snd7_read_driver_info(info);
        }
        break;
    }
    default:
        break;
    }
}

// A word of the ARM9: 0 wakes the thread for a run that isn't periodic; any other is a packet, whose commands run now
static void SoundService(u32 tag, u32 data, BOOL err) {
    const SNDCommand *command = SNDi_Queue.readyHead;

    if (err) {
        return;
    }
    Advance(port_os_cycles());
    if (data == 0) {
        RunThread(FALSE);
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
    if (snd7_shared != NULL) {
        snd7_shared->finishedCommandTag++;
    }
    // The alarms the packet started or stopped
    ScheduleAlarm();
}

// Whether the DS's driver would still hold the ARM9's packet with the tag (SND_IsFinishedCommandTag's numbering, the
// shared work's count of finished packets), unrun, at the virtual clock's cycle: the host has run it, but the driver
// hasn't made a run since it came
BOOL snd7_holds_packet(u32 tag) {
    if (!sSnd.booted || snd7_shared == NULL) {
        return FALSE;
    }
    Advance(port_os_cycles());
    return (s32)(tag - sSnd.runTag) > 0 && (s32)(snd7_shared->finishedCommandTag - tag) >= 0;
}

// The driver's start at the cycle: SND_Init's (SND_CommandInit's shared work) and SndThread's (the channels, players,
// alarms, the output on at full master volume, and the interval timer)
static void Boot(u64 cycles) {
    PortSnd7Output output = sSnd.output;
    void *outputUser = sSnd.outputUser;
    s16 *buffer = sSnd.buffer;
    u32 bufferFrames = sSnd.bufferFrames;
    PortSnd7Trace trace = sSnd.trace;
    void *traceUser = sSnd.traceUser;

    memset(&sSnd, 0, sizeof(sSnd));
    memset(&snd7_work, 0, sizeof(snd7_work));
    sSnd.output = output;
    sSnd.outputUser = outputUser;
    sSnd.buffer = buffer;
    sSnd.bufferFrames = bufferFrames;
    sSnd.trace = trace;
    sSnd.traceUser = traceUser;
    snd7_shared = NULL;
    snd7_set_random(0x12345678);
    snd7_hw_reset();
    sSnd.booted = TRUE;
    sSnd.now = cycles;
    sSnd.nextSample = cycles;
    snd7_exchannel_init();
    snd7_seq_init();
    AlarmInit();
    snd7_enable();
    snd7_set_output_selector(0, 0, 0, 0);
    snd7_set_master_volume(127);
    snd7_start_interval_timer();
    // No alarm yet, whatever an earlier run left with the OS core
    port_os_set_timed_work(AlarmWork, UINT64_MAX);
}

void port_snd7_start(void) {
    Boot(port_os_cycles());
    port_pxi_register_service(PXI_FIFO_TAG_SOUND, SoundService);
}

void port_snd7_frame(u64 cycles) {
    Advance(cycles);
    FlushOutput();
}

void port_snd7_boot_at(uint64_t cycles) {
    Boot(cycles);
}

void port_snd7_advance(uint64_t cycles) {
    Advance(cycles);
    FlushOutput();
}

void port_snd7_set_output(PortSnd7Output output, void *user) {
    FlushOutput();
    sSnd.output = output;
    sSnd.outputUser = user;
}

uint64_t port_snd7_samples(void) {
    return sSnd.samples;
}
