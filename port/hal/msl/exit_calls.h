// MSL's exit for the game files that call it. port/cmake/msl.cmake forces this header into them (src/gfl/heap.c, whose
// heap check, GFL_HeapDumpOnFailure, holds the ROM's only two calls of exit), and their calls then reach MSL's own
// exit, compiled through under the private name port_msl_exit (exit_host.c), instead of the host C library's. MSL's
// only notes whether the program is to end in failure and returns, so the game goes on from the check as it does on the
// DS; the host's would end the program there with status 1 and no message. The game's files don't change, and
// port/tools/gen_stubs.py fails the build when another of the game's objects calls the host's exit.
//
// The host's <stdlib.h> comes first, before the rename. It declares the host's exit as a function that doesn't return,
// and read after the rename, that declaration would be port_msl_exit's too: the compiler would then drop the code after
// the check's first call, as MWCC does when a header named stdlib.h declares exit (lib/nitro/include/nitro/os.h). Read
// here first, it can be included again by the file, its headers or port/include/port_host.h, which its include guard
// makes a no-op.
#ifndef PORT_HAL_MSL_EXIT_CALLS_H
#define PORT_HAL_MSL_EXIT_CALLS_H

#include <stdlib.h>

// MSL's exit: notes whether the program is to end in failure, and returns
void port_msl_exit(int status);

#define exit port_msl_exit

#endif // PORT_HAL_MSL_EXIT_CALLS_H
