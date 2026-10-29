/* Owner re-derivation from retail assembly (clean input set). */
/* Puts two points into an object's local frame (plain, rotated, or inverse-scaled and rotated), optionally scales a box; returns the kind 0..4. */
#include "psx_types.h"

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} Vec;

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} SVec;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Mat;

typedef struct {
    u8 pad0[0x18];
    s16 sx;          /* per-axis scale, 0x1000 = 1.0 */
    s16 sy;
    s16 sz;
    u8 pad1E[0x16];
    Mat mat;         /* 0x34: rotation, translation at 0x48 */
} Obj;

typedef struct {
    s32 hdr;
    s16 x0;
    s16 x1;
    s16 y0;
    s16 y1;
    s16 z0;
    s16 z1;
} Box;

extern u32 D_8017E910;   /* guest SVec *: first point out */
extern u32 D_8017E914;   /* guest SVec *: second point out */
extern Mat D_80184134;   /* inverse-scale matrix, then combined with the object's */
extern Box D_80184154;   /* scaled copy of the flagged box */
extern void func_80048EAC(Mat *m0, Mat *m1);
extern void func_8004D504(Mat *m, Vec *in, Vec *out);
extern void func_80048B6C(Vec *in, Vec *out);
extern void func_8004914C(Mat *m);
extern void func_8004864C(SVec *in, Vec *out);

s16 func_80135480(Obj *obj, s32 flags, SVec *p1, SVec *p2)
{
    Vec a;
    Vec b;
    Vec r;
    Mat m;
    SVec v;
    u32 box;
    s32 mode;
    s32 ret;

    a.vx = p1->vx - obj->mat.t[0];
    a.vz = p1->vz - obj->mat.t[2];
    if (!(flags & 0x10000000) && a.vx * a.vx + a.vz * a.vz > 0x40000) {
        return 0;
    }
    mode = flags & 0x60000000;
    if (mode != 0) {
        ret = 1;
        /* bit 31 says the low 28 bits address a box; without it only 0x40000000 counts */
        if (flags >= 0) {
            mode &= 0x40000000;
        }
        a.vy = p1->vy - obj->mat.t[1];
        b.vx = p2->vx - obj->mat.t[0];
        b.vy = p2->vy - obj->mat.t[1];
        b.vz = p2->vz - obj->mat.t[2];
        switch (mode) {
        case 0x60000000:
            /* diagonal words: each store also clears the off-diagonal entry beside it */
            *(s32 *)&D_80184134.m[0][0] = 0x1000000 / obj->sx;
            *(s32 *)&D_80184134.m[0][2] = 0;
            *(s32 *)&D_80184134.m[1][1] = 0x1000000 / obj->sy;
            *(s32 *)&D_80184134.m[2][0] = 0;
            D_80184134.m[2][2] = 0x1000000 / obj->sz;
            func_80048EAC(&obj->mat, &D_80184134);
            func_8004D504(&D_80184134, &a, &a);
            func_80048B6C(&b, &b);
            ret = 2;
            /* fall through */
        case 0x20000000:
            box = (flags & 0x0FFFFFFF) | 0x80000000;
            *(s32 *)&m.m[0][0] = obj->sx;
            *(s32 *)&m.m[0][2] = 0;
            *(s32 *)&m.m[1][1] = obj->sy;
            *(s32 *)&m.m[2][0] = 0;
            m.m[2][2] = obj->sz;
            func_8004914C(&m);
            v.vx = ((Box *)box)->x0;
            v.vy = ((Box *)box)->y0;
            v.vz = ((Box *)box)->z0;
            func_8004864C(&v, &r);
            D_80184154.x0 = r.vx;
            D_80184154.y0 = r.vy;
            D_80184154.z0 = r.vz;
            v.vx = ((Box *)box)->x1;
            v.vy = ((Box *)box)->y1;
            v.vz = ((Box *)box)->z1;
            func_8004864C(&v, &r);
            D_80184154.hdr = 0;
            D_80184154.x1 = r.vx;
            D_80184154.y1 = r.vy;
            D_80184154.z1 = r.vz;
            ret += 2;
            break;
        case 0x40000000:
            func_8004D504(&obj->mat, &a, &a);
            func_80048B6C(&b, &b);
            ret = 2;
            break;
        }
        ((SVec *)D_8017E910)->vx = a.vx;
        ((SVec *)D_8017E910)->vy = a.vy;
        ((SVec *)D_8017E910)->vz = a.vz;
        ((SVec *)D_8017E914)->vx = b.vx;
        ((SVec *)D_8017E914)->vy = b.vy;
        ((SVec *)D_8017E914)->vz = b.vz;
        return ret;
    }
    ((SVec *)D_8017E910)->vx = a.vx;
    ((SVec *)D_8017E910)->vy = p1->vy - obj->mat.t[1];
    ((SVec *)D_8017E910)->vz = a.vz;
    ((SVec *)D_8017E914)->vx = p2->vx - obj->mat.t[0];
    ((SVec *)D_8017E914)->vy = p2->vy - obj->mat.t[1];
    ((SVec *)D_8017E914)->vz = p2->vz - obj->mat.t[2];
    return 1;
}
