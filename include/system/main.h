#ifndef POKEBW2_SYSTEM_MAIN_H
#define POKEBW2_SYSTEM_MAIN_H

#include "types.h"

// The game's main file, with main, GameInit and SystemInit

// Sets a flag that func_020051e4, run by main's loop each frame, checks; the staff roll sets it while it runs
void func_020051d8(BOOL value);
void func_02005430(void);
void func_0203e874(u32 a0, u32 a1);

#endif // POKEBW2_SYSTEM_MAIN_H
