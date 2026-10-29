/* Owner re-derivation from retail assembly (clean input set). */
/* Like func_801439FC but also supports a fixed-orientation mode (+0x70 == 1) and sets the node's two sine-derived angles at +0x18/+0x1A from the target's node. */
#include "psx_types.h"
extern short D_801152A8;
extern short D_801152AA;
extern short D_801152AC;
int func_8004CFEC(int a, int b);
int func_8004787C(int a);
int func_80134510(u16 *p);
void func_80016714(char *n, int a);
void func_8012C218(char *o);
void func_80143EBC(char *o) {
    u16 pos[3];
    char *t = (char *)*(int *)(o + 0x64);
    char *n = (char *)*(int *)(o + 0xCC);
    if (*(short *)(t + 0x36) != *(short *)(o + 0xFC)) {
        if (o != 0) {
            func_80016714(n, 0x38);
            func_8012C218(o);
        }
    } else {
        pos[0] = *(u16 *)(t + 6);
        pos[1] = *(u16 *)(t + 0xA) - 0x10;
        pos[2] = *(u16 *)(t + 0xE);
        if (func_80134510(pos) == 0) {
            *(u32 *)(n + 4) |= 0x80000000;
        } else {
            *(u16 *)(n + 8) = pos[0];
            *(u16 *)(n + 0xA) = pos[1];
            *(u16 *)(n + 0xC) = pos[2];
            *(int *)(n + 4) &= 0x7FFFFFFF;
            if (*(short *)(o + 0x70) == 1) {
                *(u16 *)(n + 0x10) = 0x400;
                *(u16 *)(n + 0x12) = 0;
            } else {
                *(u16 *)(n + 0x10) = func_8004CFEC(D_801152AC, D_801152AA) + 0x400;
                *(u16 *)(n + 0x12) = func_8004CFEC(D_801152A8, D_801152AA);
            }
            *(u16 *)(n + 0x14) = -*(u16 *)(*(int *)(t + 0x20) + 0x12);
            *(u16 *)(n + 0x18) = func_8004787C((*(u16 *)(*(int *)(t + 0x20) + 0x14) & 0x3FF) << 1) + 0x4000;
            *(u16 *)(n + 0x1A) = func_8004787C((*(u16 *)(*(int *)(t + 0x20) + 0x10) & 0x3FF) << 1) + 0x4000;
        }
    }
}
