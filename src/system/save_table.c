#include "types.h"
#include "save/save_table.h"
#include "field/field_actor.h"
#include "field/unity_tower.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/bag.h"
#include "save/battle_examination.h"
#include "save/battle_rec.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/c_gear_savedata.h"
#include "save/cgear_picture_save.h"
#include "save/chatter.h"
#include "save/cygnus_save_control.h"
#include "save/dream_world.h"
#include "save/encounter.h"
#include "save/event_work.h"
#include "save/extra_block_savedata.h"
#include "save/field_actor_savedata.h"
#include "save/funfest_savedata.h"
#include "save/gimmick_savedata.h"
#include "save/hall_of_fame.h"
#include "save/high_link.h"
#include "save/join_avenue.h"
#include "save/key_data_savedata.h"
#include "save/medal_box.h"
#include "save/misc_savedata.h"
#include "save/musical_dist_save.h"
#include "save/musical_savedata.h"
#include "save/mystery_gift.h"
#include "save/player_savedata.h"
#include "save/pokedex.h"
#include "save/pokedex_skin_save.h"
#include "save/pokewood.h"
#include "save/records.h"
#include "save/resh_zek.h"
#include "save/save_control.h"
#include "save/survey_savedata.h"
#include "save/symbol_save.h"
#include "save/timesig_savedata.h"
#include "save/tournament_block.h"
#include "save/traded_pokemon.h"
#include "save/trainer_data_savedata.h"
#include "save/trainer_game_info.h"
#include "save/trainer_log.h"
#include "save/trial_house.h"
#include "save/unity_tower_floor_savedata.h"
#include "save/wifi_list.h"
#include "save/worldtrade_data.h"
#include "system/pms_word.h"

// The tables of the saves' blocks: the main save's, and the one block of each extra save. The file has no code and no
// name in the ROM; save_table.c is a guess. That it is a file apart from save_control.c shows in the order of its
// tables, which MWCC only gives when the file defines nothing else.

// Where the saves are on the card, and the magic number in their footers
#define SAVE_MAIN_ADDR 0
#define SAVE_BACKUP_ADDR 0x26000
#define SAVE_SIZE 0x26000
#define SAVE_MAGIC 0x31053527

typedef void (*SaveBlockInit)(void *block);

