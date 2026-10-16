// The wireless manager's data sharing (libwm, TwlSDK's wm/ARM9/src/wm_ds.c, by its place in the library: the file name
// is a guess): in every round of the multi-point mode each console gives a few bytes, the parent collects them in a
// set and sends the set back to all, and the game reads the sets out in step with the rounds. The parent keeps
// WM_DS_SET_COUNT sets in a ring, one being filled; in the double mode a second one is in flight. The names are
// descriptive
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/wm.h"

static void WmDataSharingSenderCallback(void *arg);
static void WmDataSharingParentCallback(void *arg);
static void WmDataSharingChildCallback(void *arg);
static void WmDataSharingSetData(WMDataSharingInfo *dsi, u16 aid, const void *data);
static void WmDataSharingSendDataSet(WMDataSharingInfo *dsi, int dropParent);
static u8 *WmGetSharedDataAddress(const WMDataSharingInfo *dsi, u16 aidBitmap, const u16 *data, u16 aid);

// The data a child sends is not at the start of its set, but at this offset, which the cache lines divide
#define WM_DS_CHILD_SEND_OFFSET 0x20

// WM_StartDataSharing: dataLength is the bytes each console gives (all share at most WM_DS_SET_DATA_MAX) and the
// AIDs in aidBitmap share, besides this console
int func_02081bd8(WMDataSharingInfo *dsi, u16 port, u16 aidBitmap, u16 dataLength, BOOL doubleMode) {
    u32 myAid;
    u32 connectedBitmap = 1;
    int result = func_02080ab0(2, WM_STATE_MP_PARENT, WM_STATE_MP_CHILD);
    u16 bit;
    u16 stationNumber;
    int i;

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (dsi == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (port >= WM_NUM_OF_PORT) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (aidBitmap == 0) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    myAid = func_02080dd8();
    if (myAid == 0) {
        connectedBitmap = func_02080dfc();
    }
    MI_CpuClearFast(dsi, sizeof(WMDataSharingInfo));
    dsi->writeIndex = 0;
    dsi->sendIndex = 0;
    dsi->readIndex = 0;
    dsi->dataLength = dataLength;
    dsi->port = port;
    dsi->aidBitmap = 0;
    dsi->doubleMode = (doubleMode != 0);
    bit = 1 << myAid;
    dsi->aidBitmap = aidBitmap | bit;
    stationNumber = MATH_CountPopulation(aidBitmap | bit);
    dsi->stationNumber = stationNumber;
    dsi->dataSetLength = dataLength * stationNumber;
    if (dsi->dataSetLength > WM_DS_SET_DATA_MAX) {
        dsi->aidBitmap = 0;
        return WM_ERRCODE_INVALID_PARAM;
    }
    dsi->dataSetLength += 4;
    dsi->state = WM_DS_STATE_RUNNING;
    if (myAid == 0) {
        for (i = 0; i < WM_DS_SET_COUNT; i++) {
            dsi->ds[i].aidBitmap = dsi->aidBitmap & (connectedBitmap | 1);
        }
        func_02080eac(port, WmDataSharingParentCallback, dsi);
        for (i = 0; i < (dsi->doubleMode == 1 ? 2 : 1); i++) {
            dsi->writeIndex = (dsi->writeIndex + 1) & 3;
            result = func_02081998(WmDataSharingSenderCallback, dsi, &dsi->ds[i], dsi->dataSetLength,
                                   connectedBitmap & dsi->aidBitmap, dsi->port, 1);
            if (result == WM_ERRCODE_NO_CHILD) {
                dsi->seqNum[i] = 0xffff;
                dsi->sendIndex = (dsi->sendIndex + 1) & 3;
            } else if (result != WM_ERRCODE_SUCCESS && result != WM_ERRCODE_OPERATING) {
                dsi->state = WM_DS_STATE_ERROR;
                return WM_ERRCODE_FAILED;
            }
        }
    } else {
        dsi->sendIndex = 3;
        func_02080eac(port, WmDataSharingChildCallback, dsi);
    }
    return WM_ERRCODE_SUCCESS;
}

// WM_EndDataSharing
int func_02081dbc(WMDataSharingInfo *dsi) {
    if (dsi == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (dsi->aidBitmap == 0) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    func_02080eac(dsi->port, NULL, NULL);
    dsi->aidBitmap = 0;
    dsi->state = WM_DS_STATE_OFF;
    return WM_ERRCODE_SUCCESS;
}

// WM_StepDataSharing: gives the game the next set (in dataSet) if there is one, and takes the data of this console for
// the next round. NO_DATASET when there was no set
int func_02081df0(WMDataSharingInfo *dsi, const void *data, WMDataSet *dataSet) {
    int result = func_02080ab0(2, WM_STATE_MP_PARENT, WM_STATE_MP_CHILD);
    u16 myAid;
    u16 connectedBitmap;
    u16 index;
    int step;

    if (result != WM_ERRCODE_SUCCESS) {
        return result;
    }
    if (dsi == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (data == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    if (dataSet == NULL) {
        return WM_ERRCODE_INVALID_PARAM;
    }
    myAid = func_02080dd8();
    if (myAid == 0) {
        connectedBitmap = func_02080dfc();
    }
    if (dsi->state == WM_DS_STATE_ERROR) {
        return WM_ERRCODE_FAILED;
    }
    if (dsi->state != WM_DS_STATE_RUNNING && dsi->state != WM_DS_STATE_SEND_FAILED) {
        return WM_ERRCODE_ILLEGAL_STATE;
    }
    result = WM_ERRCODE_NO_DATASET;
    if (myAid == 0) {
        BOOL gotSet = FALSE;
        int dropParent = FALSE;

        if (dsi->state == WM_DS_STATE_SEND_FAILED) {
            dsi->state = WM_DS_STATE_RUNNING;
            index = (dsi->writeIndex + 3) & 3;
            step = func_02081998(WmDataSharingSenderCallback, dsi, &dsi->ds[index], dsi->dataSetLength,
                                 connectedBitmap & dsi->aidBitmap, dsi->port, 1);
            if (step == WM_ERRCODE_NO_CHILD) {
                dsi->seqNum[index] = 0xffff;
                dsi->sendIndex = (dsi->sendIndex + 1) & 3;
            } else if (step != WM_ERRCODE_SUCCESS && step != WM_ERRCODE_OPERATING) {
                dsi->state = WM_DS_STATE_ERROR;
                return WM_ERRCODE_FAILED;
            }
        }
        if (dsi->readIndex != dsi->sendIndex) {
            dsi->ds[dsi->readIndex].aidBitmap |= 1;
            MI_CpuCopy16(&dsi->ds[dsi->readIndex], dataSet, sizeof(WMDataSet));
            dsi->currentSeqNum = dsi->seqNum[dsi->readIndex];
            dsi->readIndex = (dsi->readIndex + 1) & 3;
            result = WM_ERRCODE_SUCCESS;
            gotSet = TRUE;
            if (dsi->doubleMode == 0 && connectedBitmap != 0 && dsi->ds[dsi->writeIndex].aidBitmap == 1) {
                dropParent = TRUE;
            } else {
                dropParent = FALSE;
            }
        }
        WmDataSharingSendDataSet(dsi, FALSE);
        if (gotSet) {
            WmDataSharingSetData(dsi, 0, data);
            if (dsi->doubleMode == 0) {
                WmDataSharingSendDataSet(dsi, dropParent);
            }
        }
    } else {
        BOOL send = FALSE;

        if (dsi->state == WM_DS_STATE_SEND_FAILED) {
            dsi->state = WM_DS_STATE_RUNNING;
            send = TRUE;
        } else if (dsi->readIndex != dsi->writeIndex) {
            if (!(dsi->ds[dsi->readIndex].aidBitmap & 1)) {
                dsi->ds[dsi->readIndex].aidBitmap |= 1;
            } else {
                MI_CpuCopy16(&dsi->ds[dsi->readIndex], dataSet, sizeof(WMDataSet));
                send = TRUE;
                result = WM_ERRCODE_SUCCESS;
                dsi->currentSeqNum = dsi->seqNum[dsi->readIndex];
                dsi->readIndex = (dsi->readIndex + 1) & 3;
            }
        }
        if (send) {
            u16 *sendBuf = (u16 *)((u8 *)&dsi->ds[dsi->sendIndex] + WM_DS_CHILD_SEND_OFFSET);

            MI_CpuCopy16(data, sendBuf, dsi->dataLength);
            step =
                func_02081998(WmDataSharingSenderCallback, dsi, sendBuf, dsi->dataLength, dsi->aidBitmap, dsi->port, 1);
            dsi->sendIndex = (dsi->sendIndex + 1) & 3;
            if (step != WM_ERRCODE_OPERATING && step != WM_ERRCODE_SUCCESS) {
                dsi->state = WM_DS_STATE_ERROR;
                result = WM_ERRCODE_FAILED;
            }
        }
    }
    return result;
}

// What a send of a set comes back to: the parent notes the set's sequence number, and a send that found the queue full
// is to be repeated by the next step (a child takes its set back)
static void WmDataSharingSenderCallback(void *arg) {
    WMPortSendCallback *cb = arg;
    WMArm9Buf *w9b = func_02080a64();
    WMDataSharingInfo *dsi = w9b->portCallbackArgTable[cb->port];
    WMCallbackFunc callback = w9b->portCallbackTable[cb->port];

    if ((callback == WmDataSharingParentCallback || callback == WmDataSharingChildCallback) && dsi != NULL &&
        dsi == cb->arg) {
        u16 myAid = func_02080dd8();

        if (cb->errcode == WM_ERRCODE_SUCCESS) {
            if (myAid == 0) {
                dsi->seqNum[dsi->sendIndex] = cb->seqNo >> 1;
                dsi->sendIndex = (dsi->sendIndex + 1) & 3;
            }
        } else if (cb->errcode == WM_ERRCODE_SEND_QUEUE_FULL) {
            if (myAid != 0) {
                dsi->sendIndex = (dsi->sendIndex + 3) & 3;
            }
            dsi->state = WM_DS_STATE_SEND_FAILED;
        } else {
            dsi->state = WM_DS_STATE_ERROR;
        }
    }
}

// The parent's port callback: the data of a child comes in, and a child connects or disconnects
static void WmDataSharingParentCallback(void *arg) {
    WMPortRecvCallback *cb = arg;
    WMDataSharingInfo *dsi = cb->arg;

    if (dsi == NULL) {
        return;
    }
    if (cb->errcode == WM_ERRCODE_SUCCESS) {
        switch (cb->state) {
        case WM_STATECODE_PORT_RECV:
            WmDataSharingSetData(dsi, cb->aid, cb->data);
            WmDataSharingSendDataSet(dsi, FALSE);
            break;
        case WM_STATECODE_CONNECTED:
            WmDataSharingSendDataSet(dsi, FALSE);
            break;
        case WM_STATECODE_DISCONNECTED:
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF: {
            int bit = 1 << cb->aid;
            u32 irq = CPU_IRQDisable();
            u16 index = dsi->writeIndex;

            dsi->ds[index].aidBitmap &= ~bit;
            if (dsi->doubleMode == 1) {
                u16 next = (index + 1) & 3;

                dsi->ds[next].aidBitmap &= ~bit;
            }
            CPU_SetIRQMask(irq);
            WmDataSharingSendDataSet(dsi, FALSE);
            if (dsi->doubleMode == 1) {
                WmDataSharingSendDataSet(dsi, FALSE);
            }
            break;
        }
        case WM_STATECODE_PORT_INIT:
            break;
        }
    } else {
        dsi->state = WM_DS_STATE_ERROR;
    }
}

// The child's port callback: the set from the parent comes in
static void WmDataSharingChildCallback(void *arg) {
    WMPortRecvCallback *cb = arg;
    WMDataSharingInfo *dsi = cb->arg;

    if (dsi == NULL) {
        return;
    }
    if (cb->errcode == WM_ERRCODE_SUCCESS) {
        switch (cb->state) {
        case WM_STATECODE_PORT_RECV: {
            WMDataSet *recvSet = (WMDataSet *)cb->data;
            u16 length = cb->length;
            u16 aidBitmap = recvSet->aidBitmap;
            u16 myAid = func_02080dd8();

            if (length != dsi->dataSetLength) {
                if (length > WM_DS_SET_SIZE) {
                    length = WM_DS_SET_SIZE;
                }
            }
            if (length < 4) {
                break;
            }
            if (!(aidBitmap & (1 << myAid))) {
                break;
            }
            MI_CpuCopy16(recvSet, &dsi->ds[dsi->writeIndex], length);
            dsi->seqNum[dsi->writeIndex] = cb->seqNo >> 1;
            dsi->writeIndex = (dsi->writeIndex + 1) & 3;
            break;
        }
        case WM_STATECODE_CONNECTED:
        case WM_STATECODE_DISCONNECTED:
        case WM_STATECODE_PORT_INIT:
        case WM_STATECODE_DISCONNECTED_FROM_MYSELF:
            break;
        }
    } else {
        dsi->state = WM_DS_STATE_ERROR;
    }
}

// The parent takes in the data of the AID for the set that is being filled (or the next one, in the double mode, if
// the first is not waiting for it): a null data is a gap
static void WmDataSharingSetData(WMDataSharingInfo *dsi, u16 aid, const void *data) {
    u16 bit = 1 << aid;
    u16 aidBitmap = dsi->aidBitmap;
    u16 index;
    u8 *dest;
    u32 irq;

    if (!(aidBitmap & bit)) {
        return;
    }
    index = dsi->writeIndex;
    if (!(dsi->ds[dsi->writeIndex].aidBitmap & bit)) {
        if (dsi->doubleMode != 1) {
            return;
        }
        index = (dsi->writeIndex + 1) & 3;
        if (!(dsi->ds[index].aidBitmap & bit)) {
            return;
        }
    }
    dest = WmGetSharedDataAddress(dsi, aidBitmap, dsi->ds[index].data, aid);
    if (data != NULL) {
        MI_CpuCopy16(data, dest, dsi->dataLength);
    } else {
        MI_CpuClear16(dest, dsi->dataLength);
    }
    irq = CPU_IRQDisable();
    dsi->ds[index].aidBitmap &= (u16)~bit;
    dsi->ds[index].receivedBitmap |= bit;
    CPU_SetIRQMask(irq);
}

// When the set that is being filled has the data of everyone it waited for, the parent sends it to all (without its
// own data if dropParent) and starts the next one
static void WmDataSharingSendDataSet(WMDataSharingInfo *dsi, int dropParent) {
    u32 irq = CPU_IRQDisable();

    if (dsi->ds[dsi->writeIndex].aidBitmap == 0) {
        u16 connectedBitmap = func_02080dfc();
        u16 index = dsi->writeIndex;
        u16 next = (index + 1) & 3;
        u16 fresh;
        int result;

        if (dsi->doubleMode == 1) {
            fresh = (next + 1) & 3;
        } else {
            fresh = next;
        }
        MI_CpuClear16(&dsi->ds[fresh], sizeof(WMDataSet));
        dsi->ds[fresh].aidBitmap = dsi->aidBitmap & (connectedBitmap | 1);
        dsi->writeIndex = next;
        dsi->ds[index].aidBitmap = dsi->aidBitmap;
        if (dropParent == 1) {
            dsi->ds[index].aidBitmap &= 0xfffe;
        }
        CPU_SetIRQMask(irq);
        result = func_02081998(WmDataSharingSenderCallback, dsi, &dsi->ds[index], dsi->dataSetLength,
                               connectedBitmap & dsi->aidBitmap, dsi->port, 1);
        if (result == WM_ERRCODE_NO_CHILD) {
            dsi->seqNum[index] = 0xffff;
            dsi->sendIndex = (dsi->sendIndex + 1) & 3;
        } else if (result != WM_ERRCODE_SUCCESS && result != WM_ERRCODE_OPERATING) {
            dsi->state = WM_DS_STATE_ERROR;
        }
    } else {
        CPU_SetIRQMask(irq);
    }
}

// WM_GetSharedDataAddress: where the data of the AID is in a set that WM_StepDataSharing gave, or NULL if it has none
void *func_020823ac(const WMDataSharingInfo *dsi, const WMDataSet *dataSet, u16 aid) {
    int bit = 1 << aid;
    u16 aidBitmap;
    u16 receivedBitmap;

    if (dsi == NULL) {
        return NULL;
    }
    if (dataSet == NULL) {
        return NULL;
    }
    aidBitmap = dataSet->aidBitmap;
    receivedBitmap = dataSet->receivedBitmap;
    if (!(aidBitmap & bit)) {
        return NULL;
    }
    if (receivedBitmap & bit) {
        return WmGetSharedDataAddress(dsi, aidBitmap, dataSet->data, aid);
    }
    return NULL;
}

// The data of an AID in a set: after the data of the AIDs below it that the set is for
static u8 *WmGetSharedDataAddress(const WMDataSharingInfo *dsi, u16 aidBitmap, const u16 *data, u16 aid) {
    return (u8 *)data + MATH_CountPopulation(aidBitmap & ((1 << aid) - 1)) * dsi->dataLength;
}
