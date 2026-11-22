/* Select the third state when the actor's countdown expires. */
#include "psx_types.h"
extern s32 func_8012BEE8(u8 *actor);
extern void func_80184594(u8 *actor);
void func_80184AEC(u8 *actor)
{
    if (func_8012BEE8(actor))
        func_80184594(actor);
}
