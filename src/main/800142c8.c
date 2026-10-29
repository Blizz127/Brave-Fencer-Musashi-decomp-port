/* Owner re-derivation from retail assembly (clean input set). */
/* Runs a fixed sequence of subsystem init calls. */
#include "psx_types.h"

extern void func_80029044(void);
extern void func_8002906C(void);
extern void func_80029094(void);
extern void func_80029218(void);
extern void func_8001903C(void);
extern void func_80018FC8(void);
extern void func_8001B324(void);
extern void func_80028FDC(s32);
extern void func_80029020(s32);
extern void func_80028DBC(s32);
extern void func_80028E00(s32);

void func_800142C8(void) {
    func_80029044();
    func_8002906C();
    func_80029094();
    func_80029218();
    func_8001903C();
    func_80018FC8();
    func_8001B324();
    func_80028FDC(7);
    func_80029020(7);
    func_80028DBC(0);
    func_80028E00(0);
}
