/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct { u8 b[8]; } Pk;
extern u8 D_80115100[];
extern u32 D_80115200[3];
extern u8 D_80126B18[];

void func_80163328(void *o) {
    u32 i;
    for (i = 0; i < 3; i++) {
        if (D_80115200[i] & 1) {
            *(Pk *)(((u8 * *)&D_80115100)[i] + 0x7C) = ((Pk *)&D_80126B18)[i];
            *(u16 *)(((u8 * *)&D_80115100)[i] + 0x82) = 0;
        }
    }
}
