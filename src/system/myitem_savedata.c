#include "types.h"
#include "save/bag.h"
#include "constants/items.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/item.h"

// The bag's save block: the five pockets, the items the player moved to the Free Space, and the cursor memory of the
// bag and the battle bag. The name is the ROM's string. The functions' names are swan's, except where the header
// says otherwise

static void BagSave_DefragmentFreeSpace(BagSave *bag, BagItem *items, u32 max);
static u32 BagSave_GetPocketPtr(BagSave *bag, u32 pocket, BagItem **items, u32 *max);
static u32 BagSave_GetPocketPtrByItem(BagSave *bag, u32 item, BagItem **items, u32 *max);
static BagItem *BagSave_GetItemHandleAddCheckCore(BagItem *items, u32 max, u32 item, int count, u16 maxCount);
static BagItem *BagSave_GetItemHandleAddCheck(BagSave *bag, u32 item, int count);
static BagItem *BagSave_AddItemCore(BagSave *bag, u32 item, int count);
static BagItem *BagSave_GetItemHandleIfAmountCore(BagItem *items, u32 max, u32 item, u32 count);
static BagItem *BagSave_GetItemHandleIfAmount(BagSave *bag, u32 item, u32 count);
static void BagSave_MoveBetweenFreeSpace_(BagSave *bag, u16 item, BOOL toFreeSpace);
static BOOL BagSave_IsItemInDesignatedPocket(BagSave *bag, u16 item, u32 pocket);
static s32 BagSave_GetFreeSpaceItemCount(BagSave *bag);
static void BagSave_SwapItems(BagItem *a, BagItem *b);
static void BagSave_MoveLastToFirst(BagItem *items, u32 count);
static void BagSave_ClearEmpty(BagItem *items, u32 max);
static void BagSave_Defragment(BagItem *items, u32 max);
static u32 PML_ItemGetType(u32 item);
static void BagSave_ClearItemEntry(BagItem *slot);

// The DNA Splicers, by what they do
static const u16 sDnaSplicers[] = { ITEM_DNA_SPLICERS_FUSE, ITEM_DNA_SPLICERS_SEPARATE };

