#ifndef POKEBW2_FIELD_EVENT_PHRASE_INPUT_H
#define POKEBW2_FIELD_EVENT_PHRASE_INPUT_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "app/name_entry.h"
#include "gfl/str.h"
#include "system/game_event.h"

struct EventPhraseInputData {
    GameSystem *gsys;
    GameData *gameData;
    TrainerGameInfoSave *trainerInfo;
    u16 *saveBlock;
    PlayerInfo *playerInfo;
    Field *field;
    BeaconStatus *unk18;
    u16 *result;
    NameEntryParam nameEntry;
    u16 heapId;
    u16 unk5A;
    u32 mode;
};

GameEvent *EventPhraseInput_Create(GameSystem *gsys, Field *field, GameEvent *parent, unsigned int mode, u16 *result);
GameEventReturnCode EventPhraseInput_Callback(GameEvent *event, u32 *state, void *data);
void EventPhraseInput_StoreResult(struct EventPhraseInputData *data, NameEntryParam *nameEntryParams);
// Script commands
BOOL Scrcmd_PhraseInput_CheckWifiListFlags(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PhraseInput_WifiListOperation(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PhraseInput_SetWifiFriendNameWord(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_EVENT_PHRASE_INPUT_H
