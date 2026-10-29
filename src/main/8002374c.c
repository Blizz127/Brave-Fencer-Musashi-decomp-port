/* Owner re-derivation from retail assembly (clean input set). */
/* Draws an object's flat-shape list (0x1C-byte records ending with a zero header): TILE (0x60),
 * gouraud quad (0x38) and gouraud triangle (0x30), each wrapped between two draw-mode packets in one
 * linked packet, offset by the object position. */
#include "psx_types.h"

/* Packed screen point, handled as one word. */
typedef struct {
    s32 x : 16;
    s32 y : 16;
} Pt;

/* Width/height pair, read as halves or copied as one word. */
typedef union {
    u32 word;
    struct {
        u16 w;
        u16 h;
    } s;
} Size;

/* One shape record (0x1C bytes). header: low byte primitive code, bit 23 dither,
 * bits 28-29 blend, bit 30 semi-transparent; zero ends the list. */
typedef struct {
    u32 header;
    union {
        struct { Pt xy; Size wh; u32 rgb; } tile;                  /* 0x60 */
        struct { Pt xy; Size wh; u32 rgb[4]; } quad;               /* 0x38 */
        struct { Pt xy[3]; u32 rgb[3]; } tri;                      /* 0x30 */
    } u;
} ShapeRec;

typedef struct {
    u8 pad0[4];
    s32 flags;     /* 0x04: negative = hidden */
    u16 x;         /* 0x08 */
    u16 y;         /* 0x0A */
    u8 padC[0x20 - 0x0C];
    u32 shapes;    /* 0x20: guest ShapeRec * */
    u8 pad24[0x2C - 0x24];
    u16 depth;     /* 0x2C: ordering-table slot */
    u16 dx;        /* 0x2E */
    u16 dy;        /* 0x30 */
} ShapeObj;

/* libgpu DR_MODE layout. */
typedef struct {
    u32 tag;
    u32 code[2];
} DrMode;

extern u32 D_800A5E60; /* guest primitive-buffer cursor */
extern u32 D_800A6610[][4096];
extern u16 D_800B9A02;
extern u16 func_80058B40(s32 tp, s32 abr, s32 x, s32 y); /* GetTPage-shaped */
extern void func_8005A600(void *p, s32 dfe, s32 dtd, s32 tpage, void *tw);

void func_8002374C(ShapeObj *o)
{
    ShapeRec *rec;
    u16 ox;
    u16 oy;
    u32 *ot;
    u16 depth;
    u32 code;
    u32 flags;
    u32 *p;
    Pt v;

    if (o->flags < 0) {
        return;
    }
    rec = (ShapeRec *)o->shapes;
    ox = o->x - o->dx;
    oy = o->y - o->dy;
    ot = D_800A6610[D_800B9A02];
    depth = o->depth;
    for (; (code = flags = rec->header) != 0; rec++) {
        code <<= 24;
        if (flags & 0x40000000) {
            code |= 0x02000000;
        }
        p = (u32 *)D_800A5E60;
        if (flags & 0x800000) {
            func_8005A600(p, 0, 1, func_80058B40(2, (flags >> 28) & 3, 640, 0), 0);
        } else {
            func_8005A600(p, 0, 0, func_80058B40(2, (flags >> 28) & 3, 640, 0), 0);
        }
        /* p walks the packet: tag, two mode words, primitive, then a second mode packet at the end */
        switch (flags & 0xFF) {
        case 0x60:
            D_800A5E60 += 0x20;
            func_8005A600((DrMode *)D_800A5E60 - 1, 0, 0, func_80058B40(2, (flags >> 28) & 3, 640, 0), 0);
            *p++ = 0x07000000;
            p++;
            p[1] = code | (rec->u.tile.rgb & 0xFFFFFF);
            v = rec->u.tile.xy;
            v.x = ox + v.x;
            v.y = oy + v.y;
            *(Pt *)&p[2] = v;
            p[3] = rec->u.tile.wh.word;
            break;
        case 0x38:
            D_800A5E60 += 0x34;
            func_8005A600((DrMode *)D_800A5E60 - 1, 0, 0, func_80058B40(2, (flags >> 28) & 3, 640, 0), 0);
            *p++ = 0x0C000000;
            p++;
            p[1] = code | (rec->u.quad.rgb[0] & 0xFFFFFF);
            p[3] = rec->u.quad.rgb[1];
            p[5] = rec->u.quad.rgb[2];
            p[7] = rec->u.quad.rgb[3];
            v = rec->u.quad.xy;
            v.x = ox + v.x;
            v.y = oy + v.y;
            *(Pt *)&p[2] = v;
            v.x += rec->u.quad.wh.s.w;
            *(Pt *)&p[4] = v;
            v.y += rec->u.quad.wh.s.h;
            *(Pt *)&p[8] = v;
            v.x -= rec->u.quad.wh.s.w;
            *(Pt *)&p[6] = v;
            break;
        case 0x30:
            D_800A5E60 += 0x2C;
            func_8005A600((DrMode *)D_800A5E60 - 1, 0, 0, func_80058B40(2, (flags >> 28) & 3, 640, 0), 0);
            *p++ = 0x0A000000;
            p++;
            p[1] = code | (rec->u.tri.rgb[0] & 0xFFFFFF);
            p[3] = rec->u.tri.rgb[1];
            p[5] = rec->u.tri.rgb[2];
            v = rec->u.tri.xy[0];
            v.x = ox + v.x;
            v.y = oy + v.y;
            *(Pt *)&p[2] = v;
            v = rec->u.tri.xy[1];
            v.x = ox + v.x;
            v.y = oy + v.y;
            *(Pt *)&p[4] = v;
            v = rec->u.tri.xy[2];
            v.x = ox + v.x;
            v.y = oy + v.y;
            *(Pt *)&p[6] = v;
            break;
        }
        /* an unknown code links p - 2 without having built a packet, as retail does */
        p[-2] = (p[-2] & 0xFF000000) | (ot[depth] & 0xFFFFFF);
        ot[depth] = (ot[depth] & 0xFF000000) | ((u32)(p - 2) & 0xFFFFFF);
    }
}
