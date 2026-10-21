#include "dwc_util_wifi_settings.h"
#include "app/download_play_sys.h"
#include "dwc/ext.h"
#include "dwc_util_heap.h"
#include "dwc_util_string.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "stdlib.h"

static void DWCUtilWifiSettings_WritePage(int index);
static int DWCUtilWifiSettings_ParseHexDigit(u8 digit);

#define DWC_UTIL_SETTINGS_PAGES 4
#define DWC_UTIL_SETTINGS_CONNECTIONS 3

typedef struct DWCUtilWifiSettings {
    DWCBmNtrPage pages[DWC_UTIL_SETTINGS_PAGES];
    DWCUtilSettingsEdit edit;
    MATHCRC16Table crcTable;
} DWCUtilWifiSettings;

static const u8 sWifiSettingsZeroAddress[4] = { 0, 0, 0, 0 };

static DWCUtilWifiSettings *sWifiSettings;

void DWCUtilWifiSettings_Init(void) {
    sWifiSettings = DWCUtilHeap_AllocClear(sizeof(DWCUtilWifiSettings), 32);
    MATHi_CRC16InitTableRev(&sWifiSettings->crcTable, 0xa001);
    DWCBm_ReadAllNtrPages(sWifiSettings);
}

void DWCUtilWifiSettings_Exit(void) {
    DWCUtilHeap_FreeAndNull(&sWifiSettings);
}

DWCUtilSettingsEdit *DWCUtilWifiSettings_GetEdit(void) {
    return &sWifiSettings->edit;
}

void DWCUtilWifiSettings_SetAutoAddress(BOOL autoAddress) {
    sWifiSettings->edit.autoAddress = autoAddress;
}

void DWCUtilWifiSettings_SetAutoDns(BOOL autoDns) {
    sWifiSettings->edit.autoDns = autoDns;
}

void DWCUtilWifiSettings_SetSsid(const u8 *ssid) {
    MI_CpuCopy8(ssid, sWifiSettings->edit.connection.ssid, sizeof(sWifiSettings->edit.connection.ssid));
    sWifiSettings->edit.connection.status = 0;
}

