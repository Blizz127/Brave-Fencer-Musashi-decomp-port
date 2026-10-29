/* Owner re-derivation from retail assembly (clean input set). */
/* Same pattern as func_8002A1B4: accept check on a state value, then compare two other state values. */
#include "psx_types.h"

extern s32 func_8002A998(void);
extern s32 func_8002A9A8(void);
extern s32 func_8002A9B8(void);
extern s32 func_8002AA00(s32);

s32 func_8002A8E0(void) {
    if (func_8002AA00(func_8002A998() + 1) != 0) {
        return func_8002A9A8() == func_8002A9B8();
    }
    return 0;
}
