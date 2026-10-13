#include "battle/btl_tables.h"
#include "types.h"
#include "constants/abilities.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/species.h"

static BOOL func_ov169_0689ca10(u16 value, const u16 *list, u32 count);
static BOOL IsUncallableMove(u16 move);

// Searches a list of ids
static BOOL func_ov169_0689ca10(u16 value, const u16 *list, u32 count) {
    u32 i;

    for (i = 0; i < count; i++) {
        if (value == list[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static const u16 sEncoreExcludedMoves[] = { MOVE_NONE,      MOVE_ENCORE, MOVE_MIRROR_MOVE,
                                            MOVE_TRANSFORM, MOVE_MIMIC,  MOVE_SKETCH };

BOOL IsEncoreExcludedMove(u16 move) {
    return func_ov169_0689ca10(move, sEncoreExcludedMoves, 6);
}

static const u16 sMeFirstExcludedMoves[] = { MOVE_NONE,        MOVE_MIRROR_COAT, MOVE_COUNTER, MOVE_METAL_BURST,
                                             MOVE_FOCUS_PUNCH, MOVE_ME_FIRST,    MOVE_THIEF,   MOVE_COVET,
                                             MOVE_CHATTER,     MOVE_STRUGGLE };

BOOL IsMeFirstExcludedMove(u16 move) {
    return func_ov169_0689ca10(move, sMeFirstExcludedMoves, 10);
}

static const u16 sDelayedAttackMoves[] = { MOVE_NONE, MOVE_FUTURE_SIGHT, MOVE_DOOM_DESIRE };

BOOL IsDelayedAttackMove(u16 move) {
    return func_ov169_0689ca10(move, sDelayedAttackMoves, 3);
}

static const u16 sPressureFieldMoves[] = { MOVE_SNATCH, MOVE_IMPRISON, MOVE_SPIKES, MOVE_TOXIC_SPIKES,
                                           MOVE_STEALTH_ROCK };

BOOL IsPressureFieldMove(u16 move) {
    return func_ov169_0689ca10(move, sPressureFieldMoves, 5);
}

static const u16 sPledgeMoves[] = { MOVE_FIRE_PLEDGE, MOVE_GRASS_PLEDGE, MOVE_WATER_PLEDGE };

BOOL IsPledgeMove(u16 move) {
    return func_ov169_0689ca10(move, sPledgeMoves, 3);
}

static const u16 sReactiveHeldItems[] = { ITEM_FOCUS_BAND,   ITEM_POWER_HERB,  ITEM_EJECT_BUTTON,
                                          ITEM_JABOCA_BERRY, ITEM_ROWAP_BERRY, ITEM_CUSTAP_BERRY,
                                          ITEM_QUICK_CLAW,   ITEM_ABSORB_BULB, ITEM_CELL_BATTERY };

BOOL func_ov169_0689ca84(u16 item) {
    return func_ov169_0689ca10(item, sReactiveHeldItems, 9);
}

static const u16 sEscapeItems[] = { ITEM_FLUFFY_TAIL, ITEM_POKE_DOLL, ITEM_POKE_TOY };

BOOL IsEscapeItem(u16 item) {
    return func_ov169_0689ca10(item, sEscapeItems, 3);
}

static const u16 sRolePlayExcludedAbilities[] = { ABILITY_TRACE,       ABILITY_FORECAST,    ABILITY_MULTITYPE,
                                                  ABILITY_FLOWER_GIFT, ABILITY_ZEN_MODE,    ABILITY_ILLUSION,
                                                  ABILITY_IMPOSTER,    ABILITY_WONDER_GUARD };

BOOL IsRolePlayExcludedAbility(u16 ability) {
    if (IsAbilityMultitype(ability)) {
        return TRUE;
    }
    return func_ov169_0689ca10(ability, sRolePlayExcludedAbilities, 8);
}

BOOL func_ov169_0689cac4(u16 ability) {
    return func_ov169_0689cacc(ability);
}

BOOL func_ov169_0689cacc(u16 ability) {
    if (ability == ABILITY_WONDER_GUARD) {
        return FALSE;
    }
    return IsRolePlayExcludedAbility(ability);
}

static const u16 sSkillSwapExcludedAbilities[] = { ABILITY_WONDER_GUARD, ABILITY_MULTITYPE, ABILITY_ILLUSION };

BOOL IsSkillSwapExcludedAbility(u16 ability) {
    if (IsAbilityMultitype(ability)) {
        return TRUE;
    }
    return func_ov169_0689ca10(ability, sSkillSwapExcludedAbilities, 3);
}

BOOL IsAbilityMultitype(u16 ability) {
    return ability == ABILITY_MULTITYPE;
}

static const u16 sArceusPlates[] = { ITEM_FLAME_PLATE,  ITEM_SPLASH_PLATE, ITEM_ZAP_PLATE,    ITEM_MEADOW_PLATE,
                                     ITEM_ICICLE_PLATE, ITEM_FIST_PLATE,   ITEM_TOXIC_PLATE,  ITEM_EARTH_PLATE,
                                     ITEM_SKY_PLATE,    ITEM_MIND_PLATE,   ITEM_INSECT_PLATE, ITEM_STONE_PLATE,
                                     ITEM_SPOOKY_PLATE, ITEM_DRACO_PLATE,  ITEM_DREAD_PLATE,  ITEM_IRON_PLATE };

BOOL IsArceusPlate(u16 item) {
    return func_ov169_0689ca10(item, sArceusPlates, 16);
}

static const u16 sGenesectDrives[] = { ITEM_DOUSE_DRIVE, ITEM_SHOCK_DRIVE, ITEM_BURN_DRIVE, ITEM_CHILL_DRIVE };

BOOL IsGenesectDrive(u16 item) {
    return func_ov169_0689ca10(item, sGenesectDrives, 4);
}

static const u16 sBackgrounds[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10 };

BOOL func_ov169_0689cb28(u16 background) {
    return func_ov169_0689ca10(background, sBackgrounds, 10);
}

// The abilities that Mold Breaker and its kin ignore
static const u16 sMoldBreakerIgnoredAbilities[] = {
    ABILITY_WONDER_GUARD, ABILITY_SOUNDPROOF,   ABILITY_LEVITATE,     ABILITY_SAND_VEIL,    ABILITY_SNOW_CLOAK,
    ABILITY_WATER_ABSORB, ABILITY_BATTLE_ARMOR, ABILITY_LIGHTNINGROD, ABILITY_STORM_DRAIN,  ABILITY_SHELL_ARMOR,
    ABILITY_UNAWARE,      ABILITY_LIGHTNINGROD, ABILITY_STORM_DRAIN,  ABILITY_SUCTION_CUPS, ABILITY_SIMPLE,
    ABILITY_TANGLED_FEET, ABILITY_SOLID_ROCK,   ABILITY_FILTER,       ABILITY_FLASH_FIRE,   ABILITY_MOTOR_DRIVE,
    ABILITY_STICKY_HOLD,  ABILITY_MARVEL_SCALE, ABILITY_THICK_FAT,    ABILITY_HEATPROOF,    ABILITY_WHITE_SMOKE,
    ABILITY_CLEAR_BODY,   ABILITY_KEEN_EYE,     ABILITY_HYPER_CUTTER, ABILITY_INNER_FOCUS,  ABILITY_SHIELD_DUST,
    ABILITY_STURDY,       ABILITY_DAMP,         ABILITY_OBLIVIOUS,    ABILITY_LIMBER,       ABILITY_OWN_TEMPO,
    ABILITY_WATER_VEIL,   ABILITY_LEAF_GUARD,   ABILITY_INSOMNIA,     ABILITY_BIG_PECKS,    ABILITY_VITAL_SPIRIT,
    ABILITY_IMMUNITY,     ABILITY_MAGMA_ARMOR,  ABILITY_CONTRARY,     ABILITY_FRIEND_GUARD, ABILITY_MULTISCALE,
    ABILITY_TELEPATHY,    ABILITY_BIG_PECKS,    ABILITY_WONDER_SKIN,  ABILITY_MAGIC_BOUNCE, ABILITY_SAP_SIPPER,
    ABILITY_VOLT_ABSORB,  ABILITY_DRY_SKIN,     ABILITY_FLOWER_GIFT,  ABILITY_HEAVY_METAL,  ABILITY_LIGHT_METAL
};

BOOL func_ov169_0689cb38(u16 ability) {
    return func_ov169_0689ca10(ability, sMoldBreakerIgnoredAbilities, 55);
}

BOOL IsArceus(u16 species) {
    return species == SPECIES_ARCEUS;
}

// Ids that the stat-effect command takes; they are neither moves nor items, so they stay numbers
static const u16 sEffectIds[] = { 598, 602, 599, 600, 601, 603, 604, 627, 628, 642, 606, 611 };

BOOL func_ov169_0689cb5c(u16 effect) {
    return func_ov169_0689ca10(effect, sEffectIds, 12);
}

static const u32 sConditions[] = { CONDITION_ATTRACT, CONDITION_TORMENT, CONDITION_DISABLE, CONDITION_HEAL_BLOCK, 23,
                                   CONDITION_TAUNT };

u32 func_ov169_0689cb6c(u32 index) {
    if (index < 6) {
        return sConditions[index];
    }
    return 0;
}

static const u32 sConditionsAll[] = { 35,
                                      CONDITION_INGRAIN,
                                      CONDITION_LEECH_SEED,
                                      CONDITION_POISON,
                                      CONDITION_BURN,
                                      CONDITION_NIGHTMARE,
                                      CONDITION_CURSE,
                                      CONDITION_BIND,
                                      CONDITION_TAUNT,
                                      CONDITION_TORMENT,
                                      23,
                                      CONDITION_DISABLE,
                                      30,
                                      32,
                                      CONDITION_HEAL_BLOCK,
                                      CONDITION_EMBARGO,
                                      28,
                                      29,
                                      CONDITION_YAWN,
                                      CONDITION_PERISH_SONG,
                                      CONDITION_PARALYSIS,
                                      CONDITION_SLEEP,
                                      CONDITION_FREEZE,
                                      CONDITION_ATTRACT,
                                      CONDITION_CONFUSION,
                                      CONDITION_GASTRO_ACID,
                                      CONDITION_FORESIGHT,
                                      CONDITION_MEAN_LOOK,
                                      24,
                                      25,
                                      26,
                                      27,
                                      31,
                                      33,
                                      34 };

u32 func_ov169_0689cb80(u32 index) {
    if (index < 35) {
        return sConditionsAll[index];
    }
    return 0;
}

static const u16 sUncallableMoves[] = { MOVE_METRONOME, MOVE_SLEEP_TALK,  MOVE_ASSIST,       MOVE_COPYCAT,
                                        MOVE_ME_FIRST,  MOVE_MIRROR_MOVE, MOVE_NATURE_POWER, MOVE_CHATTER,
                                        MOVE_STRUGGLE,  MOVE_SKETCH,      MOVE_MIMIC };

// The moves that call other moves, and the Pokemon's own
static BOOL IsUncallableMove(u16 move) {
    return func_ov169_0689ca10(move, sUncallableMoves, 11);
}

static const u16 sSleepTalkExcludedMoves[] = { MOVE_UPROAR,       MOVE_SKULL_BASH,  MOVE_SKY_ATTACK, MOVE_SOLAR_BEAM,
                                               MOVE_BIDE,         MOVE_RAZOR_WIND,  MOVE_FLY,        MOVE_BOUNCE,
                                               MOVE_SHADOW_FORCE, MOVE_FOCUS_PUNCH, MOVE_SKY_DROP };

BOOL IsSleepTalkExcludedMove(u16 move) {
    if (IsUncallableMove(move)) {
        return TRUE;
    }
    return func_ov169_0689ca10(move, sSleepTalkExcludedMoves, 11);
}

static const u16 sAssistExcludedMoves[] = { MOVE_COUNTER,   MOVE_MIRROR_COAT,  MOVE_PROTECT,     MOVE_DETECT,
                                            MOVE_ENDURE,    MOVE_DESTINY_BOND, MOVE_FOLLOW_ME,   MOVE_RAGE_POWDER,
                                            MOVE_SNATCH,    MOVE_HELPING_HAND, MOVE_THIEF,       MOVE_COVET,
                                            MOVE_TRICK,     MOVE_SWITCHEROO,   MOVE_FEINT,       MOVE_FOCUS_PUNCH,
                                            MOVE_TRANSFORM, MOVE_BESTOW,       MOVE_DRAGON_TAIL, MOVE_CIRCLE_THROW };

BOOL IsAssistExcludedMove(u16 move) {
    if (IsUncallableMove(move)) {
        return TRUE;
    }
    return func_ov169_0689ca10(move, sAssistExcludedMoves, 20);
}

static const u16 sCopycatExcludedMoves[] = { MOVE_COUNTER,   MOVE_MIRROR_COAT,  MOVE_PROTECT,     MOVE_DETECT,
                                             MOVE_ENDURE,    MOVE_DESTINY_BOND, MOVE_FOLLOW_ME,   MOVE_RAGE_POWDER,
                                             MOVE_SNATCH,    MOVE_HELPING_HAND, MOVE_THIEF,       MOVE_COVET,
                                             MOVE_TRICK,     MOVE_SWITCHEROO,   MOVE_FEINT,       MOVE_FOCUS_PUNCH,
                                             MOVE_TRANSFORM, MOVE_BESTOW,       MOVE_DRAGON_TAIL, MOVE_CIRCLE_THROW };

BOOL IsCopycatExcludedMove(u16 move) {
    if (IsUncallableMove(move)) {
        return TRUE;
    }
    return func_ov169_0689ca10(move, sCopycatExcludedMoves, 20);
}

static const u16 sMimicExcludedMoves[] = { MOVE_NONE,      MOVE_SKETCH,   MOVE_MIMIC,
                                           MOVE_TRANSFORM, MOVE_STRUGGLE, MOVE_CHATTER };

BOOL IsMimicExcludedMove(u16 move) {
    return func_ov169_0689ca10(move, sMimicExcludedMoves, 6);
}

static const u16 sMetronomeExcludedMoves[] = {
    MOVE_METRONOME,    MOVE_SLEEP_TALK,   MOVE_STRUGGLE,     MOVE_ASSIST,       MOVE_COPYCAT,      MOVE_ME_FIRST,
    MOVE_MIRROR_MOVE,  MOVE_NATURE_POWER, MOVE_CHATTER,      MOVE_SKETCH,       MOVE_MIMIC,        MOVE_COUNTER,
    MOVE_MIRROR_COAT,  MOVE_PROTECT,      MOVE_DETECT,       MOVE_ENDURE,       MOVE_DESTINY_BOND, MOVE_TRANSFORM,
    MOVE_FOLLOW_ME,    MOVE_RAGE_POWDER,  MOVE_FOCUS_PUNCH,  MOVE_SNATCH,       MOVE_HELPING_HAND, MOVE_THIEF,
    MOVE_COVET,        MOVE_TRICK,        MOVE_SWITCHEROO,   MOVE_FEINT,        MOVE_QUASH,        MOVE_BESTOW,
    MOVE_TECHNO_BLAST, MOVE_RELIC_SONG,   MOVE_SECRET_SWORD, MOVE_FREEZE_SHOCK, MOVE_ICE_BURN,     MOVE_SNARL,
    MOVE_V_CREATE,     MOVE_SNORE,        MOVE_QUICK_GUARD,  MOVE_WIDE_GUARD,   MOVE_AFTER_YOU
};

const u16 *GetMetronomeExcludedMoves(u32 *count) {
    *count = 41;
    return sMetronomeExcludedMoves;
}
