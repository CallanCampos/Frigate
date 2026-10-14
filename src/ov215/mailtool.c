#include "types.h"
#include "field/mailtool.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "system/game_data.h"

// The mail screens' params (overlay 215's mailtool.c): what the field app call allocates, and reads the result of

MailParam *MailTool_CreateWriteParam(GameData *gameData, u32 a1, u8 partySlot, u32 mailId, u32 heapId) {
    void *mailBox = func_02009790(gameData);
    MailParam *param = GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), sizeof(MailParam), FALSE, "mailtool.c", 67);

    sys_memset32_fast(0, param, sizeof(MailParam));
    param->design = mailId;
    param->partySlot = partySlot;
    param->mailBox = mailBox;
    param->mode = MAIL_PARAM_MODE_WRITE;
    param->unk08 = a1;
    param->unk0C = 0;
    param->save = GameData_GetSaveControl(gameData);
    param->mail = CreateMailData(heapId);
    ResetMailData(param->mail);
    func_020096f8(param->mail, 0xff, partySlot, gameData);
    return param;
}

MailParam *MailTool_CreatePokemonViewParam(GameData *gameData, PartyPkm *pkm, u32 heapId) {
    MailParam *param = GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), sizeof(MailParam), FALSE, "mailtool.c", 151);

    sys_memset32_fast(0, param, sizeof(MailParam));
    param->mode = MAIL_PARAM_MODE_VIEW;
    param->save = GameData_GetSaveControl(gameData);
    param->mail = CreateMailData(heapId);
    PokeParty_GetParam(pkm, PKM_PARAM_MAIL, param->mail);
    return param;
}

MailParam *MailTool_CreateItemViewParam(GameData *gameData, u32 mailId, u32 heapId) {
    MailParam *param = GFL_HeapAllocate(HEAPID_TAIL((HeapID)heapId), sizeof(MailParam), FALSE, "mailtool.c", 185);

    sys_memset32_fast(0, param, sizeof(MailParam));
    param->mode = MAIL_PARAM_MODE_VIEW;
    param->save = GameData_GetSaveControl(gameData);
    param->mail = CreateMailData(heapId);
    func_02009754(param->mail, mailId);
    return param;
}

BOOL MailTool_IsWritten(const MailParam *param) {
    return param->written;
}

BOOL MailTool_GiveMail(MailParam *param, PartyPkm *pkm) {
    if (!MailTool_IsWritten(param)) {
        return FALSE;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (u32)param->mail);
    return TRUE;
}

void MailTool_FreeParam(MailParam *param) {
    if (param->mail != NULL) {
        GFL_HeapFree(param->mail);
    }
    GFL_HeapFree(param);
}
