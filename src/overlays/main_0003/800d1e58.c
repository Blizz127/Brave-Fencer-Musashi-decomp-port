/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

s32 func_8001BFA0();                                /* extern */
void func_8001BFB0();                            /* extern */
void func_800D1E9C(void *);                      /* extern */
extern u8 D_800D942C[];
extern s32 D_800EC9E0;

void func_800D1E58(void) {
    D_800EC9E0 = 0;
    func_800D1E9C((func_8001BFA0() << 5) + D_800D942C);
    func_8001BFB0();
}
