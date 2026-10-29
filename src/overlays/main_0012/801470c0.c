/* Owner re-derivation from retail assembly (clean input set). */
/* Sets or clears bit 31 of word 1 of the linked record, by whether any of three byte counters is nonzero. */
#include "psx_types.h"

typedef struct {
    u32 pad0;
    u32 flags; /* 0x4 */
} Rec801470c0;

typedef struct {
    u8 pad0[0x20];
    u32 link; /* 0x20: guest pointer to Rec801470c0 */
    u8 pad1[0x220 - 0x24];
    u8 c0; /* 0x220 */
    u8 c1; /* 0x221 */
    u8 c2; /* 0x222 */
} Obj801470c0;

void func_801470C0(Obj801470c0 *o)
{
    Rec801470c0 *r;

    r = (Rec801470c0 *)o->link;
    if (r != 0) {
        if (o->c1 | o->c0 | o->c2) {
            r->flags |= 0x80000000;
        } else {
            r->flags &= 0x7FFFFFFF;
        }
    }
}
