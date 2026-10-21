#ifndef POKEBW2_FIELD_GIMMICK_SKYARROW_BRIDGE_H
#define POKEBW2_FIELD_GIMMICK_SKYARROW_BRIDGE_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Skyarrow Bridge's zone 249 (gimmick 1), overlay 109: five models moving along curves, three of
// them with a 3D sound, the zone's music fading in as the camera climbs, and a sound when the player passes a point on
// the way down. Overlay 36's gimmick table calls these
void SkyarrowBridge_Setup(Field *field);
void SkyarrowBridge_End(Field *field);
void SkyarrowBridge_Update(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_SKYARROW_BRIDGE_H
