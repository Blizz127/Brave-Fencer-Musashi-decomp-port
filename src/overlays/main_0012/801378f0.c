/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x38];
    u16 f38;
    u16 f3A;
    u16 f3C;
    u16 f3E;
    u8 pad40[0x0C];
} Rec801378F0; /* size 0x4C */

typedef struct {
    u16 a;
    u16 b;
    u16 c;
    u16 d;
} Row801378F0;

extern u8 D_80127520[];
extern Rec801378F0 D_801269F0[];
extern Row801378F0 D_8017E934[];
extern void func_80016714(void *p, s32 n);
extern void func_8013A860(void);

void func_801378F0(void) {
    s32 i;

    func_80016714(D_80127520, 0xBFC);
    for (i = 0; i < 3; i++) {
        func_80016714(&D_801269F0[i], 0x4C);
        D_801269F0[i].f38 = D_8017E934[i].a;
        D_801269F0[i].f3A = D_8017E934[i].b;
        D_801269F0[i].f3C = D_8017E934[i].c;
        D_801269F0[i].f3E = D_8017E934[i].d;
    }
    func_8013A860();
}
