#include "types.h"
#include "gfl/sound.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nnsys/snd.h"

// A stream of music, which the title screen plays. The name is the ROM's string.

#define SND_STREAM_THREAD_PRIORITY 15
#define SND_STREAM_PLAYER 0

typedef struct {
    NNSSndStrmHandle handle;
    // Whether stream has been prepared, and only has to start
    BOOL prepared;
    u32 stream;
} SndStream;

static void GFL_SndStreamReset(void);

static SndStream *sSndStream;

void GFL_SndStreamInit(HeapID heapId) {
    sSndStream = GFL_HeapAllocate(heapId, sizeof(SndStream), FALSE, "snd_strm.c", 65);
    sys_memset(sSndStream, 0, sizeof(SndStream));
    NNS_SndArcStrmInit(SND_STREAM_THREAD_PRIORITY, func_02005ce4());
    NNS_SndArcStrmInitHandle(&sSndStream->handle);
    NNS_SndArcStrmAllocChannel(SND_STREAM_PLAYER);
}

void GFL_SndStreamFree(void) {
    if (sSndStream != NULL) {
        NNS_SndArcStrmFreeChannel(SND_STREAM_PLAYER);
        if (sSndStream->handle.player == NULL) {
            NNS_SndArcStrmReleaseHandle(&sSndStream->handle);
        }
        GFL_HeapFree(sSndStream);
        sSndStream = NULL;
    }
}

void GFL_SndStreamUpdate(void) {
}

BOOL GFL_SndStreamCheckInit(void) {
    return sSndStream != NULL;
}

void GFL_SndStreamPlay(u32 stream) {
    if (sSndStream->prepared && sSndStream->stream == stream) {
        NNS_SndArcStrmPreparedStart(&sSndStream->handle);
        return;
    }
    GFL_SndStreamReset();
    NNS_SndArcStrmStart(&sSndStream->handle, stream, 0);
    sSndStream->prepared = FALSE;
    sSndStream->stream = stream;
}

void GFL_SndStreamStop(void) {
    NNS_SndArcStrmStop(&sSndStream->handle, 0);
}

BOOL GFL_SndStreamIsPlaying(void) {
    if (NNS_SndArcStrmGetCurrentPlayingPos(&sSndStream->handle) != 0) {
        return TRUE;
    }
    return FALSE;
}

void GFL_SndStreamFadeStop(u32 frames) {
    NNS_SndArcStrmStop(&sSndStream->handle, frames);
}

static void GFL_SndStreamReset(void) {
    if (sSndStream->handle.player == NULL) {
        NNS_SndArcStrmReleaseHandle(&sSndStream->handle);
        NNS_SndArcStrmInitHandle(&sSndStream->handle);
    }
}
