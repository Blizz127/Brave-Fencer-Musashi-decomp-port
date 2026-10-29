/* Independently re-derived from retail assembly; clean input set. */
#include "psx_types.h"

void func_80028DBC(s32);                     /* extern */
void func_80028E00(s32);                     /* extern */
void func_80028FDC(s32);                     /* extern */
void func_80029020(s32);                     /* extern */
void func_80029044();                            /* extern */
void func_8002906C();                            /* extern */
void func_80029094();                            /* extern */
void func_80029274();                            /* extern */

void func_80014338(void) {
    func_80029044();
    func_8002906C();
    func_80029094();
    func_80028FDC(7);
    func_80029020(7);
    func_80028DBC(0);
    func_80028E00(0);
    func_80029274();
}
