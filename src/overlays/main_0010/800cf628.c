/* Owner re-derivation from retail assembly (clean input set). */
/* Handler: when func_800CFC5C classifies the current mode word D_800B9A08, saves it in D_80114E94 and replaces it with func_800CFBF8's result (running func_80011CFC, or restoring the mode and func_80011D54(5) when func_800CFD68 rejects the new mode and D_80127084 is clear); otherwise clears D_80127084 and either runs func_80011D54(6) or sets D_80114E98. */
#include "psx_types.h"

extern s32 func_800CFC5C(s32 id);
extern s16 func_800CFBF8(void);
extern s32 func_800CFD68(s32 id);
extern void func_80011CFC(void);
extern void func_80011D54(s32 a);
extern s16 D_800B9A08;
extern s32 D_80114E94;
extern s32 D_80114E98;
extern s32 D_80127084;

void func_800CF628(void)
{
    s16 *p;
    s16 t;
    s32 a;

    p = &D_800B9A08;
    if (func_800CFC5C(*p) != 0) {
        if (D_80127084 != 0) {
            D_80114E94 = *p;
            *p = func_800CFBF8();
cfc:
            func_80011CFC();
        } else {
            D_80114E94 = *p;
            t = func_800CFBF8();
            *p = t;
            if (func_800CFD68(t) != 0) {
                goto cfc;
            }
            *p = D_80114E94;
            a = 5;
            goto d54;
        }
    } else {
        D_80127084 = 0;
        if (func_800CFD68(*p) != 0) {
            a = 6;
            goto d54;
        }
        D_80114E98 = 1;
    }
    return;
d54:
    func_80011D54(a);
}
