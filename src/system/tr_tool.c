#include "types.h"
#include "battle/btl_setup.h"
#include "battle/trainer_data.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/trainer_classes.h"
#include "constants/trainer_messages.h"
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
    { TRAINER_CLASS_PKMN_TRAINER_NATE, 0 },
    { TRAINER_CLASS_PKMN_TRAINER_ROSA, 1 },
    { TRAINER_CLASS_PKMN_TRAINER_CHILI, 2 },
    { TRAINER_CLASS_PKMN_TRAINER_CILAN, 3 },
    { TRAINER_CLASS_PKMN_TRAINER_CRESS, 4 },
    { TRAINER_CLASS_NO_DATA_M_37, 5 },
    { TRAINER_CLASS_PKMN_TRAINER_BIANCA, 6 },
    { TRAINER_CLASS_DOCTOR, 7 },
    { TRAINER_CLASS_PKMN_TRAINER_RIVAL, 8 },
    { TRAINER_CLASS_PKMN_TRAINER_M_180, 9 },
    { TRAINER_CLASS_PKMN_TRAINER_F_181, 10 },
    { TRAINER_CLASS_PKMN_TRAINER_M_182, 11 },
    { TRAINER_CLASS_PKMN_TRAINER_F_183, 12 },
    { TRAINER_CLASS_PKMN_TRAINER_M_184, 13 },
    { TRAINER_CLASS_PKMN_TRAINER_F_185, 14 },
    { TRAINER_CLASS_CHAMPION, 15 },
    { TRAINER_CLASS_PKMN_TRAINER_CHEREN, 16 },
    { TRAINER_CLASS_PKMN_TRAINER_F_198, 15 },
    { TRAINER_CLASS_HERO, 17 },
    { TRAINER_CLASS_HEROINE, 18 },
    { TRAINER_CLASS_NINJA_M, 19 },
    { TRAINER_CLASS_NINJA_F, 20 },
    { TRAINER_CLASS_UDF_TROOPER_M, 21 },
    { TRAINER_CLASS_UDF_TROOPER_F, 22 },
    { TRAINER_CLASS_SCIENTIST_M_205, 23 },
    { TRAINER_CLASS_SCIENTIST_F_206, 24 },
    { TRAINER_CLASS_GUY, 25 },
    { TRAINER_CLASS_GIRL, 26 },
    { TRAINER_CLASS_PRINCE, 27 },
    { TRAINER_CLASS_PRINCESS, 28 },
    { TRAINER_CLASS_GUY_IN_SUIT_225, 29 },
    { TRAINER_CLASS_GIRL_IN_SUIT_226, 30 },
};

