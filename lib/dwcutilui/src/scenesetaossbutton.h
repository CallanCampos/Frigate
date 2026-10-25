#ifndef POKEBW2_DWCUTIL_SCENESETAOSSBUTTON_H
#define POKEBW2_DWCUTIL_SCENESETAOSSBUTTON_H

#include "types.h"

// The connection utility's AOSS screen (libaoss.h): it runs the setup, which B or closing the lid
// cancels, and after a success keeps its message for 1080 frames, then saves the keys and goes on to the connection
// test (scenesetaosscomplete.h). A failure shows a dialog, and the utility goes back to the choice of the setup.
// The file is the DWC utility library's scenesetaossbutton.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetAossButton(void);

#endif // POKEBW2_DWCUTIL_SCENESETAOSSBUTTON_H
