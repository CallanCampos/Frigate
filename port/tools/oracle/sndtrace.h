// Sound traces ("pbwsnd 1"): the state of the ARM7's sound driver after each run of its thread, as the melonDS oracle
// (pbw-oracle --sound-trace) and the port (--sound-trace) write it, for sound.py to compare. The format is in
// port/tools/oracle/README.md. This file is plain C11 with no dependency on melonDS or the decomp's headers, so that
// the port can include it as it is.
//
// The driver is TwlSDK 5.3's stock SND driver, the same ARM7 binary in Black 2 and White 2. Its thread (SndThread)
// waits for a message, a periodic alarm every 2728 ticks of 64 cycles or the ARM9's wake-up word, and each run ends
// with a draw from the driver's random generator. A record holds the driver's work in the DS's layout (SNDi_Work, the
// 0x1180 bytes that SND_ReadDriverInfo copies to the ARM9), the shared work, and what the sound unit's registers say.
// The links that stay inside the work (a channel's next one, a track's channel list, a channel's track) are the
// ARM7's addresses on both programs; the addresses of data the ARM9 loaded (a sequence, a bank, a wave) are the DS's
// on the oracle and the low 32 bits of the host's pointers on the port, so only their differences compare (a track's
// position in its sequence, cur - base)
#ifndef PORT_TOOLS_ORACLE_SNDTRACE_H
#define PORT_TOOLS_ORACLE_SNDTRACE_H

#include <stdint.h>

// The file starts with this line, 9 bytes, then holds records of PBW_SNDTRACE_RECORD_SIZE bytes, little-endian
#define PBW_SNDTRACE_MAGIC "pbwsnd 1\n"
#define PBW_SNDTRACE_MAGIC_SIZE 9
#define PBW_SNDTRACE_RECORD_SIZE 0x14c0

// A record's fields, by offset
#define PBW_SNDTRACE_CYCLE 0x00   // u64: the bus clock's cycles since power-on (the port: since its main())
#define PBW_SNDTRACE_FRAME 0x08   // u32: the video frame the run fell in, from 1 (frame N ends with the N-th VBlank)
#define PBW_SNDTRACE_COUNTER 0x0c // u32: GFL's frame count (sVBlankCount) then
#define PBW_SNDTRACE_SEED 0x10    // u32: the driver's random generator (SND_CalcRandom's state) after the run
#define PBW_SNDTRACE_PERIODS 0x14 // u32: the periodic runs the record covers: 1 for a periodic run, 0 for a wake-up
#define PBW_SNDTRACE_LOCK 0x18    // u32: the channels locked (SND_LockChannel), and those locked weakly
#define PBW_SNDTRACE_WEAK_LOCK 0x1c
#define PBW_SNDTRACE_CNT 0x20      // u32[16]: SOUNDxCNT: volume, shift, hold, pan, duty, repeat, format, start
#define PBW_SNDTRACE_WORK 0x80     // the driver's work (SNDi_Work at 0x03809fbc on the DS)
#define PBW_SNDTRACE_SHARED 0x1200 // the shared work (SNDSharedWork), zeros before the ARM9 gives one
// u32[16]: the mixer sample at which each channel's sound last started, counted from the program's first (the WAV
// dump's first sample on both programs): the sample whose mixing took the channel's start bit first
#define PBW_SNDTRACE_START_MIX 0x1480

#define PBW_SNDTRACE_WORK_SIZE 0x1180
#define PBW_SNDTRACE_SHARED_SIZE 0x280
#define PBW_SNDTRACE_CHANNELS 16

// The work's parts: 16 channels of 0x54 bytes, 16 players of 0x24, 32 tracks of 0x40, 8 alarms of 0x40
#define PBW_SNDTRACE_WORK_ADDRESS 0x03809fbcu
#define PBW_SNDTRACE_CHANNEL_SIZE 0x54
#define PBW_SNDTRACE_PLAYERS 0x540
#define PBW_SNDTRACE_PLAYER_SIZE 0x24
#define PBW_SNDTRACE_TRACKS 0x780
#define PBW_SNDTRACE_TRACK_SIZE 0x40
#define PBW_SNDTRACE_ALARMS 0xf80
#define PBW_SNDTRACE_ALARM_SIZE 0x40

static inline void pbw_sndtrace_put32(uint8_t *at, uint32_t value) {
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
    at[2] = (uint8_t)(value >> 16);
    at[3] = (uint8_t)(value >> 24);
}

static inline void pbw_sndtrace_put64(uint8_t *at, uint64_t value) {
    pbw_sndtrace_put32(at, (uint32_t)value);
    pbw_sndtrace_put32(at + 4, (uint32_t)(value >> 32));
}

#endif // PORT_TOOLS_ORACLE_SNDTRACE_H
