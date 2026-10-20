#include "dwc_util_sound.h"
#include "dwc_util_heap.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_task.h"
#include "nnsys/snd.h"

static void DWCUtilSound_UpdateTask(DWCUtilTask *task, void *arg);

typedef struct DWCUtilSound {
    NNSSndArc arc;
    u32 unk_98[2];
    NNSSndHandle handle;
    // The archive's file, and the task that runs the sound each frame
    void *data;
    DWCUtilTask *task;
} DWCUtilSound;

static DWCUtilSound *sSound;

void DWCUtilSound_Init(void) {
    u32 size;
    int i;

    sSound = DWCUtilHeap_AllocClear(sizeof(DWCUtilSound), 4);
    sSound->data = DWCUtilRomResource_LoadFile("sound/sound_data.sdat.l", &size, 32);
    NNS_SndInit();
    NNS_SndArcInitOnMemory(&sSound->arc, sSound->data);
    NNS_SndArcPlayerSetup(NULL);
    NNS_SndHandleInit(&sSound->handle);
    for (i = 0; i < 32; i++) {
        NNS_SndPlayerSetPlayerVolume(i, 0x7f);
        NNS_SndPlayerSetPlayableSeqCount(i, 1);
        NNS_SndPlayerSetAllocatableChannel(i, 0);
    }
    sSound->task = DWCUtilTask_Create(0, DWCUtilSound_UpdateTask, NULL, 200);
}

void DWCUtilSound_Exit(void) {
    DWCUtilTask_QueueRemoval(0, sSound->task);
    DWCUtilHeap_FreeAndNull(&sSound);
}

void DWCUtilSound_PlaySe(int seq) {
    NNS_SndArcPlayerStartSeqArc(&sSound->handle, 0, seq);
}

void DWCUtilSound_SetVolume(int volume) {
    NNS_SndPlayerSetVolume(&sSound->handle, volume);
}

void DWCUtilSound_SetTrackPitch(u32 trackMask, int pitch) {
    NNS_SndPlayerSetTrackPitch(&sSound->handle, trackMask, pitch);
}

void DWCUtilSound_StopSe(void) {
    NNS_SndPlayerStopSeq(&sSound->handle, 0);
}

static void DWCUtilSound_UpdateTask(DWCUtilTask *task, void *arg) {
    NNS_SndMain();
}
