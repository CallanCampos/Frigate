#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "system/shooter_item.h"

// Item data. The file's name is a guess: the ROM has no string for it. The tables are named by swan except
// DUMMY_ITEMS, UNHOLDABLE_ITEMS and LIST_ITEM_GRAPHICS, which are ours

// The icon files of no item and of the return arrow
#define ICON_NONE_CHAR 0x3fd
#define ICON_NONE_PLTT 0x3fe
#define ICON_RETURN_CHAR 0x3ff
#define ICON_RETURN_PLTT 0x400

// The item names' and descriptions' message files in ARCID_SYSTEM_MESSAGE
#define MSG_ITEM_DESCRIPTIONS TEXT_BANK_ITEM_DESCRIPTIONS
#define MSG_ITEM_NAMES TEXT_BANK_ITEM_NAMES

// Where the HMs and TM93 to TM95 are in TM_MOVE_LIST
#define TM_INDEX_HM01 92
#define TM_INDEX_TM93 98
#define HM_COUNT 6

static s32 PML_ItemGetBattleStat(ItemParams *params, u32 param);
static u16 PML_ItemMonsBallConvID(u16 id, u32 dir);

static const u16 MAIL_ITEM_IDS[] = {
    ITEM_GREET_MAIL, ITEM_FAVORED_MAIL,  ITEM_RSVP_MAIL,     ITEM_THANKS_MAIL,   ITEM_INQUIRY_MAIL,  ITEM_LIKE_MAIL,
    ITEM_REPLY_MAIL, ITEM_BRIDGE_MAIL_S, ITEM_BRIDGE_MAIL_D, ITEM_BRIDGE_MAIL_T, ITEM_BRIDGE_MAIL_V, ITEM_BRIDGE_MAIL_M,
};

// The unused item IDs and the Data Cards, a bit per item
static const u32 DUMMY_ITEMS[] = {
    0x00000001, 0x00000000, 0x00000000, 0xff0e0000, 0x0000003f, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000c00,
    0x00000000, 0xfe000000, 0x000fffff, 0x00000000, 0x00000000, 0x00000000,
};

// The items a Pokémon can't hold, a bit per item
static const u32 UNHOLDABLE_ITEMS[] = {
    0x00000000, 0x00000000, 0x00000000, 0x000e0000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xffffff00, 0xffffffff, 0xffffffff, 0xfffff3ff,
    0xffffffff, 0x00e0001f, 0x01f00000, 0x40000000, 0x0000000c, 0x7fffff00,
};

// Each ball item and its ball ID
static const u16 MONS_BALL_ITEMS[][2] = {
    { ITEM_MASTER_BALL, 1 }, { ITEM_ULTRA_BALL, 2 },   { ITEM_GREAT_BALL, 3 },   { ITEM_POKE_BALL, 4 },
    { ITEM_SAFARI_BALL, 5 }, { ITEM_NET_BALL, 6 },     { ITEM_DIVE_BALL, 7 },    { ITEM_NEST_BALL, 8 },
    { ITEM_REPEAT_BALL, 9 }, { ITEM_TIMER_BALL, 10 },  { ITEM_LUXURY_BALL, 11 }, { ITEM_PREMIER_BALL, 12 },
    { ITEM_DUSK_BALL, 13 },  { ITEM_HEAL_BALL, 14 },   { ITEM_QUICK_BALL, 15 },  { ITEM_CHERISH_BALL, 16 },
    { ITEM_FAST_BALL, 17 },  { ITEM_LEVEL_BALL, 18 },  { ITEM_LURE_BALL, 19 },   { ITEM_HEAVY_BALL, 20 },
    { ITEM_LOVE_BALL, 21 },  { ITEM_FRIEND_BALL, 22 }, { ITEM_MOON_BALL, 23 },   { ITEM_SPORT_BALL, 24 },
    { ITEM_DREAM_BALL, 25 },
};

