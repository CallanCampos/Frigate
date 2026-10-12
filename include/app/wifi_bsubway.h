#ifndef POKEBW2_APP_WIFI_BSUBWAY_H
#define POKEBW2_APP_WIFI_BSUBWAY_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Wi-Fi Battle Subway: the screen that connects to the Battle Subway's server to send the player's Wi-Fi train
// record, or to download the trainers of a Wi-Fi train or the leaders of one. The names are ours
#define OVERLAY_WIFI_BSUBWAY OVERLAY_ID(191)

// The screen's heap, a child of HEAPID_USER
#define HEAPID_WIFI_BSUBWAY 0x7d

// What the screen does
enum {
    // Sends the player's record and team of the last Wi-Fi train
    WIFI_BSUBWAY_MODE_UPLOAD,
    // Downloads the trainers of a train of the player's rank, to challenge it
    WIFI_BSUBWAY_MODE_DOWNLOAD_TRAIN,
    // Downloads the leaders of a train of any rank, to look at them
    WIFI_BSUBWAY_MODE_DOWNLOAD_LEADERS,
};

// How the screen ended
enum {
    WIFI_BSUBWAY_RESULT_OK,
    // An error ended it
    WIFI_BSUBWAY_RESULT_ERROR,
    // The player gave up, at the login or at a question
    WIFI_BSUBWAY_RESULT_CANCEL,
};

// The proc's parameter
typedef struct {
    // A WIFI_BSUBWAY_MODE_*
    u32 mode;
    GameSystem *gsys;
    // A WIFI_BSUBWAY_RESULT_*, which the proc sets
    u32 result;
} WifiBSubwayParam;

extern const GameProcFunctions WIFI_BSUBWAY_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFI_BSUBWAY_H
