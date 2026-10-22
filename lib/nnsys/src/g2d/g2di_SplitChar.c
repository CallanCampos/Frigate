#include "nnsys/g2d.h"

// NitroSystem's string splitters, which read a string's next character for a font. The game keeps only the UTF-16 one.
// The file name is NitroSystem's, as ntrtwl's reconstruction of NitroSystem has it

u16 NNSi_G2dSplitCharUTF16(const void **ppChar) {
    const u16 *pChar = (const u16 *)*ppChar;
    u16 c = *pChar;

    *ppChar = pChar + 1;
    return c;
}
