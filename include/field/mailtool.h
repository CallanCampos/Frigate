#ifndef POKEBW2_FIELD_MAILTOOL_H
#define POKEBW2_FIELD_MAILTOOL_H

#include "types.h"
#include "gfl/heap.h"
#include "pml/mail.h"
#include "save/save_control.h"
#include "struct_decls.h"

// The mail screen's callers' interface (overlay 215's mailtool.c): the params that the field app call allocates for the
// mail screens, and reads the result of. The names are ours, guessed

#define MAIL_PARAM_MODE_VIEW 0
#define MAIL_PARAM_MODE_WRITE 1

// The parameter of the mail screen (mailsys.c's MailSys_ProcInit and the others): the mail to show or to write
typedef struct MailParam {
    u16 mode;
    // The mail was written and is handed over to its owner
    BOOL written;
    // The caller's own value, always 2 here
    u32 unk08;
    u16 unk0C;
    u8 partySlot;
    // The mail's design, which is the mail item's mail ID
    u8 design;
    SaveControl *save;
    MailData *mail;
    // The save's block of mail
    void *mailBox;
} MailParam;

// Writes a new mail of the given design, to give to the party Pokémon in partySlot
MailParam *MailTool_CreateWriteParam(GameData *gameData, u32 a1, u8 partySlot, u32 mailId, u32 heapId);
// Shows the mail that a party Pokémon holds, or a mail item's blank design
MailParam *MailTool_CreatePokemonViewParam(GameData *gameData, PartyPkm *pkm, u32 heapId);
MailParam *MailTool_CreateItemViewParam(GameData *gameData, u32 mailId, u32 heapId);
// Whether the mail was written
BOOL MailTool_IsWritten(const MailParam *param);
// Gives the written mail to the Pokémon, and returns whether there was one
BOOL MailTool_GiveMail(MailParam *param, PartyPkm *pkm);
void MailTool_FreeParam(MailParam *param);

#endif // POKEBW2_FIELD_MAILTOOL_H
