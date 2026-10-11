#ifndef POKEBW2_DPW_INTERNAL_H
#define POKEBW2_DPW_INTERNAL_H

#include "types.h"

// What the sources of the dpw library call outside themselves, none of it with a name in the ROM

// The key of the game statistics client: the game's secret, four numbers of its scrambling, and its name
#define DPW_GAMESTATS_KEY "HZEdGCzcGGLvguqUEKQN0001d93500002dd5000000082db842b2syachi2ds"

// The friend key, as the two words it is kept in
typedef struct DpwFriendKey {
    u32 low;
    u32 high;
} DpwFriendKey;

// The C library
void *memcpy(void *dest, const void *src, u32 size);

// The Wi-Fi library's error: its type and group, and the other calls that look at and clear it
int func_020583b0(int *type, int *group);
int func_0205b5ec(void);
void func_02058490(void);


#endif // POKEBW2_DPW_INTERNAL_H