// The pocket of each item
static const u8 sItemPockets[ITEM_LAST + 1] = {
    BAG_POCKET_ITEMS, // ITEM_NONE
    BAG_POCKET_ITEMS, // ITEM_MASTER_BALL
    BAG_POCKET_ITEMS, // ITEM_ULTRA_BALL
    BAG_POCKET_ITEMS, // ITEM_GREAT_BALL
    BAG_POCKET_ITEMS, // ITEM_POKE_BALL
    BAG_POCKET_ITEMS, // ITEM_SAFARI_BALL
    BAG_POCKET_ITEMS, // ITEM_NET_BALL
    BAG_POCKET_ITEMS, // ITEM_DIVE_BALL
    BAG_POCKET_ITEMS, // ITEM_NEST_BALL
    BAG_POCKET_ITEMS, // ITEM_REPEAT_BALL
    BAG_POCKET_ITEMS, // ITEM_TIMER_BALL
    BAG_POCKET_ITEMS, // ITEM_LUXURY_BALL
    BAG_POCKET_ITEMS, // ITEM_PREMIER_BALL
    BAG_POCKET_ITEMS, // ITEM_DUSK_BALL
    BAG_POCKET_ITEMS, // ITEM_HEAL_BALL
    BAG_POCKET_ITEMS, // ITEM_QUICK_BALL
    BAG_POCKET_ITEMS, // ITEM_CHERISH_BALL
    BAG_POCKET_MEDICINE, // ITEM_POTION
    BAG_POCKET_MEDICINE, // ITEM_ANTIDOTE
    BAG_POCKET_MEDICINE, // ITEM_BURN_HEAL
    BAG_POCKET_MEDICINE, // ITEM_ICE_HEAL
    BAG_POCKET_MEDICINE, // ITEM_AWAKENING
    BAG_POCKET_MEDICINE, // ITEM_PARLYZ_HEAL
    BAG_POCKET_MEDICINE, // ITEM_FULL_RESTORE
    BAG_POCKET_MEDICINE, // ITEM_MAX_POTION
    BAG_POCKET_MEDICINE, // ITEM_HYPER_POTION
    BAG_POCKET_MEDICINE, // ITEM_SUPER_POTION
    BAG_POCKET_MEDICINE, // ITEM_FULL_HEAL
    BAG_POCKET_MEDICINE, // ITEM_REVIVE
    BAG_POCKET_MEDICINE, // ITEM_MAX_REVIVE
    BAG_POCKET_MEDICINE, // ITEM_FRESH_WATER
    BAG_POCKET_MEDICINE, // ITEM_SODA_POP
    BAG_POCKET_MEDICINE, // ITEM_LEMONADE
    BAG_POCKET_MEDICINE, // ITEM_MOOMOO_MILK
    BAG_POCKET_MEDICINE, // ITEM_ENERGY_POWDER
    BAG_POCKET_MEDICINE, // ITEM_ENERGY_ROOT
    BAG_POCKET_MEDICINE, // ITEM_HEAL_POWDER
    BAG_POCKET_MEDICINE, // ITEM_REVIVAL_HERB
    BAG_POCKET_MEDICINE, // ITEM_ETHER
    BAG_POCKET_MEDICINE, // ITEM_MAX_ETHER
    BAG_POCKET_MEDICINE, // ITEM_ELIXIR
    BAG_POCKET_MEDICINE, // ITEM_MAX_ELIXIR
    BAG_POCKET_MEDICINE, // ITEM_LAVA_COOKIE
    BAG_POCKET_MEDICINE, // ITEM_BERRY_JUICE
    BAG_POCKET_MEDICINE, // ITEM_SACRED_ASH
    BAG_POCKET_MEDICINE, // ITEM_HP_UP
    BAG_POCKET_MEDICINE, // ITEM_PROTEIN
    BAG_POCKET_MEDICINE, // ITEM_IRON
    BAG_POCKET_MEDICINE, // ITEM_CARBOS
    BAG_POCKET_MEDICINE, // ITEM_CALCIUM
    BAG_POCKET_MEDICINE, // ITEM_RARE_CANDY
    BAG_POCKET_MEDICINE, // ITEM_PP_UP
    BAG_POCKET_MEDICINE, // ITEM_ZINC
    BAG_POCKET_MEDICINE, // ITEM_PP_MAX
    BAG_POCKET_MEDICINE, // ITEM_OLD_GATEAU
    BAG_POCKET_ITEMS, // ITEM_GUARD_SPEC
    BAG_POCKET_ITEMS, // ITEM_DIRE_HIT
    BAG_POCKET_ITEMS, // ITEM_X_ATTACK
    BAG_POCKET_ITEMS, // ITEM_X_DEFEND
    BAG_POCKET_ITEMS, // ITEM_X_SPEED
    BAG_POCKET_ITEMS, // ITEM_X_ACCURACY
    BAG_POCKET_ITEMS, // ITEM_X_SPECIAL
    BAG_POCKET_ITEMS, // ITEM_X_SP_DEF
    BAG_POCKET_ITEMS, // ITEM_POKE_DOLL
    BAG_POCKET_ITEMS, // ITEM_FLUFFY_TAIL
    BAG_POCKET_ITEMS, // ITEM_BLUE_FLUTE
    BAG_POCKET_ITEMS, // ITEM_YELLOW_FLUTE
    BAG_POCKET_ITEMS, // ITEM_RED_FLUTE
    BAG_POCKET_ITEMS, // ITEM_BLACK_FLUTE
    BAG_POCKET_ITEMS, // ITEM_WHITE_FLUTE
    BAG_POCKET_ITEMS, // ITEM_SHOAL_SALT
    BAG_POCKET_ITEMS, // ITEM_SHOAL_SHELL
    BAG_POCKET_ITEMS, // ITEM_RED_SHARD
    BAG_POCKET_ITEMS, // ITEM_BLUE_SHARD
    BAG_POCKET_ITEMS, // ITEM_YELLOW_SHARD
    BAG_POCKET_ITEMS, // ITEM_GREEN_SHARD
    BAG_POCKET_ITEMS, // ITEM_SUPER_REPEL
    BAG_POCKET_ITEMS, // ITEM_MAX_REPEL
    BAG_POCKET_ITEMS, // ITEM_ESCAPE_ROPE
    BAG_POCKET_ITEMS, // ITEM_REPEL
    BAG_POCKET_ITEMS, // ITEM_SUN_STONE
    BAG_POCKET_ITEMS, // ITEM_MOON_STONE
    BAG_POCKET_ITEMS, // ITEM_FIRE_STONE
    BAG_POCKET_ITEMS, // ITEM_THUNDERSTONE
    BAG_POCKET_ITEMS, // ITEM_WATER_STONE
    BAG_POCKET_ITEMS, // ITEM_LEAF_STONE
    BAG_POCKET_ITEMS, // ITEM_TINY_MUSHROOM
    BAG_POCKET_ITEMS, // ITEM_BIG_MUSHROOM
    BAG_POCKET_ITEMS, // ITEM_PEARL
    BAG_POCKET_ITEMS, // ITEM_BIG_PEARL
    BAG_POCKET_ITEMS, // ITEM_STARDUST
    BAG_POCKET_ITEMS, // ITEM_STAR_PIECE
    BAG_POCKET_ITEMS, // ITEM_NUGGET
    BAG_POCKET_ITEMS, // ITEM_HEART_SCALE
    BAG_POCKET_ITEMS, // ITEM_HONEY
    BAG_POCKET_ITEMS, // ITEM_GROWTH_MULCH
    BAG_POCKET_ITEMS, // ITEM_DAMP_MULCH
    BAG_POCKET_ITEMS, // ITEM_STABLE_MULCH
    BAG_POCKET_ITEMS, // ITEM_GOOEY_MULCH
    BAG_POCKET_ITEMS, // ITEM_ROOT_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_CLAW_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_HELIX_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_DOME_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_OLD_AMBER
    BAG_POCKET_ITEMS, // ITEM_ARMOR_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_SKULL_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_RARE_BONE
    BAG_POCKET_ITEMS, // ITEM_SHINY_STONE
    BAG_POCKET_ITEMS, // ITEM_DUSK_STONE
    BAG_POCKET_ITEMS, // ITEM_DAWN_STONE
    BAG_POCKET_ITEMS, // ITEM_OVAL_STONE
    BAG_POCKET_ITEMS, // ITEM_ODD_KEYSTONE
    BAG_POCKET_ITEMS, // ITEM_GRISEOUS_ORB
    BAG_POCKET_ITEMS, // ITEM_UNUSED_113
    BAG_POCKET_ITEMS, // ITEM_UNUSED_114
    BAG_POCKET_ITEMS, // ITEM_UNUSED_115
    BAG_POCKET_ITEMS, // ITEM_DOUSE_DRIVE
    BAG_POCKET_ITEMS, // ITEM_SHOCK_DRIVE
    BAG_POCKET_ITEMS, // ITEM_BURN_DRIVE
    BAG_POCKET_ITEMS, // ITEM_CHILL_DRIVE
    BAG_POCKET_ITEMS, // ITEM_UNUSED_120
    BAG_POCKET_ITEMS, // ITEM_UNUSED_121
    BAG_POCKET_ITEMS, // ITEM_UNUSED_122
    BAG_POCKET_ITEMS, // ITEM_UNUSED_123
    BAG_POCKET_ITEMS, // ITEM_UNUSED_124
    BAG_POCKET_ITEMS, // ITEM_UNUSED_125
    BAG_POCKET_ITEMS, // ITEM_UNUSED_126
    BAG_POCKET_ITEMS, // ITEM_UNUSED_127
    BAG_POCKET_ITEMS, // ITEM_UNUSED_128
    BAG_POCKET_ITEMS, // ITEM_UNUSED_129
    BAG_POCKET_ITEMS, // ITEM_UNUSED_130
    BAG_POCKET_ITEMS, // ITEM_UNUSED_131
    BAG_POCKET_ITEMS, // ITEM_UNUSED_132
    BAG_POCKET_ITEMS, // ITEM_UNUSED_133
    BAG_POCKET_MEDICINE, // ITEM_SWEET_HEART
    BAG_POCKET_ITEMS, // ITEM_ADAMANT_ORB
    BAG_POCKET_ITEMS, // ITEM_LUSTROUS_ORB
    BAG_POCKET_ITEMS, // ITEM_GREET_MAIL
    BAG_POCKET_ITEMS, // ITEM_FAVORED_MAIL
    BAG_POCKET_ITEMS, // ITEM_RSVP_MAIL
    BAG_POCKET_ITEMS, // ITEM_THANKS_MAIL
    BAG_POCKET_ITEMS, // ITEM_INQUIRY_MAIL
    BAG_POCKET_ITEMS, // ITEM_LIKE_MAIL
    BAG_POCKET_ITEMS, // ITEM_REPLY_MAIL
    BAG_POCKET_ITEMS, // ITEM_BRIDGE_MAIL_S
    BAG_POCKET_ITEMS, // ITEM_BRIDGE_MAIL_D
    BAG_POCKET_ITEMS, // ITEM_BRIDGE_MAIL_T
    BAG_POCKET_ITEMS, // ITEM_BRIDGE_MAIL_V
    BAG_POCKET_ITEMS, // ITEM_BRIDGE_MAIL_M
    BAG_POCKET_BERRIES, // ITEM_CHERI_BERRY
    BAG_POCKET_BERRIES, // ITEM_CHESTO_BERRY
    BAG_POCKET_BERRIES, // ITEM_PECHA_BERRY
    BAG_POCKET_BERRIES, // ITEM_RAWST_BERRY
    BAG_POCKET_BERRIES, // ITEM_ASPEAR_BERRY
    BAG_POCKET_BERRIES, // ITEM_LEPPA_BERRY
    BAG_POCKET_BERRIES, // ITEM_ORAN_BERRY
    BAG_POCKET_BERRIES, // ITEM_PERSIM_BERRY
    BAG_POCKET_BERRIES, // ITEM_LUM_BERRY
    BAG_POCKET_BERRIES, // ITEM_SITRUS_BERRY
    BAG_POCKET_BERRIES, // ITEM_FIGY_BERRY
    BAG_POCKET_BERRIES, // ITEM_WIKI_BERRY
    BAG_POCKET_BERRIES, // ITEM_MAGO_BERRY
    BAG_POCKET_BERRIES, // ITEM_AGUAV_BERRY
    BAG_POCKET_BERRIES, // ITEM_IAPAPA_BERRY
    BAG_POCKET_BERRIES, // ITEM_RAZZ_BERRY
    BAG_POCKET_BERRIES, // ITEM_BLUK_BERRY
    BAG_POCKET_BERRIES, // ITEM_NANAB_BERRY
    BAG_POCKET_BERRIES, // ITEM_WEPEAR_BERRY
    BAG_POCKET_BERRIES, // ITEM_PINAP_BERRY
    BAG_POCKET_BERRIES, // ITEM_POMEG_BERRY
    BAG_POCKET_BERRIES, // ITEM_KELPSY_BERRY
    BAG_POCKET_BERRIES, // ITEM_QUALOT_BERRY
    BAG_POCKET_BERRIES, // ITEM_HONDEW_BERRY
    BAG_POCKET_BERRIES, // ITEM_GREPA_BERRY
    BAG_POCKET_BERRIES, // ITEM_TAMATO_BERRY
    BAG_POCKET_BERRIES, // ITEM_CORNN_BERRY
    BAG_POCKET_BERRIES, // ITEM_MAGOST_BERRY
    BAG_POCKET_BERRIES, // ITEM_RABUTA_BERRY
    BAG_POCKET_BERRIES, // ITEM_NOMEL_BERRY
    BAG_POCKET_BERRIES, // ITEM_SPELON_BERRY
    BAG_POCKET_BERRIES, // ITEM_PAMTRE_BERRY
    BAG_POCKET_BERRIES, // ITEM_WATMEL_BERRY
    BAG_POCKET_BERRIES, // ITEM_DURIN_BERRY
    BAG_POCKET_BERRIES, // ITEM_BELUE_BERRY
    BAG_POCKET_BERRIES, // ITEM_OCCA_BERRY
    BAG_POCKET_BERRIES, // ITEM_PASSHO_BERRY
    BAG_POCKET_BERRIES, // ITEM_WACAN_BERRY
    BAG_POCKET_BERRIES, // ITEM_RINDO_BERRY
    BAG_POCKET_BERRIES, // ITEM_YACHE_BERRY
    BAG_POCKET_BERRIES, // ITEM_CHOPLE_BERRY
    BAG_POCKET_BERRIES, // ITEM_KEBIA_BERRY
    BAG_POCKET_BERRIES, // ITEM_SHUCA_BERRY
    BAG_POCKET_BERRIES, // ITEM_COBA_BERRY
    BAG_POCKET_BERRIES, // ITEM_PAYAPA_BERRY
    BAG_POCKET_BERRIES, // ITEM_TANGA_BERRY
    BAG_POCKET_BERRIES, // ITEM_CHARTI_BERRY
    BAG_POCKET_BERRIES, // ITEM_KASIB_BERRY
    BAG_POCKET_BERRIES, // ITEM_HABAN_BERRY
    BAG_POCKET_BERRIES, // ITEM_COLBUR_BERRY
    BAG_POCKET_BERRIES, // ITEM_BABIRI_BERRY
    BAG_POCKET_BERRIES, // ITEM_CHILAN_BERRY
    BAG_POCKET_BERRIES, // ITEM_LIECHI_BERRY
    BAG_POCKET_BERRIES, // ITEM_GANLON_BERRY
    BAG_POCKET_BERRIES, // ITEM_SALAC_BERRY
    BAG_POCKET_BERRIES, // ITEM_PETAYA_BERRY
    BAG_POCKET_BERRIES, // ITEM_APICOT_BERRY
    BAG_POCKET_BERRIES, // ITEM_LANSAT_BERRY
    BAG_POCKET_BERRIES, // ITEM_STARF_BERRY
    BAG_POCKET_BERRIES, // ITEM_ENIGMA_BERRY
    BAG_POCKET_BERRIES, // ITEM_MICLE_BERRY
    BAG_POCKET_BERRIES, // ITEM_CUSTAP_BERRY
    BAG_POCKET_BERRIES, // ITEM_JABOCA_BERRY
    BAG_POCKET_BERRIES, // ITEM_ROWAP_BERRY
    BAG_POCKET_ITEMS, // ITEM_BRIGHT_POWDER
    BAG_POCKET_ITEMS, // ITEM_WHITE_HERB
    BAG_POCKET_ITEMS, // ITEM_MACHO_BRACE
    BAG_POCKET_ITEMS, // ITEM_EXP_SHARE
    BAG_POCKET_ITEMS, // ITEM_QUICK_CLAW
    BAG_POCKET_ITEMS, // ITEM_SOOTHE_BELL
    BAG_POCKET_ITEMS, // ITEM_MENTAL_HERB
    BAG_POCKET_ITEMS, // ITEM_CHOICE_BAND
    BAG_POCKET_ITEMS, // ITEM_KINGS_ROCK
    BAG_POCKET_ITEMS, // ITEM_SILVER_POWDER
    BAG_POCKET_ITEMS, // ITEM_AMULET_COIN
    BAG_POCKET_ITEMS, // ITEM_CLEANSE_TAG
    BAG_POCKET_ITEMS, // ITEM_SOUL_DEW
    BAG_POCKET_ITEMS, // ITEM_DEEP_SEA_TOOTH
    BAG_POCKET_ITEMS, // ITEM_DEEP_SEA_SCALE
    BAG_POCKET_ITEMS, // ITEM_SMOKE_BALL
    BAG_POCKET_ITEMS, // ITEM_EVERSTONE
    BAG_POCKET_ITEMS, // ITEM_FOCUS_BAND
    BAG_POCKET_ITEMS, // ITEM_LUCKY_EGG
    BAG_POCKET_ITEMS, // ITEM_SCOPE_LENS
    BAG_POCKET_ITEMS, // ITEM_METAL_COAT
    BAG_POCKET_ITEMS, // ITEM_LEFTOVERS
    BAG_POCKET_ITEMS, // ITEM_DRAGON_SCALE
    BAG_POCKET_ITEMS, // ITEM_LIGHT_BALL
    BAG_POCKET_ITEMS, // ITEM_SOFT_SAND
    BAG_POCKET_ITEMS, // ITEM_HARD_STONE
    BAG_POCKET_ITEMS, // ITEM_MIRACLE_SEED
    BAG_POCKET_ITEMS, // ITEM_BLACK_GLASSES
    BAG_POCKET_ITEMS, // ITEM_BLACK_BELT
    BAG_POCKET_ITEMS, // ITEM_MAGNET
    BAG_POCKET_ITEMS, // ITEM_MYSTIC_WATER
    BAG_POCKET_ITEMS, // ITEM_SHARP_BEAK
    BAG_POCKET_ITEMS, // ITEM_POISON_BARB
    BAG_POCKET_ITEMS, // ITEM_NEVER_MELT_ICE
    BAG_POCKET_ITEMS, // ITEM_SPELL_TAG
    BAG_POCKET_ITEMS, // ITEM_TWISTED_SPOON
    BAG_POCKET_ITEMS, // ITEM_CHARCOAL
    BAG_POCKET_ITEMS, // ITEM_DRAGON_FANG
    BAG_POCKET_ITEMS, // ITEM_SILK_SCARF
    BAG_POCKET_ITEMS, // ITEM_UP_GRADE
    BAG_POCKET_ITEMS, // ITEM_SHELL_BELL
    BAG_POCKET_ITEMS, // ITEM_SEA_INCENSE
    BAG_POCKET_ITEMS, // ITEM_LAX_INCENSE
    BAG_POCKET_ITEMS, // ITEM_LUCKY_PUNCH
    BAG_POCKET_ITEMS, // ITEM_METAL_POWDER
    BAG_POCKET_ITEMS, // ITEM_THICK_CLUB
    BAG_POCKET_ITEMS, // ITEM_STICK
    BAG_POCKET_ITEMS, // ITEM_RED_SCARF
    BAG_POCKET_ITEMS, // ITEM_BLUE_SCARF
    BAG_POCKET_ITEMS, // ITEM_PINK_SCARF
    BAG_POCKET_ITEMS, // ITEM_GREEN_SCARF
    BAG_POCKET_ITEMS, // ITEM_YELLOW_SCARF
    BAG_POCKET_ITEMS, // ITEM_WIDE_LENS
    BAG_POCKET_ITEMS, // ITEM_MUSCLE_BAND
    BAG_POCKET_ITEMS, // ITEM_WISE_GLASSES
    BAG_POCKET_ITEMS, // ITEM_EXPERT_BELT
    BAG_POCKET_ITEMS, // ITEM_LIGHT_CLAY
    BAG_POCKET_ITEMS, // ITEM_LIFE_ORB
    BAG_POCKET_ITEMS, // ITEM_POWER_HERB
    BAG_POCKET_ITEMS, // ITEM_TOXIC_ORB
    BAG_POCKET_ITEMS, // ITEM_FLAME_ORB
    BAG_POCKET_ITEMS, // ITEM_QUICK_POWDER
    BAG_POCKET_ITEMS, // ITEM_FOCUS_SASH
    BAG_POCKET_ITEMS, // ITEM_ZOOM_LENS
    BAG_POCKET_ITEMS, // ITEM_METRONOME
    BAG_POCKET_ITEMS, // ITEM_IRON_BALL
    BAG_POCKET_ITEMS, // ITEM_LAGGING_TAIL
    BAG_POCKET_ITEMS, // ITEM_DESTINY_KNOT
    BAG_POCKET_ITEMS, // ITEM_BLACK_SLUDGE
    BAG_POCKET_ITEMS, // ITEM_ICY_ROCK
    BAG_POCKET_ITEMS, // ITEM_SMOOTH_ROCK
    BAG_POCKET_ITEMS, // ITEM_HEAT_ROCK
    BAG_POCKET_ITEMS, // ITEM_DAMP_ROCK
    BAG_POCKET_ITEMS, // ITEM_GRIP_CLAW
    BAG_POCKET_ITEMS, // ITEM_CHOICE_SCARF
    BAG_POCKET_ITEMS, // ITEM_STICKY_BARB
    BAG_POCKET_ITEMS, // ITEM_POWER_BRACER
    BAG_POCKET_ITEMS, // ITEM_POWER_BELT
    BAG_POCKET_ITEMS, // ITEM_POWER_LENS
    BAG_POCKET_ITEMS, // ITEM_POWER_BAND
    BAG_POCKET_ITEMS, // ITEM_POWER_ANKLET
    BAG_POCKET_ITEMS, // ITEM_POWER_WEIGHT
    BAG_POCKET_ITEMS, // ITEM_SHED_SHELL
    BAG_POCKET_ITEMS, // ITEM_BIG_ROOT
    BAG_POCKET_ITEMS, // ITEM_CHOICE_SPECS
    BAG_POCKET_ITEMS, // ITEM_FLAME_PLATE
    BAG_POCKET_ITEMS, // ITEM_SPLASH_PLATE
    BAG_POCKET_ITEMS, // ITEM_ZAP_PLATE
    BAG_POCKET_ITEMS, // ITEM_MEADOW_PLATE
    BAG_POCKET_ITEMS, // ITEM_ICICLE_PLATE
    BAG_POCKET_ITEMS, // ITEM_FIST_PLATE
    BAG_POCKET_ITEMS, // ITEM_TOXIC_PLATE
    BAG_POCKET_ITEMS, // ITEM_EARTH_PLATE
    BAG_POCKET_ITEMS, // ITEM_SKY_PLATE
    BAG_POCKET_ITEMS, // ITEM_MIND_PLATE
    BAG_POCKET_ITEMS, // ITEM_INSECT_PLATE
    BAG_POCKET_ITEMS, // ITEM_STONE_PLATE
    BAG_POCKET_ITEMS, // ITEM_SPOOKY_PLATE
    BAG_POCKET_ITEMS, // ITEM_DRACO_PLATE
    BAG_POCKET_ITEMS, // ITEM_DREAD_PLATE
    BAG_POCKET_ITEMS, // ITEM_IRON_PLATE
    BAG_POCKET_ITEMS, // ITEM_ODD_INCENSE
    BAG_POCKET_ITEMS, // ITEM_ROCK_INCENSE
    BAG_POCKET_ITEMS, // ITEM_FULL_INCENSE
    BAG_POCKET_ITEMS, // ITEM_WAVE_INCENSE
    BAG_POCKET_ITEMS, // ITEM_ROSE_INCENSE
    BAG_POCKET_ITEMS, // ITEM_LUCK_INCENSE
    BAG_POCKET_ITEMS, // ITEM_PURE_INCENSE
    BAG_POCKET_ITEMS, // ITEM_PROTECTOR
    BAG_POCKET_ITEMS, // ITEM_ELECTIRIZER
    BAG_POCKET_ITEMS, // ITEM_MAGMARIZER
    BAG_POCKET_ITEMS, // ITEM_DUBIOUS_DISC
    BAG_POCKET_ITEMS, // ITEM_REAPER_CLOTH
    BAG_POCKET_ITEMS, // ITEM_RAZOR_CLAW
    BAG_POCKET_ITEMS, // ITEM_RAZOR_FANG
    BAG_POCKET_TMS_HMS, // ITEM_TM01
    BAG_POCKET_TMS_HMS, // ITEM_TM02
    BAG_POCKET_TMS_HMS, // ITEM_TM03
    BAG_POCKET_TMS_HMS, // ITEM_TM04
    BAG_POCKET_TMS_HMS, // ITEM_TM05
    BAG_POCKET_TMS_HMS, // ITEM_TM06
    BAG_POCKET_TMS_HMS, // ITEM_TM07
    BAG_POCKET_TMS_HMS, // ITEM_TM08
    BAG_POCKET_TMS_HMS, // ITEM_TM09
    BAG_POCKET_TMS_HMS, // ITEM_TM10
    BAG_POCKET_TMS_HMS, // ITEM_TM11
    BAG_POCKET_TMS_HMS, // ITEM_TM12
    BAG_POCKET_TMS_HMS, // ITEM_TM13
    BAG_POCKET_TMS_HMS, // ITEM_TM14
    BAG_POCKET_TMS_HMS, // ITEM_TM15
    BAG_POCKET_TMS_HMS, // ITEM_TM16
    BAG_POCKET_TMS_HMS, // ITEM_TM17
    BAG_POCKET_TMS_HMS, // ITEM_TM18
    BAG_POCKET_TMS_HMS, // ITEM_TM19
    BAG_POCKET_TMS_HMS, // ITEM_TM20
    BAG_POCKET_TMS_HMS, // ITEM_TM21
    BAG_POCKET_TMS_HMS, // ITEM_TM22
    BAG_POCKET_TMS_HMS, // ITEM_TM23
    BAG_POCKET_TMS_HMS, // ITEM_TM24
    BAG_POCKET_TMS_HMS, // ITEM_TM25
    BAG_POCKET_TMS_HMS, // ITEM_TM26
    BAG_POCKET_TMS_HMS, // ITEM_TM27
    BAG_POCKET_TMS_HMS, // ITEM_TM28
    BAG_POCKET_TMS_HMS, // ITEM_TM29
    BAG_POCKET_TMS_HMS, // ITEM_TM30
    BAG_POCKET_TMS_HMS, // ITEM_TM31
    BAG_POCKET_TMS_HMS, // ITEM_TM32
    BAG_POCKET_TMS_HMS, // ITEM_TM33
    BAG_POCKET_TMS_HMS, // ITEM_TM34
    BAG_POCKET_TMS_HMS, // ITEM_TM35
    BAG_POCKET_TMS_HMS, // ITEM_TM36
    BAG_POCKET_TMS_HMS, // ITEM_TM37
    BAG_POCKET_TMS_HMS, // ITEM_TM38
    BAG_POCKET_TMS_HMS, // ITEM_TM39
    BAG_POCKET_TMS_HMS, // ITEM_TM40
    BAG_POCKET_TMS_HMS, // ITEM_TM41
    BAG_POCKET_TMS_HMS, // ITEM_TM42
    BAG_POCKET_TMS_HMS, // ITEM_TM43
    BAG_POCKET_TMS_HMS, // ITEM_TM44
    BAG_POCKET_TMS_HMS, // ITEM_TM45
    BAG_POCKET_TMS_HMS, // ITEM_TM46
    BAG_POCKET_TMS_HMS, // ITEM_TM47
    BAG_POCKET_TMS_HMS, // ITEM_TM48
    BAG_POCKET_TMS_HMS, // ITEM_TM49
    BAG_POCKET_TMS_HMS, // ITEM_TM50
    BAG_POCKET_TMS_HMS, // ITEM_TM51
    BAG_POCKET_TMS_HMS, // ITEM_TM52
    BAG_POCKET_TMS_HMS, // ITEM_TM53
    BAG_POCKET_TMS_HMS, // ITEM_TM54
    BAG_POCKET_TMS_HMS, // ITEM_TM55
    BAG_POCKET_TMS_HMS, // ITEM_TM56
    BAG_POCKET_TMS_HMS, // ITEM_TM57
    BAG_POCKET_TMS_HMS, // ITEM_TM58
    BAG_POCKET_TMS_HMS, // ITEM_TM59
    BAG_POCKET_TMS_HMS, // ITEM_TM60
    BAG_POCKET_TMS_HMS, // ITEM_TM61
    BAG_POCKET_TMS_HMS, // ITEM_TM62
    BAG_POCKET_TMS_HMS, // ITEM_TM63
    BAG_POCKET_TMS_HMS, // ITEM_TM64
    BAG_POCKET_TMS_HMS, // ITEM_TM65
    BAG_POCKET_TMS_HMS, // ITEM_TM66
    BAG_POCKET_TMS_HMS, // ITEM_TM67
    BAG_POCKET_TMS_HMS, // ITEM_TM68
    BAG_POCKET_TMS_HMS, // ITEM_TM69
    BAG_POCKET_TMS_HMS, // ITEM_TM70
    BAG_POCKET_TMS_HMS, // ITEM_TM71
    BAG_POCKET_TMS_HMS, // ITEM_TM72
    BAG_POCKET_TMS_HMS, // ITEM_TM73
    BAG_POCKET_TMS_HMS, // ITEM_TM74
    BAG_POCKET_TMS_HMS, // ITEM_TM75
    BAG_POCKET_TMS_HMS, // ITEM_TM76
    BAG_POCKET_TMS_HMS, // ITEM_TM77
    BAG_POCKET_TMS_HMS, // ITEM_TM78
    BAG_POCKET_TMS_HMS, // ITEM_TM79
    BAG_POCKET_TMS_HMS, // ITEM_TM80
    BAG_POCKET_TMS_HMS, // ITEM_TM81
    BAG_POCKET_TMS_HMS, // ITEM_TM82
    BAG_POCKET_TMS_HMS, // ITEM_TM83
    BAG_POCKET_TMS_HMS, // ITEM_TM84
    BAG_POCKET_TMS_HMS, // ITEM_TM85
    BAG_POCKET_TMS_HMS, // ITEM_TM86
    BAG_POCKET_TMS_HMS, // ITEM_TM87
    BAG_POCKET_TMS_HMS, // ITEM_TM88
    BAG_POCKET_TMS_HMS, // ITEM_TM89
    BAG_POCKET_TMS_HMS, // ITEM_TM90
    BAG_POCKET_TMS_HMS, // ITEM_TM91
    BAG_POCKET_TMS_HMS, // ITEM_TM92
    BAG_POCKET_TMS_HMS, // ITEM_HM01
    BAG_POCKET_TMS_HMS, // ITEM_HM02
    BAG_POCKET_TMS_HMS, // ITEM_HM03
    BAG_POCKET_TMS_HMS, // ITEM_HM04
    BAG_POCKET_TMS_HMS, // ITEM_HM05
    BAG_POCKET_TMS_HMS, // ITEM_HM06
    BAG_POCKET_ITEMS, // ITEM_UNUSED_426
    BAG_POCKET_ITEMS, // ITEM_UNUSED_427
    BAG_POCKET_KEY_ITEMS, // ITEM_EXPLORER_KIT
    BAG_POCKET_KEY_ITEMS, // ITEM_LOOT_SACK
    BAG_POCKET_KEY_ITEMS, // ITEM_RULE_BOOK
    BAG_POCKET_KEY_ITEMS, // ITEM_POKE_RADAR
    BAG_POCKET_KEY_ITEMS, // ITEM_POINT_CARD
    BAG_POCKET_KEY_ITEMS, // ITEM_JOURNAL
    BAG_POCKET_KEY_ITEMS, // ITEM_SEAL_CASE
    BAG_POCKET_KEY_ITEMS, // ITEM_FASHION_CASE
    BAG_POCKET_KEY_ITEMS, // ITEM_SEAL_BAG
    BAG_POCKET_KEY_ITEMS, // ITEM_PAL_PAD
    BAG_POCKET_KEY_ITEMS, // ITEM_WORKS_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_OLD_CHARM
    BAG_POCKET_KEY_ITEMS, // ITEM_GALACTIC_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_RED_CHAIN
    BAG_POCKET_KEY_ITEMS, // ITEM_TOWN_MAP
    BAG_POCKET_KEY_ITEMS, // ITEM_VS_SEEKER
    BAG_POCKET_KEY_ITEMS, // ITEM_COIN_CASE
    BAG_POCKET_KEY_ITEMS, // ITEM_OLD_ROD
    BAG_POCKET_KEY_ITEMS, // ITEM_GOOD_ROD
    BAG_POCKET_KEY_ITEMS, // ITEM_SUPER_ROD
    BAG_POCKET_KEY_ITEMS, // ITEM_SPRAYDUCK
    BAG_POCKET_KEY_ITEMS, // ITEM_POFFIN_CASE
    BAG_POCKET_KEY_ITEMS, // ITEM_BICYCLE
    BAG_POCKET_KEY_ITEMS, // ITEM_SUITE_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_OAKS_LETTER
    BAG_POCKET_KEY_ITEMS, // ITEM_LUNAR_WING
    BAG_POCKET_KEY_ITEMS, // ITEM_MEMBER_CARD
    BAG_POCKET_KEY_ITEMS, // ITEM_AZURE_FLUTE
    BAG_POCKET_KEY_ITEMS, // ITEM_SS_TICKET
    BAG_POCKET_KEY_ITEMS, // ITEM_CONTEST_PASS
    BAG_POCKET_KEY_ITEMS, // ITEM_MAGMA_STONE
    BAG_POCKET_KEY_ITEMS, // ITEM_PARCEL
    BAG_POCKET_KEY_ITEMS, // ITEM_COUPON_1
    BAG_POCKET_KEY_ITEMS, // ITEM_COUPON_2
    BAG_POCKET_KEY_ITEMS, // ITEM_COUPON_3
    BAG_POCKET_KEY_ITEMS, // ITEM_STORAGE_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_SECRET_POTION
    BAG_POCKET_KEY_ITEMS, // ITEM_VS_RECORDER
    BAG_POCKET_KEY_ITEMS, // ITEM_GRACIDEA
    BAG_POCKET_KEY_ITEMS, // ITEM_SECRET_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_APRICORN_BOX
    BAG_POCKET_KEY_ITEMS, // ITEM_UNOWN_REPORT
    BAG_POCKET_KEY_ITEMS, // ITEM_BERRY_POTS
    BAG_POCKET_KEY_ITEMS, // ITEM_DOWSING_MCHN
    BAG_POCKET_KEY_ITEMS, // ITEM_BLUE_CARD
    BAG_POCKET_KEY_ITEMS, // ITEM_SLOWPOKE_TAIL
    BAG_POCKET_KEY_ITEMS, // ITEM_CLEAR_BELL
    BAG_POCKET_KEY_ITEMS, // ITEM_CARD_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_BASEMENT_KEY
    BAG_POCKET_KEY_ITEMS, // ITEM_SQUIRT_BOTTLE
    BAG_POCKET_KEY_ITEMS, // ITEM_RED_SCALE
    BAG_POCKET_KEY_ITEMS, // ITEM_LOST_ITEM
    BAG_POCKET_KEY_ITEMS, // ITEM_PASS
    BAG_POCKET_KEY_ITEMS, // ITEM_MACHINE_PART
    BAG_POCKET_KEY_ITEMS, // ITEM_SILVER_WING
    BAG_POCKET_KEY_ITEMS, // ITEM_RAINBOW_WING
    BAG_POCKET_KEY_ITEMS, // ITEM_MYSTERY_EGG
    BAG_POCKET_ITEMS, // ITEM_RED_APRICORN
    BAG_POCKET_ITEMS, // ITEM_BLU_APRICORN
    BAG_POCKET_ITEMS, // ITEM_YLW_APRICORN
    BAG_POCKET_ITEMS, // ITEM_GRN_APRICORN
    BAG_POCKET_ITEMS, // ITEM_PNK_APRICORN
    BAG_POCKET_ITEMS, // ITEM_WHT_APRICORN
    BAG_POCKET_ITEMS, // ITEM_BLK_APRICORN
    BAG_POCKET_ITEMS, // ITEM_FAST_BALL
    BAG_POCKET_ITEMS, // ITEM_LEVEL_BALL
    BAG_POCKET_ITEMS, // ITEM_LURE_BALL
    BAG_POCKET_ITEMS, // ITEM_HEAVY_BALL
    BAG_POCKET_ITEMS, // ITEM_LOVE_BALL
    BAG_POCKET_ITEMS, // ITEM_FRIEND_BALL
    BAG_POCKET_ITEMS, // ITEM_MOON_BALL
    BAG_POCKET_ITEMS, // ITEM_SPORT_BALL
    BAG_POCKET_ITEMS, // ITEM_PARK_BALL
    BAG_POCKET_KEY_ITEMS, // ITEM_PHOTO_ALBUM
    BAG_POCKET_KEY_ITEMS, // ITEM_GB_SOUNDS
    BAG_POCKET_KEY_ITEMS, // ITEM_TIDAL_BELL
    BAG_POCKET_MEDICINE, // ITEM_RAGE_CANDY_BAR
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_01
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_02
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_03
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_04
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_05
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_06
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_07
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_08
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_09
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_10
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_11
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_12
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_13
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_14
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_15
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_16
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_17
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_18
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_19
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_20
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_21
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_22
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_23
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_24
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_25
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_26
    BAG_POCKET_ITEMS, // ITEM_DATA_CARD_27
    BAG_POCKET_KEY_ITEMS, // ITEM_JADE_ORB
    BAG_POCKET_KEY_ITEMS, // ITEM_LOCK_CAPSULE
    BAG_POCKET_KEY_ITEMS, // ITEM_RED_ORB
    BAG_POCKET_KEY_ITEMS, // ITEM_BLUE_ORB
    BAG_POCKET_KEY_ITEMS, // ITEM_ENIGMA_STONE
    BAG_POCKET_ITEMS, // ITEM_PRISM_SCALE
    BAG_POCKET_ITEMS, // ITEM_EVIOLITE
    BAG_POCKET_ITEMS, // ITEM_FLOAT_STONE
    BAG_POCKET_ITEMS, // ITEM_ROCKY_HELMET
    BAG_POCKET_ITEMS, // ITEM_AIR_BALLOON
    BAG_POCKET_ITEMS, // ITEM_RED_CARD
    BAG_POCKET_ITEMS, // ITEM_RING_TARGET
    BAG_POCKET_ITEMS, // ITEM_BINDING_BAND
    BAG_POCKET_ITEMS, // ITEM_ABSORB_BULB
    BAG_POCKET_ITEMS, // ITEM_CELL_BATTERY
    BAG_POCKET_ITEMS, // ITEM_EJECT_BUTTON
    BAG_POCKET_ITEMS, // ITEM_FIRE_GEM
    BAG_POCKET_ITEMS, // ITEM_WATER_GEM
    BAG_POCKET_ITEMS, // ITEM_ELECTRIC_GEM
    BAG_POCKET_ITEMS, // ITEM_GRASS_GEM
    BAG_POCKET_ITEMS, // ITEM_ICE_GEM
    BAG_POCKET_ITEMS, // ITEM_FIGHTING_GEM
    BAG_POCKET_ITEMS, // ITEM_POISON_GEM
    BAG_POCKET_ITEMS, // ITEM_GROUND_GEM
    BAG_POCKET_ITEMS, // ITEM_FLYING_GEM
    BAG_POCKET_ITEMS, // ITEM_PSYCHIC_GEM
    BAG_POCKET_ITEMS, // ITEM_BUG_GEM
    BAG_POCKET_ITEMS, // ITEM_ROCK_GEM
    BAG_POCKET_ITEMS, // ITEM_GHOST_GEM
    BAG_POCKET_ITEMS, // ITEM_DRAGON_GEM
    BAG_POCKET_ITEMS, // ITEM_DARK_GEM
    BAG_POCKET_ITEMS, // ITEM_STEEL_GEM
    BAG_POCKET_ITEMS, // ITEM_NORMAL_GEM
    BAG_POCKET_MEDICINE, // ITEM_HEALTH_WING
    BAG_POCKET_MEDICINE, // ITEM_MUSCLE_WING
    BAG_POCKET_MEDICINE, // ITEM_RESIST_WING
    BAG_POCKET_MEDICINE, // ITEM_GENIUS_WING
    BAG_POCKET_MEDICINE, // ITEM_CLEVER_WING
    BAG_POCKET_MEDICINE, // ITEM_SWIFT_WING
    BAG_POCKET_ITEMS, // ITEM_PRETTY_WING
    BAG_POCKET_ITEMS, // ITEM_COVER_FOSSIL
    BAG_POCKET_ITEMS, // ITEM_PLUME_FOSSIL
    BAG_POCKET_KEY_ITEMS, // ITEM_LIBERTY_PASS
    BAG_POCKET_ITEMS, // ITEM_PASS_ORB
    BAG_POCKET_ITEMS, // ITEM_DREAM_BALL
    BAG_POCKET_ITEMS, // ITEM_POKE_TOY
    BAG_POCKET_KEY_ITEMS, // ITEM_PROP_CASE
    BAG_POCKET_KEY_ITEMS, // ITEM_DRAGON_SKULL
    BAG_POCKET_ITEMS, // ITEM_BALM_MUSHROOM
    BAG_POCKET_ITEMS, // ITEM_BIG_NUGGET
    BAG_POCKET_ITEMS, // ITEM_PEARL_STRING
    BAG_POCKET_ITEMS, // ITEM_COMET_SHARD
    BAG_POCKET_ITEMS, // ITEM_RELIC_COPPER
    BAG_POCKET_ITEMS, // ITEM_RELIC_SILVER
    BAG_POCKET_ITEMS, // ITEM_RELIC_GOLD
    BAG_POCKET_ITEMS, // ITEM_RELIC_VASE
    BAG_POCKET_ITEMS, // ITEM_RELIC_BAND
    BAG_POCKET_ITEMS, // ITEM_RELIC_STATUE
    BAG_POCKET_ITEMS, // ITEM_RELIC_CROWN
    BAG_POCKET_MEDICINE, // ITEM_CASTELIACONE
    BAG_POCKET_ITEMS, // ITEM_DIRE_HIT_2
    BAG_POCKET_ITEMS, // ITEM_X_SPEED_2
    BAG_POCKET_ITEMS, // ITEM_X_SPECIAL_2
    BAG_POCKET_ITEMS, // ITEM_X_SP_DEF_2
    BAG_POCKET_ITEMS, // ITEM_X_DEFEND_2
    BAG_POCKET_ITEMS, // ITEM_X_ATTACK_2
    BAG_POCKET_ITEMS, // ITEM_X_ACCURACY_2
    BAG_POCKET_ITEMS, // ITEM_X_SPEED_3
    BAG_POCKET_ITEMS, // ITEM_X_SPECIAL_3
    BAG_POCKET_ITEMS, // ITEM_X_SP_DEF_3
    BAG_POCKET_ITEMS, // ITEM_X_DEFEND_3
    BAG_POCKET_ITEMS, // ITEM_X_ATTACK_3
    BAG_POCKET_ITEMS, // ITEM_X_ACCURACY_3
    BAG_POCKET_ITEMS, // ITEM_X_SPEED_6
    BAG_POCKET_ITEMS, // ITEM_X_SPECIAL_6
    BAG_POCKET_ITEMS, // ITEM_X_SP_DEF_6
    BAG_POCKET_ITEMS, // ITEM_X_DEFEND_6
    BAG_POCKET_ITEMS, // ITEM_X_ATTACK_6
    BAG_POCKET_ITEMS, // ITEM_X_ACCURACY_6
    BAG_POCKET_ITEMS, // ITEM_ABILITY_URGE
    BAG_POCKET_ITEMS, // ITEM_ITEM_DROP
    BAG_POCKET_ITEMS, // ITEM_ITEM_URGE
    BAG_POCKET_ITEMS, // ITEM_RESET_URGE
    BAG_POCKET_ITEMS, // ITEM_DIRE_HIT_3
    BAG_POCKET_KEY_ITEMS, // ITEM_LIGHT_STONE
    BAG_POCKET_KEY_ITEMS, // ITEM_DARK_STONE
    BAG_POCKET_TMS_HMS, // ITEM_TM93
    BAG_POCKET_TMS_HMS, // ITEM_TM94
    BAG_POCKET_TMS_HMS, // ITEM_TM95
    BAG_POCKET_KEY_ITEMS, // ITEM_XTRANSCEIVER_MALE
    BAG_POCKET_KEY_ITEMS, // ITEM_UNUSED_622
    BAG_POCKET_KEY_ITEMS, // ITEM_GRAM_1
    BAG_POCKET_KEY_ITEMS, // ITEM_GRAM_2
    BAG_POCKET_KEY_ITEMS, // ITEM_GRAM_3
    BAG_POCKET_KEY_ITEMS, // ITEM_XTRANSCEIVER_FEMALE
    BAG_POCKET_KEY_ITEMS, // ITEM_MEDAL_BOX
    BAG_POCKET_KEY_ITEMS, // ITEM_DNA_SPLICERS_FUSE
    BAG_POCKET_KEY_ITEMS, // ITEM_DNA_SPLICERS_SEPARATE
    BAG_POCKET_KEY_ITEMS, // ITEM_PERMIT
    BAG_POCKET_KEY_ITEMS, // ITEM_OVAL_CHARM
    BAG_POCKET_KEY_ITEMS, // ITEM_SHINY_CHARM
    BAG_POCKET_KEY_ITEMS, // ITEM_PLASMA_CARD
    BAG_POCKET_KEY_ITEMS, // ITEM_GRUBBY_HANKY
    BAG_POCKET_KEY_ITEMS, // ITEM_COLRESS_MCHN
    BAG_POCKET_KEY_ITEMS, // ITEM_DROPPED_ITEM_MALE
    BAG_POCKET_KEY_ITEMS, // ITEM_DROPPED_ITEM_FEMALE
    BAG_POCKET_KEY_ITEMS, // ITEM_REVEAL_GLASS
};

