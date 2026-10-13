#ifndef POKEBW2_NITRO_CARD_H
#define POKEBW2_NITRO_CARD_H

#include "types.h"

// NitroSDK's access to the card's backup memory. The functions are unnamed: func_0206ef4c is CARD_LockBackup,
// func_0206ef58 CARD_UnlockBackup, func_0206eecc CARD_GetResultCode, func_0206f80c CARDi_RequestStreamCommand,
// func_0206f854 CARD_IdentifyBackup, func_0206f888 CARD_WaitBackupAsync, func_0206f890 CARD_TryWaitBackupAsync and
// func_0206f898 CARD_CancelBackupAsync

typedef enum {
    CARD_RESULT_SUCCESS,
    CARD_RESULT_FAILURE,
    CARD_RESULT_INVALID_PARAM,
    CARD_RESULT_UNSUPPORTED,
    CARD_RESULT_TIMEOUT,
    CARD_RESULT_ERROR,
    CARD_RESULT_NO_RESPONSE,
    CARD_RESULT_CANCELED,
} CARDResult;

typedef enum {
    CARD_REQ_READ_BACKUP = 6,
    CARD_REQ_WRITE_BACKUP = 7,
} CARDRequest;

typedef enum {
    CARD_REQUEST_MODE_RECV,
    CARD_REQUEST_MODE_SEND,
    CARD_REQUEST_MODE_SEND_VERIFY,
} CARDRequestMode;

#define CARD_RETRY_COUNT_MAX 10

// A backup type: the device, the log2 of its size and the vendor
typedef u32 CARDBackupType;
#define CARD_BACKUP_TYPE_DEVICE_FLASH 2
#define CARD_BACKUP_TYPE_DEFINE(device, sizeShift, vendor) ((device) | ((sizeShift) << 8) | ((vendor) << 16))
#define CARD_BACKUP_TYPE_FLASH_4MBITS_EX CARD_BACKUP_TYPE_DEFINE(CARD_BACKUP_TYPE_DEVICE_FLASH, 19, 0xff)

typedef void (*MIDmaCallback)(void *arg);

void func_0206ef4c(u16 lockId);
void func_0206ef58(u16 lockId);
CARDResult func_0206eecc(void);
BOOL func_0206f80c(u32 src, u32 dest, u32 size, MIDmaCallback callback, void *arg, BOOL async, CARDRequest request,
    int retries, CARDRequestMode mode);
BOOL func_0206f854(CARDBackupType type);
BOOL func_0206f888(void);
// Whether no access to the backup is in progress
BOOL func_0206f890(void);
void func_0206f898(void);

// Called when the card is pulled out; returns whether to stop the system
typedef BOOL (*CARDPulledOutCallback)(void);

void func_0206ff50(CARDPulledOutCallback callback); // CARD_SetPulledOutCallback
void func_0206f0f0(u32 command, u32 value);
void func_0206fed8(void);
BOOL func_0206ff5c(void);
void func_0206ffac(void);
void func_0206ffb0(void);
void func_0206ffbc(void);
void func_0206ffc0(void);
void func_0206ffcc(u32 cardId);

static inline BOOL CARD_ReadBackup(u32 src, void *dest, u32 size) {
    return func_0206f80c(src, (u32)dest, size, NULL, NULL, FALSE, CARD_REQ_READ_BACKUP, 1, CARD_REQUEST_MODE_RECV);
}

static inline BOOL CARD_ReadBackupAsync(u32 src, void *dest, u32 size, MIDmaCallback callback, void *arg) {
    return func_0206f80c(src, (u32)dest, size, callback, arg, TRUE, CARD_REQ_READ_BACKUP, 1, CARD_REQUEST_MODE_RECV);
}

static inline BOOL CARD_WriteAndVerifyBackupAsync(u32 dest, const void *src, u32 size, MIDmaCallback callback,
    void *arg) {
    return func_0206f80c((u32)src, dest, size, callback, arg, TRUE, CARD_REQ_WRITE_BACKUP, CARD_RETRY_COUNT_MAX,
        CARD_REQUEST_MODE_SEND_VERIFY);
}

#endif // POKEBW2_NITRO_CARD_H
