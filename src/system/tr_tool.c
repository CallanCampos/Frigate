#include "types.h"
#include "battle/btl_setup.h"
#include "battle/trainer_data.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/trainer_back_sprites.h"
#include "constants/trainer_classes.h"
#include "constants/trainer_messages.h"
#include "constants/trainer_sprites.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/pms_data.h"

// The trainers' records, parties and messages, and the trainer classes' tables. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except the structs, the constants and the fields

#define TRAINER_CLASS_COUNT 236

// The system message files of the trainers' messages and names, and of the Battle Institute trainers' messages
#define TRMSG_FILE TEXT_BANK_TRAINER_MESSAGES
#define TRNAME_FILE TEXT_BANK_TRAINER_NAMES
#define TRMSG_BATTLE_INST_START_FILE TEXT_BANK_BATTLE_INST_START_MESSAGES
#define TRMSG_BATTLE_INST_LOSE_FILE TEXT_BANK_BATTLE_INST_LOSE_MESSAGES

// The Battle Institute's trainers' records: the trainers from 620 to 639 share one, and all the others another
#define TRDATA_BATTLE_INST_GROUP_START 620
#define TRDATA_BATTLE_INST_GROUP_COUNT 20
#define TRDATA_BATTLE_INST_GROUP_FILE 661
#define TRDATA_BATTLE_INST_FILE 660

// The low byte of a trainer Pokémon's PID, by the trainer's sex: it makes the Pokémon of an even sex ratio female for
// a female trainer and male for a male one
#define PID_BASE_FEMALE_TRAINER 0x78
#define PID_BASE_MALE_TRAINER 0x88

#define PACK_IVS(hp, atk, def, spe, spa, spd)                                                                          \
    (((hp) & 0x1f) | (((atk) & 0x1f) << 5) | (((def) & 0x1f) << 10) | (((spe) & 0x1f) << 15) |                         \
     (((spa) & 0x1f) << 20) | (((spd) & 0x1f) << 25))

typedef struct {
    u16 trainerClass;
    u16 bg;
} TrClassBattleBG;

typedef struct {
    u8 trainerClass;
    u8 bgmGroup;
    u8 pedestal;
} SpecialTrainerClass;

typedef struct {
    u16 trainerClass;
    u16 sprite;
} TrClassBackSprite;

typedef struct {
    u8 sprite;
    u8 sex;
    u16 eyeBGM;
} TrClassResource;

static void TrainerData_ReadTrainerData(int trainerId, TrainerData *data);
static void TrainerData_ReadParty(int trainerId, void *party);
static void TrainerUtil_CalcBasePID(u32 species, u32 form, u8 genderAbility, u32 *pidBase, HeapID heapId);
static void TrainerUtil_SetupPkm(int trainerId, PartyPkm *pkm, u32 form, u8 genderAbility);
static u8 TrainerUtil_IsIDBattleInst(int trainerId);

static const TrClassBattleBG TR_CLASS_BATTLE_BG_OVERRIDES[] = {
    { TRAINER_CLASS_TEAM_PLASMA_GHETSIS, 20 },
};

static const SpecialTrainerClass SPECIAL_TRAINER_CLASSES[TRAINER_CLASS_SPECIAL_COUNT] = {
    { TRAINER_CLASS_LEADER_CHEREN, 0, 20 },
    { TRAINER_CLASS_LEADER_ROXIE, 0, 20 },
    { TRAINER_CLASS_LEADER_BURGH, 0, 20 },
    { TRAINER_CLASS_LEADER_ELESA, 0, 20 },
    { TRAINER_CLASS_LEADER_CLAY, 0, 20 },
    { TRAINER_CLASS_LEADER_SKYLA, 0, 20 },
    { TRAINER_CLASS_LEADER_DRAYDEN, 0, 20 },
    { TRAINER_CLASS_LEADER_MARLON, 0, 20 },
    { TRAINER_CLASS_ELITE_FOUR_SHAUNTAL, 1, 20 },
    { TRAINER_CLASS_ELITE_FOUR_GRIMSLEY, 1, 20 },
    { TRAINER_CLASS_ELITE_FOUR_MARSHAL, 1, 20 },
    { TRAINER_CLASS_ELITE_FOUR_CAITLIN, 1, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_CYNTHIA, 2, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_RIVAL, 3, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_COLRESS_195, 4, 20 },
    { TRAINER_CLASS_TEAM_PLASMA_COLRESS, 5, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_N, 6, 20 },
    { TRAINER_CLASS_TEAM_PLASMA_GRUNT_M, 7, 20 },
    { TRAINER_CLASS_TEAM_PLASMA_GRUNT_F, 7, 20 },
    { TRAINER_CLASS_TEAM_PLASMA_ZINZOLIN, 10, 20 },
    { TRAINER_CLASS_TEAM_PLASMA_SHADOW, 7, 20 },
    { TRAINER_CLASS_CHAMPION, 8, 20 },
    { TRAINER_CLASS_TEAM_PLASMA_GHETSIS, 9, 20 },
    { TRAINER_CLASS_BOSS_TRAINER_BENGA, 11, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_COLRESS_235, 12, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_CHEREN, 13, 20 },
    { TRAINER_CLASS_PKMN_TRAINER_BIANCA, 13, 20 },
    { TRAINER_CLASS_SUBWAY_BOSS_88, 14, 20 },
    { TRAINER_CLASS_SUBWAY_BOSS_102, 14, 20 },
};

