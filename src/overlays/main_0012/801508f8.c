/* Owner re-derivation from retail assembly (clean input set). */
/* Unless +0x1BF is zero or the type at +0 is 0x1A, calls func_8015094C until it returns zero. */
#include "psx_types.h"

typedef struct {
    /* 0x000 */ u16 type;
    u8 pad002[0x1BD];
    /* 0x1BF */ u8 active;
} Obj801508F8;

extern s32 func_8015094C(Obj801508F8 *obj);

/* Declared s32 but returns no value: retail keeps v0 live at the exit (nop in the
 * first delay slot), which only an int-typed function produces. The one caller
 * (801464FC) ignores v0 and v1: it next calls func_8014B5B0, which touches neither,
 * then func_80161D88, which writes both at entry. Host builds return an explicit 0
 * (the RETURN_NO_VALUE convention, docs/MATCHING.md on main). */
#ifdef __mips__
#define RETURN_NO_VALUE return
#else
#define RETURN_NO_VALUE return 0
#endif

s32 func_801508F8(Obj801508F8 *obj)
{
    if (obj->active == 0) {
        RETURN_NO_VALUE;
    }
    if (obj->type == 0x1A) {
        RETURN_NO_VALUE;
    }
    while (func_8015094C(obj)) {
    }
    RETURN_NO_VALUE;
}