static const u16 BERRY_ITEM_IDS[] = {
    ITEM_CHERI_BERRY,  ITEM_CHESTO_BERRY, ITEM_PECHA_BERRY,  ITEM_RAWST_BERRY,  ITEM_ASPEAR_BERRY, ITEM_LEPPA_BERRY,
    ITEM_ORAN_BERRY,   ITEM_PERSIM_BERRY, ITEM_LUM_BERRY,    ITEM_SITRUS_BERRY, ITEM_FIGY_BERRY,   ITEM_WIKI_BERRY,
    ITEM_MAGO_BERRY,   ITEM_AGUAV_BERRY,  ITEM_IAPAPA_BERRY, ITEM_RAZZ_BERRY,   ITEM_BLUK_BERRY,   ITEM_NANAB_BERRY,
    ITEM_WEPEAR_BERRY, ITEM_PINAP_BERRY,  ITEM_POMEG_BERRY,  ITEM_KELPSY_BERRY, ITEM_QUALOT_BERRY, ITEM_HONDEW_BERRY,
    ITEM_GREPA_BERRY,  ITEM_TAMATO_BERRY, ITEM_CORNN_BERRY,  ITEM_MAGOST_BERRY, ITEM_RABUTA_BERRY, ITEM_NOMEL_BERRY,
    ITEM_SPELON_BERRY, ITEM_PAMTRE_BERRY, ITEM_WATMEL_BERRY, ITEM_DURIN_BERRY,  ITEM_BELUE_BERRY,  ITEM_OCCA_BERRY,
    ITEM_PASSHO_BERRY, ITEM_WACAN_BERRY,  ITEM_RINDO_BERRY,  ITEM_YACHE_BERRY,  ITEM_CHOPLE_BERRY, ITEM_KEBIA_BERRY,
    ITEM_SHUCA_BERRY,  ITEM_COBA_BERRY,   ITEM_PAYAPA_BERRY, ITEM_TANGA_BERRY,  ITEM_CHARTI_BERRY, ITEM_KASIB_BERRY,
    ITEM_HABAN_BERRY,  ITEM_COLBUR_BERRY, ITEM_BABIRI_BERRY, ITEM_CHILAN_BERRY, ITEM_LIECHI_BERRY, ITEM_GANLON_BERRY,
    ITEM_SALAC_BERRY,  ITEM_PETAYA_BERRY, ITEM_APICOT_BERRY, ITEM_LANSAT_BERRY, ITEM_STARF_BERRY,  ITEM_ENIGMA_BERRY,
    ITEM_MICLE_BERRY,  ITEM_CUSTAP_BERRY, ITEM_JABOCA_BERRY, ITEM_ROWAP_BERRY,
};

// The icons of the Wonder Launcher's items, in ShooterItem_GetIndex's order: characters and palette
static const u16 LIST_ITEM_GRAPHICS[][2] = {
    { 867, 868 }, { 871, 872 }, { 863, 864 },  { 831, 837 }, { 831, 836 }, { 831, 834 }, { 831, 835 }, { 831, 833 },
    { 831, 838 }, { 831, 832 }, { 1000, 839 }, { 875, 876 }, { 877, 878 }, { 879, 880 }, { 881, 882 }, { 883, 884 },
    { 885, 886 }, { 865, 866 }, { 840, 846 },  { 840, 845 }, { 840, 843 }, { 840, 844 }, { 840, 842 }, { 840, 847 },
    { 840, 841 }, { 889, 890 }, { 848, 854 },  { 848, 853 }, { 848, 851 }, { 848, 852 }, { 848, 850 }, { 848, 855 },
    { 848, 849 }, { 887, 888 }, { 869, 870 },  { 895, 896 }, { 899, 900 }, { 873, 874 }, { 856, 861 }, { 856, 860 },
    { 856, 858 }, { 856, 859 }, { 856, 857 },  { 856, 862 }, { 901, 902 }, { 903, 904 },
};

// The moves of TM01 to TM92, HM01 to HM06 and TM93 to TM95, then 0
static const u16 TM_MOVE_LIST[] = {
    MOVE_HONE_CLAWS,   MOVE_DRAGON_CLAW, MOVE_PSYSHOCK,     MOVE_CALM_MIND,    MOVE_ROAR,         MOVE_TOXIC,
    MOVE_HAIL,         MOVE_BULK_UP,     MOVE_VENOSHOCK,    MOVE_HIDDEN_POWER, MOVE_SUNNY_DAY,    MOVE_TAUNT,
    MOVE_ICE_BEAM,     MOVE_BLIZZARD,    MOVE_HYPER_BEAM,   MOVE_LIGHT_SCREEN, MOVE_PROTECT,      MOVE_RAIN_DANCE,
    MOVE_TELEKINESIS,  MOVE_SAFEGUARD,   MOVE_FRUSTRATION,  MOVE_SOLAR_BEAM,   MOVE_SMACK_DOWN,   MOVE_THUNDERBOLT,
    MOVE_THUNDER,      MOVE_EARTHQUAKE,  MOVE_RETURN,       MOVE_DIG,          MOVE_PSYCHIC,      MOVE_SHADOW_BALL,
    MOVE_BRICK_BREAK,  MOVE_DOUBLE_TEAM, MOVE_REFLECT,      MOVE_SLUDGE_WAVE,  MOVE_FLAMETHROWER, MOVE_SLUDGE_BOMB,
    MOVE_SANDSTORM,    MOVE_FIRE_BLAST,  MOVE_ROCK_TOMB,    MOVE_AERIAL_ACE,   MOVE_TORMENT,      MOVE_FACADE,
    MOVE_FLAME_CHARGE, MOVE_REST,        MOVE_ATTRACT,      MOVE_THIEF,        MOVE_LOW_SWEEP,    MOVE_ROUND,
    MOVE_ECHOED_VOICE, MOVE_OVERHEAT,    MOVE_ALLY_SWITCH,  MOVE_FOCUS_BLAST,  MOVE_ENERGY_BALL,  MOVE_FALSE_SWIPE,
    MOVE_SCALD,        MOVE_FLING,       MOVE_CHARGE_BEAM,  MOVE_SKY_DROP,     MOVE_INCINERATE,   MOVE_QUASH,
    MOVE_WILL_O_WISP,  MOVE_ACROBATICS,  MOVE_EMBARGO,      MOVE_EXPLOSION,    MOVE_SHADOW_CLAW,  MOVE_PAYBACK,
    MOVE_RETALIATE,    MOVE_GIGA_IMPACT, MOVE_ROCK_POLISH,  MOVE_FLASH,        MOVE_STONE_EDGE,   MOVE_VOLT_SWITCH,
    MOVE_THUNDER_WAVE, MOVE_GYRO_BALL,   MOVE_SWORDS_DANCE, MOVE_STRUGGLE_BUG, MOVE_PSYCH_UP,     MOVE_BULLDOZE,
    MOVE_FROST_BREATH, MOVE_ROCK_SLIDE,  MOVE_X_SCISSOR,    MOVE_DRAGON_TAIL,  MOVE_WORK_UP,      MOVE_POISON_JAB,
    MOVE_DREAM_EATER,  MOVE_GRASS_KNOT,  MOVE_SWAGGER,      MOVE_PLUCK,        MOVE_U_TURN,       MOVE_SUBSTITUTE,
    MOVE_FLASH_CANNON, MOVE_TRICK_ROOM,  MOVE_CUT,          MOVE_FLY,          MOVE_SURF,         MOVE_STRENGTH,
    MOVE_WATERFALL,    MOVE_DIVE,        MOVE_WILD_CHARGE,  MOVE_ROCK_SMASH,   MOVE_SNARL,        MOVE_NONE,
};

