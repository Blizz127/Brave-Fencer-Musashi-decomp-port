/* Classify two endpoints against the mine model's translated and scaled frame. */
#include "model_surface_classifier.h"

extern u32 D_8017F68C;
extern u32 D_8017F690;
extern u8 D_8019F9CC[];
extern SceneSurfaceBox D_8019F9EC;

extern void *func_80048EAC(void *matrix, void *result);
extern SceneTransformVector *func_8004D504(void *matrix, SceneTransformVector *input, SceneTransformVector *output);
extern SceneTransformVector *func_80048B6C(SceneTransformVector *input, SceneTransformVector *output);
extern void func_8004914C(void *matrix);
extern SceneTransformVector *func_8004864C(SceneProbeVector *input, SceneTransformVector *output);

typedef union {
    u32 aligned[8];
    u8 bytes[32];
} ClassifierMatrix;

#ifdef __mips__
#define scale_at(record, index) (*(s16 *)((record)->prefix + 24 + 2 * (index)))
#define axis_at(record, index) (*(u32 *)((record)->matrix + 20 + 4 * (index)))
#define word_at(dst, index, value) (*(u32 *)((dst) + 4 * (index)) = (value))
#define half_at(dst, index, value) (*(u16 *)((dst) + 2 * (index)) = (value))
#define signed_word(value) ((s32)(value))
#define signed_half(value) ((s16)(value))
#define near_output ((SceneProbeVector *)D_8017F68C)
#define far_output ((SceneProbeVector *)D_8017F690)
#else
#include <stdint.h>

static int aligned_for(const void *p, unsigned alignment)
{
    return p != 0 && (uintptr_t)p % alignment == 0;
}

static s32 signed_word(u32 bits)
{
    return bits <= 0x7fffffffu ? (s32)bits : -1 - (s32)~bits;
}

static s16 signed_half(u32 bits)
{
    s32 low = (s32)(bits & 65535u);
    return (s16)(low <= 32767 ? low : low - 65536);
}

static s16 scale_at(const SceneSurfaceRecord *record, unsigned index)
{
    const u8 *p = record->prefix + 24 + 2 * index;
    return signed_half((u32)p[0] | (u32)p[1] << 8);
}

