/* Owner re-derivation from retail assembly (clean input set). */
/* Clears the camera records: D_80126DB8, D_80126948, the 3-record blocks D_80114EE8 and D_80126E60 (0xA4 bytes each), * zeroes three status bytes and calls func_800144D4. */
#include "psx_types.h"

typedef struct {
    u8 b[0xA4];
} Cam80129C40;

extern u8 D_80126DB8[];
extern u8 D_80126948[];
extern u8 D_80114EE8[]; /* first of 3 consecutive records */
extern u8 D_80126E60[]; /* first of 3 consecutive records */
extern u8 D_801150D6;
extern u8 D_801152C0;
extern u8 D_80127504;
extern void func_80016714(void *p, s32 n);
extern void func_800144D4(void);

void func_80129C40(void)
{
    Cam80129C40 *p;

    func_80016714(((Cam80129C40 *)&D_80126DB8), 0xA4);
    func_80016714(((Cam80129C40 *)&D_80126948), 0xA4);
    p = ((Cam80129C40 *)&D_80114EE8);
    func_80016714(p, 0xA4);
    func_80016714(p + 1, 0xA4);
    func_80016714(p + 2, 0xA4);
    p = ((Cam80129C40 *)&D_80126E60);
    func_80016714(p, 0xA4);
    func_80016714(p + 1, 0xA4);
    func_80016714(p + 2, 0xA4);
    D_801150D6 = 0;
    D_801152C0 = 0;
    D_80127504 = 0;
    func_800144D4();
}
