// MSL's critical regions for NITRO (MSL_ARM_critical_regions_NITRO.o in pokediamond's link; the file name is a guess
// from that): the state of the C library's locks, which atexit, the heap and the stream functions take through the
// inline functions of critical_regions.h. Data only. The three arrays are declared in the order that puts them in the
// ROM's: __cs_id, __cs_ref, __cs
#include "critical_regions.h"

OSMutex __cs[num_critical_regions];
u32 __cs_id[num_critical_regions];
s32 __cs_ref[num_critical_regions];
