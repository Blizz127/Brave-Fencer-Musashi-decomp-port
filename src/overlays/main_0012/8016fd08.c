/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

typedef struct {
    u8 padA[0xAA];
    u16 hAA;
    u16 hAC;
} O;

extern u32 D_8011F738;
extern s32 func_80172658(O *o);
extern void func_80146D90(O *o);
extern void func_80171990(O *o);

void func_8016FD08(O *o) {
    if (func_80172658(o) != 0) {
        func_80146D90(o);
        o->hAC |= 0x40;
        o->hAA |= 0x40;
        ((s32 *)&D_8011F738)[0] -= 1;
        if (((s32 *)&D_8011F738)[0] == 0) {
            func_80171990(o);
        }
    }
}