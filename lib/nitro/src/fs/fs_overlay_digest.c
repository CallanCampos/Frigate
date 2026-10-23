// NitroSDK's digests of the ARM9 overlays, which fs_overlay.c's FS_StartOverlay checks an overlay against when the game
// was not booted from the ROM. NitroSDK's build makes no object of them: its linker script leaves room for the table at
// the end of ARM9 main's .data, and its tools fill it in after the link, with the HMAC-SHA1 under the key of
// fs_overlay.c of each overlay as the ROM stores it. Here the room is this file's, and dsd's `rom build` fills it in
// the same way from the overlays just built: `dsd rom config` finds SDK_OVERLAY_DIGEST in the linked program. The file
// name is ours.
#include "nitro/fs.h"

// The room is zeros in ARM9 main's .data, where the linker script would leave it, not in .bss
#pragma explicit_zero_data on

// The game's overlays. A mod that adds overlays adds their digests' room here
#define FS_OVERLAY_DIGEST_COUNT 344

// Ahead of the overlays' digests, two more: a zero one, whose use isn't known, and that of the overlay table, an
// HMAC-SHA1 that skips words 6 and 14 of each block (sha1_set_masked), the file IDs of its entries. Nothing reads them
FSOverlayDigest OverlayTableDigests[2] = { { { 0 } } };

// One digest for each overlay, in the order of their IDs. SDK_OVERLAY_DIGEST_END, which fs_overlay.c takes for the end
// of the table, is a label of NitroSDK's linker script rather than an object: here it marks the padding that follows,
// to the 32-byte boundary where ARM9 main's .bss starts
FSOverlayDigest SDK_OVERLAY_DIGEST[FS_OVERLAY_DIGEST_COUNT] = { { { 0 } } };
