/* Owner re-derivation from retail assembly (clean input set). */
/* Advances the 32-bit counter D_80078E98 by 0x1555 (reset to 0 past 0x257F6A in its high bits) when enabled and the object's kind is not 0x1E/0x1A. */
#include "psx_types.h"

typedef struct {
    u16 kind; /* 0x0 */
} Obj8014b504;

extern s32 D_80078E98;
extern s16 D_80078E9A;

extern s32 func_8016F1AC(void);

void func_8014B504(Obj8014b504 *o)
{
    if (func_8016F1AC() == 0) {
        if (o->kind != 0x1E && o->kind != 0x1A) {
            if (D_80078E9A & 0x8000) {
                D_80078E98 += 0x1555;
                if (0x257F6A < (D_80078E98 & 0x7FFF0000)) {
                    D_80078E98 = 0;
                }
            }
        }
    }
}
