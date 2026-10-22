// NitroSDK's SND (snd_interface.c, a guess from the SDK's file names): the functions that the ARM9 calls to drive the
// ARM7's sound driver. Each puts a command with its arguments in the queue that snd_command.c sends.
#include "nitro/snd.h"

// The offsets in the driver's player and track that the setters write
#define SND_PLAYER_PARAM_PRIO 4
#define SND_PLAYER_PARAM_VOLUME 6
#define SND_PLAYER_PARAM_TEMPO_RATIO 0x1a
#define SND_TRACK_PARAM_PAN 9
#define SND_TRACK_PARAM_VOLUME 0xa
#define SND_TRACK_PARAM_PITCH 0xc
#define SND_TRACK_PARAM_MOD_SPEED 0x19
#define SND_TRACK_PARAM_MOD_DEPTH 0x1a

static void SNDi_SetPlayerParam(int playerNo, u32 offset, u32 value, int size);
static void SNDi_SetTrackParam(int playerNo, u32 trackBitMask, u32 offset, u32 value, int size);
static void PushCommand_impl(u32 id, u32 arg0, u32 arg1, u32 arg2, u32 arg3);

void SND_StopSeq(int playerNo) {
    PushCommand_impl(SND_COMMAND_STOP_SEQ, playerNo, 0, 0, 0);
}

void SND_PrepareSeq(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank) {
    PushCommand_impl(SND_COMMAND_PREPARE_SEQ, playerNo, (u32)seqBase, seqOffset, (u32)bank);
}

void SND_StartPreparedSeq(int playerNo) {
    PushCommand_impl(SND_COMMAND_START_PREPARED_SEQ, playerNo, 0, 0, 0);
}

void SND_PauseSeq(int playerNo, BOOL flag) {
    PushCommand_impl(SND_COMMAND_PAUSE_SEQ, playerNo, flag, 0, 0);
}

void SND_SetPlayerTempoRatio(int playerNo, int ratio) {
    SNDi_SetPlayerParam(playerNo, SND_PLAYER_PARAM_TEMPO_RATIO, ratio, 2);
}

void SND_SetPlayerVolume(int playerNo, int volume) {
    SNDi_SetPlayerParam(playerNo, SND_PLAYER_PARAM_VOLUME, volume, 2);
}

void SND_SetPlayerChannelPriority(int playerNo, int prio) {
    SNDi_SetPlayerParam(playerNo, SND_PLAYER_PARAM_PRIO, prio, 1);
}

void SND_SetTrackVolume(int playerNo, u32 trackBitMask, int volume) {
    SNDi_SetTrackParam(playerNo, trackBitMask, SND_TRACK_PARAM_VOLUME, volume, 2);
}

void SND_SetTrackPitch(int playerNo, u32 trackBitMask, int pitch) {
    SNDi_SetTrackParam(playerNo, trackBitMask, SND_TRACK_PARAM_PITCH, pitch, 2);
}

void SND_SetTrackPan(int playerNo, u32 trackBitMask, int pan) {
    SNDi_SetTrackParam(playerNo, trackBitMask, SND_TRACK_PARAM_PAN, pan, 1);
}

void SND_SetTrackModDepth(int playerNo, u32 trackBitMask, int depth) {
    SNDi_SetTrackParam(playerNo, trackBitMask, SND_TRACK_PARAM_MOD_DEPTH, depth, 1);
}

void SND_SetTrackModSpeed(int playerNo, u32 trackBitMask, int speed) {
    SNDi_SetTrackParam(playerNo, trackBitMask, SND_TRACK_PARAM_MOD_SPEED, speed, 1);
}

void SND_SetTrackAllocatableChannel(int playerNo, u32 trackBitMask, u32 chBitFlag) {
    PushCommand_impl(SND_COMMAND_ALLOCATABLE_CHANNEL, playerNo, trackBitMask, chBitFlag, 0);
}

void SND_StartTimer(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags) {
    PushCommand_impl(SND_COMMAND_START_TIMER, chBitMask, capBitMask, alarmBitMask, flags);
}

// NitroSDK's SND_StopTimer. The alarms that it stops get a new ID, so that a handler call that is already on its way
// to the ARM9 finds the alarm changed and does nothing
void SND_StopTimer(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags) {
    int alarmNo;
    u32 alarms = alarmBitMask;

    for (alarmNo = 0; alarmNo < SND_ALARM_NUM && alarms != 0; alarmNo++, alarms >>= 1) {
        if (alarms & 1) {
            SNDi_IncAlarmId(alarmNo);
        }
    }
    PushCommand_impl(SND_COMMAND_STOP_TIMER, chBitMask, capBitMask, alarmBitMask, flags);
}

void SND_SetupAlarm(int alarmNo, u32 tick, u32 period, SNDAlarmHandler handler, void *arg) {
    u32 id = SNDi_SetAlarmHandler(alarmNo, handler, arg);

    PushCommand_impl(SND_COMMAND_SETUP_ALARM, alarmNo, tick, period, id);
}

