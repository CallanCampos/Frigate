// DWC's auto-connect, its core (TWL-DWC 5.3, ac). The file name is descriptive: the file creates and destroys the
// control block and the other blocks of memory that auto-connect keeps, and steps the state machine (DWC_AC_Process)
// whose states the other ac files run: 1 starts the wireless manager, 2 to 6 the search, 7 and 8 the connection to the
// candidate, 9 its end, 10 to 15 the sockets coming up and the connection kept, 16 the connection up, 17 the teardown
// and 18 the failure. The next function, at 0205ac24, is the first of dwc_connectinet.c.

#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The pointers to the five blocks of memory, which DWCi_AC_GetMemPtr gives out by their flags
typedef struct DWCAcBlocks {
    void *wcm;                  // DWC_AC_WORK_WCM
    void *socConfig;            // DWC_AC_WORK_SOC_CONFIG
    DWCNetcheckParam *memFuncs; // DWC_AC_WORK_MEMFUNCS
    ACHOLD *main;               // DWC_AC_WORK_MAIN
    ACWORK *search;             // DWC_AC_WORK_SEARCH
} DWCAcBlocks;

static DWCAcBlocks sBlocks;

static void DWC_AC_SetSpecifyAp(const u8 *ssid, const u8 *key, u32 wepMode);
static void *DWCi_AC_Alloc(u32 flag, u32 size);
static void DWCi_AC_Free(u32 flag, void *block, u32 size);
static void DWCi_AC_FreeAll(void);
static void DWCi_AC_SetPhase(u8 state);
static void Free_Disused(void);
static u8 CheckDuplicate(void);
static BOOL DWCAc_CheckEula(void);
static BOOL DWCAc_CheckWirelessAllowed(void);

// Whether a DSi setting's security byte says it uses WPA: any byte from 0x10 on that is not 0xff, which a setting not
// in use has
#define SETTING_USES_WPA(setting) ((setting).base.ap.setType >= 0x10 && (setting).base.ap.setType != 0xff)

