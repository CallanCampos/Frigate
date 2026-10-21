// exit, which ARM9 main links between TwlSDK's system call library (syscall_twl.c) and MSL C's atexit
// (abort_exit_arm.c). With no system to return to, it only records whether the program ends in failure; it neither
// calls the atexit functions nor stops. It is built with another compiler than the rest of MSL (lib/msl/library.toml),
// so it may come from another of CodeWarrior's libraries; the file name is descriptive
#include "nitro/os.h"

// Whether exit's status was a failure (swan's name)
static u8 g_ExitIsError;

void exit(int status) {
    g_ExitIsError = status != 0;
}
