/* Owner re-derivation from retail assembly (clean input set). */
/* When the D_800AF630 state allows it (pad bit 0x800 held or D_800A5E78 clear), sets its byte 0xA3E5 and calls func_801281D8. */
#include "psx_types.h"

extern u8 D_800AF630[];
extern u16 D_80078DD2;
extern s32 D_800A5E78;

extern void func_801281D8(void);

void func_80015208(void)
{
    u8 *base;
    s32 flag;
    s32 mask;

    base = D_800AF630;
    if (base[0xA434] == 0 && base[0xA3E0] == 1 && base[0xA3E7] == base[0xA3E0] && base[0xA3E5] != base[0xA3E7]) {
        if (base[0xA3E6] != 0) {
            base[0xA3E5] = 1;
            return;
        }
        mask = 0x800;
        if (base[0xA3E3] == 0) {
            flag = D_80078DD2 & mask;
            if (flag == mask) {
                flag = 1;
            } else {
                flag = 0;
            }
            if (flag != 0 || D_800A5E78 == 0) {
                base[0xA3E5] = 1;
                func_801281D8();
            }
        }
    }
}
