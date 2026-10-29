/* Owner re-derivation from retail assembly (clean input set). */
/* Per-frame step: derives the input nibble from the +0xA9 kind (0x41/0x53/0x73), debounces it into +0xDE/+0xDF, then either runs the plain handlers or the D_8011DB0C mode dispatch (modes 1-3) with its effect calls. */
#include "psx_types.h"

extern u8 D_80078E78[];
extern s32 D_80127518;
extern s16 D_800B9A08;
extern u16 D_8011DB0C;
extern s16 D_8011DB1A;
extern u16 D_8011F748;
extern void func_80146994(s32 a, void *o, s32 b, s32 c);
extern s32 func_80146A6C(s32 a, void *o, s32 x, s16 y, s32 z, u32 c1, u32 c2);
extern u16 func_80148800(void *o);
extern s32 func_8014891C(void *o);
extern s32 func_80148980(void *o);
extern s32 func_801489E8(void *o);
extern s32 func_80148A48(void *o);
extern void func_80149020(void *o);
extern void func_80149B3C(void *o);
extern void func_80153C18(void *o);
extern void func_8015410C(void *o);
extern void func_80159B3C(void *o);
extern void func_80165718(void *o);
extern void func_8014ACE8(void *o, s32 a, s32 b);
extern void func_8014B310(void *o);
extern void func_8014BC44(void *o, s32 a);

void func_80153E00(u8 *o) {
    u8 *g;
    u16 k;
    s32 t;
    func_80149020(o);
    g = D_80078E78;
    switch (o[0xA9]) {
    case 0x41:
        k = func_80148800(o) & 0xF000;
        break;
    case 0x53:
    case 0x73:
        t = (func_8014891C(o) != 0) << 12;
        k = t;
        if (func_80148980(o) != 0) {
            k = t | 0x4000;
        }
        if (func_801489E8(o) != 0) {
            k |= 0x8000;
        }
        if (func_80148A48(o) != 0) {
            k |= 0x2000;
        }
        break;
    }
    k = k >> 12;
    if (k != 0 && !(k & o[0xDF])) {
        o[0xDE]--;
        if (o[0xDE] == 0) {
            func_80153C18(o);
        }
    }
    o[0xDF] = k;
    if (D_80127518 == 0) {
        if (D_800B9A08 != 0x3075) {
            *(u16 *)(o + 0x88) = *(u16 *)(o + 0x244);
            *(u16 *)(o + 0x8A) = *(u16 *)(o + 0x246);
            *(u16 *)(o + 0x8C) = *(u16 *)(o + 0x248);
        }
        func_8015410C(o);
        func_80159B3C(o);
        func_80165718(o);
        return;
    }
    switch (D_8011DB0C) {
    case 0:
        break;
    case 3:
        func_8014B310(o);
    case 1:
        func_8014BC44(o, D_8011DB1A);
        func_8014ACE8(o, 1, D_8011DB1A);
        D_8011DB0C = 0;
        D_8011F748 = 0;
        break;
    case 2:
        func_8014BC44(o, D_8011DB1A);
        func_8014ACE8(o, 1, D_8011DB1A);
        func_8014B310(o);
        func_80146994(0x42, o, 0, 0);
        func_80146A6C(0x42, o, 0x20, 0, 0, 1, 0);
        func_80146A6C(0x42, o, 0, 0, 0x20, 1, 0);
        func_80146A6C(0x42, o, -0x20, 0, 0, 1, 0);
        func_80146A6C(0x42, o, 0, 0, -0x20, 1, 0);
        D_8011DB0C = 0;
        D_8011F748 = 0;
        break;
    }
    if (D_8011F748 != 0) {
        func_8015410C(o);
        func_80159B3C(o);
        func_8014BC44(o, *(u16 *)(g + 0x3C));
        D_8011DB0C = 0;
        D_8011F748 = 0;
    }
}
