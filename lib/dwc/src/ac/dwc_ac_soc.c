// DWC's auto-connect, the sockets (TWL-DWC 5.3, ac). The file name is a guess: its functions run the states that
// follow the search, which dwc_ac.c dispatches. State 1 starts the wireless manager, and the states 10 to 15 bring the
// sockets up over the connection to the access point (SOC_Startup, the address from DHCP, the DNS servers, a test of
// the connection) or down again. The configuration that SOC_Startup gets is made from the setting that the connection
// is with, and from the memory functions of auto-connect.
//
// The DWCAc_ names are ours, taken from what the code does.

#include "dwc/ac.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"

static u8 DWCAc_StartSoc(DWCAcSearchWork *work);
static u8 DWCAc_WaitHostId(DWCAcSearchWork *work);
static u8 DWCAc_StartConnectionTest(void);
static u8 DWCAc_WaitConnectionTest(DWCAcSearchWork *work);
static u8 DWCAc_NoteConnected(DWCAcSearchWork *work);
static u8 DWCAc_StopSoc(void);
static void DWCAc_MakeSocConfig(DWCAcMain *main, DWCAcSearchWork *work, SocConfig *config);
static u32 DWCAc_ReadAddress(const u8 *address);
static u32 DWCAc_MakeNetmask(u8 bits);
static void DWCAc_SetResolver(DWCAcSearchWork *work);

// The name that the console gives DHCP (the word before the 4, in the unknown part of the configuration, points to it)
static char sSocHostName[] = "NINTENDO-DS";

// What DWCAc_MakeSocConfig starts the configuration of SOC_Startup from: DHCP on, the addresses, the allocators, the
// MTU and the window left to the function, then the two words of 0x1000 and the host name. The flag and these words are
// not known
static const SocConfig sSocConfigInit = {
    0x01000000, NULL, NULL, TRUE, 0, 0, 0, 0, 0, 0x1000, 0x1000, 0, 0, { 0, 0, 0, 0, 0, (u32)sSocHostName, 4, 0, 0 },
};

// State 1, starts the wireless manager with the notification callback of dwc_ac_wcm.c, after
// listing the targets of the first search. Returns the state that follows: 2, the search; 0x11, the teardown, if the
// wireless manager will not start; and 1, to wait, while it is not in its first state
u8 DWCAc_StepWcmStartup(void) {
    WCMConfig config;
    s32 wcmState = WCM_GetState();
    DWCAcSearchWork *work = DWCAc_GetBlock(DWC_AC_WORK_SEARCH);
    s32 result;

    if (wcmState == 1) {
        config.dmano = work->dmaNo;
        config.pbdbuffer = NULL;
        config.nbdbuffer = 0;
        config.nbdmode = 0;
        DWCAc_MakeSearchList(0);
        result = WCM_StartupAsync(&config, DWCAc_OnWcmNotify);
        if (result == 1 || result >= 4) {
            DWCAc_SetError(1);
            return 0x11;
        }
    } else {
        return 1;
    }
    return 2;
}

// The states 10 to 15. While the wireless manager holds the connection (its state 9), each does its
// part: 10 starts SOC, 11 takes it down, 12 waits for the address, 13 starts the connection test, 14 waits for it and
// 15 notes the connection. Once the connection is gone, 11 and 15 still go on as they are, 14 lets the test go first,
// and every state marks the candidate as lost and goes to 11, to take SOC down. Returns the state that follows
u8 DWCAc_StepSockets(void) {
    // The cases of both switches are in the order that the ROM lays out their bodies in
    u8 state = DWCAc_GetState();
    DWCAcSearchWork *work = DWCAc_GetBlock(DWC_AC_WORK_SEARCH);

    if (WCM_GetState() == 9) {
        switch (state) {
        case 10:
            state = DWCAc_StartSoc(work);
            break;
        case 12:
            state = DWCAc_WaitHostId(work);
            break;
        case 13:
            state = DWCAc_StartConnectionTest();
            break;
        case 14:
            state = DWCAc_WaitConnectionTest(work);
            break;
        case 15:
            state = DWCAc_NoteConnected(work);
            break;
        case 11:
            state = DWCAc_StopSoc();
            break;
        }
    } else {
        switch (state) {
        case 15:
            state = DWCAc_NoteConnected(work);
            break;
        case 11:
            state = DWCAc_StopSoc();
            break;
        case 14:
            func_ov011_0216b24c();
            func_ov011_0216b1d4();
            // fall through to the loss of the candidate
        default:
            work->infos[work->selected].result = DWC_AC_RESULT_LOST;
            state = 11;
            break;
        }
    }
    return state;
}

