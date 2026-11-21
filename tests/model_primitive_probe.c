#include <assert.h>
#include <stdint.h>
#include "model_draw.h"

typedef union {
    u32 word;
    u16 count;
} Header;

typedef struct {
    u32 vertices, vertex_count, normals, normal_count, groups, primitive_count;
} Part;

typedef enum { FLAT_TRI, FLAT_QUAD, LIT_TRI, LIT_QUAD, TEX_TRI, TEX_QUAD, SKIP } Writer;

u32 D_800A5E60;
static Header groups[40];
static Part part;
static OtRec ot;
static Header *last;
static Writer expected;
static u32 cursor, ordering, submitted;
static s32 expected_mode;

static u32 submit(Writer writer, Header *group, u32 vertices, u32 normals, u32 old_cursor,
                  u32 count, s32 mode, u32 ordering_table)
{
    assert(mode == expected_mode && count == 1);
    assert(vertices == part.vertices && normals == part.normals);
    assert(old_cursor == cursor && ordering_table == ordering);
    if (group == last) {
        assert(writer == TEX_TRI && submitted == (expected != SKIP));
    } else {
        assert(group == groups && writer == expected && submitted == 0);
        group->count = 2;
    }
    ++submitted;
    ordering += vertices;
    ot.unk4 = ordering;
    cursor = (cursor ^ normals) + vertices;
    return cursor;
}

#define DEFINE_WRITER(name, id) \
    u32 name(Header *g, u32 v, u32 n, u32 p, u32 c, s32 m, u32 o) \
    { return submit(id, g, v, n, p, c, m, o); }

DEFINE_WRITER(func_80027058, FLAT_TRI)
DEFINE_WRITER(func_80027200, FLAT_QUAD)
DEFINE_WRITER(func_800273F4, LIT_TRI)
DEFINE_WRITER(func_800275BC, LIT_QUAD)
DEFINE_WRITER(func_800277DC, TEX_TRI)
DEFINE_WRITER(func_800279AC, TEX_QUAD)

int main(void)
{
    if ((uintptr_t)&part > UINT32_MAX || (uintptr_t)groups > UINT32_MAX)
        return 77;
    DrawArg draw = {0};
    draw.unk8 = (u32)(uintptr_t)&part;
    part.groups = (u32)(uintptr_t)groups;
    u32 state = 29;
    for (unsigned i = 0; i < 2048; ++i) {
        const unsigned quad = i & 1u;
        const unsigned textured = (i >> 1) & 1u;
        const unsigned smooth = (i >> 2) & 1u;
        const unsigned raw = textured && !smooth && ((i >> 3) & 1u);
        const unsigned lit = (i >> 4) & 1u;
        unsigned tag = 32u | (quad << 3) | (textured << 2) | (smooth << 4) | raw;
        tag |= ((i >> 5) & 1u) << 1;
        unsigned words = 4u + quad + smooth;
        expected = SKIP;
        if (textured) {
            words += 2u + quad;
            if (raw) {
                words += !quad;
                expected = quad ? TEX_QUAD : TEX_TRI;
            }
        } else if (!smooth) {
            if (lit) {
                words += 2u + quad;
                expected = quad ? LIT_QUAD : LIT_TRI;
            } else {
                expected = quad ? FLAT_QUAD : FLAT_TRI;
            }
        }
        groups[0].word = (tag << 24) | (lit << 18) | 1u;
        const unsigned consumed = expected == SKIP ? 1u : 2u;
        last = groups + words * consumed;
        last->word = (37u << 24) | 1u;
        part.primitive_count = consumed + 1;
        state = state * 1664525u + 1013904223u;
        part.vertices = state;
        part.normals = ~state;
        expected_mode = i & 1 ? -(s32)i : (s32)i;
        D_800A5E60 = cursor = state + i;
        ot.unk4 = ordering = state ^ (state >> 7);
        submitted = 0;
        func_80026D64(&draw, &ot, expected_mode);
        assert(submitted == 1u + (expected != SKIP) && D_800A5E60 == cursor);
    }
    part.primitive_count = 0;
    part.groups = 0;
    submitted = 0;
    func_80026D64(&draw, &ot, 0);
    assert(!submitted);
    return 0;
}
