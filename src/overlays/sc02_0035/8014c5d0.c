/* Owner re-derivation from retail assembly (clean input set). */
/* Forwards a0/a1 and appends &D_80186118 as the off argument to func_8014C5FC,
 * sign-extending its s32 result to s16 (sll/sra 0x10). */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 x; /* 0x06 */
    u8 pad8[2];
    s16 y; /* 0x0A */
    u8 padC[2];
    u16 z; /* 0x0E */
} Obj8014C5D0;

typedef struct {
    u16 x; /* 0x00 */
    s16 y; /* 0x02 */
    u16 z; /* 0x04 */
} Off8014C5D0;

extern Off8014C5D0 D_80186118;
extern s32 func_8014C5FC(Obj8014C5D0 *a, Obj8014C5D0 *b, Off8014C5D0 *off);

s16 func_8014C5D0(Obj8014C5D0 *a0, Obj8014C5D0 *a1)
{
    return (s16)func_8014C5FC(a0, a1, &D_80186118);
}
