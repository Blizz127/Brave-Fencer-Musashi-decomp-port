#include "psx_types.h"

/* Reset the low byte of the event counter for the timed direction step. */
void func_80171B44(u8 *player)
{
    player[0x20C] = 0;
}
