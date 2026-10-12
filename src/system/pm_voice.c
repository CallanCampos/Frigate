#include "types.h"
#include "gfl/sound.h"
#include "constants/species.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "nitro/os.h"
#include "nitro/snd.h"
#include "nnsys/snd.h"
#include "save/chatter.h"
#include "save/save_control.h"

// The cries of the Pokémon, each a wave archive of one 8-bit wave that is read whole and played through NitroSystem's
// wave output, with a second channel for the chorus. Handle 0 has a buffer of its own; PokeVoice_AllocMulti gives
// two more handles theirs. The name is the ROM's string.

#define POKE_VOICE_HANDLE_MAX 3
#define POKE_VOICE_BUFFER_SIZE 0x5c30
// The last species number that has a cry; the numbers from POKE_VOICE_DIRECT_MIN are cries played by their number
#define POKE_VOICE_SPECIES_MAX 684
#define POKE_VOICE_DIRECT_MIN 652
#define POKE_VOICE_DEFAULT_RATE 13379
#define POKE_VOICE_DEFAULT_SPEED 25825
#define POKE_VOICE_PAN_CENTER 64
#define POKE_VOICE_VOLUME_MAX 127
// NitroSystem's NNS_SND_WAVE_FORMAT_PCM8
#define POKE_VOICE_WAVE_FORMAT 0
// The volume of a Chatot's recording, whose sample rate is its count of samples
#define CHATTER_VOLUME 120
// The archive with each cry's data
#define ARCID_POKE_VOICE_DATA 191

typedef struct {
    BOOL playing;
    void *buffer;
    u32 size;
    u32 sampleRate;
    void *samples;
    NNSSndWaveOutHandle handle;
    NNSSndWaveOutHandle chorusHandle;
    s8 volume;
    s8 chorusVolume;
    u8 channel : 4;
    u8 chorusChannel : 4;
    u32 speed;
    s32 chorusSpeed;
    u8 pan;
    BOOL chorus;
    // Whether buffer is from the heap
    BOOL allocated;
} PokeVoiceEntry;

typedef struct {
    u16 unk0;
    PokeVoiceEntry entries[POKE_VOICE_HANDLE_MAX];
    u16 nextHandle;
    u16 handleCount;
    PokeVoiceGetResIDFunc getResID;
    PokeVoiceGetWaveInfoFunc getWaveInfo;
    u8 masterVolume;
} PokeVoiceSystem;

// NitroSDK's SNDWaveData
typedef struct {
    u8 format;
    u8 loop;
    u16 rate;
    u16 timer;
    u16 loopStart;
    u32 loopLength;
} PokeVoiceWaveData;

// A cry's entry in the data archive
typedef struct {
    u16 unk0;
    u16 volume;
} PokeVoiceData;

typedef struct {
    u16 channel;
    u16 chorusChannel;
} PokeVoiceChannels;

// The wave archive of a form's cry
typedef struct {
    u16 species;
    u16 form;
    u16 waveArc;
} PokeVoiceFormEntry;

static u8 PokeVoice_CalcVolume(u8 volume);
static u32 PokeVoice_AcquireHandle(void);
static void PokeVoice_Reset(PokeVoiceEntry *entry);
static void PokeVoice_ResetEx(PokeVoiceEntry *entry);
static BOOL PokeVoice_LoadWaveArc(PokeVoiceEntry *entry, u32 waveArc);
static BOOL PokeVoice_StartPlaybackCore(PokeVoiceEntry *entry);
static void PokeVoice_ReleaseCore(PokeVoiceEntry *entry);
static void PokeVoice_ReverseSamples(u8 *samples, u32 size);

static const PokeVoiceChannels POKE_VOICE_DEFAULT_CHANNEL_LIST[POKE_VOICE_HANDLE_MAX] = {
    { 15, 14 },
    { 13, 12 },
    { 11, 10 },
};

