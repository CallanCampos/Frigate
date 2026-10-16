// DWC's auto-connect, its core (TWL-DWC 5.3, ac). The file name is descriptive: the file creates and destroys the
// control block and the other blocks of memory that auto-connect keeps, and steps the state machine (func_0205a1f0)
// whose states the other ac files run: 1 starts the wireless manager, 2 to 6 the search, 7 and 8 the connection to the
// candidate, 9 its end, 10 to 15 the sockets coming up and the connection kept, 16 the connection up, 17 the teardown
// and 18 the failure. The next function, at 0205ac24, is the first of dwc_connectinet.c.

#include "dwc/ac.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The pointers to the five blocks of memory, which func_0205a78c gives out by their flags
typedef struct DWCAcBlocks {
    void *wcm;               // DWC_AC_WORK_WCM
    void *unk_04;            // DWC_AC_WORK_UNK_04
    DWCAcMemFuncs *memFuncs; // DWC_AC_WORK_UNK_08
    DWCAcMain *main;         // DWC_AC_WORK_MAIN
    DWCAcSearchWork *search; // DWC_AC_WORK_SEARCH
} DWCAcBlocks;

static DWCAcBlocks sBlocks;

static void func_0205a494(const u8 *ssid, const u8 *key, u32 wepMode);
static void *func_0205a5b0(u32 flag, u32 size);
static void func_0205a5f8(u32 flag, void *block, u32 size);
static void func_0205a664(void);
static void func_0205a7d4(u8 state);
static void func_0205aaa0(void);
static u8 func_0205aad4(void);
static BOOL func_0205ab78(void);
static BOOL func_0205abe8(void);

// Whether a DSi setting's security byte says it uses WPA: any byte from 0x10 on that is not 0xff, which a setting not
// in use has
#define SETTING_USES_WPA(setting) ((setting).base.security >= 0x10 && (setting).base.security != 0xff)

// Creates auto-connect: the control block and the four more blocks of memory, the settings that it works from (the DS's
// and, on a DSi, the DSi's), and the wireless manager. FALSE if the wireless manager does not start
BOOL func_02059f04(DWCAcParam *param) {
    DWCAcMain *main;
    DWCAcMemFuncs *funcs;
    DWCAcSearchWork *work;
    s32 result;

    sBlocks.main = param->alloc(1, sizeof(DWCAcMain));
    MI_CpuClear32(sBlocks.main, sizeof(DWCAcMain));
    main = sBlocks.main;
    main->alloc = param->alloc;
    main->free = param->free;
    main->state = 1;
    main->maxState = 1;
    main->allocated = DWC_AC_WORK_MAIN;
    sBlocks.search = func_0205a5b0(DWC_AC_WORK_SEARCH, sizeof(DWCAcSearchWork));
    sBlocks.wcm = func_0205a5b0(DWC_AC_WORK_WCM, 0x4800);
    sBlocks.unk_04 = func_0205a5b0(DWC_AC_WORK_UNK_04, 0x58);
    sBlocks.memFuncs = func_0205a5b0(DWC_AC_WORK_UNK_08, sizeof(DWCAcMemFuncs));
    MI_CpuClear32(sBlocks.search, sizeof(DWCAcSearchWork));
    MI_CpuClear32(sBlocks.wcm, 0x4800);
    MI_CpuClear32(sBlocks.unk_04, 0x58);
    MI_CpuClear32(sBlocks.memFuncs, sizeof(DWCAcMemFuncs));
    work = sBlocks.search;
    work->dmaNo = param->dmaNo;
    work->radioOn = param->radioOn;
    work->unk_14c8 = param->unk_0c;
    work->ignoreEula = param->ignoreEula;
    funcs = sBlocks.memFuncs;
    funcs->alloc = param->alloc;
    funcs->free = param->free;
    funcs->unk_08 = NULL;
    work->mode = param->mode;
    work->unk_14bc_4 = param->unk_0b_0;
    work->rssiPacked = param->rssiPacked;
    func_02056a58(work);
    if (hw_isDSi()) {
        work->preferWpa = 0;
        func_02056a6c(work->dsiSettings);
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
            if (work->dsiSettings[2].base.security >= 0x10 && work->dsiSettings[1].base.security != 0xff) {
#endif
                work->preferWpa = 1;
            }
        }
    }
    if (hw_isDSi()) {
        if (work->rssiPacked == 1) {
            result = WCM_Init(sBlocks.wcm, 0x4800);
        } else {
            result = WCM_InitCore(sBlocks.wcm, 0x4800, 2);
        }
    } else {
        result = WCM_Init(sBlocks.wcm, 0x4800);
    }
    if (result == 1 || result > 4) {
        func_0205a664();
        return FALSE;
    }
    return TRUE;
}

