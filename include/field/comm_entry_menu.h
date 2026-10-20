#ifndef POKEBW2_FIELD_COMM_ENTRY_MENU_H
#define POKEBW2_FIELD_COMM_ENTRY_MENU_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The field's menu that gathers a group over the wireless network: the leader waits for the players who ask to join
// and accepts or refuses each, and the others pick a leader from the beacons they hear or ask the one they talked to.
// When the leader proceeds, everyone synchronizes and the activity starts

// The activity the group is for, which picks the messages
enum {
    COMM_ENTRY_MENU_MULTI_BATTLE, // the Union Room's multi battles
    COMM_ENTRY_MENU_MUSICAL,
    COMM_ENTRY_MENU_BATTLE_SUBWAY,
};

// What the menu does: gather the group as its leader, pick a leader from the beacons heard and ask to join, or join
// the leader the network is already connected to, without asking or asking
enum {
    COMM_ENTRY_MENU_LEADER,
    COMM_ENTRY_MENU_SELECT_LEADER,
    COMM_ENTRY_MENU_JOIN,
    COMM_ENTRY_MENU_ASK_TO_JOIN,
};

// What CommEntryMenu_Update returns: still running, the group is complete, cancelled, refused by the leader, or a
// network error
enum {
    COMM_ENTRY_MENU_RUNNING,
    COMM_ENTRY_MENU_DONE,
    COMM_ENTRY_MENU_CANCELLED,
    COMM_ENTRY_MENU_REFUSED,
    COMM_ENTRY_MENU_ERROR,
};

// Called with the work of CommEntryMenu_SetCallbacks when a player joins the group or leaves it
typedef void (*CommEntryMenuCallback)(void *work, const PlayerInfo *info, const u8 *mac);

// Creates the menu of a kind (COMM_ENTRY_MENU_LEADER...) for an activity (COMM_ENTRY_MENU_MULTI_BATTLE...), for
// minCount to maxCount players, with this player's info and a MAC address that it keeps (or NULL)
CommEntryMenu *CommEntryMenu_Create(const PlayerInfo *info, Field *field, u32 minCount, u32 maxCount, HeapID heapId,
                                    u32 kind, u32 mode, const u8 *mac);
// Frees the menu and returns the players who joined, as bits by network ID
u32 CommEntryMenu_Free(CommEntryMenu *menu);
// Sets the callbacks for the players who join and leave, which only the Union Room uses
void CommEntryMenu_SetCallbacks(CommEntryMenu *menu, void *work, CommEntryMenuCallback onJoin,
                                CommEntryMenuCallback onLeave);
// Adds the player of a network ID who asked to join, to accept at once when autoAccept is TRUE or else to ask about
void CommEntryMenu_AddEntry(CommEntryMenu *menu, u32 netId, const PlayerInfo *info, u8 autoAccept, const u8 *mac);
// Runs the menu each frame, returning a COMM_ENTRY_MENU_* result
u32 CommEntryMenu_Update(CommEntryMenu *menu);

#endif // POKEBW2_FIELD_COMM_ENTRY_MENU_H
