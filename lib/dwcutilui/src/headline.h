#ifndef POKEBW2_DWCUTIL_HEADLINE_H
#define POKEBW2_DWCUTIL_HEADLINE_H

#include "types.h"

// The connection utility's highlight on BG 1 of the main screen: one of 20 screens of the highlight's characters,
// copied into a buffer and uploaded at the next VBlank.
// The file is the DWC utility library's headline.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.

// Loads the characters and the first screen of the starting screen's highlight
void DWCi_HLlInit(void);
void DWCi_HLlEnd(void);
void DWCi_HLlLoad(int index);
// The highlight of the connection being edited, on its settings' screen and on the list of connections
void DWCi_HLlLoadSet(void);
void DWCi_HLlLoadList(void);

#endif // POKEBW2_DWCUTIL_HEADLINE_H