static u32 axis_at(const SceneSurfaceRecord *record, unsigned index)
{
    const u8 *p = record->matrix + 20 + 4 * index;
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

static void half_at(u8 *dest, unsigned index, u32 value)
{
    dest[2 * index] = (u8)value;
    dest[2 * index + 1] = (u8)(value >> 8);
}

static void word_at(u8 *dest, unsigned index, u32 value)
{
    half_at(dest, 2 * index, value);
    half_at(dest, 2 * index + 1, value >> 16);
}
#endif

s32 func_80135480(SceneSurfaceRecord *model, u32 flags, SceneProbeVector *near_point,
                  SceneProbeVector *far_point)
{
    SceneTransformVector start, end, transformed;
    ClassifierMatrix scale;
    SceneProbeVector v;
    SceneSurfaceBox *box;
    s16 result;
    s32 mode;
#ifndef __mips__
    SceneProbeVector *near_output;
    SceneProbeVector *far_output;
    if (!model || !aligned_for(near_point, _Alignof(SceneProbeVector))
        || !aligned_for(far_point, _Alignof(SceneProbeVector)))
        return 0;
    /* The original initializes each used field. Host services may copy the
       complete records; zero the remaining local bytes before passing them. */
    start = (SceneTransformVector){0};
    end = (SceneTransformVector){0};
    transformed = (SceneTransformVector){0};
    scale = (ClassifierMatrix){0};
    v = (SceneProbeVector){0};
#endif

    start.x = signed_word((u32)(s32)near_point->x - axis_at(model, 0));
    start.z = signed_word((u32)(s32)near_point->z - axis_at(model, 2));
    if (!(flags & 0x10000000u)
        && signed_word((u32)start.x * (u32)start.x + (u32)start.z * (u32)start.z) > 0x40000)
        return 0;
    mode = flags & 0x60000000u;
    if (mode) {
        result = 1;
        if (signed_word(flags) >= 0)
            mode &= 0x40000000u;
        start.y = signed_word((u32)(s32)near_point->y - axis_at(model, 1));
        end.x = signed_word((u32)(s32)far_point->x - axis_at(model, 0));
        end.y = signed_word((u32)(s32)far_point->y - axis_at(model, 1));
        end.z = signed_word((u32)(s32)far_point->z - axis_at(model, 2));
        switch (mode) {
        case 0x60000000: {
            s32 inverse_y, inverse_z;
#ifndef __mips__
            if (scale_at(model, 0) == 0) {
                bfm_surface_zero_scale(0);
                return 0;
            }
#endif
            word_at(D_8019F9CC, 0, 0x1000000 / scale_at(model, 0));
#ifndef __mips__
            if (scale_at(model, 1) == 0) {
                bfm_surface_zero_scale(1);
                return 0;
            }
            if (scale_at(model, 2) == 0) {
                bfm_surface_zero_scale(2);
                return 0;
            }
#endif
            inverse_y = 0x1000000 / scale_at(model, 1);
            inverse_z = 0x1000000 / scale_at(model, 2);
            word_at(D_8019F9CC, 1, 0);
            word_at(D_8019F9CC, 3, 0);
            word_at(D_8019F9CC, 2, inverse_y);
            half_at(D_8019F9CC, 8, inverse_z);
            func_80048EAC(model->matrix, D_8019F9CC);
            func_8004D504(D_8019F9CC, &start, &start);
            func_80048B6C(&end, &end);
            result = 2;
        }
        /* Fall through. */
        case 0x20000000: {
            s32 sx = scale_at(model, 0);
            s32 sy = scale_at(model, 1);
            u16 sz = (u16)scale_at(model, 2);
#ifdef __mips__
            box = (SceneSurfaceBox *)((flags & 0x0fffffffu) | 0x80000000u);
#else
            box = bfm_model_probe_box((flags & 0x0fffffffu) | 0x80000000u);
            if (!aligned_for(box, _Alignof(SceneSurfaceBox)))
                return 0;
#endif
            word_at(scale.bytes, 0, sx);
            word_at(scale.bytes, 1, 0);
            word_at(scale.bytes, 2, sy);
            word_at(scale.bytes, 3, 0);
            half_at(scale.bytes, 8, sz);
            func_8004914C(scale.bytes);
            v.x = box->bounds[0];
            v.y = box->bounds[2];
            v.z = box->bounds[4];
            func_8004864C(&v, &transformed);
            D_8019F9EC.bounds[0] = signed_half(transformed.x);
            D_8019F9EC.bounds[2] = signed_half(transformed.y);
            D_8019F9EC.bounds[4] = signed_half(transformed.z);
            v.x = box->bounds[1];
            v.y = box->bounds[3];
            v.z = box->bounds[5];
            func_8004864C(&v, &transformed);
            D_8019F9EC.next = 0;
            D_8019F9EC.bounds[1] = signed_half(transformed.x);
            D_8019F9EC.bounds[3] = signed_half(transformed.y);
            D_8019F9EC.bounds[5] = signed_half(transformed.z);
            result += 2;
            break;
        }
        case 0x40000000:
            func_8004D504(model->matrix, &start, &start);
            func_80048B6C(&end, &end);
            result = 2;
            break;
        }
#ifndef __mips__
        near_output = bfm_model_probe_point(D_8017F68C);
        far_output = bfm_model_probe_point(D_8017F690);
        if (!aligned_for(near_output, _Alignof(SceneProbeVector))
            || !aligned_for(far_output, _Alignof(SceneProbeVector)))
            return 0;
#endif
        near_output->x = signed_half(start.x);
        near_output->y = signed_half(start.y);
        near_output->z = signed_half(start.z);
        far_output->x = signed_half(end.x);
        far_output->y = signed_half(end.y);
        far_output->z = signed_half(end.z);
        return result;
    }
#ifndef __mips__
    near_output = bfm_model_probe_point(D_8017F68C);
    far_output = bfm_model_probe_point(D_8017F690);
    if (!aligned_for(near_output, _Alignof(SceneProbeVector))
        || !aligned_for(far_output, _Alignof(SceneProbeVector)))
        return 0;
#endif
    near_output->x = signed_half(start.x);
    near_output->y = signed_half((u32)(s32)near_point->y - axis_at(model, 1));
    near_output->z = signed_half(start.z);
    far_output->x = signed_half((u32)(s32)far_point->x - axis_at(model, 0));
    far_output->y = signed_half((u32)(s32)far_point->y - axis_at(model, 1));
    far_output->z = signed_half((u32)(s32)far_point->z - axis_at(model, 2));
    return 1;
}
