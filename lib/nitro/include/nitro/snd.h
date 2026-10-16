#ifndef POKEBW2_NITRO_SND_H
#define POKEBW2_NITRO_SND_H

#include "types.h"

// NitroSDK's sound driver interface (SND), which queues commands for the ARM7's sound driver

// A command for the driver: the ID of what to do and its arguments, which are chained in the pool of snd_command.c
typedef struct SNDCommand SNDCommand;

struct SNDCommand {
    SNDCommand *next;
    u32 id;
    u32 arg[4];
};

// The queue of snd_command.c: the pool's free commands, the ready ones that the next flush sends as a packet, the
// packets that the ARM7 has not returned yet, and the tags that tell them apart. The pool's commands are chained, and
// a packet is the chain of the ready commands
#define SND_PACKET_MAX 8

typedef struct SNDCommandQueue {
    SNDCommand *freeHead;  // the pool's commands that nothing uses
    u32 receivedTag;       // the tag of the last packet taken back from the ARM7
    SNDCommand *readyHead; // the commands pushed since the last flush
    SNDCommand *readyTail;
    SNDCommand *freeTail;
    int sentRead;  // where the next packet to come back is in SNDi_Sent
    int sentWrite; // where the next packet to go is
    int sentCount; // how many packets are on their way
    u32 nextTag;   // the tag of the packet that the ready commands will make
} SNDCommandQueue;

// They are in the .bss of nitro_snd_main.c: the 32-aligned pool and shared work of snd_command.c start after them
extern SNDCommandQueue SNDi_Queue;
extern SNDCommand *SNDi_Sent[SND_PACKET_MAX + 1];

// The commands that the interface sends
#define SND_COMMAND_STOP_SEQ 1
#define SND_COMMAND_PREPARE_SEQ 2
#define SND_COMMAND_START_PREPARED_SEQ 3
#define SND_COMMAND_PAUSE_SEQ 4
#define SND_COMMAND_PLAYER_PARAM 6
#define SND_COMMAND_TRACK_PARAM 7
#define SND_COMMAND_MUTE_TRACK 8
#define SND_COMMAND_ALLOCATABLE_CHANNEL 9
#define SND_COMMAND_START_TIMER 0xc
#define SND_COMMAND_STOP_TIMER 0xd
#define SND_COMMAND_SETUP_CHANNEL_PCM 0xe
#define SND_COMMAND_SETUP_ALARM 0x12
#define SND_COMMAND_CHANNEL_TIMER 0x13
#define SND_COMMAND_CHANNEL_VOLUME 0x14
#define SND_COMMAND_CHANNEL_PAN 0x15
#define SND_COMMAND_MASTER_VOLUME 0x17
#define SND_COMMAND_MASTER_PAN 0x18
#define SND_COMMAND_OUTPUT_SELECTOR 0x19
#define SND_COMMAND_LOCK_CHANNEL 0x1a
#define SND_COMMAND_UNLOCK_CHANNEL 0x1b
#define SND_COMMAND_SHARED_WORK 0x1d
#define SND_COMMAND_INVALIDATE_SEQ 0x1e
#define SND_COMMAND_INVALIDATE_BANK 0x1f
#define SND_COMMAND_INVALIDATE_WAVE 0x20
#define SND_COMMAND_READ_DRIVER_INFO 0x21

// What the driver has: players, 16 tracks for each and 16 variables, channels and alarms
#define SND_PLAYER_MAX 16
#define SND_PLAYER_TRACK_MAX 16
#define SND_PLAYER_VARIABLE_MAX 16
#define SND_CHANNEL_MAX 16
#define SND_ALARM_MAX 8

// Channels: SND_SetChannelVolume, SND_SetChannelPan and SND_SetupChannelPcm, under swan's names
#define SND_WAVE_FORMAT_PCM8 0
#define SND_WAVE_FORMAT_PCM16 1

#define SND_CHANNEL_LOOP_REPEAT 1

#define SND_CHANNEL_VOLUME_MAX 127
#define SND_CHANNEL_PAN_CENTER 64

void sndSetVolume(u32 chBitMask, int volume, int shift);
void sndSetPan(u32 chBitMask, int pan);
void sndQueuePacket_PlaySamples(int chNo, int format, const void *dataAddr, int loop, int loopStart, int loopLen,
                                int volume, int shift, int timer, int pan);

