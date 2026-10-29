/* Owner re-derivation from retail assembly (clean input set). */
/* Model draw: builds the object's local matrix, resolves its world/screen matrices through its parent coordinate, stores them back, then submits every part of its zero-terminated part list. */
#include "psx_types.h"

/* 4.12 rotation plus translation (0x20 bytes). */
typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Mat;

/* Hierarchical coordinate (0x50 bytes): local matrix, resolved world matrix, parent link. */
typedef struct {
    u32 flg;
    Mat coord;
    Mat workm;
    u32 param;
    u32 super; /* guest pointer: parent coordinate */
    u32 sub;
} Coord2;

typedef struct {
    u16 active;
    u16 type;   /* draw-table index (0, 1, 2, 5, 6 land here) */
    u32 attr;   /* bit 0x40: skip the light-matrix load */
    s16 pos[4];
    u16 rot[4];
    s16 scale[4];
    u32 mode;   /* bit 0x1000000: per-part list path */
    u32 parts;  /* guest u32 *: zero-terminated part list */
    u32 clip;   /* passed to func_80021174 with the world translation */
    u16 flags;  /* 1 cached local matrix, 0x10 scale, 0x20 lights, 0x40 matrices only, 0x80 colour cues */
    u16 anim;   /* index into D_800A5BD0 */
    Coord2 c2;
    u32 light;  /* guest pointer: three 0x10-byte light records, then 8-byte colour cues */
} Obj;

/* Part draw request handed to the primitive builders. */
typedef struct {
    u32 attr;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
    u32 unk14;
} DrawArg;

typedef struct {
    u32 unk0;
    u32 unk4;
    u32 unk8;
    u32 unkC;
    u32 unk10;
} AnimRec;

/* Per-buffer draw context (0x14 bytes), indexed by D_800B9A02. */
typedef struct {
    u8 pad[0x14];
} OtRec;

extern u16 D_800B9A02;
extern u32 D_80062BA8;
extern OtRec D_800A6518[];
extern AnimRec D_800A5BD0[];
extern Mat D_80074818[];
extern Mat D_80075018[];

extern void func_80020DA4(u16 *, s16 *);
extern void func_80020F34(Mat *, s16 *);
extern void func_80054AAC(Coord2 *, Mat *, Mat *);
extern s32 func_80021174(u32, s32 *);
extern void func_80021008(u32);
extern s32 func_80020248(u32 *, u32, Mat *, Mat *, u32);
extern void func_80020598(u32 *, u32, Mat *, Mat *);
extern s32 func_80020A28(u32 *, u32, Mat *, Mat *, u32);
extern void func_80056F18(u32, DrawArg *, s32);
extern void func_80052F04(Mat *);
extern void func_80052E38(Mat *);
extern void func_8002109C(u32, u32 *);
/* The retail call sites pass a second argument (the cue pointer) that the body never reads. */
extern void func_80021120(u32 *, u32);
extern void func_80057094(DrawArg *, OtRec *, s32, u32);
extern void func_800243EC(DrawArg *, OtRec *, s32);
extern void func_80027BF0(DrawArg *, OtRec *, s32);
extern void func_80055D40(DrawArg *, OtRec *, s32, u32);
extern void func_800CAE80(DrawArg *, OtRec *, s32);
/* The retail call site passes the light pointer; the body never reads it. */
extern void func_80021050(u32);

