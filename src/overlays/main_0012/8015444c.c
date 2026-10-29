/* Owner re-derivation from retail assembly (clean input set). */
/* One step of the actor script interpreter with a skip flag (the larger sibling of func_80154C24).
 * The step counter (low byte of the u16 at 0xB8) counts down; at zero the command word script[pc]
 * (script = guest word array at 0xB4, pc = byte at 0xBC, depth = byte at 0xBD, step byte at 0xDC)
 * is run through a 27-entry jump table. Commands that continue re-enter the step with the same
 * arguments; the effect commands (14-26) only act while *skip is zero. The flag word *fl is OR-ed
 * into the u16 at 0xB8 at the end. Unknown commands store cmd + 0x10 into the handle object's word
 * at 0x20 and advance. Every command body carries its own copy of the continue sequence. */
#include "psx_types.h"

/* Handle object; only the word at 0x20 is used here. */
typedef struct {
    u8 pad00[0x20];
    u32 f20;
} Handle;

/* Actor fields used here. */
typedef struct {
    u8 pad00[0x20];
    u32 handle;
    u8 pad24[0x44 - 0x24];
    u32 flags;
    u8 pad48[0xB4 - 0x48];
    u32 script;
    u16 timer;
    u16 saved;
    u8 pc;
    u8 depth;
    u8 pad_be[0xDC - 0xBE];
    u8 dc;
    u8 pad_dd[0x1A8 - 0xDD];
    u8 b1A8;
} Obj;

extern u8 D_80126738[];
extern u8 D_80126838[];
extern u8 D_80126748[];
extern u8 D_80126848[];
extern s32 func_801549F8(Obj *o);
extern void func_80154B4C(Obj *o, u32 v);
extern void func_80154A74(Obj *o, s32 i);
extern void func_801553C0(Obj *o);
extern void func_80155440(Obj *o);
extern void func_80155394(Obj *o);
extern void func_801553A8(Obj *o);
extern void func_80059A1C(u32 a, u8 *b);

void func_8015444C(Obj *o, s32 *cnt, u32 *fl, s32 *skip)
{
    Handle *h = (Handle *)o->handle;
    u8 b;
    u32 cmd;
    u32 nxt;
    u32 npc;

    if (h == 0) {
        return;
    }
    b = o->timer;
    if (b == 0) {
        return;
    }
    if (*cnt == 0) {
        o->timer = b;
        *cnt = *cnt + 1;
    }
    o->timer--;
    if ((u8)o->timer != 0) {
        goto done;
    }
    cmd = ((u32 *)o->script)[o->pc];
    switch (cmd) {
    case 0:
        o->timer = 0;
        *fl |= 0x8000;
        o->saved = 0;
        o->pc = 0;
        o->depth = 0;
        break;
    case 1:
        o->timer = 1;
        *fl |= 0x8000;
        o->pc = 0;
        o->depth = 0;
        o->dc = 0;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 2:
        o->timer = 0;
        o->pc = o->pc + 1;
        *fl |= 0x8000;
        break;
    case 3:
        nxt = ((u32 *)o->script)[o->pc + 1];
        o->timer = 1;
        o->pc = o->pc + 2;
        o->saved = nxt;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 4:
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 5:
        nxt = ((u32 *)o->script)[o->pc + 1];
        npc = ((u32 *)o->script)[o->pc + 2];
        o->timer = 1;
        o->dc = 0;
        o->pc = npc;
        o->script = nxt;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 6:
        npc = ((u32 *)o->script)[o->pc + 1];
        o->timer = 1;
        o->pc = npc;
        o->dc = func_801549F8(o);
        func_8015444C(o, cnt, fl, skip);
        break;
    case 7:
        o->timer = 1;
        *fl |= 0x4000;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 8:
        o->timer = 1;
        *fl |= 0x2000;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 9:
        o->timer = 1;
        *fl |= 0x1000;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 10:
        o->timer = 1;
        *fl |= 0x800;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 11:
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 12:
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 13:
        o->timer = 1;
        o->pc = o->pc + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 14:
        if (*skip == 0) {
            func_80154B4C(o, ((u32 *)o->script)[o->pc + 1]);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 15:
        if (*skip == 0) {
            func_80154A74(o, ((u32 *)o->script)[o->pc + 1]);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 16:
        if (*skip == 0) {
            func_801553C0(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 17:
        if (*skip == 0) {
            func_80155440(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 18:
        if (*skip == 0) {
            func_80155394(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 19:
        if (*skip == 0) {
            func_801553A8(o);
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 20:
        if (*skip == 0) {
            o->b1A8 = o->b1A8 ^ 1;
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 21:
        if (*skip == 0) {
            func_80059A1C(((u32 *)o->script)[o->pc + 1], D_80126738);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 22:
        if (*skip == 0) {
            func_80059A1C(((u32 *)o->script)[o->pc + 1], D_80126838);
        }
        o->timer = 1;
        o->pc = o->pc + 2;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 23:
        if (*skip == 0) {
            h->f20 = (u32)D_80126748;
        }
        o->timer = o->saved;
        o->pc++;
        o->depth++;
        o->dc++;
        break;
    case 24:
        if (*skip == 0) {
            h->f20 = (u32)D_80126848;
        }
        o->timer = o->saved;
        o->pc++;
        o->depth++;
        o->dc++;
        break;
    case 25:
        if (*skip == 0) {
            o->flags &= 0xFFFDFFFF;
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    case 26:
        if (*skip == 0) {
            o->flags |= 0x20000;
        }
        o->timer = 1;
        o->pc = o->pc + 1;
        o->depth = o->depth + 1;
        func_8015444C(o, cnt, fl, skip);
        break;
    default:
        h->f20 = cmd + 0x10;
        o->timer = o->saved;
        o->pc++;
        o->depth++;
        o->dc++;
        break;
    }
done:
    o->timer |= *fl;
}
