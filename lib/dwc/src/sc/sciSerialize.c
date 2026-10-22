#include "competition_internal.h"
#include "nitro/std.h"

// Writing the report's numbers and GUIDs big-endian at a cursor. The file and the names in it are GameSpy's own, as its
// sciSerialize.c (UniSpySDK) has them: sciSerializeInt8 to sciSerializeGUID are its functions in its order, and its
// data are sciSerializeGUID's formats. The ROM doesn't name the file.

// The last eight bytes of a GUID
typedef struct ScGuidTail {
    gsi_u8 data[8];
} ScGuidTail;

gsi_u8 *sciSerializeInt8(gsi_u8 *dest, int value) {
    *dest = value;
    return dest + 1;
}

// The report cursor can be unaligned; copy the swapped scalar as bytes.
gsi_u8 *sciSerializeInt16(gsi_u8 *dest, gsi_i16 value) {
    gsi_u16 swapped = Sc_SwapU16(value);
    const gsi_u8 *bytes = (const gsi_u8 *)&swapped;

    dest[0] = bytes[0];
    dest[1] = bytes[1];
    return dest + sizeof(swapped);
}

gsi_u8 *sciSerializeInt32(gsi_u8 *dest, gsi_u32 value) {
    gsi_u32 swapped = Sc_SwapU32(value);
    const gsi_u8 *bytes = (const gsi_u8 *)&swapped;

    dest[0] = bytes[0];
    dest[1] = bytes[1];
    dest[2] = bytes[2];
    dest[3] = bytes[3];
    return dest + sizeof(swapped);
}

gsi_u8 *sciSerializeGUID(gsi_u8 *dest, const char *text) {
    gsi_u32 data1 = 0;
    int data2 = 0;
    int data3 = 0;
    int data4[8];
    ScGuidTail tail;
    int i;

    STD_TSScanf(text, "%8x", &data1);
    STD_TSScanf(text + 9, "%4x", &data2);
    STD_TSScanf(text + 14, "%4x", &data3);
    STD_TSScanf(text + 19, "%2x", &data4[0]);
    STD_TSScanf(text + 21, "%2x", &data4[1]);
    STD_TSScanf(text + 24, "%2x%2x%2x%2x%2x%2x", &data4[2], &data4[3], &data4[4], &data4[5], &data4[6], &data4[7]);
    for (i = 0; i < 8; i++) {
        tail.data[i] = data4[i];
    }
    dest = sciSerializeInt32(dest, data1);
    dest = sciSerializeInt16(dest, data2);
    dest = sciSerializeInt16(dest, data3);
    *(ScGuidTail *)dest = tail;
    return dest + sizeof(tail);
}
