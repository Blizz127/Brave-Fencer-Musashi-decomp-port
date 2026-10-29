/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Records a packed id and returns two halfwords from the selected record. */
#include "psx_types.h"

extern s16 D_800B9B36;
extern u32 D_800C73D8[]; /* guest u8 * */

s16 func_80041354(s16 arg0, u16 *arg1, u16 *arg2) {
    u8 *rec;
    s16 *id;

    rec = (u8 *)D_800C73D8[arg0 & 0xFF];
    id = &D_800B9B36;
    *id = arg0;
    rec += ((arg0 & 0xFF00) >> 8) * 0xB0;
    *arg1 = *(u16 *)(rec + 0x58);
    *arg2 = *(u16 *)(rec + 0x5A);
    return *id;
}
