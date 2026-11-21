#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "forest_floor.h"

FloorPoint *D_801870AC, *D_801870B0, *D_801870B4, *D_801870B8;
FloorPoint D_801152A8;
FloorGrid *D_801D94F0;
u16 D_801D9500;

static FloorPoint probes[3], vertices[2];
static s32 distances[5];
static u16 cells[2], ids[2];
static FloorGrid grid;
static FloorFace *short_face, *long_face;
static FloorPoint movement;
static unsigned callback_calls;

void func_80047E98(FloorPoint *input, FloorPoint *output)
{
    /* Stand-in for the external VectorNormalSS service. */
    assert(input == &movement && output == input);
    assert(input->x == 0 && input->y == 768 && input->z == 0);
    assert(probes[2].x == 11 && probes[2].y == 100 && probes[2].z == 21);
    assert(D_801152A8.y == (u16)-100 && D_801152A8.w == 17);
    *output = (FloorPoint){2048, (u16)-4096, 4096, 0};
    callback_calls++;
}

static void start_contact(void)
{
    probes[0] = (FloorPoint){11, 6, 21, 4};
    probes[1] = (FloorPoint){11, 774, 21, 4};
    movement = (FloorPoint){0, 768, 0, 0};
    D_801870B8 = &movement;
    callback_calls = 0;
}

static void set_face(FloorFace *face, unsigned count, u16 flags)
{
    face->flags = flags;
    face->e[0] = (FloorPlane){0, 0};
    for (unsigned i = 1; i < count; ++i)
        face->e[i] = (FloorPlane){1, (s16)i};
}

static void reset(s16 normal_y)
{
    D_801870AC = &probes[0];
    D_801870B0 = &probes[1];
    D_801870B4 = &probes[2];
    for (unsigned i = 0; i < 3; ++i)
        probes[i] = (FloorPoint){1, 2, 3, 4};
    D_801152A8 = (FloorPoint){6, 7, 8, 9};
    D_801D9500 = 0xc4;
    vertices[0] = (FloorPoint){0, (u16)normal_y, 0, 17};
    vertices[1] = (FloorPoint){0, 0, 0, 0};
    distances[0] = -100 * normal_y;
    for (unsigned i = 1; i < 5; ++i) distances[i] = -0x2f00;
    cells[0] = 0;
    cells[1] = 2;
    ids[0] = 0;
    ids[1] = 0x8000;
    set_face(short_face, 4, 0x11);
    set_face(long_face, 5, 0x22);
    grid = (FloorGrid){256, 256, 1, 1, cells, distances, vertices, (u8 *)ids,
                       (u8 *)short_face, (u8 *)long_face};
    D_801D94F0 = &grid;
}

static void hit_and_selection(void)
{
    const s16 normals[] = {-3020, -3019, -1401, -1400, -1};
    const u16 classes[] = {0x2000, 0x4000, 0x4000, 0x8000, 0x8000};
    for (unsigned i = 0; i < sizeof normals / sizeof normals[0]; ++i) {
        FloorPoint point = {11, 10, 21, 99};
        reset(normals[i]);
        assert(func_80134510(&point) == classes[i]);
        assert(point.x == 11 && point.y == 98 && point.z == 21 && point.w == 0);
        assert(probes[0].y == 6 && probes[1].y == 774 && probes[2].y == 100);
        for (unsigned j = 0; j < 3; ++j) assert(probes[j].x == 11 && probes[j].z == 21);
        assert(probes[0].w == 0x22 && D_801D9500 == 0x22);
        assert(D_801152A8.y == (u16)normals[i] && D_801152A8.w == 17);
    }
    FloorPoint point = {11, 10, 21, 99};
    reset(-1400);
    distances[4]--;  /* The long face fails its extra plane; the short face wins. */
    assert(func_80134510(&point) == 0x8000 && point.y == 98);
    assert(D_801D9500 == 0x11 && probes[0].w == 0x11);
}

static void misses(void)
{
    for (unsigned i = 0; i < 9; ++i) {
        FloorPoint point = {11, 10, 21, 99};
        reset(-1400);
        switch (i) {
        case 0: grid.x0++; break;
        case 1: grid.z0++; break;
        case 2: cells[1] = 0; break;
        case 3: distances[0] = 1000 * 1400; break;
        case 4: distances[0] = 0; break;
        case 5: case 6: case 7: distances[i - 4]--; break;
        case 8: short_face->flags |= 0x100; long_face->flags |= 0x200; break;
        }
        assert(func_80134510(&point) == 0);
        assert(point.y == 10 && point.w == 0);
        assert(probes[0].w == 4 && D_801D9500 == 0xc4 && D_801152A8.w == 9);
        if (i >= 5) {
            assert(probes[2].x == 11 && probes[2].y == 100 && probes[2].z == 21);
        } else {
            assert(probes[2].x == 1 && probes[2].y == 2 && probes[2].z == 3);
        }
    }
}

static void moving_contact(void)
{
    for (unsigned i = 0; i < 9; ++i) {
        s32 flags = 1;
        reset(-100);
        start_contact();
        switch (i) {
        case 0: break; /* The last, five-plane face succeeds. */
        case 1: distances[4]--; break;
        case 2: short_face->flags |= 0x100; long_face->flags |= 0x100; break;
        case 3: flags = 0; short_face->flags |= 0x200; long_face->flags |= 0x200; break;
        case 4: flags = 0; short_face->flags |= 0x100; long_face->flags |= 0x100; break;
        case 5: grid.x0++; break;
        case 6: cells[1] = 0; break;
        case 7: distances[0] = 0; break;
        case 8: distances[1]--; break;
        }
        int hit = func_80134A74(flags, 11, 21, &grid);
        if (i == 0 || i == 1 || i == 4) {
            assert(hit && callback_calls == 1);
            assert(D_801D9500 == (i == 1 ? 0x11 : i == 4 ? 0x122 : 0x22));
            assert(probes[2].x == 10 && probes[2].y == 102 && probes[2].z == 19);
        } else {
            assert(!hit && !callback_calls && D_801152A8.w == 9);
            assert(D_801D9500 == (i == 3 ? 0x211 : 0xc4));
            if (i == 2 || i == 3 || i == 8) {
                assert(probes[2].x == 11 && probes[2].y == 100 && probes[2].z == 21);
            } else {
                assert(probes[2].x == 1 && probes[2].y == 2 && probes[2].z == 3);
            }
        }
    }
}

int main(void)
{
    assert(sizeof(FloorPoint) == 8 && sizeof(FloorPlane) == 4);
    assert(offsetof(FloorFace, e) == 2);
    short_face = calloc(1, sizeof(*short_face) + 4 * sizeof(FloorPlane));
    long_face = calloc(1, sizeof(*long_face) + 5 * sizeof(FloorPlane));
    assert(short_face && long_face);
    hit_and_selection();
    misses();
    moving_contact();
    FloorPoint negative = {0x8000, 0x8000, 0x8000, 0};
    assert(func_80134A28(&negative, &negative, INT32_MAX) == 0x3fffffff);
    free(long_face);
    free(short_face);
}