// Timers and alarms: NitroSDK's SND_StartTimer, SND_StopTimer and SND_SetupAlarm, by their code
typedef void (*SNDAlarmHandler)(void *arg);

void func_0207d3f4(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags);
void func_0207d410(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags);
void func_0207d450(int alarmNo, u32 tick, u32 period, SNDAlarmHandler handler, void *arg);

// The alarms' handlers, which snd_alarm.c keeps: NitroSDK's SND_AlarmInit, SNDi_IncAlarmId, SNDi_SetAlarmHandler, which
// returns the alarm's new ID, and SNDi_CallAlarmHandler, which takes the number and the ID that the ARM7 sends
void func_0207db34(void);
void func_0207db58(int alarmNo);
u8 func_0207db6c(int alarmNo, SNDAlarmHandler handler, void *arg);
void func_0207db8c(int data);

// Commands: NitroSDK's SND_FlushCommand and SND_WaitForCommandProc by their code, and SND_GetCurrentCommandTag under
// swan's name
#define SND_COMMAND_BLOCK 1
// Flush: wake the ARM7 up for the packet at once
#define SND_COMMAND_IMMEDIATE 2

BOOL func_0207d864(u32 flags);
void func_0207d96c(u32 tag);
u32 sndGetSentPacketCount(void);

// NitroSDK's SND_CalcChannelVolume: a volume in decibels as a channel volume in the low byte and a data shift in the
// high one
#define SND_CHANNEL_VOLUME(v) ((u8)(v))
#define SND_CHANNEL_DATASHIFT(v) ((v) >> 8)

#define SND_DB_MIN (-723)

u16 func_0207dd2c(int dB);

// Sound data files: a sequence, a bank of instruments and a wave archive, each a binary file with one data block
typedef struct SNDBinaryFileHeader {
    char signature[4];
    u16 byteOrder;
    u16 version;
    u32 fileSize;
    u16 headerSize;
    u16 dataBlocks;
} SNDBinaryFileHeader;

typedef struct SNDBinaryBlockHeader {
    u32 kind;
    u32 size;
} SNDBinaryBlockHeader;

typedef struct SNDSequenceData {
    SNDBinaryFileHeader fileHeader;
    SNDBinaryBlockHeader blockHeader;
    u32 baseOffset;
    u32 data[];
} SNDSequenceData;

typedef struct SNDWaveData SNDWaveData;

typedef struct SNDWaveArc SNDWaveArc;
typedef struct SNDWaveArcLink SNDWaveArcLink;

// A wave archive: its waves' offsets in the file. Loaded in the heap, the table holds the waves' addresses
struct SNDWaveArc {
    SNDBinaryFileHeader fileHeader;
    SNDBinaryBlockHeader blockHeader;
    SNDWaveArcLink *topLink; // the banks that use this archive
    u32 reserved[7];
    u32 waveCount;
    u32 offsetTable[];
};

// A bank's slot for one of its wave archives, chained in the archive's list of users
struct SNDWaveArcLink {
    SNDWaveArc *waveArc;
    SNDWaveArcLink *next;
};

#define SND_BANK_WAVEARC_MAX 4

// A bank: the offset in the file of each of its instruments
typedef struct SNDBankData {
    SNDBinaryFileHeader fileHeader;
    SNDBinaryBlockHeader blockHeader;
    SNDWaveArcLink waveArcLink[SND_BANK_WAVEARC_MAX];
    u32 instCount;
    u32 instOffset[];
} SNDBankData;

// The kinds of program of a bank. A drum set and a key split hold instruments of their own
#define SND_INST_INVALID 0
#define SND_INST_PCM 1
#define SND_INST_PSG 2
#define SND_INST_NOISE 3
#define SND_INST_DIRECT_PCM 4
#define SND_INST_NULL 5
#define SND_INST_DRUM_SET 16
#define SND_INST_KEY_SPLIT 17

typedef struct SNDInstParam {
    u16 wave[2];
    u8 original_key;
    u8 attack;
    u8 decay;
    u8 sustain;
    u8 release;
    u8 pan;
} SNDInstParam;

typedef struct SNDInstData {
    u8 type;
    u8 padding_;
    SNDInstParam param;
} SNDInstData;

// The instruments of a drum set, one for each key from min to max
typedef struct SNDDrumSet {
    u8 min;
    u8 max;
    SNDInstData instData[];
} SNDDrumSet;

// The instruments of a key split: the highest key of each, and the instruments, until a key of 0
typedef struct SNDKeySplit {
    u8 key[8];
    SNDInstData instData[];
} SNDKeySplit;

