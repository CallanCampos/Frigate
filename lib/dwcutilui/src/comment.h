#ifndef POKEBW2_DWCUTIL_COMMENT_H
#define POKEBW2_DWCUTIL_COMMENT_H

#include "types.h"

// The blank background shown while the connection utility changes screens.
// The file is the DWC utility library's comment.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_COMMENTlInit(void);
// Shows the message on the sub screen, with the digit value at its character slot if slot isn't negative
BOOL DWCi_COMMENTlDispEx(int msgIndex, int slot, int value);
BOOL DWCi_COMMENTlDelete(void);

#endif // POKEBW2_DWCUTIL_COMMENT_H