u32 BagSave_GetBlockSize(void) {
    return sizeof(BagSave);
}

void BagSave_Init(BagSave *bag) {
    sys_memset(bag, 0, sizeof(BagSave));
}

void BagSave_CopyPocket(BagSave *bag, BagItem *items, u32 pocket, BOOL load) {
    BagItem *pocketItems;
    u32 max;
    u32 i;

    BagSave_GetPocketPtr(bag, pocket, &pocketItems, &max);
    BagSave_DefragmentFreeSpace(bag, pocketItems, max);
    if (load) {
        for (i = 0; i < max; i++) {
            if (!BagSave_IsItemInFreeSpace(bag, pocketItems[i].item)) {
                sys_memcpy(&pocketItems[i], &items[i], sizeof(BagItem));
            } else {
                BagSave_ClearItemEntry(&items[i]);
            }
        }
    } else {
        for (i = 0; i < max; i++) {
            if (items[i].item != ITEM_NONE && !BagSave_IsItemInFreeSpace(bag, items[i].item)) {
                sys_memcpy(&items[i], &pocketItems[i], sizeof(BagItem));
            }
        }
    }
}

void BagSave_CopyPocketRaw(BagSave *bag, BagItem *items, u32 pocket, BOOL load) {
    BagItem *pocketItems;
    u32 max;

    BagSave_GetPocketPtr(bag, pocket, &pocketItems, &max);
    if (load) {
        sys_memcpy(pocketItems, items, max * sizeof(BagItem));
    } else {
        sys_memcpy(items, pocketItems, max * sizeof(BagItem));
    }
}

