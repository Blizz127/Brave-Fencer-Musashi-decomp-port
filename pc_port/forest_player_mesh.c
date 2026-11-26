#include "musashi_forest_player_mesh.h"
#include "forest_player_bytes.h"
#include "forest_player_archive.h"

#include <stdlib.h>
#include <string.h>

enum {
    TABLE_OFFSET = 0xf4ac,
    TABLE_BYTES = (MUSASHI_FOREST_PLAYER_PARTS + 1) * 4,
    HEADER_BYTES = 40
};

/* The stored model list uses addresses based at this archive payload origin.
 * Convert them to file offsets before reading any component. */
static const uint32_t payload_origin = UINT32_C(0x800cedf8);

struct MusashiForestPlayerMesh {
    unsigned count;
    MusashiForestPlayerPart parts[MUSASHI_FOREST_PLAYER_PARTS];
    MusashiForestPlayerVector *vectors;
    MusashiForestPlayerFace *faces;
};

typedef struct {
    const uint8_t *vertices, *normals, *primitives;
    size_t vertex_count, normal_count, face_count;
    uint32_t object_scale;
} SourcePart;

static int component_offsets(const uint8_t table[TABLE_BYTES], size_t offsets[MUSASHI_FOREST_PLAYER_PARTS+1]) {
    if (u32le(table + TABLE_BYTES - 4)) return 0;
    for (unsigned i = 0; i < MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        uint32_t address = u32le(table + 4*i);
        if (address < payload_origin || address - payload_origin >= TABLE_OFFSET) return 0;
        size_t offset = address - payload_origin;
        if (offset % 4 || (i && offset <= offsets[i-1])) return 0;
        offsets[i] = offset;
    }
    offsets[MUSASHI_FOREST_PLAYER_PARTS] = TABLE_OFFSET;
    return 1;
}

static int source_part(const uint8_t *p, size_t bytes, int scene_materials, SourcePart *out) {
    /* On disc each component is one unmapped object. Its three table offsets
     * are relative to the object directory, twelve bytes after the header. */
    if (bytes < HEADER_BYTES || u32le(p) != 0x41 || u32le(p+4) ||
        u32le(p+8) != 1) return 0;
    /* The near-bank actors use scale label -1 and the rifle round uses 3.
     * Both supply vertices already expressed in model units. */
    uint32_t scale = u32le(p+36);
    if (scale && !(scene_materials && (scale == UINT32_MAX || scale == 3))) return 0;
    size_t vertices = u32le(p+12), normals = u32le(p+20), primitives = u32le(p+28);
    size_t nv = u32le(p+16), nn = u32le(p+24), nf = u32le(p+32);
    size_t extent = bytes - 12;
    if (primitives != HEADER_BYTES - 12 || primitives >= vertices || vertices >= normals ||
        normals >= extent || !nv || !nn || !nf || nf > (vertices-primitives)/4 ||
        (normals-vertices)%8 || nv != (normals-vertices)/8 ||
        (extent-normals)%8 || nn != (extent-normals)/8) return 0;
    *out = (SourcePart){p+12+vertices, p+12+normals, p+12+primitives, nv, nn, nf, scale};
    return 1;
}

static void decode_vectors(const uint8_t *p, size_t count, MusashiForestPlayerVector *out) {
    for (size_t i = 0; i < count; ++i, p += 8)
        out[i] = (MusashiForestPlayerVector){s16le(p), s16le(p+2), s16le(p+4)};
}

static int decode_faces(const SourcePart *part, MusashiForestPlayerFace *out, int scene_materials) {
    const uint8_t *p = part->primitives;
    for (size_t i = 0; i < part->face_count; ++i) {
        size_t available = (size_t)(part->vertices - p);
        if (available < 4) return 0;
        unsigned mode = p[3], n = (mode & 8) ? 4 : 3;
        unsigned textured = mode & 4, gouraud = mode & 16;
        /* Unlit three-corner Gouraud records carry RGB at each corner and
         * three packed vertex indices. They have no normal indices. */
        if (part->object_scale == 3) {
            if (available < 24 || p[0] != 6 || p[1] != 5 || p[2] != 1 || mode != 0x31) return 0;
            MusashiForestPlayerFace face = {.mode = (uint8_t)mode, .corners = 3};
            for (unsigned k = 0; k < 3; ++k) {
                memcpy(face.corner_rgb[k], p + 4 + k*4, 3);
                face.vertex[k] = u16le(p + 16 + k*2);
                if (face.vertex[k] >= part->vertex_count) return 0;
            }
            memcpy(face.rgb, face.corner_rgb[0], 3);
            out[i] = face;
            p += 24;
            continue;
        }
        int vertex_inks = !!(p[2] & 4);
        unsigned prefix = (textured || vertex_inks) ? n : 1;
        /* The raw textured scene quad carries a flat RGB word after its
         * four UVs, then four vertex indices without a lighting normal. */
        int raw_quad = scene_materials && mode == 0x2d && p[2] == 1;
        unsigned input = raw_quad ? 7 : prefix + (gouraud ? n : (n+2)/2);
        unsigned output = 1 + n + (textured ? n : 0) + (gouraud || vertex_inks ? n-1 : 0);
        size_t length = (size_t)(input+1)*4;
        unsigned allowed_bits = scene_materials ? 0x1e : 0x1c;
        if ((!raw_quad && (mode & ~allowed_bits) != 0x20) || (!scene_materials && mode == 0x24) ||
            (p[2] && !raw_quad && (!scene_materials || p[2] != 4 || textured ||
                      (!gouraud && part->object_scale != UINT32_MAX && !(mode & 2)))) ||
            p[0] != output || p[1] != input || length > available) return 0;
        MusashiForestPlayerFace face = {0};
        /* Distinct vertex inks require Gouraud output even when all the
         * corners share one normal in the input record. */
        face.mode = (uint8_t)(vertex_inks ? mode | 16u : mode);
        face.corners = (uint8_t)n;
        const uint8_t *body = p+4, *indices = body+4*prefix;
        if (textured) {
            memset(face.rgb, 128, sizeof(face.rgb));
            face.clut = u16le(body+2);
            face.tpage = u16le(body+6);
        } else {
            memcpy(face.rgb, body, sizeof(face.rgb));
        }
        for (unsigned k = 0; k < n; ++k) {
            if (textured) {
                face.u[k] = body[k*4];
                face.v[k] = body[k*4+1];
            }
            memcpy(face.corner_rgb[k], vertex_inks ? body + 4*k : face.rgb, 3);
            face.vertex[k] = raw_quad ? u16le(indices + 4 + 2*k) :
                u16le(indices + (gouraud ? 4*k+2 : 2+2*k));
            face.normal[k] = raw_quad ? 0 : u16le(indices + (gouraud ? 4*k : 0));
            if (face.vertex[k] >= part->vertex_count || face.normal[k] >= part->normal_count)
                return 0;
        }
        out[i] = face;
        p += length;
    }
    return p == part->vertices;
}

