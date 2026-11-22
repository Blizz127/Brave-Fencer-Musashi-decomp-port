/* The mine's shared stat-event actor latches once its interaction flag is set.
 * Selectors 0x78..0x7c update persistent flags and progression, then award
 * Body, Mind, Lumina and Fusion in that order. */
#include "mine_stat_event.h"

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

void func_80173EF8(u8 *o) {
#if defined(__mips__) && defined(__GNUC__) && __GNUC__ == 2
    s32 pad[2]; /* Match this room's original eight unused frame bytes. */
#endif
    s32 v;
    u8 id;
    if (MINE_STAT_ACTOR_READY(o)) {
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
            v = func_800D0EC4();
            func_800D0F0C(v, D_80126B3A);
            break;
        }
        func_80145EE8(0);
        func_80171A1C(o);
    }
}