// State 10, starts SOC from the configuration that DWCAc_MakeSocConfig makes, with the wait of SOCL's
// yields at 4. 12 to wait for the address, or 0x11, the teardown, if SOC will not start
static u8 DWCAc_StartSoc(DWCAcSearchWork *work) {
    DWCAcMain *main = DWCAc_GetBlock(DWC_AC_WORK_MAIN);
    SocConfig *config = DWCAc_GetBlock(DWC_AC_WORK_SOC_CONFIG);

    DWCAc_MakeSocConfig(main, work, config);
    SOCLiYieldWait = 4;
    if (SOC_Startup(config) == 0) {
        return 12;
    }
    DWCAc_SetError(2);
    return 0x11;
}

// State 12, waits for the address. When it has come, gives SOC the DNS servers of the setting, and
// goes on to the connection test (13), or skips it (15) if the game asked for that or the setting on a DSi uses a
// proxy. If the address has not come in 10 seconds the candidate is given up, and SOC goes down (11)
static u8 DWCAc_WaitHostId(DWCAcSearchWork *work) {
    u8 type;

    if (SOC_GetHostID() != 0) {
        DWCAc_SetResolver(work);
        if (work->skipConnectionTest == 1) {
            return 15;
        }
        type = DWCAc_GetApTypeOfId(work->kind);
        if (type >= 10 && type <= 12) {
            if (OS_IsRunOnTwl() && work->dsiSettings[(u8)(type - 10)].unk_182 != 0) {
                return 15;
            }
        }
        return 13;
    }
    if (OS_TicksToSeconds(OS_GetTick() - work->connectedTime) < 10) {
        return 12;
    }
    work->infos[work->selected].result = DWC_AC_RESULT_GAVE_UP;
    return 11;
}

// State 13, starts the connection test of overlay 11 with the memory functions. 14 to wait
// for it, or 0x11, the teardown, if it will not start
static u8 DWCAc_StartConnectionTest(void) {
    if (func_ov011_0216b14c(DWCAc_GetBlock(DWC_AC_WORK_MEMFUNCS)) == 0) {
        return 14;
    }
    DWCAc_SetError(3);
    return 0x11;
}

// State 14, waits for the connection test. Once it is over, takes its result for the error
// if the candidate is of the type that the game was told of, and lets the test go. 15 if it went well; else the
// candidate is given up, with a note if CPS found its address already in use, and SOC goes down (11)
static u8 DWCAc_WaitConnectionTest(DWCAcSearchWork *work) {
    DWCAcMain *main = DWCAc_GetBlock(DWC_AC_WORK_MAIN);
    u32 status = func_ov011_0216b2a4();

    if (status != 0) {
        if (main->candidateType == DWCAc_GetApTypeOfId(work->kind)) {
            main->testResult = func_ov011_0216b2cc();
        }
        func_ov011_0216b1d4();
        if (status != 11) {
            work->infos[work->selected].result = DWC_AC_RESULT_GAVE_UP;
            if (CPSNoIpReason == CPS_NOIP_REASON_COLLISION) {
                main->addressInUse = 1;
            } else {
                main->addressInUse = 0;
            }
            return 11;
        } else {
            return 15;
        }
    } else {
        return 14;
    }
}

// State 15, notes the type of the access point and the key mode of the setting that the
// connection is with. 16, the connection is up
static u8 DWCAc_NoteConnected(DWCAcSearchWork *work) {
    DWCAc_NoteConnectedAp(work->kind);
    DWCAc_NoteKeyMode(work->kind);
    return 16;
}

// State 11, takes the sockets down: SOCL first, then SOC. 9, the end of the candidate, once they are
// down, else 11
static u8 DWCAc_StopSoc(void) {
    int result;

    if (SOCL_CalmDown() != 0) {
        return 11;
    }
    result = SOC_Cleanup();
    if (result == 0 || result == SOCL_ENETRESET) {
        return 9;
    }
    return 11;
}

