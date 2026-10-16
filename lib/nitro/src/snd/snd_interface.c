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

static void func_0207d60c(int playerNo, u32 offset, u32 value, int size);
static void func_0207d628(int playerNo, u32 trackBitMask, u32 offset, u32 value, int size);
static void sndQueuePacket(u32 id, u32 arg0, u32 arg1, u32 arg2, u32 arg3);

// NitroSDK's SND_StopSeq
void func_0207d300(int playerNo) {
    sndQueuePacket(SND_COMMAND_STOP_SEQ, playerNo, 0, 0, 0);
}

// NitroSDK's SND_PrepareSeq
void func_0207d314(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank) {
    sndQueuePacket(SND_COMMAND_PREPARE_SEQ, playerNo, (u32)seqBase, seqOffset, (u32)bank);
}

// NitroSDK's SND_StartPreparedSeq
void func_0207d330(int playerNo) {
    sndQueuePacket(SND_COMMAND_START_PREPARED_SEQ, playerNo, 0, 0, 0);
}

// NitroSDK's SND_PauseSeq
void func_0207d344(int playerNo, BOOL flag) {
    sndQueuePacket(SND_COMMAND_PAUSE_SEQ, playerNo, flag, 0, 0);
}

// NitroSDK's SND_SetPlayerTempoRatio
void func_0207d35c(int playerNo, int ratio) {
    func_0207d60c(playerNo, SND_PLAYER_PARAM_TEMPO_RATIO, ratio, 2);
}

// NitroSDK's SND_SetPlayerVolume
void func_0207d36c(int playerNo, int volume) {
    func_0207d60c(playerNo, SND_PLAYER_PARAM_VOLUME, volume, 2);
}

// NitroSDK's SND_SetPlayerChannelPriority
void func_0207d37c(int playerNo, int prio) {
    func_0207d60c(playerNo, SND_PLAYER_PARAM_PRIO, prio, 1);
}

// NitroSDK's SND_SetTrackVolume
void func_0207d38c(int playerNo, u32 trackBitMask, int volume) {
    func_0207d628(playerNo, trackBitMask, SND_TRACK_PARAM_VOLUME, volume, 2);
}

// NitroSDK's SND_SetTrackPitch
void func_0207d39c(int playerNo, u32 trackBitMask, int pitch) {
    func_0207d628(playerNo, trackBitMask, SND_TRACK_PARAM_PITCH, pitch, 2);
}

// NitroSDK's SND_SetTrackPan
void func_0207d3ac(int playerNo, u32 trackBitMask, int pan) {
    func_0207d628(playerNo, trackBitMask, SND_TRACK_PARAM_PAN, pan, 1);
}

// NitroSDK's SND_SetTrackModDepth
void func_0207d3bc(int playerNo, u32 trackBitMask, int depth) {
    func_0207d628(playerNo, trackBitMask, SND_TRACK_PARAM_MOD_DEPTH, depth, 1);
}

// NitroSDK's SND_SetTrackModSpeed
void func_0207d3cc(int playerNo, u32 trackBitMask, int speed) {
    func_0207d628(playerNo, trackBitMask, SND_TRACK_PARAM_MOD_SPEED, speed, 1);
}

// NitroSDK's SND_SetTrackAllocatableChannel
void func_0207d3dc(int playerNo, u32 trackBitMask, u32 chBitFlag) {
    sndQueuePacket(SND_COMMAND_ALLOCATABLE_CHANNEL, playerNo, trackBitMask, chBitFlag, 0);
}

// NitroSDK's SND_StartTimer
void func_0207d3f4(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags) {
    sndQueuePacket(SND_COMMAND_START_TIMER, chBitMask, capBitMask, alarmBitMask, flags);
}

// NitroSDK's SND_StopTimer. The alarms that it stops get a new ID, so that a handler call that is already on its way
// to the ARM9 finds the alarm changed and does nothing
void func_0207d410(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags) {
    int alarmNo;
    u32 alarms = alarmBitMask;

    for (alarmNo = 0; alarmNo < SND_ALARM_MAX && alarms != 0; alarmNo++, alarms >>= 1) {
        if (alarms & 1) {
            func_0207db58(alarmNo);
        }
    }
    sndQueuePacket(SND_COMMAND_STOP_TIMER, chBitMask, capBitMask, alarmBitMask, flags);
}

// NitroSDK's SND_SetupAlarm
void func_0207d450(int alarmNo, u32 tick, u32 period, SNDAlarmHandler handler, void *arg) {
    u32 id = func_0207db6c(alarmNo, handler, arg);

    sndQueuePacket(SND_COMMAND_SETUP_ALARM, alarmNo, tick, period, id);
}

// NitroSDK's SND_SetTrackMute
void func_0207d474(int playerNo, u32 trackBitMask, BOOL flag) {
    sndQueuePacket(SND_COMMAND_MUTE_TRACK, playerNo, trackBitMask, flag ? SND_TRACK_MUTE_ON : SND_TRACK_MUTE_OFF, 0);
}