// Runs the state at hand once and moves to the one that follows. Returns 1 once the connection is up (state 16, or 2 if
// func_0205aad4 finds another candidate like the one connected to), -1 when it has failed for good (state 18), else 0.
// Once it is up or has failed, the search's blocks of memory are let go
s32 func_0205a1f0(void) {
    u8 state = func_0205a83c();
    s32 result;

    if (state == 1) {
        if (func_0205ab78()) {
            if (func_0205abe8()) {
                state = func_0205d568();
            } else {
                func_0205a85c(8);
                state = func_0205b65c();
            }
        } else {
            func_0205a85c(7);
            state = func_0205b65c();
        }
    } else if (state < 7) {
        u32 irq = CPU_IRQDisable();

        state = func_0205c37c();
        func_0205a7d4(state);
        CPU_SetIRQMask(irq);
    } else if (state < 9) {
        state = func_020598b4();
    } else if (state < 10) {
        state = func_0205c2a4();
    } else if (state < 16) {
        state = func_0205d5f4();
    } else if (state == 17) {
        state = func_0205b65c();
    }
    func_0205a7d4(state);
    if (state == 16) {
        result = func_0205aad4();
        func_0205aaa0();
        return result;
    }
    if (state != 18) {
        return 0;
    }
    func_0205aaa0();
    return -1;
}

// What the state is, in a few values for the caller: 0 before the search, 1 searching (states 2 to 6), 2 connecting (7
// and 8), 3 the sockets coming up (10, 12 to 15), 4 for 9, 11 and 17, which go on to another candidate or to the
// teardown, 5 connected (16); for a failure (18), the error code of dwc_ac_result.c's func_0205b680
s32 func_0205a2dc(void) {
    u8 state = func_0205a83c();

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
    return func_0205b680();
}

// The type of the access point that the connection is with, 0xff while there is none
u8 func_0205a348(void) {
    u8 type = 0xff;
    u8 state = func_0205a83c();

    if (state >= 10 && state <= 16) {
        type = sBlocks.main->type;
    }
    return type;
}

// Copies the 10 bytes of spot information to spotInfo, if the connection is with an access point that has any (the type
// 4 or 8). TRUE if it did
BOOL func_0205a378(u8 *spotInfo) {
    BOOL found = FALSE;
    u8 state = func_0205a83c();

    if (state >= 10 && state <= 16) {
        DWCAcMain *main = sBlocks.main;

        if (main->type == 4 || main->type == 8) {
            sys_memcpy(main->spotInfo, spotInfo, sizeof(main->spotInfo));
            found = TRUE;
        }
    }
    return found;
}

// The key mode of the setting that the connection is made with
u8 func_0205a3d0(void) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);

    return main->keyMode;
}

// Takes auto-connect down. Once it has no state or is over (18), lets its memory go and returns TRUE; else steps the
// teardown that the state at hand needs
BOOL func_0205a3e4(void) {
    u8 state = func_0205a83c();

    if (state == 0 || state == 18) {
        func_0205a664();
        return TRUE;
    }
    func_0205975c(&state);
    func_0205a7d4(state);
    return FALSE;
}

// Gives the connection to make by hand: the first setting of the DS is made from the SSID, the WEP key and its mode and
// the fourth argument, and the search is made to try that and nothing else. The spot information, if any, and the type
// that the access point is given are what the connection reports
void func_0205a420(const u8 *ssid, const u8 *key, u32 wepMode, u16 unk_ea, const u8 *spotInfo, u32 type) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);

    if (spotInfo != NULL) {
        sys_memcpy(spotInfo, main->spotInfo, sizeof(main->spotInfo));
    } else {
        sys_memset(main->spotInfo, 0, sizeof(main->spotInfo));
    }
    main->givenType = type;
    func_0205a494(ssid, key, wepMode);
    work->dsSettings[0].unk_ea = unk_ea;
}

// Clears the first setting of the DS and fills it with the SSID (a string of 32 bytes at most) and, unless the mode is
// 0, a WEP key of the length that the mode says; makes the search look for that setting alone (mode 1)
static void func_0205a494(const u8 *ssid, const u8 *key, u32 wepMode) {
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);
    int i;

    work->mode = 1;
    sys_memset(work, 0, 0xf0);
    for (i = 0; i < 32; i++) {
        if (ssid[i] == 0) {
            break;
        }
        work->dsSettings[0].ssid[i] = ssid[i];
    }
    if (key == NULL || wepMode == 0) {
        work->dsSettings[0].wepMode = 0;
        return;
    }
    sys_memcpy(key, work->dsSettings[0].wepKey, wepMode == 1 ? 5 : wepMode == 2 ? 13 : 16);
    work->dsSettings[0].wepMode = wepMode;
}

