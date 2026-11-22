/* Update two backdrop records and submit their gradients or scratch primitives. */
#include "mine_backdrop.h"

void func_8012956C(void) {
    MineBackdropRecord *rec;
    MineBackdropPrimitive *prim;
    MineBackdropTransform *mtx;
    MineBackdropGlobals *g;
    u8 *col;
    s32 i;
    s32 k;

#if defined(__mips__) && !defined(__LP64__)
    prim = (MineBackdropPrimitive *)0x1F800010;
    mtx = (MineBackdropTransform *)0x1F800000;
#else
    prim = &mine_backdrop_primitive;
    mtx = &mine_backdrop_transform;
#endif
    g = &D_800AF630;
    if (D_80114EE0 != 0) {
        rec = D_800B9A78;
        i = 0;
        col = D_8019F950;
        k = 4;
        do {
            switch (rec->state) {
            case 0x7FFF:
                break;
            case 0x7FFD:
                if (i == 0) {
                    func_801299C8(D_801A0D70, 1, rec);
                }
                func_80015B6C(-0xA0, -0x74, 0x140, 0x8C, col[0], col[1], col[2], col[4], col[5], col[6]);
                func_80015B6C(-0xA0, 0x18, 0x140, 0x3C, col[4], col[5], col[6], col[8], col[9], col[10]);
                func_80015B6C(-0xA0, 0x54, 0x140, 0x28, col[8], col[9], col[10], col[12], col[13], col[14]);
                break;
            case 0x7FFE:
                func_801299C8(D_801A0D70, i * 2, rec);
                func_80015B6C(-0xA0, rec->off_y, 0x140, rec->height, D_8019F94C[k], D_8019F94C[k + 1], D_8019F94C[k + 2],
                              D_8019F94C[k + 4], D_8019F94C[k + 5], D_8019F94C[k + 6]);
                break;
            default:
                rec->x = rec->base_x + rec->off_x;
                rec->y = rec->base_y + rec->off_y;
                func_801299C8(D_801A0D70, i + 3, rec);
#if defined(__mips__) && !defined(__LP64__)
                *mtx = *(MineBackdropTransform *)rec;
                *prim = *(MineBackdropPrimitive *)((u8 *)rec + 0x10);
                prim->w[5] = (s32)mtx;
#else
                __builtin_memcpy(mtx, rec, sizeof(*mtx));
                __builtin_memcpy(prim, (u8 *)rec + 16, sizeof(*prim));
                prim->w[5] = 0x1f800000;
#endif
                if (rec->flag == 0) {
                    func_80051044(prim, &D_800A6518[g->template_index * 0x14], rec->state);
                } else {
                    func_8005080C(prim, &D_800A6518[g->template_index * 0x14], rec->state);
                }
                break;
            }
            rec++;
            i++;
            k += 8;
        } while (i < 2);
    }
}