// Creates auto-connect: the control block and the four more blocks of memory, the settings that it works from (the DS's
// and, on a DSi, the DSi's), and the wireless manager. FALSE if the wireless manager does not start
BOOL DWC_AC_Create(DWCACConfig *param) {
    ACHOLD *main;
    DWCNetcheckParam *funcs;
    ACWORK *work;
    s32 result;

    sBlocks.main = param->alloc(1, sizeof(ACHOLD));
    MI_CpuClear32(sBlocks.main, sizeof(ACHOLD));
    main = sBlocks.main;
    main->alloc = param->alloc;
    main->free = param->free;
    main->phase = 1;
    main->endSequence = 1;
    main->allocState = DWC_AC_WORK_MAIN;
    sBlocks.search = DWCi_AC_Alloc(DWC_AC_WORK_SEARCH, sizeof(ACWORK));
    sBlocks.wcm = DWCi_AC_Alloc(DWC_AC_WORK_WCM, 0x4800);
    sBlocks.socConfig = DWCi_AC_Alloc(DWC_AC_WORK_SOC_CONFIG, 0x58);
    sBlocks.memFuncs = DWCi_AC_Alloc(DWC_AC_WORK_MEMFUNCS, sizeof(DWCNetcheckParam));
    MI_CpuClear32(sBlocks.search, sizeof(ACWORK));
    MI_CpuClear32(sBlocks.wcm, 0x4800);
    MI_CpuClear32(sBlocks.socConfig, 0x58);
    MI_CpuClear32(sBlocks.memFuncs, sizeof(DWCNetcheckParam));
    work = sBlocks.search;
    work->dmaNo = param->dmaNo;
    work->radioOn = param->powerMode;
    work->rwin = param->rwin;
    work->ignoreEula = param->ignoreEula;
    funcs = sBlocks.memFuncs;
    funcs->alloc = param->alloc;
    funcs->free = param->free;
    funcs->bmworkarea = NULL;
    work->mode = param->mode;
    work->skipConnectionTest = param->skipConnectionTest;
    work->rssiPacked = param->rssiPacked;
    DWCi_BM_GetApInfo(work);
    if (OS_IsRunOnTwl()) {
        work->preferWpa = 0;
        DWCBm_ReadTwlSettings(work->dsiSettings);
        if (work->mode == 0) {
            if (SETTING_USES_WPA(work->dsiSettings[0]) || SETTING_USES_WPA(work->dsiSettings[1]) ||
                SETTING_USES_WPA(work->dsiSettings[2])) {
                work->preferWpa = 1;
            }
        } else if (work->mode == 0xb) {
            if (SETTING_USES_WPA(work->dsiSettings[0])) {
                work->preferWpa = 1;
            }
        } else if (work->mode == 0xc) {
            if (SETTING_USES_WPA(work->dsiSettings[1])) {
                work->preferWpa = 1;
            }
        } else if (work->mode == 0xd) {
            // BUG: the third setting's byte is tested for 0x10 and the second's for 0xff
#ifdef BUGFIX
            if (SETTING_USES_WPA(work->dsiSettings[2])) {
#else
            if (work->dsiSettings[2].base.ap.setType >= 0x10 && work->dsiSettings[1].base.ap.setType != 0xff) {
#endif
                work->preferWpa = 1;
            }
        }
    }
    if (OS_IsRunOnTwl()) {
        if (work->rssiPacked == 1) {
            result = WCM_Init(sBlocks.wcm, 0x4800);
        } else {
            result = WCM_InitEx(sBlocks.wcm, 0x4800, 2);
        }
    } else {
        result = WCM_Init(sBlocks.wcm, 0x4800);
    }
    if (result == 1 || result > 4) {
        DWCi_AC_FreeAll();
        return FALSE;
    }
    return TRUE;
}

// Runs the state at hand once and moves to the one that follows. Returns 1 once the connection is up (state 16, or 2 if
// CheckDuplicate finds another candidate like the one connected to), -1 when it has failed for good (state
// 18), else 0. Once it is up or has failed, the search's blocks of memory are let go
s32 DWC_AC_Process(void) {
    u8 state = DWCi_AC_GetPhase();
    s32 result;

    if (state == 1) {
        if (DWCAc_CheckEula()) {
            if (DWCAc_CheckWirelessAllowed()) {
                state = DWCi_AC_Start();
            } else {
                DWCi_AC_SetError(8);
                state = DWCi_AC_Error();
            }
        } else {
            DWCi_AC_SetError(7);
            state = DWCi_AC_Error();
        }
    } else if (state < 7) {
        u32 irq = OS_DisableInterrupts();

        state = DWCi_AC_SearchAP();
        DWCi_AC_SetPhase(state);
        OS_RestoreInterrupts(irq);
    } else if (state < 9) {
        state = DWCi_AC_ConnectAP();
    } else if (state < 10) {
        state = DWCi_AC_ConnectRetryAP();
    } else if (state < 16) {
        state = DWCi_AC_ConnectTest();
    } else if (state == 17) {
        state = DWCi_AC_Error();
    }
    DWCi_AC_SetPhase(state);
    if (state == 16) {
        result = CheckDuplicate();
        Free_Disused();
        return result;
    }
    if (state != 18) {
        return 0;
    }
    Free_Disused();
    return -1;
}

// What the state is, in a few values for the caller: 0 before the search, 1 searching (states 2 to 6), 2 connecting (7
// and 8), 3 the sockets coming up (10, 12 to 15), 4 for 9, 11 and 17, which go on to another candidate or to the
// teardown, 5 connected (16); for a failure (18), the error code of dwc_ac_result.c's DWCi_AC_GetResult
s32 DWC_AC_GetStatus(void) {
    u8 state = DWCi_AC_GetPhase();

    if (state <= 1) {
        return 0;
    }
    if (state < 7) {
        return 1;
    }
    if (state == 9) {
        return 4;
    }
    if (state < 10) {
        return 2;
    }
    if (state == 11) {
        return 4;
    }
    if (state < 16) {
        return 3;
    }
    if (state == 16) {
        return 5;
    }
    if (state == 17) {
        return 4;
    }
    return DWCi_AC_GetResult();
}

// The type of the access point that the connection is with, 0xff while there is none
u8 DWC_AC_GetApType(void) {
    u8 type = 0xff;
    u8 state = DWCi_AC_GetPhase();

    if (state >= 10 && state <= 16) {
        type = sBlocks.main->type;
    }
    return type;
}

// Copies the 10 bytes of spot information to spotInfo, if the connection is with an access point that has any (the type
// 4 or 8). TRUE if it did
BOOL DWC_AC_GetApSpotInfo(u8 *spotInfo) {
    BOOL found = FALSE;
    u8 state = DWCi_AC_GetPhase();

    if (state >= 10 && state <= 16) {
        ACHOLD *main = sBlocks.main;

        if (main->type == 4 || main->type == 8) {
            MI_CpuCopy8(main->apSpotInfo, spotInfo, sizeof(main->apSpotInfo));
            found = TRUE;
        }
    }
    return found;
}

// The key mode of the setting that the connection is made with
u8 DWCAc_GetKeyMode(void) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);

    return main->keyMode;
}

