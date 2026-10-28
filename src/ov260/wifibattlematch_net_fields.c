#include "types.h"
#include "app/wifibattlematch_net.h"

// wifibattlematch_net_fields.c, the file that follows wifibattlematch_net.c in overlay 260 (named descriptively: the
// ROM gives it no string). It holds the names of the fields of the Sake tables that the Wi-Fi battle competitions keep
// their arena stats and the Wi-Fi cup's in, as the strings that the requests use

// The names are listed in the order that lays their string literals out as the ROM's .data has them: MWCC sorts a
// file's literals by size, so the equal-size ones come out in an order that depends on where they are listed. Each
// designator is the name's place in the table, and the order of the lines is free
static const char *sArenaFieldNames[] = {
    [0] = "ARENA_ELO_RATING_1V1_DOUBLE",
    [1] = "ARENA_ELO_RATING_1V1_ROTATE",
    [2] = "ARENA_ELO_RATING_1V1_SHOOTER",
    [3] = "ARENA_ELO_RATING_1V1_SINGLE",
    [6] = "CHEATS_COUNTER",
    [4] = "ARENA_ELO_RATING_1V1_TRIPLE",
    [5] = "ARENA_ELO_RATING_1V1_WIFICUP",
    [7] = "COMPLETE_MATCHES_COUNTER",
    [9] = "NUM_DOUBLE_LOSE_COUNTER",
    [8] = "DISCONNECTS_COUNTER",
    [10] = "NUM_DOUBLE_WIN_COUNTER",
    [11] = "NUM_ROTATE_LOSE_COUNTER",
    [15] = "NUM_SINGLE_LOSE_COUNTER",
    [12] = "NUM_ROTATE_WIN_COUNTER",
    [16] = "NUM_SINGLE_WIN_COUNTER",
    [17] = "NUM_TRIPLE_LOSE_COUNTER",
    [18] = "NUM_TRIPLE_WIN_COUNTER",
    [13] = "NUM_SHOOTER_LOSE_COUNTER",
    [14] = "NUM_SHOOTER_WIN_COUNTER",
    [19] = "RECORD_SAVE_IDX",
    [20] = "recordid",
};

static const char *sWifiCupFieldNames[] = {
    "ARENA_ELO_RATING_1V1_WIFICUP",
    "CHEATS_WIFICUP_COUNTER",
    "DISCONNECTS_WIFICUP_COUNTER",
    "NUM_WIFICUP_LOSE_COUNTER",
    "NUM_WIFICUP_WIN_COUNTER",
    "RECORD_SAVE_IDX",
    "recordid",
    "WIFICUP_POKEMON_PARTY",
};

const char **WifiBattleMatchNet_GetArenaFieldNames(void) {
    return sArenaFieldNames;
}

int WifiBattleMatchNet_GetArenaFieldCount(void) {
    return sizeof(sArenaFieldNames) / sizeof(sArenaFieldNames[0]);
}

const char **WifiBattleMatchNet_GetWifiCupFieldNames(void) {
    return sWifiCupFieldNames;
}

int WifiBattleMatchNet_GetWifiCupFieldCount(void) {
    return sizeof(sWifiCupFieldNames) / sizeof(sWifiCupFieldNames[0]);
}

