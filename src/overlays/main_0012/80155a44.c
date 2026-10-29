/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern u8 func_80029178(s32 a);
extern s32 func_8014BEF8(u8 *o);
extern u16 func_80148800(u8 *o);
extern void func_80149CB4(u8 *o, s32 k);

#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_80155A44(u8 *o)
{
    switch (o[0x1AA]) {
    case 0:
    case 1:
        if (func_80029178(0x85) != 0 && func_8014BEF8(o) != 0 && o[0x1C0] != 0
            && (func_80148800(o) & 0x10) != 0) {
            func_80149CB4(o, 2);
            return 1;
        }
        break;
    case 2:
        if (func_80029178(0x86) != 0 && (func_80148800(o) & 0x10) != 0) {
            func_80149CB4(o, 3);
            return 1;
        }
        break;
    }
    RETURN_NO_VALUE;
}
