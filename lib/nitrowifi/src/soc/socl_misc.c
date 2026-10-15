#include "nitrowifi/soc.h"
#include "socl_internal.h"

// SOCLi_RoundUp4: the only function of pokediamond's socl_misc.o

u32 SOCLi_RoundUp4(u32 value) {
    return (value + 3) & ~3;
}