// NitroSDK's SND_SetTrackMuteEx
void func_0207d494(int playerNo, u32 trackBitMask, int muteType) {
    sndQueuePacket(SND_COMMAND_MUTE_TRACK, playerNo, trackBitMask, muteType, 0);
}

// NitroSDK's SND_LockChannel
void func_0207d4ac(u32 chBitMask, u32 flags) {
    sndQueuePacket(SND_COMMAND_LOCK_CHANNEL, chBitMask, flags, 0, 0);
}

// NitroSDK's SND_UnlockChannel
void func_0207d4c4(u32 chBitMask, u32 flags) {
    sndQueuePacket(SND_COMMAND_UNLOCK_CHANNEL, chBitMask, flags, 0, 0);
}

// swan's name for SND_SetChannelTimer
void sndSetRate(u32 chBitMask, int timer) {
    sndQueuePacket(SND_COMMAND_CHANNEL_TIMER, chBitMask, timer, 0, 0);
}

// swan's name for SND_SetChannelVolume
void sndSetVolume(u32 chBitMask, int volume, int shift) {
    sndQueuePacket(SND_COMMAND_CHANNEL_VOLUME, chBitMask, volume, shift, 0);
}

// swan's name for SND_SetChannelPan
void sndSetPan(u32 chBitMask, int pan) {
    sndQueuePacket(SND_COMMAND_CHANNEL_PAN, chBitMask, pan, 0, 0);
}

// swan's name for SND_SetupChannelPcm. The arguments share the words of the command: the channel and the timer, the
// address, the length of the loop with the volume and the data shift, and the start of the loop with the pan, the
// format and the loop mode
void sndQueuePacket_PlaySamples(int chNo, int format, const void *dataAddr, int loop, int loopStart, int loopLen,
                                int volume, int shift, int timer, int pan) {
    sndQueuePacket(SND_COMMAND_SETUP_CHANNEL_PCM, (timer << 16) | chNo, (u32)dataAddr,
                   (volume << 24) | (shift << 22) | loopLen, (loop << 26) | (format << 24) | (pan << 16) | loopStart);
}

// NitroSDK's SND_InvalidateSeqData
void func_0207d558(const void *start, const void *end) {
    sndQueuePacket(SND_COMMAND_INVALIDATE_SEQ, (u32)start, (u32)end, 0, 0);
}

// NitroSDK's SND_InvalidateBankData
void func_0207d570(const void *start, const void *end) {
    sndQueuePacket(SND_COMMAND_INVALIDATE_BANK, (u32)start, (u32)end, 0, 0);
}

// NitroSDK's SND_InvalidateWaveData
void func_0207d588(const void *start, const void *end) {
    sndQueuePacket(SND_COMMAND_INVALIDATE_WAVE, (u32)start, (u32)end, 0, 0);
}

// swan's name for SND_SetMasterVolume
void sndSetMasterVolume(int volume) {
    sndQueuePacket(SND_COMMAND_MASTER_VOLUME, volume, 0, 0, 0);
}

// NitroSDK's SND_SetOutputSelector
void func_0207d5b4(int left, int right, int channel1, int channel3) {
    sndQueuePacket(SND_COMMAND_OUTPUT_SELECTOR, left, right, channel1, channel3);
}

// NitroSDK's SND_SetMasterPan
void func_0207d5d0(int pan) {
    sndQueuePacket(SND_COMMAND_MASTER_PAN, pan, 0, 0, 0);
}

// NitroSDK's SND_ResetMasterPan
void func_0207d5e4(void) {
    sndQueuePacket(SND_COMMAND_MASTER_PAN, SND_MASTER_PAN_RESET, 0, 0, 0);
}

// NitroSDK's SND_ReadDriverInfo: has the driver copy its state to info
void func_0207d5f8(SNDDriverInfo *info) {
    sndQueuePacket(SND_COMMAND_READ_DRIVER_INFO, (u32)info, 0, 0, 0);
}

// NitroSDK's SNDi_SetPlayerParam: sets size bytes at offset of a player's state in the driver
static void func_0207d60c(int playerNo, u32 offset, u32 value, int size) {
    sndQueuePacket(SND_COMMAND_PLAYER_PARAM, playerNo, offset, value, size);
}

// NitroSDK's SNDi_SetTrackParam: the same for the tracks of a player that trackBitMask picks. The size travels with the
// player's number
static void func_0207d628(int playerNo, u32 trackBitMask, u32 offset, u32 value, int size) {
    sndQueuePacket(SND_COMMAND_TRACK_PARAM, playerNo | (size << 24), trackBitMask, offset, value);
}

// Fills the next command of the queue and puts it in
static void sndQueuePacket(u32 id, u32 arg0, u32 arg1, u32 arg2, u32 arg3) {
    SNDCommand *command = sndNewPacket(SND_COMMAND_BLOCK);

    if (command != NULL) {
        command->id = id;
        command->arg[0] = arg0;
        command->arg[1] = arg1;
        command->arg[2] = arg2;
        command->arg[3] = arg3;
        sndQueuePacket_(command);
    }
}
