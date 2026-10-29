/* Owner re-derivation from retail assembly (clean input set). */
#include "psx_types.h"
extern void func_80156FA8(u8 *a, u8 *b, u8 *c);
extern void func_80147478(u8 *o);
extern void func_80149350(u8 *o, u8 *a, u8 *b);

void func_80156ECC(u8 *o, u8 *a, u8 *b, u8 *c, u8 *d)
{
    u16 v;

    func_80156FA8(b, c, d);
    func_80147478(o);
    func_80149350(o, a, a);
    v = *(u16 *)(b + 6);
    if (*(s16 *)(b + 6) == 0) {
        func_80149350(o, b, b);
        *(u16 *)(b + 6) = v;
    }
    v = *(u16 *)(c + 6);
    if (*(s16 *)(c + 6) == 0) {
        func_80149350(o, c, c);
        *(u16 *)(c + 6) = v;
    }
    v = *(u16 *)(d + 6);
    if (*(s16 *)(d + 6) == 0) {
        func_80149350(o, d, d);
        *(u16 *)(d + 6) = v;
    }
}