// Each item's icon: characters and palette
static const u16 ITEM_GRAPHICS_CHARS_AND_PALETTES[][2] = {
    { 1021, 1022 }, { 2, 3 },       { 4, 5 },       { 6, 7 },       { 8, 9 },       { 10, 11 },     { 12, 13 },
    { 14, 15 },     { 16, 17 },     { 18, 19 },     { 20, 19 },     { 21, 22 },     { 23, 22 },     { 663, 664 },
    { 665, 666 },   { 667, 668 },   { 669, 670 },   { 24, 25 },     { 26, 27 },     { 30, 28 },     { 30, 29 },
    { 30, 31 },     { 30, 32 },     { 33, 34 },     { 33, 35 },     { 24, 36 },     { 24, 37 },     { 38, 39 },
    { 40, 42 },     { 41, 42 },     { 43, 44 },     { 45, 46 },     { 47, 48 },     { 49, 50 },     { 51, 52 },
    { 53, 54 },     { 51, 55 },     { 56, 57 },     { 58, 59 },     { 58, 60 },     { 58, 61 },     { 58, 62 },
    { 63, 64 },     { 71, 72 },     { 73, 74 },     { 83, 84 },     { 85, 86 },     { 85, 87 },     { 85, 88 },
    { 85, 89 },     { 90, 91 },     { 92, 93 },     { 85, 94 },     { 95, 96 },     { 466, 467 },   { 100, 97 },
    { 100, 98 },    { 100, 99 },    { 100, 101 },   { 100, 102 },   { 100, 103 },   { 100, 104 },   { 100, 469 },
    { 105, 106 },   { 107, 108 },   { 65, 66 },     { 65, 67 },     { 65, 68 },     { 65, 69 },     { 65, 70 },
    { 51, 75 },     { 76, 77 },     { 78, 79 },     { 78, 80 },     { 78, 81 },     { 78, 82 },     { 109, 110 },
    { 109, 111 },   { 112, 113 },   { 109, 114 },   { 115, 116 },   { 117, 118 },   { 119, 120 },   { 121, 122 },
    { 123, 124 },   { 125, 126 },   { 127, 129 },   { 128, 129 },   { 130, 131 },   { 132, 131 },   { 133, 134 },
    { 135, 134 },   { 136, 137 },   { 138, 139 },   { 470, 471 },   { 472, 473 },   { 474, 475 },   { 476, 477 },
    { 478, 479 },   { 392, 393 },   { 394, 393 },   { 431, 432 },   { 433, 432 },   { 425, 426 },   { 617, 618 },
    { 615, 616 },   { 337, 338 },   { 480, 481 },   { 482, 483 },   { 484, 485 },   { 486, 487 },   { 488, 489 },
    { 699, 700 },   { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 905, 906 },   { 905, 907 },   { 905, 908 },
    { 905, 909 },   { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 },
    { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 },
    { 1021, 1022 }, { 910, 911 },   { 648, 649 },   { 646, 647 },   { 793, 794 },   { 795, 796 },   { 797, 798 },
    { 799, 800 },   { 801, 802 },   { 803, 804 },   { 805, 806 },   { 807, 808 },   { 809, 810 },   { 811, 812 },
    { 813, 814 },   { 815, 816 },   { 164, 165 },   { 166, 167 },   { 168, 169 },   { 170, 171 },   { 172, 173 },
    { 174, 175 },   { 176, 177 },   { 178, 179 },   { 180, 181 },   { 182, 183 },   { 184, 185 },   { 186, 187 },
    { 188, 189 },   { 190, 191 },   { 192, 193 },   { 194, 195 },   { 196, 197 },   { 198, 199 },   { 200, 201 },
    { 202, 203 },   { 204, 205 },   { 206, 207 },   { 208, 209 },   { 210, 211 },   { 212, 213 },   { 214, 215 },
    { 216, 217 },   { 218, 219 },   { 220, 221 },   { 222, 223 },   { 224, 225 },   { 226, 227 },   { 228, 229 },
    { 230, 231 },   { 232, 233 },   { 554, 555 },   { 556, 557 },   { 558, 559 },   { 560, 561 },   { 562, 563 },
    { 564, 565 },   { 566, 567 },   { 568, 569 },   { 570, 571 },   { 572, 573 },   { 574, 575 },   { 576, 577 },
    { 578, 579 },   { 580, 581 },   { 582, 583 },   { 584, 585 },   { 586, 587 },   { 234, 235 },   { 236, 237 },
    { 238, 239 },   { 240, 241 },   { 242, 243 },   { 244, 245 },   { 246, 247 },   { 248, 249 },   { 588, 589 },
    { 590, 591 },   { 592, 593 },   { 594, 595 },   { 250, 251 },   { 252, 253 },   { 254, 255 },   { 256, 257 },
    { 258, 259 },   { 260, 261 },   { 252, 262 },   { 263, 264 },   { 265, 266 },   { 267, 268 },   { 269, 270 },
    { 271, 272 },   { 273, 274 },   { 275, 276 },   { 277, 278 },   { 279, 280 },   { 281, 282 },   { 283, 284 },
    { 285, 286 },   { 287, 288 },   { 289, 290 },   { 291, 292 },   { 293, 294 },   { 295, 296 },   { 297, 298 },
    { 299, 300 },   { 301, 302 },   { 303, 304 },   { 305, 304 },   { 306, 307 },   { 308, 309 },   { 310, 311 },
    { 312, 313 },   { 314, 315 },   { 316, 317 },   { 318, 319 },   { 320, 321 },   { 322, 323 },   { 324, 325 },
    { 326, 327 },   { 328, 77 },    { 329, 330 },   { 331, 332 },   { 333, 334 },   { 335, 336 },   { 337, 338 },
    { 339, 340 },   { 341, 342 },   { 341, 343 },   { 341, 344 },   { 341, 345 },   { 341, 346 },   { 490, 491 },
    { 492, 493 },   { 494, 495 },   { 496, 497 },   { 611, 612 },   { 498, 499 },   { 500, 501 },   { 502, 503 },
    { 504, 505 },   { 335, 658 },   { 659, 660 },   { 506, 507 },   { 508, 509 },   { 613, 614 },   { 650, 651 },
    { 654, 655 },   { 510, 511 },   { 636, 637 },   { 638, 639 },   { 640, 641 },   { 642, 643 },   { 652, 653 },
    { 512, 513 },   { 514, 515 },   { 683, 684 },   { 691, 692 },   { 693, 694 },   { 685, 686 },   { 687, 688 },
    { 689, 690 },   { 516, 517 },   { 518, 519 },   { 520, 521 },   { 619, 620 },   { 619, 621 },   { 619, 622 },
    { 619, 623 },   { 619, 624 },   { 619, 625 },   { 619, 626 },   { 619, 627 },   { 619, 628 },   { 619, 629 },
    { 619, 630 },   { 619, 631 },   { 619, 632 },   { 619, 633 },   { 619, 634 },   { 619, 635 },   { 522, 523 },
    { 524, 525 },   { 526, 527 },   { 528, 529 },   { 530, 531 },   { 532, 533 },   { 534, 535 },   { 695, 696 },
    { 536, 537 },   { 538, 539 },   { 540, 541 },   { 542, 543 },   { 544, 545 },   { 546, 547 },   { 397, 407 },
    { 397, 399 },   { 397, 401 },   { 397, 401 },   { 397, 402 },   { 397, 403 },   { 397, 404 },   { 397, 398 },
    { 397, 403 },   { 397, 402 },   { 397, 406 },   { 397, 407 },   { 397, 404 },   { 397, 404 },   { 397, 402 },
    { 397, 401 },   { 397, 402 },   { 397, 400 },   { 397, 401 },   { 397, 402 },   { 397, 402 },   { 397, 405 },
    { 397, 412 },   { 397, 409 },   { 397, 409 },   { 397, 410 },   { 397, 402 },   { 397, 410 },   { 397, 401 },
    { 397, 411 },   { 397, 398 },   { 397, 402 },   { 397, 401 },   { 397, 403 },   { 397, 406 },   { 397, 403 },
    { 397, 412 },   { 397, 406 },   { 397, 412 },   { 397, 413 },   { 397, 407 },   { 397, 402 },   { 397, 406 },
    { 397, 401 },   { 397, 402 },   { 397, 407 },   { 397, 398 },   { 397, 402 },   { 397, 402 },   { 397, 406 },
    { 397, 401 },   { 397, 398 },   { 397, 405 },   { 397, 402 },   { 397, 400 },   { 397, 407 },   { 397, 409 },
    { 397, 413 },   { 397, 406 },   { 397, 407 },   { 397, 406 },   { 397, 413 },   { 397, 407 },   { 397, 402 },
    { 397, 411 },   { 397, 407 },   { 397, 402 },   { 397, 402 },   { 397, 412 },   { 397, 402 },   { 397, 412 },
    { 397, 409 },   { 397, 409 },   { 397, 408 },   { 397, 402 },   { 397, 610 },   { 397, 402 },   { 397, 410 },
    { 397, 404 },   { 397, 412 },   { 397, 610 },   { 397, 399 },   { 397, 402 },   { 397, 403 },   { 397, 401 },
    { 397, 405 },   { 397, 402 },   { 397, 413 },   { 397, 610 },   { 397, 402 },   { 397, 408 },   { 397, 401 },
    { 414, 402 },   { 414, 413 },   { 414, 400 },   { 414, 402 },   { 414, 400 },   { 414, 400 },   { 1021, 1022 },
    { 1021, 1022 }, { 548, 549 },   { 550, 551 },   { 552, 553 },   { 697, 698 },   { 681, 682 },   { 606, 607 },
    { 671, 672 },   { 675, 676 },   { 673, 674 },   { 661, 662 },   { 644, 645 },   { 604, 605 },   { 656, 657 },
    { 679, 680 },   { 439, 440 },   { 441, 442 },   { 349, 350 },   { 353, 354 },   { 355, 356 },   { 357, 358 },
    { 363, 364 },   { 373, 374 },   { 437, 438 },   { 429, 430 },   { 375, 64 },    { 602, 603 },   { 608, 609 },
    { 677, 678 },   { 359, 360 },   { 361, 362 },   { 596, 597 },   { 415, 416 },   { 417, 418 },   { 419, 420 },
    { 421, 422 },   { 598, 599 },   { 600, 601 },   { 701, 702 },   { 703, 704 },   { 705, 706 },   { 709, 710 },
    { 759, 760 },   { 711, 712 },   { 997, 998 },   { 761, 762 },   { 763, 764 },   { 765, 766 },   { 767, 768 },
    { 769, 770 },   { 713, 714 },   { 771, 772 },   { 773, 774 },   { 775, 776 },   { 777, 778 },   { 779, 780 },
    { 781, 782 },   { 783, 784 },   { 733, 734 },   { 735, 736 },   { 737, 738 },   { 739, 740 },   { 741, 742 },
    { 743, 744 },   { 745, 746 },   { 723, 724 },   { 717, 718 },   { 715, 716 },   { 721, 722 },   { 727, 728 },
    { 725, 726 },   { 719, 720 },   { 731, 732 },   { 729, 730 },   { 606, 607 },   { 785, 786 },   { 787, 788 },
    { 749, 750 },   { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 },
    { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 },
    { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 },
    { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 }, { 1021, 1022 },
    { 755, 756 },   { 791, 792 },   { 751, 752 },   { 753, 754 },   { 757, 758 },   { 912, 913 },   { 914, 915 },
    { 916, 917 },   { 918, 919 },   { 920, 921 },   { 922, 923 },   { 924, 925 },   { 926, 927 },   { 928, 929 },
    { 930, 931 },   { 932, 933 },   { 934, 935 },   { 934, 936 },   { 934, 937 },   { 934, 938 },   { 934, 939 },
    { 934, 940 },   { 934, 941 },   { 934, 942 },   { 934, 943 },   { 934, 944 },   { 934, 945 },   { 934, 946 },
    { 934, 947 },   { 934, 948 },   { 934, 949 },   { 934, 950 },   { 934, 951 },   { 952, 953 },   { 952, 954 },
    { 952, 955 },   { 952, 956 },   { 952, 957 },   { 952, 958 },   { 959, 960 },   { 961, 962 },   { 963, 964 },
    { 965, 966 },   { 967, 968 },   { 969, 970 },   { 971, 972 },   { 817, 818 },   { 819, 820 },   { 973, 974 },
    { 975, 976 },   { 977, 978 },   { 979, 980 },   { 981, 982 },   { 981, 984 },   { 981, 986 },   { 987, 988 },
    { 989, 990 },   { 991, 992 },   { 993, 994 },   { 995, 996 },   { 840, 841 },   { 840, 842 },   { 840, 843 },
    { 840, 844 },   { 840, 845 },   { 840, 846 },   { 840, 847 },   { 848, 850 },   { 848, 851 },   { 848, 852 },
    { 848, 853 },   { 848, 854 },   { 848, 855 },   { 856, 857 },   { 856, 858 },   { 856, 859 },   { 856, 860 },
    { 856, 861 },   { 856, 862 },   { 863, 864 },   { 865, 866 },   { 867, 868 },   { 869, 870 },   { 848, 849 },
    { 821, 822 },   { 823, 824 },   { 397, 409 },   { 397, 412 },   { 397, 407 },   { 825, 999 },   { 827, 828 },
    { 829, 830 },   { 829, 830 },   { 829, 830 },   { 825, 826 },   { 1002, 1001 }, { 1004, 1003 }, { 1004, 1003 },
    { 1008, 1007 }, { 1010, 1009 }, { 1012, 1011 }, { 1014, 1013 }, { 1016, 1015 }, { 1018, 1017 }, { 825, 999 },
    { 825, 826 },   { 1020, 1019 },
};