// Moves the items in the Free Space behind the others of a pocket
static void BagSave_DefragmentFreeSpace(BagSave *bag, BagItem *items, u32 max) {
    u32 i;
    u32 j;

    for (i = 0; i < max - 1; i++) {
        for (j = i + 1; j < max; j++) {
            BOOL inFreeSpace = BagSave_IsItemInFreeSpace(bag, items[i].item);

            if (items[j].item != ITEM_NONE && inFreeSpace == TRUE) {
                BagSave_SwapItems(&items[i], &items[j]);
            }
        }
    }
}

// A pocket's slots and their number. The Free Space has no slots of its own
static u32 BagSave_GetPocketPtr(BagSave *bag, u32 pocket, BagItem **items, u32 *max) {
    switch (pocket) {
    case BAG_POCKET_KEY_ITEMS:
        *items = bag->keyItems;
        *max = BAG_KEY_ITEMS_MAX;
        break;
    case BAG_POCKET_ITEMS:
        *items = bag->items;
        *max = BAG_ITEMS_MAX;
        break;
    case BAG_POCKET_BERRIES:
        *items = bag->berries;
        *max = BAG_BERRIES_MAX;
        break;
    case BAG_POCKET_MEDICINE:
        *items = bag->medicine;
        *max = BAG_MEDICINE_MAX;
        break;
    case BAG_POCKET_TMS_HMS:
        *items = bag->tmsHms;
        *max = BAG_TMS_HMS_MAX;
        break;
    case BAG_POCKET_FREE_SPACE:
        *items = NULL;
        *max = BAG_FREE_SPACE_MAX;
        break;
    default:
        *items = NULL;
        break;
    }
    return pocket;
}

