#include "dsprot/dsprot_internal.h"

// The addresses of DS Protect's whole-code cipher functions, which its decryption of its own functions calls. They
// are in rodata between the detection code's and the cipher's, so they are taken to be an object of their own. The ROM
// doesn't name this file; it is named for what it holds.

const u32 DSProt_DecryptCodeEntry = (u32)(DSProt_DecryptCodeWithKey + 0x2200);
const u32 DSProt_EncryptCodeEntry = (u32)(DSProt_EncryptCodeWithKey + 0x2200);
