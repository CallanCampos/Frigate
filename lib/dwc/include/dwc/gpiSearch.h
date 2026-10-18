#ifndef POKEBW2_DWC_GPISEARCH_H
#define POKEBW2_DWC_GPISEARCH_H

#include "types.h"
#include "dwc/gpi.h"

// The searches of GameSpy's GP (gpiSearch.c), in overlay 11, on their own sockets to the search manager (see dwc/gp.h
// for the names)

// The kinds of search: profiles by their info, whether an e-mail is valid, the nicks of an e-mail, the players of a
// product, a check of a nick and an e-mail, a new user, the profiles that have this one as a buddy (with their info, or
// with their unique nicks for a list of profiles), unique nicks to suggest, profiles by their unique nick, and the
// "\\profilelist\\" of a profile (not in GameSpy's GP; the name is ours)
#define GPI_SEARCH_PROFILE 1
#define GPI_SEARCH_IS_VALID 2
#define GPI_SEARCH_NICKS 3
#define GPI_SEARCH_PLAYERS 4
#define GPI_SEARCH_CHECK 5
#define GPI_SEARCH_NEWUSER 6
#define GPI_SEARCH_OTHERS_BUDDY 7
#define GPI_SEARCH_SUGGEST_UNIQUE 8
#define GPI_SEARCH_OTHERS_BUDDY_LIST 9
#define GPI_SEARCH_PROFILE_UNIQUENICK 10
#define GPI_SEARCH_PROFILE_LIST 11

int gpiProfileSearch(GPConnection *connection, const char *nick, const char *uniquenick, const char *email,
                     const char *firstname, const char *lastname, int icquin, int skip, int blocking,
                     GPCallback callback, void *param);
int gpiOthersBuddyList(GPConnection *connection, int *profiles, int numOfProfiles, int blocking, GPCallback callback,
                       void *param);
int gpiProcessSearches(GPConnection *connection);

#endif // POKEBW2_DWC_GPISEARCH_H
