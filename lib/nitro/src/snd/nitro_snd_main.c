// NitroSDK's SND (nitro_snd_main.c, a guess from the SDK's file names): sets up the sound driver interface and holds
// the mutex that its callers lock around their commands.
#include "nitro/os.h"
#include "nitro/snd.h"

// The state of the command queue and the packets on their way, which snd_command.c works on, then the mutex and the
// flag of SND_Init. They are here because the ROM has them before the pool of snd_command.c, whose .bss starts at the
// next 32-byte boundary, 12 bytes after this one ends. Declared in the reverse of the order they lie in
SNDCommand *SNDi_Sent[SND_PACKET_MAX + 1];
SNDCommandQueue SNDi_Queue;
static OSMutex sMutex;
static BOOL sInitialized;

// NitroSDK's SND_Init
void func_0207d670(void) {
    if (!sInitialized) {
        sInitialized = TRUE;
        OS_InitMutex(&sMutex);
        func_0207d6b8();
        func_0207db34();
    }
}

// NitroSDK's SNDi_LockMutex
void func_0207d698(void) {
    OS_LockMutex(&sMutex);
}

// NitroSDK's SNDi_UnlockMutex
void func_0207d6a8(void) {
    OS_UnlockMutex(&sMutex);
}