static const SaveBlockDef SAVEDATA_INIT[SAVE_BLOCK_MAX] = {
    { SAVE_BLOCK_BOX_MAIN, BoxMainSave_GetBlockSize, (SaveBlockInit)BoxMainSave_InitBlock },
    { SAVE_BLOCK_BOX_TRAY + 0, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 1, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 2, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 3, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 4, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 5, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 6, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 7, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 8, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 9, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 10, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 11, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 12, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 13, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 14, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 15, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 16, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 17, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 18, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 19, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 20, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 21, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 22, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BOX_TRAY + 23, getSizeofBox, initFullBox },
    { SAVE_BLOCK_BAG, BagSave_GetBlockSize, (SaveBlockInit)BagSave_Init },
    { SAVE_BLOCK_PARTY, PokeParty_GetSaveDataSize, (SaveBlockInit)PokeParty_Init },
    { SAVE_BLOCK_TRAINER_DATA, getSizeofTrainerData, (SaveBlockInit)func_02008da4 },
    { SAVE_BLOCK_PLAYER, PlayerSave_GetBlockSize, (SaveBlockInit)PlayerSave_Init },
    { SAVE_BLOCK_UNITY_TOWER_SURVEY, func_02009b5c, (SaveBlockInit)func_02009b64 },
    { SAVE_BLOCK_WIFI_LIST, getSizeofWifiData, (SaveBlockInit)initWifi },
    { SAVE_BLOCK_PAL_PAD, func_0200a4b8, (SaveBlockInit)func_0200a4c0 },
    { SAVE_BLOCK_CGEAR, func_02009854, (SaveBlockInit)func_02009858 },
    { SAVE_BLOCK_TIMESIG, func_02009184, (SaveBlockInit)func_0200918c },
    { SAVE_BLOCK_MYSTERY_GIFT, func_0200a5f0, (SaveBlockInit)func_0200a5f8 },
    { SAVE_BLOCK_DREAM_WORLD, func_02009930, (SaveBlockInit)func_02009938 },
    { SAVE_BLOCK_CHATTER, PerapVoice_GetSize, (SaveBlockInit)initChatotChatter },
    { SAVE_BLOCK_ADVENTURE, getSizeofAdventureDataBlk, (SaveBlockInit)initAdventureDataBlk },
    { SAVE_BLOCK_RECORDS, func_020093d0, (SaveBlockInit)initTrainerCardInfoBlk },
    { SAVE_BLOCK_PMS, PMSWordSave_GetSize, (SaveBlockInit)PMSWordSave_Init },
    { SAVE_BLOCK_MAIL, func_020097a0, (SaveBlockInit)func_020097a8 },
    { SAVE_BLOCK_FIELD_ACTORS, ActorSave_GetBlockSize, (SaveBlockInit)ActorSave_Init },
    { SAVE_BLOCK_MUSICAL, func_0200ac58, (SaveBlockInit)func_0200ac60 },
    { SAVE_BLOCK_RESH_ZEK, func_0200af74, (SaveBlockInit)func_0200af78 },
    { SAVE_BLOCK_TRAINER_LOG, func_0200b240, (SaveBlockInit)func_0200b248 },
    { SAVE_BLOCK_EVENT_WORK, EventWork_GetBlockSize, (SaveBlockInit)EventWork_Init },
    { SAVE_BLOCK_WORLDTRADE, WorldTradeData_GetBlockSize, (SaveBlockInit)WorldTradeData_Init },
    { SAVE_BLOCK_TOURNAMENT, getSizeofTourneyBlock, clearTournamentBlock },
    { SAVE_BLOCK_GIMMICK, GimmickSave_GetBlockSize, (SaveBlockInit)GimmickSave_Init },
    { SAVE_BLOCK_BATTLE_BOX, BattleBoxSave_GetSize, (SaveBlockInit)BattleBoxSave_Init },
    { SAVE_BLOCK_DAY_CARE, getSizeofBreedingBlock, (SaveBlockInit)setEggArea },
    // The only block whose functions are an overlay's, the field's (overlay 12)
    { SAVE_BLOCK_STRENGTH_ROCKS, func_ov012_02168064, (SaveBlockInit)func_ov012_0216806c },
    { SAVE_BLOCK_TRAINER_GAME_INFO, TrainerGameInfo_GetSize, (SaveBlockInit)TrainerGameInfo_Init },
    { SAVE_BLOCK_HIGH_LINK, getSizeofProbabilityTable, (SaveBlockInit)probability },
    { SAVE_BLOCK_POKEDEX, getSizeofPokedexData, (SaveBlockInit)initPokedex },
    { SAVE_BLOCK_ENCOUNT, EncountSave_GetBlockSize, (SaveBlockInit)EncountSave_Init },
    { SAVE_BLOCK_BSUBWAY_PLAY, BSubwayPlayData_GetSize, (SaveBlockInit)BSubwayPlayData_Init },
    { SAVE_BLOCK_BSUBWAY_SCORE, BSubwayScoreData_GetSize, (SaveBlockInit)BSubwayScoreData_Init },
    { SAVE_BLOCK_BSUBWAY_3A, BSubwayWifiData_GetSize, (SaveBlockInit)BSubwayWifiData_Init },
    { SAVE_BLOCK_RECORD, func_0200f29c, (SaveBlockInit)func_0200f2a0 },
    { SAVE_BLOCK_SYMBOL, getSizeofNpcData, (SaveBlockInit)initNpcData },
    { SAVE_BLOCK_SURVEY, func_0200ec00, (SaveBlockInit)func_0200ec08 },
    { SAVE_BLOCK_UNITY_TOWER_FLOOR, UnityTowerSave_GetBlockSize, (SaveBlockInit)UnityTowerSave_Init },
    { SAVE_BLOCK_TRIAL_HOUSE, func_0200f19c, (SaveBlockInit)func_0200f1a0 },
    { SAVE_BLOCK_40, HallOfFameTeam_GetSize, (SaveBlockInit)HallOfFameTeam_Init },
    { SAVE_BLOCK_TRADED_POKEMON, TradedPokemonSave_GetSize, (SaveBlockInit)TradedPokemonSave_Init },
    { SAVE_BLOCK_MISC, getSizeofHollow_RivalData, (SaveBlockInit)clearHollowRivalData },
    { SAVE_BLOCK_JOIN_AVENUE, func_0200fffc, (SaveBlockInit)func_02010004 },
    { SAVE_BLOCK_MEDAL_BOX, MedalBox_GetBlockSize, (SaveBlockInit)MedalBox_Init },
    { SAVE_BLOCK_KEY_DATA, func_02010230, (SaveBlockInit)func_02010234 },
    { SAVE_BLOCK_FUNFEST, func_02010dc0, (SaveBlockInit)func_02010dc4 },
    { SAVE_BLOCK_POKEWOOD, func_02010ff4, (SaveBlockInit)func_02010ffc },
    { SAVE_BLOCK_DREAM_RADAR, getDreamRadarSaveSize, (SaveBlockInit)initDreamRadarSave },
};

