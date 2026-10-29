#include "psx_types.h"

typedef struct {
    u32 f0;
    u8 pad4[4];
    u32 f8;
    u8 padC[4];
    u16 f10;
    u8 pad12[0x11];
    u8 f23;
    u8 pad24[0x20];
    u16 f44;
    u8 pad46[4];
    u16 f4A;
} Obj;

extern u32 D_80127530[];
extern s32 func_80138DE0(Obj *o, s32 a, s32 b);
extern void func_80138948(Obj *o);
extern s32 func_80139220(Obj *o);
extern void func_80139A8C(Obj *o);
extern void func_80139B18(Obj *o);

void func_801387B8(Obj *o) {
    s32 s2;
    u16 *s1;
    u32 base;
    u8 *p;
    s32 a1;
    s32 a2;
    u16 v;

top:
    {
        s2 = 0;
        if (o->f8 & 0x400) {
            base = D_80127530[o->f4A];
            s1 = &o->f44;
        } else {
            s1 = &o->f10;
            base = o->f0;
        }
        p = (u8 *)(base + *s1);
        a1 = p[0];
        a2 = p[1];
        if ((u32)a1 >= 0x20) {
            s2 = func_80138DE0(o, a1, a2);
            if (!(o->f8 & 0x80220)) {
                s2 = 0;
            }
        } else {
            if (a1 == 0) {
                goto blk22;
            }
            switch (a1) {
            case 10:
                func_80139220(o);
                v = *s1 + 1;
                *s1 = v;
                break;
            case 1:
                s2 = 1;
                o->f23 = a2;
                *s1 += 2;
                break;
            case 7:
                if (!(o->f8 & 0x20000)) {
                    o->f8 = o->f8 & ~0x20;
                }
                goto blk22;
            case 23:
                o->f8 = o->f8 | 2;
            default:
                s2 = 1;
                v = *s1 + 1;
                *s1 = v;
                break;
            }
            goto next;
        blk22:
            func_80138948(o);
        }
    }
next:
    if (s2 != 0) {
        goto top;
    }
    func_80139A8C(o);
    func_80139B18(o);
}
