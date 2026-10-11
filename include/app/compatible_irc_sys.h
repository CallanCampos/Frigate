#ifndef POKEBW2_APP_COMPATIBLE_IRC_SYS_H
#define POKEBW2_APP_COMPATIBLE_IRC_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "save/player_info.h"
#include "struct_decls.h"

// Overlay 175's compatible_irc_sys.c: the infrared link of the compatibility checker (the Link Compatibility check
// that two players run over the infrared port), which overlays 176 to 180's screens drive through it. The names are
// descriptive guesses

typedef struct CompatibleIrcSys CompatibleIrcSys;

// What a player sends the other: the date, the birthday, the lead Pokémon and the trainer
typedef struct {
    // The year, month, day and weekday, a byte each from the high byte
    u32 date;
    u8 birthMonth;
    u8 birthDay;
    u16 species;
    u8 form;
    u8 sex;
    u8 isEgg;
    u8 isFourteenth : 4;
    u8 unkB_4 : 4;
    PlayerInfo trainer;
} CompatibleIrcProfile;

// Creates the link, and frees it. A link made with disabled set does nothing and always reports it is done
CompatibleIrcSys *CompatibleIrcSys_Create(int unk40, GameData *gameData, HeapID heapId, BOOL disabled);
void CompatibleIrcSys_Free(CompatibleIrcSys *sys);

// Each of these runs a step and returns whether it is done, and also returns TRUE at once if the link is off
BOOL CompatibleIrcSys_StartNet(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_EndNet(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_StartNegotiation(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_Sync(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_IsConnected(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_ExchangeIds(CompatibleIrcSys *sys, u32 value);
void CompatibleIrcSys_ResetState(CompatibleIrcSys *sys);
void CompatibleIrcSys_ResetCounts(CompatibleIrcSys *sys);
void CompatibleIrcSys_SetCount(CompatibleIrcSys *sys, int count);
// Called each frame: now and then sends the other player the count
BOOL CompatibleIrcSys_Update(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_SendCount(CompatibleIrcSys *sys);
int CompatibleIrcSys_GetPendingCount(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_SendEx(CompatibleIrcSys *sys, u16 command, u32 size, const void *data, u8 sendId, u32 a5, BOOL a6,
                             BOOL noCopy);
void CompatibleIrcSys_AddCommands(CompatibleIrcSys *sys, int base, const void *list, int count, void *work);
void CompatibleIrcSys_RemoveCommands(CompatibleIrcSys *sys, int base);
BOOL CompatibleIrcSys_SendSelection(CompatibleIrcSys *sys, u32 a, u32 b);
BOOL CompatibleIrcSys_PollSelection(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_SendPing(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_PollPing(CompatibleIrcSys *sys);
BOOL CompatibleIrcSys_ExchangeProfiles(CompatibleIrcSys *sys, GameSystem *gsys);
void CompatibleIrcSys_GetPeerProfile(CompatibleIrcSys *sys, CompatibleIrcProfile *dest);
// Fills a profile from the player's save
void CompatibleIrcSys_BuildProfile(GameSystem *gsys, CompatibleIrcProfile *profile);

#endif // POKEBW2_APP_COMPATIBLE_IRC_SYS_H
