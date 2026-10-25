#include "setting.h"
#include "dwc/ext.h"
#include "dwc_util_heap.h"
#include "dwcutil/dwc_moveid.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "stdlib.h"
#include "str.h"

static void write(int index);
static int ascii2hex(u8 digit);

#define DWC_UTIL_SETTINGS_PAGES 4
#define DWC_UTIL_SETTINGS_CONNECTIONS 3

typedef struct DWCUtilWifiSettings {
    DWCMemPage pages[DWC_UTIL_SETTINGS_PAGES];
    DWCUtilSettingsEdit edit;
    MATHCRC16Table crcTable;
} DWCUtilWifiSettings;

static const u8 sWifiSettingsZeroAddress[4] = { 0, 0, 0, 0 };

static DWCUtilWifiSettings *sWifiSettings;

void DWCi_SETTIGNlInit(void) {
    sWifiSettings = DWCi_HEAPlAllocEx(sizeof(DWCUtilWifiSettings), 32);
    MATHi_CRC16InitTableRev(&sWifiSettings->crcTable, 0xa001);
    DWCBm_ReadAllNtrPages(sWifiSettings);
}

void DWCi_SETTIGNlEnd(void) {
    DWCi_HEAPlFree_(&sWifiSettings);
}

DWCUtilSettingsEdit *DWCUtilWifiSettings_GetEdit(void) {
    return &sWifiSettings->edit;
}

void DWCi_SETTINGlSetIpAuto(BOOL autoAddress) {
    sWifiSettings->edit.autoAddress = autoAddress;
}

void DWCi_SETTINGlSetDnsAuto(BOOL autoDns) {
    sWifiSettings->edit.autoDns = autoDns;
}

void DWCi_SETTINGlSetSsid(const u8 *ssid) {
    MI_CpuCopy8(ssid, sWifiSettings->edit.connection.ssid, sizeof(sWifiSettings->edit.connection.ssid));
    sWifiSettings->edit.connection.setType = 0;
}

void DWCi_SETTINGlSetWep(const u8 *key) {
    int i;
    int length;
    u8 *dest;

    MI_CpuFill8(sWifiSettings->edit.connection.wepKey[0], 0, sizeof(sWifiSettings->edit.connection.wepKey[0]));
    length = DWCi_STRlNLen(key, 32);
    switch (length) {
    case 0:
    case 10:
    case 26:
    case 32:
        sWifiSettings->edit.connection.wepNotation = 0;
        dest = sWifiSettings->edit.connection.wepKey[0];
        for (i = 0; i < length; i += 2) {
            *dest = (ascii2hex(key[i]) << 4) + ascii2hex(key[i + 1]);
            dest++;
        }
        break;
    default:
        sWifiSettings->edit.connection.wepNotation = 1;
        MI_CpuCopy8(key, sWifiSettings->edit.connection.wepKey[0], sizeof(sWifiSettings->edit.connection.wepKey[0]));
        break;
    }
    switch (length) {
    case 0:
        sWifiSettings->edit.connection.wepMode = 0;
        break;
    case 5:
    case 10:
        sWifiSettings->edit.connection.wepMode = 1;
        break;
    case 13:
    case 26:
        sWifiSettings->edit.connection.wepMode = 2;
        break;
    default:
        sWifiSettings->edit.connection.wepMode = 3;
        break;
    }
}

void DWCUtilWifiSettings_SetIp(const u8 *digits) {
    DWCi_SETTINGlConvAddress(digits, sWifiSettings->edit.connection.ip);
}

void DWCUtilWifiSettings_SetNetmask(const u8 *digits) {
    DWCi_SETTINGlConvAddress(digits, sWifiSettings->edit.netmask);
}

void DWCi_SETTINGlSetGateway(const u8 *digits) {
    DWCi_SETTINGlConvAddress(digits, sWifiSettings->edit.connection.gateway);
}

void DWCi_SETTINGlSetDnsP(const u8 *digits) {
    DWCi_SETTINGlConvAddress(digits, sWifiSettings->edit.connection.dns[0]);
}

void DWCi_SETTINGlSetDnsS(const u8 *digits) {
    DWCi_SETTINGlConvAddress(digits, sWifiSettings->edit.connection.dns[1]);
}

void DWCi_SETTINGlGetSsid(u8 *ssid) {
    MI_CpuCopy8(sWifiSettings->edit.connection.ssid, ssid, sizeof(sWifiSettings->edit.connection.ssid));
}