void SND_SetTrackMute(int playerNo, u32 trackBitMask, BOOL flag) {
    PushCommand_impl(SND_COMMAND_MUTE_TRACK, playerNo, trackBitMask, flag ? SND_SEQ_MUTE_STOP : SND_SEQ_MUTE_OFF, 0);
}

void SND_SetTrackMuteEx(int playerNo, u32 trackBitMask, SNDSeqMute muteType) {
    PushCommand_impl(SND_COMMAND_MUTE_TRACK, playerNo, trackBitMask, muteType, 0);
}

void SND_LockChannel(u32 chBitMask, u32 flags) {
    PushCommand_impl(SND_COMMAND_LOCK_CHANNEL, chBitMask, flags, 0, 0);
}

void SND_UnlockChannel(u32 chBitMask, u32 flags) {
    PushCommand_impl(SND_COMMAND_UNLOCK_CHANNEL, chBitMask, flags, 0, 0);
}

// NitroSDK's SND_SetChannelTimer
void SND_SetChannelTimer(u32 chBitMask, int timer) {
    PushCommand_impl(SND_COMMAND_CHANNEL_TIMER, chBitMask, timer, 0, 0);
}

// NitroSDK's SND_SetChannelVolume
void SND_SetChannelVolume(u32 chBitMask, int volume, SNDChannelDataShift shift) {
    PushCommand_impl(SND_COMMAND_CHANNEL_VOLUME, chBitMask, volume, shift, 0);
}

// NitroSDK's SND_SetChannelPan
void SND_SetChannelPan(u32 chBitMask, int pan) {
    PushCommand_impl(SND_COMMAND_CHANNEL_PAN, chBitMask, pan, 0, 0);
}

// NitroSDK's SND_SetupChannelPcm. The arguments share the words of the command: the channel and the timer, the
// address, the length of the loop with the volume and the data shift, and the start of the loop with the pan, the
// format and the loop mode
void SND_SetupChannelPcm(int chNo, SNDWaveFormat format, const void *dataAddr, SNDChannelLoop loop, int loopStart,
                         int loopLen, int volume, SNDChannelDataShift shift, int timer, int pan) {
    PushCommand_impl(SND_COMMAND_SETUP_CHANNEL_PCM, (timer << 16) | chNo, (u32)dataAddr,
                     (volume << 24) | (shift << 22) | loopLen, (loop << 26) | (format << 24) | (pan << 16) | loopStart);
}

void SND_InvalidateSeqData(const void *start, const void *end) {
    PushCommand_impl(SND_COMMAND_INVALIDATE_SEQ, (u32)start, (u32)end, 0, 0);
}

void SND_InvalidateBankData(const void *start, const void *end) {
    PushCommand_impl(SND_COMMAND_INVALIDATE_BANK, (u32)start, (u32)end, 0, 0);
}

void SND_InvalidateWaveData(const void *start, const void *end) {
    PushCommand_impl(SND_COMMAND_INVALIDATE_WAVE, (u32)start, (u32)end, 0, 0);
}

// NitroSDK's SND_SetMasterVolume
void SND_SetMasterVolume(int volume) {
    PushCommand_impl(SND_COMMAND_MASTER_VOLUME, volume, 0, 0, 0);
}

void SND_SetOutputSelector(SNDOutput left, SNDOutput right, SNDChannelOut channel1, SNDChannelOut channel3) {
    PushCommand_impl(SND_COMMAND_OUTPUT_SELECTOR, left, right, channel1, channel3);
}

void SND_SetMasterPan(int pan) {
    PushCommand_impl(SND_COMMAND_MASTER_PAN, pan, 0, 0, 0);
}

void SND_ResetMasterPan(void) {
    PushCommand_impl(SND_COMMAND_MASTER_PAN, SND_MASTER_PAN_RESET, 0, 0, 0);
}

// NitroSDK's SND_ReadDriverInfo: has the driver copy its state to info
void SND_ReadDriverInfo(SNDDriverInfo *info) {
    PushCommand_impl(SND_COMMAND_READ_DRIVER_INFO, (u32)info, 0, 0, 0);
}

// NitroSDK's SNDi_SetPlayerParam: sets size bytes at offset of a player's state in the driver
static void SNDi_SetPlayerParam(int playerNo, u32 offset, u32 value, int size) {
    PushCommand_impl(SND_COMMAND_PLAYER_PARAM, playerNo, offset, value, size);
}

// NitroSDK's SNDi_SetTrackParam: the same for the tracks of a player that trackBitMask picks. The size travels with the
// player's number
static void SNDi_SetTrackParam(int playerNo, u32 trackBitMask, u32 offset, u32 value, int size) {
    PushCommand_impl(SND_COMMAND_TRACK_PARAM, playerNo | (size << 24), trackBitMask, offset, value);
}

// Fills the next command of the queue and puts it in
static void PushCommand_impl(u32 id, u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    SNDCommand *command = SND_AllocCommand(SND_COMMAND_BLOCK);

    if (command != NULL) {
        command->id = id;
        command->arg[0] = arg0;
        command->arg[1] = arg1;
        command->arg[2] = arg2;
        command->arg[3] = arg3;
        SND_PushCommand(command);
    }
}