static const PokeVoiceFormEntry POKE_VOICE_EXTRA_ENTRIES[] = {
    { SPECIES_SHAYMIN, 1, 685 },
    { SPECIES_KYUREM, 1, 686 },
    { SPECIES_KYUREM, 2, 687 },
    { SPECIES_TORNADUS, 1, 688 },
    { SPECIES_THUNDURUS, 1, 689 },
    { SPECIES_LANDORUS, 1, 690 },
};

static PokeVoiceSystem sSystem;
static u8 sBuffer[POKE_VOICE_BUFFER_SIZE];

void PokeVoice_Init(u16 a0, PokeVoiceGetResIDFunc getResID, PokeVoiceGetWaveInfoFunc getWaveInfo) {
    PokeVoiceEntry *entry;
    int i;

    sSystem.unk0 = a0;
    sSystem.masterVolume = POKE_VOICE_VOLUME_MAX;
    for (i = 0; i < POKE_VOICE_HANDLE_MAX; i++) {
        entry = &sSystem.entries[i];
        entry->playing = FALSE;
        entry->channel = POKE_VOICE_DEFAULT_CHANNEL_LIST[i].channel;
        entry->chorusChannel = POKE_VOICE_DEFAULT_CHANNEL_LIST[i].chorusChannel;
        PokeVoice_ResetEx(entry);
        if (i == 0) {
            entry->buffer = sBuffer;
            entry->samples = sBuffer;
        }
    }
    sSystem.handleCount = 1;
    sSystem.nextHandle = 0;
    sSystem.getResID = getResID;
    sSystem.getWaveInfo = getWaveInfo;
}

void PokeVoice_ReleaseAll(void) {
    PokeVoiceEntry *entry;
    int i;

    for (i = 0; i < POKE_VOICE_HANDLE_MAX; i++) {
        entry = &sSystem.entries[i];
        if (entry->playing == TRUE) {
            PokeVoice_ReleaseCore(entry);
        }
    }
}

void PokeVoice_Update(void) {
    PokeVoiceEntry *entry;
    BOOL playing;
    BOOL chorusPlaying;
    int i;

    for (i = 0; i < sSystem.handleCount; i++) {
        entry = &sSystem.entries[i];
        if (entry->playing == TRUE) {
            playing = NNS_SndWaveOutIsPlaying(entry->handle);
            if (entry->chorus == TRUE) {
                chorusPlaying = NNS_SndWaveOutIsPlaying(entry->chorusHandle);
            } else {
                chorusPlaying = FALSE;
            }
            if (!playing && !chorusPlaying) {
                PokeVoice_ReleaseCore(entry);
            }
        }
    }
}