u16 GetItemGraphicsDatID(u16 item, u32 type) {
    // BUG: 0xffff, the return arrow's icon, is past ITEM_LAST, so it becomes ITEM_NONE here and the checks for it below
    // never pass: it gets the empty icon
#ifdef BUGFIX
    if (item > ITEM_LAST && item != 0xffff) {
#else
    if (item > ITEM_LAST) {
#endif
        item = ITEM_NONE;
    }
    switch (type) {
    case ITEM_FILE_DATA:
        if (item == ITEM_NONE || item == 0xffff) {
            break;
        }
        return item;
    case ITEM_FILE_ICON_CHAR:
        if (item == ITEM_NONE) {
            return ICON_NONE_CHAR;
        }
        if (item == 0xffff) {
            return ICON_RETURN_CHAR;
        }
        return ITEM_GRAPHICS_CHARS_AND_PALETTES[item][0];
    case ITEM_FILE_ICON_PLTT:
        if (item == ITEM_NONE) {
            return ICON_NONE_PLTT;
        }
        if (item == 0xffff) {
            return ICON_RETURN_PLTT;
        }
        return ITEM_GRAPHICS_CHARS_AND_PALETTES[item][1];
    case ITEM_FILE_LIST_ICON_CHAR:
        return LIST_ITEM_GRAPHICS[ShooterItem_GetIndex(item)][0];
    case ITEM_FILE_LIST_ICON_PLTT:
        return LIST_ITEM_GRAPHICS[ShooterItem_GetIndex(item)][1];
    }
    return 0;
}

u32 PML_ItemGetIconArcID(void) {
    return ARCID_ITEMGRA;
}

u32 PML_ItemGetIconCellDatID(void) {
    return 1;
}

u32 PML_ItemGetIconAnimDatID(void) {
    return 0;
}

ArcTool *PML_ItemArcHandleCreate(HeapID heapId) {
    return GFL_ArcSysCreateFileHandle(ARCID_ITEMINFO, heapId);
}

ItemData *PML_ItemArcHandleReadFile(ArcTool *handle, u16 item, HeapID heapId) {
    if (item > ITEM_LAST) {
        item = ITEM_NONE;
    }
    return GFL_ArcToolReadHeapNew(handle, item, heapId);
}

void *PML_ItemReadDataFile(u16 item, u32 type, HeapID heapId) {
    if (item > ITEM_LAST) {
        item = ITEM_NONE;
    }
    switch (type) {
    case ITEM_FILE_DATA:
        return GFL_ArcSysReadHeapNew(ARCID_ITEMINFO, item, heapId);
    case ITEM_FILE_ICON_CHAR:
        return GFL_ArcSysReadHeapNew(ARCID_ITEMGRA, ITEM_GRAPHICS_CHARS_AND_PALETTES[item][0], heapId);
    case ITEM_FILE_ICON_PLTT:
        return GFL_ArcSysReadHeapNew(ARCID_ITEMGRA, ITEM_GRAPHICS_CHARS_AND_PALETTES[item][1], heapId);
    }
    return NULL;
}

void setItemNameToStrbuf(StrBuf *strbuf, u16 item, HeapID heapId) {
    MsgData *msgData;

    if (item > ITEM_LAST) {
        item = ITEM_NONE;
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_ITEM_NAMES, heapId);
    GFL_MsgDataLoadStrbuf(msgData, item, strbuf);
    GFL_MsgDataFree(msgData);
}

void setItemDescriptionTextToStrbuf(StrBuf *strbuf, u16 item, HeapID heapId) {
    MsgData *msgData;

    if (item > ITEM_LAST) {
        item = ITEM_NONE;
    }
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_ITEM_DESCRIPTIONS, heapId);
    GFL_MsgDataLoadStrbuf(msgData, item, strbuf);
    GFL_MsgDataFree(msgData);
}

