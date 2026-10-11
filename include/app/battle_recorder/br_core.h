#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_CORE_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_CORE_H

#include "types.h"
#include "app/battle_recorder.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_musical_look_proc.h"
#include "app/battle_recorder/br_musical_send_proc.h"
#include "app/battle_recorder/br_graphic.h"
#include "app/battle_recorder/br_net.h"
#include "app/battle_recorder/br_proc_sys.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_sidebar.h"
#include "gfl/clact.h"
#include "gfl/proc.h"
#include "save/bsubway_save.h"
#include "save/musical_savedata.h"
#include "struct_decls.h"

// The Battle Recorder's core proc (br_core.c), which overlay 272's br_main.c runs. It holds the systems its screens
// share and runs the screens, the procs of overlays 268, 269 and 270, with br_proc_sys.c. The screens' parameters and
// procs are declared here until their own files are decompiled

// The screens, which index br_core.c's table. The previous screen's ID tells a screen's before function where it
// came from
enum {
    BR_PROCID_START,
    BR_PROCID_MENU,
    BR_PROCID_RECORD,
    BR_PROCID_BTLSUBWAY,
    BR_PROCID_RNDMATCH,
    BR_PROCID_BV_RANK,
    BR_PROCID_BV_SEARCH,
    BR_PROCID_CODEIN,
    BR_PROCID_BV_SEND,
    BR_PROCID_BV_DELETE,
    BR_PROCID_BV_SAVE,
    BR_PROCID_MUSICAL_LOOK,
    BR_PROCID_MUSICAL_SEND,
    BR_PROCID_MAX,
};

// The slots of saved battle videos: the player's own, then the downloaded ones
#define BR_RECORD_NUM 4

// What the record screen plays, in its mode: a saved video's slot, or one of these
enum {
    BR_RECORD_MODE_MINE,
    BR_RECORD_MODE_OTHER1,
    BR_RECORD_MODE_OTHER2,
    BR_RECORD_MODE_OTHER3,
    // A video found in the rankings
    BR_RECORD_MODE_BROWSE,
    // A video found by its number
    BR_RECORD_MODE_CODEIN,
};

// The saved battle videos, as the menus list them
struct BrRecordInfo {
    BOOL isValid[BR_RECORD_NUM];
    StrBuf *name[BR_RECORD_NUM];
    u32 sex[BR_RECORD_NUM];
    u64 videoNumber[BR_RECORD_NUM];
    // Whether a musical photo is saved
    BOOL hasMusicalShot;
    BOOL isInit;
};

// The data that outlives the core proc, while a battle video plays. br_main.c holds it
typedef struct {
    // The video downloaded to play
    u8 video[0x1720];
    u8 unk_1720[8];
    BrProcSysRecovery procRecovery;
    u32 recordMode;
    BrRecordInfo recordInfo;
    BrBtnRecovery btnRecovery;
    // Set from the battle setup's unkDD_1 when the video has played
    u32 unk_17bc;
    u8 bvRankSearch[4];
    u32 bvRankMode;
} BrData;

// What the core proc was started for
enum {
    BR_CORE_MODE_INIT,
    // After a battle video has played
    BR_CORE_MODE_RETURN,
};

// The core proc's parameter
typedef struct {
    // 1 to play a battle video
    u32 result;
    u32 mode;
    BrData *data;
    BattleRecorderParam *mainParam;
} BrCoreParam;

// The screens' parameters, which their before functions fill

typedef struct {
    u32 mode;
    ClActUnit *unit;
    BrFade *fade;
    BrRes *res;
    BrProcSys *procSys;
    BrSidebar *sidebar;
} BrStartProcParam;

typedef struct {
    u32 menuID;
    u32 unk4;
    // What the screen that the menu starts does, such as the record screen's mode
    u32 nextMode;
    u32 fadeType;
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    BrRecordInfo *recordInfo;
    BrBtnRecovery *btnRecovery;
    u32 *result;
} BrMenuProcParam;

typedef struct {
    // 1 to play the video
    u32 result;
    u32 unk4;
    u32 mode;
    BrFade *fade;
    BrRes *res;
    BrProcSys *procSys;
    ClActUnit *unit;
    u64 videoNumber;
    BrNet *net;
    GameData *gameData;
    u8 *unk2C;
    u8 *video;
    BOOL isRecovery;
    BrRecordInfo *recordInfo;
    u32 *unk3C;
} BrRecordProcParam;

typedef struct {
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    BSubwayScoreData *score;
    GameData *gameData;
} BrBtlSubwayProcParam;

typedef struct {
    ClActUnit *unit;
    BrFade *fade;
    BrRes *res;
    BrProcSys *procSys;
    GameData *gameData;
    void *record;
} BrRndMatchProcParam;

typedef struct {
    u32 mode;
    BOOL isReturn;
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    BrNet *net;
    u8 *video;
    u32 searchUnk20;
    u32 searchUnk24;
    u8 *search;
} BrBvRankProcParam;

typedef struct {
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    GameData *gameData;
    u32 unk14;
    u32 unk18;
} BrBvSearchProcParam;

typedef struct {
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    u64 videoNumber;
} BrCodeInProcParam;

typedef struct {
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    BrNet *net;
    GameData *gameData;
} BrBvSendProcParam;

typedef struct {
    u32 mode;
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrProcSys *procSys;
    GameData *gameData;
    BrRecordInfo *recordInfo;
    // TRUE if a video was deleted
    BOOL isDelete;
} BrBvDeleteProcParam;

typedef struct {
    BrFade *fade;
    ClActUnit *unit;
    BrRes *res;
    BrNet *net;
    BrProcSys *procSys;
    GameData *gameData;
    BrRecordInfo *recordInfo;
    u64 videoNumber;
    u32 unk24;
    // TRUE if the video was saved, to the slot
    BOOL isSave;
    u32 saveSlot;
} BrBvSaveProcParam;

// The screens' procs
extern const GameProcFunctions data_ov268_021c20e8;
extern const GameProcFunctions data_ov268_021c20f4;
extern const GameProcFunctions data_ov268_021c211c;
extern const GameProcFunctions data_ov268_021c2234;
extern const GameProcFunctions data_ov268_021c2390;
extern const GameProcFunctions data_ov268_021c2574;
extern const GameProcFunctions data_ov268_021c26b8;
extern const GameProcFunctions data_ov268_021c26c4;
extern const GameProcFunctions data_ov268_021c2768;
extern const GameProcFunctions data_ov268_021c2774;
extern const GameProcFunctions data_ov268_021c27b8;

extern const GameProcFunctions BR_CORE_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_CORE_H
