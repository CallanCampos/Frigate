#ifndef POKEBW2_SYSTEM_APP_MENU_COMMON_H
#define POKEBW2_SYSTEM_APP_MENU_COMMON_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The graphics that the game's menus share (app_menu_common.c, a guessed name): the archive's id, and the files of
// its icons. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except
// AppMenuCommon_LoadBarScreen and AppMenuCommon_GetStatusIcon.
//
// Most icons come as four files: a palette and a character file at fixed indices, then a cell and an animation file
// for each of the three OBJ VRAM mappings (32, 64 and 128 KB), picked by `mapping`.

// The status icons, as AppMenuCommon_GetStatusIcon returns them
enum {
    APP_STATUS_ICON_PARALYSIS = 1,
    APP_STATUS_ICON_FREEZE,
    APP_STATUS_ICON_SLEEP,
    APP_STATUS_ICON_POISON,
    APP_STATUS_ICON_BURN,
    APP_STATUS_ICON_FAINTED,
    APP_STATUS_ICON_NONE = 8,
};

BOOL BeaconStatus_IsLocked(void);
void BeaconStatus_DoNothing(void);
u32 getUINarcIdx(void);

// The type and contest category icons: the palette, each icon's palette within it, the icons, cells and animations
u32 AppMenuCommon_GetTypeIconPaletteFile(void);
u8 AppMenuCommon_GetTypeIconPaletteIndex(u8 type);
u32 AppMenuCommon_GetTypeIconCharFile(u8 type);
u32 AppMenuCommon_GetTypeIconCellFile(u32 mapping);
u32 AppMenuCommon_GetTypeIconAnimFile(u32 mapping);
u8 AppMenuCommon_GetCategoryIconPaletteIndex(u8 index);
u32 AppMenuCommon_GetCategoryIconCharFile(u8 index);

// The touch bar's icons: their palette, characters, and cells and animations for an OBJ VRAM mapping mode
u32 AppMenuCommon_GetTouchBarPaletteFile(void);
u32 AppMenuCommon_GetTouchBarCharFile(void);
u32 AppMenuCommon_GetTouchBarCellFile(u32 mapping);
u32 AppMenuCommon_GetTouchBarAnimFile(u32 mapping);
// The touch bar's BG: its palette, characters and screen
u32 AppMenuCommon_GetBarBgPaletteFile(void);
u32 AppMenuCommon_GetBarBgCharFile(void);
u32 AppMenuCommon_GetBarScreenFile(void);
u32 AppMenuCommon_GetStatusExtraFileId(void);

// Loads the menu bar's screen into the bottom three rows of bg's screen buffer, with its characters starting at
// charBase and in palette `palette`
void AppMenuCommon_LoadBarScreen(ArcTool *arc, u8 bg, HeapID heapId, u32 charBase, u32 palette);

u32 AppMenuCommon_GetItemIconPaletteFile(void);
u32 AppMenuCommon_GetItemIconCharFile(void);
u32 AppMenuCommon_GetItemIconCellFile(u32 mapping);
u32 AppMenuCommon_GetItemIconAnimFile(u32 mapping);
u32 AppMenuCommon_GetMModelIconPaletteFile(void);
u32 AppMenuCommon_GetMModelIconCharFile(void);
u32 AppMenuCommon_GetMModelIconCellFile(u32 mapping);
u32 AppMenuCommon_GetMModelIconAnimFile(u32 mapping);
u32 AppMenuCommon_GetStatusIconPaletteFile(void);
u32 AppMenuCommon_GetStatusIconCharFile(void);
u32 AppMenuCommon_GetStatusIconCellFile(u32 mapping);
u32 AppMenuCommon_GetStatusIconAnimFile(u32 mapping);

// The status icon (APP_STATUS_ICON_*) of a party Pokémon: fainted, its status condition, or none
u32 AppMenuCommon_GetStatusIcon(PartyPkm *pkm);

u32 AppMenuCommon_GetPokemonSpriteCellFile(u32 mapping);
u32 AppMenuCommon_GetPokemonSpriteAnimFile(u32 mapping);
u32 AppMenuCommon_GetTrainerSpriteCellFile(u32 mapping);
u32 AppMenuCommon_GetTrainerSpriteAnimFile(u32 mapping);
// The files of a Poké Ball's icon: its palette and characters, where ball 0 counts as 4, then its cells and
// animations, the same for every ball
u32 AppMenuCommon_GetBallPaletteFile(u32 ball);
u32 AppMenuCommon_GetBallCharFile(u32 ball);
u32 AppMenuCommon_GetBallCellFile(u32 ball, u32 mapping);
u32 AppMenuCommon_GetBallAnimFile(u32 ball, u32 mapping);
u32 AppMenuCommon_GetMarkingIconPaletteFile(void);
u32 AppMenuCommon_GetMarkingIconCharFile(u32 mapping);
u32 AppMenuCommon_GetMarkingIconCellFile(u32 mapping);
u32 AppMenuCommon_GetMarkingIconAnimFile(u32 mapping);
u32 AppMenuCommon_GetHpBarPaletteFile(void);
u32 AppMenuCommon_GetHpBarCharFile(u32 mapping);
u32 AppMenuCommon_GetHpBarCellFile(u32 mapping);
u32 AppMenuCommon_GetHpBarAnimFile(u32 mapping);
u32 AppMenuCommon_GetPokerusIconPaletteFile(void);
u32 AppMenuCommon_GetPokerusIconCharFile(u32 mapping);
u32 AppMenuCommon_GetPokerusIconCellFile(u32 mapping);
u32 AppMenuCommon_GetPokerusIconAnimFile(u32 mapping);

#endif // POKEBW2_SYSTEM_APP_MENU_COMMON_H
