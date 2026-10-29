/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
typedef struct { u32 w[4]; } Q8017129C;
extern u32 D_8011F738;
extern u8 D_800A5E88[];
extern u8 D_800A5E98[];
extern u8 D_800A5EA8[];
extern void func_801719A4(u8 *o, s32 a);
extern void func_80171990(u8 *o);
extern void func_80171384(u8 *o);

void func_8017129C(u8 *o)
{
    Q8017129C *p;

    func_801719A4(o, 1);
    p = (Q8017129C *)D_8011F738;
    p[0] = (*(Q8017129C *)&D_800A5E88);
    p[1] = (*(Q8017129C *)&D_800A5E98);
    p[2] = (*(Q8017129C *)&D_800A5EA8);
    *(u16 *)(*(u8 **)(o + 0x20) + 0x2C) |= 0x20;
    *(Q8017129C **)(*(u8 **)(o + 0x20) + 0x80) = p;
    func_80171990(o);
    func_80171384(o);
}
