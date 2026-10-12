#ifndef POKEBW2_APP_FESTIVAL_H
#define POKEBW2_APP_FESTIVAL_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_FESTIVAL_APP OVERLAY_ID(309)

struct FestivalEventParam {
    GameSystem *gsys;
    GameData *gameData;
    u32 mode;
    LinkFestival *festival;
    FestMissionConfig *missionConfig;
    void *saveBlock;
};

// The Funfest mission screen (fmission_app.c): mode 0 shows the running mission, mode 1 ends it and shows its result
extern const GameProcFunctions FMISSION_APP_PROC_FUNCTIONS;

#endif // POKEBW2_APP_FESTIVAL_H
