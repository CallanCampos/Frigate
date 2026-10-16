#include "types.h"
#include "nitro/os.h"
#include "twl/dsp.h"
#include "twl/sndex.h"

// TwlSDK's DSP library, the shutter sound (guessed file name; dsp_shutter.c and dsp_sound.c are alternatives): plays a
// WAV file from memory through the audio component that the first DSP process runs, and tells whether it still plays.
// The ARM9 talks to the component over pipe 2 of that process, in messages of four 32-bit words that the DSP reads as
// two 16-bit halves each, the high half first. ARM code in the DSi-only LTD autoload. The ROM names none of it; the
// functions have the SDK's names where twl/dsp.h knows them, and keep their default names otherwise.

#define SHUTTER_PIPE_PORT 2

// The first word of a message says what it is: a group in the top nibble and a kind in the next. The ARM9's commands
// are of the playback group, with a play mode in the low byte of the play command. The component reports back that it
// stopped (playback group) and a status word (status group)
#define AUDIO_GROUP_MASK 0xf000
#define AUDIO_KIND_MASK 0x0f00
#define AUDIO_GROUP_PLAYBACK 0x1000
#define AUDIO_GROUP_STATUS 0x2000
#define AUDIO_KIND_PLAY 0x0100
#define AUDIO_KIND_STOP 0x0200
#define AUDIO_KIND_STATUS 0x0100
#define AUDIO_COMMAND_PLAY (AUDIO_GROUP_PLAYBACK | AUDIO_KIND_PLAY)
#define AUDIO_COMMAND_STOP (AUDIO_GROUP_PLAYBACK | AUDIO_KIND_STOP)

#define AUDIO_PLAY_MODE_ONCE 2
#define AUDIO_PRIORITY_HIGHEST 0
#define AUDIO_PRIORITY_IDLE 0x20 // nothing plays, so that any priority may start

// The WAV file's header: "RIFF", its size, "WAVE", the format chunk (its sample rate at 0x18) and the data chunk,
// whose size is at 0x28 and which starts at 0x2c
#define WAV_HEADER_SIZE 0x2c
#define WAV_RIFF 0x46464952 // "RIFF"
#define WAV_WAVE 0x45564157 // "WAVE"
#define WAV_SAMPLE_RATE_OFFSET 0x18
#define WAV_DATA_SIZE_OFFSET 0x28

// The two sample rates of the I2S, which a file has to match
#define I2S_FREQUENCY_32K 0
#define I2S_FREQUENCY_47K 1
#define WAV_SAMPLE_RATE_32K 32730
#define WAV_SAMPLE_RATE_47K 47610

// How long the sound still counts as playing after the component reported that it stopped, in milliseconds
#define SHUTTER_STOP_DELAY 30
// A wait before the request to the ARM7, in cycles (about 30 milliseconds of the 67 MHz clock)
#define SHUTTER_REQUEST_DELAY_CYCLES 2010000

static void func_02705f64(SNDEXResult result, void *arg);
static void func_02705f98(void *arg);
static void func_02705fac(void *userData, int port, int direction);
static void func_027060b4(const void *src, u32 length, int mode, int priority);

static struct {
    u16 status;       // what the component last reported
    int i2sFrequency; // which of the two I2S sample rates is set
    u8 unk_08[0xc];
    BOOL requestOn; // the ARM7 got func_02704858's request, until the answer to func_02704940's
    BOOL playing;   // a play command was sent, until the component stopped and the alarm ran out
    u8 unk_1c[8];
} sShutter;
static OSAlarm sStopAlarm;

// The priority of what plays; the lower number wins
static int sPriority = AUDIO_PRIORITY_IDLE;

// A 32-bit word of a message, which the DSP reads as two 16-bit halves, the high one first
static inline u32 SwapHalves(u32 x) {
    return (x >> 16) | (x << 16);
}

// Sends a command to the first process's audio component, which takes it as four words
static inline void SendAudioCommand(DSPProcessContext *context, u32 command, u32 address, u32 words) {
    u32 message[4];

    message[0] = SwapHalves(command);
    message[1] = SwapHalves(address);
    message[2] = SwapHalves(words);
    message[3] = 0;
    DSP_WriteProcessPipe(context, SHUTTER_PIPE_PORT, message, sizeof(message));
}

