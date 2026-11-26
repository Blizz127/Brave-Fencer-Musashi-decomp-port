/* SPDX-License-Identifier: MIT */
#include "steam_knight_roof_cells.h"

#include <stdlib.h>
#include <string.h>

enum { WIDTH = 64, CELLS = WIDTH * WIDTH, DIRECTORY = CELLS * 4, ROOF_CELLS = 4, CRATER_CELLS = 12 };

typedef struct {
    const uint8_t *data;
    size_t size;
} Span;

typedef struct {
    size_t index;
    Span cell;
} RoofCell;

static unsigned read16(const uint8_t *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static uint32_t read32(const uint8_t *p)
{
    return read16(p) | ((uint32_t)read16(p + 2) << 16);
}

static void write32(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (8 * i));
}

static int read_cells(const uint8_t *record, size_t bytes, unsigned count,
                      RoofCell roof[CRATER_CELLS], Span *materials)
{
    enum { HEADER = 16, ENTRY = 12 };
    size_t index_bytes = count * ENTRY;
    if ((count != 2 && count != ROOF_CELLS && count != CRATER_CELLS) ||
        bytes < HEADER + index_bytes || read32(record) != 0x43 ||
        read32(record + 4) != 0 || read32(record + 8) != count) return 0;
    const uint8_t *body = record + HEADER;
    size_t start = read32(record + 12);
    if (start < index_bytes || start > bytes - HEADER) return 0;
    *materials = (Span){body + start, bytes - HEADER - start};
    if (!materials->size || materials->size % 16) return 0;

    size_t end = start;
    for (unsigned i = count; i-- > 0;) {
        const uint8_t *entry = body + i * ENTRY;
        size_t slot = read32(entry), offset = read32(entry + 4);
        if (slot >= DIRECTORY || slot % 4 || read32(entry + 8) != 0 ||
            offset < index_bytes || offset % 4 || offset >= end) return 0;
        roof[i] = (RoofCell){slot / 4, {body + offset, end - offset}};
        end = offset;
    }
    if (end != index_bytes) return 0;
    int next_column = roof[0].index % WIDTH != WIDTH - 1 &&
                      roof[1].index == roof[0].index + 1;
    if (count == 2) return next_column || roof[1].index == roof[0].index + WIDTH;
    unsigned columns = count == CRATER_CELLS ? 3 : 2;
    size_t first = roof[0].index;
    if (first % WIDTH + columns > WIDTH) return 0;
    for (unsigned i = 1; i < count; ++i)
        if (roof[i].index != first + i / columns * WIDTH + i % columns) return 0;
    return 1;
}

/* A replacement face can select a material in its own appended packet table.
 * Original-room references retain their offsets. Vertex offsets are local to
 * each cell in both formats. A null output validates without copying. */
static int copy_cell(Span cell, Span base_materials, Span local_materials, uint8_t *out)
{
    if (cell.size < 28) return 0;
    size_t vertices = read32(cell.data), count = read32(cell.data + 4);
    size_t faces = read32(cell.data + 20), face_count = read32(cell.data + 24);
    if (vertices < 28 || vertices % 4 || faces % 4 || faces < vertices ||
        faces > cell.size || count > (faces - vertices) / 8 ||
        face_count > (cell.size - faces) / 12) return 0;
    for (unsigned axis = 0; axis < 3; ++axis) {
        const uint8_t *bound = cell.data + 8 + 4 * axis;
        if ((int16_t)read16(bound) > (int16_t)read16(bound + 2)) return 0;
    }
    if (out) memcpy(out, cell.data, cell.size);
    for (size_t i = 0; i < face_count; ++i) {
        size_t pos = faces + 12 * i;
        const uint8_t *face = cell.data + pos;
        uint32_t ref = read32(face);
        size_t offset = ref & 0x7fffff;
        int local = (ref & 0x800000) != 0;
        Span table = local ? local_materials : base_materials;
        if (offset % 16 || table.size < 16 || offset > table.size - 16) return 0;
        const uint8_t *mat = table.data + offset;
        unsigned last = read16(face + 10), shape = last & 14;
        if ((shape != 2 && shape != 6) || mat[3] != (ref >> 24) ||
            (mat[3] & ~2u) != (shape == 2 ? 0x2cu : 0x24u) ||
            ((read16(mat + 10) >> 7) & 3u) > 2) return 0;
        for (unsigned j = 0; j < 3; ++j) {
            unsigned vertex = read16(face + 4 + 2 * j);
            if (vertex % 8 || vertex / 8 >= count) return 0;
        }
        if (shape == 2 && (last >> 4) >= count) return 0;
        if (local && out)
            write32(out + pos, (ref & UINT32_C(0xff000000)) |
                               (uint32_t)(base_materials.size + offset));
    }
    return 1;
}