static const TrClassResource TRAINER_CLASS_RESOURCES[TRAINER_CLASS_COUNT] = {
    [TRAINER_CLASS_PKMN_TRAINER_NATE] = { 0, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_ROSA] = { 1, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_YOUNGSTER] = { 2, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LASS] = { 3, 1, SEQ_BGM_EYE_02 },
    [TRAINER_CLASS_SCHOOL_KID_M] = { 4, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SCHOOL_KID_F] = { 5, 1, SEQ_BGM_EYE_02 },
    [TRAINER_CLASS_SMASHER] = { 6, 1, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_LINEBACKER] = { 7, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_WAITER] = { 8, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_WAITRESS] = { 9, 1, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_PKMN_TRAINER_CHILI] = { 10, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CILAN] = { 11, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CRESS] = { 12, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NURSERY_AIDE] = { 13, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_PRESCHOOLER_F] = { 14, 1, SEQ_BGM_EYE_03 },
    [TRAINER_CLASS_PRESCHOOLER_M] = { 15, 0, SEQ_BGM_EYE_03 },
    [TRAINER_CLASS_TWINS] = { 16, 1, SEQ_BGM_EYE_03 },
    [TRAINER_CLASS_PKMN_BREEDER_M] = { 17, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_BREEDER_F] = { 18, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_LENORA] = { 19, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_20] = { 20, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_21] = { 21, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_22] = { 22, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_23] = { 23, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_RANGER_M] = { 24, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_RANGER_F] = { 25, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_WORKER_26] = { 26, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_BACKPACKER_M] = { 27, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_BACKPACKER_F] = { 28, 1, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_FISHERMAN] = { 29, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_MUSICIAN] = { 30, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_DANCER] = { 31, 0, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_HARLEQUIN_32] = { 32, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_ARTIST] = { 33, 0, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_BAKER] = { 34, 1, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_PSYCHIC_M] = { 35, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PSYCHIC_F] = { 36, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_NO_DATA_M_37] = { 37, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_BIANCA] = { 38, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_39] = { 39, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_N] = { 40, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_RICH_BOY] = { 41, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LADY] = { 42, 1, SEQ_BGM_EYE_02 },
    [TRAINER_CLASS_PILOT] = { 43, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_WORKER_44] = { 44, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_HOOPSTER] = { 45, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_SCIENTIST_F_46] = { 46, 1, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_NO_DATA_M_47] = { 40, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CLERK_F] = { 47, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_ACE_TRAINER_F] = { 48, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_ACE_TRAINER_M] = { 49, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BLACK_BELT] = { 50, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_SCIENTIST_M_52] = { 51, 0, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_STRIKER] = { 52, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_BRYCEN] = { 53, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_55] = { 54, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_56] = { 55, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ROUGHNECK] = { 56, 0, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_JANITOR] = { 57, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_POKEFAN_M] = { 58, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_POKEFAN_F] = { 59, 1, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_DOCTOR] = { 60, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_NURSE] = { 61, 1, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_HOOLIGANS] = { 62, 0, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_BATTLE_GIRL] = { 63, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PARASOL_LADY] = { 64, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_CLERK_M_66] = { 65, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_CLERK_M_67] = { 66, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_BACKERS_M] = { 67, 0, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_BACKERS_F] = { 68, 1, SEQ_BGM_EYE_10 },
    [TRAINER_CLASS_VETERAN_M] = { 69, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_VETERAN_F] = { 70, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BIKER] = { 71, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_INFIELDER] = { 72, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_HIKER] = { 73, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_SOCIALITE] = { 74, 1, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_GENTLEMAN] = { 75, 0, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_NO_DATA_F_77] = { 76, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_SHAUNTAL] = { 77, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_MARSHAL] = { 78, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_GRIMSLEY] = { 79, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ELITE_FOUR_CAITLIN] = { 80, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_82] = { 81, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_DEPOT_AGENT] = { 82, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_SWIMMER_M] = { 83, 0, SEQ_BGM_EYE_07 },
    [TRAINER_CLASS_SWIMMER_F] = { 84, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_POLICEMAN] = { 85, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_MAID] = { 86, 1, SEQ_BGM_EYE_13 },
    [TRAINER_CLASS_SUBWAY_BOSS_88] = { 87, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_ALDER] = { 88, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CYCLIST_M] = { 89, 0, SEQ_BGM_EYE_07 },
    [TRAINER_CLASS_CYCLIST_F] = { 90, 1, SEQ_BGM_EYE_07 },
    [TRAINER_CLASS_MOTORCYCLIST] = { 71, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_NO_DATA_M_93] = { 4, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_94] = { 70, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_F_95] = { 74, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_96] = { 75, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GAME_FREAK_MORIMOTO] = { 69, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_NO_DATA_F_98] = { 42, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_99] = { 41, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CYNTHIA] = { 91, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA] = { 40, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SUBWAY_BOSS_102] = { 92, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_103] = { 93, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_104] = { 51, 0, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_PKMN_TRAINER_M_105] = { 73, 0, SEQ_BGM_EYE_11 },
    [TRAINER_CLASS_PKMN_TRAINER_M_106] = { 35, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_F_107] = { 36, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_M_108] = { 69, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_TRAINER_F_109] = { 70, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_TRAINER_M_110] = { 49, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_PKMN_TRAINER_F_111] = { 48, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_LEADER_ELESA] = { 94, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_BURGH] = { 95, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_SKYLA] = { 96, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_CHEREN] = { 97, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_ROXIE] = { 98, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_CLAY] = { 99, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_DRAYDEN] = { 100, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LEADER_MARLON] = { 101, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BROCK] = { 102, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MISTY] = { 103, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LT_SURGE] = { 104, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ERIKA] = { 105, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SABRINA] = { 106, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BLAINE] = { 107, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIOVANNI] = { 108, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_FALKNER] = { 109, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BUGSY] = { 110, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WHITNEY] = { 111, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MORTY] = { 112, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CHUCK] = { 113, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_JASMINE] = { 114, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PRYCE] = { 115, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CLAIR] = { 116, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_JANINE] = { 117, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ROXANNE] = { 118, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BRAWLY] = { 119, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WATTSON] = { 120, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_FLANNERY] = { 121, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NORMAN] = { 122, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WINONA] = { 123, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TATE] = { 124, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LIZA] = { 125, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_JUAN] = { 126, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_RIVAL] = { 127, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUITARIST] = { 128, 1, SEQ_BGM_EYE_05 },
    [TRAINER_CLASS_BOSS_TRAINER_BRENT] = { 17, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_ABED] = { 17, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_ABIGAIL] = { 18, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_BOSS_TRAINER_BESS] = { 18, 1, SEQ_BGM_EYE_06 },
    [TRAINER_CLASS_BOSS_TRAINER_DANEIL] = { 24, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_CARLEN] = { 24, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_CARLEIGH] = { 25, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_DANELLE] = { 25, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_SUBWAY_BOSS_INGO] = { 87, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_156] = { 87, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_FREDERICK] = { 49, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_GAIUS] = { 49, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_GAIL] = { 48, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_FREIRA] = { 48, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_M_161] = { 69, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_M_162] = { 69, 0, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_F_163] = { 70, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_BOSS_TRAINER_F_164] = { 70, 1, SEQ_BGM_EYE_04 },
    [TRAINER_CLASS_SUBWAY_BOSS_EMMET] = { 92, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NO_DATA_M_166] = { 92, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ROARK] = { 129, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GARDENIA] = { 130, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_FANTINA] = { 131, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MAYLENE] = { 132, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WAKE] = { 133, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BYRON] = { 134, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CANDICE] = { 135, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_VOLKNER] = { 136, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BLUE] = { 137, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_LANCE] = { 138, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_STEVEN] = { 139, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WALLACE] = { 140, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BEAUTY] = { 141, 1, SEQ_BGM_EYE_DANCER },
    [TRAINER_CLASS_PKMN_TRAINER_M_180] = { 142, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_181] = { 143, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_182] = { 144, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_183] = { 145, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_184] = { 146, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_185] = { 147, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_COLRESS] = { 148, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_GRUNT_M] = { 149, 0, SEQ_BGM_EYE_NEO_PLASMA },
    [TRAINER_CLASS_TEAM_PLASMA_GRUNT_F] = { 150, 1, SEQ_BGM_EYE_NEO_PLASMA },
    [TRAINER_CLASS_TEAM_PLASMA_GHETSIS] = { 151, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_ROOD] = { 152, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_ZINZOLIN] = { 153, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_TEAM_PLASMA_SHADOW] = { 154, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CHAMPION] = { 155, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MASKED_MAN] = { 156, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_COLRESS_195] = { 148, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GAME_FREAK_NISHINO] = { 73, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_CHEREN] = { 97, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_F_198] = { 155, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_HERO] = { 157, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_HEROINE] = { 158, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NINJA_M] = { 159, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_NINJA_F] = { 160, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_UDF_TROOPER_M] = { 161, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_UDF_TROOPER_F] = { 162, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SCIENTIST_M_205] = { 163, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_SCIENTIST_F_206] = { 164, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUY] = { 165, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIRL] = { 166, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PRINCE] = { 167, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PRINCESS] = { 168, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BRYCEN_JET] = { 169, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_MECHACOMBO] = { 170, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_UFO] = { 171, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_ALIEN] = { 172, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_RED_FOG] = { 173, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CURSED_MAN] = { 174, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CURSED_KID] = { 175, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_OLD_STATUE] = { 176, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_CURSED_GENT] = { 177, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_DIVINE_MAID] = { 178, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_WITCH] = { 179, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PLUSH_TOY] = { 180, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUY_IN_SUIT_223] = { 181, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIRL_IN_SUIT_224] = { 182, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GUY_IN_SUIT_225] = { 183, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_GIRL_IN_SUIT_226] = { 184, 1, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_BOSS_TRAINER_BENGA] = { 185, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_M_228] = { 50, 0, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_F_229] = { 63, 1, SEQ_BGM_EYE_08 },
    [TRAINER_CLASS_PKMN_TRAINER_F_230] = { 46, 1, SEQ_BGM_EYE_09 },
    [TRAINER_CLASS_NO_DATA_M_231] = { 43, 0, SEQ_BGM_EYE_12 },
    [TRAINER_CLASS_FUTURE_BEING] = { 186, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_HARLEQUIN_233] = { 32, 0, SEQ_BGM_EYE_CLOWN },
    [TRAINER_CLASS_RED] = { 187, 0, SEQ_BGM_EYE_01 },
    [TRAINER_CLASS_PKMN_TRAINER_COLRESS_235] = { 148, 0, SEQ_BGM_EYE_01 },
};

u32 TrainerData_GetParam(int trainerId, u32 param) {
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
        return TrainerClass_GetSex(trainerClass) == 1 ? 0 : 1;
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
    sys_memcpy16(data->items, trainer->items, sizeof(trainer->items));
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
            PokeParty_CreatePkm(pkm, poke[i].species, poke[i].level, PKM_ID_NOT_SHINY, PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
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
            PokeParty_CreatePkm(pkm, poke[i].base.species, poke[i].base.level, PKM_ID_NOT_SHINY, PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
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
            PokeParty_CreatePkm(pkm, poke[i].base.species, poke[i].base.level, PKM_ID_NOT_SHINY, PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
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
            PokeParty_CreatePkm(pkm, poke[i].base.species, poke[i].base.level, PKM_ID_NOT_SHINY, PACK_IVS(iv, iv, iv, iv, iv, iv), rnd);
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
        void *personal = PML_PersonalLoad(species, form, heapId);

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
