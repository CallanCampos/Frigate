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
    void *saveBlock;
    PlayerInfo *playerInfo;
    Field *field;
    BeaconStatus *unk18;
    u16 *result;
    NameEntryParam nameEntry;
    u16 heapId;
    u16 unk5A;
    u32 mode;
};

GameEvent *EventPhraseInput_Create(GameSystem *gsys, Field *field, GameEvent *parent, u32 mode, u16 *result);
GameEventReturnCode EventPhraseInput_Callback(GameEvent *event, u32 *state, void *data);
void func_ov033_02177734(struct EventPhraseInputData *data, NameEntryParam *nameEntryParams);
// Script commands
BOOL func_ov033_021777dc(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_02177844(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_02177908(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_EVENT_PHRASE_INPUT_H