// The one block of each extra save
static const SaveBlockDef sExtraBattleRec0[] = {
    { 0, getSizeOfVSPlayer, (SaveBlockInit)clearVSPlayerBlk },
};

static const SaveBlockDef sExtraBattleRec1[] = {
    { 0, getSizeOfVSPlayer, (SaveBlockInit)clearVSPlayerBlk },
};

static const SaveBlockDef sExtraBattleRec2[] = {
    { 0, getSizeOfVSPlayer, (SaveBlockInit)clearVSPlayerBlk },
};

static const SaveBlockDef sExtraBattleRec3[] = {
    { 0, getSizeOfVSPlayer, (SaveBlockInit)clearVSPlayerBlk },
};

static const SaveBlockDef sExtraCGearPicture[] = {
    { 0, func_0200ce50, (SaveBlockInit)func_0200ce58 },
};

static const SaveBlockDef sExtraBattleExamination[] = {
    { 0, BattleExamination_GetSize, (SaveBlockInit)BattleExamination_Init },
};

static const SaveBlockDef sExtraMusicalDist[] = {
    { 0, func_0200cc94, func_0200cc9c },
};

static const SaveBlockDef sExtraPokedexSkin[] = {
    { 0, func_0200f164, func_0200f16c },
};

static const SaveBlockDef sExtraHallOfFame[] = {
    { 0, HallOfFameSave_GetSize, (SaveBlockInit)HallOfFameSave_Init },
};

static const SaveBlockDef sExtraPokewood[] = {
    { 0, getSizeOfPokewoodBlock, (SaveBlockInit)clearPokewoodBlock },
};

static const SaveBlockDef sExtraTournament0[] = {
    { 0, func_0201040c, func_02010414 },
};

static const SaveBlockDef sExtraTournament1[] = {
    { 0, func_0201040c, func_02010414 },
};

static const SaveBlockDef sExtraTournament2[] = {
    { 0, func_0201040c, func_02010414 },
};

static const SaveBlockDef sExtraBlock20[] = {
    { 0, func_020103f4, func_020103f8 },
};

const SaveDataParams save_block_data = {
    SAVEDATA_INIT, SAVE_BLOCK_MAX, SAVE_MAIN_ADDR, SAVE_BACKUP_ADDR, SAVE_SIZE, SAVE_MAGIC,
};

// The extra saves follow the main save's two copies. Only the Hall of Fame has a backup copy
const SaveDataParams sv_extra_blk_data[SAVE_EXTRA_ID_MAX] = {
    { sExtraBattleRec0, 1, 0x4c000, 0x4c000, 0x1a00, SAVE_MAGIC },
    { sExtraBattleRec1, 1, 0x4da00, 0x4da00, 0x1a00, SAVE_MAGIC },
    { sExtraBattleRec2, 1, 0x4f400, 0x4f400, 0x1a00, SAVE_MAGIC },
    { sExtraBattleRec3, 1, 0x50e00, 0x50e00, 0x1a00, SAVE_MAGIC },
    { sExtraCGearPicture, 1, 0x52800, 0x52800, 0x2800, SAVE_MAGIC },
    { sExtraBattleExamination, 1, 0x55000, 0x55000, 0x800, SAVE_MAGIC },
    { sExtraMusicalDist, 1, 0x55800, 0x55800, 0x18000, SAVE_MAGIC },
    { sExtraPokedexSkin, 1, 0x6d800, 0x6d800, 0x6800, SAVE_MAGIC },
    { sExtraHallOfFame, 1, 0x74000, 0x75800, 0x1800, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x77000, 0x77000, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x77600, 0x77600, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x77c00, 0x77c00, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x78200, 0x78200, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x78800, 0x78800, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x78e00, 0x78e00, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x79400, 0x79400, 0x600, SAVE_MAGIC },
    { sExtraPokewood, 1, 0x79a00, 0x79a00, 0x600, SAVE_MAGIC },
    { sExtraTournament0, 1, 0x7a000, 0x7a000, 0x1400, SAVE_MAGIC },
    { sExtraTournament1, 1, 0x7b400, 0x7b400, 0x1400, SAVE_MAGIC },
    { sExtraTournament2, 1, 0x7c800, 0x7c800, 0x1400, SAVE_MAGIC },
    { sExtraBlock20, 1, 0x7dc00, 0x7dc00, 0x400, SAVE_MAGIC },
};