// Takes auto-connect down. Once it has no state or is over (18), lets its memory go and returns TRUE; else steps the
// teardown that the state at hand needs
BOOL DWC_AC_Destroy(void) {
    u8 state = DWCi_AC_GetPhase();

    if (state == 0 || state == 18) {
        DWCi_AC_FreeAll();
        return TRUE;
    }
    DWCi_AC_CloseNetwork(&state);
    DWCi_AC_SetPhase(state);
    return FALSE;
}

// Gives the connection to make by hand: the first setting of the DS is made from the SSID, the WEP key and its mode and
// the fourth argument, and the search is made to try that and nothing else. The spot information, if any, and the type
// that the access point is given are what the connection reports
void DWC_AC_SetSpecifyApEx(const u8 *ssid, const u8 *key, u32 wepMode, u16 unk_ea, const u8 *spotInfo, u32 type) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    if (spotInfo != NULL) {
        MI_CpuCopy8(spotInfo, main->apSpotInfo, sizeof(main->apSpotInfo));
    } else {
        MI_CpuFill8(main->apSpotInfo, 0, sizeof(main->apSpotInfo));
    }
    main->overrideType = type;
    DWC_AC_SetSpecifyAp(ssid, key, wepMode);
    work->userInfo[0].ap.mtu = unk_ea;
}

// Clears the first setting of the DS and fills it with the SSID (a string of 32 bytes at most) and, unless the mode is
// 0, a WEP key of the length that the mode says; makes the search look for that setting alone (mode 1)
static void DWC_AC_SetSpecifyAp(const u8 *ssid, const u8 *key, u32 wepMode) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    int i;

    work->mode = 1;
    MI_CpuFill8(work, 0, 0xf0);
    for (i = 0; i < 32; i++) {
        if (ssid[i] == 0) {
            break;
        }
        work->userInfo[0].ap.ssid[i] = ssid[i];
    }
    if (key == NULL || wepMode == 0) {
        work->userInfo[0].ap.wepMode = 0;
        return;
    }
    MI_CpuCopy8(key, work->userInfo[0].ap.wepKey[0], wepMode == 1 ? 5 : wepMode == 2 ? 13 : 16);
    work->userInfo[0].ap.wepMode = wepMode;
}

// Whether the SSID of length bytes is that of an NDWCSHAP
BOOL DWC_AC_CheckWiFiStation(const u8 *ssid, u32 length) {
    if (length != 32) {
        return FALSE;
    }
    return DWCi_AC_CheckNintendoShopAP(ssid);
}

// Replaces the connection of the DS's setting `index` with the 0xf0 bytes of setting
void DWCi_AC_InsertApInfo(u32 index, const void *setting) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    MIi_CpuCopy32(setting, &work->userInfo[index], 0xf0);
}

// Stops the search from preferring the access points with WPA
void DWCAc_ClearPreferWpa(void) {
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    work->preferWpa = 0;
}

// Takes a block of memory of the flag, if it is not taken yet; NULL if it is
static void *DWCi_AC_Alloc(u32 flag, u32 size) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);

    if (main->allocState & flag) {
        return NULL;
    }
    main->allocState |= flag;
    return main->alloc(1, size);
}

// Lets a block of memory go, if it is taken; the control block's pointer is cleared with its own block
static void DWCi_AC_Free(u32 flag, void *block, u32 size) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);

    if (main == NULL) {
        return;
    }
    if (!(main->allocState & flag)) {
        return;
    }
    main->allocState &= ~flag;
    main->free(1, block, size);
    if (flag == DWC_AC_WORK_MAIN) {
        sBlocks.main = NULL;
    }
}

