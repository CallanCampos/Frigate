#include "snd_internal.h"

// NitroSystem's sequence archives: a sequence of one by index, NULL when there is none. It stands alone between
// snd_arc_stream.c and snd_fader.c, and is called only by snd_arc_player.c; the name is NitroSystem's, as ntrtwl's
// reconstruction of NitroSystem has it, and the SDK's file is seqdata.c

const NNSSndSeqArcSeqInfo *NNSi_SndSeqArcGetSeqInfo(const NNSSndSeqArc *seqArc, int index) {
    const NNSSndSeqArcSeqInfo *info;

    if (index < 0) {
        return NULL;
    }
    if (index >= seqArc->count) {
        return NULL;
    }

    info = &seqArc->info[index];
    if (info->offset == NNS_SND_SEQ_ARC_INVALID_OFFSET) {
        return NULL;
    }
    return info;
}