s32 GetItemParam(u16 item, u32 param, HeapID heapId) {
    ItemData *data = PML_ItemReadDataFile(item, ITEM_FILE_DATA, HEAPID_TAIL(heapId));
    s32 value = PML_ItemGetParam(data, param);

    GFL_HeapFree(data);
    return value;
}

s32 PML_ItemGetParam(ItemData *data, u32 param) {
    switch (param) {
    case ITEM_PARAM_PRICE:
        return data->price * 10;
    case ITEM_PARAM_HOLD_EFFECT:
        return data->holdEffect;
    case ITEM_PARAM_HOLD_PARAM:
        return data->holdParam;
    case ITEM_PARAM_IMPORTANT:
        return data->important;
    case ITEM_PARAM_REGISTRABLE:
        return data->registrable;
    case ITEM_PARAM_FIELD_POCKET:
        return data->fieldPocket;
    case ITEM_PARAM_FIELD_FUNC:
        return data->fieldFunc;
    case ITEM_PARAM_BATTLE_FUNC:
        return data->battleFunc;
    case ITEM_PARAM_PLUCK_EFFECT:
        return data->pluckEffect;
    case ITEM_PARAM_FLING_EFFECT:
        return data->flingEffect;
    case ITEM_PARAM_FLING_POWER:
        return data->flingPower;
    case ITEM_PARAM_NATURAL_GIFT_POWER:
        return data->naturalGiftPower;
    case ITEM_PARAM_NATURAL_GIFT_TYPE:
        return data->naturalGiftType;
    case ITEM_PARAM_BATTLE_POCKET:
        return data->battlePocket;
    case ITEM_PARAM_WORK_TYPE:
        return data->workType;
    case ITEM_PARAM_KIND:
        return data->kind;
    case ITEM_PARAM_UNK_E:
        return data->unkE;
    case ITEM_PARAM_SORT_INDEX:
        return data->sortIndex;
    default:
        switch (data->workType) {
        case ITEM_WORK_VALUE:
            return data->work.value;
        case ITEM_WORK_PARAMS:
            return PML_ItemGetBattleStat(&data->work.params, param);
        }
        return 0;
    }
}