static u32 BagSave_GetPocketPtrByItem(BagSave *bag, u32 item, BagItem **items, u32 *max) {
    u32 pocket = PML_ItemGetType(item);

    BagSave_GetPocketPtr(bag, pocket, items, max);
    return pocket;
}

// The slot that count more of an item go in: the item's own, unless it would hold more than maxCount, or else the
// first empty one
static BagItem *BagSave_GetItemHandleAddCheckCore(BagItem *items, u32 max, u32 item, int count, u16 maxCount) {
    u32 i;
    s32 empty = -1;

    for (i = 0; i < max; i++) {
        if (items[i].item == item) {
            if (items[i].count + count > maxCount) {
                return NULL;
            }
            return &items[i];
        }
        if (empty == -1 && items[i].item == ITEM_NONE && items[i].count == 0) {
            empty = i;
        }
    }
    if (empty == -1) {
        return NULL;
    }
    return &items[empty];
}

static BagItem *BagSave_GetItemHandleAddCheck(BagSave *bag, u32 item, int count) {
    BagItem *items;
    u32 max;
    u32 pocket = BagSave_GetPocketPtrByItem(bag, item, &items, &max);

    if (pocket == BAG_POCKET_TMS_HMS) {
        return BagSave_GetItemHandleAddCheckCore(items, max, item, count, BAG_TM_HM_COUNT_MAX);
    }
    return BagSave_GetItemHandleAddCheckCore(items, max, item, count, BAG_ITEM_COUNT_MAX);
}

