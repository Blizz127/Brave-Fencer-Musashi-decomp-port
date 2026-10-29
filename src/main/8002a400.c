/* Owner re-derivation from retail assembly (clean input set). */
/* Same pattern as func_8002A1B4: accept check on a state value, then compare two other state values. */
#include "psx_types.h"

extern s32 func_8002A4B8(void);
extern s32 func_8002A4C8(void);
extern s32 func_8002A4D8(void);
extern s32 func_8002AA00(s32);

s32 func_8002A400(void) {
    if (func_8002AA00(func_8002A4B8() + 1) != 0) {
        return func_8002A4C8() == func_8002A4D8();
    }
    return 0;
}
