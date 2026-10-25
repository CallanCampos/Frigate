// What the multiboot library's parent and child sides share: the sending of a frame and the beacon's checksum (libmb).
// It is the SDK's mb_common.c, as ntrtwl's reconstruction of NitroSDK has it and as Black 1's map of TwlSDK's libmb
// places these two functions (aletheia's layout), between mb_fileinfo.c (our mb_segment.c) and mb_gameinfo.c. The
// names are NitroSDK's, as that reconstruction has them. Its other functions, which nothing in the game calls, are not
// in the ROM and not written here
#include "mb_private.h"

// Sends a frame that is in memory to the children in a bitmap
int MBi_BlockHeaderEnd(u32 length, u16 aidBitmap, void *buf) {
    DC_FlushRange(buf, (length + 0x1f) & ~0x1f);
    DC_WaitWriteBufferEmpty();
    return MBi_SendMP(buf, length, aidBitmap);
}

// The one's-complement sum of a block of halfwords, as the beacon carries it
u16 MBi_calc_cksum(const u16 *data, int length) {
    u32 sum = 0;
    int count = length >> 1;

    while (count > 0) {
        sum += *data++;
        count--;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += sum >> 16;
    return sum ^ 0xffff;
}
