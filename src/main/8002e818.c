/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_8002EA10(void);                                  /* extern */
extern u8 D_8006A9D4[];
extern u16 D_800A4E8C;
extern u16 D_800A4E8E;
extern s16 D_800A4EE0;
extern u16 D_800A4EE2;
extern s16 D_800A4EE4;
extern u8 D_800A4EE6[];

void func_8002E818(s32 arg0) {
    s32 var_s0;
    u8 temp_a0;
    s32 temp_v0;
    u16 table_value;

    var_s0 = arg0;
    if ((D_800A4E8E & 4) && (D_800A4E8C == 0x3E)) {
        func_8002EA10();
    }
    temp_a0 = *D_800A4EE6;
    temp_v0 = temp_a0 & 4;
    if (temp_v0 == 0) {
        temp_v0 = temp_a0 | 4;
        *D_800A4EE6 = temp_v0;
        temp_v0 &= 2;
        if (temp_v0 == 0) {
            if ((u32) (var_s0 & 0xFFFF) >= 5U) {
                var_s0 = 4;
            }
            D_800A4EE0 = 0x4000;
            D_800A4EE4 = 0x2000;
            table_value = *(u16 *)(D_8006A9D4 + ((var_s0 & 0xFFFF) * 2));
            *D_800A4EE6 = temp_a0 | 5;
            D_800A4EE2 = table_value;
        }
    }
}
