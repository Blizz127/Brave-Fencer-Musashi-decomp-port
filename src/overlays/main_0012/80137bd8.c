/* func_80137BD8 [80137BD8,80137D04) - owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u8 b[20]; } T; /* 20-byte table entry */

typedef struct {
    u8 p0[4];
    s16 f4;
    u8 p6[2];
    u32 f8;
    u8 pC[0x12];
    u8 f1E;
    u8 p1F;
    u8 f20;
} O;

/* callers read v0: none-direct; void.
 * func_80139680 takes (o, table entry); its own source declares the second parameter as s32. */
extern void func_801392FC(O *o, s32 a, T *t);
extern void func_801397B0(O *o);
extern void func_80137DD4(O *o, u8 *v, T *t);
extern void func_80139680(O *o, T *t);
extern u16 D_800B9A02;
extern u8 D_800A6518[];
extern u32 D_80127524;
extern u16 D_800B99D8;

void func_80137BD8(O *o) {
    u8 v[4];

    if (o->f8 & 0x1000) {
        func_801392FC(o, o->f1E, &((T *)&D_800A6518)[D_800B9A02]);
        if (o->f20 != 0) {
            func_801397B0(o);
        }
    }
    if (D_80127524 == (u32)o) {
        if (o->f4 == 7) {
            v[0] = 0x39;
            v[1] = 0xFF;
            v[2] = 0x71;
            func_80137DD4(o, v, &((T *)&D_800A6518)[D_800B9A02]);
        } else if (o->f4 == 3) {
            if (D_800B99D8 & 4) {
                func_80139680(o, &((T *)&D_800A6518)[D_800B9A02]);
            }
        }
    }
}
