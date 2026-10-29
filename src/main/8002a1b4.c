/* Owner re-derivation from retail assembly (clean input set). */
/* Checks a state value with func_8002AA00 and, if accepted, compares two other state values for equality. */
#include "psx_types.h"

extern s32 func_8002A26C(void);
extern s32 func_8002A27C(void);
extern s32 func_8002A28C(void);
extern s32 func_8002AA00(s32);

s32 func_8002A1B4(void) {
    if (func_8002AA00(func_8002A26C() + 1) != 0) {
        return func_8002A27C() == func_8002A28C();
    }
    return 0;
}