void DWCUtilWifiSettings_SetWepKey(const u8 *key) {
    int i;
    int length;
    u8 *dest;

    MI_CpuFill8(sWifiSettings->edit.connection.wepKey[0], 0, sizeof(sWifiSettings->edit.connection.wepKey[0]));
    length = DWCUtilString_Length(key, 32);
    switch (length) {
    case 0:
    case 10:
    case 26:
    case 32:
        sWifiSettings->edit.connection.wepKeyText = 0;
        dest = sWifiSettings->edit.connection.wepKey[0];
        for (i = 0; i < length; i += 2) {
            *dest = (DWCUtilWifiSettings_ParseHexDigit(key[i]) << 4) + DWCUtilWifiSettings_ParseHexDigit(key[i + 1]);
            dest++;
        }
        break;
    default:
        sWifiSettings->edit.connection.wepKeyText = 1;
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
    DWCUtilWifiSettings_DigitsToAddress(digits, sWifiSettings->edit.connection.ip);
}

void DWCUtilWifiSettings_SetNetmask(const u8 *digits) {
    DWCUtilWifiSettings_DigitsToAddress(digits, sWifiSettings->edit.netmask);
}

void DWCUtilWifiSettings_SetGateway(const u8 *digits) {
    DWCUtilWifiSettings_DigitsToAddress(digits, sWifiSettings->edit.connection.gateway);
}

void DWCUtilWifiSettings_SetDns1(const u8 *digits) {
    DWCUtilWifiSettings_DigitsToAddress(digits, sWifiSettings->edit.connection.dns[0]);
}

void DWCUtilWifiSettings_SetDns2(const u8 *digits) {
    DWCUtilWifiSettings_DigitsToAddress(digits, sWifiSettings->edit.connection.dns[1]);
}

void DWCUtilWifiSettings_GetSsid(u8 *ssid) {
    MI_CpuCopy8(sWifiSettings->edit.connection.ssid, ssid, sizeof(sWifiSettings->edit.connection.ssid));
}

void DWCUtilWifiSettings_GetIp(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.ip;

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCUtilWifiSettings_GetNetmask(u8 *digits) {
    u8 *address = sWifiSettings->edit.netmask;

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCUtilWifiSettings_GetGateway(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.gateway;

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCUtilWifiSettings_GetDns1(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.dns[0];

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

void DWCUtilWifiSettings_GetDns2(u8 *digits) {
    u8 *address = sWifiSettings->edit.connection.dns[1];

    OS_SPrintf((char *)digits, "%3d%3d%3d%3d", address[0], address[1], address[2], address[3]);
}

u8 DWCUtilWifiSettings_GetStatus(int index) {
    return sWifiSettings->pages[index].connection.status;
}

void DWCUtilWifiSettings_EditConnection(int index) {
    DWCUtilWifiSettings *settings = sWifiSettings;

    settings->edit.connection = settings->pages[index].connection;
    settings->edit.index = index;
    if (memcmp(settings->pages[index].connection.ip, sWifiSettingsZeroAddress, 4) != 0) {
        sWifiSettings->edit.autoAddress = FALSE;
    } else {
        sWifiSettings->edit.autoAddress = TRUE;
    }
    if (memcmp(settings->pages[index].connection.dns[0], sWifiSettingsZeroAddress, 4) != 0 ||
        memcmp(settings->pages[index].connection.dns[1], sWifiSettingsZeroAddress, 4) != 0) {
        sWifiSettings->edit.autoDns = FALSE;
    } else {
        sWifiSettings->edit.autoDns = TRUE;
    }
    DWCBm_MakeMask(settings->pages[index].connection.netmaskBits, sWifiSettings->edit.netmask);
}

void DWCUtilWifiSettings_Save(void) {
    DWCUtilWifiSettings *settings = sWifiSettings;
    DWCUtilSettingsEdit *edit = &settings->edit;
    DWCBmConnection *connection = &settings->pages[edit->index].connection;

    *connection = edit->connection;
    if (edit->autoAddress) {
        MI_CpuFill8(connection->ip, 0, 4);
        MI_CpuFill8(connection->gateway, 0, 4);
        connection->netmaskBits = 0;
    } else {
        MI_CpuCopy8(edit->connection.ip, connection->ip, 4);
        MI_CpuCopy8(edit->connection.gateway, connection->gateway, 4);
        connection->netmaskBits = DWCBm_CountMaskBits(edit->netmask);
    }
    if (edit->autoDns) {
        MI_CpuFill8(connection->dns, 0, 8);
    } else {
        MI_CpuCopy8(edit->connection.dns, connection->dns, 8);
    }
    DWCUtilWifiSettings_WritePage(edit->index);
}

void DWCUtilWifiSettings_SaveAcResult(const DWCUtilAcResult *result) {
    DWCUtilWifiSettings *settings = sWifiSettings;
    DWCUtilSettingsEdit *edit = &settings->edit;
    int length = 0;
    int i;

    MI_CpuFill8(&edit->connection, 0, offsetof(DWCBmConnection, usedMask));
    MI_CpuCopy8(result->ssid, edit->connection.ssid, sizeof(edit->connection.ssid));
    switch (result->wepMode) {
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
    edit->connection.wepKeyText = 0;
    for (i = 0; i < 4; i++) {
        MI_CpuCopy8(result->keys[i], edit->connection.wepKey[i], length);
    }
    edit->connection.status = 2;
    MI_CpuFill8(edit->netmask, 0, 4);
    edit->autoAddress = TRUE;
    edit->autoDns = TRUE;
    DWCUtilWifiSettings_Save();
}

void DWCUtilWifiSettings_SaveAossResult(const AossResult *result) {
    DWCUtilWifiSettings *settings = sWifiSettings;
    DWCUtilSettingsEdit *edit = &settings->edit;

    MI_CpuFill8(&edit->connection, 0, offsetof(DWCBmConnection, usedMask));
    MI_CpuCopy8(result->wep40Key[0], edit->connection.aossKey[0], 5);
    MI_CpuCopy8(result->wep40Key[1], edit->connection.aossKey[1], 5);
    MI_CpuCopy8(result->wep40Key[2], edit->connection.aossKey[2], 5);
    MI_CpuCopy8(result->wep40Key[3], edit->connection.aossKey[3], 5);
    MI_CpuCopy8(result->wep40Ssid, edit->connection.aossSsid, 32);
    MI_CpuCopy8(result->wep104Key[0], edit->connection.wepKey[0], 13);
    MI_CpuCopy8(result->wep104Key[1], edit->connection.wepKey[1], 13);
    MI_CpuCopy8(result->wep104Key[2], edit->connection.wepKey[2], 13);
    MI_CpuCopy8(result->wep104Key[3], edit->connection.wepKey[3], 13);
    MI_CpuCopy8(result->wep104Ssid, edit->connection.ssid, 32);
    edit->connection.wepMode = 2;
    edit->connection.wepKeyText = 0;
    edit->connection.status = 1;
    MI_CpuFill8(edit->netmask, 0, 4);
    edit->autoAddress = TRUE;
    edit->autoDns = TRUE;
    DWCUtilWifiSettings_Save();
}

u8 *DWCUtilWifiSettings_GetPages(void) {
    return (u8 *)sWifiSettings;
}

void DWCUtilWifiSettings_Erase(int index) {
    DWCUtilWifiSettings *settings = sWifiSettings;

    MI_CpuFill8(&settings->pages[index], 0, offsetof(DWCBmConnection, usedMask));
    settings->pages[index].connection.status = DWC_BM_CONNECTION_UNUSED;
    DWCUtilWifiSettings_WritePage(index);
}

void DWCUtilWifiSettings_EraseAll(void) {
    DWCBmWifiIdInfo info;
    const u8 *packed;
    int i;

    MI_CpuClear16(sWifiSettings->pages, sizeof(sWifiSettings->pages));
    for (i = 0; i < DWC_UTIL_SETTINGS_CONNECTIONS; i++) {
        sWifiSettings->pages[i].connection.status = DWC_BM_CONNECTION_UNUSED;
    }
    DWCBm_MakePendingWifiId(&info);
    packed = DWCBm_PackWifiIdInfo(&info);
    for (i = 0; i < 2; i++) {
        MI_CpuCopy8(packed, sWifiSettings->pages[i].wifiId, DWC_BM_PACKED_SIZE);
    }
    for (i = 0; i < DWC_UTIL_SETTINGS_PAGES; i++) {
        DWCUtilWifiSettings_WritePage(i);
    }
}

// Writes a page to the NVRAM, with the first two, which keep the mask of the connections in use, when that changes
static void DWCUtilWifiSettings_WritePage(int index) {
    u8 status = sWifiSettings->pages[index].connection.status;
    u32 bit = 1 << index;
    BOOL used = FALSE;
    u32 write[DWC_UTIL_SETTINGS_PAGES];
    void *scratch;
    int i;

    MI_CpuFill8(write, 0, sizeof(write));
    write[index] = TRUE;
    if (index <= 2) {
        if (sWifiSettings->pages[0].connection.usedMask & bit) {
            used = TRUE;
        }
        if (status == DWC_BM_CONNECTION_UNUSED && used) {
            sWifiSettings->pages[0].connection.usedMask &= ~bit;
            sWifiSettings->pages[1].connection.usedMask &= ~bit;
            write[1] = TRUE;
            write[0] = TRUE;
        } else if (status != DWC_BM_CONNECTION_UNUSED && !used) {
            sWifiSettings->pages[0].connection.usedMask |= bit;
            sWifiSettings->pages[1].connection.usedMask |= bit;
            write[1] = TRUE;
            write[0] = TRUE;
        }
    }
    for (i = 0; i < DWC_UTIL_SETTINGS_PAGES; i++) {
        if (write[i]) {
            sWifiSettings->pages[i].crc16 =
                MATH_CalcCRC16(&sWifiSettings->crcTable, &sWifiSettings->pages[i], sizeof(DWCBmNtrPage) - sizeof(u16));
        }
    }
    scratch = DWCUtilHeap_AllocClear(0x100, 32);
    DWCBm_WriteNtrPages(sWifiSettings, write, scratch);
    DWCUtilHeap_FreeAndNull(&scratch);
}

void DWCUtilWifiSettings_DigitsToAddress(const u8 *digits, u8 *address) {
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
static int DWCUtilWifiSettings_ParseHexDigit(u8 digit) {
    if (digit <= '9') {
        return digit - '0';
    }
    if (digit <= 'F') {
        return digit - 'A' + 10;
    }
    return digit - 'a' + 10;
}