static inline u32 ReadU32(const u8 *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

// The answer to the ARM7's request that ends the shutter sound
static void func_02705f64(SNDEXResult result, void *arg) {
    if (result == SNDEX_RESULT_BUSY) {
        sys_exit();
    }
    if (result != SNDEX_RESULT_SUCCESS) {
        sys_exit();
    }
    sShutter.requestOn = FALSE;
}

// After the component reported that it stopped
static void func_02705f98(void *arg) {
    sShutter.playing = FALSE;
}

// The pipe's callback: the component's reports
static void func_02705fac(void *userData, int port, int direction) {
    DSPPipe pipe;
    u32 message[2];

    if (direction != 0) {
        return;
    }
    DSP_LoadPipe(&pipe, port, direction);
    if (DSP_GetPipeReadableSize(&pipe) < sizeof(message)) {
        return;
    }
    DSP_ReadPipe(&pipe, message, sizeof(message));
    message[0] = SwapHalves(message[0]);
    message[1] = SwapHalves(message[1]);
    if ((message[0] & AUDIO_GROUP_MASK) == AUDIO_GROUP_PLAYBACK && (message[0] & AUDIO_KIND_MASK) == AUDIO_KIND_STOP) {
        OS_CreateAlarm(&sStopAlarm);
        OS_SetAlarm(&sStopAlarm, OS_MilliSecondsToTicks(SHUTTER_STOP_DELAY), func_02705f98, NULL);
        if (sPriority == AUDIO_PRIORITY_HIGHEST) {
            func_02704940(func_02705f64, NULL);
        }
        sPriority = AUDIO_PRIORITY_IDLE;
    }
    if ((message[0] & AUDIO_GROUP_MASK) == AUDIO_GROUP_STATUS && (message[0] & AUDIO_KIND_MASK) == AUDIO_KIND_STATUS) {
        sShutter.status = message[1];
    }
}

// Sends the play command for length bytes of 16-bit samples at src to the first process, unless something of a higher
// priority plays
static void func_027060b4(const void *src, u32 length, int mode, int priority) {
    DSPProcessContext *context = DSP_FindProcess(NULL);

    if (context == NULL) {
        return;
    }
    if (sPriority < priority) {
        return;
    }
    sPriority = priority;
    DSP_SetPipeCallback(SHUTTER_PIPE_PORT, func_02705fac, NULL);
    sShutter.playing = TRUE;
    SendAudioCommand(context, AUDIO_COMMAND_PLAY | mode, (u32)src, length / 2);
}

// Plays the WAV file at src, if it fits the I2S's sample rate
BOOL DSP_PlayShutterSoundCore(const void *src, u32 length) {
    const u8 *file = src;
    u32 dataSize;
    u32 sampleRate;

    if (length < WAV_HEADER_SIZE) {
        return FALSE;
    }
    if (ReadU32(file) != WAV_RIFF) {
        return FALSE;
    }
    if (ReadU32(file + 8) != WAV_WAVE) {
        return FALSE;
    }
    sampleRate = ReadU32(file + WAV_SAMPLE_RATE_OFFSET);
    if ((sShutter.i2sFrequency == I2S_FREQUENCY_32K && sampleRate != WAV_SAMPLE_RATE_32K) ||
        (sShutter.i2sFrequency == I2S_FREQUENCY_47K && sampleRate != WAV_SAMPLE_RATE_47K)) {
        return FALSE;
    }
    dataSize = ReadU32(file + WAV_DATA_SIZE_OFFSET);
    if (length < dataSize + WAV_HEADER_SIZE) {
        return FALSE;
    }
    OS_SpinWaitSysCycles(SHUTTER_REQUEST_DELAY_CYCLES);
    while (func_02704858() != SNDEX_RESULT_SUCCESS) {
        OS_Sleep(1);
    }
    func_027060b4(file + WAV_HEADER_SIZE, dataSize, AUDIO_PLAY_MODE_ONCE, AUDIO_PRIORITY_HIGHEST);
    sShutter.requestOn = TRUE;
    return TRUE;
}

// Stops the sound
void DSP_StopSoundCore(void) {
    DSPProcessContext *context = DSP_FindProcess(NULL);

    if (context != NULL && sShutter.playing) {
        SendAudioCommand(context, AUDIO_COMMAND_STOP, 0, 0);
    }
}

BOOL DSP_IsShutterSoundPlayingCore(void) {
    return sShutter.playing | sShutter.requestOn;
}
