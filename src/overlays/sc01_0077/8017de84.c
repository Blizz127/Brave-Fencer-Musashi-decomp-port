#include "psx_types.h"

typedef void (*PlayerStateHandler)(u8 *player);
extern PlayerStateHandler D_8018A740[];

/* The event object selects its handler with the byte at offset 0x214. */
void func_8017DE84(u8 *player)
{
    D_8018A740[player[0x214]](player);
}
