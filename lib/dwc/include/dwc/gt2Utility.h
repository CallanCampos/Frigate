#ifndef POKEBW2_DWC_GT2UTILITY_H
#define POKEBW2_DWC_GT2UTILITY_H

#include "types.h"

// The addresses of GameSpy's GT2 as text (gt2Utility.c), in overlay 11. The file and the functions in the comments are
// named after GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Utility.s); the symbols keep
// their addresses

// gt2AddressToString: "ip:port", or just the ip or ":port", in string, or in one of two buffers of its own when it is
// NULL; returns the text
const char *func_ov011_0217ed14(u32 ip, u16 port, char *string);
// gt2StringToAddress: reads "host:port", "host" or ":port", the host a dotted address or a name to look up; FALSE for
// a bad address
BOOL func_ov011_0217ed7c(const char *string, u32 *ip, u16 *port);
// gti2MessageCheck: gives a missing message an empty text, and a length of -1 the text's length with its end
void func_ov011_0217ee60(const void **message, int *length);

#endif // POKEBW2_DWC_GT2UTILITY_H