static const TrClassBackSprite TRAINER_CLASS_BACK_SPRITES[] = {
    { TRAINER_CLASS_PKMN_TRAINER_NATE, TRAINER_BACK_SPRITE_PKMN_TRAINER_NATE },
    { TRAINER_CLASS_PKMN_TRAINER_ROSA, TRAINER_BACK_SPRITE_PKMN_TRAINER_ROSA },
    { TRAINER_CLASS_PKMN_TRAINER_CHILI, TRAINER_BACK_SPRITE_PKMN_TRAINER_CHILI },
    { TRAINER_CLASS_PKMN_TRAINER_CILAN, TRAINER_BACK_SPRITE_PKMN_TRAINER_CILAN },
    { TRAINER_CLASS_PKMN_TRAINER_CRESS, TRAINER_BACK_SPRITE_PKMN_TRAINER_CRESS },
    { TRAINER_CLASS_NO_DATA_M_37, TRAINER_BACK_SPRITE_NO_DATA_M },
    { TRAINER_CLASS_PKMN_TRAINER_BIANCA, TRAINER_BACK_SPRITE_PKMN_TRAINER_BIANCA },
    { TRAINER_CLASS_DOCTOR, TRAINER_BACK_SPRITE_DOCTOR },
    { TRAINER_CLASS_PKMN_TRAINER_RIVAL, TRAINER_BACK_SPRITE_PKMN_TRAINER_RIVAL },
    { TRAINER_CLASS_PKMN_TRAINER_M_180, TRAINER_BACK_SPRITE_PKMN_TRAINER_M_180 },
    { TRAINER_CLASS_PKMN_TRAINER_F_181, TRAINER_BACK_SPRITE_PKMN_TRAINER_F_181 },
    { TRAINER_CLASS_PKMN_TRAINER_M_182, TRAINER_BACK_SPRITE_PKMN_TRAINER_M_182 },
    { TRAINER_CLASS_PKMN_TRAINER_F_183, TRAINER_BACK_SPRITE_PKMN_TRAINER_F_183 },
    { TRAINER_CLASS_PKMN_TRAINER_M_184, TRAINER_BACK_SPRITE_PKMN_TRAINER_M_184 },
    { TRAINER_CLASS_PKMN_TRAINER_F_185, TRAINER_BACK_SPRITE_PKMN_TRAINER_F_185 },
    { TRAINER_CLASS_CHAMPION, TRAINER_BACK_SPRITE_CHAMPION },
    { TRAINER_CLASS_PKMN_TRAINER_CHEREN, TRAINER_BACK_SPRITE_PKMN_TRAINER_CHEREN },
    { TRAINER_CLASS_PKMN_TRAINER_F_198, TRAINER_BACK_SPRITE_CHAMPION },
    { TRAINER_CLASS_HERO, TRAINER_BACK_SPRITE_HERO },
    { TRAINER_CLASS_HEROINE, TRAINER_BACK_SPRITE_HEROINE },
    { TRAINER_CLASS_NINJA_M, TRAINER_BACK_SPRITE_NINJA_M },
    { TRAINER_CLASS_NINJA_F, TRAINER_BACK_SPRITE_NINJA_F },
    { TRAINER_CLASS_UDF_TROOPER_M, TRAINER_BACK_SPRITE_UDF_TROOPER_M },
    { TRAINER_CLASS_UDF_TROOPER_F, TRAINER_BACK_SPRITE_UDF_TROOPER_F },
    { TRAINER_CLASS_SCIENTIST_M_205, TRAINER_BACK_SPRITE_SCIENTIST_M },
    { TRAINER_CLASS_SCIENTIST_F_206, TRAINER_BACK_SPRITE_SCIENTIST_F },
    { TRAINER_CLASS_GUY, TRAINER_BACK_SPRITE_GUY },
    { TRAINER_CLASS_GIRL, TRAINER_BACK_SPRITE_GIRL },
    { TRAINER_CLASS_PRINCE, TRAINER_BACK_SPRITE_PRINCE },
    { TRAINER_CLASS_PRINCESS, TRAINER_BACK_SPRITE_PRINCESS },
    { TRAINER_CLASS_GUY_IN_SUIT_225, TRAINER_BACK_SPRITE_GUY_IN_SUIT },
    { TRAINER_CLASS_GIRL_IN_SUIT_226, TRAINER_BACK_SPRITE_GIRL_IN_SUIT },
};

