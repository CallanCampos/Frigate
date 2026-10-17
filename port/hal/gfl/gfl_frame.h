// GFL's frame count: how many passes of the game's main loop have ended since GFL started (src/system/gfl_use.c's
// sVBlankCount, which GFL_VBlankUpdate moves on after each pass's VBlank wait). Input files count their frames this
// way (port/tools/oracle/pbwinput.h). Plain C types only.
#ifndef PORT_HAL_GFL_GFL_FRAME_H
#define PORT_HAL_GFL_GFL_FRAME_H

#include <stdint.h>

uint32_t port_gfl_frame_count(void);

#endif // PORT_HAL_GFL_GFL_FRAME_H
