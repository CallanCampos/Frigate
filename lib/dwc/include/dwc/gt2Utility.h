#ifndef POKEBW2_DWC_GT2UTILITY_H
#define POKEBW2_DWC_GT2UTILITY_H

#include "types.h"

// The addresses of GameSpy's GT2 as text (gt2Utility.c), in overlay 11. The file and its functions are named after
// GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Utility.s) and GameSpy's gt2Utility.c names
// them; gt2AddressToString, which gt2.c calls, keeps its address for now, with its name in the comment

// gt2AddressToString: "ip:port", or just the ip or ":port", in string, or in one of two buffers of its own when it is
// NULL; returns the text
const char *func_ov011_0217ed14(u32 ip, u16 port, char *string);
// Reads "host:port", "host" or ":port", the host a dotted address or a name to look up; FALSE for
// a bad address
BOOL gt2StringToAddress(const char *string, u32 *ip, u16 *port);
// Gives a missing message an empty text, and a length of -1 the text's length with its end
void gti2MessageCheck(const void **message, int *length);

#endif // POKEBW2_DWC_GT2UTILITY_H
