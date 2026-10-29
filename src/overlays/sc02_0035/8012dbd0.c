/* Owner re-derivation from retail assembly (clean input set). */
/* Walks the slot pool (skipping o); each live, unmarked slot closer than the squared extent of o's +0x58 box (default 0x80) gets the x/y/k tag. */
#include "psx_types.h"

typedef struct {
    u8 p0[4];
    s16 f4;
    s16 f6;
    s16 f8;
    s16 fA;
    s16 fC;
    s16 fE;
} Box;

typedef struct {
    u8 p0[0x5C];
    u16 f5C;
    u16 f5E;
    u16 f60;
    u16 f62;
    u8 p64[0xAE - 0x64];
    u16 fAE;
} S;

extern int abs(int); /* gcc builtin: expands inline (ABS_EXPR copy form) */
extern s32 func_80013350(void *a, void *b);
extern S *func_8012DDA4(void);
extern S *func_8012DE2C(void *except);

s32 func_8012DBD0(u8 *o, s32 x, s32 y, s32 k) {
    S *s;
    Box *b;
    u32 w;
    s32 m;
    s32 t;
    s = func_8012DE2C(o);
    if (s != 0) {
        w = *(u32 *)(o + 0x58);
        if (w == 0) {
            m = 0x80;
        } else {
            b = (Box *)(w & 0xFFFFFFF);
            m = abs(b->f4);
            t = abs(b->f6);
            if (m < t) m = t;
            t = abs(b->f8);
            if (m < t) m = t;
            t = abs(b->fA);
            if (m < t) m = t;
            t = abs(b->fC);
            if (m < t) m = t;
            t = abs(b->fE);
            if (m < t) m = t;
        }
        m = m * m;
        do {
            if (s->f5C & 0x8000) {
                if (!(s->f5C & 0x4000)) {
                    if (s->f5E != k && !(s->fAE & 1) && func_80013350(o + 4, (u8 *)s + 4) < m) {
                        s->f60 = x;
                        s->f62 = y;
                        s->f5E = k;
                        s->f5C |= 1;
                    }
                }
            }
        } while ((s = func_8012DDA4()) != 0);
    }
    return 0;
}