// Whether the SSID of length bytes is that of an NDWCSHAP
BOOL func_0205a54c(const u8 *ssid, u32 length) {
    if (length != 32) {
        return FALSE;
    }
    return func_0205cf80(ssid);
}

// Replaces the connection of the DS's setting `index` with the 0xf0 bytes of setting
void func_0205a564(u32 index, const void *setting) {
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);

    sys_memcpy32(setting, &work->dsSettings[index], 0xf0);
}

// Stops the search from preferring the access points with WPA
void func_0205a590(void) {
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);

    work->preferWpa = 0;
}

// Takes a block of memory of the flag, if it is not taken yet; NULL if it is
static void *func_0205a5b0(u32 flag, u32 size) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);

    if (main->allocated & flag) {
        return NULL;
    }
    main->allocated |= flag;
    return main->alloc(1, size);
}

// Lets a block of memory go, if it is taken; the control block's pointer is cleared with its own block
static void func_0205a5f8(u32 flag, void *block, u32 size) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);

    if (main == NULL) {
        return;
    }
    if (!(main->allocated & flag)) {
        return;
    }
    main->allocated &= ~flag;
    main->free(1, block, size);
    if (flag == DWC_AC_WORK_MAIN) {
        sBlocks.main = NULL;
    }
}

// Lets go of every block that is taken, the control block last
static void func_0205a664(void) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);

    if (main == NULL) {
        return;
    }
    if (main->allocated & DWC_AC_WORK_SEARCH) {
        void *block = func_0205a78c(DWC_AC_WORK_SEARCH);

        main->allocated &= ~DWC_AC_WORK_SEARCH;
        main->free(1, block, sizeof(DWCAcSearchWork));
    }
    if (main->allocated & DWC_AC_WORK_UNK_08) {
        void *block = func_0205a78c(DWC_AC_WORK_UNK_08);

        main->allocated &= ~DWC_AC_WORK_UNK_08;
        main->free(1, block, sizeof(DWCAcMemFuncs));
    }
    if (main->allocated & DWC_AC_WORK_UNK_04) {
        void *block = func_0205a78c(DWC_AC_WORK_UNK_04);

        main->allocated &= ~DWC_AC_WORK_UNK_04;
        main->free(1, block, 0x58);
    }
    if (main->allocated & DWC_AC_WORK_WCM) {
        void *block = func_0205a78c(DWC_AC_WORK_WCM);

        main->allocated &= ~DWC_AC_WORK_WCM;
        main->free(1, block, 0x4800);
    }
    if (main->allocated & DWC_AC_WORK_MAIN) {
        main->allocated &= ~DWC_AC_WORK_MAIN;
        main->free(1, main, sizeof(DWCAcMain));
        sBlocks.main = NULL;
    }
}

// The block of memory that a flag names: the first of its bits that is set decides. NULL while it is not taken
void *func_0205a78c(u32 flag) {
    if (flag & DWC_AC_WORK_MAIN) {
        return sBlocks.main;
    }
    if (flag & DWC_AC_WORK_WCM) {
        return sBlocks.wcm;
    }
    if (flag & DWC_AC_WORK_UNK_04) {
        return sBlocks.unk_04;
    }
    if (flag & DWC_AC_WORK_UNK_08) {
        return sBlocks.memFuncs;
    }
    if (flag & DWC_AC_WORK_SEARCH) {
        return sBlocks.search;
    }
    return NULL;
}

// Sets the state, and keeps track of the highest one: when a state past 7 is reached for the first time, notes the type
// and the result of the candidate that the connection works on
static void func_0205a7d4(u8 state) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);

    main->state = state;
    if (state >= 16) {
        return;
    }
    if (state > main->maxState) {
        main->maxState = state;
        if (state > 7) {
            main->candidateType = func_0205aa40(work->kind);
            main->candidateResult = work->infos[work->selected].unk_00;
        }
    }
}

// The state of the control block, 0 while there is none
u8 func_0205a83c(void) {
    return sBlocks.main != NULL ? sBlocks.main->state : 0;
}

// Notes an error: the code, and the state it came in, from which the teardown goes on
void func_0205a85c(s32 error) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);

    main->error = error;
    main->failedState = func_0205a83c();
}

// The error that was noted last
s32 func_0205a880(void) {
    return sBlocks.main->error;
}

// Notes the type of the access point that the connection is with: the one that func_0205a420 was given, else that of
// the target id; and its spot information, which an NDWCSHAP's SSID holds (zeroes for any other)
void func_0205a894(u8 id) {
    u8 type = sBlocks.main->givenType;
    const u8 *ssid;

    if (type == 0) {
        type = func_0205aa40(id);
    }
    sBlocks.main->type = type;
    ssid = WCM_GetConnectedApSsid(NULL);
    if (ssid != NULL) {
        cp15_invalidateDC((void *)ssid, 32);
        if (func_0205cfe8(ssid, sBlocks.main->spotInfo)) {
            return;
        }
        sys_memset(sBlocks.main->spotInfo, 0, sizeof(sBlocks.main->spotInfo));
    } else {
        sys_memset(sBlocks.main->spotInfo, 0, sizeof(sBlocks.main->spotInfo));
    }
}

