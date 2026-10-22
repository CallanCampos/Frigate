#ifndef POKEBW2_DWC_GT2UTILITY_H
#define POKEBW2_DWC_GT2UTILITY_H

#include "dwc/gt2Main.h"

// The addresses of GameSpy's GT2 as text (gt2Utility.c), in overlay 11. The file and its functions are named after
// GameSpy's, as pret/pokerevo's Wii build has them (asm/SDK/DWC/gamespy/gt2Utility.s) and GameSpy's gt2Utility.c names
// them; gt2AddressToString, which gt2.c calls, keeps its address for now, with its name in the comment

// gt2AddressToString: "ip:port", or just the ip or ":port", in string, or in one of two buffers of its own when it is
// NULL; returns the text
const char *func_ov011_0217ed14(unsigned int ip, unsigned short port, char string[22]);
// Reads "host:port", "host" or ":port", the host a dotted address or a name to look up; gsi_false for
// a bad address
GT2Bool gt2StringToAddress(const char *string, unsigned int *ip, unsigned short *port);
// Gives a missing message an empty text, and a length of -1 the text's length with its end
void gti2MessageCheck(const GT2Byte **message, int *length);

#endif // POKEBW2_DWC_GT2UTILITY_H
