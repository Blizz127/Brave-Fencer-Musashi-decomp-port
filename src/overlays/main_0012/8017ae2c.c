/* Owner re-derivation from retail assembly (clean input set). */
/* Five-state timed controller for the scalar at handle + 0x12, state block D_80185BD8.
 * State 1 eases the scalar toward a target for a number of steps, state 2 and 4 count the step
 * counter down while running the per-step object updates (state 2 also scales an offset), state 3
 * steers the scalar toward the heading returned by func_8012B744 and falls back to a plain
 * update when its step counter is zero. Finished states reset the state word and counter. */
#include "psx_types.h"

/* Handle object; only the scalar at 0x12 is used here. */
typedef struct {
    u8 pad00[0x12];
    s16 f12;
} Handle;

/* Controlled object. */
typedef struct {
    u8 pad00[4];
    s16 f4[3];
    s16 fA;
    u8 padC[0x20 - 0xC];
    u32 handle;
} Obj;

/* State block: state word, step counter and the parameters of the running state. */
typedef struct {
    s16 state;
    s16 timer;
    s16 f4;
    s16 f6;
    s16 f8;
    s16 fA;
    s16 fC[2];
} St;

extern St D_80185BD8;
extern s16 D_80185BDA;
extern s16 D_80185BE4[];

extern s32 func_8012B608(s32 a, s32 b, s32 c);
extern void func_8012B178(Obj *o, s32 v);
extern void func_8012AD80(Obj *o);
extern s32 func_8012B030(Obj *o);
extern s32 func_8012B744(s16 *a, s16 *b);
extern s16 func_80012A60(s16 a, s16 b);
extern s32 func_8012BE98(Obj *o, s16 *p);

void func_8017AE2C(Obj *o)
{
    St *st = &D_80185BD8;

    switch (st->state) {
    case 0:
        break;
    case 1:
        st->timer--;
        if (st->timer <= 0) {
            ((Handle *)o->handle)->f12 = st->f4;
            st->state = 0;
            st->timer = 0;
        } else {
            ((Handle *)o->handle)->f12 = ((Handle *)o->handle)->f12 +
                func_8012B608(((Handle *)o->handle)->f12, st->f4, 6);
        }
        break;
    case 2:
        func_8012B178(o, st->f8 << 8);
        func_8012AD80(o);
        o->fA -= 8;
        func_8012B030(o);
        st->timer--;
        if (st->timer <= 0) {
            st->state = 0;
            st->timer = 0;
        }
        break;
    case 3:
        if (st->timer != 0) {
            if (st->fA != 0) {
                if (func_80012A60(((Handle *)o->handle)->f12, (s16)func_8012B744(o->f4, st->fC)) < 0x100) {
                    st->timer = 0;
                }
                ((Handle *)o->handle)->f12 = func_8012B744(o->f4, D_80185BE4);
                func_8012B178(o, -0x60000);
                func_8012AD80(o);
                o->fA -= 8;
                func_8012B030(o);
                if (func_8012BE98(o, D_80185BD8.fC) < 0x101) {
                    D_80185BD8.state = 0;
                    D_80185BDA = 0;
                }
            }
            ((Handle *)o->handle)->f12 = ((Handle *)o->handle)->f12 +
                func_8012B608(((Handle *)o->handle)->f12, func_8012B744(o->f4, st->fC), st->f6);
            st->timer--;
            if (st->timer <= 0) {
                st->timer = 0;
            }
        } else {
            ((Handle *)o->handle)->f12 = func_8012B744(o->f4, D_80185BE4);
            func_8012B178(o, -0x60000);
            func_8012AD80(o);
            o->fA -= 8;
            func_8012B030(o);
            if (func_8012BE98(o, D_80185BD8.fC) < 0x101) {
                D_80185BD8.state = 0;
                D_80185BDA = 0;
            }
        }
        break;
    case 4:
        func_8012AD80(o);
        st->timer--;
        if (st->timer <= 0) {
            st->state = 0;
            st->timer = 0;
        }
        break;
    }
}
