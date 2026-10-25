#ifndef POKEBW2_DWCUTIL_SCENESETRAKUBUTTON1_H
#define POKEBW2_DWCUTIL_SCENESETRAKUBUTTON1_H

#include "types.h"

// The connection utility's connection test after its second step (rakuraku.h): it waits for the connection,
// takes what the test found into the connection being edited and goes on to the message of the success
// (scenesetrakucomplete.h). A failure, or a connection that is not with a WEP key or none, shows a dialog, and B,
// closing the lid or the button end the test and go back to the choice of the setup of a connection.
// The file is the DWC utility library's scenesetrakubutton1.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's
// SDK code to that library's objects (layout/symbols_sdk.txt) shows it.
void DWCi_SceneSetRakuButton1(void);

#endif // POKEBW2_DWCUTIL_SCENESETRAKUBUTTON1_H
