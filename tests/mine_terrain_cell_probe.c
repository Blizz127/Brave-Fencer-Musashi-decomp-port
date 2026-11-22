#include "mine_terrain_cell.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum {
    PREVIOUS = 8, CURRENT = 16, MOVEMENT = 24, CELLS = 64, TOKENS = 128,
    TRIANGLES = 192, QUADS = 256, PLANES = 384, DISTANCES = 448, OVERLAP = 512
};
u32 D_8017F68C = PREVIOUS, D_8017F690 = CURRENT, D_8017F698 = MOVEMENT;
s16 D_8019F9C4;
u16 D_8019F9C8;

static SceneProbeVector previous, current, movement;
static u16 cells[12][2], tokens[8];
static s16 triangles[3][9], quads[3][11];
static FloorPlane planes[2];
static s32 distances[2];
static unsigned calls, expected_calls;
static s16 mode;
static u16 callback_material;
static unsigned char *overlap;

struct Response {
    u16 token;
    s16 result, stop;
    u16 write_material, publish;
};
static struct Response responses[4];

void *bfm_floor_address(u32 address)
{
    switch (address) {
    case PREVIOUS: return &previous;
    case CURRENT: return &current;
    case MOVEMENT: return &movement;
    case OVERLAP: return overlap;
    case OVERLAP + sizeof(s16): return overlap + sizeof(s16);
    case PLANES: return planes;
    case DISTANCES: return distances;
    }
    if (address >= CELLS && address < CELLS + sizeof(cells)) {
        assert((address - CELLS) % sizeof(cells[0]) == 0);
        return cells[(address - CELLS) / sizeof(cells[0])];
    }
    if (address >= TOKENS && address < TOKENS + sizeof(tokens)) {
        assert((address - TOKENS) % sizeof(tokens[0]) == 0);
        return &tokens[(address - TOKENS) / sizeof(tokens[0])];
    }
    if (address >= TRIANGLES && address < TRIANGLES + sizeof(triangles)) {
        assert((address - TRIANGLES) % sizeof(triangles[0]) == 0);
        return triangles[(address - TRIANGLES) / sizeof(triangles[0])];
    }
    if (address >= QUADS && address < QUADS + sizeof(quads)) {
        assert((address - QUADS) % sizeof(quads[0]) == 0);
        return quads[(address - QUADS) / sizeof(quads[0])];
    }
    abort();
}

s16 func_80133CD4(s16 flags, s16 *face, FloorPlane *plane_arg, s32 *distance_arg)
{
    assert(calls < expected_calls);
    struct Response response = responses[calls++];
    unsigned index = response.token & 0x7fff;
    assert((u16)flags == (u16)((response.token & 0x8000) | mode));
    assert(index < 3);
    assert(face == ((response.token & 0x8000) ? quads[index] : triangles[index]));
    assert(face[0] == (s16)(index + ((response.token & 0x8000) ? 90 : 50)));
    assert(plane_arg == planes && distance_arg == distances);
    assert(D_8019F9C8 == callback_material);
    if (response.publish) D_8019F9C8 = callback_material = response.publish;
    if (response.write_material) face[0] = (s16)response.write_material;
    D_8019F9C4 = response.stop;
    return response.result;
}

static FloorCellMap reset(void)
{
    FloorCellMap map = {255, 255, 3, 2, CELLS, DISTANCES, PLANES, TOKENS, TRIANGLES, QUADS};
    previous = (SceneProbeVector){32760, -32760, 40, 13};
    current = (SceneProbeVector){-32760, 32766, 31, 17};
    movement = (SceneProbeVector){-2, 5, 22, 23};
    D_8017F68C = PREVIOUS;
    D_8017F698 = MOVEMENT;
    D_8019F9C8 = callback_material = 71;
    D_8019F9C4 = 0;
    mode = 0x240;
    calls = expected_calls = 0;
    memset(cells, 0, sizeof(cells));
    memset(tokens, 0, sizeof(tokens));
    memset(responses, 0, sizeof(responses));
    for (unsigned i = 0; i < 3; ++i) {
        triangles[i][0] = (s16)(50 + i);
        quads[i][0] = (s16)(90 + i);
    }
    return map;
}

