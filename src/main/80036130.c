/* Disc audio report callback. During playback, compare absolute reports
 * against the requested track after the first ten notifications. */
#include "psx_types.h"

extern s32 D_8007610C;
extern s8 D_800A63E4;
extern s32 D_800A63E8;

void func_80036130(u8 event, const u8 *report)
{
    if ((u8)D_800A63E4 != 12 || event != 1)
        return;
    if (D_8007610C < 10) {
        D_8007610C++;
    } else if ((report[4] & 0x80) == 0) {
        u8 packed_track = report[1];
        if ((packed_track >> 4) * 10 + (packed_track & 15) != D_800A63E8)
            D_800A63E4 = 13;
    }
}
