// MSL's exit (lib/msl/src/exit.c) on a host, compiled through under the private name port_msl_exit: the exit of the
// game's heap check. The ROM calls exit twice, both times in GFL_HeapDumpOnFailure (src/gfl/heap.c), and
// port/cmake/msl.cmake forces exit_calls.h into that file, which sends both calls here. On the DS, exit only notes
// whether the program is to end in failure and returns: there is no system to return to, and the dump it ended was
// compiled out. So the game goes on from the check: from the dumps that the Pokémon Musical's end and the C-Gear's
// beacon search make whatever their heaps hold (src/ov012/musical_event.c, src/ov012/game_beacon_search.c), from a heap
// deleted with blocks still in it (GFL_HeapDelete), and from an allocation that a heap has no room for, which
// GFL_HeapAllocate then ends with OS_Terminate, as the DS does: status 71 and its "port:" line
// (port/hal/os/os_terminate.c). The host's own exit would end the program at the check instead, with status 1 and no
// message, which --help promises no status 1 does (port/app/main.c). The original is compiled under the private name,
// as stubs/wstring_host.c compiles MSL's wide strings: the host build leaves MSL out for the host's C library
// (port/hal/replaced.txt), whose exit is another one.
//
// exit_calls.h comes first, as in heap.c: the host's <stdlib.h> before the rename, so that its exit, which doesn't
// return, stays the host's
#include "hal/msl/exit_calls.h"
// MSL's own file, whose exit the header has renamed
#include "../../../lib/msl/src/exit.c"