void func_8001FC08(Obj *obj)
{
    DrawArg arg;
    Mat lw;
    Mat ls;
    Coord2 coord;
    u32 cue;
    u32 partno;
    u32 attr;
    u32 type;
    u16 flags;
    OtRec *ot;
    u32 otarg;
    u32 light;
    u32 *part;
    u32 mode;
    u32 listmode;
    AnimRec *rec;
    Mat *ma;
    Mat *mb;
    u32 n;
    u32 e;
    u32 kind;

    ot = &D_800A6518[D_800B9A02];
    attr = obj->attr;
    type = obj->type;
    flags = obj->flags;
    coord.super = obj->c2.super;
    otarg = D_80062BA8;
    coord.flg = 0;
    if (flags & 1) {
        obj->flags &= ~1;
        coord.coord = obj->c2.coord;
    } else {
        func_80020DA4(obj->rot, coord.coord.m[0]);
        coord.coord.t[0] = obj->pos[0];
        coord.coord.t[1] = obj->pos[1];
        coord.coord.t[2] = obj->pos[2];
    }
    light = 0;
    if (flags & 0x20) {
        light = obj->light;
    }
    if (flags & 0x80) {
        if (light) {
            cue = obj->light + 0x30;
        } else {
            cue = obj->light;
        }
    } else {
        cue = 0;
    }
    partno = 0;
    n = 0;
    if (flags & 0x10) {
        func_80020F34(&coord.coord, obj->scale);
    }
    func_80054AAC(&coord, &lw, &ls);
    obj->c2.coord = coord.coord;
    obj->c2.workm = coord.workm;
    part = (u32 *)obj->parts;
    if (part == 0) {
        return;
    }
    mode = obj->mode;
    if (mode == 0) {
        return;
    }
    if (func_80021174(obj->clip, coord.workm.t) == 0) {
        return;
    }
    if (type == 1 || type == 6) {
        rec = &D_800A5BD0[obj->anim];
        arg.attr = attr;
        arg.unk10 = 0;
    } else {
        arg.attr = attr;
    }
    if (light) {
        func_80021008(light);
    }
    if (!(mode & 0x1000000) && !(flags & 0x40)) {
        if (func_80020248(part, mode, &lw, &ls, attr & 0x40) == 0) {
            ma = D_80074818;
            mb = D_80075018;
        } else {
            ma = (Mat *)0x1F800000;
            mb = (Mat *)0x1F8000E0;
        }
        while ((e = *part) != 0) {
            kind = e & 0x7F000000;
            e &= 0x80FFFFFF;
            if (kind != 0x1000000) {
                func_80056F18(e + 0xC, &arg, 0);
                if (!(attr & 0x40)) {
                    func_80052F04(ma);
                }
                func_80052E38(mb);
                if (cue) {
                    func_8002109C(partno, &cue);
                }
                if (type == 0) {
                    func_80057094(&arg, ot, 0, otarg);
                } else if (type == 5) {
                    func_800243EC(&arg, ot, 0);
                }
            }
            if (cue) {
                func_80021120(&partno, cue);
            }
            part++;
            ma++;
            mb++;
        }
    } else {
        listmode = mode & ~0x1000000;
        if (flags & 0x40) {
            func_80020598(part, listmode, &lw, &ls);
            goto ram;
        }
        if (func_80020A28(part, listmode, &lw, &ls, attr & 0x40) == 0) {
        ram:
            ma = D_80074818;
            mb = D_80075018;
        } else {
            ma = (Mat *)0x1F800000;
            mb = (Mat *)0x1F8000E0;
        }
        while ((e = *part) != 0) {
            kind = e & 0x7F000000;
            e &= 0x80FFFFFF;
            if (kind == 0x1000000) {
                if (type == 1 || type == 6) {
                    rec++;
                }
            } else {
                if (type == 0 || type == 5) {
                    func_80056F18(e + 0xC, &arg, 0);
                } else if (type == 1 || type == 6) {
                    arg.unk8 = rec->unk8;
                    arg.unkC = rec->unkC;
                    rec++;
                } else {
                    arg.unk8 = e;
                }
                if (!(attr & 0x40)) {
                    func_80052F04(ma);
                }
                func_80052E38(mb);
                if (!(flags & 0x40) && cue) {
                    func_8002109C(partno, &cue);
                }
                if (type == 0) {
                    func_80057094(&arg, ot, 0, otarg);
                } else if (type == 1) {
                    if (n < 2) {
                        func_80027BF0(&arg, ot, 0);
                        n++;
                    } else {
                        func_80055D40(&arg, ot, 0, otarg);
                    }
                } else if (type == 6) {
                    func_800CAE80(&arg, ot, 0);
                } else if (type == 5) {
                    func_800243EC(&arg, ot, 0);
                }
            }
            if (!(flags & 0x40) && cue) {
                func_80021120(&partno, cue);
            }
            part++;
            ma++;
            mb++;
        }
    }
    if (light) {
        func_80021050(light);
    }
}
