/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    s32 w[4];
} Blk16;

typedef struct {
    s32 w[9];
} Blk36;

typedef struct {
    s32 w00[4];
    u8 pad10[0x0C];
    u16 f1C;
    u16 f1E;
    u8 pad20[0x14];
    u16 state;
    u16 flag;
    u16 f38;
    u16 f3A;
    u16 f3C;
    u16 f3E;
    u16 f40;
    u16 f42;
    u8 pad44[0x10];
} Rec8012956C; /* size 0x54 */

extern s16 D_80114EE0;
extern u8 D_80185BF0;
extern Rec8012956C D_800B9A78[];
extern u8 D_800AF630;
extern u8 D_801840B4[];
extern u8 D_801840B8[];
extern u8 D_800A6518[];
extern void func_801299C8(s16 sel, s16 idx, void *rec);
extern void func_80015B6C(s32 x, s16 y, s32 w, s16 h, s32 r0, s32 g0, s32 b0, s32 r1, s32 g1, s32 b1);
extern void func_8005080C(void *prim, void *tpl, u16 state);
extern void func_80051044(void *prim, void *tpl, u16 state);

void func_8012956C(void) {
    Rec8012956C *rec;
    Blk36 *prim;
    Blk16 *mtx;
    u8 *g;
    u8 *col;
    s32 i;
    s32 k;

    prim = (Blk36 *)0x1F800010;
    mtx = (Blk16 *)0x1F800000;
    g = &D_800AF630;
    if (D_80114EE0 != 0) {
        rec = D_800B9A78;
        i = 0;
        col = D_801840B8;
        k = 4;
        do {
            switch (rec->state) {
            case 0x7FFF:
                break;
            case 0x7FFD:
                if (i == 0) {
                    func_801299C8(D_80185BF0, 1, rec);
                }
                func_80015B6C(-0xA0, -0x74, 0x140, 0x8C, col[0], col[1], col[2], col[4], col[5], col[6]);
                func_80015B6C(-0xA0, 0x18, 0x140, 0x3C, col[4], col[5], col[6], col[8], col[9], col[10]);
                func_80015B6C(-0xA0, 0x54, 0x140, 0x28, col[8], col[9], col[10], col[12], col[13], col[14]);
                break;
            case 0x7FFE:
                func_801299C8(D_80185BF0, i * 2, rec);
                func_80015B6C(-0xA0, rec->f3A, 0x140, rec->f3E, D_801840B4[k], D_801840B4[k + 1], D_801840B4[k + 2],
                              D_801840B4[k + 4], D_801840B4[k + 5], D_801840B4[k + 6]);
                break;
            default:
                rec->f1C = rec->f40 + rec->f38;
                rec->f1E = rec->f42 + rec->f3A;
                func_801299C8(D_80185BF0, i + 3, rec);
                *mtx = *(Blk16 *)rec;
                *prim = *(Blk36 *)((u8 *)rec + 0x10);
                prim->w[5] = (s32)mtx;
                if (rec->flag == 0) {
                    func_80051044(prim, &D_800A6518[*(u16 *)(g + 0xA3D2) * 0x14], rec->state);
                } else {
                    func_8005080C(prim, &D_800A6518[*(u16 *)(g + 0xA3D2) * 0x14], rec->state);
                }
                break;
            }
            rec++;
            i++;
            k += 8;
        } while (i < 2);
    }
}
