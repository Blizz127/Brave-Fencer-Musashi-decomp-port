/* Owner re-derivation from retail assembly (clean input set). */
/* Same body as the sc01_0005 twin at this address (W16), data symbols renamed for this member. */
/* Initializes an object slot from a placement record: copies position, flags and user data, assigns
 * the next id, links the record, then runs the type's setup handler. If the handler leaves bit 0 of the
 * status word set, the slot and its child are released and 0 is returned; otherwise the child is
 * placed at the object's position plus its offsets and the object is returned. */
#include "psx_types.h"

/* Placement record. */
typedef struct {
    u16 x;     /* 0x00 */
    u16 y;     /* 0x02 */
    u16 z;     /* 0x04 */
    u16 type;  /* 0x06 */
    u16 w8;    /* 0x08 */
    u16 flags; /* 0x0A */
    s16 aim;   /* 0x0C */
    u16 wE;    /* 0x0E */
    u32 user;  /* 0x10 */
} Def;

/* Child record. Pointer-valued fields are guest addresses (u32). */
typedef struct {
    u16 kind;   /* 0x00 */
    u8 pad2[6];
    u16 x;      /* 0x08 */
    u16 y;      /* 0x0A */
    u16 z;      /* 0x0C */
    u8 padE[4];
    s16 aim;    /* 0x12 */
} Child;

typedef struct {
    u16 type;   /* 0x00 */
    u16 f2;     /* 0x02 */
    u16 f4;     /* 0x04 */
    u16 x;      /* 0x06 */
    u16 f8;     /* 0x08 */
    u16 y;      /* 0x0A */
    u16 fC;     /* 0x0C */
    u16 z;      /* 0x0E */
    u8 pad10[0x10];
    u32 child;  /* 0x20 */
    u8 pad24[0x12];
    u16 id;     /* 0x36 */
    u8 pad38[0x18];
    u16 ox;     /* 0x50 */
    u16 oy;     /* 0x52 */
    u16 oz;     /* 0x54 */
    u8 pad56[0xE];
    u32 owner;  /* 0x64 */
    u32 def;    /* 0x68 */
    u8 pad6c[4];
    u16 w70;    /* 0x70 */
    u16 flags;  /* 0x72 */
    u8 pad74[4];
    u32 shape;  /* 0x78 */
    u8 pad7c[0xC];
    u16 x88;    /* 0x88 */
    u16 y88;    /* 0x8A */
    u16 z88;    /* 0x8C */
    u8 pad8e[0x4E];
    u32 user;   /* 0xDC */
    u8 padE0[0x1C];
    u16 wFC;    /* 0xFC */
} Obj;

extern u8 D_80182418[];
extern u16 D_801270C4[];
extern u16 D_801274E4[];
extern u32 D_8011DB08;

extern void func_80016714(void *p, s32 size);

Obj *func_8012C890(Def *d, Obj *o, u32 owner)
{
    Child *c;
    Child *c2;
    u16 zz;
    s16 aim;

    o->type = d->type;
    o->x88 = o->x = d->x;
    o->y88 = o->y = d->y;
    zz = d->z;
    o->f4 = o->f8 = o->fC = 0;
    o->z88 = o->z = zz;
    o->w70 = d->w8;
    o->flags = d->flags & 0xF7FF;
    o->wFC = d->wE;
    o->user = d->user;
    o->shape = (u32)D_80182418;
    o->id = D_801270C4[0];
    D_801270C4[0]++;
    if ((s16)D_801270C4[0] == 0) {
        D_801270C4[0] = 1;
    }
    if (owner != 0) {
        o->owner = owner;
        o->def = 0;
    } else {
        o->owner = 0;
        o->def = (u32)d;
    }
    d->flags |= 0x8000;
    o->f2 = 0;
    D_801274E4[0] &= 0xFFFE;
    ((void (*)(Obj *))((u32 *)D_8011DB08)[o->type])(o);
    if (D_801274E4[0] & 1) {
        c = (Child *)o->child;
        if (c != 0) {
            switch (c->kind) {
            case 2:
                func_80016714(c, 0x38);
                break;
            case 1:
                func_80016714(c, 0x84);
                break;
            }
        }
        func_80016714(o, 0x10C);
        d->flags &= 0x7FFF;
        return 0;
    }
    if (o->owner == 0) {
        c2 = (Child *)o->child;
        if (c2 == 0) {
            return o;
        }
        if (c2->kind == 1) {
            aim = d->aim;
            if (aim != 0x7FFF) {
                c2->aim = aim;
            }
        }
    }
    if ((Child *)o->child != 0) {
        ((Child *)o->child)->x = o->x + o->ox;
        ((Child *)o->child)->y = o->y + o->oy;
        ((Child *)o->child)->z = o->z + o->oz;
    }
    return o;
}