// Lets go of every block that is taken, the control block last
static void DWCi_AC_FreeAll(void) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);

    if (main == NULL) {
        return;
    }
    if (main->allocState & DWC_AC_WORK_SEARCH) {
        void *block = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

        main->allocState &= ~DWC_AC_WORK_SEARCH;
        main->free(1, block, sizeof(ACWORK));
    }
    if (main->allocState & DWC_AC_WORK_MEMFUNCS) {
        void *block = DWCi_AC_GetMemPtr(DWC_AC_WORK_MEMFUNCS);

        main->allocState &= ~DWC_AC_WORK_MEMFUNCS;
        main->free(1, block, sizeof(DWCNetcheckParam));
    }
    if (main->allocState & DWC_AC_WORK_SOC_CONFIG) {
        void *block = DWCi_AC_GetMemPtr(DWC_AC_WORK_SOC_CONFIG);

        main->allocState &= ~DWC_AC_WORK_SOC_CONFIG;
        main->free(1, block, 0x58);
    }
    if (main->allocState & DWC_AC_WORK_WCM) {
        void *block = DWCi_AC_GetMemPtr(DWC_AC_WORK_WCM);

        main->allocState &= ~DWC_AC_WORK_WCM;
        main->free(1, block, 0x4800);
    }
    if (main->allocState & DWC_AC_WORK_MAIN) {
        main->allocState &= ~DWC_AC_WORK_MAIN;
        main->free(1, main, sizeof(ACHOLD));
        sBlocks.main = NULL;
    }
}

// The block of memory that a flag names: the first of its bits that is set decides. NULL while it is not taken
void *DWCi_AC_GetMemPtr(u32 flag) {
    if (flag & DWC_AC_WORK_MAIN) {
        return sBlocks.main;
    }
    if (flag & DWC_AC_WORK_WCM) {
        return sBlocks.wcm;
    }
    if (flag & DWC_AC_WORK_SOC_CONFIG) {
        return sBlocks.socConfig;
    }
    if (flag & DWC_AC_WORK_MEMFUNCS) {
        return sBlocks.memFuncs;
    }
    if (flag & DWC_AC_WORK_SEARCH) {
        return sBlocks.search;
    }
    return NULL;
}

// Sets the state, and keeps track of the highest one: when a state past 7 is reached for the first time, notes the type
// and the result of the candidate that the connection works on
static void DWCi_AC_SetPhase(u8 state) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    main->phase = state;
    if (state >= 16) {
        return;
    }
    if (state > main->endSequence) {
        main->endSequence = state;
        if (state > 7) {
            main->endType = DWCi_ConvConnectAPType(work->connectApType);
            main->endState = work->findList[work->connectNo].state;
        }
    }
}

// The state of the control block, 0 while there is none
u8 DWCi_AC_GetPhase(void) {
    return sBlocks.main != NULL ? sBlocks.main->phase : 0;
}

// Notes an error: the code, and the state it came in, from which the teardown goes on
void DWCi_AC_SetError(s32 error) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);

    main->error = error;
    main->phaseError = DWCi_AC_GetPhase();
}

// The error that was noted last
s32 DWCi_AC_GetError(void) {
    return sBlocks.main->error;
}

// Notes the type of the access point that the connection is with: the one that DWC_AC_SetSpecifyApEx was given, else
// that of the target id; and its spot information, which an NDWCSHAP's SSID holds (zeroes for any other)
void DWCi_AC_SetApType(u8 id) {
    u8 type = sBlocks.main->overrideType;
    u8 *ssid;

    if (type == 0) {
        type = DWCi_ConvConnectAPType(id);
    }
    sBlocks.main->type = type;
    ssid = WCM_GetApEssid(NULL);
    if (ssid != NULL) {
        DC_InvalidateRange(ssid, 32);
        if (DWCi_AC_GetPostalCode(ssid, sBlocks.main->apSpotInfo)) {
            return;
        }
        MI_CpuFill8(sBlocks.main->apSpotInfo, 0, sizeof(sBlocks.main->apSpotInfo));
    } else {
        MI_CpuFill8(sBlocks.main->apSpotInfo, 0, sizeof(sBlocks.main->apSpotInfo));
    }
}

