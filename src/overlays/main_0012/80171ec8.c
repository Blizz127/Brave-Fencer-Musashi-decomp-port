/* Owner re-derivation from retail assembly (clean input set). */
/* Computes the facing colour: marks the object (+0xA9 = 0x73), takes the camera-relative angle func_8004CFEC(cam->w68 - w5C, cam->w70 - w64) + 0x800 against the aux half-word +0x12, quantised to 8; derives x = 0x80 - func_80047948(a) * 128 / 4096 and y = func_8004787C(a) / 32 + 0x80 (0x80 on the axes), clamps both to 0..0xFF and stores (x << 8) | y at +0xAE. */
#include "psx_types.h"

typedef struct {
    u8 pad0[0x12];
    s16 h12;
} Aux;

typedef struct {
    u8 pad0[0x20];
    Aux *aux;
    u8 pad24[0x85];
    u8 bA9;
    u8 padAA[4];
    u16 hAE;
} Obj;

typedef struct {
    u8 pad0[0x5C];
    s32 w5C;
    s32 w60;
    s32 w64;
    s32 w68;
    u8 pad6C[4];
    s32 w70;
} Cam;

extern Cam *D_801151D4;
extern s32 func_8004CFEC(s32 a, s32 b);
extern s32 func_80047948(s32 a);
extern s32 func_8004787C(s32 a);

/* Returns retail's leftover v0: the packed colour ((y & 0xFF) | (x << 8)) that is stored at +0xAE (untruncated). */
s32 func_80171EC8(Obj *o)
{
    s32 a;
    s32 x;
    s32 y;
    s32 r;
    Cam *c;

    c = D_801151D4;
    o->bA9 = 0x73;
    a = func_8004CFEC(c->w68 - c->w5C, c->w70 - c->w64);
    a = (a + 0x800) & 0xFFF;
    a = (o->aux->h12 - a) & 0xFF8;
    if (a == 0x400 || a == 0xC00) {
        x = 0x80;
    } else {
        x = -(func_80047948(a) * 128) / 4096 + 0x80;
    }
    if (x < 0) {
        x = 0;
    }
    if (x >= 0x100) {
        x = 0xFF;
    }
    if (a == 0 || a == 0x800) {
        y = 0x80;
    } else {
        y = func_8004787C(a) / 32 + 0x80;
    }
    if (y < 0) {
        y = 0;
    }
    if (y >= 0x100) {
        y = 0xFF;
    }
    r = (y & 0xFF) | (x << 8);
    o->hAE = r;
    return r;
}
