/* func_80162FF4 [80162FF4,801630C4) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u8 p0[0x10]; s16 f10; s16 f12; s16 f14; } S;
typedef struct { u8 p0[0x20]; S *f20; } O;

/* callers read v0: none-direct (no jal caller); void. */
extern void func_80162F60(void *obj, s32 delta);
extern void func_80162F80(void *o, s32 a);
extern void func_80162FA0(u8 *obj, s32 delta);
extern void func_80013F3C(void *m);
extern void func_800126C4(void *m, s32 angle);
extern void func_80012558(void *m, s32 angle);
extern void func_800123F0(void *m, s32 angle);
extern void func_800484EC(void *p, void *rec, void *q);
extern void func_80146D30(u8 *o);
extern s32 D_80127090;
extern s32 D_80127094;
extern s32 D_80127098;

void func_80162FF4(O *o, s32 a, s32 b, s32 c) {
    s32 pad[4]; /* unreferenced: retail frame keeps 16 bytes below v */
    s32 v[3];
    s32 m[8];
    S *s;

    s = o->f20;
    func_80162F60(o, a);
    func_80162F80(o, b);
    func_80162FA0((u8 *)o, c);
    func_80013F3C(m);
    func_800126C4(m, s->f10);
    func_80012558(m, s->f12);
    func_800123F0(m, s->f14);
    func_800484EC(m, (u8 *)o + 0x24, v);
    D_80127090 = v[0];
    D_80127094 = v[1];
    D_80127098 = v[2];
    func_80146D30((u8 *)o);
}
