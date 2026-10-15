#include "types.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/mail.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/pms_data.h"
#include "system/version.h"

// The mail: a mailbox of MAIL_COUNT mails, and the mail of one Pokémon. The ROM's string for the file is "mail.c".

#define SAVE_BLOCK_MAIL 0x28

void ResetMailData(MailData *mail) {
    int i;

    mail->trainerId = 0;
    mail->trainerGender = 0;
    mail->region = region;
    mail->gameVersion = game_version;
    mail->unk07 = 0xff;
    sys_memset16(GFL_StrBufGetTerminator(), mail->trainerName, sizeof(mail->trainerName));
    mail->unk1E = 0xffff;
    for (i = 0; i < 3; i++) {
        PMSData_Clear(&mail->message[i]);
    }
}

BOOL func_020096a8(const MailData *mail) {
    if (mail->unk07 > MAIL_DESIGN_MAX) {
        return FALSE;
    }
    return TRUE;
}

MailData *CreateMailData(HeapID heapId) {
    MailData *mail = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(MailData), FALSE, "mail.c", 135);
    ResetMailData(mail);
    return mail;
}

void copyMailMsg(const MailData *src, MailData *dest) {
    sys_memcpy(src, dest, sizeof(MailData));
}

void func_020096f8(MailData *mail, u32 design, u32 unused, GameData *gameData) {
    PlayerInfo *info;

    ResetMailData(mail);
    mail->unk07 = design;
    info = GetGameDataPlayerInfo(gameData);
    sys_memcpy(GetPlayerName(info), mail->trainerName, sizeof(mail->trainerName));
    mail->trainerGender = getTrainerGender(info);
    mail->trainerId = getIDAsUInt(info);
}

u32 func_02009730(const MailData *mail) {
    return mail->trainerId;
}

u16 *func_02009734(MailData *mail) {
    return mail->trainerName;
}

void func_02009738(MailData *mail, const u16 *name) {
    sys_memcpy(name, mail->trainerName, sizeof(mail->trainerName));
}

u8 func_0200974c(const MailData *mail) {
    return mail->trainerGender;
}

u8 func_02009750(const MailData *mail) {
    return mail->unk07;
}

void func_02009754(MailData *mail, u32 design) {
    if (design < MAIL_DESIGN_COUNT) {
        mail->unk07 = design;
    }
}

u8 func_0200975c(const MailData *mail) {
    return mail->region;
}

u8 func_02009760(const MailData *mail) {
    return mail->gameVersion;
}

u16 func_02009764(const MailData *mail) {
    return mail->unk1E;
}

void func_02009768(MailData *mail, u16 value) {
    mail->unk1E = value;
}

PMSData *func_0200976c(MailData *mail, u8 index) {
    if (index < 3) {
        return &mail->message[index];
    }
    return &mail->message[0];
}

void func_0200977c(MailData *mail, const PMSData *src, u8 index) {
    if (index < 3) {
        PMSData_Copy(&mail->message[index], src);
    }
}

void *func_02009790(GameData *gameData) {
    return SaveControl_GetBlockPtr(GameData_GetSaveControl(gameData), SAVE_BLOCK_MAIL);
}

u32 func_020097a0(void) {
    return MAIL_COUNT * sizeof(MailData);
}

void func_020097a8(MailData *mails) {
    int i;

    for (i = 0; i < MAIL_COUNT; i++) {
        ResetMailData(&mails[i]);
    }
}

s32 func_020097c4(MailData *mails, u32 box) {
    return func_02009818(mails, MAIL_COUNT);
}

void func_020097d0(MailData *mails, u32 box, u32 index) {
    MailData *mail = func_02009844(mails, box, index);

    if (mail != NULL) {
        ResetMailData(mail);
    }
}

void func_020097e0(MailData *mails, u32 box, u32 index, const MailData *src) {
    MailData *mail = func_02009844(mails, box, index);

    if (mail != NULL) {
        copyMailMsg(src, mail);
    }
}

MailData *func_020097f4(MailData *mails, u32 box, u32 index, HeapID heapId) {
    MailData *mail = func_02009844(mails, box, index);
    MailData *copy = CreateMailData(heapId);

    if (mail != NULL) {
        copyMailMsg(mail, copy);
    }
    return copy;
}

s32 func_02009818(MailData *mails, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        if (!func_020096a8(&mails[i])) {
            return i;
        }
    }
    return -1;
}

MailData *func_02009844(MailData *mails, u32 box, s32 index) {
    if (index < MAIL_COUNT) {
        return &mails[index];
    }
    return NULL;
}
