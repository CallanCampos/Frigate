// DWC's auto-connect, the sockets (TWL-DWC 5.3, ac). The file name is a guess: its functions run the states that
// follow the search, which dwc_ac.c dispatches. State 1 starts the wireless manager, and the states 10 to 15 bring the
// sockets up over the connection to the access point (SOC_Startup, the address from DHCP, the DNS servers, a test of
// the connection) or down again. The configuration that SOC_Startup gets is made from the setting that the connection
// is with, and from the memory functions of auto-connect.
//
// The names are NitroDWC's own, as its libraries' symbols and ntrtwl's reconstruction of NitroDWC have them,
// where the code shows them; the DWCAc_ ones are ours, taken from what the code does.

#include "dwc/ac.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/wcm.h"

static u8 ConnectTestStart(ACWORK *work);
static u8 GetIPAddress(ACWORK *work);
static u8 ConnectTestCreate(void);
static u8 ConnectTestProcess(ACWORK *work);
static u8 ConnectTestEnd(ACWORK *work);
static u8 ConnectTestRetry(void);
static void MakeSOCConfig(ACHOLD *main, ACWORK *work, SOCConfig *config);
static u32 ConvAddress(const u8 *address);
static u32 ConvNetMask(u8 bits);
static void CheckSetDNS(ACWORK *work);

// The name that the console gives DHCP (the word before the 4, in the unknown part of the configuration, points to it)
static char sSocHostName[] = "NINTENDO-DS";

// What MakeSOCConfig starts the configuration of SOC_Startup from: DHCP on, the addresses, the allocators, the
// MTU and the window left to the function, then buffers of 0x1000, the host name and four DHCP retries
static const SOCConfig DWC_AC_SOC_CONFIG = {
    SOC_VENDOR_NINTENDO,
    SOC_VERSION,
    NULL,
    NULL,
    SOC_FLAG_DHCP,
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    { 0 },
    0x1000,
    0x1000,
    0,
    0,
    0,
    NULL,
    NULL,
    NULL,
    sSocHostName,
    4,
    0,
    0,
};

// State 1, starts the wireless manager with the notification callback of dwc_ac_wcm.c, after
// listing the targets of the first search. Returns the state that follows: 2, the search; 0x11, the teardown, if the
// wireless manager will not start; and 1, to wait, while it is not in its first state
u8 DWCi_AC_Start(void) {
    WCMConfig config;
    s32 wcmState = WCM_GetPhase();
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);
    s32 result;

    if (wcmState == 1) {
        config.dmano = work->dmaNo;
        config.pbdbuffer = NULL;
        config.nbdbuffer = 0;
        config.nbdmode = 0;
        DWCi_AC_MakeSearchList(0);
        result = WCM_StartupAsync(&config, DWCi_AC_CallBackWCM);
        if (result == 1 || result >= 4) {
            DWCi_AC_SetError(1);
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
u8 DWCi_AC_ConnectTest(void) {
    // The cases of both switches are in the order that the ROM lays out their bodies in
    u8 state = DWCi_AC_GetPhase();
    ACWORK *work = DWCi_AC_GetMemPtr(DWC_AC_WORK_SEARCH);

    if (WCM_GetPhase() == 9) {
        switch (state) {
        case 10:
            state = ConnectTestStart(work);
            break;
        case 12:
            state = GetIPAddress(work);
            break;
        case 13:
            state = ConnectTestCreate();
            break;
        case 14:
            state = ConnectTestProcess(work);
            break;
        case 15:
            state = ConnectTestEnd(work);
            break;
        case 11:
            state = ConnectTestRetry();
            break;
        }
    } else {
        switch (state) {
        case 15:
            state = ConnectTestEnd(work);
            break;
        case 11:
            state = ConnectTestRetry();
            break;
        case 14:
            DWC_Netcheck_Abort();
            DWC_Netcheck_Destroy();
            // fall through to the loss of the candidate
        default:
            work->findList[work->connectNo].state = DWC_AC_RESULT_LOST;
            state = 11;
            break;
        }
    }
    return state;
}

// State 10, starts SOC from the configuration that MakeSOCConfig makes, with the wait of SOCL's
// yields at 4. 12 to wait for the address, or 0x11, the teardown, if SOC will not start
static u8 ConnectTestStart(ACWORK *work) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);
    SOCConfig *config = DWCi_AC_GetMemPtr(DWC_AC_WORK_SOC_CONFIG);

    MakeSOCConfig(main, work, config);
    SOCLiYieldWait = 4;
    if (SOC_Startup(config) == 0) {
        return 12;
    }
    DWCi_AC_SetError(2);
    return 0x11;
}

// State 12, waits for the address. When it has come, gives SOC the DNS servers of the setting, and
// goes on to the connection test (13), or skips it (15) if the game asked for that or the setting on a DSi uses a
// proxy. If the address has not come in 10 seconds the candidate is given up, and SOC goes down (11)
static u8 GetIPAddress(ACWORK *work) {
    u8 type;

    if (SOC_GetHostID() != 0) {
        CheckSetDNS(work);
        if (work->skipConnectionTest == 1) {
            return 15;
        }
        type = DWCi_ConvConnectAPType(work->connectApType);
        if (type >= 10 && type <= 12) {
            if (OS_IsRunOnTwl() && work->dsiSettings[(u8)(type - 10)].unk_182 != 0) {
                return 15;
            }
        }
        return 13;
    }
    if (OS_TicksToSeconds(OS_GetTick() - work->timeOut) < 10) {
        return 12;
    }
    work->findList[work->connectNo].state = DWC_AC_RESULT_GAVE_UP;
    return 11;
}