// Notes the key mode of the setting that the connection to the target id uses: the WCM mode of its WPA if it is one of
// the DSi's three and uses WPA, else its WEP mode; the two kinds told by their SSIDs (NWCUSBAP, NDWCSHAP) have mode 2.
// For the DSi's three it also copies some of the setting's details to the control block
void func_0205a928(u8 id) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);
    DWCAcSearchWork *work = func_0205a78c(DWC_AC_WORK_SEARCH);
    u8 type = func_0205aa40(id);

    if (type >= 10 && type <= 12 && hw_isDSi()) {
        type -= 10;
        main->unk_24 = work->dsiSettings[type].unk_182;
        main->unk_25 = work->dsiSettings[type].unk_183;
        main->unk_26 = work->dsiSettings[type].unk_1e8;
        sys_memcpy(work->dsiSettings[type].unk_184, main->unk_28, sizeof(main->unk_28));
        sys_memcpy(work->dsiSettings[type].base.unk_00, main->unk_8c, sizeof(main->unk_8c));
        sys_memcpy(work->dsiSettings[type].base.unk_20, main->unk_ac, sizeof(main->unk_ac));
        if (work->dsiSettings[type].base.security == 0x10 || work->dsiSettings[type].base.security == 0x11 ||
            work->dsiSettings[type].base.security == 0x12 || work->dsiSettings[type].base.security == 0x13) {
            main->keyMode = work->dsiSettings[type].wcmMode;
        } else {
            main->keyMode = work->dsiSettings[type].base.wepMode;
        }
    } else if (id == DWC_AC_ID_NWCUSBAP || id == DWC_AC_ID_NDWCSHAP) {
        main->keyMode = 2;
    } else {
        main->keyMode = work->dsSettings[type].wepMode;
    }
}

// The type of the access point of a target id: 0 to 2 for the DS's three settings, which ids 0 to 5 are, 3 to 5 for
// NWCUSBAP, NDWCSHAP and FREESPOT (ids 0x12 to 0x14), and 10 to 12 for the DSi's three, ids 6 to 17
u8 func_0205aa40(u8 id) {
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
static void func_0205aaa0(void) {
    func_0205a5f8(DWC_AC_WORK_UNK_08, sBlocks.memFuncs, sizeof(DWCAcMemFuncs));
    func_0205a5f8(DWC_AC_WORK_SEARCH, sBlocks.search, sizeof(DWCAcSearchWork));
}

// 2 if another ordinary candidate (id below 0x12) looks like the one that was connected to, else 1; a special target
// (id 0x12 on) always gives 1. The candidates are compared as far as the other one's SSID is long, starting at the
// BSSID
//
// BUG: the comparison starts at the BSSID, whose six bytes and the SSID's length come first, where the SSIDs were
// probably meant, to tell that another access point has the same name
static u8 func_0205aad4(void) {
    u8 i;
    u8 count;
    DWCAcSearchWork *work = sBlocks.search;
    s16 selected = work->selected;
    u8 kind = work->kind;
#ifdef BUGFIX
    const char *name = (const char *)work->candidates[selected].ssid;
#else
    const char *name = (const char *)work->candidates[selected].bssid;
#endif

    if (kind >= 0x12) {
        return 1;
    }
    count = work->candidateCount;
    for (i = 0; i < count; i++) {
        if (i != selected && work->infos[i].id < 0x12) {
#ifdef BUGFIX
            if (strncmp(name, (const char *)work->candidates[i].ssid, work->candidates[i].ssidLength) == 0) {
#else
            if (strncmp(name, (const char *)work->candidates[i].bssid, work->candidates[i].ssidLength) == 0) {
#endif
                return 2;
            }
        }
    }
    return 1;
}

// On a DSi, whether the license agreement has been agreed to, in the version that the ROM asks for; always TRUE on a
// DS, or when the work says to ignore it
static BOOL func_0205ab78(void) {
    if (hw_isDSi()) {
        if (sBlocks.search->ignoreEula != 0 ||
            (func_0207c45c() && func_0207c480() != 0 && func_0207c480() >= func_0207c49c())) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

// On a DSi, whether the system settings allow the wireless connection and the parental controls do not forbid it;
// always TRUE on a DS
static BOOL func_0205abe8(void) {
    if (hw_isDSi()) {
        if (func_0207c438() == TRUE && func_0207c4d0() == 0) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}
