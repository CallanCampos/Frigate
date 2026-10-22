#include "dwc/qr2regkeys.h"
#include "dwc/ext.h"

// GameSpy's QR2 key names (qr2regkeys.c). The ROM names neither the file nor its functions: the file,
// qr2_register_keyA and qr2_registered_key_list are GameSpy's, as pret's pokerevo Wii build has them (qr2regkeys.s),
// and qr2_internal_is_master_only_key is the later SDK's

const char *qr2_registered_key_list[MAX_REGISTERED_KEYS] = {
    "",           "hostname",  "gamename",     "gamever",  "hostport", "mapname",   "gametype",      "gamevariant",
    "numplayers", "numteams",  "maxplayers",   "gamemode", "teamplay", "fraglimit", "teamfraglimit", "timeelapsed",
    "timelimit",  "roundtime", "roundelapsed", "password", "groupid",  "player_",   "score_",        "skill_",
    "ping_",      "team_",     "deaths_",      "pid_",     "team_t",   "score_t",   "nn_groupid",    "country",
    "region",
};

gsi_bool qr2_internal_is_master_only_key(const char *key) {
    if (strcmp(key, qr2_registered_key_list[31]) == 0 || strcmp(key, qr2_registered_key_list[32]) == 0) {
        return gsi_true;
    }
    return gsi_false;
}

void qr2_register_keyA(int keyid, const char *key) {
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_StackTrace, "qr2_register_keyA()\r\n");
    if (keyid < 50 || keyid > 254) {
        gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_WarmError,
                      "Attempted to register invalid key %d - %s\r\n", keyid, key);
        return;
    }
    gsDebugFormat(GSIDebugCat_QR2, GSIDebugType_Misc, GSIDebugLevel_Comment, "Registered key %d - %s\r\n", keyid, key);
    qr2_registered_key_list[keyid] = key;
}