const char *WifiBattleMatchNet_GetArenaFieldName(WifiBattleMatchNetArenaField field) {
    if (field <= 0) {
        return "";
    }
    if (field == ARENA_ADVANCED_MODIFIER) {
        return "ARENA_ADVANCED_MODIFIER";
    }
    if (field == ARENA_ELO_RATING_1V1_DOUBLE) {
        return "ARENA_ELO_RATING_1V1_DOUBLE";
    }
    if (field == ARENA_ELO_RATING_1V1_ROTATE) {
        return "ARENA_ELO_RATING_1V1_ROTATE";
    }
    if (field == ARENA_ELO_RATING_1V1_SHOOTER) {
        return "ARENA_ELO_RATING_1V1_SHOOTER";
    }
    if (field == ARENA_ELO_RATING_1V1_SINGLE) {
        return "ARENA_ELO_RATING_1V1_SINGLE";
    }
    if (field == ARENA_ELO_RATING_1V1_TRIPLE) {
        return "ARENA_ELO_RATING_1V1_TRIPLE";
    }
    if (field == ARENA_ELO_RATING_1V1_WIFICUP) {
        return "ARENA_ELO_RATING_1V1_WIFICUP";
    }
    if (field == ARENA_ELO_RATING_MAXIMUM_STAT) {
        return "ARENA_ELO_RATING_MAXIMUM_STAT";
    }
    if (field == ARENA_ELO_RATING_MINIMUM_STAT) {
        return "ARENA_ELO_RATING_MINIMUM_STAT";
    }
    if (field == ARENA_FLOOR_MODIFIER) {
        return "ARENA_FLOOR_MODIFIER";
    }
    if (field == ARENA_INITIAL_RATING) {
        return "ARENA_INITIAL_RATING";
    }
    if (field == ARENA_INTERMED_MODIFIER) {
        return "ARENA_INTERMED_MODIFIER";
    }
    if (field == ARENA_MASTER_MODIFIER) {
        return "ARENA_MASTER_MODIFIER";
    }
    if (field == ARENA_RATING_DIFF_HIGH) {
        return "ARENA_RATING_DIFF_HIGH";
    }
    if (field == ARENA_RATING_DIFF_LOW) {
        return "ARENA_RATING_DIFF_LOW";
    }
    if (field == ARENA_SCORE_WEIGHT_NORMAL) {
        return "ARENA_SCORE_WEIGHT_NORMAL";
    }
    if (field == ARENA_CHEATS_COUNTER) {
        return "CHEATS_COUNTER";
    }
    if (field == ARENA_CHEATS_WIFICUP_COUNTER) {
        return "CHEATS_WIFICUP_COUNTER";
    }
    if (field == ARENA_COMPLETE_MATCHES_COUNTER) {
        return "COMPLETE_MATCHES_COUNTER";
    }
    if (field == ARENA_DISCONNECTS_COUNTER) {
        return "DISCONNECTS_COUNTER";
    }
    if (field == ARENA_DISCONNECTS_WIFICUP_COUNTER) {
        return "DISCONNECTS_WIFICUP_COUNTER";
    }
    if (field == ARENA_NUM_DOUBLE_LOSE) {
        return "NUM_DOUBLE_LOSE_COUNTER";
    }
    if (field == ARENA_NUM_DOUBLE_WIN) {
        return "NUM_DOUBLE_WIN_COUNTER";
    }
    if (field == ARENA_NUM_ROTATE_LOSE) {
        return "NUM_ROTATE_LOSE_COUNTER";
    }
    if (field == ARENA_NUM_ROTATE_WIN) {
        return "NUM_ROTATE_WIN_COUNTER";
    }
    if (field == ARENA_NUM_SHOOTER_LOSE) {
        return "NUM_SHOOTER_LOSE_COUNTER";
    }
    if (field == ARENA_NUM_SHOOTER_WIN) {
        return "NUM_SHOOTER_WIN_COUNTER";
    }
    if (field == ARENA_NUM_SINGLE_LOSE) {
        return "NUM_SINGLE_LOSE_COUNTER";
    }
    if (field == ARENA_NUM_SINGLE_WIN) {
        return "NUM_SINGLE_WIN_COUNTER";
    }
    if (field == ARENA_NUM_TRIPLE_LOSE) {
        return "NUM_TRIPLE_LOSE_COUNTER";
    }
    if (field == ARENA_NUM_TRIPLE_WIN) {
        return "NUM_TRIPLE_WIN_COUNTER";
    }
    if (field == ARENA_NUM_WIFICUP_LOSE) {
        return "NUM_WIFICUP_LOSE_COUNTER";
    }
    if (field == ARENA_NUM_WIFICUP_WIN) {
        return "NUM_WIFICUP_WIN_COUNTER";
    }
    if (field == ARENA_NUM_WINLOSE_COUNTER_MINIMUM_STAT) {
        return "NUM_WINLOSE_COUNTER_MINIMUM_STAT";
    }
    return "";
}
