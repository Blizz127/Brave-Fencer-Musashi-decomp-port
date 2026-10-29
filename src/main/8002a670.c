/* Owner re-derivation from retail assembly (clean input set). */
/* Same pattern as func_8002A1B4: accept check on a state value, then compare two other state values. */
#include "psx_types.h"

extern s32 func_8002A728(void);
extern s32 func_8002A738(void);
extern s32 func_8002A748(void);
extern s32 func_8002AA00(s32);

s32 func_8002A670(void) {
    if (func_8002AA00(func_8002A728() + 1) != 0) {
        return func_8002A738() == func_8002A748();
    }
    return 0;
}
