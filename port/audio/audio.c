// The host's audio output (port/audio/audio.h): the music, sound effects and cries that the host ARM7's sound unit
// (port/hal/snd7) makes as the virtual clock passes them and hands over at each VBlank, played on the default device
// through an SDL3 audio stream whose speed follows how much is queued, and written as they come to a WAV file for
// --audio-dump.
#include "audio/audio.h"
#include "hal/snd7/snd7_output.h"
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FRAME_BYTES (PORT_SND7_CHANNELS * 2)
// What the stream should hold: a tenth of a second, which rides out a late frame; and what it may hold before it is
// emptied, a second
#define TARGET_QUEUED_FRAMES (PORT_SND7_SAMPLE_RATE / 10)
#define MAX_QUEUED_FRAMES PORT_SND7_SAMPLE_RATE
// How far the stream's speed may move from 1 to keep its queue at the target
#define RATIO_SPAN 0.005f

#define WAV_HEADER_SIZE 44
#define WAV_DATA_MAX (0xffffffffu - (WAV_HEADER_SIZE - 8))

static struct {
    bool audioInit;
    SDL_AudioStream *stream;
    FILE *dump;
    const char *dumpPath;
    uint64_t dumpBytes;
    bool dumpFull;
} sAudio;

static void Put16(uint8_t *bytes, unsigned value) {
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static void Put32(uint8_t *bytes, uint32_t value) {
    Put16(bytes, value & 0xffff);
    Put16(bytes + 2, value >> 16);
}

// The WAV file's head for dataBytes of samples
static void WavHeader(uint8_t header[WAV_HEADER_SIZE], uint32_t dataBytes) {
    memcpy(header, "RIFF", 4);
    Put32(header + 4, WAV_HEADER_SIZE - 8 + dataBytes);
    memcpy(header + 8, "WAVEfmt ", 8);
    Put32(header + 16, 16);
    Put16(header + 20, 1);
    Put16(header + 22, PORT_SND7_CHANNELS);
    Put32(header + 24, PORT_SND7_SAMPLE_RATE);
    Put32(header + 28, PORT_SND7_SAMPLE_RATE * FRAME_BYTES);
    Put16(header + 32, FRAME_BYTES);
    Put16(header + 34, 16);
    memcpy(header + 36, "data", 4);
    Put32(header + 40, dataBytes);
}

static void WriteDump(const int16_t *frames, uint32_t count) {
    uint8_t bytes[1024 * FRAME_BYTES];
    uint32_t done = 0;

    while (done < count && !sAudio.dumpFull) {
        uint32_t chunk = count - done < 1024 ? count - done : 1024;
        uint32_t i;

        if (sAudio.dumpBytes + (uint64_t)chunk * FRAME_BYTES > WAV_DATA_MAX) {
            fprintf(stderr, "port: audio: %s reached the 4 GiB a WAV file holds; the rest isn't written\n",
                    sAudio.dumpPath);
            sAudio.dumpFull = true;
            break;
        }
        for (i = 0; i < chunk * PORT_SND7_CHANNELS; i++) {
            Put16(&bytes[i * 2], (uint16_t)frames[done * PORT_SND7_CHANNELS + i]);
        }
        if (fwrite(bytes, FRAME_BYTES, chunk, sAudio.dump) != chunk) {
            fprintf(stderr, "port: audio: writing %s failed; the rest isn't written\n", sAudio.dumpPath);
            sAudio.dumpFull = true;
            break;
        }
        sAudio.dumpBytes += (uint64_t)chunk * FRAME_BYTES;
        done += chunk;
    }
}

// The device's stream: the frames, then its speed from what it holds
static void PutStream(const int16_t *frames, uint32_t count) {
    int queued = SDL_GetAudioStreamQueued(sAudio.stream);
    float error;

    if (queued > MAX_QUEUED_FRAMES * FRAME_BYTES) {
        SDL_ClearAudioStream(sAudio.stream);
        queued = 0;
    }
    if (!SDL_PutAudioStreamData(sAudio.stream, frames, (int)(count * FRAME_BYTES))) {
        return;
    }
    queued = queued < 0 ? 0 : queued / FRAME_BYTES + (int)count;
    error = (float)(queued - TARGET_QUEUED_FRAMES) / TARGET_QUEUED_FRAMES;
    if (error > 1) {
        error = 1;
    } else if (error < -1) {
        error = -1;
    }
    SDL_SetAudioStreamFrequencyRatio(sAudio.stream, 1 + RATIO_SPAN * error);
}

static void Output(const int16_t *frames, uint32_t count, void *user) {
    (void)user;
    if (sAudio.stream != NULL) {
        PutStream(frames, count);
    }
    if (sAudio.dump != NULL) {
        WriteDump(frames, count);
    }
}

bool port_audio_open(void) {
    SDL_AudioSpec spec;

    if (sAudio.stream != NULL) {
        return true;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        fprintf(stderr, "port: audio: no audio (%s); the game runs without sound\n", SDL_GetError());
        return false;
    }
    sAudio.audioInit = true;
    spec.format = SDL_AUDIO_S16LE;
    spec.channels = PORT_SND7_CHANNELS;
    spec.freq = PORT_SND7_SAMPLE_RATE;
    sAudio.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (sAudio.stream == NULL) {
        fprintf(stderr, "port: audio: no playback device (%s); the game runs without sound\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        sAudio.audioInit = false;
        return false;
    }
    SDL_ResumeAudioStreamDevice(sAudio.stream);
    port_snd7_set_output(Output, NULL);
    return true;
}

bool port_audio_dump_open(const char *path) {
    uint8_t header[WAV_HEADER_SIZE];

    if (sAudio.dump != NULL) {
        return true;
    }
    sAudio.dump = fopen(path, "wb");
    if (sAudio.dump == NULL) {
        fprintf(stderr, "port: audio: can't write %s\n", path);
        return false;
    }
    WavHeader(header, 0);
    if (fwrite(header, 1, sizeof(header), sAudio.dump) != sizeof(header)) {
        fprintf(stderr, "port: audio: can't write %s\n", path);
        fclose(sAudio.dump);
        sAudio.dump = NULL;
        return false;
    }
    sAudio.dumpPath = path;
    sAudio.dumpBytes = 0;
    sAudio.dumpFull = false;
    port_snd7_set_output(Output, NULL);
    return true;
}

void port_audio_close(void) {
    // The last samples first
    port_snd7_set_output(NULL, NULL);
    if (sAudio.stream != NULL) {
        SDL_DestroyAudioStream(sAudio.stream);
        sAudio.stream = NULL;
    }
    if (sAudio.audioInit) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        sAudio.audioInit = false;
    }
    if (sAudio.dump != NULL) {
        uint8_t header[WAV_HEADER_SIZE];

        WavHeader(header, (uint32_t)sAudio.dumpBytes);
        if (fseek(sAudio.dump, 0, SEEK_SET) != 0 || fwrite(header, 1, sizeof(header), sAudio.dump) != sizeof(header)) {
            fprintf(stderr, "port: audio: can't finish %s\n", sAudio.dumpPath);
        }
        fclose(sAudio.dump);
        sAudio.dump = NULL;
    }
}
