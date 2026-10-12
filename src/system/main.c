#include "types.h"
#include "gfl/g3d.h"
#include "gfl/net.h"
#include "gfl/net_system.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/config.h"
#include "save/high_link.h"
#include "save/join_avenue.h"
#include "save/save_control.h"
#include "save/trainer_data_savedata.h"
#include "system/boot_proc.h"
#include "system/brightness.h"
#include "system/game_beacon.h"
#include "system/gfl_use.h"
#include "system/machine_use.h"
#include "system/main.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wih_dwc.h"
#include "system/wipe.h"

// The game's main file: the main loop, the game's setup and its per-frame updates. The file name is a guess, as the
// ROM does not embed it. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except those of the
// statics

static BOOL IsMainMemoryPriorityFree(void);
static void GFLAppIRQHBlank(void);
static void GFLAppIRQVBlank(void);
static void GameUpdateVBlank(void);
static void GameInit(void);
static void GameUpdate(void);
static void setMonoSnd(const Config *config);

// Not static: it is the first object of .bss, and the boot code's parameter block (0x02004fb8), which is not
// decompiled, points at the start of .bss through this symbol
BOOL g_MainMemoryPriorityBlocked;

void main(void) {
    SystemInit();
    GFLAppInit();
    CPU_SetIRQFunc(OS_IE_H_BLANK, GFLAppIRQHBlank);
    CPU_SetIRQFunc(OS_IE_V_BLANK, GFLAppIRQVBlank);
    CPU_EnableInterrupts(OS_IE_V_BLANK | OS_IE_H_BLANK);
    (void)OS_EnableIrq();
    gfxSetHBlankIRQEnabled(TRUE);
    gfxSetVBlankIRQEnabled(TRUE);
    CPU_IRQEnable();
    GameInit();

    while (TRUE) {
        BOOL priority;

        func_02005430();
        GFLAppUpdate();
        GameUpdate();
        func_02005668();
        GFL_G3DSysCheckSwapBuffers();

        // The ARM9 has the main memory first while it waits for the VBlank, and the sound thread waits with it
        priority = IsMainMemoryPriorityFree();
        if (priority) {
            MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
        }
        if (!scheduler_is_thread_ended(&g_GFLSndThread)) {
            scheduler_join_thread(&g_GFLSndThread, NULL);
        }
        irq_waitFor(TRUE, OS_IE_V_BLANK);
        GameUpdateVBlank();
        if (!scheduler_is_thread_ended(&g_GFLSndThread)) {
            scheduler_start_thread(&g_GFLSndThread);
        }
        if (priority) {
            MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
        }
    }
}

void func_020051d8(BOOL blocked) {
    g_MainMemoryPriorityBlocked = blocked;
}

// FALSE while the network, a sound stream or func_020051d8 needs the ARM7 to keep its priority
static BOOL IsMainMemoryPriorityFree(void) {
    if (func_0203ff6c() > 1) {
        return FALSE;
    }
    if (func_02042b00()) {
        return FALSE;
    }
    if (func_02042b20()) {
        return FALSE;
    }
    if (GFL_SndStreamCheckInit()) {
        return FALSE;
    }
    if (g_MainMemoryPriorityBlocked == FALSE) {
        return TRUE;
    }
    return FALSE;
}

static void GFLAppIRQHBlank(void) {
    GFL_HBlankTCBUpdate();
}

static void GFLAppIRQVBlank(void) {
    GFL_VBlankExecCallback();
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
    func_0207829c(3);
}

static void GameUpdateVBlank(void) {
    SystemUpdateVBlank();
    GFL_VBlankUpdate();
}

static void GameInit(void) {
    GFL_OvlLoad(OVERLAY_ID(12));
    GFL_OvlLoad(OVERLAY_ID(27));
    GFL_TextRndInit(0);
    g_MainMemoryPriorityBlocked = FALSE;
    func_020425a0(1, 0, 1, 12);
    setupDownloadPlayConfig(12);
    downloadPlayConfigForWifi();
    GCTX_ProcMgrQueueProc(OVERLAY_NONE, &EVENT_TITLE_FUNCS, NULL);

    SaveControl_Create(7);
    func_02008ab4(&getTrainerDataBlkAddress(SaveControl_GetInstance())->config);
    saveMainSaveAreaAddress(SaveControl_GetInstance());
    setImportantJABlockAddresses(SaveControl_GetInstance());

    GFL_NetHeapInit();
    GFL_NetErrInit(8);
    GFL_DebugSetAssertFailCallback(AssertFailErrorDisp);
    PML_SpeciesNamesResidentInit(0);
    PML_PersonalInit(0);
    GFL_SndInit();
    PokeVoice_Init(1, getResIDFunc, getWaveInfoFunc);
    setMonoSnd(&getTrainerDataBlkAddress(SaveControl_GetInstance())->config);
    initKeyBlk();
    GameBeaconSys_Create(1);
    PassPower_Init();
}

static void GameUpdate(void) {
    BrightnessController_Update();
    GFL_WipeMain();
    func_02011c78();
    GFL_SndUpdate();
    PokeVoice_Update();
    GameBeaconSys_Update();
    PassPower_Update();
}

// Sets the sound's output from the options
static void setMonoSnd(const Config *config) {
    if (getSoundFlags(config) == 0) {
        func_02005c80(TRUE);
    } else {
        func_02005c80(FALSE);
    }
}
