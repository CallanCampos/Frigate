#include "types.h"
#include "twl/dsp.h"

// TwlSDK's graphics component as the ARM9 carries it (descriptive file name, after its use; the SDK's own isn't known):
// the COFF image of the DSP's graphics process, which converts between YUV and RGB and scales pictures for the camera
// code (dsp_graphics.c opens and starts it). Its strings show TwlSDK built it from
// build/dspcomponents/build/processes/graphics with CEVA's SmartNcode Teak tools. It is TeakLite DSP code, which no
// compiler of ours builds, so its bytes stay as they are: the build takes them from the base ROM (see ROM_BLOBS in
// configure.py). In the DSi-only LTD autoload.

const u8 DSP_GraphicsImage[] = {
#include "rom/dsp/dsp_graphics_image.inc"
};
