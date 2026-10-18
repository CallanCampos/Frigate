#ifndef POKEBW2_APP_COMM_TVT_CTVT_CAMERA_H
#define POKEBW2_APP_COMM_TVT_CTVT_CAMERA_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "twl/camera.h"

// The Xtransceiver's video: the DSi camera's frames, and each member's picture in a window of the top screen

CtvtCamera *CtvtCamera_Create(CommTvtWork *sys, HeapID heapId);
void CtvtCamera_Delete(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_Update(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_Draw(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_StopCamera(CommTvtWork *sys, CtvtCamera *work);
BOOL CtvtCamera_IsSoundDone(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_SetDirty(CommTvtWork *sys, CtvtCamera *work, u8 member);
void CtvtCamera_ClearDirty(CommTvtWork *sys, CtvtCamera *work, u8 member);
void CtvtCamera_Redraw(CommTvtWork *sys, CtvtCamera *work, BOOL fadeIn, BOOL updateDisplay);
BOOL CtvtCamera_IsRedrawing(CommTvtWork *sys, CtvtCamera *work);
u16 CtvtCamera_GetWidth(CommTvtWork *sys, CtvtCamera *work);
u16 CtvtCamera_GetHeight(CommTvtWork *sys, CtvtCamera *work);
void *CtvtCamera_GetOwnPicture(CommTvtWork *sys, CtvtCamera *work);
void *CtvtCamera_GetPicture(CommTvtWork *sys, CtvtCamera *work, u8 member);
u32 CtvtCamera_GetPictureSize(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_SetWindow(CommTvtWork *sys, CtvtCamera *work, u8 member);
void CtvtCamera_MarkWindow(CommTvtWork *sys, CtvtCamera *work, u8 member);
BOOL CtvtCamera_IsWindowMoving(CommTvtWork *sys, CtvtCamera *work, u8 member);
void CtvtCamera_SetEffect(CommTvtWork *sys, CtvtCamera *work, CAMERAEffect effect);
void CtvtCamera_StartRecording(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_EndRecording(CommTvtWork *sys, CtvtCamera *work);
CameraSystem *CtvtCamera_GetCameraSystem(CommTvtWork *sys, CtvtCamera *work);
void CtvtCamera_Restart(CommTvtWork *sys, CtvtCamera *work);

#endif // POKEBW2_APP_COMM_TVT_CTVT_CAMERA_H
