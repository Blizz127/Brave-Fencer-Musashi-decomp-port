/* Enter an actor state and reset its substate. */
#include "psx_types.h"

#if !defined(__mips__) || defined(__LP64__)
#include <string.h>
#endif

void func_8012AD44(u8 *actor, u16 state)
{
#if defined(__mips__) && !defined(__LP64__)
    *(u16 *)(actor + 2) = state;
    *(u16 *)(actor + 0x34) = 0;
#else
    u16 substate = 0;
    memcpy(actor + 2, &state, sizeof state);
    memcpy(actor + 0x34, &substate, sizeof substate);
#endif
}
