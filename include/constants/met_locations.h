#ifndef POKEBW2_CONSTANTS_MET_LOCATIONS_H
#define POKEBW2_CONSTANTS_MET_LOCATIONS_H

// Where a Pokémon was met or its egg was obtained, as PKM_PARAM_MET_LOCATION and PKM_PARAM_EGG_LOCATION hold them.
// Below 30000 they are places in Unova, from 30001 the special places, in the order of their names, and above
// LOCATION_EXTERNAL_BASE the people and places an egg can be received from. The names are ours

// No egg location: the Pokémon wasn't hatched
#define LOCATION_NONE 0
#define LOCATION_POKE_TRANSFER 30001
// Both trades are named "Link Trade"
#define LOCATION_IN_GAME_TRADE 30002
#define LOCATION_LINK_TRADE 30003
#define LOCATION_KANTO 30004
#define LOCATION_JOHTO 30005
#define LOCATION_HOENN 30006
#define LOCATION_SINNOH 30007
#define LOCATION_FARAWAY 30008
#define LOCATION_UNKNOWN 30009
// The event Celebi and legendary beasts, before and after the event they start (special_transfers kinds 0 to 3)
#define LOCATION_EVENT_CELEBI 30010
#define LOCATION_EVENT_CELEBI_USED 30011
#define LOCATION_EVENT_BEASTS 30012
#define LOCATION_EVENT_BEASTS_USED 30013
#define LOCATION_ENTRALINK 30014
#define LOCATION_DREAM_RADAR 30015
#define LOCATION_EXTERNAL_BASE 60000

#endif // POKEBW2_CONSTANTS_MET_LOCATIONS_H