static void check_faces(s16 final_result, s16 stop, unsigned visit_count, u16 written,
                        int expected_result, u16 expected_material)
{
    FloorCellMap map = reset();
    FloorCellMap before = map;
    cells[4][0] = 2 * sizeof(u16);
    cells[4][1] = 3;
    tokens[2] = responses[0].token = 0x8001;
    tokens[3] = responses[1].token = 2;
    tokens[4] = responses[2].token = 0x8000;
    responses[0].publish = 0x1357;
    expected_calls = visit_count;
    responses[visit_count-1].result = final_result;
    responses[visit_count-1].stop = stop;
    responses[visit_count-1].write_material = written;

    assert(func_80133AB0(mode, 0, 127, &map) == expected_result);
    assert(calls == visit_count && D_8019F9C8 == expected_material && D_8019F9C4 == stop);
    assert(memcmp(&map, &before, sizeof(map)) == 0);
    assert(movement.x == 16 && movement.y == -10 && movement.z == -9 && movement.pad == 23);
    assert(previous.x == 32760 && previous.y == -32760 && previous.z == 40 && previous.pad == 13);
    assert(current.x == -32760 && current.y == 32766 && current.z == 31 && current.pad == 17);
    assert(cells[4][0] == 4 && cells[4][1] == 3);
    assert(tokens[2] == 0x8001 && tokens[3] == 2 && tokens[4] == 0x8000);
}

static void one_cell(FloorCellMap *map, u16 cell, s16 x, s16 z)
{
    cells[cell][0] = 2;
    cells[cell][1] = 1;
    tokens[1] = responses[0].token = 1;
    expected_calls = 1;
    responses[0].result = 1;
    mode = (s16)0x8002;
    assert(func_80133AB0(mode, x, z, map) == 1);
    assert(calls == 1 && D_8019F9C8 == 51);
}

static void check_cells(void)
{
    FloorCellMap map = reset();
    /* The source narrows the doubled linear index to 16 bits before doubling again. */
    map.x0 = map.z0 = 0;
    map.w = 256; map.h = 129;
    one_cell(&map, 2, -32512, -16384);

    map = reset();
    map.x0 = map.z0 = 65535;
    map.w = map.h = 2;
    one_cell(&map, 3, -32768, -32768);

    map = reset();
    map.x0 = map.z0 = 511;
    map.w = map.h = 1;
    one_cell(&map, 0, 32767, 32767);

    map = reset();
    map.x0 = map.z0 = 256;
    map.w = map.h = 1;
    assert(func_80133AB0(0, -1, 0, &map) == 0);
    assert(func_80133AB0(0, 0, -1, &map) == 0);
    assert(func_80133AB0(0, 128, 0, &map) == 0);
    assert(func_80133AB0(0, 0, 128, &map) == 0);
    assert(func_80133AB0(0, 127, 127, &map) == 0);
    assert(calls == 0 && D_8019F9C8 == 71 && movement.pad == 23);
}

static void check_overlapping_points(void)
{
    FloorCellMap map = reset();
    map.w = 0;
    D_8017F698 = PREVIOUS;
    assert(func_80133AB0(0, 0, 0, &map) == 0);
    assert(previous.x == 16 && previous.y == -10 && previous.z == -9 && previous.pad == 13);
    assert(movement.x == -2 && movement.pad == 23);

    map = reset();
    map.h = 0;
    D_8017F698 = CURRENT;
    assert(func_80133AB0(0, 0, 0, &map) == 0);
    assert(current.x == 16 && current.y == -10 && current.z == -9 && current.pad == 17);
    assert(previous.x == 32760 && previous.pad == 13);

    /* Each component is read again after the preceding destination write. */
    const s16 initial[] = {10, 40, 90, 222, 44};
    const s16 expected[] = {10, 7, 55, 96, 44};
    overlap = malloc(sizeof(initial));
    assert(overlap != NULL);
    for (size_t i = 0; i < sizeof(initial) / sizeof(initial[0]); ++i)
        memcpy(overlap + i * sizeof(s16), &initial[i], sizeof(s16));
    current = (SceneProbeVector){17, 62, 151, 17};
    D_8017F68C = OVERLAP;
    D_8017F698 = OVERLAP + sizeof(s16);
    assert(func_80133AB0(0, 0, 0, &map) == 0);
    assert(memcmp(overlap, expected, sizeof(expected)) == 0);
    free(overlap);
    overlap = NULL;
}

int main(void)
{
    check_faces(0, 0, 3, 0, 0, 0x1357);
    check_faces(7, 0, 3, 0, 1, 90);
    check_faces(1, -1, 2, 0x7311, 1, 0x7311);
    check_faces(-4, 1, 1, 0x4567, 1, 0x1357);
    check_faces(0, 4, 2, 0x7123, 0, 0x7123);
    check_faces(0, -7, 1, 0, 0, 91);
    check_cells();
    check_overlapping_points();
    return 0;
}
