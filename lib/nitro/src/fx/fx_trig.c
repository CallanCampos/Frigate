// NitroSDK libfx's fx_trig.c: FX_AcosIdx, a binary search of the cosines in FX_SinCosTable_. The file name is a guess
// after the SDK's fx_trig.h header; the split from fx.c is only by the function boundary and the order in the library.
#include "nitro/fx.h"

u16 FX_AcosIdx(fx32 x) {
    int lo;
    int hi;
    int mid;
    fx16 key;

    if (x >= 0) {
        lo = 0;
        hi = 1 << 10;
    } else {
        lo = 1 << 10;
        hi = lo * 2;
    }
    key = x;
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        if (key == FX_SinCosTable_[mid * 2 + 1]) {
            break;
        }
        if (FX_SinCosTable_[mid * 2 + 1] < key) {
            hi = mid - 1;
        } else {
            lo = mid + 1;
        }
    }
    return ((mid * 2 + 1) << 16) / 0x2000;
}