static s32 PML_ItemGetBattleStat(ItemParams *params, u32 param) {
    switch (param) {
    case ITEM_PARAM_SLEEP_HEAL:
        return params->sleepHeal;
    case ITEM_PARAM_POISON_HEAL:
        return params->poisonHeal;
    case ITEM_PARAM_BURN_HEAL:
        return params->burnHeal;
    case ITEM_PARAM_FREEZE_HEAL:
        return params->freezeHeal;
    case ITEM_PARAM_PARALYSIS_HEAL:
        return params->paralysisHeal;
    case ITEM_PARAM_CONFUSION_HEAL:
        return params->confusionHeal;
    case ITEM_PARAM_INFATUATION_HEAL:
        return params->infatuationHeal;
    case ITEM_PARAM_GUARD_SPEC:
        return params->guardSpec;
    case ITEM_PARAM_REVIVE:
        return params->revive;
    case ITEM_PARAM_REVIVE_ALL:
        return params->reviveAll;
    case ITEM_PARAM_LEVEL_UP:
        return params->levelUp;
    case ITEM_PARAM_EVOLVE:
        return params->evolve;
    case ITEM_PARAM_ATTACK_STAGES:
        return params->attackStages;
    case ITEM_PARAM_DEFENSE_STAGES:
        return params->defenseStages;
    case ITEM_PARAM_SP_ATTACK_STAGES:
        return params->spAttackStages;
    case ITEM_PARAM_SP_DEFENSE_STAGES:
        return params->spDefenseStages;
    case ITEM_PARAM_SPEED_STAGES:
        return params->speedStages;
    case ITEM_PARAM_ACCURACY_STAGES:
        return params->accuracyStages;
    case ITEM_PARAM_CRIT_STAGES:
        return params->critStages;
    case ITEM_PARAM_PP_UP:
        return params->ppUp;
    case ITEM_PARAM_PP_MAX:
        return params->ppMax;
    case ITEM_PARAM_PP_RESTORE:
        return params->ppRestore;
    case ITEM_PARAM_PP_RESTORE_ALL:
        return params->ppRestoreAll;
    case ITEM_PARAM_HP_RESTORE:
        return params->hpRestore;
    case ITEM_PARAM_HP_EV_UP:
        return params->hpEVUp;
    case ITEM_PARAM_ATTACK_EV_UP:
        return params->attackEVUp;
    case ITEM_PARAM_DEFENSE_EV_UP:
        return params->defenseEVUp;
    case ITEM_PARAM_SPEED_EV_UP:
        return params->speedEVUp;
    case ITEM_PARAM_SP_ATTACK_EV_UP:
        return params->spAttackEVUp;
    case ITEM_PARAM_SP_DEFENSE_EV_UP:
        return params->spDefenseEVUp;
    case ITEM_PARAM_FRIENDSHIP_UP_1:
        return params->friendshipUp1;
    case ITEM_PARAM_FRIENDSHIP_UP_2:
        return params->friendshipUp2;
    case ITEM_PARAM_FRIENDSHIP_UP_3:
        return params->friendshipUp3;
    case ITEM_PARAM_UNK_51:
        return params->unk6_4;
    case ITEM_PARAM_HP_EV:
        return params->hpEV;
    case ITEM_PARAM_ATTACK_EV:
        return params->attackEV;
    case ITEM_PARAM_DEFENSE_EV:
        return params->defenseEV;
    case ITEM_PARAM_SPEED_EV:
        return params->speedEV;
    case ITEM_PARAM_SP_ATTACK_EV:
        return params->spAttackEV;
    case ITEM_PARAM_SP_DEFENSE_EV:
        return params->spDefenseEV;
    case ITEM_PARAM_HP_RESTORE_AMOUNT:
        return params->hpRestoreAmount;
    case ITEM_PARAM_PP_RESTORE_AMOUNT:
        return params->ppRestoreAmount;
    case ITEM_PARAM_FRIENDSHIP_1:
        return params->friendship1;
    case ITEM_PARAM_FRIENDSHIP_2:
        return params->friendship2;
    case ITEM_PARAM_FRIENDSHIP_3:
        return params->friendship3;
    }
    return 0;
}

