/* Owner re-derivation from retail assembly (clean input set). */
/* Projects a target object's offset through the host object's node: func_8012F214 writes a vector
 * (latched in D_8018536C), a rotation built from the target's fields at 8/0xA/0xC is applied and a
 * second vector from the rotated offset is latched in D_80185374. */
#include "psx_types.h"

/* Short vector (3 shorts + pad) and the 32-byte rotation/translation matrix. */
typedef struct {
    s16 x;
    s16 y;
    s16 z;
    s16 pad;
} SV;

typedef struct {
    s16 m[3][3];
    s32 t[3];
} Mat;

/* Byte-aligned 8-byte record (copied with unaligned word moves). */
typedef struct {
    u8 b[8];
} Rec8;

/* Target object fields used here. */
typedef struct {
    u8 pad00[8];
    u16 f8;
    u16 fA;
    u16 fC;
} Obj;

extern Rec8 D_8018536C;
extern Rec8 D_80185374;
extern void func_8012F214(void *host, void *target, SV *out);
extern void func_80049CAC(SV *r, Mat *m);
extern void func_8004914C(Mat *m);
extern void func_800491AC(Mat *m);
extern void func_8004945C(SV *a, SV *b, s32 *flag);

void func_8017BA3C(void *host, Obj *o)
{
    SV r;
    Mat m;
    SV v38;
    SV v40;
    SV v48;
    s32 flag;

    func_8012F214(host, o, &v40);
    D_8018536C = *(Rec8 *)&v40;
    r.x = -o->fC;
    r.y = o->f8;
    r.z = 0;
    v38.x = 0;
    v38.y = 0;
    v38.z = -o->fA;
    func_80049CAC(&r, &m);
    m.t[0] = v40.x;
    m.t[1] = v40.y;
    m.t[2] = v40.z;
    func_8004914C(&m);
    func_800491AC(&m);
    func_8004945C(&v38, &v48, &flag);
    D_80185374 = *(Rec8 *)&v48;
}