BOOL PokeVoice_IsPlayingAny(void) {
    int i;

    for (i = 0; i < POKE_VOICE_HANDLE_MAX; i++) {
        if (sSystem.entries[i].playing == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

void PokeVoice_AllocMulti(int count, HeapID heapId) {
    PokeVoiceEntry *entry;
    int i;

    PokeVoice_ReleaseAll();
    if (count > POKE_VOICE_HANDLE_MAX - 1) {
        count = POKE_VOICE_HANDLE_MAX - 1;
    }
    for (i = 1; i < count + 1; i++) {
        entry = &sSystem.entries[i];
        if (entry->buffer == NULL) {
            entry->buffer = GFL_HeapAllocate(heapId, POKE_VOICE_BUFFER_SIZE, TRUE, "pm_voice.c", 261);
            entry->samples = entry->buffer;
            entry->allocated = TRUE;
        }
    }
    sSystem.handleCount = count + 1;
    sSystem.nextHandle = 0;
}

void PokeVoice_FreeMulti(void) {
    PokeVoiceEntry *entry;
    int i;

    PokeVoice_ReleaseAll();
    for (i = 0; i < sSystem.handleCount; i++) {
        entry = &sSystem.entries[i];
        if (entry->buffer != NULL && entry->allocated == TRUE) {
            GFL_HeapFree(entry->buffer);
            PokeVoice_ResetEx(entry);
        }
    }
    sSystem.handleCount = 1;
    sSystem.nextHandle = 0;
}

void PokeVoice_SetMasterVolume(u32 volume) {
    sSystem.masterVolume = volume;
}

void PokeVoice_ResetMasterVolume(void) {
    sSystem.masterVolume = POKE_VOICE_VOLUME_MAX;
}

static u8 PokeVoice_CalcVolume(u8 volume) {
    return volume * sSystem.masterVolume / POKE_VOICE_VOLUME_MAX;
}

// The next handle in turn, stopping what it plays
static u32 PokeVoice_AcquireHandle(void) {
    u16 handle = sSystem.nextHandle;

    if (sSystem.entries[handle].playing == TRUE) {
        PokeVoice_ReleaseCore(&sSystem.entries[handle]);
    }
    if (sSystem.nextHandle < sSystem.handleCount - 1) {
        sSystem.nextHandle++;
    } else {
        sSystem.nextHandle = 0;
    }
    return handle;
}

static void PokeVoice_Reset(PokeVoiceEntry *entry) {
    entry->sampleRate = POKE_VOICE_DEFAULT_RATE;
    entry->size = 0;
    entry->speed = POKE_VOICE_DEFAULT_SPEED;
    entry->pan = POKE_VOICE_PAN_CENTER;
    entry->volume = 0;
    entry->chorusVolume = 0;
    entry->chorusSpeed = 0;
    entry->chorus = FALSE;
    entry->samples = entry->buffer;
}

static void PokeVoice_ResetEx(PokeVoiceEntry *entry) {
    entry->buffer = NULL;
    entry->allocated = FALSE;
    PokeVoice_Reset(entry);
}

// Reads a wave archive into the entry's buffer and takes its first wave
static BOOL PokeVoice_LoadWaveArc(PokeVoiceEntry *entry, u32 waveArc) {
    const NNSSndArcWaveArcInfo *info;
    SNDWaveArc *arc;
    PokeVoiceWaveData *wave;
    u32 size;
    int read;

    info = NNS_SndArcGetWaveArcInfo(waveArc);
    if (info == NULL) {
        return FALSE;
    }
    size = NNS_SndArcGetFileSize(info->fileId);
    if (size == 0) {
        return FALSE;
    }
    read = NNS_SndArcReadFile(info->fileId, entry->buffer, size, 0);
    if (read == -1) {
        return FALSE;
    }
    if (read != size) {
        return FALSE;
    }
    arc = entry->buffer;
    wave = (PokeVoiceWaveData *)((u8 *)arc + arc->offsetTable[0]);
    func_0207df80(arc);
    size = arc->fileHeader.fileSize - arc->offsetTable[0];
    size -= sizeof(PokeVoiceWaveData);
    entry->samples = wave + 1;
    entry->size = size;
    entry->sampleRate = wave->rate;
    entry->speed = ((u64)SND_TIMER_CLOCK << 15) / wave->rate / wave->timer;
    return TRUE;
}

static BOOL PokeVoice_StartPlaybackCore(PokeVoiceEntry *entry) {
    int volume;

    if (entry->buffer == NULL) {
        return FALSE;
    }
    if (entry->size == 0) {
        return FALSE;
    }
    entry->handle = NNS_SndWaveOutAllocChannel(entry->channel);
    if (entry->handle == NULL) {
        return FALSE;
    }
    if (!NNS_SndWaveOutStart(entry->handle, POKE_VOICE_WAVE_FORMAT, entry->samples, FALSE, 0, entry->size,
                             entry->sampleRate, PokeVoice_CalcVolume(entry->volume), entry->speed, entry->pan)) {
        return FALSE;
    }
    if (entry->chorus == TRUE) {
        entry->chorusHandle = NNS_SndWaveOutAllocChannel(entry->chorusChannel);
        if (entry->chorusHandle == NULL) {
            return FALSE;
        }
        volume = entry->volume + entry->chorusVolume;
        if (volume < 0) {
            volume = 0;
        }
        if (volume > POKE_VOICE_VOLUME_MAX) {
            volume = POKE_VOICE_VOLUME_MAX;
        }
        if (!NNS_SndWaveOutStart(entry->chorusHandle, POKE_VOICE_WAVE_FORMAT, entry->samples, FALSE, 0, entry->size,
                                 entry->sampleRate, PokeVoice_CalcVolume(volume), entry->speed + entry->chorusSpeed,
                                 entry->pan)) {
            return FALSE;
        }
    }
    entry->playing = TRUE;
    return TRUE;
}

static void PokeVoice_ReleaseCore(PokeVoiceEntry *entry) {
    NNS_SndWaveOutStop(entry->handle);
    NNS_SndWaveOutWaitForChannelStop(entry->handle);
    NNS_SndWaveOutFreeChannel(entry->handle);
    if (entry->chorus == TRUE) {
        NNS_SndWaveOutStop(entry->chorusHandle);
        NNS_SndWaveOutWaitForChannelStop(entry->chorusHandle);
        NNS_SndWaveOutFreeChannel(entry->chorusHandle);
    }
    entry->playing = FALSE;
    PokeVoice_Reset(entry);
}

u32 PokeVoice_Play(u32 species, u32 form, u32 pan, u32 chorus, u32 chorusVolume, u32 chorusSpeed, u32 reverse,
                   const PokeVoiceChatterInfo *chatterInfo) {
    u16 handle;

    // Waits until the sound thread has loaded the next BGM
    while (func_02005cbc() == TRUE) {
        func_0207aa04(2);
    }
    handle = PokeVoice_Load(species, form, pan, chorus, chorusVolume, chorusSpeed, reverse, chatterInfo);
    PokeVoice_StartPlayback(handle);
    return handle;
}

u32 PokeVoice_Load(u32 species, u32 form, u32 pan, u32 chorus, u32 chorusVolume, u32 chorusSpeed, u32 reverse,
                   const PokeVoiceChatterInfo *chatterInfo) {
    PokeVoiceEntry *entry;
    u32 handle;
    u32 waveArc;

    sSystem.getResID(species, form, &waveArc);
    handle = PokeVoice_AcquireHandle();
    entry = &sSystem.entries[handle];
    if (entry->playing == TRUE) {
        PokeVoice_ReleaseCore(entry);
    }
    if (!sSystem.getWaveInfo(species, form, chatterInfo, &entry->samples, &entry->size, &entry->sampleRate,
                             &entry->speed, &entry->volume)) {
        PokeVoice_LoadWaveArc(entry, waveArc);
    }
    if (reverse) {
        PokeVoice_ReverseSamples(entry->samples, entry->size);
    }
    entry->pan = pan;
    entry->chorus = chorus;
    entry->chorusVolume = chorusVolume;
    entry->chorusSpeed = chorusSpeed;
    return handle;
}

BOOL PokeVoice_StartPlayback(u32 handle) {
    if (!PokeVoice_StartPlaybackCore(&sSystem.entries[handle])) {
        PokeVoice_ReleaseCore(&sSystem.entries[handle]);
        return FALSE;
    }
    return TRUE;
}

void PokeVoice_AdjustVolume(u32 handle, s8 volume) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];
    int newVolume;

    if (!entry->playing) {
        return;
    }
    newVolume = entry->volume + volume;
    if (newVolume < 0) {
        newVolume = 0;
    }
    if (newVolume > POKE_VOICE_VOLUME_MAX) {
        newVolume = POKE_VOICE_VOLUME_MAX;
    }
    entry->volume = newVolume;
    NNS_SndWaveOutSetVolume(entry->handle, PokeVoice_CalcVolume(entry->volume));
    if (entry->chorus == TRUE) {
        newVolume = entry->volume + entry->chorusVolume;
        if (newVolume < 0) {
            newVolume = 0;
        }
        if (newVolume > POKE_VOICE_VOLUME_MAX) {
            newVolume = POKE_VOICE_VOLUME_MAX;
        }
        // BUG: the chorus's volume is set on the cry's own channel, so the chorus keeps its volume
#ifdef BUGFIX
        NNS_SndWaveOutSetVolume(entry->chorusHandle, PokeVoice_CalcVolume(newVolume));
#else
        NNS_SndWaveOutSetVolume(entry->handle, PokeVoice_CalcVolume(newVolume));
#endif
    }
}

void PokeVoice_AdjustSpeed(u32 handle, s32 speed) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];

    if (!entry->playing) {
        return;
    }
    entry->speed += speed;
    NNS_SndWaveOutSetSpeed(entry->handle, entry->speed);
    if (entry->chorus == TRUE) {
        NNS_SndWaveOutSetSpeed(entry->chorusHandle, entry->speed + entry->chorusSpeed);
    }
}

