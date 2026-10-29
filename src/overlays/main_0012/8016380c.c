#include "psx_types.h"

typedef struct {
    u8 b[8];
} B8;

extern void func_80015978(u8 *src, u8 *dst);
extern void func_80015954(u8 *src, u8 *dst);
extern s32 func_80133784(s32 a, B8 *b, u8 *c);

s32 func_8016380C(u8 *o, u8 *q) {
    B8 t;
    s32 r;
    t = *(B8 *)(o + 0x44);
    func_80015978(o + 4, q);
    r = func_80133784(1, &t, q);
    if (r != 0) {
        func_80015954(q, o + 4);
        return r;
    }
    return 0;
}
