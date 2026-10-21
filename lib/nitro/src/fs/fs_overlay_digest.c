// NitroSDK's digests of the ARM9 overlays, which fs_overlay.c's FS_StartOverlay checks an overlay against when the game
// was not booted from the ROM. NitroSDK's build makes no object of them: its linker script leaves room for the table at
// the end of ARM9 main's .data, and its tools fill it in after the link, with the HMAC-SHA1 under the key of
// fs_overlay.c of each overlay as the ROM stores it. This build computes them from the base ROM's overlays instead
// (tools/build/overlay_digests.py), so that they are what the original has. The file name is ours.
#include "nitro/fs.h"

// Ahead of the overlays' digests, two more: a zero one, whose use isn't known, and that of the overlay table, an
// HMAC-SHA1 that skips words 6 and 14 of each block (sha1_set_masked), the file IDs of its entries. Nothing reads them
FSOverlayDigest OverlayTableDigests[2] = {
    { { 0 } },
#ifdef BLACK2
#include "rom/b2_us/overlay_table_digest.inc"
#else
#include "rom/w2_us/overlay_table_digest.inc"
#endif
};

// One digest for each overlay, in the order of their IDs. SDK_OVERLAY_DIGEST_END, which fs_overlay.c takes for the end
// of the table, is a label of NitroSDK's linker script rather than an object: here it marks the padding that follows,
// to the 32-byte boundary where ARM9 main's .bss starts
FSOverlayDigest SDK_OVERLAY_DIGEST[] = {
#ifdef BLACK2
#include "rom/b2_us/overlay_digests.inc"
#else
#include "rom/w2_us/overlay_digests.inc"
#endif
};
