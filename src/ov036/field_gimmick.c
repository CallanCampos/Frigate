// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "field/badge_gate.h"
#include "field/field.h"
#include "field/field_gimmick_bsubway.h"
#include "field/field_gimmick_d06.h"
#include "field/field_gimmick_gate.h"
#include "field/gimmick_aspertia_city.h"
#include "field/gimmick_dragonspiral_tower_9.h"
#include "field/gimmick_funfest_levels.h"
#include "field/gimmick_giant_chasm_6.h"
#include "field/gimmick_humilau.h"
#include "field/gimmick_league_caitlin.h"
#include "field/gimmick_league_center.h"
#include "field/gimmick_league_champion.h"
#include "field/gimmick_league_grimsley.h"
#include "field/gimmick_league_lift.h"
#include "field/gimmick_league_marshal.h"
#include "field/gimmick_league_shauntal.h"
#include "field/gimmick_league_statue.h"
#include "field/gimmick_marine_tube.h"
#include "field/gimmick_nuvema.h"
#include "field/gimmick_passing_vehicles.h"
#include "field/gimmick_plasma_frigate.h"
#include "field/gimmick_plasma_frigate_deck.h"
#include "field/gimmick_plasma_frigate_lights.h"
#include "field/gimmick_pokewood_poster.h"
#include "field/gimmick_roller_coaster.h"
#include "field/gimmick_skyarrow_bridge.h"
#include "field/gimmick_tubeline_bridge.h"
#include "field/gimmick_underground_ruins.h"
#include "field/gym_driftveil.h"
#include "field/gym_driftveil_lift.h"
#include "field/gym_gimmick.h"
#include "field/gym_humilau.h"
#include "field/gym_icirrus.h"
#include "field/gym_mistralton.h"
#include "field/gym_nacrene.h"
#include "field/gym_opelucid.h"
#include "field/gym_striaton.h"
#include "field/gym_virbank.h"
#include "field/infinity_dungeon_gimmick.h"
#include "field/resort.h"
#include "field/sp_poke_gimmick.h"
#include "field/wbt_stadium_gimmick.h"
#include "gfl/overlay.h"
#include "save/gimmick_savedata.h"
#include "system/game_data.h"
#include "system/game_system.h"

typedef struct GimmickInfo {
    u32 overlayId;
    void (*init)(Field *field);
    void (*end)(Field *field);
    void (*update)(Field *field);
} GimmickInfo;