void DWCi_SETTINGlGetIp(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.ip;

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCi_SETTINGlGetNetMask(u8 *digits) {
    u8 *address = sWifiSettings->edit.netmask;

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCi_SETTINGlGetGateway(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.gateway;

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCi_SETTINGlGetDnsP(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.dns[0];

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCi_SETTINGlGetDnsS(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.dns[1];

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

u8 DWCi_SETTINGlGetSetType(int index) {
    return sWifiSettings->pages[index].ap.setType;
}

void DWCi_SETTINGlSetSetting(int index) {
    DWCUtilWifiSettings *settings = sWifiSettings;

    settings->edit.connection = settings->pages[index].ap;
    settings->edit.index = index;
    if (memcmp(settings->pages[index].ap.ip, sWifiSettingsZeroAddress, 4) != 0) {
        sWifiSettings->edit.autoAddress = FALSE;
    } else {
        sWifiSettings->edit.autoAddress = TRUE;
    }
    if (memcmp(settings->pages[index].ap.dns[0], sWifiSettingsZeroAddress, 4) != 0 ||
        memcmp(settings->pages[index].ap.dns[1], sWifiSettingsZeroAddress, 4) != 0) {
        sWifiSettings->edit.autoDns = FALSE;
    } else {
        sWifiSettings->edit.autoDns = TRUE;
    }
    DWCi_BACKUPlConvMaskAddr(settings->pages[index].ap.netmask, sWifiSettings->edit.netmask);
}

void DWCi_SETTINGlSet(void) {
    DWCUtilWifiSettings *settings = sWifiSettings;
    DWCUtilSettingsEdit *edit = &settings->edit;
    DWCBMApInfo *connection = &settings->pages[edit->index].ap;

    *connection = edit->connection;
    if (edit->autoAddress) {
        MI_CpuFill8(connection->ip, 0, 4);
        MI_CpuFill8(connection->gateway, 0, 4);
        connection->netmask = 0;
    } else {
        MI_CpuCopy8(edit->connection.ip, connection->ip, 4);
        MI_CpuCopy8(edit->connection.gateway, connection->gateway, 4);
        connection->netmask = DWCi_BACKUPlConvMaskCidr(edit->netmask);
    }
    if (edit->autoDns) {
        MI_CpuFill8(connection->dns, 0, 8);
    } else {
        MI_CpuCopy8(edit->connection.dns, connection->dns, 8);
    }
    write(edit->index);
}

void DWCi_SETTINGlSetRaku(const ATERMApConfigParam *result) {
    DWCUtilWifiSettings *settings = sWifiSettings;
    DWCUtilSettingsEdit *edit = &settings->edit;
    int length = 0;
    int i;

    MI_CpuFill8(&edit->connection, 0, offsetof(DWCBMApInfo, state));
    MI_CpuCopy8(result->ssid, edit->connection.ssid, sizeof(edit->connection.ssid));
    switch (result->auth_mode) {
    case 1:
        edit->connection.wepMode = 1;
        length = 5;
        break;
    case 2:
        edit->connection.wepMode = 2;
        length = 13;
        break;
    case 3:
        edit->connection.wepMode = 3;
        length = 16;
        break;
    default:
        edit->connection.wepMode = 0;
        break;
    }
    edit->connection.wepNotation = 0;
    for (i = 0; i < 4; i++) {
        MI_CpuCopy8(result->wepkey[i], edit->connection.wepKey[i], length);
    }
    edit->connection.setType = 2;
    MI_CpuFill8(edit->netmask, 0, 4);
    edit->autoAddress = TRUE;
    edit->autoDns = TRUE;
    DWCi_SETTINGlSet();
}

void DWCi_SETTINGlSetAoss(const AOSS_SECURE_INFO *result) {
    DWCUtilWifiSettings *settings = sWifiSettings;
    DWCUtilSettingsEdit *edit = &settings->edit;

    MI_CpuFill8(&edit->connection, 0, offsetof(DWCBMApInfo, state));
    MI_CpuCopy8(result->wep40Key[0], edit->connection.wep2[0], 5);
    MI_CpuCopy8(result->wep40Key[1], edit->connection.wep2[1], 5);
    MI_CpuCopy8(result->wep40Key[2], edit->connection.wep2[2], 5);
    MI_CpuCopy8(result->wep40Key[3], edit->connection.wep2[3], 5);
    MI_CpuCopy8(result->WEP40_SSID, edit->connection.aossSsid, 32);
    MI_CpuCopy8(result->wep104Key[0], edit->connection.wepKey[0], 13);
    MI_CpuCopy8(result->wep104Key[1], edit->connection.wepKey[1], 13);
    MI_CpuCopy8(result->wep104Key[2], edit->connection.wepKey[2], 13);
    MI_CpuCopy8(result->wep104Key[3], edit->connection.wepKey[3], 13);
    MI_CpuCopy8(result->WEP104_SSID, edit->connection.ssid, 32);
    edit->connection.wepMode = 2;
    edit->connection.wepNotation = 0;
    edit->connection.setType = 1;
    MI_CpuFill8(edit->netmask, 0, 4);
    edit->autoAddress = TRUE;
    edit->autoDns = TRUE;
    DWCi_SETTINGlSet();
}

u8 *DWCi_SETTINGlGetMemMap(void) {
    return (u8 *)sWifiSettings;
}

void DWCi_SETTINGlErasePage(int index) {
    DWCUtilWifiSettings *settings = sWifiSettings;

    MI_CpuFill8(&settings->pages[index], 0, offsetof(DWCBMApInfo, state));
    settings->pages[index].ap.setType = DWC_SETTYPE_NONE;
    write(index);
}

void DWCi_SETTINGlEraseAll(void) {
    DWCWiFiInfo info;
    const u8 *packed;
    int i;

    MI_CpuClear16(sWifiSettings->pages, sizeof(sWifiSettings->pages));
    for (i = 0; i < DWC_UTIL_SETTINGS_CONNECTIONS; i++) {
        sWifiSettings->pages[i].ap.setType = DWC_SETTYPE_NONE;
    }
    DWCi_AUTH_GetNewWiFiInfo(&info);
    packed = DWCi_BACKUPlConvWifiInfo(&info);
    for (i = 0; i < 2; i++) {
        MI_CpuCopy8(packed, sWifiSettings->pages[i].wifi, DWC_BM_PACKED_SIZE);
    }
    for (i = 0; i < DWC_UTIL_SETTINGS_PAGES; i++) {
        write(i);
    }
}

// Writes a page to the NVRAM, with the first two, which keep the mask of the connections in use, when that changes
static void write(int index) {
    u8 status = sWifiSettings->pages[index].ap.setType;
    u32 bit = 1 << index;
    BOOL used = FALSE;
    BOOL write[DWC_UTIL_SETTINGS_PAGES];
    void *scratch;
    int i;

    MI_CpuFill8(write, 0, sizeof(write));
    write[index] = TRUE;
    if (index <= 2) {
        if (sWifiSettings->pages[0].ap.state & bit) {
            used = TRUE;
        }
        if (status == DWC_SETTYPE_NONE && used) {
            sWifiSettings->pages[0].ap.state &= ~bit;
            sWifiSettings->pages[1].ap.state &= ~bit;
            write[1] = TRUE;
            write[0] = TRUE;
        } else if (status != DWC_SETTYPE_NONE && !used) {
            sWifiSettings->pages[0].ap.state |= bit;
            sWifiSettings->pages[1].ap.state |= bit;
            write[1] = TRUE;
            write[0] = TRUE;
        }
    }
    for (i = 0; i < DWC_UTIL_SETTINGS_PAGES; i++) {
        if (write[i]) {
            sWifiSettings->pages[i].crc =
                MATH_CalcCRC16(&sWifiSettings->crcTable, &sWifiSettings->pages[i], sizeof(DWCMemPage) - sizeof(u16));
        }
    }
    scratch = DWCi_HEAPlAllocEx(0x100, 32);
    DWCBm_WriteNtrPages(sWifiSettings, write, scratch);
    DWCi_HEAPlFree_(&scratch);
}

void DWCi_SETTINGlConvAddress(const u8 *digits, u8 *address) {
    u8 number[4];
    int i;
    int j;

    MI_CpuFill8(number, 0, 4);
    for (i = 0; i < 4; i++) {
        MI_CpuCopy8(digits + i * 3, number, 3);
        for (j = 0; j < 3; j++) {
            if (number[j] != 0) {
                break;
            }
            number[j] = ' ';
        }
        address[i] = atol((char *)number);
    }
}

// The value of a hex digit
static int ascii2hex(u8 digit) {
    if (digit <= '9') {
        return digit - '0';
    }
    if (digit <= 'F') {
        return digit - 'A' + 10;
    }
    return digit - 'a' + 10;
}
