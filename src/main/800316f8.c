/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Decodes an object's current motion command (11 opcodes) into state fields and sets up a ramp toward its target. */
#include "psx_types.h"

typedef struct {
    u16 w[9];
} Cmd;

typedef struct {
    u8 pad0[0x12];
    u16 f12;
    u32 cmd; /* guest Cmd * */
    s32 f18;
    s32 f1C;
    s32 f20;
    s32 f24;
    s32 f28;
    s32 f2C;
    u16 f30;
    u16 f32;
    u8 pad34[3];
    u8 f37;
    u8 f38;
    u8 pad39;
    u16 f3A;
    u16 f3C;
} Obj;

void func_800316F8(Obj *obj)
{
    Cmd *c;
    u16 tgt = 0;
    u16 amt = 0;

    c = (Cmd *)obj->cmd;
    obj->f37 = 0;
    obj->f18 = 0;
    obj->f20 = obj->f24;
    if (c == 0) {
        return;
    }
    switch (c->w[0]) {
    case 1:
        amt = c->w[1];
        tgt = c->w[2];
        obj->f30 = 0;
        obj->f38 = 1;
        obj->f3A = c->w[3];
        obj->f3C = c->w[4];
        obj->f12 = c->w[7];
        break;
    case 9:
        {
            u16 t;
            amt = c->w[1];
            t = c->w[2];
            obj->f30 = 0;
            obj->f38 = 1;
            obj->f3A = c->w[3];
            obj->f3C = c->w[4] + obj->f20;
            tgt = t + obj->f20;
            obj->f12 = c->w[7];
        }
        break;
    case 6:
        amt = c->w[2];
        tgt = c->w[3];
        obj->f30 = 0;
        obj->f38 = 1;
        obj->f3A = c->w[4];
        obj->f3C = c->w[5];
        obj->f12 = c->w[8];
        goto set30;
    case 10:
        {
            u16 t;
            amt = c->w[2];
            t = c->w[3];
            obj->f30 = 0;
            obj->f38 = 1;
            obj->f3A = c->w[4];
            obj->f3C = c->w[5] + obj->f20;
            obj->f12 = c->w[8];
            tgt = t + obj->f20;
            obj->f30 = c->w[1];
        }
        break;
    case 0:
        obj->f38 = 0;
        amt = c->w[1];
        tgt = c->w[2];
        obj->f12 = c->w[5];
        obj->f30 = 0;
        break;
    case 7:
        obj->f38 = 0;
        amt = c->w[1];
        tgt = c->w[2] + obj->f20;
        obj->f12 = c->w[5];
        obj->f30 = 0;
        break;
    case 5:
        obj->f38 = 0;
        amt = c->w[2];
        tgt = c->w[3];
        obj->f12 = c->w[6];
    set30:
        obj->f30 = c->w[1];
        break;
    case 8:
        {
            u16 t;
            obj->f38 = 0;
            amt = c->w[2];
            t = c->w[3];
            obj->f12 = c->w[6];
            tgt = t + obj->f20;
            obj->f30 = c->w[1];
        }
        break;
    case 2:
        obj->f38 = 0;
        obj->f12 = c->w[1];
        obj->f30 = 0;
        return;
    case 3:
        obj->f37 |= 2;
        obj->f1C = (s16)c->w[2] << 8;
        obj->f32 = c->w[3];
        obj->f38 = 0;
        obj->f12 = c->w[1];
        obj->f30 = 0;
        return;
    case 4:
        obj->f37 |= 2;
        obj->f1C = (s16)c->w[1] << 8;
        obj->f32 = c->w[2];
        obj->f38 = 0;
        obj->f30 = 0;
        return;
    }
    if (amt != 0 && obj->f24 != tgt) {
        obj->f37 |= 1;
        obj->f28 = tgt;
        if (tgt < obj->f24) {
            obj->f2C = -amt;
        } else {
            obj->f2C = amt;
        }
    }
}
