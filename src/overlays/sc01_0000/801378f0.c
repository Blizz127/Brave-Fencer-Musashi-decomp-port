/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W18). */
/* Clears a 0xBFC-byte work area, then for each of three slots clears its 0x4C-byte record and
 * copies four half-words from the 8-byte-stride default table into the record's second field
 * group; finishes with the follow-up setup routine. */
#include "psx_types.h"

typedef struct {
    u16 a;
    u16 b;
    u16 c;
    u16 d;
} R4;

extern u8 D_80127520[];
extern u8 D_801269F0[];
extern u8 D_80126A28[];
extern u8 D_8017F1BC[];

extern void func_80016714(u8 *p, s32 n);
extern void func_8013A860(void);

void func_801378F0(void)
{
    s32 i;
    s32 o1;
    s32 o2;
    u8 *p;

    func_80016714(D_80127520, 0xBFC);
    for (i = 0, o2 = 0, p = D_801269F0; i < 3; i++) {
        o1 = i * 0x4C;
        func_80016714(p, 0x4C);
        ((R4 *)(D_80126A28 + o1))->a = ((R4 *)(D_8017F1BC + o2))->a;
        ((R4 *)(D_80126A28 + o1))->b = ((R4 *)(D_8017F1BC + o2))->b;
        p += 0x4C;
        ((R4 *)(D_80126A28 + o1))->c = ((R4 *)(D_8017F1BC + o2))->c;
        ((R4 *)(D_80126A28 + o1))->d = ((R4 *)(D_8017F1BC + o2))->d;
        o2 += 8;
    }
    func_8013A860();
}