static const TrClassResource TRAINER_CLASS_RESOURCES[TRAINER_CLASS_COUNT] = {
    [TRAINER_CLASS_PKMN_TRAINER_NATE] = { TRAINER_SPRITE_PKMN_TRAINER_NATE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_ROSA] = { TRAINER_SPRITE_PKMN_TRAINER_ROSA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_YOUNGSTER] = { TRAINER_SPRITE_YOUNGSTER, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LASS] = { TRAINER_SPRITE_LASS, 1, SEQ_BGM_EYE_02 },
    [TRAINER_CLASS_SCHOOL_KID_M] = { TRAINER_SPRITE_SCHOOL_KID_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SCHOOL_KID_F] = { TRAINER_SPRITE_SCHOOL_KID_F, 1, SEQ_BGM_EYE_02 },
    [TRAINER_CLASS_SMASHER] = { TRAINER_SPRITE_SMASHER, 1, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_LINEBACKER] = { TRAINER_SPRITE_LINEBACKER, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_WAITER] = { TRAINER_SPRITE_WAITER, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_WAITRESS] = { TRAINER_SPRITE_WAITRESS, 1, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_PKMN_TRAINER_CHILI] = { TRAINER_SPRITE_PKMN_TRAINER_CHILI, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CILAN] = { TRAINER_SPRITE_PKMN_TRAINER_CILAN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CRESS] = { TRAINER_SPRITE_PKMN_TRAINER_CRESS, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NURSERY_AIDE] = { TRAINER_SPRITE_NURSERY_AIDE, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_PRESCHOOLER_F] = { TRAINER_SPRITE_PRESCHOOLER_F, 1, SEQ_BGM_EYE_03 },
    [TRAINER_CLASS_PRESCHOOLER_M] = { TRAINER_SPRITE_PRESCHOOLER_M, 0, SEQ_BGM_EYE_03 },
    [TRAINER_CLASS_TWINS] = { TRAINER_SPRITE_TWINS, 1, SEQ_BGM_EYE_03 },
    [TRAINER_CLASS_PKMN_BREEDER_M] = { TRAINER_SPRITE_PKMN_BREEDER_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_BREEDER_F] = { TRAINER_SPRITE_PKMN_BREEDER_F, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_LENORA] = { TRAINER_SPRITE_LENORA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_20] = { TRAINER_SPRITE_NO_DATA_M_20, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_21] = { TRAINER_SPRITE_NO_DATA_F_21, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_22] = { TRAINER_SPRITE_NO_DATA_M_22, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_23] = { TRAINER_SPRITE_NO_DATA_F_23, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_RANGER_M] = { TRAINER_SPRITE_PKMN_RANGER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_RANGER_F] = { TRAINER_SPRITE_PKMN_RANGER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_WORKER_26] = { TRAINER_SPRITE_WORKER_26, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_BACKPACKER_M] = { TRAINER_SPRITE_BACKPACKER_M, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_BACKPACKER_F] = { TRAINER_SPRITE_BACKPACKER_F, 1, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_FISHERMAN] = { TRAINER_SPRITE_FISHERMAN, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_MUSICIAN] = { TRAINER_SPRITE_MUSICIAN, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_DANCER] = { TRAINER_SPRITE_DANCER, 0, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_HARLEQUIN_32] = { TRAINER_SPRITE_HARLEQUIN, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_ARTIST] = { TRAINER_SPRITE_ARTIST, 0, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_BAKER] = { TRAINER_SPRITE_BAKER, 1, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_PSYCHIC_M] = { TRAINER_SPRITE_PSYCHIC_M, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PSYCHIC_F] = { TRAINER_SPRITE_PSYCHIC_F, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_NO_DATA_M_37] = { TRAINER_SPRITE_NO_DATA_M_37, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_BIANCA] = { TRAINER_SPRITE_PKMN_TRAINER_BIANCA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_39] = { TRAINER_SPRITE_NO_DATA_M_39, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_N] = { TRAINER_SPRITE_PKMN_TRAINER_N, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_RICH_BOY] = { TRAINER_SPRITE_RICH_BOY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LADY] = { TRAINER_SPRITE_LADY, 1, SEQ_BGM_EYE_02 },
    [TRAINER_CLASS_PILOT] = { TRAINER_SPRITE_PILOT, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_WORKER_44] = { TRAINER_SPRITE_WORKER_44, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_HOOPSTER] = { TRAINER_SPRITE_HOOPSTER, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_SCIENTIST_F_46] = { TRAINER_SPRITE_SCIENTIST_F_46, 1, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_NO_DATA_M_47] = { TRAINER_SPRITE_PKMN_TRAINER_N, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CLERK_F] = { TRAINER_SPRITE_CLERK_F, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_ACE_TRAINER_F] = { TRAINER_SPRITE_ACE_TRAINER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_ACE_TRAINER_M] = { TRAINER_SPRITE_ACE_TRAINER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BLACK_BELT] = { TRAINER_SPRITE_BLACK_BELT, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_SCIENTIST_M_52] = { TRAINER_SPRITE_SCIENTIST_M_52, 0, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_STRIKER] = { TRAINER_SPRITE_STRIKER, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_BRYCEN] = { TRAINER_SPRITE_BRYCEN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_55] = { TRAINER_SPRITE_NO_DATA_F_55, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_56] = { TRAINER_SPRITE_NO_DATA_M_56, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ROUGHNECK] = { TRAINER_SPRITE_ROUGHNECK, 0, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_JANITOR] = { TRAINER_SPRITE_JANITOR, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_POKEFAN_M] = { TRAINER_SPRITE_POKEFAN_M, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_POKEFAN_F] = { TRAINER_SPRITE_POKEFAN_F, 1, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_DOCTOR] = { TRAINER_SPRITE_DOCTOR, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_NURSE] = { TRAINER_SPRITE_NURSE, 1, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_HOOLIGANS] = { TRAINER_SPRITE_HOOLIGANS, 0, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_BATTLE_GIRL] = { TRAINER_SPRITE_BATTLE_GIRL, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PARASOL_LADY] = { TRAINER_SPRITE_PARASOL_LADY, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_CLERK_M_66] = { TRAINER_SPRITE_CLERK_M_66, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_CLERK_M_67] = { TRAINER_SPRITE_CLERK_M_67, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_BACKERS_M] = { TRAINER_SPRITE_BACKERS_M, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_BACKERS_F] = { TRAINER_SPRITE_BACKERS_F, 1, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_VETERAN_M] = { TRAINER_SPRITE_VETERAN_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_VETERAN_F] = { TRAINER_SPRITE_VETERAN_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BIKER] = { TRAINER_SPRITE_BIKER, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_INFIELDER] = { TRAINER_SPRITE_INFIELDER, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_HIKER] = { TRAINER_SPRITE_HIKER, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_SOCIALITE] = { TRAINER_SPRITE_SOCIALITE, 1, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_GENTLEMAN] = { TRAINER_SPRITE_GENTLEMAN, 0, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_NO_DATA_F_77] = { TRAINER_SPRITE_NO_DATA_F_77, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_SHAUNTAL] = { TRAINER_SPRITE_ELITE_FOUR_SHAUNTAL, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_MARSHAL] = { TRAINER_SPRITE_ELITE_FOUR_MARSHAL, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_GRIMSLEY] = { TRAINER_SPRITE_ELITE_FOUR_GRIMSLEY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_CAITLIN] = { TRAINER_SPRITE_ELITE_FOUR_CAITLIN, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_82] = { TRAINER_SPRITE_NO_DATA_M_82, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_DEPOT_AGENT] = { TRAINER_SPRITE_DEPOT_AGENT, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_SWIMMER_M] = { TRAINER_SPRITE_SWIMMER_M, 0, SEQ_BGM_EYE_07 },
    [TRAINER_CLASS_SWIMMER_F] = { TRAINER_SPRITE_SWIMMER_F, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_POLICEMAN] = { TRAINER_SPRITE_POLICEMAN, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_MAID] = { TRAINER_SPRITE_MAID, 1, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_SUBWAY_BOSS_88] = { TRAINER_SPRITE_SUBWAY_BOSS_INGO, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_ALDER] = { TRAINER_SPRITE_PKMN_TRAINER_ALDER, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CYCLIST_M] = { TRAINER_SPRITE_CYCLIST_M, 0, SEQ_BGM_EYE_07 },
    [TRAINER_CLASS_CYCLIST_F] = { TRAINER_SPRITE_CYCLIST_F, 1, SEQ_BGM_EYE_07 },
    [TRAINER_CLASS_MOTORCYCLIST] = { TRAINER_SPRITE_BIKER, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_NO_DATA_M_93] = { TRAINER_SPRITE_SCHOOL_KID_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_94] = { TRAINER_SPRITE_VETERAN_F, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_95] = { TRAINER_SPRITE_SOCIALITE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_96] = { TRAINER_SPRITE_GENTLEMAN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GAME_FREAK_MORIMOTO] = { TRAINER_SPRITE_VETERAN_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_NO_DATA_F_98] = { TRAINER_SPRITE_LADY, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_99] = { TRAINER_SPRITE_RICH_BOY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CYNTHIA] = { TRAINER_SPRITE_PKMN_TRAINER_CYNTHIA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA] = { TRAINER_SPRITE_PKMN_TRAINER_N, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SUBWAY_BOSS_102] = { TRAINER_SPRITE_SUBWAY_BOSS_EMMET, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_103] = { TRAINER_SPRITE_NO_DATA_M_103, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_104] = { TRAINER_SPRITE_SCIENTIST_M_52, 0, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_PKMN_TRAINER_M_105] = { TRAINER_SPRITE_HIKER, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_PKMN_TRAINER_M_106] = { TRAINER_SPRITE_PSYCHIC_M, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_F_107] = { TRAINER_SPRITE_PSYCHIC_F, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_M_108] = { TRAINER_SPRITE_VETERAN_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_TRAINER_F_109] = { TRAINER_SPRITE_VETERAN_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_TRAINER_M_110] = { TRAINER_SPRITE_ACE_TRAINER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_TRAINER_F_111] = { TRAINER_SPRITE_ACE_TRAINER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_LEADER_ELESA] = { TRAINER_SPRITE_LEADER_ELESA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_BURGH] = { TRAINER_SPRITE_LEADER_BURGH, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_SKYLA] = { TRAINER_SPRITE_LEADER_SKYLA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_CHEREN] = { TRAINER_SPRITE_LEADER_CHEREN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_ROXIE] = { TRAINER_SPRITE_LEADER_ROXIE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_CLAY] = { TRAINER_SPRITE_LEADER_CLAY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_DRAYDEN] = { TRAINER_SPRITE_LEADER_DRAYDEN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_MARLON] = { TRAINER_SPRITE_LEADER_MARLON, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BROCK] = { TRAINER_SPRITE_BROCK, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MISTY] = { TRAINER_SPRITE_MISTY, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LT_SURGE] = { TRAINER_SPRITE_LT_SURGE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ERIKA] = { TRAINER_SPRITE_ERIKA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SABRINA] = { TRAINER_SPRITE_SABRINA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BLAINE] = { TRAINER_SPRITE_BLAINE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIOVANNI] = { TRAINER_SPRITE_GIOVANNI, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_FALKNER] = { TRAINER_SPRITE_FALKNER, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BUGSY] = { TRAINER_SPRITE_BUGSY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WHITNEY] = { TRAINER_SPRITE_WHITNEY, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MORTY] = { TRAINER_SPRITE_MORTY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CHUCK] = { TRAINER_SPRITE_CHUCK, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_JASMINE] = { TRAINER_SPRITE_JASMINE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PRYCE] = { TRAINER_SPRITE_PRYCE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CLAIR] = { TRAINER_SPRITE_CLAIR, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_JANINE] = { TRAINER_SPRITE_JANINE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ROXANNE] = { TRAINER_SPRITE_ROXANNE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BRAWLY] = { TRAINER_SPRITE_BRAWLY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WATTSON] = { TRAINER_SPRITE_WATTSON, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_FLANNERY] = { TRAINER_SPRITE_FLANNERY, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NORMAN] = { TRAINER_SPRITE_NORMAN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WINONA] = { TRAINER_SPRITE_WINONA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TATE] = { TRAINER_SPRITE_TATE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LIZA] = { TRAINER_SPRITE_LIZA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_JUAN] = { TRAINER_SPRITE_JUAN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_RIVAL] = { TRAINER_SPRITE_PKMN_TRAINER_RIVAL, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUITARIST] = { TRAINER_SPRITE_GUITARIST, 1, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_BOSS_TRAINER_BRENT] = { TRAINER_SPRITE_PKMN_BREEDER_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_ABED] = { TRAINER_SPRITE_PKMN_BREEDER_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_ABIGAIL] = { TRAINER_SPRITE_PKMN_BREEDER_F, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_BOSS_TRAINER_BESS] = { TRAINER_SPRITE_PKMN_BREEDER_F, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_BOSS_TRAINER_DANEIL] = { TRAINER_SPRITE_PKMN_RANGER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_CARLEN] = { TRAINER_SPRITE_PKMN_RANGER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_CARLEIGH] = { TRAINER_SPRITE_PKMN_RANGER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_DANELLE] = { TRAINER_SPRITE_PKMN_RANGER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_SUBWAY_BOSS_INGO] = { TRAINER_SPRITE_SUBWAY_BOSS_INGO, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_156] = { TRAINER_SPRITE_SUBWAY_BOSS_INGO, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_FREDERICK] = { TRAINER_SPRITE_ACE_TRAINER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_GAIUS] = { TRAINER_SPRITE_ACE_TRAINER_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_GAIL] = { TRAINER_SPRITE_ACE_TRAINER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_FREIRA] = { TRAINER_SPRITE_ACE_TRAINER_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_M_161] = { TRAINER_SPRITE_VETERAN_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_M_162] = { TRAINER_SPRITE_VETERAN_M, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_F_163] = { TRAINER_SPRITE_VETERAN_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_F_164] = { TRAINER_SPRITE_VETERAN_F, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_SUBWAY_BOSS_EMMET] = { TRAINER_SPRITE_SUBWAY_BOSS_EMMET, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_166] = { TRAINER_SPRITE_SUBWAY_BOSS_EMMET, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ROARK] = { TRAINER_SPRITE_ROARK, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GARDENIA] = { TRAINER_SPRITE_GARDENIA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_FANTINA] = { TRAINER_SPRITE_FANTINA, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MAYLENE] = { TRAINER_SPRITE_MAYLENE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WAKE] = { TRAINER_SPRITE_WAKE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BYRON] = { TRAINER_SPRITE_BYRON, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CANDICE] = { TRAINER_SPRITE_CANDICE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_VOLKNER] = { TRAINER_SPRITE_VOLKNER, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BLUE] = { TRAINER_SPRITE_BLUE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LANCE] = { TRAINER_SPRITE_LANCE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_STEVEN] = { TRAINER_SPRITE_STEVEN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WALLACE] = { TRAINER_SPRITE_WALLACE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BEAUTY] = { TRAINER_SPRITE_BEAUTY, 1, SEQ_BGM_EYE_DANCER },
    [TRAINER_CLASS_PKMN_TRAINER_M_180] = { TRAINER_SPRITE_PKMN_TRAINER_M_180, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_181] = { TRAINER_SPRITE_PKMN_TRAINER_F_181, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_182] = { TRAINER_SPRITE_PKMN_TRAINER_M_182, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_183] = { TRAINER_SPRITE_PKMN_TRAINER_F_183, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_184] = { TRAINER_SPRITE_PKMN_TRAINER_M_184, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_185] = { TRAINER_SPRITE_PKMN_TRAINER_F_185, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_COLRESS] = { TRAINER_SPRITE_TEAM_PLASMA_COLRESS, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_GRUNT_M] = { TRAINER_SPRITE_TEAM_PLASMA_GRUNT_M, 0, SEQ_BGM_EYE_NEO_PLASMA },
    [TRAINER_CLASS_TEAM_PLASMA_GRUNT_F] = { TRAINER_SPRITE_TEAM_PLASMA_GRUNT_F, 1, SEQ_BGM_EYE_NEO_PLASMA },
    [TRAINER_CLASS_TEAM_PLASMA_GHETSIS] = { TRAINER_SPRITE_TEAM_PLASMA_GHETSIS, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_ROOD] = { TRAINER_SPRITE_PKMN_TRAINER_ROOD, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_ZINZOLIN] = { TRAINER_SPRITE_TEAM_PLASMA_ZINZOLIN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_SHADOW] = { TRAINER_SPRITE_TEAM_PLASMA_SHADOW, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CHAMPION] = { TRAINER_SPRITE_CHAMPION, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MASKED_MAN] = { TRAINER_SPRITE_MASKED_MAN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_COLRESS_195] = { TRAINER_SPRITE_TEAM_PLASMA_COLRESS, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GAME_FREAK_NISHINO] = { TRAINER_SPRITE_HIKER, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CHEREN] = { TRAINER_SPRITE_LEADER_CHEREN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_198] = { TRAINER_SPRITE_CHAMPION, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_HERO] = { TRAINER_SPRITE_HERO, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_HEROINE] = { TRAINER_SPRITE_HEROINE, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NINJA_M] = { TRAINER_SPRITE_NINJA_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NINJA_F] = { TRAINER_SPRITE_NINJA_F, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_UDF_TROOPER_M] = { TRAINER_SPRITE_UDF_TROOPER_M, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_UDF_TROOPER_F] = { TRAINER_SPRITE_UDF_TROOPER_F, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SCIENTIST_M_205] = { TRAINER_SPRITE_SCIENTIST_M_205, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SCIENTIST_F_206] = { TRAINER_SPRITE_SCIENTIST_F_206, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUY] = { TRAINER_SPRITE_GUY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIRL] = { TRAINER_SPRITE_GIRL, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PRINCE] = { TRAINER_SPRITE_PRINCE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PRINCESS] = { TRAINER_SPRITE_PRINCESS, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BRYCEN_JET] = { TRAINER_SPRITE_BRYCEN_JET, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MECHACOMBO] = { TRAINER_SPRITE_MECHACOMBO, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_UFO] = { TRAINER_SPRITE_UFO, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ALIEN] = { TRAINER_SPRITE_ALIEN, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_RED_FOG] = { TRAINER_SPRITE_RED_FOG, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CURSED_MAN] = { TRAINER_SPRITE_CURSED_MAN, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CURSED_KID] = { TRAINER_SPRITE_CURSED_KID, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_OLD_STATUE] = { TRAINER_SPRITE_OLD_STATUE, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CURSED_GENT] = { TRAINER_SPRITE_CURSED_GENT, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_DIVINE_MAID] = { TRAINER_SPRITE_DIVINE_MAID, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WITCH] = { TRAINER_SPRITE_WITCH, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PLUSH_TOY] = { TRAINER_SPRITE_PLUSH_TOY, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUY_IN_SUIT_223] = { TRAINER_SPRITE_GUY_IN_SUIT_223, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIRL_IN_SUIT_224] = { TRAINER_SPRITE_GIRL_IN_SUIT_224, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUY_IN_SUIT_225] = { TRAINER_SPRITE_GUY_IN_SUIT_225, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIRL_IN_SUIT_226] = { TRAINER_SPRITE_GIRL_IN_SUIT_226, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_BENGA] = { TRAINER_SPRITE_BOSS_TRAINER_BENGA, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_228] = { TRAINER_SPRITE_BLACK_BELT, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_F_229] = { TRAINER_SPRITE_BATTLE_GIRL, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_F_230] = { TRAINER_SPRITE_SCIENTIST_F_46, 1, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_NO_DATA_M_231] = { TRAINER_SPRITE_PILOT, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_FUTURE_BEING] = { TRAINER_SPRITE_FUTURE_BEING, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_HARLEQUIN_233] = { TRAINER_SPRITE_HARLEQUIN, 0, SEQ_BGM_EYE_CLOWN },
    [TRAINER_CLASS_RED] = { TRAINER_SPRITE_RED, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_COLRESS_235] = { TRAINER_SPRITE_TEAM_PLASMA_COLRESS, 0, SEQ_BGM_EYE_01 },
};

