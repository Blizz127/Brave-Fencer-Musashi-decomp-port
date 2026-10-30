/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_800118AC();                            /* extern */
void func_80015310();                            /* extern */
void func_800183E0(s32 *);                   /* extern */
extern u8 D_800D93FC[];

extern u8 D_800AF630[];
void func_800D0664(void) {
    s32 scratch[2];
    register u8 *base = D_800AF630;
    func_80015310();
    func_800183E0(D_800D93FC);
    func_800118AC();
}
