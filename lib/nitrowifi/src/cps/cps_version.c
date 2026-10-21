// The version string of cps.c (which is in overlay 11): CPS's signature, which CPS_Startup refers to with
// OSi_ReferSymbol so that the link keeps it. NitroSDK's SDK_DEFINE_MIDDLEWARE puts such a string in a section of its
// own, which the SDK's linker script gathers from every object, the overlays' too, into ARM9 main right after crt0.c.
// The configs give a file one range of each section, so the string is a file of its own here, in .text where the ROM
// has it, as syscall_twl_ltd.c is the part of syscall_twl.o in another module. The file name is descriptive.
#include "nitrowifi/cps.h"

#pragma define_section version ".text"                                                                                 \
                               ".text"                                                                                 \
                               ".text" abs32 RX

__declspec(section "version") const char CPSi_Signature[] = "[SDK+UBIQUITOUS:CPS]";