u32 TrainerData_GetParam(int trainerId, TrainerDataParam param) {
    TrainerData data;
    u32 value;

    TrainerData_ReadTrainerData(trainerId, &data);
    switch (param) {
    case TRAINER_PARAM_PARTY_KIND:
        value = data.partyKind;
        break;
    case TRAINER_PARAM_CLASS:
        value = data.trainerClass;
        break;
    case TRAINER_PARAM_BATTLE_STYLE:
        value = data.battleStyle;
        break;
    case TRAINER_PARAM_POKE_COUNT:
        value = data.pokeCount;
        break;
    case TRAINER_PARAM_ITEM_1:
    case TRAINER_PARAM_ITEM_2:
    case TRAINER_PARAM_ITEM_3:
    case TRAINER_PARAM_ITEM_4:
        value = data.items[param - TRAINER_PARAM_ITEM_1];
        break;
    case TRAINER_PARAM_AI_FLAGS:
        value = data.aiFlags;
        break;
    case TRAINER_PARAM_HEALS:
        value = data.heals;
        break;
    case TRAINER_PARAM_MONEY:
        value = data.money;
        break;
    case TRAINER_PARAM_REWARD_ITEM:
        value = data.rewardItem;
        break;
    }
    return value;
}

BOOL TrainerMsg_CheckExists(int trainerId, u32 msgId, HeapID heapId) {
    ArcTool *handle;
    u32 size;
    u16 offset;
    u16 pair[2];
    BOOL exists = FALSE;

    if (TrainerUtil_IsIDBattleInst(trainerId) == FALSE) {
        size = GFL_ArcSysGetDataLength(ARCID_TRTBL, 0);
        GFL_ArcSysReadRange(&offset, ARCID_TRTBLOFS, 0, trainerId * 2, sizeof(offset));
        handle = GFL_ArcSysCreateFileHandle(ARCID_TRTBL, heapId);
        while (offset != size) {
            GFL_ArcToolReadRange(handle, 0, offset, sizeof(pair), pair);
            if (pair[0] == trainerId && pair[1] == msgId) {
                exists = TRUE;
                break;
            }
            if (pair[0] != trainerId) {
                break;
            }
            offset += sizeof(pair);
        }
        GFL_ArcToolFree(handle);
    } else if (msgId == TRMSG_LOSE || msgId == TRMSG_WIN) {
        exists = TRUE;
    }
    return exists;
}