static MusashiForestPlayerMesh *decode_mesh(const uint8_t *block, const size_t *offsets,
                                           unsigned count, int scene_materials) {
    SourcePart source[MUSASHI_FOREST_PLAYER_PARTS];
    size_t vectors = 0, faces = 0;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *p = block + (offsets[i]-offsets[0]);
        if (!source_part(p, offsets[i+1]-offsets[i], scene_materials, source+i)) return NULL;
        vectors += source[i].vertex_count + source[i].normal_count;
        faces += source[i].face_count;
    }
    /* The component spans are disjoint. Validate the decoded array sizes. */
    if (vectors > SIZE_MAX / sizeof(MusashiForestPlayerVector) ||
        faces > SIZE_MAX / sizeof(MusashiForestPlayerFace)) return NULL;
    MusashiForestPlayerMesh *mesh = calloc(1, sizeof(*mesh));
    if (!mesh) return NULL;
    mesh->count = count;
    mesh->vectors = malloc(vectors * sizeof(*mesh->vectors));
    mesh->faces = malloc(faces * sizeof(*mesh->faces));
    if (!mesh->vectors || !mesh->faces) {
        musashi_forest_player_mesh_free(mesh);
        return NULL;
    }
    MusashiForestPlayerVector *v = mesh->vectors;
    MusashiForestPlayerFace *f = mesh->faces;
    for (unsigned i = 0; i < count; ++i) {
        const SourcePart *s = source+i;
        decode_vectors(s->vertices, s->vertex_count, v);
        decode_vectors(s->normals, s->normal_count, v+s->vertex_count);
        if (!decode_faces(s, f, scene_materials)) {
            musashi_forest_player_mesh_free(mesh);
            return NULL;
        }
        mesh->parts[i] = (MusashiForestPlayerPart){s->vertex_count, s->normal_count,
            s->face_count, v, v+s->vertex_count, f};
        v += s->vertex_count + s->normal_count;
        f += s->face_count;
    }
    return mesh;
}

MusashiForestPlayerMesh *musashi_forest_player_mesh_load(const MusashiDiscMedia *media) {
    uint8_t table[TABLE_BYTES];
    size_t offsets[MUSASHI_FOREST_PLAYER_PARTS+1];
    ForestPlayerArchive archive;
    if (!forest_player_archive_open(media, &archive) ||
        !forest_player_archive_read(&archive, payload_origin+TABLE_OFFSET, table, sizeof(table)) ||
        !component_offsets(table, offsets)) return NULL;
    size_t bytes = TABLE_OFFSET - offsets[0];
    uint8_t *block = malloc(bytes);
    if (!block) return NULL;
    MusashiForestPlayerMesh *mesh = NULL;
    if (forest_player_archive_read(&archive, payload_origin+(uint32_t)offsets[0], block, bytes)) mesh = decode_mesh(block, offsets, MUSASHI_FOREST_PLAYER_PARTS, 0);
    free(block);
    return mesh;
}

MusashiForestPlayerMesh *musashi_forest_player_mesh_from_table(
    const uint8_t *scene, size_t bytes, uint32_t origin, uint32_t table,
    unsigned *part_count)
{
    if (!scene || !part_count || table < origin || bytes > UINT32_MAX) return NULL;
    *part_count = 0;
    size_t offsets[MUSASHI_FOREST_PLAYER_PARTS+1];
    size_t start = table-origin;
    if (start > bytes) return NULL;
    for (unsigned i = 0; i <= MUSASHI_FOREST_PLAYER_PARTS; ++i) {
        if ((bytes-start)/4 <= i) return NULL;
        uint32_t address = u32le(scene + start + 4*i);
        if (!address && i) {
            offsets[i] = start;
            MusashiForestPlayerMesh *mesh = decode_mesh(scene+offsets[0], offsets, i, 1);
            if (mesh) *part_count = i;
            return mesh;
        }
        if (address < origin || address >= table || (address & 3) ||
            (i && address-origin <= offsets[i-1])) return NULL;
        offsets[i] = address-origin;
    }
    return NULL;
}

const MusashiForestPlayerPart *musashi_forest_player_mesh_part(
    const MusashiForestPlayerMesh *mesh, unsigned index) {
    if (!mesh || index >= mesh->count) return NULL;
    return mesh->parts+index;
}

void musashi_forest_player_mesh_free(MusashiForestPlayerMesh *mesh) {
    if (mesh) { free(mesh->faces); free(mesh->vectors); free(mesh); }
}