BOOL PML_ItemIsTMHM(u16 item) {
    if ((item >= ITEM_TM01 && item <= ITEM_HM06) || (item >= ITEM_TM93 && item <= ITEM_TM95)) {
        return TRUE;
    }
    return FALSE;
}

BOOL PML_ItemIsTM(u16 item) {
    if ((item >= ITEM_TM01 && item <= ITEM_TM92) || (item >= ITEM_TM93 && item <= ITEM_TM95)) {
        return TRUE;
    }
    return FALSE;
}

u16 PML_ItemGetTMWazaID(u16 item) {
    if (PML_ItemIsTMHM(item) == FALSE) {
        return MOVE_NONE;
    }
    if (item >= ITEM_TM93) {
        item -= ITEM_TM93 - TM_INDEX_TM93;
    } else {
        item -= ITEM_TM01;
    }
    return TM_MOVE_LIST[item];
}

BOOL PML_MoveIsHM(u16 move) {
    u8 i;

    for (i = 0; i < HM_COUNT; i++) {
        if (move == TM_MOVE_LIST[TM_INDEX_HM01 + i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 PML_ItemGetTMBitMask(u16 item) {
    if (PML_ItemIsTMHM(item) == FALSE) {
        return 0xff;
    }
    // The bits run TM01 to TM92, then TM93 to TM95 and the HMs: the opposite of TM_MOVE_LIST's order
    if (item >= ITEM_TM93) {
        return item - ITEM_TM93 + 92;
    }
    if (item >= ITEM_HM01) {
        return item - ITEM_HM01 + 95;
    }
    return item - ITEM_TM01;
}

u8 PML_ItemGetHMID(u16 item) {
    if (item >= ITEM_HM01 && item <= ITEM_HM06) {
        return item - ITEM_HM01;
    }
    return 0xff;
}

BOOL PML_ItemIsMail(u16 item) {
    u32 i;

    for (i = 0; i < NELEMS(MAIL_ITEM_IDS); i++) {
        if (item == MAIL_ITEM_IDS[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 PML_ItemGetMailID(u16 item) {
    u32 i;

    for (i = 0; i < NELEMS(MAIL_ITEM_IDS); i++) {
        if (item == MAIL_ITEM_IDS[i]) {
            return i;
        }
    }
    return 0;
}

u16 PML_ItemGetMailItemID(u8 mail) {
    if (mail >= NELEMS(MAIL_ITEM_IDS)) {
        return ITEM_NONE;
    }
    return MAIL_ITEM_IDS[mail];
}

BOOL PML_ItemIsBerry(u16 item) {
    u32 i;

    for (i = 0; i < NELEMS(BERRY_ITEM_IDS); i++) {
        if (item == BERRY_ITEM_IDS[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL PML_ItemIsNotSpecialMonsball(u16 item) {
    if (item < ITEM_FAST_BALL || item > ITEM_PARK_BALL) {
        return TRUE;
    }
    return FALSE;
}

// Converts a ball item to its ball ID (dir 0) or back (dir 1), or gives 0
static u16 PML_ItemMonsBallConvID(u16 id, u32 dir) {
    u32 i;

    for (i = 0; i < NELEMS(MONS_BALL_ITEMS); i++) {
        if (id == MONS_BALL_ITEMS[i][dir]) {
            return MONS_BALL_ITEMS[i][dir ^ 1];
        }
    }
    return 0;
}

u16 PML_ItemGetMonsBallID(u16 item) {
    return PML_ItemMonsBallConvID(item, 0);
}

u32 PML_ItemIsUnholdable(u16 item) {
    if (item > ITEM_LAST) {
        return FALSE;
    }
    return UNHOLDABLE_ITEMS[item / (sizeof(u32) * 8)] & (1 << (item % 32));
}

u32 PML_ItemIsDummy(u16 item) {
    if (item > ITEM_LAST) {
        return TRUE;
    }
    return DUMMY_ITEMS[item / (sizeof(u32) * 8)] & (1 << (item % 32));
}

BOOL PML_ItemIsB2W2Only(u16 item) {
    if (item >= ITEM_MEDAL_BOX) {
        return TRUE;
    }
    return FALSE;
}
