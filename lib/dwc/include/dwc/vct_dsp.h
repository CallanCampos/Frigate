#ifndef POKEBW2_DWC_VCT_DSP_H
#define POKEBW2_DWC_VCT_DSP_H

#include "types.h"
#include "struct_decls.h"

// Copies the four words of how loud the microphone is: whether somebody is speaking, the level, and two limits
void VCT_GetCaptureLevels(s32 *out);

#endif // POKEBW2_DWC_VCT_DSP_H
