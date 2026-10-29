/* func_80146750 - clean-room owner re-derivation (nmG). */
/* Claims the first free entry (leading half 0) among entries 8..19 of the 0x68-byte pool D_8011F9D0,
 * fills it from the request record and returns the entry, or NULL.
 * e is the returned entry pointer (user biv); p = &D[i] is set in the body so loop.c reduces
 * it to a giv whose copy init does not get swapped by cse2. */
#include "psx_types.h"

typedef struct {
    u16 id;       /* 0x00 */
    u8 pad02[4];
    u16 h06;      /* 0x06 */
    u8 pad08[2];
    u16 h0A;      /* 0x0A */
    u8 pad0C[2];
    u16 h0E;      /* 0x0E */
    u8 pad10[0x3C];
    u32 w4C;      /* 0x4C */
    u32 w50;      /* 0x50 */
    u32 w54;      /* 0x54 */
    u8 pad58[0x10];
} Slot80146750; /* 0x68 bytes */

typedef struct {
    u16 id;   /* 0x00 */
    u16 h2;   /* 0x02 */
    u16 h4;   /* 0x04 */
    u16 h6;   /* 0x06 */
    u32 w8;   /* 0x08 */
    u32 wC;   /* 0x0C */
    u32 w10;  /* 0x10 */
} Req80146750;

extern Slot80146750 D_8011F9D0[20];

Slot80146750 *func_80146750(Req80146750 *r)
{
    s32 i;
    Slot80146750 *p;
    Slot80146750 *e;

    i = 8;
    e = &D_8011F9D0[8];
    for (; i < 20; e++, i++) {
        p = &D_8011F9D0[i];
        if (p->id == 0) {
            D_8011F9D0[i].w4C = r->w8;
            p->id = r->id;
            D_8011F9D0[i].h06 = r->h2;
            D_8011F9D0[i].h0A = r->h4;
            D_8011F9D0[i].h0E = r->h6;
            D_8011F9D0[i].w50 = r->wC;
            D_8011F9D0[i].w54 = r->w10;
            return e;
        }
    }
    return 0;
}
