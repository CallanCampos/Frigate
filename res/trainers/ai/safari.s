#include "asm/tr_ai.inc"

// AI flag 12, Safari. This is Gen 4's Safari routine, written for Gen 4's commands, where Dummy3E takes an argument.
// This game's Dummy3E reads no argument, so the script would go on to read the 2 as a command.

Safari_Main:
    Dummy3E
    .4byte 2
    Dummy3F
    Escape
    .balign 4, 0
