/* Owner re-derivation from retail assembly (clean input set). */
/* When the +0xB8 flag is set, latches D_80126938 and dispatches on the D_80126B3A mode: each mode plays its sound/effect set (ids 0x8A..0x8E); unknown modes run the default pair. Then clears the 0x145EE8 request and runs the common step. */
#include "psx_types.h"

extern s16 D_80126938;
extern s16 D_80126B3A;
extern void func_80029124(s32 a, s32 b);
extern u8 func_80029178(s32 a);
extern void func_80029514(s32 v);
extern s32 func_8002A088(s32 a);
extern void func_8002A2D4(s32 a);
extern void func_8002A544(s32 a);
extern void func_8002A7B4(s32 a);
extern s32 func_800D0EC4(void);
extern void func_800D0F0C(s32 a, s32 b);
extern void func_800D128C(s32 a, s32 b);
extern void func_80145EE8(s32 a);
extern void func_80147060(void *o);
extern void func_80171A1C(void *o);
extern void func_80174158(void *o);
extern void func_80174188(void *o);
extern void func_801741A8(void *o);

void func_80173EF8(u8 *o) {
    s32 pad[2]; /* unused 8 frame bytes in retail */
    s32 v;
    u8 id;
    if (*(u16 *)(o + 0xB8) & 0x8000) {
        D_80126938 = 1;
        func_80147060(o);
        v = (s16)((u16)D_80126B3A);
        id = v;
        switch (v) {
        case 0:
            break;
        case 0x70:
            func_800D128C(id, 0);
            break;
        case 0x78:
            func_80029124(0x8A, 1);
            func_80029514(0xC8);
            func_80174158(o);
            func_8002A088(0x64);
            func_8002A2D4(0x20);
            func_8002A544(0x20);
            func_8002A7B4(0x18);
            break;
        case 0x79:
            func_80029124(0x8B, 1);
            func_80029514(0x258);
            func_80174158(o);
            func_8002A088(0xA0);
            func_8002A2D4(0x28);
            func_8002A544(0x40);
            func_8002A7B4(0x30);
            break;
        case 0x7A:
            func_80029124(0x8C, 1);
            func_80029514(0x384);
            func_80174158(o);
            func_8002A088(0xF0);
            func_8002A2D4(0x30);
            func_8002A544(0x60);
            func_8002A7B4(0x48);
            break;
        case 0x7B:
            func_80029124(0x8D, 1);
            func_80029514(0x4B0);
            func_80174158(o);
            func_8002A088(0x140);
            func_8002A2D4(0x40);
            func_8002A544(0x80);
            func_8002A7B4(0x60);
            break;
        case 0x7C:
            if (func_80029178(0x11D) == 0) {
                func_80174188(o);
            }
            func_8002A088(0x190);
            func_8002A2D4(0x50);
            func_8002A544(0xA0);
            func_8002A7B4(0x80);
            func_801741A8(o);
            func_80029124(0x8E, 1);
            break;
        default:
            func_800D0F0C(func_800D0EC4(), (s16)((u16)D_80126B3A));
            break;
        }
        func_80145EE8(0);
        func_80171A1C(o);
    }
}
