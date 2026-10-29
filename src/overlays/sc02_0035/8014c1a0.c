/* Owner re-derivation from retail assembly (clean input set). */
/* Thunk: forwards to func_8014C1C8 with the shared offset table D_80186110. */
#include "psx_types.h"

typedef struct {
    u8 pad0[6];
    u16 x; /* 0x06 */
    u8 pad8[2];
    u16 y; /* 0x0A */
    u8 padC[2];
    u16 z; /* 0x0E */
} Obj8014C1C8;

extern u16 D_80186110[];
extern void func_8014C1C8(Obj8014C1C8 *a, Obj8014C1C8 *b, u16 *off);

void func_8014C1A0(Obj8014C1C8 *a, Obj8014C1C8 *b)
{
    func_8014C1C8(a, b, D_80186110);
}