// Where SND_GetNextInstData goes on from: the program, then the instrument inside a drum set or key split
typedef struct SNDInstPos {
    u32 prgNo;
    u32 index;
} SNDInstPos;

// NitroSDK's SND_InvalidateSeqData, SND_InvalidateBankData and SND_InvalidateWaveData, which stop what plays from the
// range, then SND_AssignWaveArc, SND_DestroyBank, SND_DestroyWaveArc, SND_GetFirstInstDataPos, SND_GetNextInstData,
// SND_GetWaveDataCount, SND_SetWaveDataAddress and SND_GetWaveDataAddress, by their code
void func_0207d558(const void *start, const void *end);
void func_0207d570(const void *start, const void *end);
void func_0207d588(const void *start, const void *end);
void func_0207dd80(SNDBankData *bank, int index, SNDWaveArc *waveArc);
void func_0207ddfc(SNDBankData *bank);
void func_0207de50(SNDWaveArc *waveArc);
SNDInstPos func_0207de7c(const SNDBankData *bank);
BOOL func_0207de8c(const SNDBankData *bank, SNDInstData *inst, SNDInstPos *pos);
u32 func_0207df80(const SNDWaveArc *waveArc);
void func_0207df84(SNDWaveArc *waveArc, int index, const SNDWaveData *address);
const SNDWaveData *func_0207dfa4(const SNDWaveArc *waveArc, int index);

// The driver: NitroSDK's SND_Init, SNDi_LockMutex and SNDi_UnlockMutex, which guard the commands of a caller,
// SND_CommandInit, SND_RecvCommandReply, which returns the commands of the next packet the ARM7 is done with or NULL,
// SND_AllocCommand (swan's sndNewPacket) and SND_PushCommand (sndQueuePacket_), and SND_IsFinishedCommandTag, by their
// code
#define SND_COMMAND_NOBLOCK 0

void func_0207d670(void);
void func_0207d698(void);
void func_0207d6a8(void);
void func_0207d6b8(void);
SNDCommand *func_0207d73c(u32 flags);
SNDCommand *sndNewPacket(u32 flags);
void sndQueuePacket_(SNDCommand *command);
BOOL func_0207d9d0(u32 tag);

// Sequences on a player: NitroSDK's SND_StopSeq, SND_PrepareSeq, SND_StartPreparedSeq, SND_PauseSeq,
// SND_SetPlayerTempoRatio, SND_SetPlayerVolume and SND_SetPlayerChannelPriority, by their code
void func_0207d300(int playerNo);
void func_0207d314(int playerNo, const void *seqBase, u32 seqOffset, const SNDBankData *bank);
void func_0207d330(int playerNo);
void func_0207d344(int playerNo, BOOL flag);
void func_0207d35c(int playerNo, int ratio);
void func_0207d36c(int playerNo, int volume);
void func_0207d37c(int playerNo, int prio);

// The tracks of a player: NitroSDK's SND_SetTrackVolume, SND_SetTrackPitch, SND_SetTrackPan, SND_SetTrackModDepth and
// SND_SetTrackModSpeed (the depth and speed of the track's LFO), then SND_SetTrackAllocatableChannel, SND_SetTrackMute
// and SND_SetTrackMuteEx, by their code. The types of mute are the values that the first sends for a flag
#define SND_TRACK_MUTE_OFF 0
#define SND_TRACK_MUTE_ON 3
#define SND_TRACK_PAN_CENTER 64

void func_0207d38c(int playerNo, u32 trackBitMask, int volume);
void func_0207d39c(int playerNo, u32 trackBitMask, int pitch);
void func_0207d3ac(int playerNo, u32 trackBitMask, int pan);
void func_0207d3bc(int playerNo, u32 trackBitMask, int depth);
void func_0207d3cc(int playerNo, u32 trackBitMask, int speed);
void func_0207d3dc(int playerNo, u32 trackBitMask, u32 chBitFlag);
void func_0207d474(int playerNo, u32 trackBitMask, BOOL flag);
void func_0207d494(int playerNo, u32 trackBitMask, int muteType);

