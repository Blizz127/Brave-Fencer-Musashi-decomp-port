/* Owner re-derivation from retail assembly (clean input set). */
/* Per-object step: runs the four common update steps, the kind handler D_80180AC0[kind & 0x7FFF],
 * then sets flag 0x08000000 of the object's word at 0x44 and clears it again for kinds 2, 3, 11 and 12
 * and for kind 25 when its mode byte (0xDD) is 4 or is 1 with the u16 at 2 equal to 3. The object is
 * also listed in a guest handler table. */
#include "psx_types.h"

/* Object header fields used here. */
typedef struct {
    u16 kind;
    u16 f2;
    u8 pad04[0x44 - 4];
    u32 flags;
    u8 pad48[0xDD - 0x48];
    u8 mode;
} Obj;

extern u32 D_80180AC0[]; /* guest handler table: u32 elements, cast at use */
extern void func_80150B28(Obj *o);
extern void func_80158638(Obj *o);
extern void func_8015126C(Obj *o);
extern void func_8014900C(Obj *o);

void func_80159A20(Obj *o)
{
    func_80150B28(o);
    func_80158638(o);
    func_8015126C(o);
    func_8014900C(o);
    ((void (*)(Obj *))D_80180AC0[o->kind & 0x7FFF])(o);
    o->flags |= 0x08000000;
    switch (o->kind) {
    case 2:
    case 3:
    case 11:
    case 12:
        o->flags &= 0xF7FFFFFF;
        break;
    case 25:
        switch (o->mode) {
        case 1:
            if (o->f2 == 3) {
                o->flags &= 0xF7FFFFFF;
            }
            break;
        case 4:
            o->flags &= 0xF7FFFFFF;
            break;
        }
        break;
    }
}
