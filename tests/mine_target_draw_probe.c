#include "mine_target_draw.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

u8 D_801821E4[20];
u8 D_801821F8[28];
u16 D_800B99DA;

static unsigned callbacks, matrix_calls, perturb;
static TargetPrimitive expected;
static u32 matrix_words[8];
static void *first_primitive, *first_matrix;
static u8 object[16];
static s16 from_point[4], to_point[4];

static u8 next_byte(uint32_t *seed)
{
    *seed = *seed * UINT32_C(1664525) + UINT32_C(1013904223);
    return (u8)(*seed >> 24);
}

static s16 signed_byte(u8 value)
{
    return (s16)(value < 128 ? (int)value : (int)value - 256);
}

void func_80168070(u8 *obj, s16 *from, s16 *to, void *matrix)
{
    unsigned i;
    assert(matrix_calls == 0 && callbacks == 0);
    assert(obj == object && from == from_point && to == to_point);
    assert((uintptr_t)matrix % _Alignof(u32) == 0);
    first_matrix = matrix;
    matrix_calls++;
    if (perturb)
        D_800B99DA ^= 3;
    for (i = 0; i < 8; i++)
        matrix_words[i] = UINT32_C(69069) * (i + D_800B99DA);
    memcpy(matrix, matrix_words, sizeof(matrix_words));
}

static void start_group(unsigned k)
{
    if (k == 0) {
        unsigned red = (D_800B99DA & 1) ? 160 : (D_800B99DA & 2) ? 192 : 128;
        expected.color[1].red = (u8)red;
        expected.color[1].green = expected.color[1].blue = (u8)(red / 4);
        expected.tag = UINT32_C(0x50000000);
    } else if (k == 4) {
        expected.vertex[1].x = expected.vertex[1].y = 0;
        expected.vertex[0].y = expected.vertex[2].y = expected.vertex[3].y = 0;
        expected.vertex[1].z = -32;
    } else if (k == 6) {
        expected.vertex[1].z = 32;
    } else if (k == 8) {
        expected.vertex[0].x = 0;
        expected.vertex[0].z = -32;
        expected.color[0].red = expected.color[1].red;
        expected.color[0].green = expected.color[0].blue = expected.color[1].green;
    }
}

static void set_coordinates(unsigned k)
{
    unsigned i;
    if (k < 8) {
        u8 *points = k < 4 ? D_801821E4 : D_801821F8;
        unsigned offset = k < 4 ? 4 * k : 4 * (k - 4) + (k >= 6 ? 2 : 0);
        for (i = 0; i < 3; i++) {
            unsigned v = i == 0 ? 0 : i + 1;
            expected.vertex[v].x = signed_byte(points[offset + 2 * i]);
            if (k < 4)
                expected.vertex[v].y = signed_byte(points[offset + 2 * i + 1]);
            else
                expected.vertex[v].z = signed_byte(points[offset + 2 * i + 1]);
        }
    } else {
        unsigned offset = 20 + 4 * (k - 8);
        for (i = 0; i < 2; i++) {
            expected.vertex[i + 2].x = signed_byte(D_801821F8[offset + 2 * i]);
            expected.vertex[i + 2].z = signed_byte(D_801821F8[offset + 2 * i + 1]);
        }
    }
}

void func_80017758(void *input, void *matrix)
{
    TargetPrimitive *actual = input;
    unsigned k = callbacks;
    assert(matrix_calls == 1 && callbacks < 10 && matrix == first_matrix);
    if (k == 0)
        first_primitive = actual;
    assert(input == first_primitive && memcmp(matrix, matrix_words, sizeof(matrix_words)) == 0);
    start_group(k);
    set_coordinates(k);
    assert(memcmp(actual, &expected, sizeof(expected)) == 0);
    if (perturb) {
        unsigned offset = (k * 3 + 4) % sizeof(D_801821F8);
        D_801821F8[offset] ^= (u8)(139 + k);
        D_801821E4[(k * 5 + 4) % sizeof(D_801821E4)] ^= (u8)(61 + k);
        D_800B99DA ^= 1;
        actual->color[1].red ^= (u8)(k + 1);
        actual->color[1].green = (u8)(actual->color[1].green + 7);
        expected.color[1] = actual->color[1];
        actual->vertex[1].x = expected.vertex[1].x = (s16)(37 + k);
        actual->vertex[1].z = expected.vertex[1].z = (s16)(-9 - (int)k);
        matrix_words[k % 8] ^= UINT32_C(0x35c9);
        memcpy(matrix, matrix_words, sizeof(matrix_words));
    }
    callbacks++;
}

int main(void)
{
    unsigned i, j;
    uint32_t seed = UINT32_C(170967);
    _Static_assert(sizeof(TargetPrimitive) == 52, "primitive layout");
    _Static_assert(offsetof(TargetPrimitive, color) == 32, "color offset");
    _Static_assert(offsetof(TargetPrimitive, tag) == 48, "tag offset");
    for (i = 0; i < 8192; i++) {
        for (j = 0; j < sizeof(D_801821E4); j++)
            D_801821E4[j] = next_byte(&seed);
        for (j = 0; j < sizeof(D_801821F8); j++)
            D_801821F8[j] = next_byte(&seed);
        memset(&expected, 0, sizeof(expected));
        matrix_calls = callbacks = 0;
        first_primitive = first_matrix = NULL;
        perturb = (i >> 2) & 1;
        D_800B99DA = (u16)(i * 101);
        func_80167AE0(object, from_point, to_point);
        assert(matrix_calls == 1 && callbacks == 10);
    }
    return 0;
}
