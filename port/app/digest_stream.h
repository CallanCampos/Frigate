// The digest stream (--digest FILE, ADR 0031): at every VBlank, what the display hardware holds on the DS-native path
// that every screen of the game is drawn from (each VRAM bank and their mapping, the palettes, OAM, both 2D engines'
// registers, POWCNT1 and WRAMCNT, the frame's display capture and the 3D it leaves in VRAM, the 3D render list and
// rendering registers, and both screens), handed to the stream's writer that pbw-oracle's --digest shares
// (port/tools/oracle/digest.h). The writer keeps the state at the last VBlank of each value of GFL's frame count as
// that count's record, for the melonDS oracle's comparison (port/tools/oracle/compare.py digest, README.md "Digests").
// The program opens it from its options (port/app/main.c). Plain C types only.
#ifndef PORT_APP_DIGEST_STREAM_H
#define PORT_APP_DIGEST_STREAM_H

#include <stdint.h>

// Opens the file, writes the stream's header and sets the screens' frame-start hook (port/gfx/screens.h); returns 0,
// after printing why, when the file can't be written
int port_digest_stream_open(const char *path);
// Whether a stream is open
int port_digest_stream_active(void);
// The VBlank that ends video frame `frame`, at the program's frame hook, after the geometry engine's swap: hands the
// state to the writer. Returns 0, after printing a "port: --digest:" line saying why, when the writer failed: a write
// the file couldn't take, GFL's frame count falling or jumping, a frame that couldn't be encoded
int port_digest_stream_frame(uint32_t frame);
// Ends the stream, which never holds its last count (the writer's rule), and closes the file; returns 0 when a write
// failed, now or before
int port_digest_stream_close(void);

#endif // PORT_APP_DIGEST_STREAM_H
