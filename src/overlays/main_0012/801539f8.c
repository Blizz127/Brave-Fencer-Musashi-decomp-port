#include "psx_types.h"

typedef struct { u16 x; u16 y; u16 z; u16 w; } V8;
typedef struct { u8 b[8]; } B8;
typedef struct {
    u8 pad0[0x88];
    u16 f88;
    u16 f8A;
    u16 f8C;
} Obj;

extern s32 func_80133784(s32 k, V8 *p, V8 *q);

s32 func_801539F8(Obj *o, V8 *in) {
    V8 a;
    V8 b;
    V8 c;
    s32 r1;
    s32 r2;

    *(B8 *)&a = *(B8 *)((u8 *)o + 0xA0);
    *(B8 *)&b = *(B8 *)in;
    a.y = a.y - 4;
    r1 = func_80133784(0, &a, &b);
    if (r1 == 0x2000) {
        *(B8 *)&a = *(B8 *)&b;
        b.y = b.y + 8;
        r2 = func_80133784(0, &a, &b);
        if (r2 == r1) {
            if (func_80133784(2, &b, &c) == r2) {
                o->f88 = b.x;
                o->f8A = b.y;
                o->f8C = b.z;
                *(B8 *)((u8 *)o + 0x90) = *(B8 *)((u8 *)o + 0x88);
                return 0;
            }
        }
    }
    return 1;
}