// What each gimmick runs, and the overlay it needs loaded, by gimmick ID
static const GimmickInfo GIMMICK_OVERLAY_INFO[59] = {
    { 0, NULL, NULL, NULL },
    { 0x6d, SkyarrowBridge_Setup, SkyarrowBridge_End, SkyarrowBridge_Update },
    { 0x5c, GymElec_Init, GymElec_End, GymElec_Update },
    { 0x5d, GymNacrene_Init, GymNacrene_End, GymNacrene_Update },
    { 0x5f, GymStriaton_Init, GymStriaton_End, GymStriaton_Update },
    { 0x5e, GymMistralton_Init, GymMistralton_End, GymMistralton_Update },
    { 0x5b, GymInsect_Init, GymInsect_End, GymInsect_Update },
    { 0x60, DriftveilGym_Init, DriftveilGym_End, DriftveilGym_Update },
    { 0x61, DriftveilLift_Init, DriftveilLift_End, DriftveilLift_Update },
    { 0x62, GymIcirrus_Init, GymIcirrus_End, GymIcirrus_Update },
    { 0x63, GymOpelucid_Init, GymOpelucid_End, GymOpelucid_Update },
    { 0x6e, TubelineBridge_Setup, TubelineBridge_End, TubelineBridge_Update },
    { 0x68, GimmickGate_Setup, GimmickGate_Delete, GimmickGate_Update },
    { 0x6b, GimmickD06_Setup, GimmickD06_End, GimmickD06_Update },
    { 0x69, LeagueStatue_Init, LeagueStatue_End, LeagueStatue_Update },
    { 0x6a, GimmickLeagueLift_Init, GimmickLeagueLift_End, GimmickLeagueLift_Update },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x80, PassingVehicles_Init, PassingVehicles_End, PassingVehicles_Update },
    { 0x81, SpPokeGimmickA_Init, SpPokeGimmickA_End, SpPokeGimmickA_Update },
    { 0x81, SpPokeGimmickC_Init, SpPokeGimmickC_End, SpPokeGimmickC_Update },
    { 0x81, SpPokeGimmickB_Init, SpPokeGimmickB_End, SpPokeGimmickB_Update },
    { 0x82, GimmickFunfestLevels_Setup, GimmickFunfestLevels_End, GimmickFunfestLevels_Update },
    { 0, NULL, NULL, NULL },
    { 0x6c, FieldGimmickBSubway_Setup, FieldGimmickBSubway_End, FieldGimmickBSubway_Move },
    { 0x85, GimmickNuvema_Init, GimmickNuvema_End, GimmickNuvema_Update },
    { 0x89, ResortField_Setup, ResortField_Delete, ResortField_Update },
    { 0x89, ResortField0_Setup, ResortField0_Delete, ResortField0_Update },
    { 0x7f, InfinityDungeonGimmick_Setup, InfinityDungeonGimmick_Delete, InfinityDungeonGimmick_Update },
    { 0x86, WbtStadiumGimmick_Setup, WbtStadiumGimmick_End, WbtStadiumGimmick_Update },
    { 0x64, GymVirbank_Init, GymVirbank_End, GymVirbank_Update },
    { 0x65, GymHumilau_Init, GymHumilau_End, GymHumilau_Update },
    { 0x7a, GimmickShauntal_Setup, GimmickShauntal_End, GimmickShauntal_Update },
    { 0x7b, GimmickGrimsley_Setup, GimmickGrimsley_End, GimmickGrimsley_Update },
    { 0x7c, MarshalRoom_Init, MarshalRoom_End, MarshalRoom_Update },
    { 0x7d, GimmickCaitlin_Setup, GimmickCaitlin_End, GimmickCaitlin_Update },
    { 0x75, GimmickPlasmaFrigate_Setup, GimmickPlasmaFrigate_End, GimmickPlasmaFrigate_Update },
    { 0x67, BadgeGate_Setup, BadgeGate_End, BadgeGate_Update },
    { 0x76, GimmickPlasmaFrigateDeck_Setup, GimmickPlasmaFrigateDeck_End, GimmickPlasmaFrigateDeck_Update },
    { 0x78, GimmickChampion_Setup, GimmickChampion_End, GimmickChampion_Update },
    { 0x77, PlasmaFrigateLights_Init, PlasmaFrigateLights_End, PlasmaFrigateLights_Update },
    { 0x66, RollerCoaster_Init, RollerCoaster_End, RollerCoaster_Update },
    { 0x88, GimmickPokewoodPoster_Setup, GimmickPokewoodPoster_End, GimmickPokewoodPoster_Update },
    { 0x6f, MarineTube_Init, MarineTube_End, MarineTube_Update },
    { 0x87, NULL, NULL, NULL },
    { 0x74, GimmickGC6_Setup, GimmickGC6_End, GimmickGC6_Update },
    { 0x70, AspertiaCity_Init, AspertiaCity_End, AspertiaCity_Update },
    { 0x71, GimmickHumilau_Init, GimmickHumilau_End, GimmickHumilau_Update },
    { 0x72, UndergroundRuins_Init, UndergroundRuins_End, UndergroundRuins_Update },
    { 0x73, LeagueCenter_Init, LeagueCenter_End, LeagueCenter_Update },
    { 0x81, SpPokeGimmickD_Init, SpPokeGimmickD_End, SpPokeGimmickD_Update },
    { 0x84, GimmickDT9_Setup, GimmickDT9_End, GimmickDT9_Update },
};

void Field_InitGimmick(Field *field) {
    u32 id = GimmickState_GetID(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))));

    if (id != -1) {
        if (GIMMICK_OVERLAY_INFO[id].overlayId != 0) {
            GFL_OvlLoad(GIMMICK_OVERLAY_INFO[id].overlayId);
        }
        if (GIMMICK_OVERLAY_INFO[id].init != NULL) {
            GIMMICK_OVERLAY_INFO[id].init(field);
        }
    }
}

void Field_TerminateGimmick(Field *field) {
    u32 id = GimmickState_GetID(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))));

    if (id != -1) {
        if (GIMMICK_OVERLAY_INFO[id].end != NULL) {
            GIMMICK_OVERLAY_INFO[id].end(field);
        }
        if (GIMMICK_OVERLAY_INFO[id].overlayId != 0) {
            GFL_OvlUnload(GIMMICK_OVERLAY_INFO[id].overlayId);
        }
    }
}

void Field_UpdateGimmick(Field *field) {
    u32 id = GimmickState_GetID(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))));

    if (id != -1 && GIMMICK_OVERLAY_INFO[id].update != NULL) {
        GIMMICK_OVERLAY_INFO[id].update(field);
    }
}

BOOL Field_CheckGimmickID(Field *field, u32 id) {
    if (id == GimmickState_GetID(GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field))))) {
        return TRUE;
    }
    return FALSE;
}
