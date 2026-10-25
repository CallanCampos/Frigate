#include "sceneend.h"
#include "cellinfo.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "focus.h"
#include "headline.h"
#include "message.h"
#include "sceneinit.h"
#include "setting.h"
#include "task.h"

static void DWCUtilMainMenu_WaitExitUtility(void);

void DWCUtilMainMenu_ExitUtility(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x14);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x14);
    DWCi_ChangeScene(DWCUtilMainMenu_WaitExitUtility);
}

static void DWCUtilMainMenu_WaitExitUtility(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_TSKlOperate(0, 0);
        DWCi_TSKlOperate(1, 0);
        DWCUtilCell_Unload(1);
        DWCUtilCell_Unload(0);
        DWCi_HLlEnd();
        DWCi_FOCUSlEnd();
        DWCi_MSGlRelease(gDWCUtilMsg);
        DWCi_MSGlEnd();
        DWCi_SETTIGNlEnd();
        DWCi_EndUtility();
    }
}