s8 PokeVoice_GetVolume(u32 handle) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];

    if (entry->playing == FALSE) {
        return 0;
    }
    return entry->volume;
}

int PokeVoice_GetSpeed(u32 handle) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];

    if (entry->playing == FALSE) {
        return POKE_VOICE_DEFAULT_SPEED;
    }
    return entry->speed;
}

const s8 *PokeVoice_GetSamples(u32 handle) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];

    if (entry->playing == FALSE) {
        return NULL;
    }
    return entry->samples;
}

u32 PokeVoice_GetSampleCount(u32 handle) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];

    if (entry->playing == FALSE) {
        return 0;
    }
    return entry->size;
}

int PokeVoice_GetSampleRate(u32 handle) {
    PokeVoiceEntry *entry = &sSystem.entries[handle];

    if (entry->playing == FALSE) {
        return POKE_VOICE_DEFAULT_RATE;
    }
    return entry->sampleRate;
}

void PokeVoice_Release(u32 handle) {
    if (sSystem.entries[handle].playing == TRUE) {
        PokeVoice_ReleaseCore(&sSystem.entries[handle]);
    }
}

BOOL PokeVoice_IsPlaying(u32 handle) {
    return sSystem.entries[handle].playing;
}

static void PokeVoice_ReverseSamples(u8 *samples, u32 size) {
    u32 i;
    u8 sample;

    for (i = 0; i < size / 2; i++) {
        sample = samples[i];
        samples[i] = samples[size - 1 - i];
        samples[size - 1 - i] = sample;
    }
}