static uint8_t *replace_cells(const uint8_t *base_mesh, size_t base_size,
                              const uint8_t *replacement, size_t replacement_size,
                              unsigned count, size_t *out_size)
{
    if (!out_size) return NULL;
    *out_size = 0;
    if (!base_mesh || !replacement || base_size < 8 + DIRECTORY ||
        base_size > UINT32_MAX || replacement_size > UINT32_MAX - base_size ||
        read32(base_mesh) != 0x42 || base_mesh[4] != 0) return NULL;
    RoofCell roof[CRATER_CELLS];
    Span local;
    if (!read_cells(replacement, replacement_size, count, roof, &local)) return NULL;

    const uint8_t *table = base_mesh + 8;
    unsigned old_count = read16(base_mesh + 6);
    if (old_count * 16u > base_size - 8 - DIRECTORY ||
        local.size / 16 > UINT16_MAX - old_count) return NULL;
    Span materials = {table + DIRECTORY, old_count * 16u};
    size_t prefix = DIRECTORY + materials.size;
    unsigned merged_count = old_count + (unsigned)(local.size / 16);
    size_t capacity = base_size + replacement_size;
    uint8_t *out = malloc(capacity);
    if (!out) return NULL;
    memcpy(out, base_mesh, 8);
    out[6] = (uint8_t)merged_count;
    out[7] = (uint8_t)(merged_count >> 8);
    memset(out + 8, 0, DIRECTORY);
    memcpy(out + 8 + DIRECTORY, materials.data, materials.size);
    memcpy(out + 8 + prefix, local.data, local.size);
    size_t cursor = 8 + prefix + local.size;
    size_t previous = 0, previous_index = 0, applied = 0;

    /* Each populated slot ends at the next populated slot. The final sentinel
     * supplies the end of the bounded base mesh. */
    for (size_t i = 0; i <= CELLS; ++i) {
        size_t offset = i == CELLS ? base_size - 8 : read32(table + 4 * i);
        if (!offset) continue;
        if (offset < prefix || offset > base_size - 8 || offset % 4 ||
            (previous && offset <= previous)) goto reject;
        if (previous) {
            Span source = {table + previous, offset - previous};
            Span extra = {NULL, 0};
            if (applied < count && roof[applied].index == previous_index) {
                Span changed = roof[applied++].cell;
                if (!copy_cell(source, materials, extra, NULL) || changed.size < 28 ||
                    read32(source.data + 8) != read32(changed.data + 8) ||
                    read32(source.data + 16) != read32(changed.data + 16)) goto reject;
                source = changed;
                extra = local;
            }
            if (source.size > capacity - cursor ||
                !copy_cell(source, materials, extra, out + cursor)) goto reject;
            write32(out + 8 + 4 * previous_index, (uint32_t)(cursor - 8));
            cursor += source.size;
        }
        previous = offset;
        previous_index = i;
    }
    if (applied != count) goto reject;
    *out_size = cursor;
    return out;

reject:
    free(out);
    return NULL;
}

uint8_t *bfm_steam_knight_roof_mesh(const uint8_t *mesh, size_t bytes,
                                    const uint8_t *record, size_t size, size_t *out_size)
{
    return replace_cells(mesh, bytes, record, size, ROOF_CELLS, out_size);
}

uint8_t *bfm_steam_knight_gate_mesh(const uint8_t *mesh, size_t bytes,
                                    const uint8_t *record, size_t size, size_t *out_size)
{
    return replace_cells(mesh, bytes, record, size, 2, out_size);
}

uint8_t *bfm_steam_knight_house_mesh(const uint8_t *mesh, size_t bytes,
                                     const uint8_t *record, size_t size,
                                     unsigned house, size_t *out_size)
{
    if (!out_size) return NULL;
    *out_size = 0;
    static const unsigned first_sectors[] = {2270, 2336, 2398, 2398};
    if (!record || size < 32 || !house || house > sizeof first_sectors / sizeof first_sectors[0])
        return NULL;
    /* Both west-street states use the same four sectors. Their packet bounds
     * distinguish the landing from the later rolling impact. */
    if ((house == 3 && size != 2652) || (house == 4 && size != 2372))
        return NULL;
    unsigned first_cell = first_sectors[house - 1];
    if (read32(record + 16) != first_cell * 4 ||
        read32(record + 28) != (first_cell + 1) * 4) return NULL;
    return replace_cells(mesh, bytes, record, size, house == 2 ? 2 : 4, out_size);
}

uint8_t *bfm_steam_knight_crater_mesh(const uint8_t *mesh, size_t bytes,
                                      const uint8_t *record, size_t size, size_t *out_size)
{
    return replace_cells(mesh, bytes, record, size, CRATER_CELLS, out_size);
}
