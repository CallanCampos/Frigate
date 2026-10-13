#include "vct_internal.h"

#include "nitro/mi.h"
#include "nitro/os.h"

// The library's entry points: starting and stopping it, running it a tick, and sorting what the network gives it.

typedef struct VCTLibrary {
    u32 tickCount;
    u32 receivedCount;
    BOOL initialized;
} VCTLibrary;

static VCTLibrary sLibrary;
VCTSettings gVCTSettings;

// A function that does nothing but be called with the signature of a library, which keeps the library in the link
void func_02004eb0(const char *signature);
extern const char func_02005108[];

static int DispatchPacket(int aid, void *data, int size, VCTEventResult *result);

BOOL VCT_Init(VCTConfig *config)
{
    func_02004eb0(func_02005108);
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
    if (config->buffer == NULL || config->bufferSize == 0) {
        return FALSE;
    }
    if ((int)config->buffer % 32 != 0) {
        return FALSE;
    }
    sys_memset(&gVCTSettings, 0, sizeof(gVCTSettings));
    gVCTSettings.callback = config->callback;
    gVCTSettings.userData = config->userData;
    gVCTSettings.mode = config->mode;
    gVCTSettings.unkC = 0;
    if (config->aid >= VCT_AID_MAX) {
        return FALSE;
    }
    gVCTSettings.aid = config->aid;
    if (!VCT_InitPacketPool(config->buffer, config->bufferSize)) {
        VCT_ClearPacketPool();
        return FALSE;
    }
    if (!VCT_InitSessions(config)) {
        VCT_ClearPacketPool();
        return FALSE;
    }
    if (VCT_InitAudio()) {
        sLibrary.initialized = TRUE;
        return TRUE;
    }
    VCT_ClearSessions();
    VCT_ClearPacketPool();
    return FALSE;
}

void VCT_Shutdown(void)
{
    VCT_ClearSessionCount();
    VCT_ClearSessions();
    VCT_ClearPacketPool();
    gVCTSettings.mode = 0;
    sLibrary.initialized = FALSE;
}

void VCT_Tick(void)
{
    if (!sLibrary.initialized) {
        return;
    }
    sLibrary.tickCount++;
    if ((sLibrary.tickCount & 0xf) == 0) {
        VCT_CheckTimeout();
    }
    if (VCT_SendPending()) {
        VCT_DecodeReceived();
    }
}

BOOL VCT_ReceivePacket(u8 aid, void *data, int size)
{
    VCTEventResult result;

    switch (DispatchPacket(aid, data, size, &result)) {
    case 0:
        break;
    case 1:
        gVCTSettings.callback(aid, result.event, result.session, gVCTSettings.userData);
        return TRUE;
    case 2:
        return FALSE;
    }
    return TRUE;
}

// Hands a packet to the voice or the control side by its type, returning 0 for one that has been dealt with, 1 for
// one that has given an event and 2 for one that does not belong to the library
static int DispatchPacket(int aid, void *data, int size, VCTEventResult *result)
{
    VCTVoicePacket *packet = data;
    u64 now;
    u8 type;

    if (data == NULL) {
        return 0;
    }
    if (packet->magic != VCT_PACKET_MAGIC) {
        return 2;
    }
    if (gVCTSettings.mode == 0 || !sLibrary.initialized) {
        return 0;
    }
    result->event = 0;
    result->session = NULL;
    now = clock();
    type = packet->type;
    if ((type & 0xf0) == VCT_TYPE_VOICE) {
        sLibrary.receivedCount++;
        VCT_HandleVoicePacket(aid, packet, size, now);
        return 0;
    }
    if (type == VCT_TYPE_CONTROL || type == VCT_TYPE_BROADCAST) {
        return VCT_HandleControlPacket(aid, data, size, result) != FALSE ? 1 : 0;
    }
    return 0;
}