// The game's PokeVoiceGetResIDFunc: the wave archive of a species and form
void getResIDFunc(u32 species, u32 form, u32 *waveArc) {
    const PokeVoiceFormEntry *entry;
    u32 i;

    if (species < 1 || species > POKE_VOICE_SPECIES_MAX) {
        *waveArc = 1;
        return;
    }
    if (species >= POKE_VOICE_DIRECT_MIN && species <= POKE_VOICE_SPECIES_MAX) {
        *waveArc = species;
        return;
    }
    for (i = 0; i < NELEMS(POKE_VOICE_EXTRA_ENTRIES); i++) {
        entry = &POKE_VOICE_EXTRA_ENTRIES[i];
        if (entry->species == species && form == entry->form) {
            *waveArc = entry->waveArc;
            return;
        }
    }
    *waveArc = species;
}

// The game's PokeVoiceGetWaveInfoFunc: gives the cry's volume, and for a Chatot with a recording the recording,
// at a random speed
BOOL getWaveInfoFunc(u32 species, u32 form, const PokeVoiceChatterInfo *chatterInfo, void **samples, u32 *size,
                     u32 *sampleRate, u32 *speed, s8 *volume) {
    PokeVoiceData data;

    if (species < 1 || species > POKE_VOICE_SPECIES_MAX) {
        species = 1;
    }
    GFL_ArcSysRead(&data, ARCID_POKE_VOICE_DATA, species);
    *volume = data.volume;
    if (chatterInfo == NULL) {
        return FALSE;
    }
    if (species == SPECIES_CHATOT && form == 0 && doesChatotExist(chatterInfo->chatter) == TRUE) {
        PerapVoice_Decode(*samples, getChatterVoice(chatterInfo->chatter));
        *size = PERAP_VOICE_DECODED_SIZE;
        *sampleRate = PERAP_VOICE_DECODED_SIZE;
        *volume = CHATTER_VOLUME;
        *speed = GFL_RandomLC(0x2000) + 0x8000;
        return TRUE;
    }
    return FALSE;
}

void PokeVoice_CreateChatterInfo(PokeVoiceChatterInfo *info) {
    info->chatter = getChatterBlockAddress(SaveControl_GetInstance());
}