void TrainerMsg_Load(int trainerId, u32 msgId, StrBuf *strbuf, HeapID heapId) {
    ArcTool *handle;
    MsgData *msgData;
    u32 size;
    int offsetsSize;
    u16 offset;
    u16 pair[2];
    u8 battleInst = TrainerUtil_IsIDBattleInst(trainerId);

    if (battleInst == FALSE) {
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TRMSG_FILE, heapId);
        size = GFL_ArcSysGetDataLength(ARCID_TRTBL, 0);
        offsetsSize = GFL_ArcSysGetDataLength(ARCID_TRTBLOFS, 0);
        if (offsetsSize < trainerId * 2) {
            GFL_StrBufClear(strbuf);
            GFL_MsgDataFree(msgData);
            return;
        }
        GFL_ArcSysReadRange(&offset, ARCID_TRTBLOFS, 0, trainerId * 2, sizeof(offset));
        handle = GFL_ArcSysCreateFileHandle(ARCID_TRTBL, heapId);
        while (offset != size) {
            GFL_ArcToolReadRange(handle, 0, offset, sizeof(pair), pair);
            if (pair[0] == trainerId && pair[1] == msgId) {
                GFL_MsgDataLoadStrbuf(msgData, offset / sizeof(pair), strbuf);
                break;
            }
            offset += sizeof(pair);
        }
        GFL_ArcToolFree(handle);
        GFL_MsgDataFree(msgData);
        if (offset == size) {
            GFL_StrBufClear(strbuf);
        }
    } else if (battleInst == TRUE) {
        msgData = NULL;
        switch (msgId) {
        case TRMSG_START:
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TRMSG_BATTLE_INST_START_FILE, heapId);
            break;
        case TRMSG_LOSE:
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TRMSG_BATTLE_INST_LOSE_FILE, heapId);
            break;
        }
        if (msgData != NULL) {
            GFL_MsgDataLoadStrbuf(msgData, trainerId - TRAINER_ID_BATTLE_INST_START, strbuf);
            GFL_MsgDataFree(msgData);
        }
    }
}

