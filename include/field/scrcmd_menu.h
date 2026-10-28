#ifndef POKEBW2_FIELD_SCRCMD_MENU_H
#define POKEBW2_FIELD_SCRCMD_MENU_H

// Overlay 36's scrcmd_menu.c: the script commands of the yes/no window and the list menus. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Menu_YesNoWin(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Menu_ListMenuInitCommon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Menu_ListMenuAnchorTopLeft(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Menu_ListMenuAnchorTopRight(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Menu_ListMenuAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Menu_ListMenuShow(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Menu_ListMenuShow2(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_MENU_H
