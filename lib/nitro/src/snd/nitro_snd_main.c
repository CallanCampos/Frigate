// NitroSDK's SND (the SDK's file is snd_main.c; the nitro_ prefix keeps the name apart from NitroSystem's snd_main.c
// for dsd, which needs file names that are unique in a module): sets up the sound driver interface and holds the mutex
// that its callers lock around their commands.
#include "nitro/os.h"
#include "nitro/snd.h"

// The state of the command queue and the packets on their way, which snd_command.c works on, then the mutex and the
// flag of SND_Init. They are here because the ROM has them before the pool of snd_command.c, whose .bss starts at the
// next 32-byte boundary, 12 bytes after this one ends. Declared in the reverse of the order they lie in
SNDCommand *sWaitingCommandListQueue[SND_PXI_FIFO_MESSAGE_BUFSIZE + 1];
SNDCommandQueue SNDi_Queue;
static OSMutex sSndMutex;
static BOOL sSndInitialized;

void SND_Init(void) {
    if (!sSndInitialized) {
        sSndInitialized = TRUE;
        OS_InitMutex(&sSndMutex);
        SND_CommandInit();
        SND_AlarmInit();
    }
}

void SNDi_LockMutex(void) {
    OS_LockMutex(&sSndMutex);
}

void SNDi_UnlockMutex(void) {
    OS_UnlockMutex(&sSndMutex);
}