// State 13, starts the connection test of overlay 11 with the memory functions. 14 to wait
// for it, or 0x11, the teardown, if it will not start
static u8 ConnectTestCreate(void) {
    if (DWC_Netcheck_Create(DWCi_AC_GetMemPtr(DWC_AC_WORK_MEMFUNCS)) == 0) {
        return 14;
    }
    DWCi_AC_SetError(3);
    return 0x11;
}

// State 14, waits for the connection test. Once it is over, takes its result for the error
// if the candidate is of the type that the game was told of, and lets the test go. 15 if it went well; else the
// candidate is given up, with a note if CPS found its address already in use, and SOC goes down (11)
static u8 ConnectTestProcess(ACWORK *work) {
    ACHOLD *main = DWCi_AC_GetMemPtr(DWC_AC_WORK_MAIN);
    DWCNetcheckError status = DWC_Netcheck_GetError();

    if (status != 0) {
        if (main->endType == DWCi_ConvConnectAPType(work->connectApType)) {
            main->errorTest = DWC_Netcheck_GetReturnCode();
        }
        DWC_Netcheck_Destroy();
        if (status != 11) {
            work->findList[work->connectNo].state = DWC_AC_RESULT_GAVE_UP;
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
static u8 ConnectTestEnd(ACWORK *work) {
    DWCi_AC_SetApType(work->connectApType);
    DWCAc_NoteKeyMode(work->connectApType);
    return 16;
}

// State 11, takes the sockets down: SOCL first, then SOC. 9, the end of the candidate, once they are
// down, else 11
static u8 ConnectTestRetry(void) {
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
static void MakeSOCConfig(ACHOLD *main, ACWORK *work, SOCConfig *config) {
    DWCMemPage *setting;
    u8 type;

    MI_CpuCopy8(&DWC_AC_SOC_CONFIG, config, sizeof(SOCConfig));
    config->alloc = (SocAllocFunc)main->alloc;
    config->free = (SocFreeFunc)main->free;
    if (work->connectApType >= 0x12) {
        return;
    }
    type = DWCi_ConvConnectAPType(work->connectApType);
    if (type <= 2) {
        setting = &work->userInfo[type];
    } else {
        setting = &work->dsiSettings[type - 10].base;
    }
    if (setting->ap.ip[0] != 0) {
        config->flag = 0;
        config->addr.addr = ConvAddress(setting->ap.ip);
        config->netmask.addr = ConvNetMask(setting->ap.netmask);
        config->router.addr = ConvAddress(setting->ap.gateway);
        config->dns1.addr = ConvAddress(setting->ap.dns[0]);
        config->dns2.addr = ConvAddress(setting->ap.dns[1]);
    } else {
        config->flag = SOC_FLAG_DHCP;
        config->addr.addr = 0;
        config->netmask.addr = 0;
        config->router.addr = 0;
        config->dns1.addr = 0;
        config->dns2.addr = 0;
    }
    if (OS_IsRunOnTwl()) {
        if (setting->ap.mtu == 0) {
            config->mtu = 0x578;
        } else if (setting->ap.mtu > 0x5dc) {
            config->mtu = 0x5dc;
        } else if (setting->ap.mtu < 0x240) {
            config->mtu = 0x240;
        } else {
            config->mtu = setting->ap.mtu;
        }
    } else {
        config->mtu = 0x240;
    }
    if (OS_IsRunOnTwl()) {
        if (setting->ap.mtu == 0x240) {
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
static u32 ConvAddress(const u8 *address) {
    u32 value = 0;

    value |= address[0] << 24;
    value |= address[1] << 16;
    value |= address[2] << 8;
    value |= address[3];
    return ((value >> 24) & 0xff) | ((value >> 8) & 0xff00) | ((value << 8) & 0xff0000) | ((value << 24) & 0xff000000);
}

// The netmask with that many of its high bits set, as SOC_Startup wants it, with its bytes in the
// other order
static u32 ConvNetMask(u8 bits) {
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
static void CheckSetDNS(ACWORK *work) {
    DWCMemPage *setting;
    SOCInAddr dns1;
    SOCInAddr dns2;
    int dnsSum;
    u8 type;

    if (work->connectApType >= 0x12) {
        return;
    }
    type = DWCi_ConvConnectAPType(work->connectApType);
    if (type <= 2) {
        setting = &work->userInfo[type];
    } else {
        setting = &work->dsiSettings[type - 10].base;
    }
    dnsSum = setting->ap.dns[0][0] + setting->ap.dns[0][1] + setting->ap.dns[0][2] + setting->ap.dns[0][3];
    if (setting->ap.ip[0] != 0 || dnsSum == 0) {
        return;
    }
    dns1.addr = ConvAddress(setting->ap.dns[0]);
    dns2.addr = ConvAddress(setting->ap.dns[1]);
    SOC_SetResolver(&dns1, &dns2);
}
