#ifndef POKEBW2_DWC_FRIEND_H
#define POKEBW2_DWC_FRIEND_H

#include "types.h"

// Overlay 11's friend list and login (friend.c, a guessed name: the ROM names none of DWC's files). The rest of the
// file is not written yet

// The "Date" header of the authentication server's answer, such as "Fri, 03 Mar 2006 01:28:13 GMT", which the server's
// date and time are kept from (FALSE if the header can't be read)
BOOL func_ov011_0215dffc(const char *dateHeader);

#endif // POKEBW2_DWC_FRIEND_H
