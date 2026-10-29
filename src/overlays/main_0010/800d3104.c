/* Owner re-derivation from retail assembly (clean input set). */
/* State handler for a record: sets the timer byte at +0x19 to 10, advances the state byte at +0x15, returns 0. */
#include "psx_types.h"

typedef struct {
    u8 pad00[0x15];
    u8 state;
    u8 pad16[3];
    u8 timer;
} Rec800D3104;

s32 func_800D3104(Rec800D3104 *r)
{
    r->timer = 10;
    r->state = r->state + 1;
    return 0;
}
