// The party's Reshiram (Black 2) or Zekrom (White 2), for scrcmd_pokemon_misc.c's command that swaps it in. The file
// names itself in its heap allocation. GetOTZekromPartyIdx is swan's name (https://github.com/ds-pokemon-hacking/swan,
// GPL-3.0)
#include "types.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "field/field_script.h"
#include "field/scrcmd_pokemon_fld.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "system/game_data.h"

// The dragon of the version, which the player catches
#ifdef BLACK2
#define VERSION_DRAGON SPECIES_RESHIRAM
#else
#define VERSION_DRAGON SPECIES_ZEKROM
#endif

static BOOL func_ov036_021afb74(GameData *gameData, u16 species, u16 *boxOut, u16 *slotOut);

// Swaps the party Pokémon at partyIdx with the player's own dragon of the version from a box, and takes the mail off
// the one that holds it
void func_ov036_021afa2c(FieldScriptEnv *env, u32 partyIdx) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokeParty *party = GameData_GetParty(gameData);
    u16 box, slot;

    if (func_ov036_021afb74(gameData, VERSION_DRAGON, &box, &slot)) {
        HeapID heapId = FieldScriptEnv_GetHeapID(env);
        BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(gameData);
        PartyPkm *pkm = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), TRUE, "scrcmd_pokemon_fld.c", 1179);
        PartyPkm *boxPkm;
        u32 size = PokeParty_GetPkmRawSize();

        MI_CpuCopy8(PokeParty_GetPkm(party, partyIdx), pkm, size);
        if (PML_ItemIsMail(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL)) == TRUE) {
            MailData *mails = MailBox_GetFromGameData(gameData);
            MailData *mail = CreateMailData(FieldScriptEnv_GetHeapID(env));
            s32 index = MailBox_GetFreeSlot(mails, 0);

            PokeParty_GetParam(pkm, PKM_PARAM_MAIL, mail);
            GetGameDataPlayerInfo(gameData);
            if (index >= 0) {
                MailBox_SetMail(mails, 0, index, mail);
            }
            ResetMailData(mail);
            PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (u32)mail);
            PokeParty_SetParam(pkm, PKM_PARAM_ITEM, 0);
            GFL_HeapFree(mail);
        }
        if (PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL) == SPECIES_SHAYMIN) {
            PokeParty_ChangeForme(pkm, 0);
            PokeParty_RecalcStats(pkm);
        }
        boxPkm = boxPkmRegenToPartyPkm(BoxSaveAccessor_GetPkm(boxes, box, slot), heapId);
        copyPkmIntoPartyBlk(party, partyIdx, boxPkm);
        GFL_HeapFree(boxPkm);
        BoxSaveAccessor_SetPkm(boxes, box, slot, PokeParty_GetBoxPkm(pkm));
        GFL_HeapFree(pkm);
    }
}

// Finds the box slot of a Pokémon of the species that the player caught
static BOOL func_ov036_021afb74(GameData *gameData, u16 species, u16 *boxOut, u16 *slotOut) {
    PlayerInfo *player;
    BoxSaveAccessor *boxes;
    s32 lastBox;
    s32 i;
    u32 id;

    GameData_GetParty(gameData);
    player = GetGameDataPlayerInfo(gameData);
    boxes = GameData_GetBoxSaveAccessor(gameData);
    lastBox = BoxSaveAccessor_GetLastOpenedBox(boxes);
    id = getIDAsUInt(player);
    for (i = 0; i < BOX_MAX_TRAY; i++) {
        s32 box = (i + lastBox) % BOX_MAX_TRAY;
        s32 slot;

        for (slot = 0; slot < BOX_MAX_POS; slot++) {
            if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                if (id == BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_ID, NULL)) {
                    if (species == BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_SPECIES, NULL)) {
                        if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_IS_EGG, NULL) == 0) {
                            *boxOut = box;
                            *slotOut = slot;
                            return TRUE;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

void func_ov036_021afc28(FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    PokeParty *party = GameData_GetParty(gameData);
    int idx = GetOTZekromPartyIdx(gameData);

    if (idx >= 0) {
        PokeParty_SwapPkms(party, idx, 0, FieldScriptEnv_GetHeapID(env));
    }
}

int GetOTZekromPartyIdx(GameData *gameData) {
    PokeParty *party = GameData_GetParty(gameData);
    PlayerInfo *player = GetGameDataPlayerInfo(gameData);
    int count = PokeParty_GetPkmCount(party);
    u32 id = getIDAsUInt(player);
    int i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (id == PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL) &&
            PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == VERSION_DRAGON &&
            PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            return i;
        }
    }
    return -1;
}
