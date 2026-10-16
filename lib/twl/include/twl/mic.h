#ifndef POKEBW2_TWL_MIC_H
#define POKEBW2_TWL_MIC_H

#include "types.h"
#include "nitro/mic.h"

// The DSi's limited sampling of the microphone in the LTD autoload (lib/twl/src/mic/mic_twl.c), which NitroSDK's
// MIC_StartLimitedSampling, MIC_StartLimitedSamplingAsync and MIC_StopLimitedSampling call when the console runs in its
// DSi mode. They take the sampling rates 0x2bf, 0x3ff, 0x57f, 0x800, 0x83f, 0xb00, 0xbff and
// MIC_SAMPLING_RATE_8180 only. They keep their default names until the LTD autoload is analyzed
MICResult func_02703c60(const MICAutoParam *param);                                  // MICEX_StartLimitedSampling
MICResult func_02703cdc(const MICAutoParam *param, MICCallback callback, void *arg); // MICEX_StartLimitedSamplingAsync
MICResult func_02703e90(void);                                                       // MICEX_StopLimitedSampling

#endif // POKEBW2_TWL_MIC_H
