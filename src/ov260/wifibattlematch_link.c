// wifibattlematch_link.c, in overlay 260 (a descriptive name: the ROM names only the next file,
// wifibattlematch_net.c): the proc of the Wi-Fi link that overlay 290 (wifibattlematch_sys.c) chains. It loads
// overlay 262, which the link's screens are made of, and runs the proc of the random matchups of the field or the
// one of the Wi-Fi competition, which the title screen's menu starts

#include "app/wifibattlematch_link.h"
#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"

// Where overlay 290 started the link from (WifiBattleMatchParam.unk8)
enum {
    START_TITLE,
    START_FIELD,
};

static BOOL WifiBattleMatchLink_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL WifiBattleMatchLink_Main(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions WIFIBATTLEMATCH_LINK_PROC_FUNCTIONS = {
    WifiBattleMatchLink_Init,
    WifiBattleMatchLink_Main,
    WifiBattleMatchLink_Exit,
};

static BOOL WifiBattleMatchLink_Init(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;

    GFL_OvlLoad(OVERLAY_ID(262));
    switch (linkParam->param->unk8) {
    case START_FIELD:
        return WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS.init(proc, state, param, work);
    case START_TITLE:
        return WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS.init(proc, state, param, work);
    default:
        GFL_ASSERT(0);
        return FALSE;
    }
}

static BOOL WifiBattleMatchLink_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;
    BOOL result;

    switch (linkParam->param->unk8) {
    case START_FIELD:
        result = WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS.exit(proc, state, param, work);
        break;
    case START_TITLE:
        result = WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS.exit(proc, state, param, work);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    GFL_SndStop();
    GFL_OvlUnload(OVERLAY_ID(262));
    return result;
}

static BOOL WifiBattleMatchLink_Main(GameProc *proc, u32 *state, void *param, void *work) {
    WifiBattleMatchLinkParam *linkParam = param;

    switch (linkParam->param->unk8) {
    case START_FIELD:
        return WIFIBATTLEMATCH_LINK_RANDOM_PROC_FUNCTIONS.main(proc, state, param, work);
    case START_TITLE:
        return WIFIBATTLEMATCH_LINK_COMPETITION_PROC_FUNCTIONS.main(proc, state, param, work);
    default:
        GFL_ASSERT(0);
        return FALSE;
    }
}
