#ifndef POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_UTIL_H
#define POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_UTIL_H

#include "types.h"
#include "struct_decls.h"

// t_download_util.c: the BGs, windows, messages, menus and cell actors of the downloaded tournaments' screens. The name
// is ours

void TDownloadUtil_Nop(TDownloadWork *wk);
void TDownloadUtil_Init(TDownloadWork *wk);
void TDownloadUtil_Exit(TDownloadWork *wk);
void TDownloadUtil_LoadModeBG(TDownloadWork *wk, int mode);
void TDownloadUtil_ClearWindow(TDownloadWork *wk, int window);
void TDownloadUtil_InitModeWindows(TDownloadWork *wk, int mode);
void TDownloadUtil_ExitModeWindows(TDownloadWork *wk, int mode);
void TDownloadUtil_SetActorVisible(TDownloadWork *wk, int kind, int index, BOOL visible);
BOOL TDownloadUtil_IsActorAnimating(TDownloadWork *wk, int kind, int index);
void TDownloadUtil_InitTaskMenu(TDownloadWork *wk, int type);
void TDownloadUtil_FreeTaskMenu(TDownloadWork *wk);
int TDownloadUtil_UpdateTaskMenu(TDownloadWork *wk);
BOOL TDownloadUtil_UpdateMessage(TDownloadWork *wk);
void TDownloadUtil_ClearMessage(TDownloadWork *wk, BOOL clear);
void TDownloadUtil_PrintMessage(TDownloadWork *wk, int window, u32 msgId, BOOL stream);
void TDownloadUtil_PressButton(TDownloadWork *wk, int button, int a2);
void TDownloadUtil_ScrollBG(TDownloadWork *wk);
void TDownloadUtil_PrintSlotInfo(TDownloadWork *wk, int type, int index);
void TDownloadUtil_InitList(TDownloadWork *wk, int mode);
u32 TDownloadUtil_UpdateList(TDownloadWork *wk);
int TDownloadUtil_ListResult(TDownloadWork *wk, u32 result);
void TDownloadUtil_FreeList(TDownloadWork *wk);
void TDownloadUtil_MoveCursor(TDownloadWork *wk, int type, int pos);
void TDownloadUtil_SetCursorKeys(TDownloadWork *wk, BOOL active);
void TDownloadUtil_AnimatePalette(TDownloadWork *wk);
void TDownloadUtil_SetPaletteAnim(TDownloadWork *wk, BOOL start);
void TDownloadUtil_SetTab(TDownloadWork *wk, int tab);
void TDownloadUtil_ShowPageButtons(TDownloadWork *wk, int tab);
void TDownloadUtil_ClampScrollBar(TDownloadWork *wk);
void TDownloadUtil_PrintDetails(TDownloadWork *wk, int type, int index, int tab);
void TDownloadUtil_CountDetails(TDownloadWork *wk, int type, int index);
int TDownloadUtil_GetTouchedPage(TDownloadWork *wk, int tab);
BOOL TDownloadUtil_UpdatePageBar(TDownloadWork *wk, int tab, int page, BOOL touch);
void TDownloadUtil_PrintTitle(TDownloadWork *wk, int mode, int tab);
int TDownloadUtil_GetTouch(TDownloadWork *wk, BOOL held, int mode);
void TDownloadUtil_DimScreen(TDownloadWork *wk, BOOL show);
void TDownloadUtil_DimScreenUpload(TDownloadWork *wk, BOOL show);
void TDownloadUtil_PauseActors(TDownloadWork *wk, BOOL show);
void TDownloadUtil_PrintSlotName(TDownloadWork *wk, int type, int slot);
void TDownloadUtil_UpdateCursorMode(TDownloadWork *wk, BOOL keys, BOOL touch, BOOL hide);
void TDownloadUtil_UpdateArrows(TDownloadWork *wk, int type, int min, int max, int pos);
BOOL TDownloadUtil_IsButtonDone(TDownloadWork *wk, int button);
void TDownloadUtil_ShowSlot(TDownloadWork *wk, int type, int slot);

#endif // POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_UTIL_H
