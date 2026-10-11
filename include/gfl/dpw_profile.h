#ifndef POKEBW2_GFL_DPW_PROFILE_H
#define POKEBW2_GFL_DPW_PROFILE_H

#include "types.h"
#include "dpw/dpw_tr.h"
#include "struct_decls.h"

// dpw_profile.c, in overlay 189, named after what it does: the one Game Freak function between nhttp_rap_evilcheck.c
// and the HTTP library, which fills the profile that the trade server keeps from the player's

// Fills a profile from the player's, with no e-mail address
void DpwProfile_Fill(Dpw_Common_Profile *profile, PlayerInfo *playerInfo);

#endif // POKEBW2_GFL_DPW_PROFILE_H