BOOL BagSave_CheckAvailItemSpace(BagSave *bag, u16 item, u16 count, HeapID heapId) {
    if (BagSave_GetItemHandleAddCheck(bag, item, count) != NULL) {
        return TRUE;
    }
    return FALSE;
}

static BagItem *BagSave_AddItemCore(BagSave *bag, u32 item, int count) {
    BagItem *slot = BagSave_GetItemHandleAddCheck(bag, item, count);

    if (slot == NULL) {
        return NULL;
    }
    slot->item = item;
    slot->count = slot->count + count;
    return slot;
}

BOOL BagSave_AddItem(BagSave *bag, u16 item, u16 count, u32 heapId) {
    if (BagSave_AddItemCore(bag, item, count) != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL BagSave_AddItemAsFirst(BagSave *bag, u32 item, u32 count, u32 heapId) {
    BagItem *items;
    u32 max;
    BagItem *slot = BagSave_AddItemCore(bag, item, count);

    if (slot == NULL) {
        return FALSE;
    }
    BagSave_GetPocketPtrByItem(bag, item, &items, &max);
    BagSave_ForceItemAsLast(bag, slot->item);
    BagSave_MoveLastToFirst(items, BagSave_GetUniqueItemCount(items, max));
    return TRUE;
}

// An item's slot, if it holds at least count
static BagItem *BagSave_GetItemHandleIfAmountCore(BagItem *items, u32 max, u32 item, u32 count) {
    u32 i;

    for (i = 0; i < max; i++) {
        if (items[i].item == item) {
            if (items[i].count >= count) {
                return &items[i];
            }
            return NULL;
        }
    }
    return NULL;
}

static BagItem *BagSave_GetItemHandleIfAmount(BagSave *bag, u32 item, u32 count) {
    BagItem *items;
    u32 max;

    BagSave_GetPocketPtrByItem(bag, item, &items, &max);
    return BagSave_GetItemHandleIfAmountCore(items, max, item, count);
}

BOOL BagSave_IsItemFreeSpaceBit(BagSave *bag, u16 item) {
    return (bag->freeSpace[item >> 5] & (1 << (item % 32))) != 0;
}

BOOL BagSave_IsItemInFreeSpace(BagSave *bag, u16 item) {
    if (BagSave_IsItemFreeSpaceBit(bag, item)) {
        u8 pocket = BagSave_GetExistingItemPocket(bag, item);

        if (pocket != BAG_POCKET_NONE) {
            return TRUE;
        }
    }
    return FALSE;
}

static void BagSave_MoveBetweenFreeSpace_(BagSave *bag, u16 item, BOOL toFreeSpace) {
    s32 index = item >> 5;
    u32 mask;

    if (item == ITEM_NONE) {
        return;
    }
    if (!toFreeSpace) {
        mask = 0xffffffff;
        mask ^= 1 << (item % 32);
        bag->freeSpace[index] &= mask;
    } else if (BagSave_GetExistingItemPocket(bag, item) != BAG_POCKET_NONE) {
        mask = 1;
        mask <<= item % 32;
        bag->freeSpace[index] |= mask;
    }
}

void BagSave_MoveBetweenFreeSpace(BagSave *bag, u16 item, BOOL toFreeSpace) {
    BagSave_MoveBetweenFreeSpace_(bag, item, toFreeSpace);
}

u32 BagSave_GetExistingItemPocket(BagSave *bag, u16 item) {
    u32 pocket = PML_ItemGetType(item);

    if (!BagSave_IsItemInDesignatedPocket(bag, item, pocket)) {
        pocket = BAG_POCKET_NONE;
    }
    return pocket;
}

// Whether a pocket holds an item. The search ends at the first empty slot
static BOOL BagSave_IsItemInDesignatedPocket(BagSave *bag, u16 item, u32 pocket) {
    BagItem *items;
    u32 max;
    u32 i;

    BagSave_GetPocketPtr(bag, pocket, &items, &max);
    for (i = 0; i < max; i++) {
        if (items[i].item == item) {
            return TRUE;
        }
        if (items[i].item == ITEM_NONE) {
            return FALSE;
        }
    }
    return FALSE;
}

BOOL BagSave_SubItem(BagSave *bag, u16 item, u16 count, HeapID heapId) {
    BagItem *slot;
    u32 max;

    slot = BagSave_GetItemHandleIfAmount(bag, item, count);
    if (slot == NULL) {
        return FALSE;
    }
    slot->count -= count;
    if (slot->count == 0) {
        slot->item = ITEM_NONE;
    }
    BagSave_GetPocketPtrByItem(bag, item, &slot, &max);
    BagSave_Defragment(slot, max);
    return TRUE;
}

BOOL BagSave_CheckAmount(BagSave *bag, u32 item, u32 count, u32 heapId) {
    if (BagSave_GetItemHandleIfAmount(bag, item, count) != NULL) {
        return TRUE;
    }
    return FALSE;
}

u32 BagSave_GetUniqueItemCount(BagItem *items, u32 max) {
    u32 count = 0;
    u32 i;

    if (items != NULL) {
        for (i = 0; i < max; i++) {
            if (items[i].item != ITEM_NONE) {
                count++;
            }
        }
    }
    return count;
}

s32 BagSave_GetPocketItemCountCore(BagSave *bag, u32 pocket, BOOL withFreeSpace) {
    u32 max;
    BagItem *items;
    s32 count = 0;
    u32 i;

    BagSave_GetPocketPtr(bag, pocket, &items, &max);
    if (items != NULL) {
        for (i = 0; i < max; i++) {
            if (items[i].item != ITEM_NONE) {
                if (!withFreeSpace) {
                    if (!BagSave_IsItemInFreeSpace(bag, items[i].item)) {
                        count++;
                    }
                } else {
                    count++;
                }
            }
        }
    }
    return count;
}

s32 BagSave_GetPocketItemCount(BagSave *bag, u32 pocket) {
    if (pocket != BAG_POCKET_FREE_SPACE) {
        return BagSave_GetPocketItemCountCore(bag, pocket, FALSE);
    }
    return BagSave_GetFreeSpaceItemCount(bag);
}

static s32 BagSave_GetFreeSpaceItemCount(BagSave *bag) {
    int item;
    s32 count = 0;

    for (item = 0; item <= ITEM_LAST; item++) {
        if (item != ITEM_NONE && BagSave_IsItemInFreeSpace(bag, item)) {
            count++;
        }
    }
    return count;
}

u16 BagSave_GetItemCountByID(BagSave *bag, u16 item, HeapID heapId) {
    BagItem *slot = BagSave_GetItemHandleIfAmount(bag, item, 1);

    if (slot == NULL) {
        return 0;
    }
    return slot->count;
}

u32 BagSave_GetExistingItemCount(BagSave *bag, u16 item) {
    u32 pocket = BagSave_GetExistingItemPocket(bag, item);

    if (pocket != BAG_POCKET_NONE) {
        BagItem *slot = BagSave_GetItemHandle(bag, pocket, item);

        if (slot == NULL) {
            return 0;
        }
        return slot->count;
    }
    return 0;
}

static void BagSave_SwapItems(BagItem *a, BagItem *b) {
    BagItem temp = *a;

    *a = *b;
    *b = temp;
}

// Moves the slots of a pocket up by one and puts the last one first
static void BagSave_MoveLastToFirst(BagItem *items, u32 count) {
    BagItem last;
    s32 i;

    last.item = ITEM_NONE;
    if (count > 1) {
        for (i = count - 1; i >= 0; i--) {
            if (last.item == ITEM_NONE) {
                sys_memcpy(&items[i], &last, sizeof(BagItem));
            } else {
                sys_memcpy(&items[i], &items[i + 1], sizeof(BagItem));
            }
        }
        sys_memcpy(&last, &items[0], sizeof(BagItem));
    }
}

void BagSave_ForceItemAsLast(BagSave *bag, u16 item) {
    BagItem *items;
    BagItem moved;
    u32 max;
    u32 i;
    BagItem *slot;
    u32 pocket = BagSave_GetExistingItemPocket(bag, item);

    if (pocket == BAG_POCKET_NONE) {
        return;
    }
    slot = BagSave_GetItemHandle(bag, pocket, item);
    if (slot == NULL) {
        return;
    }
    moved = *slot;
    slot->item = ITEM_NONE;
    slot->count = 0;
    BagSave_GetPocketPtr(bag, pocket, &items, &max);
    BagSave_Defragment(items, max);
    for (i = 0; i < max; i++) {
        if (items[i].item == ITEM_NONE) {
            items[i] = moved;
            return;
        }
    }
    BagSave_Defragment(items, max);
}

// Clears the count of the slots that hold no item
static void BagSave_ClearEmpty(BagItem *items, u32 max) {
    u32 i;

    for (i = 0; i < max; i++) {
        if (items[i].item == ITEM_NONE) {
            items[i].count = 0;
        }
    }
}

// Moves the empty slots of a pocket behind the others
static void BagSave_Defragment(BagItem *items, u32 max) {
    u32 i;
    u32 j;

    BagSave_ClearEmpty(items, max);
    for (i = 0; i < max - 1; i++) {
        for (j = i + 1; j < max; j++) {
            if (items[i].count == 0) {
                BagSave_SwapItems(&items[i], &items[j]);
            }
        }
    }
}

void sortItemBlocks(BagSave *bag) {
    BagItem *items;
    u32 max;

    BagSave_GetPocketPtr(bag, BAG_POCKET_KEY_ITEMS, &items, &max);
    BagSave_Defragment(items, max);
    BagSave_GetPocketPtr(bag, BAG_POCKET_ITEMS, &items, &max);
    BagSave_Defragment(items, max);
    BagSave_GetPocketPtr(bag, BAG_POCKET_BERRIES, &items, &max);
    BagSave_Defragment(items, max);
    BagSave_GetPocketPtr(bag, BAG_POCKET_MEDICINE, &items, &max);
    BagSave_Defragment(items, max);
    BagSave_GetPocketPtr(bag, BAG_POCKET_TMS_HMS, &items, &max);
    BagSave_Defragment(items, max);
}

BagItem *BagSave_GetItemIndexHandle(BagSave *bag, u32 pocket, u16 index) {
    BagItem *items;
    u32 max;

    switch (pocket) {
    case BAG_POCKET_KEY_ITEMS:
        items = bag->keyItems;
        max = BAG_KEY_ITEMS_MAX;
        break;
    case BAG_POCKET_ITEMS:
        items = bag->items;
        max = BAG_ITEMS_MAX;
        break;
    case BAG_POCKET_BERRIES:
        items = bag->berries;
        max = BAG_BERRIES_MAX;
        break;
    case BAG_POCKET_MEDICINE:
        items = bag->medicine;
        max = BAG_MEDICINE_MAX;
        break;
    case BAG_POCKET_TMS_HMS:
        items = bag->tmsHms;
        max = BAG_TMS_HMS_MAX;
        break;
    case BAG_POCKET_FREE_SPACE:
        return NULL;
    }
    if (index >= max) {
        return NULL;
    }
    return &items[index];
}

BagItem *BagSave_GetItemHandle(BagSave *bag, u16 pocket, u16 item) {
    BagItem *items;
    u32 max;
    u32 i;

    BagSave_GetPocketPtr(bag, pocket, &items, &max);
    for (i = 0; i < max; i++) {
        if (items[i].item == item) {
            return &items[i];
        }
    }
    return NULL;
}

void BagSave_SwitchOwnedDNASplicers(BagSave *bag, u32 from) {
    BagItem *items = bag->keyItems;
    BagItem *found = NULL;
    int i;

    for (i = 0; i < BAG_KEY_ITEMS_MAX; i++) {
        BagItem *slot = &items[i];

        if (slot->item == sDnaSplicers[from]) {
            if (found == NULL) {
                slot->item = sDnaSplicers[from ^ 1];
                found = slot;
                slot->count = 1;
            } else {
                BagSave_ClearItemEntry(slot);
            }
        } else if (slot->item == sDnaSplicers[from ^ 1]) {
            BagSave_ClearItemEntry(slot);
        }
    }
    BagSave_MoveBetweenFreeSpace(bag, sDnaSplicers[from ^ 1], BagSave_IsItemFreeSpaceBit(bag, sDnaSplicers[from]));
    BagSave_Defragment(items, BAG_KEY_ITEMS_MAX);
}

u32 PML_ItemGetMaxStorageCount(u16 item) {
    if (PML_ItemIsTMHM(item) == TRUE) {
        return BAG_TM_HM_COUNT_MAX;
    }
    return BAG_ITEM_COUNT_MAX;
}

BagCursor *BagCursor_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(BagCursor), TRUE, __FILE__, 1371);
}

