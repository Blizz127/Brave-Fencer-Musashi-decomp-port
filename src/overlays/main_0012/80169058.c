/* func_80169058 [80169058,801691B8) - owner re-derivation from retail assembly (clean input set).
 * Re-aims the object: transforms its position, builds a random rotation (range by o->f2C bit 1),
 * combines the matrices and rotates a (0,0,10) step into o->f12..f1A; bumps o->f2.
 * v.x/v.y are stored in each arm (jump2 cross-jumps them so the arg setup stays after them);
 * tail order f12, f16, f1A, f2++, f1C. Callers read v0: none (void). */
#include "psx_types.h"

typedef struct { s32 w[8]; } M8;
typedef struct { u16 x; u16 y; u16 z; u16 w; } V8;
typedef struct {
    u8 pad0[2];
    u16 f2;
    u8 pad4[2];
    u16 f6;
    u8 pad8[2];
    u16 fA;
    u8 padC[2];
    u16 fE;
    u8 pad10[2];
    u16 f12;
    u8 pad14[2];
    u16 f16;
    u8 pad18[2];
    u16 f1A;
    s32 f1C;
    u8 pad20[0xC];
    s32 f2C;
    u8 pad30[8];
    u8 f38[1];
} Obj;

extern void func_800D20C0(V8 *a, V8 *b, s32 n);
extern void func_800D23D0(V8 *a);
extern void func_80049A1C(V8 *a, void *b);
extern void func_80048EAC(M8 *a, M8 *b);
extern void func_80048FBC(M8 *m, V8 *a, V8 *b);
extern s32 func_8005C49C(void);

void func_80169058(Obj *o) {
    V8 v;
    V8 w;
    M8 m1;
    M8 m2;
    s32 r;
    s32 r2;
    u16 a;
    s32 b;

    v.x = o->f6;
    v.y = o->fA;
    v.z = o->fE;
    func_800D20C0(&v, &w, 1);
    func_800D23D0(&w);
    func_80049A1C(&w, o->f38);
    v.x = o->f12;
    v.y = o->f16;
    v.z = o->f1A;
    func_80049A1C(&v, &m2);
    if (!(o->f2C & 2)) {
        r = func_8005C49C();
        a = (r & 0x7F) * 8;
        b = ((u32)(r & 0x7F00) >> 5) + 0xC00;
        v.x = a;
        v.y = b;
    } else {
        r2 = func_8005C49C();
        a = (r2 & 0x7F) * 8 + 0xE40;
        b = ((u32)(r2 & 0x7F00) >> 5) + 0xE00;
        v.x = a;
        v.y = b;
    }
    v.z = 0;
    func_80049A1C(&v, &m1);
    func_80048EAC(&m2, &m1);
    v.x = 0;
    v.y = 0;
    v.z = 0xA;
    func_80048FBC(&m1, &v, &v);
    o->f12 = v.x;
    o->f16 = v.y;
    o->f1A = v.z;
    o->f2 = o->f2 + 1;
    o->f1C = 0;
}
