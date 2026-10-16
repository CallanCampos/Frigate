// TwlSDK's shared WRAM manager, the part in ARM9 main: a call into the LTD autoload on a DSi. The file name is a guess;
// the binary can't tell this from mi_wram.c, which follows (neither has data).
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/mi.h"

void func_02078000(void) {
    if (OS_IsRunOnTwl()) {
        func_02768818();
    }
}
