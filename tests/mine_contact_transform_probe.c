#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "model_surface_probe.h"

extern void func_80136A94(s32, SceneSurfaceRecord *, SceneProbeVector *, void *);
SceneProbeVector D_801152A8, D_80126720;
u32 D_8017F694;
static SceneSurfaceRecord object;
static SceneProbeVector point[2], out;
static u32 keys[2], offsets[3], normal_out[3], rotate, orientation;
static unsigned resolve_calls, matrix_calls, rotate_calls;
static int swap_after_transform;

static SceneProbeVector simulated(SceneProbeVector v, unsigned call)
{
    v.x = (s16)((u16)v.x + (u16)orientation + 11u*call);
    v.y = (s16)((u16)v.y ^ (u16)(orientation >> (call + 3)));
    v.z = (s16)((u16)v.z - (u16)(orientation >> (2u*call + 1u)));
    return v;
}
static SceneProbeVector translated(SceneProbeVector v)
{
    v.x = (s16)((u16)v.x + offsets[0]);
    v.y = (s16)((u16)v.y + offsets[1]);
    v.z = (s16)((u16)v.z + offsets[2]);
    return v;
}
SceneProbeVector *bfm_model_probe_point(u32 address)
{
    ++resolve_calls;
    for (unsigned j = 0; j < 2; ++j)
        if (address == keys[j]) return &point[j];
    return 0;
}
SceneProbeVector *func_80048FBC(void *matrix, SceneProbeVector *in, SceneProbeVector *result)
{
    assert(matrix == object.matrix && in == result && rotate != 0 && matrix_calls < 2);
    assert(in == (matrix_calls == 0 ? &point[0] : &D_80126720));
    *result = simulated(*in, matrix_calls++);
    return result;
}
SceneTransformVector *func_8004864C(SceneProbeVector *in, SceneTransformVector *result)
{
    assert(in == &D_801152A8 && matrix_calls == 2 && rotate_calls++ == 0);
    result->x=(s32)normal_out[0];result->y=(s32)normal_out[1];result->z=(s32)normal_out[2];
    result->pad=(s32)(orientation ^ 0xf729u);
    if (swap_after_transform) D_8017F694 = keys[1];
    return result;
}
static u32 next(u32 *seed) { *seed = *seed * 1664525u + 1013904223u; return *seed; }
static SceneProbeVector generated(u32 *seed)
{
    SceneProbeVector v;
    v.x = (s16)next(seed); v.y = (s16)(next(seed) >> 4);
    v.z = (s16)(next(seed) >> 7); v.pad = (s16)(next(seed) >> 11);
    return v;
}
static void compare(const SceneProbeVector *actual, const SceneProbeVector *expected)
{
    assert(actual->x == expected->x && actual->y == expected->y && actual->z == expected->z);
    assert(actual->pad == expected->pad);
}
int main(void)
{
    u32 seed = 267;
    for (unsigned run = 0; run < 8192; ++run) {
        point[0] = generated(&seed); point[1] = generated(&seed); out = generated(&seed);
        D_80126720 = generated(&seed); D_801152A8 = generated(&seed);
        for (unsigned i=0;i<3;i++) {
            offsets[i] = next(&seed);
            normal_out[i] = next(&seed);
            for(unsigned j=0;j<4;j++) object.matrix[20+4*i+j]=(u8)(offsets[i] >> (8u*j));
        }
        keys[0]=next(&seed)|1u; keys[1]=next(&seed)&~1u; D_8017F694=keys[0];
        orientation=next(&seed); rotate=run&1?next(&seed)|1u:0; swap_after_transform=(run>>1)&1;
        resolve_calls=matrix_calls=rotate_calls=0;
        SceneProbeVector p0=point[0], p1=point[1], a=D_80126720, n=D_801152A8, expected=out;
        int invalid=(run%101)==0;
        if(invalid) {do { D_8017F694=next(&seed); }while(D_8017F694==keys[0]||D_8017F694==keys[1]);}
        if(!invalid){
            if(rotate){
                p0=simulated(p0,0); a=simulated(a,1);
                n.x=(s16)normal_out[0];n.y=(s16)normal_out[1];n.z=(s16)normal_out[2];
            }
            s16 pad=expected.pad;expected=translated(rotate&&swap_after_transform?p1:p0);expected.pad=pad;
            a=translated(a);
        }
        func_80136A94((s32)rotate,&object,&out,object.matrix);
        compare(&point[0],&p0);compare(&point[1],&p1);compare(&D_80126720,&a);compare(&D_801152A8,&n);compare(&out,&expected);
        assert(matrix_calls==(rotate&&!invalid?2u:0u)&&rotate_calls==(rotate&&!invalid?1u:0u));
        assert(resolve_calls==(rotate&&!invalid?2u:1u));
    }
}
