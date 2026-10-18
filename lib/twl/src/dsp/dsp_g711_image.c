#include "types.h"
#include "twl/dsp.h"

// TwlSDK's G.711 component as the ARM9 carries it (descriptive file name, after its use; the SDK's own isn't known):
// the COFF image of the DSP's audio process, which DSP_OpenStaticComponentG711Core opens and DSP_LoadG711Core starts.
// Its strings show TwlSDK built it from build/dspcomponents/build/processes/audio (main.c, g711.c) with CEVA's
// SmartNcode Teak tools. It is TeakLite DSP code, which no compiler of ours builds, so its bytes stay as they are: the
// build takes them from the base ROM (see ROM_BLOBS in configure.py). In the DSi-only LTD autoload.

const u8 DSP_G711Image[] = {
#include "rom/dsp/dsp_g711_image.inc"
};