// Makes the configuration for SOC_Startup from the setting that the connection is with: the
// allocators of auto-connect; the address, the mask, the gateway and the DNS servers if the setting has an address of
// its own, else DHCP; the MTU, which a DSi takes from the setting within 576 to 1500 (1400 if it has none); and on a
// DSi the receive window of the search's work, unless the MTU is the smallest. The allocators have the type of DWC's
// (size unsigned) where SOC wants a size with a sign
static void DWCAc_MakeSocConfig(DWCAcMain *main, DWCAcSearchWork *work, SocConfig *config) {
    DWCAcDsSetting *setting;
    u8 type;

    MI_CpuCopy8(&sSocConfigInit, config, sizeof(SocConfig));
    config->alloc = (SocAllocFunc)main->alloc;
    config->free = (SocFreeFunc)main->free;
    if (work->kind >= 0x12) {
        return;
    }
    type = DWCAc_GetApTypeOfId(work->kind);
    if (type <= 2) {
        setting = &work->dsSettings[type];
    } else {
        setting = &work->dsiSettings[type - 10].base;
    }
    if (setting->ip[0] != 0) {
        config->useDhcp = FALSE;
        config->addr = DWCAc_ReadAddress(setting->ip);
        config->netmask = DWCAc_MakeNetmask(setting->netmaskBits);
        config->gateway = DWCAc_ReadAddress(setting->gateway);
        config->dns1 = DWCAc_ReadAddress(setting->dns[0]);
        config->dns2 = DWCAc_ReadAddress(setting->dns[1]);
    } else {
        config->useDhcp = TRUE;
        config->addr = 0;
        config->netmask = 0;
        config->gateway = 0;
        config->dns1 = 0;
        config->dns2 = 0;
    }
    if (OS_IsRunOnTwl()) {
        if (setting->unk_ea == 0) {
            config->mtu = 0x578;
        } else if (setting->unk_ea > 0x5dc) {
            config->mtu = 0x5dc;
        } else if (setting->unk_ea < 0x240) {
            config->mtu = 0x240;
        } else {
            config->mtu = setting->unk_ea;
        }
    } else {
        config->mtu = 0x240;
    }
    if (OS_IsRunOnTwl()) {
        if (setting->unk_ea == 0x240) {
            config->rwin = 0;
        } else {
            config->rwin = work->rwin;
        }
    } else {
        config->rwin = 0;
    }
}

// An IPv4 address of four bytes in a setting (the first byte is the highest) as SOC_Startup wants
// it, with its bytes in the other order
static u32 DWCAc_ReadAddress(const u8 *address) {
    u32 value = 0;

    value |= address[0] << 24;
    value |= address[1] << 16;
    value |= address[2] << 8;
    value |= address[3];
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

// The netmask with that many of its high bits set, as SOC_Startup wants it, with its bytes in the
// other order
static u32 DWCAc_MakeNetmask(u8 bits) {
    u32 mask = 0xffffffff;
    int i;

    for (i = 0; i < 32 - bits; i++) {
        mask <<= 1;
    }
    return ((mask >> 24) & 0xff) | ((mask >> 8) & 0xff00) | ((mask << 8) & 0xff0000) | ((mask << 24) & 0xff000000);
}

// Gives SOC the DNS servers of the setting that the connection is with, when the address comes from
// DHCP but the setting has DNS servers of its own (those of a setting with an address of its own are in the
// configuration already)
static void DWCAc_SetResolver(DWCAcSearchWork *work) {
    DWCAcDsSetting *setting;
    u32 dns1;
    u32 dns2;
    int dnsSum;
    u8 type;

    if (work->kind >= 0x12) {
        return;
    }
    type = DWCAc_GetApTypeOfId(work->kind);
    if (type <= 2) {
        setting = &work->dsSettings[type];
    } else {
        setting = &work->dsiSettings[type - 10].base;
    }
    dnsSum = setting->dns[0][0] + setting->dns[0][1] + setting->dns[0][2] + setting->dns[0][3];
    if (setting->ip[0] != 0 || dnsSum == 0) {
        return;
    }
    dns1 = DWCAc_ReadAddress(setting->dns[0]);
    dns2 = DWCAc_ReadAddress(setting->dns[1]);
    SOC_GetResolver(&dns1, &dns2);
}
