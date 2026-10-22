#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_audio_internal.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"
#include "vct_session_internal.h"

// The library's entry points: starting and stopping it, running it a tick, and sorting what the network gives it.

struct VCTLibrary {
    u32 tickCount;
    u32 receivedCount;
    BOOL initialized;
};

static VCTLibrary sLibrary;
VCTi_Globals vct;

static int VCTi_HandleData(int aid, void *data, int size, VCTResult *result);

BOOL VCT_Init(VCTConfig *config) {
    SDK_USING_MIDDLEWARE(VCTi_Signature);
    if (config == NULL) {
        return FALSE;
    }
    if (sLibrary.initialized) {
        return TRUE;
    }
    if (config->mode != 1 && config->mode != 2 && config->mode != 3) {
        return FALSE;
    }
    if (config->callback == NULL) {
        return FALSE;
    }
    if (config->audioBuffer == NULL || config->audioBufferSize == 0) {
        return FALSE;
    }
    if ((int)config->audioBuffer % 32 != 0) {
        return FALSE;
    }
    MI_CpuFill8(&vct, 0, sizeof(vct));
    vct.callback = config->callback;
    vct.userData = config->userData;
    vct.SSPMode = config->mode;
    vct.listenMode = 0;
    if (config->aid >= VCT_AID_MAX) {
        return FALSE;
    }
    vct.myAID = config->aid;
    if (!vct_init_packet_buffer(config->audioBuffer, config->audioBufferSize)) {
        vct_cleanup_packet_buffer();
        return FALSE;
    }
    if (!vct_init_ssp(config)) {
        vct_cleanup_packet_buffer();
        return FALSE;
    }
    if (vct_init_audio()) {
        sLibrary.initialized = TRUE;
        return TRUE;
    }
    vct_finish_ssp();
    vct_cleanup_packet_buffer();
    return FALSE;
}

void VCT_Cleanup(void) {
    vct_finish_audio();
    vct_finish_ssp();
    vct_cleanup_packet_buffer();
    vct.SSPMode = 0;
    sLibrary.initialized = FALSE;
}

void VCT_Main(void) {
    if (!sLibrary.initialized) {
        return;
    }
    sLibrary.tickCount++;
    if ((sLibrary.tickCount & 0xf) == 0) {
        vct_check_timeout();
    }
    if (vct_flush_send_buffer()) {
        vct_decode_audio_buffer();
    }
}

BOOL VCT_HandleData(u8 aid, u8 *data, int size) {
    VCTResult result;

    switch (VCTi_HandleData(aid, data, size, &result)) {
    case 0:
        break;
    case 1:
        vct.callback(aid, result.event, result.session, vct.userData);
        return TRUE;
    case 2:
        return FALSE;
    }
    return TRUE;
}

// Hands a packet to the voice or the control side by its type, returning 0 for one that has been dealt with, 1 for
// one that has given an event and 2 for one that does not belong to the library
static int VCTi_HandleData(int aid, void *data, int size, VCTResult *result) {
    AudioHeader *voice = data;
    SSPHeader *control = data;
    u64 now;

    if (data == NULL) {
        return 0;
    }
    if (voice->magic != VCT_PACKET_MAGIC) {
        return 2;
    }
    if (vct.SSPMode == 0 || !sLibrary.initialized) {
        return 0;
    }
    result->event = 0;
    result->session = NULL;
    now = OS_GetTick();
    if ((voice->header & 0xf0) == VCT_HEADER_AUDIO) {
        sLibrary.receivedCount++;
        vct_handle_audio(aid, voice, size, now);
        return 0;
    }
    if (control->method == VCT_METHOD_RESPONSE || control->method == VCT_METHOD_REQUEST) {
        return vct_handle_ssp(aid, control, size, result) != FALSE ? 1 : 0;
    }
    return 0;
}
