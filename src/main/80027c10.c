/* Owner re-derivation from retail assembly (clean input set). */
/* Guest pointers held as u32 for the 64-bit native lane. */
/* Walks a model's linked primitive-group list and emits each flat (0x30) or textured (0x38) triangle group into the output primitive buffer. */
#include "psx_types.h"

typedef struct {
    s32 vertices;
    s32 pad4;
    s32 normals;
    s32 padC;
    s32 output;
} MeshData;

typedef struct {
    u8 pad0[8];
    u32 mesh; /* guest MeshData * */
    u32 groups; /* guest u32 * */
} Model;

typedef struct {
    s32 pad0;
    s32 light;
} DrawContext;

extern void func_80027D20(s32, s32, s32, u32 *, u32, s32, s32);
extern void func_80027F4C(s32, s32, s32, u32 *, u32, s32, s32);

void func_80027C10(Model *model, DrawContext *ctx, s32 depth) {
    MeshData *mesh;
    u32 *group;
    s32 vertices;
    s32 normals;
    s32 output;
    u32 header;
    u32 count;
    u32 next;

    mesh = (MeshData *)model->mesh;
    group = (u32 *)model->groups;
    vertices = mesh->vertices;
    normals = mesh->normals;
    output = mesh->output;
    do {
        header = group[0];
        count = group[1];
        group += 2;
        switch ((header >> 24) & 0xFC) {
        case 0x30:
            func_80027D20(output, vertices, normals, group, count, depth, ctx->light);
            output += count * 0x14;
            break;
        case 0x38:
            func_80027F4C(output, vertices, normals, group, count, depth, ctx->light);
            output += count * 0x18;
            break;
        }
        next = header & 0xFFFFFF;
        group = (u32 *)(next | 0x80000000);
    } while (next != 0);
}
