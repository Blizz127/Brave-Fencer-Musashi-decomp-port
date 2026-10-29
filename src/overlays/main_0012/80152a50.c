/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"

extern void func_80149020(u8 *o);
extern int func_80153800(u8 *o);
extern void func_80146D90(u8 *o);
extern void func_80152AC8(u8 *o);
extern void func_80159B3C(u8 *o);
extern void func_80165718(u8 *o);

/* unused_frame: retail reserves 16 bytes of locals that nothing references. */
void func_80152A50(u8 *o) {
    u8 unused_frame[16];
    func_80149020(o);
    if (func_80153800(o) == 0) {
        *(u16 *)(o + 4) = 0;
        *(u16 *)(o + 6) = *(u16 *)(o + 0x88);
        *(u16 *)(o + 8) = 0;
        *(u16 *)(o + 0xA) = *(u16 *)(o + 0x8A);
        *(u16 *)(o + 0xC) = 0;
        *(u16 *)(o + 0xE) = *(u16 *)(o + 0x8C);
        func_80146D90(o);
        func_80152AC8(o);
        func_80159B3C(o);
        func_80165718(o);
    }
}
