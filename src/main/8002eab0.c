/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80038308(s16);                         /* extern */
extern s16 D_800A4E86;
extern u16 D_800A4E8E[];

void func_8002EAB0(void) {
    if (*D_800A4E8E & 0x10) {
        func_80038308(D_800A4E86);
        *D_800A4E8E = (*D_800A4E8E | 9) & 0xFFEF;
    }
}
