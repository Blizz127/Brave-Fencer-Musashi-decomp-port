/* func_80175308 [80175308,80175400) - owner re-derivation from retail assembly (clean input set).
 * Initialises the table at D_8011F7A8 (small s16/u8 fields, then a 0x98-byte copy of D_80078E78 to +0x48),
 * calls func_80016714 on the block at +0xE0 and refreshes both pages via func_801757A0. */
#include "psx_types.h"

typedef struct { s32 w[38]; } Blk; /* 0x98 bytes */

/* callers read v0: none-direct; void.
 * D_800B9A02 is declared as an array here to get the computed-address form. */
extern void func_80016714(void *p, s32 n);
extern void func_801757A0(s32 id);
extern void func_800596F4(s32 a);
extern u8 D_8011F7A8[];
extern u8 D_8011F7A9;
extern u8 D_8011F7AE;
extern s16 D_8011F7B8;
extern s16 D_8011F7B4;
extern s16 D_8011F7BE;
extern s16 D_8011F7BA;
extern s16 D_8011F7B6;
extern s16 D_8011F7BC;
extern Blk D_80078E78;
extern s16 D_800B9A02[];

void func_80175308(void) {
    u8 *p;

    p = D_8011F7A8;
    p[0] = 1;
    D_8011F7A9 = 2;
    D_8011F7AE = 0x37;
    D_8011F7B8 = -0x37;
    D_8011F7B4 = -0x37;
    D_8011F7BE = 0x37;
    D_8011F7BA = 0x37;
    D_8011F7B6 = 0x37;
    D_8011F7BC = 0x37;
    *(Blk *)(p + 0x48) = D_80078E78;
    func_80016714(p + 0xE0, 0x98);
    func_801757A0(D_800B9A02[0]);
    func_800596F4(0);
    func_801757A0((s16)(((u16 *)D_800B9A02)[0] ^ 1));
}
