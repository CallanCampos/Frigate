#ifndef POKEBW2_APP_WF2DMAP_WF2DMAP_JUDGE_H
#define POKEBW2_APP_WF2DMAP_WF2DMAP_JUDGE_H

// Decides whether an object of the Wi-Fi Club's 2D map (overlay 203) can do what its player asks. The ROM has no name
// for this file: it is named after the older games' wf2dmap_judge.c, whose place in the library it has

#include "types.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_map.h"
#include "app/wf2dmap/wf2dmap_obj.h"

// Fills act and returns TRUE when the request is granted
BOOL WF2DMAP_JUDGESysCmdJudge(const WF2DMAP_MAPSYS *mapSys, const WF2DMAP_OBJSYS *objSys, const WF2DMAP_REQCMD *req,
                              WF2DMAP_ACTCMD *act);

#endif // POKEBW2_APP_WF2DMAP_WF2DMAP_JUDGE_H
