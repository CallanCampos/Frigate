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

void ResetMailData(MailData *mail) {
    int i;

    mail->trainerId = 0;
    mail->trainerGender = 0;
    mail->region = region;
    mail->gameVersion = game_version;
    mail->design = 0xff;
    MIi_CpuClear16(GFL_StrBufGetTerminator(), mail->trainerName, sizeof(mail->trainerName));
    mail->stampWord = 0xffff;
    for (i = 0; i < 3; i++) {
        PMSData_Clear(&mail->message[i]);
    }
}

BOOL MailData_IsValid(const MailData *mail) {
    if (mail->design > MAIL_DESIGN_MAX) {
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
    MI_CpuCopy8(src, dest, sizeof(MailData));
}

void MailData_InitForPlayer(MailData *mail, u32 design, u32 unused, GameData *gameData) {
    PlayerInfo *info;

    ResetMailData(mail);
    mail->design = design;
    info = GetGameDataPlayerInfo(gameData);
    MI_CpuCopy8(GetPlayerName(info), mail->trainerName, sizeof(mail->trainerName));
    mail->trainerGender = getTrainerGender(info);
    mail->trainerId = getIDAsUInt(info);
}

u32 MailData_GetTrainerID(const MailData *mail) {
    return mail->trainerId;
}

u16 *MailData_GetTrainerName(MailData *mail) {
    return mail->trainerName;
}

void MailData_SetTrainerName(MailData *mail, const u16 *name) {
    MI_CpuCopy8(name, mail->trainerName, sizeof(mail->trainerName));
}

u8 MailData_GetTrainerGender(const MailData *mail) {
    return mail->trainerGender;
}

u8 MailData_GetDesign(const MailData *mail) {
    return mail->design;
}

void MailData_SetDesign(MailData *mail, u32 design) {
    if (design < MAIL_DESIGN_COUNT) {
        mail->design = design;
    }
}

u8 MailData_GetRegion(const MailData *mail) {
    return mail->region;
}

u8 MailData_GetGameVersion(const MailData *mail) {
    return mail->gameVersion;
}

u16 MailData_GetStampWord(const MailData *mail) {
    return mail->stampWord;
}

void MailData_SetStampWord(MailData *mail, u16 value) {
    mail->stampWord = value;
}

PMSData *MailData_GetMessage(MailData *mail, u8 index) {
    if (index < 3) {
        return &mail->message[index];
    }
    return &mail->message[0];
}

void MailData_SetMessage(MailData *mail, const PMSData *src, u8 index) {
    if (index < 3) {
        PMSData_Copy(&mail->message[index], src);
    }
}

void *MailBox_GetFromGameData(GameData *gameData) {
    return SaveControl_GetBlockPtr(GameData_GetSaveControl(gameData), SAVE_BLOCK_MAIL);
}

u32 MailBox_GetSize(void) {
    return MAIL_COUNT * sizeof(MailData);
}

void MailBox_Clear(MailData *mails) {
    int i;

    for (i = 0; i < MAIL_COUNT; i++) {
        ResetMailData(&mails[i]);
    }
}

s32 MailBox_GetFreeSlot(MailData *mails, u32 box) {
    return MailBox_FindFree(mails, MAIL_COUNT);
}

void MailBox_ResetMail(MailData *mails, u32 box, u32 index) {
    MailData *mail = MailBox_GetMail(mails, box, index);

    if (mail != NULL) {
        ResetMailData(mail);
    }
}

void MailBox_SetMail(MailData *mails, u32 box, u32 index, const MailData *src) {
    MailData *mail = MailBox_GetMail(mails, box, index);

    if (mail != NULL) {
        copyMailMsg(src, mail);
    }
}

MailData *MailBox_CopyMail(MailData *mails, u32 box, u32 index, HeapID heapId) {
    MailData *mail = MailBox_GetMail(mails, box, index);
    MailData *copy = CreateMailData(heapId);

    if (mail != NULL) {
        copyMailMsg(mail, copy);
    }
    return copy;
}

s32 MailBox_FindFree(MailData *mails, s32 count) {
    s32 i;

    for (i = 0; i < count; i++) {
        if (!MailData_IsValid(&mails[i])) {
            return i;
        }
    }
    return -1;
}

MailData *MailBox_GetMail(MailData *mails, u32 box, s32 index) {
    if (index < MAIL_COUNT) {
        return &mails[index];
    }
    return NULL;
}
