#include "system/scroll_bar.h"
#include "types.h"

// A scroll bar's conversions between a value and the bar's position, with 8 bits of fraction. The file's name and
// the functions' are ours

u32 ScrollBar_GetValue(u32 max, u32 pos, u32 top, u32 bottom, u32 barSize) {
    u32 start = top + barSize / 2;
    u32 end = bottom - barSize / 2;

    if (pos <= start) {
        return 0;
    }
    if (pos >= end) {
        return max;
    }
    pos -= start;
    max <<= 8;
    return (pos * max / (end - start)) >> 8;
}

u32 ScrollBar_GetPos(u32 max, u32 value, u32 top, u32 bottom, u32 barSize) {
    u32 range = (bottom - top - barSize) << 8;

#ifdef PLATFORM_PC
    // A list that can't scroll has a max of 0 (the Pokédex's list of a few Pokémon, src/ov139/frame_list.c). The DS's
    // unsigned division (_u32_div_f) gives the dividend for a divisor of 0, which C leaves undefined: arm64 gives 0,
    // and x86-64 faults
    value = (max == 0 ? range * value : range * value / max) >> 8;
#else
    value = (range * value / max) >> 8;
#endif
    return top + value + barSize / 2;
}