// NitroSDK's SND_LockChannel and SND_UnlockChannel, by their code; SND_SetChannelTimer and SND_SetMasterVolume under
// swan's names; SND_SetOutputSelector, SND_SetMasterPan and SND_ResetMasterPan by their code
#define SND_CHANNEL_TIMER_MIN 0x10
#define SND_CHANNEL_TIMER_MAX 0xffff
#define SND_CHANNEL_LOOP_1SHOT 2
#define SND_CHANNEL_DATASHIFT_NONE 0
// The channels' timer clock, half the system clock
#define SND_TIMER_CLOCK 16756991

void func_0207d4ac(u32 chBitMask, u32 flags);
void func_0207d4c4(u32 chBitMask, u32 flags);
void sndSetRate(u32 chBitMask, int timer);
void sndSetMasterVolume(int volume);
void func_0207d5b4(int left, int right, int channel1, int channel3);
void func_0207d5d0(int pan);
void func_0207d5e4(void);
#define SND_MASTER_PAN_RESET (-1)

// What the driver reports: NitroSDK's SND_ReadDriverInfo, which copies the driver's state to info,
// SND_GetPlayerStatus and SND_GetChannelStatus, bit masks of the players and channels at work, and SND_ReadTrackInfo
// from a copied state, by their code, and SND_GetPlayerTickCounter

// The work that the driver and the ARM9 share. The driver writes it, and the functions below read it
#define SND_VARIABLE_UNSET (-1)

typedef struct SNDSharedPlayer {
    s16 variable[SND_PLAYER_VARIABLE_MAX];
    u32 tickCounter;
} SNDSharedPlayer;

typedef struct SNDSharedWork {
    u32 finishedCommandTag;
    u32 playerStatus;
    u16 channelStatus;
    u16 captureStatus;
    u32 padding_0c[5];
    SNDSharedPlayer player[SND_PLAYER_MAX];
    s16 globalVariable[SND_PLAYER_VARIABLE_MAX];
} SNDSharedWork;

extern SNDSharedWork *SNDi_SharedWork;

// The driver's state, as SND_ReadDriverInfo copies it. Its pointers are the ARM7's addresses, which the address of
// the state in the ARM7 turns into offsets. Only the fields that SND_ReadTrackInfo reads are named. The number of
// tracks, 20, and the size of a channel, 0x54, are what fill the space between the players and the address when the
// channel holds its link at 0x50: a guess
typedef struct SNDDriverChannel {
    u8 myNo;
    u8 unk_01[0x4f];
    const struct SNDDriverChannel *next;
} SNDDriverChannel;

typedef struct SNDDriverTrack {
    u8 unk_00[2];
    u16 prgNo;
    u8 volume;
    u8 volume2;
    s8 pitchBend;
    u8 bendRange;
    s8 pan;
    u8 unk_09[0xa];
    s8 transpose;
    u8 unk_14[0x28];
    const SNDDriverChannel *channel;
} SNDDriverTrack;

#define SND_TRACK_INDEX_NONE 0xff

typedef struct SNDDriverPlayer {
    u8 unk_00[8];
    u8 trackIndex[SND_PLAYER_TRACK_MAX];
    u8 unk_18[0xc];
} SNDDriverPlayer;

typedef struct SNDDriverInfo {
    u8 unk_0000[0x540];
    SNDDriverPlayer player[SND_PLAYER_MAX];
    SNDDriverTrack track[20];
    SNDDriverChannel channel[SND_CHANNEL_MAX];
    u32 arm7Address;
    u8 unk_11c4[0x1c];
} SNDDriverInfo;

// A track's state, as NNS_SndPlayerReadDriverTrackInfo reads it
typedef struct SNDTrackInfo {
    u16 prgNo;
    u8 volume;
    u8 volume2;
    s8 pitchBend;
    u8 bendRange;
    u8 pan;
    s8 transpose;
    u8 unk_08;
    u8 chCount;
    u8 channel[SND_CHANNEL_MAX];
} SNDTrackInfo;

void func_0207d5f8(SNDDriverInfo *info);
u32 func_0207dbb8(void);
u32 func_0207dbd0(void);
u32 SND_GetPlayerTickCounter(int playerNo);
BOOL func_0207dc0c(const SNDDriverInfo *driverInfo, int playerNo, int trackNo, SNDTrackInfo *trackInfo);
u32 func_0207dcc4(void);
void func_0207dcdc(SNDSharedWork *work);

// The volume in decibels, as a player or track takes it, of each linear volume 0 to 127. NitroSDK's
// SNDi_DecibelTable, under swan's name
extern const s16 VOLUME_DB_TABLE[128];

#endif // POKEBW2_NITRO_SND_H
