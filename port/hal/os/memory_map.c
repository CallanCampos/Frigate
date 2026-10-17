// Linker-script symbols of the DS memory map that NitroSDK's C still references. On the DS a symbol's address is the
// value (a stack size here); the host has no such layout. Only os_thread.c's OS_InitThread reads these, to size the
// launcher thread's stack inside DTCM, and the host's threads run on fibers with stacks of their own, so the values
// are never relied on. The fiber scheduler replaces that path (port/spikes/fibers).
unsigned char SDK_IRQ_STACKSIZE[1];
unsigned char SDK_SYS_STACKSIZE[1];
