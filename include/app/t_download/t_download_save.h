#ifndef POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_SAVE_H
#define POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_SAVE_H

#include "types.h"
#include "struct_decls.h"
#include "app/t_download/t_download_local.h"
#include "gfl/str.h"

// t_download_save.c: the tournaments kept in the save's extra blocks and those received, and saving one, named after
// the ROM's "t_download_save.c"

int TDownloadSave_LoadSaved(TDownloadWork *wk);
void TDownloadSave_FreeSaved(TDownloadWork *wk);
int TDownloadSave_LoadDownloaded(TDownloadWork *wk);
// Loads the string of a tournament: its name (0), its author (1) or its text (2)
void TDownloadSave_LoadString(TDownloadWork *wk, const TDownloadTournament *data, int kind, StrBuf *strbuf);
// How many species (0), items (1) or moves (2) the tournament has that can be listed
int TDownloadSave_CountEntries(TDownloadWork *wk, const TDownloadTournament *data, int kind);
// The number of one of those, counting from 1
u32 TDownloadSave_GetEntry(TDownloadWork *wk, const TDownloadTournament *data, int kind, int number);
void TDownloadSave_StartSave(TDownloadWork *wk, int src, int dest);
BOOL TDownloadSave_UpdateSave(TDownloadWork *wk);
void TDownloadSave_CreateWaitIcon(TDownloadWork *wk);
void TDownloadSave_UpdateWaitIcon(TDownloadWork *wk);
void TDownloadSave_FreeWaitIcon(TDownloadWork *wk);
int TDownloadSave_FindFreeSlot(TDownloadWork *wk);
BOOL TDownloadSave_IsSaved(TDownloadWork *wk, int index);
int TDownloadSave_FindSlot(TDownloadWork *wk, int start);
int TDownloadSave_NextSlot(TDownloadWork *wk, BOOL down, int pos);

#endif // POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_SAVE_H
