/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_8017D1B4(void *a0);
extern s32 D_80126954;
extern s32 D_8012695C;
extern u16 D_80126968;
extern u16 D_8012696A;
extern u16 D_8012696C;
extern u16 D_80126976;
extern u16 D_80126978;
extern u16 D_8012697A;
extern void func_8012A018(void *fn, s32 a1);
extern void func_8012A094(void *a0);
extern u8 D_80126948[];

void func_8017D024(void) {
    D_80126954 = 0x190;
    D_8012695C = 0x4B0;
    D_80126968 = 0x2AA;
    D_8012696A = 0x800;
    D_8012696C = 0;
    D_80126976 = 0;
    D_80126978 = 0;
    D_8012697A = 0;
    func_8012A018((void *)func_8017D1B4, 0);
    func_8012A094(D_80126948);
    func_8017D1B4(D_80126948);
}
