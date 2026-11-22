#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "model_surface_probe.h"

SceneSurfaceBox D_8019F9EC;
u8 D_8019F9CC[32];
static SceneSurfaceRecord record;
static FloorCellMap grid;
static u32 grid_address;
static size_t grid_bytes;
static unsigned grid_resolutions;
static SceneSurfaceBox chain[8];
static u32 record_address, addresses[8], tag;
static unsigned resolved, hits, classified, tests, maps, transforms;
static unsigned mode, depth, wanted, grid_result;
static SceneProbeVector point, initial_point;

_Static_assert(sizeof(SceneSurfaceBox) == 16, "32-bit next reference and six signed bounds");
_Static_assert(offsetof(SceneSurfaceRecord, matrix) == 52, "model matrix field");
_Static_assert(sizeof(SceneProbeVector) == 8, "probe vector");

SceneSurfaceRecord *bfm_model_probe_record(u32 address)
{
    ++resolved;
    return address == record_address ? &record : 0;
}
SceneSurfaceBox *bfm_model_probe_box(u32 address)
{
    ++hits;
    for (unsigned i = 0; i < 8; ++i)
        if (address == addresses[i])
            return &chain[i];
    return 0;
}
s32 func_80135480(SceneSurfaceRecord *source, u32 geometry,
                 SceneProbeVector *from, SceneProbeVector *far)
{
    assert(classified++ == 0 && source == &record && geometry == tag && from == &point);
    assert(far->x == point.x && far->z == point.z && far->y == (s16)((u16)point.y + 768u));
    return (s32)mode;
}
s32 func_80135EB0(SceneSurfaceBox *box, s32 flag)
{
    assert((s32)tag < 0 && flag == 0 && classified == 1 && mode >= 1 && mode <= 4);
    SceneSurfaceBox *expected = tests == 0 && mode >= 3 ? &D_8019F9EC : &chain[tests];
    assert(tests < 8 && box == expected);
    ++tests;
    return tests == wanted + 1u;
}
FloorCellMap *bfm_model_probe_grid(u32 address)
{
    assert(grid_resolutions++ == 0 && address == tag && (s32)address >= 0);
    return address != 0 && address == grid_address && grid_bytes >= sizeof(grid) ? &grid : NULL;
}
s32 func_801345F8(FloorCellMap *map)
{
    assert(maps++ == 0 && map == &grid && classified == 1 && hits == 0);
    return (s32)grid_result;
}
void func_80136A94(s32 rotate, SceneSurfaceRecord *source, SceneProbeVector *out, void *matrix)
{
    assert(transforms++ == 0 && source == &record && out == &point);
    assert(rotate == (mode == 2 || mode == 4));
    assert(matrix == (mode == 4 ? D_8019F9CC : record.matrix));
    out->x = (s16)(out->x ^ 0x453);
    out->y = (s16)(out->y ^ 0x152);
}
int main(void)
{
    record_address = 0x81357024u;
    u32 seed = 701u;
    for (unsigned i = 0; i < 8192; ++i) {
        seed = seed * 1664525u + 1013904223u;
        u32 first = 0x80250200u + ((seed >> 5) & 0x6fc0u);
        for (unsigned j = 0; j < 8; ++j) addresses[j] = first + 32u*j;
        int tagged = (i & 1) != 0;
        tag = tagged ? (first | ((seed & 7u) << 28)) : ((seed >> 1) & 0x7fffffffu);
        if (!tagged && i % 67 == 0)
            tag = 0;
        grid_address = i % 17 == 0 ? tag ^ 1u : tag;
        grid_bytes = i % 13 == 0 ? sizeof(grid) - 1 : sizeof(grid);
        unsigned grid_available = tag != 0 && tag == grid_address && grid_bytes == sizeof(grid);
        mode = i >> 1 & 7;
        depth = (i >> 4 & 7) + 1;
        wanted = i >> 7 & 15;
        grid_result = i >> 3 & 1;
        unsigned unknown_record = i % 37 == 0;
        unsigned invalid_head = i % 43 == 0;
        unsigned invalid_next = i % 53 == 0;
        if (tagged && invalid_head)
            tag ^= 0x00001000u;
        for (unsigned j = 0; j < 8; ++j)
            chain[j].next = j+1 < depth ? addresses[j+1] : invalid_next ? 0x83fffed0u : 0;
        D_8019F9EC.next = chain[0].next;
        point = (SceneProbeVector){(s16)seed,(s16)(seed >> 16),(s16)(seed >> 9),(s16)i};
        initial_point = point;
        hits=resolved=classified=tests=maps=transforms=grid_resolutions=0;
        unsigned acceptable = !unknown_record && mode >= 1 && mode <= 4;
        unsigned expected_ok = acceptable && (tagged ? (mode >= 3 || !invalid_head) && wanted < depth : grid_available && grid_result);
        int result = func_80135D20(unknown_record ? record_address + 4 : record_address,tag,&point);
        assert((unsigned)result == expected_ok && transforms == expected_ok);
        assert(resolved == 1 && classified == !unknown_record);
        assert(grid_resolutions == (acceptable && !tagged));
        assert(maps == (acceptable && !tagged && grid_available));
        unsigned expected_boxes = acceptable && tagged && (mode >= 3 || !invalid_head)
            ? (wanted < depth ? wanted+1 : depth) : 0;
        assert(tests == expected_boxes);
        if (!result) assert(memcmp(&point,&initial_point,sizeof point)==0);
        else assert(point.x == (s16)(initial_point.x ^ 0x453) && point.y == (s16)(initial_point.y ^ 0x152));
        assert(point.z == initial_point.z && point.pad == initial_point.pad);
    }
    return 0;
}
