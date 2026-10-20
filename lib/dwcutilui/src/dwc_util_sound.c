#include "dwc_util_sound.h"
#include "dwc_util_heap.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_task.h"
#include "nnsys/snd.h"

static void func_ov182_021b814c(DWCUtilTask *task, void *arg);

typedef struct DWCUtilSound {
    NNSSndArc arc;
    u32 unk_98[2];
    NNSSndHandle handle;
    // The archive's file, and the task that runs the sound each frame
    void *data;
    DWCUtilTask *task;
} DWCUtilSound;

static DWCUtilSound *sSound;

void func_ov182_021b8050(void) {
    u32 size;
    int i;

    sSound = func_ov182_021bc4d8(sizeof(DWCUtilSound), 4);
    sSound->data = func_ov182_021bb37c("sound/sound_data.sdat.l", &size, 32);
    NNS_SndInit();
    NNS_SndArcInitOnMemory(&sSound->arc, sSound->data);
    NNS_SndArcPlayerSetup(NULL);
    NNS_SndHandleInit(&sSound->handle);
    for (i = 0; i < 32; i++) {
        NNS_SndPlayerSetPlayerVolume(i, 0x7f);
        NNS_SndPlayerSetPlayableSeqCount(i, 1);
        NNS_SndPlayerSetAllocatableChannel(i, 0);
    }
    sSound->task = func_ov182_021bd200(0, func_ov182_021b814c, NULL, 200);
}

void func_ov182_021b80d4(void) {
    func_ov182_021bd268(0, sSound->task);
    func_ov182_021bc4f0(&sSound);
}

void func_ov182_021b80f4(int seq) {
    NNS_SndArcPlayerStartSeqArc(&sSound->handle, 0, seq);
}

void func_ov182_021b810c(int volume) {
    NNS_SndPlayerSetVolume(&sSound->handle, volume);
}

void func_ov182_021b8120(u32 trackMask, int pitch) {
    NNS_SndPlayerSetTrackPitch(&sSound->handle, trackMask, pitch);
}

void func_ov182_021b8138(void) {
    NNS_SndPlayerStopSeq(&sSound->handle, 0);
}

static void func_ov182_021b814c(DWCUtilTask *task, void *arg) {
    NNS_SndMain();
}