// Notes the key mode of the setting that the connection to the target id uses: the WCM mode of its WPA if it is one of
// the DSi's three and uses WPA, else its WEP mode; the two kinds told by their SSIDs (NWCUSBAP, NDWCSHAP) have mode 2.
// For the DSi's three it also copies some of the setting's details to the control block
void DWCAc_NoteKeyMode(u8 id) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    u8 type = DWCi_ConvConnectAPType(id);

    if (type >= 10 && type <= 12 && OS_IsRunOnTwl()) {
        type -= 10;
        main->unk_24 = work->dsiSettings[type].unk_182;
        main->unk_25 = work->dsiSettings[type].unk_183;
        main->unk_26 = work->dsiSettings[type].unk_1e8;
        MI_CpuCopy8(work->dsiSettings[type].unk_184, main->unk_28, sizeof(main->unk_28));
        MI_CpuCopy8(work->dsiSettings[type].base.ap.unk0, main->unk_8c, sizeof(main->unk_8c));
        MI_CpuCopy8(work->dsiSettings[type].base.ap.unk20, main->unk_ac, sizeof(main->unk_ac));
        if (work->dsiSettings[type].base.ap.setType == 0x10 || work->dsiSettings[type].base.ap.setType == 0x11 ||
            work->dsiSettings[type].base.ap.setType == 0x12 || work->dsiSettings[type].base.ap.setType == 0x13) {
            main->keyMode = work->dsiSettings[type].wcmMode;
        } else {
            main->keyMode = work->dsiSettings[type].base.ap.wepMode;
        }
    } else if (id == DWC_AC_ID_NWCUSBAP || id == DWC_AC_ID_NDWCSHAP) {
        main->keyMode = 2;
    } else {
        main->keyMode = work->userInfo[type].ap.wepMode;
    }
}

// The type of the access point of a target id: 0 to 2 for the DS's three settings, which ids 0 to 5 are, 3 to 5 for
// NWCUSBAP, NDWCSHAP and FREESPOT (ids 0x12 to 0x14), and 10 to 12 for the DSi's three, ids 6 to 17
u8 DWCi_ConvConnectAPType(u8 id) {
    if (id >= 0x12) {
        return id - 0xf;
    }
    if (id >= 0xf) {
        return id - 5;
    }
    if (id >= 0xc) {
        return id - 2;
    }
    if (id >= 9) {
        return id + 1;
    }
    if (id >= 6) {
        return id + 4;
    }
    if (id >= 3) {
        return id - 3;
    }
    return id;
}

// Lets go of the two blocks that only the search and the connection used
static void Free_Disused(void) {
    DWCi_AC_Free(DWC_AC_WORK_MEMFUNCS, sBlocks.memFuncs, sizeof(DWCNetcheckParam));
    DWCi_AC_Free(DWC_AC_WORK_SEARCH, sBlocks.search, sizeof(ACWORK));
}

// 2 if another ordinary candidate (id below 0x12) looks like the one that was connected to, else 1; a special target
// (id 0x12 on) always gives 1. The candidates are compared as far as the other one's SSID is long, starting at the
// BSSID
//
// style-ok: bug no BUGFIX, since the callers don't show which comparison was meant
// BUG: strncmp compares the BSSIDs for the length of the SSID, and stops at a zero byte of a BSSID, so the access
// points whose OUIs start with 0 come out equal. Either the SSIDs were meant (another access point with the same name)
// or the six bytes of the BSSIDs (memcmp); the callers only test the result for 0, so the code doesn't say which
static u8 CheckDuplicate(void) {
    u8 i;
    int selected;
    ACWORK *work = sBlocks.search;
    u8 count;
    u8 kind;
    const char *name;

    selected = work->connectNo;
    kind = work->connectApType;
    name = (const char *)work->bssDesc[selected].bssid;
    if (kind >= 0x12) {
        return 1;
    }
    count = work->candidateCount;
    for (i = 0; i < count; i++) {
        if (i != selected && work->findList[i].type < 0x12) {
            if (strncmp(name, (const char *)work->bssDesc[i].bssid, work->bssDesc[i].ssidLength) == 0) {
                return 2;
            }
        }
    }
    return 1;
}

// On a DSi, whether the license agreement has been agreed to, in the version that the ROM asks for; always TRUE on a
// DS, or when the work says to ignore it
static BOOL DWCAc_CheckEula(void) {
    if (OS_IsRunOnTwl()) {
        if (sBlocks.search->ignoreEula != 0 || (OS_IsAgreeEULA() && OS_GetAgreedEULAVersion() != 0 &&
                                                OS_GetAgreedEULAVersion() >= OS_GetROMHeaderEULAVersion())) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

// On a DSi, whether the system settings allow the wireless connection and the parental controls do not forbid it;
// always TRUE on a DS
static BOOL DWCAc_CheckWirelessAllowed(void) {
    if (OS_IsRunOnTwl()) {
        if (OS_IsAvailableWireless() == TRUE && OS_IsForceDisableWireless() == 0) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}