static void TrainerData_ReadTrainerData(int trainerId, TrainerData *data) {
    if (trainerId < TRAINER_ID_BATTLE_INST_START) {
        GFL_ArcSysRead(data, ARCID_TRDATA, trainerId);
    } else {
        int id = trainerId - TRAINER_ID_BATTLE_INST_START;

        if (id >= TRDATA_BATTLE_INST_GROUP_START &&
            id < TRDATA_BATTLE_INST_GROUP_START + TRDATA_BATTLE_INST_GROUP_COUNT) {
            GFL_ArcSysRead(data, ARCID_TRDATA, TRDATA_BATTLE_INST_GROUP_FILE);
        } else {
            GFL_ArcSysRead(data, ARCID_TRDATA, TRDATA_BATTLE_INST_FILE);
        }
    }
}

static void TrainerData_ReadParty(int trainerId, void *party) {
    GFL_ArcSysRead(party, ARCID_TRPOKE, trainerId);
}

u8 TrainerClass_GetSex(u32 trainerClass) {
    return TRAINER_CLASS_RESOURCES[trainerClass].sex;
}

u8 GetTrSpriteBaseDatID(u32 trainerClass, u32 flags) {
    u32 i;

    if (flags & TRAINER_SPRITE_BACK) {
        for (i = 0; i < NELEMS(TRAINER_CLASS_BACK_SPRITES); i++) {
            if (trainerClass == TRAINER_CLASS_BACK_SPRITES[i].trainerClass) {
                return TRAINER_CLASS_BACK_SPRITES[i].sprite;
            }
        }
        // Any other class has the player's back: a female class Nate's and a male class Rosa's
        return TrainerClass_GetSex(trainerClass) == 1 ? TRAINER_BACK_SPRITE_PKMN_TRAINER_NATE
                                                      : TRAINER_BACK_SPRITE_PKMN_TRAINER_ROSA;
    }
    if (trainerClass < TRAINER_CLASS_COUNT) {
        return TRAINER_CLASS_RESOURCES[trainerClass].sprite;
    }
    return 0;
}

