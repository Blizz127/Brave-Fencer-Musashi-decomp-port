/* Owner re-derivation from retail assembly (clean input set). */
/* Sweeps two 16-entry tables (stride 0x60) and four pending words, releasing finished entries. */
#include "psx_types.h"
typedef struct {
    u8 pad0[0x10];
    u8 f10[4];
    u32 f14;
    u8 pad18[0x5A - 0x18];
    u8 f5A;
    u8 pad5B[2];
    u8 f5D;
    u8 f5E;
    u8 pad5F;
} Ent;
extern u8 D_800A4F1D;
extern Ent D_800C6DD0[16];
extern u32 D_800A2B98;
extern u32 D_800A2BA0;
extern u32 D_800C7D20;
extern u32 D_800C7D2C;
extern void func_80038A58(void);
extern void func_8003D3B4(s32, s32);
extern void func_8003B250(s32, u8 *);
extern void func_8003C23C(s32, s32);
extern void func_8003BE74(s32, s32);

void func_80037FC4(void)
{
    s32 i;
    Ent *e;
    u8 *p;

    if (D_800A4F1D == 0) {
        func_80038A58();
    }
    for (i = 0; i < 16; i++) {
        p = &D_800C6DD0[i].f5E;
        if (p[-4] != 0 && p[-1] == 0 && p[0] != 0) {
            func_8003D3B4(i, 8);
            p[0] = 0;
            *(u32 *)(p - 0x4A) &= 0xFFF9FFFF;
        }
    }
    if (*(u16 *)&D_800A2B98 != 0) {
        func_8003C23C(0, *(u16 *)&D_800A2B98);
        D_800A2B98 &= 0xFF0000;
    }
    if (*(u16 *)&D_800A2BA0 != 0) {
        func_8003BE74(0, *(u16 *)&D_800A2BA0);
        D_800A2BA0 &= 0xFF0000;
    }
    e = D_800C6DD0;
    for (i = 0; i < 16; i++, e++) {
        p = &e->f5E;
        if (p[-4] != 0 && p[-1] != 0) {
            func_8003B250(i, e->f10);
            *(u32 *)(p - 0x4A) = 0;
            p[-1] = 0;
            p[0] = 0;
        }
    }
    if (*(u16 *)&D_800C7D20 != 0) {
        func_8003C23C(1, *(u16 *)&D_800C7D20);
        D_800C7D20 &= 0xFF0000;
    }
    if (*(u16 *)&D_800C7D2C != 0) {
        func_8003BE74(1, *(u16 *)&D_800C7D2C);
        D_800C7D2C &= 0xFF0000;
    }
}
