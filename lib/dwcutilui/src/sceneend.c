#include "sceneend.h"
#include "cellinfo.h"
#include "effect.h"
#include "focus.h"
#include "headline.h"
#include "main.h"
#include "message.h"
#include "sceneinit.h"
#include "setting.h"
#include "task.h"

static void endScene(void);

void DWCi_SceneEnd(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x14);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x14);
    DWCi_ChangeScene(endScene);
}

static void endScene(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_TSKlOperate(0, 0);
        DWCi_TSKlOperate(1, 0);
        DWCi_CEINlRelease(1);
        DWCi_CEINlRelease(0);
        DWCi_HLlEnd();
        DWCi_FOCUSlEnd();
        DWCi_MSGlRelease(gDWCUtilMsg);
        DWCi_MSGlEnd();
        DWCi_SETTIGNlEnd();
        DWCi_EndUtility();
    }
}