u16 GetTrainerClassEyeBGMID(u32 trainerClass) {
    return TRAINER_CLASS_RESOURCES[trainerClass].eyeBGM;
}

u8 GetTrainerClassSpecialId(u32 trainerClass) {
    int i;

    for (i = 0; i < TRAINER_CLASS_SPECIAL_COUNT; i++) {
        if (trainerClass == SPECIAL_TRAINER_CLASSES[i].trainerClass) {
            return i;
        }
    }
    return TRAINER_CLASS_SPECIAL_COUNT;
}

u32 GetTrainerClassBGMGroupId(u32 trainerClass) {
    u8 id = GetTrainerClassSpecialId(trainerClass);

    if (id >= TRAINER_CLASS_SPECIAL_COUNT) {
        return 15;
    }
    return SPECIAL_TRAINER_CLASSES[id].bgmGroup;
}

u32 GetTrainerClassBattlePedestal(u32 trainerClass) {
    u8 id = GetTrainerClassSpecialId(trainerClass);

    if (id >= TRAINER_CLASS_SPECIAL_COUNT) {
        return 20;
    }
    return SPECIAL_TRAINER_CLASSES[id].pedestal;
}

u32 CheckOverridenTrainerBattleBG(u32 trainerClass, u32 bg) {
    u32 i;

    for (i = 0; i < NELEMS(TR_CLASS_BATTLE_BG_OVERRIDES); i++) {
        if (trainerClass == TR_CLASS_BATTLE_BG_OVERRIDES[i].trainerClass) {
            return TR_CLASS_BATTLE_BG_OVERRIDES[i].bg;
        }
    }
    return bg;
}

void TrainerUtil_LoadTrainer(GameData *gameData, int trainerId, BtlSetupTrainer *trainer, HeapID heapId) {
    TrainerData *data = GFL_HeapAllocate(heapId, sizeof(TrainerData), FALSE, "tr_tool.c", 523);

    TrainerData_ReadTrainerData(trainerId, data);
    trainer->trainerId = trainerId;
    trainer->trainerClass = data->trainerClass;
    trainer->aiFlags = data->aiFlags;
    if (trainer->trainerClass == TRAINER_CLASS_PKMN_TRAINER_RIVAL) {
        GFL_StrBufLoadString(trainer->name, getPtrToRivalName(getHollow_RivalData(GameData_GetSaveControl(gameData))));
    } else {
        MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TRNAME_FILE, heapId);

        GFL_MsgDataLoadStrbuf(msgData, trainerId, trainer->name);
        GFL_MsgDataFree(msgData);
    }
    MIi_CpuCopy16(data->items, trainer->items, sizeof(trainer->items));
    PMSData_Clear(&trainer->unk18);
    PMSData_Clear(&trainer->unk20);
    GFL_HeapFree(data);
}

void TrainerUtil_LoadParty(int trainerId, PokeParty *party, HeapID heapId) {
    TrainerData *data;
    void *pokes;
    PartyPkm *pkm;
    u32 pidBase;
#ifdef BUGFIX
    u32 trainerPidBase;
#endif
    MATHRandContext32 rand;
    u32 rnd;
    u8 iv;
    int i;
    int j;

    PokeParty_InitCore(party, 6);
    data = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(TrainerData), FALSE, "tr_tool.c", 573);
    pokes = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(TrainerPokeItemMoves) * 6, FALSE, "tr_tool.c", 574);
    pkm = GFL_HeapAllocate(HEAPID_TAIL(heapId), PokeParty_GetPkmRawSize(), FALSE, "tr_tool.c", 575);
    TrainerData_ReadTrainerData(trainerId, data);
    TrainerData_ReadParty(trainerId, pokes);
    // BUG: TrainerUtil_CalcBasePID only changes the PID base for a Pokémon that asks for a sex or an ability, and the
    // base is never reset, so a later Pokémon that asks for neither keeps the one before it. Grimsley's Liepard asks to
    // be female, and his Scrafty, Krookodile and Bisharp, which ask only for an ability, come out female too.
    pidBase = TrainerClass_GetSex(data->trainerClass) == 1 ? PID_BASE_FEMALE_TRAINER : PID_BASE_MALE_TRAINER;
#ifdef BUGFIX
    trainerPidBase = pidBase;
