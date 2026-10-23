#ifndef POKEBW2_FIELD_FEST_MISSION_SPARKLE_H
#define POKEBW2_FIELD_FEST_MISSION_SPARKLE_H

// Overlay 71: the Funfest missions of type 4, where the player looks for the sparkles hidden in the zones around.
// The name is a guess (no string in the overlay names it)

#include "types.h"
#include "field/field_effect.h"
#include "field/zone.h"
#include "nitro/fx.h"
#include "struct_decls.h"

#define FEST_SPARKLE_MAX 8

// The sparkles already found in one zone
typedef struct {
    u16 zoneId;
    u16 unk2;
    u8 found[FEST_SPARKLE_MAX];
} FestSparkleZone;

// The mission's work, which the festival keeps (LinkFestival_GetEntries) and overlay 24 fills on entering a zone
typedef struct {
    u16 zoneId;
    u8 unk2;
    u8 count;
    // The places a sparkle can be, and which of them each sparkle is at
    ZoneBGEntity places[20];
    u8 placeIndex[FEST_SPARKLE_MAX];
    // What a sparkle gives, passed to the script that finds it
    u16 values[FEST_SPARKLE_MAX];
    int zoneCount;
    FestSparkleZone zones[6];
} FestSparkleWork;

void FestSparkle_Show(FesGimmick *gimmick, FestSparkleWork *work);
BOOL FestSparkle_End(FesGimmick *gimmick, FestSparkleWork *work);
// The event of picking up a sparkle at a grid or a rail position, or NULL
GameEvent *FestSparkle_CheckPickUpGrid(FesGimmick *gimmick, const VecFx32 *position);
GameEvent *FestSparkle_CheckPickUpRail(FesGimmick *gimmick, const RailPosition *position);
// Whether a sparkle not yet found is on the tile, within 20 units of height
BOOL FestSparkle_IsOnTile(FesGimmick *gimmick, u16 x, u16 z, fx32 height);

#endif // POKEBW2_FIELD_FEST_MISSION_SPARKLE_H
