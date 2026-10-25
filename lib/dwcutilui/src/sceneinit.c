#include "sceneinit.h"
#include "cellinfo.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "focus.h"
#include "graphics.h"
#include "headline.h"
#include "message.h"
#include "nitro/gx.h"
#include "scenemenu.h"
#include "scenesetselectap.h"
#include "setting.h"

static void DWCUtilMainMenu_FadeInUtility(void);
static void DWCUtilMainMenu_WaitFadeInUtility(void);

// The message files by language
static char *sMsgPaths[7] = {
    "msg/jap.bmg.l", "msg/eng.bmg.l", "msg/fre.bmg.l", "msg/ger.bmg.l",
    "msg/ita.bmg.l", "msg/spa.bmg.l", "msg/kor.bmg.l",
};

DWCUtilMsg *gDWCUtilMsg;

void DWCUtilMainMenu_InitUtility(void) {
    DWCi_SETTIGNlInit();
    DWCi_MSGlInit();
    DWCi_FOCUSlInit();
    DWCi_HLlInit();
    DWCi_COMMENTlInit();
    if (DWCi_Language() == 1 && DWCi_Flag(2)) {
        gDWCUtilMsg = DWCi_MSGlRead("msg/usa.bmg.l");
    } else {
        gDWCUtilMsg = DWCi_MSGlRead(sMsgPaths[DWCi_Language()]);
    }
    DWCUtilCell_Load(1, DWCi_Char("char/jtMain.nce.l"));
    DWCUtilCell_Load(0, DWCi_Char("char/jbMain.nce.l"));
    DWCUtilGfxResource_Load("char/jtBgMain.ncg.l", GXS_LoadBG1Char);
    DWCUtilGfxResource_Load("char/jtBgMain.ncl.l", GXS_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jtObjMain.ncg.l", GXS_LoadOBJ);
    DWCUtilGfxResource_Load("char/xtObjMain.ncl.l", GXS_LoadOBJPltt);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jbObjMain.ncg.l", GX_LoadOBJ);
    DWCUtilGfxResource_Load("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    switch (DWCi_Entry()) {
    case 0:
        DWCUtilGfxResource_Load("char/jtTop.nsc.l", GXS_LoadBG1Scr);
        break;
    case 1:
        DWCUtilGfxResource_Load("char/jtStep1.nsc.l", GXS_LoadBG1Scr);
        break;
    }
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG1);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG1);
    DWCi_ChangeScene(DWCUtilMainMenu_FadeInUtility);
}

static void DWCUtilMainMenu_FadeInUtility(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG1, 0x14);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG1, 0x14);
    DWCi_ChangeScene(DWCUtilMainMenu_WaitFadeInUtility);
}

static void DWCUtilMainMenu_WaitFadeInUtility(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        switch (DWCi_Entry()) {
        case 0:
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneMenu);
            break;
        case 1:
            DWCi_SetParam_(1, 1);
            DWCi_ChangeScene(DWCUtilScreenAp_Init);
            break;
        }
    }
}
