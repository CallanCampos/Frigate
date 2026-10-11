#include "gfl/dpw_profile.h"
#include "types.h"
#include "constants/language.h"
#include "constants/version.h"
#include "field/unity_tower.h"
#include "gfl/std.h"
#include "save/player_info.h"
#include "system/str_tool.h"

void DpwProfile_Fill(Dpw_Common_Profile *profile, PlayerInfo *playerInfo) {
    sys_memset(profile, 0, sizeof(Dpw_Common_Profile));
    profile->version = GAME_VERSION;
    profile->language = GAME_LANGUAGE;
    profile->country = UnityTowerVisitor_GetCountry(playerInfo);
    profile->region = UnityTowerVisitor_GetProvince(playerInfo);
    profile->playerId = getIDAsUInt(playerInfo);
    wcharsncpy(GetPlayerName(playerInfo), profile->playerName, 8);
    profile->unk18 = 0;
    profile->mailAddr[0] = '\0';
    profile->unk5C = 0;
}
