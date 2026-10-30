/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W16), data symbols renamed for this member. */
/* Draws the two stacked backdrop bands of a dialog (selected by record kind: a flat three-bar form,
 * or a gradient form with its band height taken from the record) and submits the 2-D polygon staged
 * for each record. Kinds 0x7FFD/0x7FFE draw background bars; 0x7FFF is skipped; any other kind builds
 * a poly packet in the scratchpad and queues it, with the first or second submit routine by flag. */
#include "psx_types.h"

/* One 0x54-byte band record. */
typedef struct {
    u8 pad0[0x1C];
    u16 ax;     /* 0x1C */
    u16 ay;     /* 0x1E */
    u8 pad20[0x14];
    u16 kind;   /* 0x34 */
    u16 flag;   /* 0x36 */
    u16 w4;     /* 0x38 */
    u16 w6;     /* 0x3A */
    u16 w8;     /* 0x3C */
    s16 wA;     /* 0x3E */
    u16 wC;     /* 0x40 */
    u16 wE;     /* 0x42 */
    u8 pad44[0x10];
} Band;

typedef struct {
    u32 w[4];
} Blk16;

typedef struct {
    u32 w[9];
} Blk36;

extern s16 D_80114EE0;
extern u8 D_800B9A78[];
extern u8 D_800AF630[];
extern u8 D_800A6518[];
extern u8 D_801B626C[];
extern u8 D_801B6270[];
extern u8 D_801B7A80;

extern void func_801299C8(u8 mode, s16 sel, u8 *rec);
extern void func_80015B6C(s16 x, s16 y, s16 w, s16 h, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1);
extern void func_80051044(u8 *pkt, u8 *tbl, u16 kind);
extern void func_8005080C(u8 *pkt, u8 *tbl, u16 kind);

void func_8012956C(void)
{
    Band *p;
    s32 i;
    s32 k;
    u8 *c;
    Blk16 *hdr = (Blk16 *)0x1F800000;
    Blk36 *pkt = (Blk36 *)0x1F800010;
    u8 *g = D_800AF630;

    if (D_80114EE0 == 0) {
        return;
    }
    p = (Band *)D_800B9A78;
    i = 0;
    c = D_801B6270;
    k = 4;
    for (; i < 2; p++, i++, k += 8) {
        switch (p->kind) {
        case 0x7FFF:
            break;
        case 0x7FFD:
            if (i == 0) {
                func_801299C8(D_801B7A80, 1, (u8 *)p);
            }
            func_80015B6C(-0xA0, -0x74, 0x140, 0x8C, c[0], c[1], c[2], c[4], c[5], c[6]);
            func_80015B6C(-0xA0, 0x18, 0x140, 0x3C, c[4], c[5], c[6], c[8], c[9], c[10]);
            func_80015B6C(-0xA0, 0x54, 0x140, 0x28, c[8], c[9], c[10], c[12], c[13], c[14]);
            break;
        case 0x7FFE:
            func_801299C8(D_801B7A80, i * 2, (u8 *)p);
            func_80015B6C(-0xA0, (s16)p->w6, 0x140, p->wA, D_801B626C[k], D_801B626C[k + 1],
                          D_801B626C[k + 2], D_801B626C[k + 4], D_801B626C[k + 5], D_801B626C[k + 6]);
            break;
        default:
            p->ax = p->wC + p->w4;
            p->ay = p->wE + p->w6;
            func_801299C8(D_801B7A80, i + 3, (u8 *)p);
            *hdr = *(Blk16 *)p;
            *pkt = *(Blk36 *)((u8 *)p + 0x10);
            pkt->w[5] = (u32)hdr;
            if (p->flag == 0) {
                func_80051044((u8 *)pkt, D_800A6518 + *(u16 *)(g + 0xA3D2) * 20, p->kind);
            } else {
                func_8005080C((u8 *)pkt, D_800A6518 + *(u16 *)(g + 0xA3D2) * 20, p->kind);
            }
            break;
        }
    }
}