void func_0200887c(BagCursor *cursor, u16 pocket, s16 *row, s16 *scroll) {
    *row = cursor->rows[pocket];
    *scroll = cursor->scrolls[pocket];
}

u16 func_02008890(BagCursor *cursor) {
    return cursor->pocket;
}

void func_02008894(BagCursor *cursor, u16 pocket, s16 row, s16 scroll) {
    cursor->rows[pocket] = row;
    cursor->scrolls[pocket] = scroll;
}

void func_020088a4(BagCursor *cursor, u16 pocket) {
    cursor->pocket = pocket;
}

void func_020088a8(BagCursor *cursor, u16 pocket, s16 *row, s16 *page) {
    *row = cursor->battleRows[pocket];
    *page = cursor->battlePages[pocket];
}

u16 func_020088bc(BagCursor *cursor) {
    return cursor->battleLastItem;
}

u16 func_020088c0(BagCursor *cursor) {
    return cursor->battleLastPocket;
}

void func_020088c4(BagCursor *cursor, s16 *rows, s16 *pages) {
    u32 i;

    for (i = 0; i < BATTLE_BAG_POCKET_COUNT; i++) {
        cursor->battleRows[i] = rows[i];
        cursor->battlePages[i] = pages[i];
    }
}

void func_020088e0(BagCursor *cursor, u16 item, u8 pocket) {
    cursor->battleLastItem = item;
    cursor->battleLastPocket = pocket;
}

