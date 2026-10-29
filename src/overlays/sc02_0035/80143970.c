/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the main_0012 twin at this address. */
#include "psx_types.h"

/* func_8012C588 returns a value that this wrapper hands back unchanged in v0 (retail callers read it). */
extern s32 func_8012C588(s32 id, s32 arg);

s32 func_80143970(s32 a) {
    return func_8012C588(0x1B, a);
}