#endif

    switch (data->partyKind) {
    case 0: {
        TrainerPoke *poke = pokes;

        for (i = 0; i < data->pokeCount; i++) {
#ifdef BUGFIX
            pidBase = trainerPidBase;
#endif
            TrainerUtil_CalcBasePID(poke[i].species, poke[i].form, poke[i].genderAbility, &pidBase, heapId);
            rnd = poke[i].difficulty + poke[i].level + poke[i].species + trainerId;
            MATH_InitRand32(&rand, rnd);
            for (j = 0; j < data->trainerClass; j++) {
                rnd = MATH_Rand32(&rand, 0x10000);
            }
            rnd = (rnd << 8) + pidBase;
            iv = poke[i].difficulty * 31 / 255;
            PokeParty_CreatePkm(pkm, poke[i].species, poke[i].level, PKM_ID_NOT_SHINY, PACK_IVS(iv, iv, iv, iv, iv, iv),
                                rnd);
            TrainerUtil_SetupPkm(trainerId, pkm, poke[i].form, poke[i].genderAbility);
            PokeParty_AddPkm(party, pkm);
        }
        break;
    }
    case PARTY_MOVES: {
        TrainerPokeMoves *poke = pokes;

        for (i = 0; i < data->pokeCount; i++) {
#ifdef BUGFIX
            pidBase = trainerPidBase;
#endif
            TrainerUtil_CalcBasePID(poke[i].base.species, poke[i].base.form, poke[i].base.genderAbility, &pidBase,
                                    heapId);
            rnd = poke[i].base.difficulty + poke[i].base.level + poke[i].base.species + trainerId;
            MATH_InitRand32(&rand, rnd);
            for (j = 0; j < data->trainerClass; j++) {
                rnd = MATH_Rand32(&rand, 0x10000);
            }
            rnd = (rnd << 8) + pidBase;
            iv = poke[i].base.difficulty * 31 / 255;
            PokeParty_CreatePkm(pkm, poke[i].base.species, poke[i].base.level, PKM_ID_NOT_SHINY,
                                PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
            for (j = 0; j < 4; j++) {
                PokeParty_SetMove(pkm, poke[i].moves[j], j);
            }
            TrainerUtil_SetupPkm(trainerId, pkm, poke[i].base.form, poke[i].base.genderAbility);
            PokeParty_AddPkm(party, pkm);
        }
        break;
    }
    case PARTY_ITEMS: {
        TrainerPokeItem *poke = pokes;

        for (i = 0; i < data->pokeCount; i++) {
#ifdef BUGFIX
            pidBase = trainerPidBase;
#endif
            TrainerUtil_CalcBasePID(poke[i].base.species, poke[i].base.form, poke[i].base.genderAbility, &pidBase,
                                    heapId);
            rnd = poke[i].base.difficulty + poke[i].base.level + poke[i].base.species + trainerId;
            MATH_InitRand32(&rand, rnd);
            for (j = 0; j < data->trainerClass; j++) {
                rnd = MATH_Rand32(&rand, 0x10000);
            }
            rnd = (rnd << 8) + pidBase;
            iv = poke[i].base.difficulty * 31 / 255;
            PokeParty_CreatePkm(pkm, poke[i].base.species, poke[i].base.level, PKM_ID_NOT_SHINY,
                                PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
            PokeParty_SetParam(pkm, PKM_PARAM_ITEM, poke[i].item);
            TrainerUtil_SetupPkm(trainerId, pkm, poke[i].base.form, poke[i].base.genderAbility);
            PokeParty_AddPkm(party, pkm);
        }
        break;
    }
    case PARTY_MOVES | PARTY_ITEMS: {
        TrainerPokeItemMoves *poke = pokes;

        for (i = 0; i < data->pokeCount; i++) {
#ifdef BUGFIX
            pidBase = trainerPidBase;
#endif
            TrainerUtil_CalcBasePID(poke[i].base.species, poke[i].base.form, poke[i].base.genderAbility, &pidBase,
                                    heapId);
            rnd = poke[i].base.difficulty + poke[i].base.level + poke[i].base.species + trainerId;
            MATH_InitRand32(&rand, rnd);
            for (j = 0; j < data->trainerClass; j++) {
                rnd = MATH_Rand32(&rand, 0x10000);
            }
            rnd = (rnd << 8) + pidBase;
            iv = poke[i].base.difficulty * 31 / 255;
            PokeParty_CreatePkm(pkm, poke[i].base.species, poke[i].base.level, PKM_ID_NOT_SHINY,
                                PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
            PokeParty_SetParam(pkm, PKM_PARAM_ITEM, poke[i].item);
            for (j = 0; j < 4; j++) {
                PokeParty_SetMove(pkm, poke[i].moves[j], j);
            }
            TrainerUtil_SetupPkm(trainerId, pkm, poke[i].base.form, poke[i].base.genderAbility);
            PokeParty_AddPkm(party, pkm);
        }
        break;
    }
    }
    GFL_HeapFree(data);
    GFL_HeapFree(pokes);
    GFL_HeapFree(pkm);
}

// Sets the low byte of the PID so that it gives the Pokémon the sex and ability of genderAbility
static void TrainerUtil_CalcBasePID(u32 species, u32 form, u8 genderAbility, u32 *pidBase, HeapID heapId) {
    int sex = genderAbility & 0xf;
    int ability = (genderAbility & 0xf0) >> 4;

    if (genderAbility != 0) {
        PersonalData *personal = PML_PersonalLoad(species, form, heapId);

        if (sex != 0) {
            *pidBase = PML_PersonalGetParam(personal, PERSONAL_SEX_RATIO);
            if (sex == 1) {
                *pidBase += 2;
            } else {
                *pidBase -= 2;
            }
        }
        if (ability == 1) {
            *pidBase &= ~1;
        } else if (ability == 2) {
            *pidBase |= 1;
        }
        PML_PersonalFree(personal);
    }
}

// Sets what CreatePkm doesn't: the friendship, 0 if the Pokémon knows Frustration, the form and the ability
static void TrainerUtil_SetupPkm(int trainerId, PartyPkm *pkm, u32 form, u8 genderAbility) {
    u16 species;
    u8 happiness = 255;
    int i;
    u32 ability;

    species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    for (i = 0; i < 4; i++) {
        if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL) == MOVE_FRUSTRATION) {
            happiness = 0;
        }
    }
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, happiness);
    PokeParty_SetParam(pkm, PKM_PARAM_FORM, form);

    ability = genderAbility & 0xf0;
    if (ability == 0x30) {
        PokeParty_SetHiddenAbil(pkm, species, form);
    } else if (ability != 0) {
        u32 param = PERSONAL_ABILITY_1;

        if (PML_PersonalGetParamSingle(species, form, PERSONAL_ABILITY_2) != 0 && ability == 0x20) {
            param = PERSONAL_ABILITY_2;
        }
        PokeParty_SetParam(pkm, PKM_PARAM_ABILITY, PML_PersonalGetParamSingle(species, form, param));
    }
    PokeParty_SetNature(pkm, (u8)(((int)PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL) >> 8) % 25));
}

static u8 TrainerUtil_IsIDBattleInst(int trainerId) {
    return trainerId >= TRAINER_ID_BATTLE_INST_START;
}