u8 func_020088e8(BagCursor *cursor) {
    return cursor->freeSpaceFilter;
}

void func_020088ec(BagCursor *cursor, u32 filter) {
    cursor->freeSpaceFilter = filter;
}

// An item's pocket
static u32 PML_ItemGetType(u32 item) {
    return sItemPockets[item];
}

u32 BagSave_GetActualItemPocket(BagSave *bag, u16 item) {
    if (BagSave_IsItemFreeSpaceBit(bag, item) == TRUE) {
        return BAG_POCKET_FREE_SPACE;
    }
    return PML_ItemGetType(item);
}

static void BagSave_ClearItemEntry(BagItem *slot) {
    slot->item = ITEM_NONE;
    slot->count = 0;
}

void BagSave_GetOwnedItemFlags(BagSave *bag, u8 *inBag) {
    BagItem *items;
    u32 max;
    u16 pocket;
    u32 i;

    sys_memset(inBag, 0, ITEM_LAST + 1);
    for (pocket = 0; pocket <= BAG_POCKET_KEY_ITEMS; pocket++) {
        BagSave_GetPocketPtr(bag, pocket, &items, &max);
        for (i = 0; i < max; i++) {
            if (items[i].item != ITEM_NONE) {
                inBag[items[i].item] = TRUE;
            }
        }
    }
}

BagItem *BagSave_GetPocketItems(BagSave *bag, u32 pocket, u32 *max) {
    BagItem *items;

    BagSave_GetPocketPtr(bag, pocket, &items, max);
    return items;
}
