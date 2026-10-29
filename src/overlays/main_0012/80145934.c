/* Owner re-derivation from retail assembly (clean input set). */
/* Picks a data table by the frame counter func_80029504(): descending thresholds 0x6A4, 0x5DC,
 * 0x578, 0x514, 0x384, 0x258, 0xC8 and 0xA, hands it to func_800D185C and then runs func_80145B24. */
#include "psx_types.h"

extern u8 D_801808B8[];
extern u8 D_80180828[];
extern u8 D_80180804[];
extern u8 D_801807E0[];
extern u8 D_80180894[];
extern u8 D_80180870[];
extern u8 D_8018084C[];
extern u8 D_801807BC[];
extern s32 func_80029504(void);
extern void func_800D185C(u8 *table);
extern void func_80145B24(void);

void func_80145934(void)
{
    s32 t;
    u8 *p;

    t = func_80029504();
    if (t >= 0x6A4) {
        p = D_80180828 + 0x90; /* same table as D_801808B8, reached through its neighbour */
    } else if (t >= 0x5DC) {
        p = D_80180828;
    } else if (t >= 0x578) {
        p = D_80180804;
    } else if (t >= 0x514) {
        p = D_801807E0;
    } else if (t >= 0x384) {
        p = D_801808B8;
    } else if (t >= 0x258) {
        p = D_80180894;
    } else if (t >= 0xC8) {
        p = D_80180870;
    } else if (t >= 0xA) {
        p = D_8018084C;
    } else {
        p = D_801807BC;
    }
    func_800D185C(p);
    func_80145B24();
}
