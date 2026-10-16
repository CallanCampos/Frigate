// The Metrowerks runtime's call_static_initializers.c: runs the static initializers, the functions of the table that
// the linker lays out at ARM9_CTOR_START, up to its zero entry. NitroSDK's startup code calls it right after _fp_init.
// The runtime's name for the function is __call_static_initializers.
#include "call_static_initializers.h"
#include "types.h"

void __call_static_initializers(void) {
    void (**ctor)(void);

    for (ctor = ARM9_CTOR_START; ctor != NULL && *ctor != NULL; ctor++) {
        (*ctor)();
    }
}
